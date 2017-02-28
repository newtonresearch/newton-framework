/*
	File:		Debugger.h

	Contains:	Debugger definitions.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__DEBUGGER_H)
#define __DEBUGGER_H 1

extern ULong	gDebuggerBits;			// 000013F4

#define kDebuggerIsPresent				0x01
#define kDebuggerUseARMisticeCard	0x08
#define kDebuggerUseExtSerial			0x20
#define kDebuggerUseGeoPort			0x40

extern ULong	gNewtTests;				// 000013F8

#endif	/* __DEBUGGER_H */
