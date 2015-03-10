/*
	File:		Journal.cc

	Contains:	Pen stroke journalling system.

	Written by:	Newton Research Group, 2010.
*/

#include "Journal.h"
#include "TestAgent.h"
#include "RecStroke.h"
#include "Funcs.h"
#include "ROMSymbols.h"

extern NewtonErr	StartBypassTablet(void);
extern NewtonErr	StopBypassTablet(void);
extern NewtonErr	InsertTabletSample(ULong inSample, ULong inTime);


/* -----------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
----------------------------------------------------------------------------- */

extern "C" {
Ref	FJournalStartRecord(RefArg inRcvr, RefArg inMsg, RefArg inArg);
Ref	FJournalStopRecord(RefArg inRcvr);
Ref	FJournalReplayStrokes(RefArg inRcvr, RefArg inData, RefArg inOriginX, RefArg inOriginY, RefArg inArg4);
Ref	FJournalReplayAStroke(RefArg inRcvr, RefArg inData, RefArg inOriginX, RefArg inOriginY, RefArg inArg4, RefArg inArg5, RefArg inArg6);
Ref	FJournalReplayALine(RefArg inRcvr, RefArg inData, RefArg inArg2, RefArg inArg3, RefArg inArg4, RefArg inArg5, RefArg inArg6);
Ref	FJournalReplayBusy(RefArg inRcvr);
}


/* -----------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------------- */

void	JournalRecordAStroke(CRecStroke * inStroke);
void	JournalInsertTabletSamople(void);
void	JournalStopReplay(void);
bool	IsJournalReplayBusy(void);


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

enum
{
	kJournalSamplePacked = 1,
	kJournalSampleRaw
};

enum
{
	kJournalIdle,
	kJournalRecording,
	kJournalReplaying
};

int	gJournallingState;		// 0C100FB8

struct JournalRecordStuff
{
	RefStruct * receiver;		// +00
	RefStruct * message;			// +04
	ULong			startTime;		// +08
	ULong			numOfStrokes;	// +0C
	int			sampleType;		// +10	was short
} * gJournalRecordStuff;		// 0C100FBC

CJournalReplayHandler * gJournalPlayer;	// 0C100FC0


#pragma mark -
/* -----------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Start recording strokes.
	Args:		inRcvr			receiver for...
				inMsg				method to be called after each stroke is recorded
				inSampleType
	Return:	error code
----------------------------------------------------------------------------- */

Ref
FJournalStartRecord(RefArg inRcvr, RefArg inMsg, RefArg inSampleType)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILNOT(gJournalRecordStuff = (JournalRecordStuff *)NewPtr(sizeof(JournalRecordStuff)), err = MemError();)
		gJournalRecordStuff->receiver = NULL;	// not in the original
		gJournalRecordStuff->message = NULL;
		gJournalRecordStuff->numOfStrokes = 0;
		XFAILNOT(gJournalRecordStuff->receiver = new RefStruct, err = MemError();)
		*gJournalRecordStuff->receiver = inRcvr;
		XFAILNOT(gJournalRecordStuff->message = new RefStruct, err = MemError();)
		*gJournalRecordStuff->message = inMsg;
		gJournalRecordStuff->sampleType = RINT(inSampleType);
		gJournallingState = kJournalRecording;
	}
	XENDTRY;
	XDOFAIL(err)
	{
		FJournalStopRecord(RA(NILREF));
	}
	XENDFAIL;
	return MAKEINT(err);
}


/* -----------------------------------------------------------------------------
	Stop recording strokes.
	Dispose all allocated structures.
	Args:		inRcvr
	Return:	NILREF
----------------------------------------------------------------------------- */

Ref
FJournalStopRecord(RefArg inRcvr)
{
	gJournallingState = kJournalIdle;
	if (gJournalRecordStuff != NULL)
	{
		if (gJournalRecordStuff->receiver)
			delete gJournalRecordStuff->receiver;
		if (gJournalRecordStuff->message)
			delete gJournalRecordStuff->message;
		FreePtr((Ptr)gJournalRecordStuff);
		gJournalRecordStuff = NULL;
	}
	return NILREF;		// original returns nil
}


/* -----------------------------------------------------------------------------
	Replay recorded strokes.
	Args:		inRcvr
				inData
				inOriginX
				inOriginY
				inNumOfStrokes
	Return:	error code
----------------------------------------------------------------------------- */

Ref
FJournalReplayStrokes(RefArg inRcvr, RefArg inData, RefArg inOriginX, RefArg inOriginY, RefArg inNumOfStrokes)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILIF(gJournallingState != kJournalIdle, err = -1;)
		XFAILNOT(gJournalPlayer = new CJournalReplayHandler, err = MemError();)
		gJournallingState = kJournalReplaying;

		CDataPtr data(inData);
		gJournalPlayer->fStrokeData = data;
		gJournalPlayer->parseStrokeFileHeader();
		if (ISINT(inNumOfStrokes))
			gJournalPlayer->setStrokesToPlay(RVALUE(inNumOfStrokes));
		gJournalPlayer->setOrigin(RINT(inOriginX), RINT(inOriginY));

		gTestReporterForNewt->agentReportStatus(13, NULL);
	}
	XENDTRY;
	return MAKEINT(err);
}


/* -----------------------------------------------------------------------------
	Replay a single stroke.
	Args:		inRcvr
				inData
				inOriginX
				inOriginY
				inSampleType
				inArg5
				inArg6
	Return:	error code
----------------------------------------------------------------------------- */

Ref
FJournalReplayAStroke(RefArg inRcvr, RefArg inData, RefArg inOriginX, RefArg inOriginY, RefArg inSampleType, RefArg inArg5, RefArg inArg6)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILIF(gJournallingState != kJournalIdle, err = -1;)
		XFAILNOT(gJournalPlayer = new CJournalReplayHandler, err = MemError();)
		gJournallingState = kJournalReplaying;

		gJournalPlayer->fNumOfStrokesToPlay = gJournalPlayer->fNumOfStrokes = 1;
		gJournalPlayer->fStrokeIndex = 1;
		gJournalPlayer->fSampleType = RINT(inSampleType);
		gJournalPlayer->f1A = RINT(inArg5);
		gJournalPlayer->f1C = RINT(inArg6);
		err = gJournalPlayer->playAStroke((JournalStroke *)BinaryData(inData), RINT(inOriginX), RINT(inOriginY), NO);

		gTestReporterForNewt->agentReportStatus(13, NULL);
	}
	XENDTRY;
	return MAKEINT(err);
}


/* -----------------------------------------------------------------------------
	Replay a line as a stroke.
	Args:		inRcvr
				inStartX
				inStartY
				inEndX
				inAEndY
				inIsSelection
				inArg6
	Return:	error code
----------------------------------------------------------------------------- */

Ref
FJournalReplayALine(RefArg inRcvr, RefArg inStartX, RefArg inStartY, RefArg inEndX, RefArg inEndY, RefArg inIsSelection, RefArg inArg6)
{
//	-- r8 r7 r3 r6 r9 r0
	// sp-1C
	short sp04 = RINT(inArg6);
	long sp00 = 20;
	NewtonErr err = noErr;
	XTRY
	{
		XFAILIF(gJournallingState != kJournalIdle, err = -1;)
		if (gJournalPlayer == NULL)
			XFAILNOT(gJournalPlayer = new CJournalReplayHandler, err = MemError();)
		else
		{
			gJournalPlayer->fSampleData = NULL;
			gJournalPlayer->fStrokeIndex = 0;
			gJournalPlayer->fIsStrokeAvailable = NO;
			gJournalPlayer->fIsReplaying = YES;
		}
		gJournallingState = kJournalReplaying;

//sp-18
		LPoint startPt, endPt, delta;
		startPt.x = RINT(inStartX);	// r9
		startPt.y = RINT(inStartY);	// r7
		endPt.x = RINT(inEndX);			// sp14
		endPt.y = RINT(inEndY);			// sp10
		delta.x = (startPt.x >= endPt.x) ? startPt.x - endPt.x : endPt.x - startPt.x;
		delta.y = (startPt.y >= endPt.y) ? startPt.y - endPt.y : endPt.y - startPt.y;

		bool isDoubleTap = NO;			// sp08
		bool isSelection = NO;			// r10
		if (NOTNIL(inIsSelection))
		{
			if (delta.x < 4 && delta.y < 4)
				isDoubleTap = YES;
			else
				isSelection = YES;
		}

		long r4 = (delta.x > delta.y) ? delta.x / 2 : delta.y / 2;

		if (isSelection)
		{
			if (r4 < 0)
				r4 = (r4 + 3);
			r4 /= 4;
		}
		if (r4 == 0)
			r4 = 10;
		ULong numOfPoints = r4 + 1;		// r8
		ULong numOfDoubleTapGapPoints;	// sp00
		if (isDoubleTap)
		{
			numOfDoubleTapGapPoints = sp04 * 9 / 60;
			numOfPoints = numOfPoints * 2 + numOfDoubleTapGapPoints + 2;
		}
		if (isSelection)
			numOfPoints += 60;

		// create the journal stroke
		JournalStroke * theStroke;		// r6
		size_t strokeSize = sizeof(JournalStroke) + numOfPoints * sizeof(TabletSample);
		XFAILNOT(theStroke = (JournalStroke *)NewPtr(strokeSize), err = MemError();)

		// fill in the header
		theStroke->dataSize = strokeSize;
		theStroke->count = numOfPoints;
		theStroke->startTime = 5;
		if (isSelection)
			theStroke->endTime = 5 + r4 * 60 / sp00  + 3600 / sp04;
		else
			theStroke->endTime = 5 + numOfPoints * 60 / sp04;
		if (theStroke->endTime == 5)
			theStroke->endTime = 6;

		// add points forming the straight line stroke
		TabletSample pt;
		ULong * p = theStroke->points;
		if (isSelection)
		{
			// don’t move for the first second
			pt.x = startPt.x * 8;
			pt.y = startPt.y * 8;
			pt.z = 4;
			for (ArrayIndex i = 0; i < 60; ++i)
				*p++ = pt.intValue;
		}

		delta.x = endPt.x - startPt.x;
		delta.y = endPt.y - startPt.y;
		for (ArrayIndex i = 0; i < r4; ++i)
		{
			// add points on the line
			pt.x = (startPt.x + delta.x * i/r4) * 8;
			pt.y = (startPt.y + delta.y * i/r4) * 8;
			pt.z = 4;
			*p++ = pt.intValue;
		}
		// ensure we have the end point
		pt.x = endPt.x * 8;
		pt.y = endPt.y * 8;
		pt.z = 4;
		*p++ = pt.intValue;

		if (isDoubleTap)
		{
			// lift the pen
			*p++ = 0x0000000E;
			// add null samples for the double-tap time
			for (ArrayIndex i = 0; i < numOfDoubleTapGapPoints; ++i)
				*p++ = 0;
			// pen down
			*p++ = 0x0000000D;
			// add the points again
			for (ArrayIndex i = 0; i < r4; ++i)
			{
				// add points on the line
				pt.x = (startPt.x + delta.x * i/r4) * 8;
				pt.y = (startPt.y + delta.y * i/r4) * 8;
				pt.z = 4;
				*p++ = pt.intValue;
			}
			// ensure we have the end point again
			pt.x = endPt.x * 8;
			pt.y = endPt.y * 8;
			pt.z = 4;
			*p++ = pt.intValue;
		}
//sp+18

		gJournalPlayer->fNumOfStrokesToPlay = gJournalPlayer->fNumOfStrokes = 1;
		gJournalPlayer->fSampleType = kJournalSamplePacked;
		gJournalPlayer->f1A = sp04;
		gJournalPlayer->f1C = sp00;
		gJournalPlayer->fStrokeIndex = 1;
		err = gJournalPlayer->playAStroke(theStroke, 0, 0, YES);
		// who FreePtr()s theStroke?

		gTestReporterForNewt->agentReportStatus(13, NULL);
	}
	XENDTRY;
	return MAKEINT(err);
}


/* -----------------------------------------------------------------------------
	Are we replaying strokes?
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

Ref
FJournalReplayBusy(RefArg inRcvr)
{
	return MAKEBOOLEAN(IsJournalReplayBusy());
}


#pragma mark -
/* -----------------------------------------------------------------------------
	P r i v a t e   F u n c t i o n s
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Record a stroke.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
JournalRecordAStroke(CRecStroke * inStroke)
{
	if (gJournallingState == kJournalRecording)
	{
		size_t sampleSize;
		if (gJournalRecordStuff->sampleType == kJournalSamplePacked)
			sampleSize = sizeof(TabletSample);
		else if (gJournalRecordStuff->sampleType == kJournalSampleRaw)
			sampleSize = sizeof(TabPt);
		else
			return;

		size_t strokeDataSize = sizeof(JournalStroke) + inStroke->count() * sampleSize;
		JournalStroke * strokeData = (JournalStroke *)NewPtr(strokeDataSize);
		if (++(gJournalRecordStuff->numOfStrokes) == 1)
			gJournalRecordStuff->startTime = inStroke->startTime();
		strokeData->dataSize = strokeDataSize;
		strokeData->count = inStroke->count();
		strokeData->startTime = gJournalRecordStuff->startTime - inStroke->startTime();
		strokeData->endTime = gJournalRecordStuff->startTime - inStroke->endTime();
		ULong * p = strokeData->points;
		for (ArrayIndex i = 0; i < inStroke->count(); ++i, p = (ULong *)((char *)p + sampleSize))
		{
			TabPt pt;
			inStroke->getTabPt(i, &pt);
			if (gJournalRecordStuff->sampleType == kJournalSamplePacked)
			{
				TabletSample sp;
				sp.x = pt.x / kTabScale;
				sp.y = pt.y / kTabScale;
				sp.z = pt.z;
				memmove(p, &sp, sampleSize);
			}
			else if (gJournalRecordStuff->sampleType == kJournalSampleRaw)
				memmove(p, &pt, sampleSize);
		}

		RefVar strokeRef(AllocateBinary(SYMA(stroke), strokeDataSize));
		memmove(BinaryData(strokeRef), strokeData, strokeDataSize);
		FreePtr((Ptr)strokeData);

		RefVar args(MakeArray(2));
		SetArraySlot(args, 0, strokeRef);
		SetArraySlot(args, 1, MAKEINT(strokeDataSize));
		DoMessage(*gJournalRecordStuff->receiver, *gJournalRecordStuff->message, args);
	}
}


/* -----------------------------------------------------------------------------
	Insert a sample into the queue.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
JournalInsertTabletSample(void)
{
	if (gJournalPlayer)
	{
		NewtonErr err;
		ULong sample;
		while (gJournalPlayer->getNextTabletSample(&sample))
		{
			if (gJournalPlayer->fPointIndex != 1
			||	((gJournalPlayer->fStrokeIndex != 1 ||	StartBypassTablet() == noErr)
			   && InsertTabletSample(0x0D, 0) == noErr))
			{
				if (sample != 0)
					InsertTabletSample(sample, 0);
			}
		}
	}
}


/* -----------------------------------------------------------------------------
	Stop replaying strokes.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
JournalStopReplay(void)
{
	if (gJournalPlayer)
	{
		StopBypassTablet();
		gJournallingState = kJournalIdle;
		delete gJournalPlayer;
		gJournalPlayer = NULL;
	}
}


/* -----------------------------------------------------------------------------
	Are we replaying strokes?
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

bool
IsJournalReplayBusy(void)
{
	return gJournalPlayer && gJournalPlayer->isJournalReplayBusy();
}


#pragma mark -
/* -----------------------------------------------------------------------------
	J o u r n a l   R e p l a y   H a n d l e r
----------------------------------------------------------------------------- */

CJournalReplayHandler::CJournalReplayHandler()
{
	fSampleData = NULL;		// not in the original
	fStrokeIndex = 0;
	fIsStrokeAvailable = NO;
	fIsReplaying = YES;
}


CJournalReplayHandler::~CJournalReplayHandler()
{
	if (fSampleData)
		FreePtr((Ptr)fSampleData);
}


void
CJournalReplayHandler::initStroke(ULong inOriginX, ULong inOriginY)
{
	if (fStrokeIndex == 1)
	{
		f18 = 0;		// doesn’t appear to be used
		fTimebase = (gJournalPlayer->fNumOfStrokes > 1) ? GetTicks() + 300 : GetTicks();
	}
	setOrigin(inOriginX, inOriginY);
	fStrokeDuration = fSampleData->endTime - fSampleData->startTime;
	fNumOfPoints = fSampleData->count;
	fPointIndex = 0;
	fStrokeStartTime = fTimebase + fSampleData->startTime;
	f3C = NO;
	if (fNumOfPoints > 60)
	{
		f3C = ((fNumOfPoints * 60) / fStrokeDuration) < (f1A - 8);
	}
	f38 = 0;
}


NewtonErr
CJournalReplayHandler::playAStroke(JournalStroke * inStroke, ULong inOriginX, ULong inOriginY, bool inTakeOwnership)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILIF(fSampleData != NULL, err = -1;)
		if (inTakeOwnership)
			fSampleData = inStroke;
		else
		{
			JournalStroke * dupStroke;
			size_t strokeSize = inStroke->dataSize;
			XFAILNOT(dupStroke = (JournalStroke *)NewPtr(strokeSize), err = MemError();)
			memmove(dupStroke, inStroke, strokeSize);
			fSampleData = dupStroke;
		}
		if (inOriginX > 0 || inOriginY > 0)
		{
			TabletSample origin;
			origin.x = inOriginX * 8;
			origin.y = inOriginY * 8;
			origin.z = 0;
			for (ArrayIndex i = 0; i < inStroke->count; ++i)
			{
				// this is only going to work if the JournalStroke holds TabletSamples
				fSampleData->points[i] += origin.intValue;
			}
		}
		initStroke(inOriginX, inOriginY);
	}
	XENDTRY;
	return err;
}


bool
CJournalReplayHandler::isJournalReplayBusy(void)
{
	return fIsReplaying;
}


bool
CJournalReplayHandler::getNextTabletSample(ULong * outSample)
{
	ULong now = GetTicks();
	bool isSampleValid = NO;
	if (fSampleData == NULL)
	{
		if (fIsStrokeAvailable)
		{
			JournalStroke * stroke;
			if ((stroke = getNextStroke()))
				playAStroke(stroke, fOrigin.x, fOrigin.y, NO);	// sets fSampleData as a side effect
			else
				fIsStrokeAvailable = NO;
		}
	}
	if (fSampleData == NULL)
	{
		fIsReplaying = NO;
		return NO;
	}

	if (fStrokeStartTime > now)
		return NO;

	if (f38 <= fPointIndex && fStrokeStartTime <= now)
	{
		if (!f3C
		||  fPointIndex < 60)
		{
			if (fPointIndex == 59)
				f30 = now;
			f38 = fNumOfPoints * (now - fStrokeStartTime) / fStrokeDuration;
		}
		else
			f38 = (now - f30) / 3 + 61;
	}

	if (fPointIndex < f38)
	{
		if (fSampleType == kJournalSamplePacked)
			*outSample = fSampleData->points[fPointIndex];
		else if (fSampleType == kJournalSampleRaw)
		{
			TabPt pt;
			memmove(&pt, (char *)fSampleData->points + fPointIndex * sizeof(TabPt), sizeof(TabPt));
			((TabletSample *)outSample)->x = pt.x * kTabScale;
			((TabletSample *)outSample)->y = pt.y * kTabScale;
			((TabletSample *)outSample)->z = pt.z;
		}
		fPointIndex++;
		if (f3C && fPointIndex == 59)
			f30 = now;
		isSampleValid = YES;
	}

	if (fPointIndex >= fNumOfPoints)	// was fSampleData->count
	{
		*outSample = 0x0000000E;	// lift the pen
		FreePtr((Ptr)fSampleData);
		fSampleData = NULL;
	}

	return isSampleValid;
}


void
CJournalReplayHandler::parseStrokeFileHeader(void)
{
	StrokeHeader * strokeInfo = (StrokeHeader *)(char *)fStrokeData;
	fIsStrokeAvailable = YES;

	fNumOfStrokesToPlay = fNumOfStrokes = strokeInfo->x04;
	fStrokeIndex = 0;
	fSampleType = strokeInfo->x02;
	f1A = strokeInfo->x06;
	f1C = strokeInfo->x08;
	fStrokeDataOffset = 24;
	fStrokeDataSize = 0;
}


void
CJournalReplayHandler::setStrokesToPlay(short inCount)
{
	fNumOfStrokesToPlay = (inCount > 0 && inCount < fNumOfStrokes) ? inCount : fNumOfStrokes;
}


JournalStroke *
CJournalReplayHandler::getNextStroke(void)
{
	if (fStrokeIndex < fNumOfStrokesToPlay)
	{
		JournalStroke * aStroke = (JournalStroke *)((char *)fStrokeData + fStrokeDataOffset + fStrokeDataSize);
		fStrokeDataOffset += fStrokeDataSize;
		fStrokeDataSize = aStroke->dataSize;
		fStrokeIndex++;
		return aStroke;
	}
	return NULL;
}

