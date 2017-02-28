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
	UChar	xShift[32+1];		// was @44	map bitsPerPixel 0..32 -> shift factor to translate x ordinate to word index in bitmap row
	UChar	depthShift[4+1];	// was @65
	ULong pixelMask[6+1];	// was @70
	UChar	xMask[32+1];		// was @88	map bitsPerPixel 0..32 -> mask  to translate x ordinate to bit index in word
	UChar	maskIndex[32+1];	// was @A9	map num of bits in pixel mask: 1 3 7 15 31 -> index into pixelMask
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

void			StartDrawing(NativePixelMap * inPixmap, Rect * inBounds);
void			StopDrawing(NativePixelMap * inPixmap, Rect * inBounds);


#endif	/* __SCREEN_H */
