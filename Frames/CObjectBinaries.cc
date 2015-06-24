/*
	File:		CObjectBinaries.cc

	Contains:	C object binary declarations.
					CObjectBinaries are a type of LargeBinary object.
					They are C objects (typically a C++ class instance) but
					allocated in the frames heap so they are subject to garbage
					collection.

	Written by:	Newton Research Group, 2007.
*/

#include "CObjectBinaries.h"
#include "ObjectHeap.h"
#include "ROMResources.h"


/*------------------------------------------------------------------------------
	C O b j e c t   B i n a r y   O b j e c t   P r o c s
------------------------------------------------------------------------------*/

size_t		CObjectBinaryLength(void * inData);
char *		CObjectBinaryDataPtr(void * inData);
void			CObjectBinarySetLength(void * inData, size_t inLength);
Ref			CObjectBinaryClone(void * inData, Ref inClass);
void			CObjectBinaryDelete(void * inData);
void			CObjectBinarySetClass(void * inData, RefArg inClass);
void			CObjectBinaryMark(void * inData);
void			CObjectBinaryUpdate(void * inData);


/*------------------------------------------------------------------------------
	D a t a
	C O b j e c t   P r o c s
	Don’t know if this is the right kind of initialization.
------------------------------------------------------------------------------*/

const IndirectBinaryProcs  gCObjectBinaryProcs
= { CObjectBinaryLength, CObjectBinaryDataPtr, CObjectBinarySetLength, CObjectBinaryClone, CObjectBinaryDelete, CObjectBinarySetClass, CObjectBinaryMark, CObjectBinaryUpdate };


/*------------------------------------------------------------------------------
	A l l o c a t i o n
------------------------------------------------------------------------------*/

Ref
AllocateCObjectBinary(void * inAddr, DeleteProc inDeleteProc, MarkProc inMarkProc, UpdateProc inUpdateProc)
{
	Ref obj = gHeap->allocateIndirectBinary(SYMA(CObject), sizeof(CObjectBinaryData));
	IndirectBinaryObject * objPtr = (IndirectBinaryObject *)ObjectPtr(obj);
	CObjectBinaryData * cBinary = (CObjectBinaryData *)&objPtr->data;
	objPtr->procs = &gCObjectBinaryProcs;
	cBinary->fData = (char *)inAddr;
	cBinary->fDeleteProc = inDeleteProc;
	cBinary->fMarkProc = inMarkProc;
	cBinary->fUpdateProc = inUpdateProc;
	return obj;
}


Ref
AllocateFramesCObject(size_t inSize, DeleteProc inDeleteProc, MarkProc inMarkProc, UpdateProc inUpdateProc)
{
	Ref obj = gHeap->allocateIndirectBinary(SYMA(CObject), sizeof(CObjectBinaryData) + inSize);
	LockRef(obj);
	IndirectBinaryObject * objPtr = (IndirectBinaryObject *)ObjectPtr(obj);
	CObjectBinaryData * cBinary = (CObjectBinaryData *)&objPtr->data;
	objPtr->procs = &gCObjectBinaryProcs;
	cBinary->fData = (char *)cBinary + sizeof(CObjectBinaryData);
	cBinary->fDeleteProc = inDeleteProc;
	cBinary->fMarkProc = inMarkProc;
	cBinary->fUpdateProc = inUpdateProc;
	return obj;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C O b j e c t   P r o c s
------------------------------------------------------------------------------*/

size_t
CObjectBinaryLength(void * inData)
{ return sizeof(CObjectBinaryData); }

char *
CObjectBinaryDataPtr(void * inData)
{ return ((CObjectBinaryData *)inData)->fData; }

void
CObjectBinarySetLength(void * inData, size_t inLength)
{ }

Ref
CObjectBinaryClone(void * inData, Ref inClass)
{ return NILREF; }

void
CObjectBinaryDelete(void * inData)
{
	CObjectBinaryData *	cBinary = reinterpret_cast<CObjectBinaryData*>(inData);
	if (cBinary->fDeleteProc)
		cBinary->fDeleteProc(cBinary->fData);
}

void
CObjectBinarySetClass(void * inData, RefArg inClass)
{ }

void
CObjectBinaryMark(void * inData)
{
	CObjectBinaryData *	cBinary = reinterpret_cast<CObjectBinaryData*>(inData);
	if (cBinary->fMarkProc)
		cBinary->fMarkProc(cBinary->fData);
}

void
CObjectBinaryUpdate(void * inData)
{
	CObjectBinaryData *	cBinary = reinterpret_cast<CObjectBinaryData*>(inData);
	if (cBinary->fUpdateProc)
		cBinary->fUpdateProc(cBinary->fData);
}
