/*
	File:		Application.cc

	Contains:	CApplication implementation.

	Written by:	Newton Research Group.
*/

#include "RootView.h"
#include "Application.h"

#include "Globals.h"
#include "Arrays.h"
#include "ROMResources.h"
#include "Funcs.h"
#include "NewtonScript.h"
#include "NewtWorld.h"
#include "Sound.h"

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FPostAndDo(RefArg inRcvr, RefArg inCmd);
Ref	FPostCommand(RefArg inRcvr, RefArg inCmd, RefArg inArg);
Ref	FPostCommandParam(RefArg inRcvr, RefArg inCmd, RefArg inArg, RefArg inArg2);

Ref	FAddDeferredCall(RefArg inRcvr, RefArg inMsg, RefArg inArg);
Ref	FAddDeferredSend(RefArg inRcvr, RefArg inTarget, RefArg inMsg, RefArg inArg);
Ref	FAddDelayedCall(RefArg inRcvr, RefArg inMsg, RefArg inArg, RefArg inTime);
Ref	FAddDelayedSend(RefArg inRcvr, RefArg inTarget, RefArg inMsg, RefArg inArg, RefArg inTime);

Ref	FAddUndoCall(RefArg inRcvr, RefArg inFunc, RefArg inArgs);
Ref	FAddUndoSend(RefArg inRcvr, RefArg inReceiver, RefArg inMsg, RefArg inArgs);
Ref	FAddUndoAction(RefArg inRcvr, RefArg inMsg, RefArg inArgs);
Ref	FClearUndoStacks(RefArg inRcvr);
Ref	FGetUndoState(RefArg inRcvr);

Ref	FHandleUnit(RefArg inRcvr, RefArg inCmd, RefArg inUnit);

Ref	FEventPause(RefArg inRcvr, RefArg inArg);
}


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

static void	MarkUndoCommand(RefArg inCmd);


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

extern bool		gNewtIsAliveAndWell;


/*------------------------------------------------------------------------------
	U t i l i t i e s
------------------------------------------------------------------------------*/

static Ref
MakeRunScriptCommand(RefArg inRcvr, RefArg inMsg, RefArg inArgs)
{
	RefVar cmd(MakeCommand(aeRunScript, gApplication, 0x08000000));
	RefVar parm(MakeArray(3));
	SetArraySlot(parm, 2, inRcvr);
	SetArraySlot(parm, 0, inMsg);
	SetArraySlot(parm, 1, inArgs);
	CommandSetFrameParameter(cmd, parm);
	return cmd;
}


static Ref
MakeUndoCommand(RefArg inRcvr, RefArg inMsg, RefArg inArgs)
{
	RefVar cmd(MakeCommand(aeRunScript, gApplication, 0x08000000));
	RefVar parm(AllocateArray(SYMA(undo), 3));
	SetArraySlot(parm, 2, inRcvr);
	SetArraySlot(parm, 0, inMsg);
	SetArraySlot(parm, 1, inArgs);
	CommandSetFrameParameter(cmd, parm);
	return cmd;
}


static void
MarkUndoCommand(RefArg inCmd)
{
	SetFrameSlot(inCmd, SYMA(undo), RA(TRUEREF));
}

#pragma mark -

/*------------------------------------------------------------------------------
	C A p p l i c a t i o n
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clApplication, CApplication, CResponder)


CApplication::CApplication()
{ }


void
CApplication::init(void)
{
	initToolbox();
	fUndoStack = MakeArray(0);
	fRedoStack = MakeArray(0);
	fNewUndo = false;
	fRedo = false;
	fIdleTime = CTime(1);
}


void
CApplication::run(void)
{ }


void
CApplication::idle(void)
{
	fNewUndo = true;
}


void
CApplication::quit(void)
{ }


/*------------------------------------------------------------------------------
	Initialize toolbox components.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CApplication::initToolbox(void)
{
	InitSound();
}


/*------------------------------------------------------------------------------
	Dispatch a command to the receiver (view) embedded in the command.
	Args:		inCmd		a command frame
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CApplication::dispatchCommand(RefArg inCmd)
{
	CResponder * rcvr;

	if ((rcvr = CommandReceiver(inCmd)) != NULL)
		rcvr->doCommand(inCmd);
	else
		ErrorNotify(-8003, kNotifyAlert);

	return CommandResult(inCmd);
}


/*------------------------------------------------------------------------------
	Perform a command.
	Args:		inCmd		a command frame
	Return:	true  if we handled the command
				false if not - the command should be passed to the next handler
------------------------------------------------------------------------------*/

bool
CApplication::doCommand(RefArg inCmd)
{
	bool isHandled = false;

	switch (CommandId(inCmd))
	{
	case aeQuit:
		x1C |= 0x80000000;
		isHandled = true;
		break;

	case aeRunScript:
		{
			RefVar cmdFrame(CommandFrameParameter(inCmd));
			RefVar cmdRcvr(GetArraySlot(cmdFrame, 2));
			RefVar cmdFunc(GetArraySlot(cmdFrame, 0));
			RefVar cmdArg(GetArraySlot(cmdFrame, 1));
			CommandSetResult(inCmd, noErr);
			if (EQ(ClassOf(cmdFrame), SYMA(undo)))
			{
				if (IsFunction(cmdFunc))
					DoBlock(cmdFunc, cmdArg);
				else
					DoMessage(cmdRcvr, cmdFunc, cmdArg);
			}
			else
			{
				CView * rcvr = GetView(cmdRcvr);
				if (rcvr != NULL)
					rcvr->runScript(cmdFunc, cmdArg, true, NULL);
				else
					CommandSetResult(inCmd, -8003);
			}
		}
		isHandled = true;
		break;

	case aeUndo:
		undo();
		isHandled = true;
		break;
	}

	return isHandled;
}


/*------------------------------------------------------------------------------
	undo
	There are two styles of undo:
	1.x	two levels of undo, no redo
	2.x	undo/redo
	In 2.x the method can be controlled by userConfiguration.undoRedo
	The undo/redo mechanism is preferred.
------------------------------------------------------------------------------*/

void
CApplication::postUndoCommand(ULong inId, CResponder * inRcvr, long inArg)
{
	RefVar	cmd(MakeCommand(inId, inRcvr ? inRcvr : this, inArg));
	postUndoCommand(cmd);
}


void
CApplication::postUndoCommand(RefArg inCmd)
{
	if (fNewUndo)
	{
		fNewUndo = false;
		if (NOTNIL(NSCallGlobalFn(SYMA(GetUserConfig), SYMA(undoRedo))))
			fRedoStack = fUndoStack;
		fUndoStack = MakeArray(0);
		fRedo = false;
	}
	MarkUndoCommand(inCmd);
	AddArraySlot(fUndoStack, inCmd);
}


void
CApplication::undo(void)
{
	RefVar undoStack(fUndoStack);
	RefVar redoStack(fRedoStack);
	bool isUndoRedo, wasRedo, isUndone;
	if ((isUndoRedo = ISNIL(NSCallGlobalFn(SYMA(GetUserConfig), SYMA(undoRedo)))))	// intentional assignment
		fRedoStack = MakeArray(0);
	fUndoStack = MakeArray(0);
	wasRedo = fRedo;
	isUndone = false;
	unwind_protect
	{
		while (!ArrayIsEmpty(undoStack))
		{
			RefVar	cmd(ArrayPop(undoStack));
			if (dispatchCommand(cmd) == noErr)
				isUndone = true;
		}
	}
	on_unwind
	{
		SetLength(undoStack, 0);
	}
	end_unwind;
	if (isUndoRedo)
		fUndoStack = redoStack;
	fRedo = !wasRedo;
	if (!isUndone)
		ErrorNotify(-8003, kNotifyAlert);
}


void
CApplication::clearUndo(void)
{
	fUndoStack = MakeArray(0);
	fRedoStack = MakeArray(0);
	fRedo = false;
}


Ref
CApplication::getUndoStack(int inDepth)
{
	if (inDepth == 0)
		return fUndoStack;
	else if (inDepth == 1)
		return fRedoStack;
	else
		return NILREF;
}


Ref
CApplication::getUndoState(void)
{
	if (NOTNIL(NSCallGlobalFn(SYMA(GetUserConfig), SYMA(undoRedo))) && fRedo)
		return SYMA(undoRedo);
	else
		return SYMA(undo);
}


/*------------------------------------------------------------------------------
	idle
------------------------------------------------------------------------------*/

void
CApplication::updateNextIdleTime(CTime inNextTime)
{
#if 1
	if ((inNextTime != CTime(0) && fIdleTime == CTime(0))		// no existing idle time, just set it
	||	 (inNextTime != CTime(0) && inNextTime < fIdleTime))	// next idle wanted sooner than atm
#else
// surely we MUST be able to cancel idle by zeroing fIdleTime?
	if (fIdleTime == CTime(0)		// no existing idle time, just set it
	||	 inNextTime < fIdleTime)	// next idle wanted sooner than atm
#endif
	{
		fIdleTime = inNextTime;
printf("CApplication::updateNextIdleTime(%lld)\n", (int64_t)fIdleTime);
	}
}


/*------------------------------------------------------------------------------
	delayed action
------------------------------------------------------------------------------*/

void
CApplication::addDelayedAction(RefArg inRcvr, RefArg inFunc, RefArg inArg, RefArg inTime)
{
	ArrayIndex index;

	if (ISNIL(fDelayedActions))
	{
		index = 0;
		fDelayedActions = MakeArray(4);
	}
	else
	{
		index = Length(fDelayedActions);
		SetLength(fDelayedActions, index + 4);
	}
	SetArraySlot(fDelayedActions, 0 + index, inRcvr);
	SetArraySlot(fDelayedActions, 1 + index, inFunc);
	SetArraySlot(fDelayedActions, 2 + index, inArg);
	if (ISINT(inTime))
	{
		RefVar actionTime(AllocateBinary(SYMA(Time), sizeof(CTime)));
		CDataPtr timeData(actionTime);
		*((CTime *)(Ptr)timeData) = TimeFromNow(RINT(inTime) * kMilliseconds);
		SetArraySlot(fDelayedActions, 3 + index, actionTime);
	}
	else
		SetArraySlot(fDelayedActions, 3 + index, RA(NILREF));
	gNewtWorld->handleEvents(kNoTimeout);
}


CTime
CApplication::nextDelayedActionTime(CTime inNextTime)
{
	CTime earliestActionTime(inNextTime);

	if (gNewtIsAliveAndWell && NOTNIL(fDelayedActions))
	{
		RefVar actionTime;
		for (ArrayIndex i = 3, count = Length(fDelayedActions); i < count; i += 4)
		{
			actionTime = GetArraySlot(fDelayedActions, i);
			if (NOTNIL(actionTime))
			{
				CDataPtr	timeData(actionTime);
				CTime theTime = *(CTime *)(Ptr)timeData;
				if (theTime != CTime(0)
				&&  theTime < earliestActionTime)
					earliestActionTime = theTime;
			}
		}
	}
	return earliestActionTime;
}


/*------------------------------------------------------------------------------
	Run Next Delayed Action.
	Each action occupies four elements on the fDelayedActions queue:
	0	receiver
	1	function symbol or bona fide function object
	2	function arguments
	3	time to run the action or NILREF if deferred action
------------------------------------------------------------------------------*/

bool
CApplication::runNextDelayedAction(void)
{
	if (NOTNIL(fDelayedActions) && Length(fDelayedActions) > 0)
	{
		CTime now(GetGlobalTime());
		RefVar actionTime;
		for (ArrayIndex i = 0; i < Length(fDelayedActions); i += 4)
		{
			actionTime = GetArraySlot(fDelayedActions, i+3);
			if (NOTNIL(actionTime))
			{
				CDataPtr	timeData(actionTime);
				CTime timeToRun = *((CTime *)(Ptr)timeData);
				if (timeToRun > now)
					continue;
			}
			// else no actionTime -- this is a deferred action
			RefVar cmdRcvr(GetArraySlot(fDelayedActions, i));
			RefVar cmdFunc(GetArraySlot(fDelayedActions, i+1));
			RefVar cmdArg(GetArraySlot(fDelayedActions, i+2));
			ArrayRemoveCount(fDelayedActions, i, 4);
			if (Length(fDelayedActions) == 0)
				fDelayedActions = NILREF;
			if (ISNIL(cmdRcvr))
				DoBlock(cmdFunc, cmdArg);
			else if (IsSymbol(cmdFunc))
				DoMessage(cmdRcvr, cmdFunc, cmdArg);
			else
				DoScript(cmdRcvr, cmdFunc, cmdArg);
			return true;
		}
	}

	return false;
}

#pragma mark -

/*------------------------------------------------------------------------------
	NewtonScript interface to deferred and delayed actions.
------------------------------------------------------------------------------*/

Ref
FAddDeferredSend(RefArg inRcvr, RefArg inTarget, RefArg inMsg, RefArg inArg)
{
	return FAddDelayedSend(inRcvr, inTarget, inMsg, inArg, RA(NILREF));
}

Ref
FAddDeferredCall(RefArg inRcvr, RefArg inMsg, RefArg inArg)
{
	return FAddDelayedCall(inRcvr, inMsg, inArg, RA(NILREF));
}

Ref
FAddDelayedSend(RefArg inRcvr, RefArg inTarget, RefArg inMsg, RefArg inArg, RefArg inTime)
{
	gApplication->addDelayedAction(inTarget, inMsg, inArg, inTime);
	return TRUEREF;
}

Ref
FAddDelayedCall(RefArg inRcvr, RefArg inMsg, RefArg inArg, RefArg inTime)
{
	gApplication->addDelayedAction(RA(NILREF), inMsg, inArg, inTime);
	return TRUEREF;
}

#pragma mark -

/*------------------------------------------------------------------------------
	NewtonScript interface to undo actions.
------------------------------------------------------------------------------*/

Ref
FAddUndoCall(RefArg inRcvr, RefArg inFunc, RefArg inArgs)
{
	RefVar cmd(MakeUndoCommand(RA(NILREF), inFunc, inArgs));
	gApplication->postUndoCommand(cmd);
	return TRUEREF;
}


Ref
FAddUndoSend(RefArg inRcvr, RefArg inReceiver, RefArg inMsg, RefArg inArgs)
{
	RefVar cmd(MakeUndoCommand(inReceiver, inMsg, inArgs));
	gApplication->postUndoCommand(cmd);
	return TRUEREF;
}


Ref
FAddUndoAction(RefArg inRcvr, RefArg inMsg, RefArg inArgs)
{
	RefVar cmd(MakeRunScriptCommand(inRcvr, inMsg, inArgs));
	gApplication->postUndoCommand(cmd);
	return TRUEREF;
}


Ref
FClearUndoStacks(RefArg inRcvr)
{
	gApplication->clearUndo();
	return NILREF;
}


Ref
FGetUndoState(RefArg inRcvr)
{
	return gApplication->getUndoState();
}


#pragma mark -
extern CView * GetView(RefArg inContext, RefArg inView);
extern CUnit * UnitFromRef(RefArg inRef);

CResponder *
GetResponder(RefArg inContext, RefArg inView)
{
	return EQ(inView, SYMA(application)) ? (CResponder *)gApplication : GetView(inContext, inView);
}

CResponder *
FailGetResponder(RefArg inContext, RefArg inView)
{
	CResponder *  responder = GetResponder(inContext, inView);
	if (responder == NULL)
		ThrowMsg("NULL responder");
	return responder;
}

Ref
FHandleUnit(RefArg inRcvr, RefArg inCmd, RefArg inUnit)
{
	CResponder * responder = FailGetResponder(inRcvr, RA(NILREF));
	CUnit * unit = UnitFromRef(inUnit);
	ULong cmdId = RINT(inCmd);
	return MAKEBOOLEAN(gApplication->dispatchCommand(MakeCommand(cmdId, responder, (long)unit)));
}


Ref
FPostAndDo(RefArg inRcvr, RefArg inCmd)
{
	gApplication->dispatchCommand(inCmd);
	return TRUEREF;
}


Ref
FPostCommand(RefArg inRcvr, RefArg inView, RefArg inCmd)
{
	CResponder * responder = FailGetResponder(inRcvr, inView);
	ULong cmdId;
	if (IsString(inCmd))
		cmdId = *(ULong *)(char *)CDataPtr(inCmd);
	else
		cmdId = RINT(inCmd);
	if (cmdId != 0)
		gApplication->dispatchCommand(MakeCommand(cmdId, responder, 0x08000000));
	return TRUEREF;
}


Ref
FPostCommandParam(RefArg inRcvr, RefArg inView, RefArg inCmd, RefArg inArg)
{
	CResponder * responder = FailGetResponder(inRcvr, inView);
	ULong cmdId;
	if (ISINT(inCmd) && (cmdId = RVALUE(inCmd)) != 0)
	{
		if (ISINT(inArg))
			gApplication->dispatchCommand(MakeCommand(cmdId, responder, inArg));
		else
		{
			RefVar cmd(MakeCommand(cmdId, responder, 0x08000000));
			CommandSetFrameParameter(cmd, inArg);
			gApplication->dispatchCommand(cmd);
		}
	}
	return TRUEREF;
}


#include "StrokeCentral.h"

extern CTime gTickleTime;		// 0C100D04
extern CTime gLastIOEvent;		// 0C100D0C
extern CTime gLastPenupTime;	// 0C100D14
extern CTime gLastWakeupTime;	// 0C104C4C

Ref
FEventPause(RefArg inRcvr, RefArg inArg)
{
	CTime now(GetGlobalTime());

	if (NOTNIL(GetGlobalVar(SYMA(ioBusy))))
		gLastIOEvent = now;

	CTime mostRecentEvent;
	if (NOTNIL(inArg))
	{
		gTickleTime = now;
		mostRecentEvent = gTickleTime;
	}
	else
	{
#if defined(correct)
		int penUpTicks = gStrokeWorld.f10;
		if (penUpTicks)
			gLastPenupTime = now - CTime((Ticks() - penUpTicks)/60, kSeconds);

		mostRecentEvent = gLastPenupTime;
		if (gLastIOEvent > mostRecentEvent)
			mostRecentEvent = gLastIOEvent;
		if (gLastWakeupTime > mostRecentEvent)
			mostRecentEvent = gLastWakeupTime;
		if (gTickleTime > mostRecentEvent)
			mostRecentEvent = gTickleTime;
#else
		mostRecentEvent = now;
#endif
	}

	CTime delta(now - mostRecentEvent);
	return MAKEINT(delta.convertTo(kSeconds));
}

#pragma mark -

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
	to NewtonScript forks -- used solely in progress dialog.
------------------------------------------------------------------------------*/

#include "Recognition.h"


extern "C" {
Ref	FForkScript(RefArg rcvr, RefArg inFn, RefArg inArgs);
Ref	FYieldToFork(RefArg rcvr);
}


Ref
FForkScript(RefArg rcvr, RefArg inFn, RefArg inArgs)
{
	RefVar result;
	if (gAppWorld->fork(NULL) == noErr)
	{
		bool saveFailed = false;
		RecognitionState * savedState = gRecognitionManager.saveRecognitionState(&saveFailed);
		unwind_protect
		{
			if (!saveFailed)
				result = DoBlock(inFn, inArgs);
		}
		on_unwind
		{
			gRecognitionManager.restoreRecognitionState(savedState);
		}
		end_unwind;
	}
	else
		ThrowMsg("couldn't fork it over");
	return result;
}


Ref
FYieldToFork(RefArg rcvr)
{
	gAppWorld->yield();
	return NILREF;
}

