/*
	File:		Relocation.cc

	Contains:	Accessors to string data area of a package file loaded into memory.

	Written by:	The Newton Tools group.
*/

#include "Relocation.h"
#include "Reporting.h"
#include "Allocation.h"


/* -----------------------------------------------------------------------------
	C F i l e R e l o c a t i o n s
----------------------------------------------------------------------------- */

CFileRelocations::CFileRelocations()
{}

void
CFileRelocations::set(ULong, ULong, ULong, ULong *, UChar, void *, const UChar *)
{}

void
CFileRelocations::relocate(ULong, ULong *, ULong) const
{}


#pragma mark -
/* -----------------------------------------------------------------------------
	C R e l o c a t i o n I t e r a t o r
----------------------------------------------------------------------------- */

CRelocationIterator::CRelocationIterator(ULong, const CFileRelocations *, ULong)
{}

void
CRelocationIterator::next(ULong *, const UChar **)
{}

void
CRelocationIterator::setupFile(ULong)
{}


#pragma mark -
/* -----------------------------------------------------------------------------
	C R e l o c a t i o n C h u n k
----------------------------------------------------------------------------- */

CRelocationChunk::CRelocationChunk()
{}

CRelocationChunk::~CRelocationChunk()
{}

void
CRelocationChunk::allocate(UShort)
{}

void
CRelocationChunk::append(ULong, const UChar *)
{}

size_t
CRelocationChunk::fileSize(void) const
{return 0;}

void
CRelocationChunk::write(FILE * ioFile, UShort, const char * inFilename) const
{}


#pragma mark -
/* -----------------------------------------------------------------------------
	C R e l o c a t i o n C o l l e c t o r
----------------------------------------------------------------------------- */

CRelocationCollector::CRelocationCollector(ULong)
{}

CRelocationCollector::~CRelocationCollector()
{}

ArrayIndex
CRelocationCollector::addRelocations(ULong, ULong, ULong, int32_t *, UChar, void *, const char *)
{return 0;}

void
CRelocationCollector::compute(ULong, ULong)
{}

size_t
CRelocationCollector::fileSize(void) const
{return 0;}

size_t
CRelocationCollector::sizeUsed(void) const
{return 0;}

int
CRelocationCollector::write(FILE * ioFile, const char *) const
{return 0;}

void
CRelocationCollector::relocate(ULong, ULong, ULong *, ULong) const
{}


ULong
SortRelocations(const ULong * a, const ULong * b)
{
	return *a - *b;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	Public functions.
----------------------------------------------------------------------------- */

void
RelocateFrames(void *, long, long, void(*)(void *, long *, long *, long), void *)
{}

void
RelocateFrames(void *, long, long)
{}

