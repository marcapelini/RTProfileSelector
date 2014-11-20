//////////////////////////////////////////////////////////////////////////////////////////////
//
//  RTProfileSelector
//
//  RTProfileSelector is a RawTherapee custom profile builder plugin that automatically 
//  selects custom processing profiles(.pp3 files) based on user - defined rules.The rules 
//  are sets of Exif fields and values which are matched against the actual values extracted 
//  from the RAW files.
//
//	Copyright 2014 Marcos Capelini 
//
//  This program is free software : you can redistribute it and / or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	v1.0 - 20-Nov-2014
//
//  Note: source best viewed with a tab size of four spaces
//
//////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>

//////////////////////////////////////////////////////////////////////////////////////////////

using std::string;

// Some useful typedefs
typedef std::map<string, string> StrMap;
typedef std::map<string, StrMap> IniMap;
typedef std::multimap<string, StrMap> IniMultiMap;
typedef std::pair<string, string> StrPair;

//////////////////////////////////////////////////////////////////////////////////////////////

// OS-specific definitions

#ifdef _WIN32
// Slash char for Windows
#define SLASH_CHAR				'\\'	
// Out of convenience, for Windows we assume the user may have copied the exiftool binary to RTProfileSelector's folder
#define DEFAULT_EXIFTOOL_CMD	(basePath + "exiftool")
// Default text viewer for Windows = Notepad
#define DEFAULT_TEXTVIEWER_CMD	("notepad.exe")
#else
// Slash char for *nix
#define SLASH_CHAR				'/'
// Exiftool is simply exiftool
#define DEFAULT_EXIFTOOL_CMD	("exiftool")
// Default text viewer for Ubuntu = Gedit
#define DEFAULT_TEXTVIEWER_CMD	("gedit")
#endif

#ifdef __GNUC__
// Disable annoying warning on GCC for not testing the return of system()
#pragma GCC diagnostic ignored "-Wunused-result"
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Utility functions for dealing with INI-styled files
//

// Removes trailing '\r', fust in case we're reading a Windows text file in Linux
inline void removeReturnChar(string& line)
{
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.resize(line.size() - 1);
}

// Parses line as INI section ("[section]")
// returns true if line was correctly parsed as a INI section and loaded int 'section'
bool parseSection(const string& line, string& section)
{
	if (line.size() < 2 ||				// must have at least opening and closing brackets
		line[0] != '[' ||				// first char must be opening bracket
		line[line.size() - 1] != ']')	// last char must be closing bracket
		return false;
	section = line.substr(1, line.size() - 2);	// section name without brackets 
	return true;
}

// Parses line as INI entry pair ("Key=Value")
// returns true if line was correctly parsed as a key=value pair and loaded into 'keyVal'
bool parseEntry(const string line, StrPair& entry)
{
	size_t eq = line.find('=');
	if (eq == string::npos ||			// key-value separator not found
		eq == 0 ||						// separator at start of line (empty key)
		line[0] == ';')					// comment line
		return false;					// => not a valid line

	entry.first = line.substr(0, eq);
	entry.second = line.substr(eq + 1);
	return true;
}

// Reads the whole INI file contents (sections, keys and values) into a map for easy access
// note: section names must be unique, otherwise entries from different sections with the same name will be merged
IniMap readIni(const string& iniPath)
{
	IniMap iniMap;
	string line, section;
	std::ifstream iniFile(iniPath);

	while (std::getline(iniFile, line))
	{
		removeReturnChar(line);
		StrPair entry;
		if (!parseSection(line, section) &&		// not a section
			!section.empty() &&					// we already have a valid section
			parseEntry(line, entry))			// line was correctly read as key=value
		{
			iniMap[section].insert(entry);		// one more entry in the current section
		}
	}

	return std::move(iniMap);
}

// Reads the whole INI file contents (sections, keys and values) into a map for easy access
// note: sections duplicated in the INI file will be treated as distinct entry maps
IniMultiMap readMultiIni(const string& iniPath)
{
	IniMultiMap iniMap;
	IniMultiMap::iterator currentSection;
	string line, section;
	std::ifstream iniFile(iniPath);

	while (std::getline(iniFile, line))
	{
		removeReturnChar(line);
		// check section
		if (parseSection(line, section))
		{
			// allows multiple sections with the same name
			currentSection = iniMap.insert(IniMultiMap::value_type(section, StrMap()));
		}
		else // may be an entry
		{	
			StrPair entry;
			if (!section.empty() &&						// already have a valid section
				parseEntry(line, entry))				// line was correctly read as key=value
			{
				currentSection->second.insert(entry);	// one more entry in the current section
			}
		}
	}

	return std::move(iniMap);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Utility functions for parsing a tab-delimited (key-TAB-value) file generated by "exiftool -t"
//

// Parses line as a key-value pair delimited by a single tab character
bool parseExifLine(const string& line, StrPair& keyVal)
{
	size_t tab = line.find('\t');
	if (tab == string::npos)
		return false;			// tab separator not found

	keyVal.first = line.substr(0, tab);
	keyVal.second = line.substr(tab + 1);
	return true;
}

// Reads the whole Exif file contents (list of tab separated key-values) into a map for easy access
StrMap readExifOutput(const string& path)
{
	StrMap exifMap;
	string line;
	std::ifstream iniFile(path);

	while (std::getline(iniFile, line))
	{
		StrPair keyVal;
		if (parseExifLine(line, keyVal))
			exifMap.insert(keyVal);
	}

	return std::move(exifMap);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// General utility functions
//

// Removes double slashes ("\\") from path values read from RT's params file on Windows
string filterDoubleSlashes(const string& path)
{
	string result = path;
	string doubleSlash = { SLASH_CHAR, SLASH_CHAR };
	string singleSlash = { SLASH_CHAR };
	size_t pos = 0;

	while ((pos = result.find(doubleSlash, pos)) != string::npos)
	{
		result.replace(pos, doubleSlash.length(), singleSlash);
		pos += singleSlash.length();
	}

	return result;
}

// Copies the contents of a source file to a destination file
bool copyFile(const string& srcPath, const string& destPath)
{
	std::ifstream  src(srcPath, std::ios::binary);
	if (!src.good())
		return false;

	std::ofstream  dst(destPath, std::ios::binary);
	if (!dst.good())
		return false;

	dst << src.rdbuf();
	return true;	// no error checking...
}

// extracts Exif fields from an image file into a map of keys/values
StrMap getExifFields(const string& exiftool, const string& cachePath, const string& imageFileName, std::ofstream& log)
{
	// remove previous Exif output file
	string exifOutFile = cachePath + SLASH_CHAR + "exif.txt";
	remove(exifOutFile.c_str());

	// uses exiftool to extract Exif values from RAW file into 'exif.txt' 
	string exifCmd = exiftool + " -t \"" + imageFileName + "\" > " + exifOutFile;
	log << "\nCalling exiftool: " << exifCmd << std::endl;
	system(exifCmd.c_str());

	// reads Exif file into map 
	return readExifOutput(exifOutFile);
}

// Shows the keys and values from the Exif field map
void showExifFields(StrMap exifFields, const string& textViewer, const string& imageFileName, const string& outputFile)
{
	remove(outputFile.c_str());

	// writes fields as key=value lines
	std::ofstream out(outputFile);

	out << "You are seeing this file because ViewExifKeys is enabled in RTProfileSelector.ini.\n\n";
	out << "Exif fields for image [" + imageFileName + "]:\n\n";

	for (const auto& entry : exifFields)
		out << entry.first << "=" << entry.second << "\n";
	out.close();

	// we don't want the text viewer to block RT...
#ifdef _WIN32
	string cmdLine = "start " + textViewer + " \"" + outputFile + "\"";
#else
	string cmdLine = textViewer + " " + outputFile + "&";
#endif
	// show file
	system(cmdLine.c_str());
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// The profile matching function: 
//
// Searches all sections from RTProfileSelectorRules.ini for the best match for the Exif 
// fields read from the RAW file
//

// Matches all parameter definition sections from RTProfileSelectorRules.ini against the Exif fields from the RAW file
IniMultiMap::const_iterator matchExifFields(const IniMultiMap &rtSelectorRulesIni, const StrMap &exifFields)
{
	IniMultiMap::const_iterator matchIter = rtSelectorRulesIni.cend();		// iterator to matching section 
	std::vector<IniMultiMap::const_iterator> matches;						// full-matches found

	// let's check all profile sections for matches
	for (auto section = rtSelectorRulesIni.begin(); section != rtSelectorRulesIni.end(); ++section)
	{
		size_t matchedKeys = 0;							// matched keys count for current section
		for (auto &keyVal : section->second)			// checks all keys in the current section
		{
			auto field = exifFields.find(keyVal.first);							// key found in Exif
			if (field != exifFields.end() && field->second == keyVal.second)	// value from Exif matches definition from section?
				++matchedKeys;
		}
		// only save the ones that had all keys matched
		if (matchedKeys != 0 && matchedKeys == section->second.size())
			matches.push_back(section);
	}

	if (!matches.empty())
	{
		// sorts matching sections so that we can pick up the one with most keys
		std::sort(begin(matches), end(matches),
			[](const IniMultiMap::const_iterator& prof1, const IniMultiMap::const_iterator& prof2)->bool
			{
				return prof1->second.size() > prof2->second.size();
			}
		);
		matchIter = *matches.begin();
	}

	return matchIter;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Functions for detecting lens ID and focal length and applying the amount of distortion
// correction stored in our simple "lens profile" files
//

// Try to get a distortion amount for the current lens and focal length, based on our simple 
// "lens profile" INI file
bool getDistortionAmount(const string& basePath, const StrMap& exifFields, string& amountStr)
{
	// let's get the lens ID first from Exif
	StrMap::const_iterator lensIdIter = exifFields.find("Lens ID");		
	if (lensIdIter == exifFields.cend())
		// I noticed there's also a "Lens Type" fields, don't know which is best or standard
		lensIdIter = exifFields.find("Lens Type");			
	if (lensIdIter == exifFields.cend())
		// ok, couldn't find lens id, lets fall back to "Camera Model" (might be a compact/fixed lens camera...)
		lensIdIter = exifFields.find("Camera Model Name");	

	string lensId = lensIdIter->second;				// here we have the "lens id" (or whatever)

	// looks for distortion section for this lens' INI file
	IniMap lensProfileIni = readIni(basePath + "Lens Profiles" + SLASH_CHAR + lensId + ".ini");
	if (lensProfileIni.empty())
		return false;

	// locate [Distortion] section
	IniMap::const_iterator lensProfileSection = lensProfileIni.find("Distortion");
	if (lensProfileSection == lensProfileIni.cend() || lensProfileSection->second.empty())
		return false;

	// now looks for focal length
	// note: for Panasonic GM1 raw file I noticed exiftool outputs two lines as "Focal Length" 
	// but since we're using a std::map, only the first occurrence will be preserved
	StrMap::const_iterator focalLengthIter = exifFields.find("Focal Length");	
	if (focalLengthIter == exifFields.cend())
		return false;

	string focalLengthStr = focalLengthIter->second;
	size_t mmPos = focalLengthStr.find("mm");			// not sure unit will always be "mm", let's hope it will
	if (mmPos == string::npos)
		return false;

	focalLengthStr = focalLengthStr.substr(0, mmPos);
	double focalLength = std::stod(focalLengthStr);		// finally the focal length as a double
	if (focalLength == 0.0)
		return false;

	// convert all values to double for proper sorting and maths
	std::map<double, double> flDistortionValues;
	for (const auto& i : lensProfileSection->second)
		flDistortionValues[std::stod(i.first)] = std::stod(i.second);

	double amount = 0;
	auto bestMatchIter = flDistortionValues.find(focalLength);
	if (bestMatchIter != flDistortionValues.end())	
	{
		amount = bestMatchIter->second;			// lens profile has the exact fl key, so we're done
	}
	else
	{
		// can't find exact fl key, so we try to get as close a value as possible
		bestMatchIter = flDistortionValues.lower_bound(focalLength);
		if (bestMatchIter == flDistortionValues.begin())		// fl lesser than min => simply use its amount
			amount = bestMatchIter->second;
		else if (bestMatchIter == flDistortionValues.end())		// fh greater than max => simply use its amount
			amount = (--flDistortionValues.end())->second;
		else
		{
			// if we got here, current fl is in-between two values from our lens profile
			auto previousEntry = bestMatchIter;
			--previousEntry;
			double fl1 = previousEntry->first;
			double amt1 = previousEntry->second;
			double fl2 = bestMatchIter->first;
			double amt2 = bestMatchIter->second;
			// interpolate amount between the two closest focal lengths
			amount = amt1 + ((focalLength - fl1) / (fl2 - fl1)) * (amt2 - amt1);	
		}
	}

	// last sanity check
	if (amount == 0.0)
		return false;

	// convert to string
	std::stringstream ss;
	ss << std::setiosflags(std::ios::fixed) << std::setprecision(3) << amount;
	amountStr = ss.str();
	return true;
}

// Applies the "lens profile" info to the selected profile file
// Currently supports only distortion information (section "[Distortion]", key "Amount" in .pp3 files)
void applyLensProfile(const string& basePath, const StrMap& exifFields, const string& profileFileName)
{
	// get distortion amount for current focal length and lens
	string distortionAmount;
	if (!getDistortionAmount(basePath, exifFields, distortionAmount))
		return;

	string tempFileName = profileFileName + ".tmp";
	std::ofstream tempFile(tempFileName);
	std::ifstream profileFile(profileFileName);
	string line, section;

	while (std::getline(profileFile, line))
	{
		removeReturnChar(line);
		StrPair entry;
		if (!parseSection(line, section) &&		// not a section
			!section.empty() &&					// already have a valid section
			parseEntry(line, entry))			// line was correctly read as key=value
		{
			// look for ditortion amount info
			if (section == "Distortion" && entry.first == "Amount")
			{
				tempFile << "Amount=" << distortionAmount << "\n";	// replace Amount value with the one we got from our lens profile
				continue;
			}
		}
		tempFile << line << "\n";
	}

	// replaces original profile file with the temp file where one we applied the corrected distortion amount
	tempFile.close();
	profileFile.close();

	remove(profileFileName.c_str());
	rename(tempFileName.c_str(), profileFileName.c_str());
}

//////////////////////////////////////////////////////////////////////////////////////////////
// 
// The main program
//
// Usage: RTProfileSelector <RT's params file for profile selection>
//
int main(int argc, const char* argv[])
{
	// if not enough parameters, print usage information
	if (argc < 2)
	{
		std::cout << "Usage: RTProfileSelector  <RT's params file for profile selection>" << std::endl;
		return 1;
	}

	// save program base path 
	string basePath = argv[0];
	size_t slash = basePath.find_last_of(SLASH_CHAR);
	if (slash == string::npos)
		basePath = "";
	else
		basePath = basePath.substr(0, slash + 1);

	// just for simple logging/debugging
	std::ofstream log(basePath + "RTProfileSelector.log");

	// reads profile selection configuration file
	IniMap rtSelectorIni = readIni(basePath + "RTProfileSelector.ini");
	// reads RT's params for profile selection
	IniMap rtProfileParams = readIni(argv[1]);

	// necessary parameters for current RAW file
	string imageFileName = filterDoubleSlashes(rtProfileParams["RT General"]["ImageFileName"]);
	string outputProfileFileName = filterDoubleSlashes(rtProfileParams["RT General"]["OutputProfileFileName"]);
	string cachePath = filterDoubleSlashes(rtProfileParams["RT General"]["CachePath"]);
	string defaultProcParams = filterDoubleSlashes(rtProfileParams["RT General"]["DefaultProcParams"]);
	slash = defaultProcParams.find_last_of(SLASH_CHAR);

	// all parameters found in RT's parameter file?
	if (slash == string::npos || imageFileName.empty() || outputProfileFileName.empty() || cachePath.empty() || defaultProcParams.empty())
	{
		log << "\nInvalid RT ini params file:" << argv[1] << std::endl;
		return 1;
	}

	// default source profile to be copied (reassigned below, if we can find a good match based on Exif)
	string sourceProfile = defaultProcParams;

	// try to get the path where custom profiles are located from RTProfileSelector.ini
	string rtCustomProfilesPath = rtSelectorIni["General"]["RTCustomProfilesPath"];
	// if path not declared in the INI, we assume the default profile is a custom one and extract
	// the path for custom profiles from it
	if (rtCustomProfilesPath.empty())
		rtCustomProfilesPath = defaultProcParams.substr(0, slash);

	// use exiftool to extract Exif values from RAW file into 'exif.txt' 
	string exiftool = rtSelectorIni["General"]["ExifTool"];
	if (exiftool.empty())
		exiftool = DEFAULT_EXIFTOOL_CMD;

	// reads Exif file into map 
	StrMap exifFields = getExifFields(exiftool, cachePath, imageFileName, log);

	if (!exifFields.empty())
	{
		// if ViewExifKeys is enabled, we generate and open a KEY=VALUE text file
		if (rtSelectorIni["General"]["ViewExifKeys"] == "1")
		{
			// check whether a specific viewer is defined in the configuration file
			string exifViewerCmd = rtSelectorIni["General"]["TextViewer"];
			if (exifViewerCmd.empty())
				exifViewerCmd = DEFAULT_TEXTVIEWER_CMD;
			// this will generate a text file and open it in a text editor so the user can easily
			// copy the KEY=VALUE pairs for creating a rule based on the current image file
			showExifFields(exifFields, exifViewerCmd, imageFileName, cachePath + SLASH_CHAR + "exif_fields.txt");
		}

		// checks all profile selection definitions for a match against the Exif values
		IniMultiMap rtSelectorRulesIni = readMultiIni(basePath + "RTProfileSelectorRules.ini");
		auto match = matchExifFields(rtSelectorRulesIni, exifFields);
		if (match != rtSelectorRulesIni.cend())
		{	// we have found a profile matching the Exif values
			sourceProfile = rtCustomProfilesPath + SLASH_CHAR + match->first;
		}
	}

	// copy matching profile as the RAW's file RT profile
	if (!copyFile(sourceProfile, outputProfileFileName))
	{
		log << "\nError copying source -> destination profile:" << std::endl;
		log << sourceProfile << " -> " << outputProfileFileName << std::endl;
		return 1;
	}
	log << "\nSuccessfuly copied  profile:" << sourceProfile << " -> " << outputProfileFileName << std::endl;

	// as a last step, lest try to apply our simple "lens profile" correction
	applyLensProfile(basePath, exifFields, outputProfileFileName);

	return 0;
}