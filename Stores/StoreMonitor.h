/*
	File:		StoreMonitor.h

	Contains:	Newton store monitor interface.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__STOREMONITOR_H)
#define __STOREMONITOR_H 1

#include "Protocols.h"
#include "PSSTypes.h"

/*------------------------------------------------------------------------------
	C S t o r e M o n i t o r
	P-class interface.

	It’s not the full store protocol.
------------------------------------------------------------------------------*/

MONITOR CStoreMonitor : public CProtocol
{
public:
	static CStoreMonitor *	make(const char * inName);
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
};

#endif	/* __STOREMONITOR_H */
