/*
	File:		PackageStore.h

	Contains:	Newton package store interface.
					The package store contains read-only data
					and is also known as a store part.

	Written by:	Newton Research Group.
*/

#if !defined(__PACKAGESTORE_H)
#define __PACKAGESTORE_H 1

#include "StoreWrapper.h"


/* -----------------------------------------------------------------------------
	P a c k a g e S t o r e D a t a
	A package store is a block of read-only data loaded from a package.
	The data is prefixed with a PackageStoreData struct that defines the objects
	within the store. An object’s id is an index into an array that gives the
	offset to the object within the package data block.
----------------------------------------------------------------------------- */

struct PackageStoreData
{
	PSSId		rootId;
	ULong		numOfObjects;
	ULong		offsetToObject[];
};


/* -----------------------------------------------------------------------------
	C P a c k a g e S t o r e
	A CStore protocol implementation.
----------------------------------------------------------------------------- */

PROTOCOL CPackageStore : public CStore
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CPackageStore)
	CAPABILITIES( "LOBJ" "" )

	CPackageStore *	make(void);
	void			destroy(void);

	NewtonErr	init(void * inStoreData, size_t inStoreSize, ULong inArg3, ArrayIndex inSocketNumber, ULong inFlags, void * inArg6);
	NewtonErr	needsFormat(bool * outNeedsFormat);
	NewtonErr	format(void);

	NewtonErr	getRootId(PSSId * outRootId);
	NewtonErr	newObject(PSSId * outObjectId, size_t inSize);
	NewtonErr	eraseObject(PSSId inObjectId);
	NewtonErr	deleteObject(PSSId inObjectId);
	NewtonErr	setObjectSize(PSSId inObjectId, size_t inSize);
	NewtonErr	getObjectSize(PSSId inObjectId, size_t * outSize);

	NewtonErr	write(PSSId inObjectId, size_t inStartOffset, void * inBuffer, size_t inLength);
	NewtonErr	read(PSSId inObjectId, size_t inStartOffset, void * outBuffer, size_t inLength);

	NewtonErr	getStoreSize(size_t * outTotalSize, size_t * outUsedSize);
	NewtonErr	isReadOnly(bool * outIsReadOnly);
	NewtonErr	lockStore(void);
	NewtonErr	unlockStore(void);

	NewtonErr	abort(void);
	NewtonErr	idle(bool * outArg1, bool * outArg2);

	NewtonErr	nextObject(PSSId inObjectId, PSSId * outNextObjectId);
	NewtonErr	checkIntegrity(ULong * inArg1);

	NewtonErr	setBuddy(CStore * inStore);
	bool			ownsObject(PSSId inObjectId);
	VAddr			address(PSSId inObjectId);
	const char * storeKind(void);
	NewtonErr	setStore(CStore * inStore, ObjectId inEnvironment);

	bool			isSameStore(void * inData, size_t inSize);
	bool			isLocked(void);
	bool			isROM(void);

	NewtonErr	vppOff(void);
	NewtonErr	sleep(void);

	NewtonErr	newWithinTransaction(PSSId * outObjectId, size_t inSize);
	NewtonErr	startTransactionAgainst(PSSId inObjectId);
	NewtonErr	separatelyAbort(PSSId inObjectId);
	NewtonErr	addToCurrentTransaction(PSSId inObjectId);
	bool			inSeparateTransaction(PSSId inObjectId);

	NewtonErr	lockReadOnly(void);
	NewtonErr	unlockReadOnly(bool inReset);
	bool			inTransaction(void);

	NewtonErr	newObject(PSSId * outObjectId, void * inData, size_t inSize);
	NewtonErr	replaceObject(PSSId inObjectId, void * inData, size_t inSize);

	NewtonErr	calcXIPObjectSize(long inArg1, long inArg2, long * outArg3);
	NewtonErr	newXIPObject(PSSId * outObjectId, size_t inSize);
	NewtonErr	getXIPObjectInfo(PSSId inObjectId, unsigned long * outArg2, unsigned long * outArg3, unsigned long * outArg4);

private:
	PackageStoreData *	fStoreData;		// +10
	size_t					fStoreSize;		// +14
	int						fLockCounter;	// +18
};

#endif	/* __PACKAGESTORE_H */
