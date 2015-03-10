/*
	File:		KernelObjects.cc

	Contains:	Kernel object implementation.

	Written by:	Newton Research Group.
*/

#include "MemObjManager.h"
#include "KernelPorts.h"
#include "KernelMonitor.h"
#include "KernelGlobals.h"
#include "KernelSemaphore.h"
#include "PageManager.h"
#include "NameServer.h"

#include <stdarg.h>

extern CObjectTable *	gTheMemArchObjTbl;
extern CUPort *			gNameServer;


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

static NewtonErr	InitSMemManager(void);
static void			InitObjectManager(void);

static NewtonErr	ObjectAlloc(ObjectMessage * inMsg, size_t inSize, ObjectId inOwner, ObjectId * outId);
static NewtonErr	ObjectDestroy(ObjectMessage * inMsg, size_t inSize, ObjectId inOwner);
static NewtonErr	ObjectStart(ObjectMessage * inMsg, size_t inSize);
static NewtonErr	ObjectSuspend(ObjectMessage * inMsg, size_t inSize);
static NewtonErr	ObjectSetRegister(ObjectMessage * inMsg, size_t inSize);
static NewtonErr	ObjectGetRegister(ObjectMessage * inMsg, size_t inSize, ULong * outResult);
static NewtonErr	GetObjectContent(ObjectMessage * inMsg, size_t inSize, ObjectMessage * outMsg);
static NewtonErr	SetDomainFaultMonitor(ObjectMessage * inMsg, size_t inSize);

ScavengeProcPtr	ObjectScavenger(CObject * inObject, ObjectId inId);

static void			DeleteTask(CTask * inTask);
static void			DeletePort(CPort * inPort);
static void			DeleteSemList(CSemaphoreOpList * inSemaphoreOpList);
static void			DeleteSemGroup(CSemaphoreGroup * inSemaphoreGroup);
static void			DeleteSharedMem(CSharedMem * inSharedMem);
static void			DeleteSharedMemMsg(CSharedMemMsg * inSharedMemMsg);
static void			DeleteMonitor(CMonitor * inMonitor);
static void			DeletePhys(CPhys * inPhys);
static void			DeleteNothing(CObject * inObj);


/*------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
------------------------------------------------------------------------------*/

CObjectTable *	gObjectTable;
CPort *			gNullPort;

CDoubleQContainer *	gCopyTasks;
CDoubleQContainer *	gBlockedOnMemory;
CDoubleQContainer *	gDeferredSends;
bool						gCopyDone;

int			gRebootProtectCount;
bool			gWantReboot;			// 0C101050 reboot after protected monitors have finished
ObjectId		gCurrentTaskId;		// 0C101054 id of currently running task
ObjectId		gCurrentMonitorId;	// 0C101058 monitor that a task is in if any
void *		gCurrentGlobals;		// 0C10105C pointer to per task globals

bool			gTaskDestroyed;				// 0C10169C	+00

CObjectManager *	gTheObjectManager;			// 0C1016A0	+04
CMonitor *			gTheObjectManagerMonitor;	// 0C1016A4	+08


/*------------------------------------------------------------------------------
	Initialize the kernel’s world.
	Create a task scheduler;
	create some queues;
	initialize the kernel object system;
	set up the kernel’s port.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
InitGlobalWorld(void)
{
	gDoSchedule = NO;
	gKernelScheduler = new CScheduler();

	gCopyTasks = new CDoubleQContainer(offsetof(CTask, fCopyQItem));
	gBlockedOnMemory = new CDoubleQContainer(offsetof(CTask, fMonitorQItem));
	gDeferredSends = new CDoubleQContainer(offsetof(CSharedMemMsg, f80));

	InitSMemManager();
	InitObjectManager();

	gNullPort = new CPort();
	return RegisterObject(gNullPort, kPortType, kSystemId, NULL);
}


/*------------------------------------------------------------------------------
	Return the ObjectId of a well-known port.
	Args:		inWhat	0 => object manager
							1 => null
							2 => name server
	Return:	ObjectId
------------------------------------------------------------------------------*/

extern "C" ObjectId
GetPortInfo(int inWhat)
{
	ObjectId	id;
	switch (inWhat)
	{
	case kGetObjectPort:
		id = *gTheObjectManagerMonitor;
		break;
	case kGetNullPort:
		id = *gNullPort;
		break;
	case kGetNameServerPort:
		id = *gNameServer;
		break;
	default:
		id = kNoId;
	}
	return id;
}


/*------------------------------------------------------------------------------
	Initialize the shared memory manager.
	This really does nothing.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

static NewtonErr
InitSMemManager(void)
{
	return noErr;
}


/*------------------------------------------------------------------------------
	Initialize the memory object manager.
	Create a monitor to handle object manager requests.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

static void
InitObjectManager(void)
{
	ObjectId			kernelId;
	ObjectId			monitorId;
	CEnvironment *	environment;

	MemObjManager::findEnvironmentId('krnl', &kernelId);
	environment = (CEnvironment *)IdToObj(kEnvironmentType, kernelId);

	gTheObjectManager = new CObjectManager;
	gTheObjectManagerMonitor = new CMonitor;
	RegisterObject(gTheObjectManagerMonitor, kMonitorType, kSystemId, &monitorId);
//	gTheObjectManagerMonitor->init(MemberFunctionCast(MonitorProcPtr, gTheObjectManager, &CObjectManager::monitorProc), 2*KByte, gTheObjectManager, environment, kIdleTaskPriority, 'OBJM');

	union
	{
		NewtonErr (CObjectManager::*fIn)(int,ObjectMessage*);
		MonitorProcPtr	fOut;
	} ptmf;
	ptmf.fIn = &CObjectManager::monitorProc;
	gTheObjectManagerMonitor->init(ptmf.fOut, 2*KByte, gTheObjectManager, environment, kIdleTaskPriority, 'OBJM');

	gObjectTable->setScavengeProc(ObjectScavenger);
}


/*------------------------------------------------------------------------------
	Register a memory object in the kernel’s object table.
	Args:		inObject			the object
				inType			its type; port, task
				inOwnerId		its owning task
				outId				the token by which this object should be referenced
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
RegisterObject(CObject * ioObject, KernelTypes inType, ObjectId inOwnerId, ObjectId * outId)
{
	ObjectId	nullID;

	if (outId == NULL)
		outId = &nullID;
	*outId = kNoId;

	if (ioObject == NULL)
		return kOSErrCouldNotCreateObject;

	*outId = gObjectTable->add(ioObject, inType, inOwnerId);

	return noErr;
}


/*------------------------------------------------------------------------------
	Convert a local (to a task) object id to its global id.
	There are three special ids:
		1	kBuiltInSMemMsgId				current task’s shared mem msg
		2	kBuiltInSMemId					current task’s shared mem
		3	kBuiltInSMemMonitorFaultId	current task’s monitor’s shared mem msg
	Args:		inId				a local object id
	Return:	global id
------------------------------------------------------------------------------*/

ObjectId
LocalToGlobalId(ObjectId inId)
{
	ObjectId	globalId = inId;

	switch (inId)
	{
	case kBuiltInSMemMsgId:
		if (gCurrentTask)
			globalId = gCurrentTask->fSharedMemMsg;
		break;

	case kBuiltInSMemId:
		if (gCurrentTask)
			globalId = gCurrentTask->fSharedMem;
		break;

	case kBuiltInSMemMonitorFaultId:
		{
			CMonitor * mon = (CMonitor *)IdToObj(kMonitorType, gCurrentTask->fMonitorId);
			if (mon)
				globalId = mon->message();
		}
		break;
	}

	return globalId;
}


/*------------------------------------------------------------------------------
	Convert a kernel object’s token to the object itself.
	Args:		inType			the object’s expected type
				inId				its token
				outObj			a pointer to its pointer
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
ConvertIdToObj(KernelTypes inType, ObjectId inId, void * outObj)
{
	NewtonErr	err = noErr;
	CObject *	obj = IdToObj(inType, LocalToGlobalId(inId));

	if (obj == NULL)
		err = kOSErrBadObjectId;
	if (outObj)
		*(CObject**)outObj = obj;

	return err;
}


/*------------------------------------------------------------------------------
	Convert a shared memory object’s token to the object itself.
	Args:		inId				its token
				outObj			a pointer to its pointer
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
ConvertMemOrMsgIdToObj(ObjectId inId, CSharedMem ** outObj)
{
	ULong				type;
	CSharedMem *	obj = (CSharedMem *)gObjectTable->get(LocalToGlobalId(inId));

	*outObj = obj;
	if (obj == NULL
	 || !((type = ObjectType(*obj)) == kSharedMemType || type == kSharedMemMsgType))
		return kOSErrBadObjectId;

	return noErr;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C O b j e c t M a n a g e r
------------------------------------------------------------------------------*/

CObjectManager::CObjectManager()
	: fDeferredId(kNoId)
{ }


/*------------------------------------------------------------------------------
	Handle a request to the object manager. This is executed as a monitor.
	Args:		inSelector		the request
				ioMsg				its parameters
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CObjectManager::monitorProc(int inSelector, ObjectMessage * ioMsg)
{
	NewtonErr	err = noErr;
	CTask *		owner = gTheObjectManagerMonitor->caller();
	ObjectId		ownerId = *owner;

	if (fDeferredId != kNoId)
	{
		gObjectTable->remove(fDeferredId);
		fDeferredId = kNoId;
	}

	if (inSelector == kKill)
	{
		owner->fState |= 0x02;
		fDeferredId = ownerId;
	}

	else
	{
		size_t	msgSize = ioMsg->size;
		switch (inSelector)
		{
		case kCreate:
			{
				ObjectId	objId;
				if ((err = ObjectAlloc(ioMsg, msgSize, ownerId, &objId)) == noErr)
					*(ObjectId*)ioMsg = objId;	// yes, result is really element 0
			}
			break;

		case kDestroy:
			err = ObjectDestroy(ioMsg, msgSize, ownerId);
			break;

		case kStart:
			err = ObjectStart(ioMsg, msgSize);
			break;

		case kSuspend:
			err = ObjectSuspend(ioMsg, msgSize);
			break;

		case kSetRegister:
			err = ObjectSetRegister(ioMsg, msgSize);
			break;

		case kGetRegister:
			{
				ULong	result;
				err = ObjectGetRegister(ioMsg, msgSize, &result);
				if (err == noErr)
					ioMsg->result = result;
			}
			break;

		case kAddDomain:
			{
				CEnvironment *	environment;
				CDomain *		domain;
				if (msgSize == MSG_SIZE(2)
				&& (environment = (CEnvironment *)IdToObj(kEnvironmentType, ioMsg->target))
				&& (domain = (CDomain *)IdToObj(kDomainType, ioMsg->request.addDomain.domainId)))
				{
					environment->add(domain, ioMsg->request.addDomain.isManager, ioMsg->request.addDomain.isStack, ioMsg->request.addDomain.isHeap);
					err = noErr;
				}
			}
			break;

		case kGetContent:
			err = GetObjectContent(ioMsg, msgSize, ioMsg);
			break;

		case kRemoveDomain:
			{
				CEnvironment *	environment;
				CDomain *		domain;
				if (msgSize == MSG_SIZE(1)
				&& (environment = (CEnvironment *)IdToObj(kEnvironmentType, ioMsg->target))
				&& (domain = (CDomain *)IdToObj(kDomainType, ioMsg->request.addDomain.domainId)))
				{
					environment->remove(domain);
					err = noErr;
				}
			}
			break;

		case kSetDomainFaultMonitor:
			err = SetDomainFaultMonitor(ioMsg, msgSize);
			break;

		case kStartTrackingExtPages:
			if (gExtPageTrackerMgr == NULL)
			{
				gExtPageTrackerMgr = new CExtPageTrackerMgr;
				if (gExtPageTrackerMgr)
					err = gExtPageTrackerMgr->makeNewTracker(ioMsg->target, ioMsg->request.tracker.huh1, ioMsg->request.tracker.huh1);
				else
					err = MemError();
			}
			break;

		case kStopTrackingExtPages:
			if (gExtPageTrackerMgr)
				err = gExtPageTrackerMgr->disposeTracker(ioMsg->target);
			break;

		default:
			err = kOSErrBadParameters;
			break;
		}

		if (gTaskDestroyed)
		{
			gTaskDestroyed = NO;
			gObjectTable->scavengeAll();
		}
	}
	return err;
}


/*------------------------------------------------------------------------------
	Those object functions in full.
------------------------------------------------------------------------------*/

static NewtonErr
ObjectAlloc(ObjectMessage * inMsg, size_t inSize, ObjectId inOwnerId, ObjectId * outId)
{
	NewtonErr err = kOSErrBadParameters;	// assume failure

	XTRY
	{
		XFAIL(inSize < MSG_SIZE(0))	// sanity check
		switch (inMsg->target)	// here an ObjectType rather than ObjectId
		{
		case kObjectPort:
			XFAIL(inSize != MSG_SIZE(0))
			{
				CPort *	port = new CPort;
				err = RegisterObject(port, kPortType, inOwnerId, outId);
			}
			break;

		case kObjectTask:
			XFAIL(inSize != MSG_SIZE(7))
			{
				ObjectId			envId, taskId;
				CEnvironment *	environment;
				CTask *			task;

				if ((envId = inMsg->request.task.envId) != kNoId)
				{
					// look up environment from its id
					XFAILNOT(environment = (CEnvironment *)IdToObj(kEnvironmentType, envId), err = kOSErrBadObjectId;)
				}
				else
				{
					// get environment from owner task
					XFAILNOT(task = (CTask *)IdToObj(kTaskType, inOwnerId), err = kOSErrBadObjectId;)
					environment = task->fEnvironment;
				}
				XFAILNOT(task = new CTask, err = kOSErrCouldNotCreateObject;)
				taskId = gObjectTable->add(task, kTaskType, kSystemId);
				XFAILIF(task->init(inMsg->request.task.proc,
										 inMsg->request.task.stackSize,
										 taskId,
										 inMsg->request.task.dataId,
										 inMsg->request.task.priority,
										 inMsg->request.task.name,
										 environment) != noErr, gObjectTable->remove(taskId); err = kOSErrCouldNotCreateObject;)
				task->assignToTask(inOwnerId);
/*
#if defined(__i386__) || defined(__x86_64__)
				unsigned long * sp = (unsigned long *)task->fRegister[kcTheStack];
//				*--sp = 0;
				task->fRegister[kcTheFrame] = (VAddr)sp;	// rbp always 16-byte aligned
				*--sp = (VAddr) TaskKillSelf;	// task can never return
				task->fRegister[kcTheStack] = (VAddr)sp;
#else
				task->fRegister[kcTheLink] = (VAddr) TaskKillSelf;	// task can never return
#endif
*/
				*outId = taskId;
				err = noErr;
			}
			break;

		case kObjectEnvironment:
			XFAIL(inSize != MSG_SIZE(1))
			{
				CEnvironment * environment;
				XFAILNOT(environment = new CEnvironment, err = kOSErrCouldNotCreateObject;)
				XFAILIF(environment->init(inMsg->request.environment.heap) != noErr, delete environment; err = kOSErrCouldNotCreateObject;)
				err = RegisterObject(environment, kEnvironmentType, inOwnerId, outId);
			}
			break;

		case kObjectDomain:
			XFAIL(inSize != MSG_SIZE(3))
			{
				ObjectId			monId, domId;
				CMonitor *		monitor;
				CDomain *		domain;

				if ((monId = inMsg->request.domain.monitorId) != kNoId)
				{
					XFAILNOT(monitor = (CMonitor *)IdToObj(kMonitorType, monId), err = kOSErrBadObjectId;)
					XFAILNOT(monitor->isFaultMonitor(), err = kOSErrNotAFaultMonitor;)	// +44
				}
				XFAILNOT(domain = new CDomain, err = kOSErrCouldNotCreateObject;)
				domId = gObjectTable->add(domain, kDomainType, inOwnerId);
				XFAILIF(err = domain->init(monId,
												inMsg->request.domain.rangeStart,
												inMsg->request.domain.rangeLength) != noErr, gObjectTable->remove(domId);)
				*outId = domId;
				err = noErr;
			}
			break;

		case kObjectSemList:
			XFAIL(inSize < MSG_SIZE(1))
			{
				CSemaphoreOpList * semList;
				XFAILNOT(semList = new CSemaphoreOpList, err = kOSErrCouldNotCreateObject;)
				XFAILIF(semList->init(inMsg->request.semList.opCount, inMsg->request.semList.ops) != noErr, delete semList; err = kOSErrCouldNotCreateObject;)
				err = RegisterObject(semList, kSemListType, inOwnerId, outId);
			}
			break;

		case kObjectSemGroup:
			XFAIL(inSize != MSG_SIZE(1))
			{
				CSemaphoreGroup * semGroup;
				XFAILNOT(semGroup = new CSemaphoreGroup, err = kOSErrCouldNotCreateObject;)
				XFAILIF(semGroup->init(inMsg->request.semGroup.size) != noErr, delete semGroup; err = kOSErrCouldNotCreateObject;)
				err = RegisterObject(semGroup, kSemGroupType, inOwnerId, outId);
			}
			break;

		case kObjectSharedMem:
			XFAIL(inSize != MSG_SIZE(0))
			{
				CTask * task;
				XFAILNOT(task = (CTask *)IdToObj(kTaskType, inOwnerId), err = kOSErrBadObjectId;)

				CSharedMem * sharedMem;
				XFAILNOT(sharedMem = new CSharedMem, err = kOSErrCouldNotCreateObject;)
				XFAILIF(sharedMem->init(task->fEnvironment) != noErr, delete sharedMem; err = kOSErrCouldNotCreateObject;)
				err = RegisterObject(sharedMem, kSharedMemType, inOwnerId, outId);
			}
			break;

		case kObjectSharedMemMsg:
			XFAIL(inSize != MSG_SIZE(0))
			{
				CTask * task;
				XFAILNOT(task = (CTask *)IdToObj(kTaskType, inOwnerId), err = kOSErrBadObjectId;)

				CSharedMemMsg * sharedMemMsg;
				XFAILNOT(sharedMemMsg = new CSharedMemMsg, err = kOSErrCouldNotCreateObject;)
				XFAILIF(sharedMemMsg->init(task->fEnvironment) != noErr, delete sharedMemMsg; err = kOSErrCouldNotCreateObject;)
				err = RegisterObject(sharedMemMsg, kSharedMemMsgType, inOwnerId, outId);
			}
			break;

		case kObjectMonitor:
			XFAIL(inSize != MSG_SIZE(7))
			{
				ObjectId			envId, monId;
				CEnvironment *	environment;
				CMonitor *		monitor;
				XFAILNOT(monitor = new CMonitor, err = kOSErrCouldNotCreateObject;)

				if ((envId = inMsg->request.monitor.envId) != kNoId)
				{
					// look up environment from its id
					XFAILNOT(environment = (CEnvironment *)IdToObj(kEnvironmentType, envId), err = kOSErrBadObjectId;)
				}
				else
				{
					// get environment from owner task
					CTask * task;
					XFAILNOT(task = (CTask *)IdToObj(kTaskType, inOwnerId), err = kOSErrBadObjectId;)
					environment = task->fEnvironment;
				}
				monId = gObjectTable->add(monitor, kMonitorType, inOwnerId);
				XFAILIF(monitor->init(inMsg->request.monitor.proc,
											 inMsg->request.monitor.stackSize,
											 inMsg->request.monitor.context,
											 environment,
											 inMsg->request.monitor.isFaultMonitor,
											 inMsg->request.monitor.name,
											 inMsg->request.monitor.isRebootProtected) != noErr, gObjectTable->remove(monId); err = kOSErrCouldNotCreateObject;)
				*outId = monId;
				err = noErr;
			}
			break;

		case kObjectPhys:
			XFAIL(inSize != MSG_SIZE(6))
			{
				CPhys *	phys;
				if (inMsg->request.phys.size >= kSectionSize)
					phys = new CPhys;
				else
					phys = new CLittlePhys;
				XFAILNOT(phys, err = kOSErrCouldNotCreateObject;)
				XFAILIF(phys->init(inMsg->request.phys.base,
										 inMsg->request.phys.size,
										 inMsg->request.phys.readOnly,
										 inMsg->request.phys.cache) != noErr, delete phys; err = kOSErrCouldNotCreateObject;)
				err = RegisterObject(phys, kPhysType, inOwnerId, outId);
			}
			break;
		}
	}
	XENDTRY;
	return err;
}


static NewtonErr
ObjectDestroy(ObjectMessage * inMsg, size_t inSize, ObjectId inOwnerId)
{
	NewtonErr err = noErr;
	XTRY
	{
		CObject *	obj;

		XFAILIF(inSize != MSG_SIZE(0), err = kOSErrBadParameters;)
		XFAILNOT(obj = gObjectTable->get(inMsg->target), err = kOSErrBadObjectId;)
		XFAILIF(inOwnerId != kNoId && inOwnerId != obj->owner(), err = kOSErrObjectNotOwnedByTask;)

		err = gObjectTable->remove(inMsg->target);
	}
	XENDTRY;
	return err;
}


static NewtonErr
ObjectStart(ObjectMessage * inMsg, size_t inSize)
{
	NewtonErr err = noErr;
	XTRY
	{
		CTask *	task;

		XFAILIF(inSize != MSG_SIZE(0), err = kOSErrBadParameters;)
		XFAIL((task = (CTask *)IdToObj(kTaskType, inMsg->target)) == NULL)

		EnterAtomic();
		ScheduleTask(task);	// correct => adds task even when current
		ExitAtomic();
	}
	XENDTRY;
	return err;
}


static NewtonErr
ObjectSuspend(ObjectMessage * inMsg, size_t inSize)
{
	NewtonErr err = noErr;
	XTRY
	{
		CTask *	task;

		XFAILIF(inSize != MSG_SIZE(0), err = kOSErrBadParameters;)
		XFAIL((task = (CTask *)IdToObj(kTaskType, inMsg->target)) == NULL)
		XFAILIF((task->fState & 0x08) == 0, err = kOSErrCannotSuspendBlockedTask;)

		EnterAtomic();
		UnScheduleTask(task);
		ExitAtomic();
	}
	XENDTRY;
	return err;
}


static NewtonErr
ObjectSetRegister(ObjectMessage * inMsg, size_t inSize)
{
	NewtonErr err = noErr;
	XTRY
	{
		CTask *	task;
		ULong		reg;

		XFAILIF(inSize != MSG_SIZE(2), err = kOSErrBadParameters;)

		XFAILIF((reg = inMsg->request.reg.number) >= kNumOfRegisters, err = kOSErrBadRegisterNumber;)
		XFAIL((task = (CTask *)IdToObj(kTaskType, inMsg->target)) == NULL)

		EnterAtomic();
		task->fRegister[reg] = inMsg->request.reg.value;
		ExitAtomic();
	}
	XENDTRY;
	return err;
}


static NewtonErr
ObjectGetRegister(ObjectMessage * inMsg, size_t inSize, ULong * outResult)
{
	NewtonErr err = noErr;
	XTRY
	{
		CTask *	task;
		ULong		reg;

		XFAILIF(inSize != MSG_SIZE(1), err = kOSErrBadParameters;)

		XFAILIF((reg = inMsg->request.reg.number) >= kNumOfRegisters, err = kOSErrBadRegisterNumber;)
		XFAIL((task = (CTask *)IdToObj(kTaskType, inMsg->target)) == NULL)

		EnterAtomic();
		*outResult = task->fRegister[reg];
		ExitAtomic();
	}
	XENDTRY;
	return err;
}


static NewtonErr
GetObjectContent(ObjectMessage * inMsg, size_t inSize, ObjectMessage * outMsg)
{
	NewtonErr err = noErr;
	XTRY
	{
		CTask * task;

		XFAILIF(inMsg->request.task.f0C != 1 || inSize != MSG_SIZE(1), err = kOSErrBadParameters;)

		HoldSchedule();
		XFAILNOT(task = (CTask *)IdToObj(kTaskType, inMsg->target), err = kOSErrUnexpectedObjectType;)
		ULong * p = (ULong *)outMsg;
		p[2] = task->fPriority;
		p[3] = task->fName;
		p[4] = task->fTaskTime;	// longs swapped!
		p[6] = task->fTaskDataSize;
		p[7] = task->fHandlesUsed;
		p[8] = task->fPtrsUsed;
		p[9] = task->fMemUsed;
		AllowSchedule();
	}
	XENDTRY;
	return err;
}


static NewtonErr
SetDomainFaultMonitor(ObjectMessage * inMsg, size_t inSize)
{
	NewtonErr err = noErr;
	XTRY
	{
		CDomain *	domain;
		CMonitor *	monitor;

		XFAILIF(inSize != MSG_SIZE(2), err = kOSErrBadParameters;)
		XFAILNOT(domain == (CDomain *)IdToObj(kDomainType, inMsg->request.faultMonitor.domainId), err = kOSErrBadParameters;)
		XFAILNOT(monitor == (CMonitor *)IdToObj(kMonitorType, inMsg->request.faultMonitor.monitorId), err = kOSErrBadObjectId;)
		err = domain->setFaultMonitor(inMsg->request.faultMonitor.monitorId);
	}
	XENDTRY;
	return err;
}

#pragma mark -

/*------------------------------------------------------------------------------
	O b j e c t   S c a v e n g e r
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Return a function that knows how to scavenge a kernel object.
	Args:		inObject			the object to scavenge
				inId				its id - we ignore this and use the object’s id
	Return:	function pointer
------------------------------------------------------------------------------*/

ScavengeProcPtr
ObjectScavenger(CObject * inObject, ObjectId inId)
{
	ScavengeProcPtr	scavenger;

	switch (ObjectType(*inObject))
	{
	case kPortType:
		scavenger = (ScavengeProcPtr) DeletePort;
		break;
	case kTaskType:
		if (((CTask *)inObject)->fMonitor != kNoId)
			((CTask *)inObject)->fState |= 0x00400000;
		else
			scavenger = (ScavengeProcPtr) DeleteTask;
		break;
//	case kEnvironmentType:	can’t be scavenged
//	case kDomainType:			can’t be scavenged
	case kSemListType:
		scavenger = (ScavengeProcPtr) DeleteSemList;
		break;
	case kSemGroupType:
		scavenger = (ScavengeProcPtr) DeleteSemGroup;
		break;
	case kSharedMemType:
		scavenger = (ScavengeProcPtr) DeleteSharedMem;
		break;
	case kSharedMemMsgType:
		scavenger = (ScavengeProcPtr) DeleteSharedMemMsg;
		break;
	case kMonitorType:
		if (((CMonitor *)inObject)->suspend(1))
			scavenger = (ScavengeProcPtr) DeleteMonitor;
		break;
	case kPhysType:
		scavenger = (ScavengeProcPtr) DeletePhys;
		break;
	default:
		scavenger = NULL;
		break;
	}

	return scavenger;
}


ScavengeProcPtr
NullScavenger(CObject * inObject, ObjectId inId)
{
	return DeleteNothing;
}


/*------------------------------------------------------------------------------
	The kernel object scavenging functions.
------------------------------------------------------------------------------*/
extern "C" NewtonErr	LowLevelCopyDoneFromKernelGlue(NewtonErr inErr, CTask * inTask, VAddr inReturn);

static void
CheckCopyTask(void)
{
	for (CTask * task = (CTask *)gCopyTasks->peek(); task != NULL; task = (CTask *)gCopyTasks->getNext(task))
	{
		//nextTask = (CTask *)gCopyTasks->getNext(task);	// original does this here and assigns in for() expression -- for a reason?
		ObjectId copyMemId = task->fCopySavedMemId;
		CSharedMem * copyMem = (CSharedMem *)gObjectTable->get(copyMemId);
		if (copyMem != NULL && (copyMem->owner() == *copyMem || gObjectTable->exists(copyMem->owner())))
			LowLevelCopyDoneFromKernelGlue(kOSErrBadObjectId, task, 0);
	}
}


static void
DeleteTask(CTask * inTask)
{
	ObjectId bequeathId = inTask->fBequeathId;
	ObjectId taskId = *inTask;
	if (inTask->fInheritedId != kNoId)
	{
		// unhook task from inheritance chain
		CTask * inheritedTask;
		if (ConvertIdToObj(kTaskType, inTask->fInheritedId, &inheritedTask) == noErr
		&& inheritedTask->fBequeathId == taskId)
			inheritedTask->setBequeathId(bequeathId);
	}
	if (inTask == gKernelScheduler->currentTask())
		gKernelScheduler->setCurrentTask(NULL);

	delete inTask;

	gObjectTable->reassignOwnership(taskId, bequeathId);
	gTaskDestroyed = YES;
	CheckCopyTask();
}


static void
DeletePort(CPort * inPort)
{ delete inPort; }

static void
DeleteSemList(CSemaphoreOpList * inSemaphoreOpList)
{ delete inSemaphoreOpList; }

static void
DeleteSemGroup(CSemaphoreGroup * inSemaphoreGroup)
{ delete inSemaphoreGroup; }

static void
DeleteSharedMem(CSharedMem * inSharedMem)
{ delete inSharedMem; CheckCopyTask(); }

static void
DeleteSharedMemMsg(CSharedMemMsg * inSharedMemMsg)
{ delete inSharedMemMsg; CheckCopyTask(); }

static void
DeleteMonitor(CMonitor * inMonitor)
{ delete inMonitor; }

static void
DeletePhys(CPhys * inPhys)
{ delete inPhys; }

static void
DeleteNothing(CObject * inObj)
{ }


#pragma mark -
/*------------------------------------------------------------------------------
	C O b j e c t T a b l e
	There are two instances of CObjectTable:
		the kernel object table
		memory architecture page object table
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Initialize the object table.
	Set up a null scavenging procedure and clear the table.
	The table is a hash table indexed by object id (without object type).
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CObjectTable::init(void)
{
	fScavenge = NullScavenger;
	fScavengeIndex = 0;
	for (ArrayIndex i = 0; i < kObjectTableSize; ++i)
		fObject[i] = NULL;

	return noErr;
}


/*------------------------------------------------------------------------------
	Set the function that determines the scavenging procedure.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CObjectTable::setScavengeProc(GetScavengeProcPtr inScavenger)
{
	fScavenge = inScavenger;
}


/*------------------------------------------------------------------------------
	Scavenge dead resources.
	Each call to this function scavenges one entry in the object table;
	successive entries are scavenged on successive calls.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CObjectTable::scavenge(void)
{
	if (fScavenge)
	{
		// start with the current entry
		fThisObj = fObject[fScavengeIndex];
		fPrevObj = NULL;
		while (fThisObj)
		{
			if (fThisObj->fId != fThisObj->owner() && !exists(fThisObj->owner()))
			{
				// we’ve found an object with no owner - a suitable candidate
				CObject * deadObject = fThisObj;
				fThisObj = deadObject->fNext;
				deadObject->setOwner(deadObject->fId);	// make owner invalid
				ScavengeProcPtr scavenger = fScavenge(deadObject, deadObject->fId);
				if (scavenger)
				{
					// update hash chain pointers around the dead object
					if (fPrevObj)
						fPrevObj->fNext = deadObject->fNext;
					else
						fObject[fScavengeIndex] = deadObject->fNext;
					// delete the dead object
					scavenger(deadObject);
				}
			}
			else
			{
				// this object is still alive - move on down the chain
				fPrevObj = fThisObj;
				fThisObj = fPrevObj->fNext;
			}
		}
		fPrevObj = NULL;
		// try the next entry next time
		fScavengeIndex = (fScavengeIndex + 1) & kObjectTableMask;
	}
}


/*------------------------------------------------------------------------------
	Scavenge all the dead resources in the table.
	Simply iterate over every object in the table.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CObjectTable::scavengeAll(void)
{
	for (ArrayIndex i = 0; i < kObjectTableSize; ++i)
		scavenge();
}


/*------------------------------------------------------------------------------
	Reassign all objects with one owner to another owner.
	Args:		inOwnerId		current owning task
				inNewOwnerId	new owner
	Return:	--
------------------------------------------------------------------------------*/

void
CObjectTable::reassignOwnership(ObjectId inOwnerId, ObjectId inNewOwnerId)
{
	if (exists(inNewOwnerId))
	{
		for (ArrayIndex i = 0; i < kObjectTableSize; ++i)
			for (CObject * obj = fObject[i]; obj != NULL; obj = obj->fNext)
				if (obj->owner() == inOwnerId)
					obj->setOwner(inNewOwnerId);
	}
}


/*------------------------------------------------------------------------------
	Return a unique id.
	Args:		--
	Return:	the id
------------------------------------------------------------------------------*/
static bool			gWrappedGlobalUniqueId = NO;	// 0C105540

ObjectId
CObjectTable::nextGlobalUniqueId(void)
{
static ObjectId	gNextGlobalUniqueId = 0;		// 0C105544
	ObjectId	uid;

	EnterAtomic();
	uid = ++gNextGlobalUniqueId;
	if (uid == 256)
		gWrappedGlobalUniqueId = YES;
	ExitAtomic();

	return uid;
}


/*------------------------------------------------------------------------------
	Convert ObjectId to index into object table.
	Args:		inId
	Return:	the index
------------------------------------------------------------------------------*/

inline ArrayIndex
CObjectTable::tableIndex(ObjectId inId)
{ return (inId >> kObjectTypeBits) & kObjectTableMask; }


/*------------------------------------------------------------------------------
	Return a unique id for an object.
	Args:		inType		the type of object
	Return:	the id
------------------------------------------------------------------------------*/

ObjectId
CObjectTable::newId(KernelTypes inType)
{
	ObjectId	uid = (nextGlobalUniqueId() << kObjectTypeBits) | inType;

	if (gWrappedGlobalUniqueId)
	{
		bool				isPhys = (inType == kPhysType || inType == kExtPhysType);
		CObjectTable *	object = (this == gObjectTable) ? gTheMemArchObjTbl : gObjectTable;
		// check new id doesn’t already exist
		while (exists(uid) && isPhys && object->exists(uid))
			uid = (nextGlobalUniqueId() << kObjectTypeBits) | inType;
	}
	
	return uid;
}


/*------------------------------------------------------------------------------
	Determine whether an object already exists in the table.
	Args:		inId			the object’s id
	Return:	YES if it exists
------------------------------------------------------------------------------*/

bool
CObjectTable::exists(ObjectId inId)
{
	ArrayIndex i = tableIndex(inId);
	for (CObject * obj = fObject[i]; obj != NULL; obj = obj->fNext)
	{
		if (obj->fId == inId)
			return YES;
	}

	return NO;
}


/*------------------------------------------------------------------------------
	Get a pointer to an object from its id.
	Args:		inId			the object’s id
	Return:	the object
------------------------------------------------------------------------------*/

CObject *
CObjectTable::get(ObjectId inId)
{
	ArrayIndex i = tableIndex(inId);
	for (CObject * obj = fObject[i]; obj != NULL; obj = obj->fNext)
	{
		if (obj->fId == inId)
		{
			if (obj->fId != kNoId)
				return obj;
			break;
		}
	}

	return NULL;
}


/*------------------------------------------------------------------------------
	Add an object to the table.
	Args:		ioObject			the object
				inType			its type
				inOwnerId		its owning task
	Return:	the object's id
------------------------------------------------------------------------------*/

ObjectId
CObjectTable::add(CObject * ioObject, KernelTypes inType, ObjectId inOwnerId)
{
	ObjectId	id = newId(inType);

	ioObject->fId = id;
	// system objects have no real owner - so they’re never scavenged
	if (inOwnerId == kSystemId)
		inOwnerId = id;
	ioObject->setOwner(inOwnerId);
	ioObject->assignToTask(inOwnerId);

	EnterFIQAtomic();
	// hash the id to get the table entry
	ArrayIndex i = tableIndex(id);
	CObject ** tableEntry = &fObject[i];
	// insert the object at the head of the hash chain
	ioObject->fNext = *tableEntry;
	*tableEntry = ioObject;
	ExitFIQAtomic();
	return *ioObject;
}


/*------------------------------------------------------------------------------
	Remove an object from the table.
	Args:		inId				the object’s id
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CObjectTable::remove(ObjectId inId)
{
	if (ObjectType(inId) != kNoType)
	{
		CObject * prev = NULL;
		ArrayIndex i = tableIndex(inId);

		for (CObject * obj = fObject[i]; obj != NULL; prev = obj, obj = obj->fNext)
		{
			if (obj->fId == inId)
			{
				// we’ve found the table entry with the given id
				obj->setOwner(obj->fId);	// make owner invalid
				ScavengeProcPtr scavenger = fScavenge(obj, inId);
				if (scavenger)
				{
					// update hash chain pointers
					if (prev)
						prev->fNext = obj->fNext;
					else
						fObject[i] = obj->fNext;
					if (obj == fThisObj)
						// this object was due for scavenging so update pointer
						fThisObj = obj->fNext;
					else if (obj == fPrevObj)
						// this object was just scavenged so update pointer
						fPrevObj = prev;
					scavenger(obj);
				}
				return noErr;
			}
		}
	}

	return kOSErrItemNotFound;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C O b j e c t T a b l e I t e r a t o r
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Constructor.
	Args:		inTable			the table
				inId				id of object where iteration is to start
	Return:	--
------------------------------------------------------------------------------*/

CObjectTableIterator::CObjectTableIterator(CObjectTable * inTable, ObjectId inId)
{
	fInitialIndex = (inId >> kObjectTypeMask) & kObjectTableMask;
	fInitialId = inId;
	fObject = NULL;
	fTable = inTable;
	setCurrentPosition(inId);
}


/*------------------------------------------------------------------------------
	Set the iterator at a position.
	Args:		inId				the object’s id
	Return:	YES if the position was set successfully
------------------------------------------------------------------------------*/

bool
CObjectTableIterator::setCurrentPosition(ObjectId inId)
{
	fObject = NULL;
	fIndex = fTable->tableIndex(inId);
	fIsDone = (fIndex == fInitialIndex && inId > fInitialId);
	do
	{
		if (!getThisLineNextEntry())
			return NO;
	} while (fId != kNoId && fId > inId);

	return YES;
}


/*------------------------------------------------------------------------------
	Update parameters for the next entry in the hash chain.
	Args:		--
	Return:	YES if there is an entry
------------------------------------------------------------------------------*/

bool
CObjectTableIterator::getThisLineNextEntry(void)
{
	fObject = fTable->fObject[(fObject == NULL) ? fInitialIndex : fIndex];
	if (fObject)
	{
		// we found an object, its id is the iterator’s value
		fId = (ObjectId)*fObject;
		return YES;
	}

	// if we’re done, say so now
	fId = kNoId;
	if (fIsDone)
		return NO;

	// update the index - if wrapped to the beginning, we’re done
	fIndex = (fIndex + 1) & kObjectTableMask;
	if (fIndex == fInitialIndex)
		fIsDone = YES;

	return YES;
}


/*------------------------------------------------------------------------------
	Return the next id existing in the table.
	Args:		--
	Return:	the id
------------------------------------------------------------------------------*/

ObjectId
CObjectTableIterator::getNextTableId(void)
{
	do
	{
		if (!getThisLineNextEntry())
			return fId;
	} while (fId == kNoId);

	if (fIsDone && fId <= fInitialId)
		fId = kNoId;

	return fId;
}


/*------------------------------------------------------------------------------
	Return the next id of the given type.
	Args:		inType			the object type
	Return:	the id
------------------------------------------------------------------------------*/

ObjectId
CObjectTableIterator::getNextTypedId(KernelTypes inType)
{
	ObjectId	id;

	do
	{
		id = getNextTableId();
	} while (id != kNoId && ObjectType(id) != inType);
	return id;
}


/*------------------------------------------------------------------------------
	Return the next object of the given type.
	Args:		inType			the object type
	Return:	the object
------------------------------------------------------------------------------*/

CObject *
CObjectTableIterator::getNextTypedObject(KernelTypes inType)
{
	return fTable->get(getNextTypedId(inType));
}

