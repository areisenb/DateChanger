// DateChanger.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


//bool ListDirectoryContents(const wchar_t *sDir)
//{ 
//    WIN32_FIND_DATA fdFile; 
//    HANDLE hFind = NULL; 
//
//    wchar_t sPath[2048]; 
//
//    //Specify a file mask. *.* = We want everything! 
//    wsprintf(sPath, L"%s\\*.*", sDir); 
//
//    if((hFind = FindFirstFile(sPath, &fdFile)) == INVALID_HANDLE_VALUE) 
//    { 
//        wprintf(L"Path not found: [%s]\n", sDir); 
//        return false; 
//    } 
//
//    do
//    { 
//        //Find first file will always return "."
//        //    and ".." as the first two directories. 
//        if(wcscmp(fdFile.cFileName, L".") != 0
//                && wcscmp(fdFile.cFileName, L"..") != 0) 
//        { 
//            //Build up our file path using the passed in 
//            //  [sDir] and the file/foldername we just found: 
//            wsprintf(sPath, L"%s\\%s", sDir, fdFile.cFileName); 
//
//            //Is the entity a File or Folder? 
//            if(fdFile.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY) 
//            { 
//                wprintf(L"Directory: %s\n", sPath); 
//                ListDirectoryContents(sPath); //Recursion, I love it! 
//            } 
//            else{ 
//                wprintf(L"File: %s\n", sPath); 
//            } 
//        }
//    } 
//    while(FindNextFile(hFind, &fdFile)); //Find the next file. 
//
//    FindClose(hFind); //Always, Always, clean things up! 
//
//    return true; 
//} 

void usage ()
{
	printf ("DataChanger - changes capture date of NEF and jpg Files\n");
    printf (" usage: [options] file specifier\n");
	printf ("   -o original timestamp\n");
	printf ("   -t target timestamp\n");
	printf ("   -d directory\n");
	printf ("   -n change count - expected 3 for jpg, 4 for NEF file\n");
	printf (" example: DateChanger -o 2006-01-01 3:00:18 -t 2014-10-19 10:03:08 -d d:\\data *.jpg\n");
}

bool loadSYSTEMTIME (int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, SYSTEMTIME* ptime)
{
	bool bRet = false;
	if ((nYear>1980) && (nYear<2100)) {
		ptime->wYear = nYear;
		if ((nMonth>0) && (nMonth<13)) {
			ptime->wMonth = nMonth;
			if ((nDay>0) && (nDay<32)) {
				ptime->wDay = nDay;
				if ((nHour>=0) && (nHour<24)) {
					ptime->wHour = nHour;
					if ((nMin>=0) && (nMin<60)) {
						ptime->wMinute = nMin;
						if ((nSec>=0) && (nSec<60)) {
							ptime->wSecond = nSec;
							ptime->wDayOfWeek = 0;
							ptime->wMilliseconds = 0;
							bRet = true;
						}
					}
				}
			}
		}
	}
	return bRet;
}

bool readTime (_TCHAR* cpDate, _TCHAR* cpTime, SYSTEMTIME* ptime)
{
	bool bRet = false;
	int nYear, nMonth, nDay, nHour, nMin, nSec;
	nYear = nMonth = nDay = nHour = nMin = nSec = 0;

	_ASSERT (cpDate);
	_ASSERT (cpTime);
	if (_stscanf_s (cpDate, _T ("%d-%d-%d"), &nYear, &nMonth, &nDay) == 3) 
		if (_stscanf_s (cpTime, _T ("%d:%d:%d"), &nHour, &nMin, &nSec) == 3)
			bRet =loadSYSTEMTIME (nYear, nMonth, nDay, nHour, nMin, nSec, ptime);

	return bRet;
}

int conv2HexNanoSec (SYSTEMTIME *pst, ULONGLONG *pul)
{
	FILETIME ft;
	ULARGE_INTEGER uli;

	if (SystemTimeToFileTime(pst, &ft) == 0)
		return -1;
	uli.LowPart = ft.dwLowDateTime ;
	uli.HighPart= ft.dwHighDateTime;
	*pul = uli.QuadPart;
	return 0;
}

int conv2SYSTIME  (SYSTEMTIME *pst, ULONGLONG ull)
{
	FILETIME ft;
	ULARGE_INTEGER uli;

	uli.QuadPart = ull;
	ft.dwLowDateTime = uli.LowPart;
	ft.dwHighDateTime = uli.HighPart;

	if (FileTimeToSystemTime (&ft, pst) == 0)
		return -1;

	return 0;
}

int ChangeDateInFile (_TCHAR* cpFileSpec, ULONGLONG ulDiff, SYSTEMTIME* pst, int nExpCntChange)
{
	FILE* fFile;
	HANDLE hFile;
	DWORD dwFileSize;
	char* cpIn;
	char szSearchString [20];
	char* cpIter;
	char* cpFound;
	int nCntChange = 0;
	
	sprintf (szSearchString, "%04d:%02d:%02d", pst->wYear, pst->wMonth, pst->wDay);

	fFile = _tfopen (cpFileSpec, "rb");
	if (fFile) {
		hFile = (HANDLE) _get_osfhandle (_fileno (fFile));
		dwFileSize = GetFileSize (hFile, NULL);
		cpIn = (char*) malloc (dwFileSize+1);
		cpIn [dwFileSize] = 0;
		fread (cpIn, 1, dwFileSize, fFile);
		_tprintf ("File: %s Size: %u kByte\n", cpFileSpec, dwFileSize/1024);

		cpIter = cpIn;
		do {
			cpFound = (char*) memchr (cpIter, szSearchString [0], dwFileSize - (cpIter-cpIn));
			if (cpFound) {
				cpIter = cpFound;
				cpFound = strstr (cpFound, szSearchString);
			}
			if (cpFound) {
				int nYear, nMonth, nDay, nHour, nMin, nSec;
				SYSTEMTIME stime;
				ULONGLONG ullTime;
				char szOrgDate [20];
				char szTarDate [20];

				memset (szOrgDate,0, sizeof (szOrgDate));
				memcpy (szOrgDate, cpFound, sizeof (szOrgDate) -1);

				if (sscanf (szOrgDate, "%d:%d:%d %d:%d:%d", &nYear, &nMonth, &nDay,&nHour, &nMin, &nSec) == 6)
					if (loadSYSTEMTIME (nYear, nMonth, nDay, nHour, nMin, nSec, &stime))
						if (conv2HexNanoSec (&stime, &ullTime) == 0) 
							if (conv2SYSTIME (&stime, ullTime + ulDiff) == 0) {
								sprintf (szTarDate, "%04d:%02d:%02d %02d:%02d:%02d", 
									stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond);
								printf ("  changed from: %s to: %s\n", szOrgDate, szTarDate);
								memcpy (cpFound, szTarDate, strlen (szTarDate));
								nCntChange++;
							}
				cpIter += strlen (cpIter)+1;
			} else {
				cpIter += strlen (cpIter)+1;
			}
		} while (cpIter < (cpIn+dwFileSize));

		fclose (fFile);

		if (nCntChange == nExpCntChange)  {
			if (fFile = _tfopen (cpFileSpec, "wb")) {
				fwrite (cpIn, 1, dwFileSize, fFile);
				fclose (fFile);
				printf ("  %d occurences replaced \n", nCntChange);
			}
			else printf ("Could not write file: %s\n", cpFileSpec);
		} else printf ("  %d occurences did not match to %d expected\n", nCntChange, nExpCntChange);

		free (cpIn);
	} else {
		printf ("error opening file:%s\n", cpFileSpec);
		return -1;
	}

	return 0;
}

int ChangeDate (_TCHAR* cpDirSpec, _TCHAR* szFileMask, ULONGLONG ullDiff, SYSTEMTIME* pst, int nCntChange)
{
    WIN32_FIND_DATA fdFile; 
    HANDLE hFind = NULL; 
	_TCHAR szDirSpec [255];

	_tcscpy_s (szDirSpec, cpDirSpec);
	_tcscat_s (szDirSpec, _T("\\"));
	_tcscat_s (szDirSpec, szFileMask);

    if((hFind = FindFirstFile(szDirSpec, &fdFile)) == INVALID_HANDLE_VALUE) 
    { 
        _tprintf(_T ("Path not found: [%s]\n"), szDirSpec); 
        return false; 
    } 

	do
    { 
        //Find first file will always return "."
        //    and ".." as the first two directories. 
        if ((_tcsicmp(fdFile.cFileName, _T (".")) != 0)
                && (_tcsicmp(fdFile.cFileName, _T ("..")) != 0) 
				&& !(fdFile.dwFileAttributes & (FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_DIRECTORY)))
		{ 
           _stprintf_s (szDirSpec, "%s\\%s", cpDirSpec, fdFile.cFileName); 
		   ChangeDateInFile (szDirSpec, ullDiff, pst, nCntChange);
        }
    } 
    while(FindNextFile(hFind, &fdFile)); //Find the next file. 

    FindClose(hFind); //Always, Always, clean things up! 
}

int _tmain(int argc, _TCHAR* argv[])
{
	SYSTEMTIME stOrgTime, stTarTime;
	bool bOrgTime = false;
	bool bTarTime = false;
	bool bErrArg = false;
	int nOptCnt;
	_TCHAR szOrgTime [256];
	_TCHAR szTarTime [256];
	ULONGLONG ullOrgTime;
	ULONGLONG ullTarTime;
	_TCHAR* cpDirSpec = NULL;
	int nCntChange = 0;

	if (argc < 8) {
		usage ();
		return -1;
	}

	for (nOptCnt=1; (nOptCnt<argc) && !(bOrgTime && bTarTime && cpDirSpec); nOptCnt++) {
		if (_tcsicmp (_T ("-o"), argv[nOptCnt]) == 0) {
			bOrgTime = readTime (argv[nOptCnt+1], argv[nOptCnt+2], &stOrgTime);
			nOptCnt += 2;
		} else if (_tcsicmp (_T ("-t"), argv[nOptCnt]) == 0) {
			bTarTime = readTime (argv[nOptCnt+1], argv[nOptCnt+2], &stTarTime);
			nOptCnt += 2;
		} else if (_tcsicmp (_T ("-d"), argv[nOptCnt]) == 0) {
			cpDirSpec = argv[nOptCnt+1];
			nOptCnt += 1;
		} else if (_tcsicmp (_T ("-n"), argv[nOptCnt]) == 0) {
			nCntChange = atoi (argv[nOptCnt+1]);
			nOptCnt += 1;
		} else bErrArg = true;
	}

	if (!bOrgTime) {
		printf ("Error Original Time not set properly!\n");
		usage ();
		return -1;
	}
	if (!bTarTime) {
		printf ("Error Original Time not set properly!\n");
		usage ();
		return -1;
	}
	if (bErrArg) {
		usage ();
		return -1;
	}

	if (!cpDirSpec) {
		printf ("No Directory specified\n");
		usage ();
		return -1;
	}

	GetDateFormat (LOCALE_USER_DEFAULT, DATE_AUTOLAYOUT, &stOrgTime, NULL, szOrgTime, sizeof (szOrgTime) / sizeof (szOrgTime[0]));
	GetDateFormat (LOCALE_USER_DEFAULT, DATE_AUTOLAYOUT, &stTarTime, NULL, szTarTime, sizeof (szTarTime) / sizeof (szTarTime[0]));

	conv2HexNanoSec (&stOrgTime, &ullOrgTime);
	conv2HexNanoSec (&stTarTime, &ullTarTime);

	while (nOptCnt < argc) {
		ChangeDate (cpDirSpec, argv [nOptCnt], ullTarTime-ullOrgTime, &stOrgTime, nCntChange);
		nOptCnt++;
	}
	


	return 0;
}



