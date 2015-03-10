/*
	File:		DockerDynArray.cc

	Contains:	Docker dynamic array implementation.
					This is a simple dynamic array that:
					o	allocates blocks of ULong ids;
					o	never reduces in size;
					o	can reuse entries containing zero.

	Written by:	Newton Research Group, 2012.
*/

#include "NewtonMemory.h"
#include "Docker.h"
#include "OSErrors.h"


/* -------------------------------------------------------------------------------
	C D o c k e r D y n A r r a y
	Dynamic array used for recording protocol extension ids.
------------------------------------------------------------------------------- */
#define kAllocationBlockSize 30

/* -------------------------------------------------------------------------------
	Constructor.
------------------------------------------------------------------------------- */

CDockerDynArray::CDockerDynArray()
{
	fArray = NULL;
	fSize = 0;
	fAllocatedSize = 0;
}


/* -------------------------------------------------------------------------------
	Destructor.
------------------------------------------------------------------------------- */

CDockerDynArray::~CDockerDynArray()
{
	if (fArray)
		FreePtr((Ptr)fArray);
}


/* -------------------------------------------------------------------------------
	Append an id to our list.
	Args:		inId
	Return:	error code
------------------------------------------------------------------------------- */

NewtonErr
CDockerDynArray::add(ULong inId)
{
	NewtonErr err = noErr;
	XTRY
	{
		if (fArray == NULL)
		{
			fAllocatedSize = kAllocationBlockSize;
			XFAILIF((fArray = (ULong *)NewPtr(fAllocatedSize * sizeof(ULong))) == NULL, err = kOSErrNoMemory;)
		}
		if (fSize >= fAllocatedSize)
		{
			fAllocatedSize += kAllocationBlockSize;
			XFAILIF((fArray = (ULong *)ReallocPtr((Ptr)fArray, fAllocatedSize * sizeof(ULong))) == NULL, err = kOSErrNoMemory;)
		}
		fArray[fSize++] = inId;
	}
	XENDTRY;
	return err;
}


/* -------------------------------------------------------------------------------
	Add an id to our list.
	Try to reuse a zero entry (one that has been unregistered).
	Args:		inId
				index		-> index of id’s entry
	Return:	error code
------------------------------------------------------------------------------- */

NewtonErr
CDockerDynArray::addAndReplaceZero(ULong inId, ArrayIndex& index)
{
	NewtonErr err = noErr;
	index = find(0);
	if (index != kIndexNotFound)
		fArray[index] = inId;
	else
	{
		err = add(inId);
		index = fSize-1;
	}
	return err;
}


/* -------------------------------------------------------------------------------
	Replace an id in our list.
	Args:		index
				inId
	Return:	--
------------------------------------------------------------------------------- */

void
CDockerDynArray::replace(ArrayIndex index, ULong inId)
{
	if (index < fSize)
		fArray[index] = inId;
}


/* -------------------------------------------------------------------------------
	Find an id in our list.
	Args:		inId
	Return:	its index in the list
				kIndexNotFound => was not found
------------------------------------------------------------------------------- */

ArrayIndex
CDockerDynArray::find(ULong inId)
{
	if (fArray != NULL)
	{
		for (ArrayIndex i = 0; i < fSize; ++i)
		{
			if (fArray[i] == inId)
				return i;
		}
	}
	return kIndexNotFound;
}

