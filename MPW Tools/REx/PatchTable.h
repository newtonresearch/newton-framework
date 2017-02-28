/*
	File:		PatchTable.h

	Contains:	Patch table munging.

	Written by:	The Newton Tools group.
*/

#ifndef __PATCHTABLE_H
#define __PATCHTABLE_H 1

#include "Chunk.h"


struct PatchInfo
{
	CChunk * fData;
	long		fOffset;
	CChunk * fPatch;
};


extern void	ReadPatchInfoFile(CChunk * outData, const char * inFilename);
extern void	ProcessPatchTable(void);

extern void	BuildPageTable(int, int, int);


#endif	/* __PATCHTABLE_H */
