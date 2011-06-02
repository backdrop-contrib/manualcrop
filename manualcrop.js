var ManualCrop = {"overlay": null, "oldSelection": null, "widget": null, "output": null};

(function ($) {

/**
 * Open the cropping overlay for an image.
 *
 * @param select
 *   The image styles selection list, triggering this event.
 * @param fid
 *   The file id of the image the user is about to crop.
 */
ManualCrop.showOverlay = function(select, fid) {
  if (!ManualCrop.overlay) {
    var styleSelect = $(select);
    var settings = Drupal.settings.manualCrop[styleSelect.val()] || {};

    // Get the destination field and the current selection.
    ManualCrop.output = $('#manualcrop-area-' + fid + '-' + styleSelect.val());
    ManualCrop.oldSelection = ManualCrop.parseStringSelection(ManualCrop.output.val());

    // Create the overlay.
    ManualCrop.overlay = $('#manualcrop-overlay-' + fid).clone();
    ManualCrop.overlay.removeAttr('id');
    ManualCrop.overlay.removeClass('element-invisible');
    ManualCrop.overlay.css('width', $(window).width() + 'px');
    ManualCrop.overlay.css('height', $(window).height() + 'px');

    // Get the image and the dimensions.
    var image = $('img.manualcrop-image', ManualCrop.overlay);
    var width = parseInt(image.attr('width'));
    var height = parseInt(image.attr('height'));

    // Scale the image to fit the maximum width and height (the visible part of the page).
    var newWidth = width;
    var maxWidth = $(window).width() - parseInt(image.css('margin-left')) - parseInt(image.css('margin-right'));
    var newHeight = height;
    var maxHeight = $(window).height() - parseInt(image.css('margin-top')) - parseInt(image.css('margin-bottom'));

    if(newWidth > maxWidth) {
      newHeight = Math.floor((newHeight * maxWidth) / newWidth);
      newWidth = maxWidth;
	}

	if(newHeight > maxHeight) {
	  newWidth = Math.floor((newWidth * maxHeight) / newHeight);
	  newHeight = maxHeight;
	}

    image.css('width', newWidth + 'px');
    image.css('height', newHeight + 'px');

    // Basic options.
    var options = {
      handles: true,
      instance: true,
      keys: true,
      parent: image.parent(),
      imageWidth: width,
      imageHeight: height,
      onSelectChange: ManualCrop.updateSelection
    };

    // Additional options based upon the effect.
    if (settings) {
      switch (settings.effect) {
        // Manual crop and scale effect.
        case 'manualcrop_crop_and_scale':
          options.aspectRatio = settings.data.width + ':' + settings.data.height;

          if (settings.data.respectminimum) {
            // Crop at least the minimum.
            options.minWidth = settings.data.width;
            options.minHeight = settings.data.height;
          }
          break;

        // Manual crop effect
        case 'manualcrop_crop':
          if (settings.data.width) {
            options.minWidth = settings.data.width;
          }
          if (settings.data.height) {
            options.minHeight = settings.data.height;
          }
      }
    }

    // Set the image style name.
    $('.manualcrop-image-style', ManualCrop.overlay).text($('option:selected', styleSelect).text());

    // Reset the image style selection list.
    styleSelect.val(-1);
    styleSelect.blur();

    // Append the cropping area (last, to prevent that '_11' is undefinded).
    $("body").append(ManualCrop.overlay);

    // Create the cropping tool.
    ManualCrop.widget = image.imgAreaSelect(options);

    // Set the initian selection.
    if (ManualCrop.oldSelection) {
      ManualCrop.resetSelection();
    }
  }
}

/**
 * Close the cropping overlay.
 */
ManualCrop.closeOverlay = function() {
  if (ManualCrop.overlay) {
    ManualCrop.widget.setOptions({remove: true});
    ManualCrop.overlay.remove();
    ManualCrop.overlay = null;
    ManualCrop.oldSelection = null;
    ManualCrop.widget = null;
  }
}

/**
 * Reset the selection to the previous state.
 */
ManualCrop.resetSelection = function() {
  if (ManualCrop.overlay) {
    if (ManualCrop.oldSelection) {
      ManualCrop.widget.setSelection(ManualCrop.oldSelection.x1, ManualCrop.oldSelection.y1, ManualCrop.oldSelection.x2, ManualCrop.oldSelection.y2);
      ManualCrop.widget.setOptions({hide: false, show: true});
      ManualCrop.widget.update();
      ManualCrop.updateSelection(null, ManualCrop.oldSelection);
    }
    else {
      ManualCrop.clearSelection();
    }
  }
}

/**
 * Remove the selection.
 */
ManualCrop.clearSelection = function() {
  if (ManualCrop.overlay) {
    ManualCrop.widget.setOptions({hide: true, show: false});
    ManualCrop.widget.update();
    ManualCrop.updateSelection();
  }
}

/**
 * When a selection updates write the position and dimensions to the output field.
 *
 * @param image
 *   Reference to the image thats being cropped.
 * @param selection
 *   Object defining the current selection.
 */
ManualCrop.updateSelection = function(image, selection) {
  if (ManualCrop.overlay) {
    if (selection && selection.width && selection.height && selection.x1 >= 0 && selection.y1 >= 0) {
      ManualCrop.output.val(selection.x1 + '|' + selection.y1 + '|' + selection.width + '|' + selection.height);

      $('.manualcrop-selection-x', ManualCrop.overlay).text(selection.x1);
      $('.manualcrop-selection-y', ManualCrop.overlay).text(selection.y1);
      $('.manualcrop-selection-width', ManualCrop.overlay).text(selection.width);
      $('.manualcrop-selection-height', ManualCrop.overlay).text(selection.height);
    }
    else {
      ManualCrop.output.val('');

      $('.manualcrop-selection-x', ManualCrop.overlay).text('-');
      $('.manualcrop-selection-y', ManualCrop.overlay).text('-');
      $('.manualcrop-selection-width', ManualCrop.overlay).text('-');
      $('.manualcrop-selection-height', ManualCrop.overlay).text('-');
    }
  }
}

/**
 * Parse a string defining the selection to an object.
 *
 * @param txtSelection
 *   The selection as a string e.a.: "x|y|width|height".
 * @return
 *   An object containing defining the selection.
 */
ManualCrop.parseStringSelection = function(txtSelection) {
  if (txtSelection) {
    var parts = txtSelection.split('|');
    var selection = {
      x1: parseInt(parts[0]),
      y1: parseInt(parts[1]),
      width: parseInt(parts[2]),
      height: parseInt(parts[3])
    };

    selection.x2 = selection.x1 + selection.width;
    selection.y2 = selection.y1 + selection.height;

    return selection;
  }

  return null;
}

})(jQuery);