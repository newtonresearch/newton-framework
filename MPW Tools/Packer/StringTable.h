/*
	File:		StringTable.h

	Contains:	Accessors to string data area of a package file loaded into memory.

	Written by:	The Newton Tools group.
*/

#ifndef __STRINGTABLE_H
#define __STRINGTABLE_H

#include "NewtonKit.h"

/* -----------------------------------------------------------------------------
	C S t r i n g T a b l e
	A block of memory containing nul-terminated C strings.
	Strings are referenced by their index into the table (offset from start of
	allocated block).
----------------------------------------------------------------------------- */

class CStringTable
{
public:
	CStringTable();
	~CStringTable();

	ArrayIndex	appendBytes(const void * inBuf, size_t inSize);
	int			write(FILE * ioFile);
	void			assureSize(size_t inSize);
	size_t		fileSize(void) const;
private:
	size_t		fUsedSize;
	size_t		fAllocatedSize;
	Ptr			fMem;
};

inline size_t CStringTable::fileSize(void) const { return fUsedSize; }

extern CStringTable gStrings;


/* -----------------------------------------------------------------------------
	Public access to the string data.
----------------------------------------------------------------------------- */

ArrayIndex	AssureUShort(ArrayIndex index);
InfoRef		AddBytes(const void * inBuf, ArrayIndex inCount);
InfoRef		AddString(const char * inStr);
InfoRef		AddUnicodeString(const char * inStr);

#endif /* __STRINGTABLE_H */
