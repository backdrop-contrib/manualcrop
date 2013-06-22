Manual Crop
===========

The Manual Crop module exposes a set of image style effects that allow you
to crop (and scale) an image after uploading.

Dependencies
------------
- Libraries 2.x
- jQuery plugins:
    - imagesLoaded: http://desandro.github.io/imagesloaded
    - imgAreaSelect: http://odyniec.net/projects/imgareaselect

Installation
------------
Start by downoading and installing the Libraries 2.x module.

Next download the imagesLoaded plugin and rename it to "jquery.imagesloaded.min.js"
or "jquery.imagesloaded.js" if you chose the uncompressed version. Copy this file
to "sites/all/libraries/jquery.imagesloaded".

Please note that the 3.x version depends on jQuery 1.5, so because Drupal ships
with jQuery 1.4 you can either update jQuery with the jQuery update module or
download an older plugin version. You can find the lastest 2.x version here:
https://github.com/desandro/imagesloaded/tree/0e68c8ea901cdec8844d4ceca2df8efb8a7a75a5

If you're using the minified 2.x version, you'll need to copy the file header from
the uncompressed or the Libraries module won't be able to detect the plugin version.

Now download and extract the imgAreaSelect plugin, rename the newly created folder
to "jquery.imgareaselect" and copy it to "sites/all/libraries".

When finished you can finally activate the module via the Modules page!

Configuration
-------------

After installing the module you need to configure your image styles before you
can start cropping. Go to Administration » Configuration » Media » Image styles
and click on the "edit" link for the styles that need a Manual Crop effect.

Add and configure one of the Manual Crop effects, you'll notice that the Manual
Crop effect will always become the first effect in the list. This is because
cropping should always be done first, otherwise the result will be unpredictable.

Next go to Administration » Structure » Content types and click on the "manage fields"
link (the Field UI module should be activated) for the content type that should
allow cropping. Now click on the "edit" link of the image field, so you can enable
and configure Manual Crop (open the "Manual Crop" fieldset) for the current field.

After saving the settings you should return to the content type overview and click
on "manage display" so you can set the (cropped) image style that should be used.
