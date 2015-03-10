/*
	File:		LargeObjectStore.h

	Contains:	Newton object store interface.

	Written by:	Newton Research Group.
*/

#if !defined(__LRGOBJSTORE_H)
#define __LRGOBJSTORE_H 1

#include "Objects.h"
#include "Store.h"
#include "StoreRootObjects.h"
#include "Compression.h"
#include "Pipes.h"


/*------------------------------------------------------------------------------
	L a r g e   O b j e c t   C a l l b a c k
------------------------------------------------------------------------------*/

struct LOCallbackInfo
{
	size_t		pkgSize;			// package size
	size_t		sizeDone;
	UniChar *	pkgName;			// package name
	size_t		partNumber;
	size_t		numOfParts;		// number of package parts
};


class CLOCallback
{
public:
	void		callback(LOCallbackInfo * info);

	void		setFunc(RefArg inFunc);
	void		setChunk(size_t inSize);
	void		setInfo(RefArg info);

	size_t	frequency(void)	const;

private:
//	int32_t		f00;
	RefStruct	fCallback;	// codeblock to call
	RefStruct	fInfo;
	size_t		fFreq;		// call back after this size has been done
};

inline	void		CLOCallback::setFunc(RefArg inFunc)		{ fCallback = inFunc; }
inline	void		CLOCallback::setChunk(size_t inSize)	{ fFreq = inSize; }
inline	void		CLOCallback::setInfo(RefArg info)		{ fInfo = info; }

inline	size_t	CLOCallback::frequency(void)	const		{ return fFreq; }


/*------------------------------------------------------------------------------
	C L r g O b j S t o r e   P r o t o c o l
	P-class interface.
------------------------------------------------------------------------------*/

PROTOCOL CLrgObjStore : public CProtocol
{
public:
	static CLrgObjStore *	make(const char * inName);
	void			destroy(void);

	NewtonErr	init(void);
	NewtonErr	create(PSSId * outId, CStore * inStore, CPipe * inPipe, size_t inStreamSize, bool inReadOnly, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CLOCallback * inCallback);
	NewtonErr	createFromCompressed(PSSId * outId, CStore * inStore, CPipe * inPipe, size_t inStreamSize, bool inReadOnly, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CLOCallback * inCallback);
	NewtonErr	deleteObject(CStore * inStore, PSSId inId);
	NewtonErr	duplicate(PSSId * outId, CStore * inStore, PSSId inId, CStore * intoStore);
	NewtonErr	resize(CStore * inStore, PSSId inId, size_t inSize);
	size_t		storageSize(CStore * inStore, PSSId inId);
	size_t		sizeOfStream(CStore * inStore, PSSId inId, bool inCompressed);
	NewtonErr	backup(CPipe * inPipe, CStore * inStore, PSSId inId, bool inCompressed, CLOCallback * inCallback);
};


/*------------------------------------------------------------------------------
	C L O P a c k a g e S t o r e
------------------------------------------------------------------------------*/

PROTOCOL CLOPackageStore : public CLrgObjStore
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CLOPackageStore)
	CAPABILITIES( "CZippyRelocStoreDecompressor" "" "CZippyStoreDecompressor" ""
					  "CLZRelocStoreDecompressor" "" "CLZStoreDecompressor" ""
					  "CSimpleRelocStoreDecompressor" "" "CSimpleStoreDecompressor" "" )

	CLOPackageStore *	make(void);
	void			destroy(void);

	NewtonErr	init(void);
	NewtonErr	create(PSSId * outId, CStore * inStore, CPipe * inPipe, size_t inStreamSize, bool inReadOnly, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CLOCallback * inCallback);
	NewtonErr	createFromCompressed(PSSId * outId, CStore * inStore, CPipe * inPipe, size_t inStreamSize, bool inReadOnly, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CLOCallback * inCallback);
	NewtonErr	deleteObject(CStore * inStore, PSSId inId);
	NewtonErr	duplicate(PSSId * outId, CStore * inStore, PSSId inId, CStore * intoStore);
	NewtonErr	resize(CStore * inStore, PSSId inId, size_t inSize);
	size_t		storageSize(CStore * inStore, PSSId inId);
	size_t		sizeOfStream(CStore * inStore, PSSId inId, bool inCompressed);
	NewtonErr	backup(CPipe * inPipe, CStore * inStore, PSSId inId, bool inCompressed, CLOCallback * inCallback);
};

// also CXIPPackageStore


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

NewtonErr	LODefaultCreate(PSSId * outId, CStore * inStore, CPipe * inPipe, size_t inStreamSize, bool inReadOnly, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CLOCallback * inCallback);
NewtonErr	LODefaultCreateFromCompressed(PSSId * outId, CStore * inStore, CPipe * inPipe, size_t inStreamSize, bool inReadOnly, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CLOCallback * inCallback);
NewtonErr	LODefaultDelete(CStore * inStore, PSSId inId);
NewtonErr	LODefaultDuplicate(PSSId * outId, CStore * inStore, PSSId inId, CStore * intoStore);
size_t		LODefaultStorageSize(CStore * inStore, PSSId inId);
size_t		LODefaultStreamSize(CStore * inStore, PSSId inId, bool inCompressed);
NewtonErr	LODefaultBackup(CPipe*, CStore * inStore, PSSId inId, bool inCompressed, CLOCallback * inCallback);
NewtonErr	LODefaultDoTransaction(CStore * inStore, PSSId inRootId, PSSId inChunksId, int, bool);

NewtonErr	GetLOAllocator(CStore * inStore, PSSId inId, CLrgObjStore ** outObjStore);

NewtonErr	LOCompanderNameStrLen(CStore * inStore, PSSId inId, size_t * outSize);
NewtonErr	LOCompanderName(CStore * inStore, PSSId inId, char * outName);
NewtonErr	LOCompanderParameterSize(CStore * inStore, PSSId inId, size_t * outSize);
NewtonErr	LOCompanderParameters(CStore * inStore, PSSId inId, void * outParms);
size_t		LOSizeOfStream(CStore * inStore, PSSId inId, bool inCompressed);
NewtonErr	LOWrite(CPipe * inPipe, CStore * inStore, PSSId inId, bool inCompressed, CLOCallback * inCallback);
NewtonErr	LODeleteByProtocol(CStore * inStore, PSSId inId);

NewtonErr	RemoveIndexTable(CStore * inStore, PSSId inId);


#endif	/* __LRGOBJSTORE_H */
