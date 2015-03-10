/*
	File:		KernelTasks.cc

	Contains:	Kernel task implementation.

	Written by:	Newton Research Group.
*/

#include <time.h>

#include "KernelTasks.h"
#include "KernelGlobals.h"
#include "Scheduler.h"
#include "VirtualMemory.h"
#include "OSErrors.h"

//#if defined(__i386__) || defined(__x86_64__)
// for IA32 & LP64 ABI, stack must be aligned at 16-byte boundary on function entry
// we’ll just always use that anyway
#define kABIStackAlignment	16
//#endif

// fState is declared as KernelObjectState -
// maybe should be just ULong flags:
#define kMemIsVirtual	0x02000000

/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

extern bool		gOSIsRunning;

extern CTime	gTaskTimeStart;
extern CTime	gDeadTaskTime;

CTask *			gIdleTask = NULL;
CTask *			gCurrentTask = NULL;
CTask *			gCurrentTaskSaved = NULL;


/* -----------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
----------------------------------------------------------------------------- */

extern "C" void
LogSwapOut(CTask * inTask)
{
	ULong taskName = inTask->fName;
	char c1, c2, c3, c4;
	c4 = taskName & 0xFF;  taskName >>= 8;
	c3 = taskName & 0xFF;  taskName >>= 8;
	c2 = taskName & 0xFF;  taskName >>= 8;
	c1 = taskName & 0xFF;
	printf("swapping out #%p %c%c%c%c\n", inTask, c1, c2, c3, c4);
}

extern "C" void
LogSwapIn(CTask * inTask)
{
	ULong taskName = inTask->fName;
	char c1, c2, c3, c4;
	c4 = taskName & 0xFF;  taskName >>= 8;
	c3 = taskName & 0xFF;  taskName >>= 8;
	c2 = taskName & 0xFF;  taskName >>= 8;
	c1 = taskName & 0xFF;
	printf("swapping in  #%p %c%c%c%c\n", inTask, c1, c2, c3, c4);
}


// Get a pointer to the object you passed to the task
// which constitutes the "task global space".

void *
GetGlobals(void)
{
	return gCurrentGlobals;
}


void
SwapInGlobals(CTask * inTask)
{
	gCurrentTaskId = *inTask;
	gCurrentGlobals = inTask->fGlobals;
	gCurrentMonitorId = inTask->fMonitorId;
}


void
InitializeExceptionGlobals(ExceptionGlobals * inGlobals)
{
	inGlobals->firstCatch = NULL;
}


/*------------------------------------------------------------------------------
	Kill this task.
	Args:		--
	Return:	--		should never return!
------------------------------------------------------------------------------*/

void
TaskKillSelf(void)
{
	MonitorDispatchSWI(GetPortSWI(kGetObjectPort), kKill, (OpaqueRef)gCurrentTaskId);
	DebugStr("Task did not kill self properly!!!");
}


/*------------------------------------------------------------------------------
	Give an object to another task.
	Args:		inId			id of the object to be passed
				inToTaskId	id of the new owner (task)
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
TaskGiveObject(ObjectId inId, ObjectId inToTaskId)
{
	return GenericSWI(kTaskGiveObject, inId, inToTaskId);
}

/*------------------------------------------------------------------------------
	kTaskGiveObject generic SWI callback.
	Give an object to another task.
	Args:		inId			id of the object to be passed
				inToTaskId	id of the new owner (task)
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
GiveObject(ObjectId inId, ObjectId inToTaskId)
{
	NewtonErr err = noErr;

	XTRY
	{
		CObject *	obj;
		CTask *		task;
		XFAILNOT(obj = gObjectTable->get(inId), err = kOSErrBadObjectId;)	// object does not exist
		XFAILIF((task = (CTask *)gObjectTable->get(inToTaskId)) != NULL
					&&  ((ObjectId)*task == task->owner()  ||  gObjectTable->exists(task->owner())), err = kOSErrBadObjectId;)	// tasks cannot belong to other tasks
		XFAILIF(obj->owner() != (ObjectId)*gCurrentTask, err = kOSErrObjectNotOwnedByTask;)	// object does not belong to current task
		obj->assignToTask(inToTaskId);
	}
	XENDTRY;

	return err;
}


/*------------------------------------------------------------------------------
	Accept an object from another task.
	Args:		inId			id of the object to be received
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
TaskAcceptObject(ObjectId inId)
{
	return GenericSWI(kTaskAcceptObject, inId);
}

/*------------------------------------------------------------------------------
	kTaskAcceptObject generic SWI callback.
	Accept an object from another task.
	Args:		inId			id of the object to be received
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
AcceptObject(ObjectId inId)
{
	NewtonErr err = noErr;

	XTRY
	{
		CObject * obj;
		XFAILNOT(obj = gObjectTable->get(inId), err = kOSErrBadObjectId;)	// object does not exist
		XFAILIF(obj->assignedOwner() != (ObjectId)*gCurrentTask, err = kOSErrObjectNotAssignedToTask;)	// object is not assigned to current task
		obj->setOwner((ObjectId)*gCurrentTask);
	}
	XENDTRY;

	return err;
}


/*------------------------------------------------------------------------------
	Yield processor to another task.
	Args:		inToTaskId		task to yield to
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
Yield(ObjectId inToTaskId)
{
	return GenericSWI(kTaskYield, inToTaskId);
}


/*------------------------------------------------------------------------------
	Bequeath resources to another task.
	Args:		inToTaskId		task to bequeath to
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
SetBequeathId(ObjectId inToTaskId)
{
	return GenericSWI(kTaskBequeath, inToTaskId);
}


/*------------------------------------------------------------------------------
	Reset task time accounting (for all tasks).
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
ResetAccountTime(void)
{
	return GenericSWI(kTaskResetAccountTime);
}


/*------------------------------------------------------------------------------
	kTaskResetAccountTime generic SWI callback.
	Reset task time accounting (for all tasks).
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
ResetAccountTimeKernelGlue(void)
{
	CTask *  task;
	CObjectTableIterator iter(gObjectTable, kNoId);

	EnterAtomic();
	while ((task = (CTask *)iter.getNextTypedObject(kTaskType)) != NULL)
		task->fTaskTime = 0;
	gTaskTimeStart = 0;
	ExitAtomic();

	return noErr;
}


/*------------------------------------------------------------------------------
	kTaskGetNextId generic SWI callback.
	Return the id of the next task in the object table.
	Args:		inPrevId			prev task id
				outNextId		next task id
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
GetNextTaskIdKernelGlue(ObjectId inPrevId, ObjectId * outNextId)
{
	CObjectTableIterator iter(gObjectTable, kNoId);

	if (iter.setCurrentPosition(inPrevId))
	{
		ObjectId id;
		while ((id = iter.getNextTableId()) != kNoId)
		{
			if (ObjectType(id) == kTaskType)
			{
				*outNextId = id;
				return noErr;
			}
		}
	}

	return kOSErrTaskDoesNotExist;
}


/*------------------------------------------------------------------------------
	Task sleep functions.
	Defined in NewtonTime.h
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Make this task wait.
	Args:		inMilliseconds		time to wait
										WARNING	This function used to expect a tick count
										but has been normalized to expect milliseconds.
	Return:	--
------------------------------------------------------------------------------*/

void
Wait(ULong inMilliseconds)
{
	Sleep(inMilliseconds * kMilliseconds);
}


/*------------------------------------------------------------------------------
	Make this task sleep.
	We do this by sending an empty message to the null port with a timeout
	interval.
	When the timeout expires, control returns (with an error code we ignore).
	NOTE	casting of args to send, to select the right function.
	Args:		inTimeout			time to sleep for; -1 => sleep forever
	Return:	--
------------------------------------------------------------------------------*/

void
Sleep(Timeout inTimeout)
{
	if (inTimeout != 0)
	{
		if (inTimeout == ~0)
			inTimeout = kNoTimeout;
		gUNullPort->send((void *)NULL, (size_t)0, inTimeout);
	}
}


/*------------------------------------------------------------------------------
	Make this task sleep.
	We do this by sending an empty message to the null port with a timeout time.
	Args:		inFutureTime			time to sleep until
	Return:	--
------------------------------------------------------------------------------*/

void
SleepTill(CTime * inFutureTime)
{
	gUNullPort->sendForSleepTill(inFutureTime);
}

#pragma mark -

/*------------------------------------------------------------------------------
	C T a s k

	The kernel task class.
	(As opposed to CUTask, the user task class.)
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Constructor.
------------------------------------------------------------------------------*/

CTask::CTask()
{
	fMonitor = kNoId;
	fCopySavedPC = 0;
	fCopySavedErr = 0;
	fCopiedSize = 0;
	fCopySavedMemId = kNoId;
	fCopySavedMemMsgId = kNoId;
	fSharedMem = kNoId;
	fSharedMemMsg = kNoId;
	fGlobals = NULL;
	fTaskTime = 0;
	fTaskDataSize = 0;
	fPtrsUsed = 0;
	fHandlesUsed = 0;
	fMemUsed = 0;
	fStackBase = 0;
	fMonitorId = kNoId;
	fState = 0;
	fTaskData = 0;
	fEnvironment = NULL;
	fSMemEnvironment = NULL;
	fCurrentTask = NULL;
	fBequeathId = kNoId;
	fInheritedId = kNoId;
	fPriority = kIdleTaskPriority;
}


/*------------------------------------------------------------------------------
	Destructor.
-----------------------------------------------------------------------------*/

CTask::~CTask()
{
	if (fMonitorQItem.fContainer != NULL)
		fMonitorQItem.fContainer->deleteFromQueue(this);
	if (fCopyQItem.fContainer != NULL)
		fCopyQItem.fContainer->deleteFromQueue(this);
	UnScheduleTask(this);
	freeStack();
	gDeadTaskTime = gDeadTaskTime + fTaskTime;
	if (fEnvironment != NULL && fEnvironment->decrRefCount())
		delete fEnvironment;
	gObjectTable->remove(fSharedMem);
	gObjectTable->remove(fSharedMemMsg);
}


/*------------------------------------------------------------------------------
	Initialize the task.
	Create a stack and shared memory objects.
	Set up its globals:
		fStackTop	->	.--.
							|	|	taskSize data
		fGlobals		->	|	|
							|	|	TaskGlobals
		fTaskData	->	|	|
							|	|	stackSize stack
		fStackBase	->	!__!
	Initialize processor registers to execute the TaskProc (CUTaskWorld::taskEntry):
		fRegister[kParm0] = fGlobals;		-> class instance (CUTaskWorld)
		fRegister[kParm1] = dataSize;		-> passed to TaskProc but ignored
		fRegister[kParm2] = inTaskId;		-> user task id
	Args:		inProc			the code to execute
				inStackSize		size of stack to allocate
				inTaskId			task’s id
				inDataId			shared mem object containing task instance context
				inPriority		task scheduler priority
				inName			identifier; useful for humans but not used by the machine AFAICT
				inEnv				task’s environment
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CTask::init(TaskProcPtr inProc, size_t inStackSize, ObjectId inTaskId, ObjectId inDataId, ULong inPriority, ULong inName, CEnvironment * inEnv)
{
	NewtonErr err = noErr;

	XTRY
	{
		size_t		amtCopied;
		CUSharedMem data(inDataId);
		TaskGlobals globalSpace;
		size_t		stackSize;
		size_t		dataSize;
		size_t		taskDataSize;

		// we’re going to allocate one chunk of memory for both stack and task data
		// so work out size of the data
		if (inDataId != kNoId)
			XFAIL(err = data.getSize(&dataSize))
		else
			dataSize = 0;

		stackSize = LONGALIGN(inStackSize);
		taskDataSize = sizeof(TaskGlobals) + LONGALIGN(dataSize);
#if !defined(correct)
		stackSize += sizeof(long)*64*KByte;	// add stack buffer since we don’t grow the stack automatically; idle task has zero stack size!
//		taskDataSize += 256;		// add guard zone?
#endif

		if (gOSIsRunning)
		{
			// we’ve got the VM system, so create a new stack with a bit extra for the task data
			fState |= kMemIsVirtual;
			XFAILIF(err = NewStack(inEnv->fHeapDomainId, stackSize + kABIStackAlignment + taskDataSize, fId, &fStackTop, &fStackBase), fStackBase = 0;)
			// point to the data area
			fTaskData = ALIGN(fStackTop - taskDataSize, kABIStackAlignment);
			// lock the data area to ensure that no faults can occur within it
			XFAIL(err = LockHeapRange(fTaskData, fStackTop))
			// lock the param area to ensure that no faults can occur within it
			XFAIL(err = LockHeapRange(fTaskData, fTaskData + sizeof(KernelParams)))
		}
		else
		{
			// can’t use VM; allocate stack in the C heap
			fState &= ~kMemIsVirtual;
			XFAILNOT(fStackBase = (VAddr)NewPtr(stackSize + kABIStackAlignment + taskDataSize), err = kOSErrCouldNotCreateObject;)

			// fTaskData is also top of yer actual stack
			fTaskData = ALIGN(fStackBase + stackSize, kABIStackAlignment);
			fStackTop = fTaskData + taskDataSize;
		}
//printf("CTask::init(stackSize=%08X,name=%c%c%c%c) stack = #%08X - #%08X\n", inStackSize, inName >> 24, inName >> 16, inName >> 8, inName, fStackBase, fStackTop);

		// create shared memory
		CSharedMem * mem = new CSharedMem;
		if (mem != NULL && mem->init(inEnv) != noErr)
		{
			delete mem, mem = NULL;
		}
		RegisterObject(mem, kSharedMemType, fId, &fSharedMem);

		// create shared memory message
		CSharedMemMsg * memMsg = new CSharedMemMsg;
		if (memMsg != NULL && memMsg->init(inEnv) != noErr)
		{
			delete memMsg, memMsg = NULL;
		}
		RegisterObject(memMsg, kSharedMemMsgType, fId, &fSharedMemMsg);

		XFAILIF(fSharedMem == kNoId || fSharedMemMsg == kNoId, err = kOSErrCouldNotCreateObject;)

		// set up “globals” pointer
		fGlobals = (void *)(fTaskData + sizeof(TaskGlobals));

		// initialize task switched globals
		InitializeExceptionGlobals(&globalSpace.fExceptionGlobals);
		globalSpace.fTaskId = fId;
		globalSpace.fErrNo = noErr;
		globalSpace.fMemErr = noErr;
		globalSpace.fTaskName = inName;
		globalSpace.fStackTop = fStackTop;
		globalSpace.fStackBottom = fStackBase;
		globalSpace.fCurrentHeap = inEnv->fHeap;
		globalSpace.fDefaultHeapDomainId = inEnv->fHeapDomainId;

		// copy those globals into shared mem
		CUSharedMem sharedGlobalSpace(fSharedMem);
		XFAIL(err = sharedGlobalSpace.setBuffer((void*)fTaskData, taskDataSize, kSMemReadWrite))
		XFAIL(err = sharedGlobalSpace.copyToShared(&globalSpace, sizeof(TaskGlobals)))

		// copy task (instance) data after that, buf-at-a-time
		char	buf[256];
		VAddr	dataStart = fTaskData + sizeof(TaskGlobals);
		VAddr	dataEnd = fTaskData + taskDataSize;
		VAddr	dataPtr;
		for (dataPtr = dataStart; dataPtr < dataEnd; dataPtr += sizeof(buf))
		{
			size_t	bufLen = dataEnd - dataPtr;
			if (bufLen > sizeof(buf))
				bufLen = sizeof(buf);
			XFAIL(err = data.copyFromShared(&amtCopied, buf, bufLen, dataPtr - dataStart))
			XFAIL(err = sharedGlobalSpace.copyToShared(buf, bufLen, dataPtr - fTaskData))
		}
		XFAIL(err)

		if ((fState & kMemIsVirtual) != 0)
			// we’ve got VM; unlock the data area, but leave the params locked
			UnlockHeapRange(fTaskData, fStackTop);

		fTaskDataSize = taskDataSize;
		f68 = fId;
		fContainer = NULL;

		// set up processor registers for proc entry
#if defined(correct)
		fRegister[kParm0] = (ULong) fGlobals;
		fRegister[kParm1] = dataSize;
		fRegister[kParm2] = inTaskId;
		fRegister[4] = 0;
		fRegister[5] = this;						// huh?
		fRegister[6] = fRegister[7] = fRegister[8] = fRegister[9] = fRegister[10] = fRegister[11] = fRegister[12] = 0;
		fRegister[kcTheStack] = fTaskData;		// top of stack
		fRegister[kcTheLink] = (VAddr) TaskKillSelf;	// kill self when done

#elif defined(__i386__)
		fRegister[0] = fRegister[1] = fRegister[2] = fRegister[3] = fRegister[4] = fRegister[5] = 0;	// eax, ebx, ecx, edx, esi, edi
		/*
			i386 passes all args on the stack, which should also be 16-byte aligned on args:
				--								<- ebp
				taskId
				dataSize
				CUTaskWorld instance
				return address				<- esp
		*/
		unsigned long * sp = (unsigned long *)fTaskData;		// top of stack
		*--sp = 0;
		fRegister[kcTheFrame] = (VAddr)sp;	// not really, but gotta start somewhere
		*--sp = inTaskId;
		*--sp = dataSize;
		*--sp = (unsigned long)fGlobals;
//------- parameter area must be 16-byte aligned
		*--sp = (VAddr) TaskKillSelf;			// task can never return
		fRegister[kcTheStack] = (VAddr)sp;

#elif defined(__x86_64__)
		/*
			LP64 passes the first 6 args in registers, any more on the stack, which should also be 16-byte aligned on args:
				--								<- rbp
				return address				<- rsp
		*/
		fRegister[kParm0] = (unsigned long)fGlobals;	// [5] rdi
		fRegister[kParm1] = dataSize;						// [4] rsi
		fRegister[kParm2] = inTaskId;						// [3] rdx
		fRegister[0] = fRegister[1] = fRegister[2] = 0;	// rax, rbx, rcx
		fRegister[9] = fRegister[10] = fRegister[11] = 0;	// r8, r9, r10

		unsigned long * sp = (unsigned long *)fTaskData;		// top of stack
		*--sp = 0;
		*--sp = 0;
		fRegister[kcTheFrame] = (VAddr)sp;	// rbp always 16-byte aligned
		*--sp = (VAddr) TaskKillSelf;			// task can never return
		fRegister[kcTheStack] = (VAddr)sp;	// rsp
#else
#warning unknown processor!
#endif
		fRegister[kcThePC] = (VAddr) inProc;			// start of the task fn

		fPSR = kUserMode;	// user mode, interrupts enabled

		fPriority = inPriority;
		fName = inName;
		fEnvironment = inEnv;
		inEnv->incrRefCount();
	}
	XENDTRY;

	return err;
}


/*------------------------------------------------------------------------------
	Free the task stack.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CTask::freeStack(void)
{
	if (fStackBase != 0)
	{
		if ((fState & kMemIsVirtual) != 0)
			FreePagedMem(fStackTop - 1);
		else
			FreePtr((Ptr)fStackBase);
	}
}


/*------------------------------------------------------------------------------
	Set the id of the task to which this task’s resources will be bequeathed
	when it dies.
	Args:		inId			the task 
	Return:	--
------------------------------------------------------------------------------*/

void
CTask::setBequeathId(ObjectId inToTaskId)
{
	CTask * toTask;

	fBequeathId = inToTaskId;
	if (ConvertIdToObj(kTaskType, inToTaskId, &toTask) == noErr)
		toTask->fInheritedId = fId;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C T a s k Q I t e m
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Constructor.
	Nil out link pointers.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

CTaskQItem::CTaskQItem()
	: fNext(NULL), fPrev(NULL)
{ }

#pragma mark -

/*------------------------------------------------------------------------------
	C T a s k Q u e u e
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Constructor.
	Nil out link pointers.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

CTaskQueue::CTaskQueue()
	:	fHead(NULL), fTail(NULL)
{ }


/*------------------------------------------------------------------------------
	Add a task to the queue.
	Args:		inTask			the task to add
				inState			its initial state
				inContainer		queue owner, like the scheduler
	Return:	--
------------------------------------------------------------------------------*/

void
CTaskQueue::add(CTask * inTask, KernelObjectState inState, CTaskContainer * inContainer)
{
	inTask->fTaskQItem.fNext = NULL;
	checkBeforeAdd(inTask);
	if (fHead == NULL)
	{
		// there’s no queue yet, the task is at the head
		fHead = inTask;
		inTask->fTaskQItem.fPrev = (CTask *)this; // huh? remove sets fPrev to NULL
	}
	else
	{
		// current last task points forward to new task
		fTail->fTaskQItem.fNext = inTask;
		// new task points back to one currently last in queue
		inTask->fTaskQItem.fPrev = fTail;
	}
	// the new task is always at the tail
	fTail = inTask;
	inTask->fState |= inState;
	inTask->fContainer = inContainer;
}


/*------------------------------------------------------------------------------
	Validate a task before adding it to the queue.
	This really does nothing.
	Args:		inTask		the task to validate
	Return:	--
------------------------------------------------------------------------------*/

void
CTaskQueue::checkBeforeAdd(CTask * inTask)
{ }


/*------------------------------------------------------------------------------
	Peek at the task at the head of the queue, without removing it.
	Args:		--
	Return:	the task
------------------------------------------------------------------------------*/

CTask *
CTaskQueue::peek(void)
{
	return fHead;
}


/*------------------------------------------------------------------------------
	Find a task (from its ObjectId) and remove it, updating its state.
	Args:		inId			the task’s id
				inState		the state it should have after removal
	Return:	the task
------------------------------------------------------------------------------*/

CTask *
CTaskQueue::findAndRemove(ObjectId inId, KernelObjectState inState)
{
	CTask *	task;
	for (task = fHead; task != NULL; task = task->fTaskQItem.fNext)
	{
		if (task->f9C == inId)
		{
			removeFromQueue(task, inState);
			break;
		}
	}
	return task;
}


/*------------------------------------------------------------------------------
	Remove the task from the head of the queue.
	Args:		inState		state bits to be masked out of the task
	Return:	the task
------------------------------------------------------------------------------*/

CTask *
CTaskQueue::remove(KernelObjectState inState)
{
	CTask *	task = fHead;
	if (task != NULL)
	{
		fHead = task->fTaskQItem.fNext;
		if (fHead != NULL) fHead->fTaskQItem.fPrev = NULL; else fTail = NULL;
		// remove queue info from the task
		task->fTaskQItem.fNext = task->fTaskQItem.fPrev = NULL;
		task->fState &= ~inState;
		task->fContainer = NULL;
	}
	return task;
}


/*------------------------------------------------------------------------------
	Remove a task from the queue.
	Args:		inTask		the task to remove
				inState		state bits to be masked out of the task
	Return:	YES if task was removed
------------------------------------------------------------------------------*/

bool
CTaskQueue::removeFromQueue(CTask * inTask, KernelObjectState inState)
{
	if (inTask == NULL || (inTask->fState & inState) == 0)
		return NO;

	CTask *	qNext = inTask->fTaskQItem.fNext;
	CTask *	qPrev = inTask->fTaskQItem.fPrev;
	if (inTask == fHead)
	{
		// we’re removing the head of the queue
		if (fTail == fHead)
			// we’re removing the only item from the queue
			fHead = fTail = NULL;
		else
		{
			fHead = qNext;
			qNext->fTaskQItem.fPrev = NULL;
		}
	}
	else
	{
		// we’re removing from the body of the queue
		qPrev->fTaskQItem.fNext = qNext;
		if (inTask == fTail)
			// we’re removing the last item from the queue
			fTail = qPrev;
		else
			qNext->fTaskQItem.fPrev = qPrev;
	}
	// remove queue info from the task
	inTask->fTaskQItem.fNext = inTask->fTaskQItem.fPrev = NULL;
	inTask->fState &= ~inState;
	inTask->fContainer = NULL;
	return YES;
}
