/*
	File:		Timers.h

	Contains:	Timer class definitions.

	Written by:	Newton Research Group.
*/

#if !defined(__TIMERS_H)
#define __TIMERS_H 1

#include "SharedMem.h"
#include "Interrupt.h"

/*--------------------------------------------------------------------------------
	C T i m e r E n g i n e
--------------------------------------------------------------------------------*/

class CTimerEngine : public CDoubleQContainer
{
public:
				CTimerEngine();

	void		init(void);
	void		start(void);
	void		alarm(void);
	bool		queueTimer(CSharedMemMsg * inMsg, Timeout inTimeout, void * inData, TimeoutProcPtr inProc);
	bool		queueTimeout(CSharedMemMsg * inMsg);
	bool		queueDelay(CSharedMemMsg * inMsg);
	bool		queue(CSharedMemMsg * inMsg);
	void		remove(CSharedMemMsg * inMsg);
};


/*----------------------------------------------------------------------
	C R e a l T i m e C l o c k
----------------------------------------------------------------------*/

class CRealTimeClock
{
public:
	static void			init(void);
	static void			setRealTimeClock(ULong inTime);

	static NewtonErr	newName(ULong * outName);
	static NewtonErr	checkIn(ULong inName);
	static NewtonErr	checkOut(ULong inName);
	static ArrayIndex	findSlot(ULong inName);
//	static NewtonErr	register(ULong inName)		{ return checkIn(inName); }	// reserved word name clash
//	static NewtonErr	deregister(ULong inName)	{ return checkOut(inName); }

	static NewtonErr	setAlarm(ULong inName, CTime inTime, ObjectId inPortId, ObjectId inMsgId, void * inObj, size_t inObjSize, ULong inType);
	static NewtonErr	setAlarm(ULong inName, ULong inTime, NewtonInterruptHandler inHandler, void * inObj, ULong inWakeUp = true, ULong inRelative = false);
	static bool			primSetAlarm(ULong inTime);
	static bool			primRawSetAlarm(ULong inTime);
	static NewtonErr	clearAlarm(ULong inName);
	static NewtonErr	alarmStatus(ULong inName, bool * outActive, CTime * outAlarmTime, long * outError);
	static NewtonErr	alarm(void);

	static bool			checkAlarmsStaySleeping(void);
	static bool			sleepingCheckFire(void)	{ checkAlarmsStaySleeping(); }
	static NewtonErr	interruptEntry(void*)	{ return alarm(); }
	static void			cleanUp(void)				{ alarm(); }

	static bool			fAlarmSet;				// is active
	static ULong		fCurrentAlarm;			// when (seconds)
	static ULong		fChangingTime;			// lock
	static ULong		fUpdatingAlarm;		// lock
	static ULong		fFixUp;
	static ULong		fAssigningName;		// lock
	static ULong		fNextName;
	static long			x20;
};


/*----------------------------------------------------------------------
	C R e a l T i m e A l a r m
----------------------------------------------------------------------*/

class CRealTimeAlarm
{
public:
	void	init(ULong inTime, ObjectId inPortId, ObjectId inMsgId, void * inObj, long inObjSize, ULong inType);
	void	init(ULong inTime, NewtonInterruptHandler inDirectHandler, void * inObj, ULong inWakeUp, ULong inRelative, ULong * ioLock);
	void	fire(ULong inTime);

	ULong			fName;				// +00
	ULong			fTime;				// +04 alarm time (seconds)
	ObjectId		fPortId;				// +08
	ObjectId		fMsgId;				// +0C
	ULong			fType;				// +10
	void *		fData;				// +14
	long			fDataSize;			// +18
	long			fError;				// +1C
	ULong			fIsEnabled;			// +20	bool actually, but passed to Swap()
	ULong			fIsWakeUp;			// +24
	bool			fIsHandler;			// +28
	NewtonInterruptHandler	fHandler;	// +2C
	ULong			fIsRelative;		// +30
};


#endif	/* __TIMERS_H */
