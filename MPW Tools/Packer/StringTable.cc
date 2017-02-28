/*
	File:		StringTable.cc

	Contains:	Accessors to string data area of a package file loaded into memory.

	Written by:	The Newton Tools group.
*/

#include "StringTable.h"
#include "Reporting.h"
#include "Allocation.h"


/* -----------------------------------------------------------------------------
	String table.
----------------------------------------------------------------------------- */

CStringTable gStrings;					//-1B52 glob148


// constructor -- start with default block of 1K
CStringTable::CStringTable()
{
	fUsedSize = 0;
	fAllocatedSize = 1*KByte;
	fMem = (Ptr)malloc(fAllocatedSize);
	ASSERT(fMem != NULL);
}

// destructor -- free allocated block
CStringTable::~CStringTable()
{
	free(fMem);
}

// append bytes to our block of memory
ArrayIndex
CStringTable::appendBytes(const void * inBuf, size_t inSize)
{
	assureSize(inSize);
	memcpy(fMem+fUsedSize, inBuf, inSize);
	ArrayIndex startOffset = (ArrayIndex)fUsedSize;
	fUsedSize += inSize;
	return startOffset;
}

// write fMem to file
int
CStringTable::write(FILE * ioFile)
{
	size_t numWritten = fwrite(fMem, fUsedSize, 1, ioFile);
	return (numWritten != 1)? EIO:0;
}

// ensure our block of memory is large enough to accommodate the proposed addition
void
CStringTable::assureSize(size_t inSize)
{
	if (inSize > (fAllocatedSize - fUsedSize)) {
		fAllocatedSize *= 2;
		ASSERT(fAllocatedSize < 16*KByte);
		fMem = (Ptr)realloc(fMem, fAllocatedSize);
		ASSERT(fMem != NULL);
	}
}

/* -----------------------------------------------------------------------------
	Public access to the string data.
----------------------------------------------------------------------------- */

ArrayIndex
AssureUShort(ArrayIndex index)
{
	ErrorMessageIf((index > 16*KByte), "offset %u is too large", index);
	return index;
}


InfoRef
AddBytes(const void * inBuf, ArrayIndex inCount)
{
	InfoRef location;
	location.offset = AssureUShort(gStrings.appendBytes(inBuf, inCount));
	location.length = inCount;
	return location;
}


InfoRef
AddString(const char * inStr)
{
	return AddBytes(inStr, (ArrayIndex)(strlen(inStr) + 1));
}


InfoRef
AddUnicodeString(const char * inStr)
{
	ArrayIndex uStrLen = (ArrayIndex)(strlen(inStr) + 1) * sizeof(UniChar);
	Ptr uStr = (Ptr)malloc_or_exit(uStrLen, "can't allocate %lu bytes for Unicode string");
	for (ArrayIndex i = 0; i < uStrLen; ++i) {
		*uStr++ = *inStr++;
	}
	InfoRef info = AddBytes(uStr, uStrLen);
	free(uStr);
	return info;
}

