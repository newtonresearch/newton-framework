/*
	File:		ZippyStoreDecompression.h

	Contains:	Zippy store compression declarations.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__ZIPPYSTOREDECOMPRESSION_H)
#define __ZIPPYSTOREDECOMPRESSION_H 1

#include "StoreCompander.h"


/* -----------------------------------------------------------------------------
	Z i p p y H e a d e r
	Each block in the zippy compression scheme is prefixed with a header
	describing its decompressed size and encoding scheme.
----------------------------------------------------------------------------- */

struct ZippyHeader
{
	uint32_t	size;
	uint32_t	flags;
};


/* -----------------------------------------------------------------------------
	C Z i p p y D e c o m p r e s s o r
----------------------------------------------------------------------------- */
#define kZippyCacheSize 16

PROTOCOL CZippyDecompressor : public CDecompressor
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CZippyDecompressor)

	CZippyDecompressor *	make(void);
	void			destroy(void);

	NewtonErr	init(void *);
	NewtonErr	decompress(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen);

private:
	void			initCache(void);
	NewtonErr	decompressChunk(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen);
	size_t		headerSize(void);
	size_t		decompressedLength(void*, size_t);
	bool			expandValue(unsigned char ** ioSrc, ArrayIndex*, unsigned char * inSrcLimit, uint32_t * outValue);
	void			finish(void*, size_t);

	int			fSequence;
	int			fAge[kZippyCacheSize];
	uint32_t		fValue[kZippyCacheSize];
// size +94
};


/* -----------------------------------------------------------------------------
	C Z i p p y S t o r e D e c o m p r e s s o r
----------------------------------------------------------------------------- */

PROTOCOL CZippyStoreDecompressor : public CStoreDecompressor
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CZippyStoreDecompressor)

	CZippyStoreDecompressor *	make(void);
	void			destroy(void);

	NewtonErr	init(CStore * inStore, PSSId inParmsId, char * inLZWBuffer = NULL);	// original has no inLZWBuffer
	NewtonErr	read(PSSId inObjId, char * outBuf, size_t inBufLen, VAddr inBaseAddr);

private:
	char *		fBuffer;
	CZippyDecompressor *	fDecompressor;
	CStore *		fStore;
};


/* -----------------------------------------------------------------------------
	C Z i p p y R e l o c S t o r e D e c o m p r e s s o r
----------------------------------------------------------------------------- */

PROTOCOL CZippyRelocStoreDecompressor : public CStoreDecompressor
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CZippyRelocStoreDecompressor)

	CZippyRelocStoreDecompressor *	make(void);
	void			destroy(void);

	NewtonErr	init(CStore * inStore, PSSId inParmsId, char * inLZWBuffer = NULL);	// original has no inLZWBuffer
	NewtonErr	read(PSSId inObjId, char * outBuf, size_t inBufLen, VAddr inBaseAddr);

private:
	char *		fBuffer;
	CZippyDecompressor *	fDecompressor;
	CStore *		fStore;
};


#endif	/* __ZIPPYSTOREDECOMPRESSION_H */
