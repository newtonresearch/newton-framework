/*
	File:		Stackmanager.cc

	Contains:	Stack implementation.
					Task stacks start out with a minimal size and grow automatically.
					It is the job of the stack manager to grow (and shrink) stacks on demand.
					The stack manager also allocates address ranges for stacks.
					By default, the stack manager will allocate each new task a 32 KByte region of address space in the stack domain.
					Tasks can request larger stack regions if necessary.
					Initially, there is no storage associated with a task’s stack region.
					As the task uses the region, it faults and the stack manager allocates storage.
					Guard bands separate stack regions so that attempts to access outside of the region bounds can be detected.
					If a task generates an access in the guard area an exception will be generated.
	Written by:	Newton Research Group.
*/

#include "StackManager.h"
#include "Environment.h"
#include "KernelTasks.h"
#include "OSErrors.h"


#if !defined(correct)
#define kStackRegionSize (32*KByte)
#define kGuardedStackRegionSize (32*KByte)
#define kTwilightStackSize 0
#else
#define kStackRegionSize (32*KByte)
#define kGuardedStackRegionSize (33*KByte)
#define kTwilightStackSize (3*KByte)
#endif

/* -------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------- */
extern Heap			gKernelHeap;

Heap					gStackManagerHeap;	// 0C104C08
CStackManager *	gStackManager;			// 0C104C0C


/* -------------------------------------------------------------------------------
	I n i t i a l i s a t i o n
------------------------------------------------------------------------------- */

NewtonErr
MakeSystemStackManager(void)
{
	NewtonErr err;

	XTRY
	{
		XFAILNOT(gStackManager = new CStackManager, err = kOSErrNoMemory;)
		err = gStackManager->init();
		SetHeap(gKernelHeap);
		gStackManagerHeap = gKernelHeap;
	}
	XENDTRY;
	XDOFAIL(err)
	{
		if (gStackManager != NULL)
			delete gStackManager, gStackManager = NULL;
	}
	XENDFAIL;

	return err;
}


void
CopyPagesAfterStackCollided(CopyPageAfterStackCollisionParams * inParams)
{
//	CStackManager *	sm = inParams->f00;
//	sm->copyPagesAfterStackCollided(inParams);
}


void
GetFaultState(ProcessorState * outState)
{
	SMemCopyFromSharedSWI(kBuiltInSMemMonitorFaultId, outState, sizeof(ProcessorState), 0, kNoId, kNoId, NULL);
}


void
SetFaultState(ProcessorState * inState)
{
	SMemCopyToSharedSWI(kBuiltInSMemMonitorFaultId, inState, sizeof(ProcessorState), 0, kNoId, kNoId);
}


#pragma mark -
#pragma mark CStackManager
/*--------------------------------------------------------------------------------
	C S t a c k M a n a g e r
--------------------------------------------------------------------------------*/

CStackManager::CStackManager()
	:	fState(NULL),
		f44(offsetof(CStackPage, f04)),
		f58(offsetof(CStackPage, f04)),
		fDomains(offsetof(CStackPage, f04))
{

	f6C[ 0] = 0;	// subpage mask -> number of subpages? almost?
	f6C[ 1] = 1;
	f6C[ 2] = 1;
	f6C[ 3] = 2;

	f6C[ 4] = 1;
	f6C[ 5] = 3;
	f6C[ 6] = 2;
	f6C[ 7] = 4;

	f6C[ 8] = 1;
	f6C[ 9] = 3;
	f6C[10] = 3;
	f6C[11] = 5;

	f6C[12] = 2;
	f6C[13] = 5;
	f6C[14] = 4;
	f6C[15] = 6;

	fAC = 6;
	fC0 = NO;
}


CStackManager::~CStackManager()
{ }


/*--------------------------------------------------------------------------------
	Initialize.
--------------------------------------------------------------------------------*/

NewtonErr
CStackManager::init(void)
{
	NewtonErr err;
	XTRY
	{
		fRoundRobinDomain = NULL;
		fRoundRobinStackRegion = 0;
		XFAIL(err = CUDomainManager::init(kNoId, 'STKF', 'STKP', KByte, KByte))
		XFAIL(err = fStackMonitor.init(MemberFunctionCast(MonitorProcPtr, this, &CStackManager::safeUserRequestEntry), KByte, this, kNoId, kIsntFaultMonitor, 'STKU'))
		XFAIL(err = fC4.init())
		err = fE4.init();
	}
	XENDTRY;
	return err;
}


CStackPage *
CStackManager::allocNewPage(ObjectId inId)
{
	CStackPage * page;
	XTRY
	{
		XFAIL((page = new CStackPage) == NULL)
		XFAILIF(page->init(this, inId) != noErr, delete page; page = NULL;)
	}
	XENDTRY;
	return page;
}


NewtonErr
CStackManager::fault(ProcessorState * ioState)
{
	NewtonErr		err;
	ULong				r2;
	CStackInfo *	info;

	SetHeap(gStackManagerHeap);
	fC4.acquire(kWaitOnBlock);
	err = kStackErrStackOverflow;
	fState = ioState;
	if ((fState->f5C & 0x02000000)!= 0)
	{
		CTask *	task;
		ConvertIdToObj(kTaskType, ioState->f58, &task);
		r2 = task->fStackTop - fState->f50;
		if (r2 > task->fTaskDataSize)
			task->fTaskDataSize = r2;
	}
	if ((info = getStackInfo(fState->fAddr)) != NULL)
	{
		if ((info->fOptions & 0x02) == 0
		|| (r2 = 0x03 << ((fState->f48 >> 3) & 0x1E)), (fState->f60 & r2) == r2)
			err = resolveFault(info);
		else
			err = kOSErrPermissionViolation;
	}
	void *	exceptionData = fState->f34;
	fC4.release();

	if (err != noErr)
	{
		if (err == 4)
			Reboot(kOSErrSorrySystemError);
		else
			Throw(exBusError, exceptionData);
	}
	return err;
}


NewtonErr
CStackManager::resolveFault(CStackInfo * info)
{ return noErr; /*INCOMPLETE*/ }


NewtonErr
CStackManager::userRequest(int inSelector, void * inData)
{
	return kOSErrBadMonitorFunction;
}


NewtonErr
CStackManager::releaseRequest(int inSelector)
{
	return releaseRequest(inSelector, YES, NULL);
}


NewtonErr
CStackManager::releaseRequest(int inSelector, bool inArg2, ArrayIndex * outNumOfFreePages)
{
	NewtonErr err;
	CUEnvironment env;
	ObjectId envId;

	SetHeap(gStackManagerHeap);
	GetEnvironment(&envId);
	env = envId;

	fE4.acquire(kWaitOnBlock);	// == sp14
	fLock.acquire(kWaitOnBlock);	// == sp10

	ArrayIndex numOfReleasedPages = 0;

	if (inSelector == 2)
	{
		CStackInfo * lastRegion = NULL;
		CHeapDomain * domain;
		for (domain = (CHeapDomain *)fDomains.peek(); domain != NULL; domain = (CHeapDomain *)fDomains.getNext(domain))
		{
			for (ArrayIndex i = 0; i < domain->fStackRegionCount; ++i)
			{
				CStackInfo * stackRegion = domain->fStackRegion[i];
				if (stackRegion != NULL && stackRegion != lastRegion)
					numOfReleasedPages += releasePagesInOneStack(domain, env, stackRegion, inArg2);
				lastRegion = stackRegion;
			}
		}
	}

	CDoubleQContainer freePages(offsetof(CStackPage, f04));
	ArrayIndex numOfFreePages = gatherFreePages(freePages, inArg2) + numOfReleasedPages;
	if (freePages.peek())
	{
		fLock.release();
		CStackPage * page;
		while ((page = (CStackPage *)freePages.remove()) != NULL)
			delete page;
	}
	else
	{
		if (inSelector == 1)
		{
			do {
				numOfReleasedPages = roundRobinPageRelease(env, inArg2);
				numOfFreePages += numOfReleasedPages;
				gatherFreePages(freePages, inArg2);
			} while (numOfReleasedPages != 0 && freePages.peek());
		}
		fLock.release();
		CStackPage * page;
		while ((page = (CStackPage *)freePages.remove()) != NULL)
			delete page;
	}

	err = (numOfFreePages == 0) ? kOSErrNoMemory : noErr;
	fE4.release();

	if (outNumOfFreePages)
		*outNumOfFreePages = numOfFreePages;
	return noErr;
}


NewtonErr
CStackManager::safeUserRequestEntry(int inSelector, void * ioData)
{
	NewtonErr err;
	union
	{
		FM_NewStack_Parms						p1;
		FM_NewHeapArea_Parms					p2;
		FM_SetHeapLimits_Parms				p3;
		FM_Free_Parms							p4;
		FM_LockHeapRange_Parms				p6;
		FM_NewHeapDomain_Parms				p8;
		FM_AddPageMappingToDomain_Parms	p9;
		FM_SetRemoveRoutine_Parms			p10;
		FM_GetHeapAreaInfo_Parms			p11;
		FM_GetSystemReleaseable_Parms		p12;
	} parms;
	ArrayIndex parmSize = 0;

	SetHeap(gStackManagerHeap);
	memmove(&parms, ioData, sizeof(parms));
	fC4.acquire(kWaitOnBlock);

	switch (inSelector)
	{
	case kSM_NewStack:
		{
			CStackInfo *	info;
			err = FMNewStack(&parms.p1, &info);
			parmSize = sizeof(FM_NewStack_Parms);
		}
		break;

	case kSM_NewHeapArea:
		err = FMNewHeapArea(&parms.p2);
		parmSize = sizeof(FM_NewHeapArea_Parms);
		break;

	case kSM_SetHeapLimits:
		err = FMSetHeapLimits(&parms.p3);
		parmSize = sizeof(FM_SetHeapLimits_Parms);
		break;

	case kSM_FreePagedMem:
		err = FMFree(&parms.p4);
		parmSize = sizeof(FM_Free_Parms);
		break;
/*
	case 5:
		err = FMFreeHeapRange(&parms);
		parmSize = 8;
		break;
*/
	case kSM_LockHeapRange:
		err = FMLockHeapRange(&parms.p6);
		parmSize = sizeof(FM_LockHeapRange_Parms);
		break;

	case kSM_UnlockHeapRange:
		err = FMUnlockHeapRange(&parms.p3);
		parmSize = sizeof(FM_SetHeapLimits_Parms);
		break;

	case kSM_NewHeapDomain:
		err = FMNewHeapDomain(&parms.p8);
		parmSize = sizeof(FM_NewHeapDomain_Parms);
		break;

	case kSM_AddPageMappingToDomain:
		err = FMAddPageMappingToDomain(&parms.p9);
		parmSize = sizeof(FM_AddPageMappingToDomain_Parms);
		break;

	case kSM_SetRemoveRoutine:
		err = FMSetRemoveRoutine(&parms.p10);
		parmSize = sizeof(FM_SetRemoveRoutine_Parms);
		break;

	case kSM_GetHeapAreaInfo:
		err = FMGetHeapAreaInfo(&parms.p11);
		parmSize = sizeof(FM_GetHeapAreaInfo_Parms);
		break;

	case kSM_GetSystemReleasable:
		err = FMGetSystemReleaseable(&parms.p12);
		parmSize = sizeof(FM_GetSystemReleaseable_Parms);
		break;

	default:
		err = kOSErrBadMonitorFunction;
		break;
	}

	fC4.release();
	memmove(ioData, &parms, parmSize);	

	return err;
}


CStackInfo *
CStackManager::getStackInfo(VAddr inAddr)
{
	CStackInfo *	info = NULL;
	CHeapDomain *	domain;
	for (domain = (CHeapDomain *)fDomains.peek(); domain != NULL; domain = (CHeapDomain *)fDomains.getNext(domain))
		if (domain->getStackInfo(inAddr, &info) == noErr)
			break;
	return info;
}


CHeapDomain *
CStackManager::getDomainForAddress(VAddr inAddr)
{
	CStackInfo *	info;
	CHeapDomain *	domain;
	for (domain = (CHeapDomain *)fDomains.peek(); domain != NULL; domain = (CHeapDomain *)fDomains.getNext(domain))
		if (domain->getStackInfo(inAddr, &info) == noErr)
			return domain;
	return NULL;
}

#if !defined(correct)
CHeapDomain *
CStackManager::getDomainForCorrectAddress(VAddr inAddr)
{
	CStackInfo *	info;
	CHeapDomain *	domain;
	for (domain = (CHeapDomain *)fDomains.peek(); domain != NULL; domain = (CHeapDomain *)fDomains.getNext(domain))
		if (domain->getCorrectStackInfo(inAddr, &info) == noErr)
			return domain;
	return NULL;
}
#endif

CHeapDomain *
CStackManager::getMatchingDomain(ObjectId inId)
{
	CHeapDomain *	domain;
	for (domain = (CHeapDomain *)fDomains.peek(); domain != NULL; domain = (CHeapDomain *)fDomains.getNext(domain))
		if (*domain == inId)
			return domain;
	return NULL;
}


void
CStackManager::setRestrictedPage(ArrayIndex index, CStackPage * inPage)
{
	fB0[index] = inPage;
}


bool
CStackManager::checkRestrictedPage(CStackPage * inPage)
{
	return inPage == fB0[0] || inPage == fB0[1];
}


void
CStackManager::copyPageState(CStackPage * outPage, CStackPage * inPage, ULong inSubPageMask)
{
	for (ArrayIndex i = 0; i < kSubPagesPerPage; ++i)
	{
		if ((inSubPageMask & 1) != 0)
		{
			outPage->fStackInfo[i] = inPage->fStackInfo[i];
			outPage->fPageNo[i] = inPage->fPageNo[i];
			outPage->f28[i] = inPage->f28[i];
			outPage->f2C[i] = inPage->f2C[i];
		}
		inSubPageMask >>= 1;
	}
	updatePageState(outPage);
}


void
CStackManager::updatePageState(CStackPage * inPage)
{
	ULong	freeSubPageMask = 0;
	for (ArrayIndex i = 0; i < kSubPagesPerPage; ++i)
	{
		if (inPage->fStackInfo[i] == NULL)
			freeSubPageMask |= (1 << i);
	}
	inPage->fFreeSubPages = freeSubPageMask;
	if (freeSubPageMask == 0)
	{
	// page is full
		if (inPage->f04.fContainer != &f58)
		{
			f44.removeFromQueue(inPage);
			f58.add(inPage);
		}
	}
	else
	{
	//	page has fewer than its full complement of subpages
		if (inPage->f04.fContainer != &f44)
		{
			f58.removeFromQueue(inPage);
			f44.add(inPage);
		}
	}
}


void
CStackManager::findOrAllocPage_ReturnUnLockedOnNoPage(CStackInfo * info, ULong inPageNo, ULong inSubPageMask)
{
	VAddr pageAddr = info->fBase + inPageNo * kPageSize;
	CStackPage * page = getMatchingPage(info, inPageNo, inSubPageMask);
	XTRY
	{
		if (page == NULL)
		{
			fLock.release();
			page = allocNewPage(kNoId);
			XFAIL(page == NULL)
			XFAILIF(remember(info->fDomainId, pageAddr, 0, *page, YES) != noErr, page = NULL;)
			fLock.acquire(kWaitOnBlock);
			f44.add(page);
			pageMatchFound(info, inPageNo, inSubPageMask, page);
		}
		else
		{
			fLock.release();
			XFAILIF(remember(info->fDomainId, pageAddr, 0, *page, YES) != noErr, forgetMappings(info, inPageNo, page); page = NULL;)
			fLock.acquire(kWaitOnBlock);
		}
	}
	XENDTRY;
	return page;
}


ULong
CStackManager::buildPerms(CStackInfo * info, ULong inPageNo, CStackPage * inPage)
{
	ULong	subPagePermissions = 0;
	for (int i = kSubPagesPerPage-1; i >= 0; i--)
	{
		subPagePermissions <<= 2;
		if (inPage->fStackInfo[i] == info  &&  inPage->fPageNo[i] == inPageNo)
		{
			if ((info->fOptions & 0x02) == 0)
				subPagePermissions |= kReadWrite;
			else
				subPagePermissions |= kReadOnly;
		}
	}
	return subPagePermissions;
}


void
CStackManager::rememberMappings(CStackInfo * info, ULong inPageNo, CStackPage * inPage)
{
	CUDomainManager::remember(info->fDomainId, info->fBase + inPageNo * kPageSize, buildPerms(info, inPageNo, inPage), *inPage, (info->fOptions & 1) == 0);
}


void
CStackManager::forgetMappings(CStackInfo * info, CStackPage * inPage, VAddr inArg3)
{
	CUDomainManager::forget(info->fDomainId, inArg3, *inPage);
}


void
CStackManager::forgetMappings(CStackInfo * info, ULong inPageNo, CStackPage * inPage)
{
	removeOwnerFromPage(info, inPageNo, inPage);
	CUDomainManager::forget(info->fDomainId, inPageNo * kPageSize, *inPage);
	updatePageState(inPage);
}


void
CStackManager::removeOwnerFromPage(CStackInfo * info, ULong inPageNo, CStackPage * inPage)
{
	for (ArrayIndex i = 0; i < kSubPagesPerPage; ++i)
	{
		if (inPage->fStackInfo[i] == info  &&  inPage->fPageNo[i] == inPageNo)
		{
			inPage->fStackInfo[i] = NULL;
			inPage->fPageNo[i] = 0;
			inPage->f28[i] = 0;
			inPage->f2C[i] = 0;
		}
	}
}


void
CStackManager::freeSubPagesAbove(CStackInfo * info, VAddr inAddr, bool inArg3, ULong * outFreeSpace, bool inArg5)
{
	freeSubPagesBetween(info, inAddr, info->fAreaEnd - 1, inArg3, outFreeSpace, inArg5);
}


void
CStackManager::freeSubPagesBelow(CStackInfo * info, VAddr inAddr, bool inArg3, ULong * outFreeSpace, bool inArg5)
{
	freeSubPagesBetween(info, inAddr, info->fAreaStart, inArg3, outFreeSpace, inArg5);
}


NewtonErr
CStackManager::freeSubPagesBetween(CStackInfo * info, VAddr inAddr1, VAddr inAddr2, bool inArg4, ULong * outFreeSpace, bool inArg6)
{
//	r5: r4 r7 r6 r10 xx sp-4
	ULong freeSpace = 0;	// r9

	NewtonErr err;
	if ((err = checkRange(info, inAddr1)) != noErr
	||  (err = checkRange(info, inAddr2)) != noErr)
		return err;

	// order the address range
	if (inAddr1 > inAddr2)
	{
		VAddr sp = inAddr1;
		inAddr1 = inAddr2;
		inAddr2 = sp;
	}

	ArrayIndex startSubPageNo = (inAddr1 - info->fBase) / kSubPageSize;
	ArrayIndex startPageNo = startSubPageNo / kSubPagesPerPage;		// sp1C
	ULong startSubPageIndex = startSubPageNo & 0x03;					// sp18

	ArrayIndex endSubPageNo = (inAddr2 - info->fBase) / kSubPageSize;
	ArrayIndex endPageNo = endSubPageNo / kSubPagesPerPage;			// sp14
	ULong endSubPageIndex = endSubPageNo & 0x03;							// sp10

	for (ArrayIndex pageNo = startPageNo; pageNo <= endPageNo; pageNo++)	// r6
	{
		CStackPage * page = info->fPage[pageNo];
		if (page != NULL && !checkRestrictedPage(page))
		{
			ULong subPageNo = (pageNo == startPageNo) ? startSubPageIndex : 0;
			ULong subPageLimit = (pageNo == endPageNo) ? endSubPageIndex : 3;
			for ( ; subPageNo <= subPageLimit; subPageNo++)
			{
				if (page->fStackInfo[subPageNo] == info && page->fPageNo[subPageNo] == pageNo)
				{
					if (inArg6 || page->f28[subPageNo] == 0)
					{
						freeSpace += kSubPageSize;
						if (inArg4)
							setSubPageInfo(NULL, 0, page, subPageNo);
					}
				}
			}
			ULong	subPageMask;
			bool sp04, sp00;
			if (countMatches(info, pageNo, page, &subPageMask, &sp04, &sp00) == 0)
			{
				if (inArg4)
					forgetMappings(info, page, info->fBase + pageNo*kPageSize);
			}
			else
			{
				if (inArg4)
					rememberMappings(info, pageNo, page);
			}
		}
	}

	if (outFreeSpace)
		*outFreeSpace = freeSpace;
	return noErr;
}


void
CStackManager::unlockSubPagesBetween(CStackInfo * info, VAddr inAddr1, VAddr inAddr2)
{
	// order the address range
	if (inAddr1 > inAddr2)
	{
		VAddr sp = inAddr1;
		inAddr1 = inAddr2;
		inAddr2 = sp;
	}
	ArrayIndex startSubPageNo = (inAddr1 - info->fBase) / kSubPageSize;
	ArrayIndex startPageNo = startSubPageNo / kSubPagesPerPage;
	ULong startSubPageIndex = startSubPageNo & 0x03;

	ArrayIndex endSubPageNo = (inAddr2 - info->fBase) / kSubPageSize;
	ArrayIndex endPageNo = endSubPageNo / kSubPagesPerPage;
	ULong endSubPageIndex = endSubPageNo & 0x03;

	for (ArrayIndex pageNo = startPageNo; pageNo <= endPageNo; pageNo++)
	{
		CStackPage * page = info->fPage[pageNo];
		if (page != NULL)
		{
			ULong subPageNo = (pageNo == startPageNo) ? startSubPageIndex : 0;
			ULong subPageLimit = (pageNo == endPageNo) ? endSubPageIndex : 3;
			for ( ; subPageNo <= subPageLimit; subPageNo++)
			{
				if (page->fStackInfo[subPageNo] == info && page->fPageNo[subPageNo] == pageNo)
				{
					page->f28[subPageNo]--;
					if (page->f28[subPageNo] == 0)
						page->f2C[subPageNo] = 0;
				}
			}
		}
	}
}


CStackPage *
CStackManager::getMatchingPage(CStackInfo * info, ULong inPageNo, ULong inSubPageMask)
{
	CStackPage * matchingPage = NULL;
	int r0, r9 = fAC+1;
	for (CStackPage * page = (CStackPage *)f44.peek(); page != NULL; page = (CStackPage *)f44.getNext(page))
	{
		ULong subPageMask;
		if ((page->fFreeSubPages == 0x0F || (((info->fOptions == 0x01) ? 0 : 1) ^ page->f30_8) == 0)
		&& (subPageMask = (page->fFreeSubPages & inSubPageMask)) == inSubPageMask
		&& (r0 = f6C[subPageMask]) < r9)
		{
			r9 = r0;
			matchingPage = page;
			if (r9 == 0)
				break;
		}
	}
	if (matchingPage != NULL)
		pageMatchFound(info, inPageNo, inSubPageMask, matchingPage);
	return matchingPage;
	
}


size_t
CStackManager::gatherFreePages(CDoubleQContainer & outFreePages, bool inArg2)
{
	size_t memGathered = 0;
	for (CStackPage * page = (CStackPage *)f44.peek(); page != NULL; page = (CStackPage *)f44.getNext(page))
	{
		if (page->fFreeSubPages == 0x0F && !checkRestrictedPage(page))
		{
			// entire page is free
			if (inArg2)
			{
				f44.removeFromQueue(page);
				outFreePages.add(page);
			}
			memGathered += kPageSize;
		}
	}
	return memGathered;
}


#include "Scheduler.h"

ArrayIndex
CStackManager::releasePagesInOneStack(CHeapDomain * inDomain, CUEnvironment inEnvironment, CStackInfo * info, bool inArg4)
{
// r5: r6 xx r4 r7
	CUEnvironment env(inEnvironment);
	ULong freeSpace;
	ULong spaceFreed = 0;

	HoldSchedule();
	XTRY
	{
		CTask * task;
		XFAIL(ConvertIdToObj(kTaskType, info->fOwnerId, &task))
		XFAIL(checkRange(info, task->fRegister[kcTheStack]))
		freeSpace = 0;
		freeSubPagesBelow(info, task->fRegister[kcTheStack] - kSubPageSize, inArg4, &freeSpace, NO);
		spaceFreed = freeSpace;
	}
	XENDTRY;
	AllowSchedule();

	if (info->fProc != NULL)
	{
		bool isManager, hasDomain;
		env.hasDomain(*inDomain, &hasDomain, &isManager);
		if (!isManager)
			env.add(*inDomain, YES, NO, NO);

		HoldSchedule();
		VAddr stackStart, stackEnd;
		bool success = info->fProc(info->fRefCon, &stackStart, &stackEnd, inArg4);
		AllowSchedule();

		if (success && inArg4)
		{
			info->fStackStart = stackStart;
			info->fStackEnd = stackEnd;
		}

		freeSpace = 0;
		freeSubPagesBelow(info, stackStart - kSubPageSize, inArg4, &freeSpace, NO);
		spaceFreed += freeSpace;

		freeSpace = 0;
		freeSubPagesAbove(info, stackEnd + kSubPageSize, inArg4, &freeSpace, NO);
		spaceFreed += freeSpace;
	}

	return spaceFreed;
}


ArrayIndex
CStackManager::roundRobinPageRelease(CUEnvironment inEnvironment, bool inArg2)
{
	CHeapDomain * domain = fRoundRobinDomain;
	ArrayIndex stackRegionIndex = fRoundRobinStackRegion;

	ArrayIndex initialStackRegionIndex = stackRegionIndex;
	if (fRoundRobinDomain->fStackRegionCount < stackRegionIndex)
		initialStackRegionIndex = 0;
	CStackInfo * prevStackRegion = fRoundRobinDomain->fStackRegion[initialStackRegionIndex];

	do {
		stackRegionIndex++;
		if (stackRegionIndex >= fRoundRobinDomain->fStackRegionCount)
		{
			// reached end of this domain -- try the next
			stackRegionIndex = 0;
			domain = (CHeapDomain *)fDomains.getNext(domain);
			if (domain == NULL)
				domain = (CHeapDomain *)fDomains.peek();
		}
		CStackInfo * stackRegion = domain->fStackRegion[stackRegionIndex];
		if (stackRegion != NULL && prevStackRegion != NULL)
		{
			ArrayIndex numOfPagesReleased = releasePagesInOneStack(domain, inEnvironment, stackRegion, inArg2);
			if (numOfPagesReleased > 0)
			{
				fRoundRobinDomain = domain;
				fRoundRobinStackRegion = stackRegionIndex;
				return numOfPagesReleased;
			}
		}
		prevStackRegion = stackRegion;
	} while (domain != fRoundRobinDomain || stackRegionIndex != initialStackRegionIndex);

	return 0;
}


void
CStackManager::setSubPageInfo(CStackInfo * info, ArrayIndex inPageNo, CStackPage * inPage, ArrayIndex inSubPageIndex)
{
	inPage->fStackInfo[inSubPageIndex] = info;
	inPage->fPageNo[inSubPageIndex] = inPageNo;
	inPage->f28[inSubPageIndex] = 0;
	inPage->f2C[inSubPageIndex] = 0;
	updatePageState(inPage);
}


void
CStackManager::copyPagesAfterStackCollided(CopyPageAfterStackCollisionParams *)
{}


void
CStackManager::pageMatchFound(CStackInfo * info, ULong inPageNo, ULong inSubPageMask, CStackPage * inPage)
{
	for (ArrayIndex subPageIndex = 0; subPageIndex < kSubPagesPerPage; subPageIndex++, inSubPageMask >>= 1)
	{
		if ((inSubPageMask & 0x01) != 0)
		{
			inPage->fStackInfo[subPageIndex] = info;
			inPage->fPageNo[subPageIndex] = inPageNo;
			inPage->f28[subPageIndex] = 0;
			inPage->f2C[subPageIndex] = 0;
		}
	}
	updatePageState(inPage);
}


ArrayIndex
CStackManager::countMatches(CStackInfo * info, ArrayIndex inPageNo, CStackPage * inPage, ULong * outSubPageMask, bool * outArg5, bool * outArg6)
{
	ArrayIndex count = 0;
	*outSubPageMask = 0;
	*outArg5 = NO;
	*outArg6 = NO;
	for (ArrayIndex subPageIndex = 0; subPageIndex < kSubPagesPerPage; subPageIndex++)
	{
		if (inPage->fStackInfo[subPageIndex] == info && inPage->fPageNo[subPageIndex] == inPageNo)
		{
			count++;
			*outSubPageMask |= (1 << subPageIndex);
			if (inPage->f28[subPageIndex] != 0)
			{
				*outArg5 = YES;
				if (inPage->f2C[subPageIndex] != 0)
					*outArg6 = YES;
			}
		}
	}
	return count;
}


#pragma mark -
/* -------------------------------------------------------------------------------
	Create a new stack in an existing heap domain.
	Args:		ioParms
				outInfo
	Return: error code
------------------------------------------------------------------------------- */

NewtonErr
CStackManager::FMNewStack(FM_NewStack_Parms * ioParms, CStackInfo ** outInfo)
{
	NewtonErr		err;
	ArrayIndex		regionIndex, regionLimit;
	ArrayIndex		regionsRequired, regionsAvailable;
	ArrayIndex		firstRegionInRange, lastRegionInRange;
	VAddr				stackRegionStart, stackRegionEnd, stackBase;
	CHeapDomain *	domain;

	XTRY
	{
		XFAILNOT(domain = getMatchingDomain(ioParms->domainId), err = kOSErrBadParameters;)
//printf("CStackManager::FMNewStack() domain=#%08X, addr=#%08X, size=#%08X", domain, ioParms->addr, (ULong)ioParms->maxSize);

		VAddr	atAddr = ioParms->addr;
		if (atAddr != 0)
		{
			//	use specified part of existing domain
			atAddr -= kTwilightStackSize;		// r6 but at an address 3K lower to allow stack to grow

#if !defined(correct)
			XFAILIF(getDomainForCorrectAddress(atAddr) != domain, err = kStackErrAddressOutOfRange;)
			ULong		offset = atAddr - domain->fCorrectBase;
#else
			XFAILIF(getDomainForAddress(atAddr) != domain, err = kStackErrAddressOutOfRange;)
			ULong		offset = atAddr - domain->fBase;		// r9
#endif
			ioParms->maxSize += (offset - (offset / kGuardedStackRegionSize) * kGuardedStackRegionSize	// add extra within page
									+ kTwilightStackSize);			// add 3K barrier
			if (ioParms->maxSize < kGuardedStackRegionSize)
				ioParms->maxSize = kGuardedStackRegionSize;		// ensure minimum size
			regionsRequired = (ioParms->maxSize - 1) / kGuardedStackRegionSize + 1;		// r7
			regionIndex = offset / kGuardedStackRegionSize;
			regionLimit = regionIndex + regionsRequired;
			XFAILIF(regionLimit > domain->fStackRegionCount, err = kStackErrNoRoomForHeap;)
		}
		else
		{
		//	use the entire domain
			ioParms->maxSize += kTwilightStackSize;
			if (ioParms->maxSize < kGuardedStackRegionSize)
				ioParms->maxSize = kGuardedStackRegionSize;
			regionsRequired = (ioParms->maxSize - 1) / kGuardedStackRegionSize + 1;
			regionIndex = 0;
			regionLimit = domain->fStackRegionCount;
		}

		// look for a block of free regions to make the required size
//printf(" => %d stack regions\n", regionsRequired);

		firstRegionInRange = kIndexNotFound;
		for (regionsAvailable = 0; regionIndex < regionLimit && regionsAvailable < regionsRequired; regionIndex++)
		{
			if (firstRegionInRange == kIndexNotFound)
			{
				if (domain->fStackRegion[regionIndex] == NULL)
				{
					firstRegionInRange = regionIndex;
					regionsAvailable = 1;
				}
			}
			else
			{
				if (domain->fStackRegion[regionIndex] == NULL)
					regionsAvailable++;
				else
				{
					firstRegionInRange = kIndexNotFound;
					regionsAvailable = 0;
				}
			}
		}
		XFAILIF(regionsAvailable != regionsRequired, err = kStackErrNoRoomForHeap;)

		// set up stack pointers
		lastRegionInRange = firstRegionInRange + regionsRequired;	// actually BEYOND the range
		stackRegionStart = domain->fBase + (firstRegionInRange * kGuardedStackRegionSize);
		stackRegionEnd = domain->fBase + (lastRegionInRange * kGuardedStackRegionSize);
		stackBase = stackRegionEnd - (regionsRequired * kGuardedStackRegionSize) + kTwilightStackSize;	// don’t enter the twilight zone

		// create info describing the stack
		CStackInfo * info;
		XFAILNOT(info = new CStackInfo, err = kOSErrNoMemory;)
		XFAIL(err = info->init(stackRegionEnd, stackBase,
									  (stackRegionEnd - stackRegionStart)/kPageSize + 1,
									  ioParms->ownerId,
									  stackRegionStart,
									  *domain))

		// all regions share that same info
		fLock.acquire(kWaitOnBlock);
		for (ArrayIndex i = firstRegionInRange; i < lastRegionInRange; ++i)
			domain->fStackRegion[i] = info;
		fLock.release();

		// return info
		*outInfo = info;
		ioParms->stackTop = stackRegionEnd;
		ioParms->stackBottom = stackBase;
	}
	XENDTRY;
	return err;
}


/* -------------------------------------------------------------------------------
	Create a new heap in an existing heap domain.
	This is the same as creating a stack, really.
	Args:		ioParms
				outInfo
	Return: error code
------------------------------------------------------------------------------- */

NewtonErr
CStackManager::FMNewHeapArea(FM_NewHeapArea_Parms * ioParms)
{
	NewtonErr err;
	XTRY
	{
		CStackInfo *		info;
		FM_NewStack_Parms	parms;
		parms.domainId = ioParms->domainId;
		parms.addr = ioParms->addr;
		parms.maxSize = ioParms->maxSize;
		parms.ownerId = kNoId;
		XFAIL(err = FMNewStack(&parms, &info))
		
		info->fOptions |= ioParms->options;
		info->fStackStart = (ioParms->addr == 0) ? info->fAreaStart : info->fAreaEnd;
		info->fStackEnd = info->fStackStart + ioParms->maxSize;

		ioParms->areaStart = info->fStackStart;
		ioParms->areaEnd = info->fStackEnd;
	}
	XENDTRY;
	return err;
}


NewtonErr
CStackManager::validateHeapLimitsParms(FM_SetHeapLimits_Parms * ioParms)
{
	NewtonErr err;
	XTRY
	{
		CStackInfo * info;
		XFAILNOT(info = getStackInfo(ioParms->start), err = kStackErrAddressOutOfRange;)

		if ((err = checkRange(info, ioParms->start)) == noErr)
			err = checkRange(info, ioParms->end);
	}
	XENDTRY;
	return err;
}


NewtonErr
CStackManager::checkRange(CStackInfo * info, VAddr inAddr)
{
	if (inAddr > info->fAreaEnd || inAddr < info->fAreaStart)
		return kStackErrAddressOutOfRange;
	return noErr;
}


NewtonErr
CStackManager::FMSetHeapLimits(FM_SetHeapLimits_Parms * ioParms)
{
	NewtonErr err;
	if ((err = validateHeapLimitsParms(ioParms)) == noErr)
	{
		CStackInfo *	info;
		info = getStackInfo(ioParms->start);
		fLock.acquire(kWaitOnBlock);
		info->fStackStart = ioParms->start;
		info->fStackEnd = ioParms->end;
		freeSubPagesBelow(info, ioParms->start - kSubPageSize, YES, NULL, NO);
		freeSubPagesAbove(info, ioParms->end-1 + kSubPageSize, YES, NULL, NO);
		fLock.release();
	}
	return err;
}


NewtonErr
CStackManager::FMFreeHeapRange(FM_SetHeapLimits_Parms * ioParms)
{
	NewtonErr err;
	if ((err = validateHeapLimitsParms(ioParms)) == noErr)
	{
		CStackInfo *	info;
		info = getStackInfo(ioParms->start);
		freeSubPagesBetween(info, ioParms->start, ioParms->end, YES, NULL, NO);
	}
	return err;
}


NewtonErr
CStackManager::FMFree(FM_Free_Parms * ioParms)
{
	NewtonErr		err;
	CStackInfo *	info;
	CHeapDomain *	domain;

	fE4.acquire(kWaitOnBlock);
	XTRY
	{
		XFAILNOT(domain = getDomainForAddress(ioParms->addr), err = kStackErrBadDomain;)
		XFAIL(err = domain->getStackInfo(ioParms->addr, &info))
		XFAILNOT(info, err = kStackErrBogusStack;)

//ArrayIndex count = 0;
		int index = ((info->fAreaEnd - 1) - domain->fBase) / kGuardedStackRegionSize;
		fLock.acquire(kWaitOnBlock);
		freeSubPagesBelow(info, info->fAreaEnd - 1, YES, NULL, YES);	// ie everything
		for ( ; index >= 0; index--)
		{
			if (domain->fStackRegion[index] != info)
				break;
			domain->fStackRegion[index] = NULL;
//count++;
		}
		fLock.release();
		delete info;
//printf("CStackManager::FMFree() domain #%08X freed %d stack regions\n", domain, count);
	}
	XENDTRY;
	fE4.release();
	return err;
}


NewtonErr
CStackManager::FMLockHeapRange(FM_LockHeapRange_Parms * ioParms)
{
	NewtonErr		err;
	ProcessorState	sp00;
	if ((err = validateHeapLimitsParms((FM_SetHeapLimits_Parms *)ioParms)) == noErr)
	{
		CStackInfo *	info = getStackInfo(ioParms->start);	// r7
		VAddr	subPage;
		VAddr	firstSubPage = TRUNC(ioParms->start, kSubPageSize);
		VAddr	lastSubPage = TRUNC(ioParms->end, kSubPageSize);
		fC0 = YES;
//		fState = sp00;
		for (subPage = firstSubPage; subPage <= lastSubPage; subPage += kSubPageSize)
		{
			sp00.fAddr = subPage;
			if ((err = resolveFault(info)) != noErr)
			{
				if (subPage != firstSubPage)
					unlockSubPagesBetween(info, firstSubPage, subPage - kSubPageSize);
				goto fail;
			}
		}
		if (ioParms->wire)
			for (subPage = firstSubPage ; subPage <= lastSubPage; subPage += kSubPageSize)
			{
			//	r0 = (subPage - info->fBase) / kSubPageSize;
			//	r0 = info->fPage[r0/4] + (r0 & 0x03);	// subpage within page?
			//	r0->f2C = YES;	// isWired?
			}
	}
fail:
	fC0 = NO;
	return err;
}


NewtonErr
CStackManager::FMUnlockHeapRange(FM_SetHeapLimits_Parms * ioParms)
{
	NewtonErr err;
	if ((err = validateHeapLimitsParms(ioParms)) == noErr)
	{
		CStackInfo *	info;
		info = getStackInfo(ioParms->start);
		unlockSubPagesBetween(info, ioParms->start, ioParms->end);
	}
	return err;
}


/* -------------------------------------------------------------------------------
	Create a new heap domain.
	Args:		ioParms
	Return:	error code
------------------------------------------------------------------------------- */

NewtonErr
CStackManager::FMNewHeapDomain(FM_NewHeapDomain_Parms * ioParms)
{
	NewtonErr err;
	XTRY
	{
		CHeapDomain *	domain;
		XFAILNOT(domain = new CHeapDomain, err = kOSErrNoMemory;)
		XFAIL(err = domain->init(this, ioParms->sectionBase, ioParms->sectionCount))

		ioParms->domainId = *domain;
		fLock.acquire(kWaitOnBlock);
		fDomains.add(domain);
		if (fRoundRobinDomain == NULL)
		{
			fRoundRobinDomain = domain;
			fRoundRobinStackRegion = 0;
		}
		fLock.release();
	}
	XENDTRY;
	return err;
}


NewtonErr
CStackManager::FMAddPageMappingToDomain(FM_AddPageMappingToDomain_Parms * ioParms)
{
	NewtonErr err;

	XTRY
	{
		ArrayIndex		index;
		CStackPage *	page;
		CStackInfo *	info;

		XFAILNOT(info = getStackInfo(ioParms->addr), err = kStackErrAddressOutOfRange;)
		XFAILIF(info->fDomainId != ioParms->domainId, err = kStackErrAddressOutOfRange;)
		XFAIL(err = CUDomainManager::remember(ioParms->domainId, ioParms->addr, 0, ioParms->pageId, YES))
		XFAILNOT(page = allocNewPage(ioParms->pageId), err = kOSErrNoMemory;)
		fLock.acquire(kWaitOnBlock);
		index = (ioParms->addr - info->fBase) / kPageSize;
		info->fPage[index] = page;
		page->f30_8 = ((info->fOptions & 1) == 0);
		for (ArrayIndex i = 0; i < kSubPagesPerPage; ++i)
			setSubPageInfo(info, index, page, i);
		rememberMappings(info, index, page);
		fLock.release();
	}
	XENDTRY;

	return err;
}


NewtonErr
CStackManager::FMSetRemoveRoutine(FM_SetRemoveRoutine_Parms * ioParms)
{
	CStackInfo *	info;
	if ((info = getStackInfo(ioParms->addr)) == NULL)
		return kStackErrAddressOutOfRange;

	info->fProc = ioParms->proc;
	info->fRefCon = ioParms->refCon;
	return noErr;
}


NewtonErr
CStackManager::FMGetHeapAreaInfo(FM_GetHeapAreaInfo_Parms * ioParms)
{
	CStackInfo *	info;
	if ((info = getStackInfo(ioParms->addr)) == NULL)
		return kStackErrAddressOutOfRange;

	ioParms->areaStart = info->fAreaStart;
	ioParms->areaEnd = info->fAreaEnd;
	return noErr;
}


NewtonErr
CStackManager::FMGetSystemReleaseable(FM_GetSystemReleaseable_Parms * ioParms)
{
	NewtonErr		err;
	CStackPage *	page;
	CStackInfo *	info;

	err = releaseRequest(2, NO, &ioParms->releasable);

	fLock.acquire(kWaitOnBlock);
	ioParms->stackUsed = 0;
	ioParms->pagesUsed = 0;
	for (page = (CStackPage *)f58.peek(); page != NULL; page = (CStackPage *)f58.getNext(page))
	{
		ioParms->pagesUsed++;
		for (ArrayIndex i = 0; i < kSubPagesPerPage; ++i)
			if ((info = page->fStackInfo[i]) != NULL && info->fOwnerId != kNoId)
				ioParms->stackUsed += kSubPageSize;
	}
	for (page = (CStackPage *)f44.peek(); page != NULL; page = (CStackPage *)f44.getNext(page))
	{
		ioParms->pagesUsed++;
		for (ArrayIndex i = 0; i < kSubPagesPerPage; ++i)
			if ((info = page->fStackInfo[i]) != NULL && info->fOwnerId != kNoId)
				ioParms->stackUsed += kSubPageSize;
	}
	fLock.release();

	return err;
}


#pragma mark -
#pragma mark CStackPage
/*--------------------------------------------------------------------------------
	C S t a c k P a g e
	A kPageSize area of memory used as stack.
--------------------------------------------------------------------------------*/

CStackPage::CStackPage()
	: fPageId(kNoId)
{
	for (ArrayIndex i = 0; i < kSubPagesPerPage; ++i)
	{
		fStackInfo[i] = NULL;
		fPageNo[i] = 0;
		f28[i] = 0;
		f2C[i] = 0;
	}
	fFreeSubPages = 0x0F;
	f30_8 = YES;
}


CStackPage::~CStackPage()
{
	if (fPageId != kNoId && fIsPageOurs)
		// we own it; release it
		CUDomainManager::release(fPageId);
}


NewtonErr
CStackPage::init(CUDomainManager * inManager, ObjectId inPageId)
{
	if (inPageId != kNoId)
	{
		//	reuse existing page
		fPageId = inPageId;
		// we don’t own it
		fIsPageOurs = NO;
		return noErr;
	}
	//	get our own
	fIsPageOurs = YES;
	return inManager->get(fPageId, 2);
}


#pragma mark -
#pragma mark CStackInfo
/* -------------------------------------------------------------------------------
	C S t a c k I n f o
	Information about a stack region - address limits & allocated pages.
------------------------------------------------------------------------------- */

CStackInfo::CStackInfo()
	: fPage(NULL)
{ }


CStackInfo::~CStackInfo()
{
	if (fPage != NULL)
		delete[] fPage;
}


/* -------------------------------------------------------------------------------
	Initialize stack region info.
	Args:		inStackTop			address of stack top
				inStackBase			address of stack base
				inNumOfPages		number of kPageSize pages required
				inOwnerId
				inRegionBase		address of stack region
				inDomainId
	Return:	error code
------------------------------------------------------------------------------- */

NewtonErr
CStackInfo::init(VAddr inStackTop, VAddr inStackBase, ArrayIndex inNumOfPages, ObjectId inOwnerId, VAddr inRegionBase, ObjectId inDomainId)
{
	NewtonErr err = noErr;

	XTRY
	{
		XFAILNOT(fPage = new CStackPage*[inNumOfPages], err = kOSErrNoMemory;)

		for (ArrayIndex i = 0; i < inNumOfPages; ++i)
			fPage[i] = NULL;
		fNumOfPages = inNumOfPages;

		fAreaEnd = inStackTop;
		fAreaStart = inStackBase;

		fOptions = 0;
		fDomainId = inDomainId;

		fStackEnd = inStackTop;
		fStackStart = inStackBase;
		fBase = inRegionBase;
		fOwnerId = inOwnerId;

		fProc = NULL;
		fRefCon = NULL;
	}
	XENDTRY;

	return err;
}


#pragma mark -
#pragma mark CHeapDomain
/* -------------------------------------------------------------------------------
	C H e a p D o m a i n
	Encapsulation of the memory address range reserved for a stack.
------------------------------------------------------------------------------- */

CHeapDomain::CHeapDomain()
	: fId(kNoId), fBase(0), fLimit(0), fStackRegionCount(0), fStackRegion(NULL)
{ }


CHeapDomain::~CHeapDomain()
{
	if (fStackRegion != NULL)
		delete[] fStackRegion;
}


/* -------------------------------------------------------------------------------
	Initialize.
	Args:		inManager		stack manager to which this domain belongs
				inSectionBase	base address of domain in MB (kSectionSize)
				inSectionCount	size of domain in MB (kSectionSize)
	Return:	error code
------------------------------------------------------------------------------- */

NewtonErr
CHeapDomain::init(CStackManager * inManager, ULong inSectionBase, ULong inSectionCount)
{
	NewtonErr err = noErr;

	XTRY
	{
		// set up domain address/size
		size_t	domainSize = inSectionCount * kSectionSize;
#if !defined(correct)
		fBase = (VAddr)malloc(domainSize + 0x100) & ~0x000000FF;	// align on 256-byte boundary
		fCorrectBase = inSectionBase * kSectionSize;
		fCorrectLimit = fCorrectBase + domainSize;
#else
		fBase = inSectionBase * kSectionSize;
#endif
		fLimit = fBase + domainSize;

		// for each 32K stack page within the domain there is a CStackInfo, initially NULL
		fStackRegionCount = domainSize / kGuardedStackRegionSize;		// !!!! this will be one short for the whole domain !!!!
		XFAILNOT(fStackRegion = new CStackInfo*[fStackRegionCount], err = kOSErrNoMemory;)
		for (ArrayIndex i = 0; i < fStackRegionCount; ++i)
			fStackRegion[i] = NULL;

//printf("CHeapDomain::init() #%08X range #%08X-#%08X, %d stack regions\n", this, fBase, fLimit, fStackRegionCount);
		// add this domain to the stack maanger
		err = inManager->addDomain(fId, fBase, domainSize);
	}
	XENDTRY;

	return err;
}


/* -------------------------------------------------------------------------------
	Return the stack info for an address within the domain.
	Args:		inAddr		the address
				outInfo		the info
	Return:	error code
------------------------------------------------------------------------------- */

NewtonErr
CHeapDomain::getStackInfo(VAddr inAddr, CStackInfo ** outInfo)
{
	if (inAddr < fBase || inAddr > fLimit)
		return kStackErrAddressOutOfRange;

	ArrayIndex	index = (inAddr - fBase) / kGuardedStackRegionSize;
	*outInfo = (index < fStackRegionCount) ? fStackRegion[index] : NULL;
	return noErr;
}

#if !defined(correct)
NewtonErr
CHeapDomain::getCorrectStackInfo(VAddr inAddr, CStackInfo ** outInfo)
{
	if (inAddr < fCorrectBase || inAddr > fCorrectLimit)
		return kStackErrAddressOutOfRange;

	ArrayIndex	index = (inAddr - fCorrectBase) / kGuardedStackRegionSize;
	*outInfo = (index < fStackRegionCount) ? fStackRegion[index] : NULL;
	return noErr;
}
#endif
