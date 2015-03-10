/*
	File:		SimpleStoreCompander.h

	Contains:	Simple store compression protocol.
					Simple => actually no compression at all.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__SIMPLESTORECOMPANDER_H)
#define __SIMPLESTORECOMPANDER_H 1

#include "StoreCompander.h"

/*------------------------------------------------------------------------------
	C S i m p l e S t o r e D e c o m p r e s s o r
------------------------------------------------------------------------------*/

PROTOCOL CSimpleStoreDecompressor : public CStoreDecompressor
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CSimpleStoreDecompressor)

	CSimpleStoreDecompressor *	make(void);
	void			destroy(void);

	NewtonErr		init(CStore * inStore, PSSId inParmsId, char * inLZWBuffer = NULL);	// original has no inLZWBuffer
	NewtonErr		read(PSSId inObjId, char * outBuf, size_t inBufLen, VAddr inBaseAddr);

private:
	char *				f10;
	CStore *				fStore;
};


/*------------------------------------------------------------------------------
	C S i m p l e R e l o c S t o r e D e c o m p r e s s o r
------------------------------------------------------------------------------*/

PROTOCOL CSimpleRelocStoreDecompressor : public CStoreDecompressor
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CSimpleRelocStoreDecompressor)

	CSimpleRelocStoreDecompressor *	make(void);
	void			destroy(void);

	NewtonErr		init(CStore * inStore, PSSId inParmsId, char * inLZWBuffer = NULL);	// original has no inLZWBuffer
	NewtonErr		read(PSSId inObjId, char * outBuf, size_t inBufLen, VAddr inBaseAddr);

private:
	CStore *				fStore;
};


/*------------------------------------------------------------------------------
	C S i m p l e S t o r e C o m p a n d e r
------------------------------------------------------------------------------*/

PROTOCOL CSimpleStoreCompander : public CStoreCompander
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CSimpleStoreCompander)

	CSimpleStoreCompander *	make(void);		// constructor
	void			destroy(void);				// destructor

	NewtonErr	init(CStore * inStore, PSSId inRootId, PSSId inParmsId, bool inShared, bool inArg5);
	size_t		blockSize(void);
	NewtonErr	read(size_t inOffset, char * outBuf, size_t inBufLen, VAddr inBaseAddr);
	NewtonErr	write(size_t inOffset, char * inBuf, size_t inBufLen, VAddr inBaseAddr);
	NewtonErr	doTransactionAgainst(int, ULong);
	bool			isReadOnly(void);

private:
//												// +00	CProtocol fields
	CStore *				fStore;			// +10
	PSSId					fRootId;			// +14
	PSSId					fChunksId;		// +18
	bool					fIsReadOnly;	// +1C
};


#endif	/* __SIMPLESTORECOMPANDER_H */
