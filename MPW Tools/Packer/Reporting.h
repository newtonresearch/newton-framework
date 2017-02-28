/*
	File:		Reporting.h

	Contains:	Functions to report packing progress.

	Written by:	The Newton Tools group.
*/

#ifndef __REPORTING_H
#define __REPORTING_H

void	Progress(const char * inMessage, ...);
void	ErrorMessage(const char * inMessage, ...);
void	WarningMessage(const char * inMessage, ...);
void	ErrorMessageIf(bool inErr, const char * inMessage, ...);
void	WarningMessageIf(bool inErr, const char * inMessage, ...);
void	InternalErrorIf(bool inErr, const char * inMessage, ...);
void	UsageErrorMessage(const char ** inUsage, const char * inMessage, ...);
bool	ReportOSErr(int inOSErr, const char * inMessage, ...);
void	ExitIfOSErr(int inOSErr, const char * inMessage, ...);

extern bool gDoProgress;

#endif /* __REPORTING_H */
