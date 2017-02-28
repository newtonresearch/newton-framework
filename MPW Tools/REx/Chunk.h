/*
	File:		Chunk.h

	Contains:	Class of a chunk of data that will form a RExHeader.ConfigEntry.

	Written by:	The Newton Tools group.
*/

#ifndef __CHUNK_H
#define __CHUNK_H

#include "NewtonKit.h"

/* -----------------------------------------------------------------------------
	C C h u n k
	A chunk of data that will form a RExHeader.ConfigEntry.
----------------------------------------------------------------------------- */

class CChunk
{
public:
				CChunk();

	void		expand(size_t inSize);
	void		append(void * inBlock, size_t inSize);
	void		append(const char * inFilename);
	void		append(Ptr inRsrc, short, short, int);
	void		roundUp(size_t inAlignment);

	Ptr		data(void);
	size_t	dataSize(void);

const size_t kChunkSize = 0x200;

private:
	Ptr		fPtr;
	size_t	fPtrSize;
	size_t	fUsedSize;
};

inline Ptr CChunk::data(void) { return fPtr; }
inline size_t CChunk::dataSize(void) { return fUsedSize; }


#include <iostream>
std::ostream & operator<<(std::ostream &__os, CChunk & ioChunk);


#endif /* __CHUNK_H */
