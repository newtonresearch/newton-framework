/*
	File:		KernelMonitor.cc

	Contains:	Kenel monitor implementation.

	Written by:	Newton Research Group.
*/

#include "KernelMonitor.h"
#include "KernelGlobals.h"
#include "UserTasks.h"
#include "SharedMem.h"
#include "OSErrors.h"

extern     void		Restart(void);
extern "C" NewtonErr	SMemSetBufferKernelGlue(ObjectId inId, void * inBuffer, size_t inSize, ULong inPermissions);
extern "C" NewtonErr	LowLevelCopyDoneFromKernelGlue(NewtonErr inErr, CTask * inTask, VAddr inReturn);

ULong gMonitorTaskPriority;	// 0C101534


/*------------------------------------------------------------------------------
	C M o n i t o r

	A monitor owns its own CTask which runs when appropriate.

	Assembler glue must have, erm, assembler linkage.
------------------------------------------------------------------------------*/

extern "C"
{
	NewtonErr	MonitorEntryGlue(CTask * inContext, int inSelector, void * inData, ProcPtr inProc);
	NewtonErr	MonitorDispatchKernelGlue(void);
	NewtonErr	MonitorExitKernelGlue(NewtonErr inMonitorResult);
	NewtonErr	MonitorThrowKernelGlue(char * inName, void * inData, void * inDestructor);
	NewtonErr	MonitorFlushKernelGlue(ObjectId inId);
}


/*------------------------------------------------------------------------------
	Constructor.
	Set up an empty queue of tasks.
------------------------------------------------------------------------------*/

CMonitor::CMonitor()
	:	fQueueCount(0),
		fQueue(offsetof(CTask, fMonitorQItem), MemberFunctionCast(DestructorProcPtr, this, &CMonitor::deleteTaskOnMonitorQ), this)
{
	fProcContext = NULL;
	fCaller = NULL;
	fProc = NULL;
	fMsgId = kNoId;
	fSuspended = 0;
	fMonitorTaskId = kNoId;
	fIsCopying = NO;
}


/*------------------------------------------------------------------------------
	Destructor.
	Suspend any tasks waiting in the queue.
	Remove our message and monitor task from the object table.
------------------------------------------------------------------------------*/

CMonitor::~CMonitor()
{
	suspend(1);
	gObjectTable->remove(fMsgId);
	gObjectTable->remove(fMonitorTaskId);
}


/*------------------------------------------------------------------------------
	Initialize the monitor.
	Create our own monitor task and init it to run MonitorEntryGlue.
	NOTE	monitor procs expect a monitor id as the first arg, as opposed to
			task procs which expect a CTask * context.
	Args:		inProc					routine to be executed in monitor
				inStackSize
				inContext				context in which code runs
				inEnvironment
				inFaultMonitor			YES => monitor is a fault monitor
				inName
				inRebootProtected		YES => Reboot() should wait for monitor to complete
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CMonitor::init(MonitorProcPtr inProc, size_t inStackSize, void * inContext, CEnvironment * inEnvironment, bool inFaultMonitor, ULong inName, bool inRebootProtected)
{
	NewtonErr err = noErr;

	XTRY
	{
		fIsFaultMonitor = inFaultMonitor;
		fRebootProtected = inRebootProtected;
		fProc = inProc;

		// create the monitor task
		XFAILNOT(fMonitorTask = new CTask, err = kOSErrCouldNotCreateObject;)

		// add it to the object table
		fMonitorTaskId = gObjectTable->add(fMonitorTask, kTaskType, kSystemId);

		// initialize it
		// NOTE that MonitorEntryGlue is nothing like a TaskProcPtr,
		// but the task registers will be adjusted in CMonitor::setUpEntry
		XFAILIF(fMonitorTask->init((TaskProcPtr) MonitorEntryGlue, inStackSize, fMonitorTaskId, kNoId, gMonitorTaskPriority, inName, inEnvironment) != noErr, err = kOSErrCouldNotCreateObject;)

		fMonitorTask->fMonitorId = fId;
		fProcContext = inContext;

		// create a shared memory message
		CSharedMemMsg * msg;
		XFAILNOT(msg = new CSharedMemMsg, err = kOSErrCouldNotCreateObject;)
		XFAILIF(msg->init(inEnvironment) != noErr, delete msg; msg = NULL; err = kOSErrCouldNotCreateObject;)
		RegisterObject(msg, kSharedMemMsgType, fId, &fMsgId);
		XFAILIF(fMsgId == kNoId, err = kOSErrCouldNotCreateObject;)
	}
	XENDTRY;

	return err;
}


/*------------------------------------------------------------------------------
	Acquire the processor to run the monitor.
	ie unschedule whatever task weÕre currently running and schedule this task.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CMonitor::acquire(void)
{
	NewtonErr err = noErr;

	XTRY
	{
		XFAILIF((fSuspended & 3) != 0, err = kOSErrNoSuchMonitor;)

		CTask *	currentTask = gCurrentTask;		// must hold a local copyÉ
		if ((currentTask->fState & 0x00800000) == 0
		&&  currentTask->fRegister[kMonSelector] == (ULong)kSuspendMonitor)
		{
			XFAILIF(owner() != currentTask->owner(), err = kOSErrObjectNotOwnedByTask;)
			suspend(2);
		}
		UnScheduleTask(currentTask);					// Ébecause this nils out the global
		if (++fQueueCount == 1)
		{
			if (!setUpEntry(currentTask))				// set up monitor task
				ScheduleTask(currentTask);				// reschedule current task after that
		}
		else
			fQueue.add(currentTask);
	}
	XENDTRY;

	return err;
}


/*------------------------------------------------------------------------------
	Suspend the monitor.
	Args:		inFlags					new suspension state
	Return:	YES	if no calling task
------------------------------------------------------------------------------*/

bool
CMonitor::suspend(ULong inFlags)
{
	fSuspended |= inFlags;
	if (fCaller)
	{
		CTask *	aTask;
		while ((aTask = (CTask *)fQueue.remove()) != NULL)
		{
			setResult(aTask, kOSErrNoSuchMonitor);
			ScheduleTask(aTask);
			--fQueueCount;
		}
		return NO;
	}
	return YES;
}


/*------------------------------------------------------------------------------
	Release the monitor.
	Args:		inResult
	Return:	--
------------------------------------------------------------------------------*/

void
CMonitor::release(NewtonErr inResult)
{
	// if we were waiting for monitor to finish, we can reboot now
	EnterAtomic();
	if (fRebootProtected && --gRebootProtectCount == 0 && gWantReboot)
		Restart();
	ExitAtomic();

	// decide what to do with the task now itÕs done
	unsigned long pc = fCaller->fRegister[kcThePC];
	if ((fCaller->fState & 0x00400000) != 0
	 || (fCaller->fState & 0x00000002) != 0)
		pc = (VAddr) TaskKillSelf;
	else if ((fCaller->fState & 0x00800000) == 0)
	{
		gKernelScheduler->setCurrentTask(fCaller);
		ScheduleTask(fCaller);
		setResult(fCaller, inResult);
	}
	else if (inResult == 4)
		pc = (VAddr) TaskKillSelf;
	else if (inResult == 5)
		gBlockedOnMemory->add(fCaller);
	else
	{
		gKernelScheduler->setCurrentTask(fCaller);
		ScheduleTask(fCaller);
		setResult(fCaller, inResult);
	}

	fCaller->fRegister[kcThePC] = pc;
	fCaller->fMonitor = kNoId;
	--fQueueCount;
	UnScheduleTask(fMonitorTask);
	fCaller = NULL;

	// if not suspended, set up monitor entry for next task in queue
	if ((fSuspended & 1) == 0)
	{
		CTask *  next = (CTask *)fQueue.remove();
		if (next)
			setUpEntry(next);
	}
}


/*------------------------------------------------------------------------------
	Set up our task for entry to the monitor.
	Copy registers from given task and schedule it. We actually run
	MonitorEntryGlue which calls a CTask member function with the right args.
	Args:		inTask					task to be executed in monitor
	Return:	YES	if monitor task is ready to run
------------------------------------------------------------------------------*/

bool
CMonitor::setUpEntry(CTask * inTask)
{
	if ((inTask->fState & 0x00800000) != 0)
	{
		SMemSetBufferKernelGlue(fMsgId, inTask->fRegister, sizeof(inTask->fRegister), 0x01);	// 0x01 == kSMemReadOnly?
		fMonitorTask->fRegister[kMonSelector] = (ULong) kMonitorFaultSelector;
	}
	else if (inTask->fRegister[kMonSelector] == (ULong) kSuspendMonitor)
	{
		setResult(inTask, noErr);
		--fQueueCount;
		return NO;
	}
	else
		fMonitorTask->fRegister[kMonSelector] = inTask->fRegister[kMonSelector];

	EnterAtomic();
	if (gWantReboot)
		// donÕt schedule anything new
		ExitAtomic();

	else
	{
		if (fRebootProtected)
			gRebootProtectCount++;	// one more task to wait for
		ExitAtomic();
		fCaller = inTask;
		inTask->fMonitor = *this;
		fIsCopying = (inTask->fCopySavedMemId != kNoId);
		fMonitorTask->fRegister[kcThePC] = (VAddr) MonitorEntryGlue;
		fMonitorTask->fRegister[kMonThePC] = (VAddr) fProc;
		fMonitorTask->fRegister[kMonUserRefCon] = inTask->fRegister[kMonUserRefCon];
	//	fMonitorTask->fRegister[kMonSelector] already set up above
		fMonitorTask->fRegister[kMonContext] = (VAddr) fProcContext;
	//	fMonitorTask->fRegister[11] = inTask->fRegister[11];	// huh?
		fMonitorTask->fRegister[kcTheStack] = fMonitorTask->fTaskData;	// top of stack is bottom of data

#if defined(__i386__)
		// args are passed on the stack
		unsigned long * sp = (unsigned long *)fMonitorTask->fTaskData;
		fMonitorTask->fRegister[kcTheFrame] = (VAddr)sp;
		*--sp = fMonitorTask->fRegister[kMonThePC];
		*--sp = fMonitorTask->fRegister[kMonUserRefCon];
		*--sp = fMonitorTask->fRegister[kMonSelector];
		*--sp = fMonitorTask->fRegister[kMonContext];
//------- parameter area must be 16-byte aligned
		*--sp = 0;	// return address will be filled in later
		fMonitorTask->fRegister[kcTheStack] = (VAddr)sp;

#elif defined(__x86_64__)
		// args are passed in registers as per ARM
		// but we need a return address on the stack, 16-byte aligned as per i386
		unsigned long * sp = (unsigned long *)fMonitorTask->fTaskData;
		fMonitorTask->fRegister[kcTheFrame] = (VAddr)sp;
		*--sp = 0;	// return address will be filled in later
		fMonitorTask->fRegister[kcTheStack] = (VAddr)sp;
#endif

		fMonitorTask->fCurrentTask = inTask;
		ScheduleTask(fMonitorTask);
	}

	return YES;
}


/*------------------------------------------------------------------------------
	Set the given register value in the caller task.
	Args:		inRegNum		the register number
				inValue		its value
	Return:	--
------------------------------------------------------------------------------*/

void
CMonitor::setCallerRegister(ArrayIndex inRegNum, unsigned long inValue)
{
	if (fCaller != NULL)
		fCaller->fRegister[inRegNum] = inValue;
}


/*------------------------------------------------------------------------------
	Set the result code of the given task.
	Args:		inTask		the task
				inResult		the result
	Return:	--
------------------------------------------------------------------------------*/

void
CMonitor::setResult(CTask * inTask, NewtonErr inResult)
{
	if ((inTask->fState & 0x00800000) == 0
	 || inResult != noErr)
		inTask->fRegister[kResultReg] = inResult;
}


/*------------------------------------------------------------------------------
	Schedule all the tasks on the queue and remove them from the queue.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CMonitor::flushTasksOnMonitor(void)
{
	CTask *	aTask;
	while ((aTask = (CTask *)fQueue.remove()) != NULL)
	{
		--fQueueCount;
		ScheduleTask(aTask);
	}
}


/*------------------------------------------------------------------------------
	Delete the task on the end of the queue.
	All we need do is decrement the count of queued tasks.
	Args:		inTask		the task to be deleted
	Return:	--
------------------------------------------------------------------------------*/

void
CMonitor::deleteTaskOnMonitorQ(CTask * inTask)
{
	EnterAtomic();
	--fQueueCount;
	ExitAtomic();
}

#pragma mark -

/*------------------------------------------------------------------------------
	Monitor dispatch callback from SWI.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
MonitorDispatchKernelGlue(void)
{
	NewtonErr	err;
	CMonitor *	monitor;
	if ((err = ConvertIdToObj(kMonitorType, gCurrentTask->fRegister[kMonMonId], &monitor)) == noErr)
	{
		gCurrentTask->fState &= ~0x00800000; 
		monitor->acquire();
	}
	return err;
}


/*------------------------------------------------------------------------------
	Monitor exit callback from SWI.
	Args:		inMonitorResult
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
MonitorExitKernelGlue(NewtonErr inMonitorResult)
{
	NewtonErr	err;
	CMonitor *	monitor;
	if ((err = ConvertIdToObj(kMonitorType, gCurrentMonitorId, &monitor)) == noErr)
		monitor->release(inMonitorResult);
	return err;
}


/*------------------------------------------------------------------------------
	Monitor throw callback from SWI.
	Args:		inName				exception name
				inData				exception data
				inDestructor		destructor
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
MonitorThrowKernelGlue(char * inName, void * inData, void * inDestructor)
{
	NewtonErr	err;
	CMonitor *	monitor;
	if ((err = ConvertIdToObj(kMonitorType, gCurrentMonitorId, &monitor)) == noErr)
	{
		if (monitor->isCopying())
		{
			LowLevelCopyDoneFromKernelGlue(kOSErrUnResolvedFault, monitor->caller(), monitor->caller()->fRegister[kcThePC]);
			monitor->release(kOSErrUnResolvedFault);
		}
		else if ((monitor->caller()->fPSR & kModeMask) == kUserMode)
		{
		//	MUST be in super mode
			monitor->setCallerRegister(kParm1, (unsigned long) inData);			// need to stack these for __i386__
			monitor->setCallerRegister(kParm2, (unsigned long) inDestructor);
			monitor->setCallerRegister(kcThePC, (unsigned long) Throw);
			monitor->release((NewtonErr)(unsigned long)inName);	// huh?? unknown Newton error?
		}
		else
			Reboot(kOSErrSorrySystemError);
	}
	return err;
}


/*------------------------------------------------------------------------------
	Monitor flush callback from SWI.
	Args:		inId
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
MonitorFlushKernelGlue(ObjectId inId)
{
	NewtonErr	err;
	CMonitor *	monitor;
	if ((err = ConvertIdToObj(kMonitorType, inId, &monitor)) == noErr)
		monitor->flushTasksOnMonitor();
	return err;
}

