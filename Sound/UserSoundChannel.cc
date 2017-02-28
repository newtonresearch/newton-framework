/*
	File:		SoundChannel.cc

	Contains:	Sound channel implementation.
					We render sound using the AudioUnit framework.

	Written by:	Newton Research Group, 2007.
*/

#include "UserSoundChannel.h"
#include "SoundErrors.h"
#include "Globals.h"
#include "ROMResources.h"
#include "Lookup.h"
#include "NewtonScript.h"
#include "Unicode.h"

extern	  Ref		ConvertToSoundFrame(RefArg inSound);
extern	float		VolumeToDecibels(int inVolume);
extern "C" Ref		FStrEqual(RefArg inRcvr, RefArg inStr1, RefArg inStr2);

/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

CFrameSoundChannel *	gSoundChannel = NULL;


/*------------------------------------------------------------------------------
	G l o b a l S o u n d C h a n n e l
------------------------------------------------------------------------------*/

CFrameSoundChannel *
GlobalSoundChannel(void)
{
	if (gSoundChannel == NULL)
	{
		NewtonErr	err;
		gSoundChannel = new CFrameSoundChannel;
		if ((err = MemError()) != noErr)
			ThrowErr(exFrames, err);

		int		outputDeviceNumber = kSoundDefaultDevice;
		RefVar	outputDevice(GetFrameSlot(gVarFrame, SYMA(userConfiguration)));
		outputDevice = GetProtoVariable(outputDevice, SYMA(outputDevice), NULL);
		if (ISINT(outputDevice))
			outputDeviceNumber = RINT(outputDevice);
		if ((err = gSoundChannel->open(0, outputDeviceNumber)) != noErr)
		{
			delete gSoundChannel;
			gSoundChannel = NULL;
			ThrowErr(exFrames, err);
		}
	}
	return gSoundChannel;
}


NewtonErr
SafeCodecInit(CSoundCodec * inCodec, CodecBlock * inParms)
{
	NewtonErr	err = noErr;

	newton_try
	{
		err = inCodec->init(inParms);
	}
	newton_catch_all
	{
		err = kSndErrGeneric;
	}
	end_try;

	return err;
}


NewtonErr
SafeCodecDelete(CSoundCodec * inCodec)
{
	NewtonErr	err = noErr;

	newton_try
	{
		inCodec->destroy();
	}
	newton_catch_all
	{
		err = kSndErrGeneric;
	}
	end_try;

	return err;
}


void
ConvertCodecBlock(SoundBlock * inSound, CodecBlock * outCodec)
{
	outCodec->x00 = 0;
	outCodec->data = inSound->data;
	outCodec->dataSize = inSound->dataSize;
	outCodec->dataType = inSound->dataType;
	outCodec->comprType = inSound->comprType;
	outCodec->sampleRate = inSound->sampleRate;
	outCodec->frame = inSound->frame;
}


void
ConvertCodecBlock(CodecBlock * inCodec, SoundBlock * outSound)
{
	if (inCodec->x00 >= 0)
	{
		outSound->data = inCodec->data;
		outSound->dataSize = inCodec->dataSize;
		outSound->dataType = inCodec->dataType;
		outSound->comprType = inCodec->comprType;
		outSound->sampleRate = inCodec->sampleRate;
		outSound->frame = inCodec->frame;
	}
}

#pragma mark -

/*------------------------------------------------------------------------------
	C U S o u n d C a l l b a c k
------------------------------------------------------------------------------*/

CUSoundCallback::CUSoundCallback()
{ }

CUSoundCallback::~CUSoundCallback()
{ }



CUSoundCallbackProc::CUSoundCallbackProc()
	:	fCallback(NULL)
{ }

CUSoundCallbackProc::~CUSoundCallbackProc()
{ }

void
CUSoundCallbackProc::setCallback(SoundCallbackProcPtr inCallback)
{
	fCallback = inCallback;
}

void
CUSoundCallbackProc::complete(SoundBlock * inSound, int inState, NewtonErr inResult)
{
	if (fCallback != NULL)
		fCallback(inSound, inState, inResult);
}

#pragma mark -

/*------------------------------------------------------------------------------
	C U S o u n d C h a n n e l
------------------------------------------------------------------------------*/

CUSoundChannel::CUSoundChannel()
{
	fState = 0;
	fChannelId = 0;
	fCodecChannelId = 0;
	fUniqueId = 0;
	fActiveNodes = NULL;
	fRecycledNodes = NULL;
	fVolume = INFINITY;	// huh? in decibels, surely, => 0
	fGain = 128;
	f34 = 0;
	f38 = 0;

#if defined(correct)
	GestaltVolumeInfo		volumeInfo;
	CUGestalt				gestalt;

	gestalt.gestalt(kGestalt_Ext_VolumeInfo, &gestaltInfo, sizeof(gestaltInfo));
	if (volumeInfo.x02)
		fState |= 0x80;
#endif
}


CUSoundChannel::~CUSoundChannel()
{
	close();

	for (SoundNode * node = fRecycledNodes, * next; node != NULL; node = next)
	{
		next = node->next;
		delete node;
	}
}


NewtonErr
CUSoundChannel::open(int inInputDevice, int inOutputDevice)
{
	NewtonErr err;
	XTRY
	{
		init(kUserSndId);	// CEventHandler base class function

		XFAILNOT(gSoundPort, err = kSndErrGeneric;)

		int	channelSelector, codecSelector;
		CUSoundReply	reply;
		if (inInputDevice == 0)
		{
			fState |= kSoundInactive;
			channelSelector = kSndOpenOutputChannel;
			codecSelector = kSndOpenDecompressorChannel;
		}
		else
		{
			fState |= kSoundRecording;
			channelSelector = kSndOpenInputChannel;
			codecSelector = kSndOpenCompressorChannel;
		}
	
		XFAIL(err = sendImmediate(channelSelector, 0, inOutputDevice, &reply, sizeof(reply)))
		XFAIL(err = reply.fError)
		fChannelId = reply.fChannel;

		XFAIL(err = sendImmediate(codecSelector, 0, fChannelId, &reply, sizeof(reply)))
		XFAILIF(err = reply.fError, close();)	// no codec required for this channel
		fCodecChannelId = reply.fChannel;
	}
	XENDTRY;
	return err;
}


NewtonErr
CUSoundChannel::close(void)
{
	NewtonErr err;
	XTRY
	{
		NewtonErr err1 = noErr, err2 = noErr;
		CUSoundReply	reply;
		if (fCodecChannelId != 0)
			err1 = sendImmediate(kSndCloseChannel, fCodecChannelId, 0, &reply, sizeof(reply));
		if (fChannelId != 0)
			err2 = sendImmediate(kSndCloseChannel, fChannelId, 0, &reply, sizeof(reply));
		XFAIL((err = err1) != noErr || (err = err2) != noErr)

		err = reply.fError;
		fChannelId = 0;
		fCodecChannelId = 0;
		abortBusy();
	}
	XENDTRY;
	return err;
}


NewtonErr
CUSoundChannel::schedule(SoundBlock * inSound, CUSoundCallback * inCallback)
{
	NewtonErr	err;
	SoundNode *	theNode = NULL;

	XTRY
	{
		if (fVolume == -INFINITY && (fState & 0x80) != 0)
			fVolume = getVolume();
		ULong	channelId = (inSound->codec == NULL) ? fChannelId : fCodecChannelId;
		XFAILNOT(inSound->comprType == kSampleStandard || inSound->comprType == kSampleLinear || inSound->comprType == kSampleMuLaw, err = kSndErrBadConfiguration;)
		XFAILIF(channelId == 0, err = kSndErrChannelIsClosed;)

		XFAIL(err = makeNode(&theNode))
		theNode->cb = inCallback;
		theNode->request.fChannel = channelId;
		theNode->request.fSelector = kSndScheduleNode;
		theNode->request.fValue = theNode->id;
		theNode->request.fSound = *inSound;
		theNode->request.fVolume = fVolume;

		int	offset = inSound->start;
		int	count = inSound->count;
		int	length = inSound->dataSize;
		if (offset < 0 || offset > length)
			offset = 0;
		if (count <= 0 || offset + count > length)
			count = length - offset;
		theNode->request.fSound.start = offset;
		theNode->request.fSound.count = count;
		theNode->request.fSound.data = (char *)inSound->data + (offset * inSound->dataType) / 8;
#if !defined(forFramework)
		XFAIL(err = gSoundPort->sendRPC(&theNode->msg, &theNode->request, sizeof(CUSoundNodeRequest), &theNode->reply, sizeof(CUSoundNodeReply)))
#endif
		theNode->next = fActiveNodes;
		fActiveNodes = theNode;
	}
	XENDTRY;
	XDOFAIL(err)
	{
		if (theNode != NULL)
			freeNode(theNode);
	}
	XENDFAIL;

	return err;
}


NewtonErr
CUSoundChannel::cancel(ULong inRefCon)
{
	NewtonErr	err;
	XTRY
	{
		XFAILNOT(fChannelId, err = kSndErrChannelIsClosed;)

		SoundNode * node = findRefCon(inRefCon);
		XFAILNOT(node, err = kSndErrNoSamples;)

		CUSoundReply	reply;
		err = sendImmediate(kSndCancelNode, fChannelId, node->id, &reply, sizeof(reply));
		if (err == noErr)
			err = reply.fError;
	}
	XENDTRY;
	return err;
}


NewtonErr
CUSoundChannel::start(bool inAsync)
{
	NewtonErr	err;
	XTRY
	{
		XFAILNOT(fCodecChannelId, err = kSndErrChannelIsClosed;)

		CUSoundReply	reply;
		err = sendImmediate(inAsync ? kSndStartChannel : kSndStartChannelSync, fCodecChannelId, 0, &reply, sizeof(reply));
		if (err == noErr)
			err = reply.fError;
		XFAIL(err)
		fState &= ~kSoundPaused;
		if (inAsync)
			fState |= kSoundPlaying;
	}
	XENDTRY;
	return err;
}


NewtonErr
CUSoundChannel::pause(SoundBlock * outSound, long * outIndex)
{
	NewtonErr	err;
	XTRY
	{
		if (outIndex != NULL)
			*outIndex = -1;

		XFAILNOT(fChannelId, err = kSndErrChannelIsClosed;)

		if ((fState & kSoundPlayPaused) != 0)
			err = start(true);
		else
		{
			CUSoundNodeReply	reply;
			err = sendImmediate(kSndPauseChannel, fCodecChannelId, 0, &reply, sizeof(reply));
			if (err == noErr)
				err = reply.fError;
			XFAIL(err)

			fState |= kSoundPlayPaused;
			SoundNode * node = findNode(reply.f14);
			if (node != NULL)
			{
				if (outSound != NULL)
					*outSound = node->request.fSound;
				if (outIndex != NULL)
					*outIndex = node->request.fSound.start + reply.f1C;
			}
		}
	}
	XENDTRY;
	return err;
}


NewtonErr
CUSoundChannel::stop(SoundBlock * outSound, long * outIndex)
{
	NewtonErr	err = noErr;
	XTRY
	{
		if (outIndex != NULL)
			*outIndex = -1;

		XFAILNOT(fChannelId, err = kSndErrChannelIsClosed;)

		if ((fState & kSoundPlaying) != 0)
		{
			CUSoundNodeReply	reply;
			err = sendImmediate(kSndStopChannel, fCodecChannelId, 0, &reply, sizeof(reply));
			if (err == noErr)
				err = reply.fError;
			XFAIL(err)

			fState &= ~(kSoundPlaying|kSoundPlayPaused);
			SoundNode * node = findNode(reply.f14);
			if (node != NULL)
			{
				if (outSound != NULL)
					*outSound = node->request.fSound;
				if (outIndex != NULL)
					*outIndex = node->request.fSound.start + reply.f1C;
			}
			abortBusy();
		}
	}
	XENDTRY;
	return err;
}


void
CUSoundChannel::abortBusy(void)
{
	for (SoundNode * node = fActiveNodes, * next; node != NULL; node = next)
	{
		CUSoundCallback * callback;
#if !defined(forFramework)
		node->msg.abort();
#endif
		next = node->next;
		if ((callback = node->cb) != NULL)
			callback->complete(&node->request.fSound, kSoundAborted, kSndErrCancelled);
		freeNode(node);
	}
	fActiveNodes = NULL;
}


void
CUSoundChannel::setOutputDevice(int inDevice)
{
	f38 = inDevice;
	if (fChannelId != 0)
	{
		CUSoundReply	reply;
		sendImmediate(kSndSetOutputDevice, fChannelId, inDevice, &reply, sizeof(reply));
	}
}


/* -----------------------------------------------------------------------------
	Watch that float does not get silently converted to int!
----------------------------------------------------------------------------- */

float
CUSoundChannel::setVolume(float inDecibels)
{
	fVolume = inDecibels;
	if (fActiveNodes != NULL || this == gSoundChannel)
	{
		CUSoundReply reply;
		if (sendImmediate(kSndSetVolume, 0, *(ULong *)&inDecibels, &reply, sizeof(reply)) == noErr)
			inDecibels = *(float *)&reply.fValue;
	}
	return inDecibels;
}


float
CUSoundChannel::getVolume(void)
{
	float	db = fVolume;
	if ((fActiveNodes != NULL || this == gSoundChannel)
	&&  (fState & 0x80) != 0)
	{
		CUSoundReply reply;
		if (sendImmediate(kSndGetVolume, 0, 0, &reply, sizeof(reply)) == noErr)
			db = *(float *)&reply.fValue;
	}
	return db;
}


void
CUSoundChannel::setInputGain(int inGain)
{
	fGain = inGain;

	CUSoundReply	reply;
	sendImmediate(kSndSetInputGain, fChannelId, inGain, &reply, sizeof(reply));
}


int
CUSoundChannel::getInputGain(void)
{
	return fGain;
}


NewtonErr
CUSoundChannel::sendImmediate(int inSelector, ULong inChannel, ULong inValue, CUSoundReply * ioReply, size_t inReplySize)
{
	size_t	size;
	CUSoundRequest	request(inChannel, inSelector, inValue);
	return gSoundPort->sendRPC(&size, &request, sizeof(request), ioReply, inReplySize);
}


void
CUSoundChannel::eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	for (SoundNode * node = fActiveNodes, * prev = NULL; node != NULL; prev = node, node = node->next)
	{
		// look for node
		if (node->id == ((CUSoundNodeReply *)inEvent)->f14)
		{
			CUSoundCallback * callback;
			// remove node from active list
			if (prev != NULL)
				prev->next = node->next;
			else
				fActiveNodes = node->next;
			// call back
			if ((callback = node->cb) != NULL)
				callback->complete(&node->request.fSound, ((CUSoundNodeReply *)inEvent)->fError, ((CUSoundNodeReply *)inEvent)->fError);	// sic; surely state == kSoundCompleted?
			// free node resources
			freeNode(node);
			break;
		}
	}
}


NewtonErr
CUSoundChannel::makeNode(SoundNode ** outNode)
{
	NewtonErr	err = noErr;
	SoundNode *	theNode;
	if ((theNode = fRecycledNodes) != NULL)
	{
		fRecycledNodes = fRecycledNodes->next;
	}
	else
	{
		XTRY
		{
			theNode = new SoundNode;
			XFAILIF(theNode == NULL, err = MemError();)
#if !defined(forFramework)
			XFAIL(err = theNode->msg.init(true))
			XFAIL(err = theNode->msg.setCollectorPort(*gAppWorld->getMyPort()))
			err = theNode->msg.setUserRefCon((OpaqueRef)this);
#endif
		}
		XENDTRY;
	}
	theNode->id = uniqueId();
	theNode->next = NULL;
	theNode->cb = NULL;
	*outNode = theNode;
	return err;
}


SoundNode *
CUSoundChannel::findNode(ULong inId)
{
	SoundNode * node;
	for (node = fActiveNodes; node != NULL; node = node->next)
		if (node->id == inId)
			break;
	return node;
}


SoundNode *
CUSoundChannel::findRefCon(OpaqueRef inRefCon)
{
	SoundNode * node;
	for (node = fActiveNodes; node != NULL; node = node->next)
		if ((OpaqueRef)node->request.fSound.frame == inRefCon)	// x44
			break;
	return node;
}


void
CUSoundChannel::freeNode(SoundNode * inNode)
{
	unsigned count = 0;
	SoundNode * node;
	for (node = fRecycledNodes; node != NULL; node = node->next)
		count++;
	if (count < 2)
	{
		inNode->next = fRecycledNodes;
		fRecycledNodes = inNode;
	}
	else if (inNode != NULL)
		delete inNode;
	if (fActiveNodes == NULL)
		fState &= ~(kSoundPlaying|kSoundPlayPaused);
}


ULong
CUSoundChannel::uniqueId(void)
{
	do {
		if (++fUniqueId == 0) fUniqueId = 1;
	} while (findNode(fUniqueId) != NULL);
	return fUniqueId;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C F r a m e S o u n d C a l l b a c k
------------------------------------------------------------------------------*/

CFrameSoundCallback::~CFrameSoundCallback()
{ }

void
CFrameSoundCallback::complete(SoundBlock * inSound, int inState, NewtonErr inResult)
{
	RefVar context(*inSound->frame);
	delete inSound->frame;
	UnlockRef(GetProtoVariable(context, SYMA(samples), NULL));
	fChannel->deleteCodec(inSound);
	newton_try
	{
		RefVar callback(GetProtoVariable(context, SYMA(callback), NULL));
		if (NOTNIL(callback))
			NSSendProto(context, SYMA(callback), MAKEINT(inState), MAKEINT(inResult));
	}
	newton_catch_all
	{ }
	end_try;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C F r a m e S o u n d C h a n n e l
------------------------------------------------------------------------------*/

CFrameSoundChannel::CFrameSoundChannel()
	:	fCodec(NULL), fIsCodecReady(false), fCallback(this)
{ }


CFrameSoundChannel::~CFrameSoundChannel()
{ }


NewtonErr
CFrameSoundChannel::open(int inInputDevice, int inOutputDevice)
{
	NewtonErr	err;

	err = CUSoundChannel::open(inInputDevice, inOutputDevice);

	if (IsString(fCodecName))
	{
		char	codecName[128];
		ConvertFromUnicode(GetUString(fCodecName), codecName);
		if (codecName[0] == 'T') codecName[0] = 'C';
		fCodec = (CSoundCodec *)MakeByName("CSoundCodec", codecName);
	}

	return err;
}


void
CFrameSoundChannel::close(void)
{
	CUSoundChannel::close();

	if (fCodec != NULL)
	{
		SafeCodecDelete(fCodec);
		fCodec = NULL;
		fIsCodecReady = false;
	}
}


NewtonErr
CFrameSoundChannel::schedule(RefArg inSound)
{
	SoundBlock	snd;
	convert(inSound, &snd);
	return CUSoundChannel::schedule(&snd, &fCallback);
}


NewtonErr
CFrameSoundChannel::convert(RefArg inSound, SoundBlock * outParms)
{
	NewtonErr	err;
	RefVar		item;
	RefVar		sndFrame(ConvertToSoundFrame(inSound));

	outParms->frame = NULL;
	outParms->codec = NULL;

	XTRY
	{
		item = GetProtoVariable(sndFrame, SYMA(sndFrameType), NULL);
		XFAILNOT(EQ(item, SYMA(codec)) || EQ(item, SYMA(simpleSound)), err = kSndErrBadConfiguration;)

		if (EQ(item, SYMA(codec)))
			openCodec(sndFrame, outParms);

		item = GetProtoVariable(sndFrame, SYMA(compressionType), NULL);
		outParms->comprType = NOTNIL(item) ? RINT(item) : kSampleStandard;
		XFAILNOT(outParms->comprType == kSampleStandard || outParms->comprType == kSampleLinear || outParms->comprType == kSampleMuLaw, err = kSndErrBadConfiguration;)

		int	dataType;
		item = GetProtoVariable(sndFrame, SYMA(dataType), NULL);
		if (NOTNIL(item))
		{
			dataType = RINT(item);
			if (dataType == 8 || dataType == 1)
				dataType = k8Bit;
			else if (dataType == 16 || dataType == 2)
				dataType = k16Bit;
			else
				XFAIL(err = kSndErrBadConfiguration)
		}
		else
			dataType = (outParms->comprType == kSampleStandard) ? k8Bit : k16Bit;
		outParms->dataType = dataType;

		item = GetProtoVariable(sndFrame, SYMA(samples), NULL);
		LockRef(item);
		outParms->data = BinaryData(item);
		if (outParms->codec != NULL)
			outParms->dataSize = Length(item);
		else
			outParms->dataSize = Length(item) / (dataType/8);	// for simpleSound, dataSize is number of samples

		item = GetProtoVariable(sndFrame, SYMA(Length), NULL);
		if (NOTNIL(item))
			outParms->dataSize = RINT(item);

		item = GetProtoVariable(sndFrame, SYMA(samplingRate), NULL);	// can be int, Fixed or double
		if (IsReal(item))
			outParms->sampleRate = CDouble(item);
		else if (ISINT(item))
			outParms->sampleRate = (float)RINT(item);
		else if (IsBinary(item)) {
#if defined(hasByteSwapping)
			// assume sound is imported from NTK and is therefore BIG-ENDIAN
			// it’s only binary so we can’t detect it and fix it at init
			Fixed rate = *(Fixed *)BinaryData(item);
			rate = BYTE_SWAP_LONG(rate);
			outParms->sampleRate = rate / (Fixed)0x00010000;
#else
			outParms->sampleRate = *(Fixed *)BinaryData(item) / (Fixed)0x00010000;	// FixedToFloat
#endif
		} else
			outParms->sampleRate = 22026.43;
		XFAILIF(outParms->sampleRate < 0, err = kSndErrBadConfiguration;)

		outParms->frame = new RefStruct(sndFrame);
		XFAIL(err = MemError())

		item = GetProtoVariable(sndFrame, SYMA(start), NULL);
		outParms->start = ISINT(item) ? RINT(item) : 0;

		item = GetProtoVariable(sndFrame, SYMA(count), NULL);
		outParms->count = ISINT(item) ? RINT(item) : outParms->dataSize;

		item = GetProtoVariable(sndFrame, SYMA(loops), NULL);
		outParms->loops = ISINT(item) ? RINT(item) : 0;

		item = GetProtoVariable(sndFrame, SYMA(volume), NULL);	// can be int or double
		if (ISINT(item))
			outParms->volume = VolumeToDecibels(RINT(item));
		else if (IsReal(item))
			outParms->volume = CDouble(item);
		else
			outParms->volume = INFINITY;

		err = initCodec(outParms);
	}
	XENDTRY;

	XDOFAIL(err)
	{
		deleteCodec(outParms);
		if (outParms->frame != NULL)
		{
			RefVar	samples(GetProtoVariable(sndFrame, SYMA(samples), NULL));
			UnlockRef(samples);
			delete outParms->frame;
		}
		ThrowErr(exFrames, err);
	}
	XENDFAIL;

	return noErr;
}


NewtonErr
CFrameSoundChannel::openCodec(RefArg inSound, SoundBlock * outParms)
{
	NewtonErr	err = noErr;

	XTRY
	{
		RefVar	item;
		item = GetProtoVariable(inSound, SYMA(bufferSize), NULL);
		outParms->bufferSize = ISINT(item) ? RINT(item) : 0;
		item = GetProtoVariable(inSound, SYMA(bufferCount), NULL);
		outParms->bufferCount = ISINT(item) ? RINT(item) : 0;
		item = GetProtoVariable(inSound, SYMA(codecName), NULL);
		XFAILNOT(IsString(item), err = kNSErrNotAString;)
		if (IsString(fCodecName) && NOTNIL(FStrEqual(RA(NILREF), item, fCodecName)))
			outParms->codec = fCodec;
		else
		{
			char	codecName[128];
			ConvertFromUnicode(GetUString(item), codecName);
			if (codecName[0] == 'T') codecName[0] = 'C';
			outParms->codec = (CSoundCodec *)MakeByName("CSoundCodec", codecName);
			XFAILNOT(outParms->codec, if ((err = MemError()) == noErr) err = kSndErrBadConfiguration;)
		}
	}
	XENDTRY;

	return err;
}


NewtonErr
CFrameSoundChannel::initCodec(SoundBlock * ioParms)
{
	NewtonErr err = noErr;
	XTRY
	{
		if (ioParms->codec != NULL)
		{
			CodecBlock	codecParms;
			bool isThisCodec = (ioParms->codec == fCodec);
			ConvertCodecBlock(ioParms, &codecParms);
			if (isThisCodec && fIsCodecReady)
			{
				codecParms.dataType = fDataType;
				codecParms.comprType = fComprType;
				codecParms.sampleRate = fSampleRate;
			}
			else
			{
				XFAIL(err = SafeCodecInit(ioParms->codec, &codecParms))
				if (isThisCodec)
				{
					fCodecParms = codecParms;
					fIsCodecReady = true;
				}
			}
			ConvertCodecBlock(&codecParms, ioParms);
		}
	}
	XENDTRY;

	return err;	
}


NewtonErr
CFrameSoundChannel::deleteCodec(SoundBlock * inParms)
{
	NewtonErr	err = noErr;
	if (inParms->codec != fCodec)
		err = SafeCodecDelete(inParms->codec);
	inParms->codec = NULL;
	return err;
}

