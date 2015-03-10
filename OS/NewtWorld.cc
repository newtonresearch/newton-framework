/*
	File:		NewtWorld.cc

	Contains:	Newt world implementation.

	Written by:	Newton Research Group.
*/

#include "RootView.h"

// OS
#include "Objects.h"
#include "Globals.h"
#include "MemObjManager.h"
#include "KernelGlobals.h"
#include "VirtualMemory.h"
#include "EventHandler.h"
#include "SystemEvents.h"
#include "NewtWorld.h"
#include "ScreenDriver.h"
#include "TabletDriver.h"
#include "OSErrors.h"
#include "Preference.h"
#include "REPTranslators.h"
#include "PartHandler.h"
#include "Platform.h"
// Frames
#include "ObjectHeap.h"
#include "Frames.h"
#include "NewtonScript.h"
#include "Funcs.h"
#include "REP.h"
#include "Strings.h"
#include "ROMSymbols.h"
#include "StoreWrapper.h"
#include "Soups.h"
// Views
#include "Notebook.h"
#include "Recognition.h"
#include "Inker.h"
#include "QDDrawing.h"


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern NewtonErr  InitForkGlobalsForFrames(NewtGlobals * inGlobals);
extern void			SwitchFramesForkGlobals(bool inDoFork);
extern void			DestroyForkGlobalsForFrames(NewtGlobals * inGlobals);
extern NewtonErr  InitForkGlobalsForQD(NewtGlobals * inGlobals);
extern void			DestroyForkGlobalsForQD(NewtGlobals * inGlobals);

extern void			RegisterVoyagerMiscIntf(void);
extern void			InitLicenseeDomain(void);
extern void			LoadStartupDriver(void);
extern void			InitEvents(void);
extern NewtonErr	InitAlertManager(void);			// in Alerts.cc
extern void			InitializeSound(void);			// in SoundServer.cc
extern NewtonErr	InitializeCommManager(void);	// in CommManager.cc
extern NewtonErr	InitCardServices(void);			// in CardServer.cc
extern NewtonErr	InitTestAgent(void);
extern void			ZapInternalStoreCheck(void);
extern NewtonErr	InitPowerManager(void);			// in PowerManager.cc
extern NewtonErr	InitPSSManager(ObjectId inEnvId, ObjectId inDomId);	// in PSSManagaer.cc

extern void			InitializeCompression(void);
extern void			InitObjects(void);
extern void			InitTranslators(void);
extern void			NTKInit(void);
extern Ref			InitUnicode(void);
extern void			InitExternal(void);
extern void			InitGraf(void);
extern void			InitFonts(void);
extern void			HandleCardEvents(void);
extern void			HandleTestAgentEvent(void);
extern void			AllocateEarlyStuff(void);
extern void			ReleaseScreenLock(void);

extern Ref			GetStores(void);

void		UserMain(void);


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

extern ULong		gNewtTests;
extern ULong		gMonitorTaskPriority;
extern ULong		gTMuxTaskPriority;

CTime					gLastWakeupTime;

CUPort *				gNewtPort;
CApplication *		gApplication;
NewtGlobals *		gNewtGlobals;
ULong					gNewtAlarmName;
bool					gNewtIsAliveAndWell = NO;
bool					gGoingToSleep = NO;

CTime					gTickleTime;	// 0C100D04


/*------------------------------------------------------------------------------
	I n i t i a l i z a t i o n
------------------------------------------------------------------------------*/

Ref
FMinimumBatteryCheck(RefArg inRcvr)
{
	bool  isDead = NO;
/*
	for ( ; ; )
	{
		PowerPlantStatus  pwr;  // size +34
		if (GetBatteryStatus(0, &pwr, NO) == 0)
		{
			if (pwr.f18 == 1 || pwr.f08 > pwr.f10)
				break;
			isDead = YES;
			SleepUntilNextWakeup();
		}
	}
*/
	return MAKEBOOLEAN(isDead);
}


NewtonErr
GetTaskStackInfo(const CUTask * inTask, VAddr * outStackTop, VAddr * outStackBase)
{
	return GenericWithReturnSWI(75, *inTask, 0, 0, outStackTop, outStackBase, NULL);
}


void
SetActionDescription(long inDescr)
{
	SetFrameSlot(RA(gVarFrame), SYMA(actionDescription), MAKEINT(inDescr));
}


void
LoadHighROMPackages(bool inFrames)	// frames vs drivers
{
#if 0
	long  sp = &sp-6C;
	r8 = gPackageSemaphore;	// 0C1016DC
	for (ArrayIndex i = 0; i < 4; ++i)	// r5
	{
		VAddr r10 = GetPackageList(i);
		if (r10 != NULL)
		{
			char  sp00 = 1;
			char  sp01 = 0;
			char  sp02 = 0;
			char  sp03 = 0;
			r6 = 0;
			newton_try
			{
				r9 = r4 = r10 + r6;
				if (inFrames)
					gAppWorld->fork(NULL);
				CPkBeginLoadEvent loader(, gAppWorld->getMyPort(), gAppWorld->getMyPort(), YES);
				;
				CUPort	sp00(PackageManagerPortId());
				if (inFrames)
					gAppWorld->releaseMutex();
				;
			}
			end_try;
		}
	}

#else
	if (!inFrames)
	{
		CMainDisplayDriver::classInfo()->registerProtocol();
	}
#endif
}

void
LoadHighROMDriverPackages(void)
{
	LoadHighROMPackages(NO);
}

void
LoadHighROMFramesPackages(void)
{
	LoadHighROMPackages(YES);
}

#pragma mark -

/*--------------------------------------------------------------------------------
	Delayed actions.
--------------------------------------------------------------------------------*/

void
RunDelayedActionProcs(void)
{
	ArrayIndex  actionCount;
	for (actionCount = 10; actionCount > 0; actionCount--)
	{
		if (gNewtIsAliveAndWell
		&&  gApplication->runNextDelayedAction())
		{
			gRootView->update();
			gApplication->idle();
		}
		else
			break;
	}
	if (actionCount == 0)	// we did all 10
		gNewtWorld->handleEvents(1);
	else if (actionCount < 10)
		gNewtWorld->handleEvents(0);
// else we didnÕt do any
}


void
CheckForDeferredActions(void)
{
	gNewtWorld->handleEvents(1);
}


#pragma mark -
/* -----------------------------------------------------------------------------
	Script execution.
--------------------------------------------------------------------------------
	Public function to execute a script in the newt task.
	Args:		inRcvrName
				inMsgName
				inData
				inDataSize
				outResult
	Return:	error code
----------------------------------------------------------------------------- */
class CRunScriptEvent : public CEvent
{
public:
	EventType	fEventType;
	char			fRcvrName[64];
	char			fMsgName[64];
	char *		fData;
	size_t		fDataSize;
	NewtonErr	fError;
	NewtonErr	fResult;
};

NewtonErr
SendRunScriptEvent(char * inRcvrName, char * inMsgName, char * inData, size_t inDataSize, NewtonErr * outResult)
{
	NewtonErr err;
	size_t replySize;
	CRunScriptEvent request;
	CRunScriptEvent reply;
	request.fEventClass = kNewtEventClass;
	request.fEventId = kIdleEventId;
	request.fEventType = 'scpt';
	strncpy(request.fRcvrName, inRcvrName, 64);
	strncpy(request.fMsgName, inMsgName, 64);
	request.fData = inData;
	request.fDataSize = inDataSize;

	err = gNewtPort->sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply));

	if (err == noErr)
	{
		if (reply.fError == noErr)
			*outResult = reply.fResult;
		else
			err = reply.fError;
	}
	return err;
}


NewtonErr
HandleRunScriptEvent(CRunScriptEvent * ioEvent)
{
	ioEvent->fError = noErr;
	newton_try
	{
		RefVar rcvr(gRootView->getVar(MakeSymbol(ioEvent->fRcvrName)));
		if (NOTNIL(rcvr))
		{
			RefVar msg(MakeSymbol(ioEvent->fMsgName));
			RefVar data;
			if (ioEvent->fData)
			{
				data = AllocateBinary(SYMA(data), ioEvent->fDataSize);
				memmove(BinaryData(data), ioEvent->fData, ioEvent->fDataSize);
			}
			RefVar args(MakeArray(1));
			SetArraySlot(args, 0, data);
			ioEvent->fResult = RINT(DoMessage(rcvr, msg, args));
		}
		else
		{
			ioEvent->fError = kNSErrPathFailed;
		}
	}
	newton_catch(exMessage)
	{
		ioEvent->fError = -1;
	}
	newton_catch("type.ref.frame")
	{
		ioEvent->fError = GetFrameSlot(*(RefStruct *)CurrentException()->data, SYMA(errorCode));
	}
	newton_catch_all
	{
		ioEvent->fError = (NewtonErr)(unsigned long)CurrentException()->data;
	}
	end_try;
}


#pragma mark -

/*--------------------------------------------------------------------------------
	C L o a d e r

	The class that loads the Newton world.
--------------------------------------------------------------------------------*/

size_t
CLoader::getSizeOf(void) const
{
	return sizeof(CLoader);
}


NewtonErr
CLoader::mainConstructor(void)
{
	return CAppWorld::mainConstructor();
}


void
CLoader::mainDestructor(void)
{
	CAppWorld::mainDestructor();
}


void
CLoader::theMain(void)
{
	RegisterVoyagerMiscIntf();
	LoadHighROMDriverPackages();
	InitLicenseeDomain();
	LoadStartupDriver();
	LoadPlatformDriver();
#if defined(correct)
	gGPIInterruptAsyncMessage.init(NO);
	gGPIInterruptMessage.fEventClass = kNewtEventClass;	// make this an init() method?
	gGPIInterruptMessage.fEventId = kIdleEventId;
	gGPIInterruptMessage.f08 = 'ext ';
#endif
	InitEvents();								// registers CHistoryCollector protocol
	InitAlertManager();						//	creates CAlertManager
	InitializeSound();						//	starts SoundServer, registers codecs
	InitializeCommManager();				//	creates CCMWorld, registers ROM protocols
	InitCardServices();						// initializes CCardDomains, CCardServer
	if ((gNewtTests & 0x0800) != 0)
		InitTestAgent();						// does a nameserver lookup of CUPort 'tagt'
	ZapInternalStoreCheck();				//	asks user (if necessary) whether persistent data should be erased

	ObjectId	userEnvId;
	ObjectId	ramsEnvId;
	ObjectId	ramsDomId;
	MemObjManager::findEnvironmentId('user', &userEnvId);
	MemObjManager::findEnvironmentId('rams', &ramsEnvId);
	MemObjManager::findDomainId('rams', &ramsDomId);

	InitPowerManager();

	CUEnvironment	environment(userEnvId);
	environment.add(ramsDomId, YES/*isManager*/, NO/*hasntStack*/, NO/*hasntHeap*/);
	ULong	savePriority = gMonitorTaskPriority;
//	gMonitorTaskPriority = gTmuxTaskPriority;
	InitPSSManager(ramsEnvId, ramsDomId);
	gMonitorTaskPriority = savePriority;
	environment.remove(ramsDomId);
	environment.add(ramsDomId, NO/*isntManager*/, NO/*hasntStack*/, NO/*hasntHeap*/);

	XTRY
	{
		CUTask	newt;
		XFAIL(newt.init((TaskProcPtr) UserMain, 26*KByte, 0, NULL, kUserTaskPriority, 'main'))
		newt.start();
	}
	XENDTRY;

	SetBequeathId(*gIdleTask);
}


void
UserMain(void)
{
	CNewtWorld theWorld;
	theWorld.init('newt', YES, 10*KByte);
}


#pragma mark -
/*--------------------------------------------------------------------------------
	C N e w t E v e n t H a n d l e r
--------------------------------------------------------------------------------*/

CNewtEventHandler::CNewtEventHandler()
{ }


/* -----------------------------------------------------------------------------
	Wake up the Newton application layer at the specified delta.
	The app layer will wake anyway at the next idle time or if there are any
	delayed actions.
	ÒWakeÓ in this context means generate an event.
	Args:		inDelta
	Return:	--
----------------------------------------------------------------------------- */

void
CNewtEventHandler::setWakeupTime(Timeout inDelta)
{
	// get absolute time app next wants to be run when idle
	CTime wuTime(gApplication->getIdleTime());

	if (inDelta != kNoTimeout)
	{
		CTime nextTime(TimeFromNow(inDelta));
		if (nextTime < wuTime)
			// we want the app to be woken sooner than it thinks
			wuTime = nextTime;
	}

	// find the next time at which a delayed action should be invoked
	wuTime = gApplication->nextDelayedActionTime(wuTime);

	if (wuTime != CTime(0))
	{
		long wait;
		CTime delta(wuTime - GetGlobalTime());
		if (delta > CTime(0))
			wait = delta.convertTo(kMilliseconds);
		else
			wait = 1*kMilliseconds;
		resetIdle(wait, kMilliseconds);
	}
	else
		stopIdle();
}


/* -----------------------------------------------------------------------------
	Handle a Newton application layer event.
	Args:		inToken
				inSize
				inEvent
	Return:	--
----------------------------------------------------------------------------- */

void
CNewtEventHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	switch (inEvent->fEventId)
	{
	case 'alrm':
//		HandleAlarmEvent(static_cast<CAlarmEvent*>(inEvent));
		break;

	case 'bats':
		NSCallGlobalFn(SYMA(BadBatteryAlert));
		break;
	case 'dead':
		NSCallGlobalFn(SYMA(BadAdapterAlert));
		break;
	case 'pwch':
		NSCallGlobalFn(SYMA(CallPowerStatusChangeFns));
		break;

	case 'bklt':
	case 'ext ':
		gTickleTime = GetGlobalTime();
		break;

	case 'card':
//		HandleNewCard(static_cast<CNewCardEvent*>(inEvent));
		break;

	case 'draw':
//		HandleRedrawEvent(static_cast<CRedrawScreenEvent*>(inEvent));
		break;

	case 'ic  ':
//		HandleInterConnect(static_cast<CInterConnectEvent*>(inEvent));
		break;

	case kIdleEventId:
		// do nothing!
		break;

	case 'irMC':
		gRootView->runScript(SYMA(IRConnectRequest), RA(NILREF), YES, NULL);
		break;

	case 'keyb':
/*		{
			CKeyboardEvent	keybEvent;	// public CEvent
			keybEvent = *(CKeyboardEvent*)inEvent;
			inEvent->f0C = 36;
			RefVar	keyVar(GetPreference(SYMA(keyRepeatFrequency)));
			inEvent->f10 = ISINT(keyVar) ? RINT(keyVar) : 100;
			keyVar = GetPreference(SYMA(keyRepeatThreshold));
			inEvent->f14 = ISINT(keyVar) ? RINT(keyVar) : 600;
			keyVar = GetPreference(SYMA(cmdKeyRepeatThreshold));
			inEvent->f1C = ISINT(keyVar) ? RINT(keyVar) : 2500;
			inEvent->f18 = 16;
			setReply(inSize, inEvent);
			if (inToken != NULL && inToken->getReplyId() != kNoId)
				replyImmed();
			HandleKeyEvent(keybEvent);
		}
*/		break;

	case 'rstr':
//		StorageCardRemoved(static_cast<CNewStoreEvent*>(inEvent));
		break;

	case 'scp!':
//		HandleSCPEvent(static_cast<CSCPEvent*>(inEvent));
		break;

	case 'scpt':
		HandleRunScriptEvent(static_cast<CRunScriptEvent*>(inEvent));
		break;

	case 'xnwt':
//		HandleExternalNewtEvent(static_cast<CExternalNewtEvent*>(inEvent));
		break;
	}

	if (inEvent->fEventId != 'keyb')
	{
		if (inEvent->fEventId != kIdleEventId)
			setReply(*inSize, inEvent);
		if (inToken != NULL && inToken->getReplyId() != kNoId)
			replyImmed();
		// these events must be actioned AFTER sending the reply
		if (inEvent->fEventId == 'powr')
		{
			if (!gGoingToSleep)
			{
				CTime now = GetGlobalTime();
				if (now - gLastWakeupTime > CTime(1, kSeconds))
				{
					// this is a power event more than one second after waking up, so it must be power-off
					gGoingToSleep = YES;
					gRootView->runScript(SYMA(GoToSleep), RA(NILREF), YES, NULL);
				}
			}
		}
		else if (inEvent->fEventId == 'stor')
		/*	StorageCardInserted(static_cast<CNewStoreEvent*>(inEvent)) */;
	}

	// run the app layer
	gApplication->run();
	// come back here as soon as idle
	setWakeupTime(0);
}


void
CNewtEventHandler::eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{ /* this really does nothing */ }


/*--------------------------------------------------------------------------------
	The system is idle.
	Remove the busy box (if shown) and run any delayed actions.
	Args:		inToken
				inSize
				inEvent
	Return:	--
--------------------------------------------------------------------------------*/

void
CNewtEventHandler::idleProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	IncrementCurrentStackPos();
	//	preserve graphics state
//	CGContextSaveGState(quartz);

	newton_try
	{
		SetActionDescription(-8103);
		BusyBoxSend(54);
		// handle the idle event just like any other event
		inEvent->fEventId = kIdleEventId;
		eventHandlerProc(inToken, inSize, inEvent);
		// run delayed/deferred NS actions
		RunDelayedActionProcs();
		// allow screen refresh
//		ReleaseScreenLock();
	}
	newton_catch_all
	{
		// exceptions shouldnÕt happen - notify debugger
		ExceptionNotify(&_info.exception);
		gREPout->exceptionNotify(&_info.exception);
		CheckForDeferredActions();
	}
	end_try;

	BusyBoxSend(53);
	//	restore graphics state
//	CGContextRestoreGState(quartz);
	// clean up temp RefVars
	DecrementCurrentStackPos();
	ClearRefHandles();
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C N e w t W o r l d
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Return the size of this class.
	Args:		--
	Return:	number of bytes
--------------------------------------------------------------------------------*/

size_t
CNewtWorld::getSizeOf(void) const
{
	return sizeof(CNewtWorld);
}


/*--------------------------------------------------------------------------------
	Initialize the forked world from its parent.
	Args:		inWorld		the parent
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNewtWorld::forkInit(CForkWorld * inWorld)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = CAppWorld::forkInit(inWorld))
		f70 = ((CNewtWorld *)inWorld)->f70;
		f74 = ((CNewtWorld *)inWorld)->f74;
		fEventHandler = ((CNewtWorld *)inWorld)->fEventHandler;
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Construct the forked world.
	We need to set up the interpreter and QD worlds.
	Args:		inWorld		the parent
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNewtWorld::forkConstructor(CForkWorld * inWorld)
{
	NewtonErr err;
	NewtGlobals * saveGlobals = GetNewtGlobals();
	SetNewtGlobals(&fGlobals);
	XTRY
	{
		XFAIL(err = CAppWorld::forkConstructor(inWorld))
		XFAIL(err = InitForkGlobalsForFrames(saveGlobals))
//		err = InitForkGlobalsForQD(saveGlobals);
	}
	XENDTRY;
	SetNewtGlobals(saveGlobals);
	return err;
}


/*--------------------------------------------------------------------------------
	Destroy the forked world.
	Reverse the effects of construction.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CNewtWorld::forkDestructor(void)
{
	acquireMutex();
	DestroyForkGlobalsForFrames(&fGlobals);
//	DestroyForkGlobalsForQD(&fGlobals);
	releaseMutex();

	CAppWorld::forkDestructor();
}


/*--------------------------------------------------------------------------------
	Construct the main thing...
	Args:		--
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNewtWorld::mainConstructor(void)
{
	NewtonErr err;
	SetNewtGlobals(&fGlobals);
	XTRY
	{
		XFAIL(err = CAppWorld::mainConstructor())

		CUSharedMemMsg	* msg;
		XFAILNOT(msg = new CUSharedMemMsg, err = MemError();)
		f70 = msg;
		f74 = 0;
		XFAIL(err = msg->init())

		CURealTimeAlarm::newName(&gNewtAlarmName);

//		InitializeCompression();	// sic -- done earlier by RegisterROMDomainManager
		InitObjects();
		InitTranslators();			// register flatten/unflatten etc

		NTKInit();
		gREPin = InitREPIn();
		gREPout = InitREPOut();
		ResetREPIdler();
		REPInit();

		InitUnicode();
		InitExternal();
		InitGraf();
//		InitFonts();

		gNewtPort = getMyPort();

		fEventHandler = new CNewtEventHandler;
		fEventHandler->init(kIdleEventId);
		fEventHandler->initIdler(0, 0, false);

		gApplication = new CARMNotebook;
		gApplication->init();

		CFormPartHandler * formPartHandler = new CFormPartHandler;
		formPartHandler->init('form');

		CBookPartHandler * bookPartHandler = new CBookPartHandler;
		bookPartHandler->init('book');

		CDictPartHandler * dictPartHandler = new CDictPartHandler;
		dictPartHandler->init('dict');

		CAutoScriptPartHandler * autoPartHandler = new CAutoScriptPartHandler;
		autoPartHandler->init('auto');

		CCommPartHandler * commPartHandler = new CCommPartHandler;
		commPartHandler->init('comm');

		HandleCardEvents();
		HandleTestAgentEvent();

		FMinimumBatteryCheck(RA(NILREF));

		if (EQRef(GetPreference(SYMA(blessedApp)), RSYMsetup))
			LoadInkerCalibration();

		AllocateEarlyStuff();

		StartDrawing(NULL, NULL);
	}
	XENDTRY;
	return err;
}


NewtonErr
CNewtWorld::preMain(void)
{
	gStrokeWorld.blockStrokes();
	gLastWakeupTime = GetGlobalTime();
	LoadHighROMFramesPackages();

	RefVar	stores(GetStores());
	RefVar	internal(GetArraySlot(stores, 0));
	RefVar	pkgSoup(StoreGetSoup(internal, RA(STRextrasSoupName)));
	SoupSetInfo(pkgSoup, SYMA(initialized), SYMA(extrasState));

	gApplication->run();

	GestaltRebootInfo	rebootInfo;
	CUGestalt	gestalt;
	NewtonErr	err = gestalt.gestalt(kGestalt_RebootInfo, &rebootInfo, sizeof(rebootInfo));
	bool			isResetSwitchPressed = NO;
	bool			isSetupBlessed = NO;
	if (rebootInfo.fRebootReason == kOSErrRebootFromResetSwitch)
	{
		isResetSwitchPressed = YES;
		isSetupBlessed = EQRef(GetPreference(SYMA(blessedApp)), RSYMsetup);
	}
	if (err == noErr
	&&  rebootInfo.fRebootReason != noErr
	&&  !isSetupBlessed)
		ErrorNotify(rebootInfo.fRebootReason, kNotifyAlert);
	ResetRebootReason();
	
#if defined(correct)
	NSCallGlobalFn(SYMA(ActivateStorePackages), internal);

	gCardEventHandler->readyToAcceptCardEvents();

	err = noErr;
	FILE *	script = fopen("bootTestScript", "r");
	if (script)
	{
		char	ch = getc(script);
		if (ch == EOF || ch == 0)
			fclose(script);
		else
		{
			fclose(script);
			freopen("bootCOutput", "w", stdout);
			setvbuf(stdout, g0C107BB4, 512, 16);
			gBootOut = MakeByName("POutTranslator", "PHammerOutTranslator");
			SetPtrName(gBootOut, 'boot');
			err = gBootOut->init({ "bootScriptOutput", 16 });
			gOldREPout = gREPout;
			gREPout = gBootOut;
			ParseFile("bootTestScript");
		}
	}
#endif

	if (err == noErr)
	{
	//	send an event (via the name server) to say the OS is up
		CSystemEvent		liveEvent(kSysEvent_AppAlive);
		CSendSystemEvent	sender(kSysEvent_AppAlive);
		sender.init();
		sender.sendSystemEvent(&liveEvent, sizeof(liveEvent));
	}

	gNewtIsAliveAndWell = YES;
	gStrokeWorld.unblockStrokes();
	handleEvents(1);

	return err;
}


void
CNewtWorld::theMain(void)
{
	StackLimits	limits;

	LockStack(&limits, 4*KByte);
	CAppWorld::theMain();
	UnlockStack(&limits);
}


void
CNewtWorld::forkSwitch(bool inDoFork)
{
	if (inDoFork)
		SetNewtGlobals(&fGlobals);
	SwitchFramesForkGlobals(inDoFork);
}


CForkWorld *
CNewtWorld::makeFork(void)
{
	return new CNewtWorld;
}


// suspiciously like CNewtEventHandler::idleProc
NewtonErr
CNewtWorld::eventDispatch(ULong inFlags, CUMsgToken * inToken, size_t * outSize, CEvent * inEvent)
{
	NewtonErr	err = noErr;

	IncrementCurrentStackPos();
//	GrafPtr	savePort = SetPort(&gScreenPort);
//	CGContextSaveGState(quartz);

	newton_try
	{
		SetActionDescription(-8103);
		BusyBoxSend(54);
		err = CAppWorld::eventDispatch(inFlags, inToken, outSize, inEvent);
		RunDelayedActionProcs();
//		ReleaseScreenLock();
	}
	newton_catch_all
	{
		Exception * x = CurrentException();
		ExceptionNotify(x);
		gREPout->exceptionNotify(x);
		CheckForDeferredActions();
	}
	end_try;

	BusyBoxSend(53);
//	SetPort(savePort);
//	CGContextRestoreGState(quartz);
	DecrementCurrentStackPos();
	ClearRefHandles();

	return err;
}

