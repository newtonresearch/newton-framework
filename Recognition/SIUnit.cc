/*
	File:		SIUnit.cc

	Contains:	Class CSIUnit, a subclass of CRecUnit, provides the mechanism
					for adding subunits and interpretations.

	Written by:	Newton Research Group, 2010.
*/

#include "SIUnit.h"
#include "Controller.h"
#include <math.h>


bool
InitInterpretation(UnitInterpretation * interpretation, ULong inParamSize, ArrayIndex inNumOfParams)
{
	bool initd = NO;
	XTRY
	{
		interpretation->label = kNoLabel;
		interpretation->score = kBadScore;
		interpretation->angle = 0;
		if (inNumOfParams == 0)
			interpretation->parms = NULL;
		else
		{
			interpretation->parms = CArray::make(inParamSize, inNumOfParams);
			XFAIL(interpretation->parms == NULL)
		}
		initd = YES;
	}
	XENDTRY;
	return initd;
}


#pragma mark CSIUnit
/* -----------------------------------------------------------------------------	
	 C S I U n i t
----------------------------------------------------------------------------- */

CSIUnit::CSIUnit()
{ }

CSIUnit::~CSIUnit()
{ }


/* -----------------------------------------------------------------------------	
	Create a new CSIUnit.
	Args:		inDomain			the domain that actually created the unit, or NULL
				inType			-> fType of unit
				inAreaList		-> fAreaList of unit
				inInterpSize
	Return:	instance of CSIUnit

	Doesn’t exist in the ROM -- may be inlined.
----------------------------------------------------------------------------- */

CSIUnit *
CSIUnit::make(CRecDomain * inDomain, ULong inType, ULong inArg3, CArray * inAreas, ULong inInterpSize)
{
	CSIUnit * siUnit;
	XTRY
	{
		XFAIL((siUnit = new CSIUnit) == NULL)
		XFAILIF(siUnit->iSIUnit(inDomain, inType, inArg3, inAreas, inInterpSize) != noErr, siUnit->release(); siUnit = NULL;)
	}
	XENDTRY;
	return siUnit;
}


/* -----------------------------------------------------------------------------
	Initialize the unit.
	This should be called by the initialize methods of subclasses of CSIUnit.
	Parameters are the same as those passed to make.
	Args:		inDomain			the domain that actually created the unit, or NULL
				inType			-> fUnitType
				inAreaList		-> fAreaList; generally just the fAreaList from
										the first subunit that will be added to the unit
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CSIUnit::iSIUnit(CRecDomain * inDomain, ULong inType, ULong inArg3, CArray * inAreas, ULong inInterpSize)
{
	NewtonErr err = iRecUnit(inDomain, inType, inArg3, inAreas);

	fHasSubUnits = 0;
	fSubUnits = NULL;
	fHasInterpretations = 0;
	fInterpretations = (CDArray *)(size_t)inInterpSize;
	return err;
}


/* -----------------------------------------------------------------------------
	Dispose of the subunit list (but not its elements), and the interpretations list,
	then call CRecUnit::dealloc() to complete disposal of this object.
	If your unit allocates any additional data that needs to be disposed,
	then you will need to override this method.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CSIUnit::dealloc(void)
{
	if (fHasSubUnits == 2)
		fSubUnits->release();
	if (fHasInterpretations == 1)
	{
		for (int i = interpretationCount()-1; i >= 0; i--)
			deleteInterpretation(i);
	}
	fHasSubUnits = 0;
	fHasInterpretations = 0;

	// super dealloc
	CRecUnit::dealloc();
}


/* -----------------------------------------------------------------------------
	Dispose of a unit that has not had either newClassification or newGroup
	called on it.
	Units that have been passed to recognition in this way must be claimed by
	calling controller->claimUnits(unit). This is generally done by the application
	after it accepts an interpretation of a classified unit.
	Recognizers will not need to override this method.
	Args:		--
	Return:	--

void
CSIUnit::dispose(void)
{}
----------------------------------------------------------------------------- */


#pragma mark Debug
/* -----------------------------------------------------------------------------
	For debug.
	Return the size in bytes occupied by this unit and all of its owned structures.
	There are methods defined for computing the sizes of most structures defined
	by the recognition system. Call these to get their sizes.
	Args:		--
	Return:	size in bytes
----------------------------------------------------------------------------- */

size_t
CSIUnit::sizeInBytes(void)
{
	size_t thisSize = 0;
	for (ArrayIndex i = 0, count = interpretationCount(); i < count; ++i)
	{
		CArray * p;
		if ((p = getParam(i)))
			thisSize += p->sizeInBytes();
	}
	if (fHasSubUnits == 2)
		thisSize += fSubUnits->sizeInBytes();
	if (fHasInterpretations == 1)
		thisSize += fInterpretations->sizeInBytes();
	thisSize += (sizeof(CSIUnit) - sizeof(CRecUnit));	// original doesn’t need sizeof evaluation b/c it uses dynamic Ptr size
	return CRecUnit::sizeInBytes() + thisSize;
}


/* -----------------------------------------------------------------------------
	For debug; see the CMsg class for a description of its use.
	Args:		outMsg
	Return:	--
----------------------------------------------------------------------------- */

void
CSIUnit::dump(CMsg * outMsg)
{
	char buf[100];

	CRecUnit::dump(outMsg);
	if (gVerbose)
		outMsg->msgStr("\n\t");
	sprintf(buf, "%u Subs (%hu, %hu)", subCount(), fMinStroke, fMaxStroke);
	outMsg->msgStr(buf);
	if (gVerbose)
	{
		sprintf(buf, "\t%u Interpretations\n", interpretationCount());
		outMsg->msgStr(buf);
	}
}

#pragma mark Interpretations

ArrayIndex
CSIUnit::openInterpList(void)
{
	ArrayIndex index;
	XTRY
	{
		// when no interpretations exist, the list pointer actually holds
		// the size of an interpretation object
		CDArray * theList = CDArray::make((size_t)fInterpretations, 1);
		XFAILNOT(theList, index = kIndexNotFound;)
		index = 0;
		fInterpretations = theList;
		fHasInterpretations = 1;
	}
	XENDTRY;
	return index;
}


void
CSIUnit::closeInterpList(void)
{
	size_t interpSize = fInterpretations->elementSize();	// barking mad
	fInterpretations->release();
	fInterpretations = (CDArray *)interpSize;
	fHasInterpretations = 0;
}


/* -----------------------------------------------------------------------------
	A recognizer's classify method calls addInterpretation to add a new interpretation to the unit.
	The pointer is simply a pointer to the desired interpretation data.
	Since the size of an interpretation is specified when the unit is created,
	the right amount of data is copied into the new interpretation.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

ArrayIndex
CSIUnit::addInterpretation(Ptr inData)
{
	ArrayIndex index = kIndexNotFound;
	XTRY
	{
		if (fHasInterpretations == 0)
			index = openInterpList();
		else if (fHasInterpretations == 1)
			index = fInterpretations->add();
		XFAIL(index == kIndexNotFound)
		fInterpretations->setEntry(index, inData);
	}
	XENDTRY;
	return index;
}


ArrayIndex
CSIUnit::insertInterpretation(ArrayIndex index)
{
	ArrayIndex i = kIndexNotFound;
	XTRY
	{
		if (fHasInterpretations == 0)
			i = openInterpList();
		else if (fHasInterpretations == 1)
			i = fInterpretations->insert(index);
	}
	XENDTRY;
	return i;
}


int
CSIUnit::deleteInterpretation(ArrayIndex index)
{
	CRecObject * p = getParam(index);
	if (p)
		p->release();
	fInterpretations->deleteEntry(index);
	if (fInterpretations->count() == 0)
		closeInterpList();
	return 1;
}


/* -----------------------------------------------------------------------------
	Return the current number of interpretations.
	If you use the standard interpretation interface, you won't need to override this method.
	If you store interpretations in a private format, you will need to overide this method.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

ArrayIndex
CSIUnit::interpretationCount(void)
{
	if (fHasInterpretations == 1)
		return fInterpretations->count();
	return 0;
}


UnitInterpretation *
CSIUnit::getInterpretation(ArrayIndex index)
{
	if (fHasInterpretations == 1)
		return (UnitInterpretation *)fInterpretations->getEntry(index);
	return NULL;
}


ArrayIndex
CSIUnit::getBestInterpretation(void)
{
	ArrayIndex index = kIndexNotFound;
	ULong bestScore = kBadScore;	// lower is better
	for (ArrayIndex i = 0, count = interpretationCount(); i < count; ++i)
	{
		UnitInterpretation * interp = getInterpretation(i);
		if (interp->label != kNoLabel
		&&  interp->score < bestScore)
		{
			bestScore = interp->score;
			index = i;
		}
	}
	return index;
}


int
CSIUnit::checkInterpretationIndex(ArrayIndex index)
{
	return (index != kIndexNotFound && index < interpretationCount()) ? 1 : 0;
}


NewtonErr
CSIUnit::interpretationReuse(ArrayIndex index, ULong inParamSize, ULong inNumOfParams)
{
	XTRY
	{
		for (ArrayIndex i = interpretationCount() - 1; i >= index; i--)
			deleteInterpretation(i);

		if (fHasInterpretations == 0)
		{
			XFAIL(index == 0)
			openInterpList();
		}
		if (fHasInterpretations == 1)
		{
			XFAILIF(index == 0, closeInterpList();)
		}
		XFAIL(index == 0)
		if (inNumOfParams == 0)
		{
			fInterpretations->reuse(index);
		}
		else
		{
			ArrayIndex i = interpretationCount();
			if (i < index)
			{
				UnitInterpretation interp;
				XFAIL(fInterpretations->reuse(index))
				for ( ; i < index; ++i)
				{
					InitInterpretation(&interp, inParamSize, inNumOfParams);
					*getInterpretation(i) = interp;
				}
			}
		}
	}
	XENDTRY;

	return noErr;
}


void
CSIUnit::compactInterpretations(void)
{
	if (fHasInterpretations == 1)
		fInterpretations->compact();
}

#pragma mark Units

void
CSIUnit::claimUnit(CRecUnitList * inUnits)
{
	markUnit(inUnits, kClaimedUnit);
}


NewtonErr
CSIUnit::markUnit(CRecUnitList * inUnits, ULong inFlags)
{
	NewtonErr err = noErr;
	setFlags(inFlags);

	XTRY
	{
		ArrayIndex i, subcount = subCount();
		if (subcount == 0)
			err = inUnits->addUnit(this);
		else for (i = 0; i < subcount; ++i)
		{
			XFAIL(err = getSub(i)->markUnit(inUnits, inFlags))
		}
	}
	XENDTRY;

	return err;
}


void
CSIUnit::doneUsingUnit(void)
{
	if (fHasInterpretations == 1)
	{
		for (int i = interpretationCount()-1; i >= 0; i--)
			deleteInterpretation(i);
	}
	fHasInterpretations = 0;
	CRecUnit::doneUsingUnit();
}


void
CSIUnit::endUnit(void)
{
	endSubs();

	for (ArrayIndex i = 0, count = interpretationCount(); i < count; ++i)
	{
		CArray * p;
		if ((p = getParam(i)))
			p->compact();
	}
	compactInterpretations();
}

#pragma mark Subunits

ULong
CSIUnit::subCount(void)
{
	XTRY
	{
		if (fHasSubUnits == 0)
			return 0;

		if (fHasSubUnits == 1)
			return 1;

		if (fHasSubUnits == 2)
		{
			XFAIL(fSubUnits == NULL)
			return fSubUnits->count();
		}
	}
	XENDTRY;
	return 0;
}


/* -----------------------------------------------------------------------------
	A recognizer calls addSub to add the specified subunit.
	addSub updates the unit's field (defined in the CRecUnit superclass).
	You will not need to override this method.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

ArrayIndex
CSIUnit::addSub(CRecUnit * inSubunit)
{
	ArrayIndex index = kIndexNotFound;

	XTRY
	{
		if (fHasSubUnits == 0)
		{
			// we don’t have any subunits yet -- make this the only one
			fHasSubUnits = 1;
			fSubUnits = (CDArray *)inSubunit;
			index = 0;
		}

		else if (fHasSubUnits == 1)
		{
			// create a list to contain our singleton and the new subunit
			CDArray * newList = CDArray::make(sizeof(CRecUnit*), 2);
			XFAIL(newList == NULL)
			newList->setEntry(0, (Ptr)fSubUnits);
			newList->setEntry(1, (Ptr)inSubunit);
			fHasSubUnits = 2;
			fSubUnits = newList;
			index = 1;
		}

		else if (fHasSubUnits == 2)
		{
			// add new subunit to our list
			index = fSubUnits->add();
			XFAIL(index == kIndexNotFound)
			fSubUnits->setEntry(index, (Ptr)inSubunit);
		}

		else
			XFAIL(-1)

		FRect subBox;
		FRect ourBox;
		inSubunit->getBBox(&subBox);
		getBBox(&ourBox);
		AddRect(&subBox, &ourBox, index == 0);
		setBBox(&ourBox);

		if (index == 0)
		{
			fStartTime = inSubunit->fStartTime;
			fDuration = (inSubunit->fStartTime + inSubunit->fDuration) - fStartTime;
		}
		else
		{
			fStartTime = MIN(fStartTime, inSubunit->fStartTime);
			fDuration = MIN(fStartTime + fDuration, inSubunit->fStartTime + inSubunit->fDuration) - fStartTime;
		}

		fElapsedTime = GetTicks() - fStartTime;

		if (fDomain && fAreaList)
		{
			setFlags(kDelayUnit);
			setDelay(fDomain->getDelay());
		}

		if (fNumOfStrokes == 0xFFFF)
		{
			fNumOfStrokes = 0;
			fMinStroke = inSubunit->fMinStroke;
			fMaxStroke = inSubunit->fMaxStroke;
		}
		else
		{
			if (inSubunit->fMinStroke < fMinStroke)
				fMinStroke = inSubunit->fMinStroke;
			if (inSubunit->fMaxStroke > fMaxStroke)
				fMaxStroke = inSubunit->fMaxStroke;

			ULong expiry = GetTicks() + fDelay;
			if (gController->endTime() > expiry)
				gController->setEndTime(expiry);

			if (inSubunit->testFlags(0x00080000))
				setFlags(0x00080000);
		}
	}
	XENDTRY;

	return index;
}


/* -----------------------------------------------------------------------------
	Return the specified subunit from the unit's fSubs list.
	The elements are indexed from 0.
	If you request an invalid index, NULL is returned.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

CRecUnit *
CSIUnit::getSub(ArrayIndex index)
{
	XTRY
	{
		if (fHasSubUnits == 0)
			return NULL;

		if (fHasSubUnits == 1)
		{
			XFAIL(index != 0)
			return (CRecUnit *)fSubUnits;
		}

		if (fHasSubUnits == 2)
		{
			return *(CRecUnit **)fSubUnits->getEntry(0);
		}
	}
	XENDTRY;

	return NULL;
}


void
CSIUnit::deleteSub(ArrayIndex index)
{
	XTRY
	{
		if (fHasSubUnits == 0)
			/* nothing to delete */ ;

		else if (fHasSubUnits == 1)
		{
			XFAIL(index != 0)
			fHasSubUnits = 0;
			if (fSubUnits)
				fSubUnits->release();
		}

		else if (fHasSubUnits == 2)
		{
			CRecUnit * subunit = getSub(index);
			fSubUnits->deleteEntry(index);
			if (fSubUnits->count() == 1)
			{
				CRecUnit * singleton = *(CRecUnit **)fSubUnits->getEntry(0);
				fSubUnits->release();
				fSubUnits = (CDArray *)singleton;
				fHasSubUnits = 1;
			}
			if (subunit)
				subunit->release();
		}
	}
	XENDTRY;
}


CDArray *
CSIUnit::getSubsCopy(void)
{
	XTRY
	{
		if (fHasSubUnits == 0)
			return NULL;

		if (fHasSubUnits == 1)
		{
			CDArray * theCopy = CDArray::make(sizeof(CRecUnit*), 1);
			XFAIL(theCopy == NULL)
			*(CRecUnit **)theCopy->getEntry(0) = (CRecUnit *)fSubUnits;
			return theCopy;
		}

		if (fHasSubUnits == 2)
		{
			return (CDArray *)fSubUnits->retain();
		}
	}
	XENDTRY;

	return NULL;
}


/* -----------------------------------------------------------------------------
	A recognizer should call endSubs when it's done adding subunits to
	(usually in the domain's group method).
	endSubs compacts the fSubs list and resets the unit's **fEndTime field
	so that the unit is immediately classified.
	You may want to override this method, if your unit contains other allocated
	structures that you want to dispose of or compact at the end of classification.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

NewtonErr
CSIUnit::endSubs(void)
{
	setDelay(0);

	if (fHasSubUnits == 2)
		fSubUnits->compact();

	ULong expiry = GetTicks() + fDelay;
	if (gController->endTime() > expiry)
		gController->setEndTime(expiry);

	return noErr;
}

#pragma mark Strokes

ULong
CSIUnit::countStrokes(void)
{
	ULong numOfStrokes = 0;
	for (ArrayIndex i = 0, subcount = subCount(); i < subcount; ++i)
	{
		numOfStrokes += getSub(i)->countStrokes();
	}
	return numOfStrokes;
}


CRecStroke *
CSIUnit::getStroke(ArrayIndex index)
{
	if (index < countStrokes())
	{
		if (index == 0)
			return getSub(0)->getStroke(0);

		ArrayIndex endCount, strokeCount = 0;
		for (ArrayIndex i = 0; i < subCount(); ++i)
		{
			CRecUnit * subunit = getSub(i);
			endCount = strokeCount + subunit->countStrokes();
			if (endCount > index)
				return subunit->getStroke(index - strokeCount);

			strokeCount = endCount;
		}
	}
	return NULL;
}


CRecUnitList *
CSIUnit::getAllStrokes(void)
{
	bool isStrokeOwner = NO;
	CRecUnitList * allUnits = CRecUnitList::make();		// r4
	CRecUnitList * uniqueUnits = CRecUnitList::make();	// r5
	XTRY
	{
		XFAIL(allUnits == NULL || uniqueUnits == NULL)
		XFAIL(allUnits->addUnit(this))

		do {
			for (ArrayIndex i = 0; i < allUnits->count(); ++i)
			{
				CSIUnit * aUnit = (CSIUnit *)allUnits->getUnit(i);
				ArrayIndex subi, subcount = aUnit->subCount();
				for (subi = 0; subi < subcount; ++subi)
				{
					CSIUnit * aSubunit = (CSIUnit *)aUnit->getSub(subi);
					isStrokeOwner = aSubunit->ownsStroke();
					XXFAIL(uniqueUnits->addUnique(aSubunit))
				}
			}
			CRecUnitList * tmp = allUnits;
			allUnits = uniqueUnits;
			uniqueUnits = tmp;
			uniqueUnits->clear();
		} while (!isStrokeOwner && allUnits->count() > 0);
		if (uniqueUnits)
			uniqueUnits->release();
		return allUnits;
	}
	XENDTRY;

failed:
	if (uniqueUnits)
		uniqueUnits->release();
	if (allUnits)
		allUnits->release();
	return NULL;
}

#pragma mark -

ULong
CSIUnit::getLabel(ArrayIndex index)
{
	UnitInterpretation * interpretation = getInterpretation(index);
	return interpretation ? interpretation->label : 0;
}


ULong
CSIUnit::getScore(ArrayIndex index)
{
	UnitInterpretation * interpretation = getInterpretation(index);
	return interpretation ? interpretation->score : 0;
}


float
CSIUnit::getAngle(ArrayIndex index)
{
	UnitInterpretation * interpretation = getInterpretation(index);
	return interpretation ? interpretation->angle : 0;
}


CArray *
CSIUnit::getParam(ArrayIndex index)
{
	UnitInterpretation * interpretation = getInterpretation(index);
	return interpretation ? interpretation->parms : NULL;
}


void
CSIUnit::setLabel(ArrayIndex index, ULong inLabel)
{
	UnitInterpretation * interpretation = getInterpretation(index);
	if (interpretation)
		interpretation->label = inLabel;
}


void
CSIUnit::setScore(ArrayIndex index, ULong inScore)	// fixed in patent
{
	UnitInterpretation * interpretation = getInterpretation(index);
	if (interpretation)
		interpretation->score = inScore;
}


void
CSIUnit::setAngle(ArrayIndex index, float inAngle)
{
	UnitInterpretation * interpretation = getInterpretation(index);
	if (interpretation)
		interpretation->angle = inAngle;
}

