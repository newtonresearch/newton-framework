/*
	File:		SoundServer.cc

	Contains:	Sound server implementation.

	Written by:	Newton Research Group, 2007.
*/

#include "SoundServer.h"
#include "SoundErrors.h"
#include "MuLawCodec.h"
#include "IMACodec.h"
#include "GSMCodec.h"
#include "DTMFCodec.h"
#include "SystemEvents.h"
#include "Power.h"


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

CUPort *			gSoundPort = NULL;		// 0C101B10	was gSndPort
PSoundDriver *	gSoundDriver = NULL;		// 0C101B14	was gSndDriver
ULong				gMaxFilterNodes;			// 0C101B18
ULong				gNumFilterNodes;			// 0C101B1C


/* -----------------------------------------------------------------------------
	I n i t i a l i z a t i o n
----------------------------------------------------------------------------- */

void
InitializeSound(void)
{
	IOPowerOff(25);
	IOPowerOff(24);

	RegisterSoundHardwareDriver();

	// create the sound server task
	CSoundServer soundServer;
	soundServer.init('sndm', true, kSpawnedTaskStackSize, kSoundTaskPriority, kNoId);

	CMuLawCodec::classInfo()->registerProtocol();
	CIMACodec::classInfo()->registerProtocol();
	CGSMCodec::classInfo()->registerProtocol();
	CDTMFCodec::classInfo()->registerProtocol();

	gMaxFilterNodes = 6;		// was (gMainCPUType == 3) ? 6 : 0;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C S o u n d S e r v e r
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------- */

CSoundServer::CSoundServer()
{
	fOutputRequest = NULL;
	fInputRequest = NULL;
	fUID = 0;
	fB8 = NULL;
	fBC = 0;
	fC0 = 0;
	fOutputChannels = NULL;
	fC8 = NULL;
	fCC = NULL;
	fD0 = 0;
	fD4 = 0;
	fD8 = 0;
	fInputChannels = NULL;
	fE0 = NULL;
	fE4 = NULL;
	fE8 = 0;
	fEC = 0;
	fF0 = 0;
	fF4 = 0;
	fDecompressorChannels = NULL;
	fCompressorChannels = NULL;
}


/* -----------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------- */

CSoundServer::~CSoundServer()
{ }


size_t
CSoundServer::getSizeOf(void) const
{ return sizeof(CSoundServer); }


/* -----------------------------------------------------------------------------
	App world constructor.
	Create the driver and sound sample buffers.
----------------------------------------------------------------------------- */

NewtonErr
CSoundServer::mainConstructor(void)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = CAppWorld::mainConstructor())

		gSoundDriver = (PSoundDriver *)MakeByName("PSoundDriver", "PMainSoundDriver");
		if (gSoundDriver == NULL)
			gSoundDriver = (PSoundDriver *)MakeByName("PSoundDriver", "PCirrusSoundDriver");
		XFAILNOT(gSoundDriver, err = kSndErrGeneric;)

#if defined(correct)
		LockPtr((Ptr)gSoundDriver);

		// assume the driver ALWAYS has SoundOutput capability
		fB8 = NewPtr(0x0EA0);
		XFAILIF(fB8 == NULL, err = MemError();)
		fC8 = NewWiredPtr(0x0EA0);
		XFAILIF(fC8 == NULL, err = MemError();)
		fCC = NewWiredPtr(0x0EA0);
		XFAILIF(fCC == NULL, err = MemError();)
		gSoundDriver->setOutputBuffers((VAddr)fC8, 0x0EA0, (VAddr)fCC, 0x0EA0);
		gSoundDriver->setOutputCallbackProc(MemberFunctionCast(SoundDriverCallbackProcPtr, this, &CSoundServer::soundOutputIH), this);
		if (gSoundDriver->classInfo()->getCapability("SoundInput"))
		{
			fE0 = NewWiredPtr(0x0EA0);
			XFAILIF(fE0 == NULL, err = MemError();)
			fE4 = NewWiredPtr(0x0EA0);
			XFAILIF(fE4 == NULL, err = MemError();)
			gSoundDriver->setInputBuffers((VAddr)fE0, 0x0EA0, (VAddr)fE4, 0x0EA0);
			gSoundDriver->setInputCallbackProc(MemberFunctionCast(SoundDriverCallbackProcPtr, this, &CSoundServer::soundInputIH), this);
		}
#endif
		gSoundDriver->powerOutputOff();
		gSoundDriver->powerInputOff();

		XFAIL(err = fSoundEventHandler.init(this))
		XFAIL(err = fPowerEventHandler.init(this))

		gSoundPort = getMyPort();
		fOutputRequest = new SoundRequest;
		XFAILIF(fOutputRequest == NULL, err = MemError();)
		XFAIL(err = fOutputRequest->msg.init(false))
		fOutputRequest->req.fSelector = kSndScheduleOutput;
		fInputRequest = new SoundRequest;
		XFAILIF(fInputRequest == NULL, err = MemError();)
		XFAIL(err = fInputRequest->msg.init(false))
		fInputRequest->req.fSelector = kSndScheduleInput;
	}
	XENDTRY;
	return err;
}


/* -----------------------------------------------------------------------------
	App world destructor.
	Free the buffers.
----------------------------------------------------------------------------- */

void
CSoundServer::mainDestructor(void)
{
	stopAll();

	for (CSoundChannel * channel = fOutputChannels; channel != NULL; channel = channel->next())
		delete channel;
	for (CSoundChannel * channel = fInputChannels; channel != NULL; channel = channel->next())
		delete channel;
	for (CSoundChannel * channel = fCompressorChannels; channel != NULL; channel = channel->next())
		delete channel;
	for (CSoundChannel * channel = fDecompressorChannels; channel != NULL; channel = channel->next())
		delete channel;

	gSoundDriver->destroy(), gSoundDriver = NULL;
	gSoundPort = NULL;

	if (fOutputRequest)
		delete fOutputRequest;
	FreePtr(fC8), fC8 = NULL;
	FreePtr(fCC), fCC = NULL;

	if (fInputRequest)
		delete fInputRequest;
	FreePtr(fE0), fE0 = NULL;
	FreePtr(fE4), fE4 = NULL;

	CAppWorld::mainDestructor();
}


/* -----------------------------------------------------------------------------
	App world main event loop.
----------------------------------------------------------------------------- */

void
CSoundServer::theMain(void)
{
//	LockStack(&fB0, 2000);
	CAppWorld::theMain();
//	UnlockStack(&fB0);
}


/* -----------------------------------------------------------------------------
	Set the input device.
	Args:		inChannelId
				inDeviceNo
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CSoundServer::setInputDevice(ULong inChannelId, int inDeviceNo)
{
	findChannel(inChannelId)->fDevice = inDeviceNo;
	return noErr;
}


/* -----------------------------------------------------------------------------
	Set the output device.
	Args:		inChannelId
				inDeviceNo
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CSoundServer::setOutputDevice(ULong inChannelId, int inDeviceNo)
{
	findChannel(inChannelId)->fDevice = inDeviceNo;
	return noErr;
}


/* -----------------------------------------------------------------------------
	Set the input volume.
	Args:		inVolume
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CSoundServer::setInputVolume(int inVolume)
{
	if (gSoundDriver->classInfo()->getCapability("SoundInput"))
	{
		if (inVolume != gSoundDriver->inputVolume())
			gSoundDriver->inputVolume(inVolume);
		return noErr;
	}
	return kSndErrBadConfiguration;
}


/* -----------------------------------------------------------------------------
	Set the output volume.
	Args:		inVolume
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CSoundServer::setOutputVolume(float inVolume)
{
	if (inVolume != fOutputVolume)
	{
		gSoundDriver->outputVolume(inVolume);
		fOutputVolume = inVolume;
	}
	return noErr;
}


#pragma mark Channels
/* -----------------------------------------------------------------------------
	Generate a unique sound channel id.
	Args:		--
	Return:	the sound channel id
----------------------------------------------------------------------------- */

ULong
CSoundServer::uniqueId(void)
{
	do {
		if (++fUID == 0) fUID = 1;
	} while (findChannel(fUID) != NULL);
	return fUID;
}


/* -----------------------------------------------------------------------------
	Find a sound channel.
	Args:		inChannelId
	Return:	the sound channel
				NULL => no channel for that id
----------------------------------------------------------------------------- */

CSoundChannel *
CSoundServer::findChannel(ULong inChannelId)
{
	for (CSoundChannel * channel = fOutputChannels; channel != NULL; channel = channel->next())
		if (channel->fId == inChannelId)
			return channel;
	for (CSoundChannel * channel = fDecompressorChannels; channel != NULL; channel = channel->next())
		if (channel->fId == inChannelId)
			return channel;
	for (CSoundChannel * channel = fInputChannels; channel != NULL; channel = channel->next())
		if (channel->fId == inChannelId)
			return channel;
	for (CSoundChannel * channel = fCompressorChannels; channel != NULL; channel = channel->next())
		if (channel->fId == inChannelId)
			return channel;
	return NULL;
}


/* -----------------------------------------------------------------------------
	Start a sound channel.
	Args:		inChannelId
				inToken
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CSoundServer::startChannel(ULong inChannelId, CUMsgToken * inToken)
{
	NewtonErr err = kSndErrChannelIsClosed;
	CSoundChannel * channel = findChannel(inChannelId);
	if (channel != NULL)
	{
		CSoundChannel * targetChannel = NULL;
		if (FLAGTEST(channel->fFlags, 0x20) || FLAGTEST(channel->fFlags, 0x10))	// has output/input codec
		{
			if (FLAGTEST(channel->fFlags, 0x08))
			{
				targetChannel = channel;
				channel = findChannel(((CCodecChannel *)channel)->f200);
			}
			else if (channel->f0C == 0)
				channel = findChannel(((CCodecChannel *)channel)->f200);
		}


		if (channel != NULL && (!FLAGTEST(channel->fFlags, 0x04) || FLAGTEST(channel->fFlags, 0x08)))
		{
			err = channel->start(inToken);
			if (err == noErr)
			{
				if (FLAGTEST(channel->fFlags, 0x01))
					startOutput(channel->fDevice);
				else if (FLAGTEST(channel->fFlags, 0x02))
					startInput(channel->fDevice);
				else if (FLAGTEST(channel->fFlags, 0x20))
					startDecompressor(channel->fDevice);
				else if (FLAGTEST(channel->fFlags, 0x10))
					startCompressor(channel->fDevice);
			}
		}


		if (targetChannel)
		{
			// we have a codec feeding this channel; wait until the codec has generated some samples for us
			if (err == kSndErrNoSamples)
				err = noErr;
			targetChannel->pause(NULL);
		}
	}
	return err;
}


/* -----------------------------------------------------------------------------
	Pause a sound channel.
	Args:		inChannelId
				ioReply
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CSoundServer::pauseChannel(ULong inChannelId, CUSoundNodeReply * ioReply)
{
	NewtonErr err = kSndErrChannelIsClosed;
	CSoundChannel * channel = findChannel(inChannelId);
	if (channel != NULL)
	{
		bool hasCodec = false;
		CSoundChannel * targetChannel;
		if (FLAGTEST(channel->fFlags, 0x20) || FLAGTEST(channel->fFlags, 0x10))	// has output/input codec
		{
			if (FLAGTEST(channel->fFlags, 0x04))
				hasCodec = true;
			targetChannel = findChannel(((CCodecChannel *)channel)->f200);
		}
		else
		{
			// targeting the channel directly
			targetChannel = channel;
		}

		if (targetChannel)
		{
			err = noErr;
			if (hasCodec)
			{
				channel->pause(ioReply);
				targetChannel->pause(NULL);
			}
			else
				targetChannel->pause(ioReply);
		}
	}
	return err;
}


/* -----------------------------------------------------------------------------
	Stop a sound channel.
	Args:		inChannelId
				ioReply
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CSoundServer::stopChannel(ULong inChannelId, CUSoundNodeReply * ioReply)
{
	NewtonErr err = kSndErrChannelIsClosed;
	CSoundChannel * channel = findChannel(inChannelId);
	if (channel != NULL)
	{
		if (FLAGTEST(channel->fFlags, 0x20) || FLAGTEST(channel->fFlags, 0x10))	// has output/input codec
		{
			if (!FLAGTEST(channel->fFlags, 0x04))
				channel = findChannel(((CCodecChannel *)channel)->f200);
		}

		if (channel)
		{
			err = noErr;
			channel->stop(ioReply, kSndErrChannelIsClosed);
			if (FLAGTEST(channel->fFlags, 0x02) || FLAGTEST(channel->fFlags, 0x10))	// is input/codec channnel
			{
				if (allInputChannelsEmpty())
				{
					gSoundDriver->stopInput();
					gSoundDriver->powerInputOff();
				}
			}
		}
	}
	return err;
}


/* -----------------------------------------------------------------------------
	Close a sound channel.
	Args:		inChannelId
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CSoundServer::closeChannel(ULong inChannelId)
{
	CSoundChannel * queue, * channel, * prevChannel;
	queue = fOutputChannels;
	for (prevChannel = NULL, channel = queue; channel != NULL; prevChannel = channel, channel = channel->next())
		if (channel->fId == inChannelId)
			break;
	if (channel == NULL)
	{
		queue = fInputChannels;
		for (prevChannel = NULL, channel = queue; channel != NULL; prevChannel = channel, channel = channel->next())
			if (channel->fId == inChannelId)
				break;
	}
	if (channel == NULL)
	{
		queue = fDecompressorChannels;
		for (prevChannel = NULL, channel = queue; channel != NULL; prevChannel = channel, channel = channel->next())
			if (channel->fId == inChannelId)
				break;
	}
	if (channel == NULL)
	{
		queue = fCompressorChannels;
		for (prevChannel = NULL, channel = queue; channel != NULL; prevChannel = channel, channel = channel->next())
			if (channel->fId == inChannelId)
				break;
	}
	if (channel != NULL)
	{
		// stop it
		channel->stop(NULL, kSndErrChannelIsClosed);
		// unthread it from its queue
		if (prevChannel)
			prevChannel->fNext = channel->next();
		else
			queue = channel->next();
		// delete it
		delete channel;
	}
	return noErr;
}


bool
CSoundServer::allInputChannelsEmpty(void)
{
	for (CSoundChannel * channel = fInputChannels; channel != NULL; channel = channel->next())
	{
		if (FLAGTEST(channel->fFlags, 0x04) && channel->f0C != 0 && !FLAGTEST(channel->fFlags, 0x08))
			return false;
	}
	return true;
}

bool
CSoundServer::allOutputChannelsEmpty(void)
{
	for (CSoundChannel * channel = fOutputChannels; channel != NULL; channel = channel->next())
	{
		if (FLAGTEST(channel->fFlags, 0x04) && channel->f0C != 0 && !FLAGTEST(channel->fFlags, 0x08))
			return false;
	}
	return true;
}


#pragma mark Nodes

NewtonErr
CSoundServer::scheduleNode(CUSoundNodeRequest * inRequest, CUMsgToken * inToken)
{
	NewtonErr err = kSndErrChannelIsClosed;
	CSoundChannel * channel = findChannel(inRequest->fChannel);
	if (channel != NULL)
		err = channel->schedule(inRequest, inToken);
	return err;
}


NewtonErr
CSoundServer::cancelNode(CUSoundNodeRequest * inRequest)
{
	NewtonErr err = kSndErrChannelIsClosed;
	CSoundChannel * channel = findChannel(inRequest->fChannel);
	if (channel != NULL)
		err = channel->cancel(inRequest);
	return err;
}

#pragma mark Input

void
CSoundServer::scheduleInputBuffer(int)
{}


/* -----------------------------------------------------------------------------
	Open a sound input channel.
	Args:		outChannelId		id of channel opened
				inForDeviceNo		id of device requiring input
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CSoundServer::openInputChannel(ULong * outChannelId, ULong inForDeviceNo)
{
	NewtonErr err = kSndErrBadConfiguration;
	XTRY
	{
		// sanity check
		XFAIL(gSoundDriver->classInfo()->getCapability("SoundInput") == NULL)
		*outChannelId = 0;
		// create codec channel
		SoundDriverInfo info;
		gSoundDriver->getSoundHardwareInfo(&info);
		CDMAChannel * channel = new CDMAChannel(uniqueId(), info);
		XFAILIF(channel == NULL, err = MemError();)
		channel->fFlags |= 0x02;
		// set target channel info
		channel->fDevice = inForDeviceNo;
		// thread channel onto head of list
		channel->fNext = fInputChannels;
		fInputChannels = channel;
		// finally set return id
		*outChannelId = channel->fId;
		err = noErr;
	}
	XENDTRY;
	return err;
}


/* -----------------------------------------------------------------------------
	Start input sound channel.
	Args:		inDeviceNo
	Return:	--
----------------------------------------------------------------------------- */

void
CSoundServer::startInput(int inDeviceNo)
{
	if (!gSoundDriver->inputIsRunning() && !allInputChannelsEmpty())
	{
		fInputRequest->msg.abort();
		fInputRequest->req.fValue = 0;
		fF4 = false;
		gSoundDriver->scheduleInputBuffer(fF0, 0x0EA0);
		gSoundDriver->powerInputOn(inDeviceNo);
		if (!gSoundDriver->inputIsEnabled())
			gSoundDriver->startInput();
		gSoundDriver->scheduleInputBuffer(1 - fF0, 0x0EA0);
	}
}


/* -----------------------------------------------------------------------------
	Stop input sound channels.
	Args:		inDeviceNo
	Return:	--
----------------------------------------------------------------------------- */

void
CSoundServer::stopInput(int inDeviceNo)
{
	for (CSoundChannel * channel = fInputChannels; channel != NULL; channel = channel->next())
		channel->stop(NULL, kSndErrNotPlayed);

	if (inDeviceNo)
	{
		gSoundDriver->stopInput();
		gSoundDriver->powerInputOff();
	}
}


/* -----------------------------------------------------------------------------
	Open a compressor sound channel.
	Args:		outChannelId		id of channel opened
				inForChannelId		id of input channel requiring compression
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CSoundServer::openCompressorChannel(ULong * outChannelId, ULong inForChannelId)
{
	NewtonErr err = kSndErrBadConfiguration;
	XTRY
	{
		// sanity check
		XFAIL(gSoundDriver->classInfo()->getCapability("SoundInput") == NULL)
		*outChannelId = 0;
		// create codec channel
		SoundDriverInfo info;
		gSoundDriver->getSoundHardwareInfo(&info);
		CCodecChannel * channel = new CCodecChannel(uniqueId(), info);
		XFAILIF(channel == NULL, err = MemError();)
		channel->fFlags |= 0x10;
		// set target channel info
		channel->f200 = inForChannelId;
		channel->f204 = findChannel(inForChannelId);
		// thread channel onto head of list
		channel->fNext = fCompressorChannels;
		fCompressorChannels = channel;
		// finally set return id
		*outChannelId = channel->fId;
		err = noErr;
	}
	XENDTRY;
	return err;
}


/* -----------------------------------------------------------------------------
	Start compressor input sound channels.
	Nothing to do here.
	Args:		inDeviceNo
	Return:	--
----------------------------------------------------------------------------- */

void
CSoundServer::startCompressor(int inDeviceNo)
{ /* this really does nothing */ }


/* -----------------------------------------------------------------------------
	Stop compressor input sound channels.
	Args:		inDeviceNo
	Return:	--
----------------------------------------------------------------------------- */

void
CSoundServer::stopCompressor(int inDeviceNo)
{
	for (CSoundChannel * channel = fCompressorChannels; channel != NULL; channel = channel->next())
		channel->stop(NULL, kSndErrNotPlayed);
}

int
CSoundServer::soundInputIH(void)
{ return 0; }


#pragma mark Output

void
CSoundServer::prepOutputChannels(void)
{ /* only used by fillDMABuffer() */ }


/* -----------------------------------------------------------------------------
	Open a sound output channel.
	Args:		outChannelId		id of channel opened
				inForDeviceNo		id of device requiring output
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CSoundServer::openOutputChannel(ULong * outChannelId, ULong inForDeviceNo)
{
	NewtonErr err = kSndErrBadConfiguration;
	XTRY
	{
		// sanity check
		XFAIL(gSoundDriver->classInfo()->getCapability("SoundOutput") == NULL)
		*outChannelId = 0;
		// create codec channel
		SoundDriverInfo info;
		gSoundDriver->getSoundHardwareInfo(&info);
		CDMAChannel * channel = new CDMAChannel(uniqueId(), info);
		XFAILIF(channel == NULL, err = MemError();)
		channel->fFlags |= 0x01;
		// set target channel info
		channel->fDevice = inForDeviceNo;
		// thread channel onto head of list
		channel->fNext = fOutputChannels;
		fOutputChannels = channel;
		// finally set return id
		*outChannelId = channel->fId;
		err = noErr;
	}
	XENDTRY;
	return err;
}


/* -----------------------------------------------------------------------------
	Schedule a sound channel buffer for output.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CSoundServer::scheduleOutputBuffer(void)
{}


/* -----------------------------------------------------------------------------
	Start output sound channel.
	Args:		inDeviceNo
	Return:	--
----------------------------------------------------------------------------- */

void
CSoundServer::startOutput(int inDeviceNo)
{
	if (!gSoundDriver->outputIsRunning() && !allOutputChannelsEmpty())
	{
		fOutputRequest->msg.abort();
		fOutputRequest->req.fValue = 0;
#if 0
		ArrayIndex bufIndex = fillDMABuffer();
		ArrayIndex bufSize = fD0[bufIndex];
		if (bufSize > 0)
		{
			gSoundDriver->powerOutputOn(inDeviceNo);
			gSoundDriver->scheduleOutputBuffer(bufIndex, bufSize);
			if (!gSoundDriver->outputIsEnabled())
			{
				if (gSoundDriver->startOutput())	// ie if error?
				{
					bufIndex = fillDMABuffer();
					gSoundDriver->scheduleOutputBuffer(bufIndex, fD0[bufIndex]);
				}
			}
		}
#endif
	}
}


/* -----------------------------------------------------------------------------
	Stop output sound channels.
	Args:		inDeviceNo
	Return:	--
----------------------------------------------------------------------------- */

void
CSoundServer::stopOutput(int inDeviceNo)
{
	for (CSoundChannel * channel = fOutputChannels; channel != NULL; channel = channel->next())
		channel->stop(NULL, kSndErrNotPlayed);

	if (inDeviceNo)
	{
		gSoundDriver->stopOutput();
		gSoundDriver->powerOutputOff();
	}
}


/* -----------------------------------------------------------------------------
	Open a decompressor sound channel.
	Args:		outChannelId		id of channel opened
				inForChannelId		id of output channel requiring decompression
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CSoundServer::openDecompressorChannel(ULong * outChannelId, ULong inForChannelId)
{
	NewtonErr err = kSndErrBadConfiguration;
	XTRY
	{
		// sanity check
		XFAIL(gSoundDriver->classInfo()->getCapability("SoundOutput") == NULL)
		*outChannelId = 0;
		// create codec channel
		SoundDriverInfo info;
		gSoundDriver->getSoundHardwareInfo(&info);
		CCodecChannel * channel = new CCodecChannel(uniqueId(), info);
		XFAILIF(channel == NULL, err = MemError();)
		channel->fFlags |= 0x20;
		// set target channel info
		channel->f200 = inForChannelId;
		channel->f204 = findChannel(inForChannelId);
		// thread channel onto head of list
		channel->fNext = fDecompressorChannels;
		fDecompressorChannels = channel;
		// finally set return id
		*outChannelId = channel->fId;
		err = noErr;
	}
	XENDTRY;
	return err;
}


/* -----------------------------------------------------------------------------
	Start a decompressor output sound channel.
	Nothing to do here.
	Args:		inDeviceNo
	Return:	--
----------------------------------------------------------------------------- */

void
CSoundServer::startDecompressor(int inDeviceNo)
{ /* this really does nothing */ }


/* -----------------------------------------------------------------------------
	Stop a decompressor output sound channel.
	Args:		inDeviceNo
	Return:	--
----------------------------------------------------------------------------- */

void
CSoundServer::stopDecompressor(int inDeviceNo)
{
	for (CSoundChannel * channel = fDecompressorChannels; channel != NULL; channel = channel->next())
		channel->stop(NULL, kSndErrNotPlayed);
}


int
CSoundServer::soundOutputIH(void)
{ return 0; }


void
CSoundServer::stopAll(void)
{
	stopCompressor(1);
	stopDecompressor(1);
	stopOutput(1);
	stopInput(1);
}


#pragma mark DMA

void
CSoundServer::fillDMABuffer(void)
{}

void
CSoundServer::emptyDMABuffer(int)
{}


#pragma mark -
/*------------------------------------------------------------------------------
	C S o u n d S e r v e r H a n d l e r
	Listen for user sound requests; when received, dispatch to server.
------------------------------------------------------------------------------*/

CSoundServerHandler::CSoundServerHandler()
	:	fServer(NULL)
{ }


NewtonErr
CSoundServerHandler::init(CSoundServer * inServer)
{
	fServer = inServer;
	return CEventHandler::init(kUserSndId);
}


void
CSoundServerHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	CUSoundNodeRequest * request = (CUSoundNodeRequest *)inEvent;

	if (request->fSelector == kSndScheduleOutput)
	{
		Swap(&fServer->fOutputRequest->req.fChannel, 0);
		fServer->scheduleOutputBuffer();
	}

	else if (request->fSelector == kSndScheduleInput)
	{
		int x = Swap(&fServer->fInputRequest->req.fChannel, 0);
		fServer->scheduleInputBuffer(x);
	}

	else if (request->fSelector == -1)
		fServer->eventTerminateLoop();

	else
	{
		NewtonErr			err;
		CUSoundNodeReply	reply;
		size_t				replySize = sizeof(CUSoundReply);	// we usually don’t reply with the extra node info
		reply.fChannel = request->fChannel;

		switch (request->fSelector)
		{
		case kSndStop:
			fServer->stopAll();
			deferReply();
			return;
		case kSndOpenOutputChannel:
			err = fServer->openOutputChannel(&reply.fChannel, request->fValue);
			break;
		case kSndOpenInputChannel:
			err = fServer->openInputChannel(&reply.fChannel, request->fValue);
			break;
		case kSndOpenCompressorChannel:
			err = fServer->openCompressorChannel(&reply.fChannel, request->fValue);
			break;
		case kSndOpenDecompressorChannel:
			err = fServer->openDecompressorChannel(&reply.fChannel, request->fValue);
			break;
		case kSndCloseChannel:
			err = fServer->closeChannel(request->fChannel);
			break;
		case kSndStartChannel:
			err = fServer->startChannel(request->fChannel, NULL);
			break;
		case kSndStartChannelSync:
			err = fServer->startChannel(request->fChannel, inToken);
			if (err == noErr)
			{
				reply.fError = err;
				deferReply();
				return;
			}
			break;
		case kSndPauseChannel:
			err = fServer->pauseChannel(request->fChannel, &reply);
			break;
		case kSndStopChannel:
			err = fServer->stopChannel(request->fChannel, &reply);
			break;
		case kSndScheduleNode:
			err = fServer->scheduleNode(request, inToken);
			if (err == noErr)
			{
				reply.fError = err;
				deferReply();
				return;
			}
			reply.f14 = request->fValue;
			reply.f18 = 0;
			replySize = sizeof(CUSoundNodeReply);	// this is the one time we need that extra node info
			break;
		case kSndCancelNode:
			err = fServer->cancelNode(request);
			break;
		case kSndSetInputGain:
			err = fServer->setInputVolume(request->fValue);
			break;
		case kSndSetInputDevice:
			err = fServer->setInputDevice(request->fChannel, request->fChannel);
			break;
		case kSndSetOutputDevice:
			err = fServer->setOutputDevice(request->fChannel, request->fChannel);
			break;
		case kSndSetVolume:
			// Watch that float does not get silently converted to int!
			err = fServer->setOutputVolume(*(float *)&request->fValue);
			break;
		case kSndGetVolume:
			// Watch that float does not get silently converted to int!
			*(float *)&reply.fValue = gSoundDriver->outputVolume();
			err = noErr;
			break;
		default:
			err = kSndErrInvalidMessage;
		}
		reply.fError = err;
		setReply(replySize, &reply);
		replyImmed();
	}
}


#pragma mark -
/*------------------------------------------------------------------------------
	C S o u n d P o w e r H a n d l e r
	Listen for power-off system messages; when received, stop sound output.
------------------------------------------------------------------------------*/

CSoundPowerHandler::CSoundPowerHandler()
	:	fServer(NULL)
{ }


NewtonErr
CSoundPowerHandler::init(CSoundServer * inServer)
{
	fServer = inServer;
	return CSystemEventHandler::init(kSysEvent_PowerOff);
}


void
CSoundPowerHandler::powerOff(CEvent * inEvent)
{
	fServer->stopAll();
}

