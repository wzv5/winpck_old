//////////////////////////////////////////////////////////////////////
// mainfunc.cpp: WinPCK 界面线程部分
// 打开并显示pck文件、查找文件、记录浏览位置 
//
// 此程序由 李秋枫/stsm/liqf 编写
//
// 此代码预计将会开源，任何基于此代码的修改发布请保留原作者信息
// 
// 2012.4.10
// 2012.10.10.OK
//////////////////////////////////////////////////////////////////////

#pragma warning ( disable : 4995 )
#pragma warning ( disable : 4311 )
#pragma warning ( disable : 4005 )

#include "tlib.h"
#include "resource.h"
//#include "globals.h"
#include "winmain.h"
#include <strsafe.h>
#include <shlwapi.h>



LPPCK_PATH_NODE	TInstDlg::GetLastShowNode()
{
	LPPCK_PATH_NODE lpCurrentNode = m_lpPckCenter->m_lpPckRootNode->child;
	LPPCK_PATH_NODE lpCurrentNodeToFind = lpCurrentNode;

	char	**lpCurrentDir = m_PathDirs.lpszDirNames;

	for(int i=0;i<m_PathDirs.nDirCount;i++)
	{
		if(0 == **lpCurrentDir)return lpCurrentNode;

		BOOL isDirFound = FALSE;

		while(NULL != lpCurrentNodeToFind)
		{
			if(0 == strcmp(lpCurrentNodeToFind->szName, *lpCurrentDir))
			{
				//lpCurrentNode = lpCurrentNodeToFind;
				lpCurrentNode = lpCurrentNodeToFind = lpCurrentNodeToFind->child;
				isDirFound = TRUE;

				break;
			}

			lpCurrentNodeToFind = lpCurrentNodeToFind->next;
		}

		if(!isDirFound)
			return lpCurrentNode;

		lpCurrentDir++;

	}
	return lpCurrentNode;

}



BOOL TInstDlg::OpenPckFile(TCHAR *lpszFileToOpen, BOOL isReOpen)
{

	double t1, t2;
	TCHAR	szString[64];

	int iListTopView;


	if(0 != *lpszFileToOpen && lpszFileToOpen != m_Filename)
	{
		StringCchCopy(m_Filename, MAX_PATH, lpszFileToOpen);
	}

	if(!isReOpen)
	{
		memset(&m_PathDirs, 0, sizeof(m_PathDirs));
		*m_PathDirs.lpszDirNames = m_PathDirs.szPaths;
	}else{
		//记录位置
		iListTopView = ListView_GetTopIndex(GetDlgItem(IDC_LIST));
	}

	if(0 != *lpszFileToOpen || OpenSingleFile(m_Filename))
	{
		t1=GetTickCount();

		//转换文件名格式 
		if(m_lpPckCenter->Open(m_Filename))
		{
			t2=GetTickCount() - t1;
			StringCchPrintf(szString, 64, GetLoadStr(IDS_STRING_OPENOK), t2);
			SetStatusBarText(4, szString);

			SetStatusBarText(0, wcsrchr(m_Filename, TEXT('\\')) + 1);
			SetStatusBarText(3, TEXT(""));

			//LPPCKHEAD	lpPckHead;
			StringCchPrintf(szString, 64, GetLoadStr(IDS_STRING_OPENFILESIZE), m_lpPckCenter->GetPckSize());
			SetStatusBarText(1, szString);

			StringCchPrintf(szString, 64, GetLoadStr(IDS_STRING_OPENFILECOUNT), m_lpPckCenter->GetPckFileCount());
			SetStatusBarText(2, szString);	

			//m_lpPckNode = m_lpPckCenter->GetPckPathNode();
			if(isReOpen)
			{
				ShowPckFiles(GetLastShowNode());//////////////1
				ListView_EnsureVisible(GetDlgItem(IDC_LIST), iListTopView, NULL);
				ListView_EnsureVisible(GetDlgItem(IDC_LIST), iListTopView + ListView_GetCountPerPage(GetDlgItem(IDC_LIST)) - 1, NULL);
			}
			else
				ShowPckFiles(m_lpPckCenter->m_lpPckRootNode->child);

			return TRUE;
		}else{
			//SetStatusBarText(4, m_lpPckCenter->GetLastErrorString());
			SetStatusBarText(4, TEXT_PROCESS_ERROR);
			m_lpPckCenter->Close();
			//DeleteClass();
			return FALSE;
		}

		//return FALSE;
	}
	SetStatusBarText(4, GetLoadStr(IDS_STRING_OPENFAIL));
	return FALSE;

}
VOID TInstDlg::SearchPckFiles()
{
	DWORD	dwFileCount = m_lpPckCenter->GetPckFileCount();

	if(0 == dwFileCount)return;

	HWND	hList = GetDlgItem(IDC_LIST);

	LPPCKINDEXTABLE	lpPckIndexTable;
	lpPckIndexTable = m_lpPckCenter->GetPckIndexTable();

	char	szClearTextSize[12], szCipherTextSize[12];
	char	szUnknown1[12]/*, szUnknown2[12]*/;

	LPPCK_PATH_NODE		lpNodeToShowPtr = m_lpPckCenter->m_lpPckRootNode;

	if(NULL == lpNodeToShowPtr)return;

	//m_lpPckCenter->m_isSearchMode = TRUE;
	m_lpPckCenter->SetListInSearchMode(TRUE);

	//显示查找文字
	char	szPrintf[64];
	StringCchPrintfA(szPrintf, 64, GetLoadStrA(IDS_STRING_SEARCHING), m_szStrToSearch);
	//SetStatusBarText(4, szPrintf);
	SendDlgItemMessageA(IDC_STATUS, SB_SETTEXTA, 4, (LPARAM)szPrintf);

	ListView_DeleteAllItems(hList);

	//清除浏览记录
	memset(&m_PathDirs, 0, sizeof(m_PathDirs));
	*m_PathDirs.lpszDirNames = m_PathDirs.szPaths;
	m_PathDirs.nDirCount = 1;

	::SendMessage(hList, WM_SETREDRAW, FALSE, 0);

	int	iListIndex = 1;

	m_currentNodeOnShow = m_lpPckCenter->m_lpPckRootNode->child;

	InsertList(hList, 0, LVIF_PARAM | LVIF_IMAGE, 0, m_lpPckCenter->m_lpPckRootNode, 3, 
					"..",
					"",
					"");


	for(DWORD i = 0;i<dwFileCount;i++)
	{
		if(NULL != strstr(lpPckIndexTable->cFileIndex.szFilename, m_szStrToSearch))
		{
			sprintf_s(szUnknown1, 12, "%.1f%%", lpPckIndexTable->cFileIndex.dwFileCipherTextSize / (double)lpPckIndexTable->cFileIndex.dwFileClearTextSize * 100.0);
			InsertList(hList, iListIndex, LVIF_PARAM | LVIF_IMAGE, 1, lpPckIndexTable, 4, 
						lpPckIndexTable->cFileIndex.szFilename,
						ultoa(lpPckIndexTable->cFileIndex.dwFileClearTextSize, szClearTextSize, 10),
						ultoa(lpPckIndexTable->cFileIndex.dwFileCipherTextSize, szCipherTextSize, 10),	
						szUnknown1);
			iListIndex++;
		}
		lpPckIndexTable++;

	}

	::SendMessage(hList, WM_SETREDRAW, TRUE, 0);

	StringCchPrintfA(szPrintf, 64, GetLoadStrA(IDS_STRING_SEARCHOK), m_szStrToSearch, iListIndex - 1);
	//SetStatusBarText(4, szPrintf);
	SendDlgItemMessageA(IDC_STATUS, SB_SETTEXTA, 4, (LPARAM)szPrintf);

}

VOID TInstDlg::ShowPckFiles(LPPCK_PATH_NODE		lpNodeToShow)
{
	HWND	hList = GetDlgItem(IDC_LIST);

	//m_lpPckCenter->m_isSearchMode = FALSE;
	m_lpPckCenter->SetListInSearchMode(FALSE);

	//DWORD	dwFileCount = m_lpClassPck->GetPckFileCount();
	LPPCKINDEXTABLE	lpPckIndexTable;
	//lpPckIndexTable = m_lpClassPck->GetPckIndexTable();

	char	szClearTextSize[12], szCipherTextSize[12];
	char	szUnknown1[12];

	LPPCK_PATH_NODE		lpNodeToShowPtr = lpNodeToShow;

	if(NULL == lpNodeToShowPtr)return;

	m_currentNodeOnShow = lpNodeToShow;

	ListView_DeleteAllItems(hList);

	::SendMessage(hList, WM_SETREDRAW, FALSE, 0);

	int i = 0;

	while(NULL != lpNodeToShowPtr && 0 != *lpNodeToShowPtr->szName)
	{
		if(NULL == lpNodeToShowPtr->lpPckIndexTable)
		{
			InsertList(hList, i, LVIF_PARAM | LVIF_IMAGE, 0, lpNodeToShowPtr, 3, 
						lpNodeToShowPtr->szName,
						"",
						"");
			i++;
		}
		lpNodeToShowPtr = lpNodeToShowPtr->next;
	}

	lpNodeToShowPtr = lpNodeToShow;
	while(NULL != lpNodeToShowPtr && 0 != *lpNodeToShowPtr->szName)
	{
		if(NULL != lpNodeToShowPtr->lpPckIndexTable)
		{
			lpPckIndexTable = lpNodeToShowPtr->lpPckIndexTable;
			
			sprintf_s(szUnknown1, 12, "%.1f%%", lpPckIndexTable->cFileIndex.dwFileCipherTextSize / (double)lpPckIndexTable->cFileIndex.dwFileClearTextSize * 100.0);

			InsertList(hList, i, LVIF_PARAM | LVIF_IMAGE, 1, lpNodeToShowPtr, 4, 
						lpNodeToShowPtr->szName,
						/*ultoa*/StrFormatByteSizeA(lpPckIndexTable->cFileIndex.dwFileClearTextSize, szClearTextSize, 12/*10*/),
						/*ultoa*/StrFormatByteSizeA(lpPckIndexTable->cFileIndex.dwFileCipherTextSize, szCipherTextSize, 12/*10*/),	
						szUnknown1);
			i++;
		}

		lpNodeToShowPtr = lpNodeToShowPtr->next;
		
	}

	::SendMessage(hList, WM_SETREDRAW, TRUE, 0);

}

