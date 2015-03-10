/*
	File:		List.cc

	Contains:	List container and iterator classes.

	Written by:	Newton Research Group.
*/

#include "List.h"
#include "ListLoop.h"
#include "ListIterator.h"
#include "ItemTester.h"
#include "ItemComparer.h"


/*------------------------------------------------------------------------------
	C L i s t
------------------------------------------------------------------------------*/

CList *
CList::make(ArrayIndex inSize)
{
	return new CList;
}


CList *
CList::make(void)
{
	return new CList;
}


CList::CList()
	:	CDynamicArray(kDefaultElementSize, kDefaultChunkSize)
{ }


CList::~CList()
{ }


void *
CList::at(ArrayIndex index)
{
	void ** elementPtr = (void **) safeElementPtrAt(index);
	if (elementPtr)
		return *elementPtr;
	return NULL;
}


NewtonErr
CList::insertAt(ArrayIndex index, void * inItem)
{
	void *	element = inItem;
	return insertElementsBefore(index, &element, 1);
}


NewtonErr
CList::replaceAt(ArrayIndex index, void * inItem)
{
	void *	element = inItem;
	return replaceElementsAt(index, &element, 1);
}


NewtonErr
CList::replace(void * inOldItem, void * inNewItem)
{
	ArrayIndex	index = getIdentityIndex(inOldItem);
	return (index == kIndexNotFound) ? kUCErrRangeCheck : replaceAt(index, inNewItem);
}


bool
CList::insertUnique(void * inItem)
{
	ArrayIndex	index = getIdentityIndex(inItem);
	bool	isUnique = (index == kIndexNotFound);
	if (isUnique)
		insertAt(count(), inItem);
	return isUnique;
}


NewtonErr
CList::remove(void * inItem)
{
	long	index = getIdentityIndex(inItem);
	return (index == -1) ? kUCErrRangeCheck : removeElementsAt(index, 1);
}


ArrayIndex
CList::getIdentityIndex(void * inItem)
{
	ArrayIndex		index;
	CItemComparer	test(inItem, NULL);
	search(&test, index);
	return index;
}


void *
CList::search(CItemTester * inComparator, ArrayIndex & outIndex)
{
	void *			result = NULL;
	void *			anItem;
	CListIterator	iter(this);
	outIndex = kIndexNotFound;
	for (anItem = iter.firstItem(); iter.more(); anItem = iter.nextItem())
	{
		if (inComparator->testItem(anItem) == kItemEqualCriteria)
		{
			result = anItem;
			outIndex = iter.fCurrentIndex;
			break;
		}
	}
	return result;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S o r t e d L i s t
------------------------------------------------------------------------------*/

CSortedList::CSortedList(CItemComparer * inComparer)
	:	CList()
{
	fComparer = inComparer;
}


CSortedList::~CSortedList()
{ }


NewtonErr
CSortedList::insert(void * inItem)
{
	NewtonErr	err;
	ArrayIndex	index;
	void *		existingItem;

	fComparer->setTestItem(inItem);
	existingItem = search(fComparer, index);
	if (existingItem != NULL)
		err = insertDuplicate(index, existingItem, inItem);
	else
		err = insertAt(index, inItem);
	return err;
}


bool
CSortedList::insertUnique(void * inItem)
{
	ArrayIndex	index;
	bool			isUnique;

	fComparer->setTestItem(inItem);
	isUnique = (search(fComparer, index) == NULL);
	if (isUnique)
		insertAt(index, inItem);
	return isUnique;
}


NewtonErr
CSortedList::insertDuplicate(ArrayIndex index, void * inItem, void * inItem2)
{
	return insertAt(index, inItem2);
}


void *
CSortedList::search(CItemTester * inComparator, ArrayIndex & outIndex)
{
	void *	foundItem = NULL;

	if (isEmpty())
		outIndex = 0;
	else
	{
		CompareResult	result;
		CListIterator	iter(this);
		do {
			iter.fCurrentIndex = (iter.fHighBound + iter.fLowBound) / 2;
			foundItem = at(iter.fCurrentIndex);
			result = inComparator->testItem(foundItem);
			if (result == kItemGreaterThanCriteria)
				iter.fLowBound = iter.fCurrentIndex + 1;
			else if (result == kItemLessThanCriteria)
				iter.fHighBound = iter.fCurrentIndex - 1;
			else
				break;
		} while ((int)iter.fHighBound >= (int)iter.fLowBound);

		if (result == kItemLessThanCriteria)
			foundItem = NULL;
		else if (result == kItemGreaterThanCriteria)
		{
			foundItem = NULL;
			iter.fCurrentIndex++;
		}
		if (iter.fCurrentIndex <= count())
			outIndex = iter.fCurrentIndex;
		else
			outIndex = kIndexNotFound;
	}
	return foundItem;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C L i s t I t e r a t o r
------------------------------------------------------------------------------*/


CListIterator::CListIterator()
	:	CArrayIterator()
{ }


CListIterator::CListIterator(CDynamicArray * inList)
	:	CArrayIterator(inList)
{ }


CListIterator::CListIterator(CDynamicArray * inList, bool inForward)
	:	CArrayIterator(inList, inForward)
{ }


CListIterator::CListIterator(CDynamicArray * inList, ArrayIndex inLowBound, ArrayIndex inHighBound, bool inForward)
	:	CArrayIterator(inList, inLowBound, inHighBound, inForward)
{ }


void *
CListIterator::firstItem(void)
{
	reset();
	return more() ? ((CList*)fDynamicArray)->at(fCurrentIndex) : NULL;
}


void *
CListIterator::currentItem(void)
{
	return (fDynamicArray) ? ((CList*)fDynamicArray)->at(fCurrentIndex) : NULL;
		
}


void *
CListIterator::nextItem(void)
{
	advance();
	return more() ? ((CList*)fDynamicArray)->at(fCurrentIndex) : NULL;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C L i s t L o o p
------------------------------------------------------------------------------*/

CListLoop::CListLoop(CList * inList)
{
	fList = inList;
	reset();
}


void
CListLoop::reset(void)
{
	fIndex = -1;
	fSize = fList->count();
}

void *
CListLoop::current(void)
{
	return (fIndex < fSize) ? fList->at(fIndex) : NULL;
}


void *
CListLoop::next(void)
{
	++fIndex;
	return current();
}


void
CListLoop::removeCurrent(void)
{
	fList->removeElementsAt(fIndex, 1);
	--fIndex;
	--fSize;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C B a c k w a r d L o o p
------------------------------------------------------------------------------*/

CBackwardLoop::CBackwardLoop(CList * inList)
{
	fList = inList;
	fIndex = inList->count();
}


void *
CBackwardLoop::current(void)
{
	return (fIndex >= 0) ? fList->at(fIndex) : NULL;
}


void *
CBackwardLoop::next(void)
{
	fIndex--;
	return current();
}

#pragma mark -

/*------------------------------------------------------------------------------
	C I t e m T e s t e r
------------------------------------------------------------------------------*/

CompareResult
CItemTester::testItem(const void * inItem) const
{
	return kItemLessThanCriteria;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C I t e m C o m p a r e r
------------------------------------------------------------------------------*/

CItemComparer::CItemComparer()
	:	fItem(NULL), fKey(NULL)
{ }


CItemComparer::CItemComparer(const void * inItem, const void * inKeyValue)
	:	fItem(inItem), fKey(inKeyValue)
{ }


CompareResult
CItemComparer::testItem(const void * inItem) const
{
	if (fItem < inItem)
		return kItemLessThanCriteria;
	else if (fItem > inItem)
		return kItemGreaterThanCriteria;
	else
		return kItemEqualCriteria;
}
