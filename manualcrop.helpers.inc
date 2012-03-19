<?php
/**
 * @file
 * Helper functions for the Manual Crop module.
 */

/**
 * Gets the crop area for an image.
 *
 * @param $file
 *    Path to an image file.
 * @param $style_name
 *    Image style machine name, or empty for all styles.
 * @return
 *    An object defining the cropping area with following items:
 *    - "style_name": The machine name of the image style this cropping area applies on.
 *    - "x": An integer representing the top left corner's x-position in pixels.
 *    - "y": An integer representing the top left corner's y-position in pixels.
 *    - "width": An integer representing the width in pixels.
 *    - "height": An integer representing the height in pixels.
 *    If the style machine name was empty an object with all the styles is returned.
 */
function manualcrop_load_crop_selection($file, $style_name = NULL) {
  if (empty($style_name)) {
    $result = db_query('SELECT c.style_name, c.x, c.y, c.width, c.height FROM {manualcrop} c INNER JOIN {file_managed} f ON c.fid = f.fid WHERE f.uri = :uri', array(
      ':uri' => $file,
    ));

    return $result->fetchAll();
  }
  else {
    $result = db_query('SELECT c.x, c.y, c.width, c.height FROM {manualcrop} c INNER JOIN {file_managed} f ON c.fid = f.fid WHERE c.style_name = :name AND f.uri = :uri', array(
      ':name' => $style_name,
      ':uri' => $file,
    ));

    return $result->fetchObject();
  }

  return NULL;
}

/**
 * Returns the styles that have crop settings.
 *
 * @return
 *    An array of styles with the style name as key and the effect data as value.
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
 *    The field instance.
 */
function manualcrop_instance_required_styles($instance) {
  // Get the widget settings.
  $settings = $instance['widget']['settings'];

  // Array of required styles.
  $required = (isset($settings['require_cropping']) ? array_values($settings['require_cropping']) : array());

  // Make sure to exclude unavailable styles.
  if (!empty($required) && isset($settings['styles_mode']) && isset($settings['styles_list'])) {
    if ($settings['styles_mode'] == 'include') {
      $required = array_intersect($required, array_values($settings['styles_list']));
    }
    else {
      $required = array_diff($required, array_values($settings['styles_list']));
    }
  }

  return $required;
}

/**
 * Update or remove a style name in all Manual Crop field widgets.
 *
 * @param $style_name
 *    Current image style name.
 * @param $new_style_name
 *    New image style name if renamed, a NULL value will remove the style from the settings.
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
 *    Image style name.
 * @return
 *    Cleaned-up image style name.
 */
function _manualcrop_image_style_name($style_name) {
  global $language;
  static $custom_strings;

  $langcode = (isset($language->language) ? $language->language : 'en');

  // Load custom string for overriding.
  if (!isset($custom_strings[$langcode])) {
    $custom_strings[$langcode] = variable_get('locale_custom_strings_' . $langcode, array());
  }

  // Get the human readable name from the custom strings or make it ourself.
  if (isset($custom_strings[$langcode]['']['image-style-' . $style_name])) {
    return $custom_strings[$langcode]['']['image-style-' . $style_name];
  }
  else {
    return ucwords(str_replace('_', ' ', $style_name));
  }
}

/**
 * Get a default crop selection using Smart Crop.
 *
 * @param $file
 *    Path to an image file.
 * @param $style_name
 *    Image style machine name.
 * @return
 *    An object defining the cropping area with following items:
 *    - "style_name": The machine name of the image style this cropping area applies on.
 *    - "x": An integer representing the top left corner's x-position in pixels.
 *    - "y": An integer representing the top left corner's y-position in pixels.
 *    - "width": An integer representing the width in pixels.
 *    - "height": An integer representing the height in pixels.
 *
 * @see image_gd_smartcrop_scale()
 * @see image_gd_smartcrop_crop()
 */
function _manualcrop_get_smartcrop_selection($file, $style_name) {
  static $smartcrop_loaded;

  if (!isset($smartcrop_loaded)) {
    if (module_exists('smartcrop')) {
      module_load_include('inc', 'smartcrop', 'image.gd');
      $smartcrop_loaded = TRUE;
    }
    else {
      $smartcrop_loaded = FALSE;
    }
  }

  if ($smartcrop_loaded) {
    $effect = manualcrop_get_styles_with_crop();

    if (isset($effect[$style_name])) {
      $effect = $effect[$style_name];
      $image = image_load($file);

      if (empty($effect['data']['width'])) {
        $effect['data']['width'] = $image->info['width'];
      }

      if (empty($effect['data']['height'])) {
        $effect['data']['height'] = $image->info['height'];
      }

      // Calculate the cropping ratio.
      $ratio = max($effect['data']['width'] / $image->info['width'], $effect['data']['height'] / $image->info['height']);

      if ($ratio <= 1 || $effect['data']['upscale']) {
        // Update the desired width and height respective to the ratio.
        $effect['data']['width'] = $effect['data']['width'] / $ratio;
        $effect['data']['height'] = $effect['data']['height'] / $ratio;
      }

      // Logic below is copied from Smart Crop (http://drupal.org/project/smartcrop),
      // an issue has been opened to create some kind of API for this so we don't need
      // to copy it. See: http://drupal.org/node/1340106
      $dx = $image->info['width'] - min($image->info['width'], $effect['data']['width']);
      $dy = $image->info['height'] - min($image->info['width'], $effect['data']['height']);
      $left = $top = 0;
      $left_entropy = $right_entropy = $top_entropy = $bottom_entropy = 0;
      $right = $image->info['width'];
      $bottom = $image->info['height'];

      // Slice from left and right edges until the correct width is reached.
      while ($dx) {
        $slice = min($dx, 10);

        // Calculate the entropy of the new slice.
        if (!$left_entropy) {
          $left_entropy = _smartcrop_gd_entropy_slice($image, $left, $top, $slice, $image->info['height']);
        }
        if (!$right_entropy) {
          $right_entropy = _smartcrop_gd_entropy_slice($image, $right - $slice, $top, $slice, $image->info['height']);
        }

        // Remove the lowest entropy slice.
        if ($left_entropy >= $right_entropy) {
          $right -= $slice;
          $right_entropy = 0;
        } else {
          $left += $slice;
          $left_entropy = 0;
        }
        $dx -= $slice;
      }

      // Slice from the top and bottom edges until the correct width is reached.
      while ($dy) {
        $slice = min($dy, 10);

        // Calculate the entropy of the new slice.
        if (!$top_entropy) {
          $top_entropy = _smartcrop_gd_entropy_slice($image, $left, $top, $effect['data']['width'], $slice);
        }
        if (!$bottom_entropy) {
          $bottom_entropy = _smartcrop_gd_entropy_slice($image, $left, $bottom - $slice, $effect['data']['width'], $slice);
        }

        // Remove the lowest entropy slice.
        if ($top_entropy >= $bottom_entropy) {
          $bottom -= $slice;
          $bottom_entropy = 0;
        } else {
          $top += $slice;
          $top_entropy = 0;
        }
        $dy -= $slice;
      }

      return (object) array(
        'style_name' => $style_name,
        'x' => $left,
        'y' => $top,
        'width' => ($right - $left),
        'height' => ($bottom - $top),
      );
    }
  }

  return NULL;
}
