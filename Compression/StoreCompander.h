/*
	File:		StoreCompander.h

	Contains:	Store compression protocol.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__STORECOMPANDER_H)
#define __STORECOMPANDER_H 1

#include "Newton.h"
#include "Store.h"
#include "StoreRootObjects.h"
#include "Compression.h"
#include "Relocation.h"


/*------------------------------------------------------------------------------
	C S t o r e D e c o m p r e s s o r
	P-class interface.
------------------------------------------------------------------------------*/

PROTOCOL CStoreDecompressor : public CProtocol
{
public:
	static CStoreDecompressor *	make(const char * inName);
	void			destroy(void);

	NewtonErr	init(CStore * inStore, PSSId inParmsId, char * inLZWBuffer = NULL);	// original has no inLZWBuffer
	NewtonErr	read(PSSId inObjId, char * outBuf, size_t inBufLen, VAddr inBaseAddr);
};


/*------------------------------------------------------------------------------
	C S t o r e C o m p a n d e r
	P-class interface.
------------------------------------------------------------------------------*/

PROTOCOL CStoreCompander : public CProtocol
{
public:
	static CStoreCompander *	make(const char * inName);
	void			destroy(void);

	NewtonErr	init(CStore * inStore, PSSId inRootId, PSSId inParmsId, bool inShared, bool inArg5);
	size_t		blockSize(void);
	NewtonErr	read(size_t inOffset, char * outBuf, size_t inBufLen, VAddr inBaseAddr);
	NewtonErr	write(size_t inOffset, char * inBuf, size_t inBufLen, VAddr inBaseAddr);
	NewtonErr	doTransactionAgainst(int, ULong);
	bool			isReadOnly(void);
};


PROTOCOL CStoreCompanderWrapper : public CStoreCompander
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CStoreCompanderWrapper)

	CStoreCompanderWrapper *	make(void);
	void			destroy(void);

	NewtonErr	init(CStore * inStore, char * inCompanderName, PSSId inRootId, PSSId inParmsId, char * inLZWBuffer = NULL);	// original has no inLZWBuffer
	NewtonErr	init(CStore * inStore, PSSId inRootId, PSSId inParmsId, bool inShared, bool inArg5);
	size_t		blockSize(void);
	NewtonErr	read(size_t inOffset, char * outBuf, size_t inBufLen, VAddr inBaseAddr);
	NewtonErr	write(size_t inOffset, char * inBuf, size_t inBufLen, VAddr inBaseAddr);
	NewtonErr	doTransactionAgainst(int, ULong);
	bool			isReadOnly(void);

private:
//										// +00	CProtocol fields
	CStore *		fStore;			// +10
	PSSId			fRootId;			// +14
	PSSId			fChunksId;		// +18
	CStoreDecompressor *	fDecompressor;	// +1C
	char *		fCompanderName;	// +20
};


#endif	/* __STORECOMPANDER_H */
