/*
	File:		TabletDriver.cc

	Contains:	Tablet driver implementation.
					The tablet driver fills the gTabletData.buffer with samples.

	Written by:	Newton Research Group, 2010.
*/

#include "TabletDriver.h"
#include "TabletBuffer.h"
#include "VirtualMemory.h"

extern "C" void	EnterAtomic(void);
extern "C" void	ExitAtomic(void);


// originally 0x0000B400 == 46080
#define kSlowSampleInterval	12.5*kMilliseconds
// originally 0x00004800 == 18432
#define kFastSampleInterval	5*kMilliseconds

/* -----------------------------------------------------------------------------
	D a t a
	Unsure of these inits.
----------------------------------------------------------------------------- */

ULong		gTabPenDownDebounceTime = kFastSampleInterval;			// 0C100B30
bool		gTabEnbPenDownAtPenUp = false;		// 0C100B34
ULong		gTabPostnSamErrRange;				// 0C100B38
ULong		gTabPressSamErrRange;				// 0C100B3C
ULong		gTabLoopObserve;						// 0C100B40
ULong		gTabLoopSampleTime;					// 0C100B44
ULong		gTabPenDownMin;						// 0C100B48
ULong		gTabPenPressMaxDelta;				// 0C100B4C
ULong		gTabPenDownSampMin;					// 0C100B50
ULong		gTabSkipD2Detect = true;				// 0C100B54


/* -----------------------------------------------------------------------------
	R e s i s t i v e   T a b l e t   D r i v e r
----------------------------------------------------------------------------- */

/* ----------------------------------------------------------------
	CResistiveTablet implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CResistiveTablet::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN16CResistiveTablet6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN16CResistiveTablet4makeEv - 0b	\n"
"		.long		__ZN16CResistiveTablet7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CResistiveTabletDriver\"	\n"
"2:	.asciz	\"CTabletDriver\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN16CResistiveTablet9classInfoEv - 4b	\n"
"		.long		__ZN16CResistiveTablet4makeEv - 4b	\n"
"		.long		__ZN16CResistiveTablet7destroyEv - 4b	\n"
"		.long		__ZN16CResistiveTablet4initERK4Rect - 4b	\n"
"		.long		__ZN16CResistiveTablet6wakeUpEv - 4b	\n"
"		.long		__ZN16CResistiveTablet8shutDownEv - 4b	\n"
"		.long		__ZN16CResistiveTablet10tabletIdleEv - 4b	\n"
"		.long		__ZN16CResistiveTablet13getSampleRateEv - 4b	\n"
"		.long		__ZN16CResistiveTablet13setSampleRateEj - 4b	\n"
"		.long		__ZN16CResistiveTablet20getTabletCalibrationEP11Calibration - 4b	\n"
"		.long		__ZN16CResistiveTablet20setTabletCalibrationERK11Calibration - 4b	\n"
"		.long		__ZN16CResistiveTablet19setDoingCalibrationEbPj - 4b	\n"
"		.long		__ZN16CResistiveTablet19getTabletResolutionEPfS0_ - 4b	\n"
"		.long		__ZN16CResistiveTablet14setOrientationEi - 4b	\n"
"		.long		__ZN16CResistiveTablet14getTabletStateEv - 4b	\n"
"		.long		__ZN16CResistiveTablet19getFingerInputStateEPb - 4b	\n"
"		.long		__ZN16CResistiveTablet19setFingerInputStateEb - 4b	\n"
"		.long		__ZN16CResistiveTablet28recalibrateTabletAfterRotateEv - 4b	\n"
"		.long		__ZN16CResistiveTablet24tabletNeedsRecalibrationEv - 4b	\n"
"		.long		__ZN16CResistiveTablet17startBypassTabletEv - 4b	\n"
"		.long		__ZN16CResistiveTablet16stopBypassTabletEv - 4b	\n"
"		.long		__ZN16CResistiveTablet27returnTabletToConsciousnessEjjj - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CResistiveTablet)

CResistiveTablet *
CResistiveTablet::make(void)
{ return this; }


void
CResistiveTablet::destroy(void)
{ }


NewtonErr
CResistiveTablet::init(const Rect & inRect)
{
#if defined(correct)
	LockHeapRange(this, this + sizeof(this), false);
	dumpRegs();
	InitADC();
	fADC = GetADCObject();
#endif

	fBounds.top = inRect.top;
	fBounds.left = inRect.left;
	fBounds.bottom = inRect.bottom - 1;
	fBounds.right = inRect.right - 1;

	fCalibration.scale.x = 0xFFFFDFA5;	// -8284
	fCalibration.scale.y = 0x000015EC;	// 5612
	fCalibration.origin.x = 0x01F5F6B0;
	fCalibration.origin.y = 0xFFEE8314;
	fCalibration.x10 = 200;
	fCalibration.x11 = 230;

	f18 = 0x000025DF;	// 9695

	f24 = 700;
	f28 = 1000;

	f14 = 0;
	f78 = 0;
	fIsCalibrating = false;

	setOrientation(2);	// kPortraitFlip?

#if defined(correct)
	EnterAtomic();
	fAC = GetBIOInterfaceObject();
	fA8 = gBIOInterface->registerInterrupt(9, this, tabPenEntry, 2);
	penUp();
	f10 = RegisterInterrupt(0x10000000, this, doACMODInterrupt, 2);
	ExitAtomic();
#else
	penUp();
#endif

	fState = kTablet6;
	setNextState();

	return noErr;
}


void
CResistiveTablet::setOrientation(int inOrientation)
{
	fOrientation = inOrientation;
}


NewtonErr
CResistiveTablet::recalibrateTabletAfterRotate(void)
{
	return noErr;
}


bool
CResistiveTablet::tabletNeedsRecalibration(void)
{
	return gGlobalsThatLiveAcrossReboot.fTabletValid != 'G0/D'; // think this is right
}


void
CResistiveTablet::setTabletCalibration(const Calibration & inCalibration)
{
	fCalibration = inCalibration;
}


void
CResistiveTablet::getTabletCalibration(Calibration * outCalibration)
{
	*outCalibration = fCalibration;
}


void
CResistiveTablet::setDoingCalibration(bool inDoingIt, ULong * outArg)
{
	fIsCalibrating = inDoingIt;
	if (outArg)
		*outArg = 1;
}


void
CResistiveTablet::setSampleRate(HardwareTimeUnits inRate)
{
	fSampleInterval = inRate;
}


HardwareTimeUnits
CResistiveTablet::getSampleRate(void)
{
	return fSampleInterval;
}


void
CResistiveTablet::getTabletResolution(float * outX, float * outY)
{
	*outX = *outY = 800.0;
}


int
CResistiveTablet::getTabletState(void)
{
	return fState;
}


NewtonErr
CResistiveTablet::setFingerInputState(bool inState)
{
	return ERRBASE_TABLET - 8;
}


NewtonErr
CResistiveTablet::getFingerInputState(bool * outState)
{
	return ERRBASE_TABLET - 8;
}


void
CResistiveTablet::wakeUp(void)
{
	EnterAtomic();
	if (fState == kTabletAsleep)
	{
#if defined(correct)
		fADC->wakeUp();
#endif
		fState = kTabletAwake;
		setNextState();
	}
	ExitAtomic();
}


void
CResistiveTablet::returnTabletToConsciousness(ULong, ULong, ULong)
{
	wakeUp();
}


void
CResistiveTablet::shutDown(void)
{
	if (fState != kTabletAsleep)
	{
		EnterAtomic();
		errorPenUp();
		fState = kTabletAsleep;
		ExitAtomic();

#if defined(correct)
		fADC->shutDown();
#endif
		fState = kTabletAsleep;
		setNextState();
	}
}


NewtonErr
CResistiveTablet::tabletIdle(void)
{
	return noErr;
}


NewtonErr
CResistiveTablet::startBypassTablet(void)
{
	NewtonErr err = noErr;
	EnterAtomic();
	XTRY
	{
		XFAILNOT(fState == kTabletAwake || fState == kTabletBypassed, err = -1;)
		fState = kTabletBypassed;
		setNextState();
	}
	XENDTRY;
	ExitAtomic();
	return err;
}


NewtonErr
CResistiveTablet::stopBypassTablet(void)
{
	NewtonErr err = noErr;
	EnterAtomic();
	XTRY
	{
		XFAILNOT(fState == kTabletBypassed, err = -1;)
		penUp();
	}
	XENDTRY;
	ExitAtomic();
	return err;
}


#pragma mark -

ULong
CResistiveTablet::convertSample(void)
{
	f78++;

	ULong t1 = f3C;
	ULong t2 = f40;
	if (t1 < t2)
	{
		// may have wrapped around -- we want t1, t2 to be ordered
		t1 = f40;
		t2 = f3C;
	}
	if (t2 >= gTabPenDownMin)
		return (f78 > gTabPenDownSampMin) ? kPenUpSample : kPenInvalidSample;
	if (t1 - t2 > gTabPenPressMaxDelta)
		return kPenInvalidSample;

	TabletSample sample;
	if (fIsCalibrating)
	{
		// return raw points
		sample.x = fPenLocation.x;
		sample.y = fPenLocation.y;
	}
	else
	{
		// scale and offset points
		LPoint pt;
		pt.x = fCalibration.origin.x + fPenLocation.x / 16 * fCalibration.scale.x;
		pt.y = fCalibration.origin.y + fPenLocation.y / 16 * fCalibration.scale.y;
		int orientation = fOrientation - 1; if (orientation < 0) orientation += 4;
		if (orientation == 2 || orientation == 3)	// flipped
			pt.x = fBounds.right - pt.x;
		if (orientation == 1 || orientation == 2)	// landscape or flipped portrait
			pt.y = fBounds.bottom - pt.y;
		// ensure pt doesn’t exceed bounds
		if (pt.x < fBounds.left)
			pt.x = fBounds.left;
		else if (pt.x > fBounds.right)
			pt.x = fBounds.right;
		if (pt.y < fBounds.top)
			pt.y = fBounds.top;
		if (orientation == 1 || orientation == 3)	// landscape
		{
			// exchange x <-> y axes
			long tmp = pt.x;
			pt.x = pt.y;
			pt.y = tmp;
		}
		sample.x = pt.x & 0x07FFE000;		// or >> 13?  or / 8192?
		sample.y = pt.y & 0x07FFE000;
	}
	sample.z = kPenMidPressureSample;
	return (ULong)sample.intValue;
}


int
CResistiveTablet::handleSample(void)
{
	XTRY
	{
		TabletSample sample;
		sample.intValue = convertSample();

		XFAIL(sample.intValue == kPenInvalidSample)

		if (sample.z == kPenUpSample)
		{
			if (!fIsPenUp
			&&  InsertTabletSample(sample.intValue, 0) != noErr)
				return kTabletAwake;
			penUp();
			return 0;
		}

		// pen is down at this point
		XFAIL(sample.z > kPenMaxPressureSample)

		if (fIsPenUp)
		{
			// pen was applied
			if (InsertTabletSample(kPenDownSample, 0) == noErr)
			{
				d2Detect();
				fIsPenUp = false;
			}
			fSampleInterval = kSlowSampleInterval;
		}
		else
		{
			// pen continues doodling
			if (d2Detect())
				InsertTabletSample(sample.intValue, 0);
		}
		return kTabletAwake;
	}
	XENDTRY;
	return 6;
}


// calback from fADC->getSample()
void
CResistiveTablet::sampleResult(NewtonErr inErr, ULong inArg2)
{
	if (inErr == 0)
	{
		if (fState == 1)
		{
			f3C = inArg2;
			fState = 2;
		}
		else if (fState == 2)
		{
			fPenLocation.x = inArg2;
			fState = 3;
		}
		else if (fState == 3)
		{
			fPenLocation.y = inArg2;
			fState = 4;
		}
		else if (fState == 4)
		{
			f40 = inArg2;
			f38 = ((f3C + inArg2)/2) >> 4;
			fState = handleSample();
		}
		else
			errorPenUp();
	}
	else
		errorPenUp();
	if (fState != kTabletAsleep)
		setUpTabTimer(0);
}


inline long Fudge(long n)
{
	if (n < 100)
		n = 100;
	return n/2;
}

bool
CResistiveTablet::d2Detect(void)
{
	if (gTabSkipD2Detect)
		return true;

	bool status = true;
	if (f48.x == 99999)
	{
		f58.x = 99999;
		status = false;
	}
	else
	{
		long dX = f48.x - (fPenLocation.x >> 4);	// r6
		long dY = f48.y - (fPenLocation.y >> 4);	// lk
		long ddX = f50.x - dX;
		if (ddX < 0) ddX = -ddX;
		long ddY = f50.y - dY;
		if (ddY < 0) ddY = -ddY;
		if (f58.x == 99999)
		{
			if (ddX > 100 || ddY > 100)
				status = false;
		}
		else
		{
			if (ddX > Fudge(f58.x) || ddY > Fudge(f58.y))
			{
				if (f2D < 4)
					f2D++;
				else
				{
					InsertTabletSample(kPenDownSample, 0);
					penUp();
				}
				return false;
			}
		}
		f58.x = ++ddX;
		f58.y = ++ddY;
		f2D = 0;
		f50.x = dX;
		f50.y = dY;
	}
	f48.x = fPenLocation.x >> 4;
	f48.y = fPenLocation.y >> 4;
	return status;
}


void
CResistiveTablet::enablePenDownInt(void)
{
#if defined(correct)
	fAC->disableInterrupt(fA8);
	fADC->primeADCPenWait();
	fAC->clearInterrupt(fA8);
	fAC->enableInterrupt(fA8);
#endif
}


void
CResistiveTablet::penUp(void)
{
	if (gTabEnbPenDownAtPenUp)
		enablePenDownInt();
	fState = 0;
	fIsPenUp = true;
	fSampleInterval = kSlowSampleInterval;
	f48.x = 99999;
}


void
CResistiveTablet::errorPenUp(void)
{
	f3C = 0x0000E000;
	f40 = 0x0000E000;
	fPenLocation.x = fPenLocation.y = 0;
	f38 = 0x00000E00;
	fState = handleSample();
}


void
CResistiveTablet::setNextState(void)
{
#if defined(correct)
	switch (fState)
	{
	case 0:
		enablePenDownInt();
		break;
	case 1:
//		getSample(ADCMuxType, unsigned long, void (*)(void*, long, unsigned long), void*);
		fADC->getSample(3, gTabPressSamErrRange, sampleResult, this);
		break;
	case 2:
		fADC->getSample(1, gTabPostnSamErrRange, sampleResult, this);
		break;
	case 3:
		fADC->getSample(2, gTabPostnSamErrRange, sampleResult, this);
		break;
	case 4:
		fADC->getSample(4, gTabPressSamErrRange, sampleResult, this);
		break;
	case kTabletAwake:
		fADC->discharge();
		setNextTime(fSampleInterval);
		break;
	case kTablet6:
		fADC->discharge();
		setNextTime(kFastSampleInterval);
		break;
//	case 7:	does nothing
	case kTabletBypassed:
	case kTabletAsleep:
		fADC->discharge();
		break;
	}
#endif
}


void
CResistiveTablet::setNextTime(HardwareTimeUnits interval)
{
	ULong delta = interval - (g0F181000 - f74);
	if (delta < kMilliseconds)
		delta = kMilliseconds;
	if (delta > interval)
		delta = kMilliseconds;
	fState = 1;
	setUpTabTimer(delta);
}


NewtonErr
CResistiveTablet::tabPenEntry(void)
{
	f74 = g0F181000;
	f78 = 0;
#if defined(correct)
	fADC->clearADCPenWait();
	WakeUpInkerFromInterrupt(0);
	gBIOInterface->disableInterrupt(fA8);
#endif
	fSampleInterval = gTabPenDownDebounceTime;
	fState = 5;
	setNextState();
	return noErr;
}


NewtonErr
CResistiveTablet::doACMODInterrupt(void)
{
	NewtonErr err;
	ClearInterrupt(f10);
	err = DisableInterrupt(f10);
	if (f14 > 0)
	{
		f14--;
		err = EnableInterrupt(f10, 2);
	}
	else
	{
		if (fState == 1)
			f74 = g0F182400;
		if (err == noErr)
			setNextState();
	}
	return err;
}


void
CResistiveTablet::setUpTabTimer(ULong inDelta)
{
	ULong r2;
	for (r2 = 0; inDelta > f18; r2++)
		inDelta -= f18;
	f14 = r2;
	EnableInterrupt(f10, 2);
}


void
CResistiveTablet::dumpRegs(void)
{
#if defined(correct)
	int r0;
	r0 = g0F048000;
	r0 = g0F04A000;
	gBIOInterface->bIOReadRegister(19);
	gBIOInterface->bIOReadRegister(18);
	gBIOInterface->bIOReadRegister(17);
#endif
}

