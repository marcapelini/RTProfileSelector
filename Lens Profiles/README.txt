RTProfileSelector Lens Profiles
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For this version of RTProfileSelector, a "lens profile" definition file 
must be named with the "Lens ID" field value as reported by exiftool.
If a "Lens ID" field is not found, RTProfileSelector will look for
the "Lens Type" field instead. Note that fixed lens cameras (e.g. compact 
cameras) usually don't have a "Lens ID", so in this case you will have to 
use the "Camera Model Name" field as the file name for the lens profile file.

This "lens profile" feature is meant as a quick-and-dirty solution for achieving 
some level of automatic lens correction in RT, in case you can't find a good 
LCP lens profile for your lens, and would rather not bother making your own. :)

A lens profile definition is just a plain INI-style file with a section 
named "[Distortion]", followed by one or more entries where each key is 
the actual focal length for which a distortion correction amount is to be
applied. The values for the correction amount can be obtained by shooting
a number of test photos at different focal lengths (in case of a zoom lens),
opening the images in RawTherapee, adjusting the "Amount" control under the 
"Distortion Correction" option in the "Transform" tab in RT so that the
distortion for each image is corrected as best as possible, and taking note
of the amount applied for each focal length. Then simply write those values
in a file named for the lens ID (or camera, if its a fixed-lens camera) and
with a .ini extension. 

Example for the Panasonic Lumix PZ 14-42 F3.5/5.6 lens. Contents of 
file "LUMIX G VARIO PZ 14-42mm F3.5-5.6.ini"

	[Distortion]
	14=-0.150
	16=-0.105
	18=-0.100
	21=-0.043
	25=-0.031
	30=0.000
	33=0.000
	36=0.018
	42=0.021

I have provided some sample files for a few Micro 4/3 Panasonic lenses, 
and at least one compact camera, but the actual values in these files may 
not be correct, as I only performed a quick test session and tried to 
visually adjust the needed correction amount for writing the examples. 
These files are just for reference on how you can write your own simple
correction profiles for any lens you have. 

RTProfileSelector will try to find the best match for the actual focal length
recorded in the RAW file. If no matching FL is found in the "[Distortion]", 
section, RTProfileSelector will interpolate over the nearest values. If the 
actual focal length is below the minimum or above the maximum in the range, 
RTProfileSelector will just use the value from the nearest FL.

Lastly, focal length values need not be declared in order, as RTProfileSelector 
will sort them internally anyway when processing the file. 
