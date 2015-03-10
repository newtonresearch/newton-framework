/*
	File:		Controller.cc

	Contains:	Recognition controller implementation.

	Written by:	Newton Research Group, 2008.
*/

#include "Controller.h"
#include "StrokeCentral.h"
#include "ClickRecognizer.h"
#include "ShapeRecognizer.h"
#include "OSErrors.h"

extern CStrokeDomain *	gStrokeDomain;
extern bool		CheckStrokeQueueEvents(ULong inStartTime, ULong inDuration);


struct Something
{
	CRecUnit *		f00;
	CRecDomain *	f04;
	RecDomainInfo * f08;
	Ptr				f0C;			// +0C	was Handle
};


bool
UnitsHitSameArea(CRecUnit * inUnit1, CRecUnit * inUnit2)
{
	CRecArea * area1 = inUnit1->getArea();
	CRecArea * area2 = inUnit2->getArea();
	return area1 != NULL  &&  area2 != NULL
		 && area1->viewId() == area2->viewId()
		 && area1->viewId() != 0;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C C o n t r o l l e r

	The CController class provides methods for grouping and classifying
	recognition units, and for handling recognition areas.
------------------------------------------------------------------------------*/

NewtonErr
InitControllerState(CController * inController)
{
	inController->fFlags = 0;
	inController->fPiecePool = CRecUnitList::make();
	inController->fUnitPool = CRecUnitList::make();
	inController->f14 = CArray::make(sizeof(Something), 0);
	inController->f20 = kDistantFuture;
	inController->f24 = kDistantFuture;
	inController->f28 = kDistantFuture;
	inController->f2C = kDistantFuture;
	
	return (inController->fPiecePool == NULL
		  || inController->fUnitPool == NULL
		  || inController->f14 == NULL) ? kOSErrNoMemory : noErr;	// original returns 1 as error
}


#pragma mark Initialisation

CController *
CController::make(void)
{
	CController * controller;
	XTRY
	{
		XFAIL((controller = new CController) == NULL)
		XFAILIF(controller->iController() != noErr, controller->release(); controller = NULL;)	// original doesn’t bother
	}
	XENDTRY;
	return controller;
}


NewtonErr
CController::iController(void)
{
	InitControllerState(this);
	fDomainList = CArray::make(sizeof(CRecDomain *), 0);
	f1C = 0;
	fHitTest = NULL;
	fExpireStroke = NULL;
	f40 = NO;
	f44 = 0;
	f48 = 0;
	f4C = 0;
	f50 = 0;
	f54 = 0;
	f58 = 0;
	f5C = 0;
	return noErr;
}


void
CController::initialize(void)
{
	//sp-40
	CArray * r4 = CArray::make(sizeof(CRecDomain *), 0);
	CArray * r5 = CArray::make(sizeof(CRecDomain *), 0);
	CArray * tmp;

	ArrayIterator domainIter;	// sp20
	CRecDomain ** domainPtr = (CRecDomain **) fDomainList->getIterator(&domainIter);
	for (ArrayIndex i = 0; i < domainIter.count(); domainPtr = (CRecDomain **) domainIter.getNext(), i++)
	{
		CRecDomain * domain = *domainPtr;
		if (domain->getType() == 'STRK')
		{
			*(CRecDomain **)(r4->addEntry()) = domain;
			domain->f1C = 2;
		}
	}

	int r7 = 2;
	ArrayIterator iter2;	// sp00
	while (r4->count() > 0)
	{
		r5->cutToIndex(0);
		r7++;
		CRecDomain ** domainPtr2 = (CRecDomain **) r4->getIterator(&iter2);
		for (ArrayIndex j = 0; j < iter2.count(); domainPtr2 = (CRecDomain **) iter2.fGetNext(&iter2), j++)	// r6
		{
			RecType r10 = (*domainPtr2)->getType();
			domainPtr = (CRecDomain **) fDomainList->getIterator(&domainIter);
			for (ArrayIndex i = 0; i < domainIter.count(); domainPtr = (CRecDomain **) domainIter.fGetNext(&domainIter), i++)	// r8
			{
				CRecDomain * domain = *domainPtr;
				if (domain->fPieceTypes->findType(r10) != 0xFFFFFFFF)
				{
					*(CRecDomain **)(r5->addEntry()) = domain;
					domain->f1C = r7;
				}
			}
		}
		tmp = r4;
		r4 = r5;
		r5 = tmp;
	}
	r4->release();
	r5->release();
}


void
CController::dealloc(void)
{
	fPiecePool->purge();
	fPiecePool->release();

	fUnitPool->purge();
	fUnitPool->release();

	ArrayIterator  iter;
	CRecDomain **	domainPtr = (CRecDomain **) fDomainList->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); domainPtr = (CRecDomain **) iter.getNext(), i++)
	{
		CRecDomain * domain = *domainPtr;
		domain->release();
	}
	fDomainList->release();
}


/*------------------------------------------------------------------------------
	Set the function to be called to detect hits... on what?
	Args:		inHandler
	Return:	--
------------------------------------------------------------------------------*/

void
CController::setHitTestRoutine(HitTestProcPtr inHandler)
{
	fHitTest = inHandler;
}


/*------------------------------------------------------------------------------
	Set the function to be called to expire strokes.
	Args:		inHandler
	Return:	--
------------------------------------------------------------------------------*/

void
CController::setExpireStrokeRoutine(ExpireStrokeProcPtr inHandler)
{
	fExpireStroke = inHandler;
}


/*------------------------------------------------------------------------------
	A domain’s initialize method should call registerDomain, passing the domain
	returned by the make method.
	Args:		ioDomain
	Return:	--
------------------------------------------------------------------------------*/

void
CController::registerDomain(CRecDomain * ioDomain)
{
	ioDomain->setController(this);
	CRecDomain **	domainPtr = (CRecDomain **) fDomainList->addEntry();
	*domainPtr = ioDomain;
	fDomainList->compact();
}


/*------------------------------------------------------------------------------
	A domain’s group method should call newGroup after creating a new output unit
	and adding a piece to it with addSub.
	Args:		inUnit
	Return:	--
------------------------------------------------------------------------------*/

void
CController::newGroup(CRecUnit * inUnit)
{
	ULong  delay = 0;
	if (inUnit->getDelay() == 0)
	{
		delay = inUnit->getDomain()->getDelay();
		inUnit->setDelay(delay);
	}
	if (fUnitPool->addUnit(inUnit) != noErr)
		signalMemoryError();
	if (!inUnit->testFlags(kClaimedUnit + kPartialUnit + kInvalidUnit + 0x00200000))
	{
		ULong  r0 = inUnit->getEndTime() + delay;
		if (f20 > r0)
			f20 = r0;
	}
}


/*------------------------------------------------------------------------------
	getDelayList returns a list of previously grouped pieces.
	A domain’s group method should call getDelayList when a new piece comes in.
	The group method can then analyze whether the new piece should be added to the group.
	Args:		inUnit
	Return:	--
------------------------------------------------------------------------------*/

CRecUnitList *
CController::getDelayList(CRecDomain * inDomain, ULong inType)
{
	return getUList(inDomain, inType, 0x10000000, 0x48000000);
}


CRecUnitList *
CController::getUList(CRecDomain * inDomain, ULong inType, ULong inWantedBits, ULong inUnwantedBits)
{
	NewtonErr err;

	CRecUnitList * theList;	//r6
	XTRY
	{
		XFAILNOT(theList = CRecUnitList::make(), err = kOSErrNoMemory;)		// original says -1
		ArrayIterator iter;
		CRecUnit ** unitPtr = (CRecUnit **) fUnitPool->getIterator(&iter);
		for (ArrayIndex i = 0; i < iter.count(); unitPtr = (CRecUnit **) iter.getNext(), i++)
		{
			CRecUnit * unit = *unitPtr;
			if (unit->getType() == inType
			&&  (inDomain == NULL || inDomain == unit->getDomain())
			&&  unit->testFlags(inWantedBits)
			&& !unit->testFlags(inUnwantedBits))
				XFAIL(err = theList->addUnit(unit))
		}
		theList->compact();
	}
	XENDTRY;

	XDOFAIL(err)
	{
		signalMemoryError();
		if (theList != NULL)
			theList->release(), theList = NULL;
	}
	XENDFAIL;

	return theList;
}


/*------------------------------------------------------------------------------
	A domain’s classify method should call newClassification after classifying the units
	that have been grouped together and storing an interpretation in the output unit.
	Args:		inUnit
	Return:	non-zero => error, but not yer actual error code
------------------------------------------------------------------------------*/

NewtonErr
CController::newClassification(CRecUnit * inUnit)
{
	//sp-5C
	NewtonErr err = noErr;
	CAreaList * areas = NULL;
	ArrayIndex f14size = f14->count();		//r9

	XTRY
	{
		XFAIL(err = (inUnit == NULL))
		inUnit->setFlags(0x00200000);
		XFAIL(err = fPiecePool->addUnit(inUnit))

		ArrayIterator iter;
		CRecUnit ** unitPtr = (CRecUnit **) fUnitPool->getIterator(&iter);
		for (ArrayIndex i = 0; i < iter.count(); unitPtr = (CRecUnit **) iter.getNext(), i++)
		{
			CRecUnit * unit = *unitPtr;
			if (unit == inUnit)
			{
				inUnit->retain();
				break;
			}
		}
		if (inUnit->fNumOfStrokes == 0xFFFF)
		{
			inUnit->fMinStroke = f1C;
			inUnit->fMaxStroke = f1C;
			f1C++;
		}
		inUnit->fNumOfStrokes = inUnit->countStrokes();	// actually CountStrokes(inUnit)

		areas = inUnit->getAreas();
		if (areas == NULL)
			XFAIL(err = (areas = CAreaList::make()) == NULL)
		if (fHitTest == NULL || !fHitTest(inUnit, areas))
			inUnit->setAreas(areas);
		areas->release();

		areas = inUnit->getAreas();
		if (areas != NULL && areas->count() > 0)
		{
			Assoc assoc, * assocPtr;
			CRecArea * area;	//r10
			XFAIL(err = (area = inUnit->getArea()) == NULL)

			CTypeAssoc * groupTypes = area->fGTypes;
			if (groupTypes->count() > 0)
			{
				assocPtr = (Assoc *)groupTypes->getIterator(&iter);
				for (ArrayIndex i = 0; i < iter.count(); assocPtr = (Assoc *)iter.getNext(), i++)
				{
					assoc = *assocPtr;
					if (inUnit->getType() == assoc.fType)
					{
						Something * p = (Something *)f14->addEntry();
						XFAILIF(p == NULL, err = kOSErrNoMemory;)	// AHA! will only break out of for loop
						p->f00 = inUnit;
						p->f04 = assoc.fDomain;
						p->f08 = assoc.fDomainInfo;
						p->f0C = assoc.fInfo;
					}
				}
				XFAIL(err)	// AHA! this will fix that
			}
			else
			{
				if (inUnit->getType() == 'STRK')
					err = 1;
			}

			CTypeAssoc * areaTypes = area->fATypes;
			assocPtr = (Assoc *)areaTypes->getIterator(&iter);
			for (ArrayIndex i = 0; i < iter.count(); assocPtr = (Assoc *)iter.getNext(), i++)
			{
				assoc = *assocPtr;
				if (assoc.fType == inUnit->getType()
				&&  assoc.f14 != 2)
				{
					ArbStuff * p = (ArbStuff *)fArbiter->f04->addEntry();
					XFAILIF(p == NULL, err = kOSErrNoMemory;)	// AHA! will only break out of for loop
					p->x00 = inUnit;
					p->x0C = assoc;
					p->x08 = assoc.f14;
					p->x04 = 0;
				}
			}
			XFAIL(err)	// AHA! this will fix that
		}
		else
		{
			markUnits(inUnit, kClaimedUnit);
			f2C = GetTicks();
		}
	}
	XENDTRY;

	XDOFAIL(err)
	{
		signalMemoryError();
	}
	XENDFAIL;

	if (areas)
		areas->release();

	ULong now = GetTicks();
	if (f14->count() > f14size)
		f24 = now;
	if (fArbiter->f04->count() != 0)
		f28 = now;

	return err;
}


/*------------------------------------------------------------------------------
	A program should call buildGTypes after adding types to a recognition area with the
	CRecArea::addAType method.
	buildGTypes causes the controller to rebuild its cached list of recognizers.
	Args:		inArea
	Return:	--
------------------------------------------------------------------------------*/

void
CController::buildGTypes(CRecArea * inArea)
{
	//sp-5C
	CTypeAssoc * groupTypes = inArea->fGTypes;
	groupTypes->clear();

	CTypeAssoc * areaTypes = inArea->fATypes->copy();	//r5
	if (areaTypes)
	{
		CTypeAssoc * newAreaTypes = CTypeAssoc::make();
		if (newAreaTypes)
		{
			Assoc aType;		// sp40
			aType.fType = 0;
			aType.fDomain = NULL;
			aType.fDomainInfo = NULL;
			aType.fInfo = 0;
			aType.fHandler = NULL;

			int r6 = 0;
			ArrayIndex areaTypesCount;
			while ((areaTypesCount = areaTypes->count()) > 0)
			{
				ArrayIterator domainIter;
				CRecDomain ** domainPtr = (CRecDomain **) fDomainList->getIterator(&domainIter);
				for (ArrayIndex i = 0; i < domainIter.count(); domainPtr = (CRecDomain **) domainIter.getNext(), i++)
				{
					CRecDomain * domain = *domainPtr;
					aType.fDomain = domain;
					for (ArrayIndex j = 0; j < areaTypesCount; j++)
					{
						if (areaTypes->getAssoc(j)->fType == domain->getType())
						{
							if (r6 < domain->f1C)
								r6 = domain->f1C;
							ArrayIterator typeIter;
							RecType * typePtr = (RecType *)domain->fPieceTypes->getIterator(&typeIter);
							for (ArrayIndex k = 0; k < typeIter.count(); typePtr = (RecType *)typeIter.getNext(), k++)
							{
								aType.fType = *typePtr;
								if (aType.fType != domain->getType())
									newAreaTypes->addAssoc(&aType);
							}
						}
					}
				}
				inArea->fGTypes->mergeAssoc(newAreaTypes);

				CTypeAssoc * swap = areaTypes;
				areaTypes = newAreaTypes;
				newAreaTypes = swap;

				newAreaTypes->cutToIndex(0);
			}

			inArea->f10 = r6;
			newAreaTypes->release();
		}
		areaTypes->release();
	}
}


ULong
CController::idle(void)
{
	NewtonErr err = noErr;
	ULong now;
	bool r5 = testFlags(0x08000000);

	XTRY
	{
		XFAIL(err = controllerError())
		now = GetTicks();
		if (r5 || f24 <= now)
		{
			doGroup();
			XFAIL(err = controllerError())
			now = GetTicks();
		}
		if (r5 || f20 <= now)
		{
			doClassify();
			XFAIL(err = controllerError())
			now = GetTicks();
		}
		if (r5 || f28 <= now)
		{
			doArbitration();
			XFAIL(err = controllerError())
			now = GetTicks();
		}
		if (r5 || f2C <= now)
		{
			cleanUp();
			XFAIL(err = controllerError())
		}
	}
	XENDTRY;

	XDOFAIL(err)
	{
		cleanupAfterError();
	}
	XENDFAIL;

	return nextIdleTime();
}


ULong	// milliseconds
CController::nextIdleTime(void)
{
	ULong idleTime = kDistantFuture;
	ULong nextTime = f20;
	if (nextTime > f24)
		nextTime = f24;
	if (nextTime > f28)
		nextTime = f28;
	if (nextTime > f2C)
		nextTime = f2C;
	ULong now = GetTicks();
	if (nextTime == kDistantFuture)
	{
		// controller claims to be idle
		ArrayIndex count = fPiecePool->count();
		if (count > 0)
		{
			// but there are still pieces in the pool -- they MUST be still in progress
			bool isStillInProgress = NO;
			for (ArrayIndex i = 0; i < count; ++i)
			{
				if (ClickInProgress(fPiecePool->getUnit(i)))
				{
					isStillInProgress = YES;
					break;
				}
			}
			if (!isStillInProgress)
			{
				signalMemoryError();
				idleTime = 0;
			}
		}
	}
	else
	{
		if (fArbiter->f21
		&&  f28 == kDistantFuture
		&&  f20 == kDistantFuture
		&&  f24 == kDistantFuture)
			fArbiter->f20 = YES;
		if (fArbiter->f20)
		{
			gController->f28 = GetTicks();
			idleTime = 0;
		}
		else
		{
			ULong ticks = nextTime - now;
			if (ticks > 0)
			{
				ULong millisecs = (ticks * 1000) / 60;		// tied to Ticks timebase
				if (millisecs > 0)
					idleTime = millisecs;
			}
		}
	}
//printf("CController::nextIdleTime() -> %u\n", idleTime);
	return idleTime;
}


/*------------------------------------------------------------------------------
	Determine whether we’re busy. What does ‘busy’ mean, exactly?
	Args:		--
	Return:	YES => we’re busy
------------------------------------------------------------------------------*/

bool
CController::checkBusy(void)
{
	return testFlags(0x40000000);
}


void
CController::classifyInArea(CRecUnit * inUnit, CRecArea * inArea)
{
	for (ArrayIndex i = 0; i < inUnit->subCount(); ++i)
	{
		classifyInArea(((CSIUnit *)inUnit)->getSub(i), inArea);
	}

	CRecDomain * domain = inUnit->getDomain();

	Ptr info = inArea->getInfoFor(inUnit->getType(), NO);
	if (info != NULL)						// && info != domain->fParameters) in the original but redundant
	{
		domain->setParameters(info);
//		domain->fParameters = info;	// in the original but redundant
	}

	CAreaList * tempAreaList;
	CAreaList * savedAreaList;
	if ((savedAreaList = inUnit->getAreas()) != NULL)
	{
		inUnit->setAreas(NULL);
		if ((tempAreaList = CAreaList::make()) != NULL)
		{
			if (tempAreaList->addArea(inArea) == noErr)
			{
				inUnit->setAreas(tempAreaList);
				domain->reclassify(inUnit);
			}
			inUnit->setAreas(NULL);
			tempAreaList->release();
		}
		inUnit->setAreas(savedAreaList);
		savedAreaList->release();
	}
}


void
CController::cleanGroupQ(CRecUnit * inUnit)
{
	ArrayIterator iter;
	CRecUnit ** unitPtr = (CRecUnit **) f14->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); i++, unitPtr++)
	{
		if (*unitPtr == inUnit)
			*unitPtr = NULL;
	}
}


void
CController::cleanUp(void)
{
printf("CController::cleanUp()\n");
	fArbiter->cleanUp();
	fPiecePool->compact();
	fUnitPool->compact();
	f14->compact();
	f2C = kDistantFuture;
}

#pragma mark Arbitration

void
CController::registerArbiter(CArbiter * inArbiter)
{
	fArbiter = inArbiter;
}

void
CController::doArbitration(void)
{
printf("CController::doArbitration()\n");
	fArbiter->doArbitration();
	f28 = kDistantFuture;
}

void
CController::deletePiece(ArrayIndex index)
{
	fPiecePool->getUnit(index)->release();
	fPiecePool->deleteEntry(index);
}

void
CController::deleteUnit(ArrayIndex index)
{
	fUnitPool->getUnit(index)->release();
	fUnitPool->deleteEntry(index);
}


int
CController::doClassify(void)
{
printf("CController::doClassify()\n");
	int isNoArea = NO;
	f20 = kDistantFuture;
	bool r10 = testFlags(0x08000000);

	ArrayIterator iter;
	CRecUnit ** unitPtr = (CRecUnit **) fUnitPool->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); unitPtr = (CRecUnit **) iter.getNext(), i++)
	{
		CRecUnit * unit = *unitPtr;
unit->dumpObject((char *)" examining ");
		if (!unit->testFlags(0x68200000))
		{
			CRecDomain * domain = unit->getDomain();
			if (unit->getDelay() > 0)
			{
				if (r10 || noEventsWithinDelay(unit, 0))
				{
					if (!unit->testFlags(0x08000000))
					{
						CRecArea * area = unit->getArea();
						XFAIL(isNoArea = (area == NULL))
						Ptr r9 = area->getInfoFor(unit->getType(), NO);
						if (r9 != NULL && r9 != domain->fParameters)
						{
							domain->setParameters(r9);
					//		domain->fParameters = info;	// in the original but redundant
							domain->classify(unit);
							XFAIL(controllerError())
						}
					}
				}
				else
				{
					ULong  r0 = unit->getEndTime() + unit->getDelay();
					if (f20 > r0)
						f20 = r0;
				}
			}
			else
			{
				if (unit->getPriority() == 0)
				{
					CRecArea * area = unit->getArea();
					XFAIL(isNoArea = (area == NULL))
					Ptr r9 = area->getInfoFor(unit->getType(), NO);
					if (r9 != NULL && r9 != domain->fParameters)
					{
						domain->setParameters(r9);
				//		domain->fParameters = info;	// in the original but redundant
						domain->classify(unit);
						XFAIL(controllerError())
					}
				}
				else
					unit->fPriority--;
			}
		}
	}

	unitPtr = (CRecUnit **) fUnitPool->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); unitPtr = (CRecUnit **) iter.getNext(), i++)
	{
		CRecUnit * unit = *unitPtr;
		if (unit->testFlags(0x00200000))
		{
			unit->release();
			fUnitPool->deleteEntry(i);
			iter.removeCurrent();
			i--;
		}
	}

	if (isNoArea)
		signalMemoryError();
	return isNoArea;
}


void
HandleAreaSwitched(CRecDomain * inDomain, Ptr inParameters)
{
	RecType domainType = inDomain->getType();
	ArrayIterator iter;
	CSIUnit ** unitPtr = (CSIUnit **) inDomain->controller()->unitPool()->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); unitPtr = (CSIUnit **) iter.getNext(), i++)
	{
		CSIUnit * unit = *unitPtr;
		if (unit->getType() == domainType  &&  unit->getDomain() == inDomain
		&&  unit->testFlags(0x10000000)  &&  !unit->testFlags(0x48000000)
		&&  unit->getArea()->getInfoFor(domainType, NO) != inParameters)
		{
			unit->endSubs();
		}
	}
}


void
CController::doGroup(void)
{
printf("CController::doGroup()\n");
	ArrayIndex groupIndex = 0;
	ArrayIndex numOfStrokes = 0;
	ArrayIndex numOfClicks = 0;
	ArrayIterator iter;
	Something * p = (Something *)f14->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); p = (Something *)iter.getNext(), i++)
	{
		CRecUnit * unit = p->f00;
unit->dumpObject((char *)" examining ");
		if (unit != NULL && !unit->testFlags(kClaimedUnit))
		{
			if ((unit->getType() == 'STXR'  &&  ++numOfStrokes > 1)
			||  (unit->getType() == 'CLIK'  &&  ++numOfClicks > 1))
			{
				// consolidate
				memcpy(f14->getEntry(groupIndex), f14->getEntry(i), sizeof(Something));
				groupIndex++;
			}
			else
			{
				if (p->f0C != p->f04->fParameters)
				{
					p->f04->setParameters(p->f0C);
//					p->f04->fParameters = p->f0C;	// in the original but redundant
					HandleAreaSwitched(p->f04, p->f0C);
				}
				if (p->f04->group(p->f00, p->f08) == 0)
				{
					memcpy(f14->getEntry(groupIndex), f14->getEntry(i), sizeof(Something));
					groupIndex++;
				}
			}
			XFAIL(controllerError())
		}
	}
printf("cutToIndex(%u)\n",groupIndex);
	f14->cutToIndex(groupIndex);
	f24 = (numOfStrokes <= 1 && numOfClicks <= 1) ? kDistantFuture : GetTicks();
}


CRecUnit *
CController::getClickInProgress(void)
{
	for (ArrayIndex i = 0; i < fPiecePool->count(); ++i)
	{
		CRecUnit * unit = fPiecePool->getUnit(i);
		if (ClickInProgress(unit))
			return unit;
	}
	return NULL;
}


CStrokeUnit *
CController::getIndexedStroke(ArrayIndex index)
{
	ArrayIterator iter;
	CStrokeUnit **	unitPtr = (CStrokeUnit **) fPiecePool->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); unitPtr = (CStrokeUnit **) iter.getNext(), i++)
	{
		CStrokeUnit * unit = *unitPtr;
		if (unit->minStroke() == index && unit->maxStroke() == index
		&&  unit->getType() == 'STRK')
			return unit;
	}
	return NULL;
}


CRecDomain *
CController::getTypedDomain(RecType inType)
{
	ArrayIterator domainIter;
	CRecDomain **	domainPtr = (CRecDomain **) fDomainList->getIterator(&domainIter);
	for (ArrayIndex i = 0; i < domainIter.count(); domainPtr = (CRecDomain **) domainIter.getNext(), i++)
	{
		CRecDomain * domain = *domainPtr;
		if (domain->getType() == inType)
			return domain;
	}
	return NULL;
}


void
CController::markUnits(CRecUnit * inUnit, ULong inFlags)
{
	NewtonErr err;
	ArrayIterator iter;	// sp04
	CRecUnitList * r4 = NULL;
	CRecUnitList * r5 = NULL;
	XTRY
	{
		r4 = CRecUnitList::make();
		XFAILIF(r4 == NULL, err = kOSErrNoMemory;)
		XFAIL(err = inUnit->markUnit(r4, inFlags))

		r5 = CRecUnitList::make();
		XFAILIF(r5 == NULL, err = kOSErrNoMemory;)

		for (ArrayIndex i = 0; i < r4->count(); ++i)	// r6
		{
			CSIUnit ** unitPtr = (CSIUnit **) fPiecePool->getIterator(&iter);
			for (ArrayIndex j = 0; j < iter.count(); unitPtr = (CSIUnit **) iter.getNext(), j++)	// sp48
			{
				CSIUnit * unit = *unitPtr;
				ArrayIndex subCount = unit->subCount();
				for (ArrayIndex subIndex = 0; subIndex < subCount; subIndex++)	// r8
				{
					CRecUnit * sp44 = unit->getSub(subIndex);
					for (ArrayIndex k = 0; k < i; k++)	// r9
					{
						if (r4->getUnit(k) == sp44)
						{
							unit->setFlags(inFlags);
							XXFAIL((err = r5->addUnique(unit)))
							subIndex = subCount;
							break;
						}
					}
				}
			}
		}
	}
	XENDTRY;
failed:
	XDOFAIL(err)
	{
		signalMemoryError();
	}
	XENDFAIL;

	if (r4 != NULL)
		r4->release();
	if (r5 != NULL)
		r5->release();
}


bool
CController::noEventsWithinDelay(CRecUnit * inUnit, ULong inDelay)
{
	bool r5 = NO;
	if (inUnit != NULL)
	{
		CRecUnit * sp24 = NULL;
		CClickUnit * r7 = NULL;
		ULong unitStartTime;										// sp28
		ULong delayDuration = inUnit->getDelay();			// sp00
		ULong delayStart = inUnit->getEndTime();			// r8
		ULong delayEnd = delayStart + delayDuration;		// r9
		if (GetTicks() <= delayEnd)
			return NO;

		ArrayIterator iter;
		CClickUnit **	clikUnitPtr = (CClickUnit **) fPiecePool->getIterator(&iter);
		for (ArrayIndex i = 0; i < iter.count(); clikUnitPtr = (CClickUnit **) iter.getNext(), i++)
		{
			CClickUnit * unit = *clikUnitPtr;		// not necessarily a CClickUnit, but that’s what we’re looking for
			unitStartTime = unit->getStartTime();
			if (unitStartTime > delayStart  &&  unitStartTime < delayEnd  &&  UnitsHitSameArea(inUnit, unit))
			{
				if (unit->getType() != 'CLIK')
				{
					r5 = YES;
					sp24 = unit;
					break;
				}
				if (r7 == NULL  &&  !unit->clikStroke()->isDone())
					r7 = unit;
			}
			if (unitStartTime > delayEnd)
				break;
		}

		if (!r5)
		{
			CRecUnit **	unitPtr = (CRecUnit **) fUnitPool->getIterator(&iter);
			for (ArrayIndex i = 0; i < iter.count(); unitPtr = (CRecUnit **) iter.getNext(), i++)
			{
				CRecUnit * unit = *unitPtr;
				unitStartTime = unit->getStartTime();
				if (unitStartTime > delayStart  &&  unitStartTime < delayEnd  &&  UnitsHitSameArea(inUnit, unit))
				{
					r5 = YES;
					sp24 = unit;
					break;
				}
				if (unitStartTime > delayEnd)
					break;
			}
		}

		if (!r5  &&  CheckStrokeQueueEvents(delayStart, delayDuration))
			r5 = YES;

		if (sp24 != NULL  ||  r7 != NULL)
		{
			CRecDomain * r6 = inUnit->getDomain();
			if (r7 == NULL  ||  r6->preGroup(NULL) == 0  ||  r6->fPieceTypes->findType('STRK') == kIndexNotFound)
			{
				r5 = YES;
				inUnit->setDelay(255);
			}
			else
			{
				CRecStroke * r1 = r7->clikStroke();
				if (r1->count() >= 50)
				{
					r1->retain();
					CStrokeUnit * r8 = CStrokeUnit::make(gStrokeDomain, 2, r7->clikStroke(), NULL);
					if (r8)
					{
						r8->addSub(r7);
						r8->endSubs();
						r5 = (r6->preGroup(r8) == 0);
						if (!r5)
							inUnit->setDelay(255);
						r8->release();
					}
				}
				else
				{
					r5 = YES;
					inUnit->setDelay(GetTicks() - delayStart + (50 - r1->count())*60/60);	// sic
				}
			}
		}
	}
	return !r5;
}


bool
CController::isExternallyArbitrated(CRecUnit * inUnit)
{
	RecType reqType = inUnit->getType();
	CTypeAssoc * types;
	CRecArea * area;
	if ((area = inUnit->getArea()) != NULL
	&&  (types = area->arbitrationTypes()) != NULL  &&  types->count() > 0)
	{
		ArrayIterator iter;
		Assoc *	assoc = (Assoc *)types->getIterator(&iter);
		for (ArrayIndex i = 0; i < iter.count(); assoc = (Assoc *)iter.getNext(), i++)
		{
			if (assoc->fType == reqType
			&&  assoc->f14 == 2)
				return YES;
		}
	}
	return NO;
}


void
CController::regroupSub(CRecUnit * inUnit, CRecUnit * inSub)
{
	NewtonErr err = noErr;
	XTRY
	{
		CRecArea * area1 = inUnit->getArea();
		if (area1 != NULL)
		{
			ArrayIterator iter;
			Assoc *	assocPtr = (Assoc *)area1->groupingTypes()->getIterator(&iter);
			for (ArrayIndex i = 0; i < iter.count(); assocPtr = (Assoc *)iter.getNext(), i++)
			{
				Assoc assoc = *assocPtr;
				if (inSub->getType() == assoc.fType
				&&  inUnit->getDomain() == assoc.fDomain)
				{
					Something * p = (Something *)f14->addEntry();
					XFAIL(err = (p == NULL))
					p->f00 = inSub;
					p->f04 = assoc.fDomain;
					p->f08 = assoc.fDomainInfo;
					p->f0C = assoc.fInfo;
				}
			}
			XFAIL(err)
		}
		f24 = GetTicks();
	}
	XENDTRY;

	XDOFAIL(err)
	{
		signalMemoryError();
	}
	XENDFAIL;
}


void
CController::regroupUnclaimedSubs(CRecUnit * inUnit)
{
	for (ArrayIndex i = 0, subCount = inUnit->subCount(); i < subCount; ++i)
	{
		CRecUnit * subUnit = ((CSIUnit *)inUnit)->getSub(i);
		if (!subUnit->testFlags(kClaimedUnit))
			regroupSub(inUnit, subUnit);
	}
}


void
CController::updateInk(FRect * inBounds)
{
	FRect box;
	for (ArrayIndex i = 0, count = fPiecePool->count(); i < count; ++i)
	{
		CRecUnit * unit = fPiecePool->getUnit(i);
		unit->getBBox(&box);
		if (unit->getType() == 'STRK'
		&&  !unit->testFlags(kClaimedUnit))
		{
			CRecStroke * stroke = ((CStrokeUnit *)unit)->getStroke();
			if (!stroke->testFlags(0x08000000)
			||  SectRectangle(&box, inBounds, &box))
				stroke->draw();
		}
	}
}


bool
CController::isLastCompleteStroke(CRecUnit * inUnit)
{
	CRecUnit * unitInProgress;
	UShort lastStroke = inUnit->fMinStroke + 1;
	return f1C == lastStroke
		 || ((unitInProgress = getClickInProgress()) != NULL && unitInProgress->fMinStroke == lastStroke);
}

#pragma mark Recognition

void
CController::triggerRecognition(void)
{
	f20 = f24 = f28 = f2C = GetTicks();
}


void
CController::recognizeInArea(CArray * inArg1, CRecArea * inArea, ULong (*inFunc)(CRecUnit * inUnit, ULong), ULong inArg4)
{
//	r4: r5 r6 r3 r0
	//sp-04
	f40 = YES;
	f44 = inArg1->count();
	f48 = 0;
	f4C = inArg4;
/*
	f5C = inFunc;
	f54 = f38;
	f38 = SpecialGetAreasHit(CRecUnit*, CArray*);
	f58 = f3C;
	f3C = SpecialExpireStroke(CRecUnit*);

	for (ArrayIndex i = 0, count = inArea->fATypes->count(); i < count; ++i)
	{
		Assoc * r0 = inArea->fATypes->getAssoc(i);
		if (r0->areaHandler == NULL)
			r0->areaHandler = SpecialHandler(CArray*);
	}
	f50 = inArea;

	ULong lastRecTime = gLastWordEndTime + 2;
	ULong now = GetTicks();
	ULong aSecondAgo = now - 60;
	if (lastRecTime < aSecondAgo  ||  lastRecTime > now)
		lastRecTime = aSecondAgo;
	ULong timePerStroke = (now - lastRecTime) / inArg1->count();
	for (ArrayIndex i = 0; i < inArg1->count(); ++i)
	{
		r0 = inArg1->getEntry(i);
		CRecUnit * strkUnit = MakeStrokeUnit(r0->f00, NULL, 0);	// CRecStroke*
		strkUnit->fStartTime = lastRecTime + i * timePerStroke;
		strkUnit->fDuration = timePerStroke - 2;
		newClassification(strkUnit);
	}
	gLastWordEndTime = now;

	setFlags(0x08000000);
	do { idle(); } while (f48 < f44);
	unsetFlags(0x08000000);

	f38 = f54;
	f3C = f58;
	f50 = 0;
	f40 = NO;
*/
}


#pragma mark Errors and Recovery

void
CController::signalMemoryError(void)
{
	setFlags(0x10000000);
	f2C = GetTicks();
	if (fArbiter->f21)
	{
		f28 = GetTicks();
		fArbiter->f20 = YES;
	}
}


bool
CController::controllerError(void)
{
	return testFlags(0x10000000);
}


void
CController::cleanupAfterError(void)
{
	CRecUnit * unitInProgress = getClickInProgress();
	clearArbiter();
	cleanUpUnits(NO);		// but keep units still in progress
	expireAllStrokes();
	cleanUpUnits(YES);	// now nuke all units
	clearController();
	unsetFlags(0xFFFFFFFF);
	if (unitInProgress)
		newClassification(unitInProgress);
}


void
CController::cleanUpUnits(bool inNuke)
{
	ArrayIndex compactIndex = 0;
	ArrayIndex count = fPiecePool->count();
	for (ArrayIndex i = 0; i < count; ++i)
	{
		CRecUnit * unit = fPiecePool->getUnit(i);
		if (!inNuke
		&& (ClickInProgress(unit)  ||  unit->getType() == 'STRK'))
			// keep raw strokes and units still in progress
			*(CRecUnit **)fPiecePool->getEntry(compactIndex++) = unit;
		else if (!ClickInProgress(unit))
			unit->release();
	}
	fPiecePool->cutToIndex(compactIndex);

	compactIndex = 0;
	count = fUnitPool->count();
	for (ArrayIndex i = 0; i < count; ++i)
	{
		CRecUnit * unit = fUnitPool->getUnit(i);
		if (!inNuke
		&& unit->getType() == 'STRK')
			*(CRecUnit **)fUnitPool->getEntry(compactIndex++) = unit;
		else
			unit->release();
	}
	fUnitPool->cutToIndex(compactIndex);
}


void
CController::expireAllStrokes(void)
{
	ArrayIterator iter;
	CRecUnit **	unitPtr = (CRecUnit **) fPiecePool->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); unitPtr = (CRecUnit **) iter.getNext(), i++)
	{
		CRecUnit * unit = *unitPtr;
		if (unit->getType() == 'STRK'
		&& !unit->testFlags(kClaimedUnit)
		&&  fExpireStroke)
		{
			if (iter.count() > 50)
				unit->setFlags(0x00040000);
			fExpireStroke(unit);
		}
	}
}


void
CController::clearController(void)
{
	fPiecePool->clear();
	fUnitPool->clear();
	f14->clear();

	fPiecePool->compact();
	fUnitPool->compact();
	f14->compact();
}


void
CController::clearArbiter(void)
{
	fArbiter->f04->clear();
	fArbiter->f08->clear();
	fArbiter->f0C->clear();
	fArbiter->f10->clear();
	fArbiter->f14->clear();
	fArbiter->f18->clear();
	fArbiter->f1C->clear();

	fArbiter->f04->compact();
	fArbiter->f08->compact();
	fArbiter->f0C->compact();
	fArbiter->f10->compact();
	fArbiter->f14->compact();
	fArbiter->f18->compact();
	fArbiter->f1C->compact();
}

