/*
	File:		SoundServer.cc

	Contains:	Sound server implementation.

	Written by:	Newton Research Group, 2007.
*/

// DON’T USE MAC MEMORY FUNCTIONS
#define __MACMEMORY__ 1
#define __QDTYPES_H 1

#include <AudioUnit/AUComponent.h>
#include <AudioUnit/AudioOutputUnit.h>
#include <AudioUnit/AudioUnitProperties.h>
#include <AudioUnit/AudioUnitParameters.h>
#include <AudioUnit/AudioCodec.h>

#include "SoundServer.h"
#include "SoundDriver.h"
#include "SoundErrors.h"
//#include "VirtualMemory.h"
#include "DTMFCodec.h"


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

CUPort *			gSoundPort = NULL;			// 0C101B10	was gSndPort
CSoundDriver *	gSoundDriver = NULL;		// 0C101B14	was gSndDriver
ULong				gMaxFilterNodes;			// 0C101B18
ULong				gNumFilterNodes;			// 0C101B1C


/* -----------------------------------------------------------------------------
	I n i t i a l i z a t i o n
----------------------------------------------------------------------------- */

void
InitializeSound(void)
{
//	IOPowerOff(25);
//	IOPowerOff(24);
	RegisterSoundHardwareDriver();

	CSoundServer soundServer;
	soundServer.init('sndm', YES, kSpawnedTaskStackSize, kSoundTaskPriority, kNoId);

//	CMuLawCodec::classInfo()->registerProtocol();
//	CIMACodec::classInfo()->registerProtocol();
//	CGSMCodec::classInfo()->registerProtocol();
	CDTMFCodec::classInfo()->registerProtocol();

	gMaxFilterNodes = 6;		// was (gMainCPUType == 3) ? 6 : 0;
}


#pragma mark -
/*------------------------------------------------------------------------------
	A u d i o   U n i t
	We use the AudioUnit in
		CSoundServer::start() and
		CSoundServer::scheduleNode()
------------------------------------------------------------------------------*/

AudioUnit	gOutputUnit;

OSStatus
MyRenderCallback(	void * inRefCon,
						AudioUnitRenderActionFlags * ioActionFlags,
						const AudioTimeStamp * inTimeStamp,
						UInt32 inBusNumber,
						UInt32 inNumberFrames,
						AudioBufferList * ioData )
{
	size_t			amtReqd = ioData->mBuffers[0].mDataByteSize;
	size_t			amtDone;
	size_t			dataSize;
	CodecBlock		codecParms;
	CSoundCodec *	codec = (CSoundCodec *)inRefCon;

	amtDone = ioData->mBuffers[0].mDataByteSize;
	codec->produce(ioData->mBuffers[0].mData, &amtDone, &dataSize, &codecParms);
	ioData->mBuffers[0].mDataByteSize = (UInt32)amtDone;

	if (amtDone < amtReqd)
	{
		// zero the remainder of the buffer
		memset((char *)ioData->mBuffers[0].mData + amtDone, 0, amtReqd - amtDone);
	}
	else if (amtDone == 0)
		*ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;

	return noErr;
}


OSStatus
SimpleRenderCallback(void * inRefCon,
							AudioUnitRenderActionFlags * ioActionFlags,
							const AudioTimeStamp * inTimeStamp,
							UInt32 inBusNumber,
							UInt32 inNumberFrames,
							AudioBufferList * ioData )
{
	CodecBlock *	codecParms = (CodecBlock *)inRefCon;

	unsigned		amtReqd = ioData->mBuffers[0].mDataByteSize;
	unsigned		amtDone = MIN(amtReqd, codecParms->dataSize - codecParms->x00);

	memcpy(ioData->mBuffers[0].mData, (char *)codecParms->data + codecParms->x00, amtDone);
	if (amtDone < amtReqd)
	{
		// zero the remainder of the buffer
		memset((char *)ioData->mBuffers[0].mData + amtDone, 0, amtReqd - amtDone);
	}
	else if (amtDone == 0)
		*ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;

	codecParms->x00 += amtDone;

	return noErr;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C S o u n d S e r v e r
----------------------------------------------------------------------------- */

CSoundServer::CSoundServer()
{
	fOutputRequest = NULL;
	fInputRequest = NULL;
	f78 = 0;
	fB8 = NULL;
	fBC = 0;
	fC0 = 0;
	fC4 = 0;
	fC8 = NULL;
	fCC = NULL;
	fD0 = 0;
	fD4 = 0;
	fD8 = 0;
	fDC = 0;
	fE0 = NULL;
	fE4 = NULL;
	fE8 = 0;
	fEC = 0;
	fF0 = 0;
	fF4 = 0;
	fF8 = 0;
	fFC = 0;
}


CSoundServer::~CSoundServer()
{ }


size_t
CSoundServer::getSizeOf(void) const
{ return sizeof(CSoundServer); }


NewtonErr
CSoundServer::mainConstructor(void)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = CAppWorld::mainConstructor())
#if 0
		gSoundDriver = (CSoundDriver *)MakeByName("CSoundDriver", "CMainSoundDriver");
		if (gSoundDriver == NULL)
			gSoundDriver = (CSoundDriver *)MakeByName("CSoundDriver", "CCirrusSoundDriver");
		XFAILNOT(gSoundDriver, err = kSndErrGeneric;)
		LockPtr((Ptr)gSoundDriver);

		XFAILNOT(fB8 = NewPtr(0x0EA0), err = MemError();)
		XFAILNOT(fC8 = NewWiredPtr(0x0EA0), err = MemError();)
		XFAILNOT(fCC = NewWiredPtr(0x0EA0), err = MemError();)
		gSoundDriver->setOutputBuffers((VAddr)fC8, 0x0EA0, (VAddr)fCC, 0x0EA0);
		gSoundDriver->setOutputCallbackProc(MemberFunctionCast(SoundCallbackProcPtr, this, &CSoundServer::soundOutputIH), this);
		if (gSoundDriver->classInfo()->getCapability("SoundInput"))
		{
			XFAILNOT(fE0 = NewWiredPtr(0x0EA0), err = MemError();)
			XFAILNOT(fE4 = NewWiredPtr(0x0EA0), err = MemError();)
			gSoundDriver->setInputBuffers((VAddr)fE0, 0x0EA0, (VAddr)fE4, 0x0EA0);
		}
		gSoundDriver->setInputCallbackProc(MemberFunctionCast(SoundCallbackProcPtr, this, &CSoundServer::soundInputIH), this);

		gSoundDriver->powerOutputOff();
		gSoundDriver->powerInputOff();
#endif
		XFAIL(err = fSoundEventHandler.init(this))
		XFAIL(err = fPowerEventHandler.init(this))

		gSoundPort = getMyPort();
		XFAILNOT(fOutputRequest = new SoundRequest, err = MemError();)
		XFAIL(err = fOutputRequest->msg.init(NO))
		fOutputRequest->req.fSelector = kSndScheduleOutput;
		XFAILNOT(fInputRequest = new SoundRequest, err = MemError();)
		XFAIL(err = fInputRequest->msg.init(NO))
		fInputRequest->req.fSelector = kSndScheduleInput;
	}
	XENDTRY;
	return err;
}


void
CSoundServer::mainDestructor(void)
{
	stopAll();
	// INCOMPLETE
	CAppWorld::mainDestructor();
}


void
CSoundServer::theMain(void)
{
//	LockStack(&fB0, 2000);
	CAppWorld::theMain();
//	UnlockStack(&fB0);
}


NewtonErr
CSoundServer::setInputDevice(ULong inChannelId, int inDeviceNo)
{
//	findChannel(inChannelId)->f2C =  inDeviceNo;
	return noErr;
}


NewtonErr
CSoundServer::setOutputDevice(ULong inChannelId, int inDeviceNo)
{
//	findChannel(inChannelId)->f2C =  inDeviceNo;
	return noErr;
}

NewtonErr
CSoundServer::setInputVolume(int)
{ return noErr; }

NewtonErr
CSoundServer::setOutputVolume(int)
{ return noErr; }

CSoundChannel *
CSoundServer::findChannel(ULong inChannelId)
{ return NULL; }


NewtonErr
CSoundServer::startChannel(ULong inChannelId, CUMsgToken * inToken)
{
#if 1
	AudioOutputUnitStart(gOutputUnit);
#endif
	return noErr;
}

NewtonErr
CSoundServer::pauseChannel(ULong inChannelId, CUSoundNodeReply * ioReply)
{ return noErr; }

NewtonErr
CSoundServer::stopChannel(ULong inChannelId, CUSoundNodeReply * ioReply)
{
#if 1
	AudioOutputUnitStop(gOutputUnit);
#endif
	return noErr;
}

NewtonErr
CSoundServer::closeChannel(ULong inChannelId)
{ return noErr; }


CodecBlock		codecParms;

NewtonErr
CSoundServer::scheduleNode(CUSoundNodeRequest * inRequest, CUMsgToken * inToken)
{
#if defined(correct)
	CSoundChannel * chanl;
	if ((chanl = findChannel(inRequest->fChannel)) == NULL)
		return kSndErrChannelIsClosed;
	return chanl->schedule(inRequest, inToken);

#else
	NewtonErr err;
	XTRY
	{
	//	set up CSoundCodec; will pass it to renderer
		codecParms.data = inRequest->fSound.data;
		codecParms.dataSize = inRequest->fSound.dataSize;
		codecParms.dataType = k16Bit;
		codecParms.comprType = kSampleLinear;
		codecParms.sampleRate = inRequest->fSound.sampleRate;

		int bytesPerSample;
		if (inRequest->fSound.codec == NULL)
		{
			// we have a simpleSound
			bytesPerSample = 1;
		}
		else
		{
			bytesPerSample = 2;
			inRequest->fSound.codec->reset(&codecParms);
		}

		// Open the default output unit
		AudioComponent comp;
		AudioComponentDescription desc;
		desc.componentType = kAudioUnitType_Output;
		desc.componentSubType = kAudioUnitSubType_DefaultOutput;
		desc.componentManufacturer = kAudioUnitManufacturer_Apple;
		desc.componentFlags = 0;
		desc.componentFlagsMask = 0;
		XFAILNOT(comp = AudioComponentFindNext(NULL, &desc), err = -1;)
		XFAIL(err = AudioComponentInstanceNew(comp, &gOutputUnit))

		// Set output unit’s stream format
		AudioStreamBasicDescription format;
		format.mSampleRate = inRequest->fSound.sampleRate;
		format.mFormatID = kAudioFormatLinearPCM;
		format.mFormatFlags = (kAudioFormatFlagIsSignedInteger \
							/*		| kAudioFormatFlagIsBigEndian \		*/
									| kAudioFormatFlagIsPacked \
									| kAudioFormatFlagIsNonInterleaved);
		format.mBytesPerPacket = bytesPerSample;
		format.mFramesPerPacket = 1;
		format.mBytesPerFrame = bytesPerSample;
		format.mChannelsPerFrame = 1;
		format.mBitsPerChannel = 8 * bytesPerSample;
		XFAIL(err = AudioUnitSetProperty(gOutputUnit,
							kAudioUnitProperty_StreamFormat,
							kAudioUnitScope_Input,
							0,
							&format,
							sizeof(AudioStreamBasicDescription)))

		// Initialize unit
		XFAIL(err = AudioUnitInitialize(gOutputUnit))

		// Set up a callback function to generate input for the output unit
		AURenderCallbackStruct renderer;
		if (inRequest->fSound.codec == NULL)
		{
			renderer.inputProc = SimpleRenderCallback;
			renderer.inputProcRefCon = &codecParms;
			codecParms.x00 = 0;
		}
		else
		{
			renderer.inputProc = MyRenderCallback;
			renderer.inputProcRefCon = inRequest->fSound.codec;
		}
		XFAIL(err = AudioUnitSetProperty(gOutputUnit,
							kAudioUnitProperty_SetRenderCallback, 
							kAudioUnitScope_Input,
							0, 
							&renderer, 
							sizeof(AURenderCallbackStruct)))

	}
	XENDTRY;
	return err;
#endif
}


NewtonErr
CSoundServer::cancelNode(CUSoundNodeRequest*)
{ return noErr; }

void
CSoundServer::scheduleInputBuffer(int)
{}

NewtonErr
CSoundServer::openInputChannel(ULong * outChannelId, ULong)
{
	*outChannelId = 1;
	return noErr;
}

NewtonErr
CSoundServer::openCompressorChannel(ULong * outChannelId, ULong)
{
	*outChannelId = 1;
	return noErr;
}

void
CSoundServer::startCompressor(int)
{ /* this really does nothing */ }

void
CSoundServer::stopCompressor(int)
{}

void
CSoundServer::startInput(int)
{}

void
CSoundServer::stopInput(int)
{}

int
CSoundServer::soundInputIH(void)
{ return 0; }

void
CSoundServer::scheduleOutputBuffer(void)
{}

void
CSoundServer::prepOutputChannels(void)
{}

NewtonErr
CSoundServer::openOutputChannel(ULong * outChannelId, ULong)
{
	*outChannelId = 1;
	return noErr;
}

NewtonErr
CSoundServer::openDecompressorChannel(ULong * outChannelId, ULong)
{
	*outChannelId = 1;
	return noErr;
}

void
CSoundServer::startDecompressor(int)
{}

void
CSoundServer::stopDecompressor(int)
{}

void
CSoundServer::startOutput(int)
{}

void
CSoundServer::stopOutput(int)
{}

int
CSoundServer::soundOutputIH(void)
{ return 0; }

NewtonErr
CSoundServer::stopAll(void)
{ return noErr; }

void
CSoundServer::fillDMABuffer(void)
{}

void
CSoundServer::emptyDMABuffer(int)
{}

bool
CSoundServer::allInputChannelsEmpty(void)
{ return YES; }

bool
CSoundServer::allOutputChannelsEmpty(void)
{ return YES; }

ULong
CSoundServer::uniqueId(void)
{ return 0; }


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
			err = fServer->stopAll();
			break;
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
			err = fServer->setOutputVolume(request->fValue);
			break;
		case kSndGetVolume:
//			reply.fValue = gSoundDriver->outputVolume();
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
	return CSystemEventHandler::init('pwof');
}


void
CSoundPowerHandler::powerOff(CEvent * inEvent)
{
	fServer->stopAll();
}

