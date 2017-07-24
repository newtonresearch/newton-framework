/*
	File:		RefStar.s

	Contains:	Newton object system ROM Ref* data.
					This file is built automatically by BuildRefStarDefs()
					from the ROM image and RS symbols.
	Written by:	Newton Research Group, 2016.
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
		.long		kHeaderSize + 4 + _len + kFlagsBinary ;\
		Ref		0, 0x55552
#define StringObj(_len) \
		.long		kHeaderSize + ((_len+1)<<1) + kFlagsBinary ;\
		Ref		0, MAKEPTR(SYMstring)

#define NILREF 2
#define MAKEINT(n) (n<<2)
#define MAKEMAGICPTR(n) (n<<2)+3
#define MAKEPTR(p) p+1


SYMsymbol:		SymbolObj(7)
		.long		0x79D979A6
		.asciz	"symbol"
		.align	2
SYMframe:		SymbolObj(6)
		.long		0x58A99953
		.asciz	"frame"
		.align	2
SYMReal:		SymbolObj(5)
		.long		0x7746D704
		.asciz	"Real"
		.align	2
SYM_weakarray:		SymbolObj(11)
		.long		0x5BBA0556
		.asciz	"_weakarray"
		.align	2


		.globl	_RNILREF, _RSNILREF
_RNILREF:		Ref	NILREF
_RSNILREF:		Ref	_RNILREF
		.globl	_RSSYMsymbol
_RSYMsymbol:		Ref	MAKEPTR(SYMsymbol)
_RSSYMsymbol:		Ref	_RSYMsymbol
		.globl	_RSSYMframe
_RSYMframe:		Ref	MAKEPTR(SYMframe)
_RSSYMframe:		Ref	_RSYMframe
		.globl	_RSSYMReal
_RSYMReal:		Ref	MAKEPTR(SYMReal)
_RSSYMReal:		Ref	_RSYMReal
		.globl	_RSSYM_weakarray
_RSYM_weakarray:		Ref	MAKEPTR(SYM_weakarray)
_RSSYM_weakarray:		Ref	_RSYM_weakarray

