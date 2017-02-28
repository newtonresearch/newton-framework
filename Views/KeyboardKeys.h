/*
	File:		KeyboardKeys.h

	Contains:	Keyboard cmd-key declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__KEYBOARDKEYS_H)
#define __KEYBOARDKEYS_H 1


/*------------------------------------------------------------------------------
	K e y   M o d i f i e r s
------------------------------------------------------------------------------*/

#define kKeyModifierShift 25

#define kCmdModifier		 (1 << 25)
#define kShiftModifier	 (1 << 26)
//#define kUnknownModifier	 (1 << 27)
#define kOptionModifier	 (1 << 28)
#define kControlModifier (1 << 29)

#define kKeyModifierMask (0x1F << kKeyModifierShift)


UniChar	GetDisplayCmdChar(RefArg inKey);
ULong	KeyCommandModifiers(RefArg inKey);
ArrayIndex	FindKeyCommandInArray(RefArg inArray, UniChar inChar, ULong inModifiers, int * outNumOfModifiersFound, bool * outIsExactMatch);
int	GetCommandCharWidth(RefArg inChar, StyleRecord * inStyle);
int	GetModifiersWidth(RefArg inKeyCode);
Ref	MatchKeyMessage(CView * inKeyView, RefArg inMsg, ULong);
Ref	SendKeyMessage(CView * inKeyView, RefArg inMsg);


#endif	/* __KEYBOARDKEYS_H */
