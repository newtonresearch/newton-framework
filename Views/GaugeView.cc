/*
	File:		GaugeView.cc

	Contains:	CGaugeView implementation.

	Written by:	Newton Research Group.
*/

#include "Quartz.h"
#include "QDGeometry.h"
#include "GaugeView.h"

#include "NewtonTime.h"
#include "Arrays.h"
#include "Globals.h"
#include "Sound.h"

#include "Unit.h"

extern void BusyBoxSend(int inCmd);


/*------------------------------------------------------------------------------
	C G a u g e V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clGaugeView, CGaugeView, CView)


/*--------------------------------------------------------------------------------
	Perform a command.
	If it’s a click then track the pen otherwise pass it on.
	Args:		inCmd		the command frame
	Return:	YES if we handled the command
--------------------------------------------------------------------------------*/

bool
CGaugeView::realDoCommand(RefArg inCmd)
{
	bool isHandled;

	if (CommandId(inCmd) == aeClick && (viewFlags & vReadOnly) == 0)
	{
		isHandled = trackSetValue((CUnit *)CommandParameter(inCmd));
		CommandSetResult(inCmd, isHandled);
		if (isHandled)
			return isHandled;
		// otherwise give superclass a chance to handle it
	}
	return CView::realDoCommand(inCmd);
}


/*--------------------------------------------------------------------------------
	Initialize.
	Set the max and min values from the context frame. Defaults are 100, 0.
	Args:		inContext	the context frame for this view
				inView		the parent view
	Return:	--
--------------------------------------------------------------------------------*/

void
CGaugeView::init(RefArg inContext, CView * inView)
{
	CView::init(inContext, inView);

	fMaxValue = 100;
	RefVar	max = getValue(SYMA(maxValue), RA(NILREF));
	if (NOTNIL(max))
		fMaxValue = RINT(max);

	fMinValue = 0;
	RefVar	min = getValue(SYMA(minValue), RA(NILREF));
	if (NOTNIL(min))
		fMinValue = RINT(min);
}


/*--------------------------------------------------------------------------------
	Draw the gauge.
	Args:		inBounds		ignored
	Return:	--
--------------------------------------------------------------------------------*/

void
CGaugeView::realDraw(Rect * inBounds)
{
	Rect	bar;
	bool  isSlider;
	int	knobWidth;
	int	barHeight;
	float barWidth;
	
	int viewValue = RINT(getValue(SYMA(viewValue), RA(NILREF)));
	viewValue = MINMAX(fMinValue, viewValue, fMaxValue);

	// adjust bounds
	bar = viewBounds;
	barHeight = bar.bottom - bar.top;
	if (EVEN(barHeight))
	{
		barHeight--;
		bar.top--;
	}
	isSlider = ((viewFlags & vReadOnly) == 0);
	knobWidth = isSlider ? barHeight : 0;
	barWidth = (bar.right - bar.left - knobWidth) * (viewValue - fMinValue) / (fMaxValue - fMinValue) + knobWidth / 2;

	// draw black bar
	Rect barBox;
	SetRect(&barBox, bar.left, bar.top+2, bar.left+barWidth, bar.top+barHeight-2);

	CGContextSetFillColorWithColor(quartz, gBlackColor);
	CGContextFillRect(quartz, MakeCGRect(barBox));

	// draw gray gauge background, if required
	if (NOTNIL(getProto(SYMA(gaugeDrawLimits))))
	{
		barBox.top++;
		barBox.bottom--;
		barBox.left += barWidth;
		barBox.right = viewBounds.right;
		CGContextSetFillColorWithColor(quartz, gGrayColor);
		CGContextFillRect(quartz, MakeCGRect(barBox));
	}

	// if a slider rather than just a gauge, draw the knob
	if (isSlider)
	{
		Point		startPt;
		startPt.h = bar.left + barWidth - knobWidth/2;
		startPt.v = bar.bottom;
		CGPoint	knobPt = MakeCGPoint(startPt);
		CGRect	knob = CGRectMake(knobPt.x, knobPt.y, knobWidth, knobWidth);
		CGFloat	midX = CGRectGetMidX(knob);
		CGFloat	midY = CGRectGetMidY(knob);

		// draw a path clockwise from the bottom of the knob
		CGPoint knobOutline[5];
		knobOutline[0] = CGPointMake(midX, knob.origin.y);
		knobOutline[1] = CGPointMake(knob.origin.x, midY);
		knobOutline[2] = CGPointMake(midX, knob.origin.y + knobWidth);
		knobOutline[3] = CGPointMake(knob.origin.x + knobWidth, midY);
		knobOutline[4] = CGPointMake(midX, knob.origin.y);
		CGContextBeginPath(quartz);
		CGContextAddLines(quartz, knobOutline, 5);
		CGContextClosePath(quartz);

		CGContextSetFillColorWithColor(quartz, gWhiteColor);
		CGContextSetStrokeColorWithColor(quartz, gBlackColor);
		CGContextSetLineWidth(quartz, 1);
		CGContextDrawPath(quartz, kCGPathFillStroke);
	}
}


/*--------------------------------------------------------------------------------
	Track the slider while the pen is down.
	Args:		inUnit		the pen unit
	Return:	YES always
--------------------------------------------------------------------------------*/

bool
CGaugeView::trackSetValue(CUnit * inUnit)
{
	// don’t show the busy box while tracking
	BusyBoxSend(53);

	CStroke * theStroke = inUnit->stroke();
	theStroke->inkOff(YES);

	RefVar snd(getProto(SYMA(_sound)));

	int  initialValue = RINT(getValue(SYMA(viewValue), RA(NILREF)));
	int  previousValue = initialValue;
	int  viewValue;

	do
	{
		Point pt = theStroke->finalPoint();
		viewValue = fMinValue + (fMaxValue - fMinValue) * (pt.h - viewBounds.left) / (viewBounds.right - viewBounds.left);
		viewValue = MINMAX(fMinValue, viewValue, fMaxValue);
		if (viewValue == previousValue)
			// no need to do anything -- just yield to other tasks
			Wait(20);	// milliseconds - was 1 tick
		else
		{
			if (NOTNIL(snd))
				FPlaySound(fContext, snd);
			previousValue = viewValue;
			setSlot(SYMA(viewValue), MAKEINT(viewValue));
			gRootView->update();
		}
	} while (!theStroke->isDone());

	if (viewValue != initialValue)
	{
		RefVar	args(MakeArray(2));
		SetArraySlot(args, 0, initialValue);
		SetArraySlot(args, 1, viewValue);
		runScript(SYMA(viewFinalChangeScript), args, NO, NULL);
	}

	BusyBoxSend(54);
	return YES;
}


/*--------------------------------------------------------------------------------
	Set a slot.
	If it’s the minValue or maxValue slot then update our variables.
	Args:		inTag			the slot name
				inValue		the new value
	Return:	--
--------------------------------------------------------------------------------*/

void
CGaugeView::setSlot(RefArg inTag, RefArg inValue)
{
	if (EQ(inTag, SYMA(maxValue)))
		fMaxValue = RINT(inValue);
	else if (EQ(inTag, SYMA(minValue)))
		fMinValue = RINT(inValue);

	CView::setSlot(inTag, inValue);
}

