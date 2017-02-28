/*
	File:		Stroker.cc

	Contains:	The stroker reads points out of the tablet queue
					and assembles strokes in the stroke queue.

	Written by:	Newton Research Group, 2010.
*/

#include "Stroker.h"
#include "RecStroke.h"
#include "Tablet.h"
#include "TabletBuffer.h"
#include "NewtonGestalt.h"
#include "Semaphore.h"


/* -----------------------------------------------------------------------------
	T y p e s
----------------------------------------------------------------------------- */

#define kStrokeQSize 64

struct StrokeQueue
{
	UShort		wrIndex;
	UShort		rdIndex;
	CSStroke *	buf[kStrokeQSize];
};

struct StrokeHiliteState
{
	FPoint	x00;
	ULong		x08;		// start time
	ULong		x0C;
	UChar		x10;
	UChar		x11;
	UChar		x12;
	UChar		x13;
	ULong		x14;
	ULong		x18;
// size +1C;
};


/* -----------------------------------------------------------------------------
	D a t a
	pen states
		1 = ?
		3 = up
		4 = active
		6 = down
----------------------------------------------------------------------------- */

bool		collect;					// 0C1008A8
int		nextTab;					// 0C1008AC
int		nextDown;				// 0C1008B0
int		nextUp;					// 0C1008B4
int		gLastPenState = 3;	// 0C1008B8
int		gLastPenTip = 1;		// 0C1008BC
ULong		gLastDownTime;			// 0C1008C0
ULong		gLastUpTime;			// 0C1008C4


short						useStylus;					// 0C10115C

ULong						gDoubleTapInterval;		// 0C101854

bool						gDefaultInk = true;		// 0C101890
FPoint					gTabScale;					// 0C101894
StrokeQueue				gStrokeQ;					// 0C10189C	original makes this a pointer

ULong						gTickOff = 0;				// 0C104C24
CULockingSemaphore * gStrokeQSemaphore;		// 0C104C28
CULockingSemaphore * gStrokeSemaphore;			// 0C104C2C
bool						gStrokeValid;				// 0C104C30
UShort					gHiliteDistance;			// 0C104C34
ULong						gMaxTapSize;				// 0C104C38
ULong						gDoubleTapDistance;		// 0C104C3C
ULong						gSamplesToTicks;			// 0C104C40
bool						gStrokeInProgress;		// 0C104C44

StrokeHiliteState		oldHilite;					// 0C106ED8
StrokeHiliteState		newHilite;					// 0C106EF4


/* -----------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------------- */

extern float	DistPoint(FPoint * inPt1, FPoint * inPt2);

void		RealStrokeInit(void);
void		StrokeReInit(void);
void		ClearStrokeBuf(void);

void		InitHiliteGlobals(StrokeHiliteState * ioState);
void		SetUpDistances(void);

void		TabInit(void);
void		TabOn(void);
void		GetTabScale(FPoint * outTabScale);

bool		GetTabPt(TabPt * outPt);
bool		LastTabPt(TabPt * outPt);
ULong		GetDownTime(void);
ULong		GetUpTime(void);
void		StrokeTime(void);
bool		StrokeNext(void);
void		InitHiliteState(CRecStroke*, StrokeHiliteState*, StrokeHiliteState*);
int		CheckHiliteState(CRecStroke*, StrokeHiliteState*, StrokeHiliteState*, bool);


#pragma mark -
/* -----------------------------------------------------------------------------
	T a b l e t
	These first functions look redundant -- superceded in NOS2?
----------------------------------------------------------------------------- */

int		NextTab(void);
int		NextDown(void);
int		NextUp(void);


void
TabBoot(void)
{
	EnterAtomic();
// this really does nothing
	ExitAtomic();
}


NewtonErr
TabletIdle(void)
{
// this really does nothing
	return noErr;
}


void
TabInit(void)
{
	nextTab = NextTab();
	nextDown = NextDown();
	nextUp = NextUp();
}


void
TabOn(void)
{
	TabInit();
	collect = true;
}


int
NextTab(void)
{ return 0; /* really */ }


int
NextDown(void)
{ return 0; /* really */ }


int
NextUp(void)
{ return 0; /* really */ }


void
GetTabScale(FPoint * outTabScale)
{
	outTabScale->x = outTabScale->y = kTabScale;
}


bool
GetTabPt(TabPt * outPt)
{
static ULong penDownTime;

	while (!StrokerBufferEmpty())
	{
		TabletSample sample;
		sample.intValue = GetStrokerData();
		switch (sample.z)
		{
		case 0:		// pen is active
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			if (gLastPenState == 6 || gLastPenState == 4)
			{
				outPt->x = sample.x / kTabScale;
				outPt->y = sample.y / kTabScale;
				outPt->z = sample.z;
				outPt->p = 0;
				IncStrokerIndex(1);
				gLastPenState = 4;
//printf("pen at %f, %f\n", outPt->x, outPt->y);
				return true;
			}
			break;

		case kPenDownSample:
			if (gLastPenState == 3)
			{
				gLastDownTime = GetStrokerData(1);
				IncStrokerIndex(2);
				gLastPenState = 6;
//penDownTime = gLastDownTime;
//printf("pen down @ %u\n", gLastDownTime);
				// and loop for an actual pen location sample
			}
			break;

		case kPenUpSample:
			if (gLastPenState == 4)
			{
				outPt->x = -1.0;
				gLastPenState = 1;
				gLastUpTime = GetStrokerData(1);
				IncStrokerIndex(4);
				gLastPenState = 3;
//printf("pen lifted @ %u, duration %u\n", gLastUpTime, gLastUpTime - penDownTime);
				return true;
			}
			IncStrokerIndex(4);
			gLastPenState = 3;
			break;

		default:
			IncStrokerIndex(1);
			break;
		}
	}
	return false;
}


bool
LastTabPt(TabPt * inPt)
{
	return inPt->x == -1.0;
}


ULong
GetDownTime(void)
{
	ULong theTime = gLastDownTime;
	gLastDownTime = 0;
	return theTime;
}


ULong
GetUpTime(void)
{
	ULong theTime = gLastUpTime;
	gLastUpTime = 0;
	return theTime;
}

#pragma mark -

/* -----------------------------------------------------------------------------
	Initialize the stroke queue.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
StrokeInit(void)
{ }

void
RealStrokeInit(void)
{
	if (gStrokeQSemaphore == NULL)
	{
		gStrokeQSemaphore = new CULockingSemaphore;
		gStrokeQSemaphore->init();
	}
	gStrokeQSemaphore->acquire(kWaitOnBlock);

	CSStroke ** p = gStrokeQ.buf;
	for (ArrayIndex i = 0; i < kStrokeQSize; ++i, ++p)
		*p = NULL;
/*
	in the original, but redundant since this is done by StrokeReInit()
	gStrokeQ.buf[0] = CSStroke::make(0);
	gStrokeQ.rdIndex = 0;
	gStrokeQ.wrIndex = 0;
*/
	StrokeReInit();

	gStrokeQSemaphore->release();

	gStrokeSemaphore = new CULockingSemaphore;
	gStrokeSemaphore->init();

	InitHiliteGlobals(&oldHilite);
	SetUpDistances();
}


void
NukeEgregiousStrokes(FRect * ioBounds)
{/*INCOMPLETE*/}


void
StrokeUpdate(FRect * ioBounds)
{
	FRect aBox;
	CSStroke * buf[kStrokeQSize];
	ArrayIndex numOfGoodStrokes = 0;
	CSStroke * stroke;
	CSStroke ** p = gStrokeQ.buf;

	gStrokeSemaphore->acquire(kWaitOnBlock);
	for (ArrayIndex i = 0; i < kStrokeQSize; ++i, ++p)
	{
		if ((stroke = *p) != NULL)
		{
			if (stroke->testFlags(0x10000000) && stroke->count() > 0)
			{
				if (!stroke->testFlags(0x08000000) || SectRectangle(&aBox, ioBounds, stroke->bounds()))
				{
					buf[numOfGoodStrokes++] = stroke;
				}
			}
		}
	}
	gStrokeQSemaphore->release();
	
	for (ArrayIndex i = 0; i < numOfGoodStrokes; ++i)
	{
		buf[i]->draw();
	}
}


bool
AcquireStroke(CRecStroke * inStroke)
{
	if (inStroke->isDone())
		return false;
	gStrokeSemaphore->acquire(kWaitOnBlock);
	return true;
}


void
ReleaseStroke(void)
{
	gStrokeSemaphore->release();
}


void
StrokeReInit(void)
{
	ClearStrokeBuf();
	gStrokeQ.buf[0] = CSStroke::make(0);
	gStrokeQ.rdIndex = 0;
	gStrokeQ.wrIndex = 0;
	gStrokeValid = true;		// original doesn’t do this -- but surely it’s essential?!

	TabInit();
	if (useStylus)
		TabOn();
	GetTabScale(&gTabScale);
}


void
ClearStrokeBuf(void)
{
	CSStroke * stroke;
	CSStroke ** p = gStrokeQ.buf;
	for (ArrayIndex i = 0; i < kStrokeQSize; ++i, ++p)
	{
		if ((stroke = *p) != NULL)
		{
			stroke->release();
			*p = NULL;
		}
	}
}


void
UnbufferStroke(CRecStroke * inStroke)
{
	CSStroke ** p = gStrokeQ.buf;
	gStrokeQSemaphore->acquire(kWaitOnBlock);
	for (ArrayIndex i = 0; i < kStrokeQSize; ++i, ++p)
	{
		if (*p == (CSStroke *)inStroke)
			*p = NULL;
	}
	gStrokeQSemaphore->release();
}


bool
ScanStrokeQueueEvents(ULong inStartTime, ULong inDuration, bool inArg3)
{
	bool isFound = false;
	ULong limit = inStartTime + inDuration;
	CSStroke * stroke;
	CSStroke ** p = gStrokeQ.buf;

	gStrokeQSemaphore->acquire(kWaitOnBlock);
	for (ArrayIndex i = 0; i < kStrokeQSize; ++i, ++p)
	{
		if ((stroke = *p) != NULL
		&&  (inArg3 || !stroke->testFlags(0x80000000))
		&&  stroke->count() > 0
		&&  stroke->startTime() > inStartTime
		&&  stroke->startTime() < limit)
		{
			isFound = true;
			break;
		}
	}
	gStrokeQSemaphore->release();

	return isFound;
}


bool
CheckStrokeQueueEvents(ULong inStartTime, ULong inDuration)
{
	return ScanStrokeQueueEvents(inStartTime, inDuration, false);
}


CSStroke *
StrokeGet(void)
{
	CSStroke * theStroke = NULL;
	gStrokeQSemaphore->acquire(kWaitOnBlock);
	XTRY
	{
		StrokeTime();
		CSStroke * stroke = gStrokeQ.buf[gStrokeQ.rdIndex];
		if (stroke == NULL || stroke->testFlags(0x80000000))
		{
			XFAIL(gStrokeQ.rdIndex == gStrokeQ.wrIndex)
			if (++gStrokeQ.rdIndex >= kStrokeQSize)
				gStrokeQ.rdIndex -= kStrokeQSize;
			stroke = gStrokeQ.buf[gStrokeQ.rdIndex];
		}
		XFAIL(stroke == NULL)
		bool isAcquired = AcquireStroke(stroke);
		if (stroke->count() > 0)
		{
			stroke->setFlags(0x80000000);
			if (!gDefaultInk)
				stroke->setFlags(0x20000000);
			theStroke = stroke;
		}
		if (isAcquired)
			ReleaseStroke();
	}
	XENDTRY;
	gStrokeQSemaphore->release();
	return theStroke;
}


/* -----------------------------------------------------------------------------
	Create a new empty stroke and add it to the queue.
	Args:		--
	Return:	true => success
----------------------------------------------------------------------------- */

bool
StrokeNext(void)
{
	XTRY
	{
		CSStroke * stroke;
		// get index for next queue entry
		ArrayIndex i = gStrokeQ.wrIndex;
		if (++i >= kStrokeQSize)
			i -= kStrokeQSize;
		// if there’s already a stroke there then the queue is full
		XFAIL((stroke = gStrokeQ.buf[i]) != NULL)
		// create a new stroke
		XFAIL((stroke = CSStroke::make(0)) == NULL)
		stroke->setFlags(0x10000000);
		// add it to the queue
		gStrokeQ.buf[i] = stroke;
		gStrokeQ.wrIndex = i;
		gStrokeValid = true;
		return true;
	}
	XENDTRY;
	gStrokeValid = false;
	return false;
}


void
StrokeTime(void)
{ }

int
RealStrokeTime(void)
{
	TabPt penLoc;
	CSStroke * stroke = NULL;
	int status = 0;
	gStrokeSemaphore->acquire(kWaitOnBlock);
	while (status == 0 && GetTabPt(&penLoc))
	{
		if (gStrokeValid)
		{
			stroke = gStrokeQ.buf[gStrokeQ.wrIndex];
			if (stroke == NULL)
				gStrokeValid = false;
		}
		if (LastTabPt(&penLoc))
		{
			// pen has been lifted
			gStrokeInProgress = false;
			if (gStrokeValid)
			{
				status = 1;
				stroke->setEndTime(GetUpTime() + gTickOff);
				CheckHiliteState(stroke, &oldHilite, &newHilite, true);
				oldHilite = newHilite;
				oldHilite.x0C = stroke->endTime();
				if (stroke->count() == 0)
					printf("<<<< no point stroke\n");
				else
					StrokeNext();
				stroke->endStroke();
			}
			else
			{
				GetUpTime();
				StrokeNext();
			}
		}
		else
		{
			// pen is down
			if (gStrokeValid)
			{
				if (!gStrokeInProgress)
				{
					gStrokeInProgress = true;
					status = 1;
				}
				stroke->addPoint(&penLoc);
				if (stroke->startTime() == 0)
				{
					// this is the first point in the stroke
					stroke->setStartTime(GetDownTime() + gTickOff);
					stroke->unsetFlags(0x000000FF << 8);
					stroke->setFlags(gLastPenTip << 8);
					InitHiliteState(stroke, &oldHilite, &newHilite);
				}
				else
				{
					// update duration of current stroke
					stroke->setEndTime(stroke->startTime() + stroke->count() * gSamplesToTicks);
					// check for hilite gesture -- holding down the pen
					CheckHiliteState(stroke, &oldHilite, &newHilite, false);
				}
			}
			else
			{
				if (!gStrokeInProgress)
				{
					GetDownTime();
					gStrokeInProgress = true;
					status = 2;
				}
			}
		}
	}
	gStrokeSemaphore->release();
	return status;
}


#pragma mark -

void
InitHiliteGlobals(StrokeHiliteState * ioState)
{
	ioState->x18 = 0;
	ioState->x10 = 1;
	ioState->x13 = 3;
	oldHilite.x0C = 0;
	ioState->x08 = 0;
}


void
InitHiliteState(CRecStroke * inStroke, StrokeHiliteState * inPrevState, StrokeHiliteState * outNewState)
{
	if (inPrevState->x13 != 0 && inPrevState->x18 == 0)
		InitHiliteGlobals(inPrevState);

	outNewState->x08 = inStroke->startTime();
	outNewState->x0C = 0;
	inStroke->getFPoint(0, &outNewState->x00);
	outNewState->x13 = 0;

	if (inPrevState->x18 == 1
	&&  outNewState->x08 - inPrevState->x0C < gDoubleTapInterval)
	{
		outNewState->x11 = 0;
	}
	else
	{
		outNewState->x11 = 1;
		outNewState->x13 = 1;
	}

	if (outNewState->x08 - inPrevState->x0C <= 40
	&&  inPrevState->x18 == 5)
	{
		outNewState->x10 = 1;
		outNewState->x13++;
		outNewState->x12 = 1;
		outNewState->x13++;
	}
	else
	{
		outNewState->x10 = 0;
		outNewState->x12 = 0;
	}
	outNewState->x14 = 1;
	outNewState->x18 = 0;
}


int
CheckHiliteState(CRecStroke * inStroke, StrokeHiliteState * inPrevState, StrokeHiliteState * outNewState, bool inFlag)
{
	outNewState->x0C = inStroke->endTime();
	outNewState->x14++;
	if (outNewState->x18 != 0)
		return 0;

	FPoint pt;
	inStroke->getFPoint(inStroke->count() - 1, &pt);
	float delta = DistPoint(&outNewState->x00, &pt);
	ULong r2 = outNewState->x08 + outNewState->x14;
	if (outNewState->x12 == 0)
	{
		if (delta < gHiliteDistance * 2)
		{
			if ((outNewState->x08 + 45 < r2 && delta < gHiliteDistance)
			||  (outNewState->x08 + 90 < r2 && delta < gHiliteDistance * 2))
			{
				// start hiliting
				inStroke->setTapEvent(4);
				outNewState->x18 = 2;
				return 1;
			}
		}
		else
		{
			outNewState->x12 = 1;
			outNewState->x13++;
		}
	}

	if (outNewState->x10 == 0 || outNewState->x11 == 0)
	{
		if (delta < gMaxTapSize
		&&  outNewState->x0C - outNewState->x08 < 10)
		{
			if (inFlag)
			{
				GetMidPoint(&outNewState->x00, &pt, &outNewState->x00);
				if (outNewState->x11 == 0
				&&  DistPoint(&inPrevState->x00, &outNewState->x00) < gDoubleTapDistance)
				{
					inStroke->setTapEvent(3);
					outNewState->x18 = 3;
					return 1;
				}
				inStroke->setTapEvent(2);
				outNewState->x18 = 1;
				return 1;
			}
		}
		else
		{
			if (outNewState->x10 == 0)
			{
				outNewState->x10 = 1;
				outNewState->x13++;
			}
			if (outNewState->x11 == 0)
			{
				outNewState->x11 = 1;
				outNewState->x13++;
				if (!inFlag && inStroke->getTapEvent() == 0 && DistPoint(&inPrevState->x00, &outNewState->x00) < gDoubleTapDistance)
				{
					inStroke->setTapEvent(5);
					outNewState->x18 = 4;
					return 1;
				}
			}
		}
	}
	
	if (outNewState->x13 == 3)
	{
		inStroke->setTapEvent(1);
		outNewState->x18 = 5;
	}
	return 0;
}


void
SetUpDistances(void)
{
	CUGestalt gestalt;
	GestaltSystemInfo sysInfo;
	gestalt.gestalt(kGestalt_SystemInfo, &sysInfo, sizeof(sysInfo));
	float aveRes = (sysInfo.fScreenResolution.h + sysInfo.fScreenResolution.v) / 2.0;
	float displayPts = aveRes / 72.0;
	// distances are in display points
	gHiliteDistance = (4 * displayPts) + 0.5;
	gMaxTapSize = (6 * displayPts) + 0.5;
	gDoubleTapDistance = (6 * displayPts) + 0.5;
	gSamplesToTicks = (GetSampleRate() / kSeconds) / 60.0;	// ticks between samples
}


#pragma mark - CSStroke
/* -----------------------------------------------------------------------------
	C S S t r o k e
	Don’t really know why this is needed -- the extra data members aren’t used.
----------------------------------------------------------------------------- */

/*------------------------------------------------------------------------------
	Make a new CSStroke.
	Args:		inNumOfPts			number of points the stroke should hold
	Return:	new CSStroke
------------------------------------------------------------------------------*/

CSStroke *
CSStroke::make(ULong inNumOfPts)
{
	CSStroke * stroke = new CSStroke;
	XTRY
	{
		XFAIL(stroke == NULL)
		XFAILIF(stroke->iStroke(inNumOfPts) != noErr, stroke->release(); stroke = NULL;)
		stroke->f54 = false;
		stroke->f4C.h = stroke->f4C.v = -1;
		stroke->f50.h = stroke->f50.v = -1;
	}
	XENDTRY;
	return stroke;
}


NewtonErr
CSStroke::addPoint(TabPt * inPt)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILIF(CRecStroke::addPoint(inPt), err = 1;)	// why not just return the err?

		if (count() == 1)
		{
			f4C.h = inPt->x + 0.5;
			f4C.v = inPt->y + 0.5;
		}
		if (!f54)
		{
			f50.h = inPt->x + 0.5;
			f50.v = inPt->y + 0.5;
			f54 = true;
		}
	}
	XENDTRY;
	return err;
	
}

