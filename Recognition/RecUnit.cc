/*
	File:		RecUnit.cc

	Contains:	Recognition unit implementation.

	Written by:	Newton Research Group.
*/

#include "RecUnit.h"
#include "RecDomain.h"
#include "OSErrors.h"


/* -----------------------------------------------------------------------------
	C R e c U n i t
	Originally TUnit. The recognition unit base class.
----------------------------------------------------------------------------- */

#pragma mark CRecUnit

CRecUnit::CRecUnit()
{ }

CRecUnit::~CRecUnit()
{ }


/* -----------------------------------------------------------------------------	
	Create a new CRecUnit.
	This is not a virtual method, so subclasses of CRecUnit are free to redefine
	the interface to their make methods.
	Args:		inDomain			the domain that actually created the unit, or NULL
				inType			-> fType of unit
				inAreaList		-> fAreaList of unit
	Return:	instance of CRecUnit

	Doesn’t exist in the ROM -- may be inlined.
----------------------------------------------------------------------------- */

CRecUnit *
CRecUnit::make(CRecDomain * inDomain, ULong inType, ULong inArg3, CArray * inAreas)
{
	CRecUnit * recUnit;
	XTRY
	{
		XFAIL((recUnit = new CRecUnit) == NULL)
		XFAILIF(recUnit->iRecUnit(inDomain, inType, inArg3, inAreas) != noErr, recUnit->release(); recUnit = NULL;)
	}
	XENDTRY;
	return recUnit;
}


/* -----------------------------------------------------------------------------
	Initialize the unit.
	This should be called by the initialize methods of subclasses of CRecUnit.
	Parameters are the same as those passed to make.
	Args:		inDomain			the domain that actually created the unit, or NULL
				inType			-> fUnitType
				inAreaList		-> fAreaList
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CRecUnit::iRecUnit(CRecDomain * inDomain, ULong inType, ULong inArg3, CArray * inAreas)
{
	iRecObject();

	fDomain = inDomain;
	fUnitType = inType;
//	NamePtr(this, inType);
	fPriority = 0;
	fStartTime = GetTicks();
	fDuration = 0;
	fElapsedTime = 0;
	f24 = inArg3;
	setDelay(inDomain ? inDomain->getDelay() : 0);

	fAreaList = NULL;
	setAreas((CAreaList *)inAreas);

	fNumOfStrokes = 0xFFFF;
	fMinStroke = 0;
	fMaxStroke = 0;

	FRect emptyBox;
	SetRectangleEmpty(&emptyBox);		// could we have a global for this?
	setBBox(&emptyBox);

	return noErr;
}


/* -----------------------------------------------------------------------------
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CRecUnit::dealloc(void)
{
	setAreas(NULL);
}


#pragma mark Debug
/* -----------------------------------------------------------------------------
	For debug.
	Return the size in bytes occupied by this unit, all of its owned structures,
	including its interpretations, but not including its subunits.
	A subclass of CRecUnit only needs to override this method if it stores
	allocated structures in the unit itself.
	Args:		--
	Return:	size in bytes
----------------------------------------------------------------------------- */

size_t
CRecUnit::sizeInBytes(void)
{
	size_t thisSize = 0;
	if (fAreaList)
		thisSize = fAreaList->sizeInBytes();
	thisSize += (sizeof(CRecUnit) - sizeof(CRecObject));	// original doesn’t need sizeof evaluation b/c it uses dynamic Ptr size
	return CRecObject::sizeInBytes() + thisSize;
}


/* -----------------------------------------------------------------------------
	For debug; see the CMsg class for a description of its use.
	Args:		outMsg
	Return:	--
----------------------------------------------------------------------------- */

void
CRecUnit::dump(CMsg * outMsg)
{
	if (testFlags(0x00400000))
		outMsg->msgStr("INVALID ");
	dumpName(outMsg);
#if defined(correct)
	outMsg->msgStr("    Size: 0x");
	// something missing here?
	outMsg->msgStr("  ");
#else
	outMsg->msgStr("    Size: ");
	outMsg->msgNum(sizeInBytes(), -1);
#endif
	if (gVerbose)
	{
		outMsg->msgStr("\n\t");
		if (fDomain)
			fDomain->dumpName(outMsg);
		else
			outMsg->msgStr("No Domain");
		outMsg->msgStr("\n\tFlags: 0x");
		outMsg->msgHex(fFlags, 8);
		outMsg->msgStr("    Priority: ");
		outMsg->msgNum(fPriority, -1);
	}
}


/* -----------------------------------------------------------------------------
	For debug; see the CMsg class for a description of its use.
	Args:		outMsg
	Return:	--
----------------------------------------------------------------------------- */

void
CRecUnit::dumpName(CMsg * outMsg)
{
	int lParen, rParen;
	if (testFlags(kClaimedUnit))
	{	lParen = '[';	rParen = ']'; }
	else
	{	lParen = '(';	rParen = ')'; }

	outMsg->msgChar(lParen);
	outMsg->msgType(fUnitType);
	outMsg->msgChar(rParen);
	outMsg->msgStr(" ");
}


#pragma mark Interpretation
/* -----------------------------------------------------------------------------
	If the unit is a CRecUnit, SubCount always returns 0.
	If the unit is a subclass of CSIUnit, SubCount returns the number of elements
	in the subs array.
	Args:		--
	Return:	number of elements in the subs array
----------------------------------------------------------------------------- */

ULong
CRecUnit::subCount(void)
{
	return 0;
}


/* -----------------------------------------------------------------------------
	If the unit is a CRecUnit, this method returns 0.
	If the unit is a subclass of CSIUnit, it returns the number of interpretations
	in the interpretations array.
	If your subclass of CRecUnit stores a single interpretation within the unit
	itself, then it should probably return 1 and implement all the appropriate
	interpretation methods.
	Args:		--
	Return:	number of interpretations in the interpretations array
----------------------------------------------------------------------------- */

ULong
CRecUnit::interpretationCount(void)
{
	return 0;
}

/* -----------------------------------------------------------------------------
	Search through the list of interpretations and return the index of the one
	with the best score. You probably won't need to override this method.
	Args:		--
	Return:	index of most highly scored interpretation
----------------------------------------------------------------------------- */

ULong
CRecUnit::getBestInterpretation(void)
{
	return kIndexNotFound;
}


/* -----------------------------------------------------------------------------
	ClaimUnit is called by the CController::ClaimUnits method;
	it shouldn't need to be called directly.
	If the unit is a CSIUnit, ClaimUnit calls itself recursively on all of its
	subunits, and then marks itself as claimed (setting the claimUnit flag in
	the flags field).
	If the unit is a CRecUnit, then it simply marks itself as claimed.
	An application should claim a unit only if accepts an interpretation of
	the unit that is passed to it by the Arbiter. It should first extract all
	desired information from the unit or its interpretation, and then call
	ClaimUnits. Once ClaimUnits has been called on a unit, the unit is volatile
	and may disappear.
	Args:		inList
	Return:	--
----------------------------------------------------------------------------- */

void
CRecUnit::claimUnit(CRecUnitList * inList)
{
	markUnit(inList, kClaimedUnit);
}


/* -----------------------------------------------------------------------------
	Set the invalid unit flag in the unit's flags field.
	A recognizer might call invalidate after deciding that a particular grouping
	that it had previously proposed has become invalid. For instance, if a single
	vertical stroke is categorized as a character unit with interpretation 'I',
	and a subsequent stroke crosses it, the recognizer might invalidate the 'I'
	unit and create a 't' unit instead.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CRecUnit::invalidate(void)
{
	setFlags(kInvalidUnit);
}


/* -----------------------------------------------------------------------------
	Args:		inList
				inArg2
	Return:	--
----------------------------------------------------------------------------- */

NewtonErr
CRecUnit::markUnit(CRecUnitList * inList, ULong inFlags)
{
	NewtonErr err = inList->addUnit(this);
	setFlags(inFlags);
	return err;
}


/* -----------------------------------------------------------------------------
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CRecUnit::doneUsingUnit(void)
{
	setAreas(NULL);
}


/* -----------------------------------------------------------------------------
	Args:		--
	Return:	number of strokes in the unit
----------------------------------------------------------------------------- */

ULong
CRecUnit::countStrokes(void)
{
	return 0;
}


/* -----------------------------------------------------------------------------
	Args:		--
	Return:	YES => we own the stroke
----------------------------------------------------------------------------- */

bool
CRecUnit::ownsStroke(void)
{
	return NO;
}


/* -----------------------------------------------------------------------------
	Args:		index
	Return:	the stroke
----------------------------------------------------------------------------- */

CRecStroke *
CRecUnit::getStroke(ArrayIndex index)
{
	return NULL;
}


/* -----------------------------------------------------------------------------
	Args:		--
	Return:	all strokes as a unit list
----------------------------------------------------------------------------- */

CRecUnitList *
CRecUnit::getAllStrokes(void)
{
	return NULL;
}


/* -----------------------------------------------------------------------------
	Args:		inid
	Return:	--
----------------------------------------------------------------------------- */

void
CRecUnit::setContextId(ULong inId)
{ }


/* -----------------------------------------------------------------------------
	Args:		--
	Return:	context id
----------------------------------------------------------------------------- */

ULong
CRecUnit::contextId(void)
{
	return 0;
}


/* -----------------------------------------------------------------------------
maybe inline:
long			checkOverlap(TUnit *a, TUnit *b);// return overlap status of two units
long			countStrokes(TUnit *a);
long			countOverlap(TUnit *a, TUnit *b);
void			markStrokes(TUnit *a, char *ap, Long min);
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */


/* -----------------------------------------------------------------------------
	Args:		inDelay
	Return:	--
----------------------------------------------------------------------------- */

void
CRecUnit::setDelay(ULong inDelay)
{
	if (inDelay > 255)
		inDelay = 255;
	fDelay = inDelay;

	if (inDelay)
		setFlags(kDelayUnit);
	else
		unsetFlags(kDelayUnit);
}

/* -----------------------------------------------------------------------------
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

CRecArea *
CRecUnit::getArea(void)
{
	if (fAreaList)
	{
		if (testFlags(kAreasUnit))
			// fAreaList is an array
			return ((CAreaList *)fAreaList)->getMergedArea();
		else
			// fAreaList is a single area
			return (CRecArea *)fAreaList;
	}
	return NULL;
}


/* -----------------------------------------------------------------------------
	Args:		--
	Return:	the list of areas
----------------------------------------------------------------------------- */

CAreaList *
CRecUnit::getAreas(void)
{
	CAreaList * areas = NULL;
	XTRY
	{
		XFAIL(fAreaList == NULL)

		if (testFlags(kAreasUnit))
		{
			// fAreaList is an array
			areas = (CAreaList *)fAreaList;
			areas->retain();
		}
		else
		{
			// fAreaList is a single area
			XFAIL((areas = CAreaList::make()) == NULL)
			XFAILIF(areas->addArea((CRecArea *)fAreaList), areas->release(); areas = NULL;)
		}
	}
	XENDTRY;
	return areas;
}


/* -----------------------------------------------------------------------------
	Args:		inAreas
	Return:	--
----------------------------------------------------------------------------- */

void
CRecUnit::setAreas(CAreaList * inAreas)
{
	// delete any existing areas
	if (fAreaList)
		fAreaList->release(), fAreaList = NULL;
	// reset the we-are-an-array-of-areas flag
	unsetFlags(kAreasUnit);
	if (inAreas)
	{
		if (inAreas->count() > 1)
		{
			// it’s a bona fide list
			fAreaList = inAreas->retain();
			setFlags(kAreasUnit);
		}
		else if (inAreas->count() == 1)
		{
			// it’s a single area
			CRecArea * anArea = inAreas->getArea(0);
			fAreaList = anArea->retain();
		}
	}
}


/* -----------------------------------------------------------------------------
	Args:		inBox
	Return:	--
----------------------------------------------------------------------------- */

void
CRecUnit::setBBox(FRect * inBox)
{
	UnfixRect(inBox, &fBBox);
}


/* -----------------------------------------------------------------------------
	Args:		outBox
	Return:	the bounds box as well, just in case
----------------------------------------------------------------------------- */

FRect *
CRecUnit::getBBox(FRect * outBox)
{
	FixRect(outBox, &fBBox);
	return outBox;
}


#pragma mark -
#pragma mark CRecUnitList
/* -----------------------------------------------------------------------------
	C R e c U n i t L i s t
----------------------------------------------------------------------------- */

CRecUnitList *
CRecUnitList::make(void)
{
	CRecUnitList * list;
	XTRY
	{
		XFAIL((list = new CRecUnitList) == NULL)
		XFAILIF(list->iRecUnitList() != noErr, list->release(); list = NULL;)
	}
	XENDTRY;
	return list;
}


NewtonErr
CRecUnitList::iRecUnitList(void)
{
	return iDArray(sizeof(CRecUnit*), 0);
}


#pragma mark Debug

void
CRecUnitList::dump(CMsg * outMsg)
{
	for (ArrayIndex i = 0; i < count(); ++i)
		getUnit(i)->dumpName(outMsg);
	outMsg->msgLF();
}


#pragma mark List Access

NewtonErr
CRecUnitList::addUnit(CRecUnit * inUnit)
{
	NewtonErr err = noErr;
	XTRY
	{
		CRecUnit **	p;
		XFAILNOT(p = reinterpret_cast<CRecUnit **>(addEntry()), err = kOSErrNoMemory;)
		*p = inUnit;
	}
	XENDTRY;
	return err;
}


NewtonErr
CRecUnitList::addUnique(CRecUnit * inUnit)
{
	if (fSize > 0)
	{
		CRecUnit **	p = reinterpret_cast<CRecUnit **>(getEntry(0));
		for (ArrayIndex i = 0; i < fSize; ++i, ++p)
		{
			if (*p == inUnit)
				return noErr;
		}
	}
	return addUnit(inUnit);
}


CRecUnit *
CRecUnitList::getUnit(ArrayIndex index)
{
	return *reinterpret_cast<CRecUnit **>(getEntry(index));
}


void
CRecUnitList::purge(void)
{
	for (ArrayIndex i = 0; i < fSize; ++i)
		getUnit(i)->release();
}
