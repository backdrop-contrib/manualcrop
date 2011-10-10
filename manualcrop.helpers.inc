<?php
/**
 * @file
 * Helper functions for the Manual Crop module.
 */

/**
 * Gets the crop area for an image.
 *
 * @param $file
 *   Path to an image file.
 * @param $style_name
 *   Image style machine name, or empty for all styles.
 * @return
 *   An object defining the cropping area with following items:
 *   - "style_name": The machine name of the image style this cropping area applies on.
 *   - "x": An integer representing the top left corner's x-position in pixels.
 *   - "y": An integer representing the top left corner's y-position in pixels.
 *   - "width": An integer representing the width in pixels.
 *   - "height": An integer representing the height in pixels.
 *   If the style machine name was empty an object with all the styles is returned.
 */
function manualcrop_load_crop_selection($file, $style_name = NULL) {
  if (empty($style_name)) {
    $result = db_query('SELECT c.style_name, c.x, c.y, c.width, c.height FROM {manualcrop} c INNER JOIN {file_managed} f ON c.fid = f.fid WHERE f.uri = :uri', array(
      ':uri' => $file,
    ));

    return $result;
  }
  else {
    $result = db_query('SELECT c.x, c.y, c.width, c.height FROM {manualcrop} c INNER JOIN {file_managed} f ON c.fid = f.fid WHERE c.style_name = :name AND f.uri = :uri', array(
      ':name' => $style_name,
      ':uri' => $file,
    ));

    foreach ($result as $record) {
      return $record;
    }
  }

  return NULL;
}

/**
 * Returns the styles that have crop settings.
 *
 * @return
 *   An array of styles with the style name as key and the effect data as value.
 */
function manualcrop_get_styles_with_crop() {
  $hascrop = &drupal_static(__FUNCTION__, array());

  if (empty($hascrop)) {
    foreach (image_styles() as $style_name => $style) {
      if (!empty($style['effects'])) {
        // Check if the first effect is a Manual Crop effect.
        $effect = reset($style['effects']);

        if ($effect['module'] == 'manualcrop') {
          $hascrop[$style_name] = $effect;
        }
      }
    }
  }

  return $hascrop;
}

/**
 * Get the list of required image styles for a field instance.
 *
 * @param $instance
 *   The field instance.
 */
function manualcrop_instance_required_styles($instance) {
  // Get the widget settings.
  $widget_settings = $instance['widget']['settings'];

  // Array of required styles.
  $required = (isset($widget_settings['require_cropping']) ? array_values($widget_settings['require_cropping']) : array());

  // Make sure to exclude unavailable styles.
  if (!empty($required) && isset($widget_settings['styles_mode']) && isset($widget_settings['styles_list'])) {
    if ($widget_settings['styles_mode'] == 'include') {
      $required = array_intersect($required, array_values($widget_settings['styles_list']));
    }
    else {
      $required = array_diff($required, array_values($widget_settings['styles_list']));
    }
  }

  return $required;
}

/**
 * Update or remove a style name in all Manual Crop field widgets.
 *
 * @param $style_name
 *   Current image style name.
 * @param $new_style_name
 *   New image style name if renamed, a NULL value will remove the style from the settings.
 */
function _manualcrop_field_widget_update_names_in_settings($style_name, $new_style_name = null) {
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
                }
                elseif (empty($require)) {
                  unset($field['widget']['settings'][$key]);
                }
              }
              else {
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
 * Transform a style name into a more readable variant.
 *
 * @param $style_name
 *   Image style name.
 * @return
 *   Cleaned-up image style name.
 */
function _manualcrop_image_style_name($style_name) {
  return ucwords(str_replace('_', ' ', $style_name));
}