/*
	File:		LZStoreCompander.h

	Contains:	Store compression protocol.

	Written by:	Newton Research Group.
*/

#if !defined(__LZSTORECOMPANDER_H)
#define __LZSTORECOMPANDER_H 1

#include "StoreCompander.h"
#include "LZDecompressor.h"


/*------------------------------------------------------------------------------
	C L Z S t o r e D e c o m p r e s s o r
------------------------------------------------------------------------------*/

PROTOCOL CLZStoreDecompressor : public CStoreDecompressor
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CLZStoreDecompressor)

	CLZStoreDecompressor *	make(void);
	void			destroy(void);

	NewtonErr		init(CStore * inStore, PSSId inParmsId, char * inLZWBuffer = NULL);	// original has no inLZWBuffer
	NewtonErr		read(PSSId inObjId, char * outBuf, size_t inBufLen, VAddr inBaseAddr);

private:
	char *				fBuffer;
	CLZDecompressor *	fDecompressor;
	CStore *				fStore;
};


/*------------------------------------------------------------------------------
	C L Z R e l o c S t o r e D e c o m p r e s s o r
------------------------------------------------------------------------------*/

PROTOCOL CLZRelocStoreDecompressor : public CStoreDecompressor
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CLZRelocStoreDecompressor)

	CLZRelocStoreDecompressor *	make(void);
	void			destroy(void);

	NewtonErr		init(CStore * inStore, PSSId inParmsId, char * inLZWBuffer = NULL);	// original has no inLZWBuffer
	NewtonErr		read(PSSId inObjId, char * outBuf, size_t inBufLen, VAddr inBaseAddr);

private:
	char *				fBuffer;
	CLZDecompressor *	fDecompressor;
	CStore *				fStore;
};


/*------------------------------------------------------------------------------
	C L Z S t o r e C o m p a n d e r
------------------------------------------------------------------------------*/

PROTOCOL CLZStoreCompander : public CStoreCompander
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CLZStoreCompander)

	CLZStoreCompander *	make(void);
	void			destroy(void);

	NewtonErr	init(CStore * inStore, PSSId inRootId, PSSId inParmsId, bool inShared, bool inArg5);
	size_t		blockSize(void);
	NewtonErr	read(size_t inOffset, char * outBuf, size_t inBufLen, VAddr inBaseAddr);
	NewtonErr	write(size_t inOffset, char * inBuf, size_t inBufLen, VAddr inBaseAddr);
	NewtonErr	doTransactionAgainst(int, ULong);
	bool			isReadOnly(void);

private:
//												// +00	CProtocol fields
	char *				fBuffer;			// +10
	CDecompressor *	fDecompressor;	// +14
	CCompressor *		fCompressor;	// +18
	CStore *				fStore;			// +1C
	PSSId					fRootId;			// +20
	PSSId					fChunksId;		// +24
	bool					fIsAllocated;	// +28
};

#endif	/* __LZSTORECOMPANDER_H */
