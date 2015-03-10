/*
	File:		RecArea.cc

	Contains:	Recognition area implementation.

	Written by:	Newton Research Group.
*/

#include "RecArea.h"
#include "RecDomain.h"
#include "Dictionaries.h"
#include "OSErrors.h"


/* -----------------------------------------------------------------------------
	C R e c A r e a
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Create a new CRecArea object.
	A view system can store anything it wants in the area and flags fields.
	If a program wants the controller to keep track of the areas and perform
	hit-testing, it should pass an FRect describing the area and a constant
	indicating the type of area.
----------------------------------------------------------------------------- */

CRecArea *
CRecArea::make(ULong inArea, ULong inFlags)
{
	CRecArea * area;
	XTRY
	{
		XFAIL((area = new CRecArea) == NULL)
		area->iRecObject();
		area->fFlags = inFlags;
		area->fArea = inArea;
		area->fATypes = NULL;
		area->fGTypes = NULL;
		area->f14 = 0;
		for (ArrayIndex i = 0; i < 3; ++i)
			area->f20[i] = NULL;
		area->fView = NULL;
		XFAILIF((area->fATypes = CTypeAssoc::make()) == NULL
			  || (area->fGTypes = CTypeAssoc::make()) == NULL, area->release(); area = NULL;)
	}
	XENDTRY;
	return area;
}


/* -----------------------------------------------------------------------------
	Dispose of the storage allocated for the CRecArea object.
	Be sure that you have first disposed of any allocated structures that you
	stored in the flags or area fields.
	If multiple views contain references to the same CRecArea, be sure to
	call dealloc only once.
----------------------------------------------------------------------------- */

void
CRecArea::dealloc(void)
{
	if (fATypes)
		fATypes->release();
	if (fGTypes)
		fGTypes->release();
	for (ArrayIndex i = 0; i < 3; ++i)
	{
		CDictChain * chain;
		if ((chain = f20[i]) != NULL)
			chain->release();
	}
}


size_t
CRecArea::sizeInBytes(void)
{
	size_t thisSize = fATypes->sizeInBytes() + fGTypes->sizeInBytes();
	thisSize += (sizeof(CRecArea) - sizeof(CRecObject));	// original doesn’t need sizeof evaluation b/c it uses dynamic Ptr size
	return CRecObject::sizeInBytes() + thisSize;
}


void
CRecArea::dump(CMsg * outMsg)
{ /* this really does nothing */ }


/* -----------------------------------------------------------------------------
	Add a type to the ATypes list in the CRecArea.
	It passes a pointer to the handler routine that should be called when units
	of this type are recognized.
	The info parameter lets you pass a RecDomainInfo containing global information;
	this information is then passed back to the handler routine.
	If you have no global data, pass NULL.
	When you are done adding types, call the CController::buildGTypes method
	so the controller can build its list of recognizers to call on the way
	to recognizing the requested types.
----------------------------------------------------------------------------- */

void
CRecArea::addAType(RecType inType, UnitHandler inHandler, ULong inArg3, RecDomainInfo * info)
{
	Assoc aType;
	aType.fType = inType;
	aType.fDomain = NULL;
	aType.fDomainInfo = NULL;
	aType.f18 = NO;
	if (info != NULL)
		aType.fDomainInfo = info;
	aType.fInfo = NULL;
	aType.fHandler = inHandler;
	aType.f14 = inArg3;

	ArrayIndex  b4Size = fATypes->count();
	fATypes->addAssoc(&aType);
	if (inArg3 == 1 && fATypes->count() > b4Size)
		f14++;
}


Ptr
CRecArea::getInfoFor(RecType inType, bool inArg2)
{
	char *			info = NULL;	// was Handle
	Size				infoSize;
	ArrayIterator	iter;
	Assoc *			aType = (Assoc *)fGTypes->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); aType = (Assoc *)iter.getNext(), ++i)
	{
		CRecDomain * domain = aType->fDomain;
		if (domain->getType() == inType)
		{
			info = aType->fInfo;
			if (info != NULL)
				break;
			if (!inArg2)
				break;
			domain->domainParameter(0, (OpaqueRef)&infoSize, 0);
			if (infoSize == 0)
			{
				aType->fInfo = NULL;
				break;
			}
			info = NewNamedPtr(infoSize, 'info');
			if (info != NULL)
				domain->domainParameter(1, 0, (OpaqueRef)info);
			aType->fInfo = info;
			break;
		}
	}
	return info;
}


void
CRecArea::paramsAllSet(RecType inType)
{
	ArrayIterator  iter;
	Assoc *			aType = (Assoc *)fGTypes->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); aType = (Assoc *)iter.getNext(), ++i)
	{
		CRecDomain * domain = aType->fDomain;
		if (domain->getType() == inType)
		{
			domain->invalParameters();
			domain->configureSubDomain(this);
			break;
		}
	}
}


#pragma mark -
/*------------------------------------------------------------------------------
	C A r e a L i s t
------------------------------------------------------------------------------*/

CAreaList *
CAreaList::make(void)
{
	CAreaList * list;
	XTRY
	{
		XFAIL((list = new CAreaList) == NULL)
		XFAILIF(list->iAreaList() != noErr, list->release(); list = NULL;)
	}
	XENDTRY;
	return list;
}


NewtonErr
CAreaList::iAreaList(void)
{
	return iDArray(sizeof(CRecArea*), 0);
}


CRecObject *
CAreaList::retain(void)
{
	for (ArrayIndex i = 0; i < count(); ++i)
		getArea(i)->retain();
	return CDArray::retain();
}


void
CAreaList::release(void)
{
	for (ArrayIndex i = 0; i < count(); ++i)
		getArea(i)->release();
	CDArray::release();
}


NewtonErr
CAreaList::addArea(CRecArea * inArea)
{
	NewtonErr err = noErr;
	XTRY
	{
		CRecArea **	p;
		XFAILNOT(p = reinterpret_cast<CRecArea **>(addEntry()), err = kOSErrNoMemory;)
		compact();
		*p = (CRecArea *)inArea->retain();
	}
	XENDTRY;
	return err;
}


CRecArea *
CAreaList::getArea(ArrayIndex index)
{
	return *reinterpret_cast<CRecArea **>(getEntry(index));
}


CRecArea *
CAreaList::getMergedArea(void)
{
	return getArea(fSize - 1);
}


bool
CAreaList::findMatchingView(ULong inView)
{
	for (ArrayIndex i = 0; i < fSize; ++i)
		if (getArea(i)->fView == inView)
			return YES;
	return NO;
}

