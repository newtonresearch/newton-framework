/*
	File:		OptionArray.cc

	Contains:	CommAPI config/options.

	Written by:	Newton Research Group, 2008.
*/

#include "DynamicArray.h"
#include "OptionArray.h"
#include "OSErrors.h"

// Private errors
// NoOption -6
// OptionTooSmall -7
// OptionTooBig -8

/*------------------------------------------------------------------------------
	C O p t i o n
------------------------------------------------------------------------------*/

COption::COption(ULong inType)
{
	fLabel = 0;
	fLength = 0;
	fFlags = inType | 0x0100;
}

void
COption::reset()
{
	fFlags &= ~(kProcessedFlagMask + kOpCodeResultMask);
}

void
COption::setAsService(ULong inServiceId)
{
	fLabel = inServiceId;
	setAsService();
}

void
COption::setAsService()
{
	fFlags = (fFlags & ~kTypeMask) | kServiceType;
}

void
COption::setAsOption(ULong inOptionId)
{
	fLabel = inOptionId;
	fFlags = (fFlags & ~kTypeMask) | kOptionType;
}

void
COption::setAsConfig(ULong inConfigId)
{
	fLabel = inConfigId;
	fFlags = (fFlags & ~kTypeMask) | kConfigType;
}

void
COption::setAsAddress(ULong inAddrId)
{
	fLabel = inAddrId;
	fFlags = (fFlags & ~kTypeMask) | kAddressType;
}

NewtonErr
COption::copyDataFrom(COption * inOption)
{
	NewtonErr err = noErr;
	size_t optLen = inOption->length();
	if (optLen > length())
	{
		// full source won’t fit in this option
		optLen = length();
		err = -7;
	}
	else if (length() > optLen)
		// this option is longer than required
		err = -8;
	memmove(fData, inOption->fData, optLen);
	return err;
}

COption *
COption::clone(void)
{
	return NULL;	// not implemented in ROM
}

#pragma mark -

/*------------------------------------------------------------------------------
	C O p t i o n E x t e n d e d
------------------------------------------------------------------------------*/

COptionExtended::COptionExtended(ULong inType)
	:	COption(inType)
{
	fServiceLabel = 0;
	fExtendedResult = noErr;
}

void
COptionExtended::setAsServiceSpecific(ULong inService)
{
	fFlags = (fFlags & ~kTypeMask) | kServiceSpecificType;
	fServiceLabel = inService;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S u b A r r a y O p t i o n
------------------------------------------------------------------------------*/

CSubArrayOption::CSubArrayOption(size_t inSize, ULong inCount)
	:	COption(kOptionType)
{
	setLabel('suba');
	setLength(sizeof(size_t) + inSize);
	fCount = inCount;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C O p t i o n A r r a y
------------------------------------------------------------------------------*/

COptionArray::COptionArray()
{
	fIsShared = false;
	fArrayBlock = NULL;
	fIterator = NULL;
	fCount = 0;
//	fSharedMemoryObject = 0;	// sic
}

COptionArray::~COptionArray()
{
	if (fIterator)
		fIterator->deleteArray();
	if (fArrayBlock)
		FreePtr(fArrayBlock);
}


NewtonErr
COptionArray::init(void)
{
	fArrayBlock = NewPtr(0);
	return MemError();
}

NewtonErr
COptionArray::init(size_t initialSize)
{
	fArrayBlock = NewPtr(initialSize);
	return MemError();
}

NewtonErr
COptionArray::init(ObjectId inSharedId, ULong inOptionCount)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = copyFromShared(inSharedId, inOptionCount))
		fSharedMemoryObject = inSharedId;
		fIsShared = true;
	}
	XENDTRY;
	return err;
}

NewtonErr
COptionArray::init(CSubArrayOption * inArray)
{
	NewtonErr err;
	XTRY
	{
		fCount = inArray->count();
		size_t arraySize = inArray->fLength - 4;
		fArrayBlock = NewPtr(arraySize);
		XFAILNOT(fArrayBlock, err = kOSErrNoMemory;)
		memmove(fArrayBlock, inArray->array(), arraySize);
	}
	XENDTRY;
	return err;
}


void
COptionArray::reset()
{
	COption * option;
	COptionIterator iter(this);
	for (option = iter.firstOption(); option != NULL; option = iter.nextOption())
		option->reset();
	if (isShared())
		unshare();
}


COption *
COptionArray::optionAt(ArrayIndex index)
{
	COption * option = (COption *)fArrayBlock;
	for (ArrayIndex i = 0; i < index; ++i)	// no bounds checking?
		option = (COption *)((Ptr) option + LONGALIGN(sizeof(COption) + option->length()));
	return option;
}

NewtonErr
COptionArray::copyOptionAt(ArrayIndex index, COption * ioOption)
{
	NewtonErr err = noErr;
	XTRY
	{
		COption * option = optionAt(index);
		XFAILNOT(option, err = -6;)

		size_t optLen = option->length();
		if (optLen > ioOption->length())
		{
			// full source won’t fit in target option
			optLen = ioOption->length();
			err = -7;
		}
		// no check target option is longer than required?
		memmove(ioOption, this, optLen + sizeof(COption));
	}
	XENDTRY;
	return err;
}


NewtonErr
COptionArray::insertOptionAt(ArrayIndex index, COption * inOption)
{
	NewtonErr err = noErr;
	if (isShared())
		unshare();
	XTRY
	{
		if (index > count())
			index = count();
		size_t optionSize = LONGALIGN(sizeof(COption) + inOption->length());
		size_t arrayBlockSize = getSize();

		char * insertionPtr;
		char * newArrayBlock = ReallocPtr(fArrayBlock, arrayBlockSize + optionSize);
		XFAILNOT(newArrayBlock, err = MemError();)

		fArrayBlock = newArrayBlock;
		if (index < count())
		{
			// insert it
			insertionPtr = (Ptr) optionAt(index);
			memmove(insertionPtr + optionSize, insertionPtr, fArrayBlock + arrayBlockSize - insertionPtr);
		}
		else
			// append it
			insertionPtr = fArrayBlock + arrayBlockSize;
		memmove(insertionPtr, inOption, optionSize);
		fCount++;
		if (fIterator)
			fIterator->insertOptionAt(index);
	}
	XENDTRY;
	return err;
}

NewtonErr
COptionArray::insertVarOptionAt(ArrayIndex index, COption * inOption, void * inData, size_t inLen)
{
	NewtonErr err = noErr;
	if (isShared())
		unshare();
	XTRY
	{
		if (index > count())
			index = count();
		size_t optionSize = LONGALIGN(sizeof(COption) + inOption->length());
		size_t arrayBlockSize = getSize();

		char * insertionPtr;
		char * newArrayBlock = ReallocPtr(fArrayBlock, arrayBlockSize + optionSize);
		XFAILNOT(newArrayBlock, err = MemError();)

		fArrayBlock = newArrayBlock;
		if (index < count())
		{
			// insert it
			insertionPtr = (Ptr) optionAt(index);
			memmove(insertionPtr + optionSize, insertionPtr, fArrayBlock + arrayBlockSize - insertionPtr);
		}
		else
			// append it
			insertionPtr = fArrayBlock + arrayBlockSize;
		memmove(insertionPtr, inOption, sizeof(COption) + inOption->length() - inLen);
		memmove(insertionPtr + sizeof(COption) + inOption->length(), inData, inLen);
		fCount++;
		if (fIterator)
			fIterator->insertOptionAt(index);
	}
	XENDTRY;
	return err;
}

NewtonErr
COptionArray::insertSubArrayAt(ArrayIndex index, COptionArray * inSubArray)
{
	if (isShared())
		unshare();
	size_t size = inSubArray->getSize();
	CSubArrayOption option(size, inSubArray->count());
	return insertVarOptionAt(index, &option, inSubArray->fArrayBlock, size);
}


NewtonErr
COptionArray::removeOptionAt(ArrayIndex index)
{
	NewtonErr err = noErr;
	if (isShared())
		unshare();
	XTRY
	{
		XFAILIF(count() == 0, err = noErr;)

		COption * thisOption = optionAt(index);
		size_t thisOptionSize = LONGALIGN(sizeof(COption) + thisOption->length());
		Ptr nextOption = (Ptr) thisOption + thisOptionSize;
		size_t arrayBlockSize = getSize();
		Ptr arrayEnd = fArrayBlock + arrayBlockSize;
		if (arrayEnd > nextOption)
			memmove(thisOption, nextOption, arrayEnd - nextOption);

		char * newArrayBlock = ReallocPtr(fArrayBlock, arrayBlockSize - thisOptionSize);
		XFAILNOT(newArrayBlock, err = MemError();)
		fArrayBlock = newArrayBlock;
		fCount--;
		if (fIterator)
			fIterator->removeOptionAt(index);
	}
	XENDTRY;
	return err;
}

NewtonErr
COptionArray::removeAllOptions(void)
{
	NewtonErr err = noErr;
	if (isShared())
		unshare();
	XTRY
	{
		char * newArrayBlock = ReallocPtr(fArrayBlock, 0);
		XFAILNOT(newArrayBlock, err = MemError();)
		fArrayBlock = newArrayBlock;
		fCount = 0;
		// iterator?
	}
	XENDTRY;
	return err;
}


NewtonErr
COptionArray::merge(COptionArray * inArray)
{
	NewtonErr err = noErr;
	if (isShared())
		unshare();
	XTRY
	{
		size_t thisSize = getSize();
		size_t thatSize = inArray->getSize();
		char * newArrayBlock = ReallocPtr(fArrayBlock, thisSize + thatSize);
		XFAILNOT(newArrayBlock, err = MemError();)
		fArrayBlock = newArrayBlock;
		memmove(fArrayBlock + thisSize, inArray, thatSize);
		fCount += inArray->count();
		// iterator?
	}
	XENDTRY;
	return err;
}


NewtonErr
COptionArray::makeShared(ULong inPermissions)
{
	NewtonErr err = noErr;
	if (!isShared())
	{
		XTRY
		{
			XFAIL(err = fSharedMemoryObject.init())
			XFAIL(err = fSharedMemoryObject.setBuffer(fArrayBlock, getSize(), inPermissions))
			fIsShared = true;
		}
		XENDTRY;
	}
	return err;
}

NewtonErr
COptionArray::unshare()
{
	fSharedMemoryObject.destroyObject();
	fIsShared = false;
	return noErr;
}


NewtonErr
COptionArray::copyFromShared(ObjectId inSharedId, ULong inCount)
{
	NewtonErr err;
	CUSharedMem shared(inSharedId);
	size_t size, newSize;
	fCount = inCount;	// is this really the best place? why do we need it anyway?
	XTRY
	{
		XFAIL(err = shared.getSize(&newSize))
		char * newArrayBlock = ReallocPtr(fArrayBlock, newSize);
		XFAILNOT(newArrayBlock, err = MemError();)
		fArrayBlock = newArrayBlock;
		err = shared.copyFromShared(&size, fArrayBlock, newSize);
	}
	XENDTRY;
	return err;
}

NewtonErr
COptionArray::copyToShared(ObjectId inSharedId)
{
	CUSharedMem shared(inSharedId);
	return shared.copyToShared(fArrayBlock, getSize());
}

NewtonErr
COptionArray::shadowCopyBack(void)
{
	NewtonErr err = noErr;
	if (fShadow)
		err = copyToShared(fSharedMemoryObject);
	return err;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C O p t i o n I t e r a t o r
------------------------------------------------------------------------------*/

COptionIterator::COptionIterator()
{
	fPreviousLink = fNextLink = this;
	fHighBound = fLowBound = fCurrentIndex = kIndexNotFound;
	fOptionArray = NULL;
	fCurrentOption = NULL;
}

COptionIterator::COptionIterator(COptionArray * itsOptionArray)
{
	init(itsOptionArray, 0, itsOptionArray->count() - 1);
}

COptionIterator::COptionIterator(COptionArray * itsOptionArray, ArrayIndex itsLowBound, ArrayIndex itsHighBound)
{
	init(itsOptionArray, itsLowBound, itsHighBound);
}

COptionIterator::~COptionIterator()
{
	if (fOptionArray)
		fOptionArray->fIterator = removeFromList();
}

void
COptionIterator::init(COptionArray * itsOptionArray, ArrayIndex itsLowBound, ArrayIndex itsHighBound)
{
	fPreviousLink = fNextLink = this;
	fCurrentOption = NULL;
	fOptionArray = itsOptionArray;
	fOptionArray->fIterator = appendToList(fOptionArray->fIterator);
	initBounds(itsLowBound, itsHighBound);
}

void
COptionIterator::initBounds(ArrayIndex itsLowBound, ArrayIndex itsHighBound)
{
	ArrayIndex limit = fOptionArray->count();
	if (limit > 0)
	{
		limit--;
		if (itsHighBound == kIndexNotFound)
			itsHighBound = 0;
		if (itsHighBound > limit)
			itsHighBound = limit;
	}
	else
		itsHighBound = kIndexNotFound;
	fHighBound = itsHighBound;

	if (itsHighBound != kIndexNotFound)
	{
		if (itsLowBound == kIndexNotFound)
			itsLowBound = 0;
		if (itsLowBound > itsHighBound)
			itsLowBound = itsHighBound;
	}
	else
		itsLowBound = kIndexNotFound;
	fLowBound = itsLowBound;

	reset();
}

void
COptionIterator::reset(void)
{
	fCurrentIndex = fLowBound;
	fCurrentOption = (fCurrentIndex == kIndexNotFound) ? NULL : (COption *)fOptionArray->fArrayBlock;
}

void
COptionIterator::resetBounds(void)
{
	if (fOptionArray->isEmpty())
		fLowBound = fHighBound = kIndexNotFound;
	else
	{
		fLowBound = 0;
		fHighBound = fOptionArray->count() - 1;
	}
}

ArrayIndex
COptionIterator::firstIndex(void)
{
	reset();
	return more() ? fCurrentIndex : kIndexNotFound;
}

ArrayIndex
COptionIterator::nextIndex(void)
{
	advance();
	return more() ? fCurrentIndex : kIndexNotFound;
}

ArrayIndex
COptionIterator::currentIndex(void)
{
	return (fOptionArray != NULL) ? fCurrentIndex : kIndexNotFound;
}

COption *
COptionIterator::firstOption(void)
{
	firstIndex();
	return fCurrentOption;
}

COption *
COptionIterator::nextOption(void)
{
	nextIndex();
	return fCurrentOption;
}

COption *
COptionIterator::currentOption(void)
{
	return (fOptionArray != NULL) ? fCurrentOption : NULL;
}

COption *
COptionIterator::findOption(ULong inLabel)
{
	COption * opt;
	for (opt = firstOption(); more(); opt = nextOption())
		if (opt->label() == inLabel)
			return opt;
	return NULL;
}

bool
COptionIterator::more(void)
{
	return fOptionArray != NULL && fCurrentIndex != kIndexNotFound;
}

void
COptionIterator::advance(void)
{
	if (fCurrentIndex < fHighBound)
	{
		fCurrentIndex++;
		fCurrentOption = (COption *)((Ptr) fCurrentOption + LONGALIGN(sizeof(COption) + fCurrentOption->length()));
	}
	else
	{
		fCurrentIndex = kIndexNotFound;
		fCurrentOption = NULL;
	}
}

void
COptionIterator::removeOptionAt(ArrayIndex index)
{
	if (fLowBound >= index)
		fLowBound--;
	if (fHighBound >= index)
		fHighBound--;
	if (fCurrentIndex >= index)
		fCurrentIndex--;
	if (fCurrentIndex != kIndexNotFound)
	{
		fCurrentOption = (COption *)fOptionArray->fArrayBlock;
		for (ArrayIndex i = 0; i < fCurrentIndex; ++i)
			fCurrentOption = (COption *)((Ptr) fCurrentOption + LONGALIGN(sizeof(COption) + fCurrentOption->length()));
	}
	if (fOptionArray != NULL && fNextLink != fOptionArray->fIterator)
		fNextLink->removeOptionAt(index);
}

void
COptionIterator::insertOptionAt(ArrayIndex index)
{
	if (fLowBound >= index)
		fLowBound++;
	if (fHighBound >= index)
		fHighBound++;
	if (fCurrentIndex >= index)
		fCurrentIndex++;
	if (fCurrentIndex != kIndexNotFound)
	{
		fCurrentOption = (COption *)fOptionArray->fArrayBlock;
		for (ArrayIndex i = 0; i < fCurrentIndex; ++i)
			fCurrentOption = (COption *)((Ptr) fCurrentOption + LONGALIGN(sizeof(COption) + fCurrentOption->length()));
	}
	if (fOptionArray != NULL && fNextLink != fOptionArray->fIterator)
		fNextLink->insertOptionAt(index);
}

void
COptionIterator::deleteArray(void)
{
	if (fOptionArray != NULL && fNextLink != fOptionArray->fIterator)
		fNextLink->deleteArray();
	fOptionArray = NULL;
	fCurrentOption = NULL;
}

COptionIterator *
COptionIterator::appendToList(COptionIterator * inList)
{
	if (inList)
	{
		fPreviousLink = inList;
		fNextLink = inList->fNextLink;
		inList->fNextLink->fPreviousLink = this;
		inList->fNextLink = this;
	}
	return this;
}

COptionIterator *
COptionIterator::removeFromList(void)
{
	COptionIterator * next = (fNextLink != this) ? fNextLink : NULL;
	fNextLink->fPreviousLink = fPreviousLink;
	fPreviousLink->fNextLink = fNextLink;
	fPreviousLink = fNextLink = this;
	return next;
}

