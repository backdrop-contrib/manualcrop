<?php
/**
 * @file
 * Implement an image widget, based upon the standard Image's module widget.
 */

/**
 * Implements hook_field_widget_info().
 */
function manualcrop_field_widget_info() {
  return array(
    'manualcrop_image' => array(
      'label' => t('Manual image crop'),
      'field types' => array('image'),
      'settings' => array(
        'progress_indicator' => 'throbber',
        'preview_image_style' => 'thumbnail',
      ),
      'behaviors' => array(
        'multiple values' => FIELD_BEHAVIOR_CUSTOM,
        'default value' => FIELD_BEHAVIOR_NONE,
      ),
    ),
  );
}

/**
 * Implements hook_field_widget_settings_form().
 */
function manualcrop_field_widget_settings_form($field, $instance) {
  // Use the image field settings form.
  return image_field_widget_settings_form($field, $instance);
}

/**
 * Implements hook_field_widget_form().
 */
function manualcrop_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  // Use the image field widget form.
  $elements = image_field_widget_form($form, $form_state, $field, $instance, $langcode, $items, $delta, $element);

  // Generate and set the settings for each style effect.
  $settings = array();
  foreach (manualcrop_get_styles_with_crop() as $style_name => $data) {
    $settings[$style_name] = array(
      'effect' => $data['name'],
      'data' => $data['data']
    );
  }
  $form['#attached']['js'][] = array('data' => array('manualCrop' => $settings), 'type' => 'setting');

  // Add our process function to the elements.
  foreach ($elements as $key => $child) {
    if (isset($child['#process']) && is_array($child['#process']) && in_array('image_field_widget_process', $child['#process'])) {
	  $elements[$key]['#process'][] = 'manualcrop_field_widget_process';
    }
  }

  return $elements;
}

/**
 * An element #process callback for the image_image field type.
 *
 * Expands the default image widget to allow cropping.
 */
function manualcrop_field_widget_process($element, &$form_state, $form) {
  $item = $element['#value'];
  $item['fid'] = $element['fid']['#value'];

  if ($element['#file'] && isset($_COOKIE['has_js']) && $_COOKIE['has_js']) {
    $hascrop = manualcrop_get_styles_with_crop();

    if (!empty($hascrop)) {
      // Override the theming function.
      $element['#theme'] = 'manualcrop_widget';

      // Get image info, width and height are used by the cropping widget.
      $image = image_get_info($element['#file']->uri);

      // Save some images data to improve processing.
      $form_state['manualcrop_widget_data']['images'][$item['fid']] = array(
        'parents' => $element['#array_parents'],
        'uri' => $element['#file']->uri,
        'filename' => $element['#file']->filename,
        'width' => $image['width'],
        'height' => $image['height'],
      );

      // Get the saved selections for this file.
      if (!$form_state['submitted']) {
        $selections = manualcrop_load_crop_selection($element['#file']->uri);

        foreach ($selections as $data) {
          $item['manualcrop_selection'][$data->style_name] = $data->x . '|' . $data->y . '|' . $data->width . '|' . $data->height;
        }
      }

      // Overlay with the original image for cropping.
      $element['manualcrop_overlay'] = array(
        '#type' => 'markup',
        '#markup' => theme('manualcrop_overlay', array(
          'attributes' => array(
            'id' => array('manualcrop-overlay-' . $item['fid']),
            'class' => array('manualcrop-overlay', 'element-invisible'),
          ),
          'image' => array(
            'path' => $element['#file']->uri,
            'alt' => t('Manual crop'),
            'width' => $image['width'],
            'height' => $image['height'],
            'attributes' => array('class' => 'manualcrop-image'),
          )
        )),
      );

      // Image style options.
      $options = array('' => t('Select a style to crop'));

      // Hidden fields to save the crop selection, one for each image style.
      foreach ($hascrop as $style_name => $data) {
        $element['manualcrop_selection'][$style_name] = array(
          '#type' => 'hidden',
          '#value' => (isset($item['manualcrop_selection'][$style_name]) ? $item['manualcrop_selection'][$style_name] : ''),
          '#attributes' => array(
            'id' => 'manualcrop-area-' . $item['fid'] . '-' . $style_name,
            'class' => array('manualcrop-cropdata'),
            'onchange' => 'ManualCrop.fieldUpdated(this, ' . $item['fid'] . ', \'' . $style_name . '\');',
          ),
        );

        $options[$style_name] = $style_name;
      }

      // Selection list for opening the overlay.
      $element['manualcrop_style'] = array(
        '#type' => 'select',
        '#title' => t('Manual crop'),
        '#description' => t('Select a style to crop the image for that style, an overlay will open that you can close after cropping.'),
        '#options' => $options,
        '#multiple' => FALSE,
        '#weight' => $element['title']['#weight'],
        '#attributes' => array(
          'class' => array('manualcrop-style-select', 'manualcrop-style-select-' . $item['fid']),
          'onchange' => 'ManualCrop.showOverlay(this, ' . $item['fid'] . ');',
        ),
        '#attached' => array(
          'css' => array(
            drupal_get_path('module', 'manualcrop') . '/manualcrop.css',
            drupal_get_path('module', 'manualcrop') . '/imgAreaSelect/imgareaselect-animated.css',
          ),
          'js' => array(
            drupal_get_path('module', 'manualcrop') . '/manualcrop.js',
            drupal_get_path('module', 'manualcrop') . '/imgAreaSelect/jquery.imgareaselect.pack.js',
          ),
        ),
      );

      $element['alt']['#weight']--;
      $element['title']['#weight']--;
    }
  }

  return $element;
}

/**
 * Returns HTML for an manual crop image field widget.
 *
 * @param $variables
 *   An associative array containing:
 *   - "element": A render element representing the manual crop image field widget.
 *
 * @ingroup themeable
 */
function theme_manualcrop_widget($variables) {
  // Use the image widget theming function.
  return theme_image_widget($variables);
}

/**
 * Returns HTML for the crop area of an image.
 *
 * @param $variables
 *   An associative array containing:
 *   - "attributes": An array of attributes.
 *   - "image": An array of variables for the image theming function.
 *
 * @ingroup themeable
 */
function theme_manualcrop_overlay($variables) {
  $output = '<div ' . drupal_attributes($variables["attributes"]) . '>';

  $output .= '<div class="manualcrop-overlay-bg"></div>';

  $output .= '<div class="manualcrop-image-holder">';
  $output .= theme('image', $variables["image"]);
  $output .= '</div>';

  $output .= '<div class="manualcrop-details">';
  $output .= t('Image style') . ': <span class="manualcrop-details-value manualcrop-image-style">&nbsp;</span>, ';
  $output .= t('Selection x') . ': <span class="manualcrop-details-value manualcrop-selection-x">-</span>, ';
  $output .= t('Selection y') . ': <span class="manualcrop-details-value manualcrop-selection-y">-</span>, ';
  $output .= t('Selection width') . ': <span class="manualcrop-details-value manualcrop-selection-width">-</span>, ';
  $output .= t('Selection height') . ': <span class="manualcrop-details-value manualcrop-selection-height">-</span>';
  $output .= '</div>';

  $output .= '<div class="manualcrop-buttons">';
  $output .= '<a class="manualcrop-button manualcrop-reset" href="javascript:void(0);" onclick="ManualCrop.resetSelection(); this.blur();">' . t('Reset selection') . '</a>';
  $output .= '<a class="manualcrop-button manualcrop-clear" href="javascript:void(0);" onclick="ManualCrop.clearSelection(); this.blur();">' . t('Clear selection') . '</a>';
  $output .= '<a class="manualcrop-button manualcrop-close clearfix" href="javascript:void(0);" onclick="ManualCrop.closeOverlay(); this.blur();">' . t('Close overlay') . '</a>';
  $output .= '</div>';

  $output .= '</div>';

  return $output;
}