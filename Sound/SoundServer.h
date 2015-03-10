/*
	File:		SoundServer.h

	Contains:	Sound server definitions.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__SOUNDSERVER_H)
#define __SOUNDSERVER_H 1

#include "SoundCodec.h"
#include "AppWorld.h"
#include <math.h>

extern CUPort *	gSoundPort;

#define kUserSndId 					'usnd'

// sound server function selectors
enum
{
	kSndStop = 1,
	kSndScheduleOutput = 4,
	kSndScheduleInput,		// 5
	kSndOpenOutputChannel,
	kSndOpenInputChannel,
	kSndCloseChannel,
	kSndStartChannel,
	kSndStartChannelSync,	// 10
	kSndPauseChannel,
	kSndStopChannel,
	kSndScheduleNode,
	kSndCancelNode,
	kSndSetVolume,			// 15
	kSndSetInputGain,
	kSndSetInputDevice,
	kSndSetOutputDevice,
	kSndOpenCompressorChannel,
	kSndOpenDecompressorChannel,	// 20
	kSndGetVolume
};

/*--------------------------------------------------------------------------------
	S o u n d B l o c k
--------------------------------------------------------------------------------*/
class CSoundCodec;

struct SoundBlock
{
						SoundBlock();

	void *			data;					// +00
	size_t			dataSize;			// +04
	int				dataType;			// +08
	int				comprType;			// +0C
	float				sampleRate;			// +10
	float				volume;				// +14
	ULong				start;				// +18
	ULong				count;				// +1C
	ULong				loops;				// +20
	RefStruct *		frame;				// +24
	CSoundCodec *	codec;				// +28
	size_t			bufferSize;			// +2C
	size_t			bufferCount;		// +30
};

inline	SoundBlock::SoundBlock()
	:	data(0),
		dataSize(0),
		dataType(k8Bit),
		comprType(kSampleStandard),
		sampleRate(22026.43),
		volume(INFINITY),
		start(0),
		count(0),
		loops(0),
		frame(NULL)
{ }


/*--------------------------------------------------------------------------------
	C U S o u n d R e q u e s t
--------------------------------------------------------------------------------*/

class CUSoundRequest : public CEvent
{
public:
					CUSoundRequest();
					CUSoundRequest(ULong inChannel, int inSelector, ULong inValue);

	ULong			fChannel;	// +08
	int			fSelector;	// +0C
	ULong			fValue;		// +10
};

inline	CUSoundRequest::CUSoundRequest()
	:	CEvent(kUserSndId), fChannel(0), fSelector(0), fValue(0)
{ }

inline	CUSoundRequest::CUSoundRequest(ULong inChannel, int inSelector, ULong inValue)
	:	CEvent(kUserSndId), fChannel(inChannel), fSelector(inSelector), fValue(inValue)
{ }


struct SoundRequest
{
	CUAsyncMessage	msg;
	CUSoundRequest	req;
};


class CUSoundNodeRequest : public CUSoundRequest
{
public:
					CUSoundNodeRequest();

	SoundBlock	fSound;		// +14
	float			fVolume;		// +48
};

inline	CUSoundNodeRequest::CUSoundNodeRequest()
	:	CUSoundRequest(0,0,0), fVolume(INFINITY)
{ }



/*--------------------------------------------------------------------------------
	C U S o u n d R e p l y
--------------------------------------------------------------------------------*/

class CUSoundReply : public CEvent
{
public:
					CUSoundReply();

	ULong			fChannel;	// +08
	NewtonErr	fError;		// +0C
	ULong			fValue;		// +10
};

inline	CUSoundReply::CUSoundReply()
	:	CEvent(kUserSndId), fChannel(0), fError(noErr), fValue(0)
{ }


class CUSoundNodeReply : public CUSoundReply
{
public:
					CUSoundNodeReply();

	ULong			f14;
	ULong			f18;
	ULong			f1C;	// samples actually played
};

inline	CUSoundNodeReply::CUSoundNodeReply()
	:	f14(0), f18(0), f1C(0)
{ }


/*------------------------------------------------------------------------------
	C S o u n d C h a n n e l
	Kernel-mode sound channel.
------------------------------------------------------------------------------*/
struct ChannelNode;

class CSoundChannel
{
public:
					CSoundChannel(ULong);
					~CSoundChannel();

	void			start(CUMsgToken * inToken);
	void			pause(CUSoundNodeReply * ioReply);
	void			stop(CUSoundNodeReply * ioReply, NewtonErr inErr);

	void			schedule(CUSoundNodeRequest * inRequest, CUMsgToken * inToken);
	void			cancel(CUSoundNodeRequest * inRequest);

	void			makeNode(ChannelNode ** outNode);
	void			cleanupNode(ChannelNode * inNode);
	void			freeNode(ChannelNode * inNode, long, int);
};


/*------------------------------------------------------------------------------
	C S o u n d S e r v e r H a n d l e r
	Listen for user sound requests; when received, dispatch to server.
------------------------------------------------------------------------------*/
class CSoundServer;

class CSoundServerHandler : public CEventHandler
{
public:
						CSoundServerHandler();

	NewtonErr		init(CSoundServer * inServer);
	virtual void	eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);

private:
	CSoundServer *	fServer;
};


/*------------------------------------------------------------------------------
	C S o u n d P o w e r H a n d l e r
	Listen for power-off system messages; when received, stop sound output.
------------------------------------------------------------------------------*/

class CSoundPowerHandler : public CSystemEventHandler
{
public:
						CSoundPowerHandler();

	NewtonErr		init(CSoundServer * inServer);
	virtual void	powerOff(CEvent * inEvent);

private:
	CSoundServer *	fServer;
};


/*------------------------------------------------------------------------------
	C S o u n d S e r v e r
------------------------------------------------------------------------------*/

class CSoundServer : public CAppWorld
{
public:
				CSoundServer();
	virtual	~CSoundServer();

	virtual size_t		getSizeOf(void) const;
	virtual NewtonErr	mainConstructor(void);
	virtual void		mainDestructor(void);
	virtual void		theMain(void);

	NewtonErr	setInputDevice(ULong, int);
	NewtonErr	setOutputDevice(ULong, int);
	NewtonErr	setInputVolume(int);
	NewtonErr	setOutputVolume(int);
	NewtonErr	startChannel(ULong, CUMsgToken*);
	NewtonErr	pauseChannel(ULong, CUSoundNodeReply*);
	NewtonErr	stopChannel(ULong, CUSoundNodeReply*);
	NewtonErr	closeChannel(ULong);
	CSoundChannel *	findChannel(ULong);

	NewtonErr	scheduleNode(CUSoundNodeRequest*, CUMsgToken*);
	NewtonErr	cancelNode(CUSoundNodeRequest*);

	void			scheduleInputBuffer(int);
	NewtonErr	openInputChannel(ULong*, ULong);
	NewtonErr	openCompressorChannel(ULong*, ULong);
	void			startCompressor(int);
	void			stopCompressor(int);
	void			startInput(int);
	void			stopInput(int);
	int			soundInputIH(void);

	void			scheduleOutputBuffer(void);
	void			prepOutputChannels(void);
	NewtonErr	openOutputChannel(ULong*, ULong);
	NewtonErr	openDecompressorChannel(ULong*, ULong);
	void			startDecompressor(int);
	void			stopDecompressor(int);
	void			startOutput(int);
	void			stopOutput(int);
	int			soundOutputIH(void);

	NewtonErr	stopAll(void);

	void			fillDMABuffer(void);
	void			emptyDMABuffer(int);

	bool			allInputChannelsEmpty(void);
	bool			allOutputChannelsEmpty(void);

	ULong			uniqueId(void);

private:
	friend class CSoundServerHandler;

	SoundRequest *			fOutputRequest;		// +70
	SoundRequest *			fInputRequest;			// +74
	int32_t					f78;
	CSoundServerHandler	fSoundEventHandler;	// +7C
	CSoundPowerHandler	fPowerEventHandler;	// +94
	StackLimits				fB0;
	Ptr						fB8;
	int32_t					fBC;
	int32_t					fC0;
	int32_t					fC4;
	Ptr						fC8;	// output buffers
	Ptr						fCC;
	int32_t					fD0;
	int32_t					fD4;
	int32_t					fD8;
	int32_t					fDC;
	Ptr						fE0;	// input buffers
	Ptr						fE4;
	int32_t					fE8;
	int32_t					fEC;
	int32_t					fF0;
	int32_t					fF4;
	int32_t					fF8;
	int32_t					fFC;
// size +0104
};


#endif	/* __SOUNDSERVER_H */
