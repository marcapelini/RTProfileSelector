RTProfileSelector ISO Profiles
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

An "ISO profile" is nothing more than an INI file named for a particular 
camera model name, and containing a "Profiles" section where the user can
associate ISO values to RT processing profiles with noise reduction parameters
suitable for a certain level of noise.

A "ISO profile" file should be named "iso.<CAMERA MODEL NAME>.ini", where
<CAMERA MODEL NAME> must be the exact value reported by ExifTool for the
field "Camera Model Name".  For example, the ISO profile for the Panasonic
GM1 would be named "iso.DMC-GM1.ini".

Example of ISO x profile association in the sample file "iso.DMC-GM1.ini":

	[Profiles]
	400=Noise Profiles/low_noise_partial.pp3
	640=Noise Profiles/mid_low_noise_partial.pp3
	1200=Noise Profiles/mid_noise_partial.pp3
	2000=Noise Profiles/high_noise_partial.pp3
	6400=Noise Profiles/very_high_noise_partial.pp3

This means that RTPS will look for a RT profile named "mid_low_noise_partial.pp3"
in the folder "Noise Profiles".  RTPS will look both in RT's custom profiles
folder, and, if the file is nor found, will look for it in its own "ISO Profiles"
folder.  

Since the processing profiles are ideally camera-independent, they can be 
declared for other cameras, with different ISO-dependent noise characteristics.
For example, my ISO profile for the the Panasonic LX-5 (a compact camera with
smaller and therefore noisier sensor) looks like this:

	[Profiles]
	80=Noise Profiles/low_noise_partial.pp3
	200=Noise Profiles/mid_low_noise_partial.pp3
	320=Noise Profiles/mid_noise_partial.pp3
	600=Noise Profiles/high_noise_partial.pp3
	1200=Noise Profiles/very_high_noise_partial.pp3

The .pp3 files are assumed to be partial profiles with specific settings related
to noise reduction. If that's not the case, that is, if your profiles are not 
partial, or contain settings that are not ISO/noise related, you can still achieve
the desired behaviour of affecting only noise-related settings by declaring a
"ISO Profile Sections" section in RTProfileSelector.ini, enabling the application
of settings only from some selected sections from any .pp3 file that is applied at
the "ISO profiles" stage:

	[ISO Profile Sections]
	Directional Pyramid Denoising=1
	Impulse Denoising=1
	RAW Bayer=1
	Sharpening=1

