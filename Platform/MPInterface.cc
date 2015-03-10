/*------------------------------------------------------------------------------
	M P I n t e r f a c e
	Objective-C interface to Newton C++.
------------------------------------------------------------------------------*/
extern int gScreenHeight;

#include "TabletTypes.h"

extern "C" void	EnterAtomic(void);
extern "C" void	ExitAtomic(void);

extern NewtonErr	InsertTabletSample(ULong inSample, ULong inTime);
extern NewtonErr	InsertAndSendTabletSample(ULong inSample, ULong inTime);

extern "C" void	PenDown(float inX, float inY);
extern "C" void	PenMoved(float inX, float inY);
extern "C" void	PenUp(void);


void
PenDown(float inX, float inY)
{
	NewtonErr err;
	TabletSample sample;
	sample.intValue = kPenDownSample;
	err = InsertTabletSample(sample.intValue, 0);
	PenMoved(inX, inY);
}


void
PenMoved(float inX, float inY)
{
	NewtonErr err;
	TabletSample sample;
	EnterAtomic();
	sample.x = inX;
	sample.y = inY;
	sample.z = kPenMidPressureSample;
	err = InsertAndSendTabletSample(sample.intValue, 0);
	ExitAtomic();
}


void
PenUp(void)
{
	NewtonErr err;
	TabletSample sample;
	EnterAtomic();
	sample.intValue = kPenUpSample;
	err = InsertAndSendTabletSample(sample.intValue, 0);
	ExitAtomic();
}

