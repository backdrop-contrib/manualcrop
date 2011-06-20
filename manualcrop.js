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

  $('.manualcrop-style-select option[value="').addClass("manualcrop-style-option-empty");
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

    // Get the image and the dimensions, don't use the ManualCrop.overlay cloned one to get the CSS in Webkit.
    var image = $('#manualcrop-overlay-' + fid + ' img.manualcrop-image');
    var width = ManualCrop.parseInt(image.attr('width'));
    var height = ManualCrop.parseInt(image.attr('height'));

    // Scale the image to fit the maximum width and height (the visible part of the page).
    var newWidth = width;
    var maxWidth = browserWidth - ManualCrop.parseInt(image.css('marginLeft')) - ManualCrop.parseInt(image.css('marginRight'));
    var newHeight = height;
    var maxHeight = browserHeight - ManualCrop.parseInt(image.css('marginTop')) - ManualCrop.parseInt(image.css('marginBottom'));

    if(newWidth > maxWidth) {
      newHeight = Math.floor((newHeight * maxWidth) / newWidth);
      newWidth = maxWidth;
	}

    if(newHeight > maxHeight) {
      newWidth = Math.floor((newWidth * maxHeight) / newHeight);
      newHeight = maxHeight;
    }

    // Set the new width and height to the cloned image.
    image = $('img.manualcrop-image', ManualCrop.overlay);
    image.css('width', newWidth + 'px');
    image.css('height', newHeight + 'px');

    // Basic imgAreaSelect options.
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
    styleSelect.val('');
    styleSelect.blur();

    // Append the cropping area (last, to prevent that '_11' is undefinded).
    $("body").append(ManualCrop.overlay);

    // Create the cropping tool.
    ManualCrop.widget = image.imgAreaSelect(options);

    // Set the initian selection.
    if (ManualCrop.oldSelection) {
      ManualCrop.resetSelection();
    }

    // Handle keyboard shortcuts.
    $(document).keyup(ManualCrop.handleKeyboard);
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

    $(document).unbind('keyup', ManualCrop.handleKeyboard);
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
 * Keyboard shortcuts handler.
 *
 * @param e
 *    The event object.
 */
ManualCrop.handleKeyboard = function(e) {
  if (ManualCrop.overlay) {
    if(e.keyCode == 27) {
      ManualCrop.closeOverlay();
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
      x1: ManualCrop.parseInt(parts[0]),
      y1: ManualCrop.parseInt(parts[1]),
      width: ManualCrop.parseInt(parts[2]),
      height: ManualCrop.parseInt(parts[3])
    };

    selection.x2 = selection.x1 + selection.width;
    selection.y2 = selection.y1 + selection.height;

    return selection;
  }

  return null;
}

/**
 * Parse a textual number to an integer.
 *
 * @param integer
 *   The textual integer.
 * @return
 *   The integer
 */
ManualCrop.parseInt = function(integer) {
  return (parseInt(integer) || 0)
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

    if (option.has("span").length == 0) {
      option.html(option.html() + '<span> ' + Drupal.t('(cropped)') + '</span>');
    }

    if ($('option', select).not('.manualcrop-style-option-cropped').length == 1) {
      // All styles have been cropped.
      select.addClass('manualcrop-style-select-cropped');
    }
  } else {
    // Style not cropped.
    $('span', option).remove();
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