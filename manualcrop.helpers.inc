<?php
/**
 * @file
 * Helper functions for the Manual Crop module.
 */

/**
 * Returns an array of supported widget types or checks if a type is supported.
 *
 * @param $widget_type
 *   If set, this function will return a boolean indicating if $widget_type
 *   is supported.
 * @param $settings
 *   Only include widgets that support these setting(s).
 *
 * @return
 *   Array of widget types.
 */
function manualcrop_supported_widgets($widget_type = NULL, $settings = array()) {
  $widgets = &drupal_static(__FUNCTION__);

  if (!isset($widgets)) {
    // Collect information about the supported widgets.
    $widgets = module_invoke_all('manualcrop_supported_widgets');
    drupal_alter('manualcrop_supported_widgets', $widgets);
  }

  // Make sure $settings contains only valid entries.
  if (!empty($settings)) {
    if (!is_array($settings)) {
      $settings = array($settings);
    }

    $widget_settings = manualcrop_manualcrop_supported_widgets();
    $widget_settings = $widget_settings['image_image'];

    $settings = array_intersect($settings, $widget_settings);
  }

  if (empty($settings)) {
    // No settings required.
    $result = array_keys($widgets);
  }
  else {
    // Filter all widgets that don't support the required settings.
    $result = array();
    foreach ($widgets as $name => $widget_settings) {
      if (!count(array_diff($settings, $widget_settings))) {
        $result[] = $name;
      }
    }
  }

  if (!empty($widget_type)) {
    return in_array($widget_type, $result);
  }
  else {
    return $result;
  }
}

/**
 * Returns the default widget settings.
 *
 * @return
 *   Array of default widget settings.
 */
function manualcrop_default_widget_settings() {
  $defaults = array(
    // Enable Manual Crop?
    'manualcrop_enable' => FALSE,
    // Enable keyboard shortcuts?
    'manualcrop_keyboard' => TRUE,
    // Show a list of thubnails instead of a selection list or button?
    'manualcrop_thumblist' => FALSE,
    // Enable inline cropping?
    'manualcrop_inline_crop' => FALSE,
    // Show the crop info (width, height...)?
    'manualcrop_crop_info' => TRUE,
    // Automatically update the preview image?
    'manualcrop_instant_preview' => TRUE,
    // Open the crop tool after uploading?
    'manualcrop_instant_crop' => FALSE,
    // Show a default crop area when opening an uncropped image?
    'manualcrop_default_crop_area' => TRUE,
    // Maximize the default crop area?
    'manualcrop_maximize_default_crop_area' => FALSE,
    // Exclude or include the selected styles?
    'manualcrop_styles_mode' => 'include',
    // List of selected styles.
    'manualcrop_styles_list' => array(),
    // List of required crop selections.
    'manualcrop_require_cropping' => array(),
  );

  if (module_exists('insert')) {
    // Filter all styles without a Manual Crop effect?
    $defaults['manualcrop_filter_insert'] = TRUE;
  }

  return $defaults;
}

/**
 * Get the unique javascript crop settings identifier.
 *
 * @param $data
 *   Field instance array (preferred) or file object.
 *
 * @return
 *   Unique javascript crop settings identifier.
 */
function manualcrop_js_identifier($data) {
  if (is_array($data) && !empty($data['field_name'])) {
    return drupal_clean_css_identifier($data['field_name']);
  }
  elseif (is_object($data) && isset($data->fid)) {
    return 'manualcrop-file-' . $data->fid;
  }
  else {
    return 'manualcrop-' . md5(serialize($data));
  }
}

/**
 * Gets the crop area for an image.
 *
 * @param $file
 *   Path to an image file.
 * @param $style_name
 *   Image style machine name, leave empty for all styles.
 *
 * @return
 *   When $style_name is set, a single crop selection will be returned. Otherwise
 *   the result is an array of crop selection objects keyed by style name.
 *   Each object contains following items:
 *   - "style_name": The machine name of the image style this cropping area applies on.
 *   - "x": An integer representing the top left corner's x-position in pixels.
 *   - "y": An integer representing the top left corner's y-position in pixels.
 *   - "width": An integer representing the width in pixels.
 *   - "height": An integer representing the height in pixels.
 */
function manualcrop_load_crop_selection($file, $style_name = NULL) {
  if (isset($style_name)) {
    $result = db_query('SELECT c.x, c.y, c.width, c.height FROM {manualcrop} c INNER JOIN {file_managed} f ON c.fid = f.fid WHERE c.style_name = :name AND f.uri = :uri', array(
      ':name' => $style_name,
      ':uri' => $file,
    ));

    return $result->fetchObject();
  }
  else {
    $result = db_query('SELECT c.style_name, c.x, c.y, c.width, c.height FROM {manualcrop} c INNER JOIN {file_managed} f ON c.fid = f.fid WHERE f.uri = :uri', array(
      ':uri' => $file,
    ));

    return $result->fetchAllAssoc('style_name');
  }
}

/**
 * Returns the styles that have crop settings.
 *
 * @param $include_reuse
 *   Set to TRUE to include styles with a Manual Crop reuse effect.
 * @param $exclude_arg
 *   Exclude the style that is set as a menu argument on this index.
 * @param $return_label
 *   Set to TRUE to return the label instead of an array.
 *
 * @return
 *   If $label is set to TRUE, this function will return an array of style labels
 *   keyed by style name. Otherwise an array of crop-enabled styles will be
 *   returned. This array is also keyed by style name and each element in this
 *   array is also an array with 2 elements:
 *   - "label": Human readable style label.
 *   - "effect": Manual Crop effect data.
 */
function manualcrop_styles_with_crop($include_reuse = FALSE, $exclude_arg = NULL, $return_label = FALSE) {
  $hascrop = &drupal_static(__FUNCTION__);

  if (!is_array($hascrop)) {
    $hascrop = array(array(), array());

    foreach (image_styles() as $style_name => $style) {
      if (!empty($style['effects'])) {
        // Check if the first effect is a Manual Crop cropping effect.
        $effect = reset($style['effects']);

        if (_manualcrop_is_own_effect($effect)) {
          $label = _manualcrop_image_style_label($style);

          $hascrop[1][$style_name] = array(
            'label' => $label,
            'effect' => $effect,
          );

          if (_manualcrop_is_own_effect($effect, TRUE)) {
            $hascrop[0][$style_name] = array(
              'label' => $label,
              'effect' => $effect,
            );
          }
        }
      }
    }
  }

  // With or without reuse effects.
  $styles = $hascrop[(int) $include_reuse];

  // Exclude a style by menu arument.
  if (!is_null($exclude_arg)) {
    $exclude_arg = arg($exclude_arg);
    if (isset($styles[$exclude_arg])) {
      unset($styles[$exclude_arg]);
    }
  }

  // Only the labels should be returned.
  if ($return_label) {
    foreach ($styles as $style_name => $style) {
      $styles[$style_name] = $style['label'];
    }
  }

  return $styles;
}

/**
 * Get the list of required image styles from the widget settings.
 *
 * @param $settings
 *   Widget settings array.
 *
 * @return
 *   List of required image styles.
 */
function manualcrop_instance_required_styles($settings) {
  // Make sure to exclude unavailable styles.
  if (!empty($settings['manualcrop_require_cropping']) && !empty($settings['manualcrop_styles_list'])) {
    if ($settings['manualcrop_styles_mode'] == 'include') {
      return array_intersect($settings['manualcrop_require_cropping'], $settings['manualcrop_styles_list']);
    }
    else {
      return array_diff($settings['manualcrop_require_cropping'], $settings['manualcrop_styles_list']);
    }
  }

  return array();
}

/**
 * Update or remove a style name in all Manual Crop field widgets.
 *
 * @param $style_name
 *   Current image style name.
 * @param $new_style_name
 *   New image style name if renamed, a NULL value will remove the style from the settings.
 */
function _manualcrop_update_style_name_in_field_widget($style_name, $new_style_name = NULL) {
  foreach (field_info_fields() as $field) {
    if ($field['module'] == 'image') {
      foreach ($field['bundles'] as $entity_type => $bundles) {
        foreach ($bundles as $bundle) {
          // Check each instance for processing.
          $instance = field_info_instance($entity_type, $field['field_name'], $bundle);
          $settings = &$instance['widget']['settings'];

          if (manualcrop_supported_widgets($instance['widget']['type']) && (!empty($settings['manualcrop_require_cropping']) || !empty($settings['manualcrop_styles_list']))) {
            $list = array();

            // Add all existing settings to the list.
            if (!empty($settings['manualcrop_require_cropping'])) {
              $list['manualcrop_require_cropping'] = &$settings['manualcrop_require_cropping'];
            }

            if (!empty($settings['manualcrop_styles_list'])) {
              $list['manualcrop_styles_list'] = &$settings['manualcrop_styles_list'];
            }

            // Process all settings.
            foreach ($list as $key => &$item) {
              if (isset($item[$style_name])) {
                if (!is_null($new_style_name)) {
                  $item[$new_style_name] = $new_style_name;
                }

                unset($item[$style_name]);
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
 * Update or remove a style name in all Manual Crop reuse image effects.
 *
 * @param $style_name
 *   Current image style name.
 * @param $new_style_name
 *   New image style name if renamed, a NULL value will remove the effect from the style.
 */
function _manualcrop_update_style_name_in_image_effect($style_name, $new_style_name = NULL) {
  foreach (image_styles() as $style) {
    if (!empty($style['effects'])) {
      // Check if the first effect is a Manual Crop effect.
      $effect = reset($style['effects']);

      // Check if this is a Manual Crop reuse effect.
      if (_manualcrop_is_own_effect($effect)) {
        $data = $effect['data'];
        $key = (isset($data['reuse_crop_style']) ? 'reuse_crop_style' : 'fallback_style');

        // Only update if needed.
        if (isset($data[$key]) && $data[$key] == $style_name) {
          if (is_null($new_style_name) && $effect['name'] == 'manualcrop_reuse') {
            // The reuse effect requires an image style.
            image_effect_delete($effect);
          }
          else {
            $effect['data'][$key] = $new_style_name;
            image_effect_save($effect);
          }
        }
      }
    }
  }
}

/**
 * Transform a style name into a more readable variant.
 *
 * @param $style
 *   Image style info array or style name.
 *
 * @return
 *   Cleaned-up image style name.
 */
function _manualcrop_image_style_label($style) {
  global $language;
  static $custom_strings;

  // Get the image style info.
  if (!is_array($style)) {
    $styles = image_styles();
    if (isset($styles[$style])) {
      $style = $styles[$style];
    }
    else {
      // Normally we shouldn't be here...
      $style = array('name' => $style);
    }
  }

  // The label is only available in Drupal 7.23 and up.
  if (isset($style['label'])) {
    return $style['label'];
  }
  else {
    $style_name = $style['name'];
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
}

/**
 * Checks if the effect is a Manual Crop effect.
 *
 * @param $effect
 *   Image style effect information array.
 * @param $crop_effect
 *   Set to TRUE to require a cropping effect; set to FALSE to require
 *   a reuse effect. Defaults to NULL, which ignores effect type.
 *
 * @return
 *   TRUE if this is a Manual Crop (cropping/reuse) effect, FALSE otherwise.
 */
function _manualcrop_is_own_effect($effect, $crop_effect = NULL) {
  if ($effect['module'] == 'manualcrop') {
    if (is_null($crop_effect)) {
      return TRUE;
    };

    return $crop_effect ^ in_array($effect['name'], array('manualcrop_reuse', 'manualcrop_auto_reuse'));
  }
  return FALSE;
}

/**
 * Get the image style name that should be used when processing the auto reuse effect.
 *
 * @param $file
 *   Path to an image file.
 * @param $data
 *   Auto reuse effect data.
 *
 * @return
 *   Image style name that can be reused or FALSE if no crop selection was found.
 */
function _manualcrop_get_auto_reuse_style_name($file, $data) {
  // Get the crop selections.
  $crop = manualcrop_load_crop_selection($file);

  if ($crop) {
    // Get a list of existing crop styles (keys) ordered by priority.
    $styles = array_flip($data['style_priority']) + $crop;
    $crop = array_intersect_key($styles, $crop);

    // Return the first crop selection style name.
    reset($crop);
    return key($crop);
  }

  return FALSE;
}

/**
 * Adds a cache control parameter to the image URI so the image will be reloaded
 * if the crop selection was changed since it was last feched by the browser.
 *
 * @param $style_name
 *   Image style name.
 * @param $path
 *   The absolute URL where the styled image can be downloaded.
 *
 * @return
 *   The altered image URL or NULL if the URL wasn't changed.
 */
function _manualcrop_add_cache_control($style_name, $url) {
  // Is cache control enabled?
  if (variable_get('manualcrop_cache_control', TRUE)) {
    $styles = manualcrop_styles_with_crop(TRUE);

    // Does this image style have a Manual Crop effect?
    if (isset($styles[$style_name])) {
      $cache_key = 'manualcrop:' . md5($url);

      // Attempt to load the HTTP cache-controller from cache.
      if ($cached_url = cache_get($cache_key)) {
        return $cached_url->data;
      }

      // Get the image path from the URL.
      $match = '/styles/' . $style_name . '/';
      $path = parse_url($url, PHP_URL_PATH);
      $path = drupal_substr($path, (strrpos($path, $match) + drupal_strlen($match)));
      $path = explode('/', $path);

      // Build the local image URI.
      $scheme = array_shift($path);
      $target = implode('/', $path);
      $image_uri = $scheme . '://' . urldecode($target);

      // Get the image effect.
      $effect = $styles[$style_name]['effect'];

      if (_manualcrop_is_own_effect($effect, FALSE)) {
        unset($style_name);

        switch ($effect['name']) {
          case 'manualcrop_reuse':
            // Use the reuse style to load the crop selection.
            if (!empty($effect['data']['reuse_crop_style'])) {
              $style_name = $effect['data']['reuse_crop_style'];
            }
            break;

          case 'manualcrop_auto_reuse':
            // Get the first applied crop selection.
            if ($crop = manualcrop_load_crop_selection($image_uri)) {
              $crop = reset($crop);
            }
            break;
        }
      }

      // Load the crop selection.
      if (isset($style_name)) {
        $crop = manualcrop_load_crop_selection($image_uri, $style_name);
      }

      // Add the cache controller and cache the new URL.
      if (!empty($crop)) {
        $url .= (strpos($url, '?') ? '&' : '?') . 'c=' . md5($crop->x . '|' . $crop->y . '|' . $crop->width . '|' . $crop->height);
        cache_set($cache_key, $url);

        return $url;
      }
    }
  }

  return NULL;
}

/**
 * Determines if the form for a Field Collection field has been prepared.
 * This function checks if the field instance is attached to a Field Collection
 * item entity, if so we'll store it and return FALSE the next time.
 *
 * @param $instance
 *   Field instance array.
 *
 * @return
 *   TRUE if this field isn't attached to a Field Collection entity or if it was
 *   checked for the first time, FALSE otherwise.
 */
function _manualcrop_check_field_collection_form_prepare($instance) {
  $processed = &drupal_static(__FUNCTION__, array());

  if (module_exists('field_collection')) {
    if (!empty($instance['entity_type']) && $instance['entity_type'] == 'field_collection_item') {
      if (isset($processed[$instance['field_name']])) {
        return FALSE;
      }

      $processed[$instance['field_name']] = TRUE;
    }
  }

  return TRUE;
}

/**
 * Add the crop functionality to the File Entity form.
 *
 * @param $form
 *   Complete form array.
 * @param $form_state
 *   Form state array.
 * @param $instance_info
 *   Field instance info, this array should contain 3 keys: entity_type, bundle
 *   and field_name.
 */
function _manualcrop_process_file_entity_form(&$form, &$form_state, $instance_info) {
  // Get the field instance settings.
  $settings = array();
  if (!empty($instance_info['entity_type']) && !empty($instance_info['bundle']) && !empty($instance_info['field_name'])) {
    // Get the field instance.
    $instance = field_info_instance($instance_info['entity_type'], $instance_info['field_name'], $instance_info['bundle']);

    if (!empty($instance)) {
      // A field instance was found, use its settings.
      $settings = $instance['widget']['settings'];

      if (empty($settings['manualcrop_enable'])) {
        // Manual Crop isn't enabled.
        return;
      }
    }
  }

  // Build the javascript identifier.
  $js_identifier = manualcrop_js_identifier($form['#entity']);

  // Prepare the form.
  manualcrop_croptool_prepare_form($form, $form_state, $js_identifier, $settings);

  // Add the croptool.
  manualcrop_croptool_process($form, $form_state, $js_identifier, $form['#entity'], NULL, $settings);

  // Add the submit handler.
  $form['#submit'][] = 'manualcrop_croptool_submit';
}

/**
 * Get the version of a library. This function uses libraries_get_version(),
 * the only difference is that the $options['file'] key can be an array.
 *
 * @param $library
 *   Library info array.
 * @param $options
 *   Options array as specified by libraries_get_version().
 *
 * @return
 *   A string containing the version of the library.
 *
 * @see libraries_get_version().
 */
function manualcrop_library_get_version($library, $options) {
  if (!empty($options['file'])) {
    $files = (array) $options['file'];
    foreach ($files as $file) {
      $options['file'] = $file;
      $version = libraries_get_version($library, $options);

      if ($version) {
        return $version;
      }
    }
  }
}

/**
 * Check if a library variant has been installed.
 *
 * @param $library
 *   Library info array.
 * @param $variant
 *   Variant name.
 *
 * @return
 *   TRUE if the variant exists, FALSE otherwise.
 */
function manualcrop_library_detect_variant($library, $variant) {
  if (!empty($library['variants'][$variant]['files'])) {
    $files = $library['variants'][$variant]['files'];

    foreach (array('js', 'css') as $type) {
      if (!empty($files[$type])) {
        reset($files[$type]);
        return file_exists(DRUPAL_ROOT . '/' . $library['library path'] . '/' . key($files[$type]));
      }
    }
  }

  return FALSE;
}

/**
 * Smart library loader, it checks for a packed or minified version and falls back
 * to the default version if neither of them were found.
 *
 * @param $name
 *   Library name.
 * @param $variants
 *   Array of allowed variants.
 *
 * @return
 *   An associative array of the library information as returned from libraries_info().
 *
 * @see libraries_load().
 */
function manualcrop_library_load($name, $variants = NULL) {
  $loaded = &drupal_static(__FUNCTION__, array());

  if (!isset($loaded[$name])) {
    $loaded[$name] = array();

    $library = cache_get($name, 'cache_libraries');
    if ($library) {
      $library = $library->data;
    }
    else {
      $library = libraries_detect($name);
      cache_set($name, $library, 'cache_libraries');
    }

    if (empty($variants)) {
      $variants = array('packed', 'minified', NULL);
    }

    foreach ((array) $variants as $variant) {
      if ($variant) {
        if (!empty($library['variants'][$variant]['installed'])) {
          $loaded[$name] = libraries_load($name, $variant);
          break;
        }
      }
      elseif ($library['installed']) {
        $loaded[$name] = libraries_load($name, $variant);
        break;
      }
    }

    return $loaded[$name];
  }
}
