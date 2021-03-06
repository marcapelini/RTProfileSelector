//////////////////////////////////////////////////////////////////////////////////////////////
//
//  RTProfileSelector
//
//  RTProfileSelector is a RawTherapee custom profile builder plugin that automatically 
//  selects custom processing profiles(.pp3 files) based on user - defined rules.The rules 
//  are sets of Exif fields and values which are matched against the actual values extracted 
//  from the raw files.
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
#include <list>
#include <map>
#include <set>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <codecvt>
#include <locale>

//////////////////////////////////////////////////////////////////////////////////////////////
// Hate this, but had to use CreateProcess() (see executeProcess() below) to get
// rid of a nagging console windows created by calling system() on MS Windows :(
#ifdef _WIN32
// By default Windows defines max macro which conflicts with std::numeric_limits<double>::max() 
#define NOMINMAX		
#include <windows.h>
#endif
//////////////////////////////////////////////////////////////////////////////////////////////

using std::string;

//////////////////////////////////////////////////////////////////////////////////////////////

// struct for storing value and source file info for INI keys
struct IniValue
{
	string value;	
	string source;
	
	// some convenience operators to work with strings
	string& operator=(const string& s) { value = s; return value;}	
	operator const string&() const { return value; }
	bool operator==(const string& s) const { return value == s; }
	bool operator!=(const string& s) const { return value != s; }
};

//////////////////////////////////////////////////////////////////////////////////////////////

// Some useful typedefs
typedef std::pair<string, string> StrPair;
typedef std::map<string, string> StrMap;

typedef std::map<string, IniValue> EntryMap;
typedef std::pair<string, IniValue> IniEntry;

typedef std::map<string, EntryMap> IniMap;
typedef std::multimap<string, EntryMap> IniMultiMap;

typedef std::set<string> StrSet;
typedef std::pair<string, StrSet> StrSetPair;
typedef std::vector<StrSetPair> StrSetVector;

// default locale for UTF-8 conversions
std::locale defaultLocale;

//////////////////////////////////////////////////////////////////////////////////////////////

// OS-specific definitions

#ifdef _WIN32
// Slash char for Windows
#define SLASH_CHAR				'\\'	
#define REVERSE_SLASH_CHAR		'/'	
// Out of convenience, for Windows we assume the user may have copied the exiftool binary to RTProfileSelector's folder
#define DEFAULT_EXIFTOOL_CMD	(basePath + "exiftool")
// Default text viewer for Windows = Notepad
#define DEFAULT_TEXTVIEWER_CMD	("notepad.exe")
#else
// Slash char for *nix
#define SLASH_CHAR				'/'
#define REVERSE_SLASH_CHAR		'\\'	
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
// A few string constants

// Exif keys
#define EXIF_LENS_ID				"Lens ID"
#define EXIF_LENS_TYPE				"Lens Type"
#define EXIF_CAMERA_MODEL			"Camera Model Name"
#define EXIF_ISO					"ISO"
#define EXIF_FOCAL_LENGTH			"Focal Length"

// PP3 file constants
#define PP3_VERSION_SECTION			"Version"
#define PP3_DISTORTION_SECTION		"Distortion"
#define PP3_DISTORTION_AMOUNT		"Amount"
#define PP3_LENS_PROFILE_SECTION	"LensProfile"
#define PP3_LENS_PROFILE_KEY		"LCPFile"

// RT's keyfile definitions
#define RT_KEYFILE_GENERAL_SECTION	"RT General"

// Folder names for our profile definitions 
#define LENS_PROFILE_DIR			"Lens Profiles"
#define ISO_PROFILE_DIR				"ISO Profiles"

// RTPS's ini-file definitions
#define RTPS_INI_SECTION_GENERAL	"General"
#define RTPS_INI_SECTION_ISO		"ISO Profile Sections"

// Partial profiles reules specific keys
#define RTPS_RULES_PRIVATE_KEY_CHAR	'@'
#define RTPS_RULES_PP3_SECTIONS_KEY	"@Sections"
#define RTPS_RULES_SECT_WILDCARD	"*"
#define RTPS_RULES_PROFILE_RANK		"@Rank"

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Utility functions for dealing with INI-styled files
//

// convert UTF-8 string to current locale single-byte string
void utf8ToString(string& utf8str)
{
	// code below doesn't work with empty strings...
	if (utf8str.empty())
		return;
	// UTF-8 to wstring
	std::wstring_convert<std::codecvt_utf8<wchar_t>> wconv;
	std::wstring wstr = wconv.from_bytes(utf8str);
	// wstring to string
	std::vector<char> buf(wstr.size());
	std::use_facet<std::ctype<wchar_t>>(defaultLocale).narrow(wstr.data(), wstr.data() + wstr.size(), '?', buf.data());
	utf8str = string(buf.data(), buf.size());
}

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
bool parseEntry(const string line, IniEntry& entry)
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
		utf8ToString(line);
		IniEntry entry;
		if (!parseSection(line, section) &&		// not a section
			!section.empty() &&					// we already have a valid section
			parseEntry(line, entry))			// line was correctly read as key=value
		{
			entry.second.source = iniPath;
			iniMap[section].insert(entry);		// one more entry in the current section
		}
	}

	return iniMap;
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
		utf8ToString(line);
		// check section
		if (parseSection(line, section))
		{
			// allows multiple sections with the same name
			currentSection = iniMap.insert(IniMultiMap::value_type(section, EntryMap()));
		}
		else // may be an entry
		{	
			IniEntry entry;
			if (!section.empty() &&						// already have a valid section
				parseEntry(line, entry))				// line was correctly read as key=value
			{
				entry.second.source = iniPath;
				currentSection->second.insert(entry);	// one more entry in the current section
			}
		}
	}

	return iniMap;
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

	return exifMap;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// General utility functions
//

// Simple trim removing tabs and spaces from begin of string
inline string trimLeft(const string& s)
{
	size_t firstNonSpace = s.find_first_not_of("\t ");
	if (string::npos != firstNonSpace)
		return s.substr(firstNonSpace);
	return s;
}

// Evaluates a string as a double.  
// Attempts to perform a division operation if "/" is found, so that we can convert Exif exposure values
// such as in "Exposure Time=1/1300"
double eval(const string& str, double defaultValue)
{
	double d = defaultValue;
	try
	{
		size_t divPos = str.find_first_of('/');
		if (divPos == string::npos)
			d = std::stod(str);		// simple conversion
		else
		{
			// does division
			double denominator = std::stod(str.substr(divPos + 1));
			if (denominator != 0.0)
				d = std::stod(str.substr(0, divPos)) / denominator;
		}
	}
	catch (...)
	{
	}
	return d;
}

// Removes double slashes ("\\") from path values read from RT's keyfile on Windows
string removeDoubleSlashes(const string& path)
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

// Adds double slashes ("\\") to path values that are to be written to .pp3 files
string addDoubleSlashes(const string& path)
{
	string result = path;

#ifdef _WIN32
	string doubleSlash = { SLASH_CHAR, SLASH_CHAR };
	string singleSlash = { SLASH_CHAR };
	size_t pos = 0;

	while ((pos = result.find(singleSlash, pos)) != string::npos)
	{
		result.replace(pos, singleSlash.length(), doubleSlash);
		pos += doubleSlash.length();
	}
#endif

	return result;
}

// Prepares path string for output in .PP3-compatible format
string adjustRTOutputSlashes(const string& path)
{
	return addDoubleSlashes(removeDoubleSlashes(path));
}

// Converts any of the reverse slash char (for Windows: '/', for *nix:'\') to the current OS path slash
string convertoToCurrentOSPath(const string& path)
{
	string result = path;
	string currentSlash = { SLASH_CHAR };
	string reverseSlash = { REVERSE_SLASH_CHAR };
	size_t pos = 0;

	while ((pos = result.find(reverseSlash, pos)) != string::npos)
	{
		result.replace(pos, 1, currentSlash);
		pos += 1;
	}

	return result;
}

// Replaces reserved (forbidden) characters with space (' ') so as to create a valid file name according to the target OS
string safeFileName(const string& str)
{
	string safeStr = str;
	std::replace_if(safeStr.begin(), safeStr.end(),
		[](char c) -> bool
		{
#ifdef _WIN32
			static const string reservedChars = "<>:\"/\\|?";
#else
			static const string reservedChars = "/";
#endif
			return reservedChars.find(c) != string::npos;
		}, '_');
	return safeStr;
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

// Launches a process, optionally redirecting output and waiting for termination
// Note: this is bad and ugly, I wanted to have as little OS-specific code as possible, but on Windows
// the call to system() always flashes a nagging console window, so had to resort to CreateProcess()
void executeProcess(const string& cmdline, const string& redirectFile, bool waitForTermination)
{
#ifdef _WIN32
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	HANDLE hfile = redirectFile.empty() ? NULL : CreateFile(redirectFile.c_str(), FILE_APPEND_DATA, FILE_SHARE_WRITE, &sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFO si = { sizeof(STARTUPINFO) };
	si.dwFlags |= STARTF_USESTDHANDLES;
	si.hStdError = si.hStdOutput = hfile;

	std::vector<char> cmd(cmdline.c_str(), cmdline.c_str() + cmdline.size() + 1);
	if (CreateProcess(NULL, &cmd[0], NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		if (waitForTermination)
			WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	if (hfile != 0)
		CloseHandle(hfile);
#else
	string cmd = cmdline;
	if (!redirectFile.empty())
		cmd += " > \"" + redirectFile + "\"";

	if (!waitForTermination)
		cmd += "&";
	system(cmd.c_str());
#endif
}

void copyKeys(StrMap& exifFields, const IniMap& rtProfileParams, const string& section, const string& scope = "")
{
	std::string preffix = scope.empty() ? "" : scope + "."; 
	auto iter = rtProfileParams.find(section);
	if (iter != rtProfileParams.end())
	{
		for (auto keyval: iter->second)
		{
			if (keyval.second == "$subdir")
				copyKeys(exifFields, rtProfileParams, section + "/" + keyval.first, preffix + keyval.first);
			else
				exifFields[preffix + keyval.first] = keyval.second;
		}
	}	
}

// Extracts Exif fields from RawTherapee key file params
StrMap getParamsExifFields(const IniMap& rtProfileParams, std::ofstream& log)
{
	StrMap exifFields;
	copyKeys(exifFields, rtProfileParams, "Common Data", "CommonData");
	copyKeys(exifFields, rtProfileParams, "EXIF");
	return exifFields;
}


// Extracts Exif fields from an image file into a map, using exiftool
StrMap getExifFields(const string& exiftool, const string& cachePath, const string& imageFileName, std::ofstream& log)
{
	// output file named for the image file
	size_t slash = imageFileName.find_last_of(SLASH_CHAR);
	size_t dot = imageFileName.find_last_of('.');
	if (slash == string::npos || dot == string::npos)
		return StrMap();

	// exif output file
	string fileName(imageFileName.begin() + slash + 1, imageFileName.begin() + dot);
	string exifOutFile = cachePath + SLASH_CHAR + fileName + ".txt";

	// uses exiftool to extract Exif values from raw file into 'FILENAME.txt' 
	string exiftoolCmd = exiftool + " -t -m -q \"" + imageFileName + "\"";
	log << "\nCalling exiftool: " << exiftoolCmd << " > " << exifOutFile << std::endl;

	// call exiftool, redirecting output to a text file
	executeProcess(exiftoolCmd, exifOutFile, true);

	// reads Exif file into map 
	StrMap exifFields = readExifOutput(exifOutFile);
	remove(exifOutFile.c_str());

	return exifFields;
}

// Saves the keys and values to a file containing a list of "key=value" lines that cam be directly copied to a rules file
void saveExifFields(const StrMap& exifFields, const string& imageFileName, const string& outputFile, const string& textViewer, bool showTextFile)
{
	remove(outputFile.c_str());

	// writes fields as key=value lines
	std::ofstream out(outputFile);

	out << "Exif fields for image [" + imageFileName + "]:\n\n";

	for (const auto& entry : exifFields)
		out << entry.first << "=" << entry.second << "\n";
	out.close();

	if (showTextFile)
		executeProcess(textViewer + " \"" + outputFile + "\"", "", false);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// The profile matching function: 
//
// Searches all sections from RTProfileSelectorRules.ini for the best match for the Exif 
// fields read from the raw file
//

// Matches value against rule
//
// If complex rules are anabled:
// - rule value preceded by '!' => negative rule (field must NOT match rule value)
//		Example:	
//					White Balance=!Manual
//
// - rule has '~' (tilde)		=> interpret as numeric range: r1 >= value >= r2
//		Example: 
//					ISO=200~400
//					Focal Length=12.0 mm ~ 14.0 mm
//
// - rule has '|' (pipe)		=> interpret as list of values 
//		Example:
//					Photo Style=Dynamic (B&W)|Black & White|Monochrome
//
bool matchValue(const string& exifValue, const string& ruleValue, bool useComplexRules)
{
	// if exif value has any reserved char, disable complex rule evaluation
	useComplexRules &= exifValue.find_first_of("!~|") == string::npos;
	// complex rules disabled => direct string comparison
	if (!useComplexRules)
		return exifValue == ruleValue;

	bool matched = false;
	// let's see if it's a list of pipe-delimited values
	size_t pipe = ruleValue.find_first_of('|');
	if (pipe != string::npos)
	{
		// check first item in the list
		matched = matchValue(exifValue, ruleValue.substr(0, pipe), useComplexRules);
		if (!matched)	// no match => calling recursively takes care of splitting remaining items)					
			matched = matchValue(exifValue, ruleValue.substr(pipe + 1), useComplexRules);
	}
	else
	{
		// rules can be negated by ! operator - operator MUST be first non-space char of value string
		string value = trimLeft(ruleValue);
		bool op_neq = !value.empty() && value[0] == '!';
		if (op_neq)
			value = value.substr(1);

		// might be a range expression: two numeric values (optionally followed by unit, as in "40mm") separated by a tilde
		size_t tilde = value.find_first_of('~');
		if (tilde != string::npos)
		{
			// I should really reserve characters for minus/plus infinity, but I'll consider a range as legal 
			// if AT LEAST ONE SIDE of a range expression is convertible to a number. 
			// This is so that we can have such ranges as: 
			//	"* ~ 400" meaning anything <= 400 or
			//	"400 ~ *" meaning anything >= 400
			// Anything that is not convertible will be ignored (not just "*") 
			double r1 = eval(value.substr(0, tilde), std::numeric_limits<double>::lowest());
			double r2 = eval(value.substr(tilde + 1), std::numeric_limits<double>::max());

			if (r1 != std::numeric_limits<double>::lowest() || r2 != std::numeric_limits<double>::max())
			{
				double val = eval(exifValue, 0.0);		
				matched = ((val >= r1) && (val <= r2)) ^ op_neq;
			}
		}
		else
		{	// must be a single value
			matched = (exifValue == value) ^ op_neq;
		}
	}
	return matched;
}

// Matches full profiles parameter definition sections from RTProfileSelectorRules.ini against the Exif fields from the raw file
IniMultiMap::const_iterator matchExifFields(const IniMultiMap &rtSelectorRulesIni, const StrMap &exifFields, bool useComplexRules)
{
	IniMultiMap::const_iterator matchIter = rtSelectorRulesIni.cend();		// iterator to matching section 
	std::vector<IniMultiMap::const_iterator> matches;						// full-matches found

	// let's check all full profile sections for matches
	for (auto section = rtSelectorRulesIni.begin(); section != rtSelectorRulesIni.end(); ++section)
	{
		const EntryMap& keys = section->second;
		// look for "@Sections" key, present in partial profile rules only 
		auto sectionsKey = keys.find(RTPS_RULES_PP3_SECTIONS_KEY);
		if (sectionsKey != keys.end())						// skip partial profiles
			continue;

		size_t matchedKeys = 0;								// matched keys count for current section
		size_t privateKeys = 0;								// private RTPS keys begin with '@'
		for (const IniEntry &keyVal : section->second)		// checks all keys in the current section
		{
			if (keyVal.first[0] == RTPS_RULES_PRIVATE_KEY_CHAR)		// skip private RTPS Keys
			{
				++privateKeys;
				continue;
			}

			auto field = exifFields.find(keyVal.first);		// key found in Exif
			// check if value from Exif matches definition from rule
			if (field != exifFields.end() && matchValue(field->second, keyVal.second.value, useComplexRules))	
				++matchedKeys;
		}
		// save only the ones that had all keys matched
		if (matchedKeys != 0 && matchedKeys == section->second.size() - privateKeys)
			matches.push_back(section);
	}

	if (!matches.empty())
	{
		// sorts matching sections so that we can pick up the one with most keys
		std::sort(begin(matches), end(matches),
			[](const IniMultiMap::const_iterator& prof1, const IniMultiMap::const_iterator& prof2) -> bool
			{
				return prof1->second.size() > prof2->second.size();
			}
		);
		matchIter = *matches.begin();
	}

	return matchIter;
}

// Matches partial profiles parameter definition sections from RTProfileSelectorRules.ini against the Exif fields from the raw file
StrSetVector getPartialProfilesMatches(const IniMap& rtSelectorIni, const IniMultiMap &rtSelectorRulesIni, const StrMap &exifFields, bool useComplexRules)
{
	std::vector<IniMultiMap::const_iterator> matches;						// full-matches found

	// let's check all parttial profile sections for matches
	for (auto section = rtSelectorRulesIni.begin(); section != rtSelectorRulesIni.end(); ++section)
	{
		const EntryMap& keys = section->second;
		// look for "@Sections" key, present in partial profile rules only 
		auto sectionsKey = keys.find(RTPS_RULES_PP3_SECTIONS_KEY);
		if (sectionsKey == keys.end())				// allow only partial profiles
			continue;

		size_t matchedKeys = 0;						// matched keys count for current section
		size_t privateKeys = 0;						// private RTPS keys begin with '@'
		for (const IniEntry &keyVal : keys)			// checks all keys in the current section
		{
			if (keyVal.first[0] == RTPS_RULES_PRIVATE_KEY_CHAR)				// skip private RTPS Keys
			{
				++privateKeys;
				continue;
			}

			auto field = exifFields.find(keyVal.first);			// key found in Exif
			// check if value from Exif matches definition from rule
			if (field != exifFields.end() && 
				 matchValue(field->second, keyVal.second.value, useComplexRules))
				++matchedKeys;
		}
		// only save the ones that had all keys matched
		if (matchedKeys != 0 && matchedKeys == section->second.size() - privateKeys)
			matches.push_back(section);
	}

	StrSetVector partialProfiles;
	if (!matches.empty())
	{
		// sorts matching profiles according to rank
		std::sort(begin(matches), end(matches),
			[](const IniMultiMap::const_iterator& prof1, const IniMultiMap::const_iterator& prof2) -> bool
			{
				auto rankIter1 = prof1->second.find(RTPS_RULES_PROFILE_RANK);
				auto rankIter2 = prof2->second.find(RTPS_RULES_PROFILE_RANK);
				
				int rank1 = atoi(rankIter1 == prof1->second.end() ? "0" : rankIter1->second.value.c_str());
				int rank2 = atoi(rankIter2 == prof2->second.end() ? "0" : rankIter2->second.value.c_str());

				return rank1 < rank2;	// highest rank profile must be applied last
			}
		);

		// get pp3 sections to be applied for each matching profile
		for (auto &matchIter : matches)
		{
			string pp3Name = matchIter->first;

			// get profile section set
			auto profileIter = std::find_if(partialProfiles.begin(), partialProfiles.end(), 
				[&pp3Name](const StrSetPair& item)
				{
					return item.first == pp3Name;
				}
			);
			if (profileIter == partialProfiles.end())
			{
				partialProfiles.push_back(StrSetPair(pp3Name, StrSet()));
				profileIter = std::prev(partialProfiles.end());
			}

			StrSet& pp3Sections = profileIter->second;
			auto sectionsValue = matchIter->second.find(RTPS_RULES_PP3_SECTIONS_KEY);
			string section;
			std::stringstream ss(sectionsValue->second.value);
			while (std::getline(ss, section, ','))
			{
				// wildcard: use any sections found in the partial profile
				if (section == RTPS_RULES_SECT_WILDCARD)			
				{
					pp3Sections.clear();
					pp3Sections.insert(section);
					break;
				}
				// an expansion list section: retrieve actual list from RTProfileSelector.ini
				else if (section[0] == '[' && section[section.length()-1] == ']')	
				{
					IniMap::const_iterator sectionsIter = rtSelectorIni.find(section.substr(1, section.length()-2));
					if (sectionsIter != rtSelectorIni.cend())
					{
						for (auto &entry : sectionsIter->second)
						{
							if (entry.second.value == "1")			// section is enabled?
								pp3Sections.insert(entry.first);	// add to profile
						}
					}
				}
				else
				{											
					pp3Sections.insert(section);			// it's a simple section name => just insert it
				}
			}
		}	
	}

	return partialProfiles;
}


//////////////////////////////////////////////////////////////////////////////////////////////
//
// Functions for applying partial profiles after the default profile has been selected
//

// Fills profile sections from rules-based partial profiles
bool getRulesPartialProfiles(   std::ostream& log, 
								const string& basePath, const string& rtCustomProfilesPath, const IniMap& rtSelectorIni, 
								const StrMap& exifFields, const StrSetVector& partialProfilesList, IniMap& partialProfile)
{
	// for each partial profile, read sections and values
	for (auto &profileItem : partialProfilesList)
	{
		// read pp3 file
		string fileName = rtCustomProfilesPath + SLASH_CHAR + profileItem.first;
		IniMap pp3Ini = readIni(fileName);
		if (pp3Ini.empty())
			continue;

		log << "Including partial profile: " << fileName << "\n";

		// filter for which sections are to be copied
		const StrSet& filterSections = profileItem.second;
		bool copyAllSections = filterSections.find(RTPS_RULES_SECT_WILDCARD) != filterSections.end();

		// copy sections from .pp3 profile to partial profile map
		for (auto& section : pp3Ini)
		{
			if (copyAllSections ||											// all sections are to be copied
				filterSections.find(section.first) != filterSections.end())	// filter => copy only enabled sections
			{
				// new behaviour (issue #2): only existing values from correspending keys will be overwriten
				auto& sectionMap = partialProfile[section.first];
				for (auto entry : section.second)
					sectionMap[entry.first] = entry.second;
			}
		}
	}

	return true;
}

// Fills profile sections from ISO-based profiles
bool getISOPartialProfile(std::ostream& log, const string& basePath, const string& rtCustomProfilesPath, const IniMap& rtSelectorIni, const StrMap& exifFields, IniMap& partialProfile)
{
	// let's find camera model and ISO setting 
	auto cameraModelIter = exifFields.find(EXIF_CAMERA_MODEL);
	if (cameraModelIter == exifFields.cend())
		return false;

	auto isoIter = exifFields.find(EXIF_ISO);
	if (isoIter == exifFields.cend())
		return false;

	string isoStr = isoIter->second;
	int iso = std::stoi(isoStr);
	if (iso <= 0)					// ISO must be a valid non-zero value
		return false;

	// ini with ISO-pp3 profile associations for current camera
	IniMap isoProfileIni = readIni(basePath + ISO_PROFILE_DIR + SLASH_CHAR + "iso." + safeFileName(cameraModelIter->second) + ".ini");
	if (isoProfileIni.empty())
		return false;

	// ISO-pp3 association section within camera ini file
	auto isoProfileSection = isoProfileIni.find("Profiles");
	if (isoProfileSection == isoProfileIni.cend() || isoProfileSection->second.empty())
		return false;

	// make map of ISO as int values vs. .pp3 name
	std::map<int, string> isoProfiles;
	for (const auto& i : isoProfileSection->second)
		isoProfiles[std::stoi(i.first)] = i.second.value;

	// look up for ISO match
	auto isoSearch = isoProfiles.lower_bound(iso);
	if (isoSearch == isoProfiles.begin() && isoSearch->first != iso)	// image ISO is lesser than first entry -> no partial .pp3 to select
		return false;

	// ISO is either a match or above an existing entry => proceed
	string isoProfileName;
	if (isoSearch == isoProfiles.end() || isoSearch->first != iso)
		isoProfileName = (--isoSearch)->second;		// pick up .pp3 from lesser ISO entry
	else
		isoProfileName = isoSearch->second;			// exact match for ISO

	// check that there's a non-empty .pp3 name
	if (isoProfileName.empty())
		return false;

	// makes sure any reverse slash is converted to current OS slash
	isoProfileName = convertoToCurrentOSPath(isoProfileName);

	// first look for .pp3 file in RT's custom profiles folder
	IniMap partialIsoIni = readIni(rtCustomProfilesPath + SLASH_CHAR + isoProfileName);
	// if not found, look in RTPS's "ISO Profiles" folder 
	if (partialIsoIni.empty())
		partialIsoIni = readIni(basePath + ISO_PROFILE_DIR + SLASH_CHAR + isoProfileName);
	// partial profile empty => nothing to do
	if (partialIsoIni.empty())
		return false;

	// user may also have declared filter for which sections are to be copied
	const EntryMap* isoSections = nullptr;
	auto isoSectionsIter = rtSelectorIni.find(RTPS_INI_SECTION_ISO);
	if (isoSectionsIter != rtSelectorIni.cend())
		isoSections = &isoSectionsIter->second;
		
	log << "ISO = " << iso << "\n";
	log << "Including ISO profile: " << isoProfileName << "\n";		

	// copy sections from .pp3 profile to partial profile map
	for (auto& section : partialIsoIni)
	{
		EntryMap::const_iterator filterSection;
		if ((isoSections == nullptr) ||									// no filter => all sections are copied
			((filterSection = isoSections->find(section.first)) != isoSections->end()
			&& filterSection->second.value == "1"))							// filter => copy only enabled sections
		{
			partialProfile[section.first] = std::move(section.second);	// transfer whole section to destination profile
		}
	}

	return true;
}


// Attempts to calculate a distortion amount for the current lens and focal length, based on our simple 
// "lens profile" INI file
bool getLensPartialProfile(std::ostream& log, const string& basePath, const StrMap& exifFields, IniMap& partialProfile)
{
	// map with lens profile entries
	IniMap lensProfileIni;

	// let's get the lens ID first from Exif
	auto lensIdIter = exifFields.find(EXIF_LENS_ID);

	// I noticed there's also a "Lens Type" field, don't know which is best or standard
	if (lensIdIter == exifFields.cend())		
		lensIdIter = exifFields.find(EXIF_LENS_TYPE);

	// flag to mark whether we've already tried "camera model" ini file (fall back in case no valid lens key found)
	bool triedCameraModel = false;

	// try to read lens INI file
	string lensFileName;
	while (lensProfileIni.empty())
	{
		// look for lens' INI file: ./Lens Profiles/lens.<Lens ID>.ini
		if (lensIdIter != exifFields.cend())
		{
			lensFileName = basePath + LENS_PROFILE_DIR + SLASH_CHAR + "lens." + safeFileName(lensIdIter->second) + ".ini";				
			lensProfileIni = readIni(lensFileName);
		}
		if (lensProfileIni.empty())
		{
			if (triedCameraModel)
			{	// have already tried "camera model" => no valid profile INI found
				return false;	
			}
			else
			{	// lens INI not found (or empty), let's try "camera model" instead
				triedCameraModel = true;
				lensIdIter = exifFields.find(EXIF_CAMERA_MODEL);
			}
		}
	} 

	log << "Checking lens ini file: " << lensFileName << "...\n";			

	// look for RT's [LensProfile] section
	auto lensProfileSection = lensProfileIni.find(PP3_LENS_PROFILE_SECTION);
	if (lensProfileSection != lensProfileIni.cend() && !lensProfileSection->second.empty())
	{
		auto lcpfile = lensProfileSection->second.find(PP3_LENS_PROFILE_KEY);
		if (lcpfile != lensProfileSection->second.end())
			lcpfile->second = adjustRTOutputSlashes(lcpfile->second);
		partialProfile[PP3_LENS_PROFILE_SECTION] = lensProfileSection->second;
		log << "Lens file using [" << PP3_LENS_PROFILE_SECTION << "] section\n";			
		return true;
	}

	// locate [Distortion] section
	auto lensDistortionSection = lensProfileIni.find(PP3_DISTORTION_SECTION);
	if (lensDistortionSection == lensProfileIni.cend() || lensDistortionSection->second.empty())
	{
		log << "Error: file does not contain [" << PP3_DISTORTION_SECTION << "] section\n";			
		return false;
	}

	// now looks for focal length
	// note: for Panasonic GM1 raw file I noticed exiftool outputs two lines as "Focal Length" 
	// but since we're using a std::map, only the first occurrence will be preserved
	auto focalLengthIter = exifFields.find(EXIF_FOCAL_LENGTH);
	if (focalLengthIter == exifFields.cend())
	{
		log << "Error: EXIF does not contain \"" << EXIF_FOCAL_LENGTH << "\" field\n";				
		return false;
	}

	string focalLengthStr = focalLengthIter->second;
	size_t mmPos = focalLengthStr.find("mm");			// not sure unit will always be "mm", let's hope it will
	if (mmPos == string::npos)
	{
		log << "Error: invalid unit for EXIF field \"" << EXIF_FOCAL_LENGTH << "\" (" <<  focalLengthStr << ") \n";
		return false;
	}

	focalLengthStr = focalLengthStr.substr(0, mmPos);
	double focalLength = std::stod(focalLengthStr);		// finally the focal length as a double
	if (focalLength == 0.0)
	{
		log << "Error: invalid EXIF field \"" << EXIF_FOCAL_LENGTH << "\" value (" <<  focalLengthStr << ") \n";
		return false;
	}

	// convert all values to double for proper sorting and maths
	std::map<double, double> flDistortionValues;
	for (const auto& i : lensDistortionSection->second)
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

	// convert to string and sets partial profile with distortion amount
	std::stringstream ss;
	ss << std::setiosflags(std::ios::fixed) << std::setprecision(3) << amount;
	partialProfile[PP3_DISTORTION_SECTION][PP3_DISTORTION_AMOUNT] = { ss.str(), "calculated from " + lensFileName };	
	log << "Processed lens distortion info file : " << lensFileName << "\n";
	log << "Calculated distortion value = " << ss.str() << "\n";	

	return true;
}

// Applies partial profiles to the selected destination profile
// Currently partial information can be filled in from partial rules, lens-based distortion profile, or Camera/ISO based partial profiles
bool applyPartialProfiles(  std::ostream& log, 
							const string& basePath, const string& rtCustomProfilesPath, const IniMap& rtSelectorIni, 
							const StrMap& exifFields, const StrSetVector& partialProfilesList, 
							const string& baseProfileFileName, const string& outputProfileFileName)
{
	// map of partial settings 
	IniMap partialProfile;

	// first: cascadingly apply partial profiles matched directly from rules
	getRulesPartialProfiles(log, basePath, rtCustomProfilesPath, rtSelectorIni, exifFields, partialProfilesList, partialProfile);

	// second: ISO-specific partial profile (selected based on nearest ISO sensitivity value)
	// note: if matched, ovewrites any corresponding entries and sections obtained from rule-bases profiles (above)
	getISOPartialProfile(log, basePath, rtCustomProfilesPath, rtSelectorIni, exifFields, partialProfile);
	
	// third: distortion amount for current lens and focal length (crudely calculated by interpolating values from user-defined INI-format lens profile)
	// note: if matched, ovewrites any corresponding entries and sections obtained from rule-bases profiles (above)	
	getLensPartialProfile(log, basePath, exifFields, partialProfile);

	// erase any version info from partial profiles: will copy [Version] section later from main profile
	partialProfile.erase(PP3_VERSION_SECTION);

	// sections merged from partial profile
	std::set<string> mergedPP3Sections;
	
	// input & output files
	std::ifstream profileFile(baseProfileFileName);
	if (!profileFile.good())
	{
		log << "\nError opening base profile file: " << baseProfileFileName << std::endl;
		return false;
	}
	
	// output streams for profile generation and debugging
	std::ostringstream tempStream;
	std::ostringstream debugStream;
	
	debugStream << "; Base profile file: " << baseProfileFileName << "\n";
	debugStream << "; Output profile file: " << outputProfileFileName << "\n\n";	
	
	// lambda for writing entry to destination & debug files
	auto writeEntry = [&](IniEntry entry)
	{
		tempStream << entry.first << "=" << entry.second.value << "\n";		// key=value
		debugStream << "; source: " << entry.second.source << "\n";			// print source PP3 file for each entry
		debugStream << entry.first << "=" << entry.second.value << "\n";	// key=value
	};

	// lambda for writing non-entry line to destination & debug files
	auto writeLine = [&](const string& line = "")
	{
		tempStream << line << "\n";
		debugStream << line<< "\n";
	};

	// parse sections & entries from "full" profile file
	EntryMap partialSection;
	string line, sectionName;
	while (std::getline(profileFile, line))
	{
		removeReturnChar(line);
		// check if line is the start of a new section
		if (parseSection(line, sectionName))
		{
			// new section detected => dumps entries (if any) from previous section 
			for (auto& entry : partialSection)
				writeEntry(entry);
			partialSection.clear();

			// looks for a section of the same name in the partial profile
			auto sectionFromPartial = partialProfile.find(sectionName);
			if (sectionFromPartial != partialProfile.end())
			{	// new behaviour (issue #2): "partial" section from partial profile will be merged into existing section (see below)
				partialSection = sectionFromPartial->second;
				partialProfile.erase(sectionFromPartial);
			}
		}
		// there's a valid section name
		if (!sectionName.empty())
		{
			// parse line as KEY=VALUE 
			IniEntry entry;
			if (parseEntry(line, entry))
			{	
				entry.second.source = baseProfileFileName;
				// current line is a valid entry: check if current partial section contains the key
				auto iter = partialSection.find(entry.first);
				if (iter != partialSection.end())
				{	// new behaviour (issue #2): current key found in partial section => use its value instead
					writeEntry(*iter);
					partialSection.erase(iter);
				}
				else
				{	// copy original entry to output
					writeEntry(entry);
				}
			}
			else
			{	// not valid entry (comment, blank line...) => copy original line to output 
				writeLine(line);
			}
		}
	}
	// dumps entries (if any) remaining from current "partial section" 
	for (auto& entry : partialSection)
		writeEntry(entry);
	writeLine();

	// insert remaining sections not found in the "full" profile
	for (auto& section : partialProfile)
	{
		// [section name]
		writeLine("[" + section.first + "]");
		for (auto& entry : section.second)
			writeEntry(entry);
		writeLine();
	}

	profileFile.close();

	// save generated profile to temp file
	string tempFileName = outputProfileFileName + ".tmp";
	std::ofstream tempFile(tempFileName);
	if (!tempFile.good())
	{
		log << "\nError creating temporary output profile file: " << tempFileName << std::endl;
		return false;
	}
	tempFile << tempStream.str();	
	tempFile.close();

	// replace destination file (if any) with generated profile
	remove(outputProfileFileName.c_str());	
	if (rename(tempFileName.c_str(), outputProfileFileName.c_str()) != 0)
	{
		log << "\nError renaming temp file to destination file: \n" 
			<< " " << tempFileName << " -> " << std::endl;
		return false;
	}
	
	// we don't really care if the debug file was succesfuly saved or not 
	// there might be problems with non-exclusive access to the file in case multiple 
	// instances of RTPS are executed simultaneously, but the debug file is really only 
	// meant to be used for profiles & rules tuning and debugging, which only makes sense 
	// for a single image at a time...  
	std::ofstream debugFile(basePath + "LastProfileDebug.txt");
	debugFile << debugStream.str();
	
	std::ofstream profileCopyFile(basePath + "LastProfile.txt");
	profileCopyFile << tempStream.str();	
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// 
// The main program
//
// Usage: RTProfileSelector <RawTherapee params file for profile selection>
//
int main(int argc, const char* argv[])
{
	// save program base path 
	string basePath = argv[0];
	size_t slash = basePath.find_last_of(SLASH_CHAR);
	if (slash == string::npos)
		basePath = "";
	else
		basePath = basePath.substr(0, slash + 1);

	// for simple logging/debugging
	std::ofstream log(basePath + "RTProfileSelector.log");

	if (argc < 2)
	{
		log << "\nToo few arguments" << std::endl;
		return 1;
	}

	// reads profile selection configuration file
	IniMap rtSelectorIni = readIni(basePath + "RTProfileSelector.ini");

	// if necessary, a specific locale can be set for reading INI-files (converting UTF-8 to single-byte char strings)
	// (ex: DefaultLocale=.1252)
	string defaultLocaleName = rtSelectorIni[RTPS_INI_SECTION_GENERAL]["DefaultLocale"];
	if (!defaultLocaleName.empty())
		defaultLocale = std::locale(defaultLocaleName);

	// reads RT's params for profile selection
	IniMap rtProfileParams = readIni(argv[1]);

	log << "\nRT key file: " << argv[1]	<< std::endl;
	if (rtProfileParams.empty())
		log << "\nEmpty key file!" << std::endl;
		
	// for debugging, save current RT params file as "LastKeyFile.txt"
	copyFile(argv[1], basePath + "LastKeyFile.txt");

	// necessary parameters for current raw file
	string imageFileName = removeDoubleSlashes(rtProfileParams[RT_KEYFILE_GENERAL_SECTION]["ImageFileName"]);
	string outputProfileFileName = removeDoubleSlashes(rtProfileParams[RT_KEYFILE_GENERAL_SECTION]["OutputProfileFileName"]);
	string cachePath = removeDoubleSlashes(rtProfileParams[RT_KEYFILE_GENERAL_SECTION]["CachePath"]);
	string defaultProcParams = removeDoubleSlashes(rtProfileParams[RT_KEYFILE_GENERAL_SECTION]["DefaultProcParams"]);
	slash = defaultProcParams.find_last_of(SLASH_CHAR);

	// all parameters found in RT's parameter file?
	if (imageFileName.empty() || outputProfileFileName.empty() || cachePath.empty() || defaultProcParams.empty())
	{
		log << "\nInvalid RT ini params file: " << argv[1] << std::endl;
		return 1;
	}

	if (slash == string::npos)
	{
		log << "\nRT misconfigured: default profile *must* be custom, not a bundled one (" << defaultProcParams << ")" << std::endl;
		return 1;
	}

	// default source profile to be copied (reassigned below, if we can find a good match based on Exif)
	string sourceProfile = defaultProcParams;

	// try to get the path where custom profiles are located from RTProfileSelector.ini
	string rtCustomProfilesPath = rtSelectorIni[RTPS_INI_SECTION_GENERAL]["RTCustomProfilesPath"];
	
	// if path not declared in the INI, we assume the default profile is a custom one and extract
	// the path for custom profiles from it
	if (rtCustomProfilesPath.empty())
		rtCustomProfilesPath = defaultProcParams.substr(0, slash);

	// use exiftool to extract Exif values from raw file into 'exif.txt' 
	string exiftool;
	if (rtSelectorIni[RTPS_INI_SECTION_GENERAL]["UseExifTool"] != "0")
	{
		exiftool = rtSelectorIni[RTPS_INI_SECTION_GENERAL]["ExifTool"];
		if (exiftool.empty())
			exiftool = DEFAULT_EXIFTOOL_CMD;
	}

	// check whether a specific viewer is defined in the configuration file
	string exifViewerCmd = rtSelectorIni[RTPS_INI_SECTION_GENERAL]["TextViewer"];
	if (exifViewerCmd.empty())
		exifViewerCmd = DEFAULT_TEXTVIEWER_CMD;

	// reads image Exif values into map (either extracted by exiftool or directly from RT keyfile) 
	StrMap exifFields;
	if (!exiftool.empty())
		exifFields = getExifFields(exiftool, cachePath, imageFileName, log);
	else
		exifFields = getParamsExifFields(rtProfileParams, log);

	// Exif-matched partial profiles list: 
	// list of partial profiles that match the EXIF info for the current image
	StrSetVector partialProfilesList;
	if (exifFields.empty())
	{
		log << "\nCould not read Exif keys from image file: " << imageFileName << std::endl;
	}
	else
	{
		// save the fields in a text file containing "key=value" lines, for easy copying to rules files
		bool viewExifKeys = rtSelectorIni[RTPS_INI_SECTION_GENERAL]["ViewExifKeys"] == "1";
		saveExifFields(exifFields, imageFileName, basePath + "ExifFields.txt", exifViewerCmd, viewExifKeys);

		// check all profile selection rules for a match against the Exif values
		bool useComplexRules = rtSelectorIni[RTPS_INI_SECTION_GENERAL]["ComplexRulesEnabled"] != "0";
		IniMultiMap rtSelectorRulesIni = readMultiIni(basePath + "RTProfileSelectorRules.ini");
		
		// have we found a profile matching the Exif values?
		auto match = matchExifFields(rtSelectorRulesIni, exifFields, useComplexRules);
		if (match != rtSelectorRulesIni.cend())
			sourceProfile = rtCustomProfilesPath + SLASH_CHAR + match->first;
			
		// get matches for partial profiles
		partialProfilesList = getPartialProfilesMatches(rtSelectorIni, rtSelectorRulesIni, exifFields, useComplexRules);
	}

	// matching basic profile selected
	log << "\nBase profile file selected: " << sourceProfile << std::endl;

	// last step: apply any partial profiles (partial rules, lens or ISO-dependent) 
	if (!applyPartialProfiles(log, basePath, rtCustomProfilesPath, rtSelectorIni, exifFields, partialProfilesList, sourceProfile, outputProfileFileName))
	{
		log << "\nError applying rules - operation aborted!" << std::endl;
		return 1;
	}

	// if ViewPP3Debug is enabled, show PP3 debug text file
	if (rtSelectorIni[RTPS_INI_SECTION_GENERAL]["ViewProfileDebug"] == "1")
		executeProcess(exifViewerCmd + " \"" + basePath + "LastProfileDebug.txt" + "\"", "", false);			

	return 0;
}