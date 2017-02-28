/*
	File:		CommServer.h

	Contains:	Desktop communications for the test reporting system.

	Written by:	Newton Research Group, 2012.
*/

#include "CommServer.h"


/* -----------------------------------------------------------------------------
	C C o m m S e r v e r
----------------------------------------------------------------------------- */

CCommServer::CCommServer(bool inArg1, bool inArg2, UniChar * inType)
{
	f0C = inArg2;
	f0D = inArg1;
	Ustrcpy(fATType, inType);
	fEvtLength = 0;
	fIsBusy = false;
}


CCommServer::~CCommServer()
{ }


void
CCommServer::setTestServerName(UniChar * inEntity, UniChar * inZone)
{
	static const UniChar kStarStr[2] = {'*',0};

	if (inEntity)
		Ustrcpy(fATEntity, inEntity);
	Ustrcpy(fATZone, inZone ? inZone : kStarStr);
}


NewtonErr
CCommServer::connectToTestServer(CEzEndpointPipe ** outPipe)
{
	NewtonErr err = noErr;
	ConnectionType connType = 3;
	Handle options;	// r6
	*outPipe = NULL;
	XTRY
	{
		XFAILIF(fATEntity[0] == 0, err = -1;)
		fPipe = new CEzEndpointPipe;
		XFAILIF(fPipe == NULL, err = -2;)
		if (fATEntity[0] == '*' && fATEntity[2] == 0)
		{
			if (fATEntity[1] >= '0' && fATEntity[1] <= '9')
				connType = fATEntity[1] - '0';
			XFAILNOT(options = NewHandle(4*sizeof(UniChar)), err = -3;)
			*(UniChar *)options = 0;
		}
		else
		{
			connType = 1;	// AppleTalk
			UniChar addrStr[96];
			for (ArrayIndex i = 0; i < 96; ++i)
				addrStr[i] = 0;
			Ustrcpy(addrStr, fATEntity);
			addrStr[Ustrlen(addrStr)] = ':';
			Ustrcat(addrStr, fATType);
			addrStr[Ustrlen(addrStr)] = '@';
			Ustrcat(addrStr, fATZone);
			XFAILNOT(options = NewHandle(96*sizeof(UniChar)), err = -3;)
			Ustrcpy(options, addrStr);
		}
		newton_try
		{
			err = fPipe.init(connType, options, 0x0D2F0000);
		}
		newton_catch(exPipe)
		{
			testPipeExceptionHandler((NewtonErr)(long)CurrentException()->data);
		}
		end_try;
	}
	XENDTRY;

	protocolInit(kNewtEventClass, kTestAgentEventId);
	*outPipe = fPipe;
	return err;
}


NewtonErr
CCommServer::disconnectFromTestServer(void)
{
	NewtonErr err = -1;
	if (fPipe)
	{
		newton_try
		{
			err = fPipe.tearDown();
		}
		newton_catch(exPipe)
		{
			testPipeExceptionHandler((NewtonErr)(long)CurrentException()->data);
		}
		end_try;

		delete fPipe, fPipe = NULL;
	}
	return err;
}


void
CCommServer::setBusy(bool inBusy)
{
	fIsBusy = inBusy;
}


bool
CCommServer::isBusy(void)
{
	return fIsBusy;
}



EventType
CCommServer::getResponse(size_t * outLength)
{
	EventType response = 0;
	DockEventHeader header;

	size_t chunkLen = 2 * sizeof(ULong);
	readChunk(&header.evtClass, chunkLen, false);

	if (header.evtClass == kNewtEventClass || header.evtId != kTestServerEventId)
	{
		chunkLen = 2 * sizeof(ULong);
		readChunk(&header.evtTag, chunkLen, false);
		response = header.evtTag;
		fEvtLength = header.evtLength;
		if (outLength)
			*outLength = header.evtLength;
	}

	return response;
}


bool
CCommServer::readChunk(void * inBuf, size_t inLength, bool inDone)
{
	bool isEOF;
	newton_try
	{
		fPipe.readChunk(inBuf, inLength, isEOF);
	}
	newton_catch(exPipe)
	{
		testPipeExceptionHandler((NewtonErr)(long)CurrentException()->data);
	}
	end_try;

	if (inDone)
		flushPadding(inLength);

	return isEOF;
}


NewtonErr
CCommServer::readString(char * inStr, size_t inLength, bool inDone)
{
	NewtonErr err = noErr;
	bool isEOF;
	bool isEOS = false;
	for (ArrayIndex i = 0; i < inLength && err == noErr && !isEOS; ++i)
	{
		isEOF = readChunk(inStr+i, 1, false);
		if (inStr[i] == 0)
			isEOS = true;
		if (isEOF)
			err = -2;
		if (i == inLength-1)
			err = -3;
		if (err)
			inStr[i] = 0;
	}

	if (inDone)
		flushPadding(inLength);

	return err;
}


void
CCommServer::flushPadding(size_t inLength)
{
	size_t delta = TRUNC(inLength, kStreamAlignment);
	if (delta > 0)
	{
		newton_try
		{
			bool isEOF;
			size_t chunkLen = kStreamAlignment-delta;
			char padding[kStreamAlignment];
			fPipe.readChunk(padding, chunkLen, isEOF);
		}
		newton_catch(exPipe)
		{
			testPipeExceptionHandler((NewtonErr)(long)CurrentException()->data);
		}
		end_try;
	}
}



void
CCommServer::sendCommandHeader(EventType inCommand, bool inDone)
{
	fPipe.writeDockerHeader(inCommand, inDone);	// was sendDockerHeader
}


void
CCommServer::sendChunk(void * inBuf, size_t inLength, bool inDone)
{
	newton_try
	{
		fPipe << inLength;
		fPipe.writeChunk(inBuf, inLength, false);
	}
	newton_catch(exPipe)
	{
		testPipeExceptionHandler((NewtonErr)(long)CurrentException()->data);
	}
	end_try;
	pad(inLength);
	if (inDone)
	{
		newton_try
		{
			fPipe.flushWrite();
		}
		newton_catch(exPipe)
		{
			testPipeExceptionHandler((NewtonErr)(long)CurrentException()->data);
		}
		end_try;
	}
}


/* -------------------------------------------------------------------------------
	Writing: pad to stream alignment.
	Args:		inLength		actual total length of streamed object
	Return:	--
------------------------------------------------------------------------------- */

void
CCommServer::pad(size_t inLength)
{
	size_t delta = TRUNC(inLength, kStreamAlignment);
	if (delta > 0)
	{
		newton_try
		{
			ULong padding = 0;
			fPipe.writeChunk(&padding, kStreamAlignment-delta, false);
		}
		newton_catch(exPipe)
		{
			testPipeExceptionHandler((NewtonErr)(long)CurrentException()->data);
		}
		end_try;
	}
}


void
CCommServer::testPipeExceptionHandler(NewtonErr inErr)
{
	fError = inErr;
}

