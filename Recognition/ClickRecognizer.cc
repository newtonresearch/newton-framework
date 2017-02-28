/*
	File:		ClickRecognizer.cc

	Contains:	Class CClickEventUnit, a subclass of CSIUnit.

	Written by:	Newton Research Group, 2010.
*/

#include "ClickRecognizer.h"

extern void		UnbufferStroke(CRecStroke * inStroke);


bool
ClickInProgress(CRecUnit * inUnit)
{
	return inUnit->getType() == 'CLIK'
		 && inUnit->testFlags(kBusyUnit);
}


#pragma mark CClickUnit
/* -----------------------------------------------------------------------------
	C C l i c k U n i t
----------------------------------------------------------------------------- */

CClickUnit *
CClickUnit::make(CRecDomain * inDomain, ULong inType, CRecStroke * inStroke, CArray * inAreas)
{
	CClickUnit * clikUnit = new CClickUnit;
	XTRY
	{
		XFAIL(clikUnit == NULL)
		XFAILIF(clikUnit->iClickUnit(inDomain, inType, inStroke, inAreas) != noErr, clikUnit->release(); clikUnit = NULL;)
	}
	XENDTRY;
	return clikUnit;
}


NewtonErr
CClickUnit::iClickUnit(CRecDomain * inDomain, ULong inType, CRecStroke * inStroke, CArray * inAreas)
{
	NewtonErr err = iRecUnit(inDomain, 'CLIK', inType, inAreas);
	fClikStroke = inStroke;
	setBBox(inStroke->bounds());
	fStartTime = inStroke->startTime();
	fDuration = inStroke->endTime() - inStroke->startTime();	// original says inStroke->startTime() - inStroke->startTime() !
	return err;
}


void
CClickUnit::dealloc(void)
{
	fClikStroke->unsetFlags(0x10000000);
	UnbufferStroke(fClikStroke);
	fClikStroke->release();	// original calls virtual delete
	// super dealloc
	CRecUnit::dealloc();
}


#pragma mark Debug

void
CClickUnit::dump(CMsg * outMsg)
{
	outMsg->msgStr("Click: ");
	CRecUnit::dump(outMsg);
#if defined(correct)
	char str[256];
	str[0] = 0;
	outMsg->msgStr(str);		// sic
#endif
	outMsg->msgLF();
}


#pragma mark Interpretation

NewtonErr
CClickUnit::markUnit(CRecUnitList * inList, ULong inFlags)
{
	fClikStroke->unsetFlags(0x10000000);
	return CRecUnit::markUnit(inList, inFlags);
}


ULong
CClickUnit::countStrokes(void)
{
	return 1;
}


bool
CClickUnit::ownsStroke(void)
{
	return true;
}


CRecStroke *
CClickUnit::getStroke(ArrayIndex index)
{
	return fClikStroke;
}


#pragma mark -
#pragma mark CClickEventUnit
/* -----------------------------------------------------------------------------
	C C l i c k E v e n t U n i t
----------------------------------------------------------------------------- */

CClickEventUnit *
CClickEventUnit::make(CRecDomain * inDomain, ULong inArg2, CArray * inAreas)
{
	CClickEventUnit * evtUnit = new CClickEventUnit;
	XTRY
	{
		XFAIL(evtUnit == NULL)
		XFAILIF(evtUnit->iClickEventUnit(inDomain, inArg2, inAreas) != noErr, evtUnit->release(); evtUnit = NULL;)
	}
	XENDTRY;
	return evtUnit;
}


NewtonErr
CClickEventUnit::iClickEventUnit(CRecDomain * inDomain, ULong inArg2, CArray * inAreas)
{
	NewtonErr err;
	err = iSIUnit(inDomain, 'CEVT', inArg2, inAreas, 0);
	fEvent = -1;
	return err;
}


#pragma mark Debug

size_t
CClickEventUnit::sizeInBytes(void)
{
	size_t thisSize = (sizeof(CClickEventUnit) - sizeof(CSIUnit));	// original doesn’t need sizeof evaluation b/c it uses dynamic Ptr size
	return CSIUnit::sizeInBytes() + thisSize;
}


void
CClickEventUnit::dump(CMsg * outMsg)
{
	outMsg->msgStr("ClickEvent: ");
	CSIUnit::dump(outMsg);
	switch (event())
	{
	case kProcessedClick:
		outMsg->msgStr("kProcessedClick");
		break;
	case kTapClick:
		outMsg->msgStr("kTapClick");
		break;
	case kDoubleTapClick:
		outMsg->msgStr("kDoubleTapClick");
		break;
	case kHiliteClick:
		outMsg->msgStr("kHiliteClick");
		break;
	// kTapDragClick involves movement -- must be reported elsewhere
	default:
		outMsg->msgStr("Unknown event type");
		break;
	}
	outMsg->msgLF();
}


#pragma mark Event

int
CClickEventUnit::event(void)
{
	if (fEvent == -1)
	{
		CRecStroke * clikStrok = ((CClickUnit *)getSub(0))->getStroke(0);
		fEvent = clikStrok->getTapEvent();
	}
	return fEvent;
}


void
CClickEventUnit::clearEvent(void)
{
	fEvent = event();
	CRecStroke * clikStrok = ((CClickUnit *)getSub(0))->getStroke(0);
	clikStrok->setTapEvent(kProcessedClick);
}

