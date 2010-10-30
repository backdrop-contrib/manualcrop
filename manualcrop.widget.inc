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
  foreach (manualcrop_get_styles_with_crop() as $name => $data) {
    $settings[$data['isid']] = array(
      'name' => $data['name'],
      'data' => $data['data']
    );
  }
  drupal_add_js(array('manualCrop' => $settings), 'setting');

  // Modify the #process function for eacht element.
  foreach ($elements as $key => $element) {
    if (isset($element['#process'])) {
      $element['#process'] = array_diff($element['#process'], array('image_field_widget_process'));
      $element['#process'][] = 'manualcrop_field_widget_process';

      $elements[$key] = $element;
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
  $element = image_field_widget_process($element, $form_state, $form);

  $item = $element['#value'];
  $item['fid'] = $element['fid']['#value'];

  if ($element['#file'] && isset($_COOKIE['has_js']) && $_COOKIE['has_js']) {
    $hascrop = manualcrop_get_styles_with_crop();

    if (sizeof($hascrop) > 0) {
      $element['#theme'] = 'manualcrop_widget';

      // Get image info, width and height are used by the cropping widget.
      $image = image_get_info($element['#file']->uri);

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

      $options = array('-1' => t('Select a style to crop'));

      // Generate hidden fields, to save the crop area, for each image style.
      foreach ($hascrop as $name => $data) {
        $element['manualcrop_area'][$data['isid']] = array(
          '#type' => 'hidden',
          '#value' => (isset($item['manualcrop_area'][$data['isid']]) ? $item['manualcrop_area'][$data['isid']] : ''),
          '#attributes' => array(
            'id' => 'manualcrop-area-' . $item['fid'] . '-' . $data['isid'],
            'class' => array('manualcrop-overlay'),
          ),
        );

        $options[$data['isid']] = $name;
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
 * Returns the styles that have crop settings.
 *
 * @return
 *   An array of styles with the style name as key and containing:
 *   - "isid": The style id.
 *   - "effect": The effect data.
 */
function manualcrop_get_styles_with_crop() {
  $hascrop = &drupal_static(__FUNCTION__, array());

  if(empty($hascrop)) {
    $styles = image_styles();

    foreach ($styles as $name => $style) {
      if (!empty($style['effects'])) {
        $effect = reset($style['effects']);

        if ($effect['module'] == 'manualcrop') {
          // The first effect is a manual crop effect.
          $hascrop[$name] = $effect;
        }
      }
    }
  }

  return $hascrop;
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
  $output .= '<a class="manualcrop-button manualcrop-reset" href="#" onclick="ManualCrop.resetSelection(); this.blur();">' . t('Reset selection') . '</a>';
  $output .= '<a class="manualcrop-button manualcrop-clear" href="#" onclick="ManualCrop.clearSelection(); this.blur();">' . t('Clear selection') . '</a>';
  $output .= '<a class="manualcrop-button manualcrop-close clearfix" href="#" onclick="ManualCrop.closeOverlay(); this.blur();">' . t('Close overlay') . '</a>';
  $output .= '</div>';

  $output .= '</div>';

  return $output;
}