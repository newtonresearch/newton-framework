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

unsigned
SymbolHashFunction(const char * inName)
{
	char ch;
	unsigned sum = 0;

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
/*
#define BYTE_SWAP_LONG(n) (((n << 24) & 0xFF000000) | ((n <<  8) & 0x00FF0000) | ((n >>  8) & 0x0000FF00) | ((n >> 24) & 0x000000FF))
	// open newton.rom source
	FILE * srcFile = fopen("/Users/simon/Library/Application Support/Einstein Platform/newton.rom", "r");
	// open newton-x.rom destination
	FILE * dstFile = fopen("/Users/simon/Library/Application Support/Einstein Platform/newton-x.rom", "w");
	// for 8MB BYTE_SWAP_LONG > newton-x.rom
	unsigned long buf[2048];	// 8KB
	unsigned long offset = 0;
	for (int i = 0; i < 1024; i++)
	{
		fread(buf, 8192, 1, srcFile);
		for (int j = 0; j < 2048; j++)
			buf[j] = BYTE_SWAP_LONG(buf[j]);
		fwrite(buf, 8192, 1, dstFile);
	}
	// close ’em
	fclose(dstFile);
	fclose(srcFile);
	return 0;
*/

	FILE * symFile = fopen("/Users/simon/Projects/Newton/Frames/Symbol_Defs.h", "w");

#define DEFSYM(name) \
fprintf(symFile, "DEFSYM(%s, %ld, 0x%08X)\n", #name, strlen(#name)+1, SymbolHashFunction(#name));
#define DEFSYM_(name, sym) \
fprintf(symFile, "DEFSYM_(%s, %s, %ld, 0x%08X)\n", #name, #sym, strlen(sym)+1, SymbolHashFunction(sym));

#include "/Users/simon/Projects/Newton/Frames/SymbolDefs.h"

	fclose(symFile);

	return 0;
}
