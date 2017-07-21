//////////////////////////////////////////////////////////////////////
// CMapViewFileWrite.cpp: 用于映射文件视图（写）
// 
//
// 此程序由 李秋枫/stsm/liqf 编写
//
// 此代码预计将会开源，任何基于此代码的修改发布请保留原作者信息
// 
// 2014.4.24
//////////////////////////////////////////////////////////////////////

#include "MapViewFile.h"




CMapViewFileWrite::CMapViewFileWrite()
{

#ifdef USE_MAX_SINGLE_FILESIZE
	dwViewSizePck = dwViewSizePkx = 0;

	*m_szPckFileName = 0;
	*m_tszPckFileName = 0;

	*m_szPkxFileName = 0;
	*m_tszPkxFileName = 0;
#endif

	isWriteMode = TRUE;
}

CMapViewFileWrite::~CMapViewFileWrite()
{
	//clear();
}

//void CMapViewFileWrite::clear()
//{
//	if(NULL != lpMapAddress)
//		UnmapViewOfFile(lpMapAddress);
//
//	if(NULL != hFileMapping)
//		CloseHandle(hFileMapping);
//
//	if(NULL != hFile && INVALID_HANDLE_VALUE != hFile)
//		CloseHandle(hFile);
//
//	if(hasPkx){
//
//		if(NULL != lpMapAddress2)
//			UnmapViewOfFile(lpMapAddress2);
//
//		if(NULL != hFileMapping2)
//			CloseHandle(hFileMapping2);
//
//		if(NULL != hFile2 && INVALID_HANDLE_VALUE != hFile2)
//			CloseHandle(hFile2);
//	}
//}

#ifdef USE_MAX_SINGLE_FILESIZE

BOOL CMapViewFileWrite::OpenPck(char *lpszFilename, DWORD dwCreationDisposition)
{

	IsPckFile = TRUE;

	if(Open(lpszFilename, dwCreationDisposition)){

		//strcpy_s(m_szPckFilename, MAX_PATH, lpszFilename);

		dwPkxSize = 0;
		GetPkxName(m_szPkxFileName, lpszFilename);
		dwPckSize = ::GetFileSize(hFile, NULL);

		//if(MAX_PCKFILE_SIZE <= dwPckSize){

			//dwMaxPckSize = dwPckSize;

			OpenPkx(m_szPkxFileName, OPEN_EXISTING);

		//}else{
			//dwMaxPckSize = MAX_PCKFILE_SIZE;
		//}

		uqwFullSize.qwValue = dwPckSize + dwPkxSize;

	}else{
		return FALSE;
	}

	return TRUE;

}

BOOL CMapViewFileWrite::OpenPck(LPCTSTR lpszFilename, DWORD dwCreationDisposition)
{

	IsPckFile = TRUE;

	if(Open(lpszFilename, dwCreationDisposition)){

		//wcscpy_s(m_tszPckFileName, MAX_PATH, lpszFilename);

		dwPkxSize = 0;
		GetPkxName(m_tszPkxFileName, lpszFilename);
		dwPckSize = ::GetFileSize(hFile, NULL);

		//if(MAX_PCKFILE_SIZE <= dwPckSize){
			//dwMaxPckSize = dwPckSize;

			OpenPkx(m_tszPkxFileName, OPEN_EXISTING);

		//}else{
			//dwMaxPckSize = MAX_PCKFILE_SIZE;
		//}

		uqwFullSize.qwValue = dwPckSize + dwPkxSize;

	}else{
		return FALSE;
	}

	return TRUE;

}

void CMapViewFileWrite::OpenPkx(char *lpszFilename, DWORD dwCreationDisposition)
{
	//char szFilename[MAX_PATH];

	//GetPkxName(szFilename, lpszFilename);
	HANDLE hFilePck = hFile;

	if(Open(lpszFilename, dwCreationDisposition)){

		hFile2 = hFile;
		hasPkx = TRUE;
		uqdwMaxPckSize.qwValue = dwPckSize;

		dwPkxSize = ::GetFileSize(hFile2, NULL);
	}else{
		uqdwMaxPckSize.qwValue = MAX_PCKFILE_SIZE;
	}

	hFile = hFilePck;
}

void CMapViewFileWrite::OpenPkx(LPCWSTR lpszFilename, DWORD dwCreationDisposition)
{
	//TCHAR szFilename[MAX_PATH];
	//GetPkxName(szFilename, lpszFilename);
	HANDLE hFilePck = hFile;

	if(Open(lpszFilename, dwCreationDisposition)){

		hFile2 = hFile;
		hasPkx = TRUE;
		uqdwMaxPckSize.qwValue = dwPckSize;

		dwPkxSize = ::GetFileSize(hFile2, NULL);

	}else{
		uqdwMaxPckSize.qwValue = MAX_PCKFILE_SIZE;
	}

	hFile = hFilePck;
}

#endif

BOOL CMapViewFileWrite::Open(char *lpszFilename, DWORD dwCreationDisposition)
{
	char szFilename[MAX_PATH];

	//IsPckFile = IsPckFilename(lpszFilename);

	if(INVALID_HANDLE_VALUE == (hFile = CreateFileA(lpszFilename, GENERIC_WRITE | GENERIC_READ, NULL, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL)))
	{
		if(isWinNt())
		{
			MakeUnlimitedPath(szFilename, lpszFilename, MAX_PATH);
			if(INVALID_HANDLE_VALUE == (hFile = CreateFileA(szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL)))
				return FALSE;
		}else{
			return FALSE;
		}

	}

	strcpy_s(m_szPckFileName, MAX_PATH, lpszFilename);

	return TRUE;
}

BOOL CMapViewFileWrite::Open(LPCWSTR lpszFilename, DWORD dwCreationDisposition)
{
	WCHAR szFilename[MAX_PATH];

	//IsPckFile = IsPckFilename(lpszFilename);

	if(INVALID_HANDLE_VALUE == (hFile = CreateFile(lpszFilename, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL)))
	{
		if(isWinNt())
		{
			MakeUnlimitedPath(szFilename, lpszFilename, MAX_PATH);
			if(INVALID_HANDLE_VALUE == (hFile = CreateFile(szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL)))
				return FALSE;
		}else{
			return FALSE;
		}

	}

	wcscpy_s(m_tszPckFileName, MAX_PATH, lpszFilename);

	return TRUE;
}

BOOL CMapViewFileWrite::Mapping(char *lpszNamespace, QWORD qwMaxSize)
{
#ifdef USE_MAX_SINGLE_FILESIZE
	if(IsPckFile && (uqdwMaxPckSize.qwValue < qwMaxSize)){

		if(NULL == (hFileMapping = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, uqdwMaxPckSize.qwValue, lpszNamespace))){
			return FALSE;
		}

		dwPckSize = uqdwMaxPckSize.qwValue;

		if(!hasPkx){
			if(0 != *m_szPkxFileName)
				OpenPkx(m_szPkxFileName, OPEN_ALWAYS);
			else if(0 != *m_tszPkxFileName)
				OpenPkx(m_tszPkxFileName, OPEN_ALWAYS);
			else{
			
				UnMaping();
				return FALSE;
			}
		}

		dwPkxSize = qwMaxSize - uqdwMaxPckSize.qwValue;
		//dwPckSize = dwMaxPckSize;

		//if(dwMaxPckSize < dwPkxSize){
		//	int a= 1;
		//	if(dwMaxPckSize < dwMaxSize){
		//		a=2;
		//	}
		//}

		uqwFullSize.qwValue = qwMaxSize;

		char szNamespace_2[16];
		memcpy(szNamespace_2, lpszNamespace, 16);
		strcat_s(szNamespace_2, 16, "_2");

		if(NULL == (hFileMapping2 = CreateFileMappingA(hFile2, NULL, PAGE_READWRITE, 0, dwPkxSize, szNamespace_2))){
			return FALSE;
		}


	}else
#endif	
	{
		UNQWORD uqwMaxSize;
		uqwMaxSize.qwValue = qwMaxSize;

		if(NULL == (hFileMapping = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, uqwMaxSize.dwValueHigh, uqwMaxSize.dwValue, lpszNamespace))){
			return FALSE;
		}	
	}
	return TRUE;
}

void CMapViewFileWrite::SetEndOfFile()
{
#ifdef USE_MAX_SINGLE_FILESIZE
	if(hasPkx){
		if(::SetEndOfFile(hFile2))
		dwPkxSize = ::SetFilePointer(hFile2, 0, 0, FILE_CURRENT);
	}
#endif
	if(::SetEndOfFile(hFile))
		dwPckSize = ::SetFilePointer(hFile, 0, 0, FILE_CURRENT);

}

DWORD CMapViewFileWrite::Write(LPVOID buffer, DWORD dwBytesToWrite)
{
	DWORD	dwFileBytesWrote = 0;
#ifdef USE_MAX_SINGLE_FILESIZE
	if(hasPkx){

		//DWORD	dwAddressPck, dwAddressPkx;

		//当起始地址大于pck文件时
		if(uqwCurrentPos.qwValue >= uqdwMaxPckSize.qwValue){
			//dwAddressPkx = dwCurrentPos - dwPckSize;

			if(!WriteFile(hFile2, buffer, dwBytesToWrite, &dwFileBytesWrote, NULL))
			{
				return 0;
			}

		}else{
			//UNQWORD	uqwReadEndAT;
			QWORD qwReadEndAT = uqwCurrentPos.qwValue + dwFileBytesWrote;

			//当Read的块全在文件pck内时
			if(qwReadEndAT < uqdwMaxPckSize.qwValue){

				if(!WriteFile(hFile, buffer, dwBytesToWrite, &dwFileBytesWrote, NULL))
				{
					return 0;
				}

			}else{

				//当Read的块在文件pck内和pkx内
				//写pck
				DWORD dwWriteInPck = uqdwMaxPckSize.qwValue - uqwCurrentPos.qwValue;
				DWORD dwWriteInPkx = dwBytesToWrite - dwWriteInPck;

				if(!WriteFile(hFile, buffer, dwWriteInPck, &dwFileBytesWrote, NULL))
				{
					return 0;
				}

				dwWriteInPck = dwFileBytesWrote;
				//写pkx
				if(!WriteFile(hFile2, ((LPBYTE)buffer) + dwWriteInPck, dwWriteInPkx, &dwFileBytesWrote, NULL))
				{
					return 0;
				}

				dwFileBytesWrote += dwWriteInPck;
			}
		}

		uqwCurrentPos.qwValue += dwFileBytesWrote;
	}else
#endif
	{

		if(!WriteFile(hFile, buffer, dwBytesToWrite, &dwFileBytesWrote, NULL))
		{
			return 0;
		}
	}
	return dwFileBytesWrote;
}

