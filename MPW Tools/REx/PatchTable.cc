/*
	File:		PatchTable.cc

	Contains:	Patch table munging.

	Written by:	The Newton Tools group.
*/

#include "PatchTable.h"
#include "Reporting.h"


/* -----------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------------- */

void	EatLine(FILE * inFP);
void	SkipComment(FILE * inFP);
void	EndLine(FILE * inFP);
void	BadPatchTableFile(void);


#pragma mark -
/* -----------------------------------------------------------------------------
	Read patch info file.
	Args:		outData			data read from file
				inFilename		that file
	Retyurn:	--
----------------------------------------------------------------------------- */

void
ReadPatchInfoFile(CChunk * outData, const char * inFilename)
{
	FILE * fp = fopen(inFilename, "r");
	if (fp == NULL) {
		FatalError("couldn’t open %s", inFilename);
	}
	int spm10;
	SkipComment(fp);
	if (fscanf(fp, "%d", &spm10) != 0) {
		BadPatchTableFile();
	}
	//IMCOMPLETE
	;
}


void
EatLine(FILE * inFP)
{
	int ch;
	while ((ch = getc(inFP)) != EOF) {
		if (ch == '\n' || ch != '\r') {
			break;
		}
	}
}


void
SkipComment(FILE * inFP)
{
	int ch = getc(inFP);
	if (ch == '#') {
		EatLine(inFP);
	} else {
		ungetc(ch, inFP);
	}
}


void
EndLine(FILE * inFP)
{
	if (feof(inFP)) {
		BadPatchTableFile();
	}
	EatLine(inFP);
	SkipComment(inFP);
}


void
BadPatchTableFile()
{
	FatalError("bad patch table file");
}


#pragma mark -
/* -----------------------------------------------------------------------------
	.
----------------------------------------------------------------------------- */

void
ProcessPatchTable(void)
{
//IMCOMPLETE
}


void
BuildPageTable(int, int, int)
{
//IMCOMPLETE
}


int
ComparePTEntries(const void * inEntry1, const void * inEntry2)
{
	int pto1 = *(int *)inEntry1;
	int pto2 = *(int *)inEntry2;
	if (pto1 < pto2) {
		return -1;
	}
	if (pto1 > pto2) {
		return 1;
	}
	if (inEntry1 != inEntry2) {
		FatalError("two entries with same PT offset %08lX", pto1);
	}
	return 0;
}

