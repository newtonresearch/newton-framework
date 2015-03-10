/*
	File:		Quartz.cc

	Contains:	Quartz core graphics interface for Newton.

	Written by:	Newton Research Group.
*/

#include "Quartz.h"
#include "DrawShape.h"

#include "Objects.h"
#include "ROMSymbols.h"
#include "ViewFlags.h"

extern void	InitQDCompression(void);

#if defined(forFramework)
long				gScreenHeight;
#else
extern long		gScreenHeight;
#endif

CGContextRef	quartz;
CGColorRef		gPattern[5];
float				gLineWidth;


/*------------------------------------------------------------------------------
	Initialize the Quartz graphics system.
	quartz must be already set up from [NSGraphicsContext currentContext]
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

void
InitGraf(void)
{
	InitQDCompression();

	// draw in RGB color space
	CGColorSpaceRef	rgbSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);

	// set up common colors
	const float	whiteRGBA[4] = { 1.0, 1.0, 1.0, 1.0 };
	const float	ltGrayRGBA[4] = { 0.75, 0.75, 0.75, 1.0 };
	const float	grayRGBA[4] = { 0.5, 0.5, 0.5, 1.0 };
	const float	dkGrayRGBA[4] = { 0.25, 0.25, 0.25, 1.0 };
	const float	blackRGBA[4] = { 0.0, 0.0, 0.0, 1.0 };

	gPattern[whitePat] = CGColorCreate(rgbSpace, whiteRGBA);
	gPattern[ltGrayPat] = CGColorCreate(rgbSpace, ltGrayRGBA);
	gPattern[grayPat] = CGColorCreate(rgbSpace, grayRGBA);
	gPattern[dkGrayPat] = CGColorCreate(rgbSpace, dkGrayRGBA);
	gPattern[blackPat] = CGColorCreate(rgbSpace, blackRGBA);

	CGColorSpaceRelease(rgbSpace);
}


#if defined(forFramework)
void
InitDrawing(CGContextRef inContext, int inScreenHeight)
{
	quartz = inContext;
	gScreenHeight = inScreenHeight;
}
#endif


void
InitFontLoader(void)
{
/*
	gFontPartHandler = new TPartHandler();	// 0C1053FC
	gFontPartHandler->init('font');
	LoadFontTable();	// which really does nothing
*/
}

// stuff that should be in Screen.cc, but is required forFramework too

extern "C" {
Ref	FGetLCDContrast(RefArg inRcvr);
Ref	FSetLCDContrast(RefArg inRcvr, RefArg inContrast);
}

Ref
FGetLCDContrast(RefArg inRcvr)
{
	long	contrast;
#if defined(correct)
	GetGrafInfo(kGrafContrast, &contrast);
#else
	contrast = 2;
#endif
	return MAKEINT(contrast);
}


Ref
FSetLCDContrast(RefArg inRcvr, RefArg inContrast)
{
#if defined(correct)
	SetGrafInfo(kGrafContrast, RINT(inContrast));
#endif
	return NILREF;
}

#pragma mark -

/*------------------------------------------------------------------------------
	U t i l i t i e s
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Transform a Rect to CG orientation.
	Args:		inRect
	Return:	Rect
------------------------------------------------------------------------------*/

Rect
RectInCG(const Rect * inRect)
{
	Rect  r;
	r.top = gScreenHeight - inRect->top;
	r.left = inRect->left;
	r.bottom = gScreenHeight - inRect->bottom;
	r.right = inRect->right;
	return r;
}


/*------------------------------------------------------------------------------
	Make a CGRect from a Newton Rect, flipping the vertical axis.
	Args:		inRect
	Return:	CGRect
------------------------------------------------------------------------------*/

CGRect
CGRectMake(const Rect * inRect)
{
	CGRect  r;
	r.origin.x = inRect->left;
	r.origin.y = gScreenHeight - inRect->bottom;
	r.size.width = RectGetWidth(inRect);
	r.size.height = RectGetHeight(inRect);
	return r;
}


CGRect
CGRectMakeFrame(const Rect * inRect, float inLineWd)
{
	CGRect r = CGRectMake(inRect);
	float  inset = inLineWd / 2.0;
	return CGRectInset(r, inset, inset);
}


/*------------------------------------------------------------------------------
	Justify a rect within a frame.
	Args:		inRect
				inFrame
				inJustify
	Return:	justified CGRect
------------------------------------------------------------------------------*/

CGRect
CGRectJustify(CGRect inRect, CGRect inFrame, ULong inJustify)
{
	CGRect  frame = inFrame;
	CGPoint rectOrigin = inFrame.origin;
	CGSize  rectSize = inRect.size;
	float  offset;

	if (CGRectGetWidth(frame) == 0
	&&  CGRectGetHeight(frame) == 0)
	{
		frame.size.width = CGRectGetWidth(inRect);
		frame.size.height = CGRectGetHeight(inRect);
	}

	switch (inJustify & vjVMask)
	{
	case vjTopV:
		offset = CGRectGetHeight(frame) - rectSize.height;
		rectOrigin.y += offset;
		break;
	case vjCenterV:
		offset = (CGRectGetHeight(frame) - rectSize.height) / 2;
		if (offset > 0)
			rectOrigin.y += offset;
		break;
	case vjBottomV:
	// no need to do anything
		break;
	case vjFullV:
		rectSize.height = CGRectGetHeight(frame);
		break;
	}

	switch (inJustify & vjHMask)
	{
	case vjLeftH:
	// no need to do anything
		break;
	case vjRightH:
		offset = CGRectGetWidth(frame) - rectSize.width;
		rectOrigin.x += offset;
		break;
	case vjCenterH:
		offset = (CGRectGetWidth(frame) - rectSize.width) / 2;
		if (offset > 0)
			rectOrigin.x += offset;
		break;
	case vjFullH:
		rectSize.width = CGRectGetWidth(frame);
		break;
	}

	return CGRectMake(rectOrigin.x, rectOrigin.y, rectSize.width, rectSize.height);
}


/*------------------------------------------------------------------------------
	Justify a rect within a frame.
	Args:		ioRect
				inFrame
				inJustify
	Return:	justified Rect
------------------------------------------------------------------------------*/

Rect *
Justify(Rect * ioRect, const Rect * inFrame, ULong inJustify)
{
	Rect  frame = *inFrame;
	Point rectOrigin = *(Point*)inFrame;
	Point rectSize;
	long  offset;

	rectSize.h = RectGetWidth(ioRect);
	rectSize.v = RectGetHeight(ioRect);

	if (RectGetWidth(&frame) == 0
	&&  RectGetHeight(&frame) == 0)
	{
		frame.right = frame.left + RectGetWidth(ioRect);
		frame.bottom = frame.top + RectGetHeight(ioRect);
	}

	switch (inJustify & vjVMask)
	{
	case vjTopV:
	// no need to do anything
		break;
	case vjCenterV:
		offset = (RectGetHeight(&frame) - rectSize.v) / 2;
		if (offset > 0)
			rectOrigin.v += offset;
		break;
	case vjBottomV:
		offset = RectGetHeight(&frame) - rectSize.v;
		rectOrigin.v += offset;
		break;
	case vjFullV:
		rectSize.v = RectGetHeight(&frame);
		break;
	}

	switch (inJustify & vjHMask)
	{
	case vjLeftH:
	// no need to do anything
		break;
	case vjRightH:
		offset = RectGetWidth(&frame) - rectSize.h;
		rectOrigin.h += offset;
		break;
	case vjCenterH:
		offset = (RectGetWidth(&frame) - rectSize.h) / 2;
		if (offset > 0)
			rectOrigin.h += offset;
		break;
	case vjFullH:
		rectSize.h = RectGetWidth(&frame);
		break;
	}

	SetRect(ioRect, rectOrigin.h, rectOrigin.v, rectOrigin.h + rectSize.h, rectOrigin.v + rectSize.v);
	return ioRect;
}

#pragma mark -

ULong
PackRGBvalues(ULong inR, ULong inG, ULong inB)
{
	return 0x10000000 + ((inR & 0xFF00) << 8) + (inG & 0xFF00) + ((inB & 0xFF00) >> 8);
}


void
UnpackRGBvalues(ULong inRGB, ULong * outR, ULong * outG, ULong * outB)
{
	*outR = (inRGB >> 16) & 0xFF;
	*outG = (inRGB >> 8) & 0xFF;
	*outB = inRGB & 0xFF;
	*outR += (*outR << 8);
	*outG += (*outG << 8);
	*outB += (*outB << 8);
}

/* This is the Grayscale Conversion according to the Luminance Model
	used by NTSC and JPEG for nonlinear data in a gamma working space:
	Y = 0.299R + 0.587G + 0.114B
*/
ULong
RGBtoGray(ULong inR, ULong inG, ULong inB, int inNumOfBits, int inDepth)
{
	ULong  gray = 19589 * inR
					+ 38443 * inG
					+  7497 * inB;

	return ~gray >> (32 - inNumOfBits);
}

#pragma mark -

#if 0
void
MakeSimpleStyle(RefArg inFamily, long inSize, long inFace)
{
	RefStruct		sp;
	PatternHandle  pat = NULL; // sp1C
	;
	sp = inFontSpec;
	style->f00 = new RefStruct(sp00);
	sp.f04 = inArg1;
	sp.f08 = inArg2;
	sp.f0C = 0;
	sp.f10 = 0;
	sp.f14 = 0;
	
	if (pat)
		DisposePattern(pat);
}


typedef struct
{
	PixelMap pix;
	char		pat[8];
} SimplePattern;

PatternHandle
MakeSimplePattern(long inBits0, long inBits1, long inBits2, long inBits3, long inBits4, long inBits5, long inBits6, long inBits7)
{
	PatternHandle  pat = (PatternHandle) NewHandle(sizeof(SimplePattern));	// NOTE NewHandle deprecated
	if (pat)
	{
		PixelMap *  pixmap = *pat;
		pixmap->baseAddr = (Ptr) offsetof(SimplePattern, pat);	// actually an offset, obviously
		pixmap->rowBytes = 1;
		SetRect(&pixmap->bounds, 0, 0, 8, 8);
		pixmap->pixMapFlags = kPixMapOffset + kOneBitDepth;
		pixmap->deviceRes.h =
		pixmap->deviceRes.v = kDefaultDPI;
		pixmap->grayTable = NULL;
		((SimplePattern*)pixmap)->pat[0] = inBits0;
		((SimplePattern*)pixmap)->pat[1] = inBits1;
		((SimplePattern*)pixmap)->pat[2] = inBits2;
		((SimplePattern*)pixmap)->pat[3] = inBits3;
		((SimplePattern*)pixmap)->pat[4] = inBits4;
		((SimplePattern*)pixmap)->pat[5] = inBits5;
		((SimplePattern*)pixmap)->pat[6] = inBits6;
		((SimplePattern*)pixmap)->pat[7] = inBits7;
	}
	return pat;
}
#endif

CGColorRef
MakeSimplePattern(const char * inBits)
{
#if 0
	PatternHandle  pat = (PatternHandle) NewHandle(sizeof(SimplePattern));	// NOTE NewHandle deprecated
	if (pat)
	{
		PixelMap *  pixmap = *pat;
		pixmap->baseAddr = (Ptr) offsetof(SimplePattern, pat);	// actually an offset, obviously
		pixmap->rowBytes = 1;
		SetRect(&pixmap->bounds, 0, 0, 8, 8);
		pixmap->pixMapFlags = kPixMapOffset + kOneBitDepth;
		pixmap->deviceRes.h =
		pixmap->deviceRes.v = kDefaultDPI;
		pixmap->grayTable = NULL;
		memmove(&((SimplePattern*)pixmap)->pat, inBits, 8);
	}
#else
	CGColorRef pat = NULL;
#endif
	return pat;
}


CGColorRef
MakeGrayPattern(RefArg inPat)
{ return NULL; }


/*------------------------------------------------------------------------------
	Set the pattern for drawing a viewÕs frame.
	Used only by View.cc.
	Args:		inPatNo
	Return:	YES	if a standard pattern was set
				NO		if a custom pattern was specified
------------------------------------------------------------------------------*/

BOOL
SetPattern(long inPatNo)
{
	BOOL			isSet = YES;
	CGColorRef	ptn;

	switch (inPatNo)
	{
	case vfWhite:
		ptn = gPattern[whitePat];
		break;
	case vfLtGray:
		ptn = gPattern[ltGrayPat];
		break;
	case vfGray:
		ptn = gPattern[grayPat];
		break;
	case vfDkGray:
		ptn = gPattern[dkGrayPat];
		break;
	case vfBlack:
		ptn = gPattern[blackPat];
		break;
	case 12:
		ptn = gPattern[blackPat];
		break;
	case vfDragger:
	case vfMatte:
		ptn = gPattern[grayPat];
		break;
	case vfCustom:
		break;
	default:
		isSet = NO;
	}
	if (isSet)
		CGContextSetFillColorWithColor(quartz, ptn);	// should SetStrokeColor?

	return isSet;
}


CGColorRef
GetStdPattern(int inPatNo)
{
	if (inPatNo > blackPat)
		inPatNo = blackPat;
	return gPattern[inPatNo];
}


CGColorRef
GetStdGrayPattern(ULong inR, ULong inG, ULong inB)
{
#if 0
	GrafPtr			thePort = GetCurrentPort();
	PixelMap *		pix = IsPrinterPort(thePort) ? &qd.pixmap : &thePort->portBits;
	long				depth = PixelDepth(pix);
	long				numOfBytes = 8 * depth;
	PatternHandle  thePattern = (PatternHandle) NewHandle(sizeof(PixelMap) + numOfBytes);	// NOTE NewHandle deprecated
	if (thePattern)
	{
		long				gray = RGBtoGray(inR, inG, inB, 16, depth);
		unsigned char  grayByte;
		Ptr				grayPtr;
		pix->baseAddr = (Ptr) sizeof(PixelMap);	// offset to gray data
		pix->rowBytes = numOfBytes / 8;				// 8 rows (of 8 pixels) in a std pattern
		SetRect(&pix->bounds, 0, 0, 8, 8);
		pix->pixMapFlags = kPixMapOffset + depth;
		pix->deviceRes.h =
		pix->deviceRes.h = kDefaultDPI;
		pix->grayTable = NULL;
		grayPtr = (Ptr)pix + sizeof(PixelMap);		// data follows immediately

		switch (depth)
		{
		case 1:
			grayByte = gray ? 0xFF : 0;
			break;
		case 2:
			grayByte = (gray << 6) | (gray << 4) | (gray << 2) | gray;
			break;
		case 4:
			grayByte = (gray << 4) | gray;
			break;
		}

		for ( ; numOfBytes > 0; numOfBytes--)
			*grayPtr++ = grayByte;
	}
#else
	CGColorRef thePattern;
	CGColorSpaceRef	rgbSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	CGContextSetFillColorSpace(quartz, rgbSpace);
	float colorComponents[4] = { inR, inG, inB, 1.0 };
	thePattern = CGColorCreate(rgbSpace, colorComponents);
	CGColorSpaceRelease(rgbSpace);
#endif
	return thePattern;
}


BOOL
GetPattern(RefArg inPatNo, BOOL * ioTakeOwnership, CGColorRef * ioPat, BOOL inDefault)
{
	BOOL	gotPattern = inDefault;

	if (ISINT(inPatNo))
	{
		int patNo = RINT(inPatNo);
		BOOL isOpaque = (patNo > 0);
		if (isOpaque)
		{
			if (*ioTakeOwnership)
				CGColorRelease(*ioPat);
			if ((patNo & 0x10000000) != 0)	// itÕs not actually an index but an RGB value
			{
				ULong	r;
				ULong	g;
				ULong	b;
				UnpackRGBvalues(patNo, &r, &g, &b);
				*ioPat = GetStdGrayPattern(r,g,b);
			}
			else
			{
				*ioPat = GetStdPattern((patNo & 0xFF) - 1);
			}
			if (*ioPat != NULL)
			{
				*ioTakeOwnership = YES;
				gotPattern = YES;
			}
		}
	}

	else if (IsFrame(inPatNo))
	{
		if (EQRef(ClassOf(inPatNo), RSYMditherPattern))
		{
#if 0
			ULong	r, g, b;
			ULong	fgColor;
			ULong	bgColor;
			int	pixDepth;
			GetGrafInfo(kGrafPixelDepth, &pixDepth);

			RefVar item(GetFrameSlot(inPatNo, SYMforeground));
			if (NOTNIL(item) && ISINT(item))
			{
				UnpackRGBvalues(RINT(item), &r, &g, &b);
				fgColor = RGBtoGray(r,g,b, pixDepth, pixDepth);
			}
			else
				fgColor = 0xFFFFFFFF >> (32-pixDepth);	// white

			item = GetFrameSlot(inPatNo, SYMbackground);
			if (NOTNIL(item) && ISINT(item))
			{
				UnpackRGBvalues(RINT(item), &r, &g, &b);
				bgColor = RGBtoGray(r,g,b, pixDepth, pixDepth);
			}
			else
				bgColor = 0;									// black

			item = GetFrameSlot(inPatNo, SYMpattern);
			if (NOTNIL(item))
			{
				CGColorRef	ptn;
				if (ISINT(item))
				{
					if (item >= 1
					&&  item <= 5)
						item--;
					else
						item = blackPat;
					ptn = GetStdPattern(item);
				}
				else
				{
					CDataPtr	patData(item);
					ptn = MakeSimplePattern(patData);
				}
				if (ptn != NULL)
				{
					if (*ioTakeOwnership)
						CGColorRelease(*ioPat);
					HLock(ptn);
					*ioPat = MakeSimpleGrayPattern(GetPixelMapBits(*ptn), fgColor, bgColor);
					HUnlock(ptn);
					DisposePattern(ptn);
				}
			}
			else	// no pattern specified; use foreground
			{
				if (*ioTakeOwnership)
					CGColorRelease(*ioPat);
				UnpackRGBvalues(fgColor, &r, &g, &b);
				*ioPat = GetStdGrayPattern(r,g,b);
			}
			if (*ioPat != NULL)
			{
				*ioTakeOwnership = YES;
				gotPattern = YES;
			}
#endif
		}
	}

	else if (EQ(ClassOf(inPatNo), RSYMgrayPattern))
	{
		if (*ioTakeOwnership)
			CGColorRelease(*ioPat);
		*ioPat = MakeGrayPattern(inPatNo);
		if (*ioPat != NULL)
		{
			*ioTakeOwnership = YES;
			gotPattern = YES;
		}
	}

	else if (IsArray(inPatNo))
	{
		if (*ioTakeOwnership)
			CGColorRelease(*ioPat);
		CDataPtr	patData(inPatNo);
		*ioPat = MakeSimplePattern(patData);
		if (*ioPat != NULL)
		{
			*ioTakeOwnership = YES;
			gotPattern = YES;
		}
	}
	
	return gotPattern;
}


#pragma mark -

/*------------------------------------------------------------------------------
	D r a w i n g
------------------------------------------------------------------------------*/
void		StrokeRoundedRect(CGContextRef inContext, CGRect inRect, float inOvalWidth, float inOvalHeight);
void		FillRoundedRect(CGContextRef inContext, CGRect inRect, float inOvalWidth, float inOvalHeight);
static void		AddRoundedRectToPath(CGContextRef inContext, CGRect inRect, float ovalWidth, float ovalHeight);

void
LineNormal(void)
{
	SetLineWidth(1.0);
	CGContextSetStrokeColorWithColor(quartz, gPattern[blackPat]);
}

void
SetLineWidth(float inWidth)
{
	gLineWidth = inWidth;
	CGContextSetLineWidth(quartz, inWidth);
}

float
LineWidth(void)
{
	return gLineWidth;
}

void
StrokeLine(CGPoint inFrom, CGPoint inTo)
{
	CGContextBeginPath(quartz);
	CGContextMoveToPoint(quartz, inFrom.x, inFrom.y);
	CGContextAddLineToPoint(quartz, inTo.x, inTo.y);
	CGContextStrokePath(quartz);
}


void
FrameRect(const Rect * inRect)
{
	CGContextStrokeRect(quartz, CGRectMakeFrame(inRect, LineWidth()));
}


void
PaintRect(const Rect * inRect)
{
	CGContextFillRect(quartz, CGRectMake(inRect));
}


void
FrameOval(const Rect * inRect)
{
	CGContextStrokeEllipseInRect(quartz, CGRectMakeFrame(inRect, LineWidth()));
}


void
PaintOval(const Rect * inRect)
{
	CGContextFillEllipseInRect(quartz, CGRectMake(inRect));
}


void
FrameRoundRect(const Rect * inRect, int inOvalWd, int inOvalHt)
{
	StrokeRoundedRect(quartz, CGRectMakeFrame(inRect, LineWidth()), inOvalWd, inOvalHt);
}


void
PaintRoundRect(const Rect * inRect, int inOvalWd, int inOvalHt)
{
	FillRoundedRect(quartz, CGRectMake(inRect), inOvalWd, inOvalHt);
}


void
StrokeRoundedRect(CGContextRef inContext, CGRect inRect, float inOvalWidth, float inOvalHeight)
{
	CGContextBeginPath(inContext);
	AddRoundedRectToPath(inContext, inRect, inOvalWidth, inOvalHeight);
	CGContextStrokePath(inContext);
}


void
FillRoundedRect(CGContextRef inContext, CGRect inRect, float inOvalWidth, float inOvalHeight)
{
	CGContextBeginPath(inContext);
	AddRoundedRectToPath(inContext, inRect, inOvalWidth, inOvalHeight);
	CGContextFillPath(inContext);
}


static void
AddRoundedRectToPath(CGContextRef inContext, CGRect inRect, float ovalWidth, float ovalHeight)
{
	if (ovalWidth == 0 || ovalHeight == 0)
		CGContextAddRect(inContext, inRect);

	else
	{
		float fw, fh;

		CGContextSaveGState(inContext);
		CGContextTranslateCTM(inContext, CGRectGetMinX(inRect), CGRectGetMinY(inRect));
		CGContextScaleCTM(inContext, ovalWidth, ovalHeight);
		fw = CGRectGetWidth(inRect) / ovalWidth;
		fh = CGRectGetHeight(inRect) / ovalHeight;
		CGContextMoveToPoint(inContext, fw, fh/2);
		CGContextAddArcToPoint(inContext, fw, fh, fw/2, fh, 1);
		CGContextAddArcToPoint(inContext, 0, fh, 0, fh/2, 1);
		CGContextAddArcToPoint(inContext, 0, 0, fw/2, 0, 1);
		CGContextAddArcToPoint(inContext, fw, 0, fw, fh/2, 1);
		CGContextClosePath(inContext);
		CGContextRestoreGState(inContext);
	}
}


struct PolygonShape
{
	short		size;
	short		reserved1;
	Rect		bBox;
	Point		points[];
};


void
OffsetPoly(void * inShape, short dx, short dy)
{
	if (dx != 0 && dy != 0)
	{
		Point * p = ((PolygonShape *)inShape)->points;
		int i, count = (((PolygonShape *)inShape)->size - 12) / sizeof(Point);
		for (i = 0; i < count; i++)
		{
			p->h += dx;
			p->v += dy;
		}
	}
}


void
PaintPoly(void * inShape)
{
	Point * p = ((PolygonShape *)inShape)->points;
	int i, count = (((PolygonShape *)inShape)->size - 12) / sizeof(Point);
	CGContextBeginPath(quartz);
	CGContextMoveToPoint(quartz, p->h, gScreenHeight - p->v);
	for (i = 1, p++; i < count; i++, p++)
	{
		CGContextAddLineToPoint(quartz, p->h, gScreenHeight - p->v);
	}
	CGContextFillPath(quartz);
}


void
FramePoly(void * inShape)
{
	Point * p = ((PolygonShape *)inShape)->points;
	int i, count = (((PolygonShape *)inShape)->size - 12) / sizeof(Point);
	CGContextBeginPath(quartz);
	CGContextMoveToPoint(quartz, p->h, gScreenHeight - p->v);
	for (i = 1, p++; i < count; i++, p++)
	{
		CGContextAddLineToPoint(quartz, p->h, gScreenHeight - p->v);
	}
	CGContextStrokePath(quartz);
}

