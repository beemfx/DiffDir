// DiffDir.cpp : Defines the entry point for the console application.
//
#include <conio.h>
#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <string>

#include "Differ.h"

class COutputter: public CDiffer::IDifferCB
{
	public: virtual void OnDiff(CDiffer::DIFF_T diff, LPCSTR strFilename)
	{
		switch(diff)
		{
		case CDiffer::SHOW:printf("\t%s\n", strFilename);break;
		case CDiffer::DATE:printf("\t*: %s\n", strFilename);break;
		case CDiffer::ADDED:printf("\t+: %s\n", strFilename);break;
		case CDiffer::DELETED:printf("\t-: %s\n", strFilename);break;
		//case CDiffer::SAME:printf("\tUNCHANGED: %s\n", strFilename);break;
		//default:printf("%s: %u\n", strFilename, diff);break;
		}
	}
};

COutputter g_Outputter;

void WalkDir(LPCSTR strDirIn, CDiffer& D)
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
			WalkDir(strSubDir.c_str(), D);
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



int _tmain(int argc, _TCHAR* argv[])
{
	printf("DiffDir v1.0 (c) 2011 Beem Software\n");
	//Okay, so the first parameter should be the directory we want to scan, and
	//the second should be the file to write our results to.
	int nResult = 0;
	if(argc < 3)
	{
		printf("Usage DiffDir \"$directory\" \"$outputfile\"\n");
		printf("$directory - The full path to the directory that you want to diff.\n");
		printf("$outputfile - The file that stores information about the directory and compares previous information.\n");
		printf("If $outputfile doesn't exist DiffDir returns 0 and creates the file. If it does exist comparisons are made with the information found and reported and the application returns 1.\n");
		for(int i=0; i<argc; i++)
		{
			printf("argv[%i] = %s\n", i, argv[i]);
		}
	}
	else
	{
		CDiffer DiffScan(&g_Outputter);

		_TCHAR* strDir = argv[1];
		_TCHAR* strOut = argv[2];
		printf("Scanning \"%s\"...\n", strDir);
		WalkDir(strDir, DiffScan);
		printf("Scanning completed.\n");
		//printf("Found the following files:\n");
		//DiffScan.ShowFiles();

		printf("Loading previous revision from \"%s\"...\n", strOut);
		CDiffer DiffPrev(&g_Outputter, strOut);
		//printf("Read in these files...\n");
		//DiffPrev.ShowFiles();

		printf("Comparing revisions...\n");

		DWORD nNumDiffs = (DiffScan == DiffPrev);

		//We only save the new diff info if there were diffs, that way an
		//application can use the output file to detect for changes.

		if(0 != nNumDiffs)
		{
			printf("Updating \"%s\"...\n", strOut);
			DiffScan.SaveDiffInfo(strOut);
		}

		nResult = nNumDiffs;
		printf("%u diffs detected.\n", nNumDiffs);
	}

	#if defined(_DEBUG)
	printf("Press any key...\n");
	_getch();
	#endif
	return nResult;
}

