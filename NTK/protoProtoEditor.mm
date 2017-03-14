/*
	File:		protoProtoEditor.mm

	Contains:	protoProtoEditor native functions for the Newton Toolkit editor view.

	Written by:	Newton Research Group, 2007.
*/

#define __MACMEMORY__ 1
#define debugLevel 0

#import <Cocoa/Cocoa.h>
#import "Objects.h"
#import "protoProtoEditor.h"
#import "RSSymbols.h"

extern "C" {
void		PrintObject(Ref obj, int indent);
int		REPprintf(const char * inFormat, ...);
}

extern "C" Ref GetProtoVariable(RefArg inRcvr, RefArg inTag, bool * outExists);
extern Ref	MakeStringOfLength(const UniChar * str, ArrayIndex numChars);

#define kTellUser				@"TellUser"


/*------------------------------------------------------------------------------
	Make a NextStep string from a NewtonScript string.
	Args:		inStr			a NewtonScript string
	Return:	an autoreleased NSString
------------------------------------------------------------------------------*/

NSString *
MakeNSString(RefArg inStr)
{
	if (IsString(inStr)) {
		return [NSString stringWithCharacters:GetUString(inStr)
												 length:(Length(inStr) - sizeof(UniChar))/sizeof(UniChar)];
	}
	return nil;
}


/*------------------------------------------------------------------------------
	Make a NewtonScript string from a NextStep string.
	Args:		inStr			an NSString
	Return:	a NewtonScript string
------------------------------------------------------------------------------*/

Ref
MakeString(NSString * inStr)
{
	RefVar s;
	UniChar buf[128];
	UniChar * str = buf;
	ArrayIndex strLen = inStr.length;
	if (strLen > 128) {
		str = (UniChar *)malloc(strLen*sizeof(UniChar));
	}
	[inStr getCharacters:str];
	// NO LINEFEEDS!
	for (UniChar * p = str; p < str + strLen; p++) {
		if (*p == 0x0A) {
			*p = 0x0D;
		}
	}
	s = MakeStringOfLength(str, strLen);
	if (str != buf) {
		free(str);
	}
	return s;
}


/*------------------------------------------------------------------------------
	Get selection text.
	Args:		inRcvr			NewtonScript context
	Return:	text
------------------------------------------------------------------------------*/

Ref
Selection(RefArg inRcvr)
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	return MakeString([txView.string substringWithRange:txView.selectedRange]);
}


/*------------------------------------------------------------------------------
	Get selection start index.
	Args:		inRcvr			NewtonScript context
	Return:	index
------------------------------------------------------------------------------*/

Ref
SelectionOffset(RefArg inRcvr)
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

#if debugLevel > 0
NSLog(@"SelectionOffset() -> %d", txView.selectedRange.location);
#endif
	return MAKEINT(txView.selectedRange.location);
}


/*------------------------------------------------------------------------------
	Get length of selection.
	Args:		inRcvr			NewtonScript context
	Return:	length
------------------------------------------------------------------------------*/

Ref
SelectionLength(RefArg inRcvr)
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

#if debugLevel > 0
NSLog(@"SelectionLength() -> %d", txView.selectedRange.length);
#endif
	return MAKEINT(txView.selectedRange.length);
}


/*------------------------------------------------------------------------------
	Select the given range of text.
	Args:		inRcvr			NewtonScript context
				inStart			offset to start of selection
				inLength			length of selection
	Return:	--
------------------------------------------------------------------------------*/

Ref
SetSelection(RefArg inRcvr, RefArg inStart, RefArg inLength)
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

#if debugLevel > 0
NSLog(@"SetSelection(%d, %d)", RINT(inStart), RINT(inLength));
#endif
	[txView setSelectedRange:NSMakeRange(RINT(inStart), RINT(inLength))];
	return NILREF;
}


/*------------------------------------------------------------------------------
	Replace the selected range of text.
	Args:		inRcvr			NewtonScript context
				inString			string to replace the selection
	Return:	the text
------------------------------------------------------------------------------*/

Ref
ReplaceSelection(RefArg inRcvr, RefArg inString)
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	[txView insertText:MakeNSString(inString)];
	return inString;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Get entire buffer text.
	Args:		inRcvr			NewtonScript context
	Return:	the text
------------------------------------------------------------------------------*/

Ref	
TextString(RefArg inRcvr)
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	return MakeString(txView.string);
}


/*------------------------------------------------------------------------------
	Get number of characters in buffer.
	Args:		inRcvr			NewtonScript context
	Return:	the count
------------------------------------------------------------------------------*/

Ref
TextLength(RefArg inRcvr)
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	return MAKEINT(txView.string.length);
}

#pragma mark -

/*------------------------------------------------------------------------------
	Get line number for offset.
	Args:		inRcvr			NewtonScript context
				inOffset			offset into buffer
	Return:	the line number
------------------------------------------------------------------------------*/

Ref
FindLine(RefArg inRcvr, RefArg inOffset)
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	NSString * str = txView.string;
	ArrayIndex target = RINT(inOffset);
	ArrayIndex lineNumber, index, strLength = str.length;
	for (index = 0, lineNumber = 0; index < strLength; lineNumber++) {
		NSRange lineRange = [str lineRangeForRange:NSMakeRange(index, 0)];
		if (NSLocationInRange(target, lineRange))
#if debugLevel > 0
{NSLog(@"FindLine(%d) -> %d", target, lineNumber);
#endif
			return MAKEINT(lineNumber);
#if debugLevel > 0
}
#endif
		index = NSMaxRange(lineRange);
	}
//	if (lineNumber > 0) lineNumber--;
#if debugLevel > 0
NSLog(@"FindLine(%d) -> %d", target, lineNumber);
#endif
	return MAKEINT(lineNumber);
}


/*------------------------------------------------------------------------------
	Get index of first character in line.
	Args:		inRcvr			NewtonScript context
				inLine			line number
	Return:	the index
------------------------------------------------------------------------------*/

Ref
LineStart(RefArg inRcvr, RefArg inLine)
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	NSString * str = txView.string;
	ArrayIndex target = RINT(inLine);
	ArrayIndex lineNumber, index, strLength = str.length;
	for (index = 0, lineNumber = 0; index < strLength; lineNumber++) {
		NSRange lineRange = [str lineRangeForRange:NSMakeRange(index, 0)];
		if (lineNumber == target)
#if debugLevel > 0
{NSLog(@"LineStart(%d) -> %d", target, lineRange.location);
#endif
			return MAKEINT(lineRange.location);
#if debugLevel > 0
}
#endif
		index = NSMaxRange(lineRange);
	}
#if debugLevel > 0
NSLog(@"LineStart(%d) -> %d // end of text\n", target, strLength);
#endif
	return MAKEINT(strLength);
}


/*------------------------------------------------------------------------------
	Get the number of lines of text.
	Args:		inRcvr			NewtonScript context
	Return:	the number of lines
------------------------------------------------------------------------------*/

Ref
NumberOfLines(RefArg inRcvr)
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	NSString * str = txView.string;
	ArrayIndex numberOfLines, index, strLength = str.length;
	for (index = 0, numberOfLines = 0; index < strLength; numberOfLines++) {
		index = NSMaxRange([str lineRangeForRange:NSMakeRange(index, 0)]);
	}
#if debugLevel > 0
NSLog(@"NumberOfLines() -> %d", numberOfLines);
#endif
	return MAKEINT(numberOfLines);
}


NSRange
CharacterRangeAtLine(NSTextView * inView, NSUInteger inLine)
{
	NSRange lineRange = NSMakeRange(0,0);

	NSString * str = [inView string];
	ArrayIndex lineIndex, charIndex, strLength = str.length;
	for (charIndex = 0, lineIndex = 0; charIndex < strLength && lineIndex <= inLine; lineIndex++) {
		lineRange = [str lineRangeForRange:NSMakeRange(charIndex, 0)];
		charIndex = NSMaxRange(lineRange);
	}

#if debugLevel > 0
NSLog(@"CharacterRangeAtLine(%d) -> {%d,%d}", inLine, lineRange.location, lineRange.length);
#endif
	return lineRange;
}


Ref
LineTop(RefArg inRcvr, RefArg inLine) // Get y-ordinate of top of line
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	ArrayIndex index = RINT(inLine);
	NSRect txRect = [txView firstRectForCharacterRange:CharacterRangeAtLine(txView, index) actualRange:nil];
	int v = txRect.origin.y + txRect.size.height + 4;
#if debugLevel > 0
NSLog(@"LineTop(%d) -> %d (%f)", index, v, txRect.origin.y + txRect.size.height);
#endif
	return MAKEINT(v);
}


Ref
LineBottom(RefArg inRcvr, RefArg inLine) // Get y-ordinate of bottom of line
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	ArrayIndex index = RINT(inLine);
	NSRect txRect = [txView firstRectForCharacterRange:CharacterRangeAtLine(txView, index) actualRange:nil];
	int v = txRect.origin.y - 4;
#if debugLevel > 0
NSLog(@"LineBottom(%d) -> %d (%f)", index, v, txRect.origin.y);
#endif
	return MAKEINT(v);
}


Ref
LineBaseline(RefArg inRcvr, RefArg inLine) // Get y-ordinate of baseline of line
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	ArrayIndex index = RINT(inLine);
	NSRect txRect = [txView firstRectForCharacterRange:CharacterRangeAtLine(txView, index) actualRange:nil];
	int v = txRect.origin.y;
#if debugLevel > 0
NSLog(@"LineBaseline(%d) -> %d (%f)", index, v, txRect.origin.y);
#endif
	return MAKEINT(v);
	// need to adjust this according to font
}


Ref
LeftEdge(RefArg inRcvr, RefArg inOffset) // Get x-ordinate of left edge of character at offset
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	ArrayIndex index = RINT(inOffset);
	NSRect txRect = [txView firstRectForCharacterRange:NSMakeRange(index, 1) actualRange:nil];
	int h = txRect.origin.x + 2;
#if debugLevel > 0
NSLog(@"LeftEdge(%d) -> %d (%f)", index, h, txRect.origin.x);
#endif
	return MAKEINT(h);
}


Ref
RightEdge(RefArg inRcvr, RefArg inOffset) // Get x-ordinate of right edge of character at offset
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	ArrayIndex index = RINT(inOffset);
	NSRect txRect = [txView firstRectForCharacterRange:NSMakeRange(index, 1) actualRange:nil];
	int h = txRect.origin.x + txRect.size.width;
#if debugLevel > 0
NSLog(@"RightEdge(%d) -> %d (%f)", index, h, txRect.origin.x + txRect.size.width);
#endif
	return MAKEINT(h);
}

#pragma mark -

/*------------------------------------------------------------------------------
	Get offset of character containing (h,v) point.
 	Args:		inRcvr			NewtonScript context
				inH 				x
				inV				y
	Return:	--
------------------------------------------------------------------------------*/

Ref
PointToOffset(RefArg inRcvr, RefArg inH, RefArg inV)
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	NSUInteger index = [txView characterIndexForPoint:NSMakePoint(RINT(inH), RINT(inV))];
#if debugLevel > 0
NSLog(@"PointToOffset({%d,%d}) -> %d", RINT(inH), RINT(inV), index);
#endif
	return MAKEINT(index == NSNotFound ? 0 : index);
}

#pragma mark -

Ref
MapChars(RefArg inRcvr, RefArg inClosure, RefArg inStart, RefArg inLength) //  Apply closure to each character from start to start+length
{
//	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	return NILREF;
}


Ref
SearchChars(RefArg inRcvr, RefArg inClosure, RefArg inStart, RefArg inLength) //  Return offset of succeeding character, or nil
{
//	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	return NILREF;
}


/*------------------------------------------------------------------------------
	Return the character at offset.
 	Args:		inRcvr			NewtonScript context
				inOfset			the offset
	Return:	character
------------------------------------------------------------------------------*/

Ref
Peek(RefArg inRcvr, RefArg inOffset)
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	return MAKECHAR([txView.string characterAtIndex:RINT(inOffset)]);
}


/*------------------------------------------------------------------------------
	Establish the keystroke handler for a key.
 	Args:		inRcvr			NewtonScript context
				inKey				a character, or a frame if modifiers are required
				inSymbol			symbol of handler in protoEditor
									handler must be set separately
	Return:	--
------------------------------------------------------------------------------*/

Ref
SetKeyHandler(RefArg inRcvr, RefArg inKey, RefArg inHandlerSymbol)
{
	RefVar keys(GetProtoVariable(inRcvr, SYMA(keys), NULL));
	// keys is an array of keyCode,handlerSymbol pairs
	AddArraySlot(keys, inKey);
	AddArraySlot(keys, inHandlerSymbol);
	// maybe we should check whether that key has already been defined?
	return NILREF;
}


/*------------------------------------------------------------------------------
	Return the name (symbol) of the keystroke handler for a key.
	Args:		inKey				a character, or a frame if modifiers are pressed
	Return:	the symbol of the handler, or nil if no handler was found
------------------------------------------------------------------------------*/

int
KeyModifiers(RefArg inKey)
{
	int modifiers = 0;
	if (IsFrame(inKey)) {
		if (NOTNIL(GetFrameSlot(inKey, SYMA(shift))))
			modifiers |= 1;
		if (NOTNIL(GetFrameSlot(inKey, SYMA(control))))
			modifiers |= 2;
		if (NOTNIL(GetFrameSlot(inKey, SYMA(option))))
			modifiers |= 4;
		if (NOTNIL(GetFrameSlot(inKey, SYMA(command))))
			modifiers |= 8;
	}
	return modifiers;
}

Ref
GetKeyHandler(RefArg inRcvr, RefArg inKey)
{
	RefVar keys(GetProtoVariable(inRcvr, SYMA(keys), NULL));
	// look up inKey in keys
	RefVar keyReqd;
	if (IsFrame(inKey)) {
		keyReqd = GetFrameSlot(inKey, SYMA(key));
	} else {
		keyReqd = inKey;
	}
	RefVar key, keyChar;
	for (ArrayIndex i = 0, count = Length(keys); i < count; ++i) {
		key = GetArraySlot(keys, i++);
		if (IsFrame(key)) {
			keyChar = GetFrameSlot(key, SYMA(key));
		} else {
			keyChar = key;
		}
		if (keyChar == keyReqd
		&&  KeyModifiers(key) == KeyModifiers(inKey)) {
			// return associated symbol
			return GetArraySlot(keys, i);
		}
	}
	return NILREF;
}

#pragma mark -

Ref
SetMetaBit(RefArg inRcvr) // Next keystroke interpreted as though option key depressed
{
//	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	return NILREF;
}


Ref
QuoteCharacter(RefArg inRcvr) // Next keystroke will be interpreted as a character only (not a command)
{
//	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	return NILREF;
}


/*------------------------------------------------------------------------------
	Return the character class for char.
	0 => whitespace
	1 => alpha
	2 => punctuation
	Args:		inRcvr			NewtonScript context
				inChar
	Return:	the class (int)
------------------------------------------------------------------------------*/

int
CharacterClass(unichar inChar)
{
	static NSCharacterSet * literalSet = nil;
	static NSCharacterSet * whitespaceSet = nil;

	if (literalSet == nil)
		literalSet = [[NSCharacterSet characterSetWithCharactersInString:@"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz"] retain];
	if ([literalSet characterIsMember:inChar])
		return 1;

	if (whitespaceSet == nil)
		whitespaceSet = [[NSCharacterSet whitespaceAndNewlineCharacterSet] retain];
	if ([whitespaceSet characterIsMember:inChar])
		return 0;

	return 2;
}


Ref
CharacterClass(RefArg inRcvr, RefArg inChar)
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	return MAKEINT(CharacterClass(RCHAR(inChar)));
}


Ref
SetCharacterClass(RefArg inRcvr, RefArg inChar, RefArg inClass) // Change the character class for char
{
//	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	return NILREF;
}


#pragma mark -
/*------------------------------------------------------------------------------
	Return the index of the first character in the token containing offset.
	A-Z,a-z,0-9,_ => literal (class 1)
	punctuation (class 2)
	whitespace (class 0)

	classify character at offset;
	choose character set based on that
	look backwards until non-character set character encountered
	return offset of that character

	Args:		inRcvr			NewtonScript context
				inOffset
	Return:	the class (int)
------------------------------------------------------------------------------*/

Ref
TokenStart(RefArg inRcvr, RefArg inOffset)
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	NSString * str = txView.string;
	ArrayIndex offset = RINT(inOffset);
	// assert offset is in range of str
	unichar charCode = [str characterAtIndex:offset];
	int charClass = CharacterClass(charCode);

	for ( ; offset > 0; offset--) {
		charCode = [str characterAtIndex:offset-1];
		if (CharacterClass(charCode) != charClass) {
			break;
		}
	}

	return MAKEINT(offset);
}


/*------------------------------------------------------------------------------
	Return the index of the first character after the token containing offset.
	
	Args:		inRcvr			NewtonScript context
				inOffset
	Return:	the class (int)
------------------------------------------------------------------------------*/

Ref
TokenEnd(RefArg inRcvr, RefArg inOffset)
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	NSString * str = txView.string;
	ArrayIndex len = str.length, offset = RINT(inOffset);
	// assert offset is in range of str
	unichar charCode = [str characterAtIndex:offset];
	int charClass = CharacterClass(charCode);

	for ( ; offset < len-1; offset++) {
		charCode = [str characterAtIndex:offset+1];
		if (CharacterClass(charCode) != charClass) {
			break;
		}
	}

	return MAKEINT(offset<len?offset+1:len);
}

#pragma mark -

/*------------------------------------------------------------------------------
	Display string in the minibuffer.
	Args:		inRcvr			NewtonScript context
				inString
	Return:	the string
------------------------------------------------------------------------------*/

Ref
TellUser(RefArg inRcvr, RefArg inString)
{
	NSString * str = MakeNSString(inString);
	// pass to view’s delegate == window controller
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));
	[txView.window.windowController tellUser:str];
	return inString;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Return visible bounds of the text container.
	Args:		inRcvr			NewtonScript context
	Return:	an integer

	NSLayoutManager * layoutMgr = [txView layoutManager];
	NSRect txRect = [layoutMgr lineFragmentRectForGlyphAtIndex:index effectiveRange:NULL];
------------------------------------------------------------------------------*/

Ref
VisibleTop(RefArg inRcvr) // Top of visible portion of view
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	NSRect boundsBox = txView.bounds;
	int v = boundsBox.origin.y + boundsBox.size.height;
	return MAKEINT(v);
}


Ref
VisibleLeft(RefArg inRcvr) // Left edge of visible portion of view
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	NSRect boundsBox = txView.bounds;
	int h = boundsBox.origin.x;
	return MAKEINT(h);
}


Ref
VisibleHeight(RefArg inRcvr) // Height of visible portion of view
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	NSRect boundsBox = txView.bounds;
	int v = boundsBox.size.height;
	return MAKEINT(v);
}


Ref
VisibleWidth(RefArg inRcvr) // Width of visible portion of view
{
	NSTextView * txView = (NSTextView *)(Ref) GetFrameSlot(inRcvr, SYMA(viewCObject));

	NSRect boundsBox = txView.bounds;
	int h = boundsBox.size.width;
	return MAKEINT(h);
}


Ref
SetVisibleTop(RefArg inRcvr, RefArg inNewTop) // Vertically scroll visible portion of view
{ return NILREF; }


Ref
SetVisibleLeft(RefArg inRcvr, RefArg inNewLeft) // Horizontally scroll visible portion of view
{ return NILREF; }
