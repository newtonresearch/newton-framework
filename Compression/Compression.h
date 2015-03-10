/*
	File:		Compression.h

	Contains:	Compression declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__COMPRESSION_H)
#define __COMPRESSION_H 1

#include "Protocols.h"

/*------------------------------------------------------------------------------
	C C o m p r e s s o r
	P-class interface.
------------------------------------------------------------------------------*/

PROTOCOL CCompressor : public CProtocol
{
public:
	static CCompressor *	make(const char * inName);
	void			destroy(void);

	NewtonErr	init(void *);
	NewtonErr	compress(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen);
	size_t		estimatedCompressedSize(void * inSrcBuf, size_t inSrcLen);
};


/*------------------------------------------------------------------------------
	C C a l l b a c k C o m p r e s s o r
	P-class interface.
------------------------------------------------------------------------------*/

PROTOCOL CCallbackCompressor : public CProtocol
{
public:
	static CCallbackCompressor *	make(const char * inName);
	void			destroy(void);

	NewtonErr	init(void *);
	void			reset(void);
	NewtonErr	writeChunk(void * inSrcBuf, size_t inSrcLen);
	NewtonErr	flush(void);
};


/*------------------------------------------------------------------------------
	C D e c o m p r e s s o r
	P-class interface.
------------------------------------------------------------------------------*/

PROTOCOL CDecompressor : public CProtocol
{
public:
	static CDecompressor *	make(const char * inName);
	void			destroy(void);

	NewtonErr	init(void *);
	NewtonErr	decompress(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen);
};


/*------------------------------------------------------------------------------
	C C a l l b a c k D e c o m p r e s s o r
	P-class interface.
------------------------------------------------------------------------------*/

PROTOCOL CCallbackDecompressor : public CProtocol
{
public:
	static CCallbackDecompressor *	make(const char * inName);
	void			destroy(void);

	NewtonErr	init(void *);
	void			reset(void);
	NewtonErr	readChunk(void * inDstBuf, size_t * outBufLen, bool * outUnderflow);
};


/*------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
------------------------------------------------------------------------------*/

void			InitializeCompression(void);
NewtonErr	InitializeStoreDecompressors(void);
NewtonErr	GetSharedLZObjects(CCompressor ** outCompressor, CDecompressor ** outDecompressor, char ** outBuffer, size_t * outBufLen);
void			ReleaseSharedLZObjects(CCompressor * inCompressor, CDecompressor * inDecompressor, char * inBuffer);

#endif	/* __COMPRESSION_H */
