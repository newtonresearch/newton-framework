/*
	File:		AppWorld.cc

	Contains:	App world implementation.

	Written by:	Newton Research Group.
*/

#include "AppWorld.h"
#include "NewtGlobals.h"
#include "Semaphore.h"
#include "NameServer.h"
#include "ListIterator.h"
#include "OSErrors.h"
//#include "Events.h"


/*--------------------------------------------------------------------------------
	C F o r k W o r l d
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Constructor.
--------------------------------------------------------------------------------*/

CForkWorld::CForkWorld()
{
	fIsFirstInstance = YES;
	fIsTasked = YES;
	fIsReady = NO;
	fIsEnabled = NO;
	fParent = NULL;
	fStackSize = kSpawnedTaskStackSize;
	fName = 0;
	fPriority = kUserTaskPriority;
	fMutex = NULL;
}


/*--------------------------------------------------------------------------------
	Destructor.
--------------------------------------------------------------------------------*/

CForkWorld::~CForkWorld()
{ }


/*--------------------------------------------------------------------------------
	Return the size of this class.
	Args:		--
	Return:	number of bytes
--------------------------------------------------------------------------------*/

size_t
CForkWorld::getSizeOf(void) const
{
	return sizeof(CForkWorld);
}


/*--------------------------------------------------------------------------------
	Spawned task calls here to construct itself.
	Args:		--
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CForkWorld::taskConstructor(void)
{
	NewtonErr err;
	XTRY
	{
		if (fIsFirstInstance)
		{
			// this is the first, main instance of the task
			XFAILNOT(fMutex = new CUMutex, err = MemError();)
			XFAIL(err = fMutex->init())
			fMutex->f0C++;
			fMutex->f10++;
			err = mainConstructor();
		}
		else
		{
			// this is a fork of the task
			fMutex->f0C++;
			fMutex->f10++;
			err = forkConstructor(fParent);
		}
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Spawned task calls here to destroy itself.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CForkWorld::taskDestructor(void)
{
	fMutex->f0C--;
	if (fIsTasked)
	{
		mainDestructor();
		if (fMutex)
			delete fMutex;
	}
	else
		forkDestructor();
}


/*--------------------------------------------------------------------------------
	Spawned task calls here to start running.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CForkWorld::taskMain(void)
{
	if (acquireMutex() == noErr)
	{
		fIsReady = YES;
		if (!fIsFirstInstance
		|| (preMain() == noErr && fIsTasked))
			theMain();
		if (fIsTasked)
			postMain();
	}
	releaseMutex();
}


/*--------------------------------------------------------------------------------
	Initialize the forked world from its parent.
	Args:		inWorld		the parent
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CForkWorld::forkInit(CForkWorld * inWorld)
{
	fIsFirstInstance = NO;
	fParent = inWorld;
	fMutex = inWorld->fMutex;
	fStackSize = inWorld->fStackSize;
	fPriority = inWorld->fPriority;
	fName = inWorld->fName;
	fIsEnabled = inWorld->fIsEnabled;
	return noErr;
}


/*--------------------------------------------------------------------------------
	Construct the forked world.
	ThereÕs nothing to do here.
	Args:		inWorld		the parent
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CForkWorld::forkConstructor(CForkWorld *)
{ return noErr; }


/*--------------------------------------------------------------------------------
	Destroy the forked world.
	ThereÕs nothing to do here.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CForkWorld::forkDestructor(void)
{ }


/*--------------------------------------------------------------------------------
	Initialize the main thing.
	Args:		inName			name of the task
				inStackSize		task stack size
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CForkWorld::mainInit(ULong inName, size_t inStackSize)
{
	fName = inName;
	fStackSize = inStackSize;
	return startTask(YES, NO, kNoTimeout, fStackSize, fPriority, fName);
}


/*--------------------------------------------------------------------------------
	Initialize the main thing.
	Args:		inName			name of the task
				inStackSize		task stack size
				inPriority		task priority
				inEnvironment	task environment
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CForkWorld::mainInit(ULong inName, size_t inStackSize, ULong inPriority, ObjectId inEnvironment)
{
	fName = inName;
	fStackSize = inStackSize;
	fPriority = inPriority;
	return startTask(YES, NO, kNoTimeout, fStackSize, fPriority, fName, inEnvironment);
}


/*--------------------------------------------------------------------------------
	Construct the main thing...
	Pass it up to our superclass - which does nothing.
	Args:		--
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CForkWorld::mainConstructor(void)
{
	return CUTaskWorld::taskConstructor();
}


/*--------------------------------------------------------------------------------
	Destroy the main thing...
	Pass it up to our superclass - which does nothing.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CForkWorld::mainDestructor(void)
{
	CUTaskWorld::taskDestructor();
}


/*--------------------------------------------------------------------------------
	A chance to do something before the main.
	ThereÕs nothing to do here.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

NewtonErr
CForkWorld::preMain(void)
{ return noErr; }


/*--------------------------------------------------------------------------------
	A chance to do something after the main.
	ThereÕs nothing to do here.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CForkWorld::postMain(void)
{ }


/*--------------------------------------------------------------------------------
	Switch to the spawned fork.
	ThereÕs nothing to do here.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CForkWorld::forkSwitch(bool inDoIt)
{ }


/*--------------------------------------------------------------------------------
	Make a new fork.
	ThereÕs nothing to do here.
	Args:		--
	Return:	NULL
--------------------------------------------------------------------------------*/

CForkWorld *
CForkWorld::makeFork(void)
{ return NULL; }


/*--------------------------------------------------------------------------------
	Erm... enable forking.
	Args:		inEnable		yes, no
	Return:	previous enabled state
--------------------------------------------------------------------------------*/

bool
CForkWorld::enableForking(bool inEnable)
{
	bool wasEnabled = fIsEnabled;
	fIsEnabled = inEnable;
	return wasEnabled;
}


/*--------------------------------------------------------------------------------
	Fork a new task.
	Args:		inWorld
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CForkWorld::fork(CForkWorld * inWorld)
{
	NewtonErr err = noErr;
	XTRY
	{
		if (fIsEnabled && fIsReady)
		{
			XFAIL(inWorld == NULL && fIsTasked == NO)	// not really an error

			CForkWorld * newFork = (inWorld != NULL) ? inWorld : makeFork();
			XFAILNOT(newFork, err = kOSErrCouldNotCreateObject;)
			XFAIL(err = newFork->forkInit(this))
			XFAIL(err = newFork->startTask(YES, NO, kNoTimeout, fStackSize, fPriority, fName))
			SetBequeathId(newFork->fChildTask);
			fIsTasked = NO;
			delete newFork;
		}
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Yield the processor to other tasks.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CForkWorld::yield(void)
{
	if (releaseMutex() == noErr)
		acquireMutex();
}


/*--------------------------------------------------------------------------------
	Acquire the mutex semaphore to allow the fork to continue.
	Args:		--
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CForkWorld::acquireMutex(void)
{
	long result = fMutex->acquire(kWaitOnBlock);
	forkSwitch(YES);
	return result;
}


/*--------------------------------------------------------------------------------
	Release the mutex semaphore after the fork is done.
	Args:		--
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CForkWorld::releaseMutex(void)
{
	forkSwitch(NO);
	long result = fMutex->release();
	if (result == kOSErrSemaphoreWouldCauseBlock)
		result = noErr;
	return result;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C A p p W o r l d S t a t e
--------------------------------------------------------------------------------*/

CAppWorldState::CAppWorldState()
{
	fErr = noErr;
	fFilter = 0xFFFFFFFF;
	fIsTokenOnly = NO;
	fOnMsgAvail = NO;
	fEventSize = 0;
	fMsgSize = 256;
	fMsgToken = NULL;
	fEvent = (CEvent *)fEventBuf;
	fDone = NO;
}

CAppWorldState::~CAppWorldState()
{
	if (fPort)
		delete fPort;
}

NewtonErr
CAppWorldState::init(void)
{
	fPort = new CUPort();
	return (fPort == NULL) ? MemError() : fPort->init();
}

NewtonErr
CAppWorldState::init(ULong inName)
{
	fPort = new CUPort(inName);
	return (fPort == NULL) ? MemError() : noErr;
}

NewtonErr
CAppWorldState::init(CUPort * inPort)
{
	fPort = inPort;
	return noErr;
}

void
CAppWorldState::nestedEventLoop(void)
{
	newton_try
	{
		gAppWorld->eventLoop(this);
	}
	newton_catch_all
	{
		fDone = YES;
	}
	end_try;
}

void
CAppWorldState::terminateNestedEventLoop(void)
{
	fDone = YES;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C A p p W o r l d
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Constructor.
--------------------------------------------------------------------------------*/

CAppWorld::CAppWorld()
{
	fPortName = 0;
	fWorldState = NULL;
	fAppState = NULL;
	fHandlers = NULL;
	fTimers = NULL;
	fEventId = kTypeWildCard;
	fEventClass = kNewtEventClass;
}


/*--------------------------------------------------------------------------------
	Destructor.
--------------------------------------------------------------------------------*/

CAppWorld::~CAppWorld()
{ }


/*--------------------------------------------------------------------------------
	Return the size of this class.
	Args:		--
	Return:	number of bytes
--------------------------------------------------------------------------------*/

size_t
CAppWorld::getSizeOf(void) const
{
	return sizeof(CAppWorld);
}


/*--------------------------------------------------------------------------------
	Initialize the forked world from its parent.
	Args:		inWorld		the parent
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CAppWorld::forkInit(CForkWorld * inWorld)
{
	NewtonErr	err;

	if ((err = CForkWorld::forkInit(inWorld)) == noErr)
	{
		CAppWorld * parentWorld = static_cast<CAppWorld*>(inWorld);
		fPortName = parentWorld->fPortName;
		fWorldState = NULL;
		fAppState = NULL;
		fItemCmp = parentWorld->fItemCmp;
		fEventCmp = parentWorld->fEventCmp;
		fHandlerCmp = parentWorld->fHandlerCmp;
		fHandlers = parentWorld->fHandlers;
		fTimers = parentWorld->fTimers;
		fEventId = parentWorld->fEventId;
		fEventClass = parentWorld->fEventClass;
	}

	return err;
}


/*--------------------------------------------------------------------------------
	Construct the forked world.
	Args:		inWorld		the parent
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CAppWorld::forkConstructor(CForkWorld * inWorld)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = CForkWorld::forkConstructor(inWorld))

		XFAILNOT(fWorldState = new CAppWorldState, err = MemError();)
		fWorldState->fEvent = (CEvent *)fWorldState->fEventBuf;
		fWorldState->fMsgToken = &fWorldState->fMsgTokenBuf;
		fAppState = fWorldState;

		ASSERTMSG(inWorld != NULL, "forking a NULL world!");
		XFAIL(err = fWorldState->init((static_cast<CAppWorld*>(inWorld))->fWorldState->fPort))
		eventTerminateLoop();
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Destroy the forked world.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CAppWorld::forkDestructor(void)
{
	if (fWorldState)
	{
		fWorldState->fPort = NULL;
		delete fWorldState;
	}
	CForkWorld::forkDestructor();
}


/*--------------------------------------------------------------------------------
	Construct the main thing.
	Args:		--
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CAppWorld::mainConstructor(void)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = CForkWorld::mainConstructor())

		XFAILNOT(fWorldState = new CAppWorldState, err = MemError();)
		fWorldState->fEvent = (CEvent *)fWorldState->fEventBuf;
		fWorldState->fMsgToken = &fWorldState->fMsgTokenBuf;
		fAppState = fWorldState;
		XFAIL(err = fWorldState->init())
		if (fPortName)
		{
			CUNameServer	ns;
			MAKE_ID_STR(fPortName,portName);
			XFAIL(err = ns.registerName(portName, (char *)kUPort, (OpaqueRef)getMyPort(), 0))
		}
		XFAILNOT(fHandlers = new CSortedList(&fEventCmp), err = MemError();)
		XFAILNOT(fTimers = new CTimerQueue(), err = MemError();)
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Destroy the main thing.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CAppWorld::mainDestructor(void)
{
	CForkWorld::mainDestructor();

	if (fHandlers)
	{
		CListIterator		iter(fHandlers);
		CEventHandler *	handler;
		for (handler = (CEventHandler *)iter.firstItem(); iter.more(); handler = (CEventHandler *)iter.nextItem())
			delete handler;
	}
	if (fPortName)
	{
		CUNameServer	ns;
		MAKE_ID_STR(fPortName, portName);
		ns.unregisterName(portName, (char *)kUPort);
	}
	if (fWorldState)
		delete fWorldState;
	if (fHandlers)
		delete fHandlers;
	if (fTimers)
		delete fTimers;
}


/*--------------------------------------------------------------------------------
	Okay. This is it. Finally.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CAppWorld::theMain(void)
{
	eventLoop(fAppState);
}


/*--------------------------------------------------------------------------------
	Initialize.
	Args:		inName			name of the task
				inDoName			??
				inStackSize		task stack size
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CAppWorld::init(ULong inName, bool inDoName, size_t inStackSize)
{
	if (inDoName)
		fPortName = inName;
	return CForkWorld::mainInit(inName, inStackSize);
}


/*--------------------------------------------------------------------------------
	Initialize.
	Args:		inName			name of the task
				inDoName			??
				inStackSize		task stack size
				inPriority		task priority
				inEnvironment	task environment
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CAppWorld::init(ULong inName, bool inDoName, size_t inStackSize, ULong inPriority, ObjectId inEnvironment)
{
	if (inDoName)
		fPortName = inName;
	return CForkWorld::mainInit(inName, inStackSize, inPriority, inEnvironment);
}


void
CAppWorld::interruptHandler(ULong *, CEvent * inEvent)
{ /*this really does nothing*/ }


NewtonErr
CAppWorld::eventDispatch(ULong inFlags, CUMsgToken * inMsgToken, size_t * outSize, CEvent * inEvent)
{
	NewtonErr			err = noErr;
	CEventHandler *	handler;

	if ((inFlags & 0x03000000) != 0
	&&  inMsgToken != NULL)
	{
		handler = NULL;
		err = eventGetCollectedEvent(inFlags, inMsgToken, outSize, &inEvent, (OpaqueRef*)&handler);
		if (err == noErr && handler != NULL)
		{
			handler->eventCompletionProc(inMsgToken, outSize, inEvent);
			eventDeferReply();
			return err;
		}
	}

	XTRY
	{
		XFAILNOT(inEvent, err = -14100;)
		if (inEvent->fEventId == fEventId)
		{
			CListIterator		iter(fHandlers);
			for (handler = (CEventHandler *)iter.firstItem(); iter.more(); handler = (CEventHandler *)iter.nextItem())
				for ( ; handler != NULL; handler = handler->getNextHandler())
					handler->eventHandlerProc(inMsgToken, outSize, inEvent);
		}
		else
		{
			ArrayIndex  index;
			fEventCmp.setTestItem(inEvent);
			XFAILNOT(handler = (CEventHandler *)fHandlers->search(&fEventCmp, index), err = -14100;)
			if (inFlags & 0x02000000)
			{
				handler->doComplete(inMsgToken, outSize, inEvent);
				eventDeferReply();
			}
			else
				err = handler->doEvent(inMsgToken, outSize, inEvent);
		}
	}
	XENDTRY;
	return err;
}


CEventHandler *
CAppWorld::eventFindHandler(EventClass inClass, EventId inId)
{
	ArrayIndex  index;
	CEvent		event(inClass, inId);
	fEventCmp.setTestItem(&event);
	return (CEventHandler *)fHandlers->search(&fEventCmp, index);
}


CEvent *
CAppWorld::eventGetEvent(void)
{
	return fAppState->fEvent;
}


NewtonErr
CAppWorld::eventGetCollectedEvent(ULong inFlags, CUMsgToken * inMsgToken, size_t * outSize, CEvent ** outEvent, OpaqueRef * outRefCon)
{
	NewtonErr err = noErr;
	XTRY
	{
		if (inFlags & 0x02000000)
		{
			ObjectId replyId = inMsgToken->getReplyId();
			if (replyId != 0)
			{
				CUSharedMem mem(replyId);
				XFAIL(err = mem.getSize(outSize, (void**)outEvent))
				err = inMsgToken->getUserRefCon(outRefCon);
			}
		}
		else if (inFlags & 0x01000000)
		{
			err = inMsgToken->cashMessageToken(outSize, *outEvent, 256);
		}
	}
	XENDTRY;
	return err;
}


size_t
CAppWorld::eventGetMsgSize(void)
{
	return fAppState->fMsgSize;
}


CUMsgToken *
CAppWorld::eventGetMsgToken(void)
{
	return fAppState->fMsgToken;
}


long
CAppWorld::eventGetMsgType(void)
{
	return fAppState->fMsgType;
}


NewtonErr
CAppWorld::eventInstallHandler(CEventHandler * inHandler)
{
	ArrayIndex			index;
	CEventHandler *	handler;
	fHandlerCmp.setTestItem(inHandler);
	if ((handler = (CEventHandler *)fHandlers->search(&fHandlerCmp, index)) != NULL)
	{
		inHandler->addHandler(handler);
		fHandlers->replaceAt(index, inHandler);
	}
	else
		fHandlers->insertAt(index, inHandler);
	return noErr;
}


NewtonErr
CAppWorld::eventRemoveHandler(CEventHandler * inHandler)
{
	ArrayIndex			index;
	CEventHandler *	handler;
	fHandlerCmp.setTestItem(inHandler);
	if ((handler = (CEventHandler *)fHandlers->search(&fHandlerCmp, index)) != NULL)
	{
		CEventHandler *	handler2;
		if ((handler2 = inHandler->removeHandler(handler)) == NULL)
			fHandlers->removeElementsAt(index, 1);
		else if (handler2 != handler)
			fHandlers->replaceAt(index, handler2);
	}
	return noErr;
}


void
CAppWorld::eventInstallIdleHandler(CEventHandler * inHandler)
{ /*this really does nothing*/ }

void
CAppWorld::eventRemoveIdleHandler(CEventHandler * inHandler)
{ /*this really does nothing*/ }


void
CAppWorld::eventSetReply(size_t inSize)
{
	fAppState->fEventSize = inSize;
}


void
CAppWorld::eventSetReply(size_t inSize, CEvent * inEvent)
{
	fAppState->fEvent = inEvent;
	fAppState->fEventSize = inSize;
}


void
CAppWorld::eventSetReply(CUMsgToken * inMsgToken)
{
	fAppState->fMsgToken = inMsgToken;
}


void
CAppWorld::eventSetReply(CUMsgToken * inMsgToken, size_t inSize, CEvent * inEvent)
{
	fAppState->fMsgToken = inMsgToken;
	fAppState->fEvent = inEvent;
	fAppState->fEventSize = inSize;
}


NewtonErr
CAppWorld::eventReplyImmed(void)
{
	NewtonErr		err = noErr;
	CUMsgToken *	msgToken = fAppState->fMsgToken;
	if (msgToken != NULL)
	{
		err = msgToken->replyRPC(fAppState->fEvent, fAppState->fEventSize);
		eventDeferReply();
	}
	return err;
}


void
CAppWorld::eventDeferReply(void)
{
	fAppState->fMsgToken = NULL;
}


void
CAppWorld::eventLoop(void)
{
	eventLoop(fWorldState);
}


void
CAppWorld::eventLoop(CAppWorldState * inState)
{
	CAppWorldState *  saveState = fAppState;
	fAppState = inState;

	do
	{
		fAppState->fEvent = (CEvent *)fAppState->fEventBuf;
		fAppState->fEventSize = 0;
		fAppState->fMsgToken = &fAppState->fMsgTokenBuf;
		fAppState->fMsgType = 0;
		Timeout tmOut = fTimers->check();
		if (!fIsTasked)
			break;

		releaseMutex();
		NewtonErr err = fAppState->waitForEvent(tmOut);
printf("CAppWorld::eventLoop() : %c%c%c%c waitForEvent(%d) -> %d\n", fName>>24,fName>>16,fName>>8,fName,tmOut,err);
		acquireMutex();

		if (err != kOSErrMessageTimedOut)
		{
			fAppState->fErr = err;
			if (err == noErr || err == kOSErrSizeTooLargeCopyTruncated)
			{
				err = eventDispatch(fAppState->fMsgType, fAppState->fMsgToken, &fAppState->fMsgSize, fAppState->fEvent);
				CUMsgToken * msgToken = fAppState->fMsgToken;
				if (msgToken != NULL
				&&  msgToken->getReplyId() != 0)
					fAppState->fMsgTokenBuf.replyRPC(fAppState->fEvent, fAppState->fEventSize, err);
			}
		}
	} while (!fAppState->fDone);

	fAppState->fDone = NO;
	fAppState = saveState;
}


void
CAppWorld::eventLoop(CAppWorldState * inState, CUMsgToken * inMsg)
{ /*this really does nothing*/ }


void
CAppWorld::eventTerminateLoop(void)
{
	fAppState->fDone = YES;
}

#pragma mark -

NewtonErr
SendRPC(CEventHandler * inHandler, CUPort * inPort, CUAsyncMessage * ioMessage, void * inRequest, size_t inReqSize, void * inReply, size_t inReplySize, Timeout inTimeout, CTime * inTimeToSend, ULong inMsgType, bool inUrgent)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = ioMessage->setUserRefCon((OpaqueRef)inHandler))
		err = inPort->sendRPC(ioMessage, inRequest, inReqSize, inReply, inReplySize, inTimeout, inTimeToSend, inMsgType, inUrgent);
	}
	XENDTRY;
	return err;
}

