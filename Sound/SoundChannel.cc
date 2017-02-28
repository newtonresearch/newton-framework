/*
	File:		SoundChannel.cc

	Contains:	Sound channel stuff.

	Written by:	Newton Research Group, 2015.
*/

#include <AudioUnit/AUComponent.h>
#include <AudioUnit/AudioOutputUnit.h>
#include <AudioUnit/AudioUnitProperties.h>
#include <AudioUnit/AudioUnitParameters.h>
#include <AudioUnit/AudioCodec.h>

#include "SoundServer.h"
#include "SoundErrors.h"

/* -----------------------------------------------------------------------------
	A u d i o   U n i t
----------------------------------------------------------------------------- */

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
/*------------------------------------------------------------------------------
	C S o u n d C h a n n e l
	Kernel-mode sound channel.
------------------------------------------------------------------------------*/
CodecBlock codecParms;


CSoundChannel::CSoundChannel(ULong inChannelId)
	:	fId(inChannelId), fNext(NULL), f0C(0), fFlags(0), fDevice(0)
{ }


CSoundChannel::~CSoundChannel()
{ }


NewtonErr
CSoundChannel::schedule(CUSoundNodeRequest * inRequest, CUMsgToken * inToken)
{
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
}


NewtonErr
CSoundChannel::cancel(CUSoundNodeRequest * inRequest)
{ return noErr; }



NewtonErr
CSoundChannel::start(CUMsgToken * inToken)
{
	AudioOutputUnitStart(gOutputUnit);
	return noErr;

#if defined(correct)
	if (f0C == 0)
		return kSndErrNoSamples;

	if (!FLAGTEST(fFlags, 0x04))
	{
		FLAGSET(fFlags, 0x04);
		if (inToken != NULL)
		{
			fMsgToken = *inToken;
			f18 = 1;
			f30 = true;
		}
	}
	if (FLAGTEST(fFlags, 0x08))
		FLAGCLEAR(fFlags, 0x08);
	return noErr;
#endif
}


void
CSoundChannel::pause(CUSoundNodeReply * ioReply)
{}


void
CSoundChannel::stop(CUSoundNodeReply * ioReply, NewtonErr inErr)
{
	AudioOutputUnitStop(gOutputUnit);
}


void
CSoundChannel::makeNode(ChannelNode ** outNode)
{}


void
CSoundChannel::freeNode(ChannelNode * inNode, long, int)
{}


void
CSoundChannel::cleanupNode(ChannelNode * inNode)
{}


#pragma mark -
/* -------------------------------------------------------------------------------
	C D M A C h a n n e l
------------------------------------------------------------------------------- */

CDMAChannel::CDMAChannel(ULong inChannelId, SoundDriverInfo& info)
	:	CSoundChannel(inChannelId), f200(), f204(NULL)
{ }


#pragma mark -
/* -------------------------------------------------------------------------------
	C C o d e c C h a n n e l
------------------------------------------------------------------------------- */

CCodecChannel::CCodecChannel(ULong inChannelId, SoundDriverInfo& info)
	:	CSoundChannel(inChannelId), f200(), f204(NULL)
{ }
