//////////////////////////////////////////////////////////////////////
// PckClassHelpFunction.cpp: PCK文件功能过程中的子功能集合
//
// 此程序由 李秋枫/stsm/liqf 编写，pck结构参考若水的pck结构.txt，并
// 参考了其易语言代码中并于读pck文件列表的部分
//
// 此代码预计将会开源，任何基于此代码的修改发布请保留原作者信息
// 
// 2015.5.19
//////////////////////////////////////////////////////////////////////
#pragma warning ( disable : 4996 )

#include "PckClass.h"

BOOL CPckClass::OpenPckAndMappingRead(CMapViewFileRead *lpRead, LPCTSTR lpFileName)
{

	if(!(lpRead->OpenPck(lpFileName)))
	{
		PrintLogE(TEXT_OPENNAME_FAIL, (TCHAR*)lpFileName, __FILE__, __FUNCTION__, __LINE__);
		return FALSE;
	}

	if(!(lpRead->Mapping(m_szMapNameRead)))
	{
		PrintLogE(TEXT_CREATEMAPNAME_FAIL, (TCHAR*)lpFileName, __FILE__, __FUNCTION__, __LINE__);
		return FALSE;

	}

	return TRUE;

}
BOOL CPckClass::OpenPckAndMappingWrite(CMapViewFileWrite *lpWrite, LPCTSTR lpFileName, DWORD dwCreationDisposition, QWORD qdwSizeToMap)
{
	if (!lpWrite->OpenPck(lpFileName, dwCreationDisposition))
	{
		PrintLogE(TEXT_OPENWRITENAME_FAIL, (TCHAR*)lpFileName, __FILE__, __FUNCTION__, __LINE__);
		return FALSE;
	}

	if(!lpWrite->Mapping(m_szMapNameWrite, qdwSizeToMap))
	{
		PrintLogE(TEXT_CREATEMAPNAME_FAIL, (TCHAR*)lpFileName, __FILE__, __FUNCTION__, __LINE__);
		return FALSE;
	}

	return TRUE;
}


void CPckClass::AfterProcess(CMapViewFileWrite *lpWrite, PCK_ALL_INFOS &PckAllInfo, DWORD dwFileCount, PCKADDR dwAddressName, PCKADDR &dwAddress)
{//mt_dwAddress->dwAddress

	PrintLogI(TEXT_LOG_FLUSH_CACHE);

	//写PCKINDEXADDR
	PckAllInfo.PckIndexAddr.dwCryptDataAddr = dwAddressName ^ m_lpThisPckKey->IndexesEntryAddressCryptKey;
	strcpy(PckAllInfo.PckIndexAddr.szAdditionalInfo,	PCK_ADDITIONAL_INFO
														PCK_ADDITIONAL_INFO_STSM);

	//写pckIndexAddr，272字节
	lpWrite->SetFilePointer(dwAddress, FILE_BEGIN);
	dwAddress += lpWrite->Write(&PckAllInfo.PckIndexAddr, sizeof(PCKINDEXADDR));

	//写pckTail
	PckAllInfo.PckTail.dwFileCount = dwFileCount; //dwFileCount + dwOldPckFileCount;//mt_dwFileCount是实际写入数，重复的已经在上面减去了
	dwAddress += lpWrite->Write(&PckAllInfo.PckTail, sizeof(PCKTAIL));

	//写pckHead
	PckAllInfo.PckHead.dwPckSize = dwAddress;
	lpWrite->SetFilePointer(0, FILE_BEGIN);
	lpWrite->Write(&PckAllInfo.PckHead, sizeof(PCKHEAD));


	//这里将文件大小重新设置一下
	lpWrite->SetFilePointer(dwAddress, FILE_BEGIN);
	lpWrite->SetEndOfFile();

}