/*
	File:		MuxStore.h

	Contains:	Newton mux store interface.

	Written by:	Newton Research Group.
*/

#if !defined(__MUXSTORE_H)
#define __MUXSTORE_H 1

#include "Store.h"
#include "StoreMonitor.h"
#include "Semaphore.h"

class CMuxStoreMonitor;

/*------------------------------------------------------------------------------
	C M u x S t o r e
	A mux store is a wrapper around a CStore that allows thread-safe access.
------------------------------------------------------------------------------*/

PROTOCOL CMuxStore : public CStore
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CMuxStore)

	CMuxStore *	make(void);
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
	NewtonErr	setStore(CStore * inStore, PSSId inEnvironment);

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

	CStore *		getStore(void) const;

private:
	NewtonErr	acquireLock(void);
	NewtonErr	releaseLock(void);

	CStore *					fStore;			// +10
	CMuxStoreMonitor *	fMonitor;		// +14
	CULockingSemaphore *	fLock;			// +18
};

inline CStore *	CMuxStore::getStore(void) const	{ return fStore; }
inline NewtonErr	CMuxStore::acquireLock(void)		{ return fLock->acquire(kWaitOnBlock); }
inline NewtonErr	CMuxStore::releaseLock(void)		{ return fLock->release(); }


/*------------------------------------------------------------------------------
	C M u x S t o r e M o n i t o r
	The mux store monitor allows thread-safe access to a CStore.
------------------------------------------------------------------------------*/

MONITOR CMuxStoreMonitor : public CStoreMonitor
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CMuxStoreMonitor)

	CMuxStoreMonitor *	make(void);
	void			destroy(void);

	NewtonErr	init(CStore * inStore);
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

	NewtonErr	newWithinTransaction(PSSId * outObjectId, size_t inSize);
	NewtonErr	startTransactionAgainst(PSSId inObjectId);
	NewtonErr	separatelyAbort(PSSId inObjectId);
	NewtonErr	addToCurrentTransaction(PSSId inObjectId);

	NewtonErr	lockReadOnly(void);
	NewtonErr	unlockReadOnly(bool inReset);

	NewtonErr	newObject(PSSId * outObjectId, void * inData, size_t inSize);
	NewtonErr	replaceObject(PSSId inObjectId, void * inData, size_t inSize);

	NewtonErr	newXIPObject(PSSId * outObjectId, size_t inSize);

private:
	CStore *		fStore;	//+10
//size+20
};


#endif	/* __MUXSTORE_H */
