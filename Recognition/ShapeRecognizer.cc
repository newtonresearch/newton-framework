/*
	File:		ShapeRecognizer.cc

	Contains:	Class CGeneralShapeUnit, a subclass of CSIUnit.

	Written by:	Newton Research Group, 2010.
*/

#include "Paths.h"
#include "StrokeRecognizer.h"
#include "ShapeRecognizer.h"
#include "Controller.h"
#include "OSErrors.h"
#include <math.h>

extern bool		OnlyStrokeWritten(CStrokeUnit * inStroke);


/* -----------------------------------------------------------------------------
	D a t q
	A full circle described in 15° intervals.
	This would be more efficiently expressed in radians
	(avoiding runtime conversion).
----------------------------------------------------------------------------- */
#define kDegreesToRadians 0.0174532925

float displayAngle[25] = 
{
	0.00, 15.0, 30.0, 45.0, 60.0, 75.0,
	90.0, 105.0, 120.0, 135.0, 150.0, 165.0,
	180.0, 195.0, 210.0, 225.0, 240.0, 255.0,
	270.0, 285.0, 300.0, 315.0, 330.0, 345.0,
	360.0
};


/* -----------------------------------------------------------------------------
	S h a p e   U t i l i t i e s
----------------------------------------------------------------------------- */
typedef TabPt GeneralPt;


void
CleanPt(TabPt * ioPt)
{
	if (ioPt->x < 0.0)
		ioPt->x = 0.0;
	if (ioPt->y < 0.0)
		ioPt->y = 0.0;
}


float
DistPoint(FPoint * inPt1, FPoint * inPt2)
{
	float dx = inPt1->x - inPt2->x;
	float dy = inPt1->y - inPt2->y;
	return sqrtf(dx*dx + dy*dy);
}


bool
PtsOnCircle(CArray * inPts, FPoint * inCentre, float inRadius, ULong * outArg4)
{
	ArrayIndex numOfPtsOnCircle = 0;
	ArrayIndex numOfPts = inPts->count();
	for (ArrayIndex i = 0; i < numOfPts; ++i)
	{
		float delta = DistPoint((FPoint *)inPts->getEntry(i), inCentre);
		if (ABS(delta - inRadius) / inRadius < 0.28125)
			numOfPtsOnCircle++;
	}
	float circularity = (float)numOfPtsOnCircle / (float)numOfPts;
	if (circularity <= 0.92)
		return NO;
	*outArg4 = (1.0 - circularity) * 5000/3 + 0.5;
	return YES;
}


bool
PtsOnEllipse(CArray * inPts, FPoint * inCentre1, FPoint * inCentre2, float inRadius, ULong * outScore)
{
	ArrayIndex numOfPtsOnEllipse = 0;
	ArrayIndex i, numOfPts = inPts->count();
	ArrayIndex interval = numOfPts / 30 + 1;
	if (interval < 3)
		interval = 2;
	float sp08 = 4.0 * inRadius / 100.0;
	for (i = 0; i < numOfPts; i += interval)
	{
		FPoint pt = *(FPoint *)inPts->getEntry(i);
		float delta = DistPoint(&pt, inCentre1) +  DistPoint(&pt, inCentre2);
		if (ABS(delta - inRadius) < sp08)
			numOfPtsOnEllipse++;
	}
	long r4 = (numOfPts - 1) / interval;
	if (r4 * interval != (numOfPts - 1))
		r4++;
	float circularity = numOfPtsOnEllipse / r4;
	if (circularity <= 0.57)
		return NO;
	float r1 = numOfPtsOnEllipse * 1.5;
	if (r1 > r4)
		r1 = r4;
	*outScore = (1.0 - r1/r4) * 1500 + 0.5;
	return YES;
}


void
ooops(void)
{ /* this really does nothing */ }

// return the quadrant of one point relative to another
// 1 | 0
// -----
// 2 | 3
ULong
GetQuadPoint(FPoint * inRefPt, FPoint * inTestPt)
{
	if (inTestPt->x >= inRefPt->x)
		return (inTestPt->y >= inRefPt->y) ? 3 : 0;
//	ooops();	// huh?
	return (inTestPt->y >= inRefPt->y) ? 2 : 1;
}

bool
IsNextQuad(ULong inThisQuad, ULong inNextQuad, int * ioDirection)
{
	ULong nextCCW = (inThisQuad == 3) ? 0 : inThisQuad + 1;
	ULong nextCW = (inThisQuad == 0) ? 3 : inThisQuad - 1;
	if (*ioDirection == 0)
	{
		if (inNextQuad == nextCCW)
		{
			*ioDirection = 1;
			return YES;
		}
		else if (inNextQuad == nextCW)
		{
			*ioDirection = -1;
			return YES;
		}
	}
	else if (*ioDirection == -1)
	{
		if (inNextQuad == nextCW)
			return YES;
	}
	else // (*ioDirection == 1)
	{
		if (inNextQuad == nextCCW)
			return YES;
	}
	return NO;
}


bool
TraceContour(CArray * inPts, FPoint * inCentre)
{
	ULong thisQuad, currentQuad = GetQuadPoint(inCentre, (FPoint *)inPts->getEntry(0));
	int runLen = 1;
	int direction = 0;
	for (ArrayIndex i = 1; i < inPts->count(); i += 2)
	{
		thisQuad = GetQuadPoint(inCentre, (FPoint *)inPts->getEntry(i));
		if (thisQuad == currentQuad)
			runLen++;
		else if (runLen < 2)
			currentQuad = thisQuad;
		else if (IsNextQuad(currentQuad, thisQuad, &direction))
		{
			currentQuad = thisQuad;
			runLen = 1;
		}
		else
			return NO;
	}
	return YES;
}


void
CurvePts(curve * inCurve, ArrayIndex * outIndex, point * outPts, int inNumOfBifurcations)
{
	if (inNumOfBifurcations != 0)
	{
		curve curv1, curv2;
		curv1.first = inCurve->first;
		curv1.control.x = (inCurve->first.x + inCurve->control.x) / 2.0;
		curv1.control.y = (inCurve->first.y + inCurve->control.y) / 2.0;
		curv2.control.x = (inCurve->control.x + inCurve->last.x) / 2.0;
		curv2.control.y = (inCurve->control.y + inCurve->last.y) / 2.0;
		curv1.last.x = curv2.first.x = (curv1.control.x + curv2.control.x) / 2.0;
		curv1.last.y = curv2.first.y = (curv1.control.y + curv2.control.y) / 2.0;
		curv2.last = inCurve->last;
		inNumOfBifurcations--;
		CurvePts(&curv1, outIndex, outPts, inNumOfBifurcations);
		CurvePts(&curv2, outIndex, outPts, inNumOfBifurcations);
	}
	else
	{
		outPts[(*outIndex)++] = inCurve->last;
	}
}


void
AccessPoint(ArrayIndex index, CDArray * inArray, GeneralPt * outPt)
{
	memcpy(outPt, inArray->getEntry(index), sizeof(GeneralPt));
}


ULong
GetAvgLength(CGeneralShapeUnit * inShape)
{
	ULong avgLen = 0;
	if (inShape->interpretationCount() > 0)
	{
		UnitInterpretation * interp = inShape->getInterpretation(0);
		if (interp->label == 0  ||  interp->label == 1  ||  interp->label == 2)
		{
			FRect box;
			inShape->getBBox(&box);
			float ht = RectangleHeight(&box);
			float wd = RectangleWidth(&box);
			avgLen = MAX(ht, wd);
		}
		else if (interp->label != 15)
		{
			ArrayIndex count;
			CDArray * shp;
			if ((shp = inShape->getGeneralShape()) != NULL  &&  (count = shp->count()) > 1)
			{
				float totalLen = 0.0;
				point * pt = (point *)shp->getEntry(0);	// this ain’t right... curve?
				for (ArrayIndex i = 1; i < count; ++i)
				{
					point * thisPt = pt;
					pt += 3;											// ...but the offsets are
					point * nextPt = pt;
					totalLen += DistPoint(thisPt, nextPt);
				}
				avgLen = totalLen / (count - 1);
			}
		}
	}
	return avgLen;
}


ULong
GetGraphicBiasedScore(CSIUnit * inUnit)
{
	ULong score;
	ULong len = GetAvgLength((CGeneralShapeUnit *)inUnit);
	if (len > 30)
		len = 30;
	ULong r5 = len * 3 / 10;
	if (r5 < 1)
		r5 = 1;
	switch (inUnit->getLabel(0))
	{
	case 9:
	case 10:
	case 11:
	case 12:
		score = inUnit->getScore(0) / r5;
		break;
	default:
		score = inUnit->getScore(0);
		break;
	}
	return score;
}


void
PurgeDeep(CSIUnit * inUnit)
{
	for (ArrayIndex i = 0; i < inUnit->subCount(); ++i)
	{
		PurgeDeep((CSIUnit *)inUnit->getSub(i));
	}
	inUnit->setStartTime('Cntx');		// sic -- huh?
	inUnit->unsetFlags(0x00100000);
	inUnit->release();
}


void
PurgeDeep(CRecUnitList * inList)
{
	for (ArrayIndex i = 0; i < inList->count(); ++i)
	{
		PurgeDeep((CSIUnit *)inList->getUnit(i));
	}
}


void
DisposeFD(CGeneralShapeUnit * inShape)
{
	if (inShape->getFD() != NULL)
	{
		CSIUnit * r0 = NULL;
		for (ArrayIndex i = 0; i < 2; ++i)
		{
			CSIUnit * r5;
			if ((r5 = inShape->getFD()->x5C[i].x00) != NULL)
			{
				if (r5 != r0)
					PurgeDeep(r5);
				r0 = r5;
			}
		}
		inShape->disposeFD();
	}
}


void
GDisposeShape(CDArray * inShape)
{
	inShape->release();
}


#pragma mark -
#pragma mark CGeneralShapeUnit
/* -----------------------------------------------------------------------------
	C G e n e r a l S h a p e U n i t
----------------------------------------------------------------------------- */

CGeneralShapeUnit *
CGeneralShapeUnit::make(CRecDomain * inDomain, ULong inUnitType, CArray * inAreas)
{
	CGeneralShapeUnit * unit;
	XTRY
	{
		XFAIL((unit = new CGeneralShapeUnit) == NULL)
		XFAILIF(unit->iGeneralShapeUnit(inDomain, inUnitType, inAreas) != noErr, unit->release(); unit = NULL;)
	}
	XENDTRY;
	return unit;
}


NewtonErr
CGeneralShapeUnit::iGeneralShapeUnit(CRecDomain * inDomain, ULong inUnitType, CArray * inAreas)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = iSIUnit(inDomain, 'GSHP', inUnitType, inAreas, 0x2C))
		fInterpretationCount = 0;
		f74 = 0;
		f78 = 0;
		newInterpretation(NULL);
		XFAILNOT(f40 = (GSBlock *)NewPtr(sizeof(GSBlock)), err = kOSErrNoMemory;)
		f40->x58 = 0;
		for (ArrayIndex i = 0; i < 8; ++i)
		{
			f40->x00[i] = 0;
			f40->x08[i] = 0;
		}
		for (ArrayIndex i = 0; i < 2; ++i)
		{
			f40->x50[i] = -1;
			f40->x5C[i].x00 = NULL;
			f40->x5C[i].x08 = -1;
		}
	}
	XENDTRY;
	return err;
}


void
CGeneralShapeUnit::dealloc(void)
{
	if (interpretationCount() > 0)
	{
		UnitInterpretation * interp = getInterpretation(0);	// sic
		CDArray * shp;
		if ((shp = getGeneralShape()))
			GDisposeShape(shp);
	}
	if (testFlags(0x00100000))
	{
		for (ArrayIndex i = 0, count = subCount(); i < count; ++i)
		{
			if (getSub(i)->testFlags(0x00100000))
			{
				deleteSub(i);
				count--;
				i--;
			}
		}
	}
	DisposeFD(this);
	// super dealloc
	CSIUnit::dealloc();
}


size_t
CGeneralShapeUnit::sizeInBytes(void)
{
	size_t thisSize = 0;
	if (interpretationCount() > 0)
	{
		UnitInterpretation * interp = getInterpretation(0);
		if (interp->label == 0  ||  interp->label == 1)
			;
		else if (interp->label == 2  ||  interp->label != 15)
		{
			CDArray * shp;
			if ((shp = getGeneralShape()))
				thisSize = shp->sizeInBytes();
		}
	}

	if (f40 != NULL)
	{
		CSIUnit * r0 = NULL;
		for (ArrayIndex i = 0; i < 2; ++i)
		{
			CSIUnit * r6;
			if ((r6 = ((GSBlock *)f40)->x5C[i].x00) != NULL)
			{
				if (r6 != r0)
					thisSize += r6->sizeInBytes();
				r0 = r6;
			}
		}
		thisSize += GetPtrSize((Ptr)f40);
	}

	thisSize += (sizeof(CGeneralShapeUnit) - sizeof(CSIUnit));	// original doesn’t need sizeof evaluation b/c it uses dynamic Ptr size
	return CSIUnit::sizeInBytes() + thisSize;
}


void
CGeneralShapeUnit::dump(CMsg * outMsg)
{ }


ArrayIndex
CGeneralShapeUnit::interpretationCount(void)
{
	return fInterpretationCount;
}


ArrayIndex
CGeneralShapeUnit::addInterpretation(Ptr inData)
{
	memmove(&fInterpretation, inData, sizeof(UnitInterpretation));
	fInterpretationCount = 1;
	return 0;
}


UnitInterpretation *
CGeneralShapeUnit::getInterpretation(ArrayIndex index)
{
	return (index < fInterpretationCount) ? &fInterpretation : NULL;
}


void
CGeneralShapeUnit::newInterpretation(CDArray * inArray)
{
	InitInterpretation(&fInterpretation, 0, 0);
	fInterpretation.x10 = inArray;
	for (ArrayIndex i = 0; i < 6; ++i)
		fInterpretation.x14[i] = 0;
	fInterpretationCount = 1;
}


void
CGeneralShapeUnit::doneUsingUnit(void)
{
	if (interpretationCount() > 0)
	{
		getInterpretation(0);	// sic
		CDArray * shp;
		if ((shp = getGeneralShape()) != NULL)
		{
			GDisposeShape(shp);
			setGeneralShape(NULL);
		}
	}
	if (testFlags(0x00100000))
	{
		ArrayIndex i, count = subCount();
		for (i = 0; i < count; ++i)
		{
			if (getSub(i)->testFlags(0x00100000))
			{
				deleteSub(i);
				count--;
				i--;
			}
		}
		unsetFlags(0x00100000);
	}
	DisposeFD(this);
	CSIUnit::doneUsingUnit();
}


void
CGeneralShapeUnit::endUnit(void)
{
	DisposeFD(this);
	CSIUnit::endUnit();
}


void
CGeneralShapeUnit::setContextId(ULong inId)
{
	fContextId = inId;
}


ULong
CGeneralShapeUnit::contextId(void)
{
	return fContextId;
}


CRecStroke *
CGeneralShapeUnit::getGSAsStroke(void)
{
	CRecStroke * strok = NULL;

	GSType label = getLabel(0);
	if (label == 0  ||  label == 1)
		strok = getEllipseAsStroke();

	else if (label != 15)
	{
		NewtonErr err = noErr;
		XTRY
		{
			ArrayIndex numOfPts;	// spB4
			CDArray * pts;			// r7		TabPts
			XFAIL((pts = getGeneralShape()) == NULL)
			numOfPts = pts->count();
			XFAIL(numOfPts == 0)

			point prevPt;
			TabPt firstPt, nextPt, aPt;		// sp98, spA4, spB8
			firstPt = *(TabPt *)pts->getEntry(0);
//			memcpy(&firstPt, pts->getEntry(0), sizeof(TabPt));
			prevPt = *(point *)&firstPt;

			XFAIL((strok = CRecStroke::make(0)) == NULL)	// r4

			*(point *)&aPt = *(point *)&firstPt;
			aPt.z = 1;
			aPt.p = 0;
			CleanPt(&aPt);
			XFAIL(err = strok->addPoint(&aPt))

			for (ArrayIndex i = 1; i < numOfPts; ++i)
			{
				nextPt = *(TabPt *)pts->getEntry(i);
//				memcpy(&nextPt, pts->getEntry(i), sizeof(TabPt));
				if ((nextPt.z & 0xFF00) == 0)
				{
					if ((firstPt.z & 0xFF00) == 0)
					{
						*(point *)&aPt = *(point *)&nextPt;
						CleanPt(&aPt);
						XFAIL(err = strok->addPoint(&aPt))
					}
					else
					{
						point sp18[8];
						curve sp00;
						sp00.first = prevPt;
						sp00.control = *(point *)&firstPt;
						sp00.last = *(point *)&nextPt;
						ArrayIndex spB0 = 0;
						ULong r3;
						float delta = DistPoint(&sp00.first, &sp00.last);
						if (delta < 6.0)
							r3 = 1;
						else if (delta < 18.0)
							r3 = 2;
						else
							r3 = 3;
						ArrayIndex numOfPtsOnCurve = 1 << r3;
						CurvePts(&sp00, &spB0, sp18, r3);	// (curve*, long*, point*, long)
						for (ArrayIndex j = 0; j < numOfPtsOnCurve; j++)
						{
							*(point *)&aPt = *(point *)&sp18[j];
							CleanPt(&aPt);
							XFAIL(err = strok->addPoint(&aPt))
						}
						XFAIL(err)
					}
					prevPt = *(point *)&nextPt;
				}
				firstPt = nextPt;
			}
			XFAIL(err)
			strok->compact();
		}
		XENDTRY;
		XDOFAIL(err)
		{
			if (strok)
				strok->release(), strok = NULL;
		}
		XENDFAIL;
	}
	return strok;
}


CRecStroke *
CGeneralShapeUnit::getEllipseAsStroke(void)
{
	NewtonErr err = noErr;
	CRecStroke * strok;

	XTRY
	{
		XFAIL((strok = CRecStroke::make(0)) == NULL)
		TabPt aPt;
		aPt.z = 1;
		aPt.p = 0;
		ShapeInterpretation * interp = (ShapeInterpretation *)getInterpretation(0);
		if (interp->label == 0)
		{
			// circle
			float xc = interp->x14[0];
			float yc = interp->x14[1];
			float r = interp->x14[2];
			// step around 360° in 15° increments
			for (ArrayIndex i = 0; i < 25; ++i)
			{
				float theta = displayAngle[i] * kDegreesToRadians;
				aPt.x = xc + r * cosf(theta);
				aPt.y = yc - r * sinf(theta);
				CleanPt(&aPt);
				XFAIL(err = strok->addPoint(&aPt))
			}
			XFAIL(err)
		}

		else if (interp->label == 1)
		{
			// ellipse
			// An ellipse in general position can be expressed parametrically as the path of a point (X(t),Y(t)),
			// where
			//   X(t) = Xc + a.cos(t).cos(phi) - b.sin(t).sin(phi)
			//   Y(t) = Yc + a.cos(t).sin(phi) + b.sin(t).cos(phi)
			// as the parameter t varies from 0 to 2π.
			// Here (Xc,Yc) is the center of the ellipse,
			// and phi is the angle between the X-axis and the major axis of the ellipse.

			float xc = interp->x14[0];
			float yc = interp->x14[1];
			float a = interp->x14[2];
			float b = interp->x14[3];
			float phi = interp->x14[4] * kDegreesToRadians;
			float sinphi = sinf(phi);
			float cosphi = cosf(phi);
			// step around 360° in 15° increments
			for (ArrayIndex i = 0; i < 25; ++i)
			{
				float theta = displayAngle[i] * kDegreesToRadians;
				float acostheta = a * cosf(theta);
				float bsintheta = b * sinf(theta);
				aPt.x = xc + acostheta * cosphi - bsintheta * sinphi;
				aPt.y = yc - acostheta * sinphi - bsintheta * cosphi;
				CleanPt(&aPt);
				XFAIL(err = strok->addPoint(&aPt))
			}
		}

		else
			XFAIL(err = -1)

		strok->compact();
	}
	XENDTRY;

	XDOFAIL(err)
	{
		if (strok)
			strok->release(), strok = NULL;
	}
	XENDFAIL;

	return strok;
}


// returns array of TabPt
CDArray *
CGeneralShapeUnit::getGeneralShape(void)
{
	if (interpretationCount() > 0)
		return ((ShapeInterpretation *)getInterpretation(0))->x10;
	return NULL;
}


void
CGeneralShapeUnit::setGeneralShape(CDArray * inArray)
{
	if (interpretationCount() > 0)
		((ShapeInterpretation *)getInterpretation(0))->x10 = inArray;
}


#pragma mark -
#pragma mark CGeneralShapeDomain
/* -----------------------------------------------------------------------------
	C G e n e r a l S h a p e D o m a i n
----------------------------------------------------------------------------- */

void
CheckScreenGlobals(void)
{
#if 0
	CUGestalt gestalt;
	GestaltSystemInfo sysInfo;
	gestalt.gestalt(kGestalt_SystemInfo, &sysInfo, sizeof(sysInfo));
	gGSScreenRect.top = 0;
	gGSScreenRect.left = 0;
	gGSScreenRect.bottom = sysInfo.fScreenHeight;
	gGSScreenRect.right = sysInfo.fScreenWidth;
	if (sysInfo.fScreenResolution.h != g0C104D0C.x
	||  sysInfo.fScreenResolution.v != g0C104D0C.y)
	{
		;
	}
	ULong r5 = GetSampleRate();
	if (r5 != g0C104D14)
	{
		;
	}
	InsetRectangle(gGSScreenRect, -gPixScreenRectInset, -gPixScreenRectInset)
#endif
}

bool
CheckClosed(CStrokeUnit * inUnit)
{return NO;}


FPoint g0C107010, g0C107018;

void
ExtractEnds(CStrokeUnit * inStroke, CGeneralShapeUnit * inShape, FPoint ** outEnds)
{
	outEnds[0] = &g0C107010;
	outEnds[1] = &g0C107018;

	ArrayIndex shapeCount;
	if (inShape != NULL)
	{
		shapeCount = inShape->subCount();
		inStroke = (CStrokeUnit *)inShape->getSub(shapeCount-1);
	}

	CRecStroke * strok = inStroke->getStroke();
	ArrayIndex finalPtIndex = strok->count() - 1;
	strok->getFPoint(0, &g0C107010);
	strok->getFPoint(finalPtIndex, &g0C107018);

	if (inShape != NULL && shapeCount > 1)
	{
		GSBlock * r0 = inShape->getFD();
		if (r0->x08[shapeCount-1])
		{
			strok->getFPoint(finalPtIndex, &g0C107010);
			strok->getFPoint(0, &g0C107018);
		}
		if (r0->x00[shapeCount-1] == shapeCount-1)
			outEnds[0] = NULL;
		else
			outEnds[1] = NULL;
	}
}


bool
CheckConnect(long inArg1, FPoint ** outArg2, CGeneralShapeUnit * inShape1, CGeneralShapeUnit * inShape2)
{return NO;}


CGeneralShapeDomain *
CGeneralShapeDomain::make(CController * inController)
{
	CGeneralShapeDomain * domain;
	XTRY
	{
		XFAIL((domain = new CGeneralShapeDomain) == NULL)
		XFAILIF(domain->iGeneralShapeDomain(inController) != noErr, domain->release(); domain = NULL;)
	}
	XENDTRY;
	return domain;
}


NewtonErr
CGeneralShapeDomain::iGeneralShapeDomain(CController * inController)
{
	iRecDomain(inController, 'GSHP', "GeneralShape Domain");
	addPieceType('STRK');
	CheckScreenGlobals();
	fDelay = gRecognitionTimeout;
	inController->registerDomain(this);
	return noErr;
}


NewtonErr
CGeneralShapeDomain::preGroup(CRecUnit * inUnit)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILNOT(inUnit, err = 1;)

		CStrokeUnit * strokUnit = (CStrokeUnit *)inUnit;
		bool wasAcquired = AcquireStroke(strokUnit->getStroke());
		bool isClosed = CheckClosed(strokUnit);
		if (wasAcquired)
			ReleaseStroke();

		CRecUnitList * shapeUnits;
		XFAIL((shapeUnits = controller()->getDelayList(this, 'GSHP')) == NULL)
		if (shapeUnits->count() > 0)
		{
			FPoint * sp00;
			bool r5 = NO;
			CGeneralShapeUnit * r6 = (CGeneralShapeUnit *)shapeUnits->getUnit(0);
			if (!isClosed)
			{
				wasAcquired = AcquireStroke(strokUnit->getStroke());
				ExtractEnds(strokUnit, NULL, &sp00);
				r5 = CheckConnect(0, &sp00, NULL, r6);
				if (wasAcquired)
					ReleaseStroke();
			}
			if (!r5)
			{
				r6->endSubs();
				err = 1;
			}
			for (ArrayIndex i = 1; i < shapeUnits->count(); ++i)
			{
				((CSIUnit *)shapeUnits->getUnit(i))->endSubs();
			}
		}
		shapeUnits->release();
	}
	XENDTRY;
	return err;
}


bool
CGeneralShapeDomain::group(CRecUnit * inUnit, RecDomainInfo * info)
{
	bool isGrouped = NO;
#if 0
	r4 = 0;
	r5 = 0;
	CRecUnitList r8 = NULL;
	sp0C = 0;
	XTRY
	{
		bool sp00 = CheckClosed();
		CRecUnitList * shapeUnits;	// r6
		XFAIL((shapeUnits = controller()->getDelayList(this, 'GSHP')) == NULL)
		if (shapeUnits->count() > 0)
		{
			CGeneralShapeUnit * r9 = (CGeneralShapeUnit *)shapeUnits->getUnit(0);
			;
		}
		;
	}
	XENDTRY;

	if (shapeUnits)
		shapeUnits->release();
	if (r8)
		DisposeContextUnits(r8);
#endif
	return isGrouped;
}


struct EqSystem
{
};

long
FindEquations(CGeneralShapeUnit*, long*, EqSystem*, GSType*, ULong*, long*)
{}

bool	gSymmetryFlag;		// 0C1018BC

void
CGeneralShapeDomain::classify(CRecUnit * inUnit)
{
	//sp-33C
	CheckScreenGlobals();
//	r8 = 10000;
//	r7 = 0;
	CGeneralShapeUnit * shapeUnit = (CGeneralShapeUnit *)inUnit;
	EqSystem sp04;
	GSType sp20C = shapeUnit->getLabel(0);
#if 0
	long sp210 = 0;
	ULong sp204 = kBadScore;
	ULong sp200;
	long sp1FC;
	long sp1F8;
	FindKeyPoints(shapeUnit, &sp20C, &sp204);
	if (sp20C != 3)
	{
		sp08 = 0;
		if (sp20C != 15)
		{
			if (sp20C != 2)
			{
				long r1 = 0;
				if (gSymmetryFlag)
					r1 = FindEquations(shapeUnit, &sp210, &sp04, &sp20C, &sp204, &sp1FC);
				switch (sp20C)
					;
				if (r1 && SolveEquations(&sp04, &sp210))
					PlugNewVals(shapeUnit, &sp210, &sp04);
			}
			else
			{
				if (gGSScreenRect.x20 && gCurveFlag && FindEllipses(shapeUnit, &sp208, &sp200, &sp1F8))
				{
					sp20C = sp208;
					sp204 = MAX(300, sp200);
					sp1FC = sp1F8;
				}
			}
		}
		ReleaseEqs(&sp04);
	}
	if (sp210 == 0x80000000)
	{
		sp20C = 10;
		sp1FC = 0;
	}
	else if (sp210 == 0x40000000)
	{
		sp20C = 11;
		sp1FC = 0;
	}
	if (sp204 < 300 && sp20C != 4)
		sp204 = kBadScore;

	shapeUnit->setLabel(0, sp20C);
	shapeUnit->setScore(0, sp204);
	shapeUnit->setAngle(0, sp1FC);

	long sp00 = 0;
	if (sp20C != 15 && sp20C != 3 && sp20C != 2)
	{
		if (shapeUnit->getFD()->f58)
		{
			SnapPtToLC(shapeUnit);
			sp00 = 1;
		}
		else
		{
			GlobalTrends(shapeUnit, sp00);
		}
	}

	shapeUnit->getFD()->x00[0] = r7;
	if (sp00)
		shapeUnit->getFD()->x00[0] = 0xFF;
#endif
	shapeUnit->endUnit();
	fController->newClassification(inUnit);
}

