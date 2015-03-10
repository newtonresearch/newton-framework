/*
	File:		Dates.h

	Contains:	Dates and times.

	Written by:	Newton Research Group, 2009.
*/

#if !defined(__DATES_H)
#define __DATES_H 1

#include "Objects.h"

/* -----------------------------------------------------------------------------
	S e l e c t o r s
----------------------------------------------------------------------------- */

// date/time elements
enum
{
	kIncludeAllElements,
	kElementNothing = 0,

// short date elements
	kElementDay = 1,
	kElementDayOfWeek,
	kElementMonth,
	kElementYear,
	kElementEra,

// time format elements
	kElementHour = 1,
	kElementMinute,
	kElementSecond,
	kElementAMPM,
	kElementSuffix
};

// formats
enum
{
	kFormatDefault,
	kFormatLong,
	kFormatAbbr,
	kFormatTerse,
	kFormatShort,
	kFormatNumeric
};

// long date/time formats
enum
{
	kFormatLongDate = 1,
	kFormatAbbrDate,
	kFormatNumericDate,
	kFormatNumericYear,
	kFormatLongMonth,
	kFormatAbbrMonth,
	kFormatNumericDay,
	kFormatLongDayOfWeek,
	kFormatAbbrDayOfWeek,

	kFormatLongTime = 21,
	kFormatShortTime,
	kFormatHour,
	kFormatMinute,
	kFormatSecond
};

// time formats
enum { kLeadZero, kNoLeadZero };
enum { kCycle24, kCycle12 };
enum { kUseHourZero, kUseHour12, kUseHour24 };
enum { kShowCentury, kNoCentury };


/* -----------------------------------------------------------------------------
	C D a t e
----------------------------------------------------------------------------- */

class CDate
{
public:
				CDate();
				CDate(ULong inMinsSince1904);
				CDate(const UniChar * inStr, size_t * outStrParsedLen, size_t inStrLen);

	void		initWithMinutes(ULong inMinsSince1904);
	void		initWithSeconds(ULong inSecsSince1993);
	void		initWithDateFrame(RefArg inDateFrame, bool inMakeValidDefaults);
	void		setCurrentTime(void);
	bool		isValidDate(void) const;

	int		stringToDate(const UniChar * inStr, size_t * outStrParsedLen, size_t inStrLen);
	int		stringToTime(const UniChar * inStr, size_t * outStrParsedLen, size_t inStrLen);
	int		stringToDateFields(const UniChar * inStr, size_t * outStrParsedLen, size_t inStrLen);
	int		stringToDateFrame(const UniChar * inStr, size_t * outStrParsedLen, size_t inStrLen);

	void		cleanUpFields(void);

	void		dateElementString(ULong inType, ULong inFormat, UniChar * outStr, size_t inStrSize, bool inLongFormat);
	void		longDateString(ULong inStrSpec, UniChar * outStr, size_t inStrSize);
	void		shortDateString(ULong inStrSpec, UniChar * outStr, size_t inStrSize);
	void		timeString(ULong inStrSpec, UniChar * outStr, size_t inStrSize);

	void		incrementMonth(int inOffset);

	ULong		daysInMonth(void) const;
	ULong		daysInYear(void) const;
	ULong		totalDays(void) const;
	ULong		totalHours(void) const;
	ULong		totalMinutes(void) const;
	ULong		totalSeconds(void) const;

	void		setFormatResource(RefArg inLocale);
	void		setFormatResource(RefArg inLongDateFormat, RefArg inShortDateFormat, RefArg inTimeFormat);

private:
	friend class CParseBuffer;
	friend Ref ToObject(const CDate & inDate);


	ULong			fYear;		// +00
	ULong			fMonth;		// +04
	ULong			fDay;			// +08
	ULong			fHour;		// +0C
	ULong			fMinute;		// +10
	ULong			fSecond;		// +14
	ULong			fDayOfWeek;	// +18
	RefStruct	fLongDateFormat;		// +1C
	RefStruct	fShortDateFormat;		// +20
	RefStruct	fTimeFormat;			// +24
};



int			DaylightSavingsOffset(void);
int			GMTOffset(void);


#endif	/* __DATES_H */
