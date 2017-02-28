/*
	File:		RExArray.h

	Contains:	REx array definition.

	Written by:	The Newton Tools group.
*/

#ifndef __REXARRAY_H
#define __REXARRAY_H 1

/* -----------------------------------------------------------------------------
	R E x A r r a y
	An array is a chunk of memory. Every element is the same size.
----------------------------------------------------------------------------- */

struct RExArray
{
	CChunk fChunk;
	size_t fElementSize;

	RExArray(size_t inElementSize);
	ArrayIndex count(void);
	Ptr at(ArrayIndex i);
};
inline RExArray::RExArray(size_t inElementSize) : fElementSize(inElementSize) { }
inline ArrayIndex RExArray::count(void) { return (ArrayIndex)fChunk.dataSize()/fElementSize; }
inline Ptr RExArray::at(ArrayIndex i) { return fChunk.data() + i*fElementSize; }

#endif	/* __REXARRAY_H */
