/*
	File:		Recognition.cc

	Contains:	CRecognition implementation.

	Written by:	Newton Research Group.
*/

//#include "MacMemory.h"

#include "Objects.h"
#include "Globals.h"
#include "Arrays.h"
#include "NewtonTime.h"
#include "Locales.h"
#include "Preference.h"
#include "NewtonScript.h"

#include "RootView.h"
#include "ViewFlags.h"
#include "Application.h"
#include "Recognition.h"
#include "ClickRecognizer.h"
#include "StrokeRecognizer.h"
#include "Dictionaries.h"
#include "QDDrawing.h"

extern void		InstallEventRecognizer(CRecognitionManager * inManager);
extern void		InstallClickRecognizer(CRecognitionManager * inManager);
extern void		InstallStrokeRecognizer(CRecognitionManager * inManager);
extern void		InstallGestureRecognizer(CRecognitionManager * inManager);
extern void		InstallShapeRecognizer(CRecognitionManager * inManager);
extern void		InstallWRecRecognizer(CRecognitionManager * inManager);
extern bool		ClicksOnlyArea(CRecUnit * inUnit);
extern void		SafeExceptionNotify(Exception * inException);

// from Ink.cc
extern void		AdjustForInk(Rect * ioBounds);

// from Journal.cc
extern int		gJournallingState;
extern void		JournalRecordAStroke(CRecStroke * inStroke);

extern bool		FromObject(RefArg inObj, Rect * outBounds);
extern Ref		ToObject(const Rect * inBounds);
extern Ref		BuildRecConfig(CView * inView, ULong inFlags);
extern Ref		BuildRCProto(CView * inView, RefArg inConfig);



typedef CRecUnitList * (*GetContextUnitsProcPtr)(CRecUnit * inUnit, long inArg2);


/* -----------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
----------------------------------------------------------------------------- */

extern "C" {
Ref	FModalRecognitionOn(RefArg rcvr, RefArg inRect);
Ref	FModalRecognitionOff(RefArg rcvr);
}

/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

extern ULong			gRecMemErrCount;
extern bool				gInhibitPopup;


CDArray *				gAreaCache;					// 0C1008A0
//CStrokeDomain *		gStrokeDomain;				// 0C101680
ULong						gLastInkWordWarning;		// 0C101684
ULong						gRecInkNotifyFlags;		// 0C101688

ULong						gRecognitionLetterSpacing;		// 0C101858
CController *			gController;				// 0C10187C
CArbiter *				gArbiter;					// 0C101880
CRecDomain *			gRootDomain;				// 0C101884
CStrokeCentral			gStrokeWorld;				// 0C1018CC
CRecognitionManager  gRecognitionManager;		// 0C106E88	

GetContextUnitsProcPtr	gContextUnitProc;		// 0C104D08


/* -----------------------------------------------------------------------------
	P r i v a t e   D a t a   S t r u c t u r e s
----------------------------------------------------------------------------- */

struct _RecognitionState
{
	ULong	x00;
	ULong	x04;
	UChar	x08;
	UChar	x09;
	ULong	x0C;
	ULong	x10;
//	size 0x14
};


/* -----------------------------------------------------------------------------
	We donÕt generally do ticks (60/sec), theyÕre a Mac legacy.
	But for stroke time measurement we need something of that order so
	FOR THE RECOGNITION SYSTEM ONLY we have a GetTicks() function.
	Args:		--
	Return:	50ths of a second counter
----------------------------------------------------------------------------- */
#include "NewtonTime.h"

ULong
GetTicks(void)
{
	CTime now(GetGlobalTime());
	return now / (20*kMilliseconds);
}


#pragma mark -
/* -----------------------------------------------------------------------------
	I G   I n t e r f a c e

	DonÕt know what IG means.
	GC prefix is for Group and Classify.

extern void		IGGetStrokesQueue(void * inData, CStrokeUnit ** * outStrokes, ArrayIndex * outCount);
extern void		IGGroupAndCompressStrokes(VAddr, CStrokeUnit *, ULong, bool, Ptr *);
extern void		UpdateStroke(CStrokeUnit * inStrokeUnit, FRect * inBox);
extern void		UpdateStrokesInList(CRecUnitList * inList, FRect * inBox);

000eabec	IGCompressStrokes(unsigned long, char**, GroupDataStruct**, GCGroupParmStruct*, short, unsigned int, unsigned int*)
000eafd4	IGAllocTrace(GroupDataStruct*, unsigned char*, short, PS_point_type**, short*)
000eb1e8	IGAddSroke(char***, GroupDataStruct**, TStrokeUnit*)
000eb300	IGGetRealStrokeIndex(GroupDataStruct*, unsigned long)
----------------------------------------------------------------------------- */

struct GroupDataStruct
{
	ArrayIndex			x2C;		// num of strokes
	int					x30;
	CStrokeUnit *		x34;
//size+84
};

void
IGNewGroupData(Ptr * ioData)		// original creates a Handle
{
	char * grpData;
	XTRY
	{
		XFAIL((grpData = NewPtr(sizeof(GroupDataStruct))) == NULL)
		memset(grpData, 0, sizeof(GroupDataStruct));
		((GroupDataStruct *)grpData)->x30 = 20;
	}
	XENDTRY;
	*ioData = grpData;
}


void
IGDisposeGroupData(Ptr * ioData)		// original uses a Handle
{
	if (*ioData != NULL)
	{
//		GCDisposeGResHandle(*ioData);
		FreePtr(*ioData);
		*ioData = NULL;
	}
}


ArrayIndex
IGGetNumOfStrokes(GroupDataStruct * inData)
{
	return inData ? inData->x2C : 0;
}


void
IGGetStrokesQueue(void * inData, CStrokeUnit ** * outStrokes, ArrayIndex * outCount)
{
	if (inData)
	{
		GroupDataStruct * grpData = (GroupDataStruct *)inData;
		*outStrokes = &grpData->x34;
		*outCount = IGGetNumOfStrokes(grpData);
	}
	else
	{
		*outStrokes = NULL;
		*outCount = 0;
	}
}


void
IGGroupAndCompressStrokes(VAddr, CStrokeUnit *, ULong, bool, Ptr *)
{}


void
UpdateStroke(CStrokeUnit * inStrokeUnit, FRect * inBox)
{
	FRect unitBounds;
	inStrokeUnit->getBBox(&unitBounds);
	int penSize = RINT(GetPreference(SYMA(userPenSize)));
	InsetRectangle(&unitBounds, -penSize, -penSize);
	if (!inStrokeUnit->getStroke()->testFlags(0x08000000)
	||  SectRectangle(&unitBounds, inBox, &unitBounds))
		inStrokeUnit->getStroke()->draw();
}

void
UpdateStroke(CRecUnit * inStrokeUnit)
{
	CUnit unit(inStrokeUnit, 0);
	unit.stroke()->inkOff(YES);
	gRootView->update();
}


void
UpdateStrokesInList(CRecUnitList * inList, FRect * inBox)
{
	ArrayIterator iter;
	CStrokeUnit * strok = (CStrokeUnit *)inList->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); strok = (CStrokeUnit *)iter.getNext(), ++i)
	{
		UpdateStroke(strok, inBox);
	}
}


#pragma mark -
/* -----------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Initialize word correction.
----------------------------------------------------------------------------- */
static void	InitCorrection(RefArg ioInfo);
static Ref	CorrectInfo(void);

void
InitCorrection(void)
{
	RefVar correction(CorrectInfo());
	InitCorrection(correction);
	SetFrameSlot(correction, SYMA(max), MAKEINT(10));
}


static void
InitCorrection(RefArg ioInfo)
{
	SetFrameSlot(ioInfo, SYMA(info), MakeArray(0));
}


static Ref
CorrectInfo(void)
{
	return GetFrameSlot(RA(gVarFrame), SYMA(correctInfo));
}


Ref
ReadDomainOptions(RefArg inArg)
{
//	FReadCursiveOptions(inArg);
	return NILREF;
}


ULong
GetElapsedTicks(ULong inFromTime)
{
	ULong now = GetTicks();
	if (now > inFromTime)
		return now - inFromTime;
	return 255 - inFromTime - 256 + now + 1;
}


void
SetUpArea(CRecArea * inArea, RefArg inRecConfig)
{
	for (ArrayIndex i = 0, count = gRecognitionManager.numOfRecognizers(); i < count; ++i)
	{
		CRecognizer * rec = gRecognitionManager.getRecognizer(i);
		rec->enableArea(inArea, inRecConfig);
	}
}


void
ConfigureArea(CRecArea * inArea, RefArg inRecConfig)
{
	for (ArrayIndex i = 0, count = gRecognitionManager.numOfRecognizers(); i < count; ++i)
	{
		CRecognizer * rec = gRecognitionManager.getRecognizer(i);
		rec->configureArea(inArea, inRecConfig);
	}
}


CRecArea *
MakeArea(CController * inController, CView * inView, ULong inMask, RefArg inRecConfig)
{
	RefVar recConfig(inRecConfig);
	CRecArea * area = CRecArea::make(0, 0);
	if (area != NULL)
	{
		if (inView)
			recConfig = BuildRecConfig(inView, inMask);
		else
			recConfig = BuildRCProto(NULL, inRecConfig);
		SetUpArea(area, recConfig);
		inController->buildGTypes(area);
		ConfigureArea(area, recConfig);
	}
	return area;
}


CRecArea *
MakeArea(CController * inController, CView * inView, ULong inMask)
{
	return MakeArea(inController, inView, inMask, RA(NILREF));
}


#pragma mark -
/* -----------------------------------------------------------------------------
	Area cache.
----------------------------------------------------------------------------- */
ULong		HandleUnit(CArray * inRecUnits);
ULong		HandleReplayUnit(CArray * inRecUnits);

struct AreaCacheEntry
{
	CRecArea *	area;
	ULong			x04;
	ULong			x08;
};


CDArray *
InitAreas(void)
{
	CDArray *	areas = CDArray::make(sizeof(AreaCacheEntry), 0);
	gAreaCache = areas;
	return areas;
}


void
PurgeAreaCache(void)
{
	if (gAreaCache != NULL)
	{
		for (ArrayIndex i = 0; i < gAreaCache->count(); ++i)
		{
			AreaCacheEntry * entry = (AreaCacheEntry *)gAreaCache->getEntry(i);
			entry->area->release();
		}
		gAreaCache->clear();
		gAreaCache->compact();
	}
}


bool
OtherViewInUse(CView * inView)
{
	ULong viewId = inView ? inView->fId : 0;
	for (ArrayIndex i = 0; i < gAreaCache->count(); ++i)
	{
		AreaCacheEntry * entry = (AreaCacheEntry *)gAreaCache->getEntry(i);
		CRecArea * area = entry->area;
		if (area->viewId() != viewId  &&  area->isRetained())
			return YES;
	}
	return NO;
}


CRecArea *
FindMatchingArea(CView * inView, ULong inMask)
{
	CRecArea * matchingArea = NULL;

	for (ArrayIndex i = 0; i < gAreaCache->count(); ++i)
	{
		AreaCacheEntry * entry = (AreaCacheEntry *)gAreaCache->getEntry(i);
		if (entry->x04 == inMask && entry->area->viewId() == inView->fId)
		{
			entry->x08 = GetTicks();
			matchingArea = entry->area;
			break;
		}
	}

	for (ArrayIndex i = 0; i < gAreaCache->count(); ++i)
	{
		AreaCacheEntry * entry = (AreaCacheEntry *)gAreaCache->getEntry(i);
		if (GetElapsedTicks(entry->x08) > 10*kSeconds)	// thatÕs in ms but you get the idea
		{
			entry->area->release();
			gAreaCache->deleteEntry(i);
			i--;
		}
	}

	if (matchingArea == NULL)
	{
		AreaCacheEntry entry;
		gRecognitionManager.setUnitHandler(gArbiter->getf21() ? HandleReplayUnit : HandleUnit );
		matchingArea = MakeArea(gController, inView, inMask);
		if (matchingArea == NULL)
			return NULL;
		entry.area = matchingArea;
		entry.x04 = inMask;
		entry.x08 = GetTicks();
		*(AreaCacheEntry *)gAreaCache->addEntry() = entry;
	}
	matchingArea->setViewId(inView->fId);
	return matchingArea;
}


bool
TryGetAreasHit(CRecUnit * inUnit, CArray * inAreas)
{
	bool isNewArea = NO;

	CUnit unit(inUnit, 0);
	CView * view = unit.findView(unit.requiredMask());
	ULong mask = unit.inputMask();
	if (mask == 0)
	{
		if (unit.getType() == 'CLIK')
		{
			if (gRootView->getPopup() != NULL)
				gRootView->setPopup(NULL, YES);
			gInhibitPopup = YES;
		}
		unit.cleanUp();
	}

	else
	{
		CAreaList * areas = (CAreaList *)inAreas;
		if (!areas->findMatchingView(view->fId))
		{
			// no view to hit; look for an area
			if (areas->count() > 0)
			{
				areas = CAreaList::make();
				isNewArea = YES;
			}
			CRecArea * area = FindMatchingArea(view, mask);
			if (area)
				areas->addArea(area);
			if (isNewArea)
			{
				inUnit->setAreas(areas);
				areas->release();
			}
		}
	}

	return isNewArea;
}


bool
GetAreasHit(CRecUnit * inUnit, CArray * inAreas)
{
	bool isNewArea = NO;
	newton_try
	{
		isNewArea = TryGetAreasHit(inUnit, inAreas);
	}
	newton_catch(exRootException)
	{
		SafeExceptionNotify(CurrentException());
	}
	end_try;
	return isNewArea;
}


void
HandleExpiredStroke(CRecUnit * inUnit)
{
	newton_try
	{
		gRecognitionManager.setx1C(1);
		if (!gArbiter->getf21())
			gStrokeWorld.addExpiredStroke((CStrokeUnit *)inUnit);
		else
			UpdateStroke(inUnit);
	}
	newton_catch(exRootException)
	{
		SafeExceptionNotify(CurrentException());
	}
	end_try;
}


CRecUnitList *
HandleGetContextUnits(CRecUnit * inUnit, long inArg2)
{
	CRecUnitList * contextUnits = NULL;

	newton_try
	{
		CUnit unit(inUnit, 0);
		CView * view = unit.findView(vAnythingAllowed);
		if (view)
		{
			RefVar cmd(MakeCommand(aeGetContext, view, (long)&unit));
			CommandSetIndexParameter(cmd, 0, inArg2);
			contextUnits = (CRecUnitList *)gApplication->dispatchCommand(cmd);
		}
	}
	newton_catch(exRootException)
	{ }
	end_try;

	return contextUnits;
}


void
SetContextUnitRoutine(GetContextUnitsProcPtr inHandler)
{
	gContextUnitProc = inHandler;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	S t r o k e   W o r l d
----------------------------------------------------------------------------- */

void
IdleStrokes(void)
{
	if (gStrokeWorld.f38)
		StrokeTime();	// does nothing
	else
	{
		gStrokeWorld.f38 = YES;
		gStrokeWorld.idleStrokes();
		gStrokeWorld.f38 = NO;
	}
}


NewtonErr
PostAndDoCommand(ULong inCmd, CUnit * inUnit, ULong inFlags)
{
	NewtonErr err = noErr;	// r7
	CView * target = inUnit->findView(inFlags);	// r6
	if (target)
	{
		bool isPopupDismissed = NO;
		if (inCmd == aeClick)
		{
			CView * popup = gRootView->getPopup();	// r2
			if (popup != NULL && popup != target)
			{
				// we have a popup but itÕs not the target of the command
				CView * parent;
				for (parent = target->fParent; parent != popup && parent != gRootView; parent = parent->fParent)
					;
				if (parent == gRootView)
				{
					// user tapped ouside the popup, so dismiss it
					gRootView->setPopup(NULL, YES);
					isPopupDismissed = YES;
					gRecognitionManager.saveClickView(NULL);
				}
			}
		}
		if (isPopupDismissed)
			err = 1;
		else
		{
			RefVar cmd(MakeCommand(inCmd, target, (long)inUnit));
			if (inCmd == aeInk)
				CommandSetFrameParameter(cmd, inUnit->strokes());
			if (inCmd == aeInk || inCmd == aeInkText)
			{
				RefVar strokesRef(inUnit->strokes());
				SetFrameSlot(strokesRef, SYMA(startTime), MAKEINT(inUnit->startTime()));
				SetFrameSlot(strokesRef, SYMA(endTime), MAKEINT(inUnit->endTime()));
			}
			err = gApplication->dispatchCommand(cmd);
		}
	}
	gInhibitPopup = NO;
	return err;
}


ULong
HandleUnitList(CArray * inRecUnits)
{
	ULong result = 0;

	gRecognitionManager.setx38(NO);

// sp24 = gRootView
// sp20 = &sp-A8
// sp1C = exRootException
// sp18 = &sp-50
// sp14 = gApplication
// sp10 = gStrokeWorld
// sp0C = gController

	for (ArrayIndex i = 0; i < inRecUnits->count(); ++i)
	{
		CRecUnit * unit = ((ArbStuff *)inRecUnits->getEntry(i))->x00;	// r6
		gRecognitionManager.setx3C(unit);
//sp-04
		RecType unitType = unit->getType();			// r9
		CRecognizer * recognizer = gRecognitionManager.findRecognizer(unitType);	// r8
		ULong cmd = recognizer->command();			// r4
		ULong startTime = unit->getStartTime();	// sp00
		gRecognitionManager.setNextClick(startTime);
		if (recognizer->testFlags(2) && gRecognitionManager.x1CValue())
		{
			CRecStroke * strok = unit->getStroke(0);
			if (strok->startTime() < (strok->f40Time() + 30)
			&&  ClicksOnlyArea(unit))
				gRecognitionManager.setx38(YES);
		}
//sp-40
		CUnit pubUnit(unit, 0);	// sp04
		ULong recFlags = pubUnit.requiredMask();
		NewtonErr err = noErr;	// r10
		Rect pubUnitBounds;		// sp48
		pubUnit.bounds(&pubUnitBounds);
		if (!gRecognitionManager.modalRecognitionOK(&pubUnitBounds)
		||  (cmd == aeClick && gRootView->doCaretClick(&pubUnit)))
		{
			cmd = 0;
			gRecognitionManager.setx38(YES);
		}
		if (gStrokeWorld.beforeLastFlush(startTime))
			cmd = 0;
		if (cmd != 0 && (cmd = recognizer->handleUnit(&pubUnit)) != 0)
		{
			newton_try
			{
				err = PostAndDoCommand(cmd, &pubUnit, recFlags);
			}
			newton_catch(exRootException)
			{
				SafeExceptionNotify(CurrentException());
			}
			end_try;
//			gApplication->fNewUndo = YES;
		}
		if (gRecognitionManager.getx3C() == unit)
		{
			bool r8 = (cmd == aeClick) && (gStrokeWorld.currentStroke() == NULL);
			if ((cmd == aeClick) && !r8 && ClicksOnlyArea(unit))
				gRecognitionManager.setx38(YES);

			if (err || cmd == aeTap || gRecognitionManager.getx38())
			{
				if (cmd)
				{
					CRecognizer * rec = gRecognitionManager.findRecognizer(unitType);
					gRecognitionManager.setx1C(rec->testFlags(1));
				}
				if (!r8)
				{
					pubUnit.cleanUp();
					pubUnit.invalidate();
					gController->markUnits(unit, kClaimedUnit);
				}
				result = 1;
				if (gRecognitionManager.getx38())
				{
					gRecognitionManager.saveClickView(NULL);
					gController->triggerRecognition();
				}
			}
		}
		gRecognitionManager.setx3C(NULL);
	}

	gInhibitPopup = NO;
	return result;
}


ULong
HandleReplayUnit(CArray * inRecUnits)
{
	if (inRecUnits->count() != 1)
	{
#if defined(correct)
		StdIOOn();
		WriteTapStats(inRecUnits);
		StdIOOff();
#endif
		gArbiter->setf20(YES);
	}
	return 1;
}


ULong
HandleUnit(CArray * inRecUnits)
{
	ULong result = 0;
	newton_try
	{
		result = HandleUnitList(inRecUnits);
	}
	newton_catch(exRootException)
	{
		SafeExceptionNotify(CurrentException());
	}
	end_try;
	return result;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C R e c o g n i t i o n M a n a g e r
----------------------------------------------------------------------------- */

NewtonErr
CRecognitionManager::init(UChar inCapability)
{
	fStrokeCentral = NULL;
	fController = NULL;
	fArbiter = NULL;
	x10 = NULL;
	x18 = 0;
	x1C = 1;
	fCapability = inCapability;
	fModalBounds = NULL;
	fRecognizers = CRecognizerList::make();
	if (fCapability >= 1)
	{
		gStrokeWorld.init();
		fStrokeCentral = &gStrokeWorld;
#if defined(hasParaGraphRecognizer)
		InitializeParagraphCompression();
#endif
		x10 = InitAreas();
		gController = fController = CController::make();
		gArbiter = fArbiter = CArbiter::make(fController);
		fController->setHitTestRoutine(GetAreasHit);
		fController->setExpireStrokeRoutine(HandleExpiredStroke);
		SetContextUnitRoutine(HandleGetContextUnits);
	}
	if (fCapability >= 2)
	{
		InitDictionaries();
	}
	if (fCapability >= 1)
	{
		initRecognizers();
		fController->initialize();
	}
	return noErr;
}


NewtonErr
CRecognitionManager::initRecognizers(void)
{
	if (fCapability >= 1)
	{
		InstallGestureRecognizer(this);
		InstallEventRecognizer(this);
		InstallStrokeRecognizer(this);
		InstallClickRecognizer(this);
		gRootDomain = CRecDomain::make(fController, 'ROOT', "CRecDomain");
	}
	if (fCapability >= 2)
	{
		InstallShapeRecognizer(this);
#if defined(hasParaGraphRecognizer)
		InstallWordRecognizer(this);
#endif
		InstallWRecRecognizer(this);
	}
	ReadDomainOptions(RA(NILREF));
	return noErr;
}


RecognitionState *
CRecognitionManager::saveRecognitionState(bool * outFail)
{
	*outFail = NO;
	RecognitionState * theState = new RecognitionState;
	if (theState == NULL)
		*outFail = YES;
	else
	{
#if 0
		theState->x00 = x3C;
		theState->x04 = x18;
		theState->x08 = x1C;
		theState->x09 = x38;
		bool	sp04;
		bool	sp00;
		theState->x0C = gStrokeWorld.saveRecognitionState(&sp04);
		theState->x10 = SaveRecognitionState(gController, &sp00);
		if (sp04 || sp00)
			*outFail = YES;
#endif
		x18 = 0;
		x1C = 0;
		x38 = 0;
		fPrevClickView = NULL;
		fClickView = NULL;
	}
	PurgeAreaCache();
	return theState;
}


void
CRecognitionManager::restoreRecognitionState(RecognitionState * inState)
{
	if (inState)
	{
#if 0
		x3C = inState->x00;
		x18 = inState->x04;
		x1C = inState->x08;
		x38 = inState->x09;
		gStrokeWorld.restoreRecognitionState(inState->x0C);
		RestoreRecognitionState(gController, inState->x10);
#endif
		fPrevClickView = NULL;
		fClickView = NULL;
		delete inState;
	}
	PurgeAreaCache();
}


Ref
FModalRecognitionOn(RefArg rcvr, RefArg inRect)
{
	Rect rect;
	if (!FromObject(inRect, &rect))
		ThrowMsg("bad modal rect");
	gRecognitionManager.enableModalRecognition(&rect);
}


void
CRecognitionManager::enableModalRecognition(Rect * inRect)
{
	if (fModalBounds)
		ThrowMsg("Can't nest modal bounds.");

	fModalBounds = new Rect;
	if (fModalBounds)
		*fModalBounds = *inRect;
}


bool
CRecognitionManager::modalRecognitionOK(Rect * inRect)
{
	if (fModalBounds == NULL)
		return YES;

	Point	center = MidPoint(inRect);
	bool	isInBounds = PtInRect(center, fModalBounds);
	if (!isInBounds)
	{
		if (gRootView->getPopup() != NULL)
			gRootView->setPopup(NULL, YES);
	}
	return isInBounds;
}


Ref
FModalRecognitionOff(RefArg rcvr)
{
	gRecognitionManager.disableModalRecognition();
}


void
CRecognitionManager::disableModalRecognition(void)
{
	if (fModalBounds)
		delete fModalBounds, fModalBounds = NULL;
}


NewtonErr
CRecognitionManager::idle(void)
{
	if (fCapability >= 1)
	{
		IdleStrokes();
		fStrokeCentral->idleCompress();
		fController->idle();
	}
	return noErr;
}


CTime
CRecognitionManager::nextIdle(void)
{
	CTime		theTime(0);	// => we donÕt need idle time
	Timeout	when = kDistantFuture;	// in ms
	if (fCapability >= 1)
	{
		theTime = fStrokeCentral->f2C;		// stroke expiry time
		when = gController->nextIdleTime();	// in ms
		if (when == kDistantFuture)
			return theTime;
	}
	CTime	wuTime(TimeFromNow(when*kMilliseconds));
	if (theTime == CTime(0) || wuTime < theTime)
		return wuTime;
	return theTime;
}


void
CRecognitionManager::ignoreClicks(Timeout inDuration)
{
	x18 = GetTicks() + inDuration;
}


void
CRecognitionManager::setNextClick(ULong inTime)
{
	if (x18 < inTime || (x18 - inTime) < 60)
		x18 = 0;
}


void
CRecognitionManager::saveClickView(CView * inView)
{
	fPrevClickView = fClickView;
	fClickView = inView;
}


void
CRecognitionManager::removeClickView(CView * inView)
{
	if (fPrevClickView == inView)
		fPrevClickView = NULL;
	if (fClickView == inView)
		fClickView = NULL;
}


NewtonErr
CRecognitionManager::update(Rect * inRect)
{
	FRect	updateBounds;
	FixRect(&updateBounds, inRect);
	if (fCapability >= 1)
	{
		int penSize = RINT(GetPreference(SYMA(userPenSize)));
		SetLineWidth(penSize);	// was PenSize(penSize, penSize);
		FRect	invalBounds = updateBounds;
		fController->updateInk(&invalBounds);
		if (!EmptyRectangle(&invalBounds))
		{
			Rect	inkRect;
			UnfixRect(&invalBounds, &inkRect);
			AdjustForInk(&inkRect);
			gRootView->smartInvalidate(&inkRect);
		}
		fStrokeCentral->updateCompressGroup(&updateBounds);
	}
	return noErr;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C S t r o k e C e n t r a l
----------------------------------------------------------------------------- */

CStrokeCentral::CStrokeCentral()
{
//	initFields();		// gStrokeCentral instance is created at startup before environment is ready
}


CStrokeCentral::~CStrokeCentral()
{
	doneFields();
}


void
CStrokeCentral::init(void)
{
	initFields();
//	SetUpPB();		this really only returns YES
//	StrokeInit();  this really does nothing
//	TabOn();			activate tablet -- legacy?
}


void
CStrokeCentral::initFields(void)
{
	fIsValidStroke = NO;
	fCurrentStroke = NULL;
	fCurrentUnit = NULL;
	f0C = 0;
	f10 = 0;
	fBlockCount = 0;
	fUnblockCount = 0;
	f38 = 0;
	f24 = 0;
	f28 = CRecUnitList::make();
	f2C = CTime(0);
	f34 = NULL;
	f20 = new RefStruct;		// canÕt make these members because of gStrokeCentral instance
	*f20 = MakeArray(0);
	f3C = NULL;
	f40 = new RefStruct;
}


void
CStrokeCentral::doneFields(void)
{
	if (f28)
		f28->release(), f28 = nil;
	if (f20)
		delete f20, f20 = NULL;
	if (f40)
		delete f40, f40 = NULL;
}


void
CStrokeCentral::startNewStroke(CRecStroke * ioStroke)
{
	fIsValidStroke = YES;
	fCurrentStroke = ioStroke;
	ioStroke->f3C = f0C;
	ioStroke->f40 = f10;
	f0C = ioStroke->fStartTime;
}


CRecStroke *
CStrokeCentral::currentStroke(void)
{
	if (fIsValidStroke)
		return fCurrentStroke;
	return NULL;
}


void
CStrokeCentral::addDeferredStroke(RefArg inArg1, long inArg2, long inArg3)
{
	AddArraySlot(*f20, inArg1);
	AddArraySlot(*f20, MAKEINT(inArg2));
	AddArraySlot(*f20, MAKEINT(inArg3));
}


void
CStrokeCentral::invalidateCurrentStroke(void)
{
	fIsValidStroke = NO;
}


void
CStrokeCentral::doneCurrentStroke(void)
{
	f0C = fCurrentStroke->fStartTime;
	f10 = fCurrentStroke->fEndTime;
	fIsValidStroke = NO;
	fCurrentStroke = NULL;

	fCurrentUnit->unsetFlags(kBusyUnit);
	fCurrentUnit = NULL;
	f2C = TimeFromNow(500*kMilliseconds);
}


void
CStrokeCentral::blockStrokes(void)
{
	fBlockCount++;
}


void
CStrokeCentral::unblockStrokes(void)
{
	if (fBlockCount > 0)
		fBlockCount--;
}


bool
CStrokeCentral::flushStrokes(void)
{
	bool isAnythingFlushed = NO;
	CRecStroke * strok;
	while ((strok = StrokeGet()) != NULL)
	{
		CClickUnit * clikUnit;
		XFAIL((clikUnit = CClickUnit::make(gRootDomain, 1, strok, NULL)) == NULL)
		clikUnit->setFlags(0x04000000);
		isAnythingFlushed = YES;

		CUnit unit(clikUnit, 0);
		unit.stroke()->inkOff(YES);
		unit.invalidate();

		if (!strok->isDone())
		{
			StrokeTime();
			Wait(20);	// milliseconds - was 1 tick
		}
		if (strok->isDone())
			clikUnit->release();
	}
	return isAnythingFlushed;
}


bool
CStrokeCentral::beforeLastFlush(long inArg1)
{
	if (f1C == 0)
		return NO;

	long  delay = f1C - inArg1;
	if (delay < 0)
		delay = -delay;
	if (delay > 600)	//10*seconds?
	{
		f1C = 0;
		return NO;
	}
	return f1C > inArg1;
}


void
CStrokeCentral::addExpiredStroke(CStrokeUnit * inStroke)
{
	inStroke->retain();
	IGGroupAndCompressStrokes((VAddr)this, inStroke, gRecognitionLetterSpacing, NO, &f34);
	f2C = TimeFromNow(500*kMilliseconds);
}


void
CStrokeCentral::IGCompressGroup(CStrokeUnit ** inStrokeUnits)
{
	ArrayIndex i;
	for (i = 0; inStrokeUnits[i] != NULL; ++i)
	{
		f28->addUnit(inStrokeUnits[i]);
	}
	f24 = i;
	compressGroup();
}


Ref
MakeStrokeRef(CRecStroke * inStroke)
{
	ArrayIndex count = inStroke->count();
	RefVar strokeRef(AllocateBinary(SYMA(stroke), count*sizeof(Point)));
	CDataPtr strokeData(strokeRef);
	Point * strokePt = (Point *)(char *)strokeData;
	FPoint pt;
	SamplePt * p = inStroke->getPoint(0);
	for (ArrayIndex i = 0; i < count; ++i, ++p, ++strokePt)
	{
		p->getPoint(&pt);
		strokePt->h = pt.x;
		strokePt->v = pt.y;
	}
	return strokeRef;
}


// Make stroke bundle for the NewtonScript world
Ref
StrokeBundle(CUnit ** inGroup, Rect * outBounds)
{
	ArrayIndex i;
	for (i = 0; inGroup[i] != NULL; ++i)
		;	// NULL entry marks end of group

	RefVar bundleRef(Clone(RA(strokeBundle)));
	RefVar strokes(MakeArray(i));
	outBounds->top = outBounds->bottom = -32768;

	CUnit * strokeUnit;
	Rect strokeBounds;
	for (i = 0; (strokeUnit = inGroup[i]) != NULL; ++i)
	{
		strokeUnit->bounds(&strokeBounds);
		UnionRect(outBounds, &strokeBounds, outBounds);
		SetArraySlot(strokes, i, MakeStrokeRef(strokeUnit->siUnit()->getStroke(0)));
	}
	InsetRect(outBounds, -2, -2);
	SetFrameSlot(bundleRef, SYMA(strokes), strokes);
	SetFrameSlot(bundleRef, SYMA(bounds), ToObject(outBounds));
	SetFrameSlot(bundleRef, SYMA(startTime), MAKEINT(inGroup[0]->siUnit()->getStartTime()));
	SetFrameSlot(bundleRef, SYMA(endTime), MAKEINT(inGroup[i-1]->siUnit()->getEndTime()));

	return bundleRef;
}


void
ExpireUsingCommand(CUnit ** inGroup)
{
	CView * theView = (*inGroup)->findView(vAnythingAllowed);
	if (theView)
	{
		RefVar recConfig(BuildRecConfig(theView, theView->viewFlags & vRecognitionAllowed));
		ULong cmdId = NOTNIL(GetVariable(recConfig, SYMA(doInkWordRecognition))) ? aeInkText : aeInk;
		RefVar cmd(MakeCommand(cmdId, theView, 0));
		Rect bounds;
		RefVar arg(StrokeBundle(inGroup, &bounds));
		CommandSetFrameParameter(cmd, arg);
		gApplication->dispatchCommand(cmd);

		AdjustForInk(&bounds);
		gRootView->smartInvalidate(&bounds);

		if (gRecMemErrCount > 0  &&  (gRecInkNotifyFlags & 0x01) != 0)
		{
			Ref lastWarningRef = GetPreference(SYMA(lastRecMemWarning));
			ULong lastWarning = ISINT(lastWarningRef) ? RVALUE(lastWarningRef) : 0;
			ULong today = RealClock() / (60 * 24);		// minutes -> days
			if (today > lastWarning)
			{
				NSCallGlobalFn(SYMA(RecognitionMemoryWarning), MAKEBOOLEAN((gRecInkNotifyFlags & 0x02) == 0));
				SetPreference(SYMA(lastRecMemWarning), MAKEINT(today));
			}
			gRecMemErrCount = 0;
		}
	}
}


void
CStrokeCentral::expireGroup(CUnit ** inGroup)
{
	if (!gArbiter->getf21())
	{
		newton_try
		{
			if (f3C == NULL)
				ExpireUsingCommand(inGroup);
			else
			{
				Rect bounds;
				RefVar sp00(StrokeBundle(inGroup, &bounds));
//				f3C(f40, sp00);
			}
		}
		newton_catch(exRootException)
		{
			ExceptionNotify(CurrentException());
		}
		end_try;
	}

	if (f3C == NULL)
	{
//		gApplication->fNewUndo = YES;
		gRootView->update();
	}
	gRecognitionManager.x1C = YES;
}


void
CStrokeCentral::compressGroup(void)
{
	ArrayIndex i;
	CUnit ** group;	// r7
	if ((group = new CUnit*[f24 + 1]) == NULL)
		OutOfMemory();
	for (i = 0; i < f24; ++i)
		group[i] = NULL;

	unwind_protect
	{
		CUnit * unit;
		for (i = 0; i < f24; ++i)
		{
			XFAILNOT(unit = new CUnit(f28->getUnit(i), 0), OutOfMemory();)
			unit->stroke()->inkOff(NO);
			group[i] = unit;
		}
		group[i] = NULL;
		if (f24 > 0)
			expireGroup(group);
	}
	on_unwind
	{ }
	end_unwind;
}


void
CStrokeCentral::updateCompressGroup(FRect * inBox)
{
	if (f28->count() > 0)
		UpdateStrokesInList(f28, inBox);

	if (f34 != NULL)
	{
		ArrayIndex count;
		CStrokeUnit ** sp04;
		IGGetStrokesQueue(f34, &sp04, &count);
		for (ArrayIndex i = 0; i < count; ++i)
		{
			CStrokeUnit * strokUnit = sp04[i];
			if (strokUnit != NULL)
				UpdateStroke(strokUnit, inBox);
		}
	}
}


void
CStrokeCentral::idleCompress(void)
{
	if (gStrokeWorld.currentStroke() == NULL)
	{
		if (f2C != CTime(0)  &&  f2C < GetGlobalTime())
			expireAll();
	}
}


void
CStrokeCentral::idleStrokes(void)
{
	if (fBlockCount > 0)
	{
		if (++fUnblockCount > 10)
		{
			fUnblockCount = 0;
			fBlockCount = 0;
		}
	}

	for ( ; ; )
	{
		StrokeTime();	// does nothing

		XTRY
		{
			XFAIL(gController->checkBusy())
			XFAIL(fCurrentStroke != NULL)
			if (fBlockCount != 0)
				return;

			CRecStroke * strok = StrokeGet();
			XFAIL(strok == NULL)
			startNewStroke(strok);

			// start a new unit -- everything starts with a click
			fCurrentUnit = CClickUnit::make(gRootDomain, 1, strok, NULL);
			XFAIL(fCurrentUnit == NULL)
			fCurrentUnit->setFlags(0x04000000);
			gController->newClassification(fCurrentUnit);

			if (gController->isExternallyArbitrated(fCurrentUnit))
			{
				CArray * unitList = CArray::make(sizeof(ArbStuff), 1);
				XFAIL(unitList == NULL)
				ArbStuff * p = (ArbStuff *)unitList->getEntry(0);
				p->x00 = fCurrentUnit;
				HandleUnit(unitList);
				unitList->release();
			}
		}
		XENDTRY;

		if (fCurrentStroke == NULL)
			return;

		idleCurrentStroke();

		if (!fCurrentUnit->testFlags(kClaimedUnit))
		{
			CClickEventUnit * clikUnit;	// r6
			ULong evt = fCurrentStroke->getTapEvent();
			switch (evt)
			{
			case kTapClick:
			case kDoubleTapClick:
			case kHiliteClick:
			case kTapDragClick:
				if ((clikUnit = CClickEventUnit::make(gRootDomain, 1, NULL)))	// intentional assignment
				{
					clikUnit->addSub(fCurrentUnit);
					bool isAcquired = AcquireStroke(fCurrentStroke);
					if (fCurrentStroke->getTapEvent() == evt)
						clikUnit->clearEvent();
					if (isAcquired)
						ReleaseStroke();
				}
				break;
			}
		}

		if (!fCurrentStroke->isDone())
			return;
fCurrentUnit->dumpObject((char *)"Finished stroke--\n");
		if (gJournallingState == 1 /*kJournalRecording*/)
			JournalRecordAStroke(fCurrentStroke);

		doneCurrentStroke();
		gController->triggerRecognition();
	}
}


void
CStrokeCentral::idleCurrentStroke(void)
{
	fCurrentUnit->setBBox(&fCurrentStroke->fBBox);
}


void
CStrokeCentral::expireAll(void)
{
	if (f34)
		IGGroupAndCompressStrokes((VAddr)this, NULL, gRecognitionLetterSpacing, YES, &f34);
	if (f28->isEmpty())
		f2C = CTime(0);
}


OpaqueRef
CStrokeCentral::saveRecognitionState(bool * outErr)
{
	*outErr = NO;
	RecognitionState *  recState = new RecognitionState;
	if (recState != NULL)
	{
		recState->f00 = fIsValidStroke;
		recState->f04 = fCurrentStroke;
		recState->f08 = fCurrentUnit;
		recState->f0C = f0C;
		recState->f10 = f10;
		recState->f14 = f20;
		recState->f18 = f24;
		recState->f1C = f28;
		recState->f20 = f2C;
		recState->f28 = f34;
		recState->f2C = f38;
		recState->f30 = f3C;
		recState->f34 = f40;
		initFields();
	}
	else
		*outErr = YES;
	return (OpaqueRef) recState;
}


void
CStrokeCentral::restoreRecognitionState(OpaqueRef inState)
{
	if (inState != 0)
	{
		RecognitionState *  recState = (RecognitionState *)inState;
		if (fCurrentUnit)
			fCurrentUnit->unsetFlags(kBusyUnit);
		doneFields();
		fIsValidStroke = recState->f00;
		fCurrentStroke = recState->f04;
		fCurrentUnit = recState->f08;
		f0C = recState->f0C;
		f10 = recState->f10;
		f20 = recState->f14;
		f24 = recState->f18;
		f28 = recState->f1C;
		f2C = recState->f20;
		f34 = recState->f28;
		f38 = recState->f2C;
		f3C = recState->f30;
		f40 = recState->f34;
		delete recState;
	}
}


#pragma mark Recognition
/*--------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
--------------------------------------------------------------------------------*/
extern ULong CountCustomDictionaries(CView * inView);

Ref	BuildRecConfig(CView * inView, ULong inFlags);

CView *
GetRecognitionView(CView * inView)
{
	if (!(inView->derivedFrom(clEditView) || inView->derivedFrom(clParagraphView))
	 && (inView->fParent->derivedFrom(clEditView) || inView->fParent->derivedFrom(clParagraphView)))
		return inView->fParent;

	return inView;
}


ULong
TextOrInkWordsEnabled(CView * inView)
{
	ULong		enabled = 0;

	CView *	recView = GetRecognitionView(inView);
	ULong		recFlags = recView->viewFlags & vRecognitionAllowed;
	RefVar	recConfig(BuildRecConfig(recView, recFlags));

	if (NOTNIL(GetVariable(recConfig, SYMA(doInkWordRecognition))))
		enabled = 1;

	if (((recView->viewFlags & vAnythingAllowed) == vAnythingAllowed && NOTNIL(GetVariable(recConfig, SYMA(doTextRecognition))))
	 || (recFlags & vWordsAllowed) != 0)
		enabled |= 2;

	return enabled;
}


bool
InkTextEnabled(CView * inView, ULong inFlags, RefArg inContext)
{
	return inView != NULL
		 && (inFlags & vAnythingAllowed) != vAnythingAllowed
		 && NOTNIL(GetProtoVariable(inContext, SYMA(doInkWordRecognition)));
}


Ref
PrepRecConfig(CView * inView, RefArg inConfig)
{
	if (FrameHasSlot(inConfig, SYMA(_parent)))
		return inConfig;

	RefVar recConfig(Clone(RA(protoRecConfig)));
	SetFrameSlot(recConfig, SYMA(_proto), inConfig);

	RefVar userConfig(GetFrameSlot(gVarFrame, SYMA(userConfiguration)));

	RefVar settings;
	if (inView)
		settings = inView->getVar(SYMA(_recogSettings));
	if (NOTNIL(settings))
	{
		settings = NSCallGlobalFn(SYMA(ExpandSettings), settings);
		SetFrameSlot(settings, SYMA(_proto), userConfig);
		SetFrameSlot(recConfig, SYMA(_parent), settings);
	}
	else
		SetFrameSlot(recConfig, SYMA(_parent), userConfig);

	return recConfig;
}


ULong
BuildInputMask(RefArg inConfig, ULong inFlags, bool inAllowAnything)
{
	if (inAllowAnything || NOTNIL(GetProtoVariable(inConfig, SYMA(allowTextRecognition))))
	{
		if (NOTNIL(GetVariable(inConfig, SYMA(doTextRecognition))))
		{
			bool isCursive = NO;
			if (NOTNIL(GetVariable(inConfig, SYMA(wordsCursiveOption))))
			{
				isCursive = YES;
				inFlags |= 0x00001000;
			}
			if (NOTNIL(GetVariable(inConfig, SYMA(lettersCursiveOption))))
			{
				isCursive = YES;
				inFlags |= 0x00006000;
			}
			if (NOTNIL(GetVariable(inConfig, SYMA(numbersCursiveOption))))
			{
				isCursive = YES;
				inFlags |= 0x001C2000;
			}
			if (isCursive && NOTNIL(GetVariable(inConfig, SYMA(punctuationCursiveOption))))
				inFlags |= 0x00008000;
		}
	}

	if (inAllowAnything || NOTNIL(GetProtoVariable(inConfig, SYMA(allowShapeRecognition))))
	{
		if (NOTNIL(GetVariable(inConfig, SYMA(doShapeRecognition))))
			inFlags |= 0x00010000;
	}

	return inFlags;
}


Ref
BuildRCProto(CView * inView, RefArg inConfig)
{
	RefVar recConfig(PrepRecConfig(inView, inConfig));
	RefVar inputMask(GetProtoVariable(recConfig, SYMA(inputMask)));
	if (ISNIL(inputMask) || NOTNIL(GetProtoVariable(recConfig, SYMA(buildInputMask))))
	{
		ULong maskBits = 0x0A00;
		RefVar baseInputMask(GetProtoVariable(recConfig, SYMA(baseInputMask)));
		if (NOTNIL(baseInputMask))
			maskBits = RINT(baseInputMask);
		inputMask = MAKEINT(BuildInputMask(recConfig, maskBits, NO));
	}
	SetFrameSlot(recConfig, SYMA(inputMask), inputMask);
	return recConfig;
}


Ref
BuildRCView(CView * inView, ULong inFlags)
{
	bool isAnythingAllowed = (inFlags & vAnythingAllowed) == vAnythingAllowed;
	RefVar recConfig(PrepRecConfig(inView, isAnythingAllowed ? RA(rcPrefsConfig) : RA(rcNoRecog)));
	if (isAnythingAllowed)
		inFlags = BuildInputMask(recConfig, 0x0A00, YES);
	SetFrameSlot(recConfig, SYMA(inputMask), MAKEINT(inFlags));

	RefVar dictionaries;
	if (inView && CountCustomDictionaries(inView) > 0)
		dictionaries = inView->getVar(SYMA(dictionaries));
	if (NOTNIL(dictionaries))
		SetFrameSlot(recConfig, SYMA(dictionaries), dictionaries);

	return recConfig;
}


Ref
BuildInkOrTextConfig(RefArg inConfig, CView * inView, ULong inFlags)
{
	RefVar recConfig(inConfig);

	if (InkTextEnabled(inView, inFlags, recConfig))
	{
		recConfig = PrepRecConfig(inView, recConfig);
		if (NOTNIL(GetVariable(recConfig, SYMA(allowTextRecognition)))
		&&  NOTNIL(GetVariable(recConfig, SYMA(doTextRecognition))))
		{
			SetFrameSlot(recConfig, SYMA(inputMask), MAKEINT(inFlags));

			RefVar dictionaries;
			if (CountCustomDictionaries(inView) > 0)
				dictionaries = inView->getVar(SYMA(dictionaries));
			if (NOTNIL(dictionaries))
				SetFrameSlot(recConfig, SYMA(dictionaries), dictionaries);

			return recConfig;
		}
	}

	return BuildRCProto(inView, recConfig);
}


Ref
BuildRecConfig(CView * inView, ULong inFlags)
{
	RefVar config;

	if ((inFlags & vAnythingAllowed) == vAnythingAllowed)
	{
		RefVar userConfig(GetFrameSlot(gVarFrame, SYMA(userConfiguration)));
		config = GetFrameSlot(userConfig, SYMA(testConfig));
	}
	else if (inView)
	{
		config = GetProtoVariable(inView->fContext, SYMA(recConfig));
	}

	if (NOTNIL(config))
		config = BuildInkOrTextConfig(config, inView, inFlags);
	else
		config = BuildRCView(inView, inFlags);

	return config;
}


Ref
BuildRecConfigForDeferred(CView * inView, ULong inFlags)
{
	RefVar config;

	if (inFlags != vAnythingAllowed)
	{
		if (inView)
			config = GetProtoVariable(inView->fContext, SYMA(recConfig));
		if (ISNIL(config) || InkTextEnabled(inView, inFlags, config))
			config = BuildRCView(inView, inFlags);
		else
			config = PrepRecConfig(inView, config);
		SetFrameSlot(config, SYMA(inputMask), MAKEINT(inFlags & ~(vClickable | vStrokesAllowed | vGesturesAllowed | vShapesAllowed)));
		SetFrameSlot(config, SYMA(allowTextRecognition), RA(TRUEREF));
		SetFrameSlot(config, SYMA(doTextRecognition), RA(TRUEREF));
		SetFrameSlot(config, SYMA(doInkWordRecognition), RA(NILREF));
//		SetFrameSlot(config, SYMA(speedCursiveOption), MAKEINT(2));
//		SetFrameSlot(config, SYMA(letterSpaceCursiveOption), RA(NILREF));
	}

	return config;
}



#pragma mark -
/*--------------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
--------------------------------------------------------------------------------*/

extern "C"
{
Ref	FPrepRecConfig(RefArg inRcvr, RefArg inView);
Ref	FBuildRecConfig(RefArg inRcvr, RefArg inView);

Ref	FPurgeAreaCache(RefArg inRcvr);
}

Ref
FPrepRecConfig(RefArg inRcvr, RefArg inConfig)
{
	CView * view = FailGetView(inRcvr);
	return PrepRecConfig(view, inConfig);
}


Ref
FBuildRecConfig(RefArg inRcvr)
{
	CView * view = FailGetView(inRcvr);
	return BuildRecConfig(view, view->viewFlags & vRecognitionAllowed);
}


Ref
FPurgeAreaCache(RefArg inRcvr)
{
	PurgeAreaCache();
	return NILREF;
}

