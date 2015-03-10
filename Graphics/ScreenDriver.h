/*
	File:		ScreenDriver.h

	Contains:	Display driver.

	Written by:	Newton Research Group.
*/

#if !defined(__SCREENDRIVER_H)
#define __SCREENDRIVER_H

#include "Screen.h"
#include "Protocols.h"


/*------------------------------------------------------------------------------
	C S c r e e n D r i v e r
	P-class interface.
------------------------------------------------------------------------------*/

PROTOCOL CScreenDriver : public CProtocol
{
public:
	static CScreenDriver *	make(const char * inName);
	void		destroy(void);

	void		screenSetup(void);
	void		getScreenInfo(ScreenInfo * outInfo);

	void		powerInit(void);
	void		powerOn(void);
	void		powerOff(void);

	void		blit(PixelMap * inPixmap, Rect * inSrcBounds, Rect * inDstBounds, int inTransferMode);
	void		doubleBlit(PixelMap *, PixelMap * inPixmap, Rect * inSrcBounds, Rect * inDstBounds, int inTransferMode);

	int		getFeature(int inSelector);
	void		setFeature(int inSelector, int inValue);
	int		autoAdjustFeatures(void);

	void		enterIdleMode(void);
	void		exitIdleMode(void);
};


/*------------------------------------------------------------------------------
	C M a i n D i s p l a y D r i v e r
------------------------------------------------------------------------------*/
struct BlitRec;

PROTOCOL CMainDisplayDriver : public CScreenDriver
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CMainDisplayDriver)
	CAPABILITIES( "four" "" )

	CMainDisplayDriver *	make(void);
	void			destroy(void);

	void	screenSetup(void);
	void	getScreenInfo(ScreenInfo * outInfo);

	void	powerInit(void);
	void	powerOn(void);
	void	powerOff(void);

	void	blit(PixelMap * inPixmap, Rect * inSrcBounds, Rect * inDstBounds, int inTransferMode);
	void	doubleBlit(PixelMap *, PixelMap * inPixmap, Rect * inSrcBounds, Rect * inDstBounds, int inTransferMode);

	int	getFeature(int inSelector);
	void	setFeature(int inSelector, int inValue);
	int	autoAdjustFeatures(void);

	void	enterIdleMode(void);
	void	exitIdleMode(void);

private:
	void	blitLandscape(BlitRec * inBlit);
	void	blitLandscapeFlip(BlitRec * inBlit);
	void	blitPortrait(PixelMap * inPixmap, Rect * inSrcBounds, Rect * inDstBounds, int inTransferMode, int inMode);
	void	blitPortraitFlip(PixelMap * inPixmap, Rect * inSrcBounds, Rect * inDstBounds, int inTransferMode, int inMode);

	ScreenGeometry			fInfo;			// +00
	PixelMap					fPixMap;			// +10
	int						fOrientation;	// +2C
	long						fContrast;		// +30
	long						fContrast2;		// +34
	bool						fBacklight;		// +38
	ULong						fScreenWidth;	// +3C
	ULong						fScreenHeight;	// +40
//	CUPhys *					f44;				// +44
};


#endif	/* __SCREENDRIVER_H */
