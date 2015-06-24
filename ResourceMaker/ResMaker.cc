/*
	File:		ResMaker.cc

	Contains:	Command line tool to generate a Symbol_Defs.h file containing
					symbol length and hash info from the SymbolDefs.h file.

	Written by:	The Newton Tools group, 2007.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>


/*----------------------------------------------------------------------
	Hash a symbol name.
	Args:		inName		C string to be made symbol
	Return:	ULong			the hash value
----------------------------------------------------------------------*/

uint32_t
SymbolHashFunction(const char * inName)
{
	char ch;
	uint32_t sum = 0;

	while ((ch = *inName++) != 0)
		sum += toupper(ch);

	return sum * 0x9E3779B9;	// 2654435769
}


/*------------------------------------------------------------------------------
	main
------------------------------------------------------------------------------*/
#define forNTK 0

int
main(int argc, const char * argv[])
{
	FILE * symFile = fopen("/Users/simon/Projects/newton-framework/Frames/Symbol_Defs.h", "w");

#define DEFSYM(name) \
fprintf(symFile, "DEFSYM(%s, %ld, 0x%08X)\n", #name, strlen(#name)+1, SymbolHashFunction(#name));
#define DEFSYM_(name, sym) \
fprintf(symFile, "DEFSYM_(%s, %s, %ld, 0x%08X)\n", #name, #sym, strlen(sym)+1, SymbolHashFunction(sym));

#include "/Users/simon/Projects/newton-framework/Frames/SymbolDefs.h"

	fclose(symFile);

	return 0;
}
