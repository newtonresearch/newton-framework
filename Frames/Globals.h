/*
	File:		Globals.h

	Contains:	NewtonScript global data.

	Written by:	Newton Research Group.
*/

#if !defined(__GLOBALS__)
#define __GLOBALS__ 1


/*------------------------------------------------------------------------------
	G l o b a l   V a r i a b l e s
------------------------------------------------------------------------------*/

extern Ref		gVarFrame;
extern Ref *	RSgVarFrame;

inline Ref		DefGlobalVar(RefArg tag, RefArg value) { SetFrameSlot(RA(gVarFrame), tag, value); return value; }
inline Ref		GetGlobalVar(RefArg tag) { return GetFrameSlot(RA(gVarFrame), tag); }


/* -----------------------------------------------------------------------------
	D e b u g   O u t p u t
----------------------------------------------------------------------------- */

#if defined(__cplusplus)
extern "C" {
#endif

void		PrintObject(Ref obj, int indent);
int		REPprintf(const char * inFormat, ...);
void		REPflush(void);

#if defined(__cplusplus)
}
#endif


#endif	/* __GLOBALS__ */
