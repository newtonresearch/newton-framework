/*
	File:		Exceptions.c

	Contains:	Newton exception interface.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "UserGlobals.h"
#include "NewtGlobals.h"
#include "ROMResources.h"
#include "Interpreter.h"
#include "OSErrors.h"
#include "ErrorNotify.h"
#include "DeveloperNotification.h"
#include "RichStrings.h"
#include "Unicode.h"

/*----------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------*/

extern void		DeleteRefStruct(RefStruct * r);

void				SetExceptionHandler(CatchHeader * inHandler);
CatchHeader *	GetExceptionHandler(void);

void	UnhandledException(ExceptionName name, void * data, ExceptionDestructor destructor);
void	UnhandledNonUserModeException(ExceptionName name, void * data, ExceptionDestructor destructor);


/*----------------------------------------------------------------------
	E x c e p t i o n   D e f i n i t i o n s
----------------------------------------------------------------------*/
DeclareException(exComm, exRootException);
DeclareException(exBadArgs, exRootException);
DeclareException(exBadType, exRootException);
DeclareException(exInterpreter, exRootException);
DeclareException(exInterpreterData, exRootException);
DeclareException(exCompiler, exRootException);
DeclareException(exCompilerData, exRootException);
DeclareException(exPipe, exRootException);
DeclareException(exTranslator, exRootException);

DefineException(evRootEvent,			evt);

DefineException(exRootException,		evt.ex);
DefineException(exSkia,					evt.ex.skia);
DefineException(exMessage,				evt.ex.msg);
DefineException(exDivideByZero,		evt.ex.div0);
DefineException(exAbort,				evt.ex.abt);
DefineException(exBusError,			evt.ex.abt.bus);
DefineException(exAlignment,			evt.ex.abt.align);
DefineException(exIllegalInstr,		evt.ex.abt.instr);
DefineException(exPermissionViolation,	evt.ex.abt.perm);
DefineException(exWriteProtected,	evt.ex.abt.wp);
DefineException(exOutOfStack,			evt.ex.abt.stack);

DefineException(exLongError,			evt.ex.longerror);
DefineException(exError,				evt.ex.error);
DefineException(exNoSoupDef,			evt.ex.nosoupdef);
DefineException(exRoot,					evt.ex.root);
DefineException(exBadArgs,				evt.ex.bad.args);
DefineException(exGraf,					evt.ex.graf);
DefineException(exComm,					evt.ex.comm);
DefineException(exNewtException,		evt.ex.msg.newt);

DefineException(exRefException,		type.ref);

DefineException(exFrames,				evt.ex.fr);
DefineException(exFramesData,			evt.ex.fr;type.ref.frame);
DefineException(exBadType,				evt.ex.fr.type);
DefineException(exBadTypeData,		evt.ex.fr.type;type.ref.frame);
DefineException(exInterpreter,		evt.ex.fr.intrp);
DefineException(exInterpreterData,	evt.ex.fr.intrp;type.ref.frame);
DefineException(exCompiler,			evt.ex.fr.comp);
DefineException(exCompilerData,		evt.ex.fr.comp;type.ref.frame);
DefineException(exStore,				evt.ex.fr.store);

DefineException(exOutOfMemory,		evt.ex.outofmem);
DefineException(exStdIO,				evt.ex.stdio);
DefineException(exComp,					evt.ex.comp);
DefineException(exPipe,					evt.ex.pipe);
DefineException(exTranslator,			evt.ex.translator);


/*----------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------*/

CatchHeader * gFIQHandler;	// 0C100D20
CatchHeader * gIRQHandler;	// 0C100D24

#define kExceptionNameLen 127
char gFramesExceptionName[kExceptionNameLen + 1];


/*----------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
----------------------------------------------------------------------*/

extern "C" {
Ref	FThrow(RefArg rcvr, RefArg inName, RefArg inData);
Ref	FRethrow(RefArg rcvr);
Ref	FCurrentException(RefArg rcvr);
Ref	FIsSubexception(RefArg rcvr, RefArg inName, RefArg inSuper);
}

void
DeleteCharArray(void * inChars)
{
	FreePtr((Ptr)inChars);
}


Ref
FThrow(RefArg rcvr, RefArg inName, RefArg inData)
{
	// validate args
	if (!IsSymbol(inName))
		ThrowBadTypeWithFrameData(kNSErrNotASymbol, inName);	// cf FIsSubexception which uses ThrowExFramesWithBadValue
	if (!Subexception(SymbolName(inName), exRootException))
		ThrowExFramesWithBadValue(kNSErrBadExceptionName, inName);

	// copy the exception name
	strncpy(gFramesExceptionName, SymbolName(inName), kExceptionNameLen);

	// handle differrent types of exception:
	// ref
	if (Subexception(gFramesExceptionName, exRefException))
		ThrowRefException(gFramesExceptionName, inName);

	// message
	else if (Subexception(gFramesExceptionName, exMessage))
	{
		if (!IsString(inData))
			ThrowBadTypeWithFrameData(kNSErrNotAString, inData);
		CRichString richStr(inData);
		ArrayIndex strLen = richStr.length() + 1;
		char * str = NewPtr(strLen);
		if (str == NULL)
			OutOfMemory();
		ConvertFromUnicode((const UniChar *)BinaryData(inData), str);
		Throw(gFramesExceptionName, str, DeleteCharArray);
	}

	// everything else -- assume error code
	// this is not actually the case -- sometimes we see object pointers
	else
		ThrowErr(gFramesExceptionName, RINT(inData));

	return NILREF;
}


Ref
FRethrow(RefArg rcvr)
{
	// validate the current exception
	RefVar currentException(gInterpreter->exceptionBeingHandled());
	if (ISNIL(currentException))
		ThrowErr(exInterpreter, kNSErrNoCurrentException);

	// copy the exception name
	strncpy(gFramesExceptionName, SymbolName(GetFrameSlot(currentException, SYMA(name))), kExceptionNameLen);

	// handle differrent types of exception:
	// message
	if (Subexception(gFramesExceptionName, exMessage))
	{
		RefVar msg(GetFrameSlot(currentException, SYMA(message)));
		if (!IsString(msg))
			ThrowBadTypeWithFrameData(kNSErrNotAString, msg);
		CRichString richStr(msg);
		ArrayIndex strLen = richStr.length() + 1;
		char * str = NewPtr(strLen);
		if (str == NULL)
			OutOfMemory();
		ConvertFromUnicode((const UniChar *)BinaryData(msg), str);
		Throw(gFramesExceptionName, str, DeleteCharArray);
	}

	// ref
	else if (Subexception(gFramesExceptionName, exRefException))
		ThrowRefException(gFramesExceptionName, GetFrameSlot(currentException, SYMA(name)));

	// everything else -- assume error code
	else
		ThrowErr(gFramesExceptionName, RINT(GetFrameSlot(currentException, SYMA(error))));
}


Ref
FCurrentException(RefArg rcvr)
{ return gInterpreter->exceptionBeingHandled(); }


Ref
FIsSubexception(RefArg rcvr, RefArg inName, RefArg inSuper)
{
	if (!IsSymbol(inName))
		ThrowExFramesWithBadValue(kNSErrNotASymbol, inName);
	if (!IsSymbol(inSuper))
		ThrowExFramesWithBadValue(kNSErrNotASymbol, inSuper);
	return MAKEBOOLEAN(Subexception(SymbolName(inName), SymbolName(inSuper)));
}


/*----------------------------------------------------------------------
	O b j e c t   S t o r e   E r r o r
----------------------------------------------------------------------*/

void
ThrowOSErr(NewtonErr err)
{
	Throw(exStore, (void *)(long)err, (ExceptionDestructor) NULL);
}


/*----------------------------------------------------------------------
	U s e r   N o t i f i c a t i o n
----------------------------------------------------------------------*/

NewtonErr
GetExceptionErr(Exception * inException)
{
	NewtonErr	err = -8007;

	if (Subexception(inException->name, exOutOfMemory))
	{
		if (inException->data == NULL)
			err = kOSErrNoMemory;
	}
	else if (Subexception(inException->name, exRefException))
	{
		RefVar	excData = *(RefVar *)inException->data;
		if (IsFrame(excData))
			err = RINT(GetFrameSlot(excData, SYMA(errorCode)));
	}
	else if (!Subexception(inException->name, exMessage))
	{
		err = (NewtonErr)(long)inException->data;
	}

	return err;
}


void
ExceptionNotify(Exception * inException)
{
	Ptr preflightCHeap = NewPtr(1*KByte);
	if (preflightCHeap == NULL)
		OutOfMemory();
	FreePtr(preflightCHeap);

	RefVar	preflightNSHeap(MakeArray(256));
	preflightNSHeap = NILREF;

	NewtonErr	excError = GetExceptionErr(inException);
	RefVar		excName;
	RefVar		excMessage;
	RefVar		excData;

	if (Subexception(inException->name, exMessage))
	{
		excMessage = MakeStringFromCString((const char *)inException->data);
	}
	else if (Subexception(inException->name, exRefException))
	{
		excData = *(RefVar *)inException->data;
		if (IsFrame(excData))
			excMessage = GetFrameSlot(excData, SYMA(message));
	}
	excName = MakeStringFromCString((const char *)inException->name);

	DefGlobalVar(SYMA(lastEx), excName);
	DefGlobalVar(SYMA(lastExMessage), excMessage);
	DefGlobalVar(SYMA(lastExError), MAKEINT(excError));
	DefGlobalVar(SYMA(lastExData), excData);
#if !defined(forFramework)
//	SetPort(gScreenPort);
//	ReleaseScreenLock();
	ActionErrorNotify(excError, kNotifyAlert);
#endif
}


void
SafeExceptionNotify(Exception * inException)
{
	newton_try
	{
		ExceptionNotify(inException);
	}
	newton_catch(exRootException)
	{ }
	end_try;
}


#pragma mark -
/*----------------------------------------------------------------------
	E x c e p t i o n   T h r o w i n g   F u n c t i o n s
----------------------------------------------------------------------*/

void
ThrowBadTypeWithFrameData(NewtonErr errorCode, RefArg value)
{
	RefVar fr(AllocateFrame());
	SetFrameSlot(fr, SYMA(errorCode), MAKEINT(errorCode));
	SetFrameSlot(fr, SYMA(value), value);
	ThrowRefException(exBadTypeData, fr);
}


void
ThrowExFramesWithBadValue(NewtonErr errorCode, RefArg value)
{
	RefVar fr(AllocateFrame());
	SetFrameSlot(fr, SYMA(errorCode), MAKEINT(errorCode));
	SetFrameSlot(fr, SYMA(value), value);
	ThrowRefException(exBadTypeData, fr);
}


void
ThrowExCompilerWithBadValue(NewtonErr errorCode, RefArg value)
{
	RefVar fr(AllocateFrame());
	SetFrameSlot(fr, SYMA(errorCode), MAKEINT(errorCode));
	SetFrameSlot(fr, SYMA(value), value);
	ThrowRefException(exCompilerData, fr);
}


void
ThrowExInterpreterWithSymbol(NewtonErr errorCode, RefArg value)
{
	RefVar fr(AllocateFrame());
	SetFrameSlot(fr, SYMA(errorCode), MAKEINT(errorCode));
	SetFrameSlot(fr, SYMA(value), value);
	ThrowRefException(exInterpreterData, fr);
}


void
ThrowOutOfBoundsException(RefArg value, ArrayIndex index)
{
	RefVar fr(AllocateFrame());
	SetFrameSlot(fr, SYMA(errorCode), MAKEINT(kNSErrOutOfBounds));
	SetFrameSlot(fr, SYMA(value), value);
	SetFrameSlot(fr, SYMA(index), MAKEINT(index));
	ThrowRefException(exFramesData, fr);
}


/*----------------------------------------------------------------------
	Common Ref exception thrower.
	At this point weÕve got a named exception and a frame with the
	approved slots.
----------------------------------------------------------------------*/

void
ThrowRefException(ExceptionName name, RefArg data)
{
	if (!(Subexception(name, exRootException) && Subexception(name, exRefException)))
		ThrowExFramesWithBadValue(kNSErrBadExceptionName, MakeSymbol(name));

	RefStruct * r = new RefStruct(data);
	Throw(name, r, (ExceptionDestructor) DeleteRefStruct);
}


/*----------------------------------------------------------------------
	Throw an exception with a text message.
	Useful for debugging more than anything else.
----------------------------------------------------------------------*/

void
ThrowMsg(const char * msg)
{
	Throw(exMessage, (void *)msg, NULL);
}


/*----------------------------------------------------------------------
	Throw an exception.
	This is where all the specialized exceptions end up.
	We look for the first exception handler in the handler list,
	unlink it and execute it.
----------------------------------------------------------------------*/

void
Throw(ExceptionName name, void * data, ExceptionDestructor destructor)
{
	int	mode;
	CatchHeader *	handler = GetExceptionHandler();

	while (handler != NULL)
	{
		if (handler->catchType == kExceptionHandler)
		{
			// Got an exception handler.
			// Remove it from the list.
			SetExceptionHandler(handler->next);
			// set ths handlerÕs exception parameters
			((NewtonExceptionHandler *)handler)->exception.name = name;
			((NewtonExceptionHandler *)handler)->exception.data = data;
			((NewtonExceptionHandler *)handler)->exception.destructor = destructor;
			// jump to the handler code
			longjmp((int *)((NewtonExceptionHandler *)handler)->state, 1);
			//	never returns!
		}	
		else if (handler->catchType == kExceptionCleanup)
		{
			ExceptionCleanup * cleanupHandler = (ExceptionCleanup *)handler;
			cleanupHandler->header.catchType = kExceptionCleanupDone;
			cleanupHandler->function(cleanupHandler->object);

			CatchHeader * firstHandler = GetExceptionHandler();
			if (handler == firstHandler)
				handler = handler->next;
			else
				handler = firstHandler;
		}
		else
			break;
	}

	// no exception handler was found
	mode = GetCPUMode();
	if (mode == kUserMode)
		UnhandledException(name, data, destructor);
	else if (mode == kFIQMode || mode == kIRQMode)
		UnhandledNonUserModeException(name, data, destructor);
}


void
UnhandledException(ExceptionName name, void * data, ExceptionDestructor destructor)
{
#if !defined(forFramework)
	if (gCurrentMonitorId)
		MonitorThrowSWI(name, data, destructor);

	else
#endif
	{
#if defined(correct)
		char	buf[256];	// sic; presumably this was originally output somehow
		sprintf(buf,
#else
		REPprintf(
#endif
				 "Unhandled exception %s -- warm reboot!\n", Subexception(name, exMessage) ? (char *)data : name);
		ForgetDeveloperNotified(name);
		if (data != NULL && destructor != NULL)
			destructor(data);
#if defined(correct)
		EnterFIQAtomic();
		Reboot(kOSErrSorrySystemError);
#else
		exit(1);
#endif
	}
}

void
UnhandledNonUserModeException(ExceptionName name, void * data, ExceptionDestructor destructor)
{
#if defined(correct)
	char	buf[256];	// sic; presumably this was originally output somehow
	sprintf(buf,
#else
	REPprintf(
#endif
			"Unhandled exception %s -- warm reboot!\n", Subexception(name, exMessage) ? (char *)data : name);
	ForgetDeveloperNotified(name);
	if (data != NULL && destructor != NULL)
		destructor(data);
#if defined(correct)
	EnterFIQAtomic();
	Reboot(kOSErrSorrySystemError);
#else
	exit(1);
#endif
}


#pragma mark -
/*----------------------------------------------------------------------
	Determine whether exception is subexception of another.
----------------------------------------------------------------------*/

bool
Subexception(ExceptionName name, ExceptionName super)
{
	const char * delim;
	do
	{
		if (strncmp(super, name, strlen(super)) == 0)
			return true;
		if ((delim = strchr(name, ';')) != NULL)
			name = delim + 1;
	} while (delim != NULL);
	return false;
}


/*----------------------------------------------------------------------
	Set the current exception handler list.
----------------------------------------------------------------------*/

void
SetExceptionHandler(CatchHeader * inHandler)
{
	int mode = GetCPUMode();
	if (mode == kUserMode)
		gFirstCatch = inHandler;
	else if (mode == kFIQMode)
		gFIQHandler = inHandler;
	else if (mode == kIRQMode)
		gIRQHandler = inHandler;
}


/*----------------------------------------------------------------------
	Return the current exception handler list.
----------------------------------------------------------------------*/

CatchHeader *
GetExceptionHandler()
{
	return gFirstCatch;

	int mode = GetCPUMode();
	if (mode == kUserMode)
		return gFirstCatch;
	else if (mode == kFIQMode)
		return gFIQHandler;
	else if (mode == kIRQMode)
		return gIRQHandler;
	return NULL;
}


/*----------------------------------------------------------------------
	Add a new exception handler to the list.
----------------------------------------------------------------------*/

void
AddExceptionHandler(CatchHeader * inHandler)
{
	inHandler->next = GetExceptionHandler();
	SetExceptionHandler(inHandler);
}


/*----------------------------------------------------------------------
	Remove an exception handler from the list.
----------------------------------------------------------------------*/

void
RemoveExceptionHandler(CatchHeader * inHandler)
{
	CatchHeader * prevHandler;
	CatchHeader * handler = GetExceptionHandler();

	if (handler == inHandler)
		// handler is top of chain
		SetExceptionHandler(inHandler->next);

	else while (handler != NULL)
	{
		prevHandler = handler;
		handler = handler->next;
		if (handler != NULL
		&&  handler == inHandler)
		{
			// remove handler from the chain
			prevHandler->next = handler->next;
			break;
		}
	}
}


/*----------------------------------------------------------------------
	Rethrow exception to the next handler.
----------------------------------------------------------------------*/

void
NextHandler(NewtonExceptionHandler * inHandler)
{
	Throw(inHandler->exception.name, inHandler->exception.data, inHandler->exception.destructor);
}


/*----------------------------------------------------------------------
	Finish using an exception handler.
----------------------------------------------------------------------*/

void
ExitHandler(NewtonExceptionHandler * inHandler)
{
	CatchHeader * handler;
	// scan chain for first non-cleanup handler
	for (handler = GetExceptionHandler(); handler != NULL && handler->catchType == kExceptionCleanup; handler = handler->next)
		;

	if (handler == (CatchHeader *)inHandler)
		// itÕs first in the list so just remove it
		RemoveExceptionHandler((CatchHeader *)inHandler);

	else
	{
		// itÕs not so call its destructor
		ForgetDeveloperNotified(inHandler->exception.name);
//		if (inHandler->exception.destructor != NULL && inHandler->exception.data != NULL)
//			inHandler->exception.destructor(inHandler->exception.data);
	}
}

#pragma mark -

NewtonErr
FramesException(Exception * inException)
{
	NewtonErr err = -1;
	if (Subexception(inException->name, exRefException))
	{
		RefVar excData = *(RefVar *)inException->data;
		if (IsFrame(excData)
		&&  FrameHasSlot(excData, SYMA(errorCode)))
			err = RINT(GetFrameSlot(excData, SYMA(errorCode)));
	}
	else
		err = (NewtonErr)(long)inException->data;
	return err;
}
