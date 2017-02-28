/*
	File:		Reporting.cc

	Contains:	Functions to report progress.

	Written by:	The Newton Tools group.
*/

#include "Reporting.h"
#include "Lexer.h"


void
FatalError(const char * inMessage, ...)
{
	if (gLex != NULL) {
		cerr << gLex->position() << " ";
	}
	cerr << "# Error: ";

	va_list  ap;
	va_start(ap, inMessage);
	vfprintf(stderr, inMessage, ap);
	va_end(ap);

	fprintf(stderr, "\n");

	exit(1);
}


void
FatalError(SourcePos & inPos, const char * inMessage, ...)
{
	cerr << inPos << " # Error: ";

	va_list  ap;
	va_start(ap, inMessage);
	vfprintf(stderr, inMessage, ap);
	va_end(ap);

	fprintf(stderr, "\n");

	exit(1);
}


void
Warning(SourcePos & inPos, const char * inMessage, ...)
{
	cerr << inPos << " # Warning: ";

	va_list  ap;
	va_start(ap, inMessage);
	vfprintf(stderr, inMessage, ap);
	va_end(ap);

	fprintf(stderr, "\n");

	exit(1);
}

