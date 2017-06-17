//////////////////////////////////////////////////////////////////////
// PckVersion.h: 用于解析完美世界公司的pck文件中的数据，并显示在List中
// 头文件
//
// 此程序由 李秋枫/stsm/liqf 编写，
//
// 此代码预计将会开源，任何基于此代码的修改发布请保留原作者信息
// 
// 2015.5.11
//////////////////////////////////////////////////////////////////////


#include "PckHeader.h"
#include "MapViewFile.h"

#if !defined(_PCKVERSION_H_)
#define _PCKVERSION_H_

class CPckVersion 
{
public:
	//void	CPckVersionInit();
	CPckVersion();
	//CPckVersion(LPPCK_RUNTIME_PARAMS inout);
	virtual ~CPckVersion();
	
	//获得版本
	LPPCK_KEYS findKeyById(LPPCK_ALL_INFOS lpPckAllInfo/*, CMapViewFileRead *lpRead*/);

	LPPCK_KEYS findKeyByFileName(LPTSTR lpszPckFile);

	LPPCK_KEYS GetKey(int verID);

	LPPCK_KEYS getInitialKey();
	LPPCK_KEYS getCurrentKey();

	LPCTSTR		GetSaveDlgFilterString();

	////关于版本差异，不同结构体取值
	//LPPCKVER	GetPckVerion();
	//DWORD	getFileClearTextSize(LPPCKFILEINDEX	lpFileIndex);
	//void	setFileClearTextSize(LPPCKFILEINDEX	lpFileIndex, DWORD val);

	//DWORD	getFileCipherTextSize(LPPCKFILEINDEX lpFileIndex);
	//void	setFileCipherTextSize(LPPCKFILEINDEX lpFileIndex, DWORD val);

protected:

	//计算未知版本
	//BOOL guessUnknowKey(CMapViewFileRead *lpRead, DWORD dwFileCount, LPPCKINDEXADDR lpPckIndexAddr);
	void SetPckVerion();
	//
	//void addPckVersionKeys(

public:

protected:
	//PCK_KEYS cPckKeys[10];

	PCK_KEYS *lp_defaultPckKey;
	TCHAR		szSaveDlgFilterString[1024];

	//PCKVER	cPckVerion;
};

#endif