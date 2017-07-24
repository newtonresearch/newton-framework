/*
	File:		SymbolTable.cc

	Contains:	Symbol table implementation.
					Symbols are plain C function names.

	Written by:	The Newton Tools group.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "SymbolTable.h"
#include "Reporting.h"

/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

extern const char *	gExportFilename[];
extern int				gExportFileCount;
extern const char *	gSymbolFilename;

/*------------------------------------------------------------------------------
	C H a s h T a b l e
------------------------------------------------------------------------------*/

CHashTable::CHashTable(int inSize)
{
	fSize = inSize;
	fTable = (CHashTableEntry **) malloc(inSize * sizeof(CHashTableEntry *));
//	ASSERT(fTable != NULL);
}

unsigned
CHashTable::hash(CHashTableEntry * inEntry)
{
	return ((unsigned long)inEntry >> 8) & (fSize - 1);
}

int
CHashTable::compare(CHashTableEntry * inEntry1, CHashTableEntry * inEntry2)
{
	return 1;
}

void
CHashTable::add(CHashTableEntry * inEntry)
{
	int	index = hash(inEntry);
	inEntry->fLink = fTable[index];
	fTable[index] = inEntry;
}

void
CHashTable::remove(CHashTableEntry * inEntry)
{
	int	index = hash(inEntry);
	CHashTableEntry * entry = fTable[index];
//	ASSERT(entry != NULL);
	if (entry == inEntry) {
		fTable[index] = inEntry->fLink;
	} else {
		CHashTableEntry * prev = fTable[index];
		for (entry = prev->fLink; entry != NULL; prev = entry, entry = entry->fLink) {
			if (entry == inEntry) {
				break;
			}
		}
//		ASSERT(entry != NULL);
		prev->fLink = inEntry->fLink;
	}
}

CHashTableEntry *
CHashTable::lookup(CHashTableEntry * inEntry)
{
	int	index = hash(inEntry);
	for (CHashTableEntry * entry = fTable[index]; entry != NULL; entry = entry->fLink) {
		if (compare(entry, inEntry) == 0) {
			return entry;
		}
	}
	return NULL;
}

CHashTableEntry *
CHashTable::search(CHashTableEntry * inEntry)
{
	for (int index = 0; index < fSize; index++) {
		for (CHashTableEntry * entry = fTable[index]; entry != NULL; entry = entry->fLink) {
			if (compare(entry, inEntry) == 0) {
				return entry;
			}
		}
	}
	return NULL;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C S y m b o l T a b l e
------------------------------------------------------------------------------*/

unsigned
CSymbolTable::hash(CHashTableEntry * inEntry)
{
	unsigned			result = 0;
	const char *	symbol = static_cast<CSymbol*>(inEntry)->fName;
	for (int i = 8; *symbol != 0 && i != 0; symbol++, i--) {
		result = result * 37 + tolower(*symbol);
	}
	return result & (fSize - 1);
}

int
CSymbolTable::compare(CHashTableEntry * inEntry1, CHashTableEntry * inEntry2)
{
	return strcmp(static_cast<CSymbol*>(inEntry1)->fName,
					  static_cast<CSymbol*>(inEntry2)->fName);
}

CSymbol *
CSymbolTable::find(const char * inName)
{
	CSymbol	entry;
	entry.fLink = NULL;
	entry.fName = (char *)inName;
	return static_cast<CSymbol*>(lookup(&entry));
}


#pragma mark -
/*------------------------------------------------------------------------------
	C S y m b o l
------------------------------------------------------------------------------*/

CSymbol::CSymbol(const char * inName, const char * inCName, CSymbol * inLink)
{
	fLink = NULL;
	fNext = NULL;
	fPrev = inLink;
	if (inLink != NULL) {
		inLink->fNext = this;
	}
	fName = (char *)malloc(strlen(inName) + 1);
	strcpy(fName, inName);
	fCName = (char *)malloc(strlen(inCName) + 1);
	strcpy(fCName, inCName);
	fOffset = -1;
	fNumArgs = -1;
}


#pragma mark -
/*------------------------------------------------------------------------------
	Fill the symbol table.
	Args:		inTable				the table to fill
				outFunctionNames	linked list of symbols
	Return:  true if error
------------------------------------------------------------------------------*/

bool
FillSymbolTable(CSymbolTable * inTable, CSymbol ** outFunctionNames, CSymbol ** outLastFunctionNames)
{
	CSymbol *	symbol = NULL;
	size_t		numOfSymbols = 0;
	bool		isVirgin = true, isLastVirgin = true;
	FILE *	expFile;
	int		lineLen;
	int		argCount;
	char		line[300];
	char		name[128];
	char *	fname;
	int		expFileIndex;

	name[0] = '_';
	for (expFileIndex = 0; expFileIndex < gExportFileCount; expFileIndex++) {
		const char *	expFilename = gExportFilename[expFileIndex];
		expFile = fopen(expFilename, "r");
		if (expFile == NULL) {
			ExitWithMessage("can’t open -via file “%s”", expFilename);
		}
		if (expFileIndex == 0) {
			// strip F prefix from C function name to make NewtonSxript function name
			fname = name+2;
		} else {
			fname = name+1;
		}
		while (!feof(expFile)) {
			argCount = -1;
			lineLen = 0;
			line[0] = 0;
			if (fgets(line, 300, expFile) != NULL) {
				if (line[0] != '\n') {
					int	numOfItems = sscanf(line, "%127s%n%i", name+1, &lineLen, &argCount);
					if (lineLen == 255) {
						ExitWithMessage("input line too long in file “%s”", expFilename);
					}
					if (numOfItems > 1 && line[0] != ';') {
						if (strlen(name+1) > 127) {
							ExitWithMessage("encountered function name longer than 127 characters in file “%s”", expFilename);
						}
						symbol = new CSymbol(fname, name, symbol);
						inTable->add(symbol);
						if (numOfItems == 2 && argCount >= 0) {
							symbol->fNumArgs = argCount;
						}
						if (isVirgin) {
							*outFunctionNames = symbol;
							isVirgin = false;
						} else {
							numOfSymbols++;
						}
						if (expFileIndex == gExportFileCount-1 && isLastVirgin) {
							*outLastFunctionNames = symbol;
							isLastVirgin = false;
						}
					}
				}
			}
		}
		fclose(expFile);
		if (isVirgin) {
			WarningMessage("no names found in the function names file “%s”", expFilename);
		}
	}

	return numOfSymbols == 0 && isVirgin;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C P o i n t e r T a b l e
------------------------------------------------------------------------------*/

unsigned
CPointerTable::hash(CHashTableEntry * inEntry)
{
	if (fFindName) {
		unsigned long  result = 0;
		const char *	symbol = static_cast<CPointer*>(inEntry)->fName;
		for (int i = 8; *symbol != 0 && i != 0; symbol++, i--) {
			result = result * 37 + tolower(*symbol);
		}
		return result & (fSize - 1);
	}
	CPointer * symbol = static_cast<CPointer*>(inEntry);
	return symbol->fFuncPtr & (fSize - 1);
}

int
CPointerTable::compare(CHashTableEntry * inEntry1, CHashTableEntry * inEntry2)
{
	if (fFindName) {
		return strcmp(static_cast<CPointer*>(inEntry1)->fName,
						  static_cast<CPointer*>(inEntry2)->fName);
	}
	return static_cast<CPointer*>(inEntry1)->fFuncPtr - static_cast<CPointer*>(inEntry2)->fFuncPtr;
}

CPointer *
CPointerTable::find(const char * inName)
{
	CPointer	entry;
	entry.fLink = NULL;
	entry.fName = (char *)inName;
	fFindName = true;
	return static_cast<CPointer*>(search(&entry));
}

CPointer *
CPointerTable::find(uint32_t inFuncPtr)
{
	CPointer	entry;
	entry.fLink = NULL;
	entry.fFuncPtr = inFuncPtr;
	fFindName = false;
	return static_cast<CPointer*>(lookup(&entry));
}


#pragma mark -
/*------------------------------------------------------------------------------
	C P o i n t e r
------------------------------------------------------------------------------*/

CPointer::CPointer(uint32_t inFuncPtr, char * inName, CPointer * inLink)
{
	fLink = NULL;
	fNext = NULL;
	fPrev = inLink;
	if (inLink != NULL) {
		inLink->fNext = this;
	}
	fName = (char *)malloc(strlen(inName) + 1);
	fFuncPtr = inFuncPtr;
	strcpy(fName, inName);
}


#pragma mark -
/*------------------------------------------------------------------------------
	Fill the symbol table.
	Args:		inTable		the table to fill
	Return:  true if error
------------------------------------------------------------------------------*/

bool
FillSymbolTable(CPointerTable * inTable)
{
	CPointer *	symbol = NULL;
	size_t		count = 0;
	bool		isVirgin = true;
	FILE *	symFile;
	int		lineLen;
	char		line[300];
	char		name[256];
	uint32_t	funcPtr;

	symFile = fopen(gSymbolFilename, "r");
	if (symFile == NULL) {
		ExitWithMessage("can’t open -sym file “%s”", gSymbolFilename);
	}
	while (!feof(symFile)) {
		lineLen = 0;
		line[0] = 0;
		if (fgets(line, 300, symFile) != NULL) {
			if (line[0] != '\n') {
				int	numOfItems = sscanf(line, "%8x%255s%n", &funcPtr, name, &lineLen);
				if (lineLen == 255) {
					ExitWithMessage("input line too long in file “%s”", gSymbolFilename);
				}
				if (numOfItems > 1 && line[0] != ';') {
					if (strlen(name) > 254) {
						ExitWithMessage("encountered function name longer than 254 characters in file “%s”", gSymbolFilename);
					}
					symbol = new CPointer(funcPtr, name, symbol);
					inTable->add(symbol);
					if (isVirgin) {
						isVirgin = false;
					} else {
						count++;
					}
				}
			}
		}
	}
	fclose(symFile);
	if (isVirgin) {
		WarningMessage("no names found in the function names file “%s”", gSymbolFilename);
	}

	return count == 0 && isVirgin;
}

