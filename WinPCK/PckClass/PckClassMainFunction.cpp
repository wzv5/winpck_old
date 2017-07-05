//////////////////////////////////////////////////////////////////////
// PckClassFunction.cpp: 用于解析完美世界公司的pck文件中的数据，并显示在List中
// 一些其他的功能 
//
// 此程序由 李秋枫/stsm/liqf 编写
//
// 此代码预计将会开源，任何基于此代码的修改发布请保留原作者信息
// 
// 2012.4.10
//////////////////////////////////////////////////////////////////////

#pragma warning ( disable : 4267 )
#pragma warning ( disable : 4311 )

#ifdef _DEBUG
#include "tlib.h"
#endif

#include "zlib.h"
#include "PckClass.h"

#pragma warning ( disable : 4996 )

//_inline int __fastcall strpathcmp (const char * src, char * &dst)
//{
//	int ret = 0 ;
//
//	while( (! (ret = (int)(*src - *dst))) && *src )
//		++src, ++dst;
//	if(0 != ret && 0 == *src && ('\\' == *dst || '/'== *dst))return 0;
//	return( ret );
//}

//strpathcmp
//大多数情况下都是存在相同的文件夹
//此时ret != 0;
//
_inline int __fastcall strpathcmp (const char * src, char * &dst)
{
	int ret = 0 ;

	while( (! (ret = (int)(*src - *dst))) && *src )
		++src, ++dst;

	if(0 != ret && 0 == *src && ('\\' == *dst || '/'== *dst))return 0;
	return( ret );
}

_inline void __fastcall strpathcpy(char * dst, char * &src)
{
	while( (*dst++ = *src) && '\\' != *++src && '/' != *src)
		; 
}

_inline char * __fastcall mystrcpy(char * dest,const char *src)
{
	while ((*dest = *src))
		++dest, ++src;
	return dest;
}

/********************************
*
*公共函数
*
********************/

BOOL CPckClass::RebuildPckFile(LPTSTR szRebuildPckFile)
{
	//char		szLogString[LOG_BUFFER];

	PrintLogI(TEXT_LOG_REBUILD);

	LPPCKINDEXTABLE_COMPRESS	lpPckIndexTable, lpPckIndexTablePtr;

	PCKADDR	dwAddress = PCK_DATA_START_AT, dwAddressName;
	DWORD	dwFileCount = m_PckAllInfo.PckTail.dwFileCount;
//#ifdef PCKV202
//	DWORD	dwTotalFileSizeAfterRebuild = m_PckAllInfo.PckHead.dwPckSize + (dwFileCount<<3);
//#elif defined PCKV203
	PCKADDR	dwTotalFileSizeAfterRebuild = m_PckAllInfo.PckHead.dwPckSize + (dwFileCount<<3);
//#endif
	
	PCK_ALL_INFOS	pckAllInfo;


	LPBYTE		lpBufferToWrite;
	CMapViewFileWrite	*lpFileWrite;
	CMapViewFileRead	*lpFileRead;

	//构建头
	pckAllInfo.PckHead.dwHeadCheckHead = m_lpThisPckKey->HeadVerifyKey1;
#if defined PCKV202
	pckAllInfo.PckHead.dwHeadCheckTail = m_lpThisPckKey->HeadVerifyKey2;
#endif

	//pckHead.dwPckSize = |PCK文件大小|;

	memset(&pckAllInfo.PckIndexAddr, 0, sizeof(PCKINDEXADDR));

	pckAllInfo.PckIndexAddr.dwIndexTableCheckHead = m_lpThisPckKey->TailVerifyKey1;
	pckAllInfo.PckIndexAddr.dwIndexTableCheckTail = m_lpThisPckKey->TailVerifyKey2;

	//pckIndexAddr.dwIndexValue = |文件名压缩数据开始| ^ FILEINDEX_ADDR_CONST;

#ifdef PCKV202
	pckAllInfo.PckTail.dwVersion = pckAllInfo.PckIndexAddr.dwVersion = m_lpThisPckKey->Version;
#elif defined PCKV203
	pckAllInfo.PckTail.dwVersion = m_lpThisPckKey->Version;
#elif defined PCKV203ZX
	pckAllInfo.PckTail.dwVersion = pckAllInfo.PckIndexAddr.dwVersion = m_lpThisPckKey->Version;
	pckAllInfo.PckIndexAddr.dwUnknown1 = 0;
#endif

	//打开源文件 
	lpFileRead = new CMapViewFileRead();

	if(!OpenPckAndMappingRead(lpFileRead, m_PckAllInfo.szFilename)){
		delete lpFileRead;
		return FALSE;
	}

	//打开目标文件 
	//以下是创建一个文件，用来保存重建后的文件
	lpFileWrite = new CMapViewFileWrite();
	
	if(!OpenPckAndMappingWrite(lpFileWrite, szRebuildPckFile, CREATE_ALWAYS, dwTotalFileSizeAfterRebuild)){

		delete lpFileRead;
		delete lpFileWrite;
		
		return FALSE;
	}

	//申请空间,文件名压缩数据 数组
	if(NULL == (lpPckIndexTable = new PCKINDEXTABLE_COMPRESS[dwFileCount]))
	{
		delete lpFileRead;
		delete lpFileWrite;

		PrintLogE(TEXT_MALLOC_FAIL, __FILE__, __FUNCTION__, __LINE__);
		return FALSE;
	}

	//使用Enum进行遍历处理
	lpPckIndexTablePtr = lpPckIndexTable;

	if(!EnumNode(m_RootNode.child->next, lpFileRead, lpFileWrite, lpPckIndexTablePtr, dwAddress))
	{
		delete lpFileRead;
		delete lpFileWrite;

		delete [] lpPckIndexTable;
		return FALSE;

	}

	if(!lpPckParams->cVarParams.bThreadRunning)
	{
		PrintLogW(TEXT_USERCANCLE);
		dwFileCount = lpPckParams->cVarParams.dwUIProgress;
	}

	//关闭读文件
	delete lpFileRead;

	//写文件索引
	lpPckIndexTablePtr = lpPckIndexTable;
	dwAddressName = dwAddress;

	//窗口中以显示的文件进度，初始化，显示写索引进度
	lpPckParams->cVarParams.dwUIProgress = 0;

	for(DWORD	i = 0;i<dwFileCount;i++)
	{
		DWORD dwNumberOfBytesToMap = lpPckIndexTablePtr->dwIndexDataLength + 8;

		if(NULL == (lpBufferToWrite = lpFileWrite->View(dwAddress, dwNumberOfBytesToMap)))
		{
			PrintLogE(TEXT_VIEWMAP_FAIL, __FILE__, __FUNCTION__, __LINE__);

			delete lpFileWrite;
			return FALSE;
		}

		memcpy(lpBufferToWrite, lpPckIndexTablePtr, dwNumberOfBytesToMap);

		lpFileWrite->UnmapView();

		dwAddress += dwNumberOfBytesToMap;
		lpPckIndexTablePtr++;

		//窗口中以显示的文件进度
		lpPckParams->cVarParams.dwUIProgress++;

	}
	
	lpFileWrite->UnMaping();

	AfterProcess(lpFileWrite, pckAllInfo, dwFileCount, dwAddressName, dwAddress);

	delete lpFileWrite;
	delete [] lpPckIndexTable;

	PrintLogI(TEXT_LOG_WORKING_DONE);

	return TRUE;

}

VOID CPckClass::RenameIndex(LPPCK_PATH_NODE lpNode, char* lpszReplaceString)
{
	size_t nBytesReadayToWrite;
	char	*lpszPosToWrite;
	
	nBytesReadayToWrite = strlen(lpNode->lpPckIndexTable->cFileIndex.szFilename) - strlen(lpNode->szName);
	lpszPosToWrite = lpNode->lpPckIndexTable->cFileIndex.szFilename + nBytesReadayToWrite;
	//nBytesReadayToWrite = MAX_PATH_PCK - nBytesReadayToWrite;

	memset(lpszPosToWrite, 0, MAX_PATH_PCK - nBytesReadayToWrite);
	strcpy(lpszPosToWrite, lpszReplaceString);

}

VOID CPckClass::RenameIndex(LPPCKINDEXTABLE lpIndex, char* lpszReplaceString)
{
	//size_t	nBytesReadayToWrite;

	memset(lpIndex->cFileIndex.szFilename, 0, MAX_PATH_PCK);
	strcpy(lpIndex->cFileIndex.szFilename, lpszReplaceString);

}

BOOL CPckClass::GetCurrentNodeString(char *szCurrentNodePathString, LPPCK_PATH_NODE lpNode)
{

	if(NULL == lpNode->parentfirst)
	{
		*szCurrentNodePathString = 0;
		//return TRUE;
	}else{
		
		GetCurrentNodeString(szCurrentNodePathString, lpNode->parentfirst);
		strcat_s(szCurrentNodePathString, MAX_PATH_PCK, lpNode->parent->szName);
		strcat_s(szCurrentNodePathString, MAX_PATH_PCK, "\\");
	}
	
	return TRUE;
}

BOOL CPckClass::RenameNode(LPPCK_PATH_NODE lpNode, char* lpszReplaceString)
{
	size_t lenNodeRes, lenrs, lenrp;
	char	lpReplacePos[MAX_PATH_PCK];

	lenNodeRes = strlen(lpNode->szName);
	lenrs = strlen(lpszReplaceString);

	GetCurrentNodeString(lpReplacePos, lpNode->child);

	//lpReplacePos = "models\z1\"
	lenrp = strlen(lpReplacePos) - 1;

	return RenameNodeEnum(lpNode, lenNodeRes, lpszReplaceString, lenrs, lenrp);

}

BOOL CPckClass::GetPckBasicInfo(LPTSTR lpszFile, PCKHEAD *lpHead, LPBYTE &lpFileIndexData, DWORD &dwPckFileIndexDataSize)
{

	BYTE	*lpFileBuffer;

	dwPckFileIndexDataSize = m_PckAllInfo.PckHead.dwPckSize - m_PckAllInfo.dwAddressName;

	//lstrcpy(lpszFile, m_Filename);
#ifdef UNICODE
	memcpy(lpszFile, m_PckAllInfo.szFilename, MAX_PATH * 2);
#else
	memcpy(lpszFile, m_PckAllInfo.szFilename, MAX_PATH);
#endif
	
	if(NULL == (lpFileIndexData = (LPBYTE) malloc (dwPckFileIndexDataSize))){

		PrintLogE(TEXT_MALLOC_FAIL, __FILE__, __FUNCTION__, __LINE__);
		return FALSE;
	}

	CMapViewFileRead	*lpcReadfile = new CMapViewFileRead();

	if(!OpenPckAndMappingRead(lpcReadfile, m_PckAllInfo.szFilename))
	{
		free(lpFileIndexData);
		delete lpcReadfile;
		return FALSE;
	}

	if(NULL == (lpFileBuffer = lpcReadfile->View(m_PckAllInfo.dwAddressName, dwPckFileIndexDataSize)))
	{
		PrintLogE(TEXT_VIEWMAP_FAIL, __FILE__, __FUNCTION__, __LINE__);

		free(lpFileIndexData);
		delete lpcReadfile;
		
		return FALSE;
	}

	memcpy(lpFileIndexData, lpFileBuffer, dwPckFileIndexDataSize);

	delete lpcReadfile;

	memcpy(lpHead, &(m_PckAllInfo.PckHead), sizeof(PCKHEAD));

	return TRUE;

}

//当修改文件名等操作造成pck文件损坏无法读取时，恢复pck的索引数据
BOOL CPckClass::SetPckBasicInfo(PCKHEAD *lpHead, LPBYTE lpFileIndexData, DWORD &dwPckFileIndexDataSize)
{
	BYTE	*lpFileBuffer;

	//memcpy(lpHead, &m_PckHead, sizeof(PCKHEAD));

	CMapViewFileWrite	*lpcWritefile = new CMapViewFileWrite();

	if(!OpenPckAndMappingWrite(lpcWritefile, m_PckAllInfo.szFilename, OPEN_EXISTING, lpHead->dwPckSize)){

		delete lpcWritefile;
		return FALSE;
	}

	lpcWritefile->SetFilePointer(0, FILE_BEGIN);
	lpcWritefile->Write(lpHead, sizeof(PCKHEAD));

	if(NULL == (lpFileBuffer = lpcWritefile->View(lpHead->dwPckSize - dwPckFileIndexDataSize, dwPckFileIndexDataSize)))
	{

		PrintLogE(TEXT_VIEWMAP_FAIL, __FILE__, __FUNCTION__, __LINE__);

		delete lpcWritefile;
		return FALSE;
	}

	memcpy(lpFileBuffer, lpFileIndexData, dwPckFileIndexDataSize);

	delete lpcWritefile;
	return TRUE;

}


/********************************
*
*私有函数
*
********************/


BOOL CPckClass::AddFileToNode(LPPCK_PATH_NODE lpRootNode, LPPCKINDEXTABLE	lpPckIndexTable)
{
	LPPCK_PATH_NODE lpChildNode = lpRootNode;
	LPPCK_PATH_NODE	lpFirstNode = NULL;

	size_t			sizePckPathNode = sizeof(PCK_PATH_NODE);

	char			*lpszFilename = lpPckIndexTable->cFileIndex.szFilename;
	char			*lpszToFind;

	do
	{
		//此节点下还没有文件，首先添加".."
		if(NULL == (lpChildNode->child))
		{
			lpChildNode->child = (LPPCK_PATH_NODE)AllocNodes(sizePckPathNode);
			lpChildNode->child->parent = lpChildNode;
			lpChildNode->child->parentfirst = lpFirstNode;

			//添加..目录
			//strcpy(lpChildNode->child->szName, "..");
			//*(DWORD*)lpChildNode->child->szName = 0x2e2e;	//".."
			memcpy(lpChildNode->child->szName, "..", 2);
		}

		lpFirstNode = lpChildNode = lpChildNode->child;

		do
		{
			lpszToFind = lpszFilename;

			if(0 == strpathcmp(lpChildNode->szName, lpszToFind))
			{
				//存在这个文件（夹）了
				lpszFilename = lpszToFind;
				break;
			}

			if(NULL == lpChildNode->next)
			{

				//添加文件（夹）
				lpChildNode->next = (LPPCK_PATH_NODE)AllocNodes(sizePckPathNode);
				lpChildNode = lpChildNode->next;

				strpathcpy(lpChildNode->szName, lpszFilename);

				//统计各文件夹的子文件夹数
				if(0 != *lpszFilename)
				{
					LPPCK_PATH_NODE	lpAddDirCount = lpFirstNode;
					do{
						++(lpAddDirCount->dwDirsCount);
						lpAddDirCount = lpAddDirCount->parentfirst;

					}while(NULL != lpAddDirCount);

				}

				break;
			}
			lpChildNode = lpChildNode->next;

		}while(1);

		++(lpFirstNode->dwFilesCount);
		lpFirstNode->qdwDirCipherTextSize += lpPckIndexTable->cFileIndex.dwFileCipherTextSize;
		lpFirstNode->qdwDirClearTextSize += lpPckIndexTable->cFileIndex.dwFileClearTextSize;

		if('\\' == *lpszFilename || '/' == *lpszFilename)
			++lpszFilename;

	}while(*lpszFilename);

	lpChildNode->lpPckIndexTable = lpPckIndexTable;

	return TRUE;

}

LPPCK_PATH_NODE CPckClass::FindFileNode(LPPCK_PATH_NODE lpBaseNode, char* lpszFile)
{
	LPPCK_PATH_NODE lpChildNode = lpBaseNode;
	//LPPCK_PATH_NODE	lpFirstNode = NULL;

	//size_t			sizePckPathNode = sizeof(PCK_PATH_NODE);

	char			*lpszFilename = lpszFile;
	char			*lpszToFind;

	do
	{
		do
		{
			lpszToFind = lpszFilename;

			if(0 == strpathcmp(lpChildNode->szName, lpszToFind))
			{
				lpszFilename = lpszToFind;

				if( NULL == lpChildNode->child && 0 == *lpszFilename)return lpChildNode;

				if((NULL == lpChildNode->child && ('\\' == *lpszFilename || '/' == *lpszFilename)) || (NULL != lpChildNode->child && 0 == *lpszFilename))
				{
					return (LPPCK_PATH_NODE)-1;
				}

				break;
			}

			if(NULL == lpChildNode->next)
			{
				return NULL;
			}
			lpChildNode = lpChildNode->next;

		}while(1);

		lpChildNode = lpChildNode->child;

		if('\\' == *lpszFilename || '/' == *lpszFilename)
			++lpszFilename;

	}while(*lpszFilename);

	return NULL;

}

BOOL CPckClass::RenameNodeEnum(LPPCK_PATH_NODE lpNode, size_t lenNodeRes, char* lpszReplaceString, size_t lenrs, size_t lenrp)
{

	lpNode = lpNode->child->next;

	while(NULL != lpNode)
	{
		if(NULL == lpNode->child)
		{
			if(!RenameNode(lpNode, lenNodeRes, lpszReplaceString, lenrs, lenrp))
				return FALSE;
		}else{
			if(!RenameNodeEnum(lpNode, lenNodeRes, lpszReplaceString, lenrs, lenrp))
				return FALSE;
		}

		lpNode = lpNode->next;
	}
	return TRUE;
}

BOOL CPckClass::RenameNode(LPPCK_PATH_NODE lpNode, size_t lenNodeRes, char* lpszReplaceString, size_t lenrs, size_t lenrp)
{
	if(lenrs >= (MAX_PATH_PCK - strlen(lpNode->lpPckIndexTable->cFileIndex.szFilename + lenNodeRes - 2)))return FALSE;
	char	szTemp[MAX_PATH_PCK] = {0};
	char	*lpszReplacePos = lpNode->lpPckIndexTable->cFileIndex.szFilename + lenrp - lenNodeRes;

	//DebugA("lpszReplaceString = %s \r\nlenNodeRes = %d\r\nlenrs = %d\r\nlenrp = %d\r\n===============================\r\n",
	//		lpszReplaceString, lenNodeRes, lenrs, lenrp);
	memcpy(szTemp, lpNode->lpPckIndexTable->cFileIndex.szFilename + lenrp, MAX_PATH_PCK - lenrp);
	memcpy(lpszReplacePos, lpszReplaceString, lenrs);
	memcpy(lpszReplacePos + lenrs, szTemp, MAX_PATH_PCK - lenrp - lenrs + lenNodeRes);

	return TRUE;
}


BOOL CPckClass::EnumNode(LPPCK_PATH_NODE lpNodeToExtract, LPVOID lpvoidFileRead, LPVOID lpvoidFileWrite, LPPCKINDEXTABLE_COMPRESS &lpPckIndexTablePtr, PCKADDR &dwAddress)
{
	CMapViewFileRead *lpFileRead = (CMapViewFileRead*)lpvoidFileRead;
	CMapViewFileWrite *lpFileWrite = (CMapViewFileWrite*)lpvoidFileWrite;

	//LPVOID			lpMapAddressToWrite;
	LPBYTE			lpBufferToWrite;
	LPBYTE			lpBufferToRead;
	//PCKFILEINDEX	pckFileIndex;
	PCKADDR			dwSrcAddress;
	//DWORD			dwFileBytesRead;
	DWORD		IndexCompressedFilenameDataLengthCryptKey1 = m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey1;
	DWORD		IndexCompressedFilenameDataLengthCryptKey2 = m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey2;

	do
	{

		if(!lpPckParams->cVarParams.bThreadRunning)
		{
			break;
		}

		//lpThisNodePtr = lpNodeToExtract;
		if(NULL != lpNodeToExtract->child)
		{

			EnumNode(lpNodeToExtract->child->next, lpvoidFileRead, lpvoidFileWrite, lpPckIndexTablePtr, dwAddress);

		}else{
			
			DWORD dwNumberOfBytesToMap = lpNodeToExtract->lpPckIndexTable->cFileIndex.dwFileCipherTextSize;

			if(NULL == (lpBufferToWrite = lpFileWrite->View(dwAddress, dwNumberOfBytesToMap)))
			{
				PrintLogE(TEXT_VIEWMAP_FAIL, __FILE__, __FUNCTION__, __LINE__);
				return FALSE;
			}

			dwSrcAddress = lpNodeToExtract->lpPckIndexTable->cFileIndex.dwAddressOffset;	//保存原来的地址

			if(NULL == (lpBufferToRead = lpFileRead->View(dwSrcAddress, dwNumberOfBytesToMap)))
			{
				PrintLogE(TEXT_VIEWMAP_FAIL, __FILE__, __FUNCTION__, __LINE__);
				return FALSE;
			}

			memcpy(lpBufferToWrite, lpBufferToRead, dwNumberOfBytesToMap);


			lpFileRead->UnmapView();

			//写入此文件的PckFileIndex文件压缩信息并压缩
			
			lpNodeToExtract->lpPckIndexTable->cFileIndex.dwAddressOffset = dwAddress;	//此文件的压缩数据起始地址

			dwAddress += lpNodeToExtract->lpPckIndexTable->cFileIndex.dwFileCipherTextSize;	//下一个文件的压缩数据起始地址

			lpPckIndexTablePtr->dwIndexDataLength = INDEXTABLE_CLEARTEXT_LENGTH;
			compress2(lpPckIndexTablePtr->buffer, &lpPckIndexTablePtr->dwIndexDataLength, 
							(BYTE*)&lpNodeToExtract->lpPckIndexTable->cFileIndex, INDEXTABLE_CLEARTEXT_LENGTH, 9);
			//将获取的
			lpPckIndexTablePtr->dwIndexValueHead = lpPckIndexTablePtr->dwIndexDataLength ^ IndexCompressedFilenameDataLengthCryptKey1;
			lpPckIndexTablePtr->dwIndexValueTail = lpPckIndexTablePtr->dwIndexDataLength ^ IndexCompressedFilenameDataLengthCryptKey2;

			lpNodeToExtract->lpPckIndexTable->cFileIndex.dwAddressOffset = dwSrcAddress;	//还原原来的地址

			lpFileWrite->UnmapView();

			++lpPckIndexTablePtr;
			++(lpPckParams->cVarParams.dwUIProgress);

		}

		lpNodeToExtract = lpNodeToExtract->next;

	}while(NULL != lpNodeToExtract);

	return TRUE;
}




//////////////////////////以下是程序过程中需要调用的过程///////////////////////////////


VOID CPckClass::EnumFile(LPSTR szFilename, BOOL IsPatition, DWORD &dwFileCount, LPFILES_TO_COMPRESS &pFileinfo, QWORD &qwTotalFileSize, size_t nLen)
{

	char		szPath[MAX_PATH], szFile[MAX_PATH];

	size_t nLenBytePath = mystrcpy(szPath, szFilename) - szPath + 1;
	strcat(szFilename, "\\*.*");

	HANDLE					hFile;
	WIN32_FIND_DATAA		WFD;

	if ((hFile = FindFirstFileA(szFilename, &WFD)) != INVALID_HANDLE_VALUE) 
	{
		if(!IsPatition)
		{
			FindNextFileA(hFile, &WFD);
			if(!FindNextFileA(hFile, &WFD))
			{
				return;
			}
		}

		do
		{
			if ((WFD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
			
				if((MAX_PATH_PCK - 13) >= mystrcpy(mystrcpy(mystrcpy(szFile, szPath), "\\"), WFD.cFileName) - szFile){
					EnumFile(szFile, FALSE, dwFileCount, pFileinfo, qwTotalFileSize, nLen);
				}

			}else{
				
				if(NULL == pFileinfo)return;
				if(0 != WFD.nFileSizeHigh)continue;

				++dwFileCount;
				
				if( MAX_PATH_PCK < nLenBytePath + strlen(WFD.cFileName)){
					mystrcpy(mystrcpy(mystrcpy(pFileinfo->szFilename, szPath), "\\"), WFD.cAlternateFileName);
				}else{
					mystrcpy(mystrcpy(mystrcpy(pFileinfo->szFilename, szPath), "\\"), WFD.cFileName);
				}

				pFileinfo->lpszFileTitle = pFileinfo->szFilename + nLen;
				pFileinfo->nBytesToCopy = MAX_PATH_PCK - nLen;

				qwTotalFileSize += (pFileinfo->dwFileSize = WFD.nFileSizeLow);
				//m_CompressTotalFileSize += (pFileinfo->dwFileSize = WFD.nFileSizeLow);

				pFileinfo->next = AllocateFileinfo();
				pFileinfo = pFileinfo->next;

			}

		}while(FindNextFileA(hFile,&WFD));

		FindClose(hFile);
	}

}

