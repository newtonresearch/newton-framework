/*
	File:		KernelTasks.h

	Contains:	Kernel task definitions.

	Written by:	Newton Research Group.
*/

#if !defined(__KERNELTASKS_H)
#define __KERNELTASKS_H 1

#include "NewtonTime.h"
#include "KernelObjects.h"
#include "DoubleQ.h"
#include "TaskGlobals.h"

class CTask;

/*----------------------------------------------------------------------
	C T a s k C o n t a i n e r
----------------------------------------------------------------------*/

class CTaskContainer // : public CObject
{
public:
	virtual			~CTaskContainer();

	virtual void	remove(CTask * inTask);
};

inline		CTaskContainer::~CTaskContainer()
{ }

inline void	CTaskContainer::remove(CTask * inTask)
{ }


/*----------------------------------------------------------------------
	C T a s k Q u e u e
----------------------------------------------------------------------*/

class CTaskQueue
{
public:
				CTaskQueue();

	void		add(CTask * inTask, KernelObjectState inState, CTaskContainer * inContainer);
	void		checkBeforeAdd(CTask * inTask);
	CTask *	peek(void);
	CTask *	findAndRemove(ObjectId inId, KernelObjectState inState);
	CTask *	remove(KernelObjectState inState);
	bool		removeFromQueue(CTask * inTask, KernelObjectState inState);

private:
	CTask *	fHead;
	CTask *	fTail;
};


/*----------------------------------------------------------------------
	C T a s k Q I t e m
----------------------------------------------------------------------*/

class CTaskQItem
{
public:
				CTaskQItem();

private:
	friend class CTaskQueue;

	CTask *	fNext;
	CTask *	fPrev;
};


/*--------------------------------------------------------------------------------
	C T a s k

	The kernel task class.
	Instances of CTask are actually copied to shared memory after normal
	instantiation; the declaration can be stack based and the instance will still
	be valid. The task instance data is preceded by task switched globals.
--------------------------------------------------------------------------------*/
class CEnvironment;
class CMonitor;

class CTask : public CObject
{
public:
					CTask();
					~CTask();

	NewtonErr	init(TaskProcPtr inProc, size_t inStackSize, ObjectId inTaskId, ObjectId inDataId, ULong inPriority, ULong inName, CEnvironment * inEnvironment);
	void			freeStack(void);
	void			setBequeathId(ObjectId inTaskId);

//protected:
	friend class CTaskQueue;

	unsigned long	fRegister[kNumOfRegisters];	// +10	processor registers
	ULong				fPSR;					// +50	Processor Status Register
	ObjectId			f68;					// +68	set in init() but never referenced
	KernelObjectState	fState;			// +6C
	CEnvironment * fEnvironment;		// +74
	CEnvironment *	fSMemEnvironment;	// +78	for copying to/from shared memory
	CTask *			fCurrentTask;		// +7C	if this task is a monitor, the task it’s running (ever referenced?)
	int				fPriority;			// +80
	ULong				fName;				// +84
	VAddr				fStackTop;			// +88
	VAddr				fStackBase;			// +8C
	CTaskContainer *  fContainer;		// +90
	CTaskQItem		fTaskQItem;			// +94	for tasks in scheduler queue
	ObjectId			f9C;					// +9C	referenced in findAndRemove() but never set
	void *			fGlobals;			// +A0
	CTime				fTaskTime;			// +A4	time spent within this task
	size_t			fTaskDataSize;		// +AC	size of task globals
	ArrayIndex		fPtrsUsed;			// +B0
	ArrayIndex		fHandlesUsed;		// +B4
	ArrayIndex		fMemUsed;			// +B8	max memory (pointers + handles) used
	CDoubleQItem	fCopyQItem;			// +BC	for tasks in queue for copy
	CDoubleQItem	fMonitorQItem;		// +C8	for tasks in monitor queue
	ObjectId			fMonitor;			// +D4	if task is running in a monitor, that monitor
	ObjectId			fMonitorId;			// +D8	if this task is a monitor, its id
	VAddr				fCopySavedPC;		// +DC	PC register saved during copy task
#if defined(__i386__) || defined(__x86_64__)
	VAddr				fCopySavedesi;
	VAddr				fCopySavededi;
#endif
	NewtonErr		fCopySavedErr;		// +E0	error code saved during copy task
	size_t			fCopiedSize;		// +E4
	ObjectId			fCopySavedMemId;	// +E8	CSharedMem object id saved during copy task
	ObjectId			fCopySavedMemMsgId;//+EC	CSharedMemMsg object id saved during copy task
	ObjectId			fSharedMem;			// +F0	shared mem object id
	ObjectId			fSharedMemMsg;		// +F4	shared mem msg object id
	VAddr				fTaskData;			// +F8	address of task globals (usually == this)
	ObjectId			fBequeathId;		// +FC
	ObjectId			fInheritedId;		// +100
};


extern "C" void	SwapInGlobals(CTask * inTask);


#endif	/* __KERNELTASKS_H */
