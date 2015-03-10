/*
	File:		REP.h

	Contains:	Read-Evaluate-Print functions.

	Written by:	Newton Research Group.
*/

#if !defined(__REP_H)
#define __REP_H

#include "Protocols.h"
#include "NewtonTime.h"

PROTOCOL PInTranslator;
PROTOCOL POutTranslator;

PInTranslator *	InitREPIn(void);
POutTranslator *	InitREPOut(void);
void					ResetREPIdler(void);
void					REPInit(void);
void					REPBreakLoop(void);
void					REPIdle(void);
Timeout				REPTime(void);
void					REPAcceptLine(void);
void					REPStackTrace(void * inStack);
void					REPExceptionNotify(Exception * inException);

#endif	/* __REP_H */
