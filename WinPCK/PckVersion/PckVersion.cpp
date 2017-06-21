//////////////////////////////////////////////////////////////////////
// PckVersion.cpp: 用于解析完美世界公司的pck文件中的数据，并显示在List中
// 头文件
//
// 此程序由 李秋枫/stsm/liqf 编写，
//
// 此代码预计将会开源，任何基于此代码的修改发布请保留原作者信息
// 
// 2015.5.11
//////////////////////////////////////////////////////////////////////


#include "PckVersion.h"


#ifdef PCKV202
																			//ver	HeadChkHead HeadChkTail  TblChkHead IndexValue	TblChkTail	
PCK_KEYS cPckKeys[10] = {	{"诛仙", TEXT("诛仙"),	PCK_VERSION_ZX,			0x20002, 0x4DCA23EF, 0x56A089B7, 0xFDFDFEEE, 0xA8937462, 0xF00DBEEF, 0xA8937462, 0xF1A43653},\
							{"圣斗士",TEXT("圣斗士"), PCK_VERSION_SDS,		0x20002, 0x4DCA23EF, 0x56A089B7, 0x7b2a7820, 0x62a4f9e1, 0xa75dc142, 0x62a4f9e1, 0x3520c3d5},\
							{0},\
						};
#elif defined PCKV203
PCK_KEYS cPckKeys[10] = {	{"笑傲江湖",TEXT("笑傲江湖"), PCK_VERSION_XAJH,0x20003, 0x5edb34f0, 0x00000000, 0x7b2a7820, 0x49ab7f1d33c3eddb, 0xa75dc142, 0x62a4f9e1, 0x3520c3d5},\
							{0},\
						};
#elif defined PCKV203ZX
PCK_KEYS cPckKeys[10] = { { "诛仙", TEXT("诛仙"),	PCK_VERSION_ZX,			0x20003, 0x4DCA23EF, 0x00000000, 0xFDFDFEEE, 0xA8937462, 0xF00DBEEF, 0xA8937462, 0xF1A43653 },\
						{0},\
						};
#endif

CPckVersion::CPckVersion()
{
	lp_defaultPckKey = cPckKeys;
	*szSaveDlgFilterString = 0;
	LPPCK_KEYS lpKeyPtr = cPckKeys;

	TCHAR szPrintf[256];

	while(lpKeyPtr->id){
		StringCchPrintf(szPrintf, 256, TEXT("%sPCK文件(*.pck)|*.pck|"), lpKeyPtr->tname);
		StringCchCat(szSaveDlgFilterString, 1024, szPrintf);

		++lpKeyPtr;
	}

	//StringCchCat(szSaveDlgFilterString, 1024, TEXT("|"));

	TCHAR *lpszStr = szSaveDlgFilterString;
	while(*lpszStr){

		if(TEXT('|') == *lpszStr)
			*lpszStr = 0;
		++lpszStr;
	}

	*lpszStr = 0;

}

CPckVersion::~CPckVersion()
{
	
}

LPCTSTR CPckVersion::GetSaveDlgFilterString()
{
	return szSaveDlgFilterString;
}

LPPCK_KEYS CPckVersion::getInitialKey()
{
	return cPckKeys;
}


LPPCK_KEYS CPckVersion::getCurrentKey()
{
	return lp_defaultPckKey;
}

LPPCK_KEYS CPckVersion::GetKey(int verID)
{
	return &cPckKeys[verID];
}

LPPCK_KEYS CPckVersion::findKeyByFileName(LPTSTR lpszPckFile)
{
	//PCKHEAD cPckHead;
	//PCKINDEXADDR cPckIndexAddr;
	//PCKTAIL cPckTail;
	PCK_ALL_INFOS pckAllInfo;

	CMapViewFileRead *lpRead = new CMapViewFileRead();

	if(!lpRead->OpenPck(lpszPckFile))
	{
		delete lpRead;
		return NULL;
	}

	if(!lpRead->Read(&pckAllInfo.PckHead, sizeof(PCKHEAD)))
	{
		delete lpRead;
		return NULL;
	}

	lpRead->SetFilePointer(-PCK_TAIL_OFFSET, FILE_END);

	if(!lpRead->Read(&pckAllInfo.PckIndexAddr, sizeof(PCKINDEXADDR)))
	{
		delete lpRead;
		return NULL;
	}

	if(!lpRead->Read(&pckAllInfo.PckTail, sizeof(PCKTAIL)))
	{
		delete lpRead;
		return NULL;
	}

	delete lpRead;

	return findKeyById(&pckAllInfo);
}

LPPCK_KEYS CPckVersion::findKeyById(LPPCK_ALL_INFOS lpPckAllInfo/*, CMapViewFileRead *lpRead*/)
{

	BOOL isFound = FALSE;
	//查找匹配项
	LPPCK_KEYS lpKeyPtr = cPckKeys;

	while(lpKeyPtr->id){

		if(	lpKeyPtr->HeadVerifyKey1 == lpPckAllInfo->PckHead.dwHeadCheckHead &&
#if defined PCKV202 || defined PCKV203ZX
			lpKeyPtr->HeadVerifyKey2 == lpPckAllInfo->PckHead.dwHeadCheckTail &&
#endif
			lpKeyPtr->TailVerifyKey1 == lpPckAllInfo->PckIndexAddr.dwIndexTableCheckHead &&
			lpKeyPtr->TailVerifyKey2 == lpPckAllInfo->PckIndexAddr.dwIndexTableCheckTail &&
			lpKeyPtr->Version == lpPckAllInfo->PckTail.dwVersion){

				isFound = TRUE;

				lp_defaultPckKey = lpKeyPtr;
				//memcpy(cPckKeys, lpKeyPtr, sizeof(PCK_KEYS));

				//if(cPckKeys->Version != lpPckTail->dwVersion)
				//	cPckKeys->Version = lpPckTail->dwVersion;

				break;
			//return lpKeyPtr;
		}

		++lpKeyPtr;
	}
	////检查文件头
	//if(PCKHEAD_VERIFY_HEAD != lpPckHead->dwHeadCheckHead || PCKHEAD_VERIFY_TAIL != lpPckHead->dwHeadCheckTail)
	//{
	//	//lstrcpy(m_lastErrorString, TEXT("PCK文件头损坏！"));
	//	//delete lpcReadfile;

	//	return NULL;
	//}


	////检查文件尾	FILEINDEX_VERIFY_HEAD		0xFDFDFEEE	#define	FILEINDEX_VERIFY_TAIL		0xF00DBEEF
	//if(FILEINDEX_VERIFY_HEAD != lpPckIndexAddr->dwIndexTableCheckHead || FILEINDEX_VERIFY_TAIL != lpPckIndexAddr->dwIndexTableCheckTail)
	//{
	//	//if(!ConfirmErrors(TEXT("PCK文件尾损坏，是否继续？"), NULL, MB_YESNO | MB_ICONQUESTION)){
	//	//	lstrcpy(m_lastErrorString, TEXT("PCK文件尾损坏！"));
	//		
	//		//delete lpcReadfile;
	//		return NULL;
	//	//}
	//}

	//if(!isFound){
	//	cPckKeys->HeadVerifyKey1 = lpPckHead->dwHeadCheckHead;
	//	cPckKeys->HeadVerifyKey2 = lpPckHead->dwHeadCheckTail;
	//	cPckKeys->TailVerifyKey1 = lpPckIndexAddr->dwIndexTableCheckHead;
	//	cPckKeys->TailVerifyKey2 = lpPckIndexAddr->dwIndexTableCheckTail;
	//	cPckKeys->Version = lpPckTail->dwVersion;
	//	isFound = guessUnknowKey(lpRead, lpPckTail->dwFileCount, lpPckIndexAddr);

	//}
	if(isFound){
		//strcat(cPckKeys->name, "-当前配置");
		return lp_defaultPckKey;// = cPckKeys;
	}else{
		lp_defaultPckKey = cPckKeys;
		return NULL;
	}

}


//BOOL CPckVersion::guessUnknowKey(CMapViewFileRead *lpRead, DWORD dwFileCount, LPPCKINDEXADDR lpPckIndexAddr)
//{
//	////m_dwAddressName start at = filesize - 280 - 0x114 * dwFileCount
//	////m_PckIndexAddr.dwIndexValue
//	////m_dwAddressName = m_PckIndexAddr.dwIndexValue ^ m_lpThisPckKey->IndexesEntryAddressCryptKey;
//	//DWORD	dwAddress280 = lpRead->GetFileSize() - PCK_TAIL_OFFSET;
//	//DWORD	dwSearchAreaSize = INDEXTABLE_CLEARTEXT_LENGTH * dwFileCount;
//	//DWORD	dwSearchAreaStart = dwAddress280 - dwSearchAreaSize;
//
//	//DWORD	dwGuessAssistKey1 = dwAddress280 ^ dwSearchAreaStart;
//
//	//DWORD	dwPaternKey1 = lpPckIndexAddr->dwIndexValue ^ 0xef2b4;
//
//	return FALSE;
//}

