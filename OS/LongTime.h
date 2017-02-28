/*
	File:		LongTime.h

	Contains:	The public interface for the real time clock.

	Written by:	Newton Research Group.
*/

#if !defined(__LONGTIME_H)
#define	__LONGTIME_H 1

#if !defined(__NEWTONTIME_H)
#include "NewtonTime.h"
#endif
#if !defined(__INTERRUPT_H)
#include "Interrupt.h"
#endif


/*--------------------------------------------------------------------------------
	C U R e a l T i m e A l a rm
--------------------------------------------------------------------------------*/

class CURealTimeAlarm
{
public:
	// Initalize the CURealTimeAlarm object for a unique "channel" specified by the
	// inName argument. This call reserves a slot in the timer table for the caller.
	static NewtonErr  checkIn(ULong inName);

	// Finish using the real time clock.
	static NewtonErr	checkOut(ULong inName);

	// Set the alarm on a channel.
	// If an alarm already exists on this channel, it will be discarded
	static NewtonErr	setAlarm(ULong inName, CTime inTime, ObjectId inPortId, ObjectId inMsgId, void * inObj, size_t inObjSize, ULong inType);

	// Set the alarm on a channel.
	// If an alarm already exists on this channel, it will be discarded.
	// It fires off an interrupt handler when the alarm fires, rather than a message.
	static NewtonErr	setAlarm(ULong inName, CTime inTime, NewtonInterruptHandler inDirectHandler, void * inObj, bool inWakeUp = true, bool inRelative = false);

	// Clear any pending alarm on a channel.
	static NewtonErr	clearAlarm(ULong inName);

	// Provide status on a channel.
	// Indicate if it is active, and if so when it is scheduled to go off.
	// If it is not active, outError indicates the last error value.
	// Error is initialized to noErr and is cleared by clearAlarm and setAlarm.
	static NewtonErr	alarmStatus(ULong inName, bool * outActive, CTime * outAlarmTime, long * outError);

	// Return the current real time clock value.
	static CTime	time();

	// Set the real time clock.
	static NewtonErr	setTime(CTime inTime);

	// Return a new alarm name, and register it.
	static NewtonErr  newName(ULong * ioName);
};

#endif	/* __LONGTIME_H */
