/*
	File:		TabletDriver.h

	Contains:	Resistive tablet driver.
	
	Written by:	Newton Research Group, 2010.
*/

#if !defined(__TABLETDRIVER_H)
#define __TABLETDRIVER_H

#include "Protocols.h"
#include "TabletTypes.h"
#include "NewtonTime.h"
#include "Interrupt.h"


/* -----------------------------------------------------------------------------
	T a b l e t   D r i v e r
	P-class interface.
----------------------------------------------------------------------------- */

PROTOCOL CTabletDriver : public CProtocol
{
public:
	static CTabletDriver * make(const char * inImplementation);
	void			destroy(void);

	NewtonErr	init(const Rect & inRect);

	void			setOrientation(int inOrientation);
	NewtonErr	recalibrateTabletAfterRotate(void);
	bool			tabletNeedsRecalibration(void);
	void			setTabletCalibration(const Calibration & inCalibration);
	void			getTabletCalibration(Calibration * outCalibration);
	void			setDoingCalibration(bool inDoingIt, ULong * outArg);

	void			setSampleRate(ULong inRate);
	ULong			getSampleRate(void);

	void			getTabletResolution(float * outX, float * outY);
	int			getTabletState(void);

	NewtonErr	setFingerInputState(bool inState);
	NewtonErr	getFingerInputState(bool * outState);

	void			wakeUp(void);
	void			returnTabletToConsciousness(ULong, ULong, ULong);

	void			shutDown(void);
	NewtonErr	tabletIdle(void);

	NewtonErr	startBypassTablet(void);
	NewtonErr	stopBypassTablet(void);
};


/* -----------------------------------------------------------------------------
	R e s i s t i v e   T a b l e t   D r i v e r
	Implementation.
----------------------------------------------------------------------------- */
struct	KeynesIntObject;
class		CBIOInterface;
class		CADC;

PROTOCOL CResistiveTablet : public CTabletDriver
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CResistiveTablet)

	CResistiveTablet *	make(void);
	void			destroy(void);

	NewtonErr	init(const Rect & inRect);

	void			setOrientation(int inOrientation);
	NewtonErr	recalibrateTabletAfterRotate(void);
	bool			tabletNeedsRecalibration(void);
	void			setTabletCalibration(const Calibration & inCalibration);
	void			getTabletCalibration(Calibration * outCalibration);
	void			setDoingCalibration(bool inDoingIt, ULong * outArg);

	void			setSampleRate(HardwareTimeUnits inRate);
	HardwareTimeUnits		getSampleRate(void);

	void			getTabletResolution(float * outX, float * outY);
	int			getTabletState(void);

	NewtonErr	setFingerInputState(bool inState);
	NewtonErr	getFingerInputState(bool * outState);

	void			wakeUp(void);
	void			returnTabletToConsciousness(ULong, ULong, ULong);

	void			shutDown(void);
	NewtonErr	tabletIdle(void);

	NewtonErr	startBypassTablet(void);
	NewtonErr	stopBypassTablet(void);

private:
	int			handleSample(void);
	ULong			convertSample(void);
	void			sampleResult(NewtonErr inErr, ULong);
	bool			d2Detect(void);
	void			enablePenDownInt(void);
	void			penUp(void);
	void			errorPenUp(void);
	void			setNextState(void);
	void			setNextTime(HardwareTimeUnits);
	NewtonErr	tabPenEntry(void);
	NewtonErr	doACMODInterrupt(void);
	void			setUpTabTimer(ULong);
	void			dumpRegs(void);

	InterruptObject *	f10;
	int32_t		f14;
	int32_t		f18;
	int			fState;				// +1C
	int32_t		f24;
	int32_t		f28;
	bool			fIsPenUp;			// +2C
	UChar			f2D;
	LPoint		fPenLocation;		// +30
	int32_t		f38;
	ULong			f3C;					// | time of last pen down / up?
	ULong			f40;					// |
	LPoint		f48;
	LPoint		f50;
	LPoint		f58;
	HardwareTimeUnits	fSampleInterval;		// +64
	ULong			f74;
	ULong			f78;
	bool			fIsCalibrating;	// +7C
	Calibration	fCalibration;		// +80
	LRect			fBounds;				// +94
	int			fOrientation;		// +A4
	KeynesIntObject *	fA8;
	CBIOInterface *	fAC;
	CADC *		fADC;					// +B0
// size +B4
};


#endif	/* __TABLETDRIVER_H */
