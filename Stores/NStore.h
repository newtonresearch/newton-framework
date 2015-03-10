/*
	File:		NStore.h

	Contains:	Newton object store interface.

	Written by:	Newton Research Group.
*/

#if !defined(__NSTORE_H)
#define __NSTORE_H 1

/*------------------------------------------------------------------------------
	N S t o r e
------------------------------------------------------------------------------*/

#if defined(__cplusplus)
#define NStore void
#else
@interface NStore
{ }
//	PROTOCOLVERSION(1.0)
//	PROTOCOL_IMPL_HEADER_MACRO(NStore);

//	CAPABILITIES( "LOBJ" )
// LOBJ says that this implementation is able to handle large binaries.
// If the store is mounted, this is implied somewhere so that it crashes if it wasn't declared.
// However, if the store is not registered on the TPSSManager, the large binaries function
// will consider this store as non-valid.

//	static size_t					Sizeof(void);
//	static const TClassInfo *	ClassInfo(void);

+ (NStore *)	new: (char *) inImplementationName;		// PROTOCOL constructor
- (void)			delete;											// PROTOCOL destructor

- (NewtonErr)	init: (void *) inArg1 with: (unsigned long) inArg2 and: (unsigned long) inArg3 and: (int) inSocketNumber and: (unsigned long) inFlags and: (void *) inArg6;
					// Initializes the store with the SPSSInfo (inArg6)
					// Flags are used by TFlashStore to determine if the store is on SRAM, Flash card or Internal Flash
- (NewtonErr)	needsFormat: (BOOL *) outNeedsFormat;
					// Tests if the store needs to be formatted. If so, a dialog will be shown to offer to format it.
- (NewtonErr)	format;
					// Formats the card. Should create an empty object of the ID rootID.
- (NewtonErr)	getRootId: (ObjectId *) outRootId;
					// There is a root object from which every other object is linked.
- (NewtonErr)	newObject: (ObjectId *) outObjectId size: (long) inSize;
					// Creates a new object. TFlashStore interface calls NewObject( nil, inSIze, outObjectID );
- (NewtonErr)	eraseObject: (ObjectId) inObjectId;
					// TFlashStore returns noErr without doing anything.
- (NewtonErr)	deleteObject: (ObjectId) inObjectId;
					// Removes the object
- (NewtonErr)	setObject: (ObjectId) inObjectId size: (long) inSize;
					// Changes the size of an object.
					// Returns kSError_ObjectNotFound if the object cannot be found.
- (NewtonErr)	getObject: (ObjectId) inObjectId size: (long *) outSize;
					// Returns the size of an object.
					// Returns kSError_ObjectNotFound if the object cannot be found.
- (NewtonErr)	nextObject: (ObjectId) inObjectId id: (ObjectId *) outNextObjectId;
					// TPackageStore sets outObjectID to 0.
					// Both TFlashStore (which does not set *outObject) and TPackageStore return noErr.
					// This isn't really implemented anywhere
- (NewtonErr)	checkIntegrity: (unsigned long *) inArg1;
					// TMuxStore calls TStoreMonitor in a try {} statement, (i.e.) TMuxStoreMonitor
					//	which calls inherited TStore.
					//	Both TFlashStore and TPackageStore return noErr
- (BOOL)			ownsObject: (ObjectId) inObjectId;
					// TMuxStore only. Both TFlashStore and TPackageStore return true.
- (NewtonErr)	address: (unsigned long) inArg1;
					// TMuxStore calls inherited. Both TFlashStore and TPackageStore return noErr
- (NewtonErr)	write: (ObjectId) inObjectId buf: (char *) inBuffer offset: (long) inStartOffset size: (long) inLength;
					// Writes the data for an object from inStartOffset for inLength bytes.
- (NewtonErr)	read: (ObjectId) inObjectId buf: (char *) outBuffer offset: (long) inStartOffset size: (long) inLength;
					// Reads the data for an object from inStartOffset for inLength bytes.
- (NewtonErr)	getStoreSize: (long *) outTotalSize used: (long *) outUsedSize;
					// Returns the total size and the used size for the store.
- (NewtonErr)	isReadOnly: (BOOL *) outIsReadOnly;
					// Physical ReadOnly
- (BOOL)			isROM;
					// A ROM Flash Card is an application card. TPackageStore returns true.
- (char *)		storeKind;
					// Returns a CString representation of the store kind (e.g. "Internal", "Package")
- (NewtonErr)	lockStore;
- (NewtonErr)	unlockStore;
					// There is normally a lock store counter.
					// This counter is increased with LockStore and decreased with UnlockStore.
- (BOOL)			isLocked;
					// Returns (the counter of [Un]LockStore is not null)
- (NewtonErr)	idle: (unsigned char *) outArg1 with: (unsigned char *) outArg2;
					// TFlashStore does nothing.
- (NewtonErr)	setBuddy: (NStore *) inStore;
					// TMuxStore calls inherited. Both TFlashStore and TPackageStore return noErr
- (NewtonErr)	setStore: (NStore *) inStore environment: (ObjectId) inEnvironment;
					// TMuxStore only. Both TFlashStore and TPackageStore return noErr
- (BOOL)			isSameStore: (void *) inData size: (unsigned long) inSize;
					// TPackageStore returns false. More complex with TFlashStore.
- (NewtonErr)	vppOff;
					// TFlashStore powers off the card, whatever the power level is.
- (NewtonErr)	sleep;
					// TMuxStore calls inherited. Both TFlashStore and TPackageStore return noErr

// The following entries are not in the jumptable, hence you can't call them (but the system may rely on them)

- (NewtonErr)	newObject: (ObjectId *) outObjectId with: (char *) inData size: (long) inSize;
					// TFlashStore::NewObject( long, unsigned long* ) calls this one with nil as first arg
- (NewtonErr)	replaceObject: (ObjectId) inObjectId with: (char *) inData size: (long) inSize;
- (NewtonErr)	newWithinTransaction: (ObjectId *) outObjectId size: (long) inSize;
					// TPackageStore calls NewObject.
					// Starts a transaction against this new object.
- (NewtonErr)	startTransactionAgainst: (ObjectId) inObjectId;
					// Start a transaction against the given object.
- (NewtonErr)	addToCurrentTransaction: (ObjectId) inObjectId;
- (NewtonErr)	abort;
					// Aborts the current transaction. (Is in the jump table.)
- (NewtonErr)	separatelyAbort: (ObjectId) inObjectId;
- (BOOL)			inTransaction;
- (BOOL)			inSeparateTransaction: (ObjectId) inObjectId;
					// TPackageStore returns noErr for those five functions.
- (NewtonErr)	lockReadOnly;
					// TFlashStore increases a counter. TPackageStore returns noErr
- (NewtonErr)	unlockReadOnly: (BOOL) inReset;
					// TFlashStore decreases a counter or resets it to zero if inReset. TPackageStore returns noErr

- (NewtonErr)	newXIPObject: (ObjectId *) outObjectId size: (long) inSize;
- (NewtonErr)	calcXIPObjectSize: (long) inArg1 with: (long) inArg2 and: (long *) outArg3;
- (NewtonErr)	getXIPObjectInfo: (ObjectId) inObjectId with: (unsigned long *) outArg2 and: (unsigned long *) outArg3 and: (unsigned long *) outArg4;
@end
#endif

#endif	/* __NSTORE_H */
