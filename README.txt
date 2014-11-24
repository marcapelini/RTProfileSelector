////////////////////////////////////////////////////////////////////////////////////

RTProfileSelector
~~~~~~~~~~~~~~~~~

RTProfileSelector is a RawTherapee custom profile builder plugin that automatically 
selects custom processing profiles(.pp3 files) based on user - defined rules. The 
rules are sets of Exif fields and values which are matched against the actual 
values extracted from the raw files.

Copyright 2014 Marcos Capelini 

///////////////////////////////////////////////////////////////////////////////////

I wrote RTProfileSelector (RTPS from now on for short) initially to perform a 
simple task: I wanted RT to detect that a raw file had been shot in monochrome 
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
a program in C++ instead. Even though I am primarily a Windows programmer, 
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

- RTPS can use any Exif field known to ExifTool, not only those informed by 
  RawTherapy when invoking a custom profile builder.
- RTPS rules file is a simple INI-style text file where each section corresponds 
  to a particular custom profile, and the entries in that section are the keys 
  and values as reported by exiftool. I find this easier to edit than single-line
  rules.
- RTPS is written in C++, and once you have the compiled binary, you don't need any 
  other external dependency to run it (except for ExifTool - see below). 
- As a "bonus", RTPS can provide some crude form of lens correction without
  the need of having one profile for each focal length.

Lastly, right now RTPS is the result of little more than a weekend's worth of coding, 
and has had very limited testing. I have no idea how it will behave with raw files
from different makers and models, under different operating systems and environments.
RTPS provides very little feedback should anything go wrong. If it behaves unexpectedly,
or doesn't seem to do what it should, you may try looking at RTProfileSelector.log file
in the same folder as the executable.  Also make sure 'exiftool' is available from the 
command line, and, if running on Windows, that you renamed it properly (see below).

If there's enough interest in this plugin, I'll try my best to improve it and correct
any reported problems.  Eventually, I would like to see something like this built into 
RawTherapee's interface, with some sort of UI for creating and managing rules and
assigning them to custom profiles. Right now, I'm pretty happy with what I have achieved,
and am busy creating rules for my raw files, new and old (I have been shooting raw
since 2003, have had a number of cameras which have given me many thousands of photos!)

  
How to set up and use RTProfileSelector:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

There are tutorials on how to install and use RTProfileSelector on the wiki section
of the GitHug repository:
 
	https://github.com/marcapelini/RTProfileSelector/wiki


Contact
~~~~~~~

If you want to reach me, here's my suggestion:

Send me an email: 
	marcapelini.dev@gmail.com (checked occasionally)
	
Pay a visit to my blog (in Portuguese) and leave a message: 
	http://fotorabiscos.blogspot.com.br/

