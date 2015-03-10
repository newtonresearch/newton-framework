/*
	File:		SoundChannel.h

	Contains:	User-mode sound channel definitions.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__SOUNDCHANNEL_H)
#define __SOUNDCHANNEL_H 1

#include "SoundServer.h"


/*------------------------------------------------------------------------------
	C U S o u n d C a l l b a c k
------------------------------------------------------------------------------*/

class CUSoundCallback
{
public:

					CUSoundCallback();
	virtual		~CUSoundCallback();

	virtual void	complete(SoundBlock * inSound, int inState, NewtonErr inResult) = 0;
};


typedef void (*SoundCallbackProcPtr)(SoundBlock*, int, long);

class CUSoundCallbackProc : public CUSoundCallback
{
public:
					CUSoundCallbackProc();
	virtual		~CUSoundCallbackProc();

	void				setCallback(SoundCallbackProcPtr);
	virtual void	complete(SoundBlock * inSound, int inState, NewtonErr inResult);

private:
	SoundCallbackProcPtr		fCallback;
};


/*--------------------------------------------------------------------------------
	S o u n d N o d e
--------------------------------------------------------------------------------*/

struct SoundNode
{
	SoundNode *				next;		// +00
	ULong						id;		// +04
	CUSoundCallback *		cb;		// +08
	CUSoundNodeRequest	request;	// +0C
	CUSoundNodeReply		reply;	// +58
#if !defined(forFramework)
	CUAsyncMessage			msg;		// +78
#endif
};


/*------------------------------------------------------------------------------
	C U S o u n d C h a n n e l
------------------------------------------------------------------------------*/

class CUSoundChannel : public CEventHandler
{
public:
					CUSoundChannel();
	virtual		~CUSoundChannel();

	NewtonErr	open(int inInputDevice, int inOutputDevice);
	NewtonErr	close(void);
	NewtonErr	schedule(SoundBlock * inSound, CUSoundCallback * inCallback);
	NewtonErr	cancel(ULong inRefCon);
	NewtonErr	start(bool inAsync);
	NewtonErr	pause(SoundBlock * outSound, long * outIndex);
	NewtonErr	stop(SoundBlock * outSound, long * outIndex);
	void			abortBusy(void);

	void			setOutputDevice(int inDevice);

	float			setVolume(float inDecibels);
	float			getVolume(void);

	void			setInputGain(int inGain);
	int			getInputGain(void);

	NewtonErr		sendImmediate(int inSelector, ULong inChannel, ULong inValue, CUSoundReply * ioReply, size_t inReplySize);
	virtual void	eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);

	NewtonErr		makeNode(SoundNode ** ouNode);
	SoundNode *		findNode(ULong inId);
	SoundNode *		findRefCon(OpaqueRef inRefCon);
	void				freeNode(SoundNode * inNode);

	ULong				uniqueId(void);

protected:
	ULong						fState;				// +14
	ULong						f18;					// +18	id of channel that plays uncompressed 8-bit samples
	ULong						f1C;					// +1C	id of channel that plays thru a codec
	ULong						fUniqueId;			// +20
	SoundNode *				fActiveNodes;		// +24
	SoundNode *				fRecycledNodes;	// +28
	float						fVolume;				// +2C	in decibels
	int						fGain;				// +30	input gain, 0..255
	long						f34;
	long						f38;
};

// state flags (protoRecorderView state constants, NPG2.1 7-36)
enum
{
	kSoundInactive			= 1,
	kSoundRecording		= 1 << 1,
	kSoundPlaying			= 1 << 2,
	kSoundPlayPaused		= 1 << 3,
	kSoundRecordPaused	= 1 << 4,
	kSoundStopping			= 1 << 5,
	kSoundSetupStore		= 1 << 6,
	kSoundMute				= 1 << 7
};


// callback states, NPG2.1 7-31)
enum
{
	kSoundCompleted,
	kSoundAborted,
	kSoundPaused
};


// device constants, NPG2.1 7-26)
enum
{
	kSoundDefaultDevice,
	kSoundInternalSpeaker	= 1,
	kSoundInternalMic			= 1 << 2,
	kSoundLineOut				= 1 << 3,
	kSoundLineIn				= 1 << 4,
};


/*------------------------------------------------------------------------------
	C F r a m e S o u n d C a l l b a c k
------------------------------------------------------------------------------*/
class CFrameSoundChannel;

class CFrameSoundCallback : public CUSoundCallback
{
public:
					CFrameSoundCallback();
					CFrameSoundCallback(CFrameSoundChannel * inContext);
	virtual		~CFrameSoundCallback();

	virtual void		complete(SoundBlock * inSound, int inState, NewtonErr inResult);

private:
	CFrameSoundChannel *		fChannel;
};

inline	CFrameSoundCallback::CFrameSoundCallback() : fChannel(NULL)  { }
inline	CFrameSoundCallback::CFrameSoundCallback(CFrameSoundChannel * inContext) : fChannel(inContext)  { }


/*------------------------------------------------------------------------------
	C F r a m e S o u n d C h a n n e l
	The sound channel to use.
------------------------------------------------------------------------------*/

class CFrameSoundChannel : public CUSoundChannel
{
public:
					CFrameSoundChannel();
	virtual		~CFrameSoundChannel();

	NewtonErr	open(int inInputDevice, int inOutputDevice);
	void			close(void);
	NewtonErr	schedule(RefArg inSound);
	NewtonErr	convert(RefArg inSound, SoundBlock * outParms);
	NewtonErr	openCodec(RefArg inCodec, SoundBlock * outParms);
	NewtonErr	initCodec(SoundBlock * inParms);
	NewtonErr	deleteCodec(SoundBlock * inParms);

protected:
	CSoundCodec *			fCodec;				// +3C
	CodecBlock				fCodecParms;		// +40
	long						fDataType;			// +4C
	int						fComprType;			// +50
	float						fSampleRate;		// +54
	bool						fIsCodecReady;		// +5C
	RefStruct				fCodecName;			// +60	doesn’t appear to be used
	CFrameSoundCallback	fCallback;			// +64
};


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern CFrameSoundChannel *	GlobalSoundChannel(void);


#endif	/* __SOUNDCHANNEL_H */
