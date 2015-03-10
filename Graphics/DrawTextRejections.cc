
extern Ref		GetLocaleSlot(RefArg inTag);	// from Locales.cc
extern void		FindWordBreaks(const UniChar * inStr, size_t inStrLen, ULong inOffset, bool inLeadingEdge, RefArg inBreakTable, ULong * outStartOffset, ULong * outEndOffset);	// from Dates.cc
UniChar *		SkipUpToTwoSpacesAndCR(UniChar * inStr, UniChar * inStrEnd);

#pragma mark -
#pragma mark QDText

struct TextObj
{
	ULong				x00;
	ULong				x04;
	StyleRecord **	x08;
	short *			x0C;
	FPoint			x10;
	TextOptions *	x18;
	ULong				x1C;
	void *			x4C;
// size +50
};
typedef TextObj * TextObjPtr;
typedef long TextObjRef;
enum TextObjectField
{
	kTextObjectField1 = 1,
	kTextObjectField5 = 5
};

void	CallDrawText(TextObjRef inText, float inArg2, float inArg3);

void
GetTextObjField(TextObjRef inTextObj, TextObjectField inField, void * outValue)
{
	TextObjPtr txObj = (TextObjPtr) inTextObj;
	txObj->x1C &= ~0xFF00;
	switch (inField)
	{
	case 0:
		*(ULong *)outValue = txObj->x00;
		break;
	case 1:
		txObj->x4C = outValue;
		txObj->x1C |= 0x0100;
		CallDrawText(inTextObj, 1.0, 1.0);
		break;
	case 2:
		*(StyleRecord ***)outValue = txObj->x08;
		break;
	case 3:
		*(short **)outValue = txObj->x0C;
		break;
	case 4:
		*(FPoint *)outValue = txObj->x10;
		break;
	case 5:
		*(TextOptions **)outValue = txObj->x18;
		break;
	case 6:
//		DispatchCalcBounds(inTextObj, outValue);
		break;
	case 7:
		txObj->x4C = outValue;
		txObj->x1C |= 0x0400;
		CallDrawText(inTextObj, 1.0, 1.0);
		break;
	}
}

void	// original returns bool
SetTextObjField(TextObjRef inTextObj, TextObjectField inField, void * inValue)
{
	TextObjPtr txObj = (TextObjPtr) inTextObj;
	switch (inField)
	{
	case 0:
		txObj->x00 = *(ULong *)inValue;
		break;
	// INCOMPLETE
	}
}

void
GetTextObjBounds(TextObjRef inTextObj, Rect * outRect)
{}

TextObjRef
NewText(void * inText, ULong inLength, StyleRecord ** inStyles, short * inRunLengths, FPoint inLoc, TextOptions * inOptions)
{
	TextObjPtr txObj = (TextObjPtr)NewPtr(sizeof(txObj));		// was NewHandle
	if (txObj)
	{
		memset(txObj, 0, sizeof(txObj));
		txObj->x0C = inRunLengths;
		txObj->x08 = inStyles;
		txObj->x04 = inLength;
		txObj->x10 = inLoc;
		txObj->x18 = inOptions;
		txObj->x1C = 0x80000000;	// => handle was allocated
	}
	return (TextObjRef)txObj;
}

void
DisposeText(TextObjRef inTextObj)
{
	if (inTextObj)
	{
//		InvalCachedTextInfo(inTextObj);
		TextObjPtr txObj = (TextObjPtr)inTextObj;
		if ((txObj->x1C & 0x80000000) != 0)
			FreePtr(inTextObj);	// was DisposHandle
	}
}

void
CallDrawText(TextObjRef inText, float inArg2, float inArg3)		// Fixed -> float
{
#if 0
// r4 r6 r5
	TextObjProc txProc;
	GrafPtr grafPort = GetCurrentPort();
	if (grafPort->grafProcs == NULL)
		txProc = qdConstants.x34;
	else
	{
		TextObjPtr txObj = *(TextObjPtr *)inText;
		int r0 = txObj->x1C & 0x0000FF00;
		int deviceType = grafPort->portBits.pixMapFlags & kPixMapDeviceType;
		if (r0 == 0 || deviceType != kPixMapDevPSPrint)
			txProc = grafPort->grafProcs->TextObjProc;
		else
			txProc = qdConstants.x34;
	}
	txProc(inText, inArg2, inArg3);
// txProc -> StdText -> DoPutText (12 pages) checks IsInkWord
// , DrText -> DrTextChunk (26 pages)
#endif
}

void
DrawTextObj(TextObjRef inTextObj)
{
	TextObjPtr txObj = (TextObjPtr) inTextObj;	// was dereferencing Handle
	txObj->x1C &= ~0x0000FF00;
	CallDrawText(inTextObj, 1.0, 1.0);		// Fixed -> float
}


#pragma mark -

NewtonErr
DoTextOnce(void * inText, size_t inLength, StyleRecord ** inStyles, short * inRuns, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * outBoundsInfo, bool inDoDraw)
{
// r6=inText  r4=inLength  r5=inStyles  r3=inRuns  r8=inOptions  r10=outBoundsInfo  r9=inDoDraw  
// sp-54
	TextObj		sp04;
	TextObjPtr	sp00 = &sp04;
	TextObjRef	textRef = &sp00;	// r7
	memset(sp04, 0, 0x50);
	sp04.x04 = inLength;
	sp04.x08 = inStyles;
	sp04.x00 = inText;
	sp04.x0C = inRuns;
	sp04.x10 = inPt;

	if (inOptions)
	{
		if (inOptions->x14 == 9)
		{
			sp04.x1C |= 0x00040000;
		}
		else if (inOptions->x14 == 10)
		{
			sp04.x1C |= 0x00040000;
			inOptions = NULL;
		}
	}
	sp04.x18 = inOptions;

	if (inLength <= 128)
	{
		sp04.x28 = QDNewTempPtr(inLength*4);
		sp04.x20 = QDNewTempPtr(inLength*2);
		sp04.x1C |= 0x60000000;
		if (sp04.x28 == NULL)
		{
			DisposeText(textRef);
			return 0;
		}
	}
	sp04.x38 = 0x00010000;
	sp04.x3C = 0x00010000;
	if (inDoDraw)
		DrawTextObj(textRef);
	if (outBoundsInfo)
		DispatchCalcBounds(textRef, outBoundsInfo);
	DisposeText(textRef);
	return sp04.x04;
}


#pragma mark -
/*------------------------------------------------------------------------------
	Draw text justified into a box.
	Args:		inStr					a rich string
				inFont				font frame
				ioRect				the box into which to draw
				inJustifyH			justification
				inDoDraw				NO => just measure the text
				inTransferMode		no longer used
	Return:	--
------------------------------------------------------------------------------*/
#if 0
ULong DrawSimpleLine(CRichString & inStr, ULong inStart, FPoint * inLoc, StyleRecord ** inStyles, TextOptions * inOptions, RefArg inLineBreakTable, long * outLineWd, bool inDoDraw);

void
DrawSimpleParagraph(CRichString & inStr, RefArg inFont, Rect * ioRect, ULong inJustifyH, bool inDoDraw /*, long inTransferMode*/)
{
	int	reqWd = RectGetWidth(*ioRect);
	int	reqHt = RectGetHeight(*ioRect);
	float	paraWd = (reqWd > 0) ? (float)reqWd : 0.0;
	int	htLimit = (reqHt > 0) ? reqHt : 10000;

	if (inDoDraw || reqWd <= 0 || reqHt <= 0)
	{
//sp-20
		StyleRecord		styleRec;		// sp14
//sp-14
		StyleRecord *	styleRecPtr = &styleRec;	// sp10
		FontInfo			fontInfo;		// sp00
		CreateTextStyleRecord(inFont, &styleRec);
		GetStyleFontInfo(&styleRec, &fontInfo);
		int	lineHt = fontInfo.ascent + fontInfo.descent + fontInfo.leading;	// r8

//sp-08
		FPoint	txLoc;
		txLoc.x = ioRect->left;
		txLoc.y = ioRect->top + fontInfo.ascent;

//sp-1C
		TextOptions	txOpts;
		txOpts.alignment = ConvertToFlush(inJustifyH, &txOpts.justification);
		txOpts.width = paraWd;
		txOpts.x0C = 0;
//		txOpts.transferMode = inTransferMode;
		txOpts.x14 = 0;
		txOpts.x18 = 0;

//sp-10
		RefVar	theBreaks(GetLocaleSlot(SYMA(lineBreakTable)));
		long	actualHt = 0;	// r10
		long	actualWd = 0;	// r9
		long	lineWd = 0;		// sp08
		ULong	offset, count = inStr.length();	// r12, sp04
		for (offset = 0; offset < count && actualHt < htLimit; )
		{
			actualHt += lineHt;
			offset = DrawSimpleLine(inStr, offset, &txLoc, &styleRecPtr, &txOpts, theBreaks, &lineWd, inDoDraw);
			txLoc.y += lineHt;
			if (lineWd > actualWd)
				actualWd = lineWd;
		}
		if (reqHt <= 0)
			ioRect->bottom = ioRect->top + actualHt;
		if (reqWd <= 0)
			ioRect->right = ioRect->left + actualWd;
	}
}


ULong
DrawSimpleLine(CRichString & inStr, ULong inStart, FPoint * inLoc, StyleRecord ** inStyles, TextOptions * inOptions, RefArg inBreakTable, long * outLineWd, bool inDoDraw)
{
// r4 r5 r2 r10
//sp-0C
//	bool sp10 = inDoDraw;
//sp-10
	StyleRecord * defaultStyle = inStyles[0];	// r6
	UniChar * s, * str = inStr.grabPtr() + inStart;	// r7
//	Ptr sp0C;
	if (*str == 0x0D)
	{
		// trivial processing -- empty line
		*outLineWd = 0;
		inStr.releasePtr();
		return inStart + 1;
	}

	for (s = str+1; *s != 0 && *s != 0x0D; s++)
		/* scan to end of line/string */;
// sp-04
	ULong numOfChars = (s - str) / sizeof(UniChar);	// spm04

	Ptr *				runData, * runDataPtr;
	short *			runLengths;
	StyleRecord **	stylePtrs, ** stylePtrPtr;
	StyleRecord *	styleRecs,  * styleRecPtr;
	size_t numOfInkWords = 0;	// sp08 (moved)
	TextObjRef textRef;		// r6
	if (inStr.format() == 0)
	{
		textRef = NewText(str, numOfChars, inStyles, NULL, *inLoc, inOptions);
	}
	else
	{
//sp-04
		size_t strLen = inStr.length() - inStart;
		numOfInkWords = inStr.numInkWordsInRange(inStart, strLen);		// sp00r
		if (numOfInkWords == 0)
		{
			textRef = NewText(str, numOfChars, inStyles, NULL, *inLoc, inOptions);
		}
		else
		{
// sp-04
			size_t numOfRuns = inStr.numInkAndTextRunsInRange(inStart, strLen);	// sp00r
			runData = new Ptr[numOfRuns];	// r8 was QDNewTempPtr()
			if (runData == NULL)
			{
				inStr.releasePtr();
				OutOfMemory();
			}
			runLengths = new short[numOfRuns];	// r9 was QDNewTempPtr()
			if (runLengths == NULL)
			{
				delete runData;		// was QDDisposeTempPtr(runData)
				inStr.releasePtr();
				OutOfMemory();
			}
//			sp0C = r8;

			inStr.getLengthsAndDataInRange(inStart, strLen, runLengths, runData);
//sp-1C
			StyleRecord	style = *defaultStyle;
			styleRecs/*sp34r*/ = new StyleRecord[numOfRuns];	// also r6
			stylePtrs/*sp30r*/ = new StyleRecord*[numOfRuns];
// sp-08
		//	RefVar	sp00r;
			Ptr	dataPtr;
			for (styleRecPtr = styleRecs, stylePtrPtr = stylePtrs, runDataPtr = runData;
				  numOfRuns != 0;
				  numOfRuns--, styleRecPtr++, stylePtrPtr++, runDataPtr++)
			{
				if ((dataPtr = *runDataPtr) != 0)
					styleRecPtr->fontFamily = AddressToRef(dataPtr);
				else
					styleRecPtr->fontFamily = defaultStyle->fontFamily;
				styleRecPtr->fontSize = defaultStyle->fontSize;
				styleRecPtr->fontFace = defaultStyle->fontFace;
				styleRecPtr->fontPattern = defaultStyle->fontPattern;
				styleRecPtr->x14 = defaultStyle->x14;
				styleRecPtr->x18 = defaultStyle->x18;
				*stylePtrPtr = styleRecPtr;
			}
// sp-0C
			textRef = NewText(str, strLen, stylePtrs, runLengths, *inLoc, inOptions);
			// disposing StyleRecord?
		}
	}
//sp-04
	ULong spx;
	GetTextObjField(textRef, kTextObjectField1, &spx);
//sp-04
	ULong startOffset = spx;
	UniChar * r1 = str + startOffset - 1;
	if (startOffset < numOfChars && r1[0] != ' ' && r1[1] != ' ')
	{
//sp-04
		ULong endOffset;
		FindWordBreaks(str, startOffset - inStart, startOffset, YES, inBreakTable, &startOffset, &endOffset);
		if (startOffset == 0)
			startOffset = spx;
	}
	if (startOffset != spx)
		SetTextObjField(textRef, kTextObjectField1, &startOffset);
//sp-08
	Rect txBounds;
	GetTextObjBounds(textRef, &txBounds);
	*outLineWd = RectGetWidth(&txBounds);

	if (inDoDraw)
		DrawTextObj(textRef);

	DisposeText(textRef);
	if (numOfInkWords != 0)
	{
		delete[] styleRecs;
		delete stylePtrs;
		delete runData;
		delete runLengths;
	}

	str += startOffset;
	inStart += (SkipUpToTwoSpacesAndCR(str, str + Ustrlen(str)) - str) / sizeof(UniChar);
	inStr.releasePtr();

	return inStart;
}

UniChar *
SkipUpToTwoSpacesAndCR(UniChar * inStr, UniChar * inStrEnd)
{
	ULong numOfSpaces = 0;
	while (inStr < inStrEnd && *inStr == ' ' && ++numOfSpaces < 2)
		inStr++;
	if (inStr < inStrEnd && *inStr == 0x0D)
		inStr++;
	return inStr;
}
#endif


#pragma mark -
#pragma mark ATSU
#if 1
// ATSU text functions

void
CreateTextStyle(RefArg inFontSpec, ATSUStyle * outStyle)
{
	RefVar	fontFamily;
	int		fontSize;
	int		fontFace;
	const float * colorComponents = NULL;

	if (ISINT(inFontSpec))
	{
		long  styleInt = RINT(inFontSpec);
		long  familyNum = styleInt & tsFamilyMask;
		fontFamily = (familyNum < Length(SYS_fonts)) ? GetArraySlot(SYS_fonts, familyNum) : NILREF;
		fontSize = (styleInt & tsSizeMask) >> tsSizeShift;
		fontFace = (styleInt & tsFaceMask) >> tsFaceShift;
	}
	else if (IsInkWord(inFontSpec))
	{
		InkWordInfo info;
		GetInkWordInfo(inFontSpec, &info);
		fontFamily = inFontSpec;
		fontSize = info.x20;
		fontFace = info.x10;
	}
	else if (IsFrame(inFontSpec))
	{
		RefVar	fonts(GetFrameSlot(RA(gVarFrame), SYMA(fonts)));
		fontFamily = GetProtoVariable(fonts, GetFrameSlot(inFontSpec, SYMA(family)));
		fontSize = RINT(GetFrameSlot(inFontSpec, SYMA(size)));
		fontFace = RINT(GetFrameSlot(inFontSpec, SYMA(face)));
		RefVar	fontColor(GetFrameSlot(inFontSpec, SYMA(color)));
		if (NOTNIL(fontColor))
		{
			bool			isOurs;
			CGColorRef	color;
			if (GetPattern(fontColor, &isOurs, &color, YES))
			{
				colorComponents = CGColorGetComponents(color);
			}
		}
	}

	if (ISNIL(fontFamily))
	{
		RefVar	userFont(GetPreference(SYMA(userFont)));
		if (ISINT(userFont))
		{
			int  familyNum = RINT(userFont) & tsFamilyMask;
			if (familyNum >= Length(SYS_fonts))
				familyNum = tsFancy;
			fontFamily = GetArraySlot(SYS_fonts, familyNum);
		}
		else  // MUST be a frame
		{
			RefVar	fonts(GetFrameSlot(RA(gVarFrame), SYMA(fonts)));
			fontFamily = GetProtoVariable(fonts, GetFrameSlot(userFont, SYMA(family)));
		}
	}

	// set those values in the ATSUStyle
	ATSUAttributeTag			 attrTag[3];
	ByteCount					attrSize[3];
	ATSUAttributeValuePtr  attrValue[3];

	ATSUCreateStyle(outStyle);

	ATSUFontID	fontId;
	Fixed			fixedFontSize = IntToFixed(fontSize);
	Boolean		face = true;

	char			nameStr[64];
	RefVar		fontName(GetFrameSlot(fontFamily, MakeSymbol("fontName")));
	ConvertFromUnicode((UniChar *)BinaryData(fontName), nameStr);
	ATSUFindFontFromName(nameStr, strlen(nameStr), kFontFullName, kFontNoPlatform, kFontNoScript, kFontNoLanguage, &fontId);

	attrTag[0] = kATSUFontTag;				attrTag[1] = kATSUSizeTag;
	attrSize[0] = sizeof(ATSUFontID);	attrSize[1] = sizeof(Fixed);
	attrValue[0] = &fontId;					attrValue[1] = &fixedFontSize;

	ATSUSetAttributes(*outStyle, 2, attrTag, attrSize, attrValue);

	int numOfFaces = 0;
	if (fontFace & kBoldFace)
	{
		attrTag[numOfFaces] = kATSUQDBoldfaceTag;
		attrSize[numOfFaces] = sizeof(Boolean);
		attrValue[numOfFaces] = &face;
		numOfFaces++;
	}
	if (fontFace & kItalicFace)
	{
		attrTag[numOfFaces] = kATSUQDItalicTag;
		attrSize[numOfFaces] = sizeof(Boolean);
		attrValue[numOfFaces] = &face;
		numOfFaces++;
	}
	if (fontFace & kUnderlineFace)
	{
		attrTag[numOfFaces] = kATSUQDUnderlineTag;
		attrSize[numOfFaces] = sizeof(Boolean);
		attrValue[numOfFaces] = &face;
		numOfFaces++;
	}
	if (numOfFaces > 0)
		ATSUSetAttributes(*outStyle, numOfFaces, attrTag, attrSize, attrValue);

	if (colorComponents != NULL)
	{
		RGBColor	color;
		color.red = colorComponents[0];
		color.green = colorComponents[1];
		color.blue = colorComponents[2];
		attrTag[0] = kATSUColorTag;
		attrSize[0] = sizeof(RGBColor);
		attrValue[0] = &color;
		ATSUSetAttributes(*outStyle, 1, attrTag, attrSize, attrValue);
	}
}


void
CreateTextStyle(StyleRecord * inStyle, ATSUStyle * outStyle)
{
	// set StyleRecord -> ATSUStyle
	ATSUAttributeTag			 attrTag[3];
	ByteCount					attrSize[3];
	ATSUAttributeValuePtr  attrValue[3];

	ATSUCreateStyle(outStyle);

	ATSUFontID	fontId;
	Fixed			fixedFontSize = FloatToFixed(inStyle->fontSize);
	Boolean		face = true;

	char			nameStr[64];
	RefVar		fontName(GetFrameSlot(inStyle->fontFamily, MakeSymbol("fontName")));
	ConvertFromUnicode((UniChar *)BinaryData(fontName), nameStr);
	ATSUFindFontFromName(nameStr, strlen(nameStr), kFontFullName, kFontNoPlatform, kFontNoScript, kFontNoLanguage, &fontId);

	attrTag[0] = kATSUFontTag;				attrTag[1] = kATSUSizeTag;
	attrSize[0] = sizeof(ATSUFontID);	attrSize[1] = sizeof(Fixed);
	attrValue[0] = &fontId;					attrValue[1] = &fixedFontSize;

	ATSUSetAttributes(*outStyle, 2, attrTag, attrSize, attrValue);

	int numOfFaces = 0;
	if (inStyle->fontFace & kBoldFace)
	{
		attrTag[numOfFaces] = kATSUQDBoldfaceTag;
		attrSize[numOfFaces] = sizeof(Boolean);
		attrValue[numOfFaces] = &face;
		numOfFaces++;
	}
	if (inStyle->fontFace & kItalicFace)
	{
		attrTag[numOfFaces] = kATSUQDItalicTag;
		attrSize[numOfFaces] = sizeof(Boolean);
		attrValue[numOfFaces] = &face;
		numOfFaces++;
	}
	if (inStyle->fontFace & kUnderlineFace)
	{
		attrTag[numOfFaces] = kATSUQDUnderlineTag;
		attrSize[numOfFaces] = sizeof(Boolean);
		attrValue[numOfFaces] = &face;
		numOfFaces++;
	}
	if (numOfFaces > 0)
		ATSUSetAttributes(*outStyle, numOfFaces, attrTag, attrSize, attrValue);
}


// don’t think this is used any more
void
GetStyleFontInfo(ATSUStyle inStyle, FontInfo * outFontInfo)
{
	ATSUTextMeasurement	ascent, descent, leading;

	ATSUGetAttribute(inStyle, kATSUAscentTag, sizeof(ATSUTextMeasurement), &ascent, NULL);
	ATSUGetAttribute(inStyle, kATSUDescentTag, sizeof(ATSUTextMeasurement), &descent, NULL);
	ATSUGetAttribute(inStyle, kATSULeadingTag, sizeof(ATSUTextMeasurement), &leading, NULL);

	outFontInfo->ascent = FixedToInt(ascent);
	outFontInfo->descent = FixedToInt(descent);
	outFontInfo->leading = FixedToInt(leading);
}


Fract
ConvertToFlush(ULong inViewJustify, Fract * outJustification)
{
	*outJustification = 0;
	switch(inViewJustify & vjHMask)
	{
	case vjFullH:
		*outJustification = fract1;
	case vjLeftH:
		return 0;
	case vjRightH:
		return fract1;
	case vjCenterH:
		return fract1 >> 1;
	}
	return 0;		// keeps the compiiler quiet
}


NewtonErr
DoTextOnce(void * inText, size_t inLength, StyleRecord ** inStyles, short * inRuns, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * outBoundsInfo, bool inDoDraw)
{
	OSStatus	status;
	ATSUAttributeTag			 attrTag[2];
	ByteCount					attrSize[2];
	ATSUAttributeValuePtr  attrValue[2];

	// create text style
	ATSUStyle			theStyle;
	CreateTextStyle(inStyles[0], &theStyle);	// should do all runs

	// create text layout bound to the text
	ATSUTextLayout		theLayout;
	UniChar *			theText = (UniChar *)inText;

	UniCharCount length = kATSUToTextEnd;
	 
	status = ATSUCreateTextLayoutWithTextPtr(theText, kATSUFromTextBeginning, kATSUToTextEnd, inLength,	// text
															1, &length, &theStyle,													// styles
															&theLayout);

	Fixed		lineWidth = FloatToFixed(inOptions->width);
	Fract		justificationFactor = FloatToFract(inOptions->justification);
	Fract		flushFactor =  FloatToFract(inOptions->alignment);

	// make sure the layout knows the proper CGContext and line width
	attrTag[0] = kATSUCGContextTag;		attrTag[1] = kATSULineWidthTag;
	attrSize[0] = sizeof(CGContextRef);	attrSize[1] = sizeof(Fixed);
	attrValue[0] = &quartz;					attrValue[1] = &lineWidth;

	status = ATSUSetLayoutControls(theLayout, 2, attrTag, attrSize, attrValue);

	// also its flushness & justification
	attrTag[0] = kATSULineFlushFactorTag;	attrTag[1] = kATSULineJustificationFactorTag;
	attrSize[0] = sizeof(Fract);				attrSize[1] = sizeof(Fract);
	attrValue[0] = &flushFactor;				attrValue[1] = &justificationFactor;

	status = ATSUSetLayoutControls(theLayout, 2, attrTag, attrSize, attrValue);


	ATSUSetTextPointerLocation(theLayout, theText, kATSUFromTextBeginning, kATSUToTextEnd, inLength);

	ATSUStyle inStyle;
	ATSUSetRunStyle(theLayout, inStyle, kATSUFromTextBeginning, kATSUToTextEnd);

	ATSUDrawText(theLayout, 0, inLength, FloatToFixed(inPt.x), FloatToFixed((float)gScreenHeight - inPt.y));

	CGContextFlush(quartz);
	ATSUDisposeTextLayout(theLayout);
	ATSUDisposeStyle(theStyle);
}


void
DrawSimpleParagraph(CRichString & inStr, RefArg inFont, Rect * ioRect, ULong inJustifyH, bool inDoDraw)
{
	int	reqWd = RectGetWidth(*ioRect);
	int	reqHt = RectGetHeight(*ioRect);
	Fixed	actualHt = 0;
	Fixed	htLimit = IntToFixed((reqHt > 0) ? reqHt : 10000);
	FixedPoint	location = { IntToFixed(ioRect->left), IntToFixed(gScreenHeight - ioRect->top) };

	if (inDoDraw || reqWd <= 0 || reqHt <= 0)
	{
		OSStatus	status;
		ATSUAttributeTag			 attrTag[2];
		ByteCount					attrSize[2];
		ATSUAttributeValuePtr  attrValue[2];

		// create text style
		ATSUStyle			theStyle;
		CreateTextStyle(inFont, &theStyle);

		// create text layout bound to the text
		ATSUTextLayout		theLayout;
		UniChar *			theText = inStr.grabPtr();

		UniCharCount length = kATSUToTextEnd;
		 
		status = ATSUCreateTextLayoutWithTextPtr(theText, kATSUFromTextBeginning, kATSUToTextEnd, inStr.length(),	// text
																1, &length, &theStyle,															// styles
																&theLayout);

		Fixed		lineWidth = IntToFixed(reqWd);
		Fract		justificationFactor;
		Fract		flushFactor = ConvertToFlush(inJustifyH, &justificationFactor);

		// make sure the layout knows the proper CGContext and line width
		attrTag[0] = kATSUCGContextTag;		attrTag[1] = kATSULineWidthTag;
		attrSize[0] = sizeof(CGContextRef);	attrSize[1] = sizeof(Fixed);
		attrValue[0] = &quartz;					attrValue[1] = &lineWidth;

		status = ATSUSetLayoutControls(theLayout, 2, attrTag, attrSize, attrValue);
		
		// also its flushness & justification
		attrTag[0] = kATSULineFlushFactorTag;	attrTag[1] = kATSULineJustificationFactorTag;
		attrSize[0] = sizeof(Fract);				attrSize[1] = sizeof(Fract);
		attrValue[0] = &flushFactor;				attrValue[1] = &justificationFactor;

		status = ATSUSetLayoutControls(theLayout, 2, attrTag, attrSize, attrValue);
		
		// break the text into lines
		UniCharArrayOffset		lineStart, lineEnd, textStart = 0;
		UniCharCount				lineLength, textLength = inStr.length();
		ATSUTextMeasurement		ascent, descent;
		ItemCount					lineIndex, numOfSoftBreaks;
		UniCharArrayOffset *		softBreaks;

		status = ATSUBatchBreakLines(theLayout, textStart, textLength, lineWidth, &numOfSoftBreaks);
		
		// obtain a list of all the line break positions
		ATSUGetSoftLineBreaks(theLayout, textStart, textLength, 0, NULL, &numOfSoftBreaks);
		softBreaks = (UniCharArrayOffset *)NewPtr(numOfSoftBreaks * sizeof(UniCharArrayOffset));
		ATSUGetSoftLineBreaks(theLayout, textStart, textLength, numOfSoftBreaks, softBreaks, &numOfSoftBreaks);
		
		// Loop over all the lines and draw them
		lineStart = textStart;
		for (lineIndex = 0; lineIndex <= numOfSoftBreaks; lineIndex++)
		{
			lineEnd = (numOfSoftBreaks > 0 && numOfSoftBreaks > lineIndex) ? softBreaks[lineIndex] : textLength;
			lineLength = lineEnd - lineStart;

			ATSUGetLineControl(theLayout, lineStart, kATSULineAscentTag, sizeof(ATSUTextMeasurement), &ascent, NULL);
			ATSUGetLineControl(theLayout, lineStart, kATSULineDescentTag, sizeof(ATSUTextMeasurement), &descent, NULL);

			// Make room for the area above the baseline.
			location.y -= ascent;
			actualHt += ascent;
			if (actualHt > htLimit)
				break;			// if we go below the bounding box, stop

			// Draw the text
			ATSUDrawText(theLayout, lineStart, lineLength, location.x, location.y);

			// Make room for the area below the baseline
			location.y -= descent;
			actualHt += descent;
			if (actualHt > htLimit)
				break;			// if we go below the bounding box, stop
			
			// Prepare for next line
			lineStart = lineEnd;
		}

		FreePtr(softBreaks);

		if (reqHt <= 0)
			ioRect->bottom = ioRect->top + FixedToInt(actualHt);
//		if (reqWd <= 0)
//			ioRect->right = ioRect->left + actualWd;

		CGContextFlush(quartz);
		inStr.releasePtr();
		ATSUDisposeTextLayout(theLayout);
		ATSUDisposeStyle(theStyle);
	}
}

#endif

