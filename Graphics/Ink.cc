/*
	File:		Ink.cc

	Contains:	Ink functions.

	Written by:	Newton Research Group, 2009.
*/

#include "Ink.h"
#include "QDGeometry.h"
#include "CoreGraphics/CGGeometry.h"
#include "ROMSymbols.h"


// from DrawInk.cc
struct CSStrokeHeader
{
	char	stream[];		// bit stream?
};
Ptr		CSCompress(CRecStroke ** inStrokes, int inArg2);
void		CSExpandGroup(CSStrokeHeader * inData, ULong inArg2, float inArg3, float inArg4);
void		CSRawExpandGroup(CSStrokeHeader * inData, ULong inArg2, float inArg3, float inArg4, float inArg5, float inArg6);
void		CSMakePathsGroup(CSStrokeHeader * inData, float inX, float inY);
void		CSMakePathsGroupInRect(CSStrokeHeader * inData, float inX, float inY, CGRect inRect);
void		CSDraw(CSStrokeHeader * inData, float inPenSize, CGPoint inLocation, bool inLive);
void		CSDrawInRect(CSStrokeHeader * inData, float inPenSize, CGSize inOriginalSize, CGRect inDisplayRect, bool inLive);


extern "C" {
Ref	FInkOff(RefArg rcvr, RefArg inUnit);
Ref	FInkOffUnHobbled(RefArg rcvr, RefArg inUnit);
Ref	FInkOn(RefArg rcvr, RefArg inUnit);
Ref	FInkConvert(RefArg rcvr, RefArg inObj, RefArg inType);
Ref	FStrokeBundleToInkWord(RefArg rcvr, RefArg inUnit);
}


/*------------------------------------------------------------------------------
	In NewtonOS 1, all ink is of class 'ink.
	In NewtonOS 2, sketches are of class 'ink2
		and ink words (that have recognition deferred) are of class 'inkWord.
------------------------------------------------------------------------------*/

bool
IsInk(RefArg inObj)
{
	if (IsBinary(inObj))
	{
		Ref objClass = ClassOf(inObj);
		return EQRef(objClass, RSYMink) || EQRef(objClass, RSYMinkWord) || EQRef(objClass, RSYMink2);
	}
	return NO;
}


bool
IsRawInk(RefArg inObj)
{
	if (IsBinary(inObj))
	{
		Ref objClass = ClassOf(inObj);
		return EQRef(objClass, RSYMink) || EQRef(objClass, RSYMink2);
	}
	return NO;
}


bool
IsOldRawInk(RefArg inObj)
{
	return IsBinary(inObj)
		 && EQRef(ClassOf(inObj), RSYMink);
}


bool
IsInkWord(RefArg inObj)
{
	return IsBinary(inObj)
		 && EQRef(ClassOf(inObj), RSYMinkWord);
}

#pragma mark Ink

/*------------------------------------------------------------------------------
	Adjust Rect bounds for thickness of ink pen.
	Don’t really know where best to put this.
#include "Objects.h"
#include "Preference.h"
#include "ROMSymbols.h"
#include "QDGeometry.h"
------------------------------------------------------------------------------*/

void
AdjustForInk(Rect * ioBounds)
{
#if 0
	int penSize = RINT(GetPreference(SYMA(userPenSize)));
#else
	int penSize = 2;
#endif
	InsetRect(ioBounds, -1, -1);
	ioBounds->bottom += penSize;
	ioBounds->right += penSize;
}


#pragma mark -

#if 0

void
SplitInkAt(RefArg inPoly, long inX, long inSlop)
{
	RefVar splitInk;
	Rect viewBounds;
	FromObject(GetProtoVariable(inPoly, SYMA(viewBounds)), &viewBounds);

	RefVar ink(GetProtoVariable(inPoly, SYMA(ink)));
	if (IsInkWord(ink))
	{
		InkWordInfo info;
		GetInkWordInfo(ink, &info);
		// scale inX position
		inX = viewBounds.left + info.x00 * (inX - viewBounds.left) / (viewBounds.right - viewBounds.left);

		CRecStroke ** inkStrokes = GetPolyAsRecStrokes(inPoly, 0);
		if (inkStrokes)
		{
			size_t itsSize = GetPtrSize(inkStrokes);
			CRecStroke ** ink1Strokes = (CRecStroke **) NewPtrClear(itsSize);
			CRecStroke ** ink2Strokes = (CRecStroke **) NewPtrClear(itsSize);
			newton_try
			{
				int slopLeft = inX - inSlop;
				int slopRight = inX + inSlop;
				CRecStroke ** p1 = ink1Strokes;
				CRecStroke ** p2 = ink2Strokes;
				CRecStroke ** iter, * strok;
				for (iter = inkStrokes; (strok = *iter) != NULL; iter++)
				{
					int strokeLeft = strok->fBBox.left + 0.5;
					int strokeRight = strok->fBBox.right + 0.5;
					if (strokeLeft < slopRight && strokeRight > slopLeft)
						// stroke is entirely within the slop -- don’t know how to assign it
						break;
					strok->clone();
					if (inX > (strokeLeft + strokeRight)/2)
						*p1++ = strok;
					else
						*p2++ = strok;
				}
				if (strok == NULL
				&&  p1 != ink1Strokes && p2 != ink2Strokes)
				{
					splitInk = MakeArray(2);

					RefVar poly(MakeInkWordPoly(ink1Strokes));
					RefVar inkWord(GetFrameSlot(poly, SYMA(ink)));
					SetInkWordScale(inkWord, info.x14);
					SetInkWordPenSize(inkWord, info.x18);
					SetArraySlot(splitInk, 0, poly);

					poly = MakeInkWordPoly(ink2Strokes);
					inkWord = GetFrameSlot(poly, SYMA(ink)));
					SetInkWordScale(inkWord, info.x14);
					SetInkWordPenSize(inkWord, info.x18);
					SetArraySlot(splitInk, 1, poly);
				}
			}
			newton_catch(exRoot)
			{ }
			end_try;
			DisposeRecStrokes(inkStrokes);
			DisposeRecStrokes(ink2Strokes);
			DisposeRecStrokes(ink1Strokes);
		}
	}
	return splitInk;
}


Ref
MergeInk(RefArg inPoly1, RefArg inPoly2)
{
	RefVar mergedInk;
	RefVar ink1(GetProtoVariable(inPoly1, SYMA(ink)));
	RefVar ink2(GetProtoVariable(inPoly2, SYMA(ink)));
	if (IsInkWord(ink1) && IsInkWord(ink2))
	{
		InkWordInfo info;
		GetInkWordInfo(ink1, &info);

		CRecStroke ** ink1Strokes = GetPolyAsRecStrokes(inPoly1, 0);
		CRecStroke ** ink2Strokes = GetPolyAsRecStrokes(inPoly2, 0);
		CRecStroke ** mergedStrokes = (CRecStroke **) NewPtrClear((CountRecStrokes(ink1Strokes) + CountRecStrokes(ink2Strokes) + 1) * sizeof(CRecStroke*));
		if (ink1Strokes && ink2Strokes && mergedStrokes)
		{
			Rect ink1Bounds;
			Rect ink2Bounds;
			InkBounds(ink1Strokes, &ink1Bounds);
			InkBounds(ink2Strokes, &ink2Bounds);
			OffsetStrokes(ink2Strokes, ink1Bounds.right - ink2Bounds.left, 0.0);
			
			CRecStroke ** p = mergedStrokes;
			CRecStroke ** iter, * strok;
			for (iter = ink1Strokes; (strok = *iter) != NULL; iter++, p++)
				*p = strok;
			for (iter = ink2Strokes; (strok = *iter) != NULL; iter++, p++)
				*p = strok;

			mergedInk = MakeInkWordPoly(mergedStrokes);
			RefVar mergedInkWord(GetFrameSlot(mergedInk, SYMA(ink)));
			SetInkWordScale(mergedInkWord, info.x14);
			SetInkWordPenSize(mergedInkWord, info.x18);
		}
		DisposeRecStrokes(ink1Strokes);
		DisposeRecStrokes(ink2Strokes);
		if (mergedStrokes)
			mergedStrokes[0] = NULL;
		DisposeRecStrokes(mergedStrokes);
	}
	return mergedInk;
}


Ref
MakeInkPoly(CRecStroke ** inStrokes)
{
	Rect bounds;
	RefVar ink(RecStrokesToInk(inStrokes, &bounds));
	RefVar poly(MakePolygonForm(NULL, 0, 14, &bounds, RINT(GetPreference(SYMA(userPenSize)))));
	SetFrameSlot(poly, SYMA(ink), ink);
	return poly;
}


Ref
MakeInkWordPoly(CRecStroke ** inStrokes)
{
	Rect bounds;
	RefVar ink(RecStrokesToInk(inStrokes, &bounds));
	InkWordInfo info;
	GetInkWordInfo(ink, &info);
	bounds.right = bounds.left + info.x00 + info.x18;
	bounds.bottom = bounds.top + info.x04 + info.x08 + info.x18;
	RefVar poly(MakePolygonForm(NULL, 0, 14, &bounds, info.x18));
	SetFrameSlot(poly, SYMA(ink), ink);
	return poly;
}


CRecStroke **
GetPolyAsRecStrokes(RefArg inPoly, ULong inPenSize)
{
	CRecStroke ** strokes = NULL;
	RefVar ink(GetProtoVariable(inPoly, SYMA(ink)));
	if (NOTNIL(ink))
	{
		RefVar bounds(GetProtoVariable(inPoly, SYMA(viewBounds)));
		strokes = InkExpand(ink, inPenSize, RINT(GetFrameSlot(bounds, SYMA(left))), RINT(GetFrameSlot(bounds, SYMA(top))));
	}
	return strokes;
}


CRecStroke **
StrokeBundleToRecStrokes(RefArg inUnit)
{
	RefVar strokes(GetFrameSlot(inUnit, SYMA(strokes)));
	ArrayIndex count = Length(strokes);
	CRecStroke ** recStrokes = (CRecStroke **) NewPtrClear((count+1)*sizeof(CRecStroke *));
	XTRY
	{
		XFAIL(recStrokes == NULL)
		for (ArrayIndex i = 0; i < count; ++i)
		{
			RefVar pts(GetArraySlot(strokes, i));
			CDataPtr ptsData(sp00);
			Point * srcPtr = (Point *)(char *)ptsData;
			int j, numOfPts = Length(pts) / sizeof(Point);
			CRecStroke * aStroke = CRecStroke::make(numOfPts);
			XFAILNOT(aStroke, DisposeRecStrokes(recStrokes); recStrokes = NULL;)
			// copy points to the stroke
			SamplePt * dstPtr = aStroke->getPoint(0);
			for (j = 0; j < numOfPts; j++, srcPtr++, dstPtr++)
			{
				FPoint pt;
				pt.x = srcPtr->h * kTabScale;
				pt.y = srcPtr->v * kTabScale;	// points in stroke bundle have tablet resolution
				SetPoint(dstPtr, &pt);
			}
			aStroke->updateBBox();
			aStroke->endStroke();
			recStrokes[i] = aStroke;
		}
		recStrokes[i] = NULL;
	}
	XENDTRY;
	return recStrokes;
}



/*------------------------------------------------------------------------------
	Compress strokes into an ink object.
	Args:		inStrokes		array of pointers to strokes
				inWords			YES => treat as ink word
	Return:	ink object of class 'ink2, or 'inkWord as appropriate
------------------------------------------------------------------------------*/

Ref
InkCompress(CRecStroke ** inStrokes, bool inWords)
{
	RefVar inkObj;
	Ptr inkData = CSCompress(inStrokes, 3);
	if (inkData)
	{
		size_t inkDataSize = GetPtrSize(inkData);
		size_t inkObjSize = inkDataSize;
		RefVar objClass;
		PackedInkWordInfo info;
		if (inWords)
		{
			GetPackedInkWordInfoFromStrokes(inStrokes, &info);
			inkObjSize += sizeof(PackedInkWordInfo);
			objClass = RSYMinkWord;
		}
		else
			objClass = RSYMink2;
		inkObj = AllocateBinary(objClass, inkObjSize);
		CDataPtr inkObjData(inkObj);
		memmove((char *)inkObjData, inkData, inkDataSize);
		if (inWords)
			memmove((char *)inkObjData + inkObjSize - sizeof(PackedInkWordInfo), &info, sizeof(PackedInkWordInfo));
		FreePtr(inkData);
	}
	return inkObj;
}

#pragma mark Drawing

void
InkExpand(RefArg inkObj, size_t inPenSize, long inArg3, long inArg4)
{
	CSExpandGroup((CSStrokeHeader *)BinaryData(inkObj), inPenSize, inArg3, inArg4);	// args long -> Fixed
}


void
InkMakePaths(RefArg inkObj, long inX, long inY)
{
	CSMakePathsGroup((CSStrokeHeader *)BinaryData(inkObj), inX, inY);				// args long -> Fixed
}
#endif

void
InkDraw(RefArg inkObj, size_t inPenSize, long inX, long inY, bool inLive)
{
#if !defined(forNTK)
	CSDraw((CSStrokeHeader *)BinaryData(inkObj), inPenSize, CGPointMake(inX, inY), inLive);
#endif
}

extern CGRect MakeCGRect(Rect inRect);

void
InkDrawInRect(RefArg inkObj, size_t inPenSize, Rect * inOriginalBounds, Rect * inBounds, bool inLive)
{
#if !defined(forNTK)
	CSDrawInRect((CSStrokeHeader *)BinaryData(inkObj), inPenSize, CGSizeMake(RectGetWidth(*inOriginalBounds), RectGetHeight(*inOriginalBounds)), MakeCGRect(*inBounds), inLive);
#endif
}


#pragma mark Ink Words
/*--------------------------------------------------------------------------------
	I n k   W o r d s
--------------------------------------------------------------------------------*/

ULong
GetQDFace(ULong inRawFace)
{
	return (inRawFace & 0x0F) | ((inRawFace & 0x30) << 3);
}


ULong
GetRawFace(ULong inQDFace)
{
	return (inQDFace & 0x0F) | ((inQDFace & 0x0180) >> 3);
}


void
PackInkWordInfo(PackedInkWordInfo * outInfo, ULong inArg2, ULong inArg3, ULong inArg4, ULong inArg5, long inArg6, ULong inArg7, ULong inArg8)
{
//	outInfo->x04 = (inArg5 << 22) | ((inArg6 & 0x00FFFF00) >> 2) | GetRawFace(inArg7);	// size | family | face?
	outInfo->x0422 = inArg5;
	outInfo->x0406 = inArg6;
	outInfo->x0400 = GetRawFace(inArg7);

//	outInfo->x00 = (inArg2 << 22) | (inArg3 << 12) | (inArg4 << 2) | (inArg8 - 1);
	outInfo->x0022 = inArg2;
	outInfo->x0012 = inArg3;
	outInfo->x0002 = inArg4;
	outInfo->x0000 = inArg8 - 1;
}


ULong
GetInkWordFontSize(ULong inSize)
{
	return (inSize * 1.75) + 0.5;
}


ULong
GetStdInkWordPenWidth(ULong inSize)
{
	if (inSize > 10)
		return inSize / 40 + 2;
	return 1;
}

void
ExpandPackedInkWordInfo(PackedInkWordInfo * inPackedInfo, InkWordInfo * outInfo)
{
#if 0
//	0DC1500D 02004000
//  037 015 000 01  008 0100 00
	outInfo->x00 = inPackedInfo->x0022;
	outInfo->x04 = inPackedInfo->x0012;
	outInfo->x08 = inPackedInfo->x0002;
	outInfo->x18 = inPackedInfo->x0000 + 1;

	outInfo->x0C = inPackedInfo->x0422;
	outInfo->x14 = inPackedInfo->x0406 / 256.0;	// x0406 is short Fixed?
	outInfo->x10 = GetQDFace(inPackedInfo->x0400);
	outInfo->x1C = GetInkWordFontSize(outInfo->x0C);
	outInfo->x20 =	(float)outInfo->x1C * outInfo->x14;	// FixedToInt(FixedMultiply(IntToFixed(outInfo->x1C), outInfo->x14));	// size
	outInfo->x24 = GetStdInkWordPenWidth(outInfo->x20);
	outInfo->x28 = outInfo->x24 + (float)outInfo->x00 * outInfo->x14;	// FixedToInt(FixedMultiply(IntToFixed(outInfo->x00), outInfo->x14));
	outInfo->x2C = outInfo->x24 + ((float)outInfo->x04 + (float)outInfo->x08) * outInfo->x14;	// FixedToInt(FixedMultiply(IntToFixed(outInfo->x04) + IntToFixed(outInfo->x08), outInfo->x14));
	outInfo->x30 = outInfo->x24 + (float)outInfo->x04 * outInfo->x14;	// FixedToInt(FixedMultiply(IntToFixed(outInfo->x04), outInfo->x14));
	outInfo->x34 = (float)outInfo->x0C * outInfo->x14;	// FixedToInt(FixedMultiply(IntToFixed(outInfo->x0C), outInfo->x14));
	outInfo->x38 = (float)outInfo->x08 * outInfo->x14;	// FixedToInt(FixedMultiply(IntToFixed(outInfo->x08), outInfo->x14));
#endif
}

void
GetPackedInkWordInfo(RefArg inObj, PackedInkWordInfo * outInfo)
{
	CDataPtr inkData(inObj);
	memmove(outInfo, (char *)inkData + Length(inObj) - sizeof(PackedInkWordInfo), sizeof(PackedInkWordInfo));
}

void
SetPackedInkWordInfo(RefArg outObj, PackedInkWordInfo * info)
{
	CDataPtr inkData(outObj);
	memmove((char *)inkData + Length(outObj) - sizeof(PackedInkWordInfo), info, sizeof(PackedInkWordInfo));
}

void
GetInkWordInfo(RefArg inObj, InkWordInfo * outInfo)
{
	PackedInkWordInfo packedInfo;
	GetPackedInkWordInfo(inObj, &packedInfo);
	ExpandPackedInkWordInfo(&packedInfo, outInfo);
}

#if 0
void
ScaleStrokesForInkWord(CRecStroke ** inStrokes, Rect * ioBox)
{
	float r6 = 0.0;
	float scaleFactor = 0.0;
	float wd = ioBox->right - ioBox->left;		// r9
	float ht = ioBox->bottom - ioBox->top;		// r7
	if (wd > 240.0)
		r6 = wd / 240.0;
	if (ht > 60.0)
		scaleFactor = ht / 60.0;
	if (r6 != 0.0)
	{
		if (scaleFactor == 0.0 || scaleFactor >= r6)
			scaleFactor = r6;
	}
	if (scaleFactor != 0.0)
	{
		FRect box, scaledBox;
		FixRect(&box, ioBox);
		CRecStroke ** iter, * strok;
		for (iter = inStrokes; (strok = *iter) != NULL; iter++)
		{
			scaledBox.left = box.left + (strok->fBBox.left - box.left) * scaleFactor;
			scaledBox.top = box.top + (strok->fBBox.top - box.top) * scaleFactor;
			scaledBox.right = box.right + (strok->fBBox.right - box.right) * scaleFactor;
			scaledBox.bottom = box.bottom + (strok->fBBox.bottom - box.bottom) * scaleFactor;
			strok->map(&scaledBox);
		}
		ioBox->right = ioBox->left + wd * scaleFactor + 1;
		ioBox->bottom = ioBox->top + ht * scaleFactor + 1;
	}
}


Ref
RecStrokesToInkWord(CRecStroke ** inStrokes, Rect * outBounds)
{
	Rect bounds;
	UnionBounds(inStrokes, &bounds);
	ScaleStrokesForInkWord(inStrokes, &bounds);
	OffsetStrokes(inStrokes, -(bounds.left-2), -(bounds.top-2));
	InsetRect(bounds, -2, -2);
	if (outBounds)
	{
		*outBounds = bounds;
		size_t penSize = RINT(GetPreference(SYMA(userPenSize)));
		outBounds->bottom += penSize;
		outBounds->right += penSize;
	}

	RefVar ink(InkCompress(inStrokes, YES));
	if (ISNIL(ink))
		OutOfMemory();
	return ink;
}


Ref
RecStrokesToInk(CRecStroke ** inStrokes, Rect * outBounds)
{
	Rect bounds;
	UnionBounds(inStrokes, &bounds);
	OffsetStrokes(inStrokes, -(bounds.left-2), -(bounds.top-2));
	InsetRect(bounds, -2, -2);
	if (outBounds)
		*outBounds = bounds;

	RefVar ink(InkCompress(inStrokes, NO));
	if (ISNIL(ink))
		OutOfMemory();
	return ink;
}


Ref
StrokeBundleToInkWord(RefArg inUnit)
{
	RefVar ink(GetProtoVariable(inUnit, SYMA(inkWord)));
	if (ISNIL(ink))
	{
		CRecStroke ** strokes = StrokeBundleToRecStrokes(inUnit);
		ink = RecStrokesToInkWord(strokes, NULL);
		SetFrameSlot(inUnit, SYMA(inkWord), ink);
		DisposeRecStrokes(strokes);
	}
	return inUnit;
}

Ref
FStrokeBundleToInkWord(RefArg rcvr, RefArg inUnit)
{
	return StrokeBundleToInkWord(inUnit);
}


bool
HandleInk(CEditView * inView, RefArg inUnit)
{
	CRecStroke ** strokes = StrokeBundleToRecStrokes(inUnit);
	HandleInk(inView, strokes);
	DisposeRecStrokes(strokes);
	return YES;
}

void
HandleInk(CEditView * inView, CRecStroke ** inStrokes)
{}


VOID
AdjustInkWordXHeight(RefArg, unsigned char)
{}

void
GetPackedInkWordInfoFromStrokes(CRecStroke ** inStrokes, PackedInkWordInfo * outInfo)
{}


Ref
InkConvert(RefArg inObj, RefArg inType)
{
	//sp-04
	RefVar ink;
	if (IsInk(inObj))
	{
		int r5, r6;
		//sp-04
		Ref reqType = inType;
		if (EQRef(reqType, SYMink))
		{
			r5 = 0;
			r6 = 2;
		}
		else if (EQRef(reqType, SYMink2))
		{
			r5 = 1;
			r6 = 3;
		}
		else if (EQRef(reqType, SYMinkword))
		{
			r5 = 2;
			r6 = 3;
		}
		else if (IsOldRawInk(inObj))
		{
			r5 = 1;
			r6 = 3;
			reqType = SYMink2;
		}
		else
		{
			r5 = 0;
			r6 = 2;
			reqType = SYMink;
		}
		if (EQRef(ClassOf(inObj), reqType))
		{
			ink = Clone(inObj);
		}
		else
		{
			//sp-04
			size_t inkSize = Length(inObj);
			if (IsInkWord(inObj))
				inkSize -= sizeof(PackedInkWordInfo);
			char * inkData, sourceData = NewPtr(inkSize);
			memmove(sourceData, BinaryData(inObj), inkSize);
			inkData = sourceData;
			if (ConvertData(&inkData, inkSize, r6))
			{
				ink = AllocateBinary(reqType, r5 == 2 ? inkSize + sizeof(PackedInkWordInfo) : inkSize);
				memmove(inkData, BinaryData(ink), inkSize);
				if (inkData != sourceData)
					FreePtr(inkData);
				if (r5 == 2)
				{
					CRecStroke ** strokes = InkExpand(ink, 0, 0, 0);
					if (strokes)
					{
						//sp-08
						Rect bounds;
						UnionBounds(strokes, &bounds);
						ScaleStrokesForInkWord(strokes, &bounds);
						ink = InkCompress(strokes, YES);
						DisposeRecStrokes(strokes);
					}
					else
						ink = NILREF;
				}
			}
		}
	}
	return ink;
}


#pragma mark NewtonScript Interface
/*--------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
--------------------------------------------------------------------------------*/

Ref
FInkConvert(RefArg rcvr, RefArg inObj, RefArg inType)
{
	return InkConvert(inObj, inType);
}

#endif
