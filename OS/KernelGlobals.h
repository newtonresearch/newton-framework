/*
	File:		KernelGlobals.h

	Contains:	Kernel global data.

	Written by:	Newton Research Group.
*/

#if !defined(__KERNELGLOBALS_H)
#define __KERNELGLOBALS_H 1

#include "Scheduler.h"
#include "KernelPorts.h"

extern CTask *				gIdleTask;				// 0C100FC4
extern CObjectTable *	gObjectTable;			// 0C100FC8 +04
extern CPort *				gNullPort;				// 0C100FCC +08
extern CScheduler *		gKernelScheduler;		// 0C100FD0 +0C
extern bool					gScheduleRequested;	// 0C100FD4 +10
extern int					gHoldScheduleLevel;	// 0C100FD8 +14

extern CTimerEngine *	gTimerEngine;			// 0C100FE0
extern bool					gDoSchedule;			// 0C100FE4 +20

extern CTask *				gCurrentTask;			// 0C100FF8 +34
extern CTask *				gCurrentTimedTask;	// 0C100FFC
extern CTask *				gCurrentMemCountTask;// 0C101000
extern CTask *				gCurrentTaskSaved;	// 0C101004

extern bool					gWantDeferred;			// 0C101028

extern CDoubleQContainer *	gCopyTasks;			// 0C101034	+70
extern CDoubleQContainer *	gBlockedOnMemory;	// 0C101038	+74
extern CDoubleQContainer *	gDeferredSends;	// 0C10103C	+78
extern bool						gCopyDone;			// 0C101040

extern CDoubleQContainer * gTimerDeferred;	// 0C101048	(0x30)

extern int					gRebootProtectCount;	// 0C10104C
extern bool					gWantReboot;			// 0C101050


#endif	/* __KERNELGLOBALS_H */
