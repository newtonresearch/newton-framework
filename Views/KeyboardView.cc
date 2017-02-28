/*
	File:		KeyboardView.cc

	Contains:	CKeyboardView implementation.

	Written by:	Newton Research Group.
*/

#include "Quartz.h"
#include "Geometry.h"
#include "QDDrawing.h"
#include "DrawShape.h"
#include "DrawText.h"
#include "KeyboardView.h"
#include "KeyboardKeys.h"
#include "RootView.h"

#include "Objects.h"
#include "ROMResources.h"
#include "Funcs.h"
#include "Arrays.h"
#include "Strings.h"
#include "Unicode.h"
#include "UStringUtils.h"
#include "Iterators.h"
#include "Locales.h"
#include "Sound.h"

#include "Notebook.h"

#include "Unit.h"


/* -----------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
----------------------------------------------------------------------------- */
extern "C" {
Ref	FKeyboardConnected(RefArg inRcvr);
Ref	FCommandKeyboardConnected(RefArg inRcvr);
Ref	FRegisterOpenKeyboard(RefArg inRcvr, RefArg inKybd);
Ref	FUnregisterOpenKeyboard(RefArg inRcvr);

Ref	FAddKeyCommand(RefArg inRcvr, RefArg inCmd);
Ref	FAddKeyCommands(RefArg inRcvr, RefArg inCmds);
Ref	FBlockKeyCommand(RefArg inRcvr, RefArg inCmd);

Ref	FKeyboardInput(RefArg inRcvr, RefArg inCmd);

Ref	FSetKeyView(RefArg inRcvr, RefArg inView, RefArg inOffset);

Ref	FGetCaretBox(RefArg inRcvr);
Ref	FKeyIn(RefArg inRcvr, RefArg inArg1, RefArg inArg2);
Ref	FPostKeyString(RefArg inRcvr, RefArg inView, RefArg inStr);
Ref	FGetKeyView(RefArg inRcvr);
Ref	FNextKeyView(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3);
Ref	FRestoreKeyView(RefArg inRcvr, RefArg inArg);

Ref	FIsKeyDown(RefArg inRcvr, RefArg inArg1, RefArg inArg2);
Ref	FTranslateKey(RefArg inRcvr, RefArg inKeyCode, RefArg inModifiers, RefArg inState);
Ref	FIsCommandKeystroke(RefArg inRcvr, RefArg inKeyCode, RefArg inModifiers);
Ref	FFindKeyCommand(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3);
Ref	FSendKeyMessage(RefArg inRcvr, RefArg inArg1, RefArg inArg2);
Ref	FMatchKeyMessage(RefArg inRcvr, RefArg inArg1, RefArg inArg2);
Ref	FGatherKeyCommands(RefArg inRcvr, RefArg inView);
Ref	FCategorizeKeyCommands(RefArg inRcvr, RefArg inArg);
Ref	FHandleKeyEvents(RefArg inRcvr, RefArg inArg);
Ref	FClearHardKeymap(RefArg inRcvr);
Ref	FGetTrueModifiers(RefArg inRcvr);
}


void
AddKeyCommand(RefArg inView, RefArg inCmd)
{}

void
AddKeyCommands(RefArg inView, RefArg inCmds)
{}

void
BlockKeyCommand(CView * inView, RefArg inCmd)
{}


int
CountOnes(ULong x)
{
	int x1 = (x >> 1) & 0xDB6DB6DB;
	x = x - x1;
	x = x - ((x1 >> 1) & 0xDB6DB6DB);
	x = (x + (x >> 3)) & 0xC71C71C7;
	x = x + (x >>  6);
	x = x + (x >> 12);
	x = x + (x >> 24);
	return x & 0x0000003F;
}

ArrayIndex
FindKeyCommandInArray(RefArg inArray, UniChar inChar, ULong inModifiers, int * outNumOfModifiersFound, bool * outIsExactMatch)
{
// r5: r1 r4 xx r8
	UniChar ch = inChar;
	RefVar keyCmd;
	ArrayIndex index = kIndexNotFound;
	int numOfBitsMatched = -1;
	if (outIsExactMatch != NULL)
		*outIsExactMatch = false;
	for (ArrayIndex i = 0, count = Length(inArray); i < count; ++i)
	{
		keyCmd = GetArraySlot(inArray, i);
		if (NOTNIL(keyCmd))
		{
			Ref ch = GetFrameSlot(keyCmd, SYMA(char));
			if (RCHAR(ch) == ch)
			{
				ULong modifiers = KeyCommandModifiers(keyCmd);
				if (modifiers == inModifiers)		// we have the reqd modifiers
				{
					index = i;
					numOfBitsMatched = CountOnes(inModifiers);
					if (outIsExactMatch != NULL)
						*outIsExactMatch = true;
					break;
				}
				else if ((modifiers & inModifiers) == modifiers)	// we have SOME of the reqd modifiers
				{
					int numBits = CountOnes(modifiers);
					if (numBits > numOfBitsMatched)
						numOfBitsMatched = numBits;
				}
			}
		}
	}
	if (outNumOfModifiersFound != NULL)
		*outNumOfModifiersFound = numOfBitsMatched;
	return index;
}

Ref
FindKeyCommand(CView * inView, UniChar inKeyChar, ULong inModifiers)
{
	RefVar keyCmd;
	int bestMatch = -1;
	CView * view = inView;
	for (bool isDone = false; !isDone; )
	{
		RefVar keyCmds(view->getProto(SYMA(_keyCommands)));
		if (IsArray(keyCmds))
		{
			int numOfModifiers;
			bool isExact;
			ArrayIndex index = FindKeyCommandInArray(keyCmds, inKeyChar, inModifiers, &numOfModifiers, &isExact);
			if (isExact)
			{
				keyCmd = GetArraySlot(keyCmds, index);
				isDone = true;
			}
			else if (index != kIndexNotFound && numOfModifiers > bestMatch)
			{
				keyCmd = GetArraySlot(keyCmds, index);
				bestMatch = numOfModifiers;
			}
			if (view == gRootView)
				isDone = true;
			else
			{
				RefVar nextKeyView(view->getProto(SYMA(_nextKeyView)));
				if (NOTNIL(nextKeyView))
				{
					if (EQ(nextKeyView, SYMA(none)))
						isDone = true;
					else
						view = GetView(nextKeyView);
				}
				else
					view = view->fParent;
			}
		}
	}
	return keyCmd;
}


#pragma mark -
/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

unsigned char	gSoftKeyMap[32];		// 256 keys as a bitmap
ULong				gSoftKeyDeadState;
bool				gSoftCapsLock;

unsigned char	gHardKeyMap[32];
ULong				gHardKeyDeadState;
bool				gHardCapsLock;

ULong				gTrueModifiers;


/*------------------------------------------------------------------------------
	K e y b o a r d   S t u f f
------------------------------------------------------------------------------*/

UniChar
ConvertToUnicode(char inCh)
{
	char		srcStr[2];
	UniChar	dstStr[2];
	srcStr[0] = inCh;
	srcStr[1] = 0;
	ConvertToUnicode(srcStr, dstStr);
	return dstStr[0];
}


Ref
GetKeyTransMapping(void)
{
	return GetFrameSlot(GetProtoVariable(IntlResources(), SYMA(keyboard)), SYMA(mapping));
}


typedef struct
{
	short	version;
	char	modTableNum[256];
	short	numOfTables;
	unsigned char	tables[8][128];
//	dead key records
} kchrRec;

typedef struct
{
	char				deadKeyCode;
	short				numOfPairs;
	struct {
	unsigned char	normal;
	unsigned char	modified;
	} map;
} DeadKeyTable;

typedef struct
{
	short				numOfTables;
	DeadKeyTable	tables[];
} DeadKeyMap;

UniChar
TranslateKey(ULong inCode, bool inIsDown, ULong inModifiers, ULong * ioState)
{
	CDataPtr		keyMap(GetKeyTransMapping());
	kchrRec *	kchr = (kchrRec *)(char *)keyMap;
	int			modTableNum = kchr->modTableNum[inModifiers];	// r6
	UChar *		table = kchr->tables[modTableNum];
	UChar			ch8bit = table[inCode];								// r1
	DeadKeyMap *	deadKeys = (DeadKeyMap *)&kchr->tables[kchr->numOfTables][0];	// r3
#if 0
	if (*ioState != 0)
	{
		DeadKeyMap *	r12 = (DeadKeyMap *)((char *)keyMap + *ioState);
		ULong		lk = 0;
		UChar		r9;
		for (; (r9 = r12->map.normal) != 0; r12++)
		{
			if (r9 == ch8bit)
				ch8bit = r12->map.modified;
				if (inIsDown)
					*ioState = 0;
		}
//L52
	}
//L64
	if (ch8bit == 0)
	{
	//	it’s a dead key
		;
		for (int i = deadKeys->numOfPairs; i > 0; i--)
		{
		}
		// INCOMPLETE
	}
//L96
#endif
	UniChar	ch = ConvertToUnicode(ch8bit);
	if (ch == 0x10)
	{
	//	function keys
		switch (inCode)
		{
		case 0x60:
			ch = 0xF725;
			break;
		case 0x61:
			ch = 0xF726;
			break;
		case 0x62:
			ch = 0xF727;
			break;
		case 0x63:
			ch = 0xF723;
			break;
		case 0x64:
			ch = 0xF728;
			break;
		case 0x65:
			ch = 0xF729;
			break;
//		case 0x66:
		case 0x67:
			ch = 0xF72B;
			break;
//		case 0x68:
		case 0x69:
			ch = 0xF72D;
			break;
//		case 0x6A:
		case 0x6B:
			ch = 0xF72E;
			break;
//		case 0x6C:
		case 0x6D:
			ch = 0xF72A;
			break;
//		case 0x6E:
		case 0x6F:
			ch = 0xF72C;
			break;
//		case 0x70:
		case 0x71:
			ch = 0xF72F;
			break;
//		case 0x72:
//		case 0x73:
//		case 0x74:
//		case 0x75:
		case 0x76:
			ch = 0xF724;
			break;
//		case 0x77:
		case 0x78:
			ch = 0xF722;
			break;
//		case 0x79:
		case 0x7A:
			ch = 0xF721;
			break;
		}
	}
	return ch;
}


// Modifiers occupy codes 0x37..0x3B / 0x3D
// which map to byte 6/7 of the keyPressMap
ULong
Modifiers(bool inHardKeys)
{
	unsigned char *	keyPressBitmap = inHardKeys ? gHardKeyMap : gSoftKeyMap;
	return (keyPressBitmap[6] >> 7)				// 0x37
		 | ((keyPressBitmap[7] << 1) & 0x1F);	// 0x38..0x3B
}


bool
IsModifierKeyCode(ULong inCode)
{
	return 0x37 <= inCode && inCode <= 0x3B;
}


UniChar
KeyLabel(ULong inCode, bool inHardKeys)
{
	ULong	state = inHardKeys ? gHardKeyDeadState : gSoftKeyDeadState;
	return TranslateKey(inCode, true, Modifiers(inHardKeys), &state);
}


bool
KeyDown(ULong inCode, bool inHardKeys)
{
	unsigned char *	keyPressBitmap = inHardKeys ? gHardKeyMap : gSoftKeyMap;
	ArrayIndex		mapIndex = inCode / 8;
	ULong		mapBit = 1 << (inCode & 0x07);
	return (keyPressBitmap[mapIndex] & mapBit) != 0;
}


UniChar
KeyIn(ULong inCode, bool inIsDown, CView * inView)
{
	bool	isHardKeys = false;
	if (inView == (CView *)-1)
	{
		// no view associated with this keyboard, so it must be a REAL one
		isHardKeys = 1;
		if (inCode == 0x38)	// shift
		{
			if (inIsDown)
				gTrueModifiers |= 0x01;
			else
			{
				gTrueModifiers &= ~0x01;
				if ((gTrueModifiers & 0x02) != 0)
					return 0;
			}
		}
		else if (inCode == 0x3A)	// option
		{
			if (inIsDown)
				gTrueModifiers |= 0x04;
			else
			{
				gTrueModifiers &= ~0x04;
				if ((gTrueModifiers & 0x08) != 0)
					return 0;
			}
		}
		else if (inCode == 0x3C)	// right shift
		{
			if (inIsDown)
				gTrueModifiers |= 0x02;
			else
			{
				gTrueModifiers &= ~0x02;
				if ((gTrueModifiers & 0x01) != 0)
					return 0;
			}
			inCode = 0x38;
		}
		else if (inCode == 0x3D)	// right option
		{
			if (inIsDown)
				gTrueModifiers |= 0x08;
			else
			{
				gTrueModifiers &= ~0x08;
				if ((gTrueModifiers & 0x04) != 0)
					return 0;
			}
			inCode = 0x3A;
		}
		else if (inCode == 0x39)	// caps lock
		{
			bool *	capsLock = &gHardCapsLock;
			if (inIsDown)
				inIsDown = *capsLock = !*capsLock;
			else
				inCode = 0;
		}
	}
	else
//L58
	{
		if (inCode == 0x3C)
			inCode = 0x38;				// shift
		else if (inCode == 0x3D)
			inCode = 0x3A;				// option
		else if (inCode == 0x39)	// caps lock
		{
			bool *	capsLock = &gSoftCapsLock;
			if (inIsDown)
				inIsDown = *capsLock = !*capsLock;
			else
				inCode = 0;
		}
		gRootView->handleKeyIn(inCode, inIsDown, inView);
	}
//L89
	unsigned char *	keyPressBitmap = isHardKeys ? gHardKeyMap : gSoftKeyMap;
	int		mapIndex = inCode / 8;
	int		mapBit = 1 << (inCode & 0x07);
	if (inIsDown)
		keyPressBitmap[mapIndex] |= mapBit;
	else
		keyPressBitmap[mapIndex] &= ~mapBit;

	ULong *	state = isHardKeys ? &gHardKeyDeadState : &gSoftKeyDeadState;
	UniChar	ch = TranslateKey(inCode, inIsDown, Modifiers(isHardKeys), state);
	if (*state != 0)
		ch = 0;
	return ch;
}


void
PostKeyString(CView * inKeyView, RefArg inStr)
{
	;
}


bool
UserVisibleChar(UniChar inChar)
{
	if (inChar >= 0xF721 && inChar <= 0xF72F)
		return false;	// it’s a function key
	return (inChar > 0x20 && inChar != 0x7F);
}

UniChar
GetDisplayCmdChar(RefArg inKeyCmd)
{
	UniChar displayChar;
	RefVar ch(GetFrameSlot(inKeyCmd, SYMA(showChar)));
	if (ISNIL(ch))
		ch = GetFrameSlot(inKeyCmd, SYMA(char));
	displayChar = RCHAR(ch);
	if (UserVisibleChar(displayChar))
		return displayChar;
	return 0;
}


ULong
KeyCommandModifiers(RefArg inKeyCmd)
{
	RefVar modifiers(GetFrameSlot(inKeyCmd, SYMA(modifiers)));
	if (NOTNIL(modifiers))
		return RINT(modifiers) & kKeyModifierMask;
	return 0;
}


int
GetModifiersWidth(RefArg inKeyCmd)
{
	int width = 0;
	ULong modifiers = KeyCommandModifiers(inKeyCmd);
	if (FLAGTEST(modifiers, kCmdModifier))
		width = 11;
	if (FLAGTEST(modifiers, kShiftModifier))
		width += 11;
	if (FLAGTEST(modifiers, kOptionModifier))
		width += 12;
	if (FLAGTEST(modifiers, kControlModifier))
		width += 12;
	return width + 10;
}


int
GetCommandCharWidth(RefArg inKeyCmd, StyleRecord * inStyle)
{
	Ref chr = GetProtoVariable(inKeyCmd, SYMA(char));
	UniChar ch = RCHAR(chr);
	UpperCaseText(&ch, 1);
	return MeasureOnce(&ch, 1, inStyle);
}


#pragma mark -
/* -----------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
----------------------------------------------------------------------------- */

Ref
FKeyboardConnected(RefArg inRcvr)
{
	return MAKEBOOLEAN(gRootView->keyboardConnected());
}


Ref
FCommandKeyboardConnected(RefArg inRcvr)
{
	return MAKEBOOLEAN(gRootView->commandKeyboardConnected());
}


Ref
FRegisterOpenKeyboard(RefArg inRcvr, RefArg inKybd)
{
	gRootView->registerKeyboard(inRcvr, RINT(inKybd));
	return NILREF;
}


Ref
FUnregisterOpenKeyboard(RefArg inRcvr)
{
	return MAKEBOOLEAN(gRootView->unregisterKeyboard(inRcvr));
}



Ref
FAddKeyCommand(RefArg inRcvr, RefArg inCmd)
{
	AddKeyCommand(inRcvr, inCmd);
	return NILREF;
}


Ref
FAddKeyCommands(RefArg inRcvr, RefArg inCmds)
{
	AddKeyCommands(inRcvr, inCmds);
	return NILREF;
}


Ref
FBlockKeyCommand(RefArg inRcvr, RefArg inCmd)
{
	CView *  view = GetView(inRcvr);
	if (view)
		BlockKeyCommand(view, inCmd);
	return NILREF;
}


Ref
FSetKeyView(RefArg inRcvr, RefArg inView, RefArg inOffset)
{
	CView *  view = NULL;
	if (NOTNIL(inView))
		view = GetView(inView);

	if (ISINT(inOffset) || ISNIL(inOffset))
	{
		RefVar	info(Clone(RA(canonicalParaCaretInfo)));
		SetFrameSlot(info, SYMA(offset), MAKEINT(NOTNIL(inOffset) ? RINT(inOffset) : 0));
		SetFrameSlot(info, SYMA(Length), MAKEINT(0));
		gRootView->setKeyViewSelection(view, info, true);
	}
	else
		gRootView->setKeyViewSelection(view, inOffset, true);

	return NILREF;
}


Ref
FGetKeyView(RefArg inRcvr)
{
	CView * keyView = gRootView->caretView();
	if (keyView != NULL) {
		return keyView->fContext;
	}
	return NILREF;
}


Ref
FNextKeyView(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3)
{
	CView * keyView = FailGetView(inArg1);
	if (keyView != NULL) {
		keyView = keyView->nextKeyView(keyView, RINT(inArg2), RINT(inArg3));
		if (keyView)
			return keyView->fContext;
	}
	return NILREF;
}


Ref
FGetCaretBox(RefArg inRcvr)
{
	CView * keyView = gRootView->caretView();
	Rect caretRect = gRootView->getCaretRect();
	if (keyView != NULL && caretRect.top != -32768)
	{
		RefVar box(ToObject(&caretRect));
		SetFrameSlot(box, SYMA(view), keyView->fContext);
//		int offset = gRootView->x70 == NULL ? gRootView->x6C : -1;
//		SetFrameSlot(box, SYMA(offset), MAKEINT(offset));
		return box;
	}
	return NILREF;
}


Ref
FKeyIn(RefArg inRcvr, RefArg inCode, RefArg inIsDown)
{
	return MAKECHAR(KeyIn(RINT(inCode), ISTRUE(inIsDown), NULL));
}


Ref
FPostKeyString(RefArg inRcvr, RefArg inView, RefArg inStr)
{
	CView * keyView = GetView(inRcvr, inView);
	if (keyView != NULL)
		PostKeyString(keyView, inStr);
	return NILREF;
}


Ref
FRestoreKeyView(RefArg inRcvr, RefArg inView)
{
	CView * keyView = GetView(inView);
	if (keyView != NULL)		// original doesn’t bother to check
		return MAKEBOOLEAN(gRootView->restoreKeyView(keyView));
	return NILREF;
}

Ref
FTranslateKey(RefArg inRcvr, RefArg inKeyCode, RefArg inModifiers, RefArg inState)
{
	ULong state = RINT(inState);
	return MAKECHAR(TranslateKey(RINT(inKeyCode), true/*isDown*/, (RINT(inModifiers) & kKeyModifierMask) >> kKeyModifierShift, &state));

}

Ref
FIsKeyDown(RefArg inRcvr, RefArg inKey, RefArg inHardKeys)
{
	return MAKEBOOLEAN(KeyDown(RINT(inKey), ISTRUE(inHardKeys)));
}

bool
IsCommandKeystroke(UniChar inKey, ULong inModifiers)
{
	return FLAGTEST(inModifiers, kCmdModifier)
		 || (inKey >= 0xF721 && inKey <= 0xF72F)
		 || inKey == 0x1B;
}

Ref
FIsCommandKeystroke(RefArg inRcvr, RefArg inKey, RefArg inModifiers)
{
	return MAKEBOOLEAN(IsCommandKeystroke(RCHAR(inKey), RINT(inModifiers)));
}


Ref
FFindKeyCommand(RefArg inRcvr, RefArg inView, RefArg inKeycode, RefArg inModifiers)
{
	CView * keyView = GetView(inView);
	if (keyView != NULL)
		return FindKeyCommand(keyView, RCHAR(inKeycode), RINT(inModifiers));
	return NILREF;
}


Ref
SendKeyMessage(CView * inView, RefArg inMsg)
{return NILREF;}

Ref
FSendKeyMessage(RefArg inRcvr, RefArg inView, RefArg inMsg)
{
	CView * keyView = GetView(inView);
	if (keyView != NULL)
		return SendKeyMessage(keyView, inMsg);
	return NILREF;
}


Ref
MatchKeyMessage(CView * inView, RefArg inMsg, ULong inModifiers)
{return NILREF;}

Ref
FMatchKeyMessage(RefArg inRcvr, RefArg inView, RefArg inMsg)
{
	CView * keyView = GetView(inView);
	if (keyView != NULL)
		return MatchKeyMessage(keyView, inMsg, 0);
	return NILREF;
}


Ref
GatherKeyCommands(CView * inView)
{return NILREF;}

Ref
FGatherKeyCommands(RefArg inRcvr, RefArg inView)
{
	CView * keyView = GetView(inView);
	if (keyView != NULL)		// original doesn’t bother to check
		return GatherKeyCommands(keyView);
	return NILREF;
}


Ref
CategorizeKeyCommands(RefArg inCmds)
{return NILREF;}

Ref
FCategorizeKeyCommands(RefArg inRcvr, RefArg inCmds)
{
	return CategorizeKeyCommands(inCmds);
}


void
HandleKeyEvents(RefArg inEvents, ArrayIndex inNumOfEvents)
{}

Ref
FHandleKeyEvents(RefArg inRcvr, RefArg inEvents)
{
	HandleKeyEvents(inEvents, Length(inEvents));
	return NILREF;
}


Ref
FGetTrueModifiers(RefArg inRcvr)
{
	return MAKEINT(gTrueModifiers);
}


void
ClearHardKeymap(void)
{
	for (ArrayIndex i = 0; i < 32; ++i)
		gHardKeyMap[i] = 0;
	gHardKeyDeadState = 0;
	gHardCapsLock = false;
	gTrueModifiers = 0;

}

Ref
FClearHardKeymap(RefArg inRcvr)
{
	ClearHardKeymap();
	return NILREF;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C R a w K e y I t e r a t o r
------------------------------------------------------------------------------*/

CRawKeyIterator::CRawKeyIterator(RefArg inKeyDef)
{
	fKeyDefs = inKeyDef;
	fNumOfRows = Length(inKeyDef);
	fNumOfKeys = 0;
	for (ArrayIndex i = 0; i < fNumOfRows; ++i)
	{
		fNumOfKeys += (Length(GetArraySlot(inKeyDef, i)) - 2) / 3;
	}
	reset();
}

void
CRawKeyIterator::copyInto(CRawKeyIterator * ioOther)
{
	ioOther->fKeyDefs = fKeyDefs;
	ioOther->fCurKeyLegend = fCurKeyLegend;
	ioOther->fCurKeyResult = fCurKeyResult;
	ioOther->fCurKeyFormat = fCurKeyFormat;
	ioOther->fRowDef = fRowDef;
	ioOther->fRow = fRow;
	ioOther->fNumOfRows = fNumOfRows;
	ioOther->fColumn = fColumn;
	ioOther->fNumOfColumns = fNumOfColumns;
	ioOther->fNumOfKeys = fNumOfKeys;
}

void
CRawKeyIterator::loadRow(void)
{
	fColumn = 0;
	if (fRow >= fNumOfRows)
	{
		fRowDef = NILREF;
		fNumOfColumns = 0;
	}
	else
	{
		fRowDef = GetArraySlot(fKeyDefs, fRow);
		fNumOfColumns = (Length(fRowDef) - 2) / 3;
	}
}

bool
CRawKeyIterator::loadKey(void)
{
	if (fColumn >= fNumOfColumns
	||  fRow >= fNumOfRows)
	{
		fCurKeyLegend = NILREF;
		fCurKeyResult = NILREF;
		fCurKeyFormat = 0;
		return false;
	}
	fCurKeyLegend = GetArraySlot(fRowDef, 2 + fColumn*3);
	fCurKeyResult = GetArraySlot(fRowDef, 3 + fColumn*3);
	fCurKeyFormat = RINT(GetArraySlot(fRowDef, 4 + fColumn*3));
	return true;
}

bool
CRawKeyIterator::next(void)
{
	fColumn++;
	if (fColumn >= fNumOfColumns)
	{
		fRow++;
		loadRow();
	}
	return loadKey();
}

void
CRawKeyIterator::reset(void)
{
	fColumn = 0;
	fRow = 0;
	loadRow();
	loadKey();
}

#pragma mark -

/*------------------------------------------------------------------------------
	C V i s K e y I t e r a t o r
------------------------------------------------------------------------------*/

CVisKeyIterator::CVisKeyIterator(RefArg inKeyDef, Rect * inRect, Point inPt)
	:	CRawKeyIterator(inKeyDef)
{
	fUnitBounds = *inRect;
	fRowOrigin = inPt;
	fKeyOrigin = inPt;
	reset();
}

void
CVisKeyIterator::copyInto(CVisKeyIterator * ioOther)
{
	CRawKeyIterator::copyInto(ioOther);
	ioOther->fKeyBox = fKeyBox;
	ioOther->fKeyFrame = fKeyFrame;
	ioOther->fKeyCap = fKeyCap;
	ioOther->fRowBounds = fRowBounds;
	ioOther->fUnitBounds = fUnitBounds;
	ioOther->fRowOrigin = fRowOrigin;
	ioOther->fKeyOrigin = fKeyOrigin;
	ioOther->fRowHeight = fRowHeight;
	ioOther->fMaxRowHeight = fMaxRowHeight;
}

bool
CVisKeyIterator::findEnclosingKey(Point inPt)
{
	bool	isFound;

	if (fRow != 0
	||  fColumn != 0)
		reset();
	do
	{
		isFound = PtInRect(inPt, &fKeyFrame) && (fCurKeyFormat & keySpacer) == 0;
	} while (!isFound && next());
	return isFound;
}

void
CVisKeyIterator::loadRow(void)
{
	CRawKeyIterator::loadRow();
	if (fRow >= fNumOfRows)
	{
		fMaxRowHeight = 0;
		fRowHeight = 0;
		fRowBounds = gZeroRect;
		return;
	}
	if (fRow > 0)
		fKeyOrigin.v += fRowHeight;
	fKeyOrigin.h = fRowOrigin.h;
	fRowHeight = fUnitBounds.bottom * RINT(GetArraySlot(fRowDef, 0)) / 8;
	fMaxRowHeight = fUnitBounds.bottom * RINT(GetArraySlot(fRowDef, 1)) / 8;
	fRowBounds.left = fRowOrigin.h;
	fRowBounds.top = fKeyOrigin.v;
	fRowBounds.bottom = fKeyOrigin.v + fMaxRowHeight;
	fHasVisKeys = false;
	fFirstVisKeyIndex =
	fLastVisKeyIndex = 0;
	long	rowSize = 0;
	for (ArrayIndex i = 0; i < fNumOfColumns; ++i)
	{
		ULong	bits = RINT(GetArraySlot(fRowDef, 4 + i*3));
		rowSize += keyHSize(bits);
		if ((bits & keySpacer) == 0)
		{
			if (!fHasVisKeys)
			{
				fHasVisKeys = true;
				fFirstVisKeyIndex = i;
			}
			fLastVisKeyIndex = i;
		}
	}
	fRowBounds.right = fRowBounds.left + fUnitBounds.right * rowSize / 8;
	fRowBounds.right++;
	fRowBounds.bottom++;
}

bool
CVisKeyIterator::loadKey(void)
{
	bool	isKeyInRange = CRawKeyIterator::loadKey();
	if (!isKeyInRange)
	{
		fKeyBox = gZeroRect;
		fKeyFrame = gZeroRect;
		fKeyCap = gZeroRect;
	}
	else
	{
		fKeyBox = fUnitBounds;
		fKeyBox.right = fKeyBox.right * keyHSize(fCurKeyFormat) / 8;
		fKeyBox.bottom = fKeyBox.bottom * keyVSize(fCurKeyFormat) / 8;
		OffsetRect(&fKeyBox, fKeyOrigin.h, fKeyOrigin.v);
		fKeyOrigin.h = fKeyBox.right;
		int	inset = keyInset(fCurKeyFormat);
		if (inset != 0)
			InsetRect(&fKeyBox, inset, inset);
		fKeyCap = fKeyFrame = fKeyBox;
		int	frameWd = keyFrameWd(fCurKeyFormat);
		if (frameWd != 0)
		{
			fKeyCap.left += frameWd;
			fKeyCap.top += frameWd;
			fKeyFrame.right += frameWd;
			fKeyFrame.bottom += frameWd;
		}
	}
	return isKeyInRange;
}

bool
CVisKeyIterator::next(void)
{
	fColumn++;
	if (fColumn >= fNumOfColumns)
	{
		fRow++;
		loadRow();
	}
	return loadKey();
}

void
CVisKeyIterator::reset(void)
{
	CRawKeyIterator::reset();
	fKeyOrigin = fRowOrigin;
	loadRow();
	loadKey();
}

bool
CVisKeyIterator::skipToStartOfNextRow(void)
{
	if (fRow < fNumOfRows)
	{
		do
		{
			fRow++;
			loadRow();
		} while (fRow <= fNumOfRows && !fHasVisKeys);
		return loadKey();
	}
	return false;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C K e y b o a r d V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clKeyboardView, CKeyboardView, CView)


CKeyboardView::~CKeyboardView()
{ }


/*--------------------------------------------------------------------------------
	Initialize.
	Args:		inProto		the context frame for this view
				inView		the parent view
	Return:	--
--------------------------------------------------------------------------------*/

void
CKeyboardView::init(RefArg inProto, CView * inView)
{
	CView::init(inProto, inView);

	//RefVar	sp08(fContext);
	RefVar	slot;		// sp04
	bool		exists;	// sp00

	fKeyDefinitions = GetProtoVariable(fContext, SYMA(keyDefinitions));

	slot = GetProtoVariable(fContext, SYMA(keyArrayIndex), &exists);
	fKeyArrayIndex = exists ? RINT(slot) : 0;

	slot = GetProtoVariable(fContext, SYMA(keyResultsAreKeycodes), &exists);
	fKeyResultsAreKeycodes = exists ? NOTNIL(slot) : false;

	slot = GetProtoVariable(fContext, SYMA(keySound), &exists);
	fHasSound = exists ? NOTNIL(slot) : false;

	slot = GetVariable(fContext, SYMA(viewFont), &exists);
//	CreateTextStyle(slot, &fStyle);
//	fStylePtr = &fStyle;
	//sp-10
//	GetStyleFontInfo(fStyle, &fFontInfo);

	fOptions.alignment = fract1/2;
	fOptions.justification = 0;
	fOptions.width = 0;
/*	fOptions.x14 = 0;
	fOptions.transferMode = srcOr;
	fOptions.x0C = 0;
	fOptions.x18 = 0;
*/
	fKeyReceiverView = GetProtoVariable(fContext, SYMA(keyReceiverView), &exists);
	if (!exists)
		fKeyReceiverView = SYMA(viewFrontKey);

//L115
	//sp-2C
	int	rowHt;			// r10
	int	rowWd;			// r5
	int	totalHt = 0;	// r8
	int	totalWd = 0;	// r9
	CRawKeyIterator	iter(fKeyDefinitions);
	int	hEighths = 0;
	int	vEighths = RINT(GetArraySlot(iter.fRowDef, 0));	// key height
	do
	{
		if (iter.fColumn == 0)
		{
			rowWd =
			rowHt = 0;
			totalHt += keyVSize(RINT(GetArraySlot(iter.fRowDef, 0)));
		}
		int hSize = keyHSize(iter.keyFormat());	// r1
		int vSize = keyVSize(iter.keyFormat());	// r0
		rowWd += hSize;
		if (rowHt < vSize)
			rowHt = vSize;
		hEighths |= hSize;
		vEighths |= vSize;
		if (iter.fColumn >= iter.fNumOfColumns - 1)
			if (totalWd < rowWd)
				totalWd = rowWd;
	} while (iter.next());
//L162
	//sp-08
	const unsigned char	sp00[] = { 07, 00, 04, 00, 06, 00, 04, 00 };
	fUnitBounds.left = 0;
	fUnitBounds.right = (0xFFF8 | sp00[hEighths & 0x07]) & (((viewBounds.right - viewBounds.left) * 8) / totalWd);
	fUnitBounds.top = 0;
	fUnitBounds.bottom = (0xFFF8 | sp00[vEighths & 0x07]) & (((viewBounds.bottom - viewBounds.top) * 8) / totalHt);
}


/*--------------------------------------------------------------------------------
	Perform a command.
	If it’s a click then track the pen otherwise pass it on.
	Args:		inCmd		the command frame
	Return:	true if we handled the command
--------------------------------------------------------------------------------*/

bool
CKeyboardView::realDoCommand(RefArg inCmd)
{
	bool isHandled;

	if (CommandId(inCmd) == aeClick)
	{
		isHandled = trackStroke(((CUnit *)CommandParameter(inCmd))->stroke(), NULL);
		if (isHandled)
		{
			CommandSetResult(inCmd, true);
			return true;
		}
		// otherwise give superclass a chance to handle it
	}
	return CView::realDoCommand(inCmd);
}


/*--------------------------------------------------------------------------------
	Determine which virtual key a point lies within.
	Args:		inPt		the point to test
	Return:	CView of the key
--------------------------------------------------------------------------------*/

bool
CKeyboardView::insideView(Point inPt)
{
	if (PtInRect(inPt, &viewBounds))
	{
		CVisKeyIterator	iter(fKeyDefinitions, &fUnitBounds, inPt);
		return iter.findEnclosingKey(inPt);
	}
	return false;
}


bool
CKeyboardView::doKey(CVisKeyIterator & inIter)
{
	RefVar	keyResult(getResultRef(inIter));
	RefVar	keycode(MakeArray(1));
	SetArraySlot(keycode, 0, keyResult);
	bool		isDone;
	RefVar	scriptResult(runCacheScript(kIndexKeyPressScript, keycode, true, &isDone));
	if (isDone)
	{
		if (NOTNIL(GetProtoVariable(fContext, SYMA(newt_feature)))
		&&  NOTNIL(scriptResult))
			handleKeyPress(inIter, keycode);
	}
	else
		handleKeyPress(inIter, keycode);

	if (ISINT(keyResult)
	&&  fKeyResultsAreKeycodes)
		return IsModifierKeyCode(RINT(keyResult));
	return false;
}


void
CKeyboardView::handleKeyPress(CVisKeyIterator & inIter, RefArg inKeycode)
{
	//sp-0C
	UniChar	visKeycode = 0;
	ULong		existingModifiers = Modifiers(false);

	if (ISINT(inKeycode)
	&&  fKeyResultsAreKeycodes)
	{
		//sp-14
		bool	wasOptDown = KeyDown(0x3A, false);		// sp10
		bool	wasCmdDown = KeyDown(0x37, false);		// sp0C
		bool	wasCtrlDown = KeyDown(0x3B, false);		// sp08
		bool	wasShiftDown = KeyDown(0x38, false);	// r9
		bool	wasCapsDown = KeyDown(0x39, false);		// sp04
		ULong	oldState = gSoftKeyDeadState;

		bool	legendsChanged = false;
		int	keycode = RINT(inKeycode);
		switch(keycode)
		{
		case 0x37:	// command
			KeyIn(0x37, wasCmdDown, this);
			legendsChanged = true;
			break;
		case 0x38:	// shift
			KeyIn(0x38, wasShiftDown, this);
			if (wasCapsDown)
				KeyIn(0x39, false, this);
			legendsChanged = true;
			break;
		case 0x39:	// caps lock
			KeyIn(0x39, true, this);
			KeyIn(0x39, false, this);
			if (wasShiftDown)
				KeyIn(0x38, false, this);
			legendsChanged = true;
			break;
		case 0x3A:	// option
			KeyIn(0x3A, wasOptDown, this);
			legendsChanged = true;
			break;
		case 0x3B:	// control
			KeyIn(0x3B, wasCtrlDown, this);
			legendsChanged = true;
			break;
		default:
			visKeycode = KeyIn(keycode, true, this);
			KeyIn(keycode, false, this);
			if (wasShiftDown)
			{
				KeyIn(0x38, false, this);
				legendsChanged = true;
			}
			if (wasOptDown)
			{
				KeyIn(0x3A, false, this);
				legendsChanged = true;
			}
			if (wasCmdDown)
			{
				KeyIn(0x37, false, this);
				legendsChanged = true;
			}
			if (wasCtrlDown)
			{
				KeyIn(0x3B, false, this);
				legendsChanged = true;
			}
			break;
		}
//L153
		if (oldState != gSoftKeyDeadState
		||  legendsChanged)
		{
			dirty();
			gRootView->update();
		}
		if (visKeycode != 0)
		{
			// it’s an actual key and not just a modifier
			// keycode is actually new modifiers
			CView *	rcvr = GetKeyReceiver(fContext, fKeyReceiverView);
			if (rcvr != NULL)
			{
				if (fHasSound)
					FPlaySound(fContext, GetProtoVariable(fContext, SYMA(keySound)));
				ULong	keyCmd = ((0x0100 | (existingModifiers << 9) | keycode) << 16) | visKeycode;
				gApplication->dispatchCommand(MakeCommand(aeKeyDown, rcvr, keyCmd));
				if (GetView(fContext) != NULL
				&&  (rcvr = GetKeyReceiver(fContext, fKeyReceiverView)) != NULL)
					gApplication->dispatchCommand(MakeCommand(aeKeyUp, rcvr, keyCmd));
			}
			else
				gRootView->runScript(SYMA(SysBeep), MakeArray(0));
		}
	}

//L257
	else
	{
		if (fHasSound)
			FPlaySound(fContext, GetProtoVariable(fContext, SYMA(keySound)));
		postKeypressCommands(inKeycode);
	}
	gRootView->update();
}


void
CKeyboardView::postKeypressCommands(RefArg inKeyStr)
{
	UniChar		str[64], * strPtr;
	ArrayIndex	strLen;
	MakeStringObject(inKeyStr, str, &strLen, 63);
	for (strPtr = str; *strPtr != kEndOfString; strPtr++)
	{
		CView *	rcvr = GetKeyReceiver(fContext, fKeyReceiverView);
		if (rcvr == NULL)
			break;
		gApplication->dispatchCommand(MakeCommand(aeKeyDown, rcvr, *strPtr));
		gApplication->dispatchCommand(MakeCommand(aeKeyUp, rcvr, *strPtr));
	}
}


Ref
CKeyboardView::getLegendRef(CRawKeyIterator & inIter)
{
	RefVar	legend(inIter.keyLegend());
	if (ISNIL(legend))
		legend = inIter.keyResult();
	if (EQ(ClassOf(legend), SYMA(array)))
		legend = GetArraySlot(legend, fKeyArrayIndex);
	else if (IsFunction(legend))
	{
		legend = DoScript(fContext, legend, RA(NILREF));
		if (EQ(ClassOf(legend), SYMA(array)))
			legend = GetArraySlot(legend, fKeyArrayIndex);
	}
	return legend;
}


Ref
CKeyboardView::getResultRef(CRawKeyIterator & inIter)
{
	RefVar	result(inIter.keyResult());
	if (EQ(ClassOf(result), SYMA(array)))
		result = GetArraySlot(result, fKeyArrayIndex);
	else if (IsFunction(result))
	{
		result = DoScript(fContext, result, RA(NILREF));
		if (EQ(ClassOf(result), SYMA(array)))
			result = GetArraySlot(result, fKeyArrayIndex);
	}
	return result;
}


bool
CKeyboardView::trackStroke(CStroke * inStroke, CVisKeyIterator * inIter)
{
	// INCOMPLETE
	return true;
}


/*------------------------------------------------------------------------------
	Draw the entire keyboard.
	Args:		inRect	the rect in which to draw
	Return:	--
------------------------------------------------------------------------------*/

void
CKeyboardView::realDraw(Rect& inRect)
{
	RefVar	highlightedKeys(GetProtoVariable(fContext, SYMA(keyHighlightKeys)));	// r7
	long		numOfHighlightedKeys = NOTNIL(highlightedKeys) ? Length(highlightedKeys) : 0;
	RefVar	code(GetProtoVariable(fContext, SYMA(keyArrayIndex)));
	fKeyArrayIndex = NOTNIL(code) ? RINT(code) : 0;
	CGContextSaveGState(quartz);
	CGContextSetGrayFillColor(quartz, 0, 1);
	bool	more;
	CVisKeyIterator	iter(fKeyDefinitions, &fUnitBounds, *(Point*)&viewBounds);
	do
	{
		if (iter.fHasVisKeys
		&&  Intersects(iter.rowFrame(), &inRect))
		{
			if (Intersects(iter.keyFrame(), &inRect)
			&&  (iter.keyFormat() & keySpacer) == 0)
			{
				bool	isHighlighted = false;
				code = getResultRef(iter);
				for (ArrayIndex i = 0; i < numOfHighlightedKeys; ++i)
					if (EQ(GetArraySlot(highlightedKeys, i), code))
					{
						isHighlighted = true;
						break;
					}
				drawKey(iter, isHighlighted, false);
			}
			more = iter.next();
		}
		else
			more = iter.skipToStartOfNextRow();
	} while (more);
	CGContextRestoreGState(quartz);
}


/*------------------------------------------------------------------------------
	Draw a single key cap.
	The key cap legend can be:
		string
		character
		integer
		bitmap
	Args:		inIter				iterator representing the key
				inIsHighlighted	key is down so highlight it
				inDo3
	Return:	--
------------------------------------------------------------------------------*/

void
CKeyboardView::drawKey(CVisKeyIterator & inIter, bool inIsHighlighted, bool inDo3)
{
	bool			isHighlighted = inIsHighlighted;	// r10
	long			sp54;
	long			sp50;
//	CObjectPtr	sp44;				// need to keep a ref to strData until fn exit?
	UniChar		textBuf[32];	// sp04
	UniChar *	textPtr;			// r9
	long			textLen;			// r8
	long			keycode = -1;	// r7
	bool			isText = false;	// r6
	bool			isBitmap = false;	// sp00

	RefVar		legend(getLegendRef(inIter));

	if (EQ(ClassOf(legend), SYMA(string)))
	{
		isText = true;
		CDataPtr	strData(legend);
		textPtr = (UniChar *)(char *)strData;
		textLen = Length(strData) / sizeof(UniChar) - 1;
	}

	else if (ISCHAR(legend))
	{
		isText = true;
		textBuf[0] = RCHAR(legend);
		textPtr = textBuf;
		textLen = 1;
	}

	else if (ISINT(legend) && fKeyResultsAreKeycodes)
	{
		isText = true;
		keycode = RINT(legend);
		textBuf[0] = KeyLabel(keycode, false);
		textPtr = textBuf;
		textLen = 1;
	}

	else if (ISINT(legend))
	{
		isText = true;
		IntegerString(RINT(legend), textBuf);
		textPtr = textBuf;
		textLen = Ustrlen(textBuf);
	}

	else if (EQ(ClassOf(legend), SYMA(frame))
	     &&  FrameHasSlot(legend, SYMA(bits)))
	{
		Rect	bounds;
		isBitmap = true;
		FromObject(GetFrameSlot(legend, SYMA(bounds)), &bounds);
		sp54 = RectGetWidth(bounds);
		sp50 = RectGetHeight(bounds);
	}

	if (fKeyResultsAreKeycodes)
	{
		if (keycode == -1)
		{
			Ref	keyResult = getResultRef(inIter);
			if (ISINT(keyResult))
				keycode = RINT(keyResult);
		}
		if (keycode != -1
		&&  KeyDown(keycode, 0))
			isHighlighted = true;
	}

	// Finally! Start drawing - the frame first
	drawKeyFrame(inIter, isHighlighted, inDo3);

	// then the keycap - it’s either a bitmap or text
	if (isBitmap)
	{
		Rect	bounds;
		bounds.left = (inIter.fKeyCap.left + inIter.fKeyCap.right - sp54) / 2;
		bounds.right = bounds.left + sp54;
		bounds.top = (inIter.fKeyCap.top + inIter.fKeyCap.bottom - sp50) / 2;
		bounds.bottom = bounds.top + sp50;		
		DrawBitmap(legend, &bounds, isHighlighted ? modeBic : modeOr);
	}

	else if (isText)
	{
#if defined(correct)
		int	glyphHt = fFontInfo.ascent + fFontInfo.descent;
		fOptions.transferMode = isHighlighted ? modeBic : modeOr;
		fOptions.width = IntToFixed(RectGetWidth(inIter.fKeyCap) + 2*3);

		FixedPoint	location;
		location.x = inIter.fKeyCap.left - 3;
		location.y = fFontInfo.ascent + (inIter.fKeyCap.top + inIter.fKeyCap.bottom - glyphHt) / 2;

//		need to set up layout and style; possibly in init
		DrawTextOnce(textPtr, textLen, &fStylePtr, NULL, location, &fOptions, NULL);
#endif
	}
}


/*------------------------------------------------------------------------------
	Draw a single key frame.
	Args:		inIter				iterator representing the key
				inIsHighlighted	key is down so highlight it
				inDo3
	Return:	--
------------------------------------------------------------------------------*/

void
CKeyboardView::drawKeyFrame(CVisKeyIterator & inIter, bool inIsHighlighted, bool inDo3)
{
	Rect	frame = *inIter.keyFrame();
	int	rounding = keyRounding(inIter.keyFormat()) / 2 + 2;
	int	openings = keyOpenings(inIter.keyFormat());
	int	frameWd = keyFrameWd(inIter.keyFormat());

	CGContextSaveGState(quartz);
	if (openings != 0)
	{
		CGContextClipToRect(quartz, MakeCGRect(frame));
		if (openings & keyTopOpen)
			frame.top -= 100;
		if (openings & keyLeftOpen)
			frame.left -= 100;
		if (openings & keyBottomOpen)
			frame.bottom += 100;
		if (openings & keyRightOpen)
			frame.right += 100;
	}
	if (inIsHighlighted)
	{
		if (rounding != 0)
			FillRoundRect(frame, rounding, rounding);
		else
			FillRect(frame);
	}
	else if (frameWd != 0)
	{
		SetLineWidth(frameWd);
		if (rounding == 0)
		{
		//	if (inDo3 && (viewFormat & vfFillMask) == vfFillWhite)
		//		EraseRect(&frame);
			StrokeRect(frame);
		}
		else
		{
		//	if (inDo3 && (viewFormat & vfFillMask) == vfFillWhite)
		//		EraseRoundRect(&frame, rounding);
			StrokeRoundRect(frame, rounding, rounding);
		}
	}
	CGContextRestoreGState(quartz);
}

