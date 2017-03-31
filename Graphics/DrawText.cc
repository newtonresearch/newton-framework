/*
	File:		DrawText.cc

	Contains:	Text drawing implementation.

	Written by:	Newton Research Group, 2007.
*/
#if 0
// text drawing function hierarchy

DrawRichString(CRichString & inStr, ULong inStart, size_t inLength, StyleRecord * inStyle, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * outBoundsInfo)
->	DoRichString(inStr, inStart, inLength, inStyle, inPt, inOptions, outBoundsInfo, true);

MeasureRichString(CRichString & inStr, ULong inStart, size_t inLength, StyleRecord * inStyle, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * outBoundsInfo)
->	DoRichString(inStr, inStart, inLength, inStyle, inPt, inOptions, outBoundsInfo, false);

	DoRichString(CRichString & inStr, ULong inStart, size_t inLength, StyleRecord * inStyle, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * outBoundsInfo, bool inDoDraw)
	->	DoTextOnce(theText, inLength, stylePtrs, runLengths, inPt, inOptions, outBoundsInfo, inDoDraw);


DrawTextOnce(void * inText, size_t inLength, StyleRecord ** inStyles, short * inRuns, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * outBoundsInfo)
->	DoTextOnce(inText, inLength, inStyles, inRuns, inPt, inOptions, outBoundsInfo, true);

MeasureTextOnce(void * inText, size_t inLength, StyleRecord ** inStyles, short * inRuns, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * outBoundsInfo)
->	DoTextOnce(inText, inLength, inStyles, inRuns, inPt, inOptions, outBoundsInfo, false);

	DoTextOnce(void * inText, size_t inLength, StyleRecord ** inStyles, short * inRuns, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * outBoundsInfo, bool inDoDraw)
	->	DrawTextObj
	->	DispatchCalcBounds


TextBox(CRichString & inStr, RefArg inFont, const Rect * inRect, ULong inJustifyH, ULong inJustifyV /*, long inTransferMode*/)
->	DrawSimpleParagraph(inStr, inFont, &box, inJustifyH, true/*, inTransferMode*/);

TextBounds(CRichString & inStr, RefArg inFont, Rect * ioRect, long inJustifyH)
->	DrawSimpleParagraph(inStr, inFont, ioRect, inJustifyH, false/*, 1*/);

	DrawSimpleParagraph(CRichString & inStr, RefArg inFont, Rect * ioRect, ULong inJustifyH, bool inDoDraw /*, long inTransferMode*/)
	uses line break table
	->	DrawSimpleLine(inStr, offset, &txLoc, &styleRecPtr, &txOpts, theBreaks, &lineWd, inDoDraw);

		DrawSimpleLine(CRichString & inStr, ULong inStart, FPoint * inLoc, StyleRecord ** inStyles, TextOptions * inOptions, RefArg inBreakTable, long * outLineWd, bool inDoDraw)
		creates QD TextObj
		->	GetTextObjBounds
		->	DrawTextObj

#endif


#include "Quartz.h"
#include <CoreText/CoreText.h>
#include "Geometry.h"
#include "Objects.h"
#include "DrawShape.h"
#include "DrawText.h"
#include "Ink.h"

#include "Globals.h"
#include "ROMResources.h"
#include "Lookup.h"
#include "Iterators.h"
#include "OSErrors.h"
#include "MagicPointers.h"
#include "Unicode.h"
#include "Strings.h"
#include "ViewFlags.h"
#include "Preference.h"
#include "UStringUtils.h"


void			DrawSimpleParagraph(CRichString & inStr, RefArg inFont, Rect * ioRect, ULong inJustifyH, bool inDoDraw /*, long inTransferMode*/);
ArrayIndex	DoRichString(CRichString & inStr, ULong inStart, size_t inCount, StyleRecord * inStyle, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * inBoundsInfo, bool inDoDraw);
ArrayIndex	DoTextOnce(void * inText, size_t inLength, StyleRecord ** inStyles, short * inRuns, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * outBoundsInfo, bool inDoDraw);
ArrayIndex	MeasureTextOnce(void * inText, size_t inLength, StyleRecord ** inStyles, short * inRuns, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * outBoundsInfo);


#pragma mark -
#pragma mark Styles
/*------------------------------------------------------------------------------
	S t y l e s
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Fill out a StyleRecord struct from a font spec Ref.
	Args:		inFontSpec
				outRec
	Return:	--
------------------------------------------------------------------------------*/

void
CreateTextStyleRecord(RefArg inFontSpec, StyleRecord * outRec)
{
	outRec->fontPattern = 0;

	if (ISINT(inFontSpec))
	{
		long  styleInt = RINT(inFontSpec);
		long  familyNum = styleInt & tsFamilyMask;
		outRec->fontFamily = (familyNum < Length(SYS_fonts)) ? GetArraySlot(SYS_fonts, familyNum) : NILREF;
		outRec->fontSize = (styleInt & tsSizeMask) >> tsSizeShift;
		outRec->fontFace = (styleInt & tsFaceMask) >> tsFaceShift;
	}
	else if (IsInkWord(inFontSpec))
	{
		InkWordInfo info;
		GetInkWordInfo(inFontSpec, &info);
		outRec->fontFamily = inFontSpec;
		outRec->fontSize = info.x20;
		outRec->fontFace = info.x10;
	}
	else if (IsFrame(inFontSpec))
	{
		RefVar	family(GetFrameSlot(inFontSpec, SYMA(family)));
		RefVar	fonts(GetGlobalVar(SYMA(fonts)));
		outRec->fontFamily = GetProtoVariable(fonts, family);
		outRec->fontSize = RINT(GetFrameSlot(inFontSpec, SYMA(size)));
		outRec->fontFace = RINT(GetFrameSlot(inFontSpec, SYMA(face)));
		RefVar	color(GetFrameSlot(inFontSpec, SYMA(color)));
		if (NOTNIL(color))
		{
/*			bool				sp00;
			PatternHandle  fontPattern;
			if (GetPattern(color, &sp00, &fontPattern, true))
			{
				outRec->x1C = fontPattern;
				outRec->fontPattern = AddressToRef(fontPattern);
			}
*/		}
	}

	if (ISNIL(outRec->fontFamily))
	{
		RefVar	userFont(GetPreference(SYMA(userFont)));
		if (ISINT(userFont))
		{
			long  familyNum = RINT(userFont) & tsFamilyMask;
			if (familyNum >= Length(SYS_fonts))
				familyNum = tsFancy;
			outRec->fontFamily = GetArraySlot(SYS_fonts, familyNum);
		}
		else  // MUST be a frame
		{
			RefVar	family(GetFrameSlot(userFont, SYMA(family)));
			RefVar	fonts(GetGlobalVar(SYMA(fonts)));
			outRec->fontFamily = GetProtoVariable(fonts, family);
		}
	}

	outRec->x10 = 0;
	outRec->x14 = 0;
	outRec->x18 = 0;
}


/*------------------------------------------------------------------------------
	Extract font info from a StyleRecord struct.
	Args:		inStyle
				outFontInfo
	Return:	--
------------------------------------------------------------------------------*/
struct FontEngineInfo
{
	long		x00;
	long		x04;
	long		x08;
	
	float		x3C;
	float		x40;

//				x80;		// has a desctructor

	RefStruct	xC0;
// size+C4
};

void
GetStyleFontInfo(StyleRecord * inStyle, FontInfo * outFontInfo)
{
#if defined(correct)
	FontEngineInfo info;
	OpenFont(GetCurrentPort()->portBits, inStyle, 1.0, 1.0, &info);
	if (info.x40 != 1.0)
	{
		float r5 = info.x00 * info.x40;
		float r6 = info.x04 * info.x40;
		float r0 = info.x08 * info.x40;
		outFontInfo->ascent = r5 + 0.5;
		outFontInfo->descent = r6 + 0.5;
		outFontInfo->leading = r0 + 0.5;
	}
	else
	{
		outFontInfo->ascent = info.x00;
		outFontInfo->descent = info.x04;
		outFontInfo->leading = info.x08;
	}
	if (info.x3C != 1.0)
		outFontInfo->widMax = info.x00 * info.x3C + 0.5;
	else
		outFontInfo->widMax = info.x0C;

#else
	outFontInfo->ascent = 8;
	outFontInfo->descent = 2;
	outFontInfo->leading = 3;
	outFontInfo->widMax = 10;
#endif
}


/*------------------------------------------------------------------------------
	Convert view justification flags to flush|justification values.
	Args:		inViewJustify
				outJustification	0.0 => flush
										1.0 => full
	Return:	flushness			0.0 => left
										0.5 => centre
										1.0 => right
------------------------------------------------------------------------------*/

float
ConvertToFlush(ULong inViewJustify, float * outJustification)
{
	*outJustification = 0.0;
	switch(inViewJustify & vjHMask)
	{
	case vjFullH:
		*outJustification = 1.0;
	case vjLeftH:
		return 0.0;
	case vjRightH:
		return 1.0;
	case vjCenterH:
		return 0.5;
	}
	return 0.0;		// keeps the compiiler quiet
}


#pragma mark -
#pragma mark Drawing
/*------------------------------------------------------------------------------
	D r a w i n g
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Draw a string of text.
	Args:		inText				array of UniChar
				inLength				number of UniChars to render
				inStyles				array of pointers to style records
				inRuns				array of run lengths matching styles
				inPt					location of text
				inOptions			
				outBoundsInfo		
	Return:	--
------------------------------------------------------------------------------*/

void
DrawTextOnce(void * inText, size_t inLength, StyleRecord ** inStyles, short * inRuns, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * outBoundsInfo)
{
	DoTextOnce(inText, inLength, inStyles, inRuns, inPt, inOptions, outBoundsInfo, true);
}


ArrayIndex
MeasureOnce(UniChar * inText, size_t inLength, StyleRecord * inStyle)
{
	TextBoundsInfo txBounds;
	MeasureTextOnce(inText, inLength, &inStyle, NULL, gZeroFPoint, NULL, &txBounds);
	return txBounds.width;	// float -> ArrayIndex
}

ArrayIndex
MeasureTextOnce(void * inText, size_t inLength, StyleRecord ** inStyles, short * inRuns, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * outBoundsInfo)
{
	return DoTextOnce(inText, inLength, inStyles, inRuns, inPt, inOptions, outBoundsInfo, false);
}


/*------------------------------------------------------------------------------
	Draw a rich string of text.
	Args:		inStr					rich string
				inStart				offset to start of text
				inLength				length of text
				inStyle				
				inPt					•baseline• of text
				inOptions			
				outBoundsInfo		
	Return:	--
------------------------------------------------------------------------------*/

void
DrawRichString(CRichString & inStr, ULong inStart, size_t inLength, StyleRecord * inStyle, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * outBoundsInfo)
{
	DoRichString(inStr, inStart, inLength, inStyle, inPt, inOptions, outBoundsInfo, true);
}

ArrayIndex
MeasureRichString(CRichString & inStr, ULong inStart, size_t inLength, StyleRecord * inStyle, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * outBoundsInfo)
{
	return DoRichString(inStr, inStart, inLength, inStyle, inPt, inOptions, outBoundsInfo, false);
}


/*------------------------------------------------------------------------------
	Draw text.
	Args:		inStr					a rich string
				inStart				offset to start of text
				inLength				length of text
				inStyle				
				inPt					location of text
				inOptions			
				outBoundsInfo		
				inDoDraw				false => do measure
	Return:	--
------------------------------------------------------------------------------*/

ArrayIndex
DoRichString(CRichString & inStr, ULong inStart, size_t inLength, StyleRecord * inStyle, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * outBoundsInfo, bool inDoDraw)
{
	ArrayIndex	strLen;
	//sp-14
//	bool sp0C = inDoDraw;
	UniChar *	theText = inStr.grabPtr() + inStart;	// sp08
//	short *	sp04 = NULL;

	if (inStr.format() == 0)
	{
		strLen = DoTextOnce(theText, inLength, &inStyle, NULL, inPt, inOptions, outBoundsInfo, inDoDraw);
	}

	else
	{
		Ptr *				runData, * runDataPtr;			// r6, r8
		short *			runLengths = NULL;							// r7
		StyleRecord **	stylePtrs, ** stylePtrPtr;		// r5, r10
		StyleRecord *	styleRecs,  * styleRecPtr;		// sp24r, r4
		//sp-04
		ArrayIndex		numOfInkWords = inStr.numInkWordsInRange(inStart, inLength);		// sp00r
		if (numOfInkWords != 0)
		{
			size_t	numOfRuns = inStr.numInkAndTextRunsInRange(inStart, inLength);	// r9
			runData = new Ptr[numOfRuns];
			runLengths = new short[numOfRuns];
			inStr.getLengthsAndDataInRange(inStart, inLength, runLengths, runData);
			//sp-20
			StyleRecord	theStyle = *inStyle;
			styleRecs/*sp24r*/ = new StyleRecord[numOfRuns];
			stylePtrs = new StyleRecord*[numOfRuns];
			//sp-04
		//	RefVar	sp00r;
			Ptr	dataPtr;
			for (styleRecPtr = styleRecs, stylePtrPtr = stylePtrs, runDataPtr = runData;
				  numOfRuns != 0;
				  numOfRuns--, styleRecPtr++, stylePtrPtr++, runDataPtr++)
			{
				if ((dataPtr = *runDataPtr) != 0)
					styleRecPtr->fontFamily = AddressToRef(dataPtr);
				else
					styleRecPtr->fontFamily = theStyle.fontFamily;
				styleRecPtr->fontSize = theStyle.fontSize;
				styleRecPtr->fontFace = theStyle.fontFace;
				styleRecPtr->fontPattern = theStyle.fontPattern;
				styleRecPtr->x14 = theStyle.x14;
				styleRecPtr->x18 = theStyle.x18;
				*stylePtrPtr = styleRecPtr;
			}
		}
		else
			stylePtrs = &inStyle;
		strLen = DoTextOnce(theText, inLength, stylePtrs, runLengths, inPt, inOptions, outBoundsInfo, inDoDraw);
		if (numOfInkWords != 0)
		{
			delete[] styleRecs;
			delete[] stylePtrs;
			delete[] runLengths;
			delete runData;
		}
	}

	inStr.releasePtr();
	return strLen;
}

void
DrawUnicodeText(const UniChar * inStr, size_t inLength, /* inFont,*/ const Rect * inBox, CGColorRef inColor, ULong inJustify);	// SRB

ArrayIndex
DoTextOnce(void * inText, size_t inLength, StyleRecord ** inStyles, short * inRuns, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * outBoundsInfo, bool inDoDraw)
{
	if (inDoDraw)
	{
#if 1
		Rect box;
		box.top = inPt.y-10;
		box.left = inPt.x;
		box.bottom = inPt.y;
		box.right = inPt.x + inOptions->width;

		ULong	justifyH = vjLeftH;
		if (inOptions->justification == fract1/2)
			justifyH = vjCenterH;
		else if (inOptions->justification == fract1)
			justifyH = vjRightH;

		DrawUnicodeText((const UniChar *)inText, inLength, &box, gBlackColor, justifyH);
#else
		char str[256];
		int strLen = MIN(inLength, 255);
		ConvertFromUnicode((const UniChar *)inText, str, strLen);

		StyleRecord * defaultStyle = inStyles[0];

//		CGContextSetGrayStrokeColor(quartz, 0.7, 1.0);
		CGContextSelectFont(quartz, "Helvetica", 0.8, kCGEncodingMacRoman);
		CGContextShowTextAtPoint(quartz, inPt.x, gScreenHeight-inPt.y, str, inLength);
		CGContextFlush(quartz);
#if 1
		printf("%s @ %f,%f\n", str, inPt.x, gScreenHeight-inPt.y);
#endif
#endif
	}

	if (outBoundsInfo)
		outBoundsInfo->width = 16.0;	// gotta return something
//		DispatchCalcBounds(textRef, outBoundsInfo);

	return 0;
}


void
DrawUnicodeText(const UniChar * inStr, size_t inLength, /* inFont,*/ const Rect * inBox, CGColorRef inColor, ULong inJustify)	// SRB
{

	CGRect box = MakeCGRect(*inBox);
	if (box.size.width < 0.0)
		box.size.width = -box.size.width;
//printf("[%.1f,%.1f, %.1f,%.1f]-%d ",box.origin.x,box.origin.y,box.size.width,box.size.height, inJustify & vjHMask);

//	box.size.width += 40;
	box.origin.y -= 8;
	box.size.height += 8;

//	fill the text box so we can see where it is
#if 0
	CGContextSaveGState(quartz);
	CGContextSetFillColorWithColor(quartz, CGColorCreateGenericRGB(1.0, 0.85, 0.85, 1.0));
	CGContextFillRect(quartz, box);
	CGContextRestoreGState(quartz);
#endif

	CGContextSetTextMatrix(quartz, CGAffineTransformIdentity);
	CGContextSetTextDrawingMode(quartz, kCGTextStroke);


//	CGContextSetAllowsAntialiasing(quartz, false);

//	CGContextSetAllowsFontSmoothing(quartz, true);
//	CGContextSetShouldSmoothFonts(quartz, false);	//improves draw quality of text

//	CGContextSetAllowsFontSubpixelPositioning(quartz, true);
//	CGContextSetShouldSubpixelPositionFonts(quartz, true);

//	CGContextSetAllowsFontSubpixelQuantization(quartz, true);
//	CGContextSetShouldSubpixelQuantizeFonts(quartz, true);


//	CGContextSelectFont(quartz, "Helvetica", 10.0, kCGEncodingMacRoman);

	// create font -- fixed for now
	CTFontRef font = CTFontCreateWithName(CFSTR("Helvetica"), 10.0, NULL);	// CTFontCreateUIFontForLanguage(kCTFontMiniSystemFontType, 0.0, NULL);

	//	inJustifyH -> CTParagraphStyleSetting
	//    create paragraph style and assign text alignment to it
	CTTextAlignment alignment = inJustify & vjHMask;
	CTParagraphStyleSetting _settings[] = { { kCTParagraphStyleSpecifierAlignment, sizeof(alignment), &alignment } };
	CTParagraphStyleRef paraStyle = CTParagraphStyleCreate(_settings, sizeof(_settings) / sizeof(_settings[0]));

	CFStringRef keys[] = { kCTFontAttributeName, kCTStrokeColorAttributeName, kCTParagraphStyleAttributeName };
	CFTypeRef values[] = { font, inColor, paraStyle };
	CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault, (const void **)&keys, (const void **)&values, sizeof(keys) / sizeof(keys[0]), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

	// Initialize an attributed string.
	CFStringRef string = CFStringCreateWithCharacters(kCFAllocatorDefault, inStr, inLength);
	CFAttributedStringRef attrStr = CFAttributedStringCreate(kCFAllocatorDefault, string, attributes);
	CFRelease(string);
	CFRelease(attributes);
	CFRelease(paraStyle);
	CFRelease(font);

#if 0
	CGContextSetTextPosition(quartz, box.origin.x, box.origin.y);

	char str[256];
	int strLen = MIN(inLength, 255);
	ConvertFromUnicode((const UniChar *)inStr, str, strLen);
	CGContextShowTextAtPoint(quartz, box.origin.x, box.origin.y, str, strLen);
/*
	CTLineRef line = CTLineCreateWithAttributedString(attrStr);
	CTLineDraw(line, quartz);
	CFRelease(line);
*/
#else
	CGMutablePathRef path = CGPathCreateMutable();
	CGPathAddRect(path, NULL, box);

	CGContextSetStrokeColorWithColor(quartz, inColor);

	// Create the framesetter with the attributed string.
	CTFramesetterRef framesetter = CTFramesetterCreateWithAttributedString(attrStr);
	CFRelease(attrStr);
	 
	// Create the frame and draw it into the graphics context
	CTFrameRef frame = CTFramesetterCreateFrame(framesetter, CFRangeMake(0, 0), path, NULL);
	CTFrameDraw(frame, quartz);
	CFRelease(frame);

	CFRelease(framesetter);
	CGPathRelease(path);
#endif

/*	char str[256];
	int strLen = MIN(inLength, 255);
	ConvertFromUnicode((const UniChar *)inStr, str, strLen);
	printf("%s @ %d,%d\n", str, inBox->left, inBox->bottom); */
}


/*------------------------------------------------------------------------------
	Draw text justified into a box.
	Args:		inStr					a rich string
				inFont				font frame
				inRect				the box into which to draw
				inJustifyH			justification
				inJustifyV
				inTransferMode		no longer used
	Return:	--
------------------------------------------------------------------------------*/

void
TextBox(CRichString & inStr, RefArg inFont, const Rect * inRect, ULong inJustifyH, ULong inJustifyV /*, long inTransferMode*/)
{
	Rect  box = *inRect;

	if (inJustifyV != vjTopV)
	{
		// calculate actual box size for justification
		box.bottom = box.top;
		DrawSimpleParagraph(inStr, inFont, &box, inJustifyH, false /*, 1*/);
		int  delta = RectGetHeight(*inRect) - RectGetHeight(box);
		box = *inRect;
		if (inJustifyV == vjCenterV)
			OffsetRect(&box, 0, delta/2);
		else if (inJustifyV == vjBottomV)
			OffsetRect(&box, 0, delta);
	}

	DrawSimpleParagraph(inStr, inFont, &box, inJustifyH, true /*, inTransferMode*/);
}


/*------------------------------------------------------------------------------
	Measure text bounds.
	Args:		inStr					a rich string
				inFont				font frame
				inRect				the box into which to draw
				inJustifyH			justification
	Return:	--
------------------------------------------------------------------------------*/

void
TextBounds(CRichString & inStr, RefArg inFont, Rect * ioRect, long inJustifyH)
{
	DrawSimpleParagraph(inStr, inFont, ioRect, inJustifyH, false/*, 1*/);
}


#pragma mark -
#pragma mark CoreText
/*------------------------------------------------------------------------------
	CoreText interface.
------------------------------------------------------------------------------*/

void
DrawSimpleParagraph(CRichString & inStr, RefArg inFont, Rect * ioRect, ULong inJustifyH, bool inDoDraw /*, long inTransferMode*/)
{
	// Initialize an attributed string.
	size_t strLen = inStr.length();
	CFStringRef string = CFStringCreateWithCharacters(kCFAllocatorDefault, inStr.grabPtr(), strLen);


	DrawUnicodeText(inStr.grabPtr(), strLen, /* inFont,*/ ioRect, gBlackColor, inJustifyH);

	// return path.size.height -> ioRect.bottom
}
