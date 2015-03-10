/*
	File:		Compression.cc

	Contains:	Compression initialization.

	Written by:	Newton Research Group.
*/

#include "Newton.h"
#include "SimpleStoreCompander.h"
#include "LZStoreCompander.h"
#include "ZippyStoreDecompression.h"
#include "LargeObjectStore.h"

extern void	InitLZCompression(void);
extern void	InitArithmeticCompression(void);
extern void	InitUnicodeCompression(void);
extern void	InitZippyCompression(void);

extern CDecompressor *	gLZDecompressor;
extern char *				gLZSharedBuffer;


/* -----------------------------------------------------------------------------
	Initialize the compression system. All known compression methods.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
InitializeCompression(void)
{
	InitLZCompression();
	InitArithmeticCompression();
	InitUnicodeCompression();
	InitZippyCompression();
}


/* -----------------------------------------------------------------------------
	Initialize the store decompression system.
	Args:		--
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
InitializeStoreDecompressors(void)
{
	// store decompressors
	CSimpleStoreDecompressor::classInfo()->registerProtocol();
	CLZStoreDecompressor::classInfo()->registerProtocol();
	CZippyStoreDecompressor::classInfo()->registerProtocol();

	// store decompressors with relocation
	CSimpleRelocStoreDecompressor::classInfo()->registerProtocol();
	CLZRelocStoreDecompressor::classInfo()->registerProtocol();
	CZippyRelocStoreDecompressor::classInfo()->registerProtocol();

	// store companders
	CSimpleStoreCompander::classInfo()->registerProtocol();
	CStoreCompanderWrapper::classInfo()->registerProtocol();
	CLZStoreCompander::classInfo()->registerProtocol();

	// package store
	CLOPackageStore::classInfo()->registerProtocol();

#if 0
	// XIP store compander
// we don’t have these yet
	CXIPPackageStore::classInfo()->registerProtocol();
	CXIPStoreCompander::classInfo()->registerProtocol();
#endif

	gLZSharedBuffer = new char[0x0520];
	gLZDecompressor = (CDecompressor *)MakeByName("CDecompressor", "CLZDecompressor");
	return gLZDecompressor->init(NULL);
}

