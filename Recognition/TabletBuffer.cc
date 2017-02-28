/*
	File:		TabletBuffer.cc

	Contains:	Tablet input queue implementation.

	Written by:	Newton Research Group, 2010.
*/

#include "TabletBuffer.h"
#include "Inker.h"
#include "Objects.h"
#include "SharedTypes.h"
#include "ROMResources.h"
#include "NewtonErrors.h"

extern "C" void	EnterAtomic(void);
extern "C" void	ExitAtomic(void);
extern ULong		GetTicks(void);


/* -----------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
----------------------------------------------------------------------------- */

extern "C" {
Ref	FInsertTabletSample(RefArg inRcvr, RefArg inX, RefArg inY, RefArg inZ, RefArg inTime);
Ref	FTabletBufferEmpty(RefArg inRcvr);

Ref	FPenPos(RefArg inRcvr);
}


/* -----------------------------------------------------------------------------
	T y p e s
----------------------------------------------------------------------------- */

#define kTabletBufferSize 250

struct TabletQueue
{
	ArrayIndex		fillerIndex;		// +00
	ArrayIndex		inkerIndex;			// +04
	ArrayIndex		strokerIndex;		// +08
	ULong				buffer[kTabletBufferSize];	// +0C	mostly TabletSample
	CUPort *			port;					// +03F4
	CUAsyncMessage *	message;			// +03F8
	CInkerEvent *	event;				// +x03FC
};


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

TabletQueue		gTabletQueue = { 0,0,0 };		// was gTabData
bool				gTBCOnlyPollTablet;
bool				gTBCPollReady;
TabletSample	gTBCPollSample;
bool				gTBCPenUp;
bool				gTBCBypassTablet;


#pragma mark TabletBuffer
/*------------------------------------------------------------------------------
	T a b l e t   B u f f e r

	The tablet generates a GPIO interrupt when the pen goes down or after a 
	sampling interval. The tablet driver handles the interrupt and causes a
	sample to be entered into the buffer:
	TabPenEntry() -> TBCWakeUpInkerFromInterrupt(0ms) -> SendForInterrupt() | gTabletQueue.port->send()
----------------------------------------------------------------------------- */

NewtonErr
/*TBC*/TabletBufferInit(CUPort * inPort)
{
	gTabletQueue.port = inPort;
	gTabletQueue.event = new CInkerEvent(2);	// size +10
	gTabletQueue.message = new CUAsyncMessage;
	gTabletQueue.message->init();
	/*TBC*/FlushTabletBuffer();

	gTBCOnlyPollTablet = false;
	gTBCPollReady = false;
	gTBCPollSample.intValue = 0;
	gTBCPenUp = true;
	gTBCBypassTablet = false;

	return noErr;
}


void
/*TBC*/FlushTabletBuffer(void)
{
	gTabletQueue.inkerIndex = gTabletQueue.fillerIndex;
	gTabletQueue.strokerIndex = gTabletQueue.fillerIndex;
}


bool
/*TBC*/TabletBufferEmpty(void)
{
	return gTabletQueue.inkerIndex == gTabletQueue.fillerIndex
		 && gTabletQueue.strokerIndex == gTabletQueue.fillerIndex;
}


#pragma mark Polling
/* -----------------------------------------------------------------------------
	The tablet can be polled directly, rather than dequeueing samples.
----------------------------------------------------------------------------- */

void
/*TBC*/SetTabletPolling(bool inPolling)
{
	gTBCOnlyPollTablet = inPolling;
	gTBCPollReady = false;
}


bool
/*TBC*/GetTabletPolling(void)
{
	return gTBCOnlyPollTablet;
}


NewtonErr
/*TBC*/PollTablet(float *  outX, float *  outY, ULong *  outZ, bool * outPenUp)
{
	NewtonErr err = noErr;
	EnterAtomic();
	if (outX)
		*outX = gTBCPollSample.x / kTabScale;
	if (outY)
		*outY = gTBCPollSample.y / kTabScale;
	if (outZ)
		*outZ = gTBCPollSample.z;
	if (outPenUp)
		*outPenUp = gTBCPenUp;
	if (gTBCPollReady)
		gTBCPollReady = false;
	else
		err = -56007;
	ExitAtomic();
	return err;
}


#pragma mark Messaging
/* -----------------------------------------------------------------------------
	The CInker object that reads samples from the queue can be awoken to handle
	events.
----------------------------------------------------------------------------- */

void
/*TBC*/WakeUpInkerFromInterrupt(Timeout inDelay)
{
	CTime timeFromNow;
	CTime * timeToSend = NULL;
	if (inDelay != kNoTimeout)
	{
		timeFromNow = TimeFromNow(inDelay*kMilliseconds);
		timeToSend = &timeFromNow;
	}
	SendForInterrupt(*gTabletQueue.port, gTabletQueue.message->getMsgId(), kNoId, gTabletQueue.event, sizeof(CInkerEvent), kMsgType_FromInterrupt, kNoTimeout, timeToSend, true);
}


void
/*TBC*/WakeUpInker(Timeout inDelay)
{
	CTime timeFromNow;
	CTime * timeToSend = NULL;
	if (inDelay != kNoTimeout)
	{
		timeFromNow = TimeFromNow(inDelay*kMilliseconds);
		timeToSend = &timeFromNow;
	}
	//gTabletQueue.event->fSelector = 2;	// sic
	gTabletQueue.port->send(gTabletQueue.message, gTabletQueue.event, sizeof(CInkerEvent), kNoTimeout, timeToSend, 1, true);
}


#pragma mark Insertion
/* -----------------------------------------------------------------------------
	Insert a sample into the tablet buffer.
	Samples can be pen location or time values.
	Args:		inSample			0x0D => append time -- pen down
									0x0E => append time and two value  -- pen was lifted
				inTime			0 => use current time
	Return:	error code
----------------------------------------------------------------------------- */
#define INCR(ix)	if (++ix >= kTabletBufferSize) ix -= kTabletBufferSize; \
						XFAIL(isFull = (ix == gTabletQueue.strokerIndex))

NewtonErr
/*TBC*/InsertTabletSample(ULong inSample, ULong inTime)
{
	if (!gTBCOnlyPollTablet)
	{
		bool isFull;
		ArrayIndex i1, i2, i3, index;
		ULong now = GetTicks();

		XTRY
		{
			// add the sample to the buffer
			index = gTabletQueue.fillerIndex;
			gTabletQueue.buffer[index] = inSample;
			INCR(index)

			if (inSample == kPenDownSample)
			{
				// add the time the pen went down
				i1 = index;
				INCR(index)
				gTabletQueue.buffer[i1] = (inTime) ? inTime : GetTicks();
			}

			else if (inSample == kPenUpSample)
			{
				// add the time the pen was lifted and a pair of points
				i1 = index;
				INCR(index)
				i2 = index;
				INCR(index)
				i3 = index;
				INCR(index)
				gTabletQueue.buffer[i1] = (inTime) ? inTime : GetTicks();
				gTabletQueue.buffer[i2] = 0x14000000;	// 5120.0 ?
				gTabletQueue.buffer[i3] = 0x14000000;
			}
		}
		XENDTRY;

		XDOFAIL(isFull)
			return -56006;
		XENDFAIL;

		gTabletQueue.fillerIndex = index;
	}

	if (inSample != kPenInvalidSample)
	{
		gTBCPollSample.intValue = inSample;
		if (gTBCPollSample.z == kPenUpSample)
		{
			// pen was lifted
			if (!gTBCPenUp)
			{
				gTBCPenUp = true;
				gTBCPollReady = true;
			}
		}
		else if (gTBCPollSample.z == kPenDownSample)
		{
			// pen was applied
			gTBCPenUp = false;
		}
		else if (gTBCPollSample.z < 8)
		{
			// pen is writing
			gTBCPenUp = false;
			gTBCPollReady = true;
		}
	}

	return noErr;
}


NewtonErr
InsertAndSendTabletSample(ULong inSample, ULong inTime)
{
	NewtonErr err = /*TBC*/InsertTabletSample(inSample, inTime);
	/*TBC*/WakeUpInker(0);
	return err;
}


#pragma mark Inker
/*------------------------------------------------------------------------------
	I n k e r
	Use points from the tablet data buffer to draw ink.
------------------------------------------------------------------------------*/

ULong
/*TBC*/GetInkerData(void)
{
	return gTabletQueue.buffer[gTabletQueue.inkerIndex];
}


ULong
/*TBC*/SetInkerData(ULong inData)
{
	return gTabletQueue.buffer[gTabletQueue.inkerIndex] = inData;
}


ULong
/*TBC*/SetInkerData(ULong inData, ULong incr)
{
	ArrayIndex index = gTabletQueue.inkerIndex + incr;
	if (index >= kTabletBufferSize)
		index -= kTabletBufferSize;
	return gTabletQueue.buffer[index] = inData;
}


void
/*TBC*/IncInkerIndex(ULong incr)
{
	ArrayIndex index = gTabletQueue.inkerIndex + incr;
	if (index >= kTabletBufferSize)
		index -= kTabletBufferSize;
	gTabletQueue.inkerIndex = index;
}


bool
/*TBC*/InkerBufferEmpty(void)
{
	return gTabletQueue.inkerIndex == gTabletQueue.fillerIndex;
}


void
/*TBC*/FlushInkerBuffer(void)
{
	gTabletQueue.strokerIndex = gTabletQueue.inkerIndex;
}

#pragma mark Stroker
/*------------------------------------------------------------------------------
	S t r o k e r

	We also use points from the tablet data buffer to assemble into strokes.
	GetTabPt
	->	xGetTabPt
		->	GetStrokerData
------------------------------------------------------------------------------*/

ULong
/*TBC*/GetStrokerData(void)
{
	return gTabletQueue.buffer[gTabletQueue.strokerIndex];
}


ULong
/*TBC*/GetStrokerData(ULong incr)
{
	ArrayIndex index = gTabletQueue.strokerIndex + incr;
	if (index >= kTabletBufferSize)
		index -= kTabletBufferSize;
	return gTabletQueue.buffer[index];
}


void
/*TBC*/IncStrokerIndex(ULong incr)
{
	ArrayIndex index = gTabletQueue.strokerIndex + incr;
	if (index >= kTabletBufferSize)
		index -= kTabletBufferSize;
	gTabletQueue.strokerIndex = index;
}


bool
/*TBC*/StrokerBufferEmpty(void)
{
	return gTabletQueue.strokerIndex == gTabletQueue.inkerIndex;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
----------------------------------------------------------------------------- */

Ref
FInsertTabletSample(RefArg inRcvr, RefArg inX, RefArg inY, RefArg inZ, RefArg inTime)
{
	TabletSample sample;
	sample.x = RINT(inX);
	sample.y = RINT(inY);
	sample.z = RINT(inZ);
	ULong time = RINT(inTime);
	return MAKEINT(InsertAndSendTabletSample(sample.intValue, time));
}


Ref
FTabletBufferEmpty(RefArg inRcvr)
{
	return MAKEBOOLEAN(TabletBufferEmpty());
}


Ref
FPenPos(RefArg inRcvr)
{
	float		x;		// was Fixed
	float		y;
	ULong		z;
	bool	isPenUp = true;
	PollTablet(&x, &y, &z, &isPenUp);
	if (!isPenUp)
	{
		RefVar	pos(Clone(RA(canonicalPoint)));
		SetFrameSlot(pos, SYMA(x), MAKEINT(x + 0.5));
		SetFrameSlot(pos, SYMA(y), MAKEINT(y + 0.5));
		return pos;
	}
	return NILREF;
}

