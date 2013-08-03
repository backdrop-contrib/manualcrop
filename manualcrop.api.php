<?php
/**
 * @file
 * API documentation for Manual Crop
 */

/**
 * Implements hook_manualcrop_supported_widgets_alter().
 *
 * Allow other modules to Manual Crop and extend supported widget types.
 */
function hook_manualcrop_supported_widgets_alter(&$widgets) {
  // Widget name + optional settings.
  $widgets['widget_name'] = array(
    'thumbnails',
  );
}
