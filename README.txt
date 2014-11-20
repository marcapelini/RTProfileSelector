////////////////////////////////////////////////////////////////////////////////////

RTProfileSelector
~~~~~~~~~~~~~~~~~

RTProfileSelector is a RawTherapee custom profile builder plugin that automatically 
selects custom processing profiles(.pp3 files) based on user - defined rules. The 
rules are sets of Exif fields and values which are matched against the actual 
values extracted from the RAW files.

Copyright 2014 Marcos Capelini 

///////////////////////////////////////////////////////////////////////////////////

I wrote RTProfileSelector (RTPS from now on for short) initially to perform a 
simple task: I wanted RT to detect that a RAW file had been shot in monochrome 
and as a result select one of my custom black & white profiles, so that I could 
be spared seeing the colour version of a picture I originally intended to be B&W. :)  
Don't get me wrong: I like having the option of switching between a colour and 
B&W rendition of a photo at any time, but I find it a lot more convenient to not 
have to manually set my B&W photos to B&W as the first step of my workflow in RT.

Anyway, it all started when I posted a comment on RT's user forum complaining
about the fact that I couldn't assign an automatic B&W profile to my monochrome
pictures opened in RhawTherapee. It was suggested to me that I should learn 
scripting and use an external tool (like exiftool) to help me assign a proper
profile:
		http://rawtherapee.com/forum/viewtopic.php?f=2&t=5575

Since I'm not familiar enough with scripting languages, I opted for writing 
a program in C++ instead . Even though I am primarily a Windows programmer, 
I tried my best to make sure RTProfileSelector is portable at least to Ubuntu 14. 
The code was compiled and tested quickly on that platform just to make sure it 
works. 
			
While I was at it, it occurred to me that the second thing that bugs me the most
in my workflow, besides the aforementioned issue with opening B&W images, is 
having to manually adjust the distortion correction amount for my lenses.  I 
shoot almost only with Micro Four Thirds cameras and lenses, which rely heavily 
on software correction of lens distortion (there's CA and vignetting correction 
too, but these don't bother me so much). I know there's work in progress to integrate 
LensFun into RT, and that RT already supports LCP lens profiles, but I thought it 
might be useful to also have a quick and dirty solution for some lenses for which 
I couldn't find a proper LCP profile. Just automatically setting the amount of 
distortion correction in RT proved sufficient for my current needs. 
 
In concept RTPS is similar to RT_ChooseProfile in that it aims at selecting/choosing 
a proper custom processing profile (.pp3) in RT based on Exif fields extracted 
from the image. There are a number of important differences in their approaches:

- RTPS can use any Exif field known to exiftool, not only those informed by 
  RawTherapy when invoking a custom profile builder.
- RTPS rules file is a simple INI-style text file where each section corresponds 
  to a particular custom profile, and the entries in that section are the keys 
  and values as reported by exiftool. I find this easier to edit than single-line
  rules.
- RTPS is written in C++, and once you have the compiled binary, you don't need any 
  other external dependency to run it (except for exiftool - see below). 
- As a "bonus", RTPS can provide some crude form of lens correction without
  the need of having one profile for each focal length.

Lastly, right now RTPS is the result of little more than a weekend's worth of coding, 
and has had very limited testing. I have no idea how it will behave with RAW files
from different makers and models, under different operating systems and environments.
RTPS provides very little feedback should anything go wrong. If it behaves unexpectedly,
or doesn't seem to do what it should, you may try looking at RTProfileSelector.log file
in the same folder as the executable.  Also make sure 'exiftool' is available from the 
command line, and, if running on Windows, that you renamed it properly (see below).

If there's enough interest in this plugin, I'll try my best to improve it and correct
any reported problems.  Eventually, I would like to see something like this built into 
RawTherapee's interface, with some sort of UI for creating and managing rules and
assigning them to custom profiles. Right now, I'm pretty happy with what I have achieved,
and am busy creating rules for my RAW files, new and old (I have been shooting RAW
since 2003, have had a number of cameras which have given me many thousands of photos!)
  
How to set up RTProfileSelector:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Place RTPS binary and configuration files somewhere on your computer
- Download/install 'exiftool'. Follow instructions at http://www.aldeid.com/wiki/Exiftool 
	For Windows, you should download exiftool (zip file) from: 
		http://www.sno.phy.queensu.ca/~phil/exiftool/index.html
		Just unzip and place "exiftool(-k).exe" somewhere (I recommend that you place
		it on the same folder as RTProfileSelector itself) and rename it "exiftool.exe"
	For Ubuntu, I suggest you install from packages:
		$ sudo apt-get install perl
		$ sudo apt-get install libimage-exiftool-perl
- Make sure you have a number of custom profiles in RawTherapee, and that AT LEAST ONE 
  of them is set as the "Default Processing Profile" in RT's preferences. This is needed
  for RTPS automatically detecting the folder where RT stores custom profiles (you can
  override this in RTProfileSelector.ini).
- Also, be sure RTPS (full path + executable) is set as your "Custom Processing 
  Profile Builder" in RT's preferences (e.g.: "C:\RTPS\RTProfileSelector.exe" on 
  my Windows machine, "/home/mc/Tools/RTProfileSelector/RTProfileSelector" on my 
  Ubuntu machine)

RTProfileSelector.ini:
~~~~~~~~~~~~~~~~~~~~~~

That's the configuration file for RTProfileSelector where you can override a number
of default settings. See comments in that file for a list of configuration entries.

One entry in particular is very useful to help you start writing your own rules:

	[General] 
	ViewExifKeys=1

If 'ViewExifKeys' is enabled ('=1'), RTPS will open a text file with the the Exif
keys and values for the current RAW file every time it is invoked by RT. Then all
you need to do to write your rule is copy the selected entries from the file to 
appropriate section in RTProfileSelectorRules.ini.  Available fields differ wildly
for RAW files from different makers and cameras, so it may take a little trial and 
error until you have your rules right, but at least with 'ViewExifKeys' enabled
you will have the entries in a ready to use format.   
	
During the process of writing a rule, it is useful to run RTPS several times for 
the same image in RawTherapee, as you try to find the right Exif fields and values.
One easy way to do this in RawTherapee is through its context menu: click on a 
preview image, select "Processing profile operations|Custom Profile Builder" and
RTPS should run right away if you configured it properly.
	
RTProfileSelectorRules.ini:
~~~~~~~~~~~~~~~~~~~~~~~~~~~

RTProfileSelectorRules.ini is a simple INI-style file where each section
must be named for one of your custom profiles, and the entries are the 
keys and respective values which you want to match the particular profile.

For example, I wanted that whenever I opened a RAW file from my Panasonic
GM1 camera which had been shot in one of the "monochrome" modes of the camera, 
my custom "Generic BW.pp3" profile was assigned and the photo started as B&W
in RT. My RTProfileSelectorRules.ini ended up with a section like this:

	[Generic BW.pp3]
	Camera Model Name=DMC-GM1
	Photo Style=Monochrome

Pictures not taken in "monochrome" or from other cameras are unaffected by this
setting, and will use the default custom profile I assigned in RT.
  
There can be as many sections as you wish, and you can have more than one section
with the same name, but with a different set of Exif fields (that is, different 
rules), if you want to set up the same profile for use with different 
cameras, or for a different combination of Exif fields. For example, I also have 
some RAW files from my old Panasonic GF1, and I wanted them too to be assigned the 
same "Generic BW.pp3", so I added one more section named "[Generic BW.pp3]" to 
the rules file:

	[Generic BW.pp3]
	Camera Model Name=DMC-GF1
	Color Effect=Black & White  

Notice in the example above that the GF1 marks B&W photos quite differently
from the GM1: with a different field name an value! That's why I needed 
the flexibility of using any available Exif field for defining the rules.

Only the profiles for which ALL fields have identical values to the ones in 
a particular RAW file are considered as "matches" and therefore selectable. 

If there are different rules which match a particular image file, RTPS will 
pick up the most restrictive one, that is, the one with the greater number of 
filtering fields.  

Contact
~~~~~~~

If you want to reach me, here's my suggestion:

Send me an email: 
	marcapelini.dev@gmail.com (checked occasionally)
	
Pay a visit to my blog (in Portuguese) and leave a message: 
	http://fotorabiscos.blogspot.com.br/

