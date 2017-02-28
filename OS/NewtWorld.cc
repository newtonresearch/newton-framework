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
#include "PackageParts.h"
#include "PackageEvents.h"
#include "PartHandler.h"
#include "Platform.h"
// Frames
#include "ObjectHeap.h"
#include "Frames.h"
#include "NewtonScript.h"
#include "Funcs.h"
#include "REP.h"
#include "Strings.h"
#include "ROMResources.h"
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
extern "C" {
Ref	FMinimumBatteryCheck(RefArg inRcvr);
}

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

extern CULockingSemaphore *	gPackageSemaphore;

CTime					gLastWakeupTime;

CUPort *				gNewtPort;
CApplication *		gApplication;
NewtGlobals *		gNewtGlobals;
ULong					gNewtAlarmName;
bool					gNewtIsAliveAndWell = false;
bool					gGoingToSleep = false;

CTime					gTickleTime;		// 0C100D04
CTime					gLastIOEvent;		// 0C100D0C
CTime					gLastPenupTime;	// 0C100D14


/*------------------------------------------------------------------------------
	I n i t i a l i z a t i o n
------------------------------------------------------------------------------*/

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


/*--------------------------------------------------------------------------------
	Load packages from the ROM extension.
	There are 10 packages in the 717006 ROM:
	 1 Cardfile app
	 2	Connection app
	 3	FaxViewer app
	 4	Formulas app
	 5	Help book
	 6	ListView
	 7	TScreenMemory:TReservedContiguousMemory protocol
	 8	TMainDisplayDriver:TScreenDriver protocol
	 9	Setup app
	10	WorldData soup

	Args:		inFrames		true => load NS parts
								false => load protocol (driver) parts
	Return:	--
--------------------------------------------------------------------------------*/

void
LoadHighROMPackages(bool inFrames)	// frames vs drivers
{
printf("LoadHighROMPackages(%s)\n", inFrames? "frames":"drivers");
	for (ArrayIndex i = 0; i < kMaxROMExtensions; ++i) {
		// if there are packages in this ROM extension
		Ptr pkgList = (Ptr)GetPackageList(i);
		if (pkgList != NULL) {
			SourceType srcType;
			srcType.deviceKind = kNoDevice;
			srcType.deviceNumber = 0;
			srcType.format = kFixedMemory;
			PackageDirectory * pkg;
			NewtonErr err = noErr;
			// iterate over them -- theyÕre contiguous in memory
			for (long pkgOffset = 0; err == noErr || err == kOSErrPackageAlreadyExists || err == kOSErrPartTypeNotRegistered; pkgOffset += CANONICAL_LONG(pkg->size)) {
				newton_try
				{
					// point to pkg directory
					pkg = (PackageDirectory *)(pkgList + pkgOffset);
					if (inFrames) {
						gNewtWorld->fork(NULL);		// each package is loaded in its own fork
					}
					// thatÕs our source
					PartSource src;
					src.mem.buffer = pkg;
				// src.mem.size = irrelevant;
					size_t size;

					// start loading pkg -- create event to send...
					CPkBeginLoadEvent loader(srcType, src, *gNewtWorld->getMyPort(), *gNewtWorld->getMyPort(), true);
					// ...to the package manager
					CUPort pkgMgr(PackageManagerPortId());
					if (inFrames) {
						gNewtWorld->releaseMutex();
					}
					gPackageSemaphore->acquire(kWaitOnBlock);
					err = pkgMgr.sendRPC(&size, &loader, sizeof(loader), &loader, sizeof(loader));
					gPackageSemaphore->release();
					if (inFrames) {
						gNewtWorld->acquireMutex();
					}

					if (err == noErr) {
						err = loader.fEventErr;
					}
				}
				cleanup
				{ }
				end_try;
printf("=> err=%d\n", err);
			}
		}
	}

// this should be a protocol part in the ROM extension (Package-8)
	if (!inFrames)
	{
		CMainDisplayDriver::classInfo()->registerProtocol();
	}
}

void
LoadHighROMDriverPackages(void)
{
	LoadHighROMPackages(false);
}

void
LoadHighROMFramesPackages(void)
{
	LoadHighROMPackages(true);
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
#define kMaxNameLen 64
class CRunScriptEvent : public CEvent
{
public:
	EventType	fEventType;
	char			fRcvrName[kMaxNameLen];
	char			fMsgName[kMaxNameLen];
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
	strncpy(request.fRcvrName, inRcvrName, kMaxNameLen);
	strncpy(request.fMsgName, inMsgName, kMaxNameLen);
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
		ioEvent->fError = (NewtonErr)(long)CurrentException()->data;
	}
	end_try;
}


#pragma mark -
/*------------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
------------------------------------------------------------------------------*/
extern int GMTOffset(void);
extern int DaylightSavingsOffset(void);

// from Dates.cc
#if defined(Fix2010)
#define kSecsSince1904 0xBA37E000
#else
#define kSecsSince1904 0xA7693A00
#endif


Ref
FSetSysAlarm(RefArg inRcvr, RefArg inTime, RefArg inCallback, RefArg inCallbackArgs)
{
	CURealTimeAlarm::clearAlarm(gNewtAlarmName);
	if (NOTNIL(inTime))
	{
		CTime timeBase(kSecsSince1904, kSeconds);
		CTime timeOfAlarm(RINT(inTime), kSeconds);
		CTime timeAdjustment(GMTOffset() + DaylightSavingsOffset(), kSeconds);
		CTime finalTime(timeBase + timeOfAlarm - timeAdjustment);
		gApplication->fAlarm.fEventClass = kNewtEventClass;
		gApplication->fAlarm.fEventId = kIdleEventId;
		gApplication->fAlarm.fEventType = 'alrm';
		gApplication->fAlarm.fTriggerTime = finalTime;
		gApplication->fAlarm.fCallbackFn = inCallback;
		gApplication->fAlarm.fCallbackArgs = inCallbackArgs;
		CURealTimeAlarm::setAlarm(gNewtAlarmName, finalTime, *gNewtPort, gNewtWorld->msgId(), &gApplication->fAlarm, sizeof(gApplication->fAlarm), 1);
	}
	return NILREF;
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
	gGPIInterruptAsyncMessage.init(false);
	gGPIInterruptMessage.fEventClass = kNewtEventClass;	// make this an init() method?
	gGPIInterruptMessage.fEventId = kIdleEventId;
	gGPIInterruptMessage.f08 = 'ext ';
#endif
	InitEvents();								// register CHistoryCollector protocol
	InitAlertManager();						//	create CAlertManager
	InitializeSound();						//	start SoundServer, register codecs
	InitializeCommManager();				//	create CCMWorld, register ROM protocols
	InitCardServices();						// initialize CCardDomains, CCardServer
	if ((gNewtTests & 0x0800) != 0)
		InitTestAgent();						// do a nameserver lookup of CUPort 'tagt'
	ZapInternalStoreCheck();				//	ask user (if necessary) whether persistent data should be erased

	ObjectId	userEnvId;
	ObjectId	ramsEnvId;
	ObjectId	ramsDomId;
	MemObjManager::findEnvironmentId('user', &userEnvId);
	MemObjManager::findEnvironmentId('rams', &ramsEnvId);
	MemObjManager::findDomainId('rams', &ramsDomId);

	InitPowerManager();

	CUEnvironment	environment(userEnvId);
	environment.add(ramsDomId, true/*isManager*/, false/*hasntStack*/, false/*hasntHeap*/);
	ULong	savePriority = gMonitorTaskPriority;
//	gMonitorTaskPriority = gTmuxTaskPriority;
	InitPSSManager(ramsEnvId, ramsDomId);
	gMonitorTaskPriority = savePriority;
	environment.remove(ramsDomId);
	environment.add(ramsDomId, false/*isntManager*/, false/*hasntStack*/, false/*hasntHeap*/);

	XTRY
	{
		CUTask	newt;
		XFAIL(newt.init((TaskProcPtr)UserMain, 26*KByte, 0, NULL, kUserTaskPriority, 'main'))
		newt.start();
	}
	XENDTRY;

	SetBequeathId(*gIdleTask);
}


void
UserMain(void)
{
	CNewtWorld theWorld;
	theWorld.init('newt', true, 10*KByte);
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
//printf("CNewtEventHandler::setWakeupTime(%d) idle=%lld\n", inDelta, (int64_t)wuTime);

	if (inDelta != kNoTimeout)
	{
		CTime nextTime(TimeFromNow(inDelta));
		if (nextTime < wuTime)
			// we want the app to be woken sooner than it thinks
			wuTime = nextTime;
	}

	// find the next time at which a delayed action should be invoked
	wuTime = gApplication->nextDelayedActionTime(wuTime);
//printf("nextDelayedAction=%lld\n", (int64_t)wuTime);

	if (wuTime == CTime(0)) {
//printf("stopIdle()\n");
		stopIdle();
	} else {
		long wait;
		CTime delta(wuTime - GetGlobalTime());
		if (delta > CTime(0))
			wait = delta.convertTo(kMilliseconds);
		else
			wait = 1*kMilliseconds;
//printf("resetIdle(%ld)\n", wait);
		resetIdle(wait, kMilliseconds);
	}
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
printf("CNewtEventHandler::eventHandlerProc(eventId='%c%c%c%c')\n", inEvent->fEventId>>24,inEvent->fEventId>>16,inEvent->fEventId>>8,inEvent->fEventId);
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
		gRootView->runScript(SYMA(IRConnectRequest), RA(NILREF), true, NULL);
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
					gGoingToSleep = true;
					gRootView->runScript(SYMA(GotoSleep), RA(NILREF), true, NULL);
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
//	GrafPtr savePort = SetPort(&gScreenPort);

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
//	SetPort(savePort);
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

		CUSharedMemMsg	* msg = new CUSharedMemMsg;
		XFAILIF(msg == NULL, err = MemError();)
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

		if (EQ(GetPreference(SYMA(blessedApp)), SYMA(setup)))
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
	RefVar	pkgSoup(StoreGetSoup(internal, RA(extrasSoupName)));
	SoupSetInfo(pkgSoup, SYMA(initialized), SYMA(extrasState));

	gApplication->run();

	GestaltRebootInfo	rebootInfo;
	CUGestalt	gestalt;
	NewtonErr	err = gestalt.gestalt(kGestalt_RebootInfo, &rebootInfo, sizeof(rebootInfo));
	bool			isResetSwitchPressed = false;
	bool			isSetupBlessed = false;
	if (rebootInfo.fRebootReason == kOSErrRebootFromResetSwitch)
	{
		isResetSwitchPressed = true;
		isSetupBlessed = EQ(GetPreference(SYMA(blessedApp)), SYMA(setup));
	}
	if (err == noErr
	&&  rebootInfo.fRebootReason != noErr
	&&  !isSetupBlessed)
		ErrorNotify(rebootInfo.fRebootReason, kNotifyAlert);
	ResetRebootReason();
	
	NSCallGlobalFn(SYMA(ActivateStorePackages), internal);

#if defined(correct)
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
		CEventSystemEvent	liveEvent(kSysEvent_AppAlive);
		CSendSystemEvent	sender(kSysEvent_AppAlive);
		sender.init();
		sender.sendSystemEvent(&liveEvent, sizeof(liveEvent));
	}

	gNewtIsAliveAndWell = true;
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
//	GrafPtr savePort = SetPort(&gScreenPort);

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
	DecrementCurrentStackPos();
	ClearRefHandles();

	return err;
}

