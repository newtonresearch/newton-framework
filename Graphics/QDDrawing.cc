/*
	File:		QDDrawing.cc

	Contains:	QD implementation using Quartz Core Graphics interface for Newton.

	Written by:	Newton Research Group.
*/

#include "Quartz.h"
#include "QDPatterns.h"
#include "QDDrawing.h"

#include "Objects.h"
#include "ViewFlags.h"
#include "ROMSymbols.h"

extern void	InitScreen(void);
extern void	InitQDCompression(void);


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/
extern int		gScreenHeight;

CGFloat				gLineWidth;

CGColorRef		gBlackColor;
CGColorRef		gDarkGrayColor;
CGColorRef		gGrayColor;
CGColorRef		gLightGrayColor;
CGColorRef		gWhiteColor;


/*------------------------------------------------------------------------------
	Initialize the graphics system.
	quartz must be already set up
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

void
InitGraf(void)
{
	InitScreen();
	InitQDCompression();

	// draw in RGB color space
	CGColorSpaceRef	rgbSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);

	// set up common colors
	const CGFloat	whiteRGBA[4] = { 1.0, 1.0, 1.0, 1.0 };
	const CGFloat	ltGrayRGBA[4] = { 0.75, 0.75, 0.75, 1.0 };
	const CGFloat	grayRGBA[4] = { 0.5, 0.5, 0.5, 1.0 };
	const CGFloat	dkGrayRGBA[4] = { 0.25, 0.25, 0.25, 1.0 };
	const CGFloat	blackRGBA[4] = { 0.0, 0.0, 0.0, 1.0 };

	gBlackColor = CGColorCreate(rgbSpace, blackRGBA);
	gDarkGrayColor = CGColorCreate(rgbSpace, dkGrayRGBA);
	gGrayColor = CGColorCreate(rgbSpace, grayRGBA);
	gLightGrayColor = CGColorCreate(rgbSpace, ltGrayRGBA);
	gWhiteColor = CGColorCreate(rgbSpace, whiteRGBA);

	CGColorSpaceRelease(rgbSpace);
}


#pragma mark -
/*------------------------------------------------------------------------------
	U t i l i t i e s
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Make a CGRect from a Newton Rect, flipping the vertical axis.
	Args:		inRect
	Return:	CGRect
------------------------------------------------------------------------------*/

CGPoint
MakeCGPoint(Point inPt)
{
	CGPoint pt;
	pt.x = inPt.h;
	pt.y = gScreenHeight - inPt.v;
	return pt;
}


CGRect
MakeCGRect(Rect inRect)
{
	CGRect  r;
	r.origin.x = inRect.left;
	r.origin.y = gScreenHeight - inRect.bottom;
	r.size.width = RectGetWidth(inRect);
	r.size.height = RectGetHeight(inRect);
	return r;
}


CGRect
MakeCGRectFrame(const Rect * inRect, float inLineWd)
{
	CGRect r = MakeCGRect(*inRect);
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

	rectSize.h = RectGetWidth(*ioRect);
	rectSize.v = RectGetHeight(*ioRect);

	if (RectGetWidth(frame) == 0
	&&  RectGetHeight(frame) == 0)
	{
		frame.right = frame.left + RectGetWidth(*ioRect);
		frame.bottom = frame.top + RectGetHeight(*ioRect);
	}

	switch (inJustify & vjVMask)
	{
	case vjTopV:
	// no need to do anything
		break;
	case vjCenterV:
		offset = (RectGetHeight(frame) - rectSize.v) / 2;
		if (offset > 0)
			rectOrigin.v += offset;
		break;
	case vjBottomV:
		offset = RectGetHeight(frame) - rectSize.v;
		rectOrigin.v += offset;
		break;
	case vjFullV:
		rectSize.v = RectGetHeight(frame);
		break;
	}

	switch (inJustify & vjHMask)
	{
	case vjLeftH:
	// no need to do anything
		break;
	case vjRightH:
		offset = RectGetWidth(frame) - rectSize.h;
		rectOrigin.h += offset;
		break;
	case vjCenterH:
		offset = (RectGetWidth(frame) - rectSize.h) / 2;
		if (offset > 0)
			rectOrigin.h += offset;
		break;
	case vjFullH:
		rectSize.h = RectGetWidth(frame);
		break;
	}

	SetRect(ioRect, rectOrigin.h, rectOrigin.v, rectOrigin.h + rectSize.h, rectOrigin.v + rectSize.v);
	return ioRect;
}


#pragma mark -

/*------------------------------------------------------------------------------
	D r a w i n g
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	L i n e
------------------------------------------------------------------------------*/

void
LineNormal(void)
{
	SetLineWidth(1);
	CGContextSetStrokeColorWithColor(quartz, gBlackColor);
}


void
SetLineWidth(short inWidth)
{
	gLineWidth = (float) inWidth;
	CGContextSetLineWidth(quartz, gLineWidth);
}


short
LineWidth(void)
{
	return (short) gLineWidth;
}


void
StrokeLine(Point inFrom, Point inTo)
{
	CGPoint pt;
	CGContextBeginPath(quartz);
	pt = MakeCGPoint(inFrom);
	CGContextMoveToPoint(quartz, pt.x, pt.y);
	pt = MakeCGPoint(inTo);
	CGContextAddLineToPoint(quartz, pt.x, pt.y);
	CGContextStrokePath(quartz);
}


/*------------------------------------------------------------------------------
	R e c t
------------------------------------------------------------------------------*/

void
StrokeRect(const Rect * inBox)
{
	StrokeRect(*inBox);
}

void
StrokeRect(Rect inBox)
{
	CGFloat  inset = gLineWidth / 2.0;
	CGRect r = MakeCGRect(inBox);
	CGContextStrokeRect(quartz, CGRectInset(r, inset, inset));
}


void
FillRect(const Rect * inBox)
{
	FillRect(*inBox);
}

void
FillRect(Rect inBox)
{
	CGContextFillRect(quartz, MakeCGRect(inBox));
}


/*------------------------------------------------------------------------------
	R o u n d R e c t
------------------------------------------------------------------------------*/
static void		AddRoundedRectToPath(CGContextRef inContext, CGRect inRect, CGFloat ovalWidth, CGFloat ovalHeight);

void
StrokeRoundRect(Rect inBox, short inOvalWd, short inOvalHt)
{
	CGFloat  inset = gLineWidth / 2.0;
	CGRect r = MakeCGRect(inBox);
	CGContextBeginPath(quartz);
	AddRoundedRectToPath(quartz, CGRectInset(r, inset, inset), (CGFloat) inOvalWd, (CGFloat) inOvalHt);
	CGContextStrokePath(quartz);
}


void
FillRoundRect(Rect inBox, short inOvalWd, short inOvalHt)
{
	CGContextBeginPath(quartz);
	AddRoundedRectToPath(quartz, MakeCGRect(inBox), (CGFloat) inOvalWd, (CGFloat) inOvalHt);
	CGContextFillPath(quartz);
}


static void
AddRoundedRectToPath(CGContextRef inContext, CGRect inRect, CGFloat ovalWidth, CGFloat ovalHeight)
{
	if (ovalWidth == 0 || ovalHeight == 0)
		CGContextAddRect(inContext, inRect);

	else
	{
		CGFloat fw, fh;

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


/*------------------------------------------------------------------------------
	O v a l
------------------------------------------------------------------------------*/

void
StrokeOval(const Rect * inBox)
{
	CGFloat  inset = gLineWidth / 2.0;
	CGRect r = MakeCGRect(*inBox);
	CGContextStrokeEllipseInRect(quartz, CGRectInset(r, inset, inset));
}


void
FillOval(const Rect * inBox)
{
	CGContextFillEllipseInRect(quartz, MakeCGRect(*inBox));
}


/*------------------------------------------------------------------------------
	P o l y g o n
------------------------------------------------------------------------------*/

// hack for NCX
// inData is a PolygonShape
void
DrawPolygon(char * inData, short inX, short inY)
{
	Point pt;
	short sh, * p = (short *)inData;
	sh = *p++;	// unused
	sh = *p++;
	ArrayIndex count = CANONICAL_SHORT(sh);

	if (count < 2)
		return;

	CGContextSetLineWidth(quartz, 2);
	CGContextBeginPath(quartz);

	sh = *p++;
	pt.v = CANONICAL_SHORT(sh);
	sh = *p++;
	pt.h = CANONICAL_SHORT(sh);
	CGContextMoveToPoint(quartz, inX + pt.h, gScreenHeight - (inY + pt.v));

	for (ArrayIndex i = 1; i < count; ++i)
	{
		sh = *p++;
		pt.v = CANONICAL_SHORT(sh);
		sh = *p++;
		pt.h = CANONICAL_SHORT(sh);
		CGContextAddLineToPoint(quartz, inX + pt.h, gScreenHeight - (inY + pt.v));
	}
	CGContextStrokePath(quartz);
}


void
KillPoly(PolygonShape * inShape)
{
	free(inShape);
}


void
OffsetPoly(PolygonShape * inShape, short dx, short dy)
{
	if (dx != 0 && dy != 0)
	{
		Point * p = inShape->points;
		for (ArrayIndex i = 0, count = (inShape->size - 12) / sizeof(Point); i < count; ++i, ++p)
		{
			p->h += dx;
			p->v += dy;
		}
	}
}


void
StrokePoly(PolygonShape * inShape)
{
	Point * p = inShape->points;
	int i, count = (inShape->size - 12) / sizeof(Point);
	CGContextBeginPath(quartz);
	CGContextMoveToPoint(quartz, p->h, gScreenHeight - p->v);
	for (i = 1, p++; i < count; i++, p++)
	{
		CGContextAddLineToPoint(quartz, p->h, gScreenHeight - p->v);
	}
	CGContextStrokePath(quartz);
}


void
FillPoly(PolygonShape * inShape)
{
	Point * p = inShape->points;
	int i, count = (inShape->size - 12) / sizeof(Point);
	CGContextBeginPath(quartz);
	CGContextMoveToPoint(quartz, p->h, gScreenHeight - p->v);
	for (i = 1, p++; i < count; i++, p++)
	{
		CGContextAddLineToPoint(quartz, p->h, gScreenHeight - p->v);
	}
	CGContextFillPath(quartz);
}


/*------------------------------------------------------------------------------
	R e g i o n
------------------------------------------------------------------------------*/

void
OffsetRgn(RegionShape * inShape, short dx, short dy)
{}

void
StrokeRgn(RegionShape * inShape)
{}

void
FillRgn(RegionShape * inShape)
{}


/*------------------------------------------------------------------------------
	A r c
------------------------------------------------------------------------------*/

void
StrokeArc(WedgeShape * inShape, short inArc1, short inArc2)
{}

void
FillArc(WedgeShape * inShape, short inArc1, short inArc2)
{}

#pragma mark -

/*------------------------------------------------------------------------------
	Return a pointer to the bitmap in a PixelMap.
	Args:		pixmap		the PixelMap
	Return:  pointer
------------------------------------------------------------------------------*/

Ptr
GetPixelMapBits(PixelMap * pixmap)
{
	Ptr	pixBits = pixmap->baseAddr;
	ULong storage = pixmap->pixMapFlags & kPixMapStorage;
	if (storage == kPixMapOffset)
		return pixBits + (OpaqueRef) pixmap;
	else if (storage == kPixMapHandle)
		return *(Handle) pixBits;
	else if (storage == kPixMapPtr)
		return pixBits;
	return NULL;
}


#pragma mark -

/*------------------------------------------------------------------------------
	C o l o u r s
------------------------------------------------------------------------------*/

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

/*------------------------------------------------------------------------------
	P a t t e r n s
------------------------------------------------------------------------------*/

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
	Set the pattern for drawing a view’s frame.
	Used only by View.cc.
	Args:		inPatNo
	Return:	YES	if a standard pattern was set
				NO		if a custom pattern was specified
------------------------------------------------------------------------------*/

bool
SetPattern(int inPatNo)
{
	bool			isSet = YES;
	CGColorRef	ptn;

	switch (inPatNo)
	{
	case vfWhite:
		ptn = gWhiteColor;
		break;
	case vfLtGray:
		ptn = gLightGrayColor;
		break;
	case vfGray:
		ptn = gGrayColor;
		break;
	case vfDkGray:
		ptn = gDarkGrayColor;
		break;
	case vfBlack:
		ptn = gBlackColor;
		break;
	case 12:
		ptn = gBlackColor;
		break;
	case vfDragger:
	case vfMatte:
		ptn = gGrayColor;
		break;
	case vfCustom:
		break;
	default:
		isSet = NO;
	}
	if (isSet)
	{
		CGContextSetStrokeColorWithColor(quartz, ptn);
		CGContextSetFillColorWithColor(quartz, ptn);
	}

	return isSet;
}


CGColorRef
GetStdPattern(int inPatNo)
{
	CGColorRef	ptn;

	if (inPatNo > blackPat)
		inPatNo = blackPat;
	switch (inPatNo)
	{
	case vfWhite:
		ptn = gWhiteColor;
		break;
	case vfLtGray:
		ptn = gLightGrayColor;
		break;
	case vfGray:
		ptn = gGrayColor;
		break;
	case vfDkGray:
		ptn = gDarkGrayColor;
		break;
	case vfBlack:
		ptn = gBlackColor;
		break;
	}

	return ptn;
}


CGColorRef
GetStdGrayPattern(ULong inR, ULong inG, ULong inB)
{
#if 0
	GrafPtr			thePort = GetCurrentPort();
	PixelMap *		pix = IsPrinterPort(thePort) ? &gScreenPixelMap : &thePort->portBits;
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
	CGFloat colorComponents[4] = { inR, inG, inB, 1.0 };
	thePattern = CGColorCreate(rgbSpace, colorComponents);
	CGColorSpaceRelease(rgbSpace);
#endif
	return thePattern;
}


bool
GetPattern(RefArg inPatNo, bool * ioTakeOwnership, CGColorRef * ioPat, bool inDefault)
{
	bool	gotPattern = inDefault;

	if (ISINT(inPatNo))
	{
		int patNo = RINT(inPatNo);
		bool isOpaque = (patNo > 0);
		if (isOpaque)
		{
			if (*ioTakeOwnership)
				CGColorRelease(*ioPat);
			if ((patNo & 0x10000000) != 0)	// it’s not actually an index but an RGB value
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

