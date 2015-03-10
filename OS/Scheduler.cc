/*
	File:		Scheduler.cc

	Contains:	Task scheduler implementation.

	Written by:	Newton Research Group.
*/

#include "Scheduler.h"
#include "KernelGlobals.h"
#include "Semaphore.h"


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

extern bool			gCountTaskTime;
extern ArrayIndex	gHandlesUsed;
extern ArrayIndex	gPtrsUsed;
extern ArrayIndex	gSavedHandlesUsed;
extern ArrayIndex	gSavedPtrsUsed;
extern ULong		gFIQInterruptOverhead;
extern ULong		gIRQInterruptOverhead;

extern CTime	gDeadTaskTime;
extern CTime	gFirstTaskEndTime;
extern CTime	gLastTaskEndTime;

CScheduler *	gKernelScheduler;			// 0C100FD0
bool				gScheduleRequested;
int				gHoldScheduleLevel;		// nested scheduler hold count
bool				gDoSchedule;				// 0C100FE4
CTask *			gCurrentTimedTask = NULL;		// 0C100FFC
CTask *			gCurrentMemCountTask = NULL;	// 0C101000
CTime				gTaskTimeStart;			// 0C101020
CTime				gDeadTaskTime;				// 0C104EB0
CTime				gFirstTaskEndTime;		// 0C104EB8
CTime				gLastTaskEndTime;			// 0C104EC0

int				gTaskPriority;				// 0C101980
//CSharedMsg	gPreemptSchedMsg;			// 0C101984
bool				gWantSchedulerToRun;		// 0C101A2C +AC
bool				gSchedulerRunning;		// 0C101A30 +B0
ULong				gNumberOfTaskSwaps;		// 0C101A34 +B4
bool				gTaskEndTimeInvalid;
ULong				gIRQAccumulatedIntOverhead;
ULong				gFIQAccumulatedIntOverhead;


/*--------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
--------------------------------------------------------------------------------*/

void
StartScheduler(void)
{
	if (!gSchedulerRunning)
	{
		DisableInterrupt(gSchedulerIntObj);
		g0F182C00 = g0F181800 + 20 * kMilliseconds;
		gSchedulerRunning = YES;
		QuickEnableInterrupt(gSchedulerIntObj);
	}
	gWantSchedulerToRun = NO;
}

void
StopScheduler(void)
{
	if (gSchedulerRunning)
		DisableInterrupt(gSchedulerIntObj);
	gSchedulerRunning = NO;
	gWantSchedulerToRun = NO;
}


CTask *
Scheduler(void)
{
	CTask *	task = gKernelScheduler->schedule();

	if (task != gCurrentMemCountTask)
	{
	//	we want some memory usage stats
		if (gCurrentMemCountTask != NULL)
		{
			long	memUsed;
			gCurrentMemCountTask->fPtrsUsed += (gPtrsUsed - gSavedPtrsUsed);
			gCurrentMemCountTask->fHandlesUsed += (gHandlesUsed - gSavedHandlesUsed);
			memUsed = gCurrentMemCountTask->fPtrsUsed + gCurrentMemCountTask->fHandlesUsed;
			if (memUsed > gCurrentMemCountTask->fMemUsed)	// actually looks like <, but whereÕs the sense in that?
				gCurrentMemCountTask->fMemUsed = memUsed;
		}
		gSavedHandlesUsed = gHandlesUsed;
		gSavedPtrsUsed = gPtrsUsed;
		gCurrentMemCountTask = task;
		gNumberOfTaskSwaps++;
	}

	if (gCountTaskTime)
	{
		if (task != gCurrentTimedTask)
		{
		//	we want some processor usage stats
			CTime	now = GetGlobalTime();
			gLastTaskEndTime = now;
			if (gTaskEndTimeInvalid)
			{
				gFirstTaskEndTime = now;
				gTaskEndTimeInvalid = NO;
			}
			else if (gCurrentTimedTask != NULL)
			{
				CTime	taskTime;
				CTime	taskOverhead;
				ULong	fiqOverhead = Swap(&gFIQInterruptOverhead, 0);
				ULong	irqOverhead = Swap(&gIRQInterruptOverhead, 0);
				gFIQAccumulatedIntOverhead += fiqOverhead;
				gIRQAccumulatedIntOverhead += irqOverhead;
				taskOverhead = fiqOverhead + irqOverhead;
				taskTime = now - gTaskTimeStart - taskOverhead;
				gCurrentTimedTask->fTaskTime = gCurrentTimedTask->fTaskTime + taskTime;
			}
			gCurrentTimedTask = task;
			gTaskTimeStart = now;
		}
	}

	return task;
}


/* -------------------------------------------------------------------------------
	There is a dedicated scheduler timer interrupt.
	This handles it.
------------------------------------------------------------------------------- */
static bool gIsPreemptiveTimerInterrupt = NO;	// for DEBUG logging

void
PreemptiveTimerInterruptHandler(void * inQueue)
{
gIsPreemptiveTimerInterrupt = YES;
	WantSchedule();
	gKernelScheduler->setCurrentTask(NULL);
	g0F182C00 = g0F181800 + 20*kMilliseconds;
}


void
WantSchedule(void)
{
	if (gHoldScheduleLevel == 0)
		gDoSchedule = YES;
	else
		gScheduleRequested = YES;
}


void
HoldSchedule(void)
{
	gHoldScheduleLevel++;
}


void
AllowSchedule(void)
{
	EnterAtomic();
	if (--gHoldScheduleLevel == 0 && gScheduleRequested)
	{
		gScheduleRequested = NO;
		gDoSchedule = YES;		
	}
	ExitAtomic();
}


void
ScheduleTask(CTask * inTask)
{
	gKernelScheduler->addWhenNotCurrent(inTask);
}


void
UnScheduleTask(CTask * inTask)
{
	gKernelScheduler->remove(inTask);
}


void
PauseSystem(void)
{
	GenericSWI(kPauseSystem);
}


void
PowerOffSystem(void)
{
	GenericSWI(kPowerOffSystem);
}


void
SleepTask(void)
{
	for ( ; ; )
		PauseSystem();
}


#pragma mark -
/*------------------------------------------------------------------------------
	C S c h e d u l e r
------------------------------------------------------------------------------*/

CScheduler::CScheduler()
{
	fHighestPriority = 0;
	fPriorityMask = 0;
	fCurrentTask = NULL;
}


/*------------------------------------------------------------------------------
	Schedule the next task.
	Args:		--
	Return:	the task to run
------------------------------------------------------------------------------*/

CTask *
CScheduler::schedule(void)
{
	if (gCurrentTask != NULL
	&&  gCurrentTask != gIdleTask)
	//	Add current task back onto end of queue
		add(gCurrentTask);
	gDoSchedule = NO;

//	Prepare to run the next highest priority task
	CTask *	task = removeHighestPriority();

//	If there are no tasks to be scheduled, stop the scheduler
	if (task == NULL)
	{
		StopScheduler();
		task = gIdleTask;
	}

/*if (gIsPreemptiveTimerInterrupt)
{
	gIsPreemptiveTimerInterrupt = NO;
	printf("CScheduler::schedule() -> %p %c%c%c%c\n", task, task->fName>>24, task->fName>>16, task->fName>>8, task->fName);
}*/
//	Update the global task priority
	gTaskPriority = task->fPriority;
	return task;
}


/*------------------------------------------------------------------------------
	Add a task to be scheduled.
	Args:		inTask		the task to schedule
	Return:	--
------------------------------------------------------------------------------*/

void
CScheduler::add(CTask * inTask)
{
//	Update the priority mask
	ArrayIndex	priority = inTask->fPriority;
	fPriorityMask |= BIT(priority);
	fHighestPriority = MAX(fHighestPriority, priority);

//	Add the task to its queue (according to priority)
	fTasks[priority].add(inTask, 0x00020000, this);

//	If weÕre only idling, schedule the task we just added
	if (gCurrentTask == gIdleTask)
		WantSchedule();

//	May need to reprioritize
	if (priority >= gTaskPriority)
		gWantSchedulerToRun = YES;
}


/*------------------------------------------------------------------------------
	Add a task to be scheduled IFF itÕs not already the current task.
	Args:		inTask		the task to schedule
	Return:	--
------------------------------------------------------------------------------*/

void
CScheduler::addWhenNotCurrent(CTask * inTask)
{
	if (inTask != gCurrentTask)
		add(inTask);
}


/*------------------------------------------------------------------------------
	Remove a task from the scheduler.
	Args:		inTask		the task to be removed
	Return:	--
------------------------------------------------------------------------------*/

void
CScheduler::remove(CTask * inTask)
{
	if (inTask != NULL)
	{
	//	If weÕre removing the current task we just need to reschedule
		if (inTask == gCurrentTask)
		{
//printf("CScheduler::remove(task=%p)\n", inTask);
			gCurrentTask = NULL;
			WantSchedule();
			gWantSchedulerToRun = YES;
		}
		else
		{
		//	Remove the task from its queue (according to priority)
			ArrayIndex priority = inTask->fPriority;
			fTasks[priority].removeFromQueue(inTask, 0x00020000);

		//	If there are no more tasks at this priority, clear the priority mask accordingly
			if (fTasks[priority].peek() == NULL)
			{
				fPriorityMask &= ~BIT(priority);
				if (priority == fHighestPriority)
					updateCurrentBucket();
			}
		}
	}
}


/*------------------------------------------------------------------------------
	Remove the highest priority task from the scheduler.
	Args:		--
	Return:	the task removed
------------------------------------------------------------------------------*/

CTask *
CScheduler::removeHighestPriority(void)
{
	CTask *	task;

	if ((task = fCurrentTask) != NULL)
	{
		fCurrentTask = NULL;
		ArrayIndex priority = task->fPriority;
		if (priority == fHighestPriority)
		{
			if (fTasks[priority].removeFromQueue(task, 0x00020000))
			{
				gWantSchedulerToRun = YES;
				if (fTasks[priority].peek() == NULL)
				{
					fPriorityMask &= ~BIT(priority);
					updateCurrentBucket();
					StopScheduler();
				}
				return task;
			}
		}
	}

	if ((task = fTasks[fHighestPriority].peek()) != NULL)
	{
		fTasks[fHighestPriority].removeFromQueue(task, 0x00020000);
		if (fTasks[fHighestPriority].peek() == NULL)
		{
			fPriorityMask &= ~BIT(fHighestPriority);
			updateCurrentBucket();
			StopScheduler();
		}
	}

	return task;
}


/*------------------------------------------------------------------------------
	Update the highest priority task available.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CScheduler::updateCurrentBucket(void)
{
	if (fHighestPriority > 0)
	{
		for (ArrayIndex i = fHighestPriority - 1; i > 0; i--)
		{
			if (fPriorityMask & BIT(i))
			{
				fHighestPriority = i;
				return;
			}
		}
		fHighestPriority = 0;
	}
}

