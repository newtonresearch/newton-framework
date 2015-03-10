/*
	File:		NTK.cc

	Contains:	Newton ToolKit interface.

	Written by:	Newton Research Group, 2007.
*/

#include <stdarg.h>

#include "Objects.h"
#include "Globals.h"
#include "Unicode.h"
#include "NTK.h"
#include "NameServer.h"
#include "OptionArray.h"
#include "StreamObjects.h"
#include "NewtonScript.h"
#include "Funcs.h"
#include "DockerErrors.h"
#include "ErrorNotify.h"

/*----------------------------------------------------------------------
	D e c l a r a t i o n s
----------------------------------------------------------------------*/

DeclareException(exTranslator, exRootException);
DeclareException(exRefException, exFrames);

/*----------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------*/

extern void			PrintObjectAux(Ref inObj, int indent, int inDepth);
extern "C" void	PrintObject(Ref inObj, int indent);

extern Ref			ParseString(RefArg inStr);
extern int			NewPackage(CPipe * inPipe, RefArg inStore, RefArg inCallback, size_t inFreq);

NewtonErr	CreateSerialInTranslator(PInTranslator ** outTranslator, CTaskSafeRingBuffer * inBuffer);
NewtonErr	CreateSerialOutTranslator(POutTranslator ** outTranslator, CTaskSafeRingBuffer * inBuffer);
NewtonErr	CreateNTKInTranslator(PInTranslator ** outTranslator, char * inTranslatorName, CTaskSafeRingBuffer * inBuffer);
NewtonErr	CreateNTKOutTranslator(POutTranslator ** outTranslator, char * inTranslatorName, CTaskSafeRingBuffer * inBuffer);

NewtonErr	CreateNub(RefArg inConnectionKind, RefArg inMachineName, RefArg inInputTranslator, RefArg inOutputTranslator);

NewtonErr	NubADSPOptions(COptionArray*, COptionArray*);

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FSetUpTetheredListener(RefArg rcvr, RefArg inStart, RefArg inConnectionKind, RefArg inMachineName);
Ref	FpkgDownload(RefArg rcvr, RefArg inConnectionKind, RefArg inMachineName);

Ref	FNTKListener(RefArg rcvr, RefArg inStart, RefArg inConnectionKind, RefArg inMachineName, RefArg inInputTranslator, RefArg inOutputTranslator);
Ref	FNTKDownload(RefArg rcvr, RefArg inConnectionKind, RefArg inMachineName, RefArg inInputTranslator, RefArg inOutputTranslator);
Ref	FNTKSend(RefArg rcvr, RefArg inRef);
Ref	FNTKAlive(RefArg rcvr);
}


/*------------------------------------------------------------------------------
	D a t a
	NTK I/O is handled by the NTK nub.
------------------------------------------------------------------------------*/
extern Ref	gREPContext;

CNTKNub *	gNTKNub;


/* -----------------------------------------------------------------------------
	Initialize NTK I/O.
	Register translators, init event handler.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
NTKInit(void)
{
	PHammerInTranslator::classInfo()->registerProtocol();
	PHammerOutTranslator::classInfo()->registerProtocol();
	PNullInTranslator::classInfo()->registerProtocol();
	PNullOutTranslator::classInfo()->registerProtocol();
	PStdioInTranslator::classInfo()->registerProtocol();
	PStdioOutTranslator::classInfo()->registerProtocol();
#if 0
	PSerialInTranslator::classInfo()->registerProtocol();
	PSerialOutTranslator::classInfo()->registerProtocol();
	PNTKInTranslator::classInfo()->registerProtocol();
	PNTKOutTranslator::classInfo()->registerProtocol();

	gREPEventHandler = new CREPEventHandler;
	gREPEventHandler->init('rep ');
	gREPEventHandler->initIdler(0, 0, NO);
#endif
}

#if !defined(forFramework)
/* -----------------------------------------------------------------------------
	Shut down NTK I/O. Usually as a result of I/O translator error.
	Close the NTK nub.
	Args:		inError
	Return:	--
----------------------------------------------------------------------------- */

void
NTKShutdown(NewtonErr inError)
{
	if (gNTKNub != NULL)
	{
		gNTKNub->stopListener();
		delete gNTKNub, gNTKNub = NULL;

		if (inError != 0
		&&  inError != -1
		&&  inError != -36006)	// Operation not supported in the current tool state
			ErrorNotify(inError, kNotifyAlert);
	}
}


NewtonErr
NTKSendStackTrace(RefArg inFrames)
{
	return gNTKNub->sendRef('fstk', inFrames);
}


NewtonErr
NTKStackTrace(void * interpreter)
{
#if 0
	CNSDebugAPI debugAPI(interpreter);
	ArrayIndex count = debugAPI->numStackFrames();
	RefVar stackFrames(MakeArray(count));
	ArrayIndex index = count-1;
	for (ArrayIndex i = 0; i < count; ++i, --index)
	{
		SetArraySlot(stackFrames, index, NTKStackFrameInfo(debugAPI, i));
	}
#else
	RefVar stackFrames;
#endif
	return NTKSendStackTrace(stackFrames);
}

#pragma mark -
/* -----------------------------------------------------------------------------
	Create the NTK nub, responsible for NTK I/O.
	Called from NewtonScript.
	Args:		inConnectionKind
				inMachineName
				inInputTranslator
				inOutputTranslator
	Return:	--
----------------------------------------------------------------------------- */

NewtonErr
CreateNub(RefArg inConnectionKind, RefArg inMachineName, RefArg inInputTranslator, RefArg inOutputTranslator)
{
#define kTranslatorNameMaxLen 63
	NewtonErr		err;
	bool				isSerial = NO;
	char				inputTranslatorName[kTranslatorNameMaxLen+1];
	char				outputTranslatorName[kTranslatorNameMaxLen+1];
	char *			inputTranslator = NULL;
	char *			outputTranslator = NULL;
	COptionArray *	sp08 = NULL;
	COptionArray *	sp04 = NULL;
	COptionArray *	sp00 = NULL;

	XTRY
	{
		// create the nub
		gNTKNub = new CNTKNub;
		XFAILNOT(gNTKNub, err = MemError();)

		if (NOTNIL(inInputTranslator) && NOTNIL(inOutputTranslator))
		{
			// create the specified I/O translators
			ConvertFromUnicode(GetUString(inInputTranslator), &inputTranslatorName, kTranslatorNameMaxLen);	// !! original does not specify encoding
			ConvertFromUnicode(GetUString(inOutputTranslator), &outputTranslatorName, kTranslatorNameMaxLen);	// !! original does not specify encoding
			inputTranslator = inputTranslatorName;
			outputTranslator = outputTranslatorName;
		}
#if 0
		if (ISINT(inConnectionKind))
		{
			// set up connection options for specified connection kind
			sp08 = new COptionArray;
			XFAILNOT(sp08, err = MemError();)
			XFAIL(err = sp08->init())
			switch (RINT(inConnectionKind))
			{
				case 0:
					XFAIL(err = EzSerialOptions(sp08, NULL, 0, 0))
					break;

				case 2:
					isSerial = YES;
					// fall thru…

				case 1:
					sp00 = new COptionArray;
					XFAILNOT(sp00, err = MemError();)
					XFAIL(err = sp00->init())
					XFAIL(err = NubADSPOptions(sp08, sp00))
					break;

				case 3:
					XFAIL(err = EzMNPSerialOptions(sp08, NULL))

					sp00 = new COptionArray;
					XFAILNOT(sp00, err = MemError();)
					XFAIL(err = sp00->init())
					XFAIL(err = EzMNPConnectOptions(sp08, NULL))
					break;
			}
			XFAIL(err)
		}
		else
		{
			EzConvertOptions(inConnectionKind, &sp08, &sp04, &sp00);
		}
#endif

		if (IsString(inMachineName))
		{
#if 0
			TCMAAppleTalkAddr	spm18;	// size +18
			if (sp00 == NULL)
			{
				sp00 = new COptionArray;
				XFAILNOT(sp00, err = MemError();)
				XFAIL(err = sp00->init())
			}
			XFAIL(err = NubADSPLookup(&spm18, inMachineName))
			XFAIL(err = sp00->insertOptionAt(sp00->f00, &spm18.f00))
#endif
		}
		else
		{
			isSerial = NOTNIL(inMachineName);
		}

		// initialize the nub
		err = gNTKNub->init(sp08, sp04, sp00, inputTranslator, outputTranslator, isSerial);
	}
	XENDTRY;

	XDOFAIL(err)
	{
		if (sp08 != NULL)
			delete sp08;
		if (sp04 != NULL)
			delete sp04;
		if (sp00 != NULL)
			delete sp00;
	}
	XENDFAIL;

	return err;
}


/* -----------------------------------------------------------------------------
	Open communications with NTK -- the Connect Inspector button in the Toolkit
	app was tapped.
	== plain C ntpTetheredListener
	Args:		rcvr
				inStart
				inConnectionKind
				inMachineName
				inInputTranslator
				inOutputTranslator
	Return:	error code
----------------------------------------------------------------------------- */

Ref
FSetUpTetheredListener(RefArg rcvr, RefArg inStart, RefArg inConnectionKind, RefArg inMachineName)
{
	return FNTKListener(rcvr, inStart, inConnectionKind, inMachineName, RA(NILREF), RA(NILREF));
}

Ref
FNTKListener(RefArg rcvr, RefArg inStart, RefArg inConnectionKind, RefArg inMachineName, RefArg inInputTranslator, RefArg inOutputTranslator)
{
	NewtonErr	err;

	if (ISTRUE(inStart))
	{
		// create the NTK nub (if it doesn’t already exist)
		if (gNTKNub == NULL)
		{
			XTRY
			{
				XFAIL(err = CreateNub(inConnectionKind, inMachineName, inInputTranslator, inOutputTranslator))
				err = gNTKNub->startListener();
			}
			XENDTRY;
			XDOFAIL(err)
			{
				if (gNTKNub)
					delete gNTKNub, gNTKNub = NULL;
			}
			XENDFAIL;
		}
		else
			err = -1;	// nub already exists
	}
	else
	{
		// shut down the NTK nub (if it exists)
		if (gNTKNub != NULL)
		{
			err = gNTKNub->stopListener();
			if (err != noErr
			&&  err != -36006)	// Operation not supported in the current tool state
				ErrorNotify(err, kNotifyAlert);
			delete gNTKNub, gNTKNub = NULL;
		}
		else
			err = -1;	// nub doesn’t exist
	}

	return MAKEINT(err);
}


/* -----------------------------------------------------------------------------
	Open communications with NTK -- the Download Package button in the Toolkit
	app was tapped.
	== plain C ntpDownloadPackage
	We are NOT expecting the nub to be already active -- this is a one-off download.
	Args:		rcvr
				inConnectionKind
				inMachineName
				inInputTranslator
				inOutputTranslator
	Return:	error code
----------------------------------------------------------------------------- */

Ref
FpkgDownload(RefArg rcvr, RefArg inConnectionKind, RefArg inMachineName)
{
	return FNTKDownload(rcvr, inConnectionKind, inMachineName, RA(NILREF), RA(NILREF));
}

Ref
FNTKDownload(RefArg rcvr, RefArg inConnectionKind, RefArg inMachineName, RefArg inInputTranslator, RefArg inOutputTranslator)
{
	NewtonErr	err;

	if (gNTKNub == NULL)
	{
		XTRY
		{
			XFAIL(err = CreateNub(inConnectionKind, inMachineName, inInputTranslator, inOutputTranslator))
			err = gNTKNub->downloadPackage();
		}
		XENDTRY;
	}
	else
		err = -1;	// nub already exists

	// wait for things to settle down
	Sleep(100*kMilliseconds);

	// shut down the nub
	if (gNTKNub != NULL)
		delete gNTKNub, gNTKNub = NULL;

	return MAKEINT(err);
}


/* -----------------------------------------------------------------------------
	Send an object to the NTK client.
	Args:		rcvr
				inRef
	Return:	error code
----------------------------------------------------------------------------- */

Ref
FNTKSend(RefArg rcvr, RefArg inRef)
{
	NewtonErr	err;
	if (gNTKNub != NULL)
		err = gNTKNub->sendRef(kTObject, inRef);
	else
		err = -1;
	return MAKEINT(err);
}


/* -----------------------------------------------------------------------------
	Determine whether the NTK nub is active.
	Args:		rcvr
	Return:	YES => it’s alive
----------------------------------------------------------------------------- */

Ref
FNTKAlive(RefArg rcvr)
{
	return MAKEBOOLEAN(gNTKNub != NULL);
}

#pragma mark -

/* -----------------------------------------------------------------------------
	P N T K I n T r a n s l a t o r
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Create the NTK input stream.
	Args:		--
	Return:	NTK input translator
----------------------------------------------------------------------------- */

NewtonErr
CreateNTKInTranslator(PInTranslator ** outTranslator, const char * inTranslatorName, CTaskSafeRingBuffer * inBuffer)
{
	NewtonErr	err = noErr;
	XTRY
	{
		XFAILNOT(outTranslator, err = -1;)
		*outTranslator = (PInTranslator *)MakeByName("PInTranslator", inTranslatorName);
		XFAILNOT(*outTranslator, err = MemError();)

		NTKTranslatorInfo	args = { inBuffer, 50*kMilliseconds, 30*kSeconds };
		XFAILIF(err = (*outTranslator)->init(&args), (*outTranslator)->destroy(); *outTranslator = NULL;)
	}
	XENDTRY;
	return err;
}

PROTOCOL_IMPL_SOURCE_MACRO(PNTKInTranslator)

PNTKInTranslator *
PNTKInTranslator::make(void)
{
	fBuffer = NULL;
	fPipe = NULL;
	fRetryInterval = 0;
	fTimeout = kNoTimeout;
	fIsFrameAvailable = NO;
	return this;
}

void
PNTKInTranslator::destroy(void)
{
	if (fPipe)
		delete fPipe;
}

NewtonErr
PNTKInTranslator::init(void * inArgs)
{
	NewtonErr	err = noErr;
	XTRY
	{
		XFAILNOT(inArgs, err = -1;)
		fRetryInterval = ((NTKTranslatorInfo*)inArgs)->x04;
		fTimeout = ((NTKTranslatorInfo*)inArgs)->x08;
		fBuffer = ((NTKTranslatorInfo*)inArgs)->x00;
		fPipe = new CTaskSafeRingPipe;
		XFAILNOT(fPipe, err = MemError();)
	//	SetPtrName(fPipe, 'ntkP');
		fPipe->init(fBuffer, NO, fRetryInterval, fTimeout);
	}
	XENDTRY;
	return err;
}

Timeout
PNTKInTranslator::idle(void)
{
	newton_try
	{
		if (!fIsFrameAvailable
		&&  fBuffer->dataCount() > 0)
		{
			NewtonErr	result = gNTKNub->doCommand();
			if (result < 0)
				NTKShutdown(result);
			else if (result > 0)
				fIsFrameAvailable = YES;
		}
	}
	newton_catch_all
	{
		NTKShutdown((NewtonErr)(unsigned long)CurrentException()->data);
	}
	end_try;

	return 1*kSeconds;
}

bool
PNTKInTranslator::frameAvailable(void)
{
	return fIsFrameAvailable;
}

Ref
PNTKInTranslator::produceFrame(int inLevel)
{
	fIsFrameAvailable = NO;
	NewtonErr	err;
	CObjectReader	reader(*fPipe);
	RefVar		frame;

	newton_try
	{
		frame = reader.read();
		if ((err = gNTKNub->sendResult(noErr)) != noErr)
			NTKShutdown(err);
	}
	cleanup
	{
		reader.~CObjectReader();
	}
	end_try;

	return frame;
}

void
PNTKInTranslator::readHeader(EventType * outCmd, size_t * outLength)
{
	fBuffer->getnCompletely((UByte*)outCmd, sizeof(EventType), fRetryInterval, fTimeout);
	fBuffer->getnCompletely((UByte*)outLength, sizeof(size_t), fRetryInterval, fTimeout);
}

void
PNTKInTranslator::readData(void * outBuf, size_t inSize)
{
	fBuffer->getnCompletely((UByte*)outBuf, inSize, fRetryInterval, fTimeout);
}

NewtonErr
PNTKInTranslator::loadPackage(void)
{
	return NewPackage(fPipe, NSCallGlobalFn(SYMA(GetDefaultStore)), RA(NILREF), 0);
}

void
PNTKInTranslator::setTimeout(Timeout inTimeout)
{
	fTimeout = inTimeout;
}


#pragma mark -

/*------------------------------------------------------------------------------
	P N T K O u t T r a n s l a t o r
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Create the NTK output stream.
	Args:		--
	Return:	NTK output translator
------------------------------------------------------------------------------*/

NewtonErr
CreateNTKOutTranslator(POutTranslator ** outTranslator, const char * inTranslatorName, CTaskSafeRingBuffer * inBuffer)
{
	NewtonErr	err;
	XTRY
	{
		XFAILNOT(outTranslator, err = -1;)
		*outTranslator = (POutTranslator *)MakeByName("POutTranslator", inTranslatorName);
		XFAILNOT(*outTranslator, err = MemError();)

		NTKTranslatorInfo	args = { inBuffer, 50*kMilliseconds, 30*kSeconds, 0xFF };
		XFAILIF(err = (*outTranslator)->init(&args), (*outTranslator)->destroy(); *outTranslator = NULL;)
	}
	XENDTRY;
	return err;
}

PROTOCOL_IMPL_SOURCE_MACRO(PNTKOutTranslator)

PNTKOutTranslator *
PNTKOutTranslator::make(void)
{
	fBuffer = NULL;
	fRetryInterval = 0;
	fTimeout = kNoTimeout;
	fBuf = NULL;
	fBufLen = 0;
	fPipe = NULL;
	fBufPtr = NULL;
	fBufRemaining = 0;
	return this;
}

void
PNTKOutTranslator::destroy(void)
{
	if (fPipe != NULL)
		delete fPipe;
	if (fBuf != NULL)
		FreePtr(fBuf);
}

NewtonErr
PNTKOutTranslator::init(void * inArgs)
{
	NewtonErr	err = noErr;
	XTRY
	{
		XFAILNOT(inArgs, err = -1;)
		fRetryInterval = ((NTKTranslatorInfo*)inArgs)->x04;
		fTimeout = ((NTKTranslatorInfo*)inArgs)->x08;
		fBuffer = ((NTKTranslatorInfo*)inArgs)->x00;
		fBufLen = ((NTKTranslatorInfo*)inArgs)->x0C;
		fBuf = NewPtr(fBufLen);
		XFAILNOT(fBuf, err = MemError();)
	//	SetPtrName(fBuf, 'repb');
		fBufPtr = fBuf;
		fBufRemaining = fBufLen;
		fPipe = new CTaskSafeRingPipe;
		XFAILNOT(fPipe, err = MemError();)
	//	SetPtrName(fPipe, 'ntkP');
		fPipe->init(fBuffer, NO, fRetryInterval, fTimeout);
	}
	XENDTRY;
	return err;
}

Timeout
PNTKOutTranslator::idle(void)
{
	return 1*kSeconds;
}

void
PNTKOutTranslator::consumeFrame(RefArg inObj, int inDepth, int indent)
{
	PrintObjectAux(inObj, indent, inDepth);
	flushText();
}

void
PNTKOutTranslator::prompt(int inLevel)
{ }

int
PNTKOutTranslator::vprint(const char * inFormat, va_list args)
{
	char		buf[256];
	int		n;
	n = vsprintf(buf, inFormat, args);
	// n > 0  => length printed
	// n < 0  => error code
	if (n < 256 && n <= fBufLen)
	{
		if (n > 0)
		{
			if (n >= fBufRemaining)
				flushText();
			memmove(fBufPtr, buf, n);
			fBufPtr += n;
			fBufRemaining -= n;
			char * p = buf;
			for (ArrayIndex i = 0; i < n; ++i)
			{
				if (*p++ == 0x0D)
				{
					flushText();
					break;
				}
			}
		}
	}
	else
		ThrowErr(exTranslator, kNSErrStringTooBig);

	return n;
}

int
PNTKOutTranslator::print(const char * inFormat, ...)
{
	int		n;
	va_list	args;
	va_start(args, inFormat);
	n = vprint(inFormat, args);
	va_end(args);
	return n;
}

int
PNTKOutTranslator::putc(int inCh)
{
	print("%c", inCh);
	return inCh;
}

void
PNTKOutTranslator::flush(void)
{
	flushText();
}

void
PNTKOutTranslator::enterBreakLoop(int inLevel)
{
	NewtonErr	err;
	if ((err = gNTKNub->enterBreakLoop(inLevel)) != noErr)
		NTKShutdown(err);
}

void
PNTKOutTranslator::exitBreakLoop(void)
{
	NewtonErr	err;
	if ((err = gNTKNub->exitBreakLoop()) != noErr)
		NTKShutdown(err);
}


void
PNTKOutTranslator::stackTrace(void * interpreter)
{
	NewtonErr	err;
	if ((err = NTKStackTrace(interpreter)) != noErr)
		NTKShutdown(err);
}

void
PNTKOutTranslator::exceptionNotify(Exception * inException)
{
	NewtonErr	err;
	if ((err = gNTKNub->exceptionNotify(inException)) != noErr)
		NTKShutdown(err);
}

void
PNTKOutTranslator::sendHeader(EventClass inId1, EventId inId2)
{
	fBuffer->putnCompletely((UByte*)&inId1, sizeof(inId1), fRetryInterval, fTimeout);
	fBuffer->putnCompletely((UByte*)&inId2, sizeof(inId2), fRetryInterval, fTimeout);
}

void
PNTKOutTranslator::sendCommand(EventType inCmd, size_t inLength)
{
	fBuffer->putnCompletely((UByte*)&inCmd, sizeof(inCmd), fRetryInterval, fTimeout);
	fBuffer->putnCompletely((UByte*)&inLength, sizeof(inLength), fRetryInterval, fTimeout);
}

void
PNTKOutTranslator::sendData(void * inData, size_t inLength)
{
	fBuffer->putnCompletely((UByte*)inData, inLength, fRetryInterval, fTimeout);
}

void
PNTKOutTranslator::consumeFrameReally(RefArg inRef)
{
	CObjectWriter	writer(inRef, *fPipe, NO);
	newton_try
	{
		size_t	objSize = writer.size();
		sendData(&objSize, sizeof(objSize));
		writer.write();
		flush();
	}
	cleanup
	{
		writer.~CObjectWriter();
	}
	end_try;
}

void
PNTKOutTranslator::consumeExceptionFrame(RefArg inRef, const char * inName)
{
	size_t	msgLen = strlen(inName) + 1;
	CObjectWriter	writer(inRef, *fPipe, 0);
	newton_try
	{
		size_t	objSize = writer.size();
		size_t	totalSize = msgLen + objSize;
		sendData(&totalSize, sizeof(totalSize));
		sendData(&msgLen, sizeof(msgLen));
		sendData((void *)inName, msgLen);
		sendData(&objSize, sizeof(objSize));
		writer.write();
		flush();
	}
	cleanup
	{
		writer.~CObjectWriter();
	}
	end_try;
}

void
PNTKOutTranslator::flushText(void)
{
	size_t	textLen = fBufLen - fBufRemaining;
	if (textLen > 0)
	{
		NewtonErr	err;
		if ((err = gNTKNub->sendTextHeader(textLen)) != noErr)
			NTKShutdown(err);
		fBuffer->putnCompletely((const UByte *)fBuf, textLen, fRetryInterval, fTimeout);
		fBufPtr = fBuf;
		fBufRemaining = fBufLen;
	}
}

void
PNTKOutTranslator::setTimeout(Timeout inTimeout)
{
	fTimeout = inTimeout;
}

#pragma mark -

/*------------------------------------------------------------------------------
	P S e r i a l I n T r a n s l a t o r
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Create the serial input stream.
	Args:		--
	Return:	serial input translator
------------------------------------------------------------------------------*/

NewtonErr
CreateSerialInTranslator(PInTranslator ** outTranslator, CTaskSafeRingBuffer * inBuf)
{
	NewtonErr	err = noErr;
	XTRY
	{
		XFAILNOT(outTranslator, err = -1;)
		*outTranslator = (PInTranslator *)MakeByName("PInTranslator", "PSerialInTranslator");
		XFAILNOT(*outTranslator, err = MemError();)

		SerialTranslatorInfo	args = { inBuf, 256 };
		XFAILIF(err = (*outTranslator)->init(&args), (*outTranslator)->destroy(); *outTranslator = NULL;)
	}
	XENDTRY;
	return err;
}

PROTOCOL_IMPL_SOURCE_MACRO(PSerialInTranslator)

PSerialInTranslator *
PSerialInTranslator::make(void)
{
	fRingBuf = NULL;
	fBuf = NULL;
	fBufSize = 0;
	return this;
}

void
PSerialInTranslator::destroy(void)
{
	if (fBuf != NULL)
		FreePtr(fBuf);
}

NewtonErr
PSerialInTranslator::init(void * inArgs)
{
	NewtonErr	err = noErr;
	XTRY
	{
		XFAILNOT(inArgs, err = -1;)
		fRingBuf = ((SerialTranslatorInfo*)inArgs)->ringBuf;
		fBufSize = ((SerialTranslatorInfo*)inArgs)->bufSize;
		fBuf = NewPtr(fBufSize);
		XFAILNOT(fBuf, err = MemError();)
	//	SetPtrName(fBuf, 'repb');
	}
	XENDTRY;
	return err;
}

Timeout
PSerialInTranslator::idle(void)
{
	return 1*kSeconds;
}

bool
PSerialInTranslator::frameAvailable(void)
{
	return fRingBuf->dataCount() > 0;
}

Ref
PSerialInTranslator::produceFrame(int inLevel)
{
	char *	p = fBuf;
	char *	limit = fBuf + fBufSize;
	while (p < limit)
	{
		int	ch = fRingBuf->getCompletely(250*kMilliseconds, kNoTimeout);
		if (ch == 0x08
		||  ch == 0x7F)		// backspace
		{
			if (p > fBuf)
				p--;
		}
		else if (ch == 0x0D
			  ||  ch == 0x0A)	// end of line
		{
			*p = 0;
			break;
		}
		else						// copy char
			*p++ = ch;
	}
	gREPout->putc('\n');
	return ParseString(MakeStringFromCString(fBuf));
}

#pragma mark -

/*------------------------------------------------------------------------------
	P S e r i a l O u t T r a n s l a t o r
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Create the serial output stream.
	Args:		--
	Return:	serial output translator
------------------------------------------------------------------------------*/

NewtonErr
CreateSerialOutTranslator(POutTranslator ** outTranslator, CTaskSafeRingBuffer * inBuf)
{
	NewtonErr	err = noErr;
	XTRY
	{
		XFAILNOT(outTranslator, err = -1;)
		*outTranslator = (POutTranslator *)MakeByName("POutTranslator", "PSerialOutTranslator");
		XFAILNOT(*outTranslator, err = MemError();)

		SerialTranslatorInfo	args = { inBuf, 256 };
		XFAILIF(err = (*outTranslator)->init(&args), (*outTranslator)->destroy(); *outTranslator = NULL;)
	}
	XENDTRY;
	return err;
}

PROTOCOL_IMPL_SOURCE_MACRO(PSerialOutTranslator)

PSerialOutTranslator *
PSerialOutTranslator::make(void)
{
	fRingBuf = NULL;
	fBuf = NULL;
	fBufSize = 0;
	return this;
}

void
PSerialOutTranslator::destroy(void)
{
	if (fBuf != NULL)
		FreePtr(fBuf);
}

NewtonErr
PSerialOutTranslator::init(void * inArgs)
{
	NewtonErr	err = noErr;
	XTRY
	{
		XFAILNOT(inArgs, err = -1;)
		fRingBuf = ((SerialTranslatorInfo*)inArgs)->ringBuf;
		fBufSize = ((SerialTranslatorInfo*)inArgs)->bufSize;
		fBuf = NewPtr(fBufSize);
		XFAILNOT(fBuf, err = MemError();)
	//	SetPtrName(fBuf, 'repb');
	}
	XENDTRY;
	return err;
}

Timeout
PSerialOutTranslator::idle(void)
{
	return 1*kSeconds;
}

void
PSerialOutTranslator::consumeFrame(RefArg inObj, int inDepth, int indent)
{
	PrintObjectAux(inObj, indent, inDepth);
}

void
PSerialOutTranslator::prompt(int inLevel)
{
	print("%7d > ", inLevel);
}

int
PSerialOutTranslator::vprint(const char * inFormat, va_list args)
{
	int		n;
	n = vsprintf(fBuf, inFormat, args);
	if (n > fBufSize)
		ThrowErr(exPipe, kNSErrStringTooBig);
	if (n > 0)
	{
		char *	p;
		int		ch;
		for (p = fBuf; (ch = *p) != 0; p++)
			putc(ch);
	}
	return n;
}

int
PSerialOutTranslator::print(const char * inFormat, ...)
{
	int		n;
	va_list	args;
	va_start(args, inFormat);
	n = vprint(inFormat, args);
	va_end(args);
	return n;
}

int
PSerialOutTranslator::putc(int inCh)
{
	fRingBuf->putCompletely(inCh, 250*kMilliseconds, kNoTimeout);
	if (inCh == 0x0D)
		fRingBuf->putCompletely(0x0A, 250*kMilliseconds, kNoTimeout);
	return inCh;
}

void
PSerialOutTranslator::flush(void)
{
	while (fRingBuf->dataCount() > 0)
		Sleep(250*kMilliseconds);
}

void
PSerialOutTranslator::enterBreakLoop(int inLevel)
{
	print("Entering break loop\n");
	prompt(inLevel);
	flush();
}

void
PSerialOutTranslator::exitBreakLoop(void)
{
	print("Exiting break loop\n");
}


void
PSerialOutTranslator::stackTrace(void * interpreter)
{
	REPStackTrace(interpreter);
}

void
PSerialOutTranslator::exceptionNotify(Exception * inException)
{
	REPExceptionNotify(inException);
}

#pragma mark -

/*------------------------------------------------------------------------------
	C K i l l E v e n t
------------------------------------------------------------------------------*/

CKillEvent::CKillEvent()
{
	fEventClass = 'ntk ';
	fEventId = 'kill';
}


/*--------------------------------------------------------------------------------
	C K i l l E v e n t H a n d l e r
--------------------------------------------------------------------------------*/

CKillEventHandler::CKillEventHandler(CNTKEndpointClient * inEndpoint)
	:	f14(inEndpoint)
{ }


NewtonErr
CKillEventHandler::init()
{
	return CEventHandler::init('kill', 'ntk ');
}


void
CKillEventHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
#if 0
	f14->makeYourPeace();
#endif
}

#pragma mark -

/*------------------------------------------------------------------------------
	C N T K T a s k
------------------------------------------------------------------------------*/

CNTKTask::CNTKTask()
{
	f70 = NULL;
	f74 = NULL;
	f78 = 0;
	f7C = 0;
	f80 = NULL;
	f84 = NULL;
	f88 = NULL;
}


CNTKTask::~CNTKTask()
{ }


size_t
CNTKTask::getSizeOf(void) const
{ return sizeof(CNTKTask); }


NewtonErr
CNTKTask::mainConstructor(void)
{
	enableForking(NO);
	return CAppWorld::mainConstructor();
}


void
CNTKTask::mainDestructor(void)
{
	CAppWorld::mainDestructor();
}


NewtonErr
CNTKTask::preMain(void)
{
	NewtonErr	err;
#if 0
	CNTKEndpointClient *	ep;
	CKillEventHandler *	evtHandler;;

	XTRY
	{
		ep = new CNTKEndpointClient;
		XFAILNOT(ep, err = MemError();)
		evtHandler = new CKillEventHandler(ep);
		XFAILNOT(evtHandler, err = MemError();)
		evtHandler->init();
		err = ep->init(f80, f84, f8C, f74, f70, f78, f7C);
	}
	XENDTRY;

	XDOFAIL(err)
	{
		if (evtHandler)
			delete evtHandler;
		if (ep)
			delete ep;
	}
	XENDFAIL;
#endif

	return err;
}


void
CNTKTask::postMain(void)
{ }


NewtonErr
CNTKTask::initNTK(COptionArray * inOpt1, COptionArray * inOpt2, COptionArray * inOpt3, CTaskSafeRingBuffer * inBuf1, CTaskSafeRingBuffer * inBuf2, size_t inBuf1Len, size_t inBuf2Len)
{
	f70 = inBuf1;
	f74 = inBuf2;
	f78 = inBuf1Len;
	f7C = inBuf2Len;
	f80 = inOpt1;
	f84 = inOpt2;
	f88 = inOpt3;
	return init('ntk ', YES, kSpawnedTaskStackSize);
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C N T K N u b
----------------------------------------------------------------------------- */

CNTKNub::CNTKNub()
	:	fNTKPort()
{
	fSavedREPIn = NULL;
	fSavedREPOut = NULL;
	fREPIn = NULL;
	fREPOut = NULL;
	fNTKIn = NULL;
	fNTKOut = NULL;
	fIsNTKTranslator = NO;
	fIsConnected = NO;
	fInputBuffer = NULL;
	fOutputBuffer = NULL;
}


CNTKNub::~CNTKNub()
{
	if (fSavedREPIn != NULL)
		gREPin = fSavedREPIn;
	if (fSavedREPOut != NULL)
		gREPout = fSavedREPOut;
	ResetREPIdler();
	if (fNTKPort != 0)	// actually checks id
	{
		CKillEvent	evt;
		fNTKPort.send(&evt, sizeof(evt));
		fNTKPort = 0;	// actually this does copyObject rather than what I think is intended
	}
	if (fREPIn)
		fREPIn->destroy();
	if (fREPOut)
		fREPOut->destroy();
	if (fInputBuffer)
		delete fInputBuffer;
	if (fOutputBuffer)
		delete fOutputBuffer;
}


/* -----------------------------------------------------------------------------
	Initialise the nub.
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::init(COptionArray * inOpt1, COptionArray * inOpt2, COptionArray * inOpt3, const char * inInTranslator, const char * inOutTranslator, bool inSerial)
{
	NewtonErr	err;
	XTRY
	{
		fInputBuffer = new CTaskSafeRingBuffer;
		XFAILNOT(fInputBuffer, err = MemError();)
		XFAIL(err = fInputBuffer->init(512, NO))

		fOutputBuffer = new CTaskSafeRingBuffer;
		XFAILNOT(fOutputBuffer, err = MemError();)
		XFAIL(err = fOutputBuffer->init(512, NO))

		{
			CNTKTask	theTask;
			XFAIL(err = theTask.initNTK(inOpt1, inOpt2, inOpt3, fInputBuffer, fOutputBuffer, 512, 512))
		}
		XFAIL(err = GetOSPortFromName('ntk ', &fNTKPort))

		if (inSerial)
		{
			fIsNTKTranslator = NO;
			XFAIL(err = CreateSerialInTranslator(&fREPIn, fInputBuffer))
			XFAIL(err = CreateSerialOutTranslator(&fREPOut, fOutputBuffer))
		}
		else
		{
			XFAIL(err = CreateNTKInTranslator(&fREPIn, inInTranslator ? inInTranslator : "PNTKInTranslator", fInputBuffer))
			XFAIL(err = CreateNTKOutTranslator(&fREPOut, inOutTranslator ? inOutTranslator : "PNTKOutTranslator", fOutputBuffer))
			fIsNTKTranslator = YES;
			fNTKIn = (PNTKInTranslator *)fREPIn;
			fNTKOut = (PNTKOutTranslator *)fREPOut;
		}
	}
	XENDTRY;
	return err;
}


/* -----------------------------------------------------------------------------
	Perform package download protocol for FNTKDownload().
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::downloadPackage(void)
{
	NewtonErr	err = noErr;

	newton_try
	{
		EventType	cmd;
		size_t	cmdLen;
		bool		done = NO;
		while (!done && err == noErr)
		{
			fNTKOut->sendHeader();
			fNTKOut->sendCommand(kTDownload, 0);
			fNTKOut->flush();
			if ((err = readCommand(&cmd, &cmdLen)) == noErr)
			{
				switch (cmd)
				{
				case kTLoadPackage:
					err = fNTKIn->loadPackage();
					done = YES;
					break;
				case kTDeletePackage:
					err = deletePackage(cmdLen);
					break;
				case kTSetTimeout:
					{
						Timeout	timeout;
						fNTKIn->readData(&timeout, sizeof(timeout));
						fNTKIn->setTimeout(timeout*kSeconds);
						fNTKOut->setTimeout(timeout*kSeconds);
					}
					break;
				default:
					err = kDockErrBadHeader;
					break;
				}
			}
		}
	}
	newton_catch_all
	{
		err = (NewtonErr)(unsigned long)CurrentException()->data;
	}
	end_try;

	sendResult(err);
	return err;
}


/* -----------------------------------------------------------------------------
	Start listening for connection from desktop client.
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::startListener(void)
{
	NewtonErr	err = noErr;

	// save current REP I/O hooks and set our own
	fSavedREPIn = gREPin;
	fSavedREPOut = gREPout;
	gREPin = fREPIn;
	gREPout = fREPOut;
	ResetREPIdler();

	if (fIsNTKTranslator)
	{
		newton_try
		{
			EventType	cmd;
			size_t	cmdLen;
			// send connect command
			fNTKOut->sendHeader();
			fNTKOut->sendCommand(kTConnect, 0);
			fNTKOut->flush();
			// wait for OK response
			if ((err = readCommand(&cmd, &cmdLen)) == noErr)
			{
				if (cmd == kTOK)
					fIsConnected = YES;
				else
					err = kDockErrBadHeader;
			}
		}
		newton_catch_all
		{
			err = (NewtonErr)(unsigned long)CurrentException()->data;
		}
		end_try;
	}

	return err;
}


/* -----------------------------------------------------------------------------
	Stop listening to desktop client.
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::stopListener(void)
{
	NewtonErr	err = noErr;

	if (fIsNTKTranslator && fIsConnected)
	{
		newton_try
		{
			// send terminate command
			fNTKOut->sendHeader();
			fNTKOut->sendCommand(kTTerminate, 0);
			fNTKOut->flush();
		}
		newton_catch_all
		{
			err = (NewtonErr)(unsigned long)CurrentException()->data;
		}
		end_try;
	}

	// NULL out state in newtoolspro (ntp) app
#if 0
	RefVar	ntp(GetFrameSlot(gRootView->fContext, MakeSymbol("newtoolspro")));
	if (NOTNIL(ntp))
		SetFrameSlot(ntp, MakeSymbol("ntpstate"), RA(NILREF));
#endif

	return err;
}


/* -----------------------------------------------------------------------------
	Read command header from desktop client.
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::readCommand(EventType * outCmd, size_t * outLength)
{
	NewtonErr	err = noErr;

	*outCmd = 0;
	*outLength = 0;
	newton_try
	{
		EventClass	hdr1;
		size_t		hdr2;	// or EventId
		fNTKIn->readHeader(&hdr1, &hdr2);
		if (hdr1 == kNewtEventClass && hdr2 == kToolkitEventId)
			fNTKIn->readHeader(outCmd, outLength);
		else
			err = kDockErrBadHeader;
	}
	newton_catch_all
	{
		err = (NewtonErr)(unsigned long)CurrentException()->data;
	}
	end_try;
	
	return err;
}


/* -----------------------------------------------------------------------------
	Perform command from desktop client.
	Performed during input translator idle().
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::doCommand(void)
{
	EventType	cmd;
	size_t		cmdLen;
	NewtonErr	err;
	if ((err = readCommand(&cmd, &cmdLen)) == noErr)
	{
		switch (cmd)
		{
		case kTCode:
			err = handleCodeBlock(cmdLen);
			sendResult(err);
			break;
		case kTExecute:
			err = 1;
			break;
		case kTLoadPackage:
			err = fNTKIn->loadPackage();
			sendResult(err);
			break;
		case kTDeletePackage:
			err = deletePackage(cmdLen);
			sendResult(err);
			break;
		case kTSetTimeout:
			{
				Timeout	timeout;
				fNTKIn->readData(&timeout, sizeof(timeout));
				fNTKIn->setTimeout(timeout*kSeconds);
				fNTKOut->setTimeout(timeout*kSeconds);
			}
			sendResult(noErr);
			break;
		case kTTerminate:
			err = -1;
			fIsConnected = NO;
			break;
		default:
			err = kDockErrBadHeader;
			sendResult(err);
			break;
		}
	}
	return err;
}


/* -----------------------------------------------------------------------------
	kTCode event handler.
	Interpret code sent (as NewtonScxript text) from desktop client.
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::handleCodeBlock(size_t inLength)
{
	NewtonErr	err = noErr;
	uint32_t		sp08;
	RefVar		codeBlock;
	RefVar		result;

	newton_try
	{
		// read...?
		fNTKIn->readData(&sp08, sizeof(sp08));
		// read NSOF codeblock frame from client
		codeBlock = fNTKIn->produceFrame(0);
		// interpret it
		result = InterpretBlock(codeBlock, gREPContext);
		// send back the resulting object
		fNTKOut->sendHeader();
		fNTKOut->sendCommand(kTCode, inLength);
		fNTKOut->consumeFrameReally(result);
	}
	newton_catch_all
	{
		err = (NewtonErr)(unsigned long)CurrentException()->data;
	}
	end_try;
	
	return err;
}


/* -----------------------------------------------------------------------------
	kTDeletePackage event handler.
	Delete the named package.
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::deletePackage(size_t inNameLen)
{
	NewtonErr	err = noErr;
	// allocate buffer for package name
	char * pkgName = NewPtr(inNameLen);
	// read package name
	fNTKIn->readData(pkgName, inNameLen);
	// perform linear search for package
#if 0
	CPMIterator	iter;
	for (iter.init(); iter.more(); iter.nextPackage())
	{
		if (Ustrcmp(iter.packageName(), pkgName) == 0)
		{
			// found it -- remove it
			NSCallGlobalFn(SYMA(RemovePackage), MAKEINT(iter.packageId()));
			break;
		}
	}
	iter.done();
#endif
// who FreePtr()s pkgName?
	return err;
}


/* -----------------------------------------------------------------------------
	kTText event producer.
	Prepare to send text.
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::sendTextHeader(size_t inLength)
{
	NewtonErr	err = noErr;

	newton_try
	{
		fNTKOut->sendHeader();
		fNTKOut->sendCommand(kTText, inLength);
	}
	newton_catch_all
	{
		err = (NewtonErr)(unsigned long)CurrentException()->data;
	}
	end_try;
	
	return err;
}


/* -----------------------------------------------------------------------------
	kTResult event producer.
	Prepare to send result.
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::sendResult(NewtonErr inResult)
{
	NewtonErr	err = noErr;

	newton_try
	{
		fNTKOut->sendHeader();
		fNTKOut->sendCommand(kTResult, sizeof(inResult));
		fNTKOut->sendData(&inResult, sizeof(inResult));
		fNTKOut->flush();
	}
	newton_catch_all
	{
		err = (NewtonErr)(unsigned long)CurrentException()->data;
	}
	end_try;
	
	return err;
}


/* -----------------------------------------------------------------------------
	kTEOM event producer.
	Is this actually used anywhere?.
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::sendEOM(void)
{
	NewtonErr	err = noErr;

	newton_try
	{
		fNTKOut->sendHeader();
		fNTKOut->sendCommand(kTEOM, 0);
		fNTKOut->flush();
	}
	newton_catch_all
	{
		err = (NewtonErr)(unsigned long)CurrentException()->data;
	}
	end_try;
	
	return err;
}


/* -----------------------------------------------------------------------------
	kTObject event producer.
	Called by FNTKSend with kTObject command, but could be used with other refs.
	Prepare to send ref.
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::sendRef(EventType inCmd, RefArg inRef)
{
	NewtonErr	err = noErr;

	newton_try
	{
		fNTKOut->sendHeader();
		fNTKOut->sendData(&inCmd, sizeof(inCmd));
		fNTKOut->consumeFrameReally(inRef);
	}
	newton_catch_all
	{
		err = (NewtonErr)(unsigned long)CurrentException()->data;
	}
	end_try;
	
	return err;
}


/* -----------------------------------------------------------------------------
	kTEnterBreakLoop event producer.
	Notify desktop client of break loop entry.
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::enterBreakLoop(int inLevel)
{
	NewtonErr	err = noErr;

	newton_try
	{
		fNTKOut->sendHeader();
		fNTKOut->sendCommand(kTEnterBreakLoop, 0);
		fNTKOut->flush();
	}
	newton_catch_all
	{
		err = (NewtonErr)(unsigned long)CurrentException()->data;
	}
	end_try;
	
	return err;
}


/* -----------------------------------------------------------------------------
	kTExitBreakLoop event producer.
	Notify desktop client of break loop exit.
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::exitBreakLoop(void)
{
	NewtonErr	err = noErr;

	newton_try
	{
		fNTKOut->sendHeader();
		fNTKOut->sendCommand(kTExitBreakLoop, 0);
		fNTKOut->flush();
	}
	newton_catch_all
	{
		err = (NewtonErr)(unsigned long)CurrentException()->data;
	}
	end_try;
	
	return err;
}


/* -----------------------------------------------------------------------------
	Notify desktop client of exception. Exceptions can be of type:
		message
		data
		error
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::exceptionNotify(Exception * inException)
{
	const char * name = inException->name;
	if (Subexception(inException->name, exMessage))
		return sendExceptionData(inException->name, (char *)inException->data);
	else if (Subexception(inException->name, exRefException))
		;//return sendExceptionData(inException->name, (RefVar) inException->data);
	return sendExceptionData(inException->name, (NewtonErr)(unsigned long)inException->data);
}


/* -----------------------------------------------------------------------------
	kTExceptionMessage event producer.
	data following command is:
		int	 total length of following data
		int	 length of exception name
		char[] exception name
		int	 length of message
		char[] message
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::sendExceptionData(const char * inExceptionName, char * inMessage)
{
	NewtonErr	err;

	if ((err = sendExceptionHeader(kTExceptionMessage)) == noErr)
	{
		newton_try
		{
			uint32_t	nameLen = strlen(inExceptionName) + 1;
			uint32_t	msgLen = strlen(inMessage) + 1;
			uint32_t	totalLen = nameLen + msgLen;
			fNTKOut->sendData(&totalLen, sizeof(totalLen));
			fNTKOut->sendData(&nameLen, sizeof(nameLen));
			fNTKOut->sendData((void *)inExceptionName, nameLen);
			fNTKOut->sendData(&msgLen, sizeof(msgLen));
			fNTKOut->sendData(inMessage, msgLen);
			fNTKOut->flush();
		}
		newton_catch_all
		{
			err = (NewtonErr)(unsigned long)CurrentException()->data;
		}
		end_try;
	}
	
	return err;
}


/* -----------------------------------------------------------------------------
	kTExceptionRef event producer.
	data following command is:
		int	 total length of following data
		int	 length of exception name
		char[] exception name
		int	 length of frame data
		char[] NSOF data
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::sendExceptionData(const char * inExceptionName, RefArg inData)
{
	NewtonErr	err;

	if ((err = sendExceptionHeader(kTExceptionRef)) == noErr)
	{
		newton_try
		{
			fNTKOut->consumeExceptionFrame(inData, inExceptionName);
		}
		newton_catch_all
		{
			err = (NewtonErr)(unsigned long)CurrentException()->data;
		}
		end_try;
	}
	
	return err;
}


/* -----------------------------------------------------------------------------
	kTExceptionError event producer.
	data following command is:
		int	 total length of following data
		int	 length of exception name
		char[] exception name
		int	 error code
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::sendExceptionData(const char * inExceptionName, NewtonErr inError)
{
	NewtonErr	err;

	if ((err = sendExceptionHeader(kTExceptionError)) == noErr)
	{
		newton_try
		{
			uint32_t	nameLen = strlen(inExceptionName) + 1;
			uint32_t	totalLen = nameLen + sizeof(inError);
			fNTKOut->sendData(&totalLen, sizeof(totalLen));
			fNTKOut->sendData(&nameLen, sizeof(nameLen));
			fNTKOut->sendData((void *)inExceptionName, nameLen);
			fNTKOut->sendData(&inError, sizeof(inError));
			fNTKOut->flush();
		}
		newton_catch_all
		{
			err = (NewtonErr)(unsigned long)CurrentException()->data;
		}
		end_try;
	}
	
	return err;
}


/* -----------------------------------------------------------------------------
	kTExceptionXXX event producer.
	Prepare to send exception data.
----------------------------------------------------------------------------- */

NewtonErr
CNTKNub::sendExceptionHeader(EventType inCmd)
{
	NewtonErr	err = noErr;

	newton_try
	{
		fNTKOut->sendHeader();
		fNTKOut->sendData(&inCmd, sizeof(inCmd));
	}
	newton_catch_all
	{
		err = (NewtonErr)(unsigned long)CurrentException()->data;
	}
	end_try;
	
	return err;
}
#endif
