/*
	File:		Chunk.cc

	Contains:	Implementation of a chunk of data that will form a RExHeader.ConfigEntry.

	Written by:	The Newton Tools group.
*/

#include "Chunk.h"
#include "Reporting.h"


/* -----------------------------------------------------------------------------
	C C h u n k
	A chunk of data that will form a RExHeader.ConfigEntry.
----------------------------------------------------------------------------- */

CChunk::CChunk()
{
	fPtr = (Ptr)malloc(kChunkSize);
	if (fPtr == NULL)
		FatalError("out of memory");
	fPtrSize = kChunkSize;
	fUsedSize = 0;
}

void
CChunk::expand(size_t inSize)
{
	if (fUsedSize + inSize > fPtrSize) {
		size_t numExtraChunks = (fUsedSize + inSize - fPtrSize)/kChunkSize + 1;
		fPtr = (Ptr)realloc(fPtr, fPtrSize + (numExtraChunks * kChunkSize));
		if (fPtr == NULL) {
			FatalError("out of memory");
		}
		fPtrSize += (numExtraChunks * kChunkSize);
	}
	fUsedSize += inSize;
}

void
CChunk::append(void * inBlock, size_t inSize)
{
	size_t start = fUsedSize;
	expand(inSize);
	if (inBlock != NULL) {
		memcpy(fPtr + start, inBlock, inSize);
	} else {
		memset(fPtr + start, 0, inSize);
	}
}

void
CChunk::append(const char * inFilename)
{
	FILE * fp = fopen(inFilename, "rb");
	if (fp == NULL) {
		FatalError("couldn't open “%s”", inFilename);
	}
	fseek(fp, 0, SEEK_END);
	size_t fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	size_t start = fUsedSize;
	expand(fileSize);
	size_t numRead = fread(fPtr + start, fileSize, 1, fp);
	if (numRead != 1) {
		FatalError("couldn't read “%s”", inFilename);
	}
	fclose(fp);
}

void
CChunk::append(Ptr inRsrc, short, short, int)
{
	// for appending resources
	// we don’t use resources in OS X
//	"couldn’t get resource type = 0x%8X in “%s”"
//	"couldn’t get size of resource in “%s”"
}

void
CChunk::roundUp(size_t inAlignment)
{
	size_t delta = fUsedSize % inAlignment;
	if (delta != 0) {
		append(NULL, inAlignment - delta);
	}
}


#pragma mark -
/* -----------------------------------------------------------------------------
	Output chunk to stream (typically the output file).
----------------------------------------------------------------------------- */

std::ostream &
operator<<(std::ostream &__os, CChunk & ioChunk)
{
	__os.write(ioChunk.data(), ioChunk.dataSize());
	return __os;
}

