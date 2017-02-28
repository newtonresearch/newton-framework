/*
	File:		EdgeListRecognizer.cc

	Contains:	EdgeList recognition domain implementation.

	Written by:	Newton Research Group, 2010.
*/

#include "EdgeListRecognizer.h"
#include "StrokeRecognizer.h"
#include "Controller.h"
#include <math.h>

extern bool		InitInterpretation(UnitInterpretation * interpretation, ULong inParamSize, ULong inNumOfParams);
extern bool		OnlyStrokeWritten(CStrokeUnit * inStroke);
extern float	DistPoint(FPoint * inPt1, FPoint * inPt2);


float AbsAngle(float inTheta) { return inTheta >= 0 ? inTheta : -inTheta; }

void	//Anon_0020E3F8
FindVertices(SamplePt * inPts, ArrayIndex inStartIndex, ArrayIndex inEndIndex, float inArg4, ULong * outNumOfVertices)
{}


/* -----------------------------------------------------------------------------
	Collapse an array of points into straight line segments.
	Args:		ioPts			array of points
	Return:	--
				ioPts => collapsed array of points
----------------------------------------------------------------------------- */

void
Collapse(CDArray * ioPts)
{
	int i, numOfPts = ioPts->count();
	FPoint * ptsBase = (FPoint *)ioPts->getEntry(0);
	if (numOfPts > 2)
	{
		for (i = 1; i < numOfPts; ++i)
		{
			FPoint * thisPt = ptsBase + i;
			FPoint * prevPt = thisPt - 1;
			if (DistPoint(prevPt, thisPt) < 7.0)
			{
				// these points are close -- collapse to a single point
				ArrayIndex iCollapse = i;
				numOfPts--;
				if (i < numOfPts)
				{
					if (i > 1)
					{
						float thetaPrev = PtsToAngleR(prevPt, prevPt-1);
						float thetaThis = PtsToAngleR(thisPt, prevPt);
						float thetaNext = PtsToAngleR(thisPt+1, thisPt);
						float deltaPrev = thetaThis - thetaPrev;
						float deltaNext = thetaNext - thetaThis;
						NORM(&deltaPrev);
						NORM(&deltaNext);
						if (AbsAngle(deltaPrev) < AbsAngle(deltaNext))
							iCollapse--;
					}
					else
						iCollapse--;
				}
				ioPts->deleteEntry(iCollapse);
				i = iCollapse - 1;
				if (i < 0)
					i = 0;
			}
		}

		float thetaBase = PtsToAngleR(ptsBase + 1, ptsBase);
		for (i = 2; i < numOfPts; ++i)
		{
			FPoint * thisPt = ptsBase + i;
			FPoint * prevPt = thisPt - 1;
			float thetaThis = PtsToAngleR(thisPt, prevPt);
			float deltaThis = thetaThis - thetaBase;
			NORM(&deltaThis);
			if (deltaThis < 0.0)
				deltaThis = -deltaThis;
			if (deltaThis < 2*M_PI/10)
			{
				// change of angle is small -- collapse this point
				ioPts->deleteEntry(i - 1);
				numOfPts--;
				i--;
				thisPt = ptsBase + i;
				thetaThis = PtsToAngleR(thisPt, thisPt - 1);
			}
			thetaBase = thetaThis;
		}
	}
}


/* -----------------------------------------------------------------------------
	Looking for a straight line:  ___
	Args:		inPts			collapsed array of points
				interp		interpretation record to complete
	Return:	true => straight line detected
----------------------------------------------------------------------------- */

bool
TestLine(CDArray * inPts, UnitInterpretation * interp)
{
	if (inPts->count() == 2)
	{
		FPoint * pt1 = (FPoint *)inPts->getEntry(0);
		FPoint * pt2 = (FPoint *)inPts->getEntry(1);
		interp->angle = PtsToAngle(pt1, pt2);	// was GetSlope
		interp->label = 4;
		interp->score = 0;
		return true;
	}
	return false;
}


void
Interpolate(FPoint * inPt1, FPoint * inPt2, float inRatio, FPoint * outPt)
{
	float scale = DistPoint(inPt1, inPt2) / inRatio;
	outPt->x = inPt1->x + scale * (inPt2->x - inPt1->x);
	outPt->y = inPt1->y + scale * (inPt2->y - inPt1->y);
}


/* -----------------------------------------------------------------------------
	Looking for a caret:  /\  or  /\_
	Args:		inPts			collapsed array of points
				interp		interpretation record to complete
	Return:	true => caret detected
----------------------------------------------------------------------------- */

bool
TestCarets(CDArray * ioPts, UnitInterpretation * interp)
{
	ArrayIndex numOfPts = ioPts->count();
	if (numOfPts == 3 || numOfPts == 4)
	{
		// fetch first three points which should form a V shape
		FPoint pt1 = *(FPoint *)ioPts->getEntry(0);	// sp20
		FPoint pt2 = *(FPoint *)ioPts->getEntry(1);	// sp18
		FPoint pt3 = *(FPoint *)ioPts->getEntry(2);	// sp10
		float d1 = DistPoint(&pt1, &pt2);	// r8
		float d2 = DistPoint(&pt2, &pt3);	// r0
		if (d2 >= d1/2)
		{
			// second stroke is at least half the length of the first stroke
			if (d1 < d2/2)
			{
				// first stroke is shorter than the second by at least half
				if (numOfPts == 4)
					return false;
				// we’re going to add a point between 2 & 3
				FPoint ptx;
				Interpolate(&pt2, &pt3, d2/d1, &ptx);
				ioPts->add();
				*(FPoint *)ioPts->getEntry(2) = ptx;
				*(FPoint *)ioPts->getEntry(3) = pt3;
				pt3 = ptx;
				numOfPts = 4;
			}
			float theta1 = PtsToAngle(&pt1, &pt2);		// was GetSlope
			float theta2 = PtsToAngle(&pt3, &pt2);		// was GetSlope
			float dtheta = DeltaAngle(theta1, theta2);
			float midtheta = MidAngle(theta1, theta2);
			interp->angle = midtheta;
			interp->score = 0;

			float adtheta = AbsAngle(dtheta);
			if (adtheta <= 110.0 && adtheta >= 70.0
			&&  midtheta >= 115.0 && midtheta <= 155.0
			&&  numOfPts == 3)
				interp->label = 6;

			else if (adtheta <= 120.0
			&&  AbsAngle(midtheta) >= 160.0
			&&  numOfPts == 3)
				interp->label = 2;

			else if (adtheta >= 100.0)
				return false;

			else if (numOfPts == 3)
				interp->label = 2;

			else
			{
				FPoint sp08 = *(FPoint *)ioPts->getEntry(3);
				float r0 = PtsToAngle(&pt3, &sp08);		// was GetSlope
				if (AbsAngle(DeltaAngle(r0, midtheta)) > 145.0)
					interp->label = 5;

				else if (AbsAngle(DeltaAngle(dtheta >= 0 ? r0 : -r0, 90.0)) < 20.0)
					interp->label = 3;

				else
					return false;
			}
			return true;
		}
	}
	return false;
}


/* -----------------------------------------------------------------------------
	Looking for a scrub:  /\/\/
	Args:		inPts			collapsed array of points
				interp		interpretation record to complete
	Return:	true => scrub detected
----------------------------------------------------------------------------- */

bool
TestScrub(CDArray * inPts, FRect * inBox, UnitInterpretation * interp)
{return false;}


#pragma mark -
#pragma mark CEdgeListDomain
/* -----------------------------------------------------------------------------
	C E d g e L i s t D o m a i n
----------------------------------------------------------------------------- */

CEdgeListDomain *
CEdgeListDomain::make(CController * inController)
{
	CEdgeListDomain * domain = new CEdgeListDomain;
	XTRY
	{
		XFAIL(domain == NULL)
		XFAILIF(domain->iEdgeListDomain(inController) != noErr, domain->release(); domain = NULL;)
	}
	XENDTRY;
	return domain;
}


NewtonErr
CEdgeListDomain::iEdgeListDomain(CController * inController)
{
	iRecDomain(inController, 'SCRB', "EdgeList");
	addPieceType('STRK');
	inController->registerDomain(this);
	return noErr;
}


bool
CEdgeListDomain::group(CRecUnit * inPiece, RecDomainInfo * info)
{
	bool isGrouped = false;
	XTRY
	{
		CAreaList * areas = inPiece->getAreas();
		CEdgeListUnit * newUnit = CEdgeListUnit::make(this, inPiece->getf24() + 1, areas);
		if (areas)
			areas->release();

		XFAIL(newUnit == NULL)

		newUnit->addSub(inPiece);
		newUnit->endSubs();
		fController->newGroup(newUnit);

		isGrouped = true;
	}
	XENDTRY;
	return isGrouped;
}


void
CEdgeListDomain::classify(CRecUnit * inUnit)
{
	CEdgeListUnit * edgeUnit = (CEdgeListUnit *)inUnit;
	if (OnlyStrokeWritten((CStrokeUnit *)edgeUnit->getSub(0)))
	{
		findCorners(edgeUnit);
		CDArray * pts;
		if ((pts = edgeUnit->getCorners()) != NULL && pts->count() < 50)
		{
			UnitInterpretation interp;
			FRect box;
			InitInterpretation(&interp, 0, 0);
			edgeUnit->getBBox(&box);
			Collapse(pts);
			if (TestLine(pts, &interp)
			||  TestCarets(pts, &interp)
			||  TestScrub(pts, &box, &interp))
			{
				edgeUnit->addInterpretation((Ptr)&interp);
				return;
			}
		}
		edgeUnit->setFlags(0x00400000);
		edgeUnit->doneUsingUnit();
	}
	else
	{
		edgeUnit->setFlags(0x00400000);
		edgeUnit->doneUsingUnit();
	}
	edgeUnit->endUnit();
	fController->newClassification(inUnit);
}


void
CEdgeListDomain::findCorners(CRecUnit * inUnit)
{
	CEdgeListUnit * edgeUnit = (CEdgeListUnit *)inUnit;
	CRecStroke * strok = ((CStrokeUnit *)edgeUnit->getSub(0))->getStroke();
	ArrayIndex i, count = strok->count();

	SamplePt * p = strok->getPoint(0);
	for (i = 0; i < count; i++, p++)
		p->unsetFlag();

	ULong numOfCorners = 0;
	p = strok->getPoint(0);
	FindVertices(p, 0, count-1, 4.0, &numOfCorners);

	CDArray * pts;
	if ((pts = CDArray::make(sizeof(FPoint), numOfCorners)) != NULL)
	{
		ArrayIndex index = 0;
		p = strok->getPoint(0);
		for (i = 0; i < count; i++, p++)
		{
			if (p->testFlag())
			{
				FPoint pt;
				p->getPoint(&pt);
				pts->setEntry(index++, (Ptr)&pt);
			}
		}
		edgeUnit->setInterpretation(pts);
		pts->release();
	}
}


#pragma mark -
#pragma mark CEdgeListUnit
/* -----------------------------------------------------------------------------
	C E d g e L i s t U n i t
----------------------------------------------------------------------------- */

CEdgeListUnit *
CEdgeListUnit::make(CRecDomain * inDomain, ULong inUnitType, CArray * inAreas)
{
	CEdgeListUnit * unit = new CEdgeListUnit;
	XTRY
	{
		XFAIL(unit == NULL)
		XFAILIF(unit->iEdgeListUnit(inDomain, inUnitType, inAreas) != noErr, unit->release(); unit = NULL;)
	}
	XENDTRY;
	return unit;
}


NewtonErr
CEdgeListUnit::iEdgeListUnit(CRecDomain * inDomain, ULong inUnitType, CArray * inAreas)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = iSIUnit(inDomain, 'SCRB', inUnitType, inAreas, 0x10))
		InitInterpretation(&fInterpretation, 0, 0);
		fNumOfInterpretations = 1;
		fCorners = NULL;
	}
	XENDTRY;
	return err;
}


void
CEdgeListUnit::dealloc(void)
{
	CDArray * corners = getCorners();
	if (corners)
		corners->release(), corners = NULL;
	// super dealloc
	CSIUnit::dealloc();
}


size_t
CEdgeListUnit::sizeInBytes(void)
{
	size_t thisSize = 0;
	CDArray * corners = getCorners();
	if (corners)
		thisSize = corners->sizeInBytes();
	thisSize += (sizeof(CEdgeListUnit) - sizeof(CSIUnit));	// original doesn’t need sizeof evaluation b/c it uses dynamic Ptr size
	return CSIUnit::sizeInBytes() + thisSize;
}


void
CEdgeListUnit::dump(CMsg * outMsg)
{
	CDArray * corners = getCorners();
	outMsg->msgStr("EdgeList: ");
	CSIUnit::dump(outMsg);
	if (corners != NULL)
	{
		char buf[256];
		sprintf(buf, " %u corners", corners->count());
		outMsg->msgStr(buf);
	}
	outMsg->msgLF();
}


ArrayIndex
CEdgeListUnit::interpretationCount(void)
{
	return fNumOfInterpretations;
}


ArrayIndex
CEdgeListUnit::addInterpretation(Ptr inPtr)
{
	memcpy(&fInterpretation, inPtr, sizeof(UnitInterpretation));
	fNumOfInterpretations = 1;
	return 0;
}


UnitInterpretation *
CEdgeListUnit::getInterpretation(ArrayIndex index)
{
	return &fInterpretation;
}


void
CEdgeListUnit::setInterpretation(CDArray * inCorners)
{
	fCorners = (CDArray *)inCorners->retain();
	endUnit();
}


void
CEdgeListUnit::endUnit(void)
{
	if (getCorners() != NULL)
		getBestInterpretation();
	CSIUnit::endUnit();
}


void
CEdgeListUnit::doneUsingUnit(void)
{
	CDArray * corners = getCorners();
	if (corners)
		corners->release(), corners = NULL;
	CSIUnit::doneUsingUnit();
}

