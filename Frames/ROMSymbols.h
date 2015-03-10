/*
	File:		ROMSymbols.h

	Contains:	Built-in symbol declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__ROMSYMBOLS__)
#define __ROMSYMBOLS__ 1

#define DEFSYM(name) extern Ref RSYM##name; extern Ref * RSSYM##name;
#define DEFSYM_(name, sym) extern Ref RSYM##name; extern Ref * RSSYM##name;
#include "SymbolDefs.h"
#undef DEFSYM
#undef DEFSYM_

#define SYMA(name) RA(SYM##name)

#endif	/* __ROMSYMBOLS__ */
