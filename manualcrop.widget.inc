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
  $settings = $instance['widget']['settings'];

  // Use the image field settings form.
  $form = image_field_widget_settings_form($field, $instance);

  $form['styles_mode'] = array(
    '#type' => 'radios',
    '#title' => t('Styles list mode'),
    '#options' => array(
      'exclude' => t('Exclude selected styles'),
      'include' => t('Include selected styles'),
    ),
    '#default_value' => (isset($settings['styles_mode']) ? $settings['styles_mode'] : 'exclude'),
    '#weight' => 16,
  );

  $form['styles_list'] = array(
    '#type' => 'select',
    '#title' => t('Styles list'),
    '#description' => t('Select all styles that should be shown or hidden (as selected above) in the widget. Please note that hiding styles will override requiring them and existing cropping selections will be kept.'),
    '#options' => drupal_map_assoc(array_keys(manualcrop_get_styles_with_crop())),
    '#default_value' => (isset($settings['styles_list']) ? $settings['styles_list'] : array()),
    '#multiple' => TRUE,
    '#weight' => 17,
  );

  $form['require_cropping'] = array(
    '#type' => 'select',
    '#title' => t('Required croppings'),
    '#description' => t('Select all styles that must have a crop selection.'),
    '#options' => drupal_map_assoc(array_keys(manualcrop_get_styles_with_crop())),
    '#default_value' => (isset($settings['require_cropping']) ? $settings['require_cropping'] : array()),
    '#multiple' => TRUE,
    '#weight' => 18,
  );

  return $form;
}

/**
 * Implements hook_field_widget_form().
 */
function manualcrop_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  // Use the image field widget form.
  $elements = image_field_widget_form($form, $form_state, $field, $instance, $langcode, $items, $delta, $element);

  // Get the widget settings.
  $instance = field_widget_instance($element, $form_state);
  $widget_settings = $instance['widget']['settings'];

  // Generate and set the settings for each style effect.
  $settings = array();
  foreach (manualcrop_get_styles_with_crop() as $style_name => $data) {
    $settings[$style_name] = array(
      'effect' => $data['name'],
      'data' => $data['data']
    );
  }

  // Required and included/excluded styles.
  $required = (isset($widget_settings['require_cropping']) ? array_values($widget_settings['require_cropping']) : array());
  if (isset($widget_settings['styles_mode']) && isset($widget_settings['styles_list'])) {
    if ($widget_settings['styles_mode'] == 'include') {
      $required = array_intersect($required, array_values($widget_settings['styles_list']));
    } else {
      $required = array_diff($required, array_values($widget_settings['styles_list']));
    }
  }

  // Add the JavaScript settings.
  $form['#attached']['js'][] = array('data' => array('manualCrop' => array(
    'styles' => $settings,
    'required' => $required,
  )), 'type' => 'setting');

  // Add a process function to the elements.
  foreach ($elements as $key => $child) {
    if (isset($child['#process']) && is_array($child['#process']) && in_array('image_field_widget_process', $child['#process'])) {
	  $elements[$key]['#process'][] = 'manualcrop_field_widget_process';
    }
  }

  // Validation and submit function.
  $form['#validate'][] = 'manualcrop_validate_crop_selection';
  $form['#submit'][] = 'manualcrop_save_crop_selection';

  if (!isset($form_state['manualcrop_widget_data'])) {
    $form_state['manualcrop_widget_data'] = array('images' => array());
  } else {
    // Reset the images array.
    $form_state['manualcrop_widget_data']['images'] = array();
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
      // Get the widget settings.
      $instance = field_widget_instance($element, $form_state);
      $widget_settings = $instance['widget']['settings'];

      // Required image styles.
      $required = (isset($widget_settings['require_cropping']) ? array_values($widget_settings['require_cropping']) : array());

      // Exclude or include styles.
      if (isset($widget_settings['styles_mode']) && isset($widget_settings['styles_list'])) {
        $list = (isset($widget_settings['styles_list']) ? array_values($widget_settings['styles_list']) : array());

        // Filter hidden styles.
        if ($widget_settings['styles_mode'] == 'include') {
          $required = array_intersect($required, $list);
          $hascrop = array_intersect_key($hascrop, array_flip($list));
        } else {
          $required = array_diff($required, $list);
          $hascrop = array_diff_key($hascrop, array_flip($list));
        }
      }

      if (!empty($hascrop)) {
        // Override the theming function.
        $element['#theme'] = 'manualcrop_widget';

        // Get image info, width and height are used by the cropping widget.
        $image = image_get_info($element['#file']->uri);

        // Save some images data to improve processing.
        $form_state['manualcrop_widget_data']['images'][$item['fid']] = array(
          'uri' => $element['#file']->uri,
          'filename' => $element['#file']->filename,
          'width' => $image['width'],
          'height' => $image['height'],
          'element_parents' => $element['#array_parents'],
          'required_styles' => $required,
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
            '#default_value' => (isset($item['manualcrop_selection'][$style_name]) ? $item['manualcrop_selection'][$style_name] : ''),
            '#attributes' => array(
              'id' => 'manualcrop-area-' . $item['fid'] . '-' . $style_name,
              'class' => array('manualcrop-cropdata'),
              'onchange' => 'ManualCrop.selectionStored(this, ' . $item['fid'] . ', \'' . $style_name . '\');',
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
              drupal_get_path('module', 'manualcrop') . '/jquery.imgareaselect/css/imgareaselect-animated.css',
            ),
            'js' => array(
              drupal_get_path('module', 'manualcrop') . '/manualcrop.js',
              drupal_get_path('module', 'manualcrop') . '/jquery.imgareaselect/scripts/jquery.imgareaselect.pack.js',
            ),
          ),
        );

        $element['alt']['#weight']--;
        $element['title']['#weight']--;
      }
    }
  }

  return $element;
}

/**
 * Update or remove a style name in all Manual Crop field widgets.
 *
 * @param $style_name
 *   Current image style name.
 * @param $new_style_name
 *   New image style name if renamed, a NULL value will remove the style from the settings.
 */
function manualcrop_field_widget_update_names_in_settings($style_name, $new_style_name = null) {
  foreach (field_info_fields() as $field) {
    if ($field['module'] == 'image') {
      foreach ($field['bundles'] as $entity_type => $bundles) {
        foreach ($bundles as $bundle) {
          // Check each instance for processing.
          $instance = field_info_instance($entity_type, $field['field_name'], $bundle);

          if ($instance['widget']['module'] == 'manualcrop' && (isset($instance['widget']['settings']['require_cropping']) || isset($instance['widget']['settings']['styles_list']))) {
            $list = array();

            // Add all existing settings to the list.
            if (isset($instance['widget']['settings']['require_cropping'])) {
              $list['require_cropping'] = &$instance['widget']['settings']['require_cropping'];
            }

            if (isset($instance['widget']['settings']['styles_list'])) {
              $list['styles_list'] = &$instance['widget']['settings']['styles_list'];
            }

            // Process all settings.
            foreach ($list as $key => &$item) {
              if (isset($item[$style_name])) {
                unset($item[$style_name]);

                if (!is_null($new_style_name)) {
                  $item[$new_style_name] = $new_style_name;
                } elseif (empty($require)) {
                  unset($field['widget']['settings'][$key]);
                }
              } else {
                // Not processed, so remove it from the list.
                unset($list[$key]);
              }
            }

            if (!empty($list)) {
              // Settings where updated, save the instance.
              field_update_instance($instance);
            }
          }
        }
      }
    }
  }
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
  $output .= '<a class="manualcrop-button manualcrop-reset" href="javascript:void(0);" onclick="ManualCrop.resetSelection(); this.blur();">' . t('Restore selection') . '</a>';
  $output .= '<a class="manualcrop-button manualcrop-clear" href="javascript:void(0);" onclick="ManualCrop.clearSelection(); this.blur();">' . t('Remove selection') . '</a>';
  $output .= '<a class="manualcrop-button manualcrop-close clearfix" href="javascript:void(0);" onclick="ManualCrop.closeOverlay(); this.blur();">' . t('Save selection') . '</a>';
  $output .= '</div>';

  $output .= '</div>';

  return $output;
}
