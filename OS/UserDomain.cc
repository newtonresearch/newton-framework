/*
	File:		UserDomain.cc

	Contains:	Address domain object code.

	Written by:	Newton Research Group.
*/

#include "KernelObjects.h"
#include "UserDomain.h"
#include "PageManager.h"
#include "ListIterator.h"
#include "OSErrors.h"


/*------------------------------------------------------------------------------
	C U D o m a i n
------------------------------------------------------------------------------*/

NewtonErr
CUDomain::init(ObjectId inMonitorId, VAddr inRangeStart, size_t inRangeLength)
{
	ObjectMessage	msg;

	msg.request.domain.monitorId = inMonitorId;
	msg.request.domain.rangeStart = inRangeStart;
	msg.request.domain.rangeLength = inRangeLength;
	return makeObject(kObjectDomain, &msg, MSG_SIZE(3));
}


NewtonErr
CUDomain::setFaultMonitor(ObjectId inMonitorId)
{
	ObjectMessage	msg;

	msg.size = MSG_SIZE(2);
	msg.request.faultMonitor.domainId = fId;
	msg.request.faultMonitor.monitorId = inMonitorId;
	
	return gUObjectMgrMonitor->invokeRoutine(kSetDomainFaultMonitor, &msg);
}


#pragma mark -
/*------------------------------------------------------------------------------
	C U D o m a i n M a n a g e r
------------------------------------------------------------------------------*/

CUMonitor	CUDomainManager::fDomainPageMonitor;		// 0C104EEC
CUMonitor	CUDomainManager::fDomainPageTableMonitor;	// 0C104EF4


/*------------------------------------------------------------------------------
	Constructor.
------------------------------------------------------------------------------*/

CUDomainManager::CUDomainManager()
{ }


/*------------------------------------------------------------------------------
	Destructor.
------------------------------------------------------------------------------*/

CUDomainManager::~CUDomainManager()
{
	CUDomain *		dom;
	CListIterator	iter(&fDomains);
	for (dom = (CUDomain *)iter.firstItem(); iter.more(); dom = (CUDomain *)iter.nextItem())
		if (dom != NULL)
			delete dom;
}


/*------------------------------------------------------------------------------
	S t a t i c   F u n c t i o n s
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Initialize page and page table monitors.
------------------------------------------------------------------------------*/

NewtonErr
CUDomainManager::staticInit(ObjectId inPageMonitorId, ObjectId inPageTableMonitorId)
{
	fDomainPageMonitor = inPageMonitorId;
	fDomainPageTableMonitor = inPageTableMonitorId;
	return noErr;
}

NewtonErr
CUDomainManager::allocatePageTable(void)
{
	return fDomainPageTableMonitor.invokeRoutine(kAllocatePageTable, 0);
}

NewtonErr
CUDomainManager::releasePageTable(VAddr inArg1)
{
	NewtonErr	err;
	if ((err = GenericSWI(63, inArg1)) == noErr)
		err = fDomainPageTableMonitor.invokeRoutine(kReleasePageTable, (void *)inArg1);
	return err;
}

NewtonErr
CUDomainManager::registerPageMonitor(void)
{
	ULong		args[3];
	args[0] = (ObjectId) fDomainPageMonitor;
	return fDomainPageMonitor.invokeRoutine(kRegister, args);
}


/*------------------------------------------------------------------------------
	Initialize.
------------------------------------------------------------------------------*/

NewtonErr
CUDomainManager::init(ObjectId inEnvironment, ULong inFaultMonName, ULong inPageMonName, size_t inFaultMonStackSize, size_t inPageMonStackSize)
{
	NewtonErr err;

	XTRY
	{
		XFAIL(err = fFaultMonitor.init(MemberFunctionCast(MonitorProcPtr, this, &CUDomainManager::faultMonProc),
												inFaultMonStackSize, this, inEnvironment, kIsFaultMonitor, inFaultMonName))
		XFAIL(err = fPageMonitor.init(MemberFunctionCast(MonitorProcPtr, this, &CUDomainManager::pageMonProc),
												inPageMonStackSize, this, inEnvironment, kIsntFaultMonitor, inPageMonName))
		XFAIL(err = fLock.init())
		CUPageManager::registerClient(fPageMonitor);
	}
	XENDTRY;

	return err;
}


NewtonErr
CUDomainManager::faultMonProc(int inSelector, void * inData)
{
	if (inSelector == -2)
	{
		NewtonErr		err;
		ProcessorState	state;
		f20 = NO;
		GetFaultState(&state);
		f24 = state.f54;
		err = fault(&state);		// vt00
		if (f20)
			SetFaultState(&state);
		return err;
	}
	return userRequest(inSelector, inData);	// vt04
}

NewtonErr
CUDomainManager::pageMonProc(int inSelector, void * inData)
{
	if (inSelector == 0x7FFFFFFF)	// CUPageManagerMonitor::releaseRequest
		return releaseRequest(inSelector);		// vt08
	return userRequest(inSelector, inData);	// vt04
}


NewtonErr
CUDomainManager::addDomain(ObjectId inId)
{
	CUDomain	domain(inId);
	return domain.setFaultMonitor(fFaultMonitor);
}


NewtonErr
CUDomainManager::addDomain(ObjectId & outDomainId, VAddr inRangeStart, size_t inRangeLength)
{
	NewtonErr err;

	XTRY
	{
		CUDomain * domain;
		XFAILNOT(domain = new CUDomain, err = kOSErrCouldNotCreateObject;)
		XFAIL(err = domain->init(fFaultMonitor, inRangeStart, inRangeLength))
		XFAILNOT(fDomains.insertUnique(domain), err = kOSErrDuplicateObject;)
		outDomainId = *domain;
	}
	XENDTRY;

	return err;
}

NewtonErr
CUDomainManager::getExternal(ObjectId & outArg1, int inArg2)
{
	NewtonErr	err;
	ULong			args[3];
	args[0] = gCurrentTaskId;
	args[1] = inArg2;
	if ((err = fDomainPageMonitor.invokeRoutine(1, args)) == noErr)
		outArg1 = args[0];
	return err;
}

NewtonErr
CUDomainManager::get(ObjectId & outArg1, int inArg2)
{
	NewtonErr	err;
	struct
	{
		ObjectId	id;
		int		selector;
		void *	context;
	} args;
	args.id = gCurrentTaskId;
	args.selector = inArg2;
	args.context = &fPageMonitor;
	if ((err = fDomainPageMonitor.invokeRoutine(5, &args)) == noErr)
		outArg1 = args.id;
	return err;
}

NewtonErr
CUDomainManager::releasePagesForFaultHandling(ULong inArg1, ULong inArg2)
{
	ULong	args[3];
	args[0] = inArg1;
	args[1] = inArg2;
	args[2] = 0;
	return fDomainPageMonitor.invokeRoutine(6, args);
}

NewtonErr
CUDomainManager::releasePagesFromOtherMonitorsForFaultHandling(ULong inArg1, ULong inArg2)
{
	ULong	args[3];
	args[0] = inArg1;
	args[1] = inArg2;
	args[2] = fPageMonitor;
	return fDomainPageMonitor.invokeRoutine(6, args);
}


NewtonErr
CUDomainManager::release(ObjectId inArg1)
{
	return GenericSWI(kPMRelease, inArg1);
}

NewtonErr
CUDomainManager::copyPhysPg(PSSId inToPage, PSSId inFromPage, ULong inSubPageMask)
{
	return GenericSWI(k18GenericSWI, inToPage, inFromPage, inSubPageMask);
}

NewtonErr
CUDomainManager::rememberPhysMapRange(ObjectId inArg1, ULong inArg2, ULong inArg3, ULong inArg4, ULong inArg5, bool inArg6)
{
	NewtonErr	err;
	ULong *		params = TaskSwitchedGlobals()->fParams;
	params[0] = inArg1;
	params[1] = inArg2;
	params[2] = inArg3;
	params[3] = inArg4;
	params[4] = inArg5;
	params[5] = inArg6;
	while ((err = GenericSWI(61)) == kOSErrNoPageTable)
		if ((err = CUDomainManager::allocatePageTable()) != noErr)
			break;
	return err;
}

NewtonErr
CUDomainManager::rememberPhysMapRange(ULong inArg1, ULong inArg2, ULong inArg3, ULong inArg4, bool inArg5)
{
	NewtonErr	err;
	ULong *		params = TaskSwitchedGlobals()->fParams;
	params[0] = f24;
	params[1] = inArg1;
	params[2] = inArg2;
	params[3] = inArg3;
	params[4] = inArg4;
	params[5] = inArg5;
	while ((err = GenericSWI(61)) == kOSErrNoPageTable)
		if ((err = CUDomainManager::allocatePageTable()) != noErr)
			break;
	return err;
}

NewtonErr
CUDomainManager::rememberPhysMap(ObjectId inArg1, VAddr inArg2, ULong inArg3, bool inArg4)
{
	NewtonErr	err;
	ULong			r7 = TRUNC(inArg2, kPageSize) | (inArg4 ? 0x01 : 0);
	while ((err = GenericSWI(10, inArg1, r7, inArg3)) == kOSErrNoPageTable)
		if ((err = CUDomainManager::allocatePageTable()) != noErr)
			break;
	return err;
}

NewtonErr
CUDomainManager::rememberPhysMap(VAddr inArg1, ULong inArg2, bool inArg3)
{
	NewtonErr	err;
	ULong			r7 = TRUNC(inArg1, kPageSize) | (inArg3 ? 0x01 : 0);
	while ((err = GenericSWI(10, f24, r7, inArg2)) == kOSErrNoPageTable)
		if ((err = CUDomainManager::allocatePageTable()) != noErr)
			break;
	return err;
}

NewtonErr
CUDomainManager::rememberPermMap(ObjectId inArg1, VAddr inArg2, ULong inArg3, Perm inArg4)
{
	NewtonErr	err;
	ULong			r6 = TRUNC(inArg2, kPageSize) | inArg4;
	while ((err = GenericSWI(11, inArg1, r6, inArg3)) == kOSErrNoPageTable)
		if ((err = CUDomainManager::allocatePageTable()) != noErr)
			break;
	return err;
}

NewtonErr
CUDomainManager::rememberPermMap(VAddr inArg1, ULong inArg2, Perm inArg3)
{
	NewtonErr	err;
	ULong			r6 = TRUNC(inArg1, kPageSize) | inArg3;
	while ((err = GenericSWI(11, f24, r6, inArg2)) == kOSErrNoPageTable)
		if ((err = CUDomainManager::allocatePageTable()) != noErr)
			break;
	return err;
}

NewtonErr
CUDomainManager::remember(ObjectId inArg1, VAddr inArg2, ULong inPermissions, ObjectId inArg4, bool inArg5)
{
	NewtonErr	err;
	ULong			r7 = TRUNC(inArg2, kPageSize) | (inArg5 ? 0x0100 : 0) | (inPermissions & 0xFF);
	while ((err = GenericSWI(12, inArg1, r7, inArg4)) == kOSErrNoPageTable)
		if ((err = CUDomainManager::allocatePageTable()) != noErr)
			break;
	return err;
}

NewtonErr
CUDomainManager::remember(VAddr inArg1, ULong inPermissions, ULong inArg3, bool inArg4)
{
	NewtonErr	err;
	ULong			r7 = TRUNC(inArg1, kPageSize) | (inArg4 ? 0x0100 : 0) | (inPermissions & 0xFF);
	while ((err = GenericSWI(12, f24, r7, inArg3)) == kOSErrNoPageTable)
		if ((err = CUDomainManager::allocatePageTable()) != noErr)
			break;
	return err;
}


NewtonErr
CUDomainManager::forgetPhysMapRange(ObjectId inArg1, VAddr inArg2, ULong inArg3, ULong inArg4)
{
	NewtonErr	err;
	ULong *		params = TaskSwitchedGlobals()->fParams;
	params[0] = inArg1;
	params[1] = inArg2;
	params[2] = inArg3;
	params[3] = inArg4;
	while ((err = GenericSWI(60)) == kOSErrNoPageTable)
		if ((err = CUDomainManager::allocatePageTable()) != noErr)
			break;
	return err;
}

NewtonErr
CUDomainManager::forgetPhysMapRange(VAddr inArg1, ULong inArg2, ULong inArg3)
{
	NewtonErr	err;
	ULong *		params = TaskSwitchedGlobals()->fParams;
	params[0] = f24;
	params[1] = inArg1;
	params[2] = inArg2;
	params[3] = inArg3;
	while ((err = GenericSWI(60)) == kOSErrNoPageTable)
		if ((err = CUDomainManager::allocatePageTable()) != noErr)
			break;
	return err;
}

NewtonErr
CUDomainManager::forgetPhysMap(ObjectId inArg1, VAddr inArg2, ULong inArg3)
{
	return GenericSWI(kForgetPhysMapping, inArg1, inArg2, inArg3);
}

NewtonErr
CUDomainManager::forgetPhysMap(VAddr inArg1, ULong inArg2)
{
	return GenericSWI(kForgetPhysMapping, f24, inArg1, inArg2);
}

NewtonErr
CUDomainManager::forgetPermMap(ObjectId inArg1, VAddr inArg2, ULong inArg3)
{
	return GenericSWI(kForgetPermMapping, inArg1, inArg2, inArg3);
}

NewtonErr
CUDomainManager::forgetPermMap(VAddr inArg1, ULong inArg2)
{
	return GenericSWI(kForgetPermMapping, f24, inArg1, inArg2);
}

NewtonErr
CUDomainManager::forget(ObjectId inArg1, VAddr inArg2, ULong inArg3)
{
	return GenericSWI(kForgetMapping, inArg1, inArg2, inArg3);
}

NewtonErr
CUDomainManager::forget(VAddr inArg1, ULong inArg2)
{
	return GenericSWI(kForgetMapping, f24, inArg1, inArg2);
}

