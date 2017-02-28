/*
	File:		Recognizers.cc

	Contains:	CRecognizer implementation.

	Written by:	Newton Research Group, 2010.
*/

#include "Objects.h"
#include "ROMResources.h"
#include "Arrays.h"
#include "Lookup.h"
#include "Recognition.h"
#include "ClickRecognizer.h"
#include "StrokeRecognizer.h"
#include "EdgeListRecognizer.h"
#include "ShapeRecognizer.h"
#include "Dictionaries.h"
#include "ViewFlags.h"
#include "Responder.h"

extern bool		OtherViewInUse(CView * inView);


CStrokeDomain *	gStrokeDomain;		// 0C101680

/* -----------------------------------------------------------------------------
	Initialize the recognizers.
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Basic recognizers.
----------------------------------------------------------------------------- */

void
InstallEventRecognizer(CRecognitionManager * inManager)
{
	CEventRecognizer *	recognizer = new CEventRecognizer;
	recognizer->init(NULL, 'CEVT', aeTap, 10, 0);
	recognizer->initServices(vGesturesAllowed, vGesturesAllowed);
	inManager->addRecognizer(recognizer);
}


void
InstallClickRecognizer(CRecognitionManager * inManager)
{
	CClickRecognizer *	recognizer = new CClickRecognizer;
	recognizer->init(NULL, 'CLIK', aeClick, 10, 2);
	recognizer->initServices(vClickable, vClickable);
	inManager->addRecognizer(recognizer);
}

/* -----------------------------------------------------------------------------
	Domain-based recognizers.
----------------------------------------------------------------------------- */

void
InstallStrokeRecognizer(CRecognitionManager * inManager)
{
	CStrokeDomain *	domain = CStrokeDomain::make(inManager->controller());
	gStrokeDomain = domain;
	CRecognizer *		recognizer = new CRecognizer;
	recognizer->init(domain, domain->getType(), aeStroke, 10, 0);
	recognizer->initServices(vStrokesAllowed, vStrokesAllowed);
	inManager->addRecognizer(recognizer);
}


void
InstallGestureRecognizer(CRecognitionManager * inManager)
{
	CEdgeListDomain *		domain = CEdgeListDomain::make(inManager->controller());
	CScrubRecognizer *	recognizer = new CScrubRecognizer;
	recognizer->init(domain, domain->getType(), aeGesture, 2, 1);
	recognizer->initServices(vGesturesAllowed, vGesturesAllowed);
	inManager->addRecognizer(recognizer);
}


void
InstallShapeRecognizer(CRecognitionManager * inManager)
{
	CGeneralShapeDomain *	domain = CGeneralShapeDomain::make(inManager->controller());
	CShapeRecognizer *		recognizer = new CShapeRecognizer;
	recognizer->init(domain, domain->getType(), aeShape, 1, 1);
	recognizer->initServices(vShapesAllowed, vShapesAllowed);
	inManager->addRecognizer(recognizer);
}


#pragma mark -
/* -----------------------------------------------------------------------------
	U t i l i t i e s
----------------------------------------------------------------------------- */

extern CController *		gController;

extern bool		CheckStrokeQueueEvents(ULong inStartTime, ULong inDuration);
extern bool		ScanStrokeQueueEvents(ULong inStartTime, ULong inDuration, bool inArg3);


bool
OnlyStrokeWritten(CStrokeUnit * inStroke)
{
	// see if we have processed a stroke that is not the one in question
	ArrayIterator iter;
	CRecUnit * unit, **	unitPtr = (CRecUnit **) gController->piecePool()->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); unitPtr = (CRecUnit **) iter.getNext(), i++)
	{
		unit = *unitPtr;
		if (unit->getType() == 'STRK'
		&& !unit->testFlags(0x40000000)
		&&  unit != inStroke)
			return false;
	}
	// see if there any still in the event queue
	return inStroke == NULL
		 || !CheckStrokeQueueEvents(inStroke->endTime(), 255);
}


bool
ClicksOnlyArea(CRecUnit * inUnit)
{
	CTypeAssoc * arbTypes = inUnit->getArea()->arbitrationTypes();
	return arbTypes->count() == 1
		 && arbTypes->getAssoc(0)->fType == 'CLIK';
}


bool
DomainOn(CRecArea * inArea, ULong inId)
{
	ArrayIterator iter;
	Assoc * group = (Assoc *)inArea->groupingTypes()->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); group = (Assoc *)iter.getNext(), i++)
	{
		if (group->fDomain->getType() == inId)
			return true;
	}
	return false;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C R e c o g n i z e r
	Wraps a CRecDomain subclass which does the actual recognition.
----------------------------------------------------------------------------- */

CRecognizer::CRecognizer()
{ }


CRecognizer::~CRecognizer()
{ }


void
CRecognizer::init(CRecDomain * inDomain, ULong inId, ULong inCmd, UChar inFlags, ULong inArbTime)
{
	fDomain = inDomain;
	fId = inId;
	fCmd = inCmd;
	fFlags = inFlags;
	fArbTime = inArbTime;
	initServices(0, 0);
}


void
CRecognizer::initServices(ULong inPossible, ULong inEnabled)
{
	fPossibleServices = inPossible;
	fEnabledServices = inEnabled;
}


CRecDomain *
CRecognizer::domain(void)
{ return fDomain; }


ULong
CRecognizer::id(void)
{ return fId; }


ULong
CRecognizer::command(void)
{ return fCmd; }


UChar
CRecognizer::flags(void)
{ return fFlags; }


bool
CRecognizer::testFlags(UChar inMask)
{ return (fFlags & inMask) != 0; }


ULong
CRecognizer::servicesPossible(void)
{ return fPossibleServices; }


ULong
CRecognizer::servicesEnabled(void)
{ return fEnabledServices; }


ULong
CRecognizer::unitConfidence(CUnit * inUnit)
{ return 0; }


void
CRecognizer::sleep(void)
{ }


void
CRecognizer::wakeUp(void)
{ }


ULong
CRecognizer::arbitrateTime(void)
{ return fArbTime; }


void
CRecognizer::buildConfig(RefArg inArg1, CView * inView, ULong inArg3)
{ }


int
CRecognizer::enableArea(CRecArea * inArea, RefArg inArg2)
{
	ULong inputMask = RINT(GetVariable(inArg2, SYMA(inputMask)));
	if ((servicesEnabled() & inputMask) != 0)
	{
		inArea->addAType(id(), gRecognitionManager.unitHandler(), arbitrateTime(), NULL);
	}
	return 0;
}


int
CRecognizer::configureArea(CRecArea * inArea, RefArg inArg2)
{ return 0; }


ULong
CRecognizer::handleUnit(CUnit * inUnit)
{ return fCmd; }


Ref
CRecognizer::getLearningData(CUnit * inUnit)
{ return NILREF; }


void
CRecognizer::doLearning(RefArg, long)
{ }


#pragma mark -
/* -----------------------------------------------------------------------------
	C E v e n t R e c o g n i z e r
----------------------------------------------------------------------------- */

ULong
CEventRecognizer::id(void)
{ return 'CEVT'; }


ULong
CEventRecognizer::handleUnit(CUnit * inUnit)
{
	ULong cmd = aeNoCommand;
	CClickEventUnit * clickUnit = (CClickEventUnit *)inUnit->siUnit();
	if (OnlyStrokeWritten(gRecognitionManager.controller()->getIndexedStroke(clickUnit->minStroke())))
	{
		switch (clickUnit->event())
		{
		case kTapClick:
			cmd = aeTap;
			break;
		case kDoubleTapClick:
			if (gRecognitionManager.prevClickView() == gRecognitionManager.clickView())
				cmd = aeDoubleTap;
			break;
		case kHiliteClick:
			cmd = aeStartHilite;
			break;
		case kTapDragClick:
			if (gRecognitionManager.prevClickView() == gRecognitionManager.clickView())
				cmd = aeTapDrag;
			break;
		}
	}
	return cmd;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C C l i c k R e c o g n i z e r
----------------------------------------------------------------------------- */

ULong
CClickRecognizer::handleUnit(CUnit * inUnit)
{
	ULong cmd = command();
	CSIUnit * rUnit = inUnit->siUnit();
	CView * rView = inUnit->findView(servicesEnabled());
	gRecognitionManager.saveClickView(rView);
	if (OtherViewInUse(rView) || gRecognitionManager.x18Value())
	{
		cmd = aeNoCommand;
		if (ClicksOnlyArea(rUnit))
			gRecognitionManager.setx38(true);
	}
	return cmd;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C S c r u b R e c o g n i z e r
----------------------------------------------------------------------------- */

ULong
CScrubRecognizer::handleUnit(CUnit * inUnit)
{
	ULong cmd = aeNoCommand;
	CSIUnit * rUnit = inUnit->siUnit();	// r7
	CRecStroke * rStroke = rUnit->getStroke(0);			// r6
	if (!gRecognitionManager.x1CValue()  ||  rStroke->startTime() > rStroke->f40Time() + 30)
	{
		UnitInterpretation * interp = rUnit->getInterpretation(0);
		int r0 = 20 - (GetTicks() - rStroke->endTime());	// wait at least 20 ticks in case of further strokes
		if (r0 > 0)
			Sleep(r0*20000/*kMacTicks*/);
		if (OnlyStrokeWritten(gRecognitionManager.controller()->getIndexedStroke(rUnit->minStroke())))
		{
			if (inUnit->isTap())
				cmd = aeTap;
			else switch (interp->label)
			{
			case 1:
				cmd = aeScrub;		// == aeGesture
				break;
			case 2:
			case 3:
			case 5:
			case 6:
				cmd = aeCaret;
				break;
			case 4:
				cmd = aeLine;
				break;
			}
		}
	}
	return cmd;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C S h a p e R e c o g n i z e r
----------------------------------------------------------------------------- */

ULong
CShapeRecognizer::handleUnit(CUnit * inUnit)
{
	ULong cmd = command();
	ULong shapeType = inUnit->shapeType();
	if (shapeType == 3 || shapeType == 15)
		cmd = aeNoCommand;
	return cmd;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C R e c o g n i z e r L i s t
----------------------------------------------------------------------------- */

CRecognizerList *
CRecognizerList::make(void)
{
	CRecognizerList * list = new CRecognizerList;
	XTRY
	{
		XFAIL(list == NULL)
		XFAILIF(list->iRecognizerList() != noErr, list->release(); list = NULL;)
	}
	XENDTRY;
	return list;
}


NewtonErr
CRecognizerList::iRecognizerList(void)
{
	return CArray::iArray(sizeof(CRecognizer *), 0);
}


void
CRecognizerList::addRecognizer(CRecognizer * inRecognizer)
{
	*reinterpret_cast<CRecognizer **>(addEntry()) = inRecognizer;
}


CRecognizer *
CRecognizerList::getRecognizer(ArrayIndex index)
{
	return *reinterpret_cast<CRecognizer **>(getEntry(index));
}


CRecognizer *
CRecognizerList::findRecognizer(RecType inId)
{
	CRecognizer **	rec = reinterpret_cast<CRecognizer **>(getEntry(0));
	for (ArrayIndex i = 0; i < count(); i++, rec++)
		if ((*rec)->id() == inId)
			return *rec;
	return NULL;
}

