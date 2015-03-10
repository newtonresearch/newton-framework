/*
	File:		QDCompression.h

	Contains:	QuickDraw PixelMap compression.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__QDCOMPRESSION_H)
#define __QDCOMPRESSION_H 1

#include "StoreCompander.h"
#include "QDTypes.h"


struct PixMapObj
{
	size_t	size;
	PixelMap	pixMap;
	int32_t	x20;
	int32_t	x24;
	int32_t	x28;
};


/*------------------------------------------------------------------------------
	C P i x e l M a p C o m p a n d e r
------------------------------------------------------------------------------*/

PROTOCOL CPixelMapCompander : public CStoreCompander
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CPixelMapCompander)
	CAPABILITIES( "CLZDecompressor" ""
					  "CLZCompressor" "" )

	CPixelMapCompander *	make(void);
	void			destroy(void);

	NewtonErr	init(CStore * inStore, PSSId inRootId, PSSId inParmsId, bool inShared, bool inArg5);
	size_t		blockSize(void);
	NewtonErr	read(size_t inOffset, char * outBuf, size_t inBufLen, VAddr inArg4);
	NewtonErr	write(size_t inOffset, char * inBuf, size_t inBufLen, VAddr inArg4);
	NewtonErr	doTransactionAgainst(int, ULong);
	bool			isReadOnly(void);

private:
	void			disposeAllocations(void);

//												// +00	CProtocol fields
	CStore *				fStore;			// +10
	PSSId					fRootId;			// +14
	PSSId					fChunksId;		// +18
	bool					fShared;			// +1C
	CDecompressor *	fDecompressor;	// +20
	CCompressor *		fCompressor;	// +24
	char *				fBuffer;			// +28
	size_t				fBufSize;		// +2C
	PSSId					fPixMapId;		// +30
	PixMapObj *			fPixMapObj;		// +34
	int32_t				f38;				// +38
	int32_t				f3C;				// +3C
	int32_t				f40;				// +40
	int					fRowLongs;		// +44
	bool					fIsAllocated;	// +48
};

#endif	/* __QDCOMPRESSION_H */