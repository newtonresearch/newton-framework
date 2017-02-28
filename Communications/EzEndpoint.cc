/*
	File:		EzEndpoint.cc

	Contains:	Communications code.
					Error policy in here is to Throw() rather than return an error code.

	Written by:	Newton Research Group, 2013.
*/

#include "Docker.h"
#include "SerialOptions.h"
#include "EndpointClient.h"
#include "CommManagerInterface.h"
#include "OSErrors.h"
#include "RSSymbols.h"
#include "Funcs.h"

DeclareException(exTranslator, exRootException);


CEzEndpointPipe * g0001E0E0;
	
// AppleTalk stubs

NewtonErr
OpenAppleTalk(ULong inId)
{ return noErr; }

NewtonErr
CloseAppleTalk(ULong inId)
{ return noErr; }

NewtonErr
CMGetOptionsForAppleTalkADSP(COptionArray * inOptions)
{ return noErr; }

#pragma mark -

NewtonErr
EzSerialOptions(COptionArray * outOptions, Handle inData, size_t inWrBufLen, size_t inRdBufLen)
{
	NewtonErr err = noErr;
	XTRY
	{
		COption serviceOption;
		serviceOption.setAsService('aser');
		XFAIL(err = outOptions->appendOption(&serviceOption))

		CCMOSerialIOParms serialOption1;
		serialOption1.fSpeed = k38400bps;
		XFAIL(err = outOptions->appendOption(&serialOption1))

		CCMOInputFlowControlParms serialOption2;
		serialOption2.fUseHardFlowControl = true;
		XFAIL(err = outOptions->appendOption(&serialOption2))

		CCMOOutputFlowControlParms serialOption3;
		serialOption3.fUseHardFlowControl = true;
		XFAIL(err = outOptions->appendOption(&serialOption3))

		if (inRdBufLen > 0 && inWrBufLen > 0)
		{
			CCMOSerialBuffers serialOption4;
			serialOption4.fSendSize = inWrBufLen;
			serialOption4.fRecvSize = inRdBufLen;
			err = outOptions->appendOption(&serialOption4);
		}
	}
	XENDTRY;
	return err;
}


bool
EzConvertOptions(RefArg inOptions, COptionArray ** outOpenOptions, COptionArray ** outBindOptions, COptionArray ** outConnectOptions)
{
	NewtonErr err = noErr;
	bool isUsingEOP = false;
	POptionDataOut * optDataOut = NULL;
	PScriptDataOut * scrDataOut = NULL;
	XTRY
	{
		*outOpenOptions = new COptionArray;
		XFAILIF(*outOpenOptions == NULL, err = kOSErrNoMemory;)	// I’m paraphrasing here; the original uses MemError()
		XFAIL(err = (*outOpenOptions)->init())

		optDataOut = (POptionDataOut *)MakeByName("PFrameSink", "POptionDataOut");
		XFAILIF(optDataOut == NULL, err = kOSErrNoMemory;)			// I’m paraphrasing here; the original uses MemError()
		scrDataOut = (PScriptDataOut *)MakeByName("PFrameSink", "PScriptDataOut");
		XFAILIF(scrDataOut == NULL, err = kOSErrNoMemory;)			// ...you get the idea

		newton_try
		{
			XTRY
			{
				RefVar args(MakeArray(0));
				RefVar specOptions(DoMessage(inOptions, SYMA(OpenOptions), args));
				XFAILIF(ISNIL(specOptions), err = -1;)

				FrameSinkParms txParms;
				txParms.x00 = *outOpenOptions;
				txParms.x04 = specOptions;
				txParms.x08 = scrDataOut;
				optDataOut->translate(&txParms, NULL);

				if (FrameHasSlot(inOptions, SYMA(BindOptions)))
				{
					*outBindOptions = new COptionArray;
					XFAILIF(*outBindOptions == NULL, err = kOSErrNoMemory;)
					XFAIL(err = (*outBindOptions)->init())

					specOptions = DoMessage(inOptions, SYMA(BindOptions), args);
					if (NOTNIL(specOptions))
					{
						txParms.x00 = *outOpenOptions;
						txParms.x04 = specOptions;
						txParms.x08 = scrDataOut;
						optDataOut->translate(&txParms, NULL);
					}
				}

				if (FrameHasSlot(inOptions, SYMA(ConnectOptions)))
				{
					*outConnectOptions = new COptionArray;
					XFAILIF(*outConnectOptions == NULL, err = kOSErrNoMemory;)
					XFAIL(err = (*outConnectOptions)->init())

					specOptions = DoMessage(inOptions, SYMA(ConnectOptions), args);
					if (NOTNIL(specOptions))
					{
						txParms.x00 = *outConnectOptions;
						txParms.x04 = specOptions;
						txParms.x08 = scrDataOut;
						optDataOut->translate(&txParms, NULL);
					}
				}

				isUsingEOP = ISTRUE(GetFrameSlot(inOptions, SYMA(useEOP)));
			}
			XENDTRY;
		}
		newton_catch(exTranslator)
		{
			// we have an error, but we don’t want to rethrow it
			err = 1;
		}
		end_try;
	}
	XENDTRY;

	XDOFAIL(err)
	{
		if (optDataOut)
			optDataOut->destroy();
		if (scrDataOut)
			scrDataOut->destroy();
		if (*outOpenOptions)
			delete *outOpenOptions;
		if (*outBindOptions)
			delete *outBindOptions;
		if (*outConnectOptions)
			delete *outConnectOptions;

		if (err < 0)
			ThrowErr(exPipe, err);
	}
	XENDFAIL;

	return isUsingEOP;
}

#pragma mark -

CEzEndpointPipe::CEzEndpointPipe()
{
	fError = noErr;
	fEndpoint = NULL;
	fTimeout = 10*kSeconds;
	f30 = NULL;
	fIsAppleTalkOpen = false;
}


CEzEndpointPipe::CEzEndpointPipe(bool)
{ }


CEzEndpointPipe::~CEzEndpointPipe()
{
	if (fEndpoint)
		tearDown();
}


void
CEzEndpointPipe::init(RefArg inOptions, Timeout inTimeout)
{
	NewtonErr err = noErr;
	XTRY
	{
		// we MUST have OpenOptions
		XFAILIF(ISNIL(inOptions), err = -1;)
		XFAILNOT(FrameHasSlot(inOptions, SYMA(OpenOptions)), err = -1;)
		// convert frame to option arrays
		COptionArray * openOpt = NULL;
		COptionArray * bindOpt = NULL;
		COptionArray * connectOpt = NULL;
		bool useEOP = EzConvertOptions(inOptions, &openOpt, &bindOpt, &connectOpt);
		// initialise this endpoint with those options
		unwind_protect
		{
			init(openOpt, bindOpt, connectOpt, useEOP, inTimeout);
		}
		on_unwind
		{
			if (openOpt)
				delete openOpt;
			if (bindOpt)
				delete bindOpt;
			if (connectOpt)
				delete connectOpt;
		}
		end_unwind;
	}
	XENDTRY;
	XDOFAIL(err)
	{
		ThrowErr(exPipe, err);
	}
	XENDFAIL;
}


void
CEzEndpointPipe::init(COptionArray * inOpenOptions, COptionArray * inBindOptions, COptionArray * inConnectOptions, bool inUseEOP, Timeout inTimeout)
{
	XTRY
	{
		commonInit(inTimeout);
		XFAIL(fError = RunModemNavigator(inOpenOptions))

		if ((fError = CMGetEndpoint(inOpenOptions, &fEndpoint, false)) == noErr)
		{
			if ((fError = fEndpoint->open(0)) == noErr)
			{
				if ((fError = fEndpoint->nBind(inBindOptions, inTimeout, true)) == noErr)
				{
					if ((fError = fEndpoint->nConnect(inConnectOptions, NULL, NULL, inTimeout, true)) == noErr)
					{
						CEndpointPipe::init(fEndpoint, 2*KByte, 2*KByte, fEzTimeout, inUseEOP, NULL);
						XFAIL(true);	// NO! Not a failure, success!
					}
					// on error, unwind endpoint create(), open(), bind()
					fEndpoint->nUnBind(0, true);
				}
				fEndpoint->close();
			}
			fEndpoint->destroy(), fEndpoint = NULL;
		}
	}
	XENDTRY;
	XDOFAIL(fError)
	{
		ThrowErr(exPipe, fError);
	}
	XENDFAIL;
}


void
CEzEndpointPipe::init(ConnectionType inType, Handle inArg2, Timeout inTimeout)
{
	newton_try
	{
		commonInit(inTimeout);
		f30 = inArg2;
		switch (inType)
		{
		case kSerial:
			getSerialEndpoint();
			break;
		case kAppleTalk:
			fError = OpenAppleTalk('sltk');
			if (fError != noErr)
				ThrowErr(exPipe, fError);
			fIsAppleTalkOpen = true;
			getADSPEndpoint();
			break;
		case kMNPSerial:
			getMNPSerialEndpoint();
			break;
		case kSharpIR:
			getSharpIREndpoint();
			break;
		case kMNPModem:
			getMNPModemEndpoint();
			break;
		case kIrDA:
			getIrDAEndpoint();
			break;
		}
		CEndpointPipe::init(fEndpoint, 2*KByte, 2*KByte, fEzTimeout, inType == kSharpIR, NULL);
	}
	newton_catch(exPipe)
	{
		tearDown();
		fError = (NewtonErr)(long)CurrentException()->data;
	}
	cleanup
	{
		NewtonErr err = tearDown();
		if (err == noErr)
			err = -1;
		if (fError)
			err = fError;
		fError = err;
	}
	end_try;
}


NewtonErr
CEzEndpointPipe::commonInit(Timeout inTimeout)
{
	XTRY
	{
		fEzTimeout = inTimeout;
		XFAIL(fError = f38.init())
		CCMOSerialBytesAvailable bytesAvail;
		XFAIL(fError = f38.appendOption(&bytesAvail))
		fBytesAvailableOption = f38.optionAt(0);
	}
	XENDTRY;
	XDOFAIL(fError)
	{
		ThrowErr(exPipe, fError);
	}
	XENDFAIL;
}



void
CEzEndpointPipe::getSerialEndpoint(void)
{
	XTRY
	{
		COptionArray options;
		// set up serial options
		XFAIL(fError = options.init())
		XFAIL(fError = EzSerialOptions(&options, f30, 2*KByte, 2*KByte))
		// get the endpoint
		XFAIL(fError = CMGetEndpoint(&options, &fEndpoint, false))
		fEndpoint->useForks(true);
		// connect the endpoint
		fError = fEndpoint->easyConnect(0, &options, fEzTimeout);
	}
	XENDTRY;
	XDOFAIL(fError)
	{
		ThrowErr(exPipe, fError);
	}
	XENDFAIL;
}


void
CEzEndpointPipe::getMNPSerialEndpoint(void)
{
	XTRY
	{
		COptionArray options;
		// set up serial options
		XFAIL(fError = options.init())
		XFAIL(fError = EzMNPSerialOptions(&options, f30))
		// get the endpoint
		XFAIL(fError = CMGetEndpoint(&options, &fEndpoint, false))
		fEndpoint->useForks(true);
		// add MNP options
		XFAIL(fError = options.removeAllOptions())
		XFAIL(fError = EzMNPConnectOptions(&options, f30))
		// connect the endpoint
		fError = fEndpoint->easyConnect(0, &options, fEzTimeout);
	}
	XENDTRY;
	XDOFAIL(fError)
	{
		ThrowErr(exPipe, fError);
	}
	XENDFAIL;
}


void
CEzEndpointPipe::getMNPModemEndpoint(void)
{
	XTRY
	{
		COptionArray options;
		// set up modem options
		XFAIL(fError = options.init())
		XFAIL(fError = EzMNPModemOptions(&options, f30))
		if (UseModemNavigator())
			XFAIL(fError = RunModemNavigator(&options))
		// get the endpoint
		XFAIL(fError = CMGetEndpoint(&options, &fEndpoint, false))
		fEndpoint->useForks(true);
		// add phone options
		XFAIL(fError = options.removeAllOptions())
		char phoneNoStr[256];
		size_t phoneNoStrLen = strlen(phoneNoStr);
		HLock(f30);
		ConvertFromUnicode((UniChar *)*f30, phoneNoStr);
		HUnlock(f30);
		//sp-14
		CCMAPhoneNumber phoneNo(phoneNoStrLen);
		XFAIL(fError = options.appendVarOption(&phoneNo, &phoneNoStr, phoneNoStrLen))	// original does not XFAIL
		//sp-0C
		CCMOIdleTimer idleTimer;
		idleTimer.fValue = 30;	// kDefaultTimeout should be defined somewhere
		XFAIL(fError = options.appendOption(&idleTimer))

		// connect the endpoint
		fError = fEndpoint->easyConnect(0, &options, fEzTimeout);
	}
	XENDTRY;
	XDOFAIL(fError)
	{
		ThrowErr(exPipe, fError);
	}
	XENDFAIL;
}


void
CEzEndpointPipe::getSharpIREndpoint(void)
{
	XTRY
	{
		COptionArray options;
		// set up serial options
		XFAIL(fError = options.init())
		XFAIL(fError = EzSharpIROptions(&options, f30))
		// get the endpoint
		XFAIL(fError = CMGetEndpoint(&options, &fEndpoint, false))
		fEndpoint->useForks(true);
		// connect the endpoint
		fError = fEndpoint->easyConnect(0, &options, fEzTimeout);
	}
	XENDTRY;
	XDOFAIL(fError)
	{
		ThrowErr(exPipe, fError);
	}
	XENDFAIL;
}


void
CEzEndpointPipe::getIrDAEndpoint(void)
{
	XTRY
	{
		COptionArray options;
		// set up serial options
		XFAIL(fError = options.init())
		XFAIL(fError = EzIrDAOptions(&options, f30))
		// get the endpoint
		XFAIL(fError = CMGetEndpoint(&options, &fEndpoint, false))
		fEndpoint->useForks(true);
		// connect the endpoint
		fError = fEndpoint->easyConnect(0, &options, fEzTimeout);
	}
	XENDTRY;
	XDOFAIL(fError)
	{
		ThrowErr(exPipe, fError);
	}
	XENDFAIL;
}


void
CEzEndpointPipe::getADSPEndpoint(void)
{
	XTRY
	{
		COptionArray options;
		// set up ADSP options
		XFAIL(fError = options.init())
		XFAIL(fError = EzADSPConnectOptions(&options, f30, 2*KByte, 2*KByte))

		COptionArray appleTalkOptions;
		// set up AppleTalk options
		XFAIL(fError = options.init())
		XFAIL(fError = CMGetOptionsForAppleTalkADSP(&appleTalkOptions))

		// get the endpoint
		XFAIL(fError = CMGetEndpoint(&appleTalkOptions, &fEndpoint, false))
		fEndpoint->useForks(true);

		// connect the endpoint
		XFAIL(fError = fEndpoint->open(0))
		XFAIL(fError = fEndpoint->nBind(NULL, 0, true))
		fError = fEndpoint->nConnect(&appleTalkOptions, NULL, NULL, 0, true);
	}
	XENDTRY;
	XDOFAIL(fError)
	{
		ThrowErr(exPipe, fError);
	}
	XENDFAIL;
}



ArrayIndex
CEzEndpointPipe::bytesAvailable(void)
{
	ArrayIndex count = 0;
	XTRY
	{
		if (fEndpoint && fBytesAvailableOption)
		{
			fBytesAvailableOption->reset();
			fBytesAvailableOption->setOpCode(opGetCurrent);
			XFAIL(fError = fEndpoint->nOptMgmt(opProcess, &f38, fEzTimeout))
			XFAIL(fError = fBytesAvailableOption->getOpCodeResults())
			count = fBytesAvailableOption->length();
		}
	}
	XENDTRY;
	XDOFAIL(fError)
	{
		ThrowErr(exPipe, fError);
	}
	XENDFAIL;
	return count;
}


void
CEzEndpointPipe::abort(void)
{ /* this really does nothing */ }


NewtonErr
CEzEndpointPipe::tearDown(void)
{
	if (f30)
		DisposeHandle(f30), f30 = NULL;
	XTRY
	{
		if (fEndpoint)
		{
			XFAIL(fError = fEndpoint->easyClose())
			fEndpoint->destroy(), fEndpoint = NULL;
		}
		if (fIsAppleTalkOpen)
		{
			XFAIL(fError = CloseAppleTalk('sltk'))
			fIsAppleTalkOpen = false;
		}
	}
	XENDTRY;
	return fError;
}
