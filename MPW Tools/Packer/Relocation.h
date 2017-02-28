/*
	File:		Relocation.h

	Contains:	Classes to relocate frame data within a package.

	Written by:	The Newton Tools group.
*/

#ifndef __RELOCATION_H
#define __RELOCATION_H

#include "NewtonKit.h"

/* -----------------------------------------------------------------------------
	C F i l e R e l o c a t i o n s
----------------------------------------------------------------------------- */

class CFileRelocations
{
public:
			CFileRelocations();
	void	set(ULong, ULong, ULong, ULong *, UChar, void *, const UChar *);
	void	relocate(ULong, ULong *, ULong) const;
};


/* -----------------------------------------------------------------------------
	C R e l o c a t i o n I t e r a t o r
----------------------------------------------------------------------------- */

class CRelocationIterator
{
public:
			CRelocationIterator(ULong, const CFileRelocations *, ULong);
	void	next(ULong *, const UChar **);
	void	setupFile(ULong);
};


/* -----------------------------------------------------------------------------
	C R e l o c a t i o n C h u n k
----------------------------------------------------------------------------- */

class CRelocationChunk
{
public:
	CRelocationChunk();
	~CRelocationChunk();
	void	allocate(UShort);
	void	append(ULong, const UChar *);
	size_t	fileSize(void) const;
	void	write(FILE * ioFile, UShort, const char * inFilename) const;
};


/* -----------------------------------------------------------------------------
	C R e l o c a t i o n C o l l e c t o r
----------------------------------------------------------------------------- */

class CRelocationCollector
{
public:
	CRelocationCollector(ULong);
	~CRelocationCollector();
	ArrayIndex	addRelocations(ULong, ULong, ULong, int32_t *, UChar, void *, const char *);
	void	compute(ULong, ULong);
	ArrayIndex	count(void) const;
	size_t	fileSize(void) const;
	size_t	sizeUsed(void) const;
	int	write(FILE * ioFile, const char *) const;
	void	relocate(ULong, ULong, ULong *, ULong) const;
private:
	ArrayIndex	fCount;	//+00
};

inline ArrayIndex CRelocationCollector::count(void) const { return fCount; }


/* -----------------------------------------------------------------------------
	Public access to relocation.
----------------------------------------------------------------------------- */

void	RelocateFrames(void *, long, long, void(*)(void *, long *, long *, long), void *);
void	RelocateFrames(void *, long, long);


#endif /* __RELOCATION_H */
