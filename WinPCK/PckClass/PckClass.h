//////////////////////////////////////////////////////////////////////
// PckClass.h: 用于解析完美世界公司的pck文件中的数据，并显示在List中
// 头文件
//
// 此程序由 李秋枫/stsm/liqf 编写，pck结构参考若水的pck结构.txt，并
// 参考了其易语言代码中并于读pck文件列表的部分
//
// 此代码预计将会开源，任何基于此代码的修改发布请保留原作者信息
// 
// 2012.4.10
//////////////////////////////////////////////////////////////////////

#include "MapViewFile.h"
//#include "Pck.h"
#include "PckHeader.h"
#include <stdio.h>
#include "PckVersion.h"



#if !defined(_PCKCLASS_H_)
#define _PCKCLASS_H_

class CPckClass  
{
//函数
public:
	void	CPckClassInit();
	//CPckClass();
	CPckClass(LPPCK_RUNTIME_PARAMS inout);
	virtual ~CPckClass();

	virtual BOOL	Init(LPCTSTR szFile);

	virtual CONST	LPPCKINDEXTABLE		GetPckIndexTable();
	virtual CONST	LPPCK_PATH_NODE		GetPckPathNode();
	CONST	LPPCKHEAD			GetPckHead();

	//设置版本
	void	SetPckVersion(int verID);

	//获取文件数
	DWORD	GetPckFileCount();

	//数据区大小
	PCKADDR	GetPckDataAreaSize();

	//数据区冗余数据大小
	PCKADDR	GetPckRedundancyDataSize();
	
	//解压文件
	BOOL	ExtractFiles(LPPCKINDEXTABLE *lpIndexToExtract, int nFileCount);
	BOOL	ExtractFiles(LPPCK_PATH_NODE *lpNodeToExtract, int nFileCount);
	
	//设置附加信息
	char*	GetAdditionalInfo();
	BOOL	SetAdditionalInfo();

	//新建pck文件
	virtual BOOL	CreatePckFileMT(LPTSTR szPckFile, LPTSTR szPath);

	//重建pck文件
	virtual BOOL	RebuildPckFile(LPTSTR szRebuildPckFile);

	//更新pck文件
	virtual BOOL	UpdatePckFile(LPTSTR szPckFile, TCHAR (*lpszFilePath)[MAX_PATH], int nFileCount, LPPCK_PATH_NODE lpNodeToInsert);

	//重命名文件
	virtual BOOL	RenameFilename();

	//删除一个节点
	virtual VOID	DeleteNode(LPPCK_PATH_NODE lpNode);

	//重命名一个节点
	virtual BOOL	RenameNode(LPPCK_PATH_NODE lpNode, char* lpszReplaceString);
protected:
	virtual BOOL	RenameNodeEnum(LPPCK_PATH_NODE lpNode, size_t lenNodeRes, char* lpszReplaceString, size_t lenrs, size_t lenrp);
	virtual BOOL	RenameNode(LPPCK_PATH_NODE lpNode, size_t lenNodeRes, char* lpszReplaceString, size_t lenrs, size_t lenrp);
public:
	virtual VOID	RenameIndex(LPPCK_PATH_NODE lpNode, char* lpszReplaceString);
	virtual VOID	RenameIndex(LPPCKINDEXTABLE lpIndex, char* lpszReplaceString);

	//预览文件
	virtual BOOL	GetSingleFileData(LPVOID lpvoidFileRead, LPPCKINDEXTABLE lpPckFileIndexTable, char *buffer, size_t sizeOfBuffer = 0);

	//获取node路径
	BOOL	GetCurrentNodeString(char*szCurrentNodePathString, LPPCK_PATH_NODE lpNode);

	//获取基本信息
	BOOL GetPckBasicInfo(LPTSTR lpszFile, PCKHEAD *lpHead, LPBYTE &lpFileIndexData, DWORD &dwPckFileIndexDataSize);
	BOOL SetPckBasicInfo(PCKHEAD *lpHead, LPBYTE lpFileIndexData, DWORD &dwPckFileIndexDataSize);

protected:

	//PckClass.cpp
	BOOL	MountPckFile(LPCTSTR	szFile);

	//PckClassFunction.cpp
	BOOL	OpenPckAndMappingRead(CMapViewFileRead *lpRead, LPCTSTR lpFileName);
	BOOL	OpenPckAndMappingWrite(CMapViewFileWrite *lpWrite, LPCTSTR lpFileName, DWORD dwCreationDisposition, QWORD qdwSizeToMap);


	//PckClassFunction.cpp

	BOOL	AllocIndexTableAndInit(LPPCKINDEXTABLE &lpPckIndexTable, DWORD dwFileCount);

	virtual void	BuildDirTree();
	void* AllocNodes(size_t	sizeStuct);
	VOID	DeAllocMultiNodes(LPPCK_PATH_NODE lpThisNode);

	VOID	MarkSelectedIndex();

	LPFILES_TO_COMPRESS AllocateFileinfo();
	VOID	DeallocateFileinfo();

	BOOL	AddFileToNode(LPPCK_PATH_NODE lpRootNode, LPPCKINDEXTABLE	lpPckIndexNode);
	LPPCK_PATH_NODE	FindFileNode(LPPCK_PATH_NODE lpBaseNode, char* lpszFile);

	BOOL	EnumNode(LPPCK_PATH_NODE lpNodeToExtract, LPVOID lpvoidFileRead, LPVOID lpvoidFileWrite, LPPCKINDEXTABLE_COMPRESS &lpPckIndexTablePtr, PCKADDR &dwAddress/*, DWORD	&dwCount, BOOL &bThreadRunning*/);
	VOID	EnumFile(LPSTR szFilename, BOOL IsPatition, DWORD &dwFileCount, LPFILES_TO_COMPRESS &pFileinfo, QWORD &qwTotalFileSize, size_t nLen);

	//PckClassExtract.cpp

	BOOL	StartExtract(LPPCK_PATH_NODE lpNodeToExtract, LPVOID lpMapAddress);
	BOOL	DecompressFile(char	*lpszFilename, LPPCKINDEXTABLE lpPckFileIndexTable, LPVOID lpvoidFileRead);


	//PckClassThread.cpp

	/////////以下过程用于多线程pck压缩的测试
	static	VOID CompressThread(VOID* pParam);
	static	VOID WriteThread(VOID* pParam);

	//添加模式
	static	VOID CompressThreadAdd(VOID* pParam);
	static	VOID WriteThreadAdd(VOID* pParam);

	//函数
	VOID PreProcess(int threadnum);
	BOOL FillPckHeaderAndInitArray(PCK_ALL_INFOS &PckAllInfo, int threadnum, DWORD dwFileCount);

	void MultiThreadInitialize(VOID CompressThread(VOID*), VOID WriteThread(VOID*), int threadnum);
	_inline BOOL __fastcall WritePckIndex(LPPCKINDEXTABLE_COMPRESS lpPckIndexTablePtr, PCKADDR &dwAddress);
	void AfterProcess(CMapViewFileWrite *lpWrite, PCK_ALL_INFOS &PckAllInfo, DWORD dwFileCount, PCKADDR dwAddressName, PCKADDR &dwAddress);

	///代码测试

	//virtual void test();

	//BOOL	ConfirmErrors(LPCSTR lpszMainString, LPCSTR lpszCaption, UINT uType);
	//BOOL	ConfirmErrors(LPCWSTR lpszMainString, LPCWSTR lpszCaption, UINT uType);
	//void	ConfirmErrors(LPCTSTR lpszCaption = NULL);

	//打印日志
	void PrintLogI(char *_text);
	void PrintLogW(char *_text);
	void PrintLogE(char *_text);
	void PrintLogD(char *_text);


	void PrintLogI(wchar_t *_text);
	void PrintLogW(wchar_t *_text);
	void PrintLogE(wchar_t *_text);
	void PrintLogD(wchar_t *_text);


	void PrintLogE(char *_maintext, char *_file, char *_func, long _line);
	void PrintLogE(wchar_t *_maintext, char *_file, char *_func, long _line);
	void PrintLogE(char *_fmt, char *_maintext, char *_file, char *_func, long _line);
	void PrintLogE(char *_fmt, wchar_t *_maintext, char *_file, char *_func, long _line);

	void PrintLog(char chLevel, char *_maintext);
	void PrintLog(char chLevel, wchar_t *_maintext);
	void PrintLog(char chLevel, char *_fmt, char *_maintext);
	void PrintLog(char chLevel, char *_fmt, wchar_t *_maintext);

//变量
public:

	//TCHAR	m_lastErrorString[1024];

protected:

	LPPCK_RUNTIME_PARAMS	lpPckParams;

	BOOL			m_ReadCompleted;
	PCK_ALL_INFOS	m_PckAllInfo;
	LPPCKINDEXTABLE	m_lpPckIndexTable;
	PCK_PATH_NODE	m_RootNode;

	LPFILES_TO_COMPRESS		m_firstFile;
	
	char			m_szEventAllWriteFinish[16];
	char			m_szEventAllCompressFinish[16];
	char			m_szEventMaxMemory[16];

	char			m_szMapNameRead[16];
	char			m_szMapNameWrite[16];

	//PCK文件版本。。
	LPPCK_KEYS		m_lpThisPckKey;

};

#endif