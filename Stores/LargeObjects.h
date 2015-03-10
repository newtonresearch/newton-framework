/*
	File:		LargeObjects.h

	Contains:	Large (virtual) binary object declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__LARGEOBJECTS_H)
#define __LARGEOBJECTS_H 1

#include "Objects.h"
#include "Stores.h"
#include "LargeObjectStore.h"

/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

// Large Objects
void			InitLargeObjects(void);
NewtonErr	CreateLargeObject(PSSId * outId, CStore * inStore, size_t inSize, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize);
NewtonErr	CreateLargeObject(PSSId * outId, CStore * inStore, CPipe * inPipe, size_t inStreamSize, bool inReadOnly, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CLOCallback * inCallback, bool inCompressed);
NewtonErr	DuplicateLargeObject(PSSId * outId, CStore * inStore, PSSId inId, CStore * intoStore);
NewtonErr	ResizeLargeObject(VAddr * outAddr, VAddr inAddr, size_t inLength, long inArg4);
NewtonErr	MapLargeObject(VAddr * outAddr, CStore * inStore, PSSId inId, bool inReadOnly);
NewtonErr	UnmapLargeObject(CStore ** outStore, PSSId * outId, VAddr inAddr);
NewtonErr	UnmapLargeObject(VAddr inAddr);
NewtonErr	AbortObject(CStore * inStore, PSSId inId);
NewtonErr	AbortObjects(CStore * inStore);
NewtonErr	CommitObject(VAddr inAddr);
NewtonErr	CommitObjects(CStore * inStore);
NewtonErr	FlushLargeObject(CStore * inStore, PSSId inId);
bool			LargeObjectAddressIsValid(VAddr inAddr);
bool			LargeObjectIsDirty(VAddr inAddr);
bool			LargeObjectIsReadOnly(VAddr inAddr);
size_t		StorageSizeOfLargeObject(VAddr inAddr);
size_t		StorageSizeOfLargeObject(CStore * inStore, PSSId inId);
Ref			WrapLargeObject(CStore * inStore, PSSId inId, RefArg inClass, VAddr inAddr);
Ref			GetEntryFromLargeObjectVAddr(VAddr inAddr);
NewtonErr	DeleteLargeObject(VAddr inAddr);
NewtonErr	DeleteLargeObject(CStore * inStore, PSSId inId);

// Packages
NewtonErr	NewPackage(CPipe * inPipe, CStore * inStore, PSSId inId, ULong * outArg4, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CCallbackCompressor * inCompressor);
NewtonErr	AllocatePackage(CPipe * inPipe, CStore * inStore, PSSId inId, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CCallbackCompressor * inCompressor, CLOCallback * inCallback = NULL);
bool			PackageAllocationOK(CStore * inStore, PSSId inId);;
NewtonErr	PackageAvailable(CStore * inStore, PSSId inId, ULong*, bool*, bool*);
NewtonErr	PackageAvailable(CStore * inStore, PSSId inId, ULong*);
NewtonErr	PackageUnavailable(PSSId inPkgId);
NewtonErr	BackupPackage(CPipe * inPipe, PSSId inPkgId);
NewtonErr	BackupPackage(CPipe * inPipe, CStore * inStore, PSSId inId, CLOCallback * inCallback);
NewtonErr	DeletePackage(PSSId inPkgId);
NewtonErr	DeallocatePackage(CStore * inStore, PSSId inPkgId);

Ref			WrapPackage(CStore * inStore, PSSId inId);
NewtonErr	StorePackage(CPipe * inPipe, CStore * inStore, CLOCallback * inCallback, PSSId * outId);


// Lookup
NewtonErr	VAddrToStore(CStore ** outStore, PSSId * outId, VAddr inAddr);
NewtonErr	StoreToVAddr(VAddr * outAddr, CStore * inStore, PSSId inId);
NewtonErr	IdToVAddr(PSSId inId, VAddr * outAddr);
NewtonErr	IdToStore(PSSId inPkgId, CStore ** outStoreObj, PSSId * outStoreId);
NewtonErr	StoreToId(CStore * inStore, PSSId inId, PSSId * outId);

// Info
size_t		ObjectSize(VAddr inAddr);
bool			IsOnStoreAsPackage(VAddr inAddr);
bool			IsOnStoreAsPackage(CStore * inStore, PSSId inId);

#endif	/* __LARGEOBJECTS_H */
