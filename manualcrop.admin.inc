<?php
/**
 * @file
 * Admin functionality for the Manual Crop module.
 */

/**
 * Form Builder; Configuration settings for the crop and scale effect.
 *
 * @param $data
 *    The current configuration for this crop and scale effect.
 * @return
 *    The form structure array.
 */
function manualcrop_crop_and_scale_form($data) {
  $form = image_scale_form($data);

  $form['width']['#required'] = TRUE;
  $form['height']['#required'] = TRUE;

  $form['respectminimum'] = array(
    '#type' => 'checkbox',
    '#title' => t('Respect minimum'),
    '#description' => t("Make sure the selected crop area is at least as big as the destination size. This doesn't enforce minimum image dimensions."),
    '#default_value' => (isset($data['respectminimum']) ? $data['respectminimum'] : TRUE),
  );

  $form['onlyscaleifcrop'] = array(
    '#type' => 'checkbox',
    '#title' => t('Only scale if cropped'),
    '#description' => t('Only scale the image if it was manually cropped.'),
    '#default_value' => (isset($data['onlyscaleifcrop']) ? $data['onlyscaleifcrop'] : FALSE),
  );

  return $form;
}

/**
 * Returns HTML for a summary of an image crop and scale effect.
 *
 * @param $variables
 *    An associative array containing:
 *    - data: The current configuration for this crop and scale effect.
 *
 * @ingroup themeable
 */
function theme_manualcrop_crop_and_scale_summary($variables) {
  $data = $variables["data"];
  $str = check_plain($data['width']) . 'x' . check_plain($data['height']);

  if ($data['upscale'] || $data['respectminimum'] || !empty($data['respectminimum'])) {
    $str .= ' (' . ($data['upscale'] ? t('upscaling allowed') : '');

    if ($data['upscale'] && ($data['respectminimum'] || !empty($data['onlyscaleifcrop']))) {
      $str .= ($data['respectminimum'] && !empty($data['onlyscaleifcrop']) ? ', ' : ' ' . t('and') . ' ');
    }

    if ($data['respectminimum']) {
      $str .= t('crop minimum dimensions') . (!empty($data['onlyscaleifcrop']) ? ' ' . t('and') . ' ' : '');
    }

    if (!empty($data['onlyscaleifcrop'])) {
      $str .= t('only scale if cropped');
    }

    $str .= ')';
  }

  return $str;
}

/**
 * Form Builder; Configuration settings for crop effect.
 *
 * @param $data
 *    The current configuration for this crop effect.
 * @return
 *    The form structure array.
 */
function manualcrop_crop_form($data) {
  $form = image_resize_form($data);

  $form['width']['#title'] = t('Minimum crop width');
  $form['width']['#description'] = t('Only applied if a user tries to crop, this enforces no minimum image width!');
  $form['width']['#required'] = FALSE;

  $form['height']['#title'] = t('Minimum crop height');
  $form['height']['#description'] = t('Only applied if a user tries to crop, this enforces no minimum image height!');
  $form['height']['#required'] = FALSE;

  $form['keepproportions'] = array(
    '#type' => 'checkbox',
    '#title' => t('Maintain proportions'),
    '#description' => t('Maintain the proportions while cropping. This requires seting a width and height.'),
    '#default_value' => (isset($data['keepproportions']) ? $data['keepproportions'] : FALSE),
    '#element_validate' => array('manualcrop_keepproportions_validate'),
    '#states' => array(
      'disabled' => array(
        'input[name="data[width]"]' => array('empty' => TRUE),
        'input[name="data[height]"]' => array('empty' => TRUE),
      ),
      'unchecked' => array(
        'input[name="data[width]"]' => array('empty' => TRUE),
        'input[name="data[height]"]' => array('empty' => TRUE),
      ),
    ),
  );

  return $form;
}

/**
 * Element validation handler; validates the keepproportions checkbox from the
 * crop effect form.
 */
function manualcrop_keepproportions_validate($element, $form_state) {
  $values = $form_state['values']['data'];

  if ($values['keepproportions'] && (!$values['width'] || !$values['height'])) {
    form_set_error('data][keepproportions', t('Set a width and height if proportions should be maintained.'));
  }
}

/**
 * Returns HTML for a summary of an image crop effect.
 *
 * @param $variables
 *    An associative array containing:
 *    - data: The current configuration for this crop effect.
 *
 * @ingroup themeable
 */
function theme_manualcrop_crop_summary($variables) {
  $data = $variables['data'];

  if ($data['width'] && $data['height']) {
    $str = ' (' . t('minimum crop dimensions ') . ' ' . check_plain($data['width']) . 'x' . check_plain($data['height']);

    if (!empty($data['keepproportions'])) {
      $str .= ' ' . t('and') . ' ' . ($data['keepproportions'] ? t('maintain proportions') : '');
    }

    return $str . ')';
  }
  elseif ($data['width'] || $data['height']) {
    return ' (' . ($data['width']) ? t('minimum crop width @width', array('@width' => $data['width'])) : t('minimum crop height @height', array('@height' => $data['height'])) . ')';
  }


}

/**
 * Form Builder; Configuration settings for the reuse effect.
 *
 * @param $data
 *    The current configuration for this reuse effect.
 * @return
 *    The form structure array.
 */
function manualcrop_reuse_form($data) {
  // Load all image styles that have a cropping effect and exclude the style
  // that's currently being edited.
  $styles = manualcrop_get_styles_with_crop();
  $current = arg(5);
  if (isset($styles[$current])) {
    unset($styles[$current]);
  }

  if (!count($styles)) {
    drupal_set_message(t('No Manual Crop enabled image style could be found. To reuse a crop selection, you need to create at least one image style that uses Manual Crop.'), 'warning');
    drupal_goto('admin/config/media/image-styles/edit/' . $current);
  }

  $form['reusestyle'] = array(
    '#type' => 'select',
    '#title' => t('Base image style'),
    '#description' => t('Base this image style upon the crop selection of the selected style.'),
    '#options' => drupal_map_assoc(array_keys($styles), '_manualcrop_image_style_name'),
    '#default_value' => (isset($data['reusestyle']) ? $data['reusestyle'] : ''),
    '#required' => TRUE,
  );

  return $form;
}

/**
 * Returns HTML for a summary of an image crop selection reuse effect.
 *
 * @param $variables
 *    An associative array containing:
 *    - data: The current configuration for this crop selection reuse effect.
 *
 * @ingroup themeable
 */
function theme_manualcrop_reuse_summary($variables) {
  return '(' . t('Using @style', array('@style' => _manualcrop_image_style_name($variables['data']['reusestyle']))) . ')';
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
 *    Effect form array.
 * @param $form_state
 *    Form state array.
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

  // If it's a Manual Crop effect do a custom submit handler to cleanup the crop selections.
  if (_manualcrop_is_crop_effect($form_state['image_effect'])) {
    array_unshift($form['#submit'], 'manualcrop_image_effect_delete_form_submit');
  }
}

/**
 * Effect deletion form submit handler; Cleanup old selection selections.
 *
 * @param $form
 *    Effect form array.
 * @param $form_state
 *    Form state array.
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
 *    Effect or style form array.
 * @param $form_state
 *    Form state array.
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
 *    Style form array.
 * @param $form_state
 *    Form state array.
 */
function manualcrop_update_style_name($form, &$form_state) {
  $style = $form_state['image_style'];

  // Check if the style name should be updated.
  if (!empty($style['effects']) && isset($form_state['values']['name']) && $style['name'] != $form_state['values']['name']) {
    $effect = reset($style['effects']);

    if (_manualcrop_is_crop_effect($effect)) {
      // Change the style name in the effect data and save it.
      $effect["data"]["style_name"] = $form_state['values']['name'];
      image_effect_save($effect);

      // Update all widgets and reuse effects.
      _manualcrop_field_widget_update_names_in_settings($style['name'], $form_state['values']['name']);
      _manualcrop_reuse_effect_update_names_in_settings($style['name'], $form_state['values']['name']);

      // Update all saved crop selections.
      db_update('manualcrop')
        ->fields(array('style_name' => $form_state['values']['name']))
        ->condition('style_name', $style['name'])
        ->execute();
    }
  }
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
 *    Effect form array.
 * @param $form_state
 *    Form state array.
 */
function manualcrop_form_system_performance_settings_submit($form, &$form_state) {
  $cache_control = (bool) $form_state['values']['manualcrop']['cache_control'];

  variable_set('manualcrop_cache_control', $cache_control);

  if (!$cache_control) {
    // Clear the cached image paths.
    cache_clear_all('manualcrop', 'cache', TRUE);
  }
}
