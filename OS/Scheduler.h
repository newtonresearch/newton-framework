/*
	File:		Scheduler.h

	Contains:	Task scheduler declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__SCHEDULER_H)
#define __SCHEDULER_H 1

#include "KernelTasks.h"
#include "Timers.h"

/*------------------------------------------------------------------------------
	C S c h e d u l e r
------------------------------------------------------------------------------*/

class CScheduler : public CObject, CTaskContainer
{
public:
				CScheduler();
	virtual	~CScheduler();

	virtual void		remove(CTask * inTask);

	void		add(CTask * inTask);
	void		addWhenNotCurrent(CTask * inTask);
	void		setCurrentTask(CTask * inTask);
	CTask *	currentTask(void);
	CTask *	schedule(void);

private:
	CTask *	removeHighestPriority(void);
	void		updateCurrentBucket(void);

	friend class CSharedMemMsg;
	ArrayIndex	fHighestPriority;		// +14
	ULong			fPriorityMask;			// +18
	CTaskQueue	fTasks[32];				// +1C
	CTask *		fCurrentTask;			// +11C
};


inline			CScheduler::~CScheduler()	{ }
inline CTask *	CScheduler::currentTask(void)		{ return fCurrentTask; }
inline void		CScheduler::setCurrentTask(CTask * inTask)	{ fCurrentTask = inTask; }


/*------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" CTask *	Scheduler(void);

extern "C" void		StartScheduler(void);
extern "C" void		StopScheduler(void);
extern void		WantSchedule(void);
extern void		HoldSchedule(void);
extern void		AllowSchedule(void);
extern void		ScheduleTask(CTask * inTask);
extern void		UnScheduleTask(CTask * inTask);
extern void		SleepTask(void);


#endif	/* __SCHEDULER_H */
