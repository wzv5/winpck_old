/*
SIMD class to detect presence of SIMD version 1.4.0

Copyright (c) 2010 Wong Shao Voon
1.1.0 - Contains the 3DNow, 3DNow+ and MMX+ detection bug fix by Leonardo Tazzini
1.2.0 - Added AMD's SSE4a and SSE5
1.3.0 - Use 2 unsigned shorts to store SIMD info, instead of 1 boolean for each SIMD type.
1.4.0 - Added the detection of Intel AES instructions

The Code Project Open License (CPOL)
http://www.codeproject.com/info/cpol10.aspx
*/


// Get SIMD information from CPU.

#ifndef __SIMD_H__
#define __SIMD_H__

class SIMD
{
public:
	SIMD();
// Intel SIMDs
	bool HasMMX();
	bool HasSSE();
	bool HasSSE2();
	bool HasSSE3();
	bool HasSSSE3();
	bool HasSSE41();
	bool HasSSE42();
	bool HasAES();
	bool HasAVX();
	bool HasHyperThreading();
// AMD SIMDs
	bool HasMMXplus();
	bool Has3Dnow();
	bool Has3DnowExt();
	bool HasSSE4a();
	bool HasSSE5();

private:
	void InitParams();
	unsigned short m_nIntelSIMDs;
	unsigned short m_nAMD_SIMDs;
};

#endif // __SIMD_H__