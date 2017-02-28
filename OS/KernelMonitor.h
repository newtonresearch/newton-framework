/*
	File:		KernelMonitor.h

	Contains:	Kernel monitor declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__KERNELMONITOR_H)
#define __KERNELMONITOR_H 1

#include "KernelTasks.h"


/*------------------------------------------------------------------------------
	C M o n i t o r
------------------------------------------------------------------------------*/

class CMonitor : public CObject
{
public:
					CMonitor();
					~CMonitor();

	NewtonErr	init(MonitorProcPtr, size_t inStackSize, void * inContext, CEnvironment * inEnvironment, bool inFaultMonitor, ULong inName, bool inRebootProtected = false);
	NewtonErr	acquire(void);
	bool			suspend(ULong inFlags);
	void			release(NewtonErr inResult);
	bool			setUpEntry(CTask * inTask);
	void			setCallerRegister(ArrayIndex inRegNum, unsigned long inValue);
	void			setResult(CTask * inTask, NewtonErr inResult);
	void			flushTasksOnMonitor(void);
	void			deleteTaskOnMonitorQ(CTask * inTask);

	ObjectId		message(void)				const;
	CTask *		caller(void)				const;
	bool			isFaultMonitor(void)		const;
	bool			isCopying(void)			const;

private:
	ArrayIndex		fQueueCount;		// +10	number of tasks waiting for entry to monitor
	ULong				fSuspended;			// +14	flags
	void *			fProcContext;		// +18	instance of class containing fProc
	ObjectId			fMsgId;				// +1C
	CTask *			fCaller;				// +20	task calling this monitor
	CDoubleQContainer	fQueue;			// +24	queue of tasks waiting for entry to monitor
	MonitorProcPtr	fProc;				// +38	routine to be executed (in context of fProcContext)
	CTask *			fMonitorTask;		// +3C	task running as a monitor
	ObjectId			fMonitorTaskId;	// +40	id of above task
	bool				fIsFaultMonitor;	// +44
	bool				fRebootProtected; // +45
	bool				fIsCopying;			// +46
};

inline ObjectId	CMonitor::message(void) const  { return fMsgId; }
inline CTask *		CMonitor::caller(void) const  { return fCaller; }
inline bool			CMonitor::isFaultMonitor(void) const  { return fIsFaultMonitor; }
inline bool			CMonitor::isCopying(void) const  { return fIsCopying; }


#endif	/* __KERNELMONITOR_H */
