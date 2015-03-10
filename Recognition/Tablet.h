/*
	File:		Tablet.h

	Contains:	Input tablet definitions.

	Written by:	Newton Research Group.
*/

#if !defined(__TABLET_H)
#define __TABLET_H 1

#include "TabletTypes.h"

class CUPort;
struct Calibration;

void			TabBoot(void);
NewtonErr	TabletIdle(void);

NewtonErr	TabInitialize(const Rect & inBounds, CUPort * inPort);
void			GetTabletResolution(float * outX, float * outY);
void			TabSetOrientation(int inOrientation);

void			SetSampleRate(ULong inRate);
ULong			GetSampleRate(void);

bool			TabletNeedsRecalibration(void);
void			SetTabletCalibration(const Calibration & inCalibration);
void			GetTabletCalibration(Calibration * outCalibration);

void			TabWakeUp(void);
NewtonErr	StartBypassTablet(void);
NewtonErr	StopBypassTablet(void);

int			GetTabletState(void);


#endif	/* __TABLET_H */
