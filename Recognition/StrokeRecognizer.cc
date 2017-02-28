/*
	File:		StrokeRecognizer.cc

	Contains:	Class CSIUnit, a subclass of CRecUnit, provides the mechanism
					for adding subunits and interpretations.

	Written by:	Newton Research Group, 2010.
*/

#include "StrokeRecognizer.h"
#include "ClickRecognizer.h"
#include "Controller.h"
#include <math.h>

extern void		UnbufferStroke(CRecStroke * inStroke);
extern bool		PtsOnCircle(CArray * inPts, FPoint * inCentre, float inRadius, ULong * outArg4);
extern bool		TraceContour(CArray * inPts, FPoint * inCentre);


#pragma mark CStrokeDomain
/*--------------------------------------------------------------------------------
	C S t r o k e D o m a i n
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Args:		inController
	Return:	a domain instance
--------------------------------------------------------------------------------*/

CStrokeDomain *
CStrokeDomain::make(CController * inController)
{
	CStrokeDomain * domain = new CStrokeDomain;
	XTRY
	{
		XFAIL(domain == NULL)
		XFAILIF(domain->iStrokeDomain(inController) != noErr, domain->release(); domain = NULL;)
	}
	XENDTRY;
	return domain;
}


NewtonErr
CStrokeDomain::iStrokeDomain(CController * inController)
{
	iRecDomain(inController, 'STRK', "Stroke");
	addPieceType('CLIK');
	fController->registerDomain(this);
	return noErr;
}


/*--------------------------------------------------------------------------------
	The controller calls group whenever a unit of a type requested by the domain
	as input is received.
	Args:		inPiece
				info
	Return:	long
--------------------------------------------------------------------------------*/

bool
CStrokeDomain::group(CRecUnit * inPiece, RecDomainInfo * info)
{
	bool isGrouped = false;
	XTRY
	{
		CRecStroke * stroke = ((CClickUnit *)inPiece)->clikStroke();
		inPiece->setBBox(stroke->bounds());
		inPiece->setDuration(GetTicks() - inPiece->getStartTime());
		if (stroke->isDone())
		{
			inPiece->setDuration(stroke->endTime() - inPiece->getStartTime());
			stroke->unsetFlags(kDelayUnit);

			CAreaList * areas = inPiece->getAreas();
			CStrokeUnit * newUnit = CStrokeUnit::make(this, 2, stroke, areas);
			if (areas)
				areas->release();
			XFAIL(newUnit == NULL)

			stroke->retain();
			newUnit->addSub(inPiece);
			newUnit->endSubs();
			fController->newGroup(newUnit);

			UnbufferStroke(stroke);
			isGrouped = true;
		}
	}
	XENDTRY;
	return isGrouped;
}


/*--------------------------------------------------------------------------------
	The controller calls classify when the delay, if specified, has occurred, or
	after the group method calls CSIUnit::endSub.
	Args:		inUnit
	Return:	--
--------------------------------------------------------------------------------*/

void
CStrokeDomain::classify(CRecUnit * inUnit)
{
	fController->newClassification(inUnit);
}


#pragma mark -
#pragma mark CStrokeUnit
/*--------------------------------------------------------------------------------
	C S t r o k e U n i t
	A unit that holds a single stroke.
--------------------------------------------------------------------------------*/

CStrokeUnit *
CStrokeUnit::make(CRecDomain * inDomain, ULong inId, CRecStroke * inStroke, CArray * inAreas)
{
	CStrokeUnit * strokeUnit = new CStrokeUnit;
	XTRY
	{
		XFAIL(strokeUnit == NULL)
		XFAILIF(strokeUnit->iStrokeUnit(inDomain, inId, inStroke, inAreas) != noErr, strokeUnit->release(); strokeUnit = NULL;)
	}
	XENDTRY;
	return strokeUnit;
}


NewtonErr
CStrokeUnit::iStrokeUnit(CRecDomain * inDomain, ULong inId, CRecStroke * inStroke, CArray * inAreas)
{
	NewtonErr  err = iSIUnit(inDomain, 'STRK', inId, inAreas, sizeof(UnitInterpretation));
	fContextId = 0;
	fStroke = inStroke;
	setBBox(fStroke->bounds());
	fStartTime = fStroke->startTime();
	fDuration = fStroke->duration();
	return err;
}


void
CStrokeUnit::dealloc(void)
{
	if (fUnitType == 'STRK')
		fStroke->release();
	// super dealloc
	CSIUnit::dealloc();
}


#pragma mark Debug

size_t
CStrokeUnit::sizeInBytes(void)
{
	size_t thisSize = 0;
	CAreaList * list = getAreas();
	if (list)
		for (ArrayIndex i = 0; i < list->count(); ++i)
			thisSize += list->getArea(i)->sizeInBytes();
	if (list)
		list->release();
	thisSize += fStroke->sizeInBytes();
	thisSize += (sizeof(CStrokeUnit) - sizeof(CSIUnit));	// original doesn’t need sizeof evaluation b/c it uses dynamic Ptr size
	return CSIUnit::sizeInBytes() + thisSize;
}

void
CStrokeUnit::dump(CMsg * outMsg)
{  /* this really does nothing */	}


void
CStrokeUnit::setContextId(ULong inId)
{
	fContextId = inId;
}


ULong
CStrokeUnit::contextId(void)
{
	return fContextId;
}

#pragma mark Strokes

ULong
CStrokeUnit::countStrokes(void)
{
	return 1;
}


bool
CStrokeUnit::ownsStroke(void)
{
	return true;
}


CRecStroke *
CStrokeUnit::getStroke(ULong index)
{
	return fStroke;
}


CRecUnitList *
CStrokeUnit::getAllStrokes(void)
{
	CRecUnitList * list;
	XTRY
	{
		XFAIL((list = CRecUnitList::make()) == NULL)
		XFAILIF(list->addUnique(this) != noErr, list->release(); list = NULL;)
	}
	XENDTRY;
	return list;
}


CArray *
CStrokeUnit::getPts(void)
{
	CArray * list;
	XTRY
	{
		ArrayIndex count = fStroke->count();
		XFAIL((list = CArray::make(sizeof(FPoint), count)) == NULL)
		SamplePt *  p = fStroke->getPoint(0);
		FPoint *		pt = (FPoint *)list->getEntry(0);
		for (ArrayIndex i = 0; i < count; ++i, ++p, ++pt)
			p->getPoint(pt);
	}
	XENDTRY;
	return list;
}


bool
CStrokeUnit::isCircle(FPoint * inCentre, float * inArg2, ULong * inArg3)
{
	bool itsACircle = false;
	XTRY
	{
		FRect box;
		CArray * pts = getPts();
		XFAIL(pts == NULL)
		getBBox(&box);
		float wd = RectangleWidth(&box);
		float ht = RectangleHeight(&box);
		float aspectRatio = (wd >= ht) ? wd/ht : ht/wd;
		if (aspectRatio <= 1.375
		&&  PtsOnCircle(pts, inCentre, *inArg2, inArg3)
		&&  TraceContour(pts, inCentre))
			itsACircle = true;
		pts->release();
	}
	XENDTRY;
	return itsACircle;
}

/*
void
SetupEllipseSystem(CArray * inPts, float, float, float, float, float*, float*)
{}

bool
MakeEllipseTemplate(FRect*, long*, FPoint * inCentre, FPoint * outCentre1, FPoint * outCentre2, long*, long*, long*, long*)
{return true;}

extern bool		PtsOnEllipse(CArray * inPts, FPoint * inCentre1, FPoint * inCentre2, float inRadius, ULong * outScore);

void
Decomp(int, int, float *, float *, float *)
{}

void
Solve(int, int, float *, float *, float *)
{}
*/

bool
CStrokeUnit::isEllipse(FPoint * inCentre, long * inArg2, long * inArg3, long * inArg4, ULong * outScore)
{
	bool itsAnEllipse = false;
	XTRY
	{
		FRect box;	// sp90
		CArray * pts = getPts();
		XFAIL(pts == NULL)
		getBBox(&box);
		float wd = RectangleWidth(&box);
		float ht = RectangleHeight(&box);
		float r0 = MAX(wd, ht) / 4;
		float sp18, sp2C;
/*
		SetupEllipseSystem(pts, r0, box.left, box.top, 1.0, &sp2C, &sp18);
		long spA0, sp14;
		Decomp(5, 5, &sp2C, &spA0, &sp14);
		int r7 = 8;
		if (wd > ht*2)
			r7 = 7;
		else if (ht > wd*2)
			r7 = 9;
		Solve(r7, 5, &sp2C, &spA0, &sp18);
		FPoint centre1, centre2;
		float radius;
		if (MakeEllipseTemplate(&box, &sp18, inCentre, &centre1, &centre2, inArg2, inArg3, &radius, inArg4)
		&&  PtsOnEllipse(pts, &centre1, &centre2, radius, outScore)
		&&  TraceContour(pts, inCentre))
			itsAnEllipse = true;
*/
		pts->release();
	}
	XENDTRY;
	return itsAnEllipse;
}

