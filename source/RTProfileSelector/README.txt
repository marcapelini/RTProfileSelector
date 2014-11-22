This folder contains:
 * Source for RTProfileSelector (C++11 compiler required)
 * Solution and project for Visual Studio Express 2013, for compiling the program on Windows
 * Workspace and project for CodeLight 6.0, for compiling the program on Linux/Ubuntu 14
 
For Windows, just open the solution 'RTProfileSelector.sln' with VS Express 2013 and build it.
 
For Linux/Ubuntu 14.04, just open the workspace 'RTProfileSelector.workspace' in CodeLite and build it.

If you want to compile from the command line, g++ 4.9 may be used (that's what worked for me anyway):
  * To install g++ on Ubuntu:
    - sudo apt-get update
    - sudo apt-get install g++
  * To compile from the command line:
    - g++ -Wall -std=c++0x  RTProfileSelector.cpp -o RTProfileSelector
