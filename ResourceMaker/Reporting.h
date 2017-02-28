/*
	File:		Reporting.h

	Contains:	Functions to report resource making progress.

	Written by:	The Newton Tools group.
*/

#ifndef __REPORTING_H
#define __REPORTING_H 1

void	Progress(const char * inMessage, ...);
void	ErrorMessage(const char * inMessage, ...);
void	WarningMessage(const char * inMessage, ...);
void	ErrorMessage(const char * inMessage, ...);
void	UsageExitWithMessage(const char * inMessage, ...);
void	ExitWithMessage(const char * inMessage, ...);

extern bool gDoProgress;

#endif /* __REPORTING_H */
