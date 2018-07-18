// DiffDir.cpp : Defines the entry point for the console application.
//
#include <conio.h>
#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <string>
#include <stdarg.h>

#include "Differ.h"

static bool DiffDir_IsQuiet();

static void DiffDir_printf( const _TCHAR* strFormat, ...)
{
	if(DiffDir_IsQuiet())return;

	va_list args;
	va_start (args, strFormat);
	_vtprintf (strFormat, args);
	va_end (args);
}

class COutputter: public CDiffer::IDifferCB
{
	public: virtual void OnDiff(CDiffer::DIFF_T diff, LPCTSTR strFilename)
	{
		switch(diff)
		{
		case CDiffer::SHOW   :DiffDir_printf(TEXT("\t%s\n"   ), strFilename);break;
		case CDiffer::DATE   :DiffDir_printf(TEXT("\t*: %s\n"), strFilename);break;
		case CDiffer::ADDED  :DiffDir_printf(TEXT("\t+: %s\n"), strFilename);break;
		case CDiffer::DELETED:DiffDir_printf(TEXT("\t-: %s\n"), strFilename);break;
		//case CDiffer::SAME   :DiffDir_printf(TEXT("\tUNCHANGED: %s\n"), strFilename);break;
		//default:DiffDir_printf(TEXT("%s: %u\n"), strFilename, diff);break;
		}
	}
};


enum APP_M
{
	APP_DIFFDIR,
	APP_LISTFILES,
	APP_HELP,
};


//Global Data;
static struct
{
	//Paths:
	const _TCHAR* strInputDir;
	const _TCHAR* strInputFile;
	const _TCHAR* strOutputFile;

	//Application Mode:
	APP_M Mode;

	//Additional flags;
	static const unsigned int F_QUIET = (1 << 0);

	unsigned int Flags;

	//Outputter:
	COutputter Outputter;

} Data;

static bool DiffDir_IsQuiet()
{
	return 0 != (Data.F_QUIET&Data.Flags);
}

static void DiffDir_WalkDir(LPCTSTR strDirIn, CDiffer& D)
{
	std::string strDir(strDirIn);
	if( '\\' != (strDir.at(strDir.length()-1)) )
	{
		strDir.append("\\");
	}

	std::string strSearch(strDir);
	strSearch.append("*");

	//printf("Scanning \"%s\"...\n", strDir.c_str());

	WIN32_FIND_DATA FD;
	HANDLE hFind = FindFirstFile(strSearch.c_str(), &FD);
	if(INVALID_HANDLE_VALUE == hFind)
		return;

	do
	{
		//If we have a directory we behave a little differently.
		if(((FILE_ATTRIBUTE_DIRECTORY&FD.dwFileAttributes) != 0))
		{
			//printf("Found directory.\n");
			if('.' == FD.cFileName[0])
				continue;

			std::string strSubDir(strDir);
			strSubDir.append(FD.cFileName);
			DiffDir_WalkDir(strSubDir.c_str(), D);
		}
		else
		{
			std::string strFile(strDir);
			strFile.append(FD.cFileName);
			//printf("Found %s.\n", strFile.c_str());
			D.AddFile(strFile.c_str(), FD);
		}

	}while(FindNextFile(hFind, &FD));

	FindClose(hFind);
}

static void DiffDir_HandleSingleFile(LPCTSTR strDirIn, CDiffer& D)
{
	WIN32_FIND_DATA FD;
	memset( &FD , 0 , sizeof(FD) );
	HANDLE hFind = FindFirstFile( Data.strInputDir , &FD );
	if( INVALID_HANDLE_VALUE != hFind )
	{
		D.AddFile( Data.strInputDir , FD );
		FindClose( hFind );
	}
}

static int DiffDir_Diff()
{
	int nResult = 0;

	CDiffer DiffScan(&Data.Outputter);

	

	DWORD RootAttributes = GetFileAttributes( Data.strInputDir );
	if( (RootAttributes != INVALID_FILE_ATTRIBUTES) && ((RootAttributes&FILE_ATTRIBUTE_DIRECTORY) == 0 ) )
	{
		DiffDir_printf(TEXT("Diffing file \"%s\"...\n"), Data.strInputDir);
		DiffDir_HandleSingleFile( Data.strInputDir , DiffScan );
	}
	else
	{
		DiffDir_printf(TEXT("Scanning \"%s\"...\n"), Data.strInputDir);
		DiffDir_WalkDir(Data.strInputDir, DiffScan);
	}
	DiffDir_printf(TEXT("Scanning completed.\n"));

	DiffDir_printf(TEXT("Loading previous revision from \"%s\"...\n"), Data.strInputFile);
	CDiffer DiffPrev(&Data.Outputter, Data.strInputFile);

	DiffDir_printf(TEXT("Comparing revisions...\n"));

	DWORD nNumDiffs = (DiffScan == DiffPrev);

	//We only save the new diff info if there were diffs, that way an
	//application can use the output file to detect for changes.

	if( 0 != nNumDiffs )
	{
		DiffDir_printf(TEXT("Updating \"%s\"...\n"), Data.strOutputFile);
		DiffScan.SaveDiffInfo(Data.strOutputFile);
	}

	nResult = nNumDiffs;
	DiffDir_printf(TEXT("%u diffs detected.\n"), nNumDiffs);

	return nResult;
}

static int DiffDir_List()
{
	int nResult = 0;

	CDiffer DiffScan(&Data.Outputter);
	DiffDir_WalkDir(Data.strInputDir, DiffScan);
	DiffScan.ShowFiles();
	DiffScan.SaveFileList(Data.strOutputFile);
	nResult = DiffScan.GetNumFiles();

	return nResult;
}


static void DiffDir_GetParms(int argc, _TCHAR* argv[])
{
	//Clear the application data:
	Data.Mode          = APP_DIFFDIR;
	Data.strInputDir   = NULL;
	Data.strInputFile  = NULL;
	Data.strOutputFile = NULL;
	Data.Flags         = 0;

	//We now check to see what the app parameters are:
	enum NEXT_M
	{
		NEXT_PARM,
		NEXT_INPUTDIR,
		NEXT_INPUTFILE,
		NEXT_OUTPUTFILE,
	};


	NEXT_M NextMode = NEXT_PARM;

	for(int i=0; i<argc; i++)
	{
		const _TCHAR* strParm = argv[i];

		if(NEXT_INPUTDIR == NextMode)
		{
			Data.strInputDir = strParm;
			NextMode = NEXT_PARM;
		}
		else if(NEXT_INPUTFILE == NextMode )
		{
			Data.strInputFile = strParm;
			NextMode = NEXT_PARM;
		}
		else if(NEXT_OUTPUTFILE == NextMode)
		{
			Data.strOutputFile = strParm;
			NextMode = NEXT_PARM;
		}
		else
		{
			NextMode = NEXT_PARM;

			//Let's see what we got:
			if(!_tcsicmp( TEXT( "-Q"),strParm))
			{
				Data.Flags |= Data.F_QUIET;
			}
			else if(!_tcsicmp( TEXT( "-L" ), strParm))
			{
				Data.Mode = APP_LISTFILES;
			}
			else if(!_tcsicmp( TEXT( "-H" ), strParm))
			{
				Data.Mode = APP_HELP;
			}
			else if(!_tcsicmp( TEXT( "-D" ), strParm))
			{
				NextMode = NEXT_INPUTDIR;
			}
			else if(!_tcsicmp( TEXT( "-O" ), strParm))
			{
				NextMode = NEXT_OUTPUTFILE;
			}
			else if(!_tcsicmp( TEXT( "-I" ), strParm))
			{
				NextMode = NEXT_INPUTFILE;
			}
		}
	}

	if( NULL == Data.strInputFile )
	{
		Data.strInputFile = Data.strOutputFile;
	}
}

static int DiffDir_Help(int argc, _TCHAR* argv[])
{
	//Unset the quiet flag if in help mode.
	Data.Flags &= ~Data.F_QUIET;

	DiffDir_printf(TEXT("Usage DiffDir.exe [-H] [-Q] [-L] -D \"inputdirectory\" -O \"outputfile\"\n"));
	DiffDir_printf(TEXT("\tinputdirectory - The full path to the directory that you want to diff.\n"));
	DiffDir_printf(TEXT("\toutputfile - The file that stores information about diff.\n"));
	DiffDir_printf(TEXT("\n\
If outputfile doesn't exist DiffDir creates the file and returns the number\n\
of files found. If it does exist comparisons are made with the information\n\
found and reported and the application returns the number of diffs found. The\n\
outputfile will then be written with current information about the directory.\n"));

	DiffDir_printf(TEXT("\nOptions:\n"));
	DiffDir_printf(TEXT("\t-H: Print this help message.\n"));
	DiffDir_printf(TEXT("\t-Q: Quiet mode, no output is written to the console.\n"));
	DiffDir_printf(TEXT("\t-L: List mode. The output file contains a list of files in the\n\t    directory, no diff is performed.\n"));
	DiffDir_printf(TEXT("\t-D: Indicates that the next parameter will be the input directory.\n"));
	DiffDir_printf(TEXT("\t-I: Indicates that the next parameter will be the input file with the\n\t    state of the directory.\n\
\t    This is optional, if it is not specified the output file will also\n\t    be used as an input file.\n"));
	DiffDir_printf(TEXT("\t-O: Indicates that the next parameter will be the output file.\n"));

	DiffDir_printf(TEXT("\nArguments received:\n"));
	for(int i=0; i<argc; i++)
	{
		DiffDir_printf(TEXT("\targv[%i] = %s\n"), i, argv[i]);
	}

	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	DiffDir_GetParms(argc, argv);

	DiffDir_printf( TEXT("DiffDir 1.1 (c) 2014 Beem Software\n"));
	if( Data.strInputFile == Data.strOutputFile )
	{
		DiffDir_printf( TEXT("Using input file as output file.\n") );
	}

	//If there is no input file or output file, change mode to help.
	if( NULL == Data.strInputDir || NULL == Data.strOutputFile )
		Data.Mode = APP_HELP;

	int nResult = 0;
	switch( Data.Mode )
	{
		case APP_DIFFDIR  : nResult = DiffDir_Diff(); break;
		case APP_LISTFILES: nResult = DiffDir_List(); break;
		case APP_HELP     : nResult = DiffDir_Help(argc, argv); break;
	}

	#if defined(_DEBUG)
	printf("Press any key...\n");
	_getch();
	#endif
	
	return nResult;
}

