/*
	File:		SoundDriver.cc

	Contains:	Sound driver implementation.

	Written by:	Newton Research Group, 2010.
*/

#include "SoundDriver.h"

/*------------------------------------------------------------------------------
	C M a i n S o u n d D r i v e r
	The original implements PCirrusSoundDriver; there was no PMainSoundDriver.
------------------------------------------------------------------------------*/

PROTOCOL CMainSoundDriver : public CSoundDriver
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CMainSoundDriver)

	CSoundDriver *	make(void);
	void			destroy(void);

	void			setSoundHardwareInfo(const SoundDriverInfo * info);
	void			getSoundHardwareInfo(SoundDriverInfo * outInfo);
	void			setOutputBuffers(VAddr inBuf1, size_t inBuf1Size, VAddr inBuf2, size_t inBuf2Size);
	void			setInputBuffers(VAddr inBuf1, size_t inBuf1Size, VAddr inBuf2, size_t inBuf2Size);
	void			scheduleOutputBuffer(VAddr inBuf, size_t inBufSize);
	void			scheduleInputBuffer(VAddr inBuf, size_t inBufSize);
	void			powerOutputOn(long);
	void			powerOutputOff(void);
	void			powerInputOn(long);
	void			powerInputOff(void);
	void			startOutput(void);
	void			startInput(void);
	void			stopOutput(void);
	void			stopInput(void);
	bool			outputIsEnabled(void);
	bool			inputIsEnabled(void);
	bool			outputIsRunning(void);
	bool			inputIsRunning(void);
	void			currentOutputPtr(void);
	void			currentInputPtr(void);
	void			outputVolume(long);
	void			outputVolume(void);
	void			inputVolume(long);
	void			inputVolume(void);
	void			enableExtSoundSource(long);
	void			disableExtSoundSource(long);
	void			outputIntHandler(void);
	void			inputIntHandler(void);

	NONVIRTUAL void			outputIntHandlerDispatcher(void);
	NONVIRTUAL void			inputIntHandlerDispatcher(void);
	NONVIRTUAL void			setOutputCallbackProc(SoundCallbackProcPtr, void*);
	NONVIRTUAL void			setInputCallbackProc(SoundCallbackProcPtr, void*);
};


PROTOCOL_IMPL_SOURCE_MACRO(CMainSoundDriver)

CSoundDriver *
CMainSoundDriver::make(void)
{ return this;}

void
CMainSoundDriver::destroy(void)
{}

void
CMainSoundDriver::setSoundHardwareInfo(const SoundDriverInfo * info)
{}

void
CMainSoundDriver::getSoundHardwareInfo(SoundDriverInfo * outInfo)
{}

void
CMainSoundDriver::setOutputBuffers(VAddr inBuf1, size_t inBuf1Size, VAddr inBuf2, size_t inBuf2Size)
{}

void
CMainSoundDriver::setInputBuffers(VAddr inBuf1, size_t inBuf1Size, VAddr inBuf2, size_t inBuf2Size)
{}

void
CMainSoundDriver::scheduleOutputBuffer(VAddr inBuf, size_t inBufSize)
{}

void
CMainSoundDriver::scheduleInputBuffer(VAddr inBuf, size_t inBufSize)
{}

void
CMainSoundDriver::powerOutputOn(long)
{}

void
CMainSoundDriver::powerOutputOff(void)
{}

void
CMainSoundDriver::powerInputOn(long)
{}

void
CMainSoundDriver::powerInputOff(void)
{}

void
CMainSoundDriver::startOutput(void)
{}

void
CMainSoundDriver::startInput(void)
{}

void
CMainSoundDriver::stopOutput(void)
{}

void
CMainSoundDriver::stopInput(void)
{}

bool
CMainSoundDriver::outputIsEnabled(void)
{ return YES; }

bool
CMainSoundDriver::inputIsEnabled(void)
{ return YES; }

bool
CMainSoundDriver::outputIsRunning(void)
{ return YES; }

bool
CMainSoundDriver::inputIsRunning(void)
{ return YES; }

void
CMainSoundDriver::currentOutputPtr(void)
{}

void
CMainSoundDriver::currentInputPtr(void)
{}

void
CMainSoundDriver::outputVolume(long)
{}

void
CMainSoundDriver::outputVolume(void)
{}

void
CMainSoundDriver::inputVolume(long)
{}

void
CMainSoundDriver::inputVolume(void)
{}

void
CMainSoundDriver::enableExtSoundSource(long)
{}

void
CMainSoundDriver::disableExtSoundSource(long)
{}

void
CMainSoundDriver::outputIntHandler(void)
{}

void
CMainSoundDriver::inputIntHandler(void)
{}


void
CMainSoundDriver::outputIntHandlerDispatcher(void)
{}

void
CMainSoundDriver::inputIntHandlerDispatcher(void)
{}

void
CMainSoundDriver::setOutputCallbackProc(SoundCallbackProcPtr, void*)
{}

void
CMainSoundDriver::setInputCallbackProc(SoundCallbackProcPtr, void*)
{}


#pragma mark -
/*------------------------------------------------------------------------------
	Register our sound driver.
------------------------------------------------------------------------------*/

void
RegisterSoundHardwareDriver(void)
{
//	CMainSoundDriver::classInfo()->registerProtocol();	// original registers PCirrusSoundDriver
}

