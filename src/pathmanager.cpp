#include "pathmanager.h"
#include "definitions.h"

#include <string>
using std::string;

#include <iostream>
using std::endl;

#include <list>
using std::list;

void PATHMANAGER::Init(std::ostream & info_output, std::ostream & error_output)
{
	//figure out the user's home directory
	char *homedir;
	#ifndef _WIN32
	homedir = getenv ( "HOME" );
	if ( homedir == NULL )
	{
		homedir = getenv ( "USER" );
		if ( homedir == NULL )
		{
			homedir = getenv ( "USERNAME" );
			if ( homedir == NULL )
			{
				error_output << "Could not find user's home directory!" << endl;
			}
		}
		home_directory = "/home/";
	}
	#else
	homedir = getenv ( "USERPROFILE" );
	if ( homedir == NULL )
	{
		homedir = "data"; // WIN 9x/Me
	}
	#endif
	home_directory += homedir;
	
	//find data dir
	char *datadir = getenv ( "VDRIFT_DATA_DIRECTORY" );
	if (datadir == NULL) {
		#ifndef _WIN32
		if (FileExists("data/settings/options.config")) {
			data_directory = "data";
		} else {
			data_directory = DATA_DIR;
		}
		#else
		data_directory = "data";
		#endif
	} else {
		data_directory = (string) datadir;
	}
	
	//find settings file
	settings_path = home_directory;
	#ifndef _WIN32
	settings_path += "/";
	settings_path += SETTINGS_DIR;
	MakeDir(settings_path);
	#else
	#ifdef _WIN32
	MakeDir(settings_path+"\\My Documents");
	MakeDir(settings_path+"\\My Documents\\My Games");
	settings_path += "\\My Documents\\My Games\\VDrift";
	MakeDir(settings_path);
	#else
	{
		settings_path += "/";
		settings_path += SETTINGS_DIR;
	}
	#endif
	#endif
	
	MakeDir(GetTrackRecordsPath());
	MakeDir(GetReplayPath());
	MakeDir(GetScreenshotPath());
	
	//print diagnostic info
	info_output << "Home directory: " << home_directory << endl;
	bool settings_file_present = FileExists(GetSettingsFile());
	info_output << "Settings file: " << GetSettingsFile();
	if (!settings_file_present)
		info_output << " (does not exist, will be created)";
	info_output << endl;
	info_output << "Data directory: " << data_directory;
	if (datadir)
		info_output << "\nVDRIFT_DATA_DIRECTORY: " << datadir;
#ifndef _WIN32
	info_output << "\nDATA_DIR: " << DATA_DIR;
#endif
	info_output << endl;
	info_output << "Log file: " << GetLogFile() << endl;
}

bool PATHMANAGER::GetFolderIndex(string folderpath, list <string> & outputfolderlist, std::string extension) const
{
//------Folder listing code for POSIX
#ifndef _WIN32
	DIR *dp;
	struct dirent *ep;
	dp = opendir (folderpath.c_str());
	if (dp != NULL)
	{
		while ( ( ep = readdir( dp ) ) )
		{
			//puts (ep->d_name);
			string newname = ep->d_name;
			if (newname[0] != '.')
			{
				outputfolderlist.push_back(newname);
			}
		}
		(void) closedir (dp);
	}
	else
		return false;
#else 	//------End POSIX-specific folder listing code ---- Start WIN32 Specific code
	HANDLE          hList;
	TCHAR           szDir[MAX_PATH+1];
	WIN32_FIND_DATA FileData;

	// Get the proper directory path
	sprintf(szDir, "%s\\*", folderpath.c_str ());

	// Get the first file
	hList = FindFirstFile(szDir, &FileData);
	if (hList == INVALID_HANDLE_VALUE)
	{ 
		//no files found.  that's OK
	}
	else
	{
		// Traverse through the directory structure
		while (FindNextFile(hList, &FileData))
		{
			// Check the object is a directory or not
			if (FileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			{} else
			{
				if (FileData.cFileName[0] != '.')
				{
					outputfolderlist.push_back (FileData.cFileName);
				}
			}
		}
	}

	FindClose(hList);
#endif //------End WIN32 specific folder listing code
	
	//remove non-matcthing extensions
	if (!extension.empty())
	{
		list <list <string>::iterator> todel;
		for (list <string>::iterator i = outputfolderlist.begin(); i != outputfolderlist.end(); ++i)
		{
			if (i->find(extension) != i->length()-extension.length())
				todel.push_back(i);
		}
		
		for (list <list <string>::iterator>::iterator i = todel.begin(); i != todel.end(); ++i)
			outputfolderlist.erase(*i);
	}
	
	outputfolderlist.sort();
	return true;
}
