/*
	File:		TimerQueue.cc

	Contains:	Implementation of the CTimerQueue class.

	Written by:	Newton Research Group.
*/

#include "TimerQueue.h"
#include "OSErrors.h"


/*--------------------------------------------------------------------------------
	C T i m e r E l e m e n t
	An element held in the timer queue.
--------------------------------------------------------------------------------*/

CTimerElement::CTimerElement(CTimerQueue * inQ, ULong inRefCon)
{
	fQueue = inQ;
	fDelta = kNoTimeout;
	fPrimed = false;
	fNext = NULL;
	fRefCon = inRefCon;
}


CTimerElement::~CTimerElement()
{
	cancel();
}


/*--------------------------------------------------------------------------------
	Prime the element to fire at a future time.
	Args:		inDelta			offset to time to fire
	Return:	true => success
--------------------------------------------------------------------------------*/

bool
CTimerElement::prime(Timeout inDelta)
{
	if (fQueue && inDelta)
	{
		if (fPrimed)
			fQueue->dequeue(this, true);
		fDelta = inDelta;
		fQueue->calibrate();
		fQueue->enqueue(this);
	}
	return fPrimed;
}


/*--------------------------------------------------------------------------------
	Cancel the element -- stop it from firing.
	Args:		--
	Return:	true => success
--------------------------------------------------------------------------------*/

bool
CTimerElement::cancel(void)
{
	if (fPrimed && fQueue)
		fQueue->dequeue(this, true);
	return true;
}


#pragma mark -
/*--------------------------------------------------------------------------------
	C T i m e r Q u e u e
	A queue that holds timer elements.
--------------------------------------------------------------------------------*/

CTimerQueue::CTimerQueue()
{
	fHead = NULL;
	fLastCalibrate = GetGlobalTime();
	fTimeoutInProgress = false;
}


CTimerQueue::~CTimerQueue()
{ }


/*--------------------------------------------------------------------------------
	Check the queue for any elements that have already expired.
	**** This function patched in System Update 717260 ****
	Args:		--
	Return:	Timeout to next firing
				kNoTimeout => nothing remaining in the queue
--------------------------------------------------------------------------------*/

Timeout
CTimerQueue::check(void)
{
	if (!isEmpty())
	{
		calibrate();

		fTimeoutInProgress = true;
		for (CTimerElement * element = fHead, * next; element != NULL; element = next)
		{
			if (element->fDelta > 4*kMicroseconds)
				break;
			next = element->fNext;
			dequeue(element, false);
			element->timeout();
		}
		fTimeoutInProgress = false;

		if (fHead)
			return fHead->fDelta;
	}
	return kNoTimeout;
}


/*--------------------------------------------------------------------------------
	Cancel a timer element in the queue.
	Args:		inRefCon			identifier for the timer element
	Return:	the element cancelled
				NULL => wasn’t found
--------------------------------------------------------------------------------*/

CTimerElement *
CTimerQueue::cancel(ULong inRefCon)
{
	CTimerElement *	element, * next, * prev = NULL;
	for (element = fHead; element != NULL; element = element->fNext)
	{
		if (element->getRefCon() == inRefCon)
		{
			if ((next = element->fNext)) // intentional assignment
				next->fDelta += element->fDelta;
			if (prev)
				prev->fNext = element->fNext;
			else
				fHead = element->fNext;
			element->fNext = NULL;
			element->fPrimed = false;
			break;
		}
		prev = element;
	}
	return element;
}


/*--------------------------------------------------------------------------------
	Enqueue an element to fire at a future time.
	Args:		inItem			the timer element
	Return:	the element
--------------------------------------------------------------------------------*/

CTimerElement *
CTimerQueue::enqueue(CTimerElement * inItem)
{
	CTimerElement *	element = NULL;
	if (inItem && inItem->fQueue == this && inItem->fDelta)
	{
		CTimerElement *	prev = NULL;
		for (element = fHead; element != NULL; element = element->fNext)
		{
			Timeout  itemDelta = inItem->fDelta;
			Timeout  elemDelta = element->fDelta;
			Timeout  delta;
			if (itemDelta < elemDelta)
				break;
			delta = itemDelta - elemDelta;
			inItem->fDelta = MAX(4*kMicroseconds, delta);
			prev = element;
		}
		inItem->fNext = element;
		if (prev)
			prev->fNext = inItem;
		else
			fHead = inItem;
		if (element)
		{
			Timeout  delta = element->fDelta - inItem->fDelta;
			inItem->fDelta = MAX(4*kMicroseconds, delta);
		}
		inItem->fPrimed = true;
		element = inItem;
//printf("CTimerQueue::enqueue(item.delta = %d)\n", inItem->fDelta);
	}
	return element;
}


/*--------------------------------------------------------------------------------
	Remove an element from the queue.
	Args:		inItem			the item to remove
				inAdjust			true => account for the element’s timeout so that future elements still fire at the same time
	Return:	the element
--------------------------------------------------------------------------------*/

CTimerElement *
CTimerQueue::dequeue(CTimerElement * inItem, bool inAdjust)
{
	CTimerElement *	element = NULL;
	if (inItem && inItem->fQueue == this)
	{
		CTimerElement *	next, * prev = NULL;
		for (element = fHead; element != NULL; element = element->fNext)
		{
			if (element == inItem)
			{
				if (inAdjust && (next = element->fNext)) // intentional assignment
					next->fDelta += element->fDelta;
				if (prev)
					prev->fNext = element->fNext;
				else
					fHead = element->fNext;
				element->fNext = NULL;
				element->fPrimed = false;
				break;
			}
			prev = element;
		}
	}
	return element;
}


/*--------------------------------------------------------------------------------
	Calibrate the queue -- reduce timeouts of elements according to elapsed time
	since last calibration.
	NOTE	All instances of 4*kMicroseconds appear to be 1*kMicroseconds
			in the original.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CTimerQueue::calibrate(void)
{
	if (!fTimeoutInProgress)
	{
		CTime		now = GetGlobalTime();
		Timeout	offset = now - fLastCalibrate;
		for (CTimerElement * element = fHead; element != NULL; element = element->fNext)
		{
			if (element->fDelta >= offset)
			{
				offset = element->fDelta - offset;
				if (offset < 4*kMicroseconds)
					offset = 4*kMicroseconds;
				element->fDelta = offset;
				break;
			}
			else
			{
				offset = offset - element->fDelta;
				element->fDelta = 4*kMicroseconds;
				if (offset <= 4*kMicroseconds)
					break;
			}
		}
		fLastCalibrate = now;
	}
}


#pragma mark -
/*--------------------------------------------------------------------------------
	C T i m e r P o r t
	A (user) port that performs timed receives.
	Doesn’t appear to be used anywhere.
--------------------------------------------------------------------------------*/

CTimerPort::CTimerPort()
{
	fQueue = NULL;
}


CTimerPort::~CTimerPort()
{
	if (fQueue)
		delete fQueue;
}


/*--------------------------------------------------------------------------------
	Initialize the port and a queue to hold timed elements.
	Args:		--
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CTimerPort::init(void)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = CUPort::init())
		fQueue = new CTimerQueue;
		XFAILIF(fQueue == NULL, err = MemError();)
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Receive.
	Args:		outSize
				content
				inSize
				token
				outMsgType
				inMsgFilter
				onMsgAvail
				tokenOnly
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CTimerPort::timedReceive(	size_t * outSize,
									void * content,
									size_t inSize,
									CUMsgToken * token,
									ULong * outMsgType,
									ULong inMsgFilter,
									bool onMsgAvail,
									bool tokenOnly)
{
	NewtonErr  err;
	do
	{
		err = receive(outSize, content, inSize, token, outMsgType, fQueue->check(), inMsgFilter, onMsgAvail, tokenOnly);
	} while (err == kOSErrMessageTimedOut);
	return err;
}
