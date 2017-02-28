/*
	File:		NewtGlobals.h

	Contains:	Newton global data declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__NEWTGLOBALS_H)
#define __NEWTGLOBALS_H 1


/*------------------------------------------------------------------------------
	D a t a   S t r u c t u r e s
------------------------------------------------------------------------------*/

class CInterpreter;
class CUPort;
class CApplication;

struct NewtGlobals
{
	ArrayIndex		stackPos;		// +00
	CInterpreter *	interpreter;	// +04
	VAddr				stackTop;		// +08
//	GrafPtr			graf;				// +0C
	Ptr				buf;				// +10
	Ptr				bufPtr;			// +14;
};


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

#if defined(hasPureFunctionSupport)
extern Ref					gConstFuncFrame;
extern Ref *				RSgConstFuncFrame;
#endif
extern Ref					gFunctionFrame;		// 0C10544C
extern Ref *				RSgFunctionFrame;
extern CInterpreter *	gInterpreter;			// 0C105458

extern CUPort *			gNewtPort;				// 0C1054A8
extern CApplication *	gApplication;			// +04 0C1054AC
extern NewtGlobals *		gNewtGlobals;			// +08 0C1054B0

class CNewtCardEventHandler;
extern CNewtCardEventHandler *	gCardEventHandler;	// +0C 0C1054B4
//extern						gExternalNewtEventHandlerList;	// +10 0C1054B8

extern unsigned char		gSoftKeyMap[32];		// +14 0C1054BC	256 keys as a bitmap
extern ULong				gSoftKeyDeadState;	//		 +20
extern bool					gSoftCapsLock;			//		 +24

extern unsigned char		gHardKeyMap[32];		// +3C 0C1054E4
extern ULong				gHardKeyDeadState;	//		 +20
extern bool					gHardCapsLock;			//		 +24

extern ULong				gTrueModifiers;		// +64 0C10550C


/*------------------------------------------------------------------------------
	I n l i n e   A c c e s s o r s
------------------------------------------------------------------------------*/

inline CUPort * GetNewtTaskPort()
{	return gNewtPort; }

inline NewtGlobals * GetNewtGlobals()
{	return gNewtGlobals; }

inline void SetNewtGlobals(NewtGlobals * inGlobals)
{	gNewtGlobals = inGlobals; }


#endif	/* __NEWTGLOBALS_H */
