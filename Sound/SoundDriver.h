/*
	File:		SoundDriver.h

	Contains:	Sound driver interface.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__SOUNDDRIVER_H)
#define __SOUNDDRIVER_H 1

#include "Protocols.h"

typedef long (*SoundDriverCallbackProcPtr)(void*);

struct SoundDriverInfo
{
	ULong f00; //  = 00000001
	ULong f04; //  = 00000001
	ULong f08; //  = 00000001
	ULong f0C; //  = 54600000
	ULong f10; //  = 00000006
	ULong f14; //  = 00000010
	ULong	f18; //  = 00000001
};


/*------------------------------------------------------------------------------
	P S o u n d D r i v e r
	P-class interface.
------------------------------------------------------------------------------*/

PROTOCOL PSoundDriver : public CProtocol
{
public:
	static PSoundDriver *	make(const char * inName);
	void			destroy(void);

	NewtonErr	setSoundHardwareInfo(const SoundDriverInfo * info);
	NewtonErr	getSoundHardwareInfo(SoundDriverInfo * outInfo);
	void			setOutputBuffers(VAddr inBuf1, size_t inBuf1Size, VAddr inBuf2, size_t inBuf2Size);
	void			setInputBuffers(VAddr inBuf1, size_t inBuf1Size, VAddr inBuf2, size_t inBuf2Size);
	void			scheduleOutputBuffer(VAddr inBuf, size_t inBufSize);
	void			scheduleInputBuffer(VAddr inBuf, size_t inBufSize);
	void			powerOutputOn(int);
	void			powerOutputOff(void);
	void			powerInputOn(int);
	void			powerInputOff(void);
	int			startOutput(void);
	void			startInput(void);
	void			stopOutput(void);
	void			stopInput(void);
	bool			outputIsEnabled(void);
	bool			inputIsEnabled(void);
	bool			outputIsRunning(void);
	bool			inputIsRunning(void);
	void			currentOutputPtr(void);
	void			currentInputPtr(void);
	void			outputVolume(float);
	float			outputVolume(void);
	void			inputVolume(int);
	int			inputVolume(void);
	void			enableExtSoundSource(int);
	void			disableExtSoundSource(int);
	void			outputIntHandler(void);
	void			inputIntHandler(void);

	NONVIRTUAL void			outputIntHandlerDispatcher(void);
	NONVIRTUAL void			inputIntHandlerDispatcher(void);
	NONVIRTUAL void			setOutputCallbackProc(SoundDriverCallbackProcPtr, void*);
	NONVIRTUAL void			setInputCallbackProc(SoundDriverCallbackProcPtr, void*);
};


extern void		RegisterSoundHardwareDriver(void);


#endif	/* __SOUNDDRIVER_H */
