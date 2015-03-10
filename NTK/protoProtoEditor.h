/*
	File:		protoProtoEditor.h

	Contains:	protoProtoEditor native functions for the Newton Toolkit editor view.

	Written by:	Newton Research Group, 2008.
*/

extern "C" {

Ref	Selection(RefArg inRcvr);
Ref	SelectionOffset(RefArg inRcvr);
Ref	SelectionLength(RefArg inRcvr);
Ref	SetSelection(RefArg inRcvr, RefArg inStart, RefArg inLength);
Ref	ReplaceSelection(RefArg inRcvr, RefArg inString);

Ref	TextString(RefArg inRcvr);			// Get entire buffer text
Ref	TextLength(RefArg inRcvr);			// Get number of characters in buffer

Ref	FindLine(RefArg inRcvr, RefArg inOffset);			// Get line number for offset
Ref	LineStart(RefArg inRcvr, RefArg inLine);			// Get index of first character in line
Ref	NumberOfLines(RefArg inRcvr);
Ref	LineTop(RefArg inRcvr, RefArg inLine);				// Get coordinate of top of line
Ref	LineBottom(RefArg inRcvr, RefArg inLine);			// Get coordinate of bottom of line
Ref	LineBaseline(RefArg inRcvr, RefArg inLine);		// Get coordinate of baseline of line
Ref	LeftEdge(RefArg inRcvr, RefArg inOffset);			// Get coordinate of left edge of character at offset
Ref	RightEdge(RefArg inRcvr, RefArg inOffset);		// Get coordinate of right edge of character at offset

Ref	PointToOffset(RefArg inRcvr, RefArg inH, RefArg inV);		// Get offset of character containing (h,v) point

Ref	MapChars(RefArg inRcvr, RefArg inClosure, RefArg inStart, RefArg inLength);		//  Apply closure to each character from start to start+length
Ref	SearchChars(RefArg inRcvr, RefArg inClosure, RefArg inStart, RefArg inLength);	//  Return offset of succeeding character, or nil
Ref	Peek(RefArg inRcvr, RefArg inOffset);									// Returns the character at offset
Ref	SetKeyHandler(RefArg inRcvr, RefArg inKey, RefArg inSymbol);	// Establish the keystroke handler for key
Ref	GetKeyHandler(RefArg inRcvr, RefArg inKey);							// Returns the name of the keystroke handler for key, or nil

Ref	SetMetaBit(RefArg inRcvr);			// Next keystroke interpreted as though option key depressed
Ref	QuoteCharacter(RefArg inRcvr);	// Next keystroke will be interpreted as a character only (not a command)
Ref	CharacterClass(RefArg inRcvr, RefArg inChar);	 // Returns the character class for char
Ref	SetCharacterClass(RefArg inRcvr, RefArg inChar, RefArg inClass);	// Change the character class for char

Ref	TokenStart(RefArg inRcvr, RefArg inOffset);	// Returns the index of the first character in the token containing offset
Ref	TokenEnd(RefArg inRcvr, RefArg inOffset);		// Returns the index of the first character after the token containing offset

Ref	TellUser(RefArg inRcvr, RefArg inString);

Ref	VisibleTop(RefArg inRcvr);			// Top of visible portion of view
Ref	VisibleLeft(RefArg inRcvr);		// Left edge of visible portion of view
Ref	VisibleHeight(RefArg inRcvr);		// Height of visible portion of view
Ref	VisibleWidth(RefArg inRcvr);		// Width of visible portion of view
Ref	SetVisibleTop(RefArg inRcvr, RefArg inNewTop);		// Vertically scroll visible portion of view
Ref	SetVisibleLeft(RefArg inRcvr, RefArg inNewLeft);	// Horizontally scroll visible portion of view

}
