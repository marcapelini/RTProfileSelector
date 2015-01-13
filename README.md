## RTProfileSelector #

RTProfileSelector is a [RawTherapee](http://rawtherapee.com/) plugin that automatically selects custom processing profiles (.pp3 files) based on user-defined rules. The rules are sets of Exif fields and values which are matched against the actual values extracted from the raw files the first time they are opened in RawTherapee.  

A few things you can automatise in RawTherapee through RTProfileSelector:
* Assign your own custom processing profiles to approximatelly match your camera settings (such as "monochrome"/"black & white", "vivid colour", "film modes", etc.)
* Set noise reduction parameters in RawTherapee according to camera model and ISO value
* Assign lens correction profiles (LCP) based on the lens and camera model used

RTProfileSelector is written in C++11 and compiles on both Windows and Linux. RTProfileSelector uses [ExifTool](http://www.sno.phy.queensu.ca/~phil/exiftool/) for extracting metadata from images.

For installation procedures and online documentation, please go to the project's [wiki section](https://github.com/marcapelini/RTProfileSelector/wiki).
