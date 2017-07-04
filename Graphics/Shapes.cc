/*
	File:		Shapes.cc

	Contains:	Shape creation and drawing functions.

	Written by:	Newton Research Group.
*/

#include "View.h"
#include "Geometry.h"
#include "DrawText.h"
#include "DrawShape.h"

#include "Objects.h"
#include "ROMResources.h"
#include "Iterators.h"
#include "Lookup.h"
#include "Preference.h"


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FMakeBitmap(RefArg inRcvr, RefArg inWd, RefArg inHt, RefArg inOptions);
Ref	FMakeLine(RefArg inRcvr, RefArg inX1, RefArg inY1, RefArg inX2, RefArg inY2);
Ref	FMakeRect(RefArg inRcvr, RefArg inLeft, RefArg inTop, RefArg inRight, RefArg inBottom);
Ref	FMakeText(RefArg inRcvr, RefArg inStr, RefArg inLeft, RefArg inTop, RefArg inRight, RefArg inBottom);
Ref	FMakeTextBox(RefArg inRcvr, RefArg inStr, RefArg inLeft, RefArg inTop, RefArg inRight, RefArg inBottom);
Ref	FMakeTextLines(RefArg inRcvr, RefArg inStr, RefArg inBox, RefArg inLineHeight, RefArg inFont);
Ref	FMakeShape(RefArg inRcvr, RefArg inObj);
}


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern void		GetStyleFontInfo(StyleRecord * inStyle, FontInfo * outFontInfo);
extern bool		IsPrimShape(RefArg inObj);

Ref	CommonMakePict(CView * inView, Rect& inRect, RefArg inArg3, RefArg inArg4);


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
	S h a p e   C r e a t i o n
------------------------------------------------------------------------------*/

Ref
MakePixelsObject(const Rect * inBounds, int inDepth, int inRowBytes, int inResX, int inResY,
					  RefArg inStore, RefArg inCompanderName, RefArg inCompanderData)
{
//	ignore compander stuff
	RefVar	data(AllocateBinary(SYMA(pixels), sizeof(PixelMap) + RectGetHeight(*inBounds) * inRowBytes));
	CDataPtr dataPtr(data);
	PixelMap * pixmap = (PixelMap *)(char *)dataPtr;
	pixmap->baseAddr = sizeof(PixelMap);
	pixmap->rowBytes = inRowBytes;
	pixmap->bounds = *inBounds;
	pixmap->pixMapFlags = kPixMapOffset + kPixMapVersion2 + inDepth;
	pixmap->deviceRes.h = inResX;
	pixmap->deviceRes.v = inResY;
	pixmap->grayTable = NULL;
	
	return data;
}


// MakeBitmap isn’t really a shape creation function,
// but this seems like a logical place for it.

Ref
FMakeBitmap(RefArg inRcvr, RefArg inWd, RefArg inHt, RefArg inOptions)
{
	RefVar	options;
	RefVar	store;
	RefVar	companderName;
	RefVar	companderData;

	int		height = RINT(inHt);
	if (height < 0)
		ThrowErr(exGraf, -8805);

	int		width = RINT(inWd);
	if (width < 0)
		ThrowErr(exGraf, -8806);

	int		pixRowBytes = ALIGN(width, 32) / 8;
	int		pixDepth = 1;
	int		xRes = kDefaultDPI;
	int		yRes = kDefaultDPI;

	if (NOTNIL(inOptions))
	{
		RefVar	resolution;
		options = Clone(inOptions);

		if (FrameHasSlot(inOptions, SYMA(depth)))
		{
			int  i, depth = RINT(GetFrameSlot(inOptions, SYMA(depth)));
			// ensure depth is a power-of-two
			for (i = depth; i > 1; i >>= 1)
				if (i & 1)
					break;
			if (i != 1)
				ThrowErr(exGraf, -8807);
			pixDepth = depth;
			if (pixDepth != 1)
				pixRowBytes *= pixDepth;
			RemoveSlot(options, SYMA(depth));
		}

		if (FrameHasSlot(inOptions, SYMA(rowBytes)))
		{
			int  rowBytes = RINT(GetFrameSlot(inOptions, SYMA(rowBytes)));
			if (TRUNC(rowBytes, 4) != 0 || rowBytes < pixRowBytes)
				ThrowErr(exGraf, -8808);
			pixRowBytes = rowBytes;
			RemoveSlot(options, SYMA(rowBytes));
		}

		if (FrameHasSlot(inOptions, SYMA(resolution)))
		{
			resolution = GetFrameSlot(inOptions, SYMA(resolution));
			if (IsArray(resolution))
			{
				xRes = RINT(GetArraySlot(resolution, 0));
				yRes = RINT(GetArraySlot(resolution, 1));
			}
			else
				xRes = yRes = RINT(resolution);
			RemoveSlot(options, SYMA(resolution));
		}

		if (FrameHasSlot(inOptions, SYMA(store)))
		{
			store = GetFrameSlot(inOptions, SYMA(store));
			RemoveSlot(options, SYMA(store));
		}

		if (FrameHasSlot(inOptions, SYMA(companderName)))
		{
			companderName = GetFrameSlot(inOptions, SYMA(companderName));
			RemoveSlot(options, SYMA(companderName));
			if (FrameHasSlot(inOptions, SYMA(companderData)))
			{
				companderName = GetFrameSlot(inOptions, SYMA(companderData));
				RemoveSlot(options, SYMA(companderData));
			}
		}
	}

	Rect  bounds;
	bounds.top = 0;
	bounds.left = 0;
	bounds.bottom = height;
	bounds.right = width;

	RefVar	pixData(MakePixelsObject(&bounds, pixDepth, pixRowBytes, xRes, yRes, store, companderName, companderData));

	if (xRes != kDefaultDPI)
		bounds.right = width * kDefaultDPI / xRes;
	if (yRes != kDefaultDPI)
		bounds.bottom = height * kDefaultDPI / yRes;

	RefVar	boundsRect(AllocateBinary(SYMA(boundsRect), sizeof(Rect)));
	CDataPtr boundsData(boundsRect);
	memmove((char *)boundsData, &bounds, sizeof(Rect));

	RefVar	theShape(Clone(RA(canonicalBitmapShape)));
	SetFrameSlot(theShape, SYMA(bounds), boundsRect);
	SetFrameSlot(theShape, SYMA(data), pixData);
	if (NOTNIL(options))
	{
		CObjectIterator	iter(options);
		for ( ; !iter.done(); iter.next())
			SetFrameSlot(theShape, iter.tag(), iter.value());
	}
	return theShape;
}


Ref
FMakeLine(RefArg inRcvr, RefArg inX1, RefArg inY1, RefArg inX2, RefArg inY2)
{
	Rect  bounds;
	bounds.top = RINT(inX1);
	bounds.left = RINT(inY1);
	bounds.bottom = RINT(inX2);
	bounds.right = RINT(inY2);

	RefVar	theShape(AllocateBinary(SYMA(line), sizeof(bounds)));
	CDataPtr shapeData(theShape);
	memmove((char *)shapeData, &bounds, sizeof(bounds));
	return theShape;
}


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
	*(Rect *)(char *)shapeData = bounds;
	return theShape;
}


Ref
FMakeRect(RefArg inRcvr, RefArg inLeft, RefArg inTop, RefArg inRight, RefArg inBottom)
{
	return MakeRectShape(SYMA(rectangle), inLeft, inTop, inRight, inBottom);
}


Ref
FMakeText(RefArg inRcvr, RefArg inStr, RefArg inLeft, RefArg inTop, RefArg inRight, RefArg inBottom)
{
	RefVar	theShape(Clone(RA(canonicalTextShape)));
	RefVar	it(MakeRectShape(SYMA(boundsRect), inLeft, inTop, inRight, inBottom));
	SetFrameSlot(theShape, SYMA(bounds), it);
	it = Clone(inStr);
	SetClass(it, SYMA(textData));
	SetFrameSlot(theShape, SYMA(data), it);
	return theShape;
}


Ref
FMakeTextBox(RefArg inRcvr, RefArg inStr, RefArg inLeft, RefArg inTop, RefArg inRight, RefArg inBottom)
{
	RefVar	theShape(Clone(RA(canonicalTextShape)));
	RefVar	it(MakeRectShape(SYMA(boundsRect), inLeft, inTop, inRight, inBottom));
	SetFrameSlot(theShape, SYMA(bounds), it);
	it = Clone(inStr);
	SetClass(it, SYMA(textBox));
	SetFrameSlot(theShape, SYMA(data), it);
	return theShape;
}


Ref
FMakeTextLines(RefArg inRcvr, RefArg inStr, RefArg inBox, RefArg inLineHeight, RefArg inFont)
{
	Rect			bounds;
	FromObject(inBox, &bounds);
	int			lineHt, numOfLines;
	CRichString str(inStr);		// sp30
	FontInfo		fntInfo;			// sp20
	StyleRecord	style;			// sp00

	CreateTextStyleRecord(inFont, &style);
	GetStyleFontInfo(&style, &fntInfo);
	if (NOTNIL(inLineHeight))
		lineHt = RINT(inLineHeight);
	else
		lineHt = fntInfo.ascent + fntInfo.descent + fntInfo.leading;
	numOfLines = RectGetHeight(bounds) / lineHt;
	if (numOfLines == 0)
		return NILREF;
/*
	RefVar	sp08(MakeArray(numOfLines));
	RefVar	sp04;
	int		sp00 = 0;
	for (ArrayIndex i = 0; ; ++i)
	{
		UniChar  unich = str.getChar(i); // r10
		if (IsWhitespace(unich) || unich == 0)
		{
			// sp-1C
			TextBoundsInfo	txBounds;
			MeasureRichString(sp68, r5, r4-r5, &sp38, sp1C, NULL, &txBounds);
			r8 = (sp14 + fixed1/2) & 0xFFFF;
			if (IsSpace(unich))
				if (r8 <= r9)
					;
			Substring();

			SetArraySlot(sp40, sp38++, FMakeText(RefArg inRcvr, RefArg inStr, RefArg inLeft, RefArg inTop, RefArg inRight, RefArg inBottom));
			if (r7 == 0)
			{
				SetLength(sp40, sp38);
				return sp40;
			}
			else if (sp38 == sp44)
			{
				return sp40;
			}
			else
			{
				UniChar  unich = str.getChar(i);
				while (IsWhitespace(unich) && unich != 0)
					i++;
				r5 = i;
				r7 = -1;
				r6 += sp48;
			}
		}
	}
*/
// big, but not too daunting

	return NILREF;
}


Ref
FMakeShape(RefArg inRcvr, RefArg inObj)
{
	RefVar	theShape;
	RefVar	objClass(ClassOf(inObj));
	Rect		bounds;

	if (EQ(objClass, SYMA(polygonShape)))
	{
		CDataPtr objData(inObj);
		newton_try
		{
			PolygonShape * poly = (PolygonShape *)(char *)objData;
			Point * src = poly->points;
			int numOfPoints = poly->size;
			bool isRect = (numOfPoints == 0);	// no points in the poly => must be rectangular
			if (isRect || numOfPoints == 10 || numOfPoints == 11)
			{
				theShape = AllocateBinary(isRect ? SYMA(rectangle) : SYMA(oval), sizeof(Rect));		// dunno how we can be so sure that 10/11 points => oval, but there you go
				CDataPtr rectData(theShape);
				Rect * rectPtr = (Rect *)(char *)rectData;
				// reset polygon bounds
				rectPtr->top = -32768;
				rectPtr->bottom = -32768;
				// iterate over polygon data, updating actual bounds
				for (ArrayIndex i = 0; i < numOfPoints; ++i, ++src)
				{
					UnionRect(rectPtr, *src);
				}
			}
			else
			{
				// create polygonData object
				ArrayIndex polySize = sizeof(PolygonShape) + numOfPoints * sizeof(Point);
				RefVar polyObj(AllocateBinary(SYMA(polygonData), polySize));
				// point to its data
				CDataPtr dataPts(polyObj);
				PolygonShape * polyData = (PolygonShape *)(char *)dataPts;
				polyData->size = polySize;
				// reset polygon bounds
				bounds.top = -32768;
				bounds.bottom = -32768;
				// copy polygon data, updating actual bounds
				Point * dst = polyData->points;
				for (ArrayIndex i = 0; i < numOfPoints; ++i, ++dst, ++src)
				{
					UnionRect(&bounds, *src);
					*dst = *src;
				}
				polyData->bBox = bounds;
				// create the shape
				theShape = Clone(RA(canonicalPolygonShape));
				SetFrameSlot(theShape, SYMA(data), polyObj);
			}
		}
		cleanup
		{
			objData.~CDataPtr();
		}
		end_try;
	}

	else if (EQ(objClass, SYMA(picture)) && IsBinary(inObj))
	{
		// extract bounds from PictureShape
		CDataPtr objData(inObj);
		PictureShape * picData = (PictureShape *)(char *)objData;
		memmove(&bounds, &picData->bBox, sizeof(Rect));

		// create boundsRect object from that Rect
		RefVar boundsObj(AllocateBinary(SYMA(boundsRect), sizeof(Rect)));
		CDataPtr boundsData(boundsObj);
		memmove((char *)boundsData, &bounds, sizeof(Rect));

		// create the shape
		theShape = Clone(RA(canonicalPictureShape));
		SetFrameSlot(theShape, SYMA(bounds), boundsObj);
		SetFrameSlot(theShape, SYMA(data), inObj);
	}

	else if (EQ(objClass, SYMA(frame)))
	{
		if (FrameHasSlot(inObj, SYMA(bits))
		||  FrameHasSlot(inObj, SYMA(colorData)))
		{
			if (!FromObject(GetProtoVariable(inObj, SYMA(bounds)), &bounds))
				ThrowErr(exGraf, -8801);
			RefVar boundsObj(AllocateBinary(SYMA(boundsRect), sizeof(Rect)));
			CDataPtr boundsData(boundsObj);
			memmove((char *)boundsData, &bounds, sizeof(Rect));
			theShape = Clone(RA(canonicalBitmapShape));
			SetFrameSlot(theShape, SYMA(colorData), GetFrameSlot(inObj, SYMA(colorData)));
			SetFrameSlot(theShape, SYMA(data), GetFrameSlot(inObj, SYMA(bits)));
			SetFrameSlot(theShape, SYMA(bounds), boundsObj);
			if (FrameHasSlot(inObj, SYMA(mask)))
				SetFrameSlot(theShape, SYMA(mask), GetFrameSlot(inObj, SYMA(mask)));
		}
	}

	else if (IsArray(inObj))
	{
		theShape = inObj;
	}

	else if (IsFrame(inObj) && FromObject(inObj, &bounds))
	{
		// create rectangle shape
		theShape = AllocateBinary(SYMA(rectangle), sizeof(Rect));
		CDataPtr shapeData(theShape);
		memmove((char *)shapeData, &bounds, sizeof(Rect));
	}

	else if (IsPrimShape(inObj))
	{
		theShape = inObj;
	}

	else
	{
		CView * theView = GetView(inObj);
		if (theView != NULL)
		{
			// create pict from view
			theView->outerBounds(&bounds);
			theShape = CommonMakePict(theView, bounds, RA(NILREF), RA(NILREF));
		}
	}
	
	return theShape;
}

#pragma mark -

Ref
CommonMakePict(CView * inView, Rect& inRect, RefArg inArg3, RefArg inArg4)
{
	return NILREF;
}


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
		pixmap->baseAddr = offsetof(SimplePattern, pat);	// actually an offset, obviously
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
	PatternHandle  pat = (PatternHandle) NewHandle(sizeof(SimplePattern));	// NOTE NewHandle deprecated
	if (pat)
	{
		PixelMap *  pixmap = *pat;
		pixmap->baseAddr = offsetof(SimplePattern, pat);	// actually an offset, obviously
		pixmap->rowBytes = 1;
		SetRect(&pixmap->bounds, 0, 0, 8, 8);
		pixmap->pixMapFlags = kPixMapOffset + kOneBitDepth;
		pixmap->deviceRes.h =
		pixmap->deviceRes.v = kDefaultDPI;
		pixmap->grayTable = NULL;
		memmove(&((SimplePattern*)pixmap)->pat, inBits, 8);
	}
	return pat;
}
#endif
