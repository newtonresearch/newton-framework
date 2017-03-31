/*
	File:		REP.cc

	Contains:	Read-Evaluate-Print functions.

	Written by:	Newton Research Group.
*/

#include <stdarg.h>

#include "Objects.h"
#include "Globals.h"
#include "NewtGlobals.h"
#include "REPTranslators.h"
#include "Funcs.h"
#include "ROMResources.h"

/*----------------------------------------------------------------------
	D e c l a r a t i o n s
----------------------------------------------------------------------*/

DeclareException(exMessage, exRootException);
DeclareException(exCompiler, exRootException);
DeclareException(exInterpreter, exRootException);
DeclareException(exTranslator, exRootException);


/*----------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------*/

extern void			PrintObjectAux(Ref inObj, int indent, int inDepth);
extern "C" void	PrintObject(Ref inObj, int indent);

extern Ref			ParseString(RefArg inStr);

NewtonErr	CreateHammerInTranslator(PInTranslator ** outTranslator);
NewtonErr	CreateHammerOutTranslator(POutTranslator ** outTranslator);
NewtonErr	CreateNullInTranslator(PInTranslator ** outTranslator);
NewtonErr	CreateNullOutTranslator(POutTranslator ** outTranslator);
NewtonErr	CreateStdioInTranslator(PInTranslator ** outTranslator);
NewtonErr	CreateStdioOutTranslator(POutTranslator ** outTranslator);


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FBreakLoop(RefArg inRcvr);
Ref	FExitBreakLoop(RefArg inRcvr);
}

/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

ULong			gNewtConfig = 0x42;	// 000013FC

bool			gPrintLiterals = false;

Ref			gREPContext;			// 0C101810
int			gREPLevel;				// 0C101814

PInTranslator *	gREPin;			// 0C10181C
POutTranslator *	gREPout;			// 0C101820

bool *		gBreakLoopDone;		// 0C105178

#pragma mark -

/*------------------------------------------------------------------------------
	E r r o r   M e s s a g e s
	GetFramesErrorString is defined in ErrorString.m since it uses Cocoa.
------------------------------------------------------------------------------*/
extern "C" const char * GetFramesErrorString(NewtonErr inErr);

void
PrintFramesErrorMsg(const char * inStr, RefArg inData)
{
	char tagName[16];		// original allocs this when tagLen is known, but tag names are short
	const char * s;
	for (s = inStr; *s != 0; s++)
	{
		if (*s == '%')
		{
			const char * tagStart = s+1;
			const char * tagEnd = strchr(tagStart, '.');
			if (tagEnd)
			{
				size_t tagLen = tagEnd - tagStart;
				unwind_protect
				{
					strncpy(tagName, tagStart, tagLen);
					tagName[tagLen] = 0;
					PrintObject(GetFrameSlot(inData, MakeSymbol(tagName)), 0);
				}
				on_unwind
				{
					s = tagEnd;
				}
				end_unwind;
			}
			else
				gREPout->putc(*s);
		}
		else
			gREPout->putc(*s);
	}
}

#pragma mark -

/*------------------------------------------------------------------------------
	R e a d - E v a l u a t e - P r i n t   L o o p
------------------------------------------------------------------------------*/

// for NCX, NTX
FILE * logout = NULL;
void	RedirectStdioOutTranslator(FILE * inFRef);

void
CaptureStdioOutTranslator(const char * inFilename)
{
	logout = fopen(inFilename, "w");
	if (logout)
		RedirectStdioOutTranslator(logout);
}


void
EndCaptureStdioOutTranslator(void)
{
	if (logout)
	{
		fflush(logout);
		fclose(logout), logout = NULL;
		RedirectStdioOutTranslator(NULL);
	}
}


void
RedirectStdioOutTranslator(FILE * inFRef)
{
	if (inFRef == NULL)
	{
		inFRef = logout ? logout : stdout;
	}
	StdioTranslatorInfo	args = { inFRef };
	gREPout->init(&args);
}


/*------------------------------------------------------------------------------
	Initialize the REP input stream.
	Args:		--
	Return:	input translator
------------------------------------------------------------------------------*/

PInTranslator *
InitREPIn(void)
{
	PInTranslator *	translator = NULL;
	if (gNewtConfig & 0x02)
		CreateStdioInTranslator(&translator);
	else
		CreateNullInTranslator(&translator);
	return translator;
}


/*------------------------------------------------------------------------------
	Initialize the REP output stream.
	Args:		--
	Return:	output translator
------------------------------------------------------------------------------*/

POutTranslator *
InitREPOut(void)
{
	POutTranslator *	translator = NULL;
	if (gNewtConfig & 0x02)
		CreateStdioOutTranslator(&translator);
	else
		CreateNullOutTranslator(&translator);
	return translator;
}


/*------------------------------------------------------------------------------
	Reset the NTK event handler idle.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
ResetREPIdler(void)
{
/*
	Timeout	idleTime = REPTime();
	if (idleTime != 0)
		gREPEventHandler->resetIdle(idleTime);
	else
		gREPEventHandler->stopIdle();
*/
}


/*------------------------------------------------------------------------------
	Initialize the Read-Evaluate-Print context.
	Set up key NS variables in the global vars frame (see also InitScriptGlobals
	and RunInitScript which happen later at CNotebook init).
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
REPInit(void)
{
	if (gREPin == NULL)
		ThrowErr(exCompiler, kNSErrNoREP);

	DefGlobalVar(SYMA(vars), RA(gVarFrame));
	DefGlobalVar(SYMA(functions), RA(gFunctionFrame));

	DefGlobalVar(SYMA(trace), RA(NILREF));
	DefGlobalVar(SYMA(printDepth), MAKEINT(3));
	DefGlobalVar(SYMA(prettyPrint), RA(TRUEREF));

	gREPContext = gVarFrame;
	AddGCRoot(&gREPContext);
	gREPLevel = 0;

	gREPout->print("\nWelcome to NewtonScript!\n\n");
	gREPout->prompt(gREPLevel);
	gREPout->flush();
}


/*------------------------------------------------------------------------------
	Print a NewtonScript object.
	Args:		inObj		the object
				indent	number of spaces to indent after new line
	Return:	--
------------------------------------------------------------------------------*/

void
PrintObject(Ref inObj, int indent)
{
	gREPout->consumeFrame(inObj, 0, indent);
}


/*------------------------------------------------------------------------------
	Enter Read-Evaluate-Print break loop.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/
void	BreakLoop(void);

void
REPBreakLoop(void)
{
	unwind_protect
	{
		gREPout->enterBreakLoop(gREPLevel);
		BreakLoop();
	}
	on_unwind
	{
		gREPout->exitBreakLoop();
	}
	end_unwind;
}

void
BreakLoop(void)
{
	while (!*gBreakLoopDone)
		REPIdle();
}


/*------------------------------------------------------------------------------
	Handle Read-Evaluate-Print idle time.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
REPIdle(void)
{
	gREPin->idle();
	gREPout->idle();
	REPAcceptLine();
}


Timeout
REPTime(void)
{
	if (gREPin != NULL
	&& gREPout != NULL)
	{
		Timeout	timeIn = gREPin->idle();
		Timeout	timeOut = gREPout->idle();
		if (timeIn == 0)
			return timeOut;
		if (timeOut == 0)
			return timeIn;
		if (timeIn > timeOut)
			return timeOut;
		else
			return timeIn;
	}
	return 0;
}


/*------------------------------------------------------------------------------
	Read input and compile it;
	Evaluate the resulting codeblock;
	Print the result.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
REPAcceptLine(void)
{
	IncrementCurrentStackPos();
	newton_try
	{
		if (gREPin->frameAvailable())
		{
			int		indent;
			RefVar	result;
			RefVar	codeBlock(gREPin->produceFrame(0));
			if (NOTNIL(GetGlobalVar(MakeSymbol("showCodeBlocks"))))
			{
				gREPout->consumeFrame(codeBlock, 0, 0);
				gREPout->putc(0x0D);
			}
			if (NOTNIL(codeBlock))
			{
				result = InterpretBlock(codeBlock, gREPContext);
				indent = gREPout->print("#%-8lX ", (Ref)result);
				gREPout->consumeFrame(result, 0, indent);
				gREPout->putc(0x0D);
			}
			gREPout->prompt(gREPLevel);
			gREPout->flush();
		}
	}
	newton_catch_all
	{
		gREPout->exceptionNotify(CurrentException());
		gREPout->prompt(gREPLevel);
		gREPout->flush();
	}
	end_try;
	DecrementCurrentStackPos();
	ClearRefHandles();
}


/*------------------------------------------------------------------------------
	Read-Evaluate-Print output.
	Is this ever actually used in favour of direct calls to gREPout methods?
	The problem is passing on the variable number of args -- we create a vprint
	member function to handle this; you might also do it with assembler glue.
	Args:		inFormat		as for printf
	Return:	number of chars printed or error code (as for printf)
------------------------------------------------------------------------------*/

int
REPprintf(const char * inFormat, ...)
{
	int		result = 0;
	va_list	args;
	va_start(args, inFormat);
	result = gREPout->vprint(inFormat, args);
	va_end(args);
	return result;
}

void
REPflush(void)
{
	gREPout->flush();
}


/*------------------------------------------------------------------------------
	Trace Read-Evaluate-Print stack.
	Implemented in DebugAPI.cc which better encapsulates stack details.
	Args:		interpreter
	Return:	--
------------------------------------------------------------------------------

void
REPStackTrace(void * interpreter)
{}*/


/*------------------------------------------------------------------------------
	Print Read-Evaluate-Print exception.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/
#define kExceptionPrefixStr		"    !!! Exception: "
#define kExceptionErrorStr			"    !!! Exception: %s\n"
#define kExceptionErrorCodeStr	"    !!! Exception: %s (%d)\n"
#define kExceptionBuildErrorStr	"    File \"%s\"; Line %d !!! Exception: "

void
REPExceptionNotify(Exception * inException)
{
	const char * str;

	if (Subexception(inException->name, exMessage))
		gREPout->print(kExceptionErrorStr, (char *)inException->data);

	else if (Subexception(inException->name, exRefException))
	{
		RefVar data(*(RefStruct *)inException->data);
		if (IsFrame(data))
		{
			RefVar filename(GetFrameSlot(data, SYMA(filename)));
			RefVar lineNumber(GetFrameSlot(data, SYMA(lineNumber)));
			RefVar err(GetFrameSlot(data, SYMA(errorCode)));
			if (NOTNIL(filename) && NOTNIL(lineNumber))
			{
				RemoveSlot(data, SYMA(filename));
				RemoveSlot(data, SYMA(lineNumber));
				if (ISINT(err) && (str = GetFramesErrorString(RINT(err))) != NULL)
				{
					CDataPtr filenameStr(ASCIIString(filename));
					gREPout->print(kExceptionBuildErrorStr, (char *)filenameStr, RINT(lineNumber));
					PrintFramesErrorMsg(str, data);
					gREPout->print("\n");
				}
			}
			else if (ISINT(err) && (str = GetFramesErrorString(RINT(err))) != NULL)
			{
				gREPout->print(kExceptionPrefixStr);
				PrintFramesErrorMsg(str, data);
				gREPout->print("\n");
			}
			else
			{
				int indent = gREPout->print(kExceptionPrefixStr);
				PrintObject(data, indent);
				gREPout->print("\n");
			}
		}
		else
		{
			int indent = gREPout->print(kExceptionPrefixStr);
			PrintObject(data, indent);
			gREPout->print("\n");
		}
	}

	else if ((str = GetFramesErrorString((NewtonErr)(long)inException->data)) != NULL)
		gREPout->print(kExceptionErrorStr, str);

	else
		gREPout->print(kExceptionErrorCodeStr, (char *)inException->name, (NewtonErr)(long)inException->data);
}

#pragma mark -

/*------------------------------------------------------------------------------
	B r e a k   L o o p
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Enter break loop from NewtonScript.
	Args:		inRcvr		evaluation context
	Return:	NULL
------------------------------------------------------------------------------*/

Ref
FBreakLoop(RefArg inRcvr)
{
	RefVar	savedContext(gREPContext);
	gREPContext = inRcvr;
	gREPLevel++;

	bool		isDone = false;
	bool *	savedDone = gBreakLoopDone;
	gBreakLoopDone = &isDone;

	unwind_protect
	{
		REPBreakLoop();
	}
	on_unwind
	{
		gREPContext = savedContext;
		gREPLevel--;
		gBreakLoopDone = savedDone;
	}
	end_unwind;

	return NILREF;
}


/*------------------------------------------------------------------------------
	Exit break loop to NewtonScript.
	Args:		inRcvr		ignored
	Return:	NULL
------------------------------------------------------------------------------*/

Ref
FExitBreakLoop(RefArg inRcvr)
{
	if (gBreakLoopDone != NULL)
		*gBreakLoopDone = true;
	else
		ThrowErr(exInterpreter, kNSErrNotInBreakLoop);
	return NILREF;
}

#pragma mark -

/*------------------------------------------------------------------------------
	T r a n s l a t o r s
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	P H a m m e r I n T r a n s l a t o r
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Create the Hammer input stream.
	Args:		--
	Return:	Hammer input translator
------------------------------------------------------------------------------*/

NewtonErr
CreateHammerInTranslator(PInTranslator ** outTranslator)
{
	NewtonErr	err = noErr;
	XTRY
	{
		XFAILNOT(outTranslator, err = -1;)
		*outTranslator = (PInTranslator *)MakeByName("PInTranslator", "PHammerInTranslator");
		XFAILNOT(*outTranslator, err = MemError();)

		TranslatorInfo	args = { "%NewtonScript Listener", 1024 };
		XFAILIF(err = (*outTranslator)->init(&args), (*outTranslator)->destroy(); *outTranslator = NULL;)
	}
	XENDTRY;
	return err;
}

/* ----------------------------------------------------------------
	PHammerInTranslator implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
PHammerInTranslator::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN19PHammerInTranslator6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN19PHammerInTranslator4makeEv - 0b	\n"
"		.long		__ZN19PHammerInTranslator7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"PHammerInTranslator\"	\n"
"2:	.asciz	\"PInTranslator\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN19PHammerInTranslator9classInfoEv - 4b	\n"
"		.long		__ZN19PHammerInTranslator4makeEv - 4b	\n"
"		.long		__ZN19PHammerInTranslator7destroyEv - 4b	\n"
"		.long		__ZN19PHammerInTranslator4initEPv - 4b	\n"
"		.long		__ZN19PHammerInTranslator4idleEv - 4b	\n"
"		.long		__ZN19PHammerInTranslator14frameAvailableEv - 4b	\n"
"		.long		__ZN19PHammerInTranslator12produceFrameEi - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(PHammerInTranslator)

PHammerInTranslator *
PHammerInTranslator::make(void)
{
	fileRef = NULL;
	fBuf = NULL;
	fBufSize = 0;
	f1C = 0;
	f20 = 0;
	return this;
}

void
PHammerInTranslator::destroy(void)
{
	if (fileRef != NULL)
		fclose(fileRef);
	if (fBuf != NULL)
		FreePtr(fBuf), fBuf = NULL;
}

NewtonErr
PHammerInTranslator::init(void * inArgs)
{
	NewtonErr	err = noErr;
	XTRY
	{
		fBufSize = ((TranslatorInfo*)inArgs)->bufSize;
		fBuf = NewPtr(((TranslatorInfo*)inArgs)->bufSize);
		XFAILNOT(fBuf, err = MemError();)
	//	SetPtrName(fBuf, 'REPb');
		fileRef = fopen(((TranslatorInfo*)inArgs)->filename, "r");
	//	if (fileRef != NULL)
	//		set_input_notify(fileRef, &f1C);
	}
	XENDTRY;
	return err;
}

Timeout
PHammerInTranslator::idle(void)
{
	return 250*kMilliseconds;
}

bool
PHammerInTranslator::frameAvailable(void)
{
	return fileRef != NULL
	/*	&& (f1C - f20) != 0 */;
}

Ref
PHammerInTranslator::produceFrame(int inLevel)
{
	RefVar	result;

	if (fileRef != NULL
	&& fgets(fBuf, fBufSize, fileRef) != NULL)
	{
		f20 = f1C;
		result = ParseString(MakeStringFromCString(fBuf));
	}
	return result;
}

#pragma mark -

/*------------------------------------------------------------------------------
	P H a m m e r O u t T r a n s l a t o r
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Create the Hammer output stream.
	Args:		--
	Return:	Hammer output translator
------------------------------------------------------------------------------*/

NewtonErr
CreateHammerOutTranslator(POutTranslator ** outTranslator)
{
	NewtonErr	err = noErr;
	XTRY
	{
		XFAILNOT(outTranslator, err = -1;)
		*outTranslator = (POutTranslator *)MakeByName("POutTranslator", "PHammerOutTranslator");
		XFAILNOT(*outTranslator, err = MemError();)

		TranslatorInfo	args = { "%NewtonScript Listener", 256 };
		XFAILIF(err = (*outTranslator)->init(&args), (*outTranslator)->destroy(); *outTranslator = NULL;)
	}
	XENDTRY;
	return err;
}

/* ----------------------------------------------------------------
	PHammerOutTranslator implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
PHammerOutTranslator::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN20PHammerOutTranslator6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN20PHammerOutTranslator4makeEv - 0b	\n"
"		.long		__ZN20PHammerOutTranslator7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"PHammerOutTranslator\"	\n"
"2:	.asciz	\"POutTranslator\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN20PHammerOutTranslator9classInfoEv - 4b	\n"
"		.long		__ZN20PHammerOutTranslator4makeEv - 4b	\n"
"		.long		__ZN20PHammerOutTranslator7destroyEv - 4b	\n"
"		.long		__ZN20PHammerOutTranslator4initEPv - 4b	\n"
"		.long		__ZN20PHammerOutTranslator4idleEv - 4b	\n"
"		.long		__ZN20PHammerOutTranslator12consumeFrameERK6RefVarii - 4b	\n"
"		.long		__ZN20PHammerOutTranslator6promptEi - 4b	\n"
"		.long		__ZN20PHammerOutTranslator5printEPKcz - 4b	\n"
"		.long		__ZN20PHammerOutTranslator6vprintEPKcP13__va_list_tag - 4b	\n"
"		.long		__ZN20PHammerOutTranslator4putcEi - 4b	\n"
"		.long		__ZN20PHammerOutTranslator5flushEv - 4b	\n"
"		.long		__ZN20PHammerOutTranslator14enterBreakLoopEi - 4b	\n"
"		.long		__ZN20PHammerOutTranslator13exitBreakLoopEv - 4b	\n"
"		.long		__ZN20PHammerOutTranslator10stackTraceEPv - 4b	\n"
"		.long		__ZN20PHammerOutTranslator15exceptionNotifyEP9Exception - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(PHammerOutTranslator)

PHammerOutTranslator *
PHammerOutTranslator::make(void)
{
	fileRef = NULL;
	fBuf = NULL;
	return this;
}

void
PHammerOutTranslator::destroy(void)
{
	if (fileRef != NULL)
		fclose(fileRef);
	if (fBuf != NULL)
		FreePtr(fBuf), fBuf = NULL;
}

NewtonErr
PHammerOutTranslator::init(void * inArgs)
{
	NewtonErr	err = noErr;
	XTRY
	{
		fBuf = NewPtr(((TranslatorInfo*)inArgs)->bufSize);
		XFAILNOT(fBuf, err = MemError();)
	//	SetPtrName(fBuf, 'REPb');
		fileRef = fopen(((TranslatorInfo*)inArgs)->filename, "w");
		if (fileRef != NULL)
			setvbuf(fileRef, fBuf, 0x0200 /*_IOFBF?*/, ((TranslatorInfo*)inArgs)->bufSize);
		fileRef = stdout;
	}
	XENDTRY;
	return err;
}

Timeout
PHammerOutTranslator::idle(void)
{
	return 250*kMilliseconds;
}

void
PHammerOutTranslator::consumeFrame(RefArg inObj, int inDepth, int indent)
{
	if (fileRef != NULL)
		PrintObjectAux(inObj, indent, inDepth);
}

void
PHammerOutTranslator::prompt(int inLevel)
{
	if (fileRef != NULL)
		print("%7d > ", inLevel);
}

int
PHammerOutTranslator::vprint(const char * inFormat, va_list args)
{
	return vfprintf(fileRef, inFormat, args);
}

int
PHammerOutTranslator::print(const char * inFormat, ...)
{
	if (fileRef != NULL)
	{
		int		result;
		va_list	args;
		va_start(args, inFormat);
		result = vprint(inFormat, args);
		va_end(args);
		return result;
	}
	return 0;
}

int
PHammerOutTranslator::putc(int inCh)
{
	if (fileRef != NULL)
		return fputc(inCh, fileRef);
	return 0;
}

void
PHammerOutTranslator::flush(void)
{
	if (fileRef != NULL)
	{
		NewtonErr	err;
		if ((err = fflush(fileRef)) != noErr)
			ThrowErr(exTranslator, err);
	}
}

void
PHammerOutTranslator::enterBreakLoop(int inLevel)
{
	print("Entering break loop\n");
	prompt(inLevel);
	flush();
}

void
PHammerOutTranslator::exitBreakLoop(void)
{
	print("Exiting break loop\n");
}


void
PHammerOutTranslator::stackTrace(void * interpreter)
{
	REPStackTrace(interpreter);
}

void
PHammerOutTranslator::exceptionNotify(Exception * inException)
{
	REPExceptionNotify(inException);
}

#pragma mark -

/*------------------------------------------------------------------------------
	P N u l l I n T r a n s l a t o r
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Create a null input stream.
	Args:		--
	Return:	null input translator
------------------------------------------------------------------------------*/

NewtonErr
CreateNullInTranslator(PInTranslator ** outTranslator)
{
	NewtonErr	err = noErr;
	XTRY
	{
		XFAILNOT(outTranslator, err = -1;)
		*outTranslator = (PInTranslator *)MakeByName("PInTranslator", "PNullInTranslator");
		XFAILNOT(*outTranslator, err = MemError();)

		XFAILIF(err = (*outTranslator)->init(NULL), (*outTranslator)->destroy(); *outTranslator = NULL;)
	}
	XENDTRY;
	return err;
}

/* ----------------------------------------------------------------
	PNullInTranslator implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
PNullInTranslator::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN17PNullInTranslator6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN17PNullInTranslator4makeEv - 0b	\n"
"		.long		__ZN17PNullInTranslator7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"PNullInTranslator\"	\n"
"2:	.asciz	\"PInTranslator\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN17PNullInTranslator9classInfoEv - 4b	\n"
"		.long		__ZN17PNullInTranslator4makeEv - 4b	\n"
"		.long		__ZN17PNullInTranslator7destroyEv - 4b	\n"
"		.long		__ZN17PNullInTranslator4initEPv - 4b	\n"
"		.long		__ZN17PNullInTranslator4idleEv - 4b	\n"
"		.long		__ZN17PNullInTranslator14frameAvailableEv - 4b	\n"
"		.long		__ZN17PNullInTranslator12produceFrameEi - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(PNullInTranslator)

PNullInTranslator *
PNullInTranslator::make(void)
{ return this; }

void
PNullInTranslator::destroy(void)
{ }

NewtonErr
PNullInTranslator::init(void * inArgs)
{ return noErr; }

Timeout
PNullInTranslator::idle(void)
{ return kNoTimeout; }

bool
PNullInTranslator::frameAvailable(void)
{ return false; }

Ref
PNullInTranslator::produceFrame(int inLevel)
{ return NILREF; }

#pragma mark -

/*------------------------------------------------------------------------------
	P N u l l O u t T r a n s l a t o r
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Create a null output stream.
	Args:		--
	Return:	null output translator
------------------------------------------------------------------------------*/

NewtonErr
CreateNullOutTranslator(POutTranslator ** outTranslator)
{
	NewtonErr	err = noErr;
	XTRY
	{
		XFAILNOT(outTranslator, err = -1;)
		*outTranslator = (POutTranslator *)MakeByName("POutTranslator", "PNullOutTranslator");
		XFAILNOT(*outTranslator, err = MemError();)

		XFAILIF(err = (*outTranslator)->init(NULL), (*outTranslator)->destroy(); *outTranslator = NULL;)
	}
	XENDTRY;
	return err;
}

/* ----------------------------------------------------------------
	PNullOutTranslator implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
PNullOutTranslator::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN18PNullOutTranslator6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN18PNullOutTranslator4makeEv - 0b	\n"
"		.long		__ZN18PNullOutTranslator7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"PNullOutTranslator\"	\n"
"2:	.asciz	\"POutTranslator\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN18PNullOutTranslator9classInfoEv - 4b	\n"
"		.long		__ZN18PNullOutTranslator4makeEv - 4b	\n"
"		.long		__ZN18PNullOutTranslator7destroyEv - 4b	\n"
"		.long		__ZN18PNullOutTranslator4initEPv - 4b	\n"
"		.long		__ZN18PNullOutTranslator4idleEv - 4b	\n"
"		.long		__ZN18PNullOutTranslator12consumeFrameERK6RefVarii - 4b	\n"
"		.long		__ZN18PNullOutTranslator6promptEi - 4b	\n"
"		.long		__ZN18PNullOutTranslator5printEPKcz - 4b	\n"
"		.long		__ZN18PNullOutTranslator6vprintEPKcP13__va_list_tag - 4b	\n"
"		.long		__ZN18PNullOutTranslator4putcEi - 4b	\n"
"		.long		__ZN18PNullOutTranslator5flushEv - 4b	\n"
"		.long		__ZN18PNullOutTranslator14enterBreakLoopEi - 4b	\n"
"		.long		__ZN18PNullOutTranslator13exitBreakLoopEv - 4b	\n"
"		.long		__ZN18PNullOutTranslator10stackTraceEPv - 4b	\n"
"		.long		__ZN18PNullOutTranslator15exceptionNotifyEP9Exception - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(PNullOutTranslator)

PNullOutTranslator *
PNullOutTranslator::make(void)
{ return this; }

void
PNullOutTranslator::destroy(void)
{ }

NewtonErr
PNullOutTranslator::init(void * inArgs)
{ fIsInBreakLoop = false; return noErr; }

Timeout
PNullOutTranslator::idle(void)
{
	if (fIsInBreakLoop)
	{
		if (gBreakLoopDone != NULL)
			*gBreakLoopDone = true;
	}
	return kNoTimeout;
}

void
PNullOutTranslator::consumeFrame(RefArg inObj, int inDepth, int indent)
{ }

void
PNullOutTranslator::prompt(int inLevel)
{ }

int
PNullOutTranslator::vprint(const char * inFormat, va_list args)
{ return 0; }

int
PNullOutTranslator::print(const char * inFormat, ...)
{ return 0; }

int
PNullOutTranslator::putc(int inCh)
{ return 0; }

void
PNullOutTranslator::flush(void)
{ }

void
PNullOutTranslator::enterBreakLoop(int inLevel)
{ fIsInBreakLoop = true; }

void
PNullOutTranslator::exitBreakLoop(void)
{ fIsInBreakLoop = false; }

void
PNullOutTranslator::stackTrace(void * interpreter)
{ }

void
PNullOutTranslator::exceptionNotify(Exception * inException)
{ }

#pragma mark -

/*------------------------------------------------------------------------------
	P S t d i o I n T r a n s l a t o r
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Create the stdio input stream.
	Args:		--
	Return:	stdio input translator
------------------------------------------------------------------------------*/

NewtonErr
CreateStdioInTranslator(PInTranslator ** outTranslator)
{
	NewtonErr	err = noErr;
	XTRY
	{
		XFAILNOT(outTranslator, err = -1;)
		*outTranslator = (PInTranslator *)MakeByName("PInTranslator", "PStdioInTranslator");
		XFAILNOT(*outTranslator, err = MemError();)

		StdioTranslatorInfo	args = { stdin, 1024 };
		XFAILIF(err = (*outTranslator)->init(&args), (*outTranslator)->destroy(); *outTranslator = NULL;)
	}
	XENDTRY;
	return err;
}

/* ----------------------------------------------------------------
	PStdioInTranslator implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
PStdioInTranslator::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN18PStdioInTranslator6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN18PStdioInTranslator4makeEv - 0b	\n"
"		.long		__ZN18PStdioInTranslator7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"PStdioInTranslator\"	\n"
"2:	.asciz	\"PInTranslator\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN18PStdioInTranslator9classInfoEv - 4b	\n"
"		.long		__ZN18PStdioInTranslator4makeEv - 4b	\n"
"		.long		__ZN18PStdioInTranslator7destroyEv - 4b	\n"
"		.long		__ZN18PStdioInTranslator4initEPv - 4b	\n"
"		.long		__ZN18PStdioInTranslator4idleEv - 4b	\n"
"		.long		__ZN18PStdioInTranslator14frameAvailableEv - 4b	\n"
"		.long		__ZN18PStdioInTranslator12produceFrameEi - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(PStdioInTranslator)

PStdioInTranslator *
PStdioInTranslator::make(void)
{
	fileRef = NULL;
	fBuf = NULL;
	return this;
}

void
PStdioInTranslator::destroy(void)
{
	if (fBuf != NULL)
		FreePtr(fBuf), fBuf = NULL;
}

NewtonErr
PStdioInTranslator::init(void * inArgs)
{
	NewtonErr	err = noErr;
	XTRY
	{
		fileRef = ((StdioTranslatorInfo*)inArgs)->fref;
		fBufSize = ((StdioTranslatorInfo*)inArgs)->bufSize;
		fBuf = NewPtr(fBufSize);
		XFAILNOT(fBuf, err = MemError();)
	}
	XENDTRY;
	return noErr;
}

Timeout
PStdioInTranslator::idle(void)
{
	return kNoTimeout;
}

bool
PStdioInTranslator::frameAvailable(void)
{
	return fileRef != NULL
		&& feof(fileRef) == 0;
}

Ref
PStdioInTranslator::produceFrame(int inLevel)
{
	RefVar	result;

	if (fileRef != NULL
	&& fgets(fBuf, fBufSize, fileRef) != NULL)
	{
		result = ParseString(MakeStringFromCString(fBuf));
	}
	return result;
}

#pragma mark -

/*------------------------------------------------------------------------------
	P S t d i o O u t T r a n s l a t o r
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Create the stdio output stream.
	Args:		--
	Return:	stdio output translator
------------------------------------------------------------------------------*/

NewtonErr
CreateStdioOutTranslator(POutTranslator ** outTranslator)
{
	NewtonErr	err = noErr;
	XTRY
	{
		XFAILNOT(outTranslator, err = -1;)
		*outTranslator = (POutTranslator *)MakeByName("POutTranslator", "PStdioOutTranslator");
		XFAILNOT(*outTranslator, err = MemError();)

		StdioTranslatorInfo	args = { stdout };
		XFAILIF(err = (*outTranslator)->init(&args), (*outTranslator)->destroy(); *outTranslator = NULL;)
	}
	XENDTRY;
	return err;
}

/* ----------------------------------------------------------------
	PStdioOutTranslator implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
PStdioOutTranslator::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN19PStdioOutTranslator6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN19PStdioOutTranslator4makeEv - 0b	\n"
"		.long		__ZN19PStdioOutTranslator7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"PStdioOutTranslator\"	\n"
"2:	.asciz	\"POutTranslator\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN19PStdioOutTranslator9classInfoEv - 4b	\n"
"		.long		__ZN19PStdioOutTranslator4makeEv - 4b	\n"
"		.long		__ZN19PStdioOutTranslator7destroyEv - 4b	\n"
"		.long		__ZN19PStdioOutTranslator4initEPv - 4b	\n"
"		.long		__ZN19PStdioOutTranslator4idleEv - 4b	\n"
"		.long		__ZN19PStdioOutTranslator12consumeFrameERK6RefVarii - 4b	\n"
"		.long		__ZN19PStdioOutTranslator6promptEi - 4b	\n"
"		.long		__ZN19PStdioOutTranslator5printEPKcz - 4b	\n"
"		.long		__ZN19PStdioOutTranslator6vprintEPKcP13__va_list_tag - 4b	\n"
"		.long		__ZN19PStdioOutTranslator4putcEi - 4b	\n"
"		.long		__ZN19PStdioOutTranslator5flushEv - 4b	\n"
"		.long		__ZN19PStdioOutTranslator14enterBreakLoopEi - 4b	\n"
"		.long		__ZN19PStdioOutTranslator13exitBreakLoopEv - 4b	\n"
"		.long		__ZN19PStdioOutTranslator10stackTraceEPv - 4b	\n"
"		.long		__ZN19PStdioOutTranslator15exceptionNotifyEP9Exception - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(PStdioOutTranslator)

PStdioOutTranslator *
PStdioOutTranslator::make(void)
{
	fileRef = NULL;
	return this;
}

void
PStdioOutTranslator::destroy(void)
{ }

NewtonErr
PStdioOutTranslator::init(void * inArgs)
{
	fileRef = ((StdioTranslatorInfo*)inArgs)->fref;
	return noErr;
}

Timeout
PStdioOutTranslator::idle(void)
{
	return kNoTimeout;
}

void
PStdioOutTranslator::consumeFrame(RefArg inObj, int inDepth, int indent)
{
	if (fileRef != NULL)
		PrintObjectAux(inObj, indent, inDepth);
}

void
PStdioOutTranslator::prompt(int inLevel)
{ }

int
PStdioOutTranslator::vprint(const char * inFormat, va_list args)
{
	return vfprintf(fileRef, inFormat, args);
}

int
PStdioOutTranslator::print(const char * inFormat, ...)
{
	if (fileRef != NULL)
	{
		int		result = 0;
		va_list	args;
		va_start(args, inFormat);
		result = vprint(inFormat, args);
		va_end(args);
		return result;
	}
	return 0;
}

int
PStdioOutTranslator::putc(int inCh)
{
	if (fileRef != NULL)
		return fputc(inCh, fileRef);
	return 0;
}

void
PStdioOutTranslator::flush(void)
{
	if (fileRef != NULL)
	{
		NewtonErr	err;
		if ((err = fflush(fileRef)) != noErr)
			ThrowErr(exTranslator, err);
	}
}

void
PStdioOutTranslator::enterBreakLoop(int inLevel)
{
	print("Entering break loop\n");
	prompt(inLevel);
	flush();
}

void
PStdioOutTranslator::exitBreakLoop(void)
{
	print("Exiting break loop\n");
}


void
PStdioOutTranslator::stackTrace(void * interpreter)
{
	REPStackTrace(interpreter);
}

void
PStdioOutTranslator::exceptionNotify(Exception * inException)
{
	REPExceptionNotify(inException);
}

