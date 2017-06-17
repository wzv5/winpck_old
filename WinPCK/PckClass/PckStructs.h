//////////////////////////////////////////////////////////////////////
// PckStructs.h: 用于解析完美世界公司的pck文件中的数据，并显示在List中
// 头文件
//
// 此程序由 李秋枫/stsm/liqf 编写
//
// 此代码预计将会开源，任何基于此代码的修改发布请保留原作者信息
// 
// 2015.5.13
//////////////////////////////////////////////////////////////////////

#include "PckConf.h"

#if !defined(_PCKSTRUCTS_H_)
#define _PCKSTRUCTS_H_


typedef enum _FMTPCK
{
	FMTPCK_PCK					= 0,
	FMTPCK_ZUP					= 1,
	FMTPCK_CUP					= 2,
	FMTPCK_UNKNOWN				= 0x7fffffff
}FMTPCK;

typedef struct _PCK_KEYS
{
	char	name[64];
	TCHAR	tname[64];
	int		id;
	//unsigned long	PCK_VERSION;
	DWORD	Version;

	//unsigned long	PCKHEAD_VERIFY_HEAD;
	DWORD	HeadVerifyKey1;

	//unsigned long	PCKHEAD_VERIFY_TAIL;
	DWORD	HeadVerifyKey2;

	//unsigned long	FILEINDEX_VERIFY_HEAD;
	DWORD	TailVerifyKey1;

	//unsigned long	FILEINDEX_ADDR_CONST;
	PCKADDR	IndexesEntryAddressCryptKey;

	//unsigned long	FILEINDEX_VERIFY_TAIL;
	DWORD	TailVerifyKey2;

	//BYTE	FILEINDEX_LEVEL0;
	//unsigned long	INDEXTABLE_OR_VALUE_HEAD;
	DWORD	IndexCompressedFilenameDataLengthCryptKey1;

	//unsigned long	INDEXTABLE_OR_VALUE_TAIL;
	DWORD	IndexCompressedFilenameDataLengthCryptKey2;
	//DWORD	INDEXTABLE_OR_VALUE_HEAD_LEVEL0;
	//DWORD	INDEXTABLE_OR_VALUE_TAIL_LEVEL0;

}PCK_KEYS, *LPPCK_KEYS;

//typedef struct _PCKVERSION_VALUE_OFFSET_OF_INDEX
//{
//	int AddressOffset;
//	int UnCompressedDataSize;
//	int CompressedDataSize;
//	int Unknown1;
//	int Unknown2;
//}PCKVER, *LPPCKVER;


//****** structures ******* 
#pragma pack(push, 4)

typedef struct _PCK_HEAD {
	DWORD		dwHeadCheckHead;
#if defined PCKV202 || defined PCKV203ZX
	DWORD		dwPckSize;
	DWORD		dwHeadCheckTail;
#elif defined PCKV203
	QWORD		dwPckSize;
#endif
}PCKHEAD, *LPPCKHEAD;

typedef struct _PCK_TAIL {
	DWORD		dwFileCount;
	DWORD		dwVersion;
}PCKTAIL, *LPPCKTAIL;

#ifdef PCKV202

typedef struct _PCK_INDEX_ADDR {
	DWORD		dwIndexTableCheckHead;
	DWORD		dwVersion;
	DWORD		dwCryptDataAddr;
	char		szAdditionalInfo[MAX_PATH_PCK];
	DWORD		dwIndexTableCheckTail;
}PCKINDEXADDR, *LPPCKINDEXADDR;

typedef struct _PCK_FILE_INDEX {
	char		szFilename[MAX_PATH_PCK];
	DWORD		dwUnknown1;
	DWORD		dwAddressOffset;
	DWORD		dwFileClearTextSize;
	DWORD		dwFileCipherTextSize;
	DWORD		dwUnknown2;
}PCKFILEINDEX, *LPPCKFILEINDEX;

#elif defined PCKV203

typedef struct _PCK_INDEX_ADDR {
	DWORD		dwIndexTableCheckHead;
	QWORD		dwCryptDataAddr;
	char		szAdditionalInfo[MAX_PATH_PCK];
	DWORD		dwIndexTableCheckTail;
	//DWORD		dwAddress;
}PCKINDEXADDR, *LPPCKINDEXADDR;

typedef struct _PCK_FILE_INDEX {
	char		szFilename[MAX_PATH_PCK];
	DWORD		dwUnknown1;
	QWORD		dwAddressOffset;
	//DWORD		dwUnknown3;
	DWORD		dwFileClearTextSize;
	DWORD		dwFileCipherTextSize;
	DWORD		dwUnknown2;
}PCKFILEINDEX, *LPPCKFILEINDEX;

#elif defined PCKV203ZX

typedef struct _PCK_INDEX_ADDR {
	DWORD		dwIndexTableCheckHead;
	DWORD		dwVersion;
	DWORD		dwCryptDataAddr;
	DWORD		dwUnknown1;
	char		szAdditionalInfo[MAX_PATH_PCK];
	DWORD		dwIndexTableCheckTail;
	DWORD		dwUnknown2;
}PCKINDEXADDR, *LPPCKINDEXADDR;

typedef struct _PCK_FILE_INDEX {
	char		szFilename[MAX_PATH_PCK];
	DWORD		dwUnknown1;
	DWORD		dwUnknown2;
	DWORD		dwAddressOffset;
	DWORD		dwUnknown3;
	DWORD		dwFileClearTextSize;
	DWORD		dwFileCipherTextSize;
	DWORD		dwUnknown4;
	DWORD		dwUnknown5;
}PCKFILEINDEX, *LPPCKFILEINDEX;

#endif

#pragma pack(pop)

typedef struct _PCK_INDEX_TABLE {
	DWORD			dwIndexValueHead;
	DWORD			dwIndexValueTail;
	PCKFILEINDEX	cFileIndex;
	DWORD			dwIndexDataLength;
	BOOL			bSelected;
}PCKINDEXTABLE, *LPPCKINDEXTABLE;


typedef struct _PCK_PATH_NODE {
	char			szName[MAX_PATH_PCK];
	DWORD			dwFilesCount;
	DWORD			dwDirsCount;
	QWORD			qdwDirClearTextSize;
	QWORD			qdwDirCipherTextSize;
	LPPCKINDEXTABLE	lpPckIndexTable;
	_PCK_PATH_NODE	*parentfirst;
	_PCK_PATH_NODE	*parent;
	_PCK_PATH_NODE	*child;
	_PCK_PATH_NODE	*next;
}PCK_PATH_NODE, *LPPCK_PATH_NODE;


typedef struct _FILES_TO_COMPRESS {
	DWORD			dwCompressedflag;
	DWORD			dwFileSize;
	char			*lpszFileTitle;
	DWORD			nBytesToCopy;
	//char			szBase64Name[MAX_PATH_PCK];
	char			szFilename[MAX_PATH];
	_FILES_TO_COMPRESS	*next;
	_PCK_INDEX_TABLE	*samePtr;
}FILES_TO_COMPRESS, *LPFILES_TO_COMPRESS;


typedef struct _PCK_INDEX_TABLE_COMPRESS {
	DWORD			dwIndexValueHead;
	DWORD			dwIndexValueTail;
	BYTE			buffer[INDEXTABLE_CLEARTEXT_LENGTH];
	DWORD			dwIndexDataLength;
	DWORD			dwCompressedFilesize;
	DWORD			dwMallocSize;
	DWORD			dwAddressOfDuplicateOldDataArea;
	BOOL			bInvalid;
}PCKINDEXTABLE_COMPRESS, *LPPCKINDEXTABLE_COMPRESS;


typedef struct _PCK_ALL_INFOS {

	PCKHEAD			PckHead;
	PCKINDEXADDR	PckIndexAddr;
	PCKTAIL			PckTail;
	PCKADDR			dwAddressName;	//指向文件名列表索引的地址
	TCHAR			szFilename[MAX_PATH];

}PCK_ALL_INFOS, *LPPCK_ALL_INFOS;


#endif