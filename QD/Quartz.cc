/*
	File:		Quartz.cc

	Contains:	Quartz core graphics interface for Newton.

	Written by:	The Newton OS group.
*/

#include "Quartz.h"

#include "Globals.h"
#include "Iterators.h"
#include "Lookup.h"
#include "View.h"

extern long		gScreenHeight;

CGContextRef	quartz;

const Rect	gZeroRect = { 0, 0, 0, 0 };


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FShapeBounds(RefArg inRcvr, RefArg inShape);
Ref	FPictureBounds(RefArg inRcvr, RefArg inPicture);
}


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


void
FillRoundedRect(CGContextRef inContext, CGRect inRect, float inOvalWidth)
{
	CGContextBeginPath(inContext);
	AddRoundedRectToPath(inContext, inRect, inOvalWidth, inOvalWidth);
	CGContextFillPath(inContext);
}


void
StrokeRoundedRect(CGContextRef inContext, CGRect inRect, float inOvalWidth)
{
	CGContextBeginPath(inContext);
	AddRoundedRectToPath(inContext, inRect, inOvalWidth, inOvalWidth);
	CGContextStrokePath(inContext);
}

#pragma mark -

/*------------------------------------------------------------------------------
	C P i x e l O b j

	A class to convert 1-bit bitmaps to pixel maps.
------------------------------------------------------------------------------*/

typedef struct
{
	long  baseAddr;		// +00
	short rowBytes;		// +04		// pads to long
	short reserved1;
	Rect  bounds;			// +08
	char  data[];			// +10
} FramBitmap;

class CPixelObj
{
public:
						CPixelObj();
						~CPixelObj();

	void				init(RefArg inBitmap);
	void				init(RefArg inBitmap, BOOL);
	Ref				getFramBitmap(void);
	PixelMap *		framBitmapToPixMap(const FramBitmap * inFramBits);

//private:
	RefVar		fBitmapRef; // +00
	PixelMap		fPixmap;		// +04
	PixelMapPtr fPixPtr;		// +20
	PixelMapPtr fMaskPtr;	// +24
	UChar *		fColorTable;// +28
	long			fBitDepth;  // +2C
	BOOL			fIsLocked;  // +30
};


CPixelObj::CPixelObj()
{
	fBitDepth = kOneBitDepth;
	fColorTable = NULL;
	fIsLocked = NO;
	fPixPtr = &fPixmap;
}


CPixelObj::~CPixelObj()
{
	if (fColorTable)
		DisposePtr((Ptr)fColorTable);
	if (fIsLocked)
		UnlockRef(fBitmapRef);
}


void
CPixelObj::init(RefArg inBitmap)
{
	fBitmapRef = inBitmap;
	if (IsInstance(inBitmap, SYMpicture))
		throw2(exGraf, -8803);

	if (IsFrame(inBitmap))
	{
		if (IsInstance(inBitmap, SYMbitmap))
		{
			RefVar	colorData(GetFrameSlot(inBitmap, RSYM(colorData)));
			if (ISNIL(colorData))
				fBitmapRef = GetFrameSlot(inBitmap, RSYM(data));
			else
				fBitmapRef = getFramBitmap();
		}
		else
			fBitmapRef = getFramBitmap();
	}

	LockRef(fBitmapRef);
	fIsLocked = YES;
	if (IsInstance(fBitmapRef, SYMpixels))
		fPixPtr = (PixelMap *) BinaryData(fBitmapRef);
	else
		fPixPtr = framBitmapToPixMap((const FramBitmap *) BinaryData(fBitmapRef));
}


void
CPixelObj::init(RefArg inBitmap, BOOL inHuh)
{
	BOOL  isPixelData = NO;
	fBitmapRef = inBitmap;
	fMaskPtr = NULL;
	if (IsInstance(inBitmap, SYMpicture))
		throw2(exGraf, -8803);

	RefVar	bits(GetFrameSlot(inBitmap, SYMdata));
	if (NOTNIL(bits))
		isPixelData = IsInstance(bits, SYMpixels);
	LockRef(fBitmapRef);
	fIsLocked = YES;
	if (isPixelData)
		fPixPtr = (PixelMap *) BinaryData(fBitmapRef);
	else
	{
		bits = GetFrameSlot(inBitmap, SYMmask);
		if (NOTNIL(bits))
		{
			CObjectPtr  obj(bits);
			fMaskPtr = framBitmapToPixMap((const FramBitmap *) (char *) obj);
		}
		if (inHuh || fMaskPtr == NULL)
			fPixPtr = framBitmapToPixMap((const FramBitmap *) BinaryData(getFramBitmap()));
	}
}


Ref
CPixelObj::getFramBitmap(void)
{
	RefVar	framBitmap;
	RefVar	colorTable;
	RefVar	colorData(GetFrameSlot(fBitmapRef, SYMcolorData));
	if (ISNIL(colorData))
		framBitmap = GetFrameSlot(fBitmapRef, RSYM(bits));
	else
	{
		GrafPtr  thePort;
		GetPort(&thePort);
		long  portDepth = 8; // for CG grayscale; PixelDepth(&thePort->portBits);
		if (IsArray(colorData))
		{
			long  maxDepth = 32767; // improbably large number
			long  minDepth = 0;
			CObjectIterator	iter(colorData);
			for ( ; !iter.done(); iter.next())
			{
				long  bitDepth = RINT(GetFrameSlot(iter.value(), SYMbitDepth));
				if (bitDepth == portDepth)
				{
					fBitDepth = bitDepth;
					framBitmap = GetFrameSlot(iter.value(), SYMcBits);
					colorTable = GetFrameSlot(iter.value(), SYMcolorTable);
				}
				else if ((portDepth < bitDepth && bitDepth < maxDepth && (maxDepth = bitDepth, 1))
						|| (maxDepth == 32767 && portDepth > bitDepth && bitDepth > minDepth && (minDepth = bitDepth, 1)))
				{
					fBitDepth = bitDepth;
					framBitmap = GetFrameSlot(iter.value(), SYMcBits);
					colorTable = GetFrameSlot(iter.value(), SYMcolorTable);
				}
			}
		}
		else
		{
			fBitDepth = RINT(GetFrameSlot(colorData, SYMbitDepth));
			framBitmap = GetFrameSlot(colorData, SYMcBits);
			colorTable = GetFrameSlot(colorData, SYMcolorTable);
		}

		if (fBitDepth > 1 && NOTNIL(colorTable))
		{
			CObjectPtr  obj(colorTable);
			long			numOfColors = Length(colorTable);	// r7
			size_t		colorTableSize = 1 << fBitDepth;		// r6
			fColorTable = (UChar *) NewPtr(colorTableSize);
			if (fColorTable)
			{
/*				// INCOMPLETE
				for (int i = 0; i < numOfColors; i++)
					fColorTable[i] = RGBtoGray(, fBitDepth, fBitDepth);
*/			}
		}
	}

	return framBitmap;
}


PixelMap *
CPixelObj::framBitmapToPixMap(const FramBitmap * inBitmap)
{
	fPixPtr->baseAddr = (Ptr) inBitmap->data;
	fPixPtr->rowBytes = inBitmap->rowBytes;
	fPixPtr->bounds = inBitmap->bounds;
	fPixPtr->pixMapFlags = kPixMapPtr + fBitDepth;
	fPixPtr->deviceRes.h =
	fPixPtr->deviceRes.v = kDefaultDPI;
	fPixPtr->grayTable = fColorTable;
	if (fColorTable)
		fPixPtr->pixMapFlags |= kPixMapGrayTable;

	return fPixPtr;
}


#pragma mark -

/*------------------------------------------------------------------------------
	D r a w i n g
------------------------------------------------------------------------------*/

BOOL
IsStyleFrame(RefArg inObj)
{
	return EQRef(ClassOf(inObj), SYMframe);
}

#if 0
struct SaveLevel
{
	SaveLevel *  init(SaveLevel * inLevel);

	SaveLevel * f00;
	long			f04;
	long			f08;
	long			f0C;
};

SaveLevel *
SaveLevel::init(SaveLevel * inLevel)
{
	f00 = inLevel;
	f04 = 0;
	f08 = 0;
	f0C = 0;
	return this;
}

class CPattern
{
public:
						CPattern();
						~CPattern();

	BOOL				getFillPattern(RefArg inPat, BOOL);

private:
	PatternHandle  fPattern;		// +00
	BOOL				fGotPattern;	// +04
	BOOL				f05;				// +05
};

inline
CPattern::CPattern()
{  fPattern = NULL; fGotPattern = NO; f05 = YES; }

CPattern::~CPattern()
{
	if (fGotPattern)
	{
		if (f05 && fPattern == GetFgPattern())
			SetFgPattern(GetStdPattern(blackPat));
		DisposePattern(fPattern);
	}
}

BOOL
CPattern::getFillPattern(RefArg inPatNo, BOOL inHuh)
{
	BOOL  result = inHuh;
	if (NOTNIL(inPatNo))
		result = GetPattern(inPatNo, &fGotPattern, &fPattern, inHuh);
	return result;
}

#pragma mark -

typedef struct
{
	RefStruct	f00;
	long			f04;
} StyleCache;

class CSaveStyle
{
public:
						CSaveStyle();
						~CSaveStyle();

	BOOL				setStyle(RefArg inStyle, Point inPt, long inHuh);
	void				beginLevel(SaveLevel * inLevel);
	void				endLevel(void);
	StyleCache *	lookupCache(void);

private:
	long			f04;

	CPattern		f0C;
	CPattern		f14;
	CPattern		f1C;

	RefStruct	f2C;
	SaveLevel * f30;
	SaveLevel	f34;
	long			f44;
	long			f48;
	GrafPtr		f4C;
	RefStruct	f50;
	StyleCache  f54[3];
	long			f6C;
// size +70
};

CSaveStyle::CSaveStyle()
{
	GetPort(&f4C);
	f50 = MAKEINT(-1);
	f34.init(NULL);
	f30 = &f34;
	f44 = 0;
	f48 = 0;
	f04 = 0;
	f6C = 0;
}

CSaveStyle::~CSaveStyle()
{
	endLevel();
}

BOOL
CSaveStyle::setStyle(RefArg inStyle, Point inPt, long inHuh)
{
	if (EQ(inStyle, f50))
		return YES;

// ITÕS A WHOPPER
}

void
CSaveStyle::beginLevel(SaveLevel * inLevel)
{
	f30 = inLevel->init(f30);
}

void
CSaveStyle::endLevel(void)
{
#if 0
	if (f30->f0C & 0x01)
	{
		CQDScaler::stopScaling();
		f48--;
	}
	if (f30->f0C & 0x02)
	{
		CQDScaler::replaceClip(f30->f04, NULL, 0);
		DisposeRgn(f30->f04);
		f30 = f30->f00;	// next?
	}
#endif
}

StyleCache *
CSaveStyle::lookupCache(void)
{
	for (int i = 0; i < 3; i++)
	{
		if (EQ(f50, f54[i].f00))
			return &f54[i];
	}

	StyleCache * result = &f54[f6C];
	if (++f6C >= 3)
		f6C = 0;
	result->f00 = f50;
	result->f04 = -1;
	return result;
}


void
MakeSimpleStyle(RefArg inFamily, long inSize, long inFace)
{
#if 0
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
#endif
}


typedef struct
{
	PixelMap pix;
	char		pat[8];
} SimplePattern;

PatternHandle
MakeSimplePattern(long inBits0, long inBits1, long inBits2, long inBits3, long inBits4, long inBits5, long inBits6, long inBits7)
{
	PatternHandle  pat = (PatternHandle) NewHandle(sizeof(SimplePattern));
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

PatternHandle
MakeSimplePattern(const char * inBits)
{
	PatternHandle  pat = (PatternHandle) NewHandle(sizeof(SimplePattern));
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
		BlockMove(inBits, &((SimplePattern*)pixmap)->pat, 8);
	}
	return pat;
}

#else
typedef void CSaveStyle;
#endif

#pragma mark -

/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

void  DrawPicture(char * inPicture, size_t inPictureSize, CGRect inRect);
void  DrawBitmap(RefArg inBitmap, CGRect inRect);
void  DrawShape(RefArg inShape, RefArg inStyle, CGPoint inPt);
void  DrawShapeList(RefArg inShapes, CSaveStyle * inStyle, CGPoint inPt);
void  DrawOneShape(RefArg inShape, CSaveStyle * inStyle, CGPoint inPt);


/*------------------------------------------------------------------------------
	Draw a picture.
	Determine what kind of data is inIcon -
		PNG data
		Newton bitmap
		Newton shape
	Args:		inIcon
				inFrame
				inJustify
	Return:	--
------------------------------------------------------------------------------*/

// Read image size from PNG data.

typedef struct
{
	ULong		length;
	char		id[4];
	ULong		width;
	ULong		height;
	UChar		bitDepth;
	UChar		colorType;
	UChar		compressionMethod;
	UChar		filterMethod;
	UChar		interlaceMethod;
} PNGchunkIHDR;

ULong
PNGgetULong(const char * inData)
{
   return	((ULong)(*inData) << 24)
			 +	((ULong)(*(inData + 1)) << 16)
			 +	((ULong)(*(inData + 2)) << 8)
			 +	(ULong)(*(inData + 3));
}

Rect *
PictureBounds(const char * inData, Rect * outRect)
{
	if (memcmp(inData, "\x89PNG", 4) != 0)
		ThrowMsg("picture is not PNG");

	ULong	chunkLength;
	for (inData += 8; (chunkLength = PNGgetULong(inData)) != 0; inData += chunkLength)
	{
		if (memcmp(inData+4, "IHDR", 4) == 0)
		{
			if (chunkLength != 13)
				ThrowMsg("invalid PNG IHDR chunk");

			outRect->left = 0;
			outRect->top = 0;
			outRect->right = PNGgetULong(inData+8);
			outRect->bottom = PNGgetULong(inData+12);
			break;
		}
	}

	return outRect;
}


Ref
FPictureBounds(RefArg inRcvr, RefArg inPicture)
{
	Rect		bounds;
	CDataPtr	picData(inPicture);
	return ToObject(PictureBounds(picData, &bounds));
}


BOOL
IsPrimShape(RefArg inShape)
{
	if (ISPTR(inShape))
	{
		RefVar	shapeClass(ClassOf(inShape));
		return EQRef(shapeClass, SYMrectangle)
			 || EQRef(shapeClass, SYMline)
			 || EQRef(shapeClass, SYMtextBox)
			 || EQRef(shapeClass, SYMink)
			 || EQRef(shapeClass, SYMroundRectangle)
			 || EQRef(shapeClass, SYMoval)
			 || EQRef(shapeClass, SYMbitmap)
			 ||(EQRef(shapeClass, SYMpicture) && IsFrame(inShape))
			 || EQRef(shapeClass, SYMpolygon)
			 || EQRef(shapeClass, SYMwedge)
			 || EQRef(shapeClass, SYMregion)
			 || EQRef(shapeClass, SYMtext);
	}
	return NO;
}


Rect *
ShapeBounds(RefArg inShape, Rect * outRect)
{
	RefVar	shapeClass(ClassOf(inShape));
	if (IsArray(inShape))
	{
		Rect  shapeRect;
		*outRect = gZeroRect;
		CObjectIterator	iter(inShape);
		for ( ; !iter.done(); iter.next())
		{
			if (NOTNIL(iter.value()) && !IsStyleFrame(iter.value()))
				UnionRect(ShapeBounds(iter.value(), &shapeRect), outRect, outRect);
		}
	}
	else
	{
//L64
		if (EQRef(shapeClass, SYMregion))
			;
//L92
		else if (EQRef(shapeClass, SYMink))
			;
//L125
		else if (EQRef(shapeClass, SYMpolygon))
			;
//L165
		else if (IsPrimShape(inShape))
		{
			RefVar	boundsFrame;
//L175
			if (EQRef(shapeClass, SYMbitmap)
			||  EQRef(shapeClass, SYMpicture)
			||  EQRef(shapeClass, SYMtext))
				boundsFrame = GetProtoVariable(inShape, RSYM(bounds));
			else
				boundsFrame = inShape;
//L222
			CDataPtr data(boundsFrame);
			*outRect = *(Rect *) (char *) data;

			// fix up line bounds so itÕs always a non-empty rect
			if ( EQRef(shapeClass, SYMline))
			{
				short swap;
				if (outRect->bottom == outRect->top)
					outRect->bottom++;
				else if (outRect->bottom < outRect->top)
				{
					swap = outRect->top;
					outRect->top = outRect->bottom;
					outRect->bottom = swap;
				}
				if (outRect->right == outRect->left)
					outRect->right++;
				else if (outRect->right < outRect->left)
				{
					swap = outRect->left;
					outRect->left = outRect->right;
					outRect->right = swap;
				}
			}
		}
		else
			throw2(exGraf, -8804);
	}

	return outRect;
}


Ref
FShapeBounds(RefArg inRcvr, RefArg inShape)
{
	Rect  bounds;
	return ToObject(ShapeBounds(inShape, &bounds));
}


void
DrawPicture(RefArg inIcon, const Rect * inFrame, ULong inJustify /*, long inTransferMode*/)
{
	Rect  iconRect;

	if (IsBinary(inIcon) && EQRef(ClassOf(inIcon), SYMpicture))
	{
		CDataPtr	iconData(inIcon);
		DrawPicture(iconData, Length(inIcon), Justify(PictureBounds(iconData, &iconRect), inFrame, inJustify));
	}

	else if (IsFrame(inIcon) && !IsInstance(inIcon, SYMbitmap)
			&& (FrameHasSlot(inIcon, SYMbits) || FrameHasSlot(inIcon, SYMcolorData)))
	{
		RefVar	iconBounds(GetFrameSlot(inIcon, SYMbounds));
		if (!FromObject(iconBounds, &iconRect))
			ThrowMsg("bad pictBounds frame");
		DrawBitmap(inIcon, Justify(&iconRect, inFrame, inJustify));
	}

	else
	{
		CGRect	shapeRect;
		if (inJustify)
			shapeRect = Justify(ShapeBounds(inIcon, &iconRect), inFrame, inJustify);
		else
			shapeRect = CGRectMake(inFrame->left, inFrame->bottom, RectGetWidth(inFrame), RectGetHeight(inFrame));
		RefVar	style(AllocateFrame());
		//Ref		mode = MAKEINT(inTransferMode);
		//SetFrameSlot(style, RSYM(transferMode), RARG(mode));
		DrawShape(inIcon, style, shapeRect.origin);
	}
}


/*------------------------------------------------------------------------------
	Draw a PNG picture.
	Pictures are always PNGs in Newton 3.0.
	Args:		inPicture
				inPictureSize
				inRect
	Return:	--
------------------------------------------------------------------------------*/

void
DrawPicture(char * inPicture, size_t inPictureSize, CGRect inRect)
{
	CGDataProviderRef source = CGDataProviderCreateWithData(NULL, inPicture, inPictureSize, NULL);
	CGImageRef  image = CGImageCreateWithPNGDataProvider(source, NULL, true, kCGRenderingIntentDefault);
	CGContextDrawImage(quartz, inRect, image);
	CGImageRelease(image);
}


/*------------------------------------------------------------------------------
	Draw a bitmap.
	Args:		inBitmap
				inRect
	Return:	--
------------------------------------------------------------------------------*/

const unsigned char colorTable1bit[] = { 0xFF, 0x00 };
const unsigned char colorTable2bit[] = { 0xFF, 0xAA, 0x55, 0x00 };
const unsigned char colorTable4bit[] = { 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88,
													  0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00 };
const unsigned char * colorTable[5] = { NULL, colorTable1bit, colorTable2bit, NULL, colorTable4bit };
const size_t lastIndex[5] = { 0, 1, 3, 0, 15 };

void
DrawBitmap(RefArg inBitmap, CGRect inRect)
{
	CPixelObj	pix;
	newton_try
	{
		pix.init(inBitmap);

		size_t	pixmapWidth = pix.fPixPtr->bounds.right - pix.fPixPtr->bounds.left;
		size_t	pixmapHeight = pix.fPixPtr->bounds.bottom - pix.fPixPtr->bounds.top;

		if (inRect.size.width == 0.0)
		{
			inRect.size.width = pixmapWidth;
			inRect.size.height = pixmapHeight;
		}

		CGImageRef			image = NULL;
//		CGColorSpaceRef	colorSpace = CGColorSpaceCreateDeviceGray();
		CGColorSpaceRef	colorSpace = CGColorSpaceCreateIndexed(CGColorSpaceCreateDeviceGray(), lastIndex[pix.fBitDepth], colorTable[pix.fBitDepth]);
		CGDataProviderRef source = CGDataProviderCreateWithData(NULL, pix.fPixPtr->baseAddr, pixmapHeight * pix.fPixPtr->rowBytes, NULL);
		if (source != NULL)
		{
			image = CGImageCreate(pixmapWidth, pixmapHeight, pix.fBitDepth, pix.fBitDepth, pix.fPixPtr->rowBytes,
										colorSpace, kCGImageAlphaNone, source,
										NULL, true, kCGRenderingIntentDefault);
			CGDataProviderRelease(source);
		}
		CGColorSpaceRelease(colorSpace);

		CGContextDrawImage(quartz, inRect, image);
		CGImageRelease(image);
	}
	newton_catch_all
	{
		pix.~CPixelObj();
	}
	end_try;
}


/*------------------------------------------------------------------------------
	Draw a shape.
	Args:		inShape
				inStyle
				inPt
	Return:	--
------------------------------------------------------------------------------*/

void
DrawShape(RefArg inShape, RefArg inStyle, CGPoint inPt)
{
	CSaveStyle  theStyle;
	CGContextSaveGState();
	if (theStyle.setStyle(inStyle, inPt, 0))
	{
		SaveLevel level;
		theStyle.beginLevel(&level);
		unwind_protect
		{
			DrawShapeList(inShape, &theStyle, inPt);
		}
		on_unwind
		{
			theStyle.endLevel();
			CGContextRestoreGState();
		}
		end_unwind;
	}
}


/*------------------------------------------------------------------------------
	Draw a shape list.
	Args:		inShapes
				inStyle
				inPt
	Return:	--
------------------------------------------------------------------------------*/

void
DrawShapeList(RefArg inShapes, CSaveStyle * inStyle, CGPoint inPt)
{
	if (IsArray(inShapes))
	{
		BOOL  hasNoStyle = NO;
		CObjectIterator	iter(inShapes);
		for ( ; !iter.done(); iter.next())
		{
			if (IsStyleFrame(iter.value()) || ISNIL(iter.value()))
				hasNoStyle = !inStyle.setStyle(iter.value(), inPt, 0);
			else if (!hasNoStyle)
			{
				if (IsArray(shape))
				{
					RefVar		oldStyle(inStyle->f50);
					SaveLevel	level;
					inStyle->beginLevel(&level);
					unwind_protect
					{
						DrawShapeList(shape, inStyle, inPt);
						inStyle->setStyle(oldStyle, inPt, 1);
					}
					on_unwind
					{
						inStyle->endLevel();
					}
					end_unwind;
				}
				else
					DrawOneShape(shape, inStyle, inPt);
			}
		}
	}
	else
		DrawOneShape(inShapes, inStyle, inPt);
}


/*------------------------------------------------------------------------------
	Draw a shape.
	Args:		inShape
				inStyle
				inPt
	Return:	--
------------------------------------------------------------------------------*/

void
DrawOneShape(RefArg inShape, CSaveStyle * inStyle, CGPoint inPt)
{
//	r7 = inShape
// r4 = inStyle
//	r5 = inPt

	//sp-08
	FuncPtr sp = OffsetRect;

	if (inStyle->f01 && inStyle->f0C == NULL)
		SetFgPattern(GetStdPattern(blackPat));

	//sp-10
	Rect		bounds;  // sp00
	RefVar	sp0C(inShape);
	long		sp08 = 0;
	Ref		shapeClass(ClassOf(inShape));  // r6
	ShapeBounds(inShape, &bounds);

	if (inStyle->f48 == 0)
		OffsetRect(&bounds, inPt.h, inPt.v);
//L45
	//sp-08
	if (inStyle->f44 != 0)
	{
		// trivial rejection
		GrafPtr  thePort;
		GetPort(&thePort);
		if (!Intersects(bounds, thePort->clipRgn.rgnBBox))
			return;
	}
//L69
	r10 = &inStyle->f14
	if (EQRef(shapeClass, SYMrectangle))
	{
		r9 = PaintRect
		r1 = FrameRect
//L422
		sp1C = r1
		if (r9)
		{
			sp-14
			CDataPtr sp10(sp28);
			char * r7 = sp10; // via operator
			r8 = 0;
			if ()
			{
			}
//L862
			if (sp24 && r8 == NULL)
				r7 = r8 = NewFakeHandle(r7, Length(sp28));
			newton_try
			{
				if (inStyle->f48 == 0)
					sp98(r7, inPt.h, inPt.v);
//L893
				if (inStyle->f01)
					r9(r7);
				if (inStyle->f00)
				{
					r0 = *r10;
					if (r0 == 0)
						r0 = GetStdPattern(blackPat);
					SetFgPattern(r0);
					sp9C(r7);
				}
//L909
				if (inStyle->f48 == 0)
					sp98(r7, -inPt.h, -inPt.v);
//L921
				if (r8)
					DisposeHandle(r8);
			}
			cleanup
			{
				sp7C.~CDataPtr();
			}
			end_try;
		}
//L931
		
	}
	else if (EQRef(shapeClass, SYMline))
	{
		;
	}
//L141
	else if (EQRef(shapeClass, SYMink))
	{
		;
	}
//L244
	else if (EQRef(shapeClass, SYMroundRectangle))
	{
		;
	}
//L302
	else if (EQRef(shapeClass, SYMoval))
	{
//L422
	}
	else if (EQRef(shapeClass, SYMwedge))
	{
		;
	}
//L379
	else if (EQRef(shapeClass, SYMregion))
	{
		;
	}
//L403
	else if (EQRef(shapeClass, SYMpolygon))
	{
		;
	}
//L458
	else if (EQRef(shapeClass, SYMpicture))
	{
		;
	}
//L560
	else if (EQRef(shapeClass, SYMbitmap))
	{
		;
	}
//L606
	else if (EQRef(shapeClass, SYMtext))
	{
		;
	}
//L931

}

#pragma mark -

Ref
MakeRectShape(RefArg inClass, RefArg inLeft, RefArg inTop, RefArg inRight, RefArg inBottom)
{
	Rect  bounds;
	bounds.top = RINT(inTop);
	bounds.left = RINT(inLeft);
	bounds.bottom = RINT(inBottom);
	bounds.right = RINT(inRight);

	RefVar	theShape(AllocateBinary(inClass, sizeof(bounds)));
	CDataPtr shapeData(theShape);
	memmove((char *) shapeData, &bounds, sizeof(bounds));

	return theShape;
}


Ref
FMakeRect(RefArg inRcvr, RefArg inLeft, RefArg inTop, RefArg inRight, RefArg inBottom)
{
	return MakeRectShape(RSYM(rectangle), inLeft, inTop, inRight, inBottom);
}


Ref
FMakeLine(RefArg inRcvr, RefArg inX1, RefArg inY1, RefArg inX2, RefArg inY2)
{
	Rect  bounds;
	bounds.top = RINT(inX1);
	bounds.left = RINT(inY1);
	bounds.bottom = RINT(inX2);
	bounds.right = RINT(inY2);

	RefVar	theShape(AllocateBinary(RSYM(line), sizeof(bounds)));
	CDataPtr shapeData(theShape);
	memmove((char *) shapeData, &bounds, sizeof(bounds));

	return theShape;
}


SYS_textShape

Ref
FMakeText(RefArg inRcvr, RefArg inStr, RefArg inLeft, RefArg inTop, RefArg inRight, RefArg inBottom)
{
	RefVar	theShape(Clone(SYS_textShape));

	RefVar	it(MakeRectShape(RSYM(boundsRect), inLeft, inTop, inRight, inBottom));
	SetFrameSlot(theShape, RSYM(bounds), it);

	it = Clone(inStr);
	SetClass(it, RSYM(textData));
	SetFrameSlot(theShape, RSYM(data), it);

	return theShape;
}

#pragma mark -

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
	Args:		inRect
				inFrame
				inJustify
	Return:	justified CGRect
------------------------------------------------------------------------------*/

CGRect
Justify(const Rect * inRect, const Rect * inFrame, ULong inJustify)
{
	Rect  frame = *inFrame;
	Point rectOrigin = *(Point*)inFrame;
	Point rectSize;
	long  offset;

	rectSize.h = RectGetWidth(inRect);
	rectSize.v = RectGetHeight(inRect);

	if (RectGetWidth(&frame) == 0
	&&  RectGetHeight(&frame) == 0)
	{
		frame.right = frame.left + RectGetWidth(inRect);
		frame.bottom = frame.top + RectGetHeight(inRect);
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

	return CGRectMake(rectOrigin.h, gScreenHeight - (rectOrigin.v + rectSize.v), rectSize.h, rectSize.v);
}


BOOL
SetPattern(long inPatNo)
{
	BOOL  isSet = YES;
	float pat;

	switch (inPatNo)
	{
	case vfWhite:
		pat = 1.0;
		break;
	case vfLtGray:
		pat = 0.8;
		break;
	case vfGray:
		pat = 0.5;
		break;
	case vfDkGray:
		pat = 0.3;
		break;
	case vfBlack:
		pat = 0.0;
		break;
	case 12:
		pat = 0.0;
		break;
	case vfDragger:
	case vfMatte:
		pat = 0.5;
		break;
	case vfCustom:
		break;
	default:
		isSet = NO;
	}
	if (isSet)
		CGContextSetGrayFillColor(quartz, pat, 1);

	return isSet;
}

