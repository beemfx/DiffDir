/******************************************************************************
DiffDir v1.0 (c) 2011 Beem Softare

DiffDir detects the difference between the contents of a directory and a
previous revision of that directory. It was original built to detect if the
data build for Emergence need to be carried out or not, but it does have quite
a few uses. Namely it could be used for source control, and also for comparing
the contents of two directories (the code can do this, not the app) for backup
purposes, etc.

The usage is simple. It takes two inputs on the command line, one should be
the full path to a directory (by full it should be C:\full\path) no partial
paths or anything like that, and then the second parameter should be the
output file that saves the diff info. If this file doesn't exist it is assumed
that all files have been added, if it does exist then it is used for the
comparison and overwritten once the comparison is complete, so two similar
calls to DiffDir made in succession will almost certainly gaurantee no changes
have been made. DiffDir is meant to compare the contents after some event
has occured.
******************************************************************************/
#include "Differ.h"

#include <sstream>
#include <stdio.h>
#include <tchar.h>

CDiffer::CDiffer(IDifferCB* pDiffer):m_pOut(pDiffer){}
CDiffer::CDiffer(IDifferCB* pDiffer, LPCTSTR strFile):m_pOut(pDiffer){LoadDiffInfo(strFile);}
CDiffer::~CDiffer(){}

void CDiffer::SaveDiffInfo(LPCTSTR strFile)
{
	FILE* fout = _tfopen(strFile, TEXT("w"));
	if(!fout)return;

	//We just create a text file:
	for(CFileTree::const_iterator c = m_Files.cbegin(); c != m_Files.cend(); c++)
	{
		fprintf(fout, "%08X\t%08X\t%s\n", c->second.FileTime.dwHighDateTime, c->second.FileTime.dwLowDateTime, c->first.c_str());
	}

	fclose(fout);
	fout=0;
}

void CDiffer::SaveFileList(LPCTSTR strFile)
{
	FILE* fout = _tfopen(strFile, TEXT("w"));
	if(!fout)return;

	//We just create a text file:
	for(CFileTree::const_iterator c = m_Files.cbegin(); c != m_Files.cend(); c++)
	{
		fprintf(fout, "%s\n", c->first.c_str());
	}

	fclose(fout);
	fout=0;
}

void CDiffer::LoadDiffInfo(LPCTSTR strFile)
{
	FILE* fin = _tfopen(strFile, TEXT("r"));
	if(!fin)return;

	while(!feof(fin))
	{
		char strT[1024]={0};
		fgets(strT, 1023, fin);
		strT[1023] = 0;
		LoadDiffInfo_ParseLine(strT);
	}

	fclose(fin);
	fin=0;
}

void CDiffer::LoadDiffInfo_ParseLine(LPCTSTR strLine)
{
	std::string strL(strLine);
	//If the length is less than 18 then there is no date information
	//so it is probably a bogus line (probably the last line).
	if(strL.length() < 18)return;

	SFileData FI;

	std::string strHigh = strL.substr(0, 8);
	std::string strLow  = strL.substr(9, 8);
	std::string strFile = strL.substr(18, max(0, strL.length()-19));

	FI.strFilename = strFile;

	std::stringstream ss;
	ss << std::hex << strHigh;
	ss >> FI.FileTime.dwHighDateTime;
	ss.clear();
	ss << std::hex << strLow;
	ss >> FI.FileTime.dwLowDateTime;
	//printf("Read the line: \"%s\" %08X %08X\n", strFile.c_str(), FI.FileTime.dwHighDateTime, FI.FileTime.dwLowDateTime);
	m_Files[strFile] = FI;
}


void CDiffer::AddFile(LPCTSTR strFullPath, WIN32_FIND_DATA& FD)
{
	SFileData FI;
	FI.strFilename = strFullPath;
	FI.FileTime = FD.ftLastWriteTime;
	m_Files[FI.strFilename] = FI;
}

void CDiffer::ShowFiles()
{
	for(CFileTree::const_iterator c = m_Files.cbegin(); c != m_Files.cend(); c++)
	{
		m_pOut->OnDiff(this->SHOW, c->first.c_str());
	}
}

int CDiffer::GetNumFiles()const
{
	return m_Files.size();
}

DWORD CDiffer::CompareTo(const CDiffer& rhs)
{
	DWORD nNumDiffs = 0;
	//Okay this is the current file system and rhs is the saved information.
	//We are also gauranteed to ahve a sorted list

	CFileTree::const_iterator cur = m_Files.cbegin();
	CFileTree::const_iterator old = rhs.m_Files.cbegin();

	while(cur != m_Files.cend() || old != rhs.m_Files.cend())
	{
		if(m_Files.cend() == cur)
		{
			//So if the current structure is endend, then everything else in
			//the rhs has been deleted.
			m_pOut->OnDiff(DELETED, old->first.c_str());
			old++;
			nNumDiffs++;
		}
		else if(rhs.m_Files.cend() == old)
		{
			//If the old struct is ended, then everything else in the lhs
			//has been added.
			m_pOut->OnDiff(ADDED, cur->first.c_str());
			cur++;
			nNumDiffs++;
		}
		else
		{
			//Otherwise we are in the case where we are still search through
			//both. So the first thing to do is compare strings. Then there
			//are three cases, either they are the same, in which case there
			//may be a date diff, or they are different, in which case something
			//has been added or deleted.
			
			int nCmp = cur->first.compare(old->first);

			if(0 == nCmp)
			{
				if
				(
					(cur->second.FileTime.dwHighDateTime == old->second.FileTime.dwHighDateTime)
					&&
					(cur->second.FileTime.dwLowDateTime == old->second.FileTime.dwLowDateTime)
				)
				{
					m_pOut->OnDiff(SAME, cur->first.c_str());
				}
				else
				{
					m_pOut->OnDiff(DATE, cur->first.c_str());
					nNumDiffs++;
				}
				cur++;
				old++;
			}
			else if(nCmp < 0)
			{
				m_pOut->OnDiff(ADDED, cur->first.c_str());
				cur++;
				nNumDiffs++;
			}
			else if(nCmp > 0)
			{
				m_pOut->OnDiff(DELETED, old->first.c_str());
				old++;
				nNumDiffs++;
			}
			else
			{
				printf("ERROR\n");
				cur++;
				old++;
			}
		}
	}
	return nNumDiffs;
}
DWORD CDiffer::operator==(const CDiffer& rhs)
{
	return CompareTo(rhs);
}