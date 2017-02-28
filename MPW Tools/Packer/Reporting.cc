/*
	File:		Reporting.cc

	Contains:	Functions to report packing progress.

	Written by:	The Newton Tools group.
*/

#include "Reporting.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

extern const char * gCmdName;

bool gDoProgress = false;		//-1B2C


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
v_goof(const char * inLevel, const char * inMessage, va_list ap)
{
	fprintf(stderr,  "### %s - ", gCmdName);
	fprintf(stderr,  "%s - ", inLevel);
	vfprintf(stderr, inMessage, ap);
	fputs(".\n", stderr);
}


void
UsageErrorMessage(const char ** inUsage, const char * inMessage, ...)
{
	va_list  ap;
	va_start(ap, inMessage);
	v_goof("bad usage", inMessage, ap);
	va_end(ap);

	if (*inUsage != NULL) {
		fprintf(stderr,  "### %s - ", gCmdName);
		const char * s;
		while ((s = *inUsage++) != NULL) {
			fputs(s, stderr);
		}
	}
	exit(1);
}


void
ErrorMessage(const char * inMessage, ...)
{
	va_list  ap;
	va_start(ap, inMessage);
	v_goof("error", inMessage, ap);
	va_end(ap);
	exit(2);
}


void
ErrorMessageIf(bool inErr, const char * inMessage, ...)
{
	if (inErr) {
		va_list  ap;
		va_start(ap, inMessage);
		v_goof("error", inMessage, ap);
		va_end(ap);
		exit(2);
	}
}


void
InternalErrorIf(bool inErr, const char * inMessage, ...)
{
	if (inErr) {
		va_list  ap;
		va_start(ap, inMessage);
		v_goof("internal error", inMessage, ap);
		va_end(ap);
		exit(3);
	}
}


void
WarningMessage(const char * inMessage, ...)
{
	va_list  ap;
	va_start(ap, inMessage);
	v_goof("warning", inMessage, ap);
	va_end(ap);
}


void
WarningMessageIf(bool inErr, const char * inMessage, ...)
{
	if (inErr) {
		va_list  ap;
		va_start(ap, inMessage);
		v_goof("warning", inMessage, ap);
		va_end(ap);
	}
}


void
v_err_goof(int inErr, const char * inMessage, va_list ap)
{
	char buf[256];
	v_goof("error", inMessage, ap);
	strerror_r(inErr, buf, 256);
	fprintf(stderr, "# %s\n", buf);
}


bool
ReportOSErr(int inOSErr, const char * inMessage, ...)
{
	if (inOSErr != 0) {
		va_list  ap;
		va_start(ap, inMessage);
		v_err_goof(inOSErr, inMessage, ap);
		va_end(ap);
		return true;
	}
	return false;
}


void
ExitIfOSErr(int inOSErr, const char * inMessage, ...)
{
	if (inOSErr != 0) {
		va_list  ap;
		va_start(ap, inMessage);
		v_err_goof(inOSErr, inMessage, ap);
		va_end(ap);
		exit(2);
	}
}
