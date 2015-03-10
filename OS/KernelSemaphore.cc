/*
	File:		KernelSemaphore.cc

	Contains:	Kernel semaphore implementation.

	Written by:	Newton Research Group.
*/

#include "KernelSemaphore.h"
#include "Scheduler.h"
#include "OSErrors.h"


/*------------------------------------------------------------------------------
	U t i l i t i e s
------------------------------------------------------------------------------*/

void
MarkMessageDone(CTask * ioTask, long inResult)
{
	ioTask->fRegister[kResultReg] = inResult;
}


extern "C" NewtonErr
DoSemaphoreOp(ObjectId inGroupId, ObjectId inListId, SemFlags inBlocking, CTask * inTask)
{
	NewtonErr err;
	XTRY
	{
		CSemaphoreGroup * semGroup;
		XFAILNOT(semGroup = (CSemaphoreGroup *)IdToObj(kSemGroupType, inGroupId), err = kOSErrBadSemaphoreGroupId;)

		CSemaphoreOpList * opList;
		XFAILNOT(opList = (CSemaphoreOpList *)IdToObj(kSemListType, inListId), err = kOSErrBadSemaphoreOpListId;)

		err = semGroup->semOp(opList, inBlocking, inTask);
	}
	XENDTRY;
	return err;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S e m a p h o r e
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Constructor.
	Initialize semaphore value to zero.
------------------------------------------------------------------------------*/

CSemaphore::CSemaphore()
{
	fVal = 0;
}


/*------------------------------------------------------------------------------
	Destructor.
	Unblock all waiting tasks.
------------------------------------------------------------------------------*/

CSemaphore::~CSemaphore()
{
	CTask *	task;
	while ((task = fZeroTasks.remove(0x00100000)) != NULL)
	{
		task->fRegister[kcThePC] += 4;
		ScheduleTask(task);
		MarkMessageDone(task, kOSErrSemaphoreGroupNoLongerExists);
	}
	while ((task = fIncTasks.remove(0x00100000)) != NULL)
	{
		task->fRegister[kcThePC] += 4;
		ScheduleTask(task);
		MarkMessageDone(task, kOSErrSemaphoreGroupNoLongerExists);
	}
}


/*------------------------------------------------------------------------------
	Remove a task from our waiting lists.
	Args:		inTask		the task to be removed
	Return:	--
------------------------------------------------------------------------------*/

void
CSemaphore::remove(CTask * inTask)
{
	fZeroTasks.removeFromQueue(inTask, 0x00100000);
	fIncTasks.removeFromQueue(inTask, 0x00100000);
}


/*------------------------------------------------------------------------------
	Block a task.  Wait for semaphore to become zero before unblocking it.
	Args:		inTask		the task to be blocked
				inBlocking	should we really wait, or is this synchronous?
	Return:	--
------------------------------------------------------------------------------*/

void
CSemaphore::blockOnZero(CTask * inTask, SemFlags inBlocking)
{
	if (inBlocking == kNoWaitOnBlock)
	{
		UnScheduleTask(inTask);
		fZeroTasks.add(inTask, 0x00100000, this);
	}
}


/*------------------------------------------------------------------------------
	Unblock all tasks waiting for semaphore to become zero.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CSemaphore::wakeTasksOnZero(void)
{
	CTask *	task;
	while ((task = fZeroTasks.remove(0x00100000)) != NULL)
	{
		ScheduleTask(task);
	}
	WantSchedule();
}


/*------------------------------------------------------------------------------
	Block a task.  Wait for semaphore to increase before unblocking it.
	Args:		inTask		the task to be blocked
				inBlocking	should we really wait, or is this synchronous?
	Return:	--
------------------------------------------------------------------------------*/

void
CSemaphore::blockOnInc(CTask * inTask, SemFlags inBlocking)
{
	if (inBlocking == kNoWaitOnBlock)
	{
		UnScheduleTask(inTask);
		fIncTasks.add(inTask, 0x00100000, this);
	}
}


/*------------------------------------------------------------------------------
	Unblock all tasks waiting for semaphore to increase.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CSemaphore::wakeTasksOnInc(void)
{
	CTask *	task;
	while ((task = fIncTasks.remove(0x00100000)) != NULL)
	{
		ScheduleTask(task);
	}
	WantSchedule();
}

#pragma mark -

/*----------------------------------------------------------------------
	C S e m a p h o r e O p L i s t

	A convenience wrapper for a list of SemOps.
----------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Destructor.
	Delete oplist array.
------------------------------------------------------------------------------*/

CSemaphoreOpList::~CSemaphoreOpList()
{
	if (fOpList)
		delete[] fOpList;
}


/*------------------------------------------------------------------------------
	Initialize our oplist.
	Args:		inNumOfOps	number of SemOps in…
				inOps			…this array
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CSemaphoreOpList::init(ArrayIndex inNumOfOps, SemOp * inOps)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILNOT(fOpList = new SemOp[inNumOfOps], fCount = 0; err = -1;)
		memmove(fOpList, inOps, inNumOfOps * sizeof(SemOp));
		fCount = inNumOfOps;
	}
	XENDTRY;
	return err;
}


/*----------------------------------------------------------------------
	C S e m a p h o r e G r o u p
----------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Destructor.
	Delete group array.
------------------------------------------------------------------------------*/

CSemaphoreGroup::~CSemaphoreGroup()
{
	if (fGroup)
		delete[] fGroup;
}


/*------------------------------------------------------------------------------
	Initialize a new group of the specified size.
	Args:		inCount		number of Semaphhores in the group
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CSemaphoreGroup::init(ArrayIndex inCount)
{
	NewtonErr err = noErr;
	XTRY
	{
		fRefCon = 0;
		XFAILNOT(fGroup = new CSemaphore[inCount], fCount = 0; err = -1;)
		fCount = inCount;
	}
	XENDTRY;
	return err;
}


/*------------------------------------------------------------------------------
	Perform a list of semaphore operations.
	Args:		inList		operations to perform
				inBlocking	whether task should block
				inTask		task performing these operations
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CSemaphoreGroup::semOp(CSemaphoreOpList * inList, SemFlags inBlocking, CTask * inTask)
{
	CSemaphore *	sem;
	ArrayIndex	index;
	UShort	semNum;
	short		semOper;

	for (index = 0; index < inList->fCount; index++)
	{
		semNum  = inList->fOpList[index].num;
		semOper = inList->fOpList[index].op;
		if (semNum < fCount)
		{
			sem = fGroup + semNum;
			if (semOper == 0)
			{
				if (sem->fVal != 0)
				{
					fGroup[semNum].blockOnZero(inTask, inBlocking);
					unwindOp(inList, index);
					return kOSErrSemaphoreWouldCauseBlock;
				}
			}
			else if (semOper > 0 || sem->fVal >= -semOper)	// - = abs
				sem->fVal += semOper;
			else
			{
				fGroup[semNum].blockOnInc(inTask, inBlocking);
				unwindOp(inList, index);
				return kOSErrSemaphoreWouldCauseBlock;
			}
		}
	}

	// now all the semaphores have been updated, see if any tasks can be unblocked
	for (index = 0; index < inList->fCount; index++)
	{
		semNum  = inList->fOpList[index].num;
		semOper = inList->fOpList[index].op;
		if (semNum <= fCount)
		{
			sem = fGroup + semNum;
			if (semOper > 0)
				sem->wakeTasksOnInc();
			else if (semOper == 0 && sem->fVal == 0)
				sem->wakeTasksOnZero();
		}
	}

	return noErr;
}


/*------------------------------------------------------------------------------
	Undo a list of semaphore operations.
	Args:		inList		operations to unperform
				index			last operation at which to start unwinding
	Return:	--
------------------------------------------------------------------------------*/

void
CSemaphoreGroup::unwindOp(CSemaphoreOpList * inList, ArrayIndex index)
{
	UShort	semNum;
	short		semOper;

	while (--index != kIndexNotFound)
	{
		semNum  = inList->fOpList[index].num;
		semOper = inList->fOpList[index].op;
		if (semNum < fCount)
		{
			fGroup[semNum].fVal -= semOper;
		}
	}
}

