This is repository for file manager and collection creator for Sony e-readers originally hosted on [Sourceforge](http://sonyfmngr.sourceforge.net)

I am not sure this program is of any interest to anyone now, but history is a history...

Project was created and maintained by [the-ebook](http://www.the-ebook.org) and [MobileRead](http://www.mobileread.com) **Uroboros** (SourceForge.net user **yiselieren**).
Unfortunately his last version (version 1.11) was getting old. Since Sony was coming with new e-readers he suggested that starting with 
version 1.12 project should be maintained differently, so on Oct 26, 2010 I started **sonyfmgr** project on SourceForge.

For 1st and 2nd generation of Sony e-readers (505,300,600,700,900) you will need *mngr505*. For newer 3rd generation devices you will need *mngr650*

+ **Last mngr505 version is 1.12**

+ **Last mngr650 version is 1.17**

You could download binaries for various platforms (Windows, Linux, Mac) [here](https://github.com/rupor-github/sonyfmgr/releases)

#### Changes
##### 1.17 (mngr650 only)
 * Changed how path separators are handled during conversion process. Path separators will always be translated into OS native format.

##### 1.16 (mngr650 only)
 * Added new configuration setting: "Do not scan database directories". If selected - known directories used by device to keep miscelaneous files (thumbnails, layouts, e.t.c) would not be scanned for books. This speeds up collection creation significantly.

##### 1.15 (mngr650 only)
 * Fixed Windows build - there was a number of IO issues related to mix of static and dynamic run-times
 * Removed regular expression to shorten collection name. To avoid user confusion regular expression field is now empty
 * Fixed number of small cosmetic items

##### 1.14 (mngr650 only)
 * Generic settings are on  a single page now
 * There is no FB2LRF and EPUB converters - generic "Converter" page to run any program could be used instead
 * Added setting for separate EPUB Viewer
 * Completly removed internal EPUB conversion code
 * Fixed incompatibility of some viewers on Windows with Unix style path separators
 * Default FB2LRF converter on Windows switched to command line one (fb2lrf_c)

##### 1.13 (mngr650 only)
 * Cleaned menu - enumerate/unenumerate
 * Fixed configuration storing (str2ini and ini2str). EPUB profile now should work correctly.
 * Translation of underscores to spaces in collection names is now separate checkbox in configuration
 * Default regular expression for collection names will now attempt to shorten first part of resulting name (shorter collection names)
 * Implemented rudimentary thumbnail management (EPUB, LRF. FB2) to allow thumbnails generation during collection creation on computer - not on the device.

##### 1.12
 * Support for 3rd generation of Sony devices
 * Proper un-mounting of USB devices (Windows)
 * Better support for EPUB files
 * Various bug fixes

Enjoy!
