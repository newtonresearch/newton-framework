/*
	File:		CStore.h

	Contains:	Newton object store interface.

	Written by:	Newton Research Group.
*/

#if !defined(__STORE_H)
#define __STORE_H 1

#include "Protocols.h"
#include "PSSTypes.h"

/*------------------------------------------------------------------------------
	C S t o r e
	P-class interface.
------------------------------------------------------------------------------*/

PROTOCOL CStore : public CProtocol
{
// Implementations will need to consider:
//	PROTOCOLVERSION(1.0)
//	PROTOCOL_IMPL_HEADER_MACRO(CStore);

//	CAPABILITIES( "LOBJ" )
// LOBJ says that this implementation is able to handle large binaries.
// If the store is mounted, this is implied somewhere so that it crashes if it wasn't declared.
// However, if the store is not registered on the PSSManager, the large binaries function
// will consider this store as non-valid.
public:
	static CStore *	make(const char * inName);
	void			destroy(void);

	NewtonErr	init(void * inStoreData, size_t inStoreSize, ULong inArg3, ArrayIndex inSocketNumber, ULong inFlags, void * inPSSInfo);
				// Initializes the store with the SPSSInfo (inPSSInfo)
				// Flags are used by TFlashStore to determine if the store is on SRAM, Flash card or Internal Flash
	NewtonErr	needsFormat(bool * outNeedsFormat);
				// Tests if the store needs to be formatted. If so, a dialog will be shown to offer to format it.
	NewtonErr	format(void);
				// Formats the card. Should create an empty object of the ID rootID.

	NewtonErr	getRootId(PSSId * outRootId);
				// There is a root object from which every other object is linked.
	NewtonErr	newObject(PSSId * outObjectId, size_t inSize);
				// Creates a new object. TFlashStore interface calls NewObject( NULL, inSize, outObjectID );
	NewtonErr	eraseObject(PSSId inObjectId);
				// TFlashStore returns noErr without doing anything.
	NewtonErr	deleteObject(PSSId inObjectId);
				// Removes the object
	NewtonErr	setObjectSize(PSSId inObjectId, size_t inSize);
				// Changes the size of an object.
				// Returns kStoreErrObjectNotFound if the object cannot be found.
	NewtonErr	getObjectSize(PSSId inObjectId, size_t * outSize);
				// Returns the size of an object.
				// Returns kStoreErrObjectNotFound if the object cannot be found.

	NewtonErr	write(PSSId inObjectId, size_t inStartOffset, void * inBuffer, size_t inLength);
				// Writes the data for an object from inStartOffset for inLength bytes.
	NewtonErr	read(PSSId inObjectId, size_t inStartOffset, void * outBuffer, size_t inLength);
				// Reads the data for an object from inStartOffset for inLength bytes.

	NewtonErr	getStoreSize(size_t * outTotalSize, size_t * outUsedSize);
				// Returns the total size and the used size for the store.
	NewtonErr	isReadOnly(bool * outIsReadOnly);
				// Physical ReadOnly
	NewtonErr	lockStore(void);
	NewtonErr	unlockStore(void);
				// There is normally a lock store counter.
				// This counter is increased with LockStore and decreased with UnlockStore.

	NewtonErr	abort(void);
				// Aborts the current transaction.
	NewtonErr	idle(bool * outArg1, bool * outArg2);
				// TFlashStore does nothing.

	NewtonErr	nextObject(PSSId inObjectId, PSSId * outNextObjectId);
				// Suspect PSSIds are actually VAddrs
				// TPackageStore sets outNextObjectId to 0.
				// Both TFlashStore (which does not set *outObject) and TPackageStore return noErr.
				// This isn't really implemented anywhere.
	NewtonErr	checkIntegrity(ULong * inArg1);
				//	Both TFlashStore and TPackageStore return noErr
				// This isn't really implemented anywhere.

	NewtonErr	setBuddy(CStore * inStore);
				// TMuxStore calls inherited. Both TFlashStore and TPackageStore return noErr
	bool			ownsObject(PSSId inObjectId);
				// TMuxStore only. Both TFlashStore and TPackageStore return true.
	VAddr			address(PSSId inObjectId);
				// TMuxStore calls inherited. Both TFlashStore and TPackageStore return 0.
	const char * storeKind(void);
				// Returns a CString representation of the store kind (e.g. "Internal", "Package")
	NewtonErr	setStore(CStore * inStore, ObjectId inEnvironment);
				// TMuxStore only. Both TFlashStore and TPackageStore return noErr

	bool			isSameStore(void * inData, size_t inSize);
				// TPackageStore returns false. More complex with TFlashStore.
	bool			isLocked(void);
				// Returns (the counter of [Un]LockStore is not null)
	bool			isROM(void);
				// A ROM Flash Card is an application card. TPackageStore returns true.

// Power management
	NewtonErr	vppOff(void);
				// TFlashStore powers off the card, whatever the power level is.
	NewtonErr	sleep(void);
				// TMuxStore calls inherited. Both TFlashStore and TPackageStore return noErr

	NewtonErr	newWithinTransaction(PSSId * outObjectId, size_t inSize);
				// TPackageStore calls NewObject.
				// Starts a transaction against this new object.
	NewtonErr	startTransactionAgainst(PSSId inObjectId);
				// Start a transaction against the given object.
	NewtonErr	separatelyAbort(PSSId inObjectId);
	NewtonErr	addToCurrentTransaction(PSSId inObjectId);
	bool			inSeparateTransaction(PSSId inObjectId);

	NewtonErr	lockReadOnly(void);
				// TFlashStore increases a counter. TPackageStore returns noErr
	NewtonErr	unlockReadOnly(bool inReset);
				// TFlashStore decreases a counter or resets it to zero if inReset. TPackageStore returns noErr
	bool			inTransaction(void);

// The following entries are not in the jumptable, hence you can't call them (but the system may rely on them)

	NewtonErr	newObject(PSSId * outObjectId, void * inData, size_t inSize);
				// TFlashStore::NewObject( long, unsigned long* ) calls this one with NULL as first arg
	NewtonErr	replaceObject(PSSId inObjectId, void * inData, size_t inSize);

//	eXecute-In-Place
	NewtonErr	calcXIPObjectSize(long inArg1, long inArg2, long * outArg3);
	NewtonErr	newXIPObject(PSSId * outObjectId, size_t inSize);
	NewtonErr	getXIPObjectInfo(PSSId inObjectId, unsigned long * outArg2, unsigned long * outArg3, unsigned long * outArg4);

};

#endif	/* __STORE_H */
