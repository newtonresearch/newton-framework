/*
	File:		StorePackages.h

	Contains:	Newton object store packages interface.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__STOREPACKAGES_H)
#define __STOREPACKAGES_H 1

#include "Compression.h"

/*------------------------------------------------------------------------------
	C S t o r e P a c k a g e W r i t e r
------------------------------------------------------------------------------*/

class CRelocationGenerator;
class CFrameRelocationGenerator;
struct RelocationHeader;
struct RelocationEntry;

class CStorePackageWriter
{
public:
					CStorePackageWriter();
	virtual		~CStorePackageWriter();

	void			init(CStore * inStore, PSSId inObjectId, size_t inSize, CCallbackCompressor * inCompressor, RelocationHeader * ioRelocHeader, RelocationEntry * ioRelocEntry);
	NewtonErr	writeChunk(char * inBuf, size_t inSize, bool);
	void			writeCompressedData(void * inBuf, size_t inSize);
	void			flush(void);
	void			abort(void);

private:
	CCallbackCompressor *	fCompressor;	// +04
	void *			f08;
	long				f0C;
	long				f10;
	long				f14;
	CStore *			fStore;			// +18
	PSSId				fObjectId;		// +1C
	ULong				f20;
	CRelocationGenerator *			f28;
	CFrameRelocationGenerator *	f2C;
};


#endif	/* __STOREPACKAGES_H */
