/*
	File:		EndpointClient.cc

	Contains:	Communications code.

	Written by:	Newton Research Group, 2008.
*/

#include <limits.h>

#include "EndpointClient.h"
#include "Lookup.h"
#include "Funcs.h"
#include "UStringUtils.h"
#include "ROMSymbols.h"
#include "OSErrors.h"
#include "CommManagerInterface.h"
#include "CommErrors.h"

DeclareException(exComm, exRootException);
DeclareException(exTranslator, exRootException);

extern void		CheckForDeferredActions(void);

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
// for protoBasicEndpoint
Ref	CINewInstantiate(RefArg rcvr, RefArg inEndpoint, RefArg inOptions);
Ref	CINewInstantiateFromEndpoint(RefArg rcvr, RefArg inEndpoint, RefArg inOptions, RefArg inCEndpoint);
Ref	CINewDispose(RefArg rcvr);
Ref	CINewDisposeLeavingTool(RefArg rcvr);
Ref	CINewDisposeLeavingCEndpoint(RefArg rcvr);
Ref	CINewOption(RefArg rcvr, RefArg inOptions, RefArg inCallbackSpec);
Ref	CINewState(RefArg rcvr);
Ref	CINewSetState(RefArg rcvr, RefArg inState);
Ref	CINewBind(RefArg rcvr, RefArg inOptions, RefArg inCallbackSpec);
Ref	CINewConnect(RefArg rcvr, RefArg inOptions, RefArg inCallbackSpec);
Ref	CINewListen(RefArg rcvr, RefArg inOptions, RefArg inCallbackSpec);
Ref	CINewAccept(RefArg rcvr, RefArg inOptions, RefArg inCallbackSpec);
Ref	CINewDisconnect(RefArg rcvr, RefArg inCancelPending, RefArg inCallbackSpec);
Ref	CINewUnbind(RefArg rcvr, RefArg inCallbackSpec);
Ref	CINewAbort(RefArg rcvr, RefArg inCallbackSpec);
Ref	CINewOutput(RefArg rcvr, RefArg inData, RefArg inOptions, RefArg inOutputSpec);
Ref	CINewSetInputSpec(RefArg rcvr, RefArg inputSpec);
Ref	CINewInput(RefArg rcvr);
Ref	CINewPartial(RefArg rcvr);
Ref	CINewFlushPartial(RefArg rcvr);
Ref	CINewFlushInput(RefArg rcvr);
Ref	CINewRequestsPending(RefArg rcvr, RefArg inType);
// for protoStreamingEndpoint
Ref	CISNewInstantiate(RefArg rcvr, RefArg inEndpoint, RefArg inOptions);
Ref	CISNewInstantiateFromEndpoint(RefArg rcvr, RefArg inEndpoint, RefArg inOptions, RefArg inCEndpoint);
Ref	CISStreamIn(RefArg rcvr, RefArg inSpec);
Ref	CISStreamOut(RefArg rcvr, RefArg inData, RefArg inSpec);
};


/*------------------------------------------------------------------------------
	C E n d p o i n t C l i e n t
------------------------------------------------------------------------------*/

CEndpointClient::CEndpointClient()
{
	fCEndpoint = NULL;
}

CEndpointClient::~CEndpointClient()
{ }

NewtonErr
CEndpointClient::init(CEndpoint * inEndpoint, EventId inEventId, EventClass inEventClass)
{
	NewtonErr err = CEventHandler::init(inEventId, inEventClass);
	fCEndpoint = inEndpoint;
	inEndpoint->setClientHandler((ULong)this);
	return err;
}

bool
CEndpointClient::testEvent(CEvent * inEvent)
{
	return (ULong)this == ((CEndpointEvent*)inEvent)->f0C;
}

void
CEndpointClient::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	CEndpointEvent * epEvent = (CEndpointEvent *)inEvent;
	switch ((epEvent)->f10 + 12)
	{
	case 0:
		abortComplete(epEvent);
		break;
	case 1:
		unbindComplete(epEvent);
		break;
	case 2:
		bindComplete(epEvent);
		break;
	case 3:
		disconnectComplete(epEvent);
		break;
	case 4:
		releaseComplete(epEvent);
		break;
	case 5:
		connectComplete(epEvent);
		break;
	case 6:
		acceptComplete(epEvent);
		break;
	case 7:
		listenComplete(epEvent);
		break;
	case 8:
		optMgmtComplete(epEvent);
		break;
	case 9:
		getProtAddr(epEvent);
		break;
	case 10:
		sndComplete(epEvent);
		break;
	case 11:
		rcvComplete(epEvent);
		break;
	case 14:
		disconnect(epEvent);
		break;
	case 15:
		release(epEvent);
		break;
	default:
		dfault(epEvent);
	}
}

void
CEndpointClient::eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{ }

void
CEndpointClient::getProtAddr(CEndpointEvent * inEvent)
{ }

void
CEndpointClient::optMgmtComplete(CEndpointEvent * inEvent)
{ }

void
CEndpointClient::dfault(CEndpointEvent * inEvent)
{ }

void
CEndpointClient::defaultComplete(CEndpointEvent * inEvent)
{ }

void
CEndpointClient::bindComplete(CEndpointEvent * inEvent)
{ }

void
CEndpointClient::listenComplete(CEndpointEvent * inEvent)
{ }

void
CEndpointClient::acceptComplete(CEndpointEvent * inEvent)
{ }

void
CEndpointClient::connectComplete(CEndpointEvent * inEvent)
{ }

void
CEndpointClient::sndComplete(CEndpointEvent * inEvent)
{ }

void
CEndpointClient::rcvComplete(CEndpointEvent * inEvent)
{ }

void
CEndpointClient::abortComplete(CEndpointEvent * inEvent)
{ }

void
CEndpointClient::unbindComplete(CEndpointEvent * inEvent)
{ }

void
CEndpointClient::disconnect(CEndpointEvent * inEvent)
{
	dfault(inEvent);
}

void
CEndpointClient::disconnectComplete(CEndpointEvent * inEvent)
{ }

void
CEndpointClient::release(CEndpointEvent * inEvent)
{
	dfault(inEvent);
}

void
CEndpointClient::releaseComplete(CEndpointEvent * inEvent)
{ }

#pragma mark -

bool
UseModemNavigator(void)
{
	return true;	// original says YES
}

bool
ContainsModemService(COptionArray * inOptions)
{
	return false;	// original is a BIT more complicated
}


NewtonErr
RunModemNavigator(COptionArray * inOptions)
{
	return noErr;	// original is a LOT more complicated
}


bool
IsRaw(RefArg inObj)
{
	return IsBinary(inObj) && !IsInstance(inObj, SYMA(string));
}


bool
IsRawOrString(RefArg inObj)
{
	return IsBinary(inObj);
}


UByte
CharacterToUByte(UniChar inChar, CharEncoding incoding)
{
	UniChar str[1];
	UByte cstr[2];
	str[0] = inChar;
	ConvertFromUnicode(str, cstr, 1, incoding);
	return cstr[0];
}


FormType
GetDataForm(RefArg inFormSymbol, FormUser inUser)
{
	FormType form = kBadFormType;

	if (NOTNIL(inFormSymbol))
	{
		if (EQ(inFormSymbol, SYMA(raw))
		||  EQ(inFormSymbol, SYMA(binary)))
			form = kBinaryFormType;
		else if (EQ(inFormSymbol, SYMA(template)))
			form = kTemplateFormType;
		else if (EQ(inFormSymbol, SYMA(string)))
			form = kStringFormType;
		else if (EQ(inFormSymbol, SYMA(number)))
			form = kNumberFormType;
		else if (EQ(inFormSymbol, SYMA(export)))
			form = kExportFormType;
		else if (EQ(inFormSymbol, SYMA(frame)))
			form = kFrameFormType;
		else if (EQ(inFormSymbol, SYMA(bytes)))
			form = kBytesFormType;
		else if (EQ(inFormSymbol, SYMA(char)))
			form = kCharFormType;
	}
	else
	{
		if (inUser == 0)
			form = kTemplateFormType;
		else if (inUser == 1)
			form = kStringFormType;
		else
			form = kExportFormType;
	}

	if ((inUser == 0 && (form == kFrameFormType || form == kExportFormType))
	||  (inUser == 1 && form == kExportFormType))
		form = kBadFormType;

	return form;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C N e w S c r i p t E n d p o i n t C l i e n t
------------------------------------------------------------------------------*/

CNewScriptEndpointClient::CNewScriptEndpointClient()
{
	fNSEndpoint = NILREF;
	fInputSpec = NILREF;
	fInputBuffer = NULL;
	fPartialInterval = kNoTimeout;
	fTimeout = kNoTimeout;
	fEncoding = kDefaultEncoding;
	fCEndpoint = NULL;
	fRcvFlags = 0;
	f24 = NO;
	f70 = NO;
	fIsInputSpecActive = NO;
	fB1 = NO;
	fEndSequence = NILREF;
	fByteArray = NILREF;
	fProxyArray = NILREF;
	fSendQueue = MakeArray(0);
	fCommandQueue = MakeArray(0);
	fAbortQueue = MakeArray(0);
	fOptionDataOutXlator = NULL;
	fOptionDataInXlator = NULL;
	fDataOutXlator = NULL;
	fDataInXlator = NULL;
	fFrameDataOutXlator = NULL;
	fFrameDataInXlator = NULL;
}


CNewScriptEndpointClient::~CNewScriptEndpointClient()
{
	if (fCEndpoint)
		fCEndpoint->destroy();
	if (fInputBuffer)
		FreePtr(fInputBuffer);
	if (NOTNIL(fNSEndpoint))
		SetFrameSlot(fNSEndpoint, SYMA(ciprivate), RA(NILREF));
	if (fOptionDataOutXlator)
		fOptionDataOutXlator->destroy();
	if (fOptionDataInXlator)
		fOptionDataInXlator->destroy();
	if (fDataOutXlator)
		fDataOutXlator->destroy();
	if (fDataInXlator)
		fDataInXlator->destroy();
	if (fFrameDataOutXlator)
		fFrameDataOutXlator->destroy();
	if (fFrameDataInXlator)
		fFrameDataInXlator->destroy();
}


NewtonErr
CNewScriptEndpointClient::initScriptEndpointClient(RefArg inNSEndpoint, RefArg inOptions, CEndpoint * inCEndpoint)
{
	NewtonErr err = noErr;

	bool hasCEndpoint = (inCEndpoint != NULL);
	fNSEndpoint = inNSEndpoint;
	XTRY
	{
		XFAILNOT(NOTNIL(inNSEndpoint), err = kCommScriptNoEndpointAvailable;)

		RefVar internalBufferSize(GetVariable(inNSEndpoint, SYMA(internalBufferSize)));
		size_t bufSize = ISINT(internalBufferSize) ? RINT(internalBufferSize) : 512;
		XFAIL(err = f74.init(bufSize))
		if (!hasCEndpoint)
		{
			bool hasOptions = NOTNIL(inOptions);
			COptionArray optionArray;
			XFAIL(err = optionArray.init())
			XFAILNOT(hasOptions, err = kCMErrNoServiceSpecified;)
			XFAIL(err = convertToOptionArray(inOptions, &optionArray))
//			Smells like a hack
//			if (UseModemNavigator() && ContainsModemService(&optionArray))
//				RunModemNavigator(optionArray);
			XFAIL(err = CMGetEndpoint(&optionArray, &inCEndpoint, NO))
			if (hasOptions)
				XFAIL(err = convertFromOptionArray(inOptions, &optionArray))
		}
		XFAIL(err = init(inCEndpoint, 'newt', 'endp'))
		SetFrameSlot(inNSEndpoint, SYMA(ciprivate), AddressToRef(this));

		RefVar encoding(GetVariable(inNSEndpoint, SYMA(encoding)));
		if (NOTNIL(encoding))
			fEncoding = (CharEncoding)RINT(encoding);

		XFAIL(err = initIdler(0, kMilliseconds, 0, NO))
		err = fCEndpoint->open((ULong)this);
	}
	XENDTRY;

	return err;
}


void
CNewScriptEndpointClient::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
//	GrafPtr savePort = SetPort(gGrafPort);
	newton_try
	{
		CEndpointClient::eventHandlerProc(inToken, inSize, inEvent);
//		gRootView->update();
	}
	newton_catch(exRootException)
	{
		if (NOTNIL(fNSEndpoint) && NOTNIL(GetVariable(fNSEndpoint, SYMA(ExceptionHandler))))
		{
			RefVar error(AllocateFrame());
			SetFrameSlot(error, SYMA(name), MakeSymbol(CurrentException()->name));
			SetFrameSlot(error, SYMA(data), MAKEINT(CurrentException()->data;));
			RefVar args(MakeArray(1));
			SetArraySlot(args, 0, error);
			DoMessage(fNSEndpoint, SYMA(ExceptionHandler), args);
		}
		else
			ExceptionNotify(CurrentException());
	}
	end_try;
//	SetPort(savePort);
}


void
CNewScriptEndpointClient::idleProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	NewtonErr err = noErr;

	if (fIsDataAvailable && NOTNIL(fInputSpec))
	{
		if (NOTNIL(GetVariable(fInputSpec, SYMA(PartialScript))))
		{
			RefVar data(getPartialData(&err));
			if (NOTNIL(data) && err == noErr)
			{
				RefVar args(MakeArray(2));
				SetArraySlot(args, 0, fNSEndpoint);
				SetArraySlot(args, 1, data);
				newton_try
				{
					DoMessage(fInputSpec, SYMA(PartialScript), args);
				}
				newton_catch(exRootException)
				{
					if (NOTNIL(fNSEndpoint) && NOTNIL(GetVariable(fNSEndpoint, SYMA(ExceptionHandler))))
					{
						RefVar error(AllocateFrame());
						SetFrameSlot(error, SYMA(name), MakeSymbol(CurrentException()->name));
						SetFrameSlot(error, SYMA(data), MAKEINT(CurrentException()->data;));
						SetFrameSlot(error, SYMA(debug), SYMA(PartialScript));
						RefVar args(MakeArray(1));
						SetArraySlot(args, 0, error);
						DoMessage(fNSEndpoint, SYMA(ExceptionHandler), args);
						CheckForDeferredActions();
					}
					else
						ExceptionNotify(CurrentException());
				}
				end_try;
			}
		}
		fIsDataAvailable = NO;
		if (err)
		{
			clearInputSpec();
			doCompletion(err, fInputSpec, SYMA(ExceptionHandler));
		}
	}

	if (err == noErr)
	{
		if (fPartialInterval > 0 && NOTNIL(fInputSpec))
			resetIdle(fPartialInterval, kMilliseconds);
	}
}


void
CNewScriptEndpointClient::dfault(CEndpointEvent * inEvent)
{
	if (NOTNIL(fNSEndpoint) && NOTNIL(GetVariable(fNSEndpoint, SYMA(EventHandler))))
	{
		RefVar args(MakeArray(1));
		RefVar sp18(AllocateFrame());
		MAKE_ID_STR(((CDefaultEvent *)inEvent)->f20,sp10);
		RefVar sp0C(AllocateBinary(SYMA(string), 10));
		ConvertToUnicode(sp10, (UniChar *)BinaryData(sp0C));
		SetFrameSlot(sp18, SYMA(eventCode), MAKEINT(inEvent->f10));
		SetFrameSlot(sp18, SYMA(data), MAKEINT(inEvent->f14));
		SetFrameSlot(sp18, SYMA(serviceId), sp0C);
		SetFrameSlot(sp18, SYMA(time), MAKEINT(inEvent->f18.convertTo(kMilliseconds)));
		SetArraySlot(args, 0, sp18);
		newton_try
		{
			DoMessage(fNSEndpoint, SYMA(EventHandler), args);
		}
		newton_catch(exRootException)
		{
			if (NOTNIL(fNSEndpoint) && NOTNIL(GetVariable(fNSEndpoint, SYMA(ExceptionHandler))))
			{
				RefVar error(AllocateFrame());
				SetFrameSlot(error, SYMA(name), MakeSymbol(CurrentException()->name));
				SetFrameSlot(error, SYMA(data), MAKEINT(CurrentException()->data;));
				SetFrameSlot(error, SYMA(debug), SYMA(CompletionScript));
				RefVar args(MakeArray(1));
				SetArraySlot(args, 0, error);
				DoMessage(fNSEndpoint, SYMA(ExceptionHandler), args);
			}
			else
				ExceptionNotify(CurrentException());
		}
		end_try;
	}
}


void
CNewScriptEndpointClient::optMgmtComplete(CEndpointEvent * inEvent)
{
	COptMgmtCompleteEvent * evt = (COptMgmtCompleteEvent *)inEvent;
	optionCommandComplete(evt->f08, fCommandQueue, evt->f20);
}


void
CNewScriptEndpointClient::bindComplete(CEndpointEvent * inEvent)
{
	CBindCompleteEvent * evt = (CBindCompleteEvent *)inEvent;
	optionCommandComplete(evt->f08, fCommandQueue, evt->f20);
}


void
CNewScriptEndpointClient::listenComplete(CEndpointEvent * inEvent)
{
	CConnectCompleteEvent * evt = (CConnectCompleteEvent *)inEvent;
	if (evt->f20)
		delete evt->f20;
	optionCommandComplete(evt->f08, fCommandQueue, evt->f24);
}


void
CNewScriptEndpointClient::acceptComplete(CEndpointEvent * inEvent)
{
	CConnectCompleteEvent * evt = (CConnectCompleteEvent *)inEvent;
	if (evt->f20)
		delete evt->f20;
	optionCommandComplete(evt->f08, fCommandQueue, evt->f24);
}


void
CNewScriptEndpointClient::connectComplete(CEndpointEvent * inEvent)
{
	CConnectCompleteEvent * evt = (CConnectCompleteEvent *)inEvent;
	if (evt->f20)
		delete evt->f20;
	optionCommandComplete(evt->f08, fCommandQueue, evt->f24);
}


void
CNewScriptEndpointClient::sndComplete(CEndpointEvent * inEvent)
{
	RefVar options(GetArraySlot(fSendQueue, 0));
	ArrayRemoveCount(fSendQueue, 0, 1);	// should remove 3?

	CSndCompleteEvent * evt = (CSndCompleteEvent *)inEvent;
	if (NOTNIL(options))
		UnlockRef(options);
	else
	{
		if (evt->f24)
			delete evt->f24;	// sic
		else if (evt->f20)
			free(evt->f20);
	}
	optionCommandComplete(evt->f08, fSendQueue, evt->f2C);
}


void
CNewScriptEndpointClient::rcvComplete(CEndpointEvent * inEvent)
{
	NewtonErr err;
	CRcvCompleteEvent * evt = (CRcvCompleteEvent *)inEvent;
	fIsInputSpecActive = NO;
	if (fIsTargetActive)
	{
		if (NOTNIL(fTarget) && IsRawOrString(fTarget))
			UnlockRef(fTarget);
		else
			err = kCommScriptExpectedTarget;

		if (ISNIL(fInputSpec))
			err = kCommScriptNoActiveInputSpec;

		if (evt->f30 && evt->f08 == 0)
		{
			if (NOTNIL(fRcvOptions))	// original says != NULL
			{
				convertFromOptionArray(fRcvOptions, evt->f30);
				f60 = 1;
			}
			delete evt->f30;
		}

		if (err == noErr)
		{
			fB1 = YES;
			if (fIsTargetActive)
				err = rawRcvComplete(evt);
			else
				err = filterRcvComplete(evt->f2C);
			fB1 = NO;
		}

		if (err == noErr)
			err = postReceive();

		if (err)
		{
			RefVar inputSpec(fInputSpec);
			clearInputSpec();
			doCompletion(err, inputSpec, RA(NILREF));	// original says NULL
		}
	}
}


void
CNewScriptEndpointClient::releaseComplete(CEndpointEvent * inEvent)
{
	commandComplete(inEvent->f08, fCommandQueue, RA(NILREF));
}


void
CNewScriptEndpointClient::disconnectComplete(CEndpointEvent * inEvent)
{
	commandComplete(inEvent->f08, fCommandQueue, RA(NILREF));
}


void
CNewScriptEndpointClient::unbindComplete(CEndpointEvent * inEvent)
{
	commandComplete(inEvent->f08, fCommandQueue, RA(NILREF));
}


void
CNewScriptEndpointClient::abortComplete(CEndpointEvent * inEvent)
{
	commandComplete(inEvent->f08, fAbortQueue, RA(NILREF));
}


void
CNewScriptEndpointClient::optionCommandComplete(NewtonErr inErr, RefArg inQueue, COptionArray * inOptionArray)
{
	RefVar options(GetArraySlot(inQueue, 0));
	ArrayRemoveCount(inQueue, 0, 1);
	if (inErr == noErr)
		inErr = convertFromOptionArray(options, inOptionArray);
	if (inOptionArray)
		delete inOptionArray;
	commandComplete(inErr, inQueue, options);
}


void
CNewScriptEndpointClient::commandComplete(NewtonErr inErr, RefArg inQueue, RefArg inOptions)
{
	RefVar spec(GetArraySlot(inQueue, 0));
	ArrayRemoveCount(inQueue, 0, 1);
	doCompletion(inErr, spec, inOptions);
}


void
CNewScriptEndpointClient::doCompletion(NewtonErr inErr, RefArg inputSpec, RefArg inOptions)
{
	if (NOTNIL(inputSpec) && NOTNIL(GetVariable(inputSpec, SYMA(CompletionScript))))
	{
		RefVar args(MakeArray(3));
		SetArraySlot(args, 0, fNSEndpoint);
		SetArraySlot(args, 1, inOptions);
		SetArraySlot(args, 2, (inErr == noErr) ? NILREF : MAKEINT(inErr));
		newton_try
		{
			DoMessage(inputSpec, SYMA(CompletionScript), args);
		}
		newton_catch(exRootException)
		{
			if (NOTNIL(fNSEndpoint) && NOTNIL(GetVariable(fNSEndpoint, SYMA(ExceptionHandler))))
			{
				RefVar error(AllocateFrame());
				SetFrameSlot(error, SYMA(name), MakeSymbol(CurrentException()->name));
				SetFrameSlot(error, SYMA(data), MAKEINT(CurrentException()->data;));
				SetFrameSlot(error, SYMA(debug), SYMA(CompletionScript));
				RefVar args(MakeArray(1));
				SetArraySlot(args, 0, error);
				DoMessage(fNSEndpoint, SYMA(ExceptionHandler), args);
			}
			else
				ExceptionNotify(CurrentException());
		}
		end_try;
	}
	else if (inErr)
	{
		if (NOTNIL(fNSEndpoint) && NOTNIL(GetVariable(fNSEndpoint, SYMA(ExceptionHandler))))
		{
			RefVar error(AllocateFrame());
			SetFrameSlot(error, SYMA(name), MakeSymbol(exComm));
			SetFrameSlot(error, SYMA(data), MAKEINT(inErr));
			SetFrameSlot(error, SYMA(debug), SYMA(CompletionScript));
			RefVar args(MakeArray(1));
			SetArraySlot(args, 0, error);
			DoMessage(fNSEndpoint, SYMA(ExceptionHandler), args);
		}
		else
			ThrowErr(exComm, inErr);
	}
}


Ref
CNewScriptEndpointClient::parseInput(FormType inType, long inArg2, long inArg3, UChar * inBuf, RefArg inArg5, NewtonErr * outErr)
{
	RefVar obj;
	*outErr = noErr;
	if (inType == kFrameFormType)
	{
		if (fFrameDataInXlator == NULL)
		{
			fFrameDataInXlator = (PUnflattenPtr *)MakeByName("PFrameSource", "PUnflattenPtr");
			if (fFrameDataInXlator == NULL)
			{
				*outErr = MemError();
				if (*outErr != noErr)
					return NILREF;
			}
		}
		newton_try
		{
			UnflattenPtrParms xParms;
			xParms.x00 = inBuf;
			xParms.x04 = inArg3;
			xParms.x08 = NILREF;
			obj = fFrameDataInXlator->translate(&xParms, NULL);
		}
		newton_catch(exTranslator)
		{
			*outErr = (NewtonErr)CurrentException()->data;
		}
		end_try;
	}
	else
	{
		newton_try
		{
			ScriptDataInParms xParms;
			xParms.x00 = inBuf;
			xParms.x04 = inArg3;
			xParms.x08 = inType;
			xParms.x0C = inArg2;
			xParms.x10 = inArg5;
			obj = getScriptDataInXlator()->translate(&xParms, NULL);
		}
		newton_catch(exTranslator)
		{
			*outErr = (NewtonErr)(unsigned long)CurrentException()data;
		}
		end_try;
	}
	return obj;
}


NewtonErr
CNewScriptEndpointClient::doInputSpec(RefArg inputSpec)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILIF(fIsInputSpecActive, err = kCommScriptInputSpecAlreadyActive;)
		clearInputSpec();
		if (ISNIL(inputSpec))
		{
			if (fInputBuffer)
			{
				FreePtr(fInputBuffer), fInputBuffer = NULL;
			}
			fInputBufLen = 0;
			stopIdle();
		}
		else
		{
			XFAIL(err = readInputSlots())
			XFAIL(err = initInputBuffers())
			err = postReceive();
			if (fPartialInterval > 0 && err == noErr)
				resetIdle(fPartialInterval, kMilliseconds);
			else
				stopIdle();
		}
	}
	XENDTRY;

	XDOFAIL(err != noErr && err != kCommScriptInputSpecAlreadyActive)
	{
		clearInputSpec();
		stopIdle();
	}
	XENDFAIL;

	return err;
}


NewtonErr
CNewScriptEndpointClient::postReceive(void)
{
	NewtonErr err = noErr;
	COptionArray * optionArray = NULL;
	Timeout timeout = kNoTimeout;
	if (!fIsInputSpecActive && !fB1)
	{
		ULong flags = fRcvFlags;
		
		XTRY
		{
			if (f60 == 0 && NOTNIL(fRcvOptions))
				XFAIL(err = prepOptions(fRcvOptions, &optionArray))
			if (fIsTargetActive)
			{
				XFAILIF(ISNIL(fTarget) || !IsRawOrString(fTarget), err = kCommScriptExpectedTarget;)
				LockRef(fTarget);
				fIsInputSpecActive = YES;
				UChar * dataPtr = (UChar *)BinaryData(fTarget) + fTargetOffset;
				ArrayIndex dataSize = fByteCount;
				err = fCEndpoint->nRcv(dataPtr, &dataSize, fByteCount, &flags, fTimeout, NO, optionArray);
			}
			else
			{
				f74.reset();
				ArrayIndex threshold = 1;
				if (f70)
					f70 = fByteCount > 0 && ISNIL(fEndSequence) && fPartialInterval == kNoTimeout;
				if (f70)
				{
					ArrayIndex r0 = fByteCount - fInputBufWrIndex;
					if (r0 > 512)
						r0 = 512;
					threshold = r0;
				}
				err = fCEndpoint->nRcv(&f74, threshold, &flags, fTimeout, NO, optionArray);
			}
		}
		XENDTRY;
		XDOFAIL(err)
		{
			fIsInputSpecActive = NO;
			if (optionArray)
				delete optionArray;
		}
		XENDFAIL;
	}
	return err;
}


void
CNewScriptEndpointClient::clearInputSpec(void)
{
	fInputSpec = NILREF;
	fEndSequence = NILREF;
	fByteArray = NILREF;
	fProxyArray = NILREF;
	fByteCount = 0;
	fPartialInterval = kNoTimeout;
	fSevenBit = NO;
	fRcvOptions = NILREF;
	f60 = 2;
	fForm = kStringFormType;
}


// forward declaration
Ref CloneOptions(RefArg inOptions);

NewtonErr
CNewScriptEndpointClient::readInputSlots(void)
{
	NewtonErr err = noErr;
	RefVar item;

	fForm = GetDataForm(GetVariable(fInputSpec, SYMA(form)), 1);
	if (fForm == kBadFormType)
		err = kCommScriptBadForm;
	else
	{
		item = GetVariable(fInputSpec, SYMA(target));
		if (NOTNIL(item) && IsFrame(item))
			err = readTarget(item);
		else
		{
			fTarget = NILREF;
			if (fForm == kTemplateFormType)
				err = kCommScriptExpectedTemplate;
			else if (fForm == kBinaryFormType)
				err = kCommScriptExpectedTarget;
		}
	}

	item = GetVariable(fInputSpec, SYMA(rcvFlags));
	if (ISINT(item))
	{
		fRcvFlags = RINT(item);
		f24 = (fRcvFlags & 0x02) != 0;
	}
	else
		fRcvFlags = 0;

	if (err == noErr)
	{
		item = GetVariable(fInputSpec, SYMA(termination));
		if (NOTNIL(item) && IsFrame(item))
			err = readTermination(item);
		else
		{
			if (fForm == kStringFormType || fForm == kBytesFormType)
			{
				item = GetVariable(fInputSpec, SYMA(discardAfter));
				fDiscardAfter = ISINT(item) ? RINT(item) : 1*KByte;
			}
			else
				fDiscardAfter = INT_MAX;
			fByteCount = 0;
			fEndSequence = NILREF;
		}
	}

	f70 = NOTNIL(GetVariable(fInputSpec, SYMA(optimize)));

	if (err == noErr)
	{
		item = GetVariable(fInputSpec, SYMA(filter));
		if (NOTNIL(item) && IsFrame(item))
			err = readFilter(item);
		else
		{
			fByteArray = NILREF;
			fProxyArray = NILREF;
			fSevenBit = NO;
		}
	}

	if (err == noErr)
	{
		item = GetVariable(fInputSpec, SYMA(rcvOptions));
		if (NOTNIL(item))
		{
			fRcvOptions = CloneOptions(item);
			f60 = NO;
		}
		else
			fRcvOptions = NILREF;
	}

	item = GetVariable(fInputSpec, SYMA(partialFrequency));
	if (ISINT(item))
	{
		if (fForm == kStringFormType || fForm == kBytesFormType)
			fPartialInterval = RINT(item);
		else
			err = kCommScriptInappropriatePartial;
	}
	else
		fPartialInterval = kNoTimeout;

	item = GetVariable(fInputSpec, SYMA(reqTimeout));
	if (ISINT(item))
		fTimeout = RINT(item) * kMilliseconds;
	else
		fTimeout = kNoTimeout;

	return err;
}


NewtonErr
CNewScriptEndpointClient::readTermination(RefArg inSpec)
{
	NewtonErr err = noErr;
	RefVar item;
	bool exists;

	XTRY
	{
		XFAILNOT(fForm == kStringFormType || fForm == kBytesFormType || fForm == kBinaryFormType, err = kCommScriptInappropriateTermination;)

		item = GetVariable(inSpec, SYMA(useEOP), &exists);
		if (exists)
		{
			if (fRcvFlags & 0x02)
			{
				if (ISNIL(item))
					f24 = 0;
			}
			else
			{
				if (NOTNIL(item))
					XFAIL(err = kCommScriptInappropriateTermination)
			}
		}

		item = GetVariable(inSpec, SYMA(byteCount));
		if (ISINT(item))
		{
			fByteCount = RINT(item);
			fDiscardAfter = INT_MAX;
		}
		else
		{
			fByteCount = 0;
			item = GetVariable(fInputSpec, SYMA(discardAfter));
			fDiscardAfter = ISINT(item) ? RINT(item) : 1*KByte;
		}

		item = GetVariable(inSpec, SYMA(endSequence));
		fEndSequence = NILREF;
		if (NOTNIL(item))
		{
			XFAILIF(fForm == kBinaryFormType, err = kCommScriptInappropriateTermination;)
			fEndSequence = MakeArray(0);
			if (IsArray(item))
			{
				for (ArrayIndex i = 0, count = Length(item); i < count && err == noErr; ++i)
					err = addEndArrayElement(GetArraySlot(item, i));
			}
			else
				err = addEndArrayElement(item);
			if (Length(fEndSequence) == 0 || err != noErr)
				fEndSequence = NILREF;
		}
	}
	XENDTRY;

	return err;
}


NewtonErr
CNewScriptEndpointClient::readTarget(RefArg inSpec)
{
	NewtonErr err = noErr;
	RefVar item;

	XTRY
	{
		if (fForm == kBinaryFormType)
		{
			item = GetVariable(inSpec, SYMA(data));
			XFAILNOT(NOTNIL(item) && IsRawOrString(item), err = kCommScriptExpectedTarget;)
			fTarget = item;

			item = GetVariable(inSpec, SYMA(offset));
			f60 = ISINT(item) ? RINT(item) : 0;
		}
		else if (fForm == kTemplateFormType)
		{
			item = GetVariable(inSpec, SYMA(typeList));
			XFAILNOT((NOTNIL(item) && IsArray(item)), err = kCommScriptExpectedTemplate;)
			fTarget = item;
		}
		else
		{
			fTarget = NILREF;
			err = kCommScriptInappropriateTarget;
		}
	}
	XENDTRY;

	return err;
}


NewtonErr
CNewScriptEndpointClient::readFilter(RefArg inSpec)
{
	NewtonErr err = noErr;
	RefVar item;

	XTRY
	{
		XFAILIF(fForm == kBinaryFormType, err = kCommScriptInappropriateFilter;)
		fByteArray = NILREF;
		fProxyArray = NILREF;
		item = GetVariable(inSpec, SYMA(byteProxy));
		if (NOTNIL(item))
		{
			fByteArray = MakeArray(0);
			fProxyArray = MakeArray(0);
			if (IsArray(item))
			{
				for (ArrayIndex i = 0, count = Length(item); i < count && err == noErr; ++i)
					err = addProxyFrame(GetArraySlot(item, i));
			}
			else
				err = addProxyFrame(item);
			if (Length(fByteArray) == 0 || err != noErr)
			{
				fByteArray = NILREF;
				fProxyArray = NILREF;
			}
		}

		item = GetVariable(inSpec, SYMA(sevenBit));
		fSevenBit = NOTNIL(item);
	}
	XENDTRY;

	return err;
}


NewtonErr
CNewScriptEndpointClient::addProxyFrame(RefArg inSpec)
{
	NewtonErr err;
	RefVar byte(GetVariable(inSpec, SYMA(byte)));
	RefVar proxy(GetVariable(inSpec, SYMA(proxy)));
	XTRY
	{
		XFAIL(err = addProxyArrayElement(fByteArray, byte))
		XFAIL(err = addProxyArrayElement(fProxyArray, proxy))
	}
	XENDTRY;
	return err;
}


NewtonErr
CNewScriptEndpointClient::addProxyArrayElement(RefArg inArray, RefArg inElement)
{
	NewtonErr err = noErr;

	newton_try
	{
		if (ISCHAR(inElement))
		{
			UniChar ch = RCHAR(inElement);
			if (Umbstrnlen(&ch, fEncoding, 1) == 1)
				AddArraySlot(inArray, CharacterToUByte(ch, fEncoding));
			else
				err = kCommScriptCharNotSingleByte;
		}

		else if (ISINT(inElement))
			AddArraySlot(inArray, inElement);

		else if (ISNIL(inElement))
			AddArraySlot(inArray, RA(NILREF));

		else
			err = kCommScriptInvalidProxy;
	}
	newton_catch(exOutOfMemory)
	{
		err = (NewtonErr)(unsigned long)CurrentException()data;
	}
	end_try;

	return err;
}


NewtonErr
CNewScriptEndpointClient::checkForInput(long inArg1, bool inArg2)
{
	NewtonErr err = noErr;
	int index;

	if (fByteCount > 0 && !inArg2)
	{
		if (fForm == kFrameFormType && f44)
			err = getFrameLength();
		else
			return postInput(inArg1, 1);
	}

	else if (NOTNIL(fEndSequence) && (index = checkEndArray(fInputBuffer + inArg1 - 1)) > 0)
	{
		return postInput(inArg1, index+2);
	}

	else if (inArg2 && f24 && f74.peek() == -1)
	{
		return postInput(inArg1, 2);
	}

	if (fInputBufLen == inArg1)
	{
		memmove(fInputBuffer, fInputBuffer + 0x40, fInputBufLen - 0x40);
		fInputBufWrIndex -= 0x40;
		if (fInputBufRdIndex > 0)
		{
			long newInputBufRdIndex = fInputBufRdIndex - 0x40;
			fInputBufRdIndex = (newInputBufRdIndex < 0) ? 0 : newInputBufRdIndex;
		}
	}

	return err;
}


NewtonErr
CNewScriptEndpointClient::initInputBuffers(void)
{
	NewtonErr err = noErr;
	fIsTargetActive = NO;

	if (fForm == kBinaryFormType)
	{
		long sizeAvailable = Length(fTarget) - fTargetOffset;
		if (fByteCount == 0 || fByteCount > sizeAvailable)
			fByteCount = sizeAvailable;
		fIsTargetActive = YES;
	}

	else if (fForm == kFrameFormType)
	{
		f44 = YES;
		fByteCount = 4;
		if (fInputBuffer == NULL)
		{
			fInputBuffer = (UChar *)NewPtr(4);
			if (MemError())
				err = kOSErrNoMemory;
			fInputBufLen = 4;
		}
	}

	return err;
}


NewtonErr
CNewScriptEndpointClient::getFrameLength(void)
{
	NewtonErr err = noErr;

	size_t r5 = *(size_t*)fInputBuffer;
	fInputBufWrIndex -= 4;
	if (r5 > 0)
	{
		memmove(fInputBuffer, fInputBuffer + 4, fInputBufWrIndex);
		f44 = NO;
		if (fInputBufLen < r5)
		{
			UChar * r7 = (UChar *)realloc(fInputBuffer, r5);
			if (MemError())
				err = kOSErrNoMemory;
			fInputBufLen = r5;
			fInputBuffer = r7;
		}
		fByteCount = r5;
	}
	else
		f44 = YES;

	return err;
}


NewtonErr
CNewScriptEndpointClient::rawRcvComplete(CRcvCompleteEvent * inEvent)
{
	NewtonErr err = noErr;
	int terminationCondition = (f24 && (inEvent->f2C & 0x01) != 0) ? 2 : 0;

	if (NOTNIL(GetVariable(fInputSpec, SYMA(InputScript))))
	{
		RefVar terminator(AllocateFrame());
		SetFrameSlot(terminator, SYMA(condition), terminationCondition == 2 ? SYMA(useEOP) : SYMA(byteCount));
		SetFrameSlot(terminator, SYMA(byteCount), MAKEINT(inEvent->f28));
		RefVar options;
		if (f60 == 1)
		{
			options = fRcvOptions;
			fRcvOptions = NILREF;
			f60 = 2;
		}
		RefVar args(MakeArray(4));
		SetArraySlot(args, 0, fNSEndpoint);
		SetArraySlot(args, 1, fTarget);
		SetArraySlot(args, 2, terminator);
		SetArraySlot(args, 3, options);
		newton_try
		{
			DoMessage(fInputSpec, SYMA(InputScript), args);
		}
		newton_catch(exRootException)
		{
			if (NOTNIL(fNSEndpoint) && NOTNIL(GetVariable(fNSEndpoint, SYMA(ExceptionHandler))))
			{
				RefVar error(AllocateFrame());
				SetFrameSlot(error, SYMA(name), MakeSymbol(CurrentException()->name));
				SetFrameSlot(error, SYMA(data), MAKEINT(CurrentException()->data;));
				SetFrameSlot(error, SYMA(debug), SYMA(InputScript));
				RefVar args(MakeArray(1));
				SetArraySlot(args, 0, error);
				DoMessage(fNSEndpoint, SYMA(ExceptionHandler), args);
			}
			else
				ExceptionNotify(CurrentException());
		}
		end_try;
	}

	return err;
}


NewtonErr
CNewScriptEndpointClient::filterRcvComplete(ULong inData)
{
	NewtonErr err = noErr;
	UChar proxyChar;
	bool isProxyChar;

	ArrayIndex count = NOTNIL(fByteArray) ? Length(fByteArray) : 0;
	ULong sp08 = inData & 0x01;
	while (fInputBuffer != NULL && err == noErr && f74.get() != -1)
	{
		isProxyChar = YES;
		for (ArrayIndex i = 0; i < count; ++i)
		{
			UChar byte = RINT(GetArraySlot(fByteArray, i));
			if (byte == inData)
			{
				Ref proxy = GetArraySlot(fProxyArray, i);
				if (ISNIL(proxy))
					isProxyChar = NO;
				else
					proxyChar = RINT(proxy);
				break;
			}
		}
		if (isProxyChar)
		{
			if (fSevenBit)
				proxyChar &= 0x7F;
			fIsDataAvailable = YES;
			fInputBuffer[fInputBufWrIndex++] = proxyChar;
			err = checkForInput(fInputBufWrIndex, !sp08);
		}
	}

	return err;
}


NewtonErr
CNewScriptEndpointClient::doInput(void)
{
	if (ISNIL(fInputSpec))
		return kCommScriptNoActiveInputSpec;
	return postInput(fInputBufWrIndex, 0);
}


NewtonErr
CNewScriptEndpointClient::postInput(long inArg1, int inTerminationCondition)
{
	NewtonErr err = noErr;
	bool hasInputScript = NOTNIL(GetVariable(fInputSpec, SYMA(InputScript)));
	RefVar data;	// r7
	long r8 = 0;
	if (inArg1 > 0)
	{
		r8 = (inArg1 < fDiscardAfter) ? inArg1 : fDiscardAfter;
		if (hasInputScript)
			data = parseInput(fForm, fEncoding, r8, fInputBuffer + inArg1 - r8, fTarget, &err);
		if (fInputBufWrIndex > inArg1)
			memmove(fInputBuffer, fInputBuffer + inArg1, fInputBufWrIndex - inArg1);
		fInputBufWrIndex -= inArg1;
		if (fForm == kFrameFormType)
		{
			f44 = YES;
			fByteCount = 4;
		}
	}
	fInputBufRdIndex = 0;
	fIsDataAvailable = NO;
	if (NOTNIL(data) && hasInputScript && err == noErr)
	{
		RefVar terminator;
		if (inTerminationCondition != 0)
		{
			terminator = AllocateFrame();
			SetFrameSlot(terminator, SYMA(byteCount), MAKEINT(r8));
			if (inTerminationCondition == 1)
				SetFrameSlot(terminator, SYMA(condition), SYMA(byteCount));
			else if (inTerminationCondition == 2)
				SetFrameSlot(terminator, SYMA(condition), SYMA(useEOP));
			else
			{
				SetFrameSlot(terminator, SYMA(condition), SYMA(endSequence));
				SetFrameSlot(terminator, SYMA(index), MAKEINT(inTerminationCondition-3));
			}
		}
		RefVar options;
		if (f60 == 1)
		{
			options = fRcvOptions;
			fRcvOptions = NILREF;
			f60 = 2;
		}
		RefVar args(MakeArray(4));
		SetArraySlot(args, 0, fNSEndpoint);
		SetArraySlot(args, 1, data);
		SetArraySlot(args, 2, terminator);
		SetArraySlot(args, 3, options);
		newton_try
		{
			DoMessage(fInputSpec, SYMA(InputScript), args);
		}
		newton_catch(exRootException)
		{
			if (NOTNIL(fNSEndpoint) && NOTNIL(GetVariable(fNSEndpoint, SYMA(ExceptionHandler))))
			{
				RefVar error(AllocateFrame());
				SetFrameSlot(error, SYMA(name), MakeSymbol(CurrentException()->name));
				SetFrameSlot(error, SYMA(data), MAKEINT(CurrentException()->data;));
				SetFrameSlot(error, SYMA(debug), SYMA(InputScript));
				RefVar args(MakeArray(1));
				SetArraySlot(args, 0, error);
				DoMessage(fNSEndpoint, SYMA(ExceptionHandler), args);
			}
			else
				ExceptionNotify(CurrentException());
		}
		end_try;
	}
	return err;
}


void
CNewScriptEndpointClient::doFlushInput(void)
{
	fInputBufWrIndex = 0;
	fInputBufRdIndex = 0;
	fIsDataAvailable = NO;
}


Ref
CNewScriptEndpointClient::doPartial(NewtonErr * outErr)
{
	*outErr = kCommScriptInappropriatePartial;
	if (fForm == kStringFormType || fForm == kBytesFormType)
		*outErr = noErr;
	if (ISNIL(fInputSpec))
		*outErr = kCommScriptNoActiveInputSpec;
	if (*outErr == noErr)
		getPartialData(outErr);
	return NILREF;
}


Ref
CNewScriptEndpointClient::getPartialData(NewtonErr * outErr)
{
	RefVar obj;
	*outErr = noErr;
	if (fInputBufWrIndex > 0)
	{
		obj = parseInput(fForm, fEncoding, fInputBufWrIndex, fInputBuffer, fTarget, outErr);
		fInputBufRdIndex = fInputBufWrIndex;
		fIsDataAvailable = NO;
	}
	return obj;
}


void
CNewScriptEndpointClient::doFlushPartial(void)
{
	long	amt = fInputBufWrIndex - fInputBufRdIndex;
	if (amt > 0)
		memmove(fInputBuffer, fInputBuffer + fInputBufRdIndex, amt);
	fInputBufWrIndex = amt;
	fInputBufRdIndex = 0;
	fIsDataAvailable = NO;
}


PScriptDataIn *
CNewScriptEndpointClient::getScriptDataInXlator(void)
{
	if (fDataInXlator == NULL)
	{
		fDataInXlator = (PScriptDataIn *)MakeByName("PFrameSource", "PScriptDataIn");
		if (fDataInXlator == NULL)
			ThrowErr(exTranslator, MemError());
	}
	return fDataInXlator;
}


PScriptDataOut *
CNewScriptEndpointClient::getScriptDataOutXlator(void)
{
	if (fDataOutXlator == NULL)
	{
		fDataOutXlator = (PScriptDataOut *)MakeByName("PFrameSink", "PScriptDataOut");
		if (fDataOutXlator == NULL)
			ThrowErr(exTranslator, MemError());
	}
	return fDataOutXlator;
}


void
CNewScriptEndpointClient::queueCallback(RefArg inCallbackSpec)
{
	AddArraySlot(fCommandQueue, inCallbackSpec);
}


void
CNewScriptEndpointClient::unwindCallback(void)
{
	SetLength(fCommandQueue, Length(fCommandQueue) - 1);
}


void
CNewScriptEndpointClient::queueOptions(RefArg inOptions, RefArg inCallbackSpec)
{
	AddArraySlot(fCommandQueue, inOptions);
	AddArraySlot(fCommandQueue, inCallbackSpec);
}


void
CNewScriptEndpointClient::unwindOptions(void)
{
	SetLength(fCommandQueue, Length(fCommandQueue) - 2);
}


bool
CNewScriptEndpointClient::getParms(RefArg inCallbackSpec, Timeout * outTimeout)
{
	bool isSync = YES;
	if (NOTNIL(inCallbackSpec))
	{
		if (NOTNIL(GetVariable(inCallbackSpec, SYMA(async))))
			isSync = NO;
		Ref reqTimeout = GetVariable(inCallbackSpec, SYMA(reqTimeout));
		if (ISINT(reqTimeout))
			*outTimeout = reqTimeout * kMilliseconds;
	}
	return isSync;
}


NewtonErr
CNewScriptEndpointClient::prepOptions(RefArg inOptions, COptionArray ** outArray)
{
	NewtonErr err;
	XTRY
	{
		*outArray = new COptionArray;
		XFAIL(err = MemError())
		XFAIL(err = (*outArray)->init())
		err = convertToOptionArray(inOptions, *outArray);
	}
	XENDTRY;
	return err;
}


NewtonErr
CNewScriptEndpointClient::convertToOptionArray(RefArg inOptions, COptionArray * ioArray)
{
	NewtonErr err = noErr;
	XTRY
	{
		if (fOptionDataOutXlator == NULL)
		{
			fOptionDataOutXlator = (POptionDataOut *)MakeByName("PFrameSink", "POptionDataOut");
			XFAIL(err = MemError())
		}
		newton_try
		{
			OptionDataOutParms xParms;
			xParms.x00 = ioArray;
			xParms.x04 = inOptions;
			xParms.x08 = getScriptDataOutXlator();
			fOptionDataOutXlator->translate(&xParms, NULL);
		}
		newton_catch(exTranslator)
		{
			err = (NewtonErr)(unsigned long)CurrentException()data;
		}
		end_try;
	}
	XENDTRY;
	return err;
}


NewtonErr
CNewScriptEndpointClient::convertFromOptionArray(RefArg outOptions, COptionArray * inArray)
{
	NewtonErr err = noErr;
	XTRY
	{
		if (fOptionDataInXlator == NULL)
		{
			fOptionDataInXlator = (POptionDataIn *)MakeByName("PFrameSource", "POptionDataIn");
			XFAIL(err = MemError())
		}
		newton_try
		{
			OptionDataInParms xParms;
			xParms.x00 = inArray;
			xParms.x04 = outOptions;
			xParms.x08 = getScriptDataInXlator();
			fOptionDataInXlator->translate(&xParms, NULL);
		}
		newton_catch(exTranslator)
		{
			err = (NewtonErr)(unsigned long)CurrentException()data;
		}
		end_try;
	}
	XENDTRY;
	return err;
}


int
CNewScriptEndpointClient::doState(void)
{
	return MAKEINT(fCEndpoint->getState());
}


NewtonErr
CNewScriptEndpointClient::doBind(RefArg inOptions, RefArg inCallbackSpec)
{
	NewtonErr err;
	COptionArray * optionArray = NULL;
	Timeout timeout = kNoTimeout;
	bool isSync = getParms(inCallbackSpec, &timeout);
	XTRY
	{
		if (NOTNIL(inOptions))
			XFAIL(err = prepOptions(inOptions, &optionArray))
		if (!isSync)
			queueOptions(inOptions, inCallbackSpec);
		XFAIL(err = fCEndpoint->nBind(optionArray, timeout, isSync))
		if (isSync && NOTNIL(inOptions))
			err = convertFromOptionArray(inOptions, optionArray);
	}
	XENDTRY;
	if (optionArray && (err || isSync))
		delete optionArray;
	if (err && !isSync)
		unwindOptions();
	return err;
}


NewtonErr
CNewScriptEndpointClient::doListen(RefArg inOptions, RefArg inCallbackSpec)
{
	NewtonErr err;
	COptionArray * optionArray = NULL;
	Timeout timeout = kNoTimeout;
	bool isSync = getParms(inCallbackSpec, &timeout);
	XTRY
	{
		if (NOTNIL(inOptions))
			XFAIL(err = prepOptions(inOptions, &optionArray))
		if (!isSync)
			queueOptions(inOptions, inCallbackSpec);
		XFAIL(err = fCEndpoint->nListen(optionArray, NULL, NULL, timeout, isSync))
		if (isSync && NOTNIL(inOptions))
			err = convertFromOptionArray(inOptions, optionArray);
	}
	XENDTRY;
	if (optionArray && (err || isSync))
		delete optionArray;
	if (err && !isSync)
		unwindOptions();
	return err;
}


NewtonErr
CNewScriptEndpointClient::doAccept(RefArg inOptions, RefArg inCallbackSpec)
{
	NewtonErr err;
	COptionArray * optionArray = NULL;
	Timeout timeout = kNoTimeout;
	bool isSync = getParms(inCallbackSpec, &timeout);
	XTRY
	{
		if (NOTNIL(inOptions))
			XFAIL(err = prepOptions(inOptions, &optionArray))
		if (!isSync)
			queueOptions(inOptions, inCallbackSpec);
		XFAIL(err = fCEndpoint->nAccept(fCEndpoint, optionArray, NULL, NULL, timeout, isSync))
		if (isSync && NOTNIL(inOptions))
			err = convertFromOptionArray(inOptions, optionArray);
	}
	XENDTRY;
	if (optionArray && (err || isSync))
		delete optionArray;
	if (err && !isSync)
		unwindOptions();
	return err;
}


NewtonErr
CNewScriptEndpointClient::doConnect(RefArg inOptions, RefArg inCallbackSpec)
{
	NewtonErr err;
	COptionArray * optionArray = NULL;
	Timeout timeout = kNoTimeout;
	bool isSync = getParms(inCallbackSpec, &timeout);
	XTRY
	{
		if (NOTNIL(inOptions))
			XFAIL(err = prepOptions(inOptions, &optionArray))
		if (!isSync)
			queueOptions(inOptions, inCallbackSpec);
		XFAIL(err = fCEndpoint->nConnect(optionArray, NULL, NULL, timeout, isSync))
		if (isSync && NOTNIL(inOptions))
			err = convertFromOptionArray(inOptions, optionArray);
	}
	XENDTRY;
	if (optionArray && (err || isSync))
		delete optionArray;
	if (err && !isSync)
		unwindOptions();
	return err;
}


NewtonErr
CNewScriptEndpointClient::doOption(RefArg inOptions, RefArg inCallbackSpec)
{
	NewtonErr err;
	COptionArray * optionArray = NULL;
	Timeout timeout = kNoTimeout;
	bool isSync = getParms(inCallbackSpec, &timeout);
	XTRY
	{
		XFAILIF(ISNIL(inOptions), err = kCommScriptInvalidOption;)
		XFAIL(err = prepOptions(inOptions, &optionArray))
		if (!isSync)
			queueOptions(inOptions, inCallbackSpec);
		XFAIL(err = fCEndpoint->nOptMgmt(0x0500, optionArray, timeout, isSync))
		if (isSync)
			err = convertFromOptionArray(inOptions, optionArray);
	}
	XENDTRY;
	if (optionArray && (err || isSync))
		delete optionArray;
	if (err && !isSync)
		unwindOptions();
	return err;
}


NewtonErr
CNewScriptEndpointClient::doDisconnect(RefArg inCancelPending, RefArg inCallbackSpec)
{
	NewtonErr err;
	Timeout timeout = kNoTimeout;
	bool isSync = getParms(inCallbackSpec, &timeout);
	if (!isSync)
		queueCallback(inCallbackSpec);
	XTRY
	{
		if (NOTNIL(inCancelPending))
			err = fCEndpoint->nDisconnect(NULL, 0, 0, timeout, isSync);
		else
			err = fCEndpoint->nRelease(timeout, isSync);
	}
	XENDTRY;
	if (err && isSync)
		unwindCallback();
	return err;
}


NewtonErr
CNewScriptEndpointClient::doUnBind(RefArg inCallbackSpec)
{
	NewtonErr err;
	Timeout timeout = kNoTimeout;
	bool isSync = getParms(inCallbackSpec, &timeout);
	if (!isSync)
		queueCallback(inCallbackSpec);
	XTRY
	{
		err = fCEndpoint->nUnBind(timeout, isSync);
	}
	XENDTRY;
	if (err && isSync)
		unwindCallback();
	return err;
}


NewtonErr
CNewScriptEndpointClient::doAbort(RefArg inCallbackSpec)
{
	NewtonErr err;
	Timeout timeout = kNoTimeout;
	bool isSync = getParms(inCallbackSpec, &timeout);
	if (!isSync)
		AddArraySlot(fAbortQueue, inCallbackSpec);
	XTRY
	{
		err = fCEndpoint->nAbort(isSync);
	}
	XENDTRY;
	if (err && isSync)
		SetLength(fAbortQueue, Length(fAbortQueue) - 1);
	return err;
}


NewtonErr
CNewScriptEndpointClient::doOutput(RefArg inData, RefArg inOptions, RefArg inCallbackSpec)
{
	NewtonErr err;
	FormType form;
	ULong flags = 1;
	COptionArray * optionArray = NULL;
	Timeout timeout = kNoTimeout;
	bool isSync = getParms(inCallbackSpec, &timeout);
	XTRY
	{
		if (NOTNIL(inOptions))
			XFAIL(err = prepOptions(inOptions, &optionArray))
		if (NOTNIL(inCallbackSpec))
		{
			RefVar sndFlags(GetVariable(inCallbackSpec, SYMA(sendFlags)));
			if (ISINT(sndFlags))
				flags = RINT(sndFlags);
			form = GetDataForm(GetVariable(inCallbackSpec, SYMA(form)), 2);
		}
		else
			form = GetDataForm(RA(NILREF), 2);

		if (!isSync)
		{
			AddArraySlot(fSendQueue, RA(NILREF));
			AddArraySlot(fSendQueue, inOptions);
			AddArraySlot(fSendQueue, inCallbackSpec);
		}

		switch (form)
		{
		case kBadFormType:
			err = kCommScriptBadForm;
			break;
		case 1:
			if (IsRaw(inData))
				err = outputRaw(inData, inCallbackSpec, isSync, flags, timeout, optionArray);
			else
				err = outputData(inData, form, isSync, flags, timeout, optionArray);
			break;
		case kBinaryFormType:
			if (IsRawOrString(inData))
				err = outputRaw(inData, inCallbackSpec, isSync, flags, timeout, optionArray);
			else
				err = outputData(inData, form, isSync, flags, timeout, optionArray);
			break;
		case kFrameFormType:
			if (IsFrame(inData))
			{
				err = outputFrame(inData, isSync, flags, timeout, optionArray);
				break;
			}
		default:
			err = outputData(inData, form, isSync, flags, timeout, optionArray);
			break;
		}
		XFAIL(err)

		if (isSync && NOTNIL(inOptions))
			err = convertFromOptionArray(inOptions, optionArray);
	}
	XENDTRY;
	if (optionArray && (err || isSync))
		delete optionArray;
	if (err && !isSync)
		SetLength(fSendQueue, Length(fSendQueue) - 3);
	return err;
}


NewtonErr
CNewScriptEndpointClient::outputFrame(RefArg inData, bool inSync, ULong inFlags, Timeout inTimeout, COptionArray * inOptionArray)
{
	NewtonErr err = noErr;
	char * dataPtr = NULL;
	ArrayIndex dataSize;
	
	XTRY
	{
		if (fFrameDataOutXlator == NULL)
		{
			fFrameDataOutXlator = (PFlattenPtr *)MakeByName("PFrameSink", "PFlattenPtr");
			if (fFrameDataOutXlator == NULL)
				XFAIL(err = MemError())
		}
		newton_try
		{
			FlattenPtrParms xParms;
			xParms.x00 = inData;
			xParms.allocHandle = NO;
			xParms.offset = sizeof(ArrayIndex);
			dataPtr = fFrameDataOutXlator->translate(&xParms, NULL);
		}
		newton_catch(exTranslator)
		{
			err = (NewtonErr)(unsigned long)CurrentException()data;
		}
		end_try;
		XFAIL(err)
		dataSize = GetPtrSize(dataPtr);
		*(ArrayIndex*)dataPtr = dataSize - sizeof(ArrayIndex);
		err = fCEndpoint->nSnd((UChar *)dataPtr, &dataSize, inFlags, inTimeout, inSync, inOptionArray);
	}
	XENDTRY;

	if (err || inSync)
	{
		if (dataPtr)
			FreePtr(dataPtr);
	}

	return err;
}


NewtonErr
CNewScriptEndpointClient::outputData(RefArg inData, FormType inForm, bool inSync, ULong inFlags, Timeout inTimeout, COptionArray * inOptionArray)
{
	NewtonErr err = noErr;
	char * dataPtr = NULL;
	ArrayIndex dataSize;

	XTRY
	{
		newton_try
		{
			ScriptDataOutParms xParms;
			xParms.x00 = inData;
			xParms.x04 = inForm;
			xParms.x08 = fEncoding;
			xParms.x0C = NO;
			xParms.x10 = 0;
			dataPtr = getScriptDataOutXlator()->translate(&xParms, NULL);
		}
		newton_catch(exTranslator)
		{
			err = (NewtonErr)(unsigned long)CurrentException()data;
		}
		end_try;
		XFAIL(err)
		dataSize = GetPtrSize(dataPtr);
		*(ArrayIndex*)dataPtr = dataSize - sizeof(ArrayIndex);	// watch this--get translate to do this size prefix?
		err = fCEndpoint->nSnd((UChar *)dataPtr, &dataSize, inFlags, inTimeout, inSync, inOptionArray);
	}
	XENDTRY;

	if (err || inSync)
	{
		if (dataPtr)
			FreePtr(dataPtr);
	}

	return err;
}


NewtonErr
CNewScriptEndpointClient::outputRaw(RefArg inData, RefArg inCallbackSpec, bool inSync, ULong inFlags, Timeout inTimeout, COptionArray * inOptionArray)
{
	NewtonErr err = noErr;
	LockRef(inData);
	XTRY
	{
		UChar * dataPtr = (UChar *)BinaryData(inData);
		ArrayIndex dataSize = Length(inData);
		if (NOTNIL(inCallbackSpec))
		{
			RefVar target(GetVariable(inCallbackSpec, SYMA(target)));
			if (NOTNIL(target) && IsFrame(target))
			{
				size_t targetOffset = 0;
				RefVar offset(GetVariable(target, SYMA(offset)));
				if (ISINT(offset))
					targetOffset = RINT(offset);
				dataPtr += targetOffset;
				RefVar length(GetVariable(target, SYMA(length)));
				if (ISINT(length))
				{
					size_t targetLength = RINT(length);
					XFAILIF(targetOffset + targetLength > dataSize, err = kCommScriptInappropriateTarget;)
					dataSize = targetLength;
				}
				else
					dataSize -= targetOffset;
			}
		}
		if (!inSync)
			SetArraySlot(fSendQueue, Length(fSendQueue) - 3, inData);
		err = fCEndpoint->nSnd(dataPtr, &dataSize, inFlags, inTimeout, inSync, inOptionArray);
	}
	XENDTRY;

	if (err || inSync)
		UnlockRef(inData);

	return err;
}




int
CNewScriptEndpointClient::checkEndArray(UChar * inBuf)
{
	bool isFound = NO;
	RefVar endItem;
	int i, count = Length(fEndSequence);
	for (i = 0; i < count && !isFound; ++i)
	{
		endItem = GetArraySlot(fEndSequence, i);
		if (ISINT(endItem))
		{
			isFound = (RINT(endItem) == *inBuf);
		}
		else	// must be a byte array
		{
			int byteIndex = Length(endItem) - 1;
			if (inBuf - fInputBuffer >= byteIndex)
			{
				isFound = YES;
				UChar * endSeq = (UChar *)BinaryData(endItem);
				UChar * bufSeq = inBuf - byteIndex;
				for ( ; byteIndex >= 0 && isFound; byteIndex--)
					isFound = (endSeq[byteIndex] == bufSeq[byteIndex]);
			}
		}
	}
	return isFound ? i : 0;
}


#pragma mark -
/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

CEndpointClient *
GetClient(RefArg inNSEndpoint)
{
	Ref ci;
	CEndpointClient * epClient = NULL;
	if (NOTNIL(inNSEndpoint)
	&&  NOTNIL(ci = GetVariable(inNSEndpoint, SYMA(ciprivate))))
	{
		epClient = (CEndpointClient *)RefToAddress(ci);
	}
	return epClient;
}


CEndpoint *
GetClientEndpoint(RefArg inNSEndpoint)
{
	CEndpoint * ep = NULL;
	CEndpointClient * epClient;
	if ((epClient = GetClient(inNSEndpoint)) != NULL)
		ep = epClient->getEndpoint();
	return ep;
}


Ref
CloneOptions(RefArg inOptions)
{
	RefVar options(Clone(inOptions));
	if (IsArray(inOptions))
	{
		for (ArrayIndex i = 0; i < Length(options); ++i)
		{
			SetArraySlot(options, i, Clone(GetArraySlot(inOptions, i)));
		}
	}
	return options;
}

#pragma mark protoBasicEndpoint

Ref
CINewInstantiate(RefArg rcvr, RefArg inEndpoint, RefArg inOptions)
{
	NewtonErr err;
	RefVar options(CloneOptions(inOptions));
	CNewScriptEndpointClient * epClient = new CNewScriptEndpointClient;
	if (epClient == NULL)
		ThrowErr(exComm, kOSErrNoMemory);
	if ((err = epClient->initScriptEndpointClient(inEndpoint, options, NULL)) != noErr)
		ThrowErr(exComm, err);
	return options;
}


Ref
CINewInstantiateFromEndpoint(RefArg rcvr, RefArg inEndpoint, RefArg inOptions, RefArg inCEndpoint)
{
	NewtonErr err;
	RefVar options(CloneOptions(inOptions));
	CNewScriptEndpointClient * epClient = new CNewScriptEndpointClient;
	if (epClient == NULL)
		ThrowErr(exComm, kOSErrNoMemory);
	if ((err = epClient->initScriptEndpointClient(inEndpoint, options, (CEndpoint *)RefToAddress(inCEndpoint))) != noErr)
		ThrowErr(exComm, err);
	return options;
}


Ref
CINewDispose(RefArg rcvr)
{
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient)
		delete epClient;
	SetFrameSlot(rcvr, SYMA(ciprivate), RA(NILREF));
	return NILREF;
}


Ref
CINewDisposeLeavingTool(RefArg rcvr)
{
	NewtonErr err;
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient)
	{
		if ((err = epClient->getEndpoint()->deleteLeavingTool()) != noErr)
			ThrowErr(exComm, err);
		epClient->setNilEndpoint();
		delete epClient;
	}
	SetFrameSlot(rcvr, SYMA(ciprivate), RA(NILREF));
	return NILREF;
}


Ref
CINewDisposeLeavingCEndpoint(RefArg rcvr)
{
	NewtonErr err;
	RefVar epRef;
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient)
	{
		CEndpoint * ep = epClient->getEndpoint();
		epRef = AddressToRef(ep);
		ep->setClientHandler(0);
		ep->abort();
		epClient->setNilEndpoint();
		delete epClient;
	}
	SetFrameSlot(rcvr, SYMA(ciprivate), RA(NILREF));
	return epRef;
}


Ref
CINewOption(RefArg rcvr, RefArg inOptions, RefArg inCallbackSpec)
{
	NewtonErr err;
	RefVar options(CloneOptions(inOptions));
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);
	if ((err = epClient->doOption(options, inCallbackSpec)) != noErr)
		ThrowErr(exComm, err);
	return options;
}


Ref
CINewState(RefArg rcvr)
{
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	return (epClient != NULL) ? epClient->doState() : NILREF;
}


Ref
CINewSetState(RefArg rcvr, RefArg inState)
{
	NewtonErr err;
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient != NULL)
	{
//		if ((err = epClient->getEndpoint()->setState(RINT(inState))) != noErr)
//			ThrowErr(exComm, err);
	}
	return NILREF;
}


Ref
CINewBind(RefArg rcvr, RefArg inOptions, RefArg inCallbackSpec)
{
	NewtonErr err;
	RefVar options(CloneOptions(inOptions));
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);
	if ((err = epClient->doBind(options, inCallbackSpec)) != noErr)
		ThrowErr(exComm, err);
	return options;
}


Ref
CINewConnect(RefArg rcvr, RefArg inOptions, RefArg inCallbackSpec)
{
	NewtonErr err;
	RefVar options(CloneOptions(inOptions));
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);
	if ((err = epClient->doConnect(options, inCallbackSpec)) != noErr)
		ThrowErr(exComm, err);
	return options;
}


Ref
CINewListen(RefArg rcvr, RefArg inOptions, RefArg inCallbackSpec)
{
	NewtonErr err;
	RefVar options(CloneOptions(inOptions));
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);
	if ((err = epClient->doListen(options, inCallbackSpec)) != noErr)
		ThrowErr(exComm, err);
	return options;
}


Ref
CINewAccept(RefArg rcvr, RefArg inOptions, RefArg inCallbackSpec)
{
	NewtonErr err;
	RefVar options(CloneOptions(inOptions));
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);
	if ((err = epClient->doAccept(options, inCallbackSpec)) != noErr)
		ThrowErr(exComm, err);
	return options;
}


Ref
CINewDisconnect(RefArg rcvr, RefArg inCancelPending, RefArg inCallbackSpec)
{
	NewtonErr err;
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);
	if ((err = epClient->doDisconnect(inCancelPending, inCallbackSpec)) != noErr)
		ThrowErr(exComm, err);
	return NILREF;
}


Ref
CINewUnbind(RefArg rcvr, RefArg inCallbackSpec)
{
	NewtonErr err;
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);
	if ((err = epClient->doUnBind(inCallbackSpec)) != noErr)
		ThrowErr(exComm, err);
	return NILREF;
}


Ref
CINewAbort(RefArg rcvr, RefArg inCallbackSpec)
{
	NewtonErr err;
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);
	if ((err = epClient->doAbort(inCallbackSpec)) != noErr)
		ThrowErr(exComm, err);
	return NILREF;
}


Ref
CINewOutput(RefArg rcvr, RefArg inData, RefArg inOptions, RefArg inOutputSpec)
{
	NewtonErr err;
	RefVar options(CloneOptions(inOptions));
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);
	if ((err = epClient->doOutput(inData, options, inOutputSpec)) != noErr)
		ThrowErr(exComm, err);
	return options;
}


Ref
CINewSetInputSpec(RefArg rcvr, RefArg inputSpec)
{
	NewtonErr err;
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);
	if ((err = epClient->doInputSpec(inputSpec)) != noErr)
		ThrowErr(exComm, err);
	return NILREF;
}


Ref
CINewInput(RefArg rcvr)
{
	NewtonErr err;
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);
	if ((err = epClient->doInput()) != noErr)
		ThrowErr(exComm, err);
	return NILREF;
}


Ref
CINewPartial(RefArg rcvr)
{
	NewtonErr err;
	Ref data;
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);
	data = epClient->doPartial(&err);
	if (err)
		ThrowErr(exComm, err);
	return data;
}


Ref
CINewFlushPartial(RefArg rcvr)
{
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);
	epClient->doFlushPartial();
	return NILREF;
}


Ref
CINewFlushInput(RefArg rcvr)
{
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);
	epClient->doFlushInput();
	return NILREF;
}


Ref
CINewRequestsPending(RefArg rcvr, RefArg inType)
{
	NewtonErr err;
	int selector;
	CNewScriptEndpointClient * epClient = (CNewScriptEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);

	if (EQ(inType, SYMA(sync)))
		selector = 1;
	else if (EQ(inType, SYMA(async)))
		selector = 2;
	else
		selector = 3;

	return MAKEBOOLEAN(epClient->getEndpoint()->isPending(selector));
}

#pragma mark protoStreamingEndpoint

Ref
CISNewInstantiate(RefArg rcvr, RefArg inEndpoint, RefArg inOptions)
{
	NewtonErr err;
	CStreamingEndpointClient * epClient = new CStreamingEndpointClient;
	if (epClient == NULL)
		ThrowErr(exComm, kOSErrNoMemory);
	if ((err = epClient->initScriptEndpointClient(inEndpoint, inOptions, NULL)) != noErr)
		ThrowErr(exComm, err);
	return NILREF;
}


Ref
CISNewInstantiateFromEndpoint(RefArg rcvr, RefArg inEndpoint, RefArg inOptions, RefArg inCEndpoint)
{
	NewtonErr err;
	CStreamingEndpointClient * epClient = new CStreamingEndpointClient;
	if (epClient == NULL)
		ThrowErr(exComm, kOSErrNoMemory);
	if ((err = epClient->initScriptEndpointClient(inEndpoint, inOptions, (CEndpoint *)RefToAddress(inCEndpoint))) != noErr)
		ThrowErr(exComm, err);
	return NILREF;
}


Ref
CISStreamIn(RefArg rcvr, RefArg inSpec)
{
	NewtonErr err;
	RefVar data;
	CStreamingEndpointClient * epClient = (CStreamingEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);
	data = epClient->doStreamIn(inSpec, &err);
	if (err != noErr)
		ThrowErr(exComm, err);
	return data;
}


Ref
CISStreamOut(RefArg rcvr, RefArg inData, RefArg inSpec)
{
	NewtonErr err;
	CStreamingEndpointClient * epClient = (CStreamingEndpointClient *)GetClient(rcvr);
	if (epClient == NULL)
		ThrowErr(exComm, kCommScriptNoEndpointAvailable);
	err = epClient->doStreamOut(inData, inSpec);
	if (err != noErr)
		ThrowErr(exComm, err);
	return NILREF;
}

#pragma mark -


/*------------------------------------------------------------------------------
	C S t r e a m i n g C a l l b a c k
------------------------------------------------------------------------------*/

CStreamingCallback::CStreamingCallback()
{
	fReceiver = INVALIDPTRREF;		// why not just leave it as NULL?
	f10 = 0;								// suspect this might be set by an inline
}


CStreamingCallback::~CStreamingCallback()
{ }


bool
CStreamingCallback::status(size_t inNumOfBytesGot, size_t inNumOfBytesPut)
{
	if (fReceiver == INVALIDPTRREF)
		return YES;

	RefVar args(MakeArray(2));
	if (f10)
	{
		SetArraySlot(args, 0, inNumOfBytesGot == 0xFFFFFFFF ? NILREF : MAKEINT(inNumOfBytesGot));
		SetArraySlot(args, 1, f04 == 0xFFFFFFFF ? NILREF : MAKEINT(f04));
	}
	else
	{
		SetArraySlot(args, 0, inNumOfBytesPut == 0xFFFFFFFF ? NILREF : MAKEINT(inNumOfBytesPut));
		SetArraySlot(args, 1, f08 == 0xFFFFFFFF ? NILREF : MAKEINT(f08));
	}
	return NOTNIL(DoMessage(fReceiver, SYMA(ProgressScript), args));
}

