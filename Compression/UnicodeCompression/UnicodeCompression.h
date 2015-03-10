/*
	File:		UnicodeCompression.h

	Contains:	Unicode text compressor/decompressor interface.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__UNICODECOMPRESSION_H)
#define __UNICODECOMPRESSION_H 1

#include "Compression.h"

enum CompressionType
{
	kNoCompression,
	kCompression,
	kUnicodeCompression
};

typedef NewtonErr (*WriteProcPtr)(VAddr, void*, size_t, bool);
typedef NewtonErr (*ReadProcPtr)(VAddr, void*, size_t*, bool*);

CCallbackCompressor *	NewCompressor(CompressionType inCompression, WriteProcPtr inWriter, VAddr instance);
CCallbackDecompressor *	NewDecompressor(CompressionType inCompression, ReadProcPtr inReader, VAddr instance);

#endif	/* __UNICODECOMPRESSION_H */
