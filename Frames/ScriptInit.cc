/*
	File:		ScriptInit.cc

	Contains:	Initialization of NewtonScript runtime.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "Globals.h"
#include "NewtGlobals.h"
#include "Frames.h"
#include "Funcs.h"
#include "ObjectHeap.h"
#include "Iterators.h"
#include "MagicPointers.h"
#include "RAMInfo.h"
#include "ROMResources.h"

#if !defined(correct)
extern void			MakeROMResources(void);
extern bool			EnableFramesFunctionProfiling(bool inEnable);
#endif
extern void			InitMagicPointerTables(void);
extern void			InitSymbols(void);
extern void			InitPrinter(void);
extern void			InitClasses(void);
extern void			InitInterpreter(void);
extern void			InitQueries(void);

extern Ref				gInheritanceFrame;
extern const Ref *	gTagCache;
extern ArrayIndex		gCurrentStackPos;


#if 0
/* -----------------------------------------------------------------------------
	Initialize the toolbox.
	Not referenced anywhere - maybe by alert manager?
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */
Heap gGrafHeap;

void
InitToolbox(void)
{
	Heap taskHeap = GetHeap();
	if (NewVMHeap(0, 48*KByte, &gGrafHeap, kHeapNotCacheable) == noErr)
		SetHeap(gGrafHeap);
	InitGraf();
	SetHeap(taskHeap);
	InitFonts();
}
#endif


/* -----------------------------------------------------------------------------
	Initialize the NewtonScript object system.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
InitObjects(void)
{
#if !defined(correct)
	MakeROMResources();		// load NS data
#endif

	gCurrentStackPos =
	gNewtGlobals->stackPos = 1;
	gHeap = new CObjectHeap(InternalRAMInfo(kFramesHeapAlloc), true);

	AddGCRoot(&gFunctionFrame);
	AddGCRoot(&(gVarFrame = AllocateFrame()));

	InitMagicPointerTables();
	FindOffsetCacheClear();
	InitSymbols();
	InitPrinter();
	InitClasses();
	InitInterpreter();
	InitQueries();
}


void
InitFormFunctions(RefArg inFuncs)
{ /* this really does nothing */ }


/* -----------------------------------------------------------------------------
	Initialize NewtonScript globals.
	This sets up the gVarFrame with predefined variables.
	Called from CNotebook::initToolbox() before other init.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
InitScriptGlobals(void)
{
//	Add all the (nil) slots we need to global vars
	RefVar	newVars(Clone(RA(varsMapStarter)));
	CObjectIterator	varsIter(gVarFrame);
	RefVar	tag, value;
	for ( ; !varsIter.done(); varsIter.next())
	{
		tag = varsIter.tag();
		value = varsIter.value();
		SetFrameSlot(newVars, tag, value);
	}
	ReplaceObject(gVarFrame, newVars);

//	Add string and phone classes
	RefVar	stringClasses(Clone(SYS_classes));
	gInheritanceFrame = stringClasses;
	DefGlobalVar(SYMA(classes), stringClasses);

	InitFormFunctions(RA(gFunctionFrame));
	gTagCache = Slots(RA(slotCacheTable));

//	Add extra global functions
	CObjectIterator	funcIter(RA(gFunky));
	for ( ; !funcIter.done(); funcIter.next())
	{
		tag = funcIter.tag();
		value = funcIter.value();
		SetFrameSlot(RA(gFunctionFrame), tag, value);
	}

#if defined(hasPureFunctionSupport)
//	Add constant functions
	DefGlobalVar(SYMA(constantFunctions), Clone(RA(constantFunctions)));
#endif

// •• This has already been done by REPInit()
	DefGlobalVar(SYMA(functions), RA(gFunctionFrame));
	DefGlobalVar(SYMA(vars), RA(gVarFrame));
// ••
//DefGlobalVar(SYMA(trace), SYMA(functions));	// switch on function tracing
//EnableFramesFunctionProfiling(true);

//	Set up key vars
	newton_try
	{
		DoBlock(RA(bootInitNSGlobals), RA(NILREF));
	}
	newton_catch(exRootException)
	{ }
	end_try;
}


/* -----------------------------------------------------------------------------
	Run initialization scripts.
	Called from CNotebook::initToolbox() after all other init.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
RunInitScripts(void)
{
	newton_try
	{
		DoBlock(RA(bootRunInitScripts), RA(NILREF));
	}
	newton_catch(exRootException)
	{ }
	end_try;
}
