/*
	File:		LZCompressionData-j.h

	Contains:	LZ compression data.
	
	Written by:	Newton Research Group, 2006.
					With a little help from Philz, and a disassembly of the DILs.
*/

#if !defined(__LZCOMPRESSIONDATA_H)
#define __LZCOMPRESSIONDATA_H

extern unsigned int const O10[3];
extern unsigned int const O9[4];
extern unsigned int const O8[5];
extern unsigned int const O7[6];
extern unsigned int const O6[7];
extern unsigned int const O5[8];
extern unsigned int const O4[9];
extern unsigned int const O3[10];
extern unsigned int const O2[11];
extern unsigned int const O1[12];

extern unsigned int const CL2[11];
extern unsigned int const CL2Base[11];
extern unsigned int const CL2B[11];
extern unsigned int const CL[6];
extern unsigned int const CLBase[6];
extern unsigned int const CLB[6];
extern unsigned int const LL[5];
extern unsigned int const LLB[5];
extern unsigned int const LLBase[5];

extern unsigned char const CopyValue[256];
extern unsigned char const LZCopyBits[256];

#endif	/* __LZCOMPRESSIONDATA_H */
