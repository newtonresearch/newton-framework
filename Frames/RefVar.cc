/*
	File:		RefVar.cc

	Contains:	C++ stuff to keep Ref usage simple.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ObjHeader.h"


Ref	RNILREF  = NILREF;
Ref *	RSNILREF = &RNILREF;

Ref	RTRUEREF = TRUEREF;
Ref *	RSTRUEREF = &RTRUEREF;


/*----------------------------------------------------------------------
	R e f S t r u c t
----------------------------------------------------------------------*/

void
DeleteRefStruct(RefStruct * r)
{
	if (r != NULL)
		delete(r);
}


/*----------------------------------------------------------------------
	C O b j e c t P t r
----------------------------------------------------------------------*/

CObjectPtr::CObjectPtr()
	:	RefVar()
{
	if (ISNIL(h->ref))
		ThrowErr(exFrames, kNSErrTFramesObjectPtrOfNil);
	LockRef(h->ref);
}

CObjectPtr::CObjectPtr(Ref r)
	:	RefVar(r)
{
	if (ISNIL(h->ref))
		ThrowErr(exFrames, kNSErrTFramesObjectPtrOfNil);
	LockRef(h->ref);
}

CObjectPtr::CObjectPtr(RefArg r)
	:	RefVar(r)
{
	if (ISNIL(h->ref))
		ThrowErr(exFrames, kNSErrTFramesObjectPtrOfNil);
	LockRef(h->ref);
}

CObjectPtr::CObjectPtr(const RefStruct & r)
	:	RefVar(r)
{
	if (ISNIL(h->ref))
		ThrowErr(exFrames, kNSErrTFramesObjectPtrOfNil);
	LockRef(h->ref);
}

CObjectPtr::~CObjectPtr()
{
	if (NOTNIL(h->ref))
		UnlockRef(h->ref);
}

CObjectPtr &
CObjectPtr::operator=(Ref r)
{
	if (NOTNIL(h->ref))
		UnlockRef(h->ref);
	h->ref = r;
	if (NOTNIL(h->ref))
		LockRef(h->ref);
	return *this;
}

CObjectPtr &
CObjectPtr::operator=(const CObjectPtr & op)
{
	if (NOTNIL(h->ref))
		UnlockRef(h->ref);
	h->ref = op.h->ref;
	if (NOTNIL(h->ref))
		LockRef(h->ref);
	return *this;
}

CObjectPtr::operator char*() const
{
	if (ISNIL(h->ref))
		ThrowErr(exFrames, kNSErrUnassignedTFramesObjectPtr);
	return (char *)ObjectPtr(h->ref);
}

#pragma mark -

/*----------------------------------------------------------------------
	C D a t a P t r
----------------------------------------------------------------------*/

CDataPtr &
CDataPtr::operator=(Ref r)
{
	if (NOTNIL(h->ref))
		UnlockRef(h->ref);
	h->ref = r;
	if (NOTNIL(h->ref))
		LockRef(h->ref);
	return *this;
}

CDataPtr &
CDataPtr::operator=(const CDataPtr & dp)
{
	if (NOTNIL(h->ref))
		UnlockRef(h->ref);
	h->ref = dp.h->ref;
	if (NOTNIL(h->ref))
		LockRef(h->ref);
	return *this;
}

CDataPtr::operator char*() const
{
	if (ISNIL(h->ref))
		ThrowErr(exFrames, kNSErrUnassignedTFramesObjectPtr);
	return BinaryData(h->ref);
}

CDataPtr::operator unsigned char*() const
{
	if (ISNIL(h->ref))
		ThrowErr(exFrames, kNSErrUnassignedTFramesObjectPtr);
	return (unsigned char *)BinaryData(h->ref);
}

