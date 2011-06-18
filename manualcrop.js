var ManualCrop = {"overlay": null, "oldSelection": null, "widget": null, "output": null};

(function ($) {

/**
 * Mark required image styles and trigger the onchange event of all hidden fields
 * that hold crop data, this way all css classes in the selection lists will be updated.
 */
$(document).ready(function () {
  for (var key in Drupal.settings.manualCrop.required) {
    $('.manualcrop-style-select option[value="' + Drupal.settings.manualCrop.required[key] + '"]').addClass("manualcrop-style-option-required");
  }

  $('.manualcrop-cropdata').trigger('change');
});

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
    $(document).keyup(function(e) {
      // Close the overlay when someone presses escape
      if(e.keyCode == 27) { ManualCrop.closeOverlay(); }
    });

    var browserWidth = $(window).width();
    var browserHeight = $(window).height();

    var styleSelect = $(select);
    var settings = Drupal.settings.manualCrop.styles[styleSelect.val()] || {};

    // Get the destination field and the current selection.
    ManualCrop.output = $('#manualcrop-area-' + fid + '-' + styleSelect.val());
    ManualCrop.oldSelection = ManualCrop.parseStringSelection(ManualCrop.output.val());

    // Create the overlay.
    ManualCrop.overlay = $('#manualcrop-overlay-' + fid).clone();
    ManualCrop.overlay.removeAttr('id');
    ManualCrop.overlay.removeClass('element-invisible');
    ManualCrop.overlay.css('width', browserWidth + 'px');
    ManualCrop.overlay.css('height', browserHeight + 'px');

    // Get the image and the dimensions.
    // We have to use this, instead of the ManualCrop.overlay cloned one to get the CSS in Chrome
    var image = $('img.manualcrop-image'); //, ManualCrop.overlay);
    //image.load(); // here's why http://stackoverflow.com/questions/318630/get-real-image-width-and-height-with-javascript-in-safari-chrome
    var width = parseInt(image.attr('width')) || 0;
    var height = parseInt(image.attr('height')) || 0;

    // Scale the image to fit the maximum width and height (the visible part of the page).
    var newWidth = width;
    var maxWidth = browserWidth - (parseInt(image.css('marginLeft')) || 0) - (parseInt(image.css('marginRight')) || 0);
    var newHeight = height;
    var maxHeight = browserHeight - (parseInt(image.css('marginTop')) || 0) - (parseInt(image.css('marginBottom')) || 0);

    if(newWidth > maxWidth) {
      newHeight = Math.floor((newHeight * maxWidth) / newWidth);
      newWidth = maxWidth;
	  }

	  if(newHeight > maxHeight) {
	    newWidth = Math.floor((newWidth * maxHeight) / newHeight);
	    newHeight = maxHeight;
	  }

    //alert("bWidth: " + browserWidth + ", nWidth: " + newWidth + ", mWidth: " + maxWidth + ", bHeight: " + browserHeight + ", nHeight: " + newHeight + ", mHeight: " + maxHeight);
    image = $('img.manualcrop-image', ManualCrop.overlay);
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
    ManualCrop.output.trigger('change');

    ManualCrop.widget.setOptions({remove: true});
    ManualCrop.overlay.remove();
    ManualCrop.overlay = null;
    ManualCrop.oldSelection = null;
    ManualCrop.widget = null;
    ManualCrop.output = null;
  }
}

/**
 * Reset the selection to the previous state.
 */
ManualCrop.resetSelection = function() {
  if (ManualCrop.overlay && ManualCrop.oldSelection) {
    ManualCrop.widget.setSelection(ManualCrop.oldSelection.x1, ManualCrop.oldSelection.y1, ManualCrop.oldSelection.x2, ManualCrop.oldSelection.y2);
    ManualCrop.widget.setOptions({hide: false, show: true});
    ManualCrop.widget.update();
    ManualCrop.updateSelection(null, ManualCrop.oldSelection);

    // Hide reset button.
    $(".manualcrop-reset", ManualCrop.overlay).hide();
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

    if (ManualCrop.oldSelection) {
      // Show reset button.
      $(".manualcrop-reset", ManualCrop.overlay).show();
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
      x1: parseInt(parts[0]) || 0,
      y1: parseInt(parts[1]) || 0,
      width: parseInt(parts[2]) || 0,
      height: parseInt(parts[3]) || 0
    };

    selection.x2 = selection.x1 + selection.width;
    selection.y2 = selection.y1 + selection.height;

    return selection;
  }

  return null;
}

/**
 * A new cropping area was saved to the hidden field, find the corresponding
 * select option and append a css class to indicate the crop status.
 *
 * This is a seperate function so it can be triggered after loading.
 *
 * @param element
 *   The hidden field that stores the selection.
 * @param fid
 *   The file id.
 * @param styleName
 *
 */
ManualCrop.selectionStored = function(element, fid, styleName) {
  var select = $('.manualcrop-style-select-' + fid);
  var option = $('.manualcrop-style-select-' + fid + " option[value='" + styleName + "']");

  // Check if the style has been cropped.
  if ($(element).val()) {
    option.addClass('manualcrop-style-option-cropped');

    if ($('option', select).not('.manualcrop-style-option-cropped').length == 1) {
      // All styles have been cropped.
      select.addClass('manualcrop-style-select-cropped');
    }
  }
  else {
    // Style not cropped.
    option.removeClass('manualcrop-style-option-cropped');
    select.removeClass('manualcrop-style-select-cropped');
  }

  // Uncropped required styles in the select?
  if ($("option.manualcrop-style-option-required", select).not('.manualcrop-style-option-cropped').length > 0) {
    select.addClass('manualcrop-style-select-required');
  } else {
    select.removeClass('manualcrop-style-select-required');
  }
}

})(jQuery);
