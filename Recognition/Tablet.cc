/*
	File:		Tablet.cc

	Contains:	Interface to tablet driver.

	Written by:	Newton Research Group, 2010.
*/

#include "Tablet.h"
#include "TabletBuffer.h"
#include "TabletDriver.h"
#include "Objects.h"
#include "SharedTypes.h"
#include "TaskGlobals.h"
#include "UserGlobals.h"
#include "VirtualMemory.h"

extern "C" void	EnterAtomic(void);
extern "C" void	ExitAtomic(void);


/* -----------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
----------------------------------------------------------------------------- */

extern "C" {
Ref	FStartBypassTablet(RefArg inRcvr);
Ref	FStopBypassTablet(RefArg inRcvr);
}


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

CTabletDriver *	gTablet;				// 0C104D34


#pragma mark -
#pragma mark Tablet
/* -----------------------------------------------------------------------------
	T a b l e t
----------------------------------------------------------------------------- */

NewtonErr
SetTabletCalibrationDataSWI(void)
{
	if (IsSuperMode())
	{
		TaskGlobals *	taskGlobals = TaskSwitchedGlobals();
		gGlobalsThatLiveAcrossReboot.fTabletValid = taskGlobals->fParams[0];
		gGlobalsThatLiveAcrossReboot.fTabletXScale = taskGlobals->fParams[1];
		gGlobalsThatLiveAcrossReboot.fTabletXOffset = taskGlobals->fParams[2];
		gGlobalsThatLiveAcrossReboot.fTabletYScale = taskGlobals->fParams[3];
		gGlobalsThatLiveAcrossReboot.fTabletYOffset = taskGlobals->fParams[4];
	}
	else
		GenericSWI(kSetTabletCalibration);
	return noErr;
}

NewtonErr
GetTabletCalibrationDataSWI(void)
{
	if (IsSuperMode())
	{
		TaskGlobals *	taskGlobals = TaskSwitchedGlobals();
		taskGlobals->fParams[0] = gGlobalsThatLiveAcrossReboot.fTabletValid;
		taskGlobals->fParams[1] = gGlobalsThatLiveAcrossReboot.fTabletXScale;
		taskGlobals->fParams[2] = gGlobalsThatLiveAcrossReboot.fTabletXOffset;
		taskGlobals->fParams[3] = gGlobalsThatLiveAcrossReboot.fTabletYScale;
		taskGlobals->fParams[4] = gGlobalsThatLiveAcrossReboot.fTabletYOffset;
	}
	else
		GenericSWI(kGetTabletCalibration);
	return noErr;
}


#pragma mark -
#pragma mark TabletDriver
/* -----------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e   t o   T a b l e t   D r i v e r
----------------------------------------------------------------------------- */

NewtonErr
TabInitialize(const Rect & inBounds, CUPort * inPort)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = TabletBufferInit(inPort))
		gTablet = (CTabletDriver *)MakeByName("CTabletDriver", "CMainTabletDriver");
		if (gTablet == NULL)
		{
			CResistiveTablet::classInfo()->registerProtocol();
			gTablet = (CTabletDriver *)CTabletDriver::make("CResistiveTablet");	// sic -- but why doesnÕt this work?
			gTablet = (CTabletDriver *)CResistiveTablet::classInfo()->make();
		}
		err = gTablet->init(inBounds);
	}
	XENDTRY;
	return err;	
}


int
GetTabletState(void)
{
	return gTablet->getTabletState();
}


void
GetTabletResolution(float * outResX, float * outResY)
{
	gTablet->getTabletResolution(outResX, outResY);
}


void
TabSetOrientation(int inOrientation)
{
	gTablet->setOrientation(inOrientation);
}


void
SetSampleRate(ULong inRate)
{
	gTablet->setSampleRate(inRate);
}


ULong
GetSampleRate(void)
{
	return gTablet->getSampleRate();
}


bool
TabletNeedsRecalibration(void)
{
	return gTablet->tabletNeedsRecalibration();
}


void
SetTabletCalibration(const Calibration & inCalibration)
{
	gTablet->setTabletCalibration(inCalibration);
}


void
GetTabletCalibration(Calibration * outCalibration)
{
	return gTablet->getTabletCalibration(outCalibration);
}


void
TabWakeUp(void)
{
	gTablet->wakeUp();
}


NewtonErr
StartBypassTablet(void)
{
	return gTablet->startBypassTablet();
}


NewtonErr
StopBypassTablet(void)
{
	return gTablet->stopBypassTablet();
}


#pragma mark -
#pragma mark PlainC
/* -----------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
----------------------------------------------------------------------------- */

Ref
FStartBypassTablet(RefArg inRcvr)
{
	return MAKEINT(StartBypassTablet());
}


Ref
FStopBypassTablet(RefArg inRcvr)
{
	return MAKEINT(StopBypassTablet());
}

