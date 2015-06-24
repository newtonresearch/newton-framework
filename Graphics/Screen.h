/*
	File:		Screen.h

	Contains:	Display driver.

	Written by:	Newton Research Group.
*/

#if !defined(__SCREEN_H)
#define __SCREEN_H 	

#include "QDTypes.h"

/*------------------------------------------------------------------------------
	Screen orientations.
------------------------------------------------------------------------------*/

enum ScreenOrientation
{
	kPortrait,
	kLandscape,
	kPortraitFlip,		// default, for some reason
	kLandscapeFlip
};


/*------------------------------------------------------------------------------
	GetGrafInfo selector.
------------------------------------------------------------------------------*/

enum GrafInfoSelector
{
	kGrafPixelMap,
	kGrafResolution,
	kGrafPixelDepth,
	kGrafContrast,
	kGrafOrientation,
	kGrafBacklight,
	kGrafInfo6,
	kGrafScreen
};


/*------------------------------------------------------------------------------
	Information returned by CScreenDriver::getScreenInfo.
------------------------------------------------------------------------------*/

struct ScreenGeometry
{
	ULong		height;		// +00
	ULong		width;		// +04
	ULong		depth;		// +08
	ULong		f0C;			// +0C
	Point		resolution;	// +10
};


struct ScreenInfo
{
	ULong		height;		// +00
	ULong		width;		// +04
	ULong		depth;		// +08
	ULong		f0C;			// +0C
	Point		resolution;	// +10
	ULong		f14;			// +14
	ULong		f18;			// +18
};


/*------------------------------------------------------------------------------
	Screen dimensions.
------------------------------------------------------------------------------*/

struct ScreenParams
{
	UChar	pixShift[5];	// was @44
	UChar	depthShift[5];	// was @65
	UChar	pixMask[5];		// was @88
};

extern const ScreenParams	gScreenConstants;
extern NativePixelMap		gScreenPixelMap;

extern int	gScreenWidth;
extern int	gScreenHeight;


/*------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
------------------------------------------------------------------------------*/

void			SetOrientation(int inOrientation);

void			SetGrafInfo(int inSelector, int inValue);
NewtonErr	GetGrafInfo(int inSelector, void * outInfo);

void			StartDrawing(PixelMap * inPixmap, Rect * inBounds);
void			StopDrawing(PixelMap * inPixmap, Rect * inBounds);


#endif	/* __SCREEN_H */
