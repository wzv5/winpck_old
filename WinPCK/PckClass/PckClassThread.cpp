
//////////////////////////////////////////////////////////////////////
// PckClassThread.cpp: 用于解析完美世界公司的pck文件中的数据，并显示在List中
// 主要功能，包括压缩、更新、重命名等
//
// 此程序由 李秋枫/stsm/liqf 编写
//
// 此代码预计将会开源，任何基于此代码的修改发布请保留原作者信息
// 
// 2012.4.10
//////////////////////////////////////////////////////////////////////

#include "zlib.h"

#include "PckClass.h"
#include <process.h>


#pragma warning ( disable : 4996 )
#pragma warning ( disable : 4244 )
#pragma warning ( disable : 4267 )
#pragma warning ( disable : 4311 )

_inline char * __fastcall mystrcpy(char * dest,const char *src)
{
        while ((*dest = *src))
                dest++, src++;
        return dest;
}



/////////以下变量用于多线程pck压缩的测试
CRITICAL_SECTION	g_cs;
CRITICAL_SECTION	g_mt_threadID;
CRITICAL_SECTION	g_mt_nMallocBlocked;
CRITICAL_SECTION	g_mt_lpMaxMemory;
CRITICAL_SECTION	g_dwCompressedflag;
CRITICAL_SECTION	g_mt_pckCompressedDataPtrArrayPtr;


int					mt_threadID;		//线程ID

LPPCKINDEXTABLE_COMPRESS	mt_lpPckIndexTable, mt_lpPckIndexTablePtr;				//压缩后的文件名索引，及全局指针
BYTE**				mt_pckCompressedDataPtrArray, **mt_pckCompressedDataPtrArrayPtr;//内存申请指针的数组及其全局指针
//HANDLE			mt_evtToWrite;														//线程事件
HANDLE				mt_evtAllWriteFinish;											//线程事件
HANDLE				mt_evtAllCompressFinish;										//线程事件
HANDLE				mt_evtMaxMemory;												//线程事件
//HANDLE				mt_hFileToWrite, mt_hMapFileToWrite;							//全局写文件的句柄
CMapViewFileWrite	*mt_lpFileWrite;												//全局写文件的句柄
QWORD				mt_CompressTotalFileSize;										//预计的压缩文件大小
LPDWORD				mt_lpdwCount;													//从界面线程传过来的文件计数指针
DWORD				mt_dwFileCount;													//从界面线程传过来的总文件数量指针
PCKADDR				mt_dwAddress;													//全局压缩过程的写文件的位置

//size_t			mt_nLen;															//传来的目录的长度，用于截取文件名放到pck中
BOOL				*mt_lpbThreadRunning;											//线程是否处于运行状态的值，当为false时线程退出
DWORD				*mt_lpMaxMemory;												//压缩过程允许使用的最大内存，这是一个计数回显用的，引用从界面线程传来的数值并回显
DWORD				mt_MaxMemory;													//压缩过程允许使用的最大内存
int					mt_nMallocBlocked;												//因缓存用完被暂停的线程数
DWORD				mt_level;														//压缩等级


//添加时使用变量
PCKADDR				mt_dwAddressName;												//读出的pck文件的压缩文件名索引起始位置
char				mt_szCurrentNodeString[MAX_PATH_PCK];							//（界面线程中当前显示的）节点对应的pck中的文件路径
int					mt_nCurrentNodeStringLen;										//其长度



VOID CPckClass::CompressThread(VOID* pParam)
{
	CPckClass *pThis = (CPckClass*)pParam;

	char	szFileMappingNameSpaceFormat[16];
	char	szFileMappingNameSpace[32];
	int		level = mt_level;

	DWORD	IndexCompressedFilenameDataLengthCryptKey1 = pThis->m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey1;
	DWORD	IndexCompressedFilenameDataLengthCryptKey2 = pThis->m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey2;

	
	memcpy(szFileMappingNameSpaceFormat, pThis->m_szMapNameRead, 16);
	strcat_s(szFileMappingNameSpaceFormat, 16, "%d");

	EnterCriticalSection(&g_mt_threadID);
	sprintf_s(szFileMappingNameSpace, 32, szFileMappingNameSpaceFormat, mt_threadID++);
	LeaveCriticalSection(&g_mt_threadID); 

	BYTE	*bufCompressData = (BYTE*)1;
	BYTE	**ptrCurrentIndex;		//暂存mt_pckCompressedDataPtrArrayPtr
	
	//HANDLE	hFileToRead;
	//HANDLE	hMapFileToRead;
	//LPVOID	lpMapAddressToRead;
	LPBYTE		lpBufferToRead;

	
	PCKFILEINDEX		pckFileIndex;

	//memset(pckFileIndex.dwAttachedValue, 0, sizeof(pckFileIndex.dwAttachedValue));
	pckFileIndex.dwUnknown1 = pckFileIndex.dwUnknown2 = 0;
#ifdef PCKV203ZX
	pckFileIndex.dwUnknown3 = pckFileIndex.dwUnknown4 = pckFileIndex.dwUnknown5 = 0;
#endif

	//开始
	LPPCKINDEXTABLE_COMPRESS	lpPckIndexTablePtr = mt_lpPckIndexTable;
	LPFILES_TO_COMPRESS			lpfirstFile = pThis->m_firstFile;

	memset(pckFileIndex.szFilename, 0, MAX_PATH_PCK);

	//patch 140424
	CMapViewFileRead	*lpFileRead = new CMapViewFileRead();

	while(NULL != lpfirstFile->next)
	{
		EnterCriticalSection(&g_dwCompressedflag);
		if(0 == lpfirstFile->dwCompressedflag)
		{
			lpfirstFile->dwCompressedflag = 1;

			LeaveCriticalSection(&g_dwCompressedflag); 

			if(PCK_BEGINCOMPRESS_SIZE < lpfirstFile->dwFileSize)
			{
				pckFileIndex.dwFileClearTextSize = lpfirstFile->dwFileSize;
				pckFileIndex.dwFileCipherTextSize = compressBound(lpfirstFile->dwFileSize);//lpfirstFile->dwFileSize * 1.001 + 12;
				lpfirstFile->dwFileSize = pckFileIndex.dwFileCipherTextSize;
			}else{
				pckFileIndex.dwFileCipherTextSize = pckFileIndex.dwFileClearTextSize = lpfirstFile->dwFileSize;
			}

			//判断使用的内存是否超过最大值
			EnterCriticalSection(&g_mt_lpMaxMemory);
			if( (*mt_lpMaxMemory) >= mt_MaxMemory)
			{
				LeaveCriticalSection(&g_mt_lpMaxMemory); 
				{
					EnterCriticalSection(&g_mt_nMallocBlocked);
					mt_nMallocBlocked++;
					LeaveCriticalSection(&g_mt_nMallocBlocked); 
				}
				WaitForSingleObject(mt_evtMaxMemory, INFINITE);
				{
					EnterCriticalSection(&g_mt_nMallocBlocked);
					mt_nMallocBlocked--;
					LeaveCriticalSection(&g_mt_nMallocBlocked); 
				}
			}else
				LeaveCriticalSection(&g_mt_lpMaxMemory);
			
			{
				EnterCriticalSection(&g_mt_lpMaxMemory);
				(*mt_lpMaxMemory) += lpfirstFile->dwFileSize;
				LeaveCriticalSection(&g_mt_lpMaxMemory);
			}

			//pckFileIndex.dwFileCipherTextSize = pckFileIndex.dwFileClearTextSize = lpfirstFile->dwFileSize;

			//lpfirstFile->dwFileSize = pckFileIndex.dwFileCipherTextSize = pckFileIndex.dwFileCipherTextSize * 1.001 + 12;
			
			memcpy(pckFileIndex.szFilename, lpfirstFile->lpszFileTitle, lpfirstFile->nBytesToCopy);
			//pckFileIndex.dwUnknown1 = pckFileIndex.dwUnknown2 = 0;

			if(!*(mt_lpbThreadRunning))
			{
				//int i = 1;
				break;
			}

			//如果文件大小为0，则跳过打开文件步骤
			if(0 != pckFileIndex.dwFileClearTextSize)
			{
				//lpFileRead = new CMapViewFileRead();

				////////////////////////////打开源文件/////////////////////////////////
				//打开要进行压缩的文件
				if(!lpFileRead->Open(lpfirstFile->szFilename))
				{
					*(mt_lpbThreadRunning) = FALSE;
					//delete lpFileRead;
					lpFileRead->clear();
					break;
				}

				//创建一个文件映射
				if(!lpFileRead->Mapping(szFileMappingNameSpace))
				{
					*(mt_lpbThreadRunning) = FALSE;
					//delete lpFileRead;
					lpFileRead->clear();
					break;
				}


				//创建一个文件映射的视图用来作为source
				if(NULL == (lpBufferToRead = lpFileRead->View(0, 0)))
				{
					*(mt_lpbThreadRunning) = FALSE;
					//delete lpFileRead;
					lpFileRead->clear();
					break;
				}

				////////////////////////////打开源文件/////////////////////////////////

				//lpfirstFile->dwFileSize = pckFileIndex.dwFileCipherTextSize = pckFileIndex.dwFileCipherTextSize * 1.001 + 12;
				//

				bufCompressData = (BYTE*) malloc (pckFileIndex.dwFileCipherTextSize);
			
				if(PCK_BEGINCOMPRESS_SIZE < pckFileIndex.dwFileClearTextSize)
				{
					compress2(bufCompressData, &pckFileIndex.dwFileCipherTextSize, 
									lpBufferToRead, pckFileIndex.dwFileClearTextSize, level);
				}else{
					memcpy((BYTE*)bufCompressData, lpBufferToRead, pckFileIndex.dwFileClearTextSize);
				}

				//delete lpFileRead;
				lpFileRead->clear();

			}//文件不为0时的处理

			{

				EnterCriticalSection(&g_cs);

				ptrCurrentIndex = mt_pckCompressedDataPtrArrayPtr++;

				lpPckIndexTablePtr = mt_lpPckIndexTablePtr;
				mt_lpPckIndexTablePtr++;

				//写入此文件的PckFileIndex文件压缩信息并压缩
				pckFileIndex.dwAddressOffset = mt_dwAddress;		//此文件的压缩数据起始地址
				mt_dwAddress += pckFileIndex.dwFileCipherTextSize;	//下一个文件的压缩数据起始地址
				
				//窗口中以显示的文件进度
				(*mt_lpdwCount)++;

				LeaveCriticalSection(&g_cs);
			}

			//test
			//lpPckIndexTablePtr->dwAddress = pckFileIndex.dwAddressOffset;//////test

			lpPckIndexTablePtr->dwCompressedFilesize = pckFileIndex.dwFileCipherTextSize;
			lpPckIndexTablePtr->dwMallocSize = lpfirstFile->dwFileSize;

			lpPckIndexTablePtr->dwIndexDataLength = INDEXTABLE_CLEARTEXT_LENGTH;
			compress2(lpPckIndexTablePtr->buffer, &lpPckIndexTablePtr->dwIndexDataLength, 
							(BYTE*)&pckFileIndex, INDEXTABLE_CLEARTEXT_LENGTH, level);
			//将获取的
			lpPckIndexTablePtr->dwIndexValueHead = lpPckIndexTablePtr->dwIndexDataLength ^ IndexCompressedFilenameDataLengthCryptKey1;
			lpPckIndexTablePtr->dwIndexValueTail = lpPckIndexTablePtr->dwIndexDataLength ^ IndexCompressedFilenameDataLengthCryptKey2;


			EnterCriticalSection(&g_mt_pckCompressedDataPtrArrayPtr);
			//int		ilen = sizeof(BYTE*);
			//BYTE	**tes1t = mt_pckCompressedDataPtrArrayPtr - 1;
			*(ptrCurrentIndex) = bufCompressData;
			LeaveCriticalSection(&g_mt_pckCompressedDataPtrArrayPtr);
			//SetEvent(mt_evtToWrite);//改为Sleep(10);
			
		}else{

			LeaveCriticalSection(&g_dwCompressedflag);
		}
		//加密的文件名数据数组
		//lpPckIndexTablePtr++;
		//lpPckFileIndex++;
		//lppckCompressedDataPtrArray++;

		//下一个文件列表
		lpfirstFile = lpfirstFile->next;
	}

	delete lpFileRead;

	EnterCriticalSection(&g_mt_threadID);

	mt_threadID--;
	if(0 == mt_threadID)
		SetEvent(mt_evtAllCompressFinish);

	LeaveCriticalSection(&g_mt_threadID); 
	

	_endthread();
}


VOID CPckClass::WriteThread(VOID* pParam)
{
	CPckClass *pThis = (CPckClass*)pParam;

	BYTE**		bufferPtrToWrite = mt_pckCompressedDataPtrArray;
	PCKADDR		dwTotalFileSizeAfterCompress = mt_CompressTotalFileSize;

	//mt_lpFileWrite
	//HANDLE	hFileToWrite = mt_hFileToWrite;
	//HANDLE	hMapFileToWrite = mt_hMapFileToWrite;
	//LPVOID	lpMapAddressToWrite;
	LPBYTE		lpBufferToWrite;

	LPFILES_TO_COMPRESS			lpfirstFile = pThis->m_firstFile;
	LPPCKINDEXTABLE_COMPRESS	lpPckIndexTablePtr = mt_lpPckIndexTable;

	PCKADDR						dwAddress = PCK_DATA_START_AT;
	DWORD						nWrite = 0;
	//DWORD						dwFileCount;

	//用于测试的一个文本框
	//HWND	hTest = CreateWindow(TEXT("EDIT"), TEXT("DebugString"), WS_OVERLAPPEDWINDOW | WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_WANTRETURN | WS_VISIBLE , CW_USEDEFAULT, CW_USEDEFAULT, 600, 400, NULL, (HMENU)1001, TApp::GetInstance(), NULL);

	//VS2010的一个bug,定义时初始化，无论以下过程如何改变其值，返回的都 是初始值 

	while(1)
	{
		
		EnterCriticalSection(&g_mt_pckCompressedDataPtrArrayPtr);
		if(NULL != (*bufferPtrToWrite))
		{
			LeaveCriticalSection(&g_mt_pckCompressedDataPtrArrayPtr);
			//此处代码应该在WriteThread线程中实现
			//判断一下dwAddress的值会不会超过dwTotalFileSizeAfterCompress
			//如果超过，说明文件空间申请的过小，重新申请一下ReCreateFileMapping
			//新文件大小在原来的基础上增加(lpfirstFile->dwFileSize + 1mb) >= 64mb ? (lpfirstFile->dwFileSize + 1mb) :64mb
			//1mb=0x100000
			//64mb=0x4000000
			if((dwAddress + lpPckIndexTablePtr->dwCompressedFilesize + PCK_SPACE_DETECT_SIZE) > dwTotalFileSizeAfterCompress)
			{
		
				dwTotalFileSizeAfterCompress += 
					((lpPckIndexTablePtr->dwCompressedFilesize + PCK_SPACE_DETECT_SIZE) > PCK_STEP_ADD_SIZE ? (lpPckIndexTablePtr->dwCompressedFilesize + PCK_SPACE_DETECT_SIZE) : PCK_STEP_ADD_SIZE);

				mt_lpFileWrite->UnMaping();

				if(!mt_lpFileWrite->Mapping(pThis->m_szMapNameWrite, dwTotalFileSizeAfterCompress))
				{
					{
						EnterCriticalSection(&g_cs);
						*(mt_lpbThreadRunning) = FALSE;
						LeaveCriticalSection(&g_cs);
					}

					WaitForSingleObject(mt_evtAllCompressFinish, INFINITE);
					mt_dwFileCount = nWrite;
					mt_dwAddress = dwAddress;

					//释放
					while(0 != *bufferPtrToWrite)
					{
						free(*bufferPtrToWrite);
						bufferPtrToWrite++;
					}

					break;

				}

			}

			//处理lpPckFileIndex->dwAddressOffset
			//文件映射地址必须是64k(0x10000)的整数

			if(0 != lpPckIndexTablePtr->dwCompressedFilesize)
			{

				//lpBufferToWrite = mt_lpFileWrite->View(dwAddress, lpPckIndexTablePtr->dwCompressedFilesize)

				//DWORD	dwMapViewBlockHigh, dwMapViewBlockLow;

				//dwMapViewBlockHigh = dwAddress & 0xffff0000;
				//dwMapViewBlockLow = dwAddress & 0xffff;

				if(NULL == (lpBufferToWrite = mt_lpFileWrite->View(dwAddress, lpPckIndexTablePtr->dwCompressedFilesize)))
				{
					mt_lpFileWrite->UnMaping();
					{
						EnterCriticalSection(&g_cs);
						*(mt_lpbThreadRunning) = FALSE;
						LeaveCriticalSection(&g_cs);
					}

					WaitForSingleObject(mt_evtAllCompressFinish, INFINITE);
					mt_dwFileCount = nWrite;
					mt_dwAddress = dwAddress;

					//释放
					while(0 != *bufferPtrToWrite)
					{
						free(*bufferPtrToWrite);
						bufferPtrToWrite++;
					}
					break;
				}
				//检查地址是否正确
				//if(lpPckIndexTablePtr->dwAddress != dwAddress)OutputDebugStringA("lpPckIndexTablePtr->dwAddress != dwAddress!!!\r\n");

				memcpy(lpBufferToWrite, *bufferPtrToWrite, lpPckIndexTablePtr->dwCompressedFilesize);
				dwAddress += lpPckIndexTablePtr->dwCompressedFilesize;

				free(*bufferPtrToWrite);
				mt_lpFileWrite->UnmapView();
			}

			nWrite++;
			//char	str[64],str1[32];
			//sprintf(str, "%d", nWrite);
			//sprintf(str1, "%d", dwAddress);
			//ConfirmErrors(str1,str,MB_OK);
			
			{
				EnterCriticalSection(&g_mt_lpMaxMemory);
				(*mt_lpMaxMemory) -= lpPckIndexTablePtr->dwMallocSize;

				if((*mt_lpMaxMemory) < mt_MaxMemory)
				{
					LeaveCriticalSection(&g_mt_lpMaxMemory);
					
					{
						EnterCriticalSection(&g_mt_nMallocBlocked);
						int nMallocBlocked = mt_nMallocBlocked;
						LeaveCriticalSection(&g_mt_nMallocBlocked);
						for(int i=0;i<nMallocBlocked;i++)
							SetEvent(mt_evtMaxMemory);

					}


				}else{
					LeaveCriticalSection(&g_mt_lpMaxMemory);
				}
			}		

			if(mt_dwFileCount == nWrite)
			{
				WaitForSingleObject(mt_evtAllCompressFinish, INFINITE);
				mt_dwAddress = dwAddress;
				
				break;
			}
			

			bufferPtrToWrite++;
			lpPckIndexTablePtr++;

		}else{
			
			LeaveCriticalSection(&g_mt_pckCompressedDataPtrArrayPtr);

			EnterCriticalSection(&g_cs);
			if(!(*mt_lpbThreadRunning))
			{
				LeaveCriticalSection(&g_cs);
				//取消
				WaitForSingleObject(mt_evtAllCompressFinish, INFINITE);
				//lstrcpy(m_lastErrorString, TEXT_USERCANCLE);
				//char	str[64],str1[32];
				//sprintf(str, "%d", nWrite);
				//sprintf(str1, "%d", dwAddress);
				//ConfirmErrors(str1,str,MB_OK);
				mt_dwFileCount = nWrite;
				mt_dwAddress = dwAddress;

				break;
			}else{
				LeaveCriticalSection(&g_cs);
			}

			//ResetEvent(mt_evtToWrite);
			//WaitForSingleObject(mt_evtToWrite, INFINITE); //等待所有的子线程结束
			::Sleep(10);

		}

	}

	//mt_hMapFileToWrite = hMapFileToWrite;
	SetEvent(mt_evtAllWriteFinish);
	
	_endthread();
}

VOID CPckClass::CompressThreadAdd(VOID* pParam)
{
	CPckClass *pThis = (CPckClass*)pParam;

	char	szFileMappingNameSpaceFormat[16];
	char	szFileMappingNameSpace[32];
	int		level = mt_level;

	DWORD	IndexCompressedFilenameDataLengthCryptKey1 = pThis->m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey1;
	DWORD	IndexCompressedFilenameDataLengthCryptKey2 = pThis->m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey2;


	
	memcpy(szFileMappingNameSpaceFormat, pThis->m_szMapNameRead, 16);
	strcat_s(szFileMappingNameSpaceFormat, 16, "%d");

	EnterCriticalSection(&g_mt_threadID);
	sprintf_s(szFileMappingNameSpace, 32, szFileMappingNameSpaceFormat, mt_threadID++);
	LeaveCriticalSection(&g_mt_threadID); 

	BYTE	*bufCompressData = (BYTE*)1;
	BYTE	**ptrCurrentIndex;		//暂存mt_pckCompressedDataPtrArrayPtr
	
	//HANDLE	hFileToRead;
	//HANDLE	hMapFileToRead;
	//LPVOID	lpMapAddressToRead;
	LPBYTE				lpBufferToRead;
	
	PCKFILEINDEX		pckFileIndex;

	pckFileIndex.dwUnknown1 = pckFileIndex.dwUnknown2 = 0;
#ifdef PCKV203ZX
	pckFileIndex.dwUnknown3 = pckFileIndex.dwUnknown4 = pckFileIndex.dwUnknown5 = 0;
#endif
	//memset(pckFileIndex.dwAttachedValue, 0, sizeof(pckFileIndex.dwAttachedValue));
//#ifdef PCKV202
//	memset(&pckFileIndex.dwUnknown1, 0, sizeof(DWORD) * 5);
//#elif defined PCKV203
//	memset(&pckFileIndex.dwUnknown1, 0, sizeof(DWORD) * 6);
//#endif

	//开始
	LPPCKINDEXTABLE_COMPRESS	lpPckIndexTablePtr = mt_lpPckIndexTable;
	LPFILES_TO_COMPRESS			lpfirstFile = pThis->m_firstFile;
	//LPPCKFILEINDEX				lpPckFileIndex = mt_lpPckFileIndex;

	//char	*lpFilenamePtr = pckFileIndex.szFilename + MAX_PATH_PCK - mt_nLen;
	//memset(lpFilenamePtr, 0, mt_nLen);
	memset(pckFileIndex.szFilename, 0, MAX_PATH_PCK);

	//patch 140424
	CMapViewFileRead	*lpFileRead = new CMapViewFileRead();


	while(NULL != lpfirstFile->next)
	{
		EnterCriticalSection(&g_dwCompressedflag);
		if(0 == lpfirstFile->dwCompressedflag)
		{
			lpfirstFile->dwCompressedflag = 1;

			LeaveCriticalSection(&g_dwCompressedflag); 

			if(PCK_BEGINCOMPRESS_SIZE < lpfirstFile->dwFileSize)
			{
				pckFileIndex.dwFileClearTextSize = lpfirstFile->dwFileSize;
				pckFileIndex.dwFileCipherTextSize = compressBound(lpfirstFile->dwFileSize);//lpfirstFile->dwFileSize * 1.001 + 12;
				lpfirstFile->dwFileSize = pckFileIndex.dwFileCipherTextSize;
			}else{
				pckFileIndex.dwFileCipherTextSize = pckFileIndex.dwFileClearTextSize = lpfirstFile->dwFileSize;
			}

			//判断使用的内存是否超过最大值
			EnterCriticalSection(&g_mt_lpMaxMemory);
			if( (*mt_lpMaxMemory) >= mt_MaxMemory)
			{
				LeaveCriticalSection(&g_mt_lpMaxMemory); 
				{
					EnterCriticalSection(&g_mt_nMallocBlocked);
					mt_nMallocBlocked++;
					LeaveCriticalSection(&g_mt_nMallocBlocked); 
				}
				WaitForSingleObject(mt_evtMaxMemory, INFINITE);
				{
					EnterCriticalSection(&g_mt_nMallocBlocked);
					mt_nMallocBlocked--;
					LeaveCriticalSection(&g_mt_nMallocBlocked); 
				}
			}else
				LeaveCriticalSection(&g_mt_lpMaxMemory);
			
			{
				EnterCriticalSection(&g_mt_lpMaxMemory);
				(*mt_lpMaxMemory) += lpfirstFile->dwFileSize;
				LeaveCriticalSection(&g_mt_lpMaxMemory);
			}

			memcpy(mystrcpy(pckFileIndex.szFilename, mt_szCurrentNodeString), lpfirstFile->lpszFileTitle, lpfirstFile->nBytesToCopy - mt_nCurrentNodeStringLen);

			if(!*(mt_lpbThreadRunning))
			{
				//int i = 1;
				break;
			}

			//如果文件大小为0，则跳过打开文件步骤
			if(0 != pckFileIndex.dwFileClearTextSize)
			{
				//CMapViewFileRead	*lpFileRead = new CMapViewFileRead();
				////////////////////////////打开源文件/////////////////////////////////
				//打开要进行压缩的文件
				if(!lpFileRead->Open(lpfirstFile->szFilename))
				{
					*(mt_lpbThreadRunning) = FALSE;
					//delete lpFileRead;
					lpFileRead->clear();

					break;
				}

				//创建一个文件映射
				if(!lpFileRead->Mapping(szFileMappingNameSpace))
				{
					*(mt_lpbThreadRunning) = FALSE;
					//delete lpFileRead;
					lpFileRead->clear();
					break;
				}


				//创建一个文件映射的视图用来作为source
				if(NULL == (lpBufferToRead = lpFileRead->View(0, 0)))
				{
					*(mt_lpbThreadRunning) = FALSE;
					//delete lpFileRead;
					lpFileRead->clear();
					break;
				}

				////////////////////////////打开源文件/////////////////////////////////

				bufCompressData = (BYTE*) malloc (pckFileIndex.dwFileCipherTextSize);
			
				if(PCK_BEGINCOMPRESS_SIZE < pckFileIndex.dwFileClearTextSize)
				{
					compress2(bufCompressData, &pckFileIndex.dwFileCipherTextSize, 
									lpBufferToRead, pckFileIndex.dwFileClearTextSize, level);
				}else{
					memcpy(bufCompressData, lpBufferToRead, pckFileIndex.dwFileClearTextSize);
					//pckFileIndex.dwFileCipherTextSize = pckFileIndex.dwFileClearTextSize;
				}

				//delete lpFileRead;
				lpFileRead->clear();
			}//文件不为0时的处理


			EnterCriticalSection(&g_cs);

			ptrCurrentIndex = mt_pckCompressedDataPtrArrayPtr++;

			lpPckIndexTablePtr = mt_lpPckIndexTablePtr;
			mt_lpPckIndexTablePtr++;

			//窗口中以显示的文件进度
			(*mt_lpdwCount)++;

			if(NULL != lpfirstFile->samePtr)
			{
				if(pckFileIndex.dwFileCipherTextSize > lpfirstFile->samePtr->cFileIndex.dwFileCipherTextSize)
				{

					lpfirstFile->samePtr->cFileIndex.dwAddressOffset = mt_dwAddress;
					mt_dwAddress += pckFileIndex.dwFileCipherTextSize;	//下一个文件的压缩数据起始地址

				}else{
					lpPckIndexTablePtr->dwAddressOfDuplicateOldDataArea = lpfirstFile->samePtr->cFileIndex.dwAddressOffset;
				}

				lpfirstFile->samePtr->cFileIndex.dwFileCipherTextSize = pckFileIndex.dwFileCipherTextSize;
				lpfirstFile->samePtr->cFileIndex.dwFileClearTextSize = pckFileIndex.dwFileClearTextSize;

				lpPckIndexTablePtr->bInvalid = TRUE;
				

			}else{

				//写入此文件的PckFileIndex文件压缩信息并压缩
				pckFileIndex.dwAddressOffset = mt_dwAddress;		//此文件的压缩数据起始地址
				mt_dwAddress += pckFileIndex.dwFileCipherTextSize;	//下一个文件的压缩数据起始地址
			}

			LeaveCriticalSection(&g_cs);


			//test
			//lpPckIndexTablePtr->dwAddress = pckFileIndex.dwAddressOffset;//////test
			lpPckIndexTablePtr->dwCompressedFilesize = pckFileIndex.dwFileCipherTextSize;
			lpPckIndexTablePtr->dwMallocSize = lpfirstFile->dwFileSize;

			if(NULL == lpfirstFile->samePtr)
			{

				lpPckIndexTablePtr->dwIndexDataLength = INDEXTABLE_CLEARTEXT_LENGTH;
				compress2(lpPckIndexTablePtr->buffer, &lpPckIndexTablePtr->dwIndexDataLength, 
								(BYTE*)&pckFileIndex, INDEXTABLE_CLEARTEXT_LENGTH, level);
				//将获取的
				//lpPckIndexTablePtr->dwIndexValueHead = lpPckIndexTablePtr->dwIndexDataLength ^ INDEXTABLE_OR_VALUE_HEAD;
				//lpPckIndexTablePtr->dwIndexValueTail = lpPckIndexTablePtr->dwIndexDataLength ^ INDEXTABLE_OR_VALUE_TAIL;
				lpPckIndexTablePtr->dwIndexValueHead = lpPckIndexTablePtr->dwIndexDataLength ^ IndexCompressedFilenameDataLengthCryptKey1;
				lpPckIndexTablePtr->dwIndexValueTail = lpPckIndexTablePtr->dwIndexDataLength ^ IndexCompressedFilenameDataLengthCryptKey2;

			}

			EnterCriticalSection(&g_mt_pckCompressedDataPtrArrayPtr);
			//int		ilen = sizeof(BYTE*);
			//BYTE	**tes1t = mt_pckCompressedDataPtrArrayPtr - 1;
			*(ptrCurrentIndex) = bufCompressData;
			LeaveCriticalSection(&g_mt_pckCompressedDataPtrArrayPtr);
			//SetEvent(mt_evtToWrite);//改为Sleep(10);
			
		}else{

			LeaveCriticalSection(&g_dwCompressedflag);
		}
		//加密的文件名数据数组
		//lpPckIndexTablePtr++;
		//lpPckFileIndex++;
		//lppckCompressedDataPtrArray++;

		//下一个文件列表
		lpfirstFile = lpfirstFile->next;
	}

	delete lpFileRead;

	EnterCriticalSection(&g_mt_threadID);

	mt_threadID--;
	if(0 == mt_threadID)
		SetEvent(mt_evtAllCompressFinish);

	LeaveCriticalSection(&g_mt_threadID); 
	

	_endthread();
}


VOID CPckClass::WriteThreadAdd(VOID* pParam)
{
	CPckClass *pThis = (CPckClass*)pParam;

	BYTE**		bufferPtrToWrite = mt_pckCompressedDataPtrArray;
	PCKADDR		dwTotalFileSizeAfterCompress = mt_CompressTotalFileSize;

	//HANDLE	hFileToWrite = mt_hFileToWrite;
	//HANDLE	hMapFileToWrite = mt_hMapFileToWrite;
	//LPVOID	lpMapAddressToWrite;
	LPBYTE		lpBufferToWrite;


	LPFILES_TO_COMPRESS			lpfirstFile = pThis->m_firstFile;
	LPPCKINDEXTABLE_COMPRESS	lpPckIndexTablePtr = mt_lpPckIndexTable;

	PCKADDR						dwAddress = mt_dwAddressName;
	DWORD						nWrite = 0;


	while(1)
	{
		
		EnterCriticalSection(&g_mt_pckCompressedDataPtrArrayPtr);
		if(NULL != (*bufferPtrToWrite))
		{
			LeaveCriticalSection(&g_mt_pckCompressedDataPtrArrayPtr);
			//此处代码应该在WriteThread线程中实现
			//判断一下dwAddress的值会不会超过dwTotalFileSizeAfterCompress
			//如果超过，说明文件空间申请的过小，重新申请一下ReCreateFileMapping
			//新文件大小在原来的基础上增加(lpfirstFile->dwFileSize + 1mb) >= 64mb ? (lpfirstFile->dwFileSize + 1mb) :64mb
			//1mb=0x100000
			//64mb=0x4000000
			if((dwAddress + lpPckIndexTablePtr->dwCompressedFilesize + PCK_SPACE_DETECT_SIZE) > dwTotalFileSizeAfterCompress)
			{
				mt_lpFileWrite->UnMaping();
			
				dwTotalFileSizeAfterCompress += 
					((lpPckIndexTablePtr->dwCompressedFilesize + PCK_SPACE_DETECT_SIZE) > PCK_STEP_ADD_SIZE ? (lpPckIndexTablePtr->dwCompressedFilesize + PCK_SPACE_DETECT_SIZE) : PCK_STEP_ADD_SIZE);


				if(!mt_lpFileWrite->Mapping(pThis->m_szMapNameWrite, dwTotalFileSizeAfterCompress))
				{
					{
						EnterCriticalSection(&g_cs);
						*(mt_lpbThreadRunning) = FALSE;
						LeaveCriticalSection(&g_cs);
					}

					WaitForSingleObject(mt_evtAllCompressFinish, INFINITE);
					mt_dwFileCount = nWrite;
					mt_dwAddress = dwAddress;

					//释放
					while(0 != *bufferPtrToWrite)
					{
						free(*bufferPtrToWrite);
						bufferPtrToWrite++;
					}

					break;

				}

			}

			//处理lpPckFileIndex->dwAddressOffset
			//文件映射地址必须是64k(0x10000)的整数
			//添加 文件用的地址，重名时用
			PCKADDR	dwAddressTemp;
			if(0 != lpPckIndexTablePtr->dwAddressOfDuplicateOldDataArea/* && lpPckIndexTablePtr->bInvalid*/)
			{
				//当bInvalid为真且是写入老数据区时，变量dwIndexDataLength用来指示此时应该把数据向哪写，临时作dwAddress用
				dwAddressTemp = lpPckIndexTablePtr->dwAddressOfDuplicateOldDataArea;
			}else{
				dwAddressTemp = dwAddress;
			}

			if(0 != lpPckIndexTablePtr->dwCompressedFilesize)
			{
				//DWORD	dwMapViewBlockHigh, dwMapViewBlockLow;

				//dwMapViewBlockHigh = dwAddressTemp & 0xffff0000;
				//dwMapViewBlockLow = dwAddressTemp & 0xffff;

				
				if(NULL == (lpBufferToWrite = mt_lpFileWrite->View(dwAddressTemp, lpPckIndexTablePtr->dwCompressedFilesize)))
				{
					mt_lpFileWrite->UnMaping();
					{
						EnterCriticalSection(&g_cs);
						*(mt_lpbThreadRunning) = FALSE;
						LeaveCriticalSection(&g_cs);
					}

					WaitForSingleObject(mt_evtAllCompressFinish, INFINITE);
					mt_dwFileCount = nWrite;
					mt_dwAddress = dwAddress;

					//释放
					while(0 != *bufferPtrToWrite)
					{
						free(*bufferPtrToWrite);
						bufferPtrToWrite++;
					}
					break;
				}
				//检查地址是否正确
				//if(lpPckIndexTablePtr->dwAddress != dwAddress)OutputDebugStringA("lpPckIndexTablePtr->dwAddress != dwAddress!!!\r\n");

				memcpy(lpBufferToWrite, *bufferPtrToWrite, lpPckIndexTablePtr->dwCompressedFilesize);
				if(0 == lpPckIndexTablePtr->dwAddressOfDuplicateOldDataArea/* && lpPckIndexTablePtr->bInvalid*/)
				{
					dwAddress += lpPckIndexTablePtr->dwCompressedFilesize;
				}

				free(*bufferPtrToWrite);
				mt_lpFileWrite->UnmapView();
			}

			nWrite++;
			
			{
				EnterCriticalSection(&g_mt_lpMaxMemory);
				(*mt_lpMaxMemory) -= lpPckIndexTablePtr->dwMallocSize;

				if((*mt_lpMaxMemory) < mt_MaxMemory)
				{
					LeaveCriticalSection(&g_mt_lpMaxMemory);
					
					{
						EnterCriticalSection(&g_mt_nMallocBlocked);
						int nMallocBlocked = mt_nMallocBlocked;
						LeaveCriticalSection(&g_mt_nMallocBlocked);
						for(int i=0;i<nMallocBlocked;i++)
							SetEvent(mt_evtMaxMemory);

					}


				}else{
					LeaveCriticalSection(&g_mt_lpMaxMemory);
				}
			}		

			if(mt_dwFileCount == nWrite)
			{
				WaitForSingleObject(mt_evtAllCompressFinish, INFINITE);
				mt_dwAddress = dwAddress;
				
				break;
			}
			

			bufferPtrToWrite++;
			lpPckIndexTablePtr++;

		}else{
			
			LeaveCriticalSection(&g_mt_pckCompressedDataPtrArrayPtr);

			EnterCriticalSection(&g_cs);
			if(!(*mt_lpbThreadRunning))
			{
				LeaveCriticalSection(&g_cs);
				//取消
				WaitForSingleObject(mt_evtAllCompressFinish, INFINITE);

				mt_dwFileCount = nWrite;
				mt_dwAddress = dwAddress;

				break;
			}else{
				LeaveCriticalSection(&g_cs);
			}

			::Sleep(10);

		}

	}

	//mt_hMapFileToWrite = hMapFileToWrite;
	SetEvent(mt_evtAllWriteFinish);
	
	_endthread();
}



VOID CPckClass::PreProcess(int threadnum)
{

	if(1 != threadnum)
	{
		mt_lpbThreadRunning = &lpPckParams->cVarParams.bThreadRunning;//&bThreadRunning;
		mt_lpdwCount = &lpPckParams->cVarParams.dwUIProgress;//dwCount;
		mt_MaxMemory = lpPckParams->dwMTMaxMemory;//nMaxMemory;
		mt_lpMaxMemory = &lpPckParams->cVarParams.dwMTMemoryUsed;//nMaxMemory;
		//*mt_lpMaxMemory = 0;
		mt_nMallocBlocked = 0;
		mt_level = lpPckParams->dwCompressLevel;//level;
	}

}

BOOL CPckClass::FillPckHeaderAndInitArray(PCK_ALL_INFOS &PckAllInfo, int threadnum, DWORD dwFileCount)
{

	//构建头
	PckAllInfo.PckHead.dwHeadCheckHead = m_lpThisPckKey->HeadVerifyKey1;
#if defined PCKV202 || defined PCKV203ZX
	PckAllInfo.PckHead.dwHeadCheckTail = m_lpThisPckKey->HeadVerifyKey2;
#endif
	//pckHead.dwPckSize = |PCK文件大小|;

	ZeroMemory(&PckAllInfo.PckIndexAddr, sizeof(PCKINDEXADDR));

	PckAllInfo.PckIndexAddr.dwIndexTableCheckHead = m_lpThisPckKey->TailVerifyKey1;
	PckAllInfo.PckIndexAddr.dwIndexTableCheckTail = m_lpThisPckKey->TailVerifyKey2;
#ifdef PCKV203ZX
	PckAllInfo.PckIndexAddr.dwUnknown1 = 0xffffffff;
	PckAllInfo.PckIndexAddr.dwUnknown2 = 0;
#endif
	//pckIndexAddr.dwIndexValue = |文件名压缩数据开始| ^ FILEINDEX_ADDR_CONST;

#if defined PCKV202 || defined PCKV203ZX
	PckAllInfo.PckTail.dwVersion = PckAllInfo.PckIndexAddr.dwVersion = m_lpThisPckKey->Version;
#elif defined PCKV203
	PckAllInfo.PckTail.dwVersion = m_lpThisPckKey->Version;
#endif
	//pckTail.dwFileCount = dwFileCount;

	//申请空间,文件名压缩数据 数组
	if(NULL == (mt_lpPckIndexTable = new PCKINDEXTABLE_COMPRESS[dwFileCount]))
	{
		PrintLogE(TEXT_MALLOC_FAIL, __FILE__, __FUNCTION__, __LINE__);
		return FALSE;
	}
	memset(mt_lpPckIndexTable, 0, sizeof(PCKINDEXTABLE_COMPRESS) * dwFileCount);

	//////////////MT MODE START//////////////
	if(1 != threadnum)
	{
		if(NULL == (mt_pckCompressedDataPtrArray = (BYTE**) malloc (sizeof(BYTE*) * (dwFileCount  + 1))))
		{
			PrintLogE(TEXT_MALLOC_FAIL, __FILE__, __FUNCTION__, __LINE__);
			return FALSE;
		}
		memset(mt_pckCompressedDataPtrArray, 0, sizeof(BYTE*) * (dwFileCount + 1));
	}
	//////////////MT MODE END//////////////

	return TRUE;
}

void CPckClass::MultiThreadInitialize(VOID CompressThread(VOID*), VOID WriteThread(VOID*), int threadnum)
{
		//mt_evtToWrite = CreateEventA(NULL, FALSE, FALSE, "WritePckData");
		mt_evtAllWriteFinish = CreateEventA(NULL, FALSE, FALSE, m_szEventAllWriteFinish);
		mt_evtAllCompressFinish =  CreateEventA(NULL, TRUE, FALSE, m_szEventAllCompressFinish);
		mt_evtMaxMemory = CreateEventA(NULL, FALSE, FALSE, m_szEventMaxMemory);
		//mt_evtAbort = CreateEventA(NULL, FALSE, FALSE, "PckCompressAbort");
		mt_lpPckIndexTablePtr = mt_lpPckIndexTable;
		mt_pckCompressedDataPtrArrayPtr = mt_pckCompressedDataPtrArray;

		mt_threadID = 0;

		InitializeCriticalSection(&g_cs);
		InitializeCriticalSection(&g_mt_threadID);
		InitializeCriticalSection(&g_mt_nMallocBlocked);
		InitializeCriticalSection(&g_mt_lpMaxMemory);
		InitializeCriticalSection(&g_dwCompressedflag);
		InitializeCriticalSection(&g_mt_pckCompressedDataPtrArrayPtr);

		for(int i=0;i<threadnum;i++)
		{
			_beginthread(CompressThread, 0, this);
		}
		_beginthread(WriteThread, 0, this);

		WaitForSingleObject(mt_evtAllCompressFinish, INFINITE);

		//SetEvent(mt_evtToWrite);		//防止Write线程卡住 
		WaitForSingleObject(mt_evtAllWriteFinish, INFINITE);

		DeleteCriticalSection(&g_cs);
		DeleteCriticalSection(&g_mt_threadID);
		DeleteCriticalSection(&g_mt_nMallocBlocked);
		DeleteCriticalSection(&g_mt_lpMaxMemory);
		DeleteCriticalSection(&g_dwCompressedflag);
		DeleteCriticalSection(&g_mt_pckCompressedDataPtrArrayPtr);

		CloseHandle(mt_evtMaxMemory);
		//CloseHandle(mt_evtToWrite);
		CloseHandle(mt_evtAllCompressFinish);
		CloseHandle(mt_evtAllWriteFinish);
		
		if(!lpPckParams->cVarParams.bThreadRunning)
		{
			//lstrcpy(m_lastErrorString, TEXT_USERCANCLE);
			PrintLogW(TEXT_USERCANCLE);
		}

}

_inline BOOL __fastcall CPckClass::WritePckIndex(LPPCKINDEXTABLE_COMPRESS lpPckIndexTablePtr, PCKADDR &dwAddress)
{
	//处理lpPckFileIndex->dwAddressOffset
	//文件映射地址必须是64k(0x10000)的整数
	//DWORD	dwMapViewBlockHigh, dwMapViewBlockLow, ;

	//dwMapViewBlockHigh = mt_dwAddress & 0xffff0000;
	//dwMapViewBlockLow = mt_dwAddress & 0xffff;
	LPBYTE	lpBufferToWrite;
	DWORD	dwNumberOfBytesToMap = lpPckIndexTablePtr->dwIndexDataLength + 8;
	
	if(NULL == (lpBufferToWrite = mt_lpFileWrite->View(dwAddress, dwNumberOfBytesToMap)))
	{
		//delete mt_lpFileWrite;
		PrintLogE(TEXT_WRITE_PCK_INDEX, __FILE__, __FUNCTION__, __LINE__);
		return FALSE;
	}

	memcpy(lpBufferToWrite, lpPckIndexTablePtr, dwNumberOfBytesToMap);
	mt_lpFileWrite->UnmapView();

	dwAddress += dwNumberOfBytesToMap;
	//lpPckIndexTablePtr++;

	//窗口中以显示的文件进度
	lpPckParams->cVarParams.dwUIProgress++;

	return TRUE;
}


BOOL CPckClass::CreatePckFileMT(LPTSTR szPckFile, LPTSTR szPath)
{
	char		szLogString[LOG_BUFFER];

	sprintf(szLogString, TEXT_LOG_CREATE, szPckFile);
	PrintLogI(szLogString);

	DWORD		dwFileCount = 0/*, dwOldPckFileCount*/;					//文件数量, 原pck文件中的文件数
	QWORD		qwTotalFileSize = 0, qwTotalFileSizeTemp;			//未压缩时所有文件大小
	size_t		nLen;

	DWORD IndexCompressedFilenameDataLengthCryptKey1 = m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey1;
	DWORD IndexCompressedFilenameDataLengthCryptKey2 = m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey2;
	
	char		szPathMbsc[MAX_PATH];

	int level = lpPckParams->dwCompressLevel;
	int threadnum = lpPckParams->dwMTThread;

	//LOG
	sprintf(szLogString, TEXT_LOG_LEVEL_THREAD, level, threadnum);
	PrintLogI(szLogString);

	PreProcess(threadnum);

	//开始查找文件
	LPFILES_TO_COMPRESS		lpfirstFile;


	mt_dwAddress = PCK_DATA_START_AT;

	nLen = lstrlen(szPath) - 1;
	if('\\' == *(szPath + nLen))*(szPath + nLen) = 0;
	BOOL	IsPatition = lstrlen(szPath) == 2 ? TRUE : FALSE;

	nLen = WideCharToMultiByte(CP_ACP, 0, szPath, -1, szPathMbsc, MAX_PATH, "_", 0);

	if(NULL == m_firstFile)m_firstFile = AllocateFileinfo();
	if(NULL == m_firstFile)
	{
		PrintLogE(TEXT_MALLOC_FAIL, __FILE__, __FUNCTION__, __LINE__);
		return FALSE;
	}

	lpfirstFile = m_firstFile;

	EnumFile(szPathMbsc, IsPatition, dwFileCount, lpfirstFile, qwTotalFileSize, nLen);
	if(0 == dwFileCount)return TRUE;


	//文件数写入窗口类中保存以显示进度
	mt_dwFileCount = lpPckParams->cVarParams.dwUIProgressUpper = dwFileCount;

	//计算大概需要多大空间qwTotalFileSize
	qwTotalFileSizeTemp = qwTotalFileSize * 0.6;
#if defined PCKV202 || defined PCKV203ZX
	if(0 != (qwTotalFileSizeTemp >> 32))
#elif defined PCKV203
	if(0 != (qwTotalFileSizeTemp >> 33))
#endif
	{
		PrintLogE(TEXT_COMPFILE_TOOBIG, __FILE__, __FUNCTION__, __LINE__);
		return FALSE;
	}
	mt_CompressTotalFileSize = qwTotalFileSizeTemp;
	if(PCK_SPACE_DETECT_SIZE >= mt_CompressTotalFileSize)mt_CompressTotalFileSize = PCK_STEP_ADD_SIZE;

	PCK_ALL_INFOS	pckAllInfo;
	//
	////构建头
	////申请空间,文件名压缩数据 数组
	if(!FillPckHeaderAndInitArray(pckAllInfo, threadnum, dwFileCount))
		return FALSE;
	
	//开始压缩
	//打开文件
	LPBYTE				lpBufferToWrite, lpBufferToRead;
	

	//以下是创建一个文件，用来保存压缩后的文件
	mt_lpFileWrite = new CMapViewFileWrite();

	if(!OpenPckAndMappingWrite(mt_lpFileWrite, szPckFile, CREATE_ALWAYS, mt_CompressTotalFileSize)){

		delete mt_lpFileWrite;
		return FALSE;

	}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	lpPckParams->cVarParams.dwUIProgress = 0;
	PCKADDR	dwAddressName;

	if(1 == threadnum)
	{

		PCKFILEINDEX	pckFileIndex;
		pckFileIndex.dwUnknown1 = pckFileIndex.dwUnknown2 = 0;
#ifdef PCKV203ZX
		pckFileIndex.dwUnknown3 = pckFileIndex.dwUnknown4 = pckFileIndex.dwUnknown5 = 0;
#endif

		//初始化指针
		LPPCKINDEXTABLE_COMPRESS	lpPckIndexTablePtr = mt_lpPckIndexTable;
		lpfirstFile = m_firstFile;
		
		//初始化变量值 
		
		//char	*lpFilenamePtr = pckFileIndex.szFilename + MAX_PATH_PCK - mt_nLen;
		//memset(lpFilenamePtr, 0, mt_nLen);
		memset(pckFileIndex.szFilename, 0, MAX_PATH_PCK);
		
		//patch 140424
		CMapViewFileRead	*lpFileRead = new CMapViewFileRead();

		while(NULL != lpfirstFile->next)
		{
			//pckFileIndex.dwFileCipherTextSize = pckFileIndex.dwFileClearTextSize = lpfirstFile->dwFileSize;
			memcpy(pckFileIndex.szFilename, lpfirstFile->lpszFileTitle, lpfirstFile->nBytesToCopy);

			if(PCK_BEGINCOMPRESS_SIZE < lpfirstFile->dwFileSize)
			{
				pckFileIndex.dwFileClearTextSize = lpfirstFile->dwFileSize;
				pckFileIndex.dwFileCipherTextSize = compressBound(lpfirstFile->dwFileSize);//lpfirstFile->dwFileSize * 1.001 + 12;
				lpfirstFile->dwFileSize = pckFileIndex.dwFileCipherTextSize;
			}else{
				pckFileIndex.dwFileCipherTextSize = pckFileIndex.dwFileClearTextSize = lpfirstFile->dwFileSize;
			}
	
			if(!lpPckParams->cVarParams.bThreadRunning)
			{
				PrintLogW(TEXT_USERCANCLE);
				//目前已压缩了多少文件，将数据写入dwFileCount，写文件名列表和文件头、尾，完成文件操作
				mt_dwFileCount = lpPckParams->cVarParams.dwUIProgress;
				break;
			}
	
			//如果文件大小为0，则跳过打开文件步骤
			if(0 != lpfirstFile->dwFileSize)
			{
				//判断一下dwAddress的值会不会超过dwTotalFileSizeAfterCompress
				//如果超过，说明文件空间申请的过小，重新申请一下ReCreateFileMapping
				//新文件大小在原来的基础上增加(lpfirstFile->dwFileSize + 1mb) >= 64mb ? (lpfirstFile->dwFileSize + 1mb) :64mb
				//1mb=0x100000
				//64mb=0x4000000
				if((mt_dwAddress + lpfirstFile->dwFileSize + PCK_SPACE_DETECT_SIZE) > mt_CompressTotalFileSize)
				{
					mt_lpFileWrite->UnMaping();
				
					mt_CompressTotalFileSize += 
						((lpfirstFile->dwFileSize + PCK_SPACE_DETECT_SIZE) > PCK_STEP_ADD_SIZE ? (lpfirstFile->dwFileSize + PCK_SPACE_DETECT_SIZE) : PCK_STEP_ADD_SIZE);
	
					
					if(!mt_lpFileWrite->Mapping(m_szMapNameWrite, mt_CompressTotalFileSize))
					{
						lpPckParams->cVarParams.bThreadRunning = FALSE;
						mt_dwFileCount = lpPckParams->cVarParams.dwUIProgress;
						break;
					}
	
				}
	
				if(NULL == (lpBufferToWrite = mt_lpFileWrite->View(mt_dwAddress, lpfirstFile->dwFileSize)))
				{
					//CloseHandle(mt_hMapFileToWrite);
					mt_lpFileWrite->UnMaping();

					lpPckParams->cVarParams.bThreadRunning = FALSE;
					mt_dwFileCount = lpPckParams->cVarParams.dwUIProgress;
					break;
				}
	
				////////////////////////////打开源文件/////////////////////////////////
				//CMapViewFileRead	*lpFileRead = new CMapViewFileRead();
				////打开要进行压缩的文件
				if(!lpFileRead->Open(lpfirstFile->szFilename))
				{
					PrintLogE(TEXT_OPENNAME_FAIL, lpfirstFile->szFilename, __FILE__, __FUNCTION__, __LINE__);

					mt_lpFileWrite->UnMaping();
					//delete lpFileRead;
					lpFileRead->clear();

					lpPckParams->cVarParams.bThreadRunning = FALSE;
					mt_dwFileCount = lpPckParams->cVarParams.dwUIProgress;
					break;
				}
	
				//创建一个文件映射
				if(!lpFileRead->Mapping(m_szMapNameRead))
				{
					mt_lpFileWrite->UnMaping();
					//delete lpFileRead;
					lpFileRead->clear();

					lpPckParams->cVarParams.bThreadRunning = FALSE;
					mt_dwFileCount = lpPckParams->cVarParams.dwUIProgress;
					break;
				}
	

				//创建一个文件映射的视图用来作为source
				if(NULL == (lpBufferToRead = lpFileRead->View(0, 0)))
				{
					mt_lpFileWrite->UnMaping();
					//delete lpFileRead;
					lpFileRead->clear();

					lpPckParams->cVarParams.bThreadRunning = FALSE;
					mt_dwFileCount = lpPckParams->cVarParams.dwUIProgress;
					break;
				}
	
			////////////////////////////打开源文件/////////////////////////////////
			
				if(PCK_BEGINCOMPRESS_SIZE < pckFileIndex.dwFileCipherTextSize)
				{
					//pckFileIndex.dwFileCipherTextSize = pckFileIndex.dwFileCipherTextSize * 1.001 + 12;
	
					compress2(lpBufferToWrite, &pckFileIndex.dwFileCipherTextSize, 
									lpBufferToRead, pckFileIndex.dwFileClearTextSize, level);
				}else{
					memcpy(lpBufferToWrite, lpBufferToRead, pckFileIndex.dwFileClearTextSize);
				}

				mt_lpFileWrite->UnmapView();
				//delete lpFileRead;
				lpFileRead->clear();

			}//文件不为0时的处理
	
			//写入此文件的PckFileIndex文件压缩信息并压缩
			pckFileIndex.dwAddressOffset = mt_dwAddress;		//此文件的压缩数据起始地址
			mt_dwAddress += pckFileIndex.dwFileCipherTextSize;	//下一个文件的压缩数据起始地址
	
			lpPckIndexTablePtr->dwIndexDataLength = INDEXTABLE_CLEARTEXT_LENGTH;
			compress2(lpPckIndexTablePtr->buffer, &lpPckIndexTablePtr->dwIndexDataLength, 
							(BYTE*)&pckFileIndex, INDEXTABLE_CLEARTEXT_LENGTH, level);
			//将获取的
			lpPckIndexTablePtr->dwIndexValueHead = lpPckIndexTablePtr->dwIndexDataLength ^ IndexCompressedFilenameDataLengthCryptKey1;
			lpPckIndexTablePtr->dwIndexValueTail = lpPckIndexTablePtr->dwIndexDataLength ^ IndexCompressedFilenameDataLengthCryptKey2;
	
			//窗口中以显示的文件进度
			lpPckParams->cVarParams.dwUIProgress++;
			//加密的文件名数据数组
			lpPckIndexTablePtr++;
			//下一个文件列表
			lpfirstFile = lpfirstFile->next;
		}

		delete lpFileRead;

	}else{
	
		MultiThreadInitialize(CompressThread, WriteThread, threadnum);

	}

	
	//LOG
	PrintLogI(TEXT_LOG_COMPRESSOK);
	
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	//写文件索引
	LPPCKINDEXTABLE_COMPRESS lpPckIndexTablePtr = mt_lpPckIndexTable;
	dwAddressName = mt_dwAddress;

	//窗口中以显示的文件进度，初始化，显示写索引进度mt_hFileToWrite
	lpPckParams->cVarParams.dwUIProgress = 0;

	for(DWORD	i = 0;i<mt_dwFileCount;i++)
	{
		WritePckIndex(lpPckIndexTablePtr, mt_dwAddress);
		lpPckIndexTablePtr++;

	}
	
	mt_lpFileWrite->UnMaping();

	AfterProcess(mt_lpFileWrite, pckAllInfo, mt_dwFileCount, dwAddressName, mt_dwAddress);

	delete mt_lpFileWrite;
	DeallocateFileinfo();
	delete [] mt_lpPckIndexTable;

	if(1 != threadnum)
	{
		free(mt_pckCompressedDataPtrArray);
	}

	//LOG
	PrintLogI(TEXT_LOG_WORKING_DONE);

	return TRUE;
}



//更新pck包
BOOL CPckClass::UpdatePckFile(LPTSTR szPckFile, TCHAR (*lpszFilePath)[MAX_PATH], int nFileCount, LPPCK_PATH_NODE lpNodeToInsert)
{
	DWORD		dwFileCount = 0, dwOldPckFileCount;					//文件数量, 原pck文件中的文件数
	QWORD		qwTotalFileSize = 0, qwTotalFileSizeTemp;			//未压缩时所有文件大小
	size_t		nLen;

	DWORD		IndexCompressedFilenameDataLengthCryptKey1 = m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey1;
	DWORD		IndexCompressedFilenameDataLengthCryptKey2 = m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey2;

	char		szPathMbsc[MAX_PATH];
	char		szLogString[LOG_BUFFER];

	int level = lpPckParams->dwCompressLevel;
	int threadnum = lpPckParams->dwMTThread;

	PreProcess(threadnum);

	//开始查找文件
	LPFILES_TO_COMPRESS			lpfirstFile;//, lpPrevFile = NULL;
	//LPFILES_TO_COMPRESS		m_firstFile;

	//mt_dwAddress = mt_dwAddressName;
	//if(!m_ReadCompleted)wcscpy_s(szPckFile, MAX_PATH, m_Filename);

	TCHAR	(*lpszFilePathPtr)[MAX_PATH] = (TCHAR(*)[MAX_PATH])lpszFilePath;
	
	//lpszFilePathPtr = (TCHAR(*)[MAX_PATH])lpszFilePath;

	DWORD				dwAppendCount = nFileCount;
	LPPCK_PATH_NODE		lpNodeToInsertPtr;// = lpNodeToInsert;


	//设置参数
	if(m_ReadCompleted)
	{
		lstrcpy(szPckFile, m_PckAllInfo.szFilename);

		mt_dwAddress = mt_dwAddressName = m_PckAllInfo.dwAddressName;
		dwOldPckFileCount = m_PckAllInfo.PckTail.dwFileCount;

		lpNodeToInsertPtr = lpNodeToInsert;

		//取得当前节点的相对路径
		if(!GetCurrentNodeString(mt_szCurrentNodeString, lpNodeToInsert))
		{
			free(lpszFilePath);
			return FALSE;
		}
		
		mt_nCurrentNodeStringLen = strlen(mt_szCurrentNodeString);

		sprintf(szLogString, TEXT_LOG_UPDATE_ADD
							"-"
							TEXT_LOG_LEVEL_THREAD, level, threadnum);
		PrintLogI(szLogString);

	}else{

		mt_dwAddress = mt_dwAddressName = m_PckAllInfo.dwAddressName = PCK_DATA_START_AT;
		dwOldPckFileCount = 0;

		lpNodeToInsertPtr = m_RootNode.child;

		*mt_szCurrentNodeString = 0;

		mt_nCurrentNodeStringLen = 0;

		sprintf(szLogString, TEXT_LOG_UPDATE_NEW
							"-"
							TEXT_LOG_LEVEL_THREAD, level, threadnum);
		PrintLogI(szLogString);

	}

	if(NULL == m_firstFile)m_firstFile = AllocateFileinfo();
	if(NULL == m_firstFile)
	{
		PrintLogE(TEXT_MALLOC_FAIL, __FILE__, __FUNCTION__, __LINE__);
		free(lpszFilePath);
		return FALSE;
	}

	lpfirstFile = m_firstFile;

	//读文件
	CMapViewFileRead *lpcFileRead = new CMapViewFileRead();

	for(DWORD i = 0;i<dwAppendCount;i++)
	{
		WideCharToMultiByte(CP_ACP, 0, *lpszFilePathPtr, -1, szPathMbsc, MAX_PATH, "_", 0);
		nLen = (size_t)(strrchr(szPathMbsc, '\\') - szPathMbsc) + 1;
		
		if(FILE_ATTRIBUTE_DIRECTORY == (FILE_ATTRIBUTE_DIRECTORY & GetFileAttributesA(szPathMbsc) ))
		{
			//文件夹
			EnumFile(szPathMbsc, FALSE, dwFileCount, lpfirstFile, qwTotalFileSize, nLen);
		}else{
			//文件
			//CMapViewFileRead	cFileRead;

			if(!lpcFileRead->Open(szPathMbsc))
			{
				DeallocateFileinfo();
				free(lpszFilePath);
				PrintLogE(TEXT_OPENNAME_FAIL, *lpszFilePathPtr, __FILE__, __FUNCTION__, __LINE__);

				delete lpcFileRead;
				return FALSE;
			}
			
			strcpy(lpfirstFile->szFilename, szPathMbsc);
			//memcpy(lpfirstFile->szFilename, szPathMbsc, MAX_PATH);

			lpfirstFile->lpszFileTitle = lpfirstFile->szFilename + nLen;
			lpfirstFile->nBytesToCopy = MAX_PATH_PCK - nLen;

			qwTotalFileSize += (lpfirstFile->dwFileSize = lpcFileRead->GetFileSize());

			lpfirstFile->next = AllocateFileinfo();
			lpfirstFile = lpfirstFile->next;

			lpcFileRead->clear();

			dwFileCount++;
		}

		lpszFilePathPtr++;
	}

	delete lpcFileRead;

	free(lpszFilePath);

	if(0 == dwFileCount)return TRUE;

	//参数说明：
	// mt_dwFileCount	添加的文件总数，计重复
	// dwAllCount		界面显示的总数->lpPckParams->cVarParams.dwUIProgressUpper
	// dwFileCount		计算过程使用参数，在下面的计算过程中将使用此参数表示添加的文件总数，不计重复
	// 

	//文件数写入窗口类中保存以显示进度
	DWORD dwPrepareToAdd = mt_dwFileCount = lpPckParams->cVarParams.dwUIProgressUpper = dwFileCount;

	//计算大概需要多大空间qwTotalFileSize
	qwTotalFileSizeTemp = qwTotalFileSize * 0.6 + mt_dwAddressName;
#if defined PCKV202 || defined PCKV203ZX
	if(0 != (qwTotalFileSizeTemp >> 32))
#elif defined PCKV203
	if(0 != (qwTotalFileSizeTemp >> 33))
#endif
	{
		PrintLogE(TEXT_COMPFILE_TOOBIG, __FILE__, __FUNCTION__, __LINE__);
		return FALSE;
	}
	mt_CompressTotalFileSize = qwTotalFileSizeTemp;

	if(PCK_SPACE_DETECT_SIZE >= mt_CompressTotalFileSize)mt_CompressTotalFileSize = PCK_STEP_ADD_SIZE;


	//与原来目录中的文件对比，是否有重名
	//策略：无条件覆盖吧				如果重名且都为文件或文件夹，则覆盖
	//
	//调用FindFileNode返回-1退出，返回0，表示直接添加，非0就是有重复的
	//写专用的writethread和compressthread,以调用
	//在PCKINDEXTABLE_COMPRESS里添加add专用属性，以判断是否启用此节点（重名时）0使用，1不使用
	//结束 时使用2个循环写入压缩索引 

	//dwFileCount变量在此处指的是添加的文件除去重名的数量 

	//lpPrevFile = NULL;
	if(m_ReadCompleted)
	{
		lpfirstFile = m_firstFile;
		while(NULL != lpfirstFile->next)
		{
			LPPCK_PATH_NODE lpDuplicateNode;
			lpDuplicateNode = FindFileNode(lpNodeToInsertPtr, lpfirstFile->lpszFileTitle);


			if(-1 == (int)lpDuplicateNode)
			{
				DeallocateFileinfo();
				PrintLogE(TEXT_ERROR_DUP_FOLDER_FILE);
				return FALSE;
			}

			if(NULL != lpDuplicateNode)
			{
				lpfirstFile->samePtr = lpDuplicateNode->lpPckIndexTable;
				dwFileCount--;
			}

			lpfirstFile = lpfirstFile->next;

		}
	}

	PCK_ALL_INFOS	pckAllInfo;

	//日志
	sprintf(szLogString, "预添加文件数=%d:预期文件大小=%d, 开始进行作业...", dwPrepareToAdd, mt_CompressTotalFileSize);
	PrintLogI(szLogString);
	
	//构建头
	//申请空间,文件名压缩数据 数组， 这里是第2索引
	if(!FillPckHeaderAndInitArray(pckAllInfo, threadnum, mt_dwFileCount))
		return FALSE;
	
	//开始压缩
	//打开文件
	LPBYTE		lpBufferToWrite, lpBufferToRead;

	//以下是创建一个文件，用来保存压缩后的文件
	mt_lpFileWrite = new CMapViewFileWrite();

	//OPEN_ALWAYS，新建新的pck(CREATE_ALWAYS)或更新存在的pck(OPEN_EXISTING)

	if(!OpenPckAndMappingWrite(mt_lpFileWrite, szPckFile, OPEN_ALWAYS, mt_CompressTotalFileSize)){

		delete mt_lpFileWrite;
		return FALSE;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//mt_dwAddress = 12;
	lpPckParams->cVarParams.dwUIProgress = 0;
	//DWORD	/*dwAddress = 12, */dwAddressName;

	BOOL		bUseCurrent;		//重名时是否使用当前数据，添加模式下有效
	PCKADDR		dwAddress;			//添加模式下，重名时用来指示被重名文件远大于添加文件时，压缩数据的存放地址
	DWORD		dwOverWriteModeMaxLength;	//重名时，用来覆盖被重名文件时使用的最大ViewMap大小，不然多出的的数据会把后面文件的数据清0

	if(1 == threadnum)
	{

		PCKFILEINDEX	pckFileIndex;
		pckFileIndex.dwUnknown1 = pckFileIndex.dwUnknown2 = 0;
#ifdef PCKV203ZX
		pckFileIndex.dwUnknown3 = pckFileIndex.dwUnknown4 = pckFileIndex.dwUnknown5 = 0;
#endif
		//初始化指针
		LPPCKINDEXTABLE_COMPRESS	lpPckIndexTablePtr = mt_lpPckIndexTable;
		lpfirstFile = m_firstFile;
		
		//初始化变量值 
		
		//char	*lpFilenamePtr = pckFileIndex.szFilename + MAX_PATH_PCK - mt_nLen;
		//memset(lpFilenamePtr, 0, mt_nLen);
		char	*lpFilenamePtr = pckFileIndex.szFilename;
		memset(lpFilenamePtr, 0, MAX_PATH_PCK);

		//patch 140424
		CMapViewFileRead	*lpFileRead = new CMapViewFileRead();
		
		while(NULL != lpfirstFile->next)
		{

			memcpy(mystrcpy(pckFileIndex.szFilename, mt_szCurrentNodeString), lpfirstFile->lpszFileTitle, lpfirstFile->nBytesToCopy - mt_nCurrentNodeStringLen);

			if(PCK_BEGINCOMPRESS_SIZE < lpfirstFile->dwFileSize)
			{
				pckFileIndex.dwFileClearTextSize = lpfirstFile->dwFileSize;
				pckFileIndex.dwFileCipherTextSize = compressBound(lpfirstFile->dwFileSize);//lpfirstFile->dwFileSize * 1.001 + 12;
				lpfirstFile->dwFileSize = pckFileIndex.dwFileCipherTextSize;
			}else{
				pckFileIndex.dwFileCipherTextSize = pckFileIndex.dwFileClearTextSize = lpfirstFile->dwFileSize;
			}
	
			if(!lpPckParams->cVarParams.bThreadRunning)
			{
				PrintLogW(TEXT_USERCANCLE);

				//目前已压缩了多少文件，将数据写入dwFileCount，写文件名列表和文件头、尾，完成文件操作
				mt_dwFileCount = lpPckParams->cVarParams.dwUIProgress;
				break;
			}
	
			//如果文件大小为0，则跳过打开文件步骤
			if(0 != lpfirstFile->dwFileSize)
			{

				//判断一下dwAddress的值会不会超过dwTotalFileSizeAfterCompress
				//如果超过，说明文件空间申请的过小，重新申请一下ReCreateFileMapping
				//新文件大小在原来的基础上增加(lpfirstFile->dwFileSize + 1mb) >= 64mb ? (lpfirstFile->dwFileSize + 1mb) :64mb
				//1mb=0x100000
				//64mb=0x4000000
				if((mt_dwAddress + lpfirstFile->dwFileSize + PCK_SPACE_DETECT_SIZE) > mt_CompressTotalFileSize)
				{
					//CloseHandle(mt_hMapFileToWrite);
					mt_lpFileWrite->UnMaping();
				
					mt_CompressTotalFileSize += 
						((lpfirstFile->dwFileSize + PCK_SPACE_DETECT_SIZE) > PCK_STEP_ADD_SIZE ? (lpfirstFile->dwFileSize + PCK_SPACE_DETECT_SIZE) : PCK_STEP_ADD_SIZE);
	
					if(!mt_lpFileWrite->Mapping(m_szMapNameWrite, mt_CompressTotalFileSize))
					{
						lpPckParams->cVarParams.bThreadRunning = FALSE;
						mt_dwFileCount = lpPckParams->cVarParams.dwUIProgress;
						break;
					}
	
				}
				//BOOL		bUseCurrent;
				//DWORD		dwAddress;
				//添加模式
				dwOverWriteModeMaxLength = lpfirstFile->dwFileSize;

				if(NULL != lpfirstFile->samePtr)
				{
					//如果现有的文件大小大于pck中的文件的已压缩大小，使用新buffer
					if(dwOverWriteModeMaxLength >= lpfirstFile->samePtr->cFileIndex.dwFileCipherTextSize)
					{
						lpfirstFile->samePtr->cFileIndex.dwAddressOffset = mt_dwAddress;
						bUseCurrent = TRUE;
						//dwOverWriteModeMaxLength = lpfirstFile->dwFileSize;

						dwAddress = mt_dwAddress;
					}else{

						dwAddress = lpfirstFile->samePtr->cFileIndex.dwAddressOffset;

						bUseCurrent = FALSE;
						dwOverWriteModeMaxLength = lpfirstFile->samePtr->cFileIndex.dwFileCipherTextSize;
					}
				}else{
					dwAddress = mt_dwAddress;
					//dwOverWriteModeMaxLength = lpfirstFile->dwFileSize;
				}

				//处理lpPckFileIndex->dwAddressOffset
				//文件映射地址必须是64k(0x10000)的整数
				//DWORD	dwMapViewBlockHigh, dwMapViewBlockLow/*, dwNumberOfBytesToMap*/;
	
				//dwMapViewBlockHigh = dwAddress & 0xffff0000;
				//dwMapViewBlockLow = dwAddress & 0xffff;
	
				
				if(NULL == (lpBufferToWrite = mt_lpFileWrite->View(dwAddress, dwOverWriteModeMaxLength)))
				{
					//CloseHandle(mt_hMapFileToWrite);
					mt_lpFileWrite->UnMaping();
					lpPckParams->cVarParams.bThreadRunning = FALSE;
					mt_dwFileCount = lpPckParams->cVarParams.dwUIProgress;
					break;
				}
				
				//CMapViewFileRead	*lpFileRead = new CMapViewFileRead();

				if(!lpFileRead->Open(lpfirstFile->szFilename))
				{
					mt_lpFileWrite->UnMaping();
					//delete lpFileRead;
					lpFileRead->clear();


					lpPckParams->cVarParams.bThreadRunning = FALSE;
					mt_dwFileCount = lpPckParams->cVarParams.dwUIProgress;
					break;
				}

				if(!lpFileRead->Mapping(m_szMapNameRead))
				{
					mt_lpFileWrite->UnMaping();
					//delete lpFileRead;
					lpFileRead->clear();

					lpPckParams->cVarParams.bThreadRunning = FALSE;
					mt_dwFileCount = lpPckParams->cVarParams.dwUIProgress;
					break;
				}

				if(NULL == (lpBufferToRead = lpFileRead->View(0, 0)))
				{
					mt_lpFileWrite->UnMaping();
					//delete lpFileRead;
					lpFileRead->clear();

					lpPckParams->cVarParams.bThreadRunning = FALSE;
					mt_dwFileCount = lpPckParams->cVarParams.dwUIProgress;
					break;
				}
	
				////////////////////////////打开源文件/////////////////////////////////
			
				if(PCK_BEGINCOMPRESS_SIZE < pckFileIndex.dwFileClearTextSize)
				{
					//pckFileIndex.dwFileCipherTextSize = dwOverWriteModeMaxLength/*pckFileIndex.dwFileCipherTextSize * 1.001 + 12*/;
	
					compress2(lpBufferToWrite, &pckFileIndex.dwFileCipherTextSize, 
									lpBufferToRead, pckFileIndex.dwFileClearTextSize, level);
				}else{
					memcpy(lpBufferToWrite, lpBufferToRead, pckFileIndex.dwFileClearTextSize);
				}
	
				mt_lpFileWrite->UnmapView();
				//delete lpFileRead;
				lpFileRead->clear();

			}//文件不为0时的处理
			else{
				if(NULL != lpfirstFile->samePtr)
				{
					bUseCurrent = FALSE;
				}
			}
	
			//写入此文件的PckFileIndex文件压缩信息并压缩
			if(NULL == lpfirstFile->samePtr)
			{
				
				pckFileIndex.dwAddressOffset = mt_dwAddress;		//此文件的压缩数据起始地址
				mt_dwAddress += pckFileIndex.dwFileCipherTextSize;	//下一个文件的压缩数据起始地址
		
				lpPckIndexTablePtr->dwIndexDataLength = INDEXTABLE_CLEARTEXT_LENGTH;
				compress2(lpPckIndexTablePtr->buffer, &lpPckIndexTablePtr->dwIndexDataLength, 
								(BYTE*)&pckFileIndex, INDEXTABLE_CLEARTEXT_LENGTH, level);
				//将获取的
				lpPckIndexTablePtr->dwIndexValueHead = lpPckIndexTablePtr->dwIndexDataLength ^ IndexCompressedFilenameDataLengthCryptKey1;
				lpPckIndexTablePtr->dwIndexValueTail = lpPckIndexTablePtr->dwIndexDataLength ^ IndexCompressedFilenameDataLengthCryptKey2;

				//窗口中以显示的文件进度
				lpPckParams->cVarParams.dwUIProgress++;

				//添加node


			}else{
				lpPckIndexTablePtr->bInvalid = TRUE;

				lpfirstFile->samePtr->cFileIndex.dwFileCipherTextSize = pckFileIndex.dwFileCipherTextSize;
				lpfirstFile->samePtr->cFileIndex.dwFileClearTextSize = pckFileIndex.dwFileClearTextSize;

				if(bUseCurrent)
				{
					mt_dwAddress += pckFileIndex.dwFileCipherTextSize;
					
				}
			}

			//加密的文件名数据数组
			lpPckIndexTablePtr++;
			//下一个文件列表
			lpfirstFile = lpfirstFile->next;
		}

		delete lpFileRead;

	}else{

		MultiThreadInitialize(CompressThreadAdd, WriteThreadAdd, threadnum);

	}
	
	
	
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//打印报告用参数
	DWORD	dwUseNewDataAreaInDuplicateFile = 0;

	//写文件索引
	PCKADDR dwAddressName = mt_dwAddress;

	//窗口中以显示的文件进度，初始化，显示写索引进度mt_hFileToWrite
	lpPckParams->cVarParams.dwUIProgress = 0;
	//dwAllCount = mt_dwFileCount + dwOldPckFileCount;	//这里的文件数包含了重名的数量，应该是下面的公式
	lpPckParams->cVarParams.dwUIProgressUpper = dwFileCount + dwOldPckFileCount;


	//写原来的文件
	LPPCKINDEXTABLE	lpPckIndexTableSrc = m_lpPckIndexTable;

	//__try{

		for(DWORD	i = 0;i<dwOldPckFileCount;i++)
		{

			PCKINDEXTABLE_COMPRESS	pckIndexTableTemp;


			pckIndexTableTemp.dwIndexDataLength = INDEXTABLE_CLEARTEXT_LENGTH;
			compress2(pckIndexTableTemp.buffer, &pckIndexTableTemp.dwIndexDataLength, 
							(BYTE*)&lpPckIndexTableSrc->cFileIndex, INDEXTABLE_CLEARTEXT_LENGTH, level);
			//将获取的
			pckIndexTableTemp.dwIndexValueHead = pckIndexTableTemp.dwIndexDataLength ^ IndexCompressedFilenameDataLengthCryptKey1;
			pckIndexTableTemp.dwIndexValueTail = pckIndexTableTemp.dwIndexDataLength ^ IndexCompressedFilenameDataLengthCryptKey2;

			WritePckIndex(&pckIndexTableTemp, mt_dwAddress);

			//是否使用了老数据区，只有重名时才有可能
			if(lpPckIndexTableSrc->cFileIndex.dwAddressOffset >= m_PckAllInfo.dwAddressName)
				dwUseNewDataAreaInDuplicateFile++;

			lpPckIndexTableSrc++;

		}

		//写添加的文件
		LPPCKINDEXTABLE_COMPRESS lpPckIndexTablePtr = mt_lpPckIndexTable;

		dwFileCount = mt_dwFileCount;
		for(DWORD	i = 0;i<mt_dwFileCount;i++)
		{
			////处理lpPckFileIndex->dwAddressOffset
			////文件映射地址必须是64k(0x10000)的整数

			if(!lpPckIndexTablePtr->bInvalid)
			{
				WritePckIndex(lpPckIndexTablePtr, mt_dwAddress);

			}else{
				dwFileCount--;
			}
			lpPckIndexTablePtr++;

		}
	//}__except{

	//}

	mt_lpFileWrite->UnMaping();

	//pckTail.dwFileCount = dwFileCount + dwOldPckFileCount;//mt_dwFileCount是实际写入数，重复的已经在上面减去了
	AfterProcess(mt_lpFileWrite, pckAllInfo, dwFileCount + dwOldPckFileCount, dwAddressName, mt_dwAddress);

	delete mt_lpFileWrite;
	DeallocateFileinfo();
	delete [] mt_lpPckIndexTable;

	if(1 != threadnum)
	{
		free(mt_pckCompressedDataPtrArray);
	}

	//在这里重新打开一次，或者直接打开，由界面线程完成

	lpPckParams->cVarParams.dwOldFileCount = dwOldPckFileCount;
	lpPckParams->cVarParams.dwPrepareToAddFileCount = dwPrepareToAdd;
	lpPckParams->cVarParams.dwChangedFileCount = mt_dwFileCount;
	lpPckParams->cVarParams.dwDuplicateFileCount = mt_dwFileCount - dwFileCount;
	lpPckParams->cVarParams.dwUseNewDataAreaInDuplicateFileSize = dwUseNewDataAreaInDuplicateFile;
	lpPckParams->cVarParams.dwFinalFileCount = pckAllInfo.PckTail.dwFileCount;


	PrintLogI(TEXT_LOG_WORKING_DONE);

	//sprintf(szLogString, "操作完成", dwPrepareToAdd, mt_CompressTotalFileSize);
	//PrintLogI(szLogString);

	return TRUE;

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//重命名文件
BOOL CPckClass::RenameFilename()
{

	PrintLogI(TEXT_LOG_RENAME);

	//LPBYTE		lpBufferToWrite;
	int			level = lpPckParams->dwCompressLevel;
	DWORD		IndexCompressedFilenameDataLengthCryptKey1 = m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey1;
	DWORD		IndexCompressedFilenameDataLengthCryptKey2 = m_lpThisPckKey->IndexCompressedFilenameDataLengthCryptKey2;

	//以下是创建一个文件，用来保存压缩后的文件
	mt_lpFileWrite = new CMapViewFileWrite();
	if (!mt_lpFileWrite->OpenPck(m_PckAllInfo.szFilename, OPEN_EXISTING))
	{

		PrintLogE(TEXT_OPENWRITENAME_FAIL, m_PckAllInfo.szFilename, __FILE__, __FUNCTION__, __LINE__);

		delete mt_lpFileWrite;
		return FALSE;
	}

	PCKADDR dwFileSize = mt_lpFileWrite->GetFileSize() + 0x1000000;
	
	if(!mt_lpFileWrite->Mapping(m_szMapNameWrite, dwFileSize))
	{
		PrintLogE(TEXT_CREATEMAPNAME_FAIL, m_PckAllInfo.szFilename, __FILE__, __FUNCTION__, __LINE__);

		delete mt_lpFileWrite;
		return FALSE;
	}
	//写文件索引
	
	PCKADDR dwAddress = m_PckAllInfo.dwAddressName;

	//窗口中以显示的文件进度，初始化，显示写索引进度mt_hFileToWrite
	lpPckParams->cVarParams.dwUIProgress = 0;
	//dwAllCount = mt_dwFileCount + dwOldPckFileCount;	//这里的文件数包含了重名的数量，应该是下面的公式
	lpPckParams->cVarParams.dwUIProgressUpper = m_PckAllInfo.PckTail.dwFileCount;


	//写原来的文件
	LPPCKINDEXTABLE	lpPckIndexTableSrc = m_lpPckIndexTable;

	////写入索引是否成功
	//BOOL isWritePckIndexSuccess = TRUE;

	for(DWORD	i = 0;i<lpPckParams->cVarParams.dwUIProgressUpper;++i)
	{

		PCKINDEXTABLE_COMPRESS	pckIndexTableTemp;

		if(lpPckIndexTableSrc->bSelected)
		{
			--m_PckAllInfo.PckTail.dwFileCount;
			++lpPckIndexTableSrc;

			//窗口中以显示的文件进度
			++lpPckParams->cVarParams.dwUIProgress;
			continue;
		}


		pckIndexTableTemp.dwIndexDataLength = INDEXTABLE_CLEARTEXT_LENGTH;
		compress2(pckIndexTableTemp.buffer, &pckIndexTableTemp.dwIndexDataLength, 
						(BYTE*)&lpPckIndexTableSrc->cFileIndex, INDEXTABLE_CLEARTEXT_LENGTH, level);
		//将获取的
		pckIndexTableTemp.dwIndexValueHead = pckIndexTableTemp.dwIndexDataLength ^ IndexCompressedFilenameDataLengthCryptKey1;
		pckIndexTableTemp.dwIndexValueTail = pckIndexTableTemp.dwIndexDataLength ^ IndexCompressedFilenameDataLengthCryptKey2;

		WritePckIndex(&pckIndexTableTemp, dwAddress);

		++lpPckIndexTableSrc;

	}

	mt_lpFileWrite->UnMaping();

	//写pckIndexAddr，272字节
	mt_lpFileWrite->SetFilePointer(dwAddress, FILE_BEGIN);
	//DWORD	dwBytesWrite;
	//WriteFile(mt_hFileToWrite, &m_PckIndexAddr, sizeof(PCKINDEXADDR), &dwBytesWrite, NULL);
	dwAddress += mt_lpFileWrite->Write(&m_PckAllInfo.PckIndexAddr, sizeof(PCKINDEXADDR));

	//写pckTail
	//pckTail.dwFileCount = mt_dwFileCount + dwOldPckFileCount;//mt_dwFileCount是实际写入数，重复的已经在上面减去了
	//WriteFile(mt_hFileToWrite, &m_PckTail, sizeof(PCKTAIL), &dwBytesWrite, NULL);
	dwAddress += mt_lpFileWrite->Write(&(m_PckAllInfo.PckTail), sizeof(PCKTAIL));
	
	//写pckHead
	m_PckAllInfo.PckHead.dwPckSize = dwAddress;
	mt_lpFileWrite->SetFilePointer(0, FILE_BEGIN);
	mt_lpFileWrite->Write(&m_PckAllInfo.PckHead, sizeof(PCKHEAD));


	//这里将文件大小重新设置一下
	mt_lpFileWrite->SetFilePointer(dwAddress, FILE_BEGIN);
	mt_lpFileWrite->SetEndOfFile();

	delete mt_lpFileWrite;

	PrintLogI(TEXT_LOG_WORKING_DONE);

	return TRUE;
}

