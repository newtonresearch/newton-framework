/*
	File:		Reporting.cc

	Contains:	Functions to report progress.

	Written by:	The Newton Tools group.
*/

#include "Reporting.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

extern bool gDoProgress;
extern const char * gCmdName;

/*------------------------------------------------------------------------------
	Reporting.
------------------------------------------------------------------------------*/

void
Progress(const char * inMessage, ...)
{
	if (gDoProgress)
	{
		va_list  ap;
		va_start(ap, inMessage);
		fputs("# ", stderr);
		vfprintf(stderr, inMessage, ap);
		fputs(".\n", stderr);
		va_end(ap);
	}
}

void
vgoof(const char * inLevel, const char * inMessage, va_list ap)
{
	fprintf(stderr,  "### %s - ", gCmdName);
	fprintf(stderr,  "%s - ", inLevel);
	vfprintf(stderr, inMessage, ap);
	fputs(".\n", stderr);
}

void
WarningMessage(const char * inMessage, ...)
{
	va_list  ap;
	va_start(ap, inMessage);
	vgoof("warning", inMessage, ap);
	va_end(ap);
}

void
ErrorMessage(const char * inMessage, ...)
{
	va_list  ap;
	va_start(ap, inMessage);
	vgoof("error", inMessage, ap);
	va_end(ap);
}

void
UsageExitWithMessage(const char * inMessage, ...)
{
	va_list  ap;
	va_start(ap, inMessage);
	fputs("# ", stderr);
	vfprintf(stderr, inMessage, ap);
	fputs(".\n", stderr);
	va_end(ap);
	exit(1);
}

void
ExitWithMessage(const char * inMessage, ...)
{
	va_list  ap;
	va_start(ap, inMessage);
	fputs("# ", stderr);
	vfprintf(stderr, inMessage, ap);
	fputs(".\n", stderr);
	va_end(ap);
	exit(2);
}

