//////////////////////////////////////////////////////////////////////
// PckClassFeedback.cpp: 信息回馈、信息输出、调试等 
//
// 此程序由 李秋枫/stsm/liqf 编写，pck结构参考若水的pck结构.txt，并
// 参考了其易语言代码中并于读pck文件列表的部分
//
// 此代码预计将会开源，任何基于此代码的修改发布请保留原作者信息
// 
// 2015.5.19
//////////////////////////////////////////////////////////////////////

#include "PckClass.h"
#include "PckControlCenter.h"

void CPckClass::PrintLogI(char *_text)
{
	lpPckParams->lpPckControlCenter->PrintLogI(_text);
}

void CPckClass::PrintLogW(char *_text)
{
	lpPckParams->lpPckControlCenter->PrintLogW(_text);
}

void CPckClass::PrintLogE(char *_text)
{
	lpPckParams->lpPckControlCenter->PrintLogE(_text);
}


void CPckClass::PrintLogD(char *_text)
{
	lpPckParams->lpPckControlCenter->PrintLogD(_text);
}


void CPckClass::PrintLogI(wchar_t *_text)
{
	lpPckParams->lpPckControlCenter->PrintLogI(_text);
}

void CPckClass::PrintLogW(wchar_t *_text)
{
	lpPckParams->lpPckControlCenter->PrintLogW(_text);
}


void CPckClass::PrintLogD(wchar_t *_text)
{
	lpPckParams->lpPckControlCenter->PrintLogD(_text);
}


void CPckClass::PrintLogE(wchar_t *_text)
{
	lpPckParams->lpPckControlCenter->PrintLogE(_text);
}

void CPckClass::PrintLogE(char *_maintext, char *_file, char *_func, long _line)
{
	lpPckParams->lpPckControlCenter->PrintLogE(_maintext, _file, _func, _line);
}

void CPckClass::PrintLogE(wchar_t *_maintext, char *_file, char *_func, long _line)
{
	lpPckParams->lpPckControlCenter->PrintLogE(_maintext, _file, _func, _line);
}

void CPckClass::PrintLogE(char *_fmt, char *_maintext, char *_file, char *_func, long _line)
{
	lpPckParams->lpPckControlCenter->PrintLogE(_fmt, _maintext, _file, _func, _line);
}

void CPckClass::PrintLogE(char *_fmt, wchar_t *_maintext, char *_file, char *_func, long _line)
{
	lpPckParams->lpPckControlCenter->PrintLogE(_fmt, _maintext, _file, _func, _line);
}

void CPckClass::PrintLog(char chLevel, char *_maintext)
{
	lpPckParams->lpPckControlCenter->PrintLog(chLevel, _maintext);
}

void CPckClass::PrintLog(char chLevel, wchar_t *_maintext)
{
	lpPckParams->lpPckControlCenter->PrintLog(chLevel, _maintext);
}

void CPckClass::PrintLog(char chLevel, char *_fmt, char *_maintext)
{
	lpPckParams->lpPckControlCenter->PrintLog(chLevel, _fmt, _maintext);
}

void CPckClass::PrintLog(char chLevel, char *_fmt, wchar_t *_maintext)
{
	lpPckParams->lpPckControlCenter->PrintLog(chLevel, _fmt, _maintext);
}


