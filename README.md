#Manual Crop

_IN PROCESS, CURRENTLY INCOMPLETE_

The Manual Crop module exposes a set of image style effects that allow you
to crop (and scale) an image after uploading.

![Manual Crop](https://github.com/backdrop-contrib/manualcrop/blob/1.x-1.x/images/manualcrop-image-effect.png "Manual Crop image effect example")


##Dependencies

- Libraries 2.x
- jQuery plugins:
    - imagesLoaded:
        + Website: http://desandro.github.io/imagesloaded
        + Download: https://github.com/desandro/imagesloaded/archive/v3.2.0.zip
    - imgAreaSelect:
        + Website: http://odyniec.net/projects/imgareaselect
        + Download: https://github.com/odyniec/imgareaselect/archive/v0.9.11-rc.1.tar.gz

##Installation

 - Start by downloading and installing the Libraries module.
 - Next download and extract the imagesLoaded plugin, rename the extracted folder to
"jquery.imagesloaded" and place it under "libraries". The plugin should
now be located at `libraries/jquery.imagesloaded/imagesloaded.pkgd.min.js`.
 - Now download and extract the imgAreaSelect plugin, rename extracted folder to
"jquery.imgareaselect" and copy it into "libraries". The plugin should
now be located at `libraries/jquery.imgareaselect/jquery.imgareaselect.dev.js`.
 - Install this module using the official 
  [Backdrop CMS instructions](https://backdropcms.org/guide/modules)

##Configuration and Usage

After installing the module you need to configure your image styles before you
can start cropping. Go to `Administration » Configuration » Media » Image styles`
and click on the "edit" link for the styles that need a Manual Crop effect.

Add and configure one of the Manual Crop effects. You'll notice that the Manual
Crop effect will always become the first effect in the list. This is because
cropping should always be done first, otherwise the result will be unpredictable.

Next go to `Administration » Structure » Content types` and click on the `manage fields`
link (the Field UI module should be activated) for the content type that should
allow cropping. Now click on the `edit` link of the image field, so you can enable
and configure Manual Crop (open the "Manual Crop" fieldset) for the current field.

After saving the settings you should return to the content type overview and click
on `manage display` so you can set the (cropped) image style that should be used.

Manual Crop adds a "?c=md5_hash" query string parameter to the image url so the
at client-side cached image gets refreshed whenever the crop selection changes.
To prevent an SEO impact, this can be disabled by unchecking the
`Reload cache-control` setting at `admin/config/media/manualcrop`.

##Issues

Bugs and Feature requests should be reported in the 
[Issue Queue](https://github.com/backdrop-contrib/manualcrop/issues)

##Current Maintainers

 - [Laryn Kragt Bakker](https://github.com/laryn) - [CEDC.org](https://cedc.org)
 - Help wanted!

##Credits

- Ported to Backdrop CMS by [Laryn Kragt Bakker](https://github.com/laryn) - [CEDC.org](https://cedc.org).
- Current maintainer for the Drupal module: [Matthijs](https://www.drupal.org/u/matthijs).

##License

This project is GPL v2 software. See the [LICENSE.txt](https://github.com/backdrop-contrib/manualcrop/blob/1.x-1.x/LICENSE.txt) 
file in this directory for complete text.