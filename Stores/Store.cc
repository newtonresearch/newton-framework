/*
	File:		Store.cc

	Contains:	Newton object store interface.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "Store.h"

#pragma mark CStore
/*------------------------------------------------------------------------------
	C S t o r e
	This is a p-class interface, so there’s no implementation here.
	But you might use this file as a template for a store implementation.
------------------------------------------------------------------------------*/

#pragma mark Initialisation
/* -----------------------------------------------------------------------------
	Initialize the store.
	Args:		inStoreData
				inStoreSize
				inArg3
				inSocketNumber
				inFlags
				inArg6
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::init(void * inStoreData, size_t inStoreSize, ULong inArg3, ArrayIndex inSocketNumber, ULong inFlags, void * inArg6)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Does this store require formatting?.
	Args:		outNeedsFormat
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::needsFormat(bool * outNeedsFormat)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Format the store. Should create an empty object of the id rootId.
	Args:		--
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::format(void)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Check the integrity of the store.
	Args:		inArg1
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::checkIntegrity(ULong * inArg1)
{ return noErr; }


#pragma mark Store info
/* -----------------------------------------------------------------------------
	Return the store’s root object id.
	Args:		outRootId
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::getRootId(PSSId * outRootId)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Return the store size.
	Args:		outTotalSize
				outUsedSize
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::getStoreSize(size_t * outTotalSize, size_t * outUsedSize)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Return the kind of store.
	Args:		--
	Return	C string
----------------------------------------------------------------------------- */

const char *
CStore::storeKind(void)
{ return "Unknown"; }


/* -----------------------------------------------------------------------------
	Is this store read-only?
	Args:		outIsReadOnly
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::isReadOnly(bool * outIsReadOnly)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Is this store in ROM? A ROM Flash Card is an application card.
	Args:		--
	Return	true => it’s in ROM
----------------------------------------------------------------------------- */

bool
CStore::isROM(void)
{ return false; }


/* -----------------------------------------------------------------------------
	Lock the store.
	Locking REQUIRES an implementation.
	There is normally a lock store counter.
	This counter is increased with lockStore() and decreased with unlockStore()
	Args:		--
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::lockStore(void)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Unlock the store.
	Args:		--
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::unlockStore(void)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Is this store locked?
	Args:		--
	Return	error code
----------------------------------------------------------------------------- */

bool
CStore::isLocked(void)
{ return false; }


#pragma mark MuxStore
/* -----------------------------------------------------------------------------
	Set the store’s buddy.
	Args:		inStore
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::setBuddy(CStore * inStore)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Set the store.
	Args:		inStore
				inEnvironment
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::setStore(CStore * inStore, ObjectId inEnvironment)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Is this the same store?
	Args:		inData
				inSize
	Return	YEs => is the same store
----------------------------------------------------------------------------- */

bool
CStore::isSameStore(void * inData, size_t inSize)
{ return false; }


#pragma mark Object access
/* -----------------------------------------------------------------------------
	Create a new object in the store.
	Args:		outObjectId
				inSize
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::newObject(PSSId * outObjectId, size_t inSize)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Erase an object from the store.
	Args:		inObjectId
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::eraseObject(PSSId inObjectId)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Delete an object from the store.
	Args:		inObjectId
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::deleteObject(PSSId inObjectId)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Set the size of an object in the store.
	Args:		inObjectId
				inSize
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::setObjectSize(PSSId inObjectId, size_t inSize)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Return the size of an object in the store.
	Args:		inObjectId
				outSize
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::getObjectSize(PSSId inObjectId, size_t * outSize)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Return the id of the next object in the store.
	Don’t think this interface is used anywhere.
	Args:		inObjectId
				outNextObjectId
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::nextObject(PSSId inObjectId, PSSId * outNextObjectId)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Does this store own the given object?
	Args:		inObjectId
	Return	true => store owns the object
----------------------------------------------------------------------------- */

bool
CStore::ownsObject(PSSId inObjectId)
{ return false; }


/* -----------------------------------------------------------------------------
	Return the address of an object in the store.
	Args:		inObjectId
	Return	address
----------------------------------------------------------------------------- */

VAddr
CStore::address(PSSId inObjectId)
{ return 0; }


/* -----------------------------------------------------------------------------
	Read an object in the store.
	Args:		inObjectId			object’s id
				inStartOffset		offset into object
				outBuffer			buffer to receive object data
				inLength				size of object to read -- may not be whole object
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::read(PSSId inObjectId, size_t inStartOffset, void * outBuffer, size_t inLength)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Write an object to the store.
	Args:		inObjectId			object’s id
				inStartOffset		offset into buffer for object data
				inBuffer				object data
				inLength				size of object data
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::write(PSSId inObjectId, size_t inStartOffset, void * inBuffer, size_t inLength)
{ return noErr; }


#pragma mark Not in the jump table
/* -----------------------------------------------------------------------------
	The following entries are not in the jumptable, hence you can't call them
	(but the system may rely on them).
----------------------------------------------------------------------------- */

NewtonErr
CStore::newObject(PSSId * outObjectId, void * inData, size_t inSize)
{ return noErr; }


NewtonErr
CStore::replaceObject(PSSId inObjectId, void * inData, size_t inSize)
{ return noErr; }


NewtonErr
CStore::newWithinTransaction(PSSId * outObjectId, size_t inSize)
{ return noErr; }


NewtonErr
CStore::startTransactionAgainst(PSSId inObjectId)
{ return noErr; }


NewtonErr
CStore::addToCurrentTransaction(PSSId inObjectId)
{ return noErr; }


NewtonErr
CStore::abort(void)
{ return noErr; }


NewtonErr
CStore::separatelyAbort(PSSId inObjectId)
{ return noErr; }


bool
CStore::inTransaction(void)
{ return false; }


bool
CStore::inSeparateTransaction(PSSId inObjectId)
{ return false; }


NewtonErr
CStore::lockReadOnly(void)
{ return noErr; }


NewtonErr
CStore::unlockReadOnly(bool inReset)
{ return noErr; }


#pragma mark eXecute-In-Place
/* -----------------------------------------------------------------------------
	The following entries are for eXecute-In-Place objects.
	I have no idea what this means.
----------------------------------------------------------------------------- */

NewtonErr
CStore::calcXIPObjectSize(long inArg1, long inArg2, long * outArg3)
{ return noErr; }

NewtonErr
CStore::newXIPObject(PSSId * outObjectId, size_t inSize)
{ return noErr; }

NewtonErr
CStore::getXIPObjectInfo(PSSId inObjectId, unsigned long * outArg2, unsigned long * outArg3, unsigned long * outArg4)
{ return noErr; }


#pragma mark Power management
/* -----------------------------------------------------------------------------
	Idle the store.
	Args:		--
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::idle(bool * outArg1, bool * outArg2)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Send the store to sleep.
	Args:		--
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::sleep(void)
{ return noErr; }


/* -----------------------------------------------------------------------------
	Power off the card.
	Args:		--
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CStore::vppOff(void)
{ return noErr; }

