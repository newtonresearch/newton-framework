/*
	File:		UserMonitor.cc

	Contains:	User routines for creating and accessing monitors.

	Written by:	Newton Research Group.
*/

#include "UserMonitor.h"
#include "KernelObjects.h"


/*--------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
--------------------------------------------------------------------------------*/

void
MonitorExitAction(ObjectId inMonitorId, ULong inAction)
{}


/*--------------------------------------------------------------------------------
	C U M o n i t o r
--------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Destructor.
	Ask the object manager monitor to destroy the object.
------------------------------------------------------------------------------*/

CUMonitor::~CUMonitor()
{
	destroyObject();
}


/*------------------------------------------------------------------------------
	Initialization.
	Ask the object manager monitor to make a monitor object.
	Args:		inMonitorProc
				inStackSize
				inContext				instance data for inMonitorProc
				inEnvironmentId
				inFaultMonitor			YES => is a fault monitor
				inName
				inRebootProtected		YES => wait for monitor before rebooting
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CUMonitor::init(MonitorProcPtr inMonitorProc, size_t inStackSize, void * inContext,
			ObjectId inEnvironmentId, bool inFaultMonitor, ULong inName,
			bool inRebootProtected)
{
	ObjectMessage	msg;

	msg.request.monitor.proc = inMonitorProc;
	msg.request.monitor.stackSize = inStackSize;
	msg.request.monitor.context = inContext;
	msg.request.monitor.envId = inEnvironmentId;
	msg.request.monitor.isRebootProtected = inRebootProtected;
	msg.request.monitor.isFaultMonitor = inFaultMonitor;
	msg.request.monitor.name = inName;

	return makeObject(kObjectMonitor, &msg, MSG_SIZE(7));
}


/*------------------------------------------------------------------------------
	Ask the object manager monitor to destroy us.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CUMonitor::destroyObject(void)
{
	if ((ObjectId)*this != kNoId && fObjectCreatedByUs)
	{
		invokeRoutine(kSuspendMonitor, NULL);
		CUObject::destroyObject();
	}
}

