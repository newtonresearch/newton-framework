/*
	File:		UserMonitor.h

	Contains:	User routines for creating and accessing monitors.

	Written by:	Newton Research Group.
*/

#if !defined(__USERMONITOR_H)
#define __USERMONITOR_H 1

#if !defined(__NEWTON_H)
#include "Newton.h"
#endif

#if !defined(__USEROBJECTS_H)
#include "UserObjects.h"
#endif

#include "UserSharedMem.h"
#include "UserGlobals.h"
#include "UserTasks.h"


/*--------------------------------------------------------------------------------
	C U M o n i t o r
--------------------------------------------------------------------------------*/

class CUMonitor : public CUObject
{
public:
				CUMonitor(ObjectId id = 0);
				~CUMonitor();

	void		operator=(const CUMonitor & inCopy);

	NewtonErr	init(MonitorProcPtr inMonitorProc, size_t inStackSize, void * inContext = NULL,
							ObjectId inEnvironmentId = 0, bool inFaultMonitor = false, ULong inName = 'MNTR',
							bool inRebootProtected = false);

	NewtonErr	invokeRoutine(int inSelector, void * ioMsg);

	void		destroyObject(void);
	void		setDestroyKernelObject(bool inNewStatus);
};

/*------------------------------------------------------------------------------
	C U M o n i t o r   I n l i n e s
------------------------------------------------------------------------------*/

inline			CUMonitor::CUMonitor(ObjectId id) : CUObject(id)  { }
inline void		CUMonitor::operator=(const CUMonitor & inCopy) { copyObject(inCopy.fId); }
inline void		CUMonitor::setDestroyKernelObject(bool inNewStatus)  { fObjectCreatedByUs = inNewStatus; }
inline NewtonErr	CUMonitor::invokeRoutine(int inSelector, void * ioMsg)  { return MonitorDispatchSWI(fId, inSelector, (OpaqueRef)ioMsg); }

#define kIsFaultMonitor		true
#define kIsntFaultMonitor	false


#endif	/* __USERMONITOR_H */
