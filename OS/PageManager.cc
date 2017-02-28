/*
	File:		PageManager.cc

	Contains:	Virtual memory page management code.

	Written by:	Newton Research Group.
*/

#include "PageManager.h"
#include "UserGlobals.h"
#include "UserMonitor.h"
#include "OSErrors.h"

extern bool					gWantDeferred;

extern CPageManager *	gThePageManager;
extern CUMonitor			gThePageManagerMonitor;

CPageTracker *				gPageTracker;
CExtPageTrackerMgr *		gExtPageTrackerMgr;


extern "C" NewtonErr		AddPTable(VAddr inVAddr, PAddr inPageTable, bool inClear);
extern "C" NewtonErr		RemovePMappings(PAddr inBase, size_t inSize);	// in MMU.c


/*------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
------------------------------------------------------------------------------*/

ArrayIndex
SystemFreePageCount(void)
{
	ArrayIndex	count;

	CUPageManager::freePageCount(&count);
	return count;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C D y n A r r a y
------------------------------------------------------------------------------*/

CDynArray::CDynArray(int inUsed, int inSize)
{
	fAllocSize = inSize;
	fUsedSize = inUsed;
	if (inSize != 0)
		fArray = new void*[inSize];
}


int
CDynArray::resize(int inDelta)
{
	if (inDelta > 0)
	{
		if (fAllocSize == 0)
			fArray = new void*[0];
		else if (fUsedSize + inDelta > fAllocSize)
			return -1;	// can’t resize above allocated size
	}
	else if (fUsedSize < -inDelta)
		return -1;		// can’t resize below what we already have

	fUsedSize += inDelta;
	return fUsedSize;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C R i n g
------------------------------------------------------------------------------*/

int
CRing::push(void * inItem)
{
	int	index;
	if (fItems.resize(1) == -1)
		return -1;
	index = fItems.fAllocSize;
	while (--index > fIndex)
		fItems.fArray[index] = fItems.fArray[index-1];
	fItems.fArray[index] = inItem;
	return 0;
}

void *
CRing::pop(void)
{
	int		index = fIndex;
	void *	item = fItems.fArray[index];
	while (++index < fItems.fUsedSize)
		fItems.fArray[index-1] = fItems.fArray[index];
	fItems.resize(-1);
	return item;
}

void
CRing::rotate(int inDelta)
{
	if (inDelta >= 0)
	{
		fIndex += (inDelta % fItems.fUsedSize);
		if (fIndex >= fItems.fUsedSize)
			fIndex -= fItems.fUsedSize;
	}
	else
	{
		inDelta = -inDelta;
		fIndex -= (inDelta % fItems.fUsedSize);
		if (fIndex < 0)
			fIndex += fItems.fUsedSize;
	}
}

void **
CRing::operator[](int index)
{
	index = fIndex + index;
	if (index >= fItems.fUsedSize)
		index -= fItems.fUsedSize;
	return &fItems.fArray[index];
}

#pragma mark -

/*------------------------------------------------------------------------------
	C P a g e T a b l e M a n a g e r
------------------------------------------------------------------------------*/

extern CPageTableManager *	gThePageTableManager;

NewtonErr
AllocatePageTable(VAddr inVAddr)
{
	NewtonErr	err;
	PAddr			pageTable;
	if ((err = gThePageTableManager->allocatePageTable(inVAddr, pageTable)) == noErr)
		AddPTable(inVAddr, pageTable, true);
	return err;
}



CPageTableManager::CPageTableManager()
	:	fRing(32)
{ }


NewtonErr
CPageTableManager::monitorProc(int inSelector, VAddr inArg2)
{
	if (inSelector == 1)
		return allocatePageTable();

	if (inSelector == 2)
		return releasePageTable(inArg2);

	return -1;
}


struct SubPage
{
	bool		f00;
	PAddr		f04;  // address
	long		f08;
	ObjectId	f0C;  // page table owner
};

NewtonErr
CPageTableManager::allocatePageTable(void)
{
	ObjectId pageTableId;

	if (CUPageManager::get(pageTableId, gCurrentTaskId, 0) != noErr)
		return kOSErrNoPageTable;

	EnterAtomic();
	CPhys *	pageTable = PhysIdToObj(kPhysType, pageTableId);
	if (pageTable != NULL)
	{
		for (ArrayIndex i = 0; i < kSubPagesPerPage; ++i)
		{
			SubPage *	subPage = new SubPage;
			subPage->f00 = true;
			subPage->f04 = pageTable->base() + (kSubPageSize * i);
			subPage->f0C = *pageTable;
			fRing.push(subPage);
		}
	}
	ExitAtomic();
	return noErr;
}


NewtonErr
CPageTableManager::allocatePageTable(VAddr inArg1, PAddr & inArg2)
{
	EnterAtomic();
	if (fRing.fItems.fUsedSize == 0)
		goto fail;
	// INCOMPLETE

	ExitAtomic();
	return noErr;

fail:
	ExitAtomic();
	return kOSErrNoPageTable;
}


NewtonErr
CPageTableManager::releasePageTable(VAddr inArg1)
{
	EnterAtomic();
	// INCOMPLETE
	ExitAtomic();
	return noErr;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C P a g e T r a c k e r
------------------------------------------------------------------------------*/

void
CPageTracker::put(CLittlePhys * inPage)
{
	EnterAtomic();
	fPages.add(inPage);
	fPageCount++;
	ExitAtomic();
}


CLittlePhys *
CPageTracker::take(void)
{
	CLittlePhys *	page = NULL;

	EnterAtomic();
	if (fPages.peek())
	{
		fPageCount--;
		page = (CLittlePhys *)fPages.remove();
	}
	ExitAtomic();

	return page;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C E x t P a g e T r a c k e r
------------------------------------------------------------------------------*/

NewtonErr
CExtPageTracker::init(CExtPageTracker ** ioTracker, ObjectId inPhysId, PAddr inBase, size_t inSize)
{
	NewtonErr err = noErr;
	XTRY
	{
		CPhys * phys; // r10
		XFAILNOT(phys = PhysIdToObj(kPhysType, inPhysId), err = kOSErrBadObjectId;)

		// base address and size must be page aligned
		XFAILIF(PAGEOFFSET(inBase) != 0 || PAGEOFFSET(inSize) != 0, err = kOSErrBadParameters;)

		ArrayIndex  numOfPages = inSize / kPageSize;	// r9
		CExtPageTracker * tracker;  // 44 from func
		XFAILNOT(tracker = (CExtPageTracker *)NewPtr(sizeof(CExtPageTracker) + numOfPages * sizeof(CLittlePhys)), err = kOSErrCouldNotCreateObject;)

		tracker->f00 = true;
		tracker->f01 = false;
		tracker->fPhysId = inPhysId;

		CDoubleQItem	qItem;
		tracker->f08 = qItem;
		tracker->fPages.init(sizeof(CPhys));
		tracker->fPageCount = 0;
		tracker->fFreePageCount = numOfPages;

		PAddr pageAddr = phys->base() + inBase;
		tracker->f24 = pageAddr;
		tracker->f28 = pageAddr + inSize;

		for (ArrayIndex i = 0; i < numOfPages; ++i, pageAddr += kPageSize)
		{
#if defined(correct)
			CLittlePhys littlePhys, * thisPhys = &tracker->f2C[i];
			*thisPhys = littlePhys;
			thisPhys->init(pageAddr, kPageSize);
			tracker->put(thisPhys);
#endif
		}

		*ioTracker = tracker;
	}
	XENDTRY;
	return err;
}


bool
CExtPageTracker::put(CLittlePhys * inPhys)
{
	PAddr page = inPhys->base();

	if (page >= f24 && page < f28)
	{
		EnterAtomic();
		inPhys->anon0038AB1C();
		fPages.add(inPhys);
		fPageCount++;
		ExitAtomic();
		return true;
	}
	return false;
}


CLittlePhys *
CExtPageTracker::take(void)
{
	CLittlePhys *  page = NULL;

	EnterAtomic();
	if (f00)
	{
		if (fPages.peek())
		{
			fPageCount--;
			page = (CLittlePhys *)fPages.remove();
		}
	}
	ExitAtomic();

	return page;
}


bool
CExtPageTracker::removeReferences(ObjectId inId, bool * outDone)
{
	*outDone = false;
	if (inId != fPhysId)
		return false;

	if (inId)
	{
		f00 = false;
		f01 = true;
		if (IsSuperMode())
			*outDone = true;
		else
			doDeferral();
	}
	return true;
}


void
CExtPageTracker::doDeferral(void)
{
	ObjectId id;

	if (f01)
	{
		RemovePMappings(f24, f28 - f24);
#if defined(correct)
		for (ArrayIndex i = 0; i < fFreePageCount; ++i)
			if ((id = f2C[i].fId) != kNoId)
				gTheMemArchObjTbl->remove(id);
#endif
		f01 = false;
	}
}

#pragma mark -

/*------------------------------------------------------------------------------
	C E x t P a g e T r a c k e r M g r
------------------------------------------------------------------------------*/
/* don’t know where this came from…
CExtPageTrackerMgr::CExtPageTrackerMgr()
	: f04(8), f00(false)	// 8 returned from func
{ }
*/

NewtonErr
CExtPageTrackerMgr::makeNewTracker(ObjectId inId1, PAddr inBase, size_t inSize)
{
	NewtonErr  err;
	CExtPageTracker * tracker;
	if ((err = CExtPageTracker::init(&tracker, inId1, inBase, inSize)) == noErr)
		f04.add(tracker);
	return err;
}


bool
CExtPageTrackerMgr::put(CLittlePhys * inPhys)
{
	CExtPageTracker * tracker;
	for (tracker = (CExtPageTracker *)f04.peek(); tracker != NULL; tracker = (CExtPageTracker *)f04.getNext(tracker))
	{
		if (tracker->put(inPhys))
			return true;
	}
	return false;
}


CLittlePhys *
CExtPageTrackerMgr::take(void)
{
	CLittlePhys *  page;
	CExtPageTracker * tracker;
	for (tracker = (CExtPageTracker *)f04.peek(); tracker != NULL; tracker = (CExtPageTracker *)f04.getNext(tracker))
	{
		if ((page = tracker->take()) != NULL)
			return page;
	}
	return NULL;
}


void
CExtPageTrackerMgr::doDeferral(void)
{
	if (f00)
	{
		CExtPageTracker * tracker;
		for (tracker = (CExtPageTracker *)f04.peek(); tracker != NULL; tracker = (CExtPageTracker *)f04.getNext(tracker))
			tracker->doDeferral();
		f00 = false;
	}
}


NewtonErr
CExtPageTrackerMgr::unhookTracker(ObjectId inId)
{
	bool  done;
	CExtPageTracker * tracker;
	for (tracker = (CExtPageTracker *)f04.peek(); tracker != NULL; tracker = (CExtPageTracker *)f04.getNext(tracker))
	{
		if (tracker->removeReferences(inId, &done))
		{
			if (done)
			{
				f00 = true;
				gWantDeferred = true;  // reschedule
				return noErr;
			}
		}
	}
	return kOSErrBadObjectId;
}


NewtonErr
CExtPageTrackerMgr::disposeTracker(ObjectId inId)
{
	bool  done;
	CExtPageTracker * tracker, * next;
	if ((tracker = (CExtPageTracker *)f04.peek()))
	{
		for (next = (CExtPageTracker *)f04.getNext(tracker); tracker != NULL; next = (CExtPageTracker *)f04.getNext(tracker))
		{
			if (tracker->removeReferences(inId, &done))
			{
				f04.removeFromQueue(tracker);
				FreePtr((Ptr)tracker);
				return noErr;
			}
			tracker = next;
		}
	}
	return kOSErrBadObjectId;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C P a g e M a n a g e r
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Constructor.
------------------------------------------------------------------------------*/

CPageManager::CPageManager()
	:	f00(false), fClients(16)
{ }


/*------------------------------------------------------------------------------
	Make a page.
	Args:		outId
				inOwner
				inPage
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CPageManager::make(ObjectId & outId, ObjectId inOwner, CLittlePhys * inPage)
{
	EnterAtomic();
	outId = gTheMemArchObjTbl->add(inPage, kPhysType, inOwner);
	ExitAtomic();
	return noErr;
}


/*------------------------------------------------------------------------------
	Monitor function.  Invoked by CUPageManager.
	Args:		inSelector		selector
				ioMsg				message
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CPageManager::monitorProc(int inSelector, PMMessage * ioMsg)
{
	switch (inSelector)
	{
	case kGetPage:
		return get(ioMsg->result, ioMsg->request.owner, ioMsg->request.index);
		break;
	case kGetExtPage:
		return getExternal(ioMsg->result, ioMsg->request.owner, ioMsg->request.index);
		break;
	case kReleasePage:
		return release(ioMsg->request.owner);
		break;
	case kRegister:
		return registerClient(ioMsg->request.owner);
		break;
	case kGetFreePageCount:
		ioMsg->result = gPageTracker->pageCount();
		break;
	case 5:
		return get(ioMsg->result, ioMsg->request.owner, ioMsg->request.index, ioMsg->request.monitor);
		break;
	case 6:
		return releasePagesForFaultHandling((PMReleasePagesForFaultHandling *)ioMsg);
		break;
	}
	return noErr;
}


/*------------------------------------------------------------------------------
	Get a page in the default monitor.
	Args:		in				selector
				in				message
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CPageManager::get(ObjectId & outId, ObjectId inOwnerId, int inArg3)
{
	CUMonitor	monitor;
	return get(outId, inOwnerId, inArg3, &monitor);
}


/*------------------------------------------------------------------------------
	Get a page in the specified monitor.
	Args:		in				selector
				in				message
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CPageManager::get(ObjectId & outId, ObjectId inOwnerId, int inArg3, CUMonitor * inMonitor)
{
	CObject *	thePage;

	if ((thePage = gPageTracker->take()) == NULL
	 && (thePage = queryClients(inArg3, inMonitor)) == NULL)
	{
		f00 = true;
		return kOSErrNoAvailablePage;
	}

	EnterAtomic();
	outId = gTheMemArchObjTbl->add(thePage, kPhysType, inOwnerId);
	ExitAtomic();
	return noErr;
}

NewtonErr
CPageManager::getExternal(ObjectId & outId, ObjectId inOwnerId, int inArg3)
{
	CObject *	thePage;

	if (gExtPageTrackerMgr == NULL
	 || (thePage = gExtPageTrackerMgr->take()) == NULL)
		return kOSErrNoAvailablePage;

	EnterAtomic();
	outId = gTheMemArchObjTbl->add(thePage, kExtPhysType, inOwnerId);
	ExitAtomic();
	return noErr;
}


/*------------------------------------------------------------------------------
	Monitors call here to register with the page manager.
	Args:		inClientId		the monitor
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CPageManager::registerClient(ObjectId inClientId)
{
	NewtonErr	err = noErr;

	EnterAtomic();
	if (ObjectType(inClientId) != kMonitorType
	 || gObjectTable->get(inClientId) == NULL)
		err = kOSErrBadObjectId;
	ExitAtomic();

	if (err == noErr)
		fClients.push((void*)inClientId);
	return err;
}


NewtonErr
ReleasePage(ObjectId inPageId)
{
	return gThePageManager->release(inPageId);
}


NewtonErr
CPageManager::release(ObjectId inPageId)
{
	NewtonErr	err = noErr;
	CPhys *		page;

	EnterAtomic();
	if (GetPhys(inPageId, page))
	{
		// INCOMPLETE
		;
	}
	else
		err = kOSErrBadObjectId;
	ExitAtomic();

	return err;
}


NewtonErr
CPageManager::releasePagesForFaultHandling(PMReleasePagesForFaultHandling * inArg1)
{
	while (inArg1->f04 < gPageTracker->pageCount())
	{
		if (!askOnePageToAClient(1, inArg1->f08))
			break;
	}
	while (inArg1->f00 < gPageTracker->pageCount())
	{
		if (!askOnePageToAClient(2, inArg1->f08))
			break;
	}
	return (inArg1->f00 < gPageTracker->pageCount()) ? noErr : kOSErrNoMemory;
}


/*------------------------------------------------------------------------------
	Request an existing page.
	Args:		inArg1
				inMonitor		the monitor client - unused
	Return:	the page
------------------------------------------------------------------------------*/

CLittlePhys *
CPageManager::queryClients(int inArg1, CUMonitor * inMonitor)
{
	ObjectId			theClient;
	CLittlePhys *  page;

	for (ArrayIndex i = 0; i < fClients.fItems.fUsedSize; ++i)
	{
		theClient = *(ObjectId*)fClients[0];
		fClients.rotate(1);
		CUPageManagerMonitor::releaseRequest(theClient, inArg1);
		if ((page = gPageTracker->take()) != NULL)
			return page;
	}

	// couldn’t find any pages
	return NULL;
}


/*------------------------------------------------------------------------------
	Request an existing page for a client.
	Args:		inArg1
				inClientId		the monitor
	Return:	true if a page was available
------------------------------------------------------------------------------*/

bool
CPageManager::askOnePageToAClient(int inArg1, ObjectId inClientId)
{
	ObjectId	theClient;

	for (ArrayIndex i = 0; i < fClients.fItems.fUsedSize; ++i)
	{
		theClient = *(ObjectId*)fClients[0];
		fClients.rotate(1);
		if (theClient != inClientId
		 && CUPageManagerMonitor::releaseRequest(theClient, inArg1) == noErr)
			return true;
	}

	// couldn’t find any pages
	return false;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C U P a g e M a n a g e r
------------------------------------------------------------------------------*/

NewtonErr
CUPageManager::get(ObjectId & outId, ObjectId inOwnerId, int index)
{
	NewtonErr	err;
	PMMessage	msg;
	msg.request.owner = inOwnerId;
	msg.request.index = index;
	if ((err = gThePageManagerMonitor.invokeRoutine(kGetPage, &msg)) == noErr)
		outId = msg.result;
	return err;
}


NewtonErr
CUPageManager::getExternal(ObjectId & outId, ObjectId inOwnerId, int index)
{
	NewtonErr	err;
	PMMessage	msg;
	msg.request.owner = inOwnerId;
	msg.request.index = index;
	if ((err = gThePageManagerMonitor.invokeRoutine(kGetExtPage, &msg)) == noErr)
		outId = msg.result;
	return err;
}


NewtonErr
CUPageManager::release(ObjectId inId)
{
	return GenericSWI(kPMRelease, inId, 0, 0);
}


NewtonErr
CUPageManager::registerClient(ObjectId inClientId)
{
	PMMessage	msg;
	msg.request.owner = inClientId;
	return gThePageManagerMonitor.invokeRoutine(kRegister, &msg);
}

NewtonErr
CUPageManager::freePageCount(ULong * outCount)
{
	NewtonErr	err;
	PMMessage	msg;
	if ((err = gThePageManagerMonitor.invokeRoutine(kGetFreePageCount, &msg)) == noErr)
		*outCount = msg.result;
	return err;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C U P a g e M a n a g e r M o n i t o r
------------------------------------------------------------------------------*/

NewtonErr
CUPageManagerMonitor::releaseRequest(ObjectId inMonitorId, int inArg2)
{
	CUMonitor	monitor;
	monitor = inMonitorId;	// could use constructor instead?

	return monitor.invokeRoutine(0x7FFFFFFF, (void*)inArg2);
}
