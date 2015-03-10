/*
	File:		Unit.cc

	Contains:	 interface to recognition unit.

	Written by:	Newton Research Group.
*/

#include "RootView.h"
#include "QDDrawing.h"

#include "Unit.h"

#include "UStringUtils.h"
#include "ROMResources.h"
#include "ROMSymbols.h"

#include "Recognition.h"
#include "StrokeRecognizer.h"
#include "ShapeRecognizer.h"
#include "WordRecognizer.h"

extern void					AdjustForInk(Rect * ioBounds);
extern PolygonShape *	AsPolygon(CRecStroke * inStroke);
extern Ref					ToObject(const Rect * inBounds);



// should probably think about giving these uniquer names
enum
{
	firstX,
	firstY,
	lastX,
	lastY,
	finalX,
	finalY,
	firstXY,
	lastXY,
	finalXY
};


ULong	gWordId;						// 0C101844
Ptr	gSaveWordTrainingData;	// 0C101864


/*--------------------------------------------------------------------------------
	C U n i t
	Was TUnitPublic.
--------------------------------------------------------------------------------*/

CUnit::CUnit(CRecUnit * inUnit, ULong inContextId)
{
	f00 = (CSIUnit *)inUnit;	// perhaps f00 really is a CRecUnit?
	f14 = NULL;
	f18 = NULL;
	f1C = NULL;
	f34 = 0;
	f04 = NULL;
//	f30 = NILREF;	// redundant
	f28.left = f28.right = -32768;
}


CUnit::~CUnit()
{
	if (f1C)
		delete f1C;
	if (f14)
		KillPoly(f14);
	if (f18)
		KillPoly(f18);
	if (f04)
		delete f04;
}


ULong
CUnit::contextId(void)
{
	return f00->contextId();
}


void
CUnit::bounds(Rect * outRect)
{
	CRecognizer * recognizer = gRecognitionManager.findRecognizer(getType());
	outRect->left = outRect->right = -32768;
	XTRY
	{
		XFAIL(recognizer == NULL)
		if (recognizer->testFlags(8))
		{
			CStroke * strok;
			XFAIL((strok = stroke()) == NULL)
			strok->bounds(outRect);
		}
		else
		{
			FRect box;
			f00->getBBox(&box);
			UnfixRect(&box, outRect);
			outRect->right++;
			outRect->bottom++;
		}
	}
	XENDTRY;
}


void
CUnit::cleanUp(void)
{
	if (getType() == 'CLIK')
	{
		stroke()->inkOff(YES);
		gStrokeWorld.invalidateCurrentStroke();
	}
}


ULong
CUnit::startTime(void)
{
	return f00->getStartTime();
}


ULong
CUnit::endTime(void)
{
	CStrokeUnit * strok = gController->getIndexedStroke(f00->maxStroke());
	return strok->getEndTime();
}


int
CUnit::gestureAngle(void)
{
	UnitInterpretation interp = *f00->getInterpretation(0);
	int theta, angle = interp.angle + 0.5;
	int delta = interp.label == 5 ? 30 : 20;

	theta = angle;
	if (theta < 0.0) theta = -theta;
	if (theta <= delta)
		return 0;

	theta = angle - 90;
	if (theta < 0.0) theta = -theta;
	if (theta <= delta)
		return 90;

	theta = angle + 90;
	if (theta < 0.0) theta = -theta;
	if (theta <= delta)
		return -90;

	theta = angle - 180;
	if (theta < 0.0) theta = -theta;
	if (theta <= delta)
		return 180;

	theta = angle - 135;
	if (theta < 0.0) theta = -theta;
	if (theta <= delta)
		return 135;

	theta = angle + 180;
	if (theta < 0.0) theta = -theta;
	if (theta <= delta)
		return 180;

	return angle;
}


Point
CUnit::gesturePoint(long inArg)
{
	TabPt sp00;
//	memcpy(&sp00, f00->f3C->vf1C(inArg), sizeof(TabPt));

	Point thePt;
	thePt.h = sp00.x + 0.5;
	thePt.v = sp00.y + 0.5;
	return thePt;
}


bool
CUnit::isTap(void)
{
	Rect box;
	bounds(&box);
	return box.right - box.left < 6
		 && box.bottom - box.top < 6;
}


RecType
CUnit::getType(void)
{
	return f00->getType();
}


int
CUnit::caretType(void)
{
	UnitInterpretation interp = *f00->getInterpretation(0);
	if (interp.label == 2 || interp.label == 3 || interp.label == 5 || interp.label == 6)
		return interp.label;
	return 0;
}

#pragma mark Shapes

PolygonShape *
CUnit::cleanShape(void)
{
	CRecStroke * strok;
	if (f14 == NULL
	&&  getType() == 'GSHP'
	&& (strok = ((CGeneralShapeUnit *)f00)->getGSAsStroke()) != NULL)
	{
		f14 = AsPolygon(strok);
		strok->release();
	}
	return f14;
}


PolygonShape *
CUnit::roughShape(void)
{
	if (f18 == NULL)
		f18 = AsPolygon(f00->getStroke(0));

	return f18;
}


ULong
CUnit::shapeType(void)
{
	if (f00->getType() == 'GSHP')
		return f00->getLabel(0);
	return 0;
}


CStroke *
CUnit::stroke(void)
{
	CRecStroke * strok;
	if (f1C == NULL
	&&  (strok = f00->getStroke(0)) != NULL)
		f1C = CStroke::make(strok, NO);

	return f1C;
}


Ref
CUnit::strokes(void)
{
	return GetFrameSlot(wordInfo(), SYMA(strokes));
}


Ref
CUnit::trainingData(void)
{
	CRecognizer * recognizer;
	if (gSaveWordTrainingData[00] != 0
	&& (recognizer = gRecognitionManager.findRecognizer(getType())) != NULL)
		return recognizer->getLearningData(this);
	return NILREF;
}

#pragma mark Words

void
CUnit::setWordBase(void)
{
#if 0
	FPoint sp00, sp08;
	((CStdWordUnit *)f00)->getWordBase(*sp08, &sp00, 0);
	f28.top = sp08.y + 0.5;
	f28.left = sp08.x + 0.5;
	f28.bottom = sp00.y + 0.5;
	f28.right = sp00.x + 0.5;

	UniChar ** str = ((CStdWordUnit *)f00)->getString(0);
	if (str != NULL
	&&  Ustrlen(*str) == 1
	&& (*str[0] == '?' || *str[0] == '!'))
	{
		Rect box;
		bounds(&box);
		f28.top = f28.bottom = box.bottom;
	}
#endif
}


CWordList *
CUnit::makeWordList(bool inArg1, bool inArg2)
{
	CWordList * wordList = NULL;
	XTRY
	{
		XFAIL(getType() != gWordId)
		XFAIL((wordList = new CWordList) == NULL)
		ArrayIndex i, count = f00->interpretationCount();
#if 0
		r6 = 0; r8 = 0;
		// INCOMPLETE
#endif
	}
	XENDTRY;
	return wordList;
}


CWordList *
CUnit::extractWords(void)
{
	if (f04 == NULL)
		f04 = makeWordList(NO, NO);
	return f04;
}


void *
CUnit::word(void)
{
	return extractWords()->word(0);
}


ULong
CUnit::wordScore(void)
{
	return extractWords()->score(0);
}


CWordList *
CUnit::words(void)
{
	CWordList * wordList = f04;
	f04 = NULL;
	return wordList;
}


Ref
MakeWordInfo(CUnit * inUnit)
{return AllocateFrame();}


Ref
CUnit::wordInfo(void)
{
	if (ISNIL(f30))
		f30 = MakeWordInfo(this);
	return f30;
}

#pragma mark Masks

ULong
CUnit::inputMask(void)
{
	ULong mask = 0;
	CView * view;
	if ((view = findView(requiredMask())) != NULL)
		mask = view->viewFlags & vRecognitionAllowed;
	return mask;
}


ULong
CUnit::requiredMask(void)
{
	ULong mask = 0;
	CRecognizer * recognizer;
	if ((recognizer = gRecognitionManager.findRecognizer(getType())) != NULL)
		mask = recognizer->servicesEnabled();
	if ((mask & vStrokesAllowed) != 0)
		mask |= vWordsAllowed;
	if ((mask & vGesturesAllowed) != 0)
		mask |= vClickable;
	return mask;
}

#pragma mark Views

void
CUnit::setViewHit(CView * inView, ULong inFlags)
{
	f34 = inView;
	f38 = inFlags;
}


CView *
CUnit::findView(ULong inFlags)
{
	if (f34 != NULL
	&&  f38 == inFlags)
		return f34;

	CView * theView;
	if (gArbiter->getf21())
	{
		theView = gRootView->findView(MidPoint(&gRootView->viewBounds), vAnythingAllowed, NULL);
	}
	else
	{
		Rect unitBounds;
		bounds(&unitBounds);
		theView = gRootView->findView(MidPoint(&unitBounds), inFlags, NULL);
		if (theView == NULL)
		{
			Point xPt = { 10, 10 };
			theView = gRootView->findView(MidPoint(&unitBounds), inFlags, &xPt);
		}
	}
	setViewHit(theView, inFlags);
if (theView) printf("CUnit::findView(0x%08X) -> t:%d,l:%d,b:%d,r:%d\n", inFlags, theView->viewBounds.top, theView->viewBounds.left, theView->viewBounds.bottom, theView->viewBounds.right);
	return theView;
}


void
CUnit::invalidate(void)
{
	if (f1C)
		f1C->invalidate();

	else
	{
		Rect unitBounds;
		bounds(&unitBounds);
		AdjustForInk(&unitBounds);
		gRootView->smartInvalidate(&unitBounds);

		Rect dirtyBounds, invalidBounds;
		SetEmptyRect(&dirtyBounds);
		SetEmptyRect(&invalidBounds);
		for (ArrayIndex i = 0, count = f00->countStrokes(); i < count; ++i)
		{
			Rect strokeBounds;
			CRecStroke * strok = f00->getStroke(i);
			GetStrokeRect(strok, &strokeBounds);
			AdjustForInk(&strokeBounds);
			if (strok->testFlags(kBusyUnit))
				UnionRect(&strokeBounds, &invalidBounds, &invalidBounds);
			else
			{
				UnionRect(&strokeBounds, &dirtyBounds, &dirtyBounds);
				strok->setFlags(kPartialUnit);
			}
		}
		if (!EmptyRect(&invalidBounds))
			gRootView->smartInvalidate(&invalidBounds);
		if (!EmptyRect(&dirtyBounds))
			gRootView->smartScreenDirty(&dirtyBounds);
	}
}


#pragma mark -
/*--------------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
--------------------------------------------------------------------------------*/

extern "C"
{
Ref		FInkOff(RefArg inRcvr, RefArg inUnit);
Ref		FInkOffUnHobbled(RefArg inRcvr, RefArg inUnit);
Ref		FInkOn(RefArg inRcvr, RefArg inUnit);

Ref		FGetUnitStartTime(RefArg inRcvr, RefArg inUnit);
Ref		FGetUnitEndTime(RefArg inRcvr, RefArg inUnit);
Ref		FGetUnitDownTime(RefArg inRcvr, RefArg inUnit);
Ref		FGetUnitUpTime(RefArg inRcvr, RefArg inUnit);

Ref		FStrokeBounds(RefArg inRcvr, RefArg inUnit);
Ref		FStrokeDone(RefArg inRcvr, RefArg inUnit);

Ref		FGetPoint(RefArg inRcvr, RefArg inSelector, RefArg inUnit);
Ref		FGetPointsArray(RefArg inRcvr, RefArg inUnit);
Ref		FGetPointsArrayXY(RefArg inRcvr, RefArg inUnit);

Ref		FCountGesturePoints(RefArg inRcvr, RefArg inUnit);
Ref		FGesturePoint(RefArg inRcvr, RefArg index, RefArg inUnit);
Ref		FGestureType(RefArg inRcvr, RefArg inUnit);

Ref		FGetScoreArray(RefArg inRcvr, RefArg inUnit);
Ref		FGetWordArray(RefArg inRcvr, RefArg inUnit);

Ref		FGetPolygons(RefArg inRcvr, RefArg inUnit);
Ref		FDrawPolygons(RefArg inRcvr, RefArg inPen);	// more of a drawing function, really
};


CUnit *
UnitFromRef(RefArg inRef)
{
	CUnit * unit = (CUnit *)RefToAddress(inRef);
	if (unit == NULL)
		ThrowMsg("NULL unit");
	return unit;
}


CStroke *
StrokeFromRef(RefArg inRef)
{
	CStroke * strok = UnitFromRef(inRef)->stroke();
	if (strok == NULL)
		ThrowMsg("NULL stroke");
	return strok;
}


Ref
FInkOff(RefArg inRcvr, RefArg inUnit)
{
	CStroke * strok = StrokeFromRef(inUnit);
	strok->inkOff(YES);
	return TRUEREF;
}


Ref
FInkOffUnHobbled(RefArg inRcvr, RefArg inUnit)
{
	CStroke * strok = StrokeFromRef(inUnit);
	strok->inkOff(YES, NO);
	return TRUEREF;
}


Ref
FInkOn(RefArg inRcvr, RefArg inUnit)
{
	CStroke * strok = StrokeFromRef(inUnit);
	strok->inkOn();
	return TRUEREF;
}


Ref
FGetUnitStartTime(RefArg inRcvr, RefArg inUnit)
{
	CUnit * unit = UnitFromRef(inRcvr);
	return MAKEINT(unit->startTime());
}


Ref
FGetUnitEndTime(RefArg inRcvr, RefArg inUnit)
{
	CUnit * unit = UnitFromRef(inRcvr);
	return MAKEINT(unit->endTime());
}


Ref
FGetUnitDownTime(RefArg inRcvr, RefArg inStroke)
{
	CStroke * strok = StrokeFromRef(inStroke);
	return MAKEINT(strok->downTime());
}


Ref
FGetUnitUpTime(RefArg inRcvr, RefArg inStroke)
{
	CStroke * strok = StrokeFromRef(inStroke);
	return MAKEINT(strok->upTime());
}


Ref
FStrokeBounds(RefArg inRcvr, RefArg inUnit)
{
	CUnit * unit = UnitFromRef(inUnit);
	Rect bounds;
	unit->bounds(&bounds);
	return ToObject(&bounds);
}


Ref
FStrokeDone(RefArg inRcvr, RefArg inUnit)
{
	CStroke * strok = StrokeFromRef(inUnit);
	return MAKEBOOLEAN(strok->isDone());
}


Ref
FGetPoint(RefArg inRcvr, RefArg inSelector, RefArg inUnit)
{
	CStroke * strok = StrokeFromRef(inUnit);
	int selector = RINT(inSelector);
	int ordinate = 0;
	RefVar thePt;
	switch (selector)
	{
	case firstX:
		ordinate = strok->firstPoint().h;
		break;
	case firstY:
		ordinate = strok->firstPoint().v;
		break;
//	case lastX:
//	case lastY:
	case finalX:
		ordinate = strok->finalPoint().h;
		break;
	case finalY:
		ordinate = strok->finalPoint().v;
		break;

	case firstXY:
		{
			Point pt = strok->firstPoint();
			thePt = Clone(RA(canonicalPoint));
			SetFrameSlot(thePt, SYMA(x), pt.h);
			SetFrameSlot(thePt, SYMA(y), pt.v);
		}
		break;
//	case lastXY:
	case finalXY:
		{
			Point pt = strok->finalPoint();
			thePt = Clone(RA(canonicalPoint));
			SetFrameSlot(thePt, SYMA(x), pt.h);
			SetFrameSlot(thePt, SYMA(y), pt.v);
		}
		break;
	}
	if (ISNIL(thePt))
		thePt = MAKEINT(ordinate);
	return thePt;
}


Ref
FGetPointsArray(RefArg inRcvr, RefArg inUnit)
{
	CStroke * strok = StrokeFromRef(inUnit);
	ArrayIndex numOfPoints = strok->count();		// pt -> v, h
	RefVar thePoints(MakeArray(numOfPoints*2));
	for (ArrayIndex i = 0; i < numOfPoints; ++i)
	{
		Point pt = strok->getPoint(i);
		SetArraySlot(thePoints, i*2, MAKEINT(pt.h));
		SetArraySlot(thePoints, i*2+1, MAKEINT(pt.v));
	}
	return thePoints;
}


Ref
FGetPointsArrayXY(RefArg inRcvr, RefArg inUnit)
{
	CStroke * strok = StrokeFromRef(inUnit);
	ArrayIndex numOfPoints = strok->count();		// pt -> x, y
	RefVar thePoints(MakeArray(numOfPoints*2));
	for (ArrayIndex i = 0; i < numOfPoints; ++i)
	{
		Point pt = strok->getPoint(i);
		SetArraySlot(thePoints, i*2, MAKEINT(pt.h));
		SetArraySlot(thePoints, i*2+1, MAKEINT(pt.v));
	}
	return thePoints;
}


Ref
FCountGesturePoints(RefArg inRcvr, RefArg inUnit)
{
	CUnit * unit = UnitFromRef(inUnit);
//	return MAKEINT(unit->countGesturePoints());
	return MAKEINT(0);
}


Ref
FGesturePoint(RefArg inRcvr, RefArg index, RefArg inUnit)
{
	CUnit * unit = UnitFromRef(inUnit);
	//INCOMPLETE
	return NILREF;
}


Ref
FGestureType(RefArg inRcvr, RefArg inUnit)
{
	CUnit * unit = UnitFromRef(inUnit);
	return MAKEINT(unit->caretType());
}


Ref
FGetScoreArray(RefArg inRcvr, RefArg inUnit)
{
	RefVar scoreArray;
	CUnit * unit = UnitFromRef(inUnit);
	CWordList * words = unit->words();
	if (words)
	{
//		scoreArray = MakeScoreArray(words);
		delete words;
	}
	return scoreArray;
}


Ref
FGetWordArray(RefArg inRcvr, RefArg inUnit)
{
	RefVar wordArray;
	CUnit * unit = UnitFromRef(inUnit);
	CWordList * words = unit->words();
	if (words)
	{
//		wordArray = MakeStringArray(words);
		delete words;
	}
	return wordArray;
}


Ref
FGetPolygons(RefArg inRcvr, RefArg inUnit)
{
	CView * view = FailGetView(inRcvr);
	CUnit * unit = UnitFromRef(inUnit);
	RefVar polygons(MakeArray(0));

	//INCOMPLETE

	return polygons;
}


Ref
FDrawPolygons(RefArg inRcvr, RefArg inPen) { return NILREF; }

