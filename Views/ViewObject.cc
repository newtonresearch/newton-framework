/*
	File:		ViewObject.cc

	Contains:	CViewObject definition.

	Written by:	Newton Research Group.
*/

#include "ViewObject.h"


/*--------------------------------------------------------------------------------
	C V i e w O b j e c t
--------------------------------------------------------------------------------*/

CViewObject::~CViewObject()
{}


void *
CViewObject::operator new(size_t inSize)
{
	void * p;
	if ((p = calloc(1, inSize)) == NULL)
		OutOfMemory();
	return p;
}


void
CViewObject::operator delete(void * inObj)
{
	free(inObj);
}


ULong
CViewObject::classId(void) const
{
	return kFirstClassId;
}


bool
CViewObject::derivedFrom(long inClassId) const
{
	return inClassId == kFirstClassId;
}

/*
ULong
CViewObject::key(void)
{
	return 0;
}
*/
