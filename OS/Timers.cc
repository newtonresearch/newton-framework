/*
	File:		Timers.cc

	Contains:	Timer implementation.

	Written by:	Newton Research Group.
*/
#include <mach/mach.h>
#include <mach/clock.h>
#include <mach/mach_error.h>
#include <stdio.h>
#include <time.h>

#include "Objects.h"
#include "KernelGlobals.h"
#include "NewtonTime.h"
#include "Interrupt.h"
#include "LongTime.h"	// CURealTimeAlarm
#include "UserPorts.h"	// SendForInterrupt
#include "Semaphore.h"
#include "OSErrors.h"
#include "Dates.h"

extern void SetPlatformAlarm(int64_t inDelta);


#if defined(forFramework)
// we need a stub for the scheduler
void CScheduler::remove(CTask * inTask) {}

#else
CTimerEngine *	gTimerEngine;		// 0C100FE0

CDoubleQContainer * gTimerDeferred;	// 0C101048

CTime	gTimerSample;					// 0C10156C
long	gTimerInterruptCount;		// 0C101574	+08
//	CSharedMemMsg	gOverflowDetectMsg;	// 0C101578	+0C


/*------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
------------------------------------------------------------------------------*/
		CTime	GetClock(void);
#if defined(correct)
// we don’t need all this overflow stuff
static void	UpdateClock(void);
static void	StartTimerOverflowDetect(void);
static void	RestartTimerOverflowDetect(void);
#endif

void
InitTime(void)
{
	gTimerDeferred = new CDoubleQContainer(offsetof(CSharedMemMsg, fTimerQItem));
	gTimerEngine = new CTimerEngine();
	gTimerEngine->init();
}


void
StartTime(void)
{
	gTimerInterruptCount = 0;
	gTimerEngine->start();
#if defined(correct)
	StartTimerOverflowDetect();
#endif
}


void
TimerInterruptHandler(void * intObj)
{
	gTimerInterruptCount++;
	gTimerEngine->alarm();
}

#if defined(correct)
static void
UpdateClock(void)
{
	gTimerSample = GetClock();
}


static void
StartTimerOverflowDetect(void)
{
	UpdateClock();
	gTimerEngine->queueTimer(&gOverflowDetectMsg, 1*kMinutes, NULL, RestartTimerOverflowDetect);
}


static void
RestartTimerOverflowDetect(void)
{
	UpdateClock();
	gTimerEngine->queueTimer(&gOverflowDetectMsg, 1*kMinutes, NULL, RestartTimerOverflowDetect);
}
#endif

/* -----------------------------------------------------------------------------
	Read the RTC.
	SUPER MODE - accesses RTC hardware.
	Args:		--
	Return:	time in seconds
----------------------------------------------------------------------------- */

ULong
GetRealTimeClock(void)
{
#if defined(correct)
	ULong	time1, time2;
	do
	{
		time1 = g0F181000;
		time2 = g0F181000;
	} while (time1 != time2);
	return time1;
#else
	return time(NULL);
#endif
}


void
SafeShortTimerDelay(ULong delay)
{
#if defined(correct)
	g0001952C |= 0x0800;
	long start = g00019530;
	while (g00019530 - start < delay)
		/*wait*/ ;
#endif
}


NewtonErr
PrimRegisterDelayedFunction(ObjectId inMsgId, Timeout inTimeout, void * inData, TimeoutProcPtr inProc)
{
	NewtonErr err = noErr;
	XTRY
	{
		CSharedMemMsg * msg;
		XFAILNOT(msg = (CSharedMemMsg *)IdToObj(kSharedMemMsgType, inMsgId), err = kOSErrBadObjectId;)	// original says kOSErrBadObject
		XFAILNOT(gTimerEngine->queueTimer(msg, inTimeout, inData, inProc), err = kOSErrTimerExpired;)
	}
	XENDTRY;
	return err;
}


NewtonErr
PrimRemoveDelayedFunction(ObjectId inMsgId)
{
	NewtonErr err = noErr;
	XTRY
	{
		CSharedMemMsg * msg;
		XFAILNOT(msg = (CSharedMemMsg *)IdToObj(kSharedMemMsgType, inMsgId), err = kOSErrBadObjectId;)	// original says kOSErrBadObject
		gTimerEngine->remove(msg);
	}
	XENDTRY;
	return err;
}

#pragma mark -

void
SetAlarm1(ULong inWhen)
{
#if defined(correct)
	g0F182800 = inWhen;
	g0F184000 |= 0x0020;
	gIntMaskShadowReg |= 0x0020;
#endif
}

void
FireAlarm(void)
{
#if defined(correct)
	g0F183800 = 0x0020;
	gIntMaskShadowReg |= 0x0020;
#endif
}

void
DisableAlarm1(void)
{
#if defined(correct)
	g0F184000 &= ~0x0020;
	gIntMaskShadowReg &= ~0x0020;
	g0F183800 = 0x0020;
#endif
}


/* -----------------------------------------------------------------------------
	Set a timer interrupt.
	SUPER MODE - accesses RTC hardware.
	Args:		inTime		absolute time at which to fire
	Return:	true => alarm set
----------------------------------------------------------------------------- */

bool
SetAlarm(CTime & inTime)
{
	bool isSet = false;
#if defined(correct)
	if (inTime.fTime.hi >= 0)
	{
		CTime now(gTimerSample);		// r2 r3
		ULong counter = g0F181000;		// bring last timer sample up-to-date
		if (counter <= now.fTime.lo)
			now.fTime.hi++;				// lo-word wrapped around
		now.fTime.lo = counter;
		if (inTime.fTime.hi > now.fTime.hi  ||  (inTime.fTime.hi == now.fTime.hi && inTime.fTime.lo > now.fTime.lo))
		{
			SetAlarm1(inTime.fTime.lo);
			// check that the timer hasn’t expired already!
			counter = g0F181000;			// bring last timer sample up-to-date
			if (counter <= now.fTime.lo)
				now.fTime.hi++;			// lo-word wrapped around
			now.fTime.lo = counter;
			if (inTime.fTime.hi > now.fTime.hi  ||  (inTime.fTime.hi == now.fTime.hi && inTime.fTime.lo > now.fTime.lo))
				isSet = true;
			else
				DisableAlarm1();
		}
	}
#else
	CTime now(GetClock());
//printf("SetAlarm(%ld) now=%ld delta=%ld\n", (long)inTime,(long)now,(long)(inTime-now));
	if (inTime > now)
	{
		CTime delta = inTime - now;
		// if alarm is more than 1µs away, set up interrupt
		if (delta > CTime(1*kMicroseconds))
		{
//printf(" ->SetPlatformAlarm(%ld)\n", (long)delta);
			SetPlatformAlarm(delta);
			isSet = true;
		}
	}
#endif
	return isSet;
}

bool
SetAlarmAtomic(CTime & inTime)
{
	bool	isSet;
	EnterFIQAtomic();
	isSet = SetAlarm(inTime);
	ExitFIQAtomic();
	return isSet;
}


/* -----------------------------------------------------------------------------
	Clear the timer interrupt.
	There is only one alarm timer; any queuing of alarms is handled externally.
	SUPER MODE - accesses RTC hardware.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
ClearAlarm(void)
{
	DisableAlarm1();
}

void
ClearAlarmAtomic(void)
{
	EnterFIQAtomic();
	ClearAlarm();
	ExitFIQAtomic();
}


#pragma mark -

/*------------------------------------------------------------------------------
	C T i m e r E n g i n e
------------------------------------------------------------------------------*/

CTimerEngine::CTimerEngine()
	: CDoubleQContainer(offsetof(CSharedMemMsg, fTimerQItem))
{ }

void
CTimerEngine::init(void)
{ /* this really does nothing */ }

void
CTimerEngine::start(void)
{ /* this really does nothing */ }


void
CTimerEngine::alarm(void)
{
	EnterAtomic();

	CSharedMemMsg * msg = (CSharedMemMsg *)peek();
	if (msg == NULL)
	{
		// nothing in the queue, disable the timer interrupt
		ClearAlarmAtomic();
//printf("CTimerEngine::alarm() empty queue\n");
	}

	else
	{
		do
		{
			if (msg->fExpiryTime <= GetClock())
			{
				// message has expired, remove alarm from queue
				CDoubleQContainer::remove();
				if (MASKTEST(msg->fFlags, kSMemMsgFlags_TimerMask) == kSMemMsgFlags_Timer)
					msg->fFlags = MASKCLEAR(msg->fFlags, kSMemMsgFlags_TimerMask);
				// fire the callback
//printf("CTimerEngine::alarm() callback sender=%d notify=%d\n", msg->fSendingPort,msg->fNotify);
				msg->fCallback(msg->fCallbackData);
			}
		} while ((msg = (CSharedMemMsg *)peek()) != NULL && !SetAlarmAtomic(msg->fExpiryTime));	// if alarms in the queue, reset timer interrupt
	}

	ExitAtomic();
}


bool
CTimerEngine::queueTimer(CSharedMemMsg * inMsg, Timeout inTimeout, void * inData, TimeoutProcPtr inProc)
{
	bool	isQueued;

	checkBeforeAdd(inMsg);
	if (MASKTEST(inMsg->fFlags, kSMemMsgFlags_TimerMask) != 0)
		return false;	// message is already timed
	
	inMsg->fCallbackData = inData;
	inMsg->fCallback = inProc;
	inMsg->fExpiryTime = GetClock() + CTime(inTimeout);

	EnterAtomic();
	FLAGSET(inMsg->fFlags, kSMemMsgFlags_Timer);
	isQueued = queue(inMsg);
	if (!isQueued)
		inMsg->fFlags = MASKCLEAR(inMsg->fFlags, kSMemMsgFlags_TimerMask);
	ExitAtomic();

	return isQueued;
}


bool
CTimerEngine::queueTimeout(CSharedMemMsg * inMsg)
{
	bool	isQueued;

	if (inMsg->fTimeout == kTimeOutImmediate)
		return false;
	
	inMsg->fCallbackData = inMsg;
	inMsg->fCallback = (TimeoutProcPtr) QueueNotify;
	inMsg->fExpiryTime = GetClock() + CTime(inMsg->fTimeout);

	EnterAtomic();
	FLAGSET(inMsg->fFlags, kSMemMsgFlags_Timeout);
	isQueued = queue(inMsg);
	if (!isQueued)
		inMsg->fFlags = MASKCLEAR(inMsg->fFlags, kSMemMsgFlags_TimerMask);
	ExitAtomic();

	return isQueued;
}


bool
CTimerEngine::queueDelay(CSharedMemMsg * inMsg)
{
	bool	isQueued;

	if (inMsg->fExpiryTime == CTime(0))
		return 0;

	inMsg->fCallbackData = inMsg;
	inMsg->fCallback = (TimeoutProcPtr) QueueNotify;

	EnterAtomic();
	FLAGSET(inMsg->fFlags, kSMemMsgFlags_Delay);
	isQueued = queue(inMsg);
	if (!isQueued)
		inMsg->fFlags = MASKCLEAR(inMsg->fFlags, kSMemMsgFlags_TimerMask);
	ExitAtomic();

	return isQueued;
}


bool
CTimerEngine::queue(CSharedMemMsg * inMsg)
{
	bool	isSet = true;
	CSharedMemMsg *	msg;

	EnterAtomic();
	if ((msg = (CSharedMemMsg *)peek()) != NULL)
	{
		// there’s something already in the queue -- find where to place the new message
		for ( ; msg != NULL; msg = (CSharedMemMsg *)getNext(msg))
		{
			if (inMsg->fExpiryTime < msg->fExpiryTime)
			{
				// message will expire before this message in queue
//printf("CTimerEngine::queue() %d -> %d\n", inMsg->fSendingPort,inMsg->fNotify);
				if (msg == peek())
				{
					if ((isSet = SetAlarmAtomic(inMsg->fExpiryTime)))	// intentional assignment
						addToFront(inMsg);
					else
						alarm();
				}
				else
				{
					addBefore(msg, inMsg);
				}
				break;
			}
		}
		if (msg == NULL)
			add(inMsg);
	}
	else
	{
		// queue is empty
//printf("CTimerEngine::queue() empty sender=%d notify=%d\n", inMsg->fSendingPort,inMsg->fNotify);
		if ((isSet = SetAlarmAtomic(inMsg->fExpiryTime)))	// intentional assignment
			// alarm was set OK, add message to the queue
			addToFront(inMsg);
		else
			// no alarm set, better fire it now
			alarm();
	}
	ExitAtomic();

	return isSet;
}


void
CTimerEngine::remove(CSharedMemMsg * inMsg)
{
	EnterAtomic();
	if (gTimerDeferred->removeFromQueue(inMsg) == false)
	{
		// message wasn’t in the gTimerDeferred queue
		if (inMsg == peek())
		{
			// message is first in our queue, remove it
			CDoubleQContainer::remove();
			if (peek())
				// more in the queue, fire an alarm to check them now
				alarm();
			else
				// queue is now empty, no alarm required
				ClearAlarmAtomic();
		}
		else
			// message is in our queue, remove it
			removeFromQueue(inMsg);
	}
	inMsg->fFlags = MASKCLEAR(inMsg->fFlags, kSMemMsgFlags_TimerMask);
	ExitAtomic();
}

#pragma mark -

/*------------------------------------------------------------------------------
	C R e a l T i m e A l a r m
------------------------------------------------------------------------------*/

#define kNumOfAlarms 16
CRealTimeAlarm gAlarm[kNumOfAlarms];	// 0C106A44

/*------------------------------------------------------------------------------
	Initialize the alarm.
	Args:		inTime			time (in seconds) to execute the alarm
				inPortId			port to which message should be sent
				inMsgId			message to send
				inData			data to accompany message
				inDataSize		size of that data
				inType			type of message
	Return:	error code
------------------------------------------------------------------------------*/

void
CRealTimeAlarm::init(ULong inTime, ObjectId inPortId, ObjectId inMsgId, void * inData, long inDataSize, ULong inType)
{
	fTime = inTime;
	fPortId = inPortId;
	fMsgId = inMsgId;
	fType = inType;
	fData = inData;
	fDataSize = inDataSize;
	fError = noErr;
	fIsWakeUp = true;
	fIsHandler = false;
	fIsRelative = false;
	fIsEnabled = true;
}


/*------------------------------------------------------------------------------
	Initialize the alarm.
	It fires off an interrupt handler when the alarm fires, rather than a message.
	Args:		inTime			time (in seconds) to execute the alarm
				inHandler		interrupt handler
				inData			data
				inWakeUp			should Newt wake up
				inRelative		is time relative (or absolute)
				ioLock			lock semaphore
	Return:	error code
------------------------------------------------------------------------------*/

void
CRealTimeAlarm::init(ULong inTime, NewtonInterruptHandler inHandler, void * inData, ULong inWakeUp, ULong inRelative, ULong * ioLock)
{
	fIsRelative = inRelative;
	fIsWakeUp = inWakeUp;
	fIsHandler = true;
	fHandler = inHandler;
	fData = inData;
	fError = noErr;

	if (inRelative)
	{
		while (Swap(ioLock, 1) != 0)
			;				// acquire lock
		fTime = GetRealTimeClock() + inTime;
		fIsEnabled = true;
		*ioLock = 0;	// release lock
	}
	else
	{
		fTime = inTime;
		fIsEnabled = true;
	}
}


/*------------------------------------------------------------------------------
	Fire off the alarm.
	Args:		inWhen		the time now, in seeconds
	Return:	--
------------------------------------------------------------------------------*/

void
CRealTimeAlarm::fire(ULong inWhen)
{
	// if it’s not time yet, or we’re not enabled, forget it
	if (fTime > inWhen || Swap(&fIsEnabled, 0) == 0)
		return;

	if (fIsHandler)
		fError = fHandler(fData);
	else
		fError = SendForInterrupt(fPortId, fMsgId, 0, fData, fDataSize);
}

#pragma mark -

/*------------------------------------------------------------------------------
	C R e a l T i m e C l o c k
------------------------------------------------------------------------------*/

InterruptObject *	gAlarmIntObj;			// 0C101824

bool		CRealTimeClock::fAlarmSet;
ULong		CRealTimeClock::fCurrentAlarm;
ULong		CRealTimeClock::fChangingTime;
ULong		CRealTimeClock::fUpdatingAlarm;
ULong		CRealTimeClock::fFixUp;
ULong		CRealTimeClock::fAssigningName;
ULong		CRealTimeClock::fNextName;


void
InitRealTimeClock(void)
{
	CRealTimeClock::init();
}


void
SetRealTimeClock(ULong inSeconds)
{
	CRealTimeClock::setRealTimeClock(inSeconds);
}


bool
RegisterRealTimeClockHandler(void * inQueue, NewtonInterruptHandler inHandler)
{
	gAlarmIntObj = RegisterInterrupt(0x0004, inQueue, inHandler, 0x0001);
	return gAlarmIntObj != NULL;
}


void
SetRealTimeClockAlarm(ULong inSeconds)
{
	g0F181400 = inSeconds;
	EnableInterrupt(gAlarmIntObj, 0x0401);
}


void
ClearRealTimeClockAlarm(void)
{
	DisableInterrupt(gAlarmIntObj);
}


/*------------------------------------------------------------------------------
	Initialize our data structures.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CRealTimeClock::init(void)
{
	// zero out all available alarm slots
	for (ArrayIndex slot = 0; slot < kNumOfAlarms; slot++)
	{
		gAlarm[slot].fName = 0;
		gAlarm[slot].fIsEnabled = false;
	}

	fAlarmSet = false;
	fChangingTime = 0;
	fUpdatingAlarm = 0;
	fFixUp = 0;
	fAssigningName = 0;
	fNextName = 2;

	RegisterRealTimeClockHandler(NULL, interruptEntry);
}


/*------------------------------------------------------------------------------
	Set the real-time clock.
	Args:		inTime			the time in seconds
	Return:	--
------------------------------------------------------------------------------*/

void
CRealTimeClock::setRealTimeClock(ULong inTime)
{
	while (Swap(&fChangingTime, 1) != 0)
		;	// acquire lock

	ClearRealTimeClockAlarm();
	fAlarmSet = false;

	// keep writing the time to the RTC hardware until it’s set correctly
	ULong oldTime = GetRealTimeClock();
	while (inTime != GetRealTimeClock())
		g0F181000 = inTime;

	// adjust all alarms for the new real time
	for (ArrayIndex slot = 0; slot < kNumOfAlarms; slot++)
	{
		if (gAlarm[slot].fIsEnabled && gAlarm[slot].fIsRelative)
			gAlarm[slot].fTime += (inTime - oldTime);
	}

	fChangingTime = 0;	// release lock
	cleanUp();
}


/*------------------------------------------------------------------------------
	Return a new alarm name, and register it.
	Args:		outName			the (unique) name
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CRealTimeClock::newName(ULong * outName)
{
	NewtonErr	err;

	while (Swap(&fAssigningName, 1) == 0)	// sic
		;	// acquire lock

	*outName = fNextName++;

/*	Newt really does this
	for (ArrayIndex slot = 0; slot < kNumOfAlarms; slot++)
	{
		r3 = *outName;
		r12 = gAlarm[slot].name;
	}
*/
	if ((err = checkIn(*outName)) != noErr)
		*outName = 0;
	fAssigningName = 0;	// release the lock
	return err;
}


/*------------------------------------------------------------------------------
	Reserve a slot in the timer table for the caller.
	Args:		inName		the unique channel name
	Return:	error code	-2 => no slots free
------------------------------------------------------------------------------*/

NewtonErr
CRealTimeClock::checkIn(ULong inName)
{
	for (ArrayIndex slot = 0; slot < kNumOfAlarms; slot++)
	{
		if (gAlarm[slot].fName == 0)
		{
			gAlarm[slot].fName = inName;
			return noErr;
		}
	}
	return -2;
}


/*------------------------------------------------------------------------------
	Release a slot in the timer table for the caller.
	Args:		inName		the unique channel name
	Return:	error code	always noErr
------------------------------------------------------------------------------*/

NewtonErr
CRealTimeClock::checkOut(ULong inName)
{
	ArrayIndex slot = findSlot(inName);
	if (slot != kIndexNotFound)
	{
		gAlarm[slot].fName = 0;
		gAlarm[slot].fIsEnabled = false;
	}
	return noErr;
}


/*------------------------------------------------------------------------------
	Find a named slot in the timer table.
	Args:		inName		the unique channel name
	Return:	slot index	-1 => not found
------------------------------------------------------------------------------*/

ArrayIndex
CRealTimeClock::findSlot(ULong inName)
{
	for (ArrayIndex slot = 0; slot < kNumOfAlarms; slot++)
	{
		if (gAlarm[slot].fName == inName)
			return slot;
	}
	return kIndexNotFound;
}


/*------------------------------------------------------------------------------
	Set the alarm on a channel.
	If an alarm already exists on this channel, it will be discarded.
	Args:		inName			the channel name
				inTime			time to execute the alarm
				inPortId			port to which message should be sent
				inMsgId			message to send
				inData			data to accompany message
				inDataSize		size of that data
				inType			type of message
	Return:	error code	-1 => slot not found
------------------------------------------------------------------------------*/

NewtonErr
CRealTimeClock::setAlarm(ULong inName, CTime inTime, ObjectId inPortId, ObjectId inMsgId, void * inData, size_t inDataSize, ULong inType)
{
	ArrayIndex	slot = findSlot(inName);
	if (slot == kIndexNotFound)
		return -1;

	CRealTimeAlarm * rta = &gAlarm[slot];
	rta->init(inTime.convertTo(kSeconds), inPortId, inMsgId, inData, inDataSize, inType);
	if (primSetAlarm(rta->fTime) == false)
		cleanUp();
	return noErr;
}


/*------------------------------------------------------------------------------
	Set the alarm on a channel.
	If an alarm already exists on this channel, it will be discarded.
	It fires off an interrupt handler when the alarm fires, rather than a message.
	Args:		inName			the channel name
				inTime			time to execute the alarm
				inHandler		interrupt handler
				inData			data
				inWakeUp			should Newt wake up
				inRelative		is time relative
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CRealTimeClock::setAlarm(ULong inName, ULong inTime, NewtonInterruptHandler inHandler, void * inData, ULong inWakeUp, ULong inRelative)
{
	ArrayIndex	slot = findSlot(inName);
	if (slot == kIndexNotFound)
		return -1;

	CRealTimeAlarm * rta = &gAlarm[slot];
	rta->init(inTime, inHandler, inData, inWakeUp, inRelative, &fChangingTime);
	if (primSetAlarm(rta->fTime) == false)
		cleanUp();
	return noErr;
}


/*------------------------------------------------------------------------------
	Set the alarm.
	Args:		inTime			time in seconds to execute the alarm
	Return:	true if successful
------------------------------------------------------------------------------*/

bool
CRealTimeClock::primSetAlarm(ULong inTime)
{
	// alarm must be in the future
	if (inTime > GetRealTimeClock())
	{
		if (!fAlarmSet)
			return primRawSetAlarm(inTime);
		else
		{
			if (primRawSetAlarm(inTime))
				return true;
			cleanUp();
		}
	}
	return false;
}


/*------------------------------------------------------------------------------
	Set the alarm. Yes, really do it this time.
	Args:		inTime			time in seconds to execute the alarm
	Return:	true if successful
------------------------------------------------------------------------------*/

bool
CRealTimeClock::primRawSetAlarm(ULong inTime)
{
	bool wasSet = true;

	if (fAlarmSet && inTime < fCurrentAlarm)
	{
		SetRealTimeClockAlarm(inTime);
		if (inTime > GetRealTimeClock())
		{
			fCurrentAlarm = inTime;
			fAlarmSet = true;
		}
		else
			wasSet = false;
	}

	return wasSet;
}


/*------------------------------------------------------------------------------
	Clear any pending alarm on a channel.
	Args:		inName			the channel name
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CRealTimeClock::clearAlarm(ULong inName)
{
	ArrayIndex	slot = findSlot(inName);
	if (slot == kIndexNotFound)
		return -1;

	gAlarm[slot].fIsEnabled = false;
	cleanUp();
	return noErr;
}


/*------------------------------------------------------------------------------
	Provide status on a channel.
	Indicate if it is active, and if so when it is scheduled to go off.
	If it is not active, outError indicates the last error value.
	Error is initialized to noErr and is cleared by clearAlarm and setAlarm.
	Args:		inName			the channel name
				outActive		active result
				outAlarmTime	alarm time result
				outError			error result
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CRealTimeClock::alarmStatus(ULong inName, bool * outActive, CTime * outAlarmTime, long * outError)
{
	ArrayIndex	slot = findSlot(inName);
	if (slot == kIndexNotFound)
		return -1;

	*outActive = gAlarm[slot].fIsEnabled;
	if (*outActive)
	{
		*outAlarmTime = CTime(gAlarm[slot].fTime, kSeconds);
	}
	*outError = gAlarm[slot].fError;
	return noErr;
}


/*------------------------------------------------------------------------------
	Fire any alarms that are primed to go off now, but don’t wake the Newt.
	Args:		--
	Return:	true
------------------------------------------------------------------------------*/

bool
CRealTimeClock::checkAlarmsStaySleeping(void)
{
	ULong now = GetRealTimeClock();
	ULong	nextAlarmTime = 0;
	bool	gotATime;

	do
	{
		gotATime = false;
		for (int slot = 0; slot < kNumOfAlarms; slot++)
		{
			CRealTimeAlarm * rta = &gAlarm[slot];
			if (rta->fIsEnabled && rta->fTime < now)
			{
				if (rta->fIsWakeUp)
					return false;

				rta->fire(now);
				fAlarmSet = false;
			}
			if (rta->fIsEnabled)
			{
				if (gotATime)
				{
					if (rta->fTime < nextAlarmTime)
						nextAlarmTime = rta->fTime;
				}
				else
				{
					gotATime = true;
					nextAlarmTime = rta->fTime;
				}
			}
		}
		ClearRealTimeClockAlarm();
	} while (gotATime && primRawSetAlarm(nextAlarmTime) == false);

	return true;
}


/*------------------------------------------------------------------------------
	Fire any alarms that are primed to go off now.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CRealTimeClock::alarm(void)
{
	ULong	nextAlarmTime = 0;
	bool	gotATime;

	fFixUp = 1;
	do
	{
		if (Swap(&fUpdatingAlarm, 1) != 0)
			break;	// abandon if locked

		ClearRealTimeClockAlarm();
		GetRealTimeClock();	// sic
		fAlarmSet = false;

		do
		{
			gotATime = false;
			fFixUp = 0;
			// step thru the timer table
			for (ArrayIndex slot = 0; slot < kNumOfAlarms; ++slot)
			{
				ULong now = GetRealTimeClock();
				CRealTimeAlarm * rta = &gAlarm[slot];
				// try to fire the alarm
				rta->fire(now);
				// find the time of the next alarm
				if (rta->fIsEnabled)
				{
					if (gotATime)
					{
						if (rta->fTime < nextAlarmTime)
							nextAlarmTime = rta->fTime;
					}
					else
					{
						gotATime = true;
						nextAlarmTime = rta->fTime;
					}
				}
			}
		} while (fFixUp || (gotATime && primRawSetAlarm(nextAlarmTime) == false));

		fUpdatingAlarm = 0;
	} while (fFixUp);

	return noErr;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C U R e a l T i m e A l a r m

	User-mode access to the real-time clock.
------------------------------------------------------------------------------*/

struct RTCMessage
{
	ULong		selector;	// +00
	ULong		name;			// +04
	CTime		time;			// +08
	ObjectId	portId;		// +10
	ObjectId	msgId;		// +14
	ULong		type;			// +18
	void *	obj;			// +1C
	size_t	objSize;		// +20
	bool		isWakeUp;	// +24
	bool		isHandler;	// +28
	NewtonInterruptHandler	handler;	// +2C
	bool		isRelative;	// +30
};

struct RTCStatus
{
	ULong		selector;	// +00
	ULong		name;			// +04
	bool		isActive;	// +08
	CTime		alarmTime;	// +0C
	long		error;		// +14
};

//	Selector
enum
{
	kSetAlarmMessage,
	kSetAlarmHandler,
	kClearAlarm,
	kAlarmStatus,
	kCheckInAlarm,
	kNewAlarmName,
	kCheckOutAlarm,
	kAlarmTime,
	kSetAlarmTime
};


/*------------------------------------------------------------------------------
	Supervisor mode dispatcher to the real-time clock.
	Callback from SWI.
------------------------------------------------------------------------------*/

NewtonErr
RealTimeClockDispatch(void)
{
	NewtonErr	err;
	RTCMessage * msg = (RTCMessage *)TaskSwitchedGlobals()->fParams;

	switch (msg->selector)
	{
	case kSetAlarmMessage:
		err = CRealTimeClock::setAlarm(msg->name, msg->time, msg->portId, msg->msgId, msg->obj, msg->objSize, msg->type);
		break;

	case kSetAlarmHandler:
		err = CRealTimeClock::setAlarm(msg->name, msg->time, msg->handler, msg->obj, msg->isWakeUp, msg->isRelative);
		break;

	case kClearAlarm:
		err = CRealTimeClock::clearAlarm(msg->name);
		break;

	case kAlarmStatus:
		{
			RTCStatus * status = (RTCStatus *)msg;
			err = CRealTimeClock::alarmStatus(status->name, &status->isActive, &status->alarmTime, &status->error);
		}
		break;

	case kCheckInAlarm:
		err = CRealTimeClock::checkIn(msg->name);
		break;

	case kNewAlarmName:
		err = CRealTimeClock::newName(&msg->name);
		break;

	case kCheckOutAlarm:
		err = CRealTimeClock::checkOut(msg->name);
		break;

	case kAlarmTime:
		{
			CTime theTime(GetRealTimeClock(), kSeconds);
			msg->time = theTime;
		}
		err = noErr;
		break;

	case kSetAlarmTime:
		CRealTimeClock::setRealTimeClock(msg->time.convertTo(kSeconds));
		err = noErr;
		break;

	default:
		err = kOSErrBadParameters;
	}

	return err;
}


/*------------------------------------------------------------------------------
	Return a new alarm name, and register it.
	Args:		outName			the (unique) name
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CURealTimeAlarm::newName(ULong * outName)
{
	NewtonErr	err;
	RTCMessage * msg = (RTCMessage *)TaskSwitchedGlobals()->fParams;
	msg->selector = kNewAlarmName;

	if (IsSuperMode())
		err = RealTimeClockDispatch();
	else
		err = GenericSWI(kRealTimeClockDispatch);

	*outName = msg->name;
	return err;
}


/*------------------------------------------------------------------------------
	Initalize the CURealTimeAlarm object for a unique "channel" specified by the
	"inName" argument. This call reserves a slot in the timer table for the caller.
	Args:		inName		the unique channel name
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CURealTimeAlarm::checkIn(ULong inName)
{
	RTCMessage * msg = (RTCMessage *)TaskSwitchedGlobals()->fParams;
	msg->selector = kCheckInAlarm;
	msg->name = inName;

	if (IsSuperMode())
		return RealTimeClockDispatch();
	return GenericSWI(kRealTimeClockDispatch);
}


/*------------------------------------------------------------------------------
	Finish using the real time clock.
	Args:		inName		the channel name we’re finished with
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CURealTimeAlarm::checkOut(ULong inName)
{
	RTCMessage * msg = (RTCMessage *)TaskSwitchedGlobals()->fParams;
	msg->selector = kCheckOutAlarm;
	msg->name = inName;

	if (IsSuperMode())
		return RealTimeClockDispatch();
	return GenericSWI(kRealTimeClockDispatch);
}


/*------------------------------------------------------------------------------
	Set the alarm on a channel.
	If an alarm already exists on this channel, it will be discarded.
	Args:		inName			the channel name
				inTime			time to execute the alarm
				inPortId			port to which message should be sent
				inMsgId			message to send
				inData			data to accompany message
				inDataSize		size of that data
				inType			type of message
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CURealTimeAlarm::setAlarm(ULong inName, CTime inTime, ObjectId inPortId, ObjectId inMsgId, void * inData, size_t inDataSize, ULong inType)
{
	RTCMessage * msg = (RTCMessage *)TaskSwitchedGlobals()->fParams;
	msg->selector = kSetAlarmMessage;
	msg->name = inName;
	msg->time = inTime;
	msg->portId = inPortId;
	msg->msgId = inMsgId;
	msg->type = inType;
	msg->obj = inData;
	msg->objSize = inDataSize;

	if (IsSuperMode())
		return RealTimeClockDispatch();
	return GenericSWI(kRealTimeClockDispatch);
}


/*------------------------------------------------------------------------------
	Set the alarm on a channel.
	If an alarm already exists on this channel, it will be discarded.
	It fires off an interrupt handler when the alarm fires, rather than a message.
	Args:		inName			the channel name
				inTime			time to execute the alarm
				inHandler		interrupt handler
				inData			data
				inWakeUp			should Newt wake up
				inRelative		is time relative
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CURealTimeAlarm::setAlarm(ULong inName, CTime inTime, NewtonInterruptHandler inHandler, void * inData, bool inWakeUp, bool inRelative)
{
	RTCMessage * msg = (RTCMessage *)TaskSwitchedGlobals()->fParams;
	msg->selector = kSetAlarmHandler;
	msg->name = inName;
	msg->time = inTime;
	msg->obj = inData;
	msg->isWakeUp = inWakeUp;
	msg->handler = inHandler;
	msg->isRelative = inRelative;

	if (IsSuperMode())
		return RealTimeClockDispatch();
	return GenericSWI(kRealTimeClockDispatch);
}


/*------------------------------------------------------------------------------
	Clear any pending alarm on a channel.
	Args:		inName			the channel name
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CURealTimeAlarm::clearAlarm(ULong inName)
{
	RTCMessage * msg = (RTCMessage *)TaskSwitchedGlobals()->fParams;
	msg->selector = kClearAlarm;
	msg->name = inName;

	if (IsSuperMode())
		return RealTimeClockDispatch();
	return GenericSWI(kRealTimeClockDispatch);
}


/*------------------------------------------------------------------------------
	Provide status on a channel.
	Indicate if it is active, and if so when it is scheduled to go off.
	If it is not active, outError indicates the last error value.
	Error is initialized to noErr and is cleared by clearAlarm and setAlarm.
	Args:		inName			the channel name
				outActive		active result
				outAlarmTime	alarm time result
				outError			error result
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CURealTimeAlarm::alarmStatus(ULong inName, bool * outActive, CTime * outAlarmTime, long * outError)
{
	NewtonErr	err;
	RTCStatus * msg = (RTCStatus *)TaskSwitchedGlobals()->fParams;
	msg->selector = kAlarmStatus;

	if (IsSuperMode())
		err = RealTimeClockDispatch();
	else
		err = GenericSWI(kRealTimeClockDispatch);

	*outActive = msg->isActive;
	*outAlarmTime = msg->alarmTime;
	*outError = msg->error;
	return err;
}


/*------------------------------------------------------------------------------
	Return the current real time clock value.
	Args:		--
	Return:	the time
------------------------------------------------------------------------------*/

CTime
CURealTimeAlarm::time()
{
	RTCMessage * msg = (RTCMessage *)TaskSwitchedGlobals()->fParams;
	msg->selector = kAlarmTime;

	if (IsSuperMode())
		RealTimeClockDispatch();
	else
		GenericSWI(kRealTimeClockDispatch);

	return msg->time;
}


/*------------------------------------------------------------------------------
	Set the real-time clock.
	Args:		inTime			the time to set
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CURealTimeAlarm::setTime(CTime inTime)
{
	RTCMessage * msg = (RTCMessage *)TaskSwitchedGlobals()->fParams;
	msg->selector = kSetAlarmTime;
	msg->time = inTime;

	if (IsSuperMode())
		return RealTimeClockDispatch();
	return GenericSWI(kRealTimeClockDispatch);
}

#endif	/* !defined(forFramework) */
#pragma mark -

/*------------------------------------------------------------------------------
	C T i m e
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Set the CTime.
	Args:		inAmount		the number of…
				inUnits		…time units
	Return:	--
------------------------------------------------------------------------------*/

void
CTime::set(ULong inAmount, TimeUnits inUnits)
{
	fTime = inAmount;
	fTime *= inUnits;
}


/*------------------------------------------------------------------------------
	Convert the CTime into another time unit.
	Args:		inUnits		the required time units
	Return:	the time in those units
------------------------------------------------------------------------------*/

ULong
CTime::convertTo(TimeUnits inUnits)
{
	return fTime / inUnits;
}

#pragma mark -
/*------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e   t o   C T i m e
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Return the CTime.
	Args:		--
	Return:	the time
------------------------------------------------------------------------------*/

CTime
GetClock(void)
{
#if defined(correct)
	CTime now(gTimerSample);		// does this get updated at interrupt?
	// if counter has wrapped around since last read, adjust
	ULong	counter = g0F181800;
	if (counter <= now.fTime.lo)
		now.fTime.hi++;
	now.fTime.lo = counter;
	return now;

#else
	kern_return_t ret;
	clock_serv_t theClock;
	mach_timespec_t theTime;

	ret = host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &theClock);
	ret = clock_get_time(theClock, &theTime);

	CTime now(theTime.tv_sec, kSeconds);
	CTime plus(theTime.tv_nsec/1000, kMicroseconds);
	return now + plus;
#endif
}


/*------------------------------------------------------------------------------
	Return the current CTime.
	Args:		--
	Return:	time
------------------------------------------------------------------------------*/

CTime
GetGlobalTime(void)
{
#if defined(forFramework)
	return GetClock();

#else
	if (IsSuperMode())
		return GetClock();

	int64_t theTime;
	GenericWithReturnSWI(kGetGlobalTime, kNoId, 0, 0, (OpaqueRef *)&theTime, 0, 0);
	return CTime(theTime);
#endif
}


/*------------------------------------------------------------------------------
	Return the time spent in a task.
	Args:		inTaskId		the id of the task
	Return:	time
------------------------------------------------------------------------------*/

CTime
GetTaskTime(ObjectId inTaskId)
{
#if !defined(forFramework)
	int64_t theTime;
	GenericWithReturnSWI(kGetGlobalTime, inTaskId ? inTaskId : gCurrentTaskId, 0, 0, (OpaqueRef *)&theTime, 0, 0);
	return CTime(theTime);
#endif	/* !defined(forFramework) */
	return CTime(0);
}


/*------------------------------------------------------------------------------
	Return an absolute CTime in the future.
	Args:		inDelta		delay to future time
	Return:	time in the future
------------------------------------------------------------------------------*/

CTime
TimeFromNow(Timeout deltaTime)
{
	CTime now(GetGlobalTime());
	return now + CTime(deltaTime);
}

