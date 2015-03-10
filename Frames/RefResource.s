/*
	File:		RefResource.s

	Contains:	Ref resources -- symbols, strings, arrays and frames.

	Written by:	Newton Research Group, 2007.
*/
		.data
		.align	2

#define kFlagsBinary (0x40<<24)
#define kFlagsArray  (0x41<<24)
#define kFlagsFrame  (0x43<<24)

#if __LP64__
/* Refs are 64-bit, object header is long size|flags, quad gcStuff, quad class */
#define Ref .quad
#define kHeaderSize 20
#define ArrayObj(_len) \
		.long		kHeaderSize + (_len<<3) + kFlagsArray ;\
		Ref		0, MAKEPTR(SYMarray)
#define FrameMapObj(_len) \
		.long		kHeaderSize + ((_len+1)<<3) + kFlagsArray ;\
		Ref		0, MAKEINT(2), NILREF
#define FrameMapProtoObj(_len) \
		.long		kHeaderSize + ((_len+1)<<3) + kFlagsArray ;\
		Ref		0, MAKEINT(2+4), NILREF
#define FrameObj(_len, _map) \
		.long		kHeaderSize + (_len<<3) + kFlagsFrame ;\
		Ref		0, MAKEPTR(_map)

#else
/* Refs are 32-bit, object header is long size|flags, long gcStuff, long class */
#define Ref .long
#define kHeaderSize 12
#define ArrayObj(_len) \
		.long		kHeaderSize + (_len<<2) + kFlagsArray ;\
		Ref		0, MAKEPTR(SYMarray)
#define FrameMapObj(_len) \
		.long		kHeaderSize + ((_len+1)<<2) + kFlagsArray ;\
		Ref		0, MAKEINT(2), NILREF
#define FrameMapProtoObj(_len) \
		.long		kHeaderSize + ((_len+1)<<2) + kFlagsArray ;\
		Ref		0, MAKEINT(2+4), NILREF
#define FrameObj(_len, _map) \
		.long		kHeaderSize + (_len<<2) + kFlagsFrame ;\
		Ref		0, MAKEPTR(_map)
#endif

#define SymbolObj(_len) \
		.long		kHeaderSize + _len + kFlagsBinary ;\
		Ref		0, 0x55552
#define StringObj(_len) \
		.long		kHeaderSize + (_len<<1) + kFlagsBinary ;\
		Ref		0, MAKEPTR(SYMstring)

#define NILREF 2
#define MAKEINT(n) (n<<2)
#define MAKEMAGICPTR(n) (n<<2)+3
#define MAKEPTR(p) p+1


/* ----------------------------------------------------------------
	Symbols.
	Ref layout:
		long size|flags, quad gc, quad class, long hash, asciz name
---------------------------------------------------------------- */

#define DEFSYM(name, len, hash) \
SYM##name: \
		SymbolObj(len) ;\
		.long		hash ;\
		.asciz	#name ;\
		.align	2
#define DEFSYM_(name, sym, len, hash) \
SYM##name: \
		SymbolObj(len) ;\
		.long		hash ;\
		.asciz	sym ;\
		.align	2
#include "Symbol_Defs.h"
#undef DEFSYM
#undef DEFSYM_


/* ----------------------------------------------------------------
	Strings.
---------------------------------------------------------------- */

STRdrawing:
		StringObj(8)
		.short	'D','r','a','w','i','n','g',0
		.align	2

STRnan:
		StringObj(13)
		.short	'N','o','t',' ','a',' ','n','u','m','b','e','r',0
		.align	2

STRnTooLarge:
		StringObj(17)
		.short	'N','u','m','b','e','r',' ','t','o','o',' ','l','a','r','g','e',0
		.align	2

STRnTooSmall:
		StringObj(17)
		.short	'N','u','m','b','e','r',' ','t','o','o',' ','s','m','a','l','l',0
		.align	2

STRextrasSoupName:
		StringObj(9)
		.short	'P','a','c','k','a','g','e','s',0
		.align	2

STRspace:
		StringObj(2)
		.short	' ',0
		.align	2

STRuPackageNeedsCardAlertText:
		StringObj(117)
		.short	'T','h','e',' ','p','a','c','k','a','g','e',' ','"','^','0','"',' ','s','t','i','l','l',' ','n','e','e','d','s',' ','t','h','e',' ','c','a','r','d',' ','y','o','u',' ','r','e','m','o','v','e','d','.',' '
		.short	'P','l','e','a','s','e',' ','i','n','s','e','r','t',' ','i','t',' ','n','o','w',',',' ','o','r',' ','i','n','f','o','r','m','a','t','i','o','n',' ','o','n',' ','t','h','e',' ','c','a','r','d',' ','m','a','y',' ','b','e',' ','d','a','m','a','g','e','d','.',0
		.align	2


inkName:
		StringObj(11)
		.short	' ','-','s','k','e','t','c','h','-',' ',0
		.align	2


/* ----------------------------------------------------------------
	Arrays.
---------------------------------------------------------------- */

emptyArray:
		ArrayObj(0)

symbolTable:
		ArrayObj(0)		# should be populated, obviously
		Ref		NILREF
		Ref		NILREF

slotCacheTable:
		ArrayObj(35)
		Ref		MAKEPTR(SYMviewQuitScript)
		Ref		MAKEPTR(SYMstyles)
		Ref		MAKEPTR(SYMtabs)
		Ref		MAKEPTR(SYMrealData)
		Ref		MAKEPTR(SYMtext)
		Ref		MAKEPTR(SYMviewTransferMode)
		Ref		MAKEPTR(SYMviewFont)
		Ref		MAKEPTR(SYMviewOriginY)
		Ref		MAKEPTR(SYMviewOriginX)
		Ref		MAKEPTR(SYMviewJustify)
		Ref		MAKEPTR(SYMviewFlags)
		Ref		MAKEPTR(SYMviewDrawScript)
		Ref		MAKEPTR(SYMviewIdleScript)
		Ref		MAKEPTR(SYMviewSetupChildrenScript)
		Ref		MAKEPTR(SYMviewChildren)
		Ref		MAKEPTR(SYMbuttonClickScript)
		Ref		MAKEPTR(SYMviewClickScript)
		Ref		MAKEPTR(SYMviewFormat)
		Ref		MAKEPTR(SYMviewBounds)
		Ref		MAKEPTR(SYMviewShowScript)
		Ref		MAKEPTR(SYMviewStrokeScript)
		Ref		MAKEPTR(SYMviewGestureScript)
		Ref		MAKEPTR(SYMviewHiliteScript)
		Ref		MAKEPTR(SYMallocateContext)
		Ref		MAKEPTR(SYMkeyPressScript)
		Ref		MAKEPTR(SYMviewKeyUpScript)
		Ref		MAKEPTR(SYMviewKeyDownScript)
		Ref		MAKEPTR(SYMviewKeyRepeatScript)
		Ref		MAKEPTR(SYMstepAllocateContext)
		Ref		MAKEPTR(SYMviewChangedScript)
		Ref		MAKEPTR(SYMviewRawInkScript)
		Ref		MAKEPTR(SYMviewInkWordScript)
		Ref		MAKEPTR(SYMviewKeyStringScript)
		Ref		MAKEPTR(SYMviewCaretActivateScript)
		Ref		MAKEPTR(SYMview)

_clickSong:
		ArrayObj(17)
		Ref		MAKEINT(1),MAKEINT(4),MAKEINT(2),MAKEINT(2),MAKEINT(5),MAKEINT(0),MAKEINT(5),MAKEINT(5),MAKEINT(3),MAKEINT(2),MAKEINT(4),MAKEINT(0),MAKEINT(4),MAKEINT(2),MAKEINT(3),MAKEINT(2),MAKEINT(5)
_clicks:
		Ref		0	# this gets loaded in MakeROMResources()


/* ----------------------------------------------------------------
	Frames.
---------------------------------------------------------------- */

#define DEFFRAME(name, ref)
#define DEFFRAME1(name, tag1, value1) \
name##Map: \
		FrameMapObj(1) ;\
		Ref		MAKEPTR(SYM##tag1) ;\
name: \
		FrameObj(1, name##Map) ;\
		Ref		value1
#define DEFFRAME2(name, tag1, value1, tag2, value2) \
name##Map: \
		FrameMapObj(2) ;\
		Ref		MAKEPTR(SYM##tag1), MAKEPTR(SYM##tag2) ;\
name: \
		FrameObj(2, name##Map) ;\
		Ref		value1, value2
#define DEFFRAME3(name, tag1, value1, tag2, value2, tag3, value3) \
name##Map: \
		FrameMapObj(3) ;\
		Ref		MAKEPTR(SYM##tag1), MAKEPTR(SYM##tag2), MAKEPTR(SYM##tag3) ;\
name: \
		FrameObj(3, name##Map) ;\
		Ref		value1, value2, value3
#define DEFFRAME4(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4) \
name##Map: \
		FrameMapObj(4) ;\
		Ref		MAKEPTR(SYM##tag1), MAKEPTR(SYM##tag2), MAKEPTR(SYM##tag3), MAKEPTR(SYM##tag4) ;\
name: \
		FrameObj(4, name##Map) ;\
		Ref		value1, value2, value3, value4
#define DEFFRAME5(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5) \
name##Map: \
		FrameMapObj(5) ;\
		Ref		MAKEPTR(SYM##tag1), MAKEPTR(SYM##tag2), MAKEPTR(SYM##tag3), MAKEPTR(SYM##tag4), MAKEPTR(SYM##tag5) ;\
name: \
		FrameObj(5, name##Map) ;\
		Ref		value1, value2, value3, value4, value5
#define DEFPROTOFRAME5(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5) \
name##Map: \
		FrameMapProtoObj(5) ;\
		Ref		MAKEPTR(SYM##tag1), MAKEPTR(SYM##tag2), MAKEPTR(SYM##tag3), MAKEPTR(SYM##tag4), MAKEPTR(SYM##tag5) ;\
name: \
		FrameObj(5, name##Map) ;\
		Ref		value1, value2, value3, value4, value5
#define DEFFRAME6(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6) \
name##Map: \
		FrameMapObj(6) ;\
		Ref		MAKEPTR(SYM##tag1), MAKEPTR(SYM##tag2), MAKEPTR(SYM##tag3), MAKEPTR(SYM##tag4), MAKEPTR(SYM##tag5), MAKEPTR(SYM##tag6) ;\
name: \
		FrameObj(6, name##Map) ;\
		Ref		value1, value2, value3, value4, value5, value6
#define DEFFRAME7(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6, tag7, value7) \
name##Map: \
		FrameMapObj(7) ;\
		Ref		MAKEPTR(SYM##tag1), MAKEPTR(SYM##tag2), MAKEPTR(SYM##tag3), MAKEPTR(SYM##tag4), MAKEPTR(SYM##tag5), MAKEPTR(SYM##tag6), MAKEPTR(SYM##tag7) ;\
name: \
		FrameObj(7, name##Map) ;\
		Ref		value1, value2, value3, value4, value5, value6, value7
#define DEFFRAME8(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6, tag7, value7, tag8, value8) \
name##Map: \
		FrameMapObj(8) ;\
		Ref		MAKEPTR(SYM##tag1), MAKEPTR(SYM##tag2), MAKEPTR(SYM##tag3), MAKEPTR(SYM##tag4), MAKEPTR(SYM##tag5), MAKEPTR(SYM##tag6), MAKEPTR(SYM##tag7), MAKEPTR(SYM##tag8) ;\
name: \
		FrameObj(8, name##Map) ;\
		Ref		value1, value2, value3, value4, value5, value6, value7, value8
#define DEFFRAME10(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6, tag7, value7, tag8, value8, tag9, value9, tag10, value10) \
name##Map: \
		FrameMapObj(10) ;\
		Ref		MAKEPTR(SYM##tag1), MAKEPTR(SYM##tag2), MAKEPTR(SYM##tag3), MAKEPTR(SYM##tag4), MAKEPTR(SYM##tag5), MAKEPTR(SYM##tag6), MAKEPTR(SYM##tag7), MAKEPTR(SYM##tag8), MAKEPTR(SYM##tag9), MAKEPTR(SYM##tag10) ;\
name: \
		FrameObj(10, name##Map) ;\
		Ref		value1, value2, value3, value4, value5, value6, value7, value8, value9, value10
#define DEFFRAME17(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6, tag7, value7, tag8, value8, tag9, value9, tag10, value10, tag11, value11, tag12, value12, tag13, value13, tag14, value14, tag15, value15, tag16, value16, tag17, value17) \
name##Map: \
		FrameMapObj(17) ;\
		Ref		MAKEPTR(SYM##tag1), MAKEPTR(SYM##tag2), MAKEPTR(SYM##tag3), MAKEPTR(SYM##tag4), MAKEPTR(SYM##tag5), MAKEPTR(SYM##tag6), MAKEPTR(SYM##tag7), MAKEPTR(SYM##tag8), MAKEPTR(SYM##tag9), MAKEPTR(SYM##tag10), MAKEPTR(SYM##tag11), MAKEPTR(SYM##tag12), MAKEPTR(SYM##tag13), MAKEPTR(SYM##tag14), MAKEPTR(SYM##tag15), MAKEPTR(SYM##tag16), MAKEPTR(SYM##tag17) ;\
name: \
		FrameObj(17, name##Map) ;\
		Ref		value1, value2, value3, value4, value5, value6, value7, value8, value9, value10, value11, value12, value13, value14, value15, value16, value17
#define DEFPROTOFRAME10(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6, tag7, value7, tag8, value8, tag9, value9, tag10, value10) \
name##Map: \
		FrameMapProtoObj(10) ;\
		Ref		MAKEPTR(SYM##tag1), MAKEPTR(SYM##tag2), MAKEPTR(SYM##tag3), MAKEPTR(SYM##tag4), MAKEPTR(SYM##tag5), MAKEPTR(SYM##tag6), MAKEPTR(SYM##tag7), MAKEPTR(SYM##tag8), MAKEPTR(SYM##tag9), MAKEPTR(SYM##tag10) ;\
name: \
		FrameObj(10, name##Map) ;\
		Ref		value1, value2, value3, value4, value5, value6, value7, value8, value9, value10
#include "FrameDefs.h"
#undef DEFFRAME
#undef DEFFRAME1
#undef DEFFRAME2
#undef DEFFRAME3
#undef DEFFRAME4
#undef DEFFRAME5
#undef DEFPROTOFRAME5
#undef DEFFRAME6
#undef DEFFRAME7
#undef DEFFRAME8
#undef DEFFRAMEX
#undef DEFFRAME10
#undef DEFFRAME17
#undef DEFPROTOFRAME10


/* ----------------------------------------------------------------
	Phase II
	Create Refs and Ref stars for pseudo-RefArg creation.
---------------------------------------------------------------- */
#include "MagicPointers.h"

#define DEFSYM(name, len, hash) \
		.globl	_RSYM##name, _RSSYM##name ;\
_RSYM##name: \
		Ref		MAKEPTR(SYM##name) ;\
_RSSYM##name: \
		Ref		_RSYM##name
#define DEFSYM_(name, sym, len, hash) \
		.globl	_RSYM##name, _RSSYM##name ;\
_RSYM##name: \
		Ref		MAKEPTR(SYM##name) ;\
_RSSYM##name: \
		Ref		_RSYM##name
#include "Symbol_Defs.h"
#undef DEFSYM
#undef DEFSYM_


#define DEFSTR(name) \
		.globl	_RS##name ;\
_R##name: \
		Ref		MAKEPTR(##name) ;\
_RS##name: \
		Ref		_R##name
#define DEFMPSTR(name, str) \
		.globl	_RS##name ;\
_R##name: \
		Ref		str ;\
_RS##name: \
		Ref		_R##name
#include "StringDefs.h"
#undef DEFMPSTR
#undef DEFSTR


#define DEFARRAY(name) \
		.globl	_RS##name ;\
_R##name: \
		Ref		MAKEPTR(name) ;\
_RS##name: \
		Ref		_R##name
#define DEFFRAME(name, ref) \
		.globl	_RS##name ;\
_R##name: \
		Ref		ref ;\
_RS##name: \
		Ref		_R##name
#define DEFFRAME1(name, tag1, value1) \
		.globl	_RS##name ;\
_R##name: \
		Ref		MAKEPTR(name) ;\
_RS##name: \
		Ref		_R##name
#define DEFFRAME2(name, tag1, value1, tag2, value2) \
		.globl	_RS##name ;\
_R##name: \
		Ref		MAKEPTR(name) ;\
_RS##name: \
		Ref		_R##name
#define DEFFRAME3(name, tag1, value1, tag2, value2, tag3, value3) \
		.globl	_RS##name ;\
_R##name: \
		Ref		MAKEPTR(name) ;\
_RS##name: \
		Ref		_R##name
#define DEFFRAME4(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4) \
		.globl	_RS##name ;\
_R##name: \
		Ref		MAKEPTR(name) ;\
_RS##name: \
		Ref		_R##name
#define DEFFRAME5(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5) \
		.globl	_RS##name ;\
_R##name: \
		Ref		MAKEPTR(name) ;\
_RS##name: \
		Ref		_R##name
#define DEFPROTOFRAME5(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5) \
		.globl	_RS##name ;\
_R##name: \
		Ref		MAKEPTR(name) ;\
_RS##name: \
		Ref		_R##name
#define DEFFRAME6(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6) \
		.globl	_RS##name ;\
_R##name: \
		Ref		MAKEPTR(name) ;\
_RS##name: \
		Ref		_R##name
#define DEFFRAME7(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6, tag7, value7) \
		.globl	_RS##name ;\
_R##name: \
		Ref		MAKEPTR(name) ;\
_RS##name: \
		Ref		_R##name
#define DEFFRAME8(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6, tag7, value7, tag8, value8) \
		.globl	_RS##name ;\
_R##name: \
		Ref		MAKEPTR(name) ;\
_RS##name: \
		Ref		_R##name
#define DEFFRAME10(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6, tag7, value7, tag8, value8, tag9, value9, tag10, value10) \
		.globl	_RS##name ;\
_R##name: \
		Ref		MAKEPTR(name) ;\
_RS##name: \
		Ref		_R##name
#define DEFFRAME17(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6, tag7, value7, tag8, value8, tag9, value9, tag10, value10, tag11, value11, tag12, value12, tag13, value13, tag14, value14, tag15, value15, tag16, value16, tag17, value17) \
		.globl	_RS##name ;\
_R##name: \
		Ref		MAKEPTR(name) ;\
_RS##name: \
		Ref		_R##name
#define DEFPROTOFRAME10(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6, tag7, value7, tag8, value8, tag9, value9, tag10, value10) \
		.globl	_RS##name ;\
_R##name: \
		Ref		MAKEPTR(name) ;\
_RS##name: \
		Ref		_R##name
DEFARRAY(symbolTable)
DEFARRAY(slotCacheTable)
DEFARRAY(_clickSong)
DEFARRAY(_clicks)
#include "FrameDefs.h"
#undef DEFARRAY
#undef DEFFRAME
#undef DEFFRAME1
#undef DEFFRAME2
#undef DEFFRAME3
#undef DEFFRAME4
#undef DEFFRAME5
#undef DEFPROTOFRAME5
#undef DEFFRAME6
#undef DEFFRAME7
#undef DEFFRAME10
#undef DEFFRAME17
#undef DEFPROTOFRAME10

