/*
 *  main.m
 *  Newton
 *
 *  Created by simon on Wed Feb 06 2002.
 *
 */

#import <Cocoa/Cocoa.h>

#include "Objects.h"
#include "ObjHeader.h"
#include "ObjectHeap.h"
#include "Symbols.h"
#include "Iterators.h"
#include "Frames.h"
#include "Globals.h"
#include "NewtGlobals.h"
#include "Exceptions.h"
#include "Lookup.h"

#include "NSOF.h"

extern "C" {
void	InitMagicPointerTables(void);
void	InitPrinter(void);
void	InitCharEncoding(void);
void	InitInterpreter(void);
Ref	DoBlock(RefArg codeBlock, RefArg args);
void	PrintObject(Ref obj, int indent);
}

/*----------------------------------------------------------------------
	S t u b s

0x0C100B58	ClassInfoRegistrySeed
0x0C100B5C	NBPStartLookup

0x0C100B88	DumpDict
0x0C100BBC	OrigPhrase
0x0C100CB8	ConnEntriesEqual

0x0C100F8C	DSTime
0x0C100F90	DSDate
0x0C100F94	DSPhone

0x0C1010CC	GetHiliteIndex
0x0C1010D0	BookRemoved
0x0C10155C	NTKAlive
0x0C101690	FinishRecognizing
0x0C1016A8	GetOnlineEndpoint,LoadOnlinePackage
0x0C1016E0	GetPackageStores
0x0C101754	ExtractData
0x0C10180C	GetGlobals
0x0C101820	Print
0x0C101844	WRecIsBeingUsed
0x0C10187C	FinishRecognizing
0x0C1018CC	FlushStrokes,BlockStrokes,UnblockStrokes
0x0C101934	GetRoot
0x0C101940	ViewAutopsy
0x0C104D48	TestMDropConnection
0x0C104F50	EnablePowerStats
0x0C104F58	InRepeatedKeyCommand
0x0C105178	ExitBreakLoop
0x0C105458	CurrentException
0x0C105468	StackTrace,SetDebugMode
0x0C1054AC	AddDelayedAction
0x0C10550C	GetTrueModifiers
0x0C105524	modalstate
0x0C10596C	GetStores
0x0C105AF0	
0x0C1061C4	ResetPowerStats
0x0C106E88	FinishRecognizing
0x0C107800	GetSortID
----------------------------------------------------------------------*/

#if defined(correct)

Heap g0C1014FC;

void
InitToolbox(void)
{
	Heap saveHeap = GetHeap();
	if (NewVMHeap(0, 0xC000, &g0C1014FC, kHeapNotCacheable) == noErr)	// 48K
		SetHeap(g0C1014FC);
	InitGraf();
	SetHeap(saveHeap);
	InitFonts();
}

#endif /* correct */


/*----------------------------------------------------------------------
	P s e u d o - R O M   I n i t i a l i z a t i o n
----------------------------------------------------------------------*/
extern "C" {
void	InitStartupHeap(void);
void	InitBuiltInSymbols(void);
}

void
Startup()
{
	InitStartupHeap();
	InitBuiltInSymbols();
	InitCharEncoding();
}


void
Shutdown()
{
}


/*----------------------------------------------------------------------
	I n i t i a l i z a t i o n

	To initialize the object system:
	optionally call SetObjectHeapSize(yourFavoriteSize, long allocateInTempMemory = 1);
----------------------------------------------------------------------*/

void	InitObjects(void);

void
InitObjects(void)
{
	gRefStackPos = 1;
	GetNewtGlobals()->fLastId = 1;
	gMainHeap = new ObjectHeap(/*InternalRAMInfo(1, 0)*/ 16*KByte);

	AddGCRoot(&functions);
	AddGCRoot(&vars);
	vars = AllocateFrame();

	InitMagicPointerTables();
	FindOffsetCacheClear();
	InitSymbols();
	InitPrinter();
	InitClasses();
	InitInterpreter();
//	InitQueries();
}


/*----------------------------------------------------------------------
	T e s t   P r o g r a m
----------------------------------------------------------------------*/

int main(int argc, const char *argv[])
{
	int	mainResult;

	Startup();

	InitObjects();

	RegisterClass("company", "string");
	RegisterClass("address", "string");
	RegisterClass("title", "string");
	RegisterClass("name", "string");
	RegisterClass("phone", "string");
	RegisterClass("homePhone", "phone");
	RegisterClass("workPhone", "phone");
	RegisterClass("faxPhone", "phone");
	RegisterClass("otherPhone", "phone");
	RegisterClass("carPhone", "phone");
	RegisterClass("beeperPhone", "phone");
	RegisterClass("mobilePhone", "phone");
	RegisterClass("homeFaxPhone", "phone");

	SetFrameSlot(vars, RSYM(printDepth), MAKEINT(15));
	SetFrameSlot(vars, RSYM(prettyPrint), TRUEREFVAR);

#if 0
	NSAutoreleasePool * pool =[[NSAutoreleasePool alloc] init];
	FDObjectReader * reader = [[FDObjectReader alloc] init];
	NSString * file = [NSString stringWithCString: argv[1]];
	newton_try
	{
		Ref		result = [reader read: file];
	//	Ref		name = MakeSymbol("testFunction");
	//	BOOL		exists;
		RefVar	fn = AllocRefVar(GetArraySlotRef(result, 2));
		RefVar	args = AllocRefVar(AllocateArray(RSYM(array), 2));
		PrintObject(result, 0);
		fprintf(stdout, "\n");
		SetArraySlotRef(args->ref, 0, MAKEINT(1));
		SetArraySlotRef(args->ref, 1, MAKEINT(2));
		result = DoBlock(fn, args);
	//	gMainHeap->uriah();
	//	gMainHeap->uriahBinaryObjects();
		FreeRefVar(args);
		FreeRefVar(fn);
	}
	newton_catch_all
	{
		Exception *	x = CurrentException();
		fprintf(stdout, "Sorry, an exception has occurred. %s", x->name);
	}
	end_try;
	[reader dealloc];
	[pool release];
#endif

	mainResult = NSApplicationMain(argc, argv);

	Shutdown();

	return mainResult;
}
