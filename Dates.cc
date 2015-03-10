/*
	File:		Dates.cc

	Contains:	Dates and times.

	Written by:	Newton Research Group, 2009.
*/

#include "Objects.h"
#include "Dates.h"
#include "Locales.h"
#include "Dictionaries.h"
#include "ROMResources.h"
#include "UStringUtils.h"
#include "Preference.h"
#include "LongTime.h"
#include <time.h>			// for time()


extern "C" {

Ref	FTime(RefArg inRcvr);
Ref	FTimeInSeconds(RefArg inRcvr);
Ref	FSetTimeInSeconds(RefArg inRcvr, RefArg inSecsSince1993);
Ref	FSleep(RefArg inRcvr, RefArg inMilliseconds);

Ref	FIsValidDate(RefArg inRcvr, RefArg inDate);
Ref	FDateNTime(RefArg inRcvr, RefArg inMinsSince1904);
Ref	FHourMinute(RefArg inRcvr, RefArg inMinsSince1904);
Ref	FTimeStr(RefArg inRcvr, RefArg inMinsSince1904, RefArg inTimeStrSpec);
Ref	FTimeFrameStr(RefArg inRcvr, RefArg inDateFrame, RefArg inTimeStrSpec);
Ref	FShortDate(RefArg inRcvr, RefArg inMinsSince1904);
Ref	FShortDateStr(RefArg inRcvr, RefArg inMinsSince1904, RefArg inDateStrSpec);
Ref	FLongDateStr(RefArg inRcvr, RefArg inMinsSince1904, RefArg inDateStrSpec);
Ref	FStringToDate(RefArg inRcvr, RefArg inStr);
Ref	FStringToDateFrame(RefArg inRcvr, RefArg inStr);
Ref	FStringToTime(RefArg inRcvr, RefArg inStr);
Ref	FDate(RefArg inRcvr, RefArg inMinsSince1904);
Ref	FDateFromSeconds(RefArg inRcvr, RefArg inSecsSince1993);
Ref	FTotalMinutes(RefArg inRcvr, RefArg inDateFrame);
Ref	FIncrementMonth(RefArg inRcvr, RefArg inMinsSince1904, RefArg inNumOfMonths);
}

// the realtime clock counts seconds since 1/1/1904, same as the minutes time base
// the NewtonScript world, having only signed 30-bit integers, would run out of seconds too soon
// so it uses a time base of 1/1/1993
// however...
// NS numbers will still run out on 5/1/2010, so...
// we could adjust the seconds time base, to, say, 2003, giving us another ten years grace
// sadly, C’s ULong seconds will run out in 2040 anyway

#if defined(Fix2010)
#define kSecsSince1904 0xBA37E000
#else
#define kSecsSince1904 0xA7693A00
#endif

// bit fields in element descriptor ULong
#define kElementTypeWidth 3
#define kElementFormatWidth 3
#define kElementTypeMask ((1 << kElementTypeWidth) - 1)
#define kElementFormatMask ((1 << kElementFormatWidth) - 1)


typedef UniChar UStr31[32];	// element str
#define kStr31Len 31
typedef UniChar UStr63[64];	// date/time str
#define kStr63Len 63

ULong	GetNextElementType(ULong * ioStrSpec);
ULong	GetNextElementFormat(ULong * ioStrSpec);

/*	date/time string parsing status codes:
	0 => success
	2 => full string not used
  -1 => nothing parsed
*/


/*------------------------------------------------------------------------------
	S t r i n g   P a r s i n g
------------------------------------------------------------------------------*/
#define XXFAILIF(expr, action)		if (expr) { action goto failed; }

enum NumberType { kNumberType0 };
enum SignType { kNegativeSign, kPositiveSign };

class CNumberParser
{
public:
				CNumberParser();

	void		reset(void);
	double	stringToNumber(const UniChar * inStr, size_t * outParsedStrLen, size_t inStrLen);
	void		setInteger(const char * inStr);
	void		setDecimal(const char * inStr);
	void		setPrefix(const char * inStr);
	void		setSuffix(const char * inStr);
	void		setSign(SignType inSign);
	void		setNumberType(NumberType inType);

private:
	double	f00;
	double	f08;
	long		f10;
	char		fPrefix[4];	// +14
	char		fSuffix[4];	// +18
	NumberType	fType;	// +1C
	SignType		fSign;	// +20
	long		f24;
//size+28
};


double
DecimalStrToDouble(const char * inStr)
{
	double theNumber = 0.0;
	if (inStr != NULL)
	{
		int i;
		for (i = strlen(inStr) - 1; inStr[i] == '0' && i >= 0; i--)
			;
		for ( ; i >= 0; i--)
			theNumber = theNumber * 10.0 + (inStr[i] - '0');
	}
	return theNumber;
}


CNumberParser::CNumberParser()
{
	reset();
}


void
CNumberParser::reset(void)
{
	f00 = 0.0;
	f10 = 0;
	fPrefix[0] = 0;
	fSuffix[0] = 0;
	setNumberType(kNumberType0);
	setSign(kPositiveSign);
	f24 = 0;
}


double
CNumberParser::stringToNumber(const UniChar * inStr, size_t * outParsedStrLen, size_t inStrLen)
{
	NewtonErr err;
	double theNumber = 0.0;
	*outParsedStrLen = 0;
	if (inStr != NULL && inStrLen > 0)
	{
		reset();
		err = gNumberLexDictionary->parseString(this, inStr, outParsedStrLen, inStrLen);
		if (err != -1)
		{
			// INCOMPLETE -- do something with the FPU
			if (fSign == kNegativeSign)
				theNumber = -theNumber;
		}
	}
	return theNumber;
}


void
CNumberParser::setInteger(const char * inStr)
{
	if (inStr != NULL)
	{
		char * end;
		f00 = strtod(inStr, &end);
	}
}


void
CNumberParser::setDecimal(const char * inStr)
{
	if (inStr != NULL)
	{
		f10 = strlen(inStr);
		f08 = DecimalStrToDouble(inStr);
	}
}


void
CNumberParser::setPrefix(const char * inStr)
{
	strncpy(fPrefix, inStr, 3);
}


void
CNumberParser::setSuffix(const char * inStr)
{
	strncpy(fSuffix, inStr, 3);
}


void
CNumberParser::setSign(SignType inSign)
{
	fSign = inSign;
}


void
CNumberParser::setNumberType(NumberType inType)
{
	fType = inType;
}

#pragma mark -

#define kParseBufLen 64
#define kParseBufSize (kParseBufLen - 1)

class CParseBuffer
{
public:
	void		init(void);
	bool		convert(void * inContext);

	UChar		buf[kParseBufLen];	// +00
	UChar		bufLen;					// +40
	UChar		contentType;			// +41
};

/*
	contentType:
	 5		year := atol(buf)
	 6		month := atol(buf)
	 7		day := atol(buf)
	 9		hour := atol(buf)
	10		minute := atol(buf)
	11		second := atol(buf)
	12		make am
	13		make pm
	14		set integer
	15		set decimal
	16		set sign
	21..32	month <= contentType
	39		year, month, day := current
	40..46	dayOfWeek <= contentType
	51		set prefix
	52		set suffix
*/


void
CParseBuffer::init(void)
{
	buf[0] = 0;
	bufLen = 0;
	contentType = 0;
}


bool
CParseBuffer::convert(void * inContext)
{
	CDate * date = (CDate *)inContext;
	bool status = YES;

	if (bufLen > 0
	||  contentType != 0)
	{
		long numericValue;
		buf[bufLen] = 0;
		numericValue = atol((const char *)buf);
		switch (contentType)
		{
		case 5:
			if (numericValue <= 99
			|| (numericValue >= 1904 && numericValue <= 2920))
				date->fYear = numericValue;
			else
				status = NO;
			break;

		case 6:
			if (numericValue <= 12)
				date->fMonth = numericValue;
			else
				status = NO;
			break;

		case 7:
			if (numericValue <= 31)
				date->fDay = numericValue;
			else
				status = NO;
			break;

		case 9:
			if (numericValue <= 24)
				date->fHour = numericValue;
			else
				status = NO;
			break;

		case 10:
			if (numericValue <= 60)
				date->fMinute = numericValue;
			else
				status = NO;
			break;

		case 11:
			if (numericValue <= 60)
				date->fSecond = numericValue;
			else
				status = NO;
			break;

		case 12:
			if (date->fHour == 12)
				date->fHour = 0;
			break;

		case 13:
			if (date->fHour < 12)
				date->fHour += 12;
			break;

		case 14:
			((CNumberParser *)inContext)->setInteger((const char *)buf);
			break;

		case 15:
			((CNumberParser *)inContext)->setDecimal((const char *)buf);
			break;

		case 16:
			((CNumberParser *)inContext)->setSign(kNegativeSign);
			break;

		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		case 32:
			date->fMonth = contentType - 20;
			break;

		case 39:
			{
				CDate now;
				now.setCurrentTime();
				date->fYear = now.fYear;
				date->fMonth = now.fMonth;
				date->fDay = now.fDay + 1;
			}
			break;

		case 40:
		case 41:
		case 42:
		case 43:
		case 44:
		case 45:
		case 46:
			date->fDayOfWeek = contentType - 40;
			break;

		case 51:
			((CNumberParser *)inContext)->setPrefix((const char *)buf);
			break;

		case 52:
			((CNumberParser *)inContext)->setSuffix((const char *)buf);
			break;
		}
	}

	return status;
}

#pragma mark -

// from Carbon’s TextUtils.h
struct NBreakTable
{
	char		flags1;
	char		flags2;
	short		version;
	short		classTableOff;
	short		auxCTableOff;
	short		backwdTableOff;
	short		forwdTableOff;
	short		doBackup;
	short		length;		/* length of NBreakTable */
	char		charTypes[256];
	short		tables[1];
};
typedef struct NBreakTable		NBreakTable;
typedef NBreakTable *			NBreakTablePtr;


void
FindWordBreaks(const UniChar * inStr, size_t inStrLen, ULong inOffset, bool inLeadingEdge, RefArg inBreakTable, ULong * outStartOffset, ULong * outEndOffset)
{
	if (inStr != NULL && inStrLen > 0)
	{
		CDataPtr breakTable(inBreakTable);
		UByte * breakTablePtr = (UByte *)(char *)breakTable;	// r0
		const UniChar * wordStart;		// r5
		const UniChar * wordEnd;		// r10
		const UniChar * strEnd = inStr + inStrLen;
		int offset = inOffset;	// r6
		// if not the leading edge, start with the previous char
		if (!inLeadingEdge)
			offset--;
		// trivial rejection of starting offset
		if (offset < 0)
		{
			wordStart = inStr;
			wordEnd = inStr;
		}
		else if (offset >= inStrLen)
		{
			wordStart = strEnd;
			wordEnd = strEnd;
		}
		else
		{
			// gotta actually find a word
			UByte * classTable = breakTablePtr + ((NBreakTablePtr)breakTablePtr)->classTableOff;	// spm0C
			UByte * backwdTable = breakTablePtr + ((NBreakTablePtr)breakTablePtr)->backwdTableOff;	// spm08
			UByte * forwdTable = breakTablePtr + ((NBreakTablePtr)breakTablePtr)->forwdTableOff;	// spm04
			const UniChar * probe = inStr + offset;	// spm10
			const UniChar * p;	// r6
			UByte state;
			//sp-08
			UChar ch;
			if (offset >= 0x8000
			||  offset >= ((NBreakTablePtr)breakTablePtr)->doBackup)
			{
				// look backwards for start of word
				state = 2;	// starting state -- index into state table (of short offsets)
				for (p = probe; p > inStr; p--)
				{
					ch = A_CONST_CHAR(*p);		// was ConvertFromUnicode(p, cbuf, kMacRomanEncoding, 1);
					int charClassOffset = classTable[ch];
					int stateRowOffset = *((UShort *)backwdTable + (state>>1));
					state = backwdTable[stateRowOffset + charClassOffset];
					if ((state & 0x80) != 0)
					{
						// current offset is marked
						wordStart = p;
						state &= 0x7F;
					}
					if (state == 0)
						// exit code
						break;
				}
				if (p == inStr)
					wordStart = inStr;
			}
			else
				wordStart = inStr;

			// look forwards for end of word
			const UniChar * mark, * here;
			state = 2;
			for (p = wordStart; p <= strEnd; )
			{
				here = p;
				if (p >= strEnd)
					ch = 0;
				else
					ch = A_CONST_CHAR(*p);	// was ConvertFromUnicode(p, cbuf, kMacRomanEncoding, 1);
				p++;

				int charClassOffset = classTable[ch];
				if (p > strEnd)
					charClassOffset = 0;
				int stateRowOffset = *((UShort *)forwdTable + (state>>1));
				state = forwdTable[stateRowOffset + charClassOffset];
				if ((state & 0x80) != 0)
				{
					// current offset is marked
					mark = here;
					state &= 0x7F;
				}
				if (state == 0)
				{
					// exit code
					if (mark <= probe)
					{
						// haven’t yet reached the initially specified character
						// reset the start and keep looking for the end
						wordStart = mark;
						p = mark;
						state = 2;
					}
					else
					{
						// we’re done
						wordEnd = mark;
						break;
					}
				}
				else if (p > strEnd)
				{
					// reached end of string without encountering word end
					// stop now
					wordEnd = mark;
					break;
				}
			}
		}
		*outStartOffset = wordStart - inStr;
		*outEndOffset = wordEnd - inStr;
	}
	
	else	// not a valid string
	{
		*outStartOffset = 0;
		*outEndOffset = 0;
	}
}


NewtonErr
CDictionary::findLongestWord(UChar * outWord, const UniChar * inStr, size_t inStrLen, size_t * outWordLen)
{
	NewtonErr err = -1;
	UChar * sp18;
	ULong * sp14 = NULL;
	ULong * sp10 = NULL;
	UChar * sp0C = NULL;
	RefVar lineBreaks(GetLocaleSlot(SYMA(lineBreakTable)));	// sp08
	ULong lineStart, lineEnd;	// sp04, sp00
	size_t wordLen = inStrLen;	// r4
	do
	{
		// find extent of line
		FindWordBreaks(inStr, wordLen, wordLen-1, YES, lineBreaks, &lineStart, &lineEnd);

		UChar saveChar = outWord[lineEnd];
		outWord[lineEnd] = 0;
		verifyStart();
		verifyWord(outWord, &sp18, &sp14, &sp10, YES, &sp0C);
		outWord[lineEnd] = saveChar;
		if (airusResult == 2 || airusResult == 3)
		{
			CDictionary * dict = positionToHandle(*sp14);
			err = dict->fId;	// huh?
			break;
		}
	}
	while ((wordLen = lineStart) > 0);	// not sure about this
	*outWordLen = wordLen;
	return err;
}


NewtonErr
CDictionary::parseString(void * inContext, const UniChar * inStr, size_t * outParsedStrLen, size_t inStrLen)
{
//	r4 = inDict
//	r5 = inContext -- will be CDate* or CNumberParser*
//	r6 = inStr
//	r10 = -1
//	r7 = 0
//	r8 = inStrLen

	NewtonErr err = noErr;
	size_t parsedLen;

	XTRY
	{
		// convert/copy Unicode into C string buffer
		size_t strLen = Ustrlen(inStr);
		if (strLen > inStrLen)
			strLen = inStrLen;
		UChar * s = (UChar *)NewPtr(strLen + 1);
		XFAILIF(s == NULL, err = -1;)
		ConvertFromUnicode(inStr, s, strLen, kMacRomanEncoding);


		size_t wordLen;
		err = findLongestWord(s, inStr, strLen, &wordLen);
		if (wordLen > 0)
		{
			CParseBuffer parser;
			UChar * sp54;
			ULong * sp50 = NULL;
			ULong * sp4C = NULL;
			UChar * sp48 = (UChar *)1;
			bool sp44 = YES;
			parser.init();
			for (ArrayIndex i = 0; i < wordLen; ++i)
			{
				UChar saveChar = s[i+1];
				s[i+1] = 0;
				verifyStart();
				verifyWord(s, &sp54, &sp50, &sp4C, sp44, &sp48);
				s[i+1] = saveChar;
				XXFAILIF(sp4C == NULL, err = -1;)
				UChar r0 = *sp4C;
				UChar r9 = r0 & 0xC0;
				UChar r7 = r0 & 0x3F;
				if (r9 != 0)
				{
					if (r9 == 0x80
					||  r9 == 0xC0)
					{
						if (r9 == 0x80 && r7 != 0)
							parser.contentType = r7;
						XXFAILIF(parser.convert(inContext) == NO, err = -1;)
						parser.init();
					}
					if (r9 == 0x40
					||  r9 == 0xC0)
					{
						if (parser.bufLen < kParseBufSize)
						{
							parser.buf[parser.bufLen++] = s[i];
							parser.buf[parser.bufLen++] = 0;
							if (r7 != 0)
							{
								parser.contentType = r7;
								parser.bufLen = 0;
								err = -1;
							}
						}
					}
				}
				else if (r7 != 0)
				{
					parser.contentType = r7;
					parser.bufLen = 0;
					err = -1;
				}
			}
			if (parser.bufLen > 0)
				XFAILIF(parser.convert(inContext) == NO, err = -1;)
		}
failed:
		FreePtr((Ptr)s);
	}
	XENDTRY;

	XDOFAIL(err == -1)
	{
		parsedLen = 0;
	}
	XENDFAIL;

	*outParsedStrLen = parsedLen;
	return err;
}

#pragma mark -

/*------------------------------------------------------------------------------
	D a t e
------------------------------------------------------------------------------*/

unsigned long	RealClock(void);


CDate::CDate()
{
	fDayOfWeek = 0;
	fSecond = 0;
	fMinute = 0;
	fHour = 0;
	fDay = 0;
	fMonth = 0;
	fYear = 0;
}


CDate::CDate(ULong inMinsSince1904)
{
	initWithMinutes(inMinsSince1904);
}


void
CDate::initWithMinutes(ULong inMinsSince1904)
{
	div_t day = div(inMinsSince1904, 24*60);	// quot => numOfDays,	rem => minInDay
	div_t year = div(day.quot * 4, 1461);		// quot => numOfYears,	rem => dayInYear

	ULong theYear = 1904 + year.quot;
	ULong theDay = year.rem / 4;

	ULong firstMonth = 1;
	ULong startOfMarch = (theYear & 0x03) == 0 ? 60 : 59;		// is this test really good enough?
	if (theDay >= startOfMarch)
	{
		firstMonth = 3;
		theDay -= startOfMarch;
	}

	div_t month = div(theDay * 128 + 71, 3919);	
	fYear = theYear;
	fMonth = firstMonth + month.quot;
	fDay = 1 + month.rem / 128;

	div_t hour = div(day.rem, 60);
	fHour = hour.quot;
	fMinute = hour.rem;
	fSecond = 0;

	fDayOfWeek = day.quot % 7;
}


void
CDate::initWithSeconds(ULong inSecsSince1904)
{
	div_t minutes = div(inSecsSince1904, 60);
	initWithMinutes(minutes.quot);
	fSecond = minutes.rem;
}


void
CDate::initWithDateFrame(RefArg inDateFrame, bool inMakeValidDefaults)
{
	if (NOTNIL(inDateFrame))
	{
		RefVar rVal;

		rVal = GetFrameSlot(inDateFrame, SYMA(year));
		if (ISINT(rVal))
			fYear = RINT(rVal);
		else
			fYear = inMakeValidDefaults ? 1904 : 0xFFFFFFFF;

		rVal = GetFrameSlot(inDateFrame, SYMA(month));
		if (ISINT(rVal))
			fMonth = RINT(rVal);
		else
			fMonth = inMakeValidDefaults ? 1 : 0xFFFFFFFF;

		rVal = GetFrameSlot(inDateFrame, SYMA(date));
		if (ISINT(rVal))
			fDay = RINT(rVal);
		else
			fDay = inMakeValidDefaults ? 1 : 0xFFFFFFFF;

		rVal = GetFrameSlot(inDateFrame, SYMA(hour));
		if (ISINT(rVal))
			fHour = RINT(rVal);
		else
			fHour = inMakeValidDefaults ? 0 : 0xFFFFFFFF;

		rVal = GetFrameSlot(inDateFrame, SYMA(minute));
		if (ISINT(rVal))
			fMinute = RINT(rVal);
		else
			fMinute = inMakeValidDefaults ? 0 : 0xFFFFFFFF;

		rVal = GetFrameSlot(inDateFrame, SYMA(second));
		if (ISINT(rVal))
			fSecond = RINT(rVal);
		else
			fSecond = inMakeValidDefaults ? 0 : 0xFFFFFFFF;

		if (inMakeValidDefaults)
			cleanUpFields();
	}
}


void
CDate::setCurrentTime(void)
{
	initWithMinutes(RealClock());
}


bool
CDate::isValidDate(void) const
{
	return fMonth <= 12
		 && fDay <= daysInMonth();
}


int
CDate::stringToDate(const UniChar * inStr, size_t * outParsedStrLen, size_t inStrLen)
{
	// set up defaults for those elements we can’t parse
	setCurrentTime();
	ULong thisMinute = fMinute;
	ULong thisHour = fHour;
	ULong thisDayOfWeek = fDayOfWeek;
	ULong thisDay = fDay;
	ULong thisMonth = fMonth;
	ULong thisYear = fYear;

	int status = stringToDateFields(inStr, outParsedStrLen, inStrLen);
	if (status == 0 || status == 2)
	{
		if (fDay == 0xFFFFFFFF && fDayOfWeek != 0xFFFFFFFF)	// we have the day of the week but no date
			fDay = thisDay + ((fDayOfWeek + 7) - thisDayOfWeek) % 7;

		if (fYear == 0xFFFFFFFF)
			fYear = thisYear;
		if (fMonth == 0xFFFFFFFF)
			fMonth = thisMonth;
		if (fDay == 0xFFFFFFFF)
			fDay = thisDay;

		bool isTimeValid = fHour != 0xFFFFFFFF || fMinute != 0xFFFFFFFF || fSecond != 0xFFFFFFFF;
		if (fHour == 0xFFFFFFFF)
			fHour = thisHour;
		if (fMinute == 0xFFFFFFFF)
			fMinute = isTimeValid ? 0 : thisMinute;
		if (fSecond == 0xFFFFFFFF)
			fSecond = 0;
		cleanUpFields();
	}
	return status;
}


int
CDate::stringToTime(const UniChar * inStr, size_t * outParsedStrLen, size_t inStrLen)
{
	int status = 0;
	setCurrentTime();
	fHour = 0;
	fMinute = 0;
	fSecond = 0;
	gTimeLexDictionary->parseString(this, inStr, outParsedStrLen, inStrLen);
	if (*outParsedStrLen == 0)
	{
		fDayOfWeek = 0;
		fSecond = 0;
		fMinute = 0;
		fHour = 0;
		fDay = 0;
		fMonth = 0;
		fYear = 0;
		status = -1;
	}
	else if (*outParsedStrLen < inStrLen)
		status = 2;
	return status;
}


int
CDate::stringToDateFields(const UniChar * inStr, size_t * outParsedStrLen, size_t inStrLen)
{
	int status = 0;
	size_t timeStrParsedLen;
	size_t dateStrParsedLen;
	ULong thisYear = 1904 + ((RealClock() / (24*60)) * 4) / 1461;

	fSecond = 0xFFFFFFFF;
	fMinute = 0xFFFFFFFF;
	fHour = 0xFFFFFFFF;
	gTimeLexDictionary->parseString(this, inStr, &timeStrParsedLen, inStrLen);

	fDayOfWeek = 0xFFFFFFFF;
	fDay = 0xFFFFFFFF;
	fMonth = 0xFFFFFFFF;
	gDateLexDictionary->parseString(this, inStr, &dateStrParsedLen, inStrLen);

	if (dateStrParsedLen > 0 || timeStrParsedLen > 0)
	{
		if (dateStrParsedLen > timeStrParsedLen)
		{
			fSecond = 0xFFFFFFFF;
			fMinute = 0xFFFFFFFF;
			fHour = 0xFFFFFFFF;
			gTimeLexDictionary->parseString(this, inStr + dateStrParsedLen, &timeStrParsedLen, inStrLen - dateStrParsedLen);
		}
		else
		{
			fDayOfWeek = 0xFFFFFFFF;
			fDay = 0xFFFFFFFF;
			fMonth = 0xFFFFFFFF;
			gDateLexDictionary->parseString(this, inStr + timeStrParsedLen, &dateStrParsedLen, inStrLen - timeStrParsedLen);
		}
		if (fYear != thisYear
		&&  fYear < 100)
		{
			fYear += (thisYear / 100) * 100;
			if (fYear < 1920)
				fYear += 100;
		}
	}

	*outParsedStrLen = dateStrParsedLen + timeStrParsedLen;
	if (*outParsedStrLen == 0)
	{
		status = -1;
	}
	else if (*outParsedStrLen < inStrLen)
		status = 2;

	return status;
}


int
CDate::stringToDateFrame(const UniChar * inStr, size_t * outParsedStrLen, size_t inStrLen)
{
	RefVar theDate(Clone(RA(canonicalDate)));

	int status = stringToDateFields(inStr, outParsedStrLen, inStrLen);
	if (status == 0 || status == 2)
	{
		SetFrameSlot(theDate, SYMA(year), fYear < 2920 ? MAKEINT(fYear) : MAKEINT(2920));
		if (fMonth != 0xFFFFFFFF)
			SetFrameSlot(theDate, SYMA(month), MAKEINT(fMonth));
		if (fDay != 0xFFFFFFFF)
			SetFrameSlot(theDate, SYMA(date), MAKEINT(fDay));
		if (fHour != 0xFFFFFFFF)
			SetFrameSlot(theDate, SYMA(hour), MAKEINT(fHour));
		if (fMinute != 0xFFFFFFFF)
			SetFrameSlot(theDate, SYMA(minute), MAKEINT(fMinute));
		if (fSecond != 0xFFFFFFFF)
			SetFrameSlot(theDate, SYMA(second), MAKEINT(fSecond));
		if (fDayOfWeek != 0xFFFFFFFF)
			SetFrameSlot(theDate, SYMA(dayOfWeek), MAKEINT(fDayOfWeek));
	}
	SetFrameSlot(theDate, SYMA(status), MAKEINT(status));
	
	return status;
}


void
CDate::cleanUpFields(void)
{
	initWithMinutes(totalMinutes());
}


void
CDate::dateElementString(ULong inType, ULong inFormat, UniChar * outStr, size_t inStrSize, bool inLongFormat)
{
	UStr31 str;
	RefVar strings;
	RefVar suffix;
	*outStr = 0;
	str[0] = 0;

	if (ISNIL(fLongDateFormat) || ISNIL(fShortDateFormat))
	{
		RefVar locale(GetCurrentLocale());
		fLongDateFormat = GetProtoVariable(locale, SYMA(longDateFormat));
		fShortDateFormat = GetProtoVariable(locale, SYMA(shortDateFormat));
	}

	RefVar dateFormat(inLongFormat ? fLongDateFormat : fShortDateFormat);
	switch (inType)
	{
	case kElementDay:
		if (fDay < 10)
		{
			IntegerString(fDay, str);
			if (RINT(GetProtoVariable(dateFormat, SYMA(dayLeadingZ))) == kLeadZero)
				Ustrcpy(outStr, gZeroStr);
			Ustrncat(outStr, str, inStrSize);
		}
		else
			IntegerString(fDay, outStr);
		suffix = GetProtoVariable(dateFormat, inLongFormat ? SYMA(longDaySuffix) : SYMA(shortDaySuffix));
		break;

	case kElementDayOfWeek:
		switch (inFormat)
		{
		case kFormatShort:
			strings = gLocaleStrCache->shortDofWeek;
			if (NOTNIL(strings))
				break;
		case kFormatTerse:
			strings = gLocaleStrCache->terseDofWeek;
			if (NOTNIL(strings))
				break;
		case kFormatAbbr:
			strings = gLocaleStrCache->abbrDofWeek;
			if (NOTNIL(strings))
				break;
		case kFormatLong:
			strings = gLocaleStrCache->longDofWeek;
			break;
		}
		if (NOTNIL(strings))
		{
			CDataPtr dayStr(GetArraySlot(strings, fDayOfWeek));
			Ustrncpy(outStr, (const UniChar *)(char *)dayStr, inStrSize);
		}
		break;

	case kElementMonth:
		switch (inFormat)
		{
		case kFormatNumeric:
			if (fMonth < 10)
			{
				IntegerString(fMonth, str);
				if (RINT(GetProtoVariable(dateFormat, SYMA(monthLeadingZ))) == kLeadZero)
					Ustrcpy(outStr, gZeroStr);
				Ustrncat(outStr, str, inStrSize);
			}
			else
				IntegerString(fMonth, outStr);
			break;
		case kFormatShort:
			strings = GetProtoVariable(dateFormat, SYMA(shortMonth));
			if (NOTNIL(strings))
				break;
		case kFormatTerse:
			strings = GetProtoVariable(dateFormat, SYMA(terseMonth));
			if (NOTNIL(strings))
				break;
		case kFormatAbbr:
			strings = GetProtoVariable(dateFormat, SYMA(abbrMonth));
			if (NOTNIL(strings))
				break;
		case kFormatLong:
			strings = gLocaleStrCache->longMonth;
			break;
		}
		if (NOTNIL(strings))
		{
			CDataPtr monStr(GetArraySlot(strings, fMonth-1));
			Ustrncpy(outStr, (const UniChar *)(char *)monStr, inStrSize);
		}
		suffix = GetProtoVariable(dateFormat, inLongFormat ? SYMA(longMonthSuffix) : SYMA(shortMonthSuffix));
		break;

	case kElementYear:
		{
			int offs = 0;
			IntegerString(fYear, str);
			if (!inLongFormat && RINT(GetProtoVariable(dateFormat, SYMA(yearLeading))) == kNoCentury)
			{
				offs = Ustrlen(str) - 2;
				if (offs < 0) offs = 0;
			}
			Ustrncpy(outStr, str + offs, inStrSize);
			suffix = GetProtoVariable(dateFormat, inLongFormat ? SYMA(longYearSuffix) : SYMA(shortYearSuffix));
		}
		break;
	}

	if (NOTNIL(suffix))
	{
		CDataPtr suffixStr(suffix);
		Ustrcat(outStr, (const UniChar *)(char *)suffixStr);
	}
}


void
CDate::longDateString(ULong inStrSpec, UniChar * outStr, size_t inStrSize)
{
	bool hasDay;
	bool hasDayOfWeek;
	bool hasMonth;
	bool hasYear;
	ULong dayFormat, dayOfWeekFormat, monthFormat, yearFormat;
	UStr31 elementStr;
	int numOfElements = 0;

	if (ISNIL(fLongDateFormat))
		fLongDateFormat = GetProtoVariable(GetCurrentLocale(), SYMA(longDateFormat));
	Ref dateOrder = GetProtoVariable(fLongDateFormat, SYMA(longDateOrder));
	ULong dateOrderSpec = ISINT(dateOrder) ? RINT(dateOrder) : 0;
	RefVar dateDelim(GetProtoVariable(fLongDateFormat, SYMA(longDateDelim)));	// r6

	if (inStrSpec == kIncludeAllElements)
	{
		hasDay = YES;
		hasMonth = YES;
		hasYear = YES;
		dayFormat = dayOfWeekFormat = monthFormat = yearFormat = kFormatDefault;
		numOfElements = 4;
	}
	else
	{
		hasDay = NO;
		hasMonth = NO;
		hasYear = NO;
		while (inStrSpec != 0)
		{
			ULong elementType = GetNextElementType(&inStrSpec);
			ULong elementFormat = GetNextElementFormat(&inStrSpec);
			numOfElements++;
			switch (elementType)
			{
			case kElementDay:			hasDay = YES;			dayFormat = elementFormat;			break;
			case kElementDayOfWeek:	hasDayOfWeek = YES;	dayOfWeekFormat = elementFormat;	break;
			case kElementMonth:		hasMonth = YES;		monthFormat = elementFormat;		break;
			case kElementYear:		hasYear = YES;			yearFormat = elementFormat;		break;
			}
		}
	}

	if (hasYear && fYear >= 2920)
	{
		hasYear = NO;
		numOfElements--;
	}

	int index = 0;
	CDataPtr delimStr(GetArraySlot(dateDelim, index++));
	Ustrncpy(outStr, (const UniChar *)(char *)delimStr, inStrSize);

	while (dateOrderSpec != 0)
	{
		ULong theFormat = 0;
		ULong elementType = GetNextElementType(&dateOrderSpec);
		ULong elementFormat = GetNextElementFormat(&dateOrderSpec);
		if ((elementType == kElementDay && hasDay && (theFormat = dayFormat, YES))
		||  (elementType == kElementDayOfWeek && hasDayOfWeek && (theFormat = dayOfWeekFormat, YES))
		||  (elementType == kElementMonth && hasMonth && (theFormat = monthFormat, YES))
		||  (elementType == kElementYear && hasYear && (theFormat = yearFormat, YES)))
		{
			if (theFormat)
				elementFormat = theFormat;
			numOfElements--;
			dateElementString(elementType, elementFormat, elementStr, kStr31Len, YES);
			if (numOfElements > 0 || dateOrderSpec == 0)
			{
				CDataPtr delimStr(GetArraySlot(dateDelim, index));
				Ustrcat(elementStr, (const UniChar *)(char *)delimStr);
			}
			Ustrncat(outStr, elementStr, inStrSize);
		}
		index++;
	}
}


void
CDate::shortDateString(ULong inStrSpec, UniChar * outStr, size_t inStrSize)
{
	bool hasDay;
	bool hasMonth;
	bool hasYear;
	UStr31 elementStr;
	int numOfElements = 0;

	if (ISNIL(fShortDateFormat))
		fShortDateFormat = GetProtoVariable(GetCurrentLocale(), SYMA(shortDateFormat));
	ULong dateOrderSpec = RINT(GetProtoVariable(fShortDateFormat, SYMA(shortDateOrder)));
	RefVar dateDelim(GetProtoVariable(fShortDateFormat, SYMA(shortDateDelim)));

	if (inStrSpec == kIncludeAllElements)
	{
		hasDay = YES;
		hasMonth = YES;
		hasYear = YES;
		numOfElements = 3;
	}
	else
	{
		hasDay = NO;
		hasMonth = NO;
		hasYear = NO;
		while (inStrSpec != 0)
		{
			ULong elementType = GetNextElementType(&inStrSpec);
			ULong elementFormat = GetNextElementFormat(&inStrSpec);
			numOfElements++;
			switch (elementType)
			{
			case kElementDay:		hasDay = YES;		break;
			case kElementMonth:	hasMonth = YES;	break;
			case kElementYear:	hasYear = YES;		break;
			}
		}
	}

	if (hasYear && fYear >= 2920)
	{
		hasYear = NO;
		numOfElements--;
	}

	int index = 0;
	CDataPtr delimStr(GetArraySlot(dateDelim, index));
	Ustrncpy(outStr, (const UniChar *)(char *)delimStr, inStrSize);
	for (index++; dateOrderSpec != 0; index++)
	{
		ULong elementType = GetNextElementType(&dateOrderSpec);
		ULong elementFormat = GetNextElementFormat(&dateOrderSpec);
		if ((elementType == kElementDay && hasDay)
		||  (elementType == kElementMonth && hasMonth)
		||  (elementType == kElementYear && hasYear))
		{
			numOfElements--;
			dateElementString(elementType, kFormatNumeric, elementStr, kStr31Len, NO);
			if (numOfElements > 0 || dateOrderSpec == 0)
			{
				CDataPtr delimStr(GetArraySlot(dateDelim, index));
				Ustrcat(elementStr, (const UniChar *)(char *)delimStr);
			}
			Ustrncat(outStr, elementStr, inStrSize);
		}
	}
}


void
CDate::timeString(ULong inStrSpec, UniChar * outStr, size_t inStrSize)
{
	bool hasHours;
	bool hasMinutes;
	bool hasSeconds;
	bool hasAMPM;
	bool hasSuffix;
	UStr31 elementStr;
	elementStr[0] = 0;
	*outStr = 0;

	if (inStrSpec == kIncludeAllElements)
	{
		hasHours = YES;
		hasSeconds = YES;
		hasMinutes = YES;
		hasAMPM = YES;
		hasSuffix = YES;
	}
	else
	{
		hasHours = NO;
		hasSeconds = NO;
		hasMinutes = NO;
		hasAMPM = NO;
		hasSuffix = NO;
		while (inStrSpec != 0)
		{
			ULong elementType = GetNextElementType(&inStrSpec);
			ULong elementFormat = GetNextElementFormat(&inStrSpec);	// ignored for time elements
			switch (elementType)
			{
			case kElementHour:	hasHours = YES;	break;
			case kElementMinute:	hasMinutes = YES;	break;
			case kElementSecond:	hasSeconds = YES;	break;
			case kElementAMPM:	hasAMPM = YES;		break;
			case kElementSuffix:	hasSuffix = YES;	break;
			}
		}
	}

	if (ISNIL(fTimeFormat))
		fTimeFormat = GetProtoVariable(GetCurrentLocale(), SYMA(timeFormat));
	bool timeCycle = RINT(GetProtoVariable(fTimeFormat, SYMA(timeCycle))) == kCycle12;
	hasAMPM = hasAMPM && timeCycle;
	hasSuffix = hasSuffix && !timeCycle;

	// CAUTION: inStrSize not updated in each Ustrncat below

	if (hasHours)
	{
		ULong hour = fHour;
		if (hour == 0)
		{
			ULong form = RINT(GetProtoVariable(fTimeFormat, SYMA(midnightForm)));
			if (form == kUseHour12)
				hour = 12;
			else if (form == kUseHour24)
				hour = 24;
		}
		if (hour < 10
		&&  RINT(GetProtoVariable(fTimeFormat, SYMA(hourLeadingZ))) == kLeadZero)
			Ustrncat(outStr, gZeroStr, inStrSize);
		IntegerString(hour, elementStr);
		Ustrncat(outStr, elementStr, inStrSize);
		if (hasMinutes || hasSeconds)
		{
			CDataPtr sepStr(GetProtoVariable(fTimeFormat, SYMA(timeSepStr1)));
			Ustrncat(outStr, (const UniChar *)(char *)sepStr, inStrSize);
		}
	}

	if (hasMinutes)
	{
		if (fMinute < 10
		&&  RINT(GetProtoVariable(fTimeFormat, SYMA(minuteLeadingZ))) == kLeadZero)
			Ustrncat(outStr, gZeroStr, inStrSize);
		IntegerString(fMinute, elementStr);
		Ustrncat(outStr, elementStr, inStrSize);
		if (hasSeconds)
		{
			CDataPtr sepStr(GetProtoVariable(fTimeFormat, SYMA(timeSepStr2)));
			Ustrncat(outStr, (const UniChar *)(char *)sepStr, inStrSize);
		}
	}

	if (hasSeconds)
	{
		if (fSecond < 10
		&&  RINT(GetProtoVariable(fTimeFormat, SYMA(secondLeadingZ))) == kLeadZero)
			Ustrncat(outStr, gZeroStr, inStrSize);
		IntegerString(fSecond, elementStr);
		Ustrncat(outStr, elementStr, inStrSize);
	}

	if (hasAMPM)
	{
		if (fHour < 12)
		{
			CDataPtr morningStr(GetProtoVariable(fTimeFormat, SYMA(morningStr)));
			Ustrncat(outStr, (const UniChar *)(char *)morningStr, inStrSize);
		}
		else
		{
			CDataPtr eveningStr(GetProtoVariable(fTimeFormat, SYMA(eveningStr)));
			Ustrncat(outStr, (const UniChar *)(char *)eveningStr, inStrSize);
		}
	}
	else if (hasSuffix)
	{
		CDataPtr suffixStr(GetProtoVariable(fTimeFormat, SYMA(suffixStr)));
		Ustrncat(outStr, (const UniChar *)(char *)suffixStr, inStrSize);
	}
}


ULong
GetNextElementType(ULong * ioStrSpec)
{
	ULong spec = *ioStrSpec;
	*ioStrSpec = spec >> kElementTypeWidth;
	return spec & kElementTypeMask;
}


ULong
GetNextElementFormat(ULong * ioStrSpec)
{
	ULong spec = *ioStrSpec;
	*ioStrSpec = spec >> kElementFormatWidth;
	return spec & kElementFormatMask;
}


void
CDate::setFormatResource(RefArg inLocale)
{
	if (ISNIL(inLocale))
		setFormatResource(RA(NILREF), RA(NILREF), RA(NILREF));
	else
	{
		if (ISNIL(fLongDateFormat))
			fLongDateFormat = GetProtoVariable(inLocale, SYMA(longDateFormat));
		if (ISNIL(fShortDateFormat))
			fShortDateFormat = GetProtoVariable(inLocale, SYMA(shortDateFormat));
		if (ISNIL(fTimeFormat))
			fTimeFormat = GetProtoVariable(inLocale, SYMA(timeFormat));
	}
}


void
CDate::setFormatResource(RefArg inLongDateFormat, RefArg inShortDateFormat, RefArg inTimeFormat)
{
	fLongDateFormat = inLongDateFormat;
	fShortDateFormat = inShortDateFormat;
	fTimeFormat = inTimeFormat;

	if (ISNIL(fLongDateFormat) || ISNIL(fShortDateFormat) || ISNIL(fTimeFormat))
	{
		RefVar locale(GetCurrentLocale());
		if (ISNIL(fLongDateFormat))
			fLongDateFormat = GetProtoVariable(locale, SYMA(longDateFormat));
		if (ISNIL(fShortDateFormat))
			fShortDateFormat = GetProtoVariable(locale, SYMA(shortDateFormat));
		if (ISNIL(fTimeFormat))
			fTimeFormat = GetProtoVariable(locale, SYMA(timeFormat));
	}
}


void
CDate::incrementMonth(int inOffset)
{
	int nextMonth = fMonth + inOffset;

	if ((fYear == 1904 && nextMonth < 1)
	||  (fYear == 2920 && nextMonth > 12))
		return;

	div_t yearsOff = div(nextMonth - 1, 12);
	fMonth = ((yearsOff.rem + 12) % 12) + 1;	//	month must be positive

	if (fYear <= 2920)
		fYear += (nextMonth > 0) ? yearsOff.quot : nextMonth / 12 - 1;

	ULong daysThisMonth = daysInMonth();
	if (fDay > daysThisMonth)
		fDay = daysThisMonth;
}


ULong
CDate::daysInMonth(void) const
{
	ULong numOfDays = 0;
	switch (fMonth)
	{
	case 1:	// Jan
	case 3:	// Mar
	case 5:	// May
	case 7:	// Jul
	case 8:	// Aug
	case 10:	// Oct
	case 12:	// Dec
		numOfDays = 31;
		break;

	case 4:	// Apr
	case 6:	// Jun
	case 9:	// Sep
	case 11:	// Nov
		numOfDays = 30;
		break;

	case 2:	// Feb
		if ((fYear >= 2920 || fYear == 0xFFFFFFFF)
		||  ((fYear & 0x03) == 0 && ((fYear % 100) != 0 || (fYear % 400) == 0)))
			numOfDays = 29;
		else
			numOfDays = 28;
		break;
	}
	return numOfDays;
}


ULong
CDate::daysInYear(void) const
{
	ULong numOfDays = 0;
	switch (fMonth)
	{
	case 1:	// Jan
		numOfDays = 0;
		break;
	case 2:	// Feb
		numOfDays = 31;
		break;
	case 3:	// Mar
		numOfDays = 59;
		break;
	case 4:	// Apr
		numOfDays = 90;
		break;
	case 5:	// May
		numOfDays = 120;
		break;
	case 6:	// Jun
		numOfDays = 151;
		break;
	case 7:	// Jul
		numOfDays = 181;
		break;
	case 8:	// Aug
		numOfDays = 212;
		break;
	case 9:	// Sep
		numOfDays = 243;
		break;
	case 10:	// Oct
		numOfDays = 273;
		break;
	case 11:	// Nov
		numOfDays = 304;
		break;
	case 12:	// Dec
		numOfDays = 334;
		break;
	}
	if (fMonth >= 3
	&& ((fYear >= 2920 || fYear == 0xFFFFFFFF)
	||  ((fYear & 0x03) == 0 && ((fYear % 100) != 0 || (fYear % 400) == 0))))
		numOfDays++;
	return numOfDays + fDay;
}


ULong
CDate::totalDays(void) const
{
	int numOfDays = fDay - 1;

	int numOfYears = fYear - 1904;
	if (numOfYears < 0)
		numOfYears = 0;
	numOfDays += (numOfYears * 1461 + 3)/4;	// why not * 365 ?

	int numOfMonths = fMonth - 1;
	if (numOfMonths >= 2)
	{
		numOfMonths -= 2;
		numOfDays += 59;
		if ((fYear & 0x03) == 0)	// need a more accurate leap year check, surely?
			numOfDays++;
	}
//	(numOfMonths * 65 * 15 + numOfMonths * 4) * 4 + numOfMonths
	numOfDays += (numOfMonths * 3917 + 52) / 128;

	return numOfDays;
}


ULong
CDate::totalHours(void) const
{
	return totalDays() * 24 + fHour;
}


ULong
CDate::totalMinutes(void) const
{
	return totalDays() * 24*60 + fHour * 60 + fMinute;
}


ULong
CDate::totalSeconds(void) const
{
	return totalDays() * 24*60*60 + fHour * 60*60 + fMinute * 60 + fSecond;
}



Ref
ToObject(const CDate & inDate)
{
	RefVar theDate(Clone(RA(canonicalDate)));

	SetFrameSlot(theDate, SYMA(year), inDate.fYear < 2920 ? MAKEINT(inDate.fYear) : MAKEINT(2920));
	SetFrameSlot(theDate, SYMA(month), MAKEINT(inDate.fMonth));
	SetFrameSlot(theDate, SYMA(date), MAKEINT(inDate.fDay));
	SetFrameSlot(theDate, SYMA(dayOfWeek), MAKEINT(inDate.fDayOfWeek));
	SetFrameSlot(theDate, SYMA(hour), MAKEINT(inDate.fHour));
	SetFrameSlot(theDate, SYMA(minute), MAKEINT(inDate.fMinute));
	SetFrameSlot(theDate, SYMA(second), MAKEINT(inDate.fSecond));
	SetFrameSlot(theDate, SYMA(daysInMonth), MAKEINT(inDate.daysInMonth()));
	return theDate;
}

#pragma mark -

/*------------------------------------------------------------------------------
	T i m e
------------------------------------------------------------------------------*/

// get seconds based realtime clock
unsigned long
RealClockSeconds(void)
{
#if defined(correct)
	CTime now(CURealTimeAlarm::time());
	return now.convertTo(kSeconds)
#else
	return time(NULL)
#endif
	+ GMTOffset() + DaylightSavingsOffset();
}


// set realtime clock seconds - since 1/1/04 12:00am
void
SetRealClockSeconds(unsigned long inSecsSince1904)
{
	CTime theTime(inSecsSince1904 - GMTOffset() - DaylightSavingsOffset(), kSeconds);
#if defined(correct)
	CURealTimeAlarm::setTime(theTime);
#endif
}


// get minutes based realtime clock
unsigned long
RealClock(void)
{
	return RealClockSeconds() / 60;
}


// set realtime clock minutes - since 1/1/04 12:00am
void
SetRealClock(unsigned long inMinsSince1904)
{
	SetRealClockSeconds(inMinsSince1904 * 60);
}


int
GMTOffset(void)
{
	RefVar loc(GetPreference(SYMA(location)));
	return RINT(GetFrameSlot(loc, SYMA(gmt)));
}


int
DaylightSavingsOffset(void)
{
	return RINT(GetPreference(SYMA(daylightSavings)));
}


void
LongDateString(ULong inMinsSince1904, ULong inStrSpec, UniChar * outStr, size_t inStrSize, RefArg inResource)
{
	CDate theDate;
	*outStr = 0;
	theDate.initWithMinutes(inMinsSince1904);
	theDate.setFormatResource(inResource);
	theDate.longDateString(inStrSpec, outStr, inStrSize);
}


void
ShortDateString(ULong inMinsSince1904, ULong inStrSpec, UniChar * outStr, size_t inStrSize, RefArg inResource)
{
	CDate theDate;
	*outStr = 0;
	theDate.initWithMinutes(inMinsSince1904);
	theDate.setFormatResource(inResource);
	theDate.shortDateString(inStrSpec, outStr, inStrSize);
}


void
TimeString(ULong inMinsSince1904, ULong inStrSpec, UniChar * outStr, size_t inStrSize, RefArg inResource)
{
	CDate theDate;
	*outStr = 0;
	theDate.initWithMinutes(inMinsSince1904);
	theDate.setFormatResource(inResource);
	theDate.timeString(inStrSpec, outStr, inStrSize);
}


void
TimeFrameString(RefArg inDateFrame, ULong inStrSpec, UniChar * outStr, size_t inStrSize, RefArg inResource)
{
	CDate theDate;
	*outStr = 0;
	theDate.initWithDateFrame(inDateFrame, NO);
	theDate.setFormatResource(inResource);
	theDate.timeString(inStrSpec, outStr, inStrSize);
}

#pragma mark -

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

Ref
FTime(RefArg inRcvr)
{
	return MAKEINT(RealClock());
}


Ref
FSetTime(RefArg inRcvr, RefArg inMinsSince1904)
{
	SetRealClock(RINT(inMinsSince1904));
	return NILREF;
}


Ref
FTimeInSeconds(RefArg inRcvr)
{
	return MAKEINT(RealClockSeconds() - kSecsSince1904);		// adjust to secs since 1993 (original says + 0x5896C600)
}


Ref
FSetTimeInSeconds(RefArg inRcvr, RefArg inSecsSince1993)
{
	SetRealClockSeconds(RINT(inSecsSince1993) + kSecsSince1904);		// adjust to secs since 1904
	return NILREF;
}


Ref
FSleep(RefArg inRcvr, RefArg inMilliseconds)
{
	Wait(RINT(inMilliseconds));
	return NILREF;
}

#pragma mark -

Ref
FIsValidDate(RefArg inRcvr, RefArg inDate)
{
	CDate theDate;

	if (IsString(inDate))
	{
		CDataPtr str(inDate);
		size_t strParsedLen;
		theDate.stringToDateFrame((const UniChar *)(char *)str, &strParsedLen, 0xFFFFFFFF);
	}
	else if (IsFrame(inDate))
		theDate.initWithDateFrame(inDate, NO);

	return MAKEBOOLEAN(theDate.isValidDate());
}


Ref
FDateNTime(RefArg inRcvr, RefArg inMinsSince1904)
{
	UStr63 timeStr;
	UStr63 dateStr;
	ULong timeStrSpec = RINT(GetFrameSlot(RA(dateTimeStrSpecs), SYMA(shortTimeStrSpec)));
	ULong time = ISINT(inMinsSince1904) ? RINT(inMinsSince1904) : RealClock();
	ShortDateString(time, 0, dateStr, kStr63Len, RA(NILREF));
	TimeString(time, timeStrSpec, timeStr, kStr63Len, RA(NILREF));
	Ustrcat(dateStr, gSpaceStr);
	Ustrncat(dateStr, timeStr, kStr63Len);
	return MakeString(dateStr);
}


Ref
FHourMinute(RefArg inRcvr, RefArg inMinsSince1904)
{
	UStr63 str;
	ULong timeStrSpec = RINT(GetFrameSlot(RA(dateTimeStrSpecs), SYMA(shortTimeStrSpec)));
	ULong time = ISINT(inMinsSince1904) ? RINT(inMinsSince1904) : RealClock();
	TimeString(time, timeStrSpec, str, kStr63Len, RA(NILREF));
	return MakeString(str);
}


Ref
FTimeStr(RefArg inRcvr, RefArg inMinsSince1904, RefArg inTimeStrSpec)
{
	UStr63 str;
	ULong timeStrSpec = RINT(inTimeStrSpec);
	ULong time = ISINT(inMinsSince1904) ? RINT(inMinsSince1904) : RealClock();
	TimeString(time, timeStrSpec, str, kStr63Len, RA(NILREF));
	return MakeString(str);
}


Ref
FTimeFrameStr(RefArg inRcvr, RefArg inDateFrame, RefArg inTimeStrSpec)
{
	UStr63 str;
	ULong timeStrSpec = RINT(inTimeStrSpec);
	if (IsFrame(inDateFrame))
	{
		TimeFrameString(inDateFrame, timeStrSpec, str, kStr63Len, RA(NILREF));
		return MakeString(str);
	}
	return NILREF;
}


Ref
FShortDate(RefArg inRcvr, RefArg inMinsSince1904)
{
	UStr63 dayStr;
	UStr63 dateStr;
	ULong dayStrSpec = RINT(GetFrameSlot(RA(dateTimeStrSpecs), SYMA(abbrDayOfWeekStrSpec)));
	ULong dateStrSpec = RINT(GetFrameSlot(RA(dateTimeStrSpecs), SYMA(monthDayStrSpec)));
	ULong time = ISINT(inMinsSince1904) ? RINT(inMinsSince1904) : RealClock();
	LongDateString(time, dayStrSpec, dayStr, kStr63Len, RA(NILREF));
	ShortDateString(time, dateStrSpec, dateStr, kStr63Len, RA(NILREF));
	Ustrcat(dayStr, gSpaceStr);
	Ustrncat(dayStr, dateStr, kStr63Len);
	return MakeString(dayStr);
}


Ref
FShortDateStr(RefArg inRcvr, RefArg inMinsSince1904, RefArg inDateStrSpec)
{
	UStr63 str;
	ULong dateStrSpec = RINT(inDateStrSpec);
	ULong time = ISINT(inMinsSince1904) ? RINT(inMinsSince1904) : RealClock();
	ShortDateString(time, dateStrSpec, str, kStr63Len, RA(NILREF));
	return MakeString(str);
}


Ref
FLongDateStr(RefArg inRcvr, RefArg inMinsSince1904, RefArg inDateStrSpec)
{
	UStr63 str;
	ULong dateStrSpec = RINT(inDateStrSpec);
	ULong time = ISINT(inMinsSince1904) ? RINT(inMinsSince1904) : RealClock();
	LongDateString(time, dateStrSpec, str, kStr63Len, RA(NILREF));
	return MakeString(str);
}


Ref
FStringToDate(RefArg inRcvr, RefArg inStr)
{
	RefVar theTime;
	size_t strLen = (Length(inStr) - sizeof(UniChar)) / sizeof(UniChar);
	if (strLen > 0)
	{
		CDate theDate;
		CDataPtr str(inStr);
		size_t strParsedLen;
		int status = theDate.stringToDate((const UniChar *)(char *)str, &strParsedLen, strLen);
		if (status == 0 || status == 2)
			theTime = MAKEINT(theDate.totalMinutes());
	}
	return theTime;
}


Ref
FStringToDateFrame(RefArg inRcvr, RefArg inStr)
{
	RefVar theTime;
	size_t strLen = (Length(inStr) - sizeof(UniChar)) / sizeof(UniChar);
	if (strLen > 0)
	{
		CDate theDate;
		CDataPtr str(inStr);
		size_t strParsedLen;
		theTime = theDate.stringToDateFrame((const UniChar *)(char *)str, &strParsedLen, strLen);
	}
	return theTime;
}


Ref
FStringToTime(RefArg inRcvr, RefArg inStr)
{
	RefVar theTime;
	size_t strLen = (Length(inStr) - sizeof(UniChar)) / sizeof(UniChar);
	if (strLen > 0)
	{
		CDate theDate;
		CDataPtr str(inStr);
		size_t strParsedLen;
		int status = theDate.stringToTime((const UniChar *)(char *)str, &strParsedLen, strLen);
		if (status == 0 || status == 2)
			theTime = MAKEINT(theDate.totalMinutes());
	}
	return theTime;
}


Ref
FDate(RefArg inRcvr, RefArg inMinsSince1904)
{
	CDate theDate;
	theDate.initWithMinutes(RINT(inMinsSince1904));
	return ToObject(theDate);
}


Ref
FDateFromSeconds(RefArg inRcvr, RefArg inSecsSince1993)
{
	CDate theDate;
	theDate.initWithSeconds(RINT(inSecsSince1993) + kSecsSince1904);		// adjust to secs since 1904
	return ToObject(theDate);
}


Ref
FTotalMinutes(RefArg inRcvr, RefArg inDateFrame)
{
	CDate theDate;
	theDate.initWithDateFrame(inDateFrame, YES);
	return MAKEINT(theDate.totalMinutes());
}


Ref
FIncrementMonth(RefArg inRcvr, RefArg inMinsSince1904, RefArg inNumOfMonths)
{
	ULong time = RINT(inMinsSince1904);
	int numOfMonths = RINT(inNumOfMonths);
/*	rather weak attempt at sanity check
	long newTime = time + numOfMonths;
	if (newTime < 0)
		return 0;
	if (newTime > 0x1FFFFFFF)
		return -1;
*/
	CDate theDate(time);
	theDate.incrementMonth(numOfMonths);
	return MAKEINT(theDate.totalMinutes());
}

