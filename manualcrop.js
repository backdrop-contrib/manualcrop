var ManualCrop = { "overlay": null, "oldstate": null, "jcrop": null, "output": null };

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
	var jcropOptions = {
	  onSelect: ManualCrop.updateCoords,
      onChange: ManualCrop.updateCoords
	};

	// Get the destination field and current value.
	ManualCrop.output = $('#manualcrop-area-' + fid + '-' + styleSelect.val());
	ManualCrop.oldstate = ManualCrop.parseStringSelection(ManualCrop.output.val());

	// Create the overlay.
	ManualCrop.overlay = $('#manualcrop-overlay-' + fid).clone();
	ManualCrop.overlay.removeAttr('id');
	ManualCrop.overlay.removeClass('element-invisible');

	// Additional jcrop options.
	if (settings) {
	  if (settings.name == "manualcrop_crop_and_scale") {
		jcropOptions.aspectRatio = settings.data.width / settings.data.height;

		if (settings.data.respectminimum) {
		  jcropOptions.minSize = [settings.data.width, settings.data.height]
		}
	  }
	  else if (settings.name == "manualcrop_crop") {
		if (settings.data.width && settings.data.height) {
		  jcropOptions.minSize = [settings.data.width, settings.data.height];
		}
		else if (settings.data.width) {
		  jcropOptions.minSize = [settings.data.width, 0];
		}
		else if (settings.data.height) {
		  jcropOptions.minSize = [0, settings.data.height];
		}
	  }
	}

	// Current selection.
	if (ManualCrop.oldstate) {
	  jcropOptions.setSelect = [ManualCrop.oldstate.x, ManualCrop.oldstate.y, ManualCrop.oldstate.x2, ManualCrop.oldstate.y2];
	}

	// Create the Jcrop field.
	ManualCrop.jcrop = $('img.manualcrop-image', ManualCrop.overlay);
	jcropOptions.boxWidth = ManualCrop.jcrop.parent().width();
	jcropOptions.boxHeight = ManualCrop.jcrop.parent().height();
	ManualCrop.jcrop = ManualCrop.jcrop.Jcrop(jcropOptions);

	// Set the name of the style.
	$('.manualcrop-image-style', ManualCrop.overlay).text($('option:selected', styleSelect).text());

	// Add a handler for tressing ESC.
	ManualCrop.overlay.keydown(function(event) {
	  if (event.keycode == 27) {
		ManualCrop.resetSelection();
		ManualCrop.closeOverlay();
	  }
	});

	// Insert the overlay and reset the style selection.
	$("body").append(ManualCrop.overlay);
	styleSelect.val(-1);
  }
}

/**
 * Close the cropping overlay.
 */
ManualCrop.closeOverlay = function() {
  if (ManualCrop.overlay) {
	ManualCrop.jcrop.destroy()
	ManualCrop.overlay.remove();
	ManualCrop.overlay = null;
	ManualCrop.oldstate = null;
	ManualCrop.jcrop = null;
  }
}

/**
 * Reset the selection to the previous state.
 */
ManualCrop.resetSelection = function() {
  if (ManualCrop.overlay) {
	if (ManualCrop.oldstate) {
	  if (ManualCrop.jcrop.tellSelect()) {
		ManualCrop.jcrop.animateTo([ManualCrop.oldstate.x, ManualCrop.oldstate.y, ManualCrop.oldstate.x2, ManualCrop.oldstate.y2]);
	  }
	  else {
		ManualCrop.jcrop.setSelect([ManualCrop.oldstate.x, ManualCrop.oldstate.y, ManualCrop.oldstate.x2, ManualCrop.oldstate.y2]);
	  }
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
	ManualCrop.jcrop.release();
  }
}

/**
 * When a selection updates set the new coordinates to the output field.
 *
 * @param coords
 *   Array of coordinates.
 */
ManualCrop.updateCoords = function(coords) {
  if (ManualCrop.overlay) {
	ManualCrop.output.val(coords.x + ';' + coords.y + ';' + coords.w + ';' + coords.h);

	$('.manualcrop-selection-x', ManualCrop.overlay).text(coords.x);
	$('.manualcrop-selection-y', ManualCrop.overlay).text(coords.y);
	$('.manualcrop-selection-width', ManualCrop.overlay).text(coords.w);
	$('.manualcrop-selection-height', ManualCrop.overlay).text(coords.h);
  }
}

/**
 * Parse a string selection to a coordinates object.
 *
 * @param txtcoords
 *   The selection as a string e.a.: "x;y;width;height".
 * @return
 *   An object containing the selection coordinates.
 */
ManualCrop.parseStringSelection = function(txtcoords) {
  if (txtcoords) {
	var parts = txtcoords.split(';');
	var coords = {
	  x: Number(parts[0]),
	  y: Number(parts[1]),
	  w: Number(parts[2]),
	  h: Number(parts[3])
	};

	coords.x2 = coords.x + coords.w;
	coords.y2 = coords.y + coords.h;

	return coords;
  }

  return null;
}

})(jQuery);