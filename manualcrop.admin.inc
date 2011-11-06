<?php
/**
 * @file
 * Admin functionality for the Manual Crop module.
 */

/**
 * Form Builder; Configuration settings for Manual Crop and scale effect.
 *
 * @param $data
 *   The current configuration for this Manual Crop and scale effect.
 */
function manualcrop_crop_and_scale_form($data) {
  $form = image_scale_form($data);

  $form['width']['#required'] = TRUE;
  $form['height']['#required'] = TRUE;

  $form['respectminimum'] = array(
    '#type' => 'checkbox',
    '#title' => t('Respect minimum'),
    '#description' => t("Make sure the selected crop area is at least as big as the destination size. This doesn't enforce minimum image dimensions."),
    '#default_value' => (isset($data['respectminimum']) ? TRUE : FALSE),
  );

  return $form;
}

/**
 * Returns HTML for a summary of an image Manual Crop and scale effect.
 *
 * @param $variables
 *   An associative array containing:
 *   - data: The current configuration for this Manual Crop and scale effect.
 *
 * @ingroup themeable
 */
function theme_manualcrop_crop_and_scale_summary($variables) {
  $data = $variables["data"];
  $str = check_plain($data['width']) . 'x' . check_plain($data['height']);

  if ($data['upscale'] || $data['respectminimum']) {
    $str .= ' (' . ($data['upscale'] ? t('upscaling allowed') : '');
    $str .= ($data['upscale'] && $data['respectminimum'] ? ' ' . t('and') . ' ' : '');
    $str .= ($data['respectminimum'] ? t('crop minimum dimensions') : '') . ')';
  }

  return $str;
}

/**
 * Form Builder; Configuration settings for Manual Crop effect.
 *
 * @param $data
 *   The current configuration for this Manual Crop effect.
 */
function manualcrop_crop_form($data) {
  $form = image_resize_form($data);

  $form['width']['#title'] = t('Minimum crop width');
  $form['width']['#description'] = t("Only applied if a user tries to crop, this enforces no minimum image width!");
  $form['width']['#required'] = FALSE;

  $form['height']['#title'] = t('Minimum crop height');
  $form['height']['#description'] = t("Only applied if a user tries to crop, this enforces no minimum image height!");
  $form['height']['#required'] = FALSE;

  return $form;
}

/**
 * Returns HTML for a summary of an image resize effect.
 *
 * @param $variables
 *   An associative array containing:
 *   - data: The current configuration for this resize effect.
 *
 * @ingroup themeable
 */
function theme_manualcrop_crop_summary($variables) {
  $data = $variables['data'];

  if ($data['width'] && $data['height']) {
    return ' (' . t('minimum crop dimensions ') . ' ' . check_plain($data['width']) . 'x' . check_plain($data['height']) . ')';
  }
  elseif ($data['width'] || $data['height']) {
    return ' (' . ($data['width']) ? t('minimum crop width @width', array('@width' => $data['width'])) : t('minimum crop height @height', array('@height' => $data['height'])) . ')';
  }
}

/**
 * Implements hook_form_FORM_ID_alter().
 */
function manualcrop_form_image_effect_form_alter(&$form, &$form_state) {
  if (!is_array($form['#submit'])) {
    $form['#submit'] = array('image_effect_form_submit');
  }

  // If it's a Manual Crop effect do a custom submit handler first.
  // Otherwise reorden the effects after the submit.
  if ($form_state['image_effect']['module'] == 'manualcrop') {
    array_unshift($form['#submit'], 'manualcrop_image_effect_form_submit');
  }
  elseif (!empty($form_state['image_style']['effects'])) {
    $form['#submit'][] = 'manualcrop_force_effect_order';
  }
}

/**
 * Effect form submit handler; Sets the image style name and weight of the effect.
 *
 * @param $form
 *   Effect form array.
 * @param $form_state
 *   Form state array.
 */
function manualcrop_image_effect_form_submit($form, &$form_state) {
  $style = $form_state['image_style'];
  $effect = $form_state['image_effect'];

  // Set style name so the effect can use it.
  $form_state['values']['data']['style_name'] = $style['name'];

  if (count($style['effects']) == 0) {
    // First effect, so weight is 0.
    $form_state['values']['weight'] = 0;
  }
  elseif (!isset($effect['ieid'])) {
    // New effect, weight must be the weight of the first one minus 1.
    $first = reset($style['effects']);
    $form_state['values']['weight'] = $first["weight"] - 1;
  }
}

/**
 * Implements hook_form_FORM_ID_alter().
 */
function manualcrop_form_image_effect_delete_form_alter(&$form, &$form_state) {
  if (!is_array($form['#submit'])) {
    $form['#submit'] = array('image_effect_delete_form_submit');
  }

  // If it's a Manual Crop effect do a custom submit handler to cleanup the cropped areas.
  if ($form_state['image_effect']['module'] == 'manualcrop') {
    array_unshift($form['#submit'], 'manualcrop_image_effect_delete_form_submit');
  }
}

/**
 * Effect deletion form submit handler; Cleanup old selection areas.
 *
 * @param $form
 *   Effect form array.
 * @param $form_state
 *   Form state array.
 */
function manualcrop_image_effect_delete_form_submit($form, &$form_state) {
  manualcrop_image_style_delete($form_state['image_style']);
}

/**
 * Implements hook_form_FORM_ID_alter().
 */
function manualcrop_form_image_style_form_alter(&$form, &$form_state) {
  $style = $form_state['image_style'];

  if (!empty($style['effects'])) {
    if (!is_array($form['#submit'])) {
      $form['#submit'] = array('image_style_form_submit');
    }

    // Reorder effects on submit.
    $form['#submit'][] = 'manualcrop_force_effect_order';

    // Update style name in effect data.
    $form['#submit'][] = 'manualcrop_update_style_name';

    // Filter all Manual Crop effects out of the form if there's already a Manual Crop effect.
    if (isset($form['effects']['new']['new'])) {
      foreach ($style['effects'] as $effect) {
        if ($effect['module'] == 'manualcrop') {
          $form['effects']['new']['new']['#options'] = array_diff_key($form['effects']['new']['new']['#options'], manualcrop_image_effect_info());
          break;
        }
      }
    }
  }
}

/**
 * Effect and style form submit handler; Force the Manual Crop effect to be the first one.
 *
 * @param $form
 *   Effect or style form array.
 * @param $form_state
 *   Form state array.
 */
function manualcrop_force_effect_order($form, &$form_state) {
  // In both forms (style and effect) we find the style in the same key.
  $style = $form_state['image_style'];

  // If it's the style form, check if the style name was updated.
  if ($form['#form_id'] == 'image_style_form' && isset($form_state['values']['name']) && $style['name'] != $form_state['values']['name']) {
    $style['name'] = $form_state['values']['name'];
  }

  // The style array is received trough the loader to make sure the function
  // works with the latest version.
  $style = image_style_load($style['name']);

  if (!empty($style['effects'])) {
    foreach ($style['effects'] as $eid => $effect) {
      if ($effect['module'] == 'manualcrop') {
        $first = reset($style['effects']);

        if ($eid != key($style['effects'])) {
          // Change the weight of this effect to the weight of the first effect minus 1.
          $effect['weight'] = $first['weight'] - 1;
          image_effect_save($effect);
        }

        break;
      }
    }
  }
}

/**
 * Style form submit handler; Update the style name in the effect data and in the widgets.
 *
 * @param $form
 *   Style form array.
 * @param $form_state
 *   Form state array.
 */
function manualcrop_update_style_name($form, &$form_state) {
  $style = $form_state['image_style'];

  // Check if the style name should be updated.
  if (!empty($style['effects']) && isset($form_state['values']['name']) && $style['name'] != $form_state['values']['name']) {
    $effect = reset($style['effects']);

    if ($effect['module'] == 'manualcrop') {
      // Change the style name in the effect data and save the effect.
      $effect["data"]["style_name"] = $form_state['values']['name'];
      image_effect_save($effect);

      _manualcrop_field_widget_update_names_in_settings($style['name'], $form_state['values']['name']);

      db_update('manualcrop')
        ->fields(array('style_name' => $form_state['values']['name']))
        ->condition('style_name', $style['name'])
        ->execute();
    }
  }
}

/**
 * Implements hook_field_widget_settings_form().
 */
function manualcrop_field_widget_settings_form($field, $instance) {
  $settings = $instance['widget']['settings'];

  // Use the image field settings form.
  $form = image_field_widget_settings_form($field, $instance);

  $form['inline_crop'] = array(
    '#type' => 'checkbox',
    '#title' => t('Inline cropping'),
    '#description' => t('Instead of opening an overlay, use inline cropping.'),
    '#default_value' => (isset($settings['inline_crop']) ? $settings['inline_crop'] : FALSE),
    '#weight' => 16,
  );

  $form['thumblist'] = array(
    '#type' => 'checkbox',
    '#title' => t('List thumbs'),
    '#description' => t('Instead showing a button or selection list, show all thumbails (this will disable the preview thumbnail).'),
    '#default_value' => (isset($settings['thumblist']) ? $settings['thumblist'] : FALSE),
    '#weight' => 17,
  );

  $form['styles_mode'] = array(
    '#type' => 'radios',
    '#title' => t('Styles list mode'),
    '#options' => array(
      'exclude' => t('Exclude selected styles'),
      'include' => t('Include selected styles'),
    ),
    '#default_value' => (isset($settings['styles_mode']) ? $settings['styles_mode'] : 'exclude'),
    '#weight' => 18,
  );

  $form['styles_list'] = array(
    '#type' => 'select',
    '#title' => t('Styles list'),
    '#description' => t('Select all styles that should be shown or hidden (as selected above) in the widget. Please note that hiding styles will override requiring them and existing cropping selections will be kept.'),
    '#options' => drupal_map_assoc(array_keys(manualcrop_get_styles_with_crop())),
    '#default_value' => (isset($settings['styles_list']) ? $settings['styles_list'] : array()),
    '#multiple' => TRUE,
    '#weight' => 19,
  );

  $form['require_cropping'] = array(
    '#type' => 'select',
    '#title' => t('Required croppings'),
    '#description' => t('Select all styles that must have a crop selection.'),
    '#options' => drupal_map_assoc(array_keys(manualcrop_get_styles_with_crop())),
    '#default_value' => (isset($settings['require_cropping']) ? $settings['require_cropping'] : array()),
    '#multiple' => TRUE,
    '#weight' => 20,
  );

  return $form;
}

/**
 * Implements hook_form_FORM_ID_alter().
 */
function manualcrop_form_system_performance_settings_alter(&$form, &$form_state) {
  $form['manualcrop'] = array(
    '#type' => 'fieldset',
    '#title' => t('Manual Crop'),
    '#collapsible' => FALSE,
    '#tree' => TRUE,
  );

  $form['manualcrop']['cache_control'] = array(
    '#type' => 'checkbox',
    '#title' => t('Reload cache-control'),
    '#description' => t('When enabled, a cache-controlling variable will be added to the cropped image path. This variable will change as soon as you re-crop the image, this way the visitor his/her browser will reload the image.'),
    '#default_value' => variable_get('manualcrop_cache_control', TRUE),
  );

  $form['#submit'][] = 'manualcrop_form_system_performance_settings_submit';

  return $form;
}

/**
 * Performance form submit handler; Update the cache-control settings.
 *
 * @param $form
 *   Effect form array.
 * @param $form_state
 *   Form state array.
 */
function manualcrop_form_system_performance_settings_submit($form, &$form_state) {
  $cache_control = (bool) $form_state['values']['manualcrop']['cache_control'];

  variable_set('manualcrop_cache_control', $cache_control);

  if (!$cache_control) {
    // Clear the cached image paths.
    cache_clear_all('manualcrop', 'cache', TRUE);
  }
}