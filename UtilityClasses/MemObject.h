/*
	File:		MemObject.h

	Contains:	Interface to CMemObject.

	Written by:	Newton Research Group.
*/

#if !defined(__MEMOBJECT_H)
#define __MEMOBJECT_H 1

#if !defined(__USERSHAREDMEM_H)
#include "UserSharedMem.h"
#endif

#if !defined(__USERPORTS_H)
#include "UserPorts.h"
#endif

/*
	Class CMemObject

	Basic memory object.  A CMemObject is either Private or Shared.
	A private CMemObject contains the raw pointer and size of the block,
	which is memory owned by the task using it (usually created via malloc).

	A shared CMemObject contains a shared memory object reference to the block.

	Basic operations on a CMemObject:

		isPrivate		return true if internal object, false otherwise

		isShared			returns true if the memory is set to be shared,
							false otherwise

		getId				if the object is shared, returns the shared memory
							id.  useful when you need to pass the id to an
							external task world.

		getSize			returns the size of the object in bytes

		copyTo			copies data to the object, you supply the ptr to the data
							to copy, the size of the data, and an optional offset.

		copyFrom			copies data from the object, you supply a ptr to a buffer
							to copy to, along with a size and optional offset.

		destroy			disposes of any memory associated with the memory object,
							only has an effect on internal objects

	Operations valid on Private blocks:

		init(ULong size, Ptr buffer, bool makeShared = NO, ULong permissions = kSMemReadOnly)

							intializes a CMemObject to the buffer supplied, you must
							also pass in the size.  if you intend to "share" this memory
							with another task, set makeShared and determine the permissions

							use kSMemReadOnly if you want the external client to only have
							read access, use kSMemReadWrite to give full read/write permission

		init(ULong size, bool makeShared = NO, ULong permissions = kSMemReadOnly)

							basically same as init above, however all you need to supply
							is the size, the allocation will be done automatically


		makeShared		makes the internal object into a shared memory object.  you
							can supply the permissions.  before passing the id of the
							object to another task, you must make the object shared.

		getBase			returns a raw pointer to the data, 0 if the object is
							external


	Operations valid on Shared blocks:

		make(ObjectId sharedObjectId, UMsgToken * msgToken = NULL)

							given a memory object id, Make will intialize the CMemObject
							to its memory reference.  useful when you have received an id
							from the owner task and want to make a CMemObject out of it.

*/


struct MemObjectFlags
{
	unsigned int	internal:		1;		// set if internal object
	unsigned int	createdMemory:	1;		// set if memory object created the memory, valid only if internal set
	unsigned int	shared:			1;		// set if memory object is shared
	unsigned int	useToken:		1;		// set if memory object should use UMsgToken in copies
	unsigned int	unused1:			1;
	unsigned int	unused2:			1;
	unsigned int	unused3:			1;
	unsigned int	unused4:			1;
};


class CMemObject : public SingleObject
{
public:
						CMemObject();
						~CMemObject();

	NewtonErr		init(size_t inSize, Ptr inBuffer, bool inMakeShared = NO, ULong inPermissions = kSMemReadOnly);
	NewtonErr		init(size_t inSize, bool inMakeShared = NO, ULong inPermissions = kSMemReadOnly);
	long				make(ObjectId inSharedObjectId, CUMsgToken * inMsgToken = NULL);
	NewtonErr		makeShared(ULong inPermissions = kSMemReadOnly);
	void				destroy(void);

	bool				isInternal(void);
	bool				isShared(void);
	ObjectId			getId(void);				// returns id if type is external, 0 otherwise

	NewtonErr		copyTo(Ptr inBuffer, size_t inSize, ULong inOffset = 0);
	NewtonErr		copyFrom(size_t * outSize, Ptr outBuffer, size_t inSize, ULong inOffset = 0);
	ULong				getSize(void);

	// internal only calls…
	void *			getBase(void);			// returns fBuffer if type is internal

private:
	MemObjectFlags	fFlags;					// object flags
	CUSharedMem		fSharedMemoryObject;	// shared memory object if type external
	CUMsgToken		fSharedMemoryToken;	// validation token for shared memory object
	size_t			fSize;					// size if internal, cache of size if type external
	void *			fBuffer;					// ptr to memory if type internal
};


inline bool		CMemObject::isInternal(void)	{ return fFlags.internal; }
inline bool		CMemObject::isShared(void)		{ return fFlags.shared; }
inline ULong	CMemObject::getSize(void)		{ return fSize; }


#endif	/* __MEMOBJECT_H */
