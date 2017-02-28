/*
	File:		Reporting.h

	Contains:	Functions to report progress.

	Written by:	The Newton Tools group.
*/

#ifndef __REPORTING_H
#define __REPORTING_H 1

struct SourcePos;

extern void	FatalError(const char * inMessage, ...);
extern void	FatalError(SourcePos & inPos, const char * inMessage, ...);
extern void	Warning(SourcePos & inPos, const char * inMessage, ...);

#endif	/* __REPORTING_H */
