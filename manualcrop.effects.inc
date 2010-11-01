<?php
/**
 * @file
 * Functions needed to execute the manual crop image effects.
 */

/**
 * Implements hook_image_effect_info().
 */
function manualcrop_image_effect_info() {
  return array(
    'manualcrop_crop_and_scale' => array(
      'label' => t('Crop and scale manually'),
      'help' => t('Crop and scale a user-selected area, respecting the ratio of the destination width and height.'),
      'effect callback' => 'manualcrop_crop_and_scale_effect',
      'form callback' => 'manualcrop_crop_and_scale_form',
      'summary theme' => 'manualcrop_crop_and_scale_summary',
    ),
    'manualcrop_crop' => array(
      'label' => t('Manual crop'),
      'help' => t('Crop a freely user-selected area.'),
      'effect callback' => 'manualcrop_crop_effect',
      'form callback' => 'manualcrop_crop_form',
      'summary theme' => 'manualcrop_crop_summary',
    ),
  );
}

/**
 * Image effect callback; Crop and scale an image resource.
 *
 * @param $image
 *   An image object returned by image_load().
 * @param $data
 *   An array of attributes to use when performing the manual crop and scale effect
 *   with the following items:
 *   - "width": An integer representing the desired width in pixels.
 *   - "height": An integer representing the desired height in pixels.
 *   - "upscale": A Boolean indicating that the image should be upscalled if
 *     the dimensions are larger than the original image.
 * @return
 *   TRUE on success. FALSE on failure to crop and scale image.
 * @see image_crop_effect()
 * @see image_scale_effect()
 */
function manualcrop_crop_and_scale_effect(&$image, $data) {
  // The width and height will be possibly overwritten for the cropping, so copy the data array for later.
  $scale_data = $data;
  $crop = manualcrop_load_crop_selection($image->source, $data["isid"]);

  if ($crop) {
    // Only crop if a crop was applied.
    $data["width"] = $crop->width;
    $data["height"] = $crop->height;
    $data["anchor"] = $crop->x . "-" . $crop->y;

    if (!image_crop_effect($image, $data)) {
      return FALSE;
    }
  }

  return image_scale_effect($image, $scale_data);
}

/**
 * Image effect callback; Crop an image resource.
 *
 * @param $image
 *   An image object returned by image_load().
 * @param $data
 *   An array of attributes to use when performing the manual crop effect with the
 *   following items:
 *   - "width": An integer representing the desired width in pixels.
 *   - "height": An integer representing the desired height in pixels.
 *   - "upscale": A Boolean indicating that the image should be upscalled if
 *     the dimensions are larger than the original image.
 * @return
 *   TRUE on success. FALSE on failure to crop image.
 * @see image_crop_effect()
 */
function manualcrop_crop_effect(&$image, $data) {
  $crop = manualcrop_load_crop_selection($image->source, $data["isid"]);

  if ($crop) {
    $data["width"] = $crop->width;
    $data["height"] = $crop->height;
    $data["anchor"] = $crop->x . "-" . $crop->y;

    return image_crop_effect($image, $data);
  }

  return TRUE;
}