/*
	File:		NewtonTime.h

	Contains:	Timing gear for Newton build system.

	Written by:	Newton Research Group.
	
	To do:		remove ticks

*/

#if !defined(__NEWTONTIME_H)
#define __NEWTONTIME_H 1

#if !defined(__NEWTONTYPES_H)
#include "NewtonTypes.h"
#endif

#if !defined(__KERNELTYPES_H)
#include "KernelTypes.h"
#endif


/*--------------------------------------------------------------------------------
	NOTE
	The maximum time that can be represented by a Timeout is 14 minutes
	(assuming 200ns clock).
	If you need to compute a future time that is a greater amount into the future,
	you must use a CTime and apply your own conversion beyond minutes.
	For example, if you wanted to get the time for one day you would:
		CTime oneDay(1*24*60, kMinutes);
	(1 day, 24 hours in a day, 60 minutes in an hour)
--------------------------------------------------------------------------------*/

typedef uint32_t	Timeout;
typedef uint32_t	HardwareTimeUnits;	// used only by the tablet driver

enum
{
	kNoTimeout = 0,
	kTimeOutImmediate = ~0
};


/*--------------------------------------------------------------------------------
	Use TimeUnits enumeration to construct relative time values
	These enumerations would be the second argument to the CTime
	constructor that takes amount and units. For example a relative
	CTime object that is 10 seconds in length could be constructed as:

		CTime(10, kSeconds)

	Note that the send delayed call in ports takes an ABSOLUTE CTime
	object. Such an object is created by getting the absolute time
	from GetGlobalTime() + delta.
--------------------------------------------------------------------------------*/

#if defined(hasCirrus)
/* New chipset has a different time base than old. We change it here. */

enum TimeUnits
{
	kSystemTimeUnits	=       1,			// to convert to (amount, units)
	kMicroseconds		=       4,			// clock ticks in a microsecond (3.6864MHz clk)
	kMilliseconds		=    3686,
	kMacTicks			=   61440,			// for compatibility
	kSeconds				= 3686400,
	kMinutes				=      60 * kSeconds
};

#elif	defined(hasRunt)
/* use 200ns clock */

enum TimeUnits
{
	kSystemTimeUnits	=     1,
	kMicroseconds		=     5,							// number of units in a microsecond
	kMilliseconds		= (1000*kMicroseconds),
	kSeconds				= (1000*kMilliseconds),
	kMinutes				=   (60*kSeconds)
};

#else
/* use microseconds */

enum TimeUnits
{
	kSystemTimeUnits	=     1,							// to convert to (amount, units)
	kMicroseconds		=     1,							// number of units in a microsecond (sic)
	kMilliseconds		= (1000*kMicroseconds),
	kSeconds				= (1000*kMilliseconds),
	kMinutes				=   (60*kSeconds)
};

#endif

unsigned long	RealClock(void);						// gets minutes based realtime clock
unsigned long	RealClockSeconds(void);				// gets seconds based realtime clock
void		SetRealClock(unsigned long minutes);		// sets realtime clock minutes - since 1/1/04 12:00am
void		SetRealClockSeconds(unsigned long seconds);	// sets realtime clock seconds - since 1/1/04 12:00am


#ifdef __cplusplus
/*--------------------------------------------------------------------------------
	C T i m e

	CTimes are used to represent moments in time.
	For instance, if you want to send a message a day from now, you find now
	and add in one day.
--------------------------------------------------------------------------------*/

class CTime
{
public:
			CTime()								{ }
			CTime(const CTime & x)			{ fTime = x.fTime; };
			CTime(ULong amount, TimeUnits units) { set(amount, units); }

			CTime(int64_t time)					{ fTime = time; }
			operator	int64_t() const			{ return fTime; }

	void	set(ULong amount, TimeUnits units);
	ULong	convertTo(TimeUnits units);

	CTime& operator=(int64_t b)		{ this->fTime = b; return *this; }
	CTime& operator+(const CTime& b)	{ this->fTime += b.fTime; return *this; }
	CTime& operator-(const CTime& b)	{ this->fTime -= b.fTime; return *this; }

	bool	operator== (const CTime& b) const	{ return fTime == b.fTime; }
	bool	operator!= (const CTime& b) const	{ return fTime != b.fTime; }
	bool	operator>  (const CTime& b) const	{ return fTime >  b.fTime; }
	bool	operator>= (const CTime& b) const	{ return fTime >= b.fTime; }
	bool	operator<  (const CTime& b) const	{ return fTime <  b.fTime; }
	bool	operator<= (const CTime& b) const	{ return fTime <= b.fTime; }

private:
	int64_t	fTime;
};


// GetGlobalTime -
// returns a CTime moment representing the current time.
CTime	GetGlobalTime(void);

// GetTaskTime -
// returns a CTime duration representing the time spent in the specified task.
CTime	GetTaskTime(ObjectId inTaskId = 0);

// TimeFromNow -
// returns a CTime moment representing a time in the future.
// NOTE: You can only specify a moment up to 14 minutes into the future
// using this method.  If you need a longer distance into the future,
// simply add the future CTime duration to the current time from GetGlobalTime.
CTime	TimeFromNow(Timeout inDeltaTime);

void	Wait(ULong inMilliseconds);
void	Sleep(Timeout inTimeout);
void	SleepTill(CTime * inFutureTime);

#endif	/* __cplusplus */

#endif	/* __NEWTONTIME_H */
