/*
	File:		Queues.cc

	Contains:	Support for single and doubly linked lists.

	Written by:	Newton Research Group.
*/

#include "SingleQ.h"
#include "DoubleQ.h"

/*------------------------------------------------------------------------------
	C S i n g l e Q C o n t a i n e r
------------------------------------------------------------------------------*/

CSingleQContainer::CSingleQContainer()
{
	init(0);
}

CSingleQContainer::CSingleQContainer(ULong inOffsetToSingleQItem)
{
	init(inOffsetToSingleQItem);
}

void
CSingleQContainer::init(ULong inOffsetToSingleQItem)
{
	fOffsetToSingleQItem = inOffsetToSingleQItem;
	fHead = NULL;
}

void
CSingleQContainer::add(void * inItem)
{
	CSingleQItem *	qItem = (CSingleQItem *)((Ptr)inItem + fOffsetToSingleQItem);
	qItem->fLink = fHead;
	fHead = qItem;
}

void *
CSingleQContainer::remove()
{
	CSingleQItem *	qItem = fHead;
	if (qItem)
		fHead = qItem->fLink;
	if (qItem)
		return (Ptr)qItem - fOffsetToSingleQItem;
	return NULL;
}

void *
CSingleQContainer::peek()
{
	if (fHead)
		return (Ptr)fHead - fOffsetToSingleQItem;
	return NULL;
}

void *
CSingleQContainer::getNext(void * inItem)
{
	CSingleQItem *	qItem;
	if (inItem && (qItem = ((CSingleQItem *)((Ptr)inItem + fOffsetToSingleQItem))->fLink))
		return (Ptr)qItem - fOffsetToSingleQItem;
	return NULL;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C D o u b l e Q I t e m
------------------------------------------------------------------------------*/

CDoubleQItem::CDoubleQItem()
	:	fNext(NULL), fPrev(NULL), fContainer(NULL)
{ }

#pragma mark -

/*------------------------------------------------------------------------------
	C D o u b l e Q C o n t a i n e r
------------------------------------------------------------------------------*/

CDoubleQContainer::CDoubleQContainer()
{
	init(0);
}

CDoubleQContainer::CDoubleQContainer(ULong inOffsetToDoubleQItem)
{
	init(inOffsetToDoubleQItem);
}

CDoubleQContainer::CDoubleQContainer(ULong inOffsetToDoubleQItem, DestructorProcPtr inDestructor, void * inInstance)
{
	init(inOffsetToDoubleQItem);
	fDestructor = inDestructor;
	fDestructorInstance = inInstance;
}

void
CDoubleQContainer::init(ULong inOffsetToDoubleQItem)
{
	fHead =
	fTail = NULL;
	fOffsetToDoubleQItem = inOffsetToDoubleQItem;
	fDestructor = NULL;
	fDestructorInstance = NULL;
}


void
CDoubleQContainer::add(void * inItem)
{
	checkBeforeAdd(inItem);

	// point to queue info within the object weÕre adding
	CDoubleQItem *	qItem = (CDoubleQItem *)((Ptr)inItem + fOffsetToDoubleQItem);
	// our object is added to the end of the queue, so thereÕs nothing after it
	qItem->fNext = NULL;
	if (fHead == NULL)
	{
		// thereÕs nothing in the queue yet so our object is at the head
		fHead = qItem;
		// and thereÕs nothing before our object
		qItem->fPrev = NULL;
	}
	else
	{
		// thread our object onto the queueÕs tail object
		fTail->fNext = qItem;
		qItem->fPrev = fTail;
	}
	// our object is now the tail object
	fTail = qItem;
	qItem->fContainer = this;
}

void
CDoubleQContainer::addBefore(void * inBeforeItem, void * inItem)
{
	checkBeforeAdd(inItem);

	CDoubleQItem *	beforeItem = (CDoubleQItem *)((Ptr)inBeforeItem + fOffsetToDoubleQItem);
	CDoubleQItem *	qItem = (CDoubleQItem *)((Ptr)inItem + fOffsetToDoubleQItem);
	if (fHead == NULL || fHead == beforeItem)
		// thereÕs nothing in the queue, or we want it before the first already in the queue
		// so our object must go to the front
		addToFront(inItem);
	else
	{
		// thread our object before the other one
		qItem->fNext = beforeItem;
		qItem->fPrev = beforeItem->fPrev;
		beforeItem->fPrev = qItem;
		qItem->fPrev->fNext = qItem;
		qItem->fContainer = this;
	}
}

void
CDoubleQContainer::addToFront(void * inItem)
{
	checkBeforeAdd(inItem);

	CDoubleQItem *	qItem = (CDoubleQItem *)((Ptr)inItem + fOffsetToDoubleQItem);
	// our object is added to the front of the queue, so thereÕs nothing before it
	qItem->fPrev = NULL;
	if (fHead == NULL)
	{
		// the queue is empty -- we must be the tail as well as the head
		fTail = qItem;
		// and thereÕs nothing after our object
		qItem->fNext = NULL;
	}
	else
	{
		// thereÕs already an object at the head of the queue (think of fHead = headItem)
		// so thread our object before that
		fHead->fPrev = qItem;
		qItem->fNext = fHead;
	}
	// our object is now head of the queue
	fHead = qItem;
	qItem->fContainer = this;
}

void
CDoubleQContainer::checkBeforeAdd(void * inItem)
{ /* this really does nothing */ }


void *
CDoubleQContainer::peek(void)
{
	return fHead ? (void *)((Ptr)fHead - fOffsetToDoubleQItem) : NULL;
}

void *
CDoubleQContainer::getNext(void * inItem)
{
	CDoubleQItem *	qItem = (CDoubleQItem *)((Ptr)inItem + fOffsetToDoubleQItem);
	if (inItem && qItem->fContainer == this && qItem->fNext)
		return ((Ptr)qItem->fNext - fOffsetToDoubleQItem);
	return NULL;
}


void *
CDoubleQContainer::remove(void)
{
	CDoubleQItem *	qItem = fHead;
	if (qItem)
	{
		// there was an object at the head of the queue
		CDoubleQItem *	qNext = qItem->fNext;
		// we have a new head
		fHead = qNext;
		if (qNext) qNext->fPrev = NULL; else fTail = NULL;
		// remove (the now invalid) queue item pointers
		qItem->fNext =
		qItem->fPrev = NULL;
		qItem->fContainer = NULL;
		return ((Ptr)qItem - fOffsetToDoubleQItem);
	}
	return NULL;
}


bool
CDoubleQContainer::removeFromQueue(void * inItem)
{
	CDoubleQItem *	qItem = (CDoubleQItem *)((Ptr)inItem + fOffsetToDoubleQItem);
	if (inItem && qItem->fContainer == this)
	{
		CDoubleQItem *	qItemNext = qItem->fNext;
		CDoubleQItem *	qItemPrev = qItem->fPrev;
		if (qItem == fHead)
		{
			// our object was at the front of the queue
			if (fTail == fHead)
				fHead = fTail = NULL;
			else
			{
				fHead = qItemNext;
				qItem->fPrev = NULL;
			}
		}
		else
		{
			// unthread our object
			qItemPrev->fNext = qItemNext;
			if (fTail == qItem)
				fTail = qItemPrev;
			else
				qItemNext->fPrev = qItemPrev;
		}
		// remove (the now invalid) queue item pointers
		qItem->fNext =
		qItem->fPrev = NULL;
		qItem->fContainer = NULL;
		return true;
	}
	return false;
}


bool
CDoubleQContainer::deleteFromQueue(void * inItem)
{
	if (removeFromQueue(inItem))
	{
		if (fDestructor)
			fDestructor(fDestructorInstance, inItem);
		return true;
	}
	return false;
}
