/*
	File:		DrawShape.cc

	Contains:	Shape creation and drawing functions.

	Written by:	Newton Research Group, 2007.
*/

#include "Quartz.h"
#include "Geometry.h"
#include "Objects.h"
#include "DrawShape.h"
#include "DrawText.h"

#include "Lookup.h"
#include "Iterators.h"
#include "ROMResources.h"
#include "OSErrors.h"
#include "MagicPointers.h"
#include "Strings.h"
#include "ViewFlags.h"


extern "C" void		PrintObject(Ref obj, int indent);

extern void		DrawPicture(char * inPicture, size_t inPictureSize, const Rect * inRect);
extern void		InkDrawInRect(RefArg inkData, size_t inPenSize, Rect * inOriginalBounds, Rect * inBounds, bool inLive);


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern "C" {
Ref		FIsPrimShape(RefArg inRcvr, RefArg inShape);
Ref		FDrawShape(RefArg inRcvr, RefArg inShape, RefArg inStyle);
Ref		FOffsetShape(RefArg inRcvr, RefArg ioShape, RefArg indX, RefArg indY);
Ref		FShapeBounds(RefArg inRcvr, RefArg inShape);
Ref		FPointsToArray(RefArg inRcvr, RefArg inShape);
Ref		FArrayToPoints(RefArg inRcvr, RefArg inArray);
};

class CSaveStyle;

void		DrawShapeList(RefArg inShapes, CSaveStyle * inStyle, Point inOffset);
void		DrawOneShape(RefArg inShape, CSaveStyle * inStyle, Point inOffset);
void		GetBoundsRect(RefArg inShape, Rect * outRect, Point inOffset, CSaveStyle * inStyle);


/*--------------------------------------------------------------------------------
	Read a number from an object.
	Args:		inObj		the NS object
				outNum	the number
	Return:	true if successful
--------------------------------------------------------------------------------*/

bool
FromObject(RefArg inObj, short * outNum)
{
	if (ISINT(inObj))
	{
		*outNum = RVALUE(inObj);
		return true;
	}
	return false;
}

/*--------------------------------------------------------------------------------
	Read a bounds frame from an object.
	Args:		inObj			the NS object
				outBounds	a rect for the bounds
	Return:	true if successful
--------------------------------------------------------------------------------*/

bool
FromObject(RefArg inObj, Rect * outBounds)
{
	RefVar var;
	var = GetFrameSlot(inObj, SYMA(top));
	if (FromObject(var, &outBounds->top))
	{
		var = GetFrameSlot(inObj, SYMA(left));
		if (FromObject(var, &outBounds->left))
		{
			var = GetFrameSlot(inObj, SYMA(bottom));
			if (FromObject(var, &outBounds->bottom))
			{
				var = GetFrameSlot(inObj, SYMA(right));
				if (FromObject(var, &outBounds->right))
				{
					return true;
				}
			}
		}
	}
	return false;
}


/*--------------------------------------------------------------------------------
	Create an object from a bounds frame.
	Args:		inBounds		the bounds
	Return:	a frame containing top, left, bottom, right slots
--------------------------------------------------------------------------------*/

Ref
SetBoundsRect(RefArg ioFrame, const Rect * inBounds)
{
	RefVar	r;

	r = MAKEINT(inBounds->top);
	SetFrameSlot(ioFrame, SYMA(top), r);
	r = MAKEINT(inBounds->left);
	SetFrameSlot(ioFrame, SYMA(left), r);
	r = MAKEINT(inBounds->bottom);
	SetFrameSlot(ioFrame, SYMA(bottom), r);
	r = MAKEINT(inBounds->right);
	SetFrameSlot(ioFrame, SYMA(right), r);

	return ioFrame;
}


Ref
ToObject(const Rect * inBounds)
{
	RefVar frame(Clone(RA(canonicalRect)));
	return SetBoundsRect(frame, inBounds);
}

#pragma mark -

/*------------------------------------------------------------------------------
	F o n t s
------------------------------------------------------------------------------*/

Ref
FamilyNumToSym(int inFontSpec)
{
	Ref	fontFamily;

	switch (inFontSpec)
	{
	case 0:
		fontFamily = SYMA(espy);				// change these to match OS X font names?
		break;
	case 1:
		fontFamily = SYMA(newYork);
		break;
	case 2:
		fontFamily = SYMA(geneva);
		break;
	case 3:
		fontFamily = SYMA(handwriting);
		break;
	default:
		fontFamily = NILREF;
		break;
	}

	return fontFamily;
}


Ref
GetFontFamilySym(RefArg inFontSpec)
{
	Ref	fontFamily;

	if (ISINT(inFontSpec))
		fontFamily = FamilyNumToSym(RVALUE(inFontSpec) & tsFamilyMask);

	else if (IsFrame(inFontSpec))
		fontFamily = RINT(GetFrameSlot(inFontSpec, SYMA(family)));

	else
		fontFamily = NILREF;

	return fontFamily;
}


ULong
GetFontFace(RefArg inFontSpec)
{
	ULong	fontFace;

	if (ISINT(inFontSpec))
		fontFace = (RVALUE(inFontSpec) & tsFaceMask) >> tsFaceShift;

	else if (IsFrame(inFontSpec))
		fontFace = RINT(GetFrameSlot(inFontSpec, SYMA(face)));
/*
	else if (IsInkWord(inFontSpec))
	{
		CInkWordGlyph	inkGlyph(inFontSpec, 0xFFFFFFFF, 0xFFFFFFFF);	// size+64
		fontFace = inkGlyph.x04;
	}
*/
	else
		fontFace = 0;

	return fontFace;
}


ULong
GetFontSize(RefArg inFontSpec)
{
	ULong	fontSize;

	if (ISINT(inFontSpec))
		fontSize = (RVALUE(inFontSpec) & tsSizeMask) >> tsSizeShift;

	else if (IsFrame(inFontSpec))
		fontSize = RINT(GetFrameSlot(inFontSpec, SYMA(size)));
/*
	else if (IsInkWord(inFontSpec))
	{
		CInkWordGlyph	inkGlyph(inFontSpec, 0xFFFFFFFF, 0xFFFFFFFF);
		fontSize = inkGlyph.x0C;
	}
*/
	else
		fontSize = 0;

	return fontSize;
}


#pragma mark -

/*------------------------------------------------------------------------------
	S a v e L e v e l
------------------------------------------------------------------------------*/

struct SaveLevel
{
	SaveLevel *  init(SaveLevel * inLevel);

	SaveLevel * fPrev;
//	RgnHandle	fClipRgn;	// +04
	long			f08;
	unsigned		flags;		// +0C
};


/*------------------------------------------------------------------------------
	C P a t t e r n
	Pattern means colour in this context.
	This is a hangover from the days of black & white bitmaps when the only way
	of achieving shades of grey was to dither bits in a pattern.
------------------------------------------------------------------------------*/

class CPattern
{
public:
						CPattern();
						~CPattern();

	bool				getFillPattern(RefArg inPatNo, bool inDoDefault);
	CGColorRef		color(void) const;
	void				setf05(bool inValue);

private:
#if 0
	PatternHandle  fPattern;			// +00
#else
	CGColorRef		fPattern;			// +00
#endif
	bool				fPatternIsOurs;	// +04
	bool				f05;					// +05
};

inline	CGColorRef		CPattern::color(void) const
{ return fPattern; }

inline	void				CPattern::setf05(bool inValue)
{ f05 = inValue; }


/*------------------------------------------------------------------------------
	S t y l e C a c h e
------------------------------------------------------------------------------*/

struct StyleInfo
{
	RefStruct	f00;
	unsigned		f04;

	bool			has(ULong inMask) const;
	void			clear(ULong inMask);
};

// bits in f04
enum
{
	kBitTransferMode =	1,
	kBitFillPattern =		1 << 1,
	kBitPenPattern =		1 << 2,
	kBitFontPattern =		1 << 3,
	kBitPenSize =			1 << 4,
	kBitJustification =	1 << 5,
	kBitFont =				1 << 6,
	kBitClipping =			1 << 7,
	kBitTransform =		1 << 8,
	kBitSelection =		1 << 9,
	kBitEverything =		0xFFFFFFFF
};

inline	bool		StyleInfo::has(ULong inMask) const
{ return (f04 & inMask) != 0; }

inline	void		StyleInfo::clear(ULong inMask)
{ f04 &= ~inMask; }


/*------------------------------------------------------------------------------
	C S a v e S t y l e
------------------------------------------------------------------------------*/

class CSaveStyle
{
public:
						CSaveStyle();
						~CSaveStyle();

	void				beginLevel(SaveLevel * inLevel);
	void				endLevel(void);
	bool				setStyle(RefArg inStyle, Point inOffset, unsigned inFlags);

	Ref				getStyle(void)				const;
	void				setFillPattern(void);
	void				setStrokePattern(void);

//private:
	StyleInfo *		lookupCache(void);

	bool			fDoStroke;				// +00
	bool			fDoFill;					// +01
	bool			fDoText;					// +02
	long			fSelection;				// +04
	long			fTransferMode;			// +08
	CPattern		fFillPattern;			// +0C
	CPattern		fStrokePattern;		// +14
	CPattern		fTextPattern;			// +1C
	Fract			fAlignment;				// +24
	Fract			fJustification;		// +28
	RefStruct	fFontSpec;				// +2C
	SaveLevel * fCurrentLevel;			// +30
	SaveLevel	fInitialLevel;			// +34
	int			fClippingDepth;		// +44
	int			f48;
//	GrafPtr		fThePort;				// +4C
	float			fLineWidth;
	RefStruct	fCurrentStyle;			// +50
	StyleInfo	fStyleCache[3];		// +54
	int			fStyleCacheIndex;		// +6C
};

inline	Ref	CSaveStyle::getStyle(void)	const
{ return fCurrentStyle; }

inline	void	CSaveStyle::setFillPattern(void)
{
#if 0
	PatternHandle  ptn = fFillPattern.pattern();
	if (ptn == NULL)
		ptn = GetStdPattern(blackPat);
	SetFgPattern(ptn);
#else
	CGColorRef  clr = fFillPattern.color();
	if (clr == NULL)
		clr = GetStdPattern(blackPat);					// GetStdPattern must return CGColorRef
	CGContextSetFillColorWithColor(quartz, clr);
#endif
}

inline	void	CSaveStyle::setStrokePattern(void)
{
#if 0
	PatternHandle  ptn = fStrokePattern.pattern();
	if (ptn == NULL)
		ptn = GetStdPattern(blackPat);
	SetFgPattern(ptn);
#else
	CGColorRef  clr = fStrokePattern.color();
	if (clr == NULL)
		clr = GetStdPattern(blackPat);					// GetStdPattern must return CGColorRef
	CGContextSetStrokeColorWithColor(quartz, clr);
#endif
}

#pragma mark -

/*------------------------------------------------------------------------------
	C P a t t e r n
------------------------------------------------------------------------------*/

CPattern::CPattern()
	:	fPattern(NULL), fPatternIsOurs(false), f05(true)
{ }


CPattern::~CPattern()
{
	if (fPatternIsOurs)
	{
#if 0
		if (f05 && fPattern == GetFgPattern())
			SetFgPattern(GetStdPattern(blackPat));
		DisposePattern(fPattern);
#else
//		CFRelease(fPattern);
#endif
	}
}


bool
CPattern::getFillPattern(RefArg inPatNo, bool inDoDefault)
{
	bool  result = inDoDefault;
	if (NOTNIL(inPatNo))
		result = GetPattern(inPatNo, &fPatternIsOurs, &fPattern, inDoDefault);
	return result;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S a v e S t y l e
------------------------------------------------------------------------------*/

SaveLevel *
SaveLevel::init(SaveLevel * inLevel)
{
	fPrev = inLevel;
//	fClipRgn = NULL;
	f08 = 0;
	flags = 0;
	return this;
}


CSaveStyle::CSaveStyle()
{
//	GetPort(&fThePort);
	CGContextSaveGState(quartz);
	fLineWidth = LineWidth();

	fCurrentStyle = MAKEINT(-1);
	fInitialLevel.init(NULL);
	fCurrentLevel = &fInitialLevel;
	fClippingDepth = 0;
	f48 = 0;
	fSelection = 0;
	fStyleCacheIndex = 0;
}


CSaveStyle::~CSaveStyle()
{
	endLevel();
	SetLineWidth(fLineWidth);
	CGContextRestoreGState(quartz);
}


void
CSaveStyle::beginLevel(SaveLevel * inLevel)
{
	fCurrentLevel = inLevel->init(fCurrentLevel);
}


void
CSaveStyle::endLevel(void)
{
	if (fCurrentLevel->flags & 0x01)
	{
#if 0
		CQDScaler::stopScaling();
#endif
		f48--;
	}

	if (fCurrentLevel->flags & 0x02)
	{
#if 0
		CQDScaler::replaceClip(fCurrentLevel->fClipRgn, NULL, 0);
		DisposeRgn(fCurrentLevel->fClipRgn);
#endif
		fClippingDepth--;
	}

	fCurrentLevel = fCurrentLevel->fPrev;
}


StyleInfo *
CSaveStyle::lookupCache(void)
{
	for (ArrayIndex i = 0; i < 3; ++i)
	{
		if (EQ(fCurrentStyle, fStyleCache[i].f00))
			return &fStyleCache[i];
	}

	StyleInfo * cachedStyle = &fStyleCache[fStyleCacheIndex++];
	if (fStyleCacheIndex >= 3)
		fStyleCacheIndex = 0;
	cachedStyle->f00 = fCurrentStyle;
	cachedStyle->f04 = kBitEverything;
	return cachedStyle;
}


bool
CSaveStyle::setStyle(RefArg inStyle, Point inOffset, unsigned inFlags)
{
// if already using this style, ignore the request
	if (EQ(inStyle, fCurrentStyle))
		return true;

	bool  sp04 = (inFlags & 0x01) != 0;	// restoring?
	bool  sp00 = (inFlags & 0x02) != 0;
	fCurrentStyle = inStyle;

// set defaults
	LineNormal();
	fDoStroke = true;
	fDoFill = false;
	fDoText = false;
	fTransferMode = modeOr;
	fAlignment = 0;
	fJustification = 0;
	fSelection = 0;

// modify defaults from inStyle
	bool  gotFont = false;
	bool  r10 = true;
	if (NOTNIL(inStyle))
	{
		StyleInfo * cachedStyle = lookupCache();
		RefVar item;

		if (cachedStyle->has(kBitClipping))
		{
			if (NOTNIL(item = GetProtoVariable(inStyle, SYMA(clipping))))
			{
				// set clipping
			}
			else
				cachedStyle->clear(kBitClipping);
		}

		if (!sp04 && cachedStyle->has(kBitTransform))
		{
			if (NOTNIL(item = GetProtoVariable(inStyle, SYMA(transform))))
			{
				// set transform
			}
			else
				cachedStyle->clear(kBitClipping);
		}

		if (cachedStyle->has(kBitSelection))
		{
			if (NOTNIL(item = GetProtoVariable(inStyle, SYMA(selection))))
			{
				fSelection = RINT(item);
			}
			else
				cachedStyle->clear(kBitSelection);
		}

		if (cachedStyle->has(kBitPenSize))
		{
			if (NOTNIL(item = GetProtoVariable(inStyle, SYMA(penSize))))
			{
				if (ISINT(item))
				{
					int pnSize = RINT(item);
					if (pnSize < 0)
						pnSize = 0;
					SetLineWidth(pnSize);
				}
				else
				{
					int pnSizeX = RINT(GetArraySlot(item, 0));
					int pnSizeY = RINT(GetArraySlot(item, 1));
					if (pnSizeX < 0)
						pnSizeX = 0;
					if (pnSizeY < 0)
						pnSizeY = 0;
					SetLineWidth(MAX(pnSizeX, pnSizeY));
				}
			}
			else
				cachedStyle->clear(kBitPenSize);
		}

		if (cachedStyle->has(kBitFillPattern))
		{
			if (NOTNIL(item = GetProtoVariable(inStyle, SYMA(fillPattern))))
			{
				fDoFill = fFillPattern.getFillPattern(item, false);
			}
			else
				cachedStyle->clear(kBitFillPattern);
		}

		if (!sp00)
		{
			if (cachedStyle->has(kBitTransferMode))
			{
				if (NOTNIL(item = GetProtoVariable(inStyle, SYMA(transferMode))))
				{
					fTransferMode = RINT(item);
					if (fTransferMode == modeMask)
						fTransferMode = modeCopy;
//					PenMode(fTransferMode+8);
				}
				else
					cachedStyle->clear(kBitTransferMode);
			}

			if (cachedStyle->has(kBitPenPattern))
			{
				if (NOTNIL(item = GetProtoVariable(inStyle, SYMA(penPattern))))
				{
					fDoStroke = fStrokePattern.getFillPattern(item, true);
				}
				else
					cachedStyle->clear(kBitPenPattern);
			}

			if (cachedStyle->has(kBitFontPattern))
			{
				if (NOTNIL(item = GetProtoVariable(inStyle, SYMA(textPattern))))
				{
					fDoText = fTextPattern.getFillPattern(item, true);
					if (fDoText)
						fTextPattern.setf05(true);
				}
				else
					cachedStyle->clear(kBitFontPattern);
			}

			if (cachedStyle->has(kBitJustification))
			{
				if (NOTNIL(item = GetProtoVariable(inStyle, SYMA(justification))))
				{
					if (EQ(item, SYMA(center)))
						fJustification = fract1/2;
					else if (EQ(item, SYMA(right)))
						fJustification = fract1;
				}
				else
					cachedStyle->clear(kBitJustification);
			}

			if (cachedStyle->has(kBitFont))
			{
				if (NOTNIL(item = GetProtoVariable(inStyle, SYMA(font))))
				{
					fFontSpec = item;
					gotFont = true;
				}
				else
					cachedStyle->clear(kBitFont);
			}
		}
	}

	if (!gotFont)
		fFontSpec = MAKEINT((10<<10) + 2);	// GetPreference(SYMA(userFont));

	return r10;
}

#pragma mark -


/*------------------------------------------------------------------------------
	S h a p e   T e s t i n g
------------------------------------------------------------------------------*/

bool
IsPrimShape(RefArg inObj)
{
	if (ISPTR(inObj))
	{
		RefVar	objClass(ClassOf(inObj));
		return EQ(objClass, SYMA(rectangle))
			 || EQ(objClass, SYMA(line))
			 || EQ(objClass, SYMA(textBox))
			 || EQ(objClass, SYMA(ink))
			 || EQ(objClass, SYMA(roundRectangle))
			 || EQ(objClass, SYMA(oval))
			 || EQ(objClass, SYMA(bitmap))
			 ||(EQ(objClass, SYMA(picture)) && IsFrame(inObj))
			 || EQ(objClass, SYMA(polygon))
			 || EQ(objClass, SYMA(wedge))
			 || EQ(objClass, SYMA(region))
			 || EQ(objClass, SYMA(text));
	}
	return false;
}


inline bool
IsStyleFrame(RefArg inObj)
{
	return EQ(ClassOf(inObj), SYMA(frame));
}


/*------------------------------------------------------------------------------
	B o u n d i n g
------------------------------------------------------------------------------*/
#if 1
#if defined(hasByteSwapping)
// byte-swap shape bounds for NCX
void
MungeBounds(RefArg inShape)
{
	RefVar shapeClass(ClassOf(inShape));

	if (IsArray(inShape))
	{
		CObjectIterator iter(inShape);
		for ( ; !iter.done(); iter.next())
		{
			if (NOTNIL(iter.value()) && !IsStyleFrame(iter.value()))
				MungeBounds(iter.value());
		}
	}

	else if (EQ(shapeClass, SYMA(region)))
	{
		CDataPtr rgnData(GetProtoVariable(inShape, SYMA(data)));
		RegionShape * rgn = (RegionShape *)(Ptr) rgnData;
		rgn->bBox.top = BYTE_SWAP_SHORT(rgn->bBox.top);
		rgn->bBox.left = BYTE_SWAP_SHORT(rgn->bBox.left);
		rgn->bBox.bottom = BYTE_SWAP_SHORT(rgn->bBox.bottom);
		rgn->bBox.right = BYTE_SWAP_SHORT(rgn->bBox.right);
	}

	else if (EQ(shapeClass, SYMA(ink)))
	{
		CDataPtr boundsData(GetProtoVariable(inShape, SYMA(bounds)));
		Rect * boundsPtr = (Rect *)(Ptr) boundsData;
		boundsPtr->top = BYTE_SWAP_SHORT(boundsPtr->top);
		boundsPtr->left = BYTE_SWAP_SHORT(boundsPtr->left);
		boundsPtr->bottom = BYTE_SWAP_SHORT(boundsPtr->bottom);
		boundsPtr->right = BYTE_SWAP_SHORT(boundsPtr->right);

		CDataPtr	originalBoundsData(GetProtoVariable(inShape, SYMA(originalBounds)));
		boundsPtr = (Rect *)(Ptr) originalBoundsData;
		boundsPtr->top = BYTE_SWAP_SHORT(boundsPtr->top);
		boundsPtr->left = BYTE_SWAP_SHORT(boundsPtr->left);
		boundsPtr->bottom = BYTE_SWAP_SHORT(boundsPtr->bottom);
		boundsPtr->right = BYTE_SWAP_SHORT(boundsPtr->right);
	}

	else if (EQ(shapeClass, SYMA(polygon)))
	{
		CDataPtr polyData(GetProtoVariable(inShape, SYMA(data)));
		PolygonShape * poly = (PolygonShape *)(Ptr) polyData;
		poly->bBox.top = BYTE_SWAP_SHORT(poly->bBox.top);
		poly->bBox.left = BYTE_SWAP_SHORT(poly->bBox.left);
		poly->bBox.bottom = BYTE_SWAP_SHORT(poly->bBox.bottom);
		poly->bBox.right = BYTE_SWAP_SHORT(poly->bBox.right);
		poly->size = BYTE_SWAP_SHORT(poly->size);
		Point * p = poly->points;
		int i, count = (poly->size - 12) / sizeof(Point);
		for (i = 0; i < count; i++, p++)
		{
			p->h = BYTE_SWAP_SHORT(p->h);
			p->v = BYTE_SWAP_SHORT(p->v);
		}
	}

	else if (IsPrimShape(inShape))
	{
		RefVar	boundsObj;
		if (EQ(shapeClass, SYMA(bitmap))
		||  EQ(shapeClass, SYMA(picture))
		||  EQ(shapeClass, SYMA(text)))
			boundsObj = GetProtoVariable(inShape, SYMA(bounds));
		else
			boundsObj = inShape;

		CDataPtr boundsData(boundsObj);
		Rect * boundsPtr = (Rect *)(Ptr) boundsData;
		boundsPtr->top = BYTE_SWAP_SHORT(boundsPtr->top);
		boundsPtr->left = BYTE_SWAP_SHORT(boundsPtr->left);
		boundsPtr->bottom = BYTE_SWAP_SHORT(boundsPtr->bottom);
		boundsPtr->right = BYTE_SWAP_SHORT(boundsPtr->right);

		if (EQ(shapeClass, SYMA(bitmap)))
		{
			RefVar bits;
			if (NOTNIL(bits = GetProtoVariable(inShape, SYMA(data))))
			{
				CDataPtr pixmapData(bits);
				if (IsInstance(bits, SYMA(pixels)))
				{
					PixelMap * pixmap = (PixelMap *)(Ptr)pixmapData;
					pixmap->reserved1 = 0x11EB;	// sign it so we know bounds have been swapped
					pixmap->bounds.top = BYTE_SWAP_SHORT(pixmap->bounds.top);
					pixmap->bounds.left = BYTE_SWAP_SHORT(pixmap->bounds.left);
					pixmap->bounds.bottom = BYTE_SWAP_SHORT(pixmap->bounds.bottom);
					pixmap->bounds.right = BYTE_SWAP_SHORT(pixmap->bounds.right);
				}
				else	// it’s a FramBitmap
				{
					FramBitmap * bitmap = (FramBitmap *)(Ptr)pixmapData;
					bitmap->reserved1 = 0x11EB;	// sign it so we know bounds have been swapped
					bitmap->bounds.top = BYTE_SWAP_SHORT(bitmap->bounds.top);
					bitmap->bounds.left = BYTE_SWAP_SHORT(bitmap->bounds.left);
					bitmap->bounds.bottom = BYTE_SWAP_SHORT(bitmap->bounds.bottom);
					bitmap->bounds.right = BYTE_SWAP_SHORT(bitmap->bounds.right);
				}
			}
			if (NOTNIL(bits = GetProtoVariable(inShape, SYMA(mask))))
			{
				CDataPtr pixmapData(bits);
				PixelMap * bitmap = (PixelMap *)(Ptr)pixmapData;
				bitmap->reserved1 = 0x11EB;	// sign it so we know bounds have been swapped
				bitmap->bounds.top = BYTE_SWAP_SHORT(bitmap->bounds.top);
				bitmap->bounds.left = BYTE_SWAP_SHORT(bitmap->bounds.left);
				bitmap->bounds.bottom = BYTE_SWAP_SHORT(bitmap->bounds.bottom);
				bitmap->bounds.right = BYTE_SWAP_SHORT(bitmap->bounds.right);
			}
			if (NOTNIL(bits = GetProtoVariable(inShape, SYMA(bits))))
			{
				CDataPtr pixmapData(bits);
				PixelMap * bitmap = (PixelMap *)(Ptr)pixmapData;
				bitmap->reserved1 = 0x11EB;	// sign it so we know bounds have been swapped
				bitmap->bounds.top = BYTE_SWAP_SHORT(bitmap->bounds.top);
				bitmap->bounds.left = BYTE_SWAP_SHORT(bitmap->bounds.left);
				bitmap->bounds.bottom = BYTE_SWAP_SHORT(bitmap->bounds.bottom);
				bitmap->bounds.right = BYTE_SWAP_SHORT(bitmap->bounds.right);
			}
			if (NOTNIL(bits = GetProtoVariable(inShape, SYMA(colorData)))
			&&  NOTNIL(bits = GetProtoVariable(bits, SYMA(cBits))))
			{
				CDataPtr pixmapData(bits);
				PixelMap * bitmap = (PixelMap *)(Ptr)pixmapData;
				bitmap->reserved1 = 0x11EB;	// sign it so we know bounds have been swapped
				bitmap->bounds.top = BYTE_SWAP_SHORT(bitmap->bounds.top);
				bitmap->bounds.left = BYTE_SWAP_SHORT(bitmap->bounds.left);
				bitmap->bounds.bottom = BYTE_SWAP_SHORT(bitmap->bounds.bottom);
				bitmap->bounds.right = BYTE_SWAP_SHORT(bitmap->bounds.right);
			}
		}

		else if (EQ(shapeClass, SYMA(roundRectangle)))
		{
			((RoundRectShape *)boundsPtr)->hRadius = BYTE_SWAP_SHORT(((RoundRectShape *)boundsPtr)->hRadius);
			((RoundRectShape *)boundsPtr)->vRadius = BYTE_SWAP_SHORT(((RoundRectShape *)boundsPtr)->vRadius);
		}
		else if (EQ(shapeClass, SYMA(wedge)))
		{
			((WedgeShape *)boundsPtr)->arc1 = BYTE_SWAP_SHORT(((WedgeShape *)boundsPtr)->arc1);
			((WedgeShape *)boundsPtr)->arc2 = BYTE_SWAP_SHORT(((WedgeShape *)boundsPtr)->arc2);
		}
		else if (EQ(shapeClass, SYMA(text)))
		{
			CDataPtr textData(GetProtoVariable(inShape, SYMA(data)));
			UniChar * s = (UniChar *)(Ptr) textData;
			for (ArrayIndex i = 0, count = Length(textData) / sizeof(UniChar); i < count; ++i, ++s)
				*s = BYTE_SWAP_SHORT(*s);
		}
	}

	else
		ThrowErr(exGraf, -8804);
}
#endif
#endif

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

	else if (EQ(shapeClass, SYMA(region)))
	{
		RefVar	rgnObj(GetProtoVariable(inShape, SYMA(data)));
		CDataPtr rgnData(rgnObj);
		RegionShape * rgn = (RegionShape *)((Ptr) rgnData);
		*outRect = rgn->bBox;
	}

	else if (EQ(shapeClass, SYMA(ink)))
	{
		RefVar	boundsObj(GetProtoVariable(inShape, SYMA(bounds)));
		CDataPtr boundsData(boundsObj);
		*outRect = *(Rect *)(Ptr) boundsData;
	}

	else if (EQ(shapeClass, SYMA(polygon)))
	{
		RefVar	polyObj(GetProtoVariable(inShape, SYMA(data)));
		CDataPtr polyData(polyObj);
		PolygonShape * poly = (PolygonShape *)((Ptr) polyData);
		*outRect = poly->bBox;
		// expand bounds to ensure all points are included
		outRect->bottom++;
		outRect->right++;
	}

	else if (IsPrimShape(inShape))
	{
		RefVar	boundsObj;
		if (EQ(shapeClass, SYMA(bitmap))
		||  EQ(shapeClass, SYMA(picture))
		||  EQ(shapeClass, SYMA(text)))
			boundsObj = GetProtoVariable(inShape, SYMA(bounds));
		else
			boundsObj = inShape;

		CDataPtr boundsData(boundsObj);
		*outRect = *(Rect *)(Ptr) boundsData;

		// fix up line bounds so it’s always a non-empty rect
		if (EQ(shapeClass, SYMA(line)))
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
		ThrowErr(exGraf, -8804);

	return outRect;
}


/*------------------------------------------------------------------------------
	Get a shape’s bounds rect.
	Args:		inShape
				outRect
				inOffset
				inStyle
	Return:	--
------------------------------------------------------------------------------*/

void
GetBoundsRect(RefArg inShape, Rect * outRect, Point inOffset, CSaveStyle * inStyle)
{
	RefVar	boundsRect(GetProtoVariable(inShape, SYMA(bounds)));
	CDataPtr boundsData(boundsRect);
	*outRect = *(Rect *)(Ptr) boundsData;
	if (inStyle->f48 == 0)
		OffsetRect(outRect, inOffset.h, inOffset.v);
}

#pragma mark -
#if !forFramework
#include "View.h"
extern CView *	FailGetView(RefArg inContext);
/*------------------------------------------------------------------------------
	D r a w i n g
------------------------------------------------------------------------------*/

Ref
FIsPrimShape(RefArg inRcvr, RefArg inShape)
{
	return MAKEBOOLEAN(IsPrimShape(inShape));
}


Ref
FDrawShape(RefArg inRcvr, RefArg inShape, RefArg inStyle)
{
	CView *  view = FailGetView(inRcvr);
	Point  pt;
	pt.h = view->viewBounds.left;
	pt.v = view->viewBounds.top;
	//saveScale = QDScaler::forceScaling(1);
	DrawShape(inShape, inStyle, pt);
	//QDScaler::forceScaling(saveScale);
	return NILREF;
}


/*------------------------------------------------------------------------------
	Offset [all the subshapes within] a shape.
	Args:		inShape
				indX
				indY
	Return:	--
------------------------------------------------------------------------------*/

Ref
FOffsetShape(RefArg inRcvr, RefArg ioShape, RefArg indX, RefArg indY)
{
	RefVar	shapeClass(ClassOf(ioShape));
	long		dx = RINT(indX);
	long		dy = RINT(indY);
	if ((dx | dy) == 0)
		return ioShape;

	if (IsArray(ioShape))
	{
		CObjectIterator	iter(ioShape);
		for ( ; !iter.done(); iter.next())
		{
			if (NOTNIL(iter.value()) && !IsStyleFrame(iter.value()))
				FOffsetShape(inRcvr, iter.value(), indX, indY);
		}
	}

	else if (/*EQ(shapeClass, SYMA(region))
		  ||  */EQ(shapeClass, SYMA(polygon)))
	{
		RefVar	shape(GetProtoVariable(ioShape, SYMA(data)));
		CDataPtr shapeData(shape);
/*		PolygonHandle  poly = NewFakeHandle(shapeData, Length(shape));
		OffsetPoly(poly, indX, indY);
*/	}

	else if (EQ(shapeClass, SYMA(bitmap))
		  ||  EQ(shapeClass, SYMA(picture))
		  ||  EQ(shapeClass, SYMA(text))
		  ||  EQ(shapeClass, SYMA(ink)))
	{
		CDataPtr boundsData(GetProtoVariable(ioShape, SYMA(bounds)));
		OffsetRect((Rect *)(Ptr) boundsData, indX, indY);
	}

	return ioShape;
}


Ref
FShapeBounds(RefArg inRcvr, RefArg inShape)
{
	Rect  bounds;
	return ToObject(ShapeBounds(inShape, &bounds));
}


/*------------------------------------------------------------------------------
	Convert the points data in a polygon shape from the binary data structure
	in the shape view to an array.
	Args:		inShape		binary object of class 'polygonShape
	Return:	array object
				[0] = shape type
				[1] = number of points
				[2],[3]... points
------------------------------------------------------------------------------*/

Ref
FPointsToArray(RefArg inRcvr, RefArg inShape)
{
	if (!EQ(ClassOf(inShape), SYMA(polygonShape)))
		ThrowMsg("not a polygonShape");

	CDataPtr ppd(inShape);
	int16_t * pd = (int16_t *)(char *)ppd;
	ArrayIndex numOfPoints = pd[1];
	ArrayIndex numOfElements = 2 + numOfPoints*2;
	RefVar points(MakeArray(numOfElements));
	SetArraySlot(points, 0, MAKEINT(pd[0]));	// shape type
	SetArraySlot(points, 1, MAKEINT(pd[1]));	// number of points
	Point * p = (Point *)(pd+2);
	for (ArrayIndex i = 2; i < numOfElements; ++p)
	{
		SetArraySlot(points, i, p->h), ++i;
		SetArraySlot(points, i, p->v), ++i;
	}
	return points;
}


/*------------------------------------------------------------------------------
	Convert the points in an array to the binary data structure of a polygon shape.
	Args:		inArray		array of points
								[0] = type; 4=>closed, 5=>open polygon
	Return:	polygon shape object
------------------------------------------------------------------------------*/

Ref
FArrayToPoints(RefArg inRcvr, RefArg inArray)
{
//	ArrayIndex numOfPoints = Length(inArray);	// original uses this instead of element 1 of array
	ArrayIndex numOfPoints = RINT(GetArraySlot(inArray, 1));
	ArrayIndex numOfElements = 2 + numOfPoints*2;
	RefVar polyPoints(AllocateBinary(SYMA(polygonShape), 2*sizeof(int16_t) + numOfPoints*sizeof(Point)));
	CDataPtr ppd(polyPoints);
	int16_t * pd = (int16_t *)(char *)ppd;
	pd[0] = RINT(GetArraySlot(inArray, 0));	// shape type
	pd[1] = numOfPoints;
	Point * p = (Point *)(pd+2);
	for (ArrayIndex i = 2; i < numOfElements; ++p)
	{
		p->h = RINT(GetArraySlot(inArray, i)), ++i;
		p->v = RINT(GetArraySlot(inArray, i)), ++i;
	}
	return polyPoints;
}

#endif

#pragma mark -

/*------------------------------------------------------------------------------
	Draw a shape.
	Args:		inShape
				inStyle
				inOffset
	Return:	--
------------------------------------------------------------------------------*/

void
DrawShape(RefArg inShape, RefArg inStyle, Point inOffset)
{
	CSaveStyle  theStyle;
	if (theStyle.setStyle(inStyle, inOffset, 0))
	{
		SaveLevel level;
		theStyle.beginLevel(&level);
		unwind_protect
		{
			DrawShapeList(inShape, &theStyle, inOffset);
		}
		on_unwind
		{
			theStyle.endLevel();
			if (unwind_failed())
				theStyle.~CSaveStyle();
		}
		end_unwind;
	}
}


/*------------------------------------------------------------------------------
	Draw a shape list.
	Args:		inShapes
				inStyle
				inOffset
	Return:	--
------------------------------------------------------------------------------*/

void
DrawShapeList(RefArg inShapes, CSaveStyle * inStyle, Point inOffset)
{
	if (IsArray(inShapes))
	{
		bool  badStyle = false;
		CObjectIterator	iter(inShapes);
		for ( ; !iter.done(); iter.next())
		{
			RefVar	shape = iter.value();
			if (IsStyleFrame(shape) || ISNIL(shape))
			//	it’s a style frame so update the style
				badStyle = !inStyle->setStyle(shape, inOffset, 0);
			else if (!badStyle)
			{
				if (IsArray(shape))
				{
				//	it’s a group of shapes; draw them in a new level
					RefVar		oldStyle(inStyle->getStyle());
					SaveLevel	level;
					inStyle->beginLevel(&level);
					unwind_protect
					{
					//	recurse
						DrawShapeList(shape, inStyle, inOffset);
					// reset the style -- shouldn’t this be in the on_unwind?
						inStyle->setStyle(oldStyle, inOffset, 1);
					}
					on_unwind
					{
						inStyle->endLevel();
					}
					end_unwind;
				}
				else
				//	it’s a single shape
					DrawOneShape(shape, inStyle, inOffset);
			}
		}
	}
	else
		DrawOneShape(inShapes, inStyle, inOffset);
}


/*------------------------------------------------------------------------------
	Draw a shape.
	Args:		inShape
				inStyle
				inOffset
	Return:	--
------------------------------------------------------------------------------*/

void
DrawOneShape(RefArg inShape, CSaveStyle * inStyle, Point inOffset)
{
	//sp-08
	OffsetFunc	offsetFn = (OffsetFunc) OffsetRect;	// sp00
	FillFunc		fillFn = 0;					// r9
	StrokeFunc	strokeFn;					// r1

	if (inStyle->fDoFill)
	{
		inStyle->setFillPattern();
	}

	//sp-10
	RefVar	shapeData(inShape);		// sp0C
	bool		isShapeHandle = false;		// sp08

	RefVar	shapeClass(ClassOf(inShape));		// r6

	Rect		shapeBounds;  // sp00
	ShapeBounds(inShape, &shapeBounds);

	if (inStyle->f48 == 0)
		OffsetRect(&shapeBounds, inOffset.h, inOffset.v);

//000DFDB4
	//sp-08
#if 0
	if (inStyle->fClippingDepth != 0)
	{
		// trivial rejection
		GrafPtr  thePort;
		GetPort(&thePort);
		if (!Intersects(shapeBounds, thePort->clipRgn.rgnBBox))
			return;
	}
#endif

//000DFE14
	if (EQ(shapeClass, SYMA(rectangle)))			// •• rectangle
	{
		fillFn = (FillFunc) (void(*)(const Rect*)) FillRect;
		strokeFn = (StrokeFunc) (void(*)(const Rect*)) StrokeRect;
		goto commondraw;
	}

	else if (EQ(shapeClass, SYMA(line)))			// •• line
	{
		if (inStyle->fDoStroke)
		{
			inStyle->setStrokePattern();

			CDataPtr lineData(inShape);
			Rect		boundsRect;
			Point		ptFrom, ptTo;
			boundsRect = *(Rect *)(Ptr) lineData;

			if (inStyle->f48 == 0)
				OffsetRect(&boundsRect, inOffset.h, inOffset.v);

			ptFrom.h = boundsRect.left;
			ptFrom.v = boundsRect.top;
			ptTo.h = boundsRect.right;
			ptTo.v = boundsRect.bottom;
			StrokeLine(ptFrom, ptTo);

			if (inStyle->fSelection != 0)
				shapeBounds = boundsRect;
		}
	}

//000DFF34
	else if (EQ(shapeClass, SYMA(ink)))
	{
		RefVar	obj(GetProtoVariable(inShape, SYMA(bounds)));
		CDataPtr	boundsData(obj);
		Rect		boundsRect;
		boundsRect = *(Rect *)(Ptr) boundsData;

		obj = GetProtoVariable(inShape, SYMA(originalBounds));
		CDataPtr	originalBoundsData(obj);
		Rect		originalBoundsRect;
		originalBoundsRect = *(Rect *)(Ptr) originalBoundsData;

		if (inStyle->f48 == 0)
		{
			OffsetRect(&boundsRect, inOffset.h, inOffset.v);
			OffsetRect(&originalBoundsRect, inOffset.h, inOffset.v);
		}

		shapeData = GetProtoVariable(inShape, SYMA(data));
		inStyle->setStrokePattern();
		InkDrawInRect(shapeData, LineWidth(), &originalBoundsRect, &boundsRect, false);
	}

//000E00D0
	else if (EQ(shapeClass, SYMA(roundRectangle)))
	{
		CDataPtr		roundRectData(inShape);
		RoundRectShape	roundRect;
		roundRect = *(RoundRectShape *)(Ptr) roundRectData;

		if (inStyle->f48 == 0)
			OffsetRect(&roundRect.rect, inOffset.h, inOffset.v);

		if (inStyle->fDoFill)
			FillRoundRect(roundRect.rect, roundRect.hRadius, roundRect.vRadius);

		if (inStyle->fDoStroke)
		{
			inStyle->setStrokePattern();
			StrokeRoundRect(roundRect.rect, roundRect.hRadius, roundRect.vRadius);
		}
	}

//000E01B8
	else if (EQ(shapeClass, SYMA(oval)))
	{
		fillFn = (FillFunc) (void(*)(const Rect*)) FillOval;
		strokeFn = (FillFunc) (void(*)(const Rect*)) StrokeOval;
		goto commondraw;
	}

//000E01DC
	else if (EQ(shapeClass, SYMA(wedge)))
	{
		// sp-0C
		CDataPtr		wedgeData(inShape);
		WedgeShape	wedge;
		wedge = *(WedgeShape *)(Ptr) wedgeData;

		if (inStyle->f48 == 0)
			OffsetRect(&wedge.rect, inOffset.h, inOffset.v);
#if 0
		WedgeBox(&shapeBounds, wedge.arc1, wedge.arc2);

		if (inStyle->fDoFill)
			FillArc(&wedge.rect, wedge.arc1, wedge.arc2);

		if (inStyle->fDoStroke)
		{
			inStyle->setStrokePattern();
			StrokeArc(&wedge.rect, wedge.arc1, wedge.arc2);
		}
#endif
	}

//000E02EC
	else if (EQ(shapeClass, SYMA(region)))
	{
		shapeData = GetProtoVariable(inShape, SYMA(data));
		isShapeHandle = true;
		offsetFn = (OffsetFunc) (void(*)(RegionShape*,short,short)) OffsetRgn;
		fillFn = (FillFunc) (void(*)(RegionShape*)) FillRgn;
		strokeFn = (StrokeFunc) (void(*)(RegionShape*)) StrokeRgn;
		goto commondraw;
	}

//000E034C
	else if (EQ(shapeClass, SYMA(polygon)))
	{
		shapeData = GetProtoVariable(inShape, SYMA(data));
		isShapeHandle = true;
		offsetFn = (OffsetFunc) (void(*)(PolygonShape*,short,short)) OffsetPoly;
		fillFn = (FillFunc) (void(*)(PolygonShape*)) FillPoly;
		strokeFn = (FillFunc) (void(*)(PolygonShape*)) StrokePoly;

//000E0398
commondraw:
		if (fillFn)
		{
			//sp-14
			char		mutableShapeData[16];	// sp00
			char *	mutableShapeDataPtr = NULL;	// r8
			CDataPtr data(shapeData);			// sp10
			void *	shapePtr = (void *)(Ptr) data;

			if ((ObjectFlags(shapeData) & 0x40/*kObjReadOnly*/) != 0
			&&  inStyle->f48 == 0)
			{
			// we need to keep a copy that we can modify
				size_t	shapeLen = Length(shapeData);
				if (shapeLen > 16)
				{
				// it’s bigger than a rectangle; keep it in a handle
					mutableShapeDataPtr = NewPtr(shapeLen);	// was NewHandle
					memmove(mutableShapeDataPtr, shapePtr, shapeLen);
					shapePtr = mutableShapeDataPtr;
				}
				else
				{
					memmove(mutableShapeData, shapePtr, shapeLen);
					shapePtr = mutableShapeData;
				}
			}
/*
			if (isShapeHandle
			&&  h == NULL)
			{
				h = NewFakeHandle(shapePtr, Length(shapeData));
				shapePtr = h;
			}
*/
			newton_try
			{
				if (inStyle->f48 == 0)
					offsetFn(shapePtr, inOffset.h, inOffset.v);

				if (inStyle->fDoFill)
					fillFn(shapePtr);

				if (inStyle->fDoStroke)
				{
					inStyle->setStrokePattern();
					strokeFn(shapePtr);
				}

				if (inStyle->f48 == 0)
					offsetFn(shapePtr, -inOffset.h, -inOffset.v);

				if (mutableShapeDataPtr != NULL)
					FreePtr(mutableShapeDataPtr);
			}
			cleanup
			{
				data.~CDataPtr();
			}
			end_try;
		}
	}

//000E0428
	else if (EQ(shapeClass, SYMA(picture)))			// •• picture
	{
		shapeData = GetProtoVariable(inShape, SYMA(data));
		Rect  boundsRect;
		GetBoundsRect(inShape, &boundsRect, inOffset, inStyle);
		CDataPtr pictData(shapeData);
		size_t	pictSize = Length(shapeData);
//		PicHandle pict = NewFakeHandle(pictData, pictSize);	// old style -- draw picture handle
		unwind_protect
		{
			if (inStyle->f48 == 0)
				DrawPicture((char*)pictData, pictSize, &boundsRect);	// args originally (Picture**, Rect*, unsigned char)
			else
			{
				//int saveScale = QDScaler::forceScaling(2);
				//map boundsRect;
				DrawPicture((char*)pictData, pictSize, &boundsRect);	// new style -- draw PNG
				//QDScaler::forceScaling(saveScale);
			}
		}
		on_unwind
		{
//			DisposeHandle(pict);
			if (unwind_failed())
				pictData.~CDataPtr();
		}
		end_unwind;
	}

//000E05C0
	else if (EQ(shapeClass, SYMA(bitmap)))				// •• bitmap
	{
		Rect boundsRect;
		GetBoundsRect(inShape, &boundsRect, inOffset, inStyle);
		RefVar mask(GetFrameSlot(inShape, SYMA(mask)));
		int mode = inStyle->fTransferMode;
		if (mode == modeMask) {
			if (NOTNIL(mask))
				DrawBitmap(mask, &boundsRect, modeBic);
			mode = modeOr;
		}
		DrawBitmap(inShape, &boundsRect, mode);
	}

//000E0678
	else if (EQ(shapeClass, SYMA(text)))				// •• text
	{
		Rect  boundsRect;
		GetBoundsRect(inShape, &boundsRect, inOffset, inStyle);
		ULong	justifyH = vjLeftH;
		if (inStyle->fJustification == fract1/2)
			justifyH = vjCenterH;
		else if (inStyle->fJustification == fract1)
			justifyH = vjRightH;

		shapeData = GetProtoVariable(inShape, SYMA(data));
		CDataPtr textData(shapeData);
		CRichString str((UniChar *)(Ptr) textData, Length(shapeData));
		if (EQ(ClassOf(shapeData), SYMA(textBox)))
		{
			newton_try
			{
			//	set up clipping regions
				RefVar	fontSpec(Clone(RA(canonicalGrayFontSpec)));	// sp10
				SetFrameSlot(fontSpec, SYMA(size), MAKEINT(GetFontSize(inStyle->fFontSpec)));
				SetFrameSlot(fontSpec, SYMA(face), MAKEINT(GetFontFace(inStyle->fFontSpec)));
				SetFrameSlot(fontSpec, SYMA(family), GetFontFamilySym(inStyle->fFontSpec));
			//	SetFrameSlot(fontSpec, SYMA(color), spF0);
				TextBox(str, fontSpec, &boundsRect, justifyH, vjTopV /*,srcOr*/);
			}
			cleanup
			{
				textData.~CDataPtr();
			}
			end_try;
		}
		else
		{
//000E0994
		// it’s just a string of text (not justified within a box)
			StyleRecord style;
			CreateTextStyleRecord(inStyle->fFontSpec, &style);
		/*	if (inStyle->fDoText)
				style.fontPattern = AddressToRef((inStyle->fTextPattern != NULL) ? inStyle->fTextPattern : GetStdPattern(blackPat));
			else if (inStyle->fDoFill)
				style.fontPattern = AddressToRef((inStyle->fFillPattern != NULL) ? inStyle->fFillPattern : GetStdPattern(blackPat));
		*/
			FPoint	location;
			location.x = boundsRect.left;
			location.y = boundsRect.bottom;

			TextOptions options;
			options.alignment = inStyle->fAlignment;
			options.justification = inStyle->fJustification;
			options.width = RectGetWidth(boundsRect);
		/*	options.x0C = 0;
			options.transferMode = (inStyle->fTransferMode == patCopy) ? srcOr : inStyle->fTransferMode;
			options.x14 = 0;
			options.x18 = 0;
		*/
			DrawRichString(str, 0, str.length(), &style, location, &options, NULL);
		}
	}

#if 0
//000E0B8C
	// draw grab handles
	int	grabHandleSize = inStyle->fSelection & 0xFFFF;
	if (grabHandleSize != 0)
	{
		int grabHandleOffset = grabHandleSize / 2;
		int dh = RectGetWidth(bounds);
		int dv = RectGetHeight(bounds;
		bounds.top -= grabHandleOffset;
		bounds.left -= grabHandleOffset;
		bounds.bottom += grabHandleSize;
		bounds.right += grabHandleSize;

		if (EQ(shapeClass, SYMA(line)))
		{
			PenState pnState;
			GetPenState(&pnState);
			OffsetRect(&bounds, pnState.pnSize.h/2, pnState.pnSize.v/2);
			InvertRect(&bounds);
			OffsetRect(&bounds, dh, dv);
			InvertRect(&bounds);
		}
		else if (EQ(shapeClass, SYMA(rectangle)))							// it’s a rect…
			  && (dh < grabHandleOffset || dv < grabHandleOffset))	// …but too small to have distinct grab handles
		{
			if (dv < grabHandleOffset)
			{
				int correction = dv/2;
				bounds.top += correction;
				bounds.bottom += correction;
				InvertRect(&bounds);
				OffsetRect(&bounds, dh, 0);
				InvertRect(&bounds);
			}
			else
			{
				int correction = dh/2;
				bounds.left += correction;
				bounds.right += correction;
				InvertRect(&bounds);
				OffsetRect(&bounds, 0, dv);
				InvertRect(&bounds);
			}
		}
		else	// any other shape has grab handles in the corners of its bounding rectangle
		{
			InvertRect(&bounds);
			OffsetRect(&bounds, dh, 0);
			InvertRect(&bounds);
			OffsetRect(&bounds, 0, dv);
			InvertRect(&bounds);
			OffsetRect(&bounds, -dh, 0);
			InvertRect(&bounds);
		}
	}
#endif
}

