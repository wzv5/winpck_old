//////////////////////////////////////////////////////////////////////
// PckClassMount.cpp: 用于解析完美世界公司的pck文件中的数据，并显示在List中
// 有关类的初始化等
//
// 此程序由 李秋枫/stsm/liqf 编写，pck结构参考若水的pck结构.txt，并
// 参考了其易语言代码中并于读pck文件列表的部分
//
// 此代码预计将会开源，任何基于此代码的修改发布请保留原作者信息
// 
// 2015.5.27
//////////////////////////////////////////////////////////////////////
#pragma warning ( disable : 4996 )

#include "zlib.h"

#include "PckClass.h"


BOOL CPckClass::MountPckFile(LPCTSTR	szFile)
{

	DWORD	dwFileBytesRead;
	CMapViewFileRead	*lpcReadfile = new CMapViewFileRead();

	if(!OpenPckAndMappingRead(lpcReadfile, szFile)){
		delete lpcReadfile;
		return FALSE;
	}

	//读入m_PckHead，pck文件头
	if(!lpcReadfile->Read(&m_PckAllInfo.PckHead, sizeof(PCKHEAD)))
	{

		PrintLogE(TEXT_READFILE_FAIL, __FILE__, __FUNCTION__, __LINE__);

		delete lpcReadfile;
		return FALSE;
	}

	lpcReadfile->SetFilePointer(-PCK_TAIL_OFFSET, FILE_END);
	
	if(!lpcReadfile->Read(&m_PckAllInfo.PckIndexAddr, sizeof(PCKINDEXADDR)))
	{

		PrintLogE(TEXT_READFILE_FAIL, __FILE__, __FUNCTION__, __LINE__);

		delete lpcReadfile;
		return FALSE;
	}

	//处理附加信息
	char	*lpszAdditionalInfo = m_PckAllInfo.PckIndexAddr.szAdditionalInfo;
	for(int i=0;i<255;i++)
	{
		if(0 == *lpszAdditionalInfo)*lpszAdditionalInfo = 32;
		lpszAdditionalInfo++;
	}
	*lpszAdditionalInfo-- = 0;
	for(int i=0;i<255;i++)
	{
		if(32 != *lpszAdditionalInfo)break;
			*lpszAdditionalInfo = 0;
		lpszAdditionalInfo--;
	}

	if(!lpcReadfile->Read(&(m_PckAllInfo.PckTail), sizeof(PCKTAIL)))
	{
		PrintLogE(TEXT_READFILE_FAIL, __FILE__, __FUNCTION__, __LINE__);

		delete lpcReadfile;
		return FALSE;
	}

	if(NULL == (m_lpThisPckKey = lpPckParams->lpPckVersion->findKeyById(&m_PckAllInfo))){

		m_lpThisPckKey = lpPckParams->lpPckVersion->getInitialKey();

		//打印详细原因：
		char szPrintf[1024];

	#if defined PCKV202 || defined PCKV203ZX
		StringCchPrintfA(szPrintf, 1024, "调试信息："
						"HEAD->dwHeadCheckHead = 0x%08x, "
						"HEAD->dwPckSize = 0x%08x, "
						"HEAD->dwHeadCheckTail = 0x%08x, "
						"TAIL->dwFileCount = 0x%08x, "
						"TAIL->dwVersion = 0x%08x, "
						"INDEX->dwIndexTableCheckHead = 0x%08x, "
						"INDEX->dwCryptDataAddr = 0x%08x, "
						"INDEX->dwIndexTableCheckTail = 0x%08x",\
						m_PckAllInfo.PckHead.dwHeadCheckHead, \
						m_PckAllInfo.PckHead.dwPckSize, \
						m_PckAllInfo.PckHead.dwHeadCheckTail, \
						m_PckAllInfo.PckTail.dwFileCount, \
						m_PckAllInfo.PckTail.dwVersion, \
						m_PckAllInfo.PckIndexAddr.dwIndexTableCheckHead, \
						m_PckAllInfo.PckIndexAddr.dwCryptDataAddr, \
						m_PckAllInfo.PckIndexAddr.dwIndexTableCheckTail);



	#elif defined PCKV203
		StringCchPrintfA(szPrintf, 1024, "调试信息："
						"HEAD->dwHeadCheckHead = 0x%08x, "
						"HEAD->dwPckSize = 0x%016llx, "
						"TAIL->dwFileCount = 0x%08x, "
						"TAIL->dwVersion = 0x%08x, "
						"INDEX->dwIndexTableCheckHead = 0x%08x, "
						"INDEX->dwCryptDataAddr = 0x%016llx, "
						"INDEX->dwIndexTableCheckTail = 0x%08x", \
						m_PckAllInfo.PckHead.dwHeadCheckHead, \
						m_PckAllInfo.PckHead.dwPckSize, \
						m_PckAllInfo.PckTail.dwFileCount, \
						m_PckAllInfo.PckTail.dwVersion, \
						m_PckAllInfo.PckIndexAddr.dwIndexTableCheckHead, \
						m_PckAllInfo.PckIndexAddr.dwCryptDataAddr, \
						m_PckAllInfo.PckIndexAddr.dwIndexTableCheckTail);


	#endif

		PrintLogE(TEXT_UNKNOWN_PCK_FILE);
		PrintLogD(szPrintf);
		
		delete lpcReadfile;
		return FALSE;
	}

	if(!AllocIndexTableAndInit(m_lpPckIndexTable, m_PckAllInfo.PckTail.dwFileCount)){
		delete lpcReadfile;
		return FALSE;
	}

	//文件名索引起始地址	FILEINDEX_ADDR_CONST		0xA8937462
	m_PckAllInfo.dwAddressName = m_PckAllInfo.PckIndexAddr.dwCryptDataAddr ^ m_lpThisPckKey->IndexesEntryAddressCryptKey;

	LPPCKINDEXTABLE lpPckIndexTable = m_lpPckIndexTable;
	BOOL			isLevel0;
	DWORD			byteLevelKey;

	//pck是压缩时，文件名的压缩长度不会超过0x100，所以当
	//开始一个字节，如果0x75，就没有压缩，如果是0x74就是压缩的	0x75->FILEINDEX_LEVEL0
	//
	lpcReadfile->SetFilePointer(m_PckAllInfo.dwAddressName, FILE_BEGIN);

	if(!lpcReadfile->Read(&byteLevelKey, 4))
	{
		PrintLogE(TEXT_READFILE_FAIL, __FILE__, __FUNCTION__, __LINE__);

		delete lpcReadfile;
		return FALSE;
	}

	byteLevelKey ^= m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey1;

	isLevel0 = (INDEXTABLE_CLEARTEXT_LENGTH == byteLevelKey)/* ? TRUE : FALSE*/;

	//开始读文件
	BYTE	*lpFileBuffer;

	if(NULL == (lpFileBuffer = lpcReadfile->View(m_PckAllInfo.dwAddressName, lpcReadfile->GetFileSize() - m_PckAllInfo.dwAddressName)))
	{
		PrintLogE(TEXT_VIEWMAP_FAIL, __FILE__, __FUNCTION__, __LINE__);

		delete lpcReadfile;
		return FALSE;
	}

	if(isLevel0)
	{
		for(DWORD i = 0;i<m_PckAllInfo.PckTail.dwFileCount;i++)
		{

			memcpy(lpPckIndexTable, lpFileBuffer, dwFileBytesRead = (8 + INDEXTABLE_CLEARTEXT_LENGTH));

			lpPckIndexTable->dwIndexDataLength = lpPckIndexTable->dwIndexValueHead ^ m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey1;

			if( (lpPckIndexTable->dwIndexValueTail ^ m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey2) != lpPckIndexTable->dwIndexDataLength)
			{

				PrintLogE(TEXT_READ_INDEX_FAIL, __FILE__, __FUNCTION__, __LINE__);

				delete lpcReadfile;
				return FALSE;
			}

			lpFileBuffer += dwFileBytesRead;
			++lpPckIndexTable;

		}
	}else{

		dwFileBytesRead = INDEXTABLE_CLEARTEXT_LENGTH;

		for(DWORD i = 0;i<m_PckAllInfo.PckTail.dwFileCount;++i)
		{

			memcpy(lpPckIndexTable, lpFileBuffer, 8);
			lpFileBuffer += 8;

			lpPckIndexTable->dwIndexDataLength = lpPckIndexTable->dwIndexValueHead ^ m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey1;

			if( (lpPckIndexTable->dwIndexValueTail ^ m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey2) != lpPckIndexTable->dwIndexDataLength)
			{

				PrintLogE(TEXT_READ_INDEX_FAIL, __FILE__, __FUNCTION__, __LINE__);

				delete lpcReadfile;
				return FALSE;
			}

			uncompress((BYTE*)&lpPckIndexTable->cFileIndex, &dwFileBytesRead,
						lpFileBuffer, lpPckIndexTable->dwIndexDataLength);

			lpFileBuffer += lpPckIndexTable->dwIndexDataLength;
			++lpPckIndexTable;

		}
	}

	delete lpcReadfile;

	return TRUE;
}

void CPckClass::BuildDirTree()
{

	LPPCKINDEXTABLE lpPckIndexTable = m_lpPckIndexTable;

	for(DWORD i = 0;i<m_PckAllInfo.PckTail.dwFileCount;++i)
	{
		//建立目录
		AddFileToNode(&m_RootNode, lpPckIndexTable);
		++lpPckIndexTable;
	}

}
