/*
	File:		SoundDriver.cc

	Contains:	Sound driver implementation.

	Written by:	Newton Research Group, 2010.
*/

#include "SoundDriver.h"
#include "SoundErrors.h"
#include "NewtonGestalt.h"
#include "Power.h"


/*------------------------------------------------------------------------------
	P M a i n S o u n d D r i v e r
	The original implements PCirrusSoundDriver; there was no PMainSoundDriver.
------------------------------------------------------------------------------*/

PROTOCOL PMainSoundDriver : public PSoundDriver
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PMainSoundDriver)
	CAPABILITIES( "SoundOutput" "" )

	PSoundDriver *	make(void);
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


/* ----------------------------------------------------------------
	PMainSoundDriver implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
PMainSoundDriver::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN16PMainSoundDriver6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN16PMainSoundDriver4makeEv - 0b	\n"
"		.long		__ZN16PMainSoundDriver7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"PMainSoundDriver\"	\n"
"2:	.asciz	\"PSoundDriver\"	\n"
"3:	.asciz	\"SoundOutput\", \"\" \n"
"		.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN16PMainSoundDriver9classInfoEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver4makeEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver7destroyEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver20setSoundHardwareInfoEPK15SoundDriverInfo - 4b	\n"
"		.long		__ZN16PMainSoundDriver20getSoundHardwareInfoEP15SoundDriverInfo - 4b	\n"
"		.long		__ZN16PMainSoundDriver16setOutputBuffersEmmmm - 4b	\n"
"		.long		__ZN16PMainSoundDriver15setInputBuffersEmmmm - 4b	\n"
"		.long		__ZN16PMainSoundDriver20scheduleOutputBufferEmm - 4b	\n"
"		.long		__ZN16PMainSoundDriver19scheduleInputBufferEmm - 4b	\n"
"		.long		__ZN16PMainSoundDriver13powerOutputOnEi - 4b	\n"
"		.long		__ZN16PMainSoundDriver14powerOutputOffEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver12powerInputOnEi - 4b	\n"
"		.long		__ZN16PMainSoundDriver13powerInputOffEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver11startOutputEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver10startInputEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver10stopOutputEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver9stopInputEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver15outputIsEnabledEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver14inputIsEnabledEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver15outputIsRunningEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver14inputIsRunningEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver16currentOutputPtrEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver15currentInputPtrEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver12outputVolumeEf - 4b	\n"
"		.long		__ZN16PMainSoundDriver12outputVolumeEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver11inputVolumeEi - 4b	\n"
"		.long		__ZN16PMainSoundDriver11inputVolumeEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver20enableExtSoundSourceEi - 4b	\n"
"		.long		__ZN16PMainSoundDriver21disableExtSoundSourceEi - 4b	\n"
"		.long		__ZN16PMainSoundDriver16outputIntHandlerEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver15inputIntHandlerEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver26outputIntHandlerDispatcherEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver25inputIntHandlerDispatcherEv - 4b	\n"
"		.long		__ZN16PMainSoundDriver21setOutputCallbackProcEPFlPvES0_ - 4b	\n"
"		.long		__ZN16PMainSoundDriver20setInputCallbackProcEPFlPvES0_ - 4b	\n"
CLASSINFO_END
);
}

#pragma mark -
/* -----------------------------------------------------------------------------
	G e s t a l t   I n f o
----------------------------------------------------------------------------- */

struct GestaltSoundHWInfo
{
//size +24
};

struct GestaltVolumeInfo
{
	bool		x00;
	bool		x01;
	bool		x02;
	bool		x03;
	double	volumedB;
	int32_t	numOfSteps;		// number of detents on volume slider
	int32_t	x10;
//size +14
};

// would normally be built by PCirrusSoundDriver; see SoundDriver.h
GestaltVolumeInfo gFakeVolumeInfo = { 0, 0, 0, 0, -6.0206, 16, 1 };


PROTOCOL_IMPL_SOURCE_MACRO(PMainSoundDriver)

PSoundDriver *
PMainSoundDriver::make(void)
{
		// need to register fake kGestalt_Ext_VolumeInfo gestalt, done by PCirrusSoundDriver::new() like this:
		CUGestalt gestalt;

//		GestaltSoundHWInfo hwInfo;
//		gestalt.gestalt(kGestalt_Ext_SoundHWInfo, &hwInfo, sizeof(hwInfo));
//...
//		configureOutputValues(true);

		gestalt.registerGestalt(kGestalt_Ext_VolumeInfo, &gFakeVolumeInfo, sizeof(GestaltVolumeInfo));
//...

	return this;
}

void
PMainSoundDriver::destroy(void)
{}


NewtonErr
PMainSoundDriver::setSoundHardwareInfo(const SoundDriverInfo * info)
{
	return kSndErrBadConfiguration;
}


NewtonErr
PMainSoundDriver::getSoundHardwareInfo(SoundDriverInfo * outInfo)
{
	outInfo->f00 = 1;
	outInfo->f04 = 1;
	outInfo->f08 = 1;
//	outInfo->f0C = ? 0x54600000 : 0x38400000 : 0x1C200000; == (Fixed) 21600 14400 7200
	outInfo->f10 = 6;
	outInfo->f14 = 16;	// |
	outInfo->f18 = 1;		// | saved as a pair

	return noErr;
}


void
PMainSoundDriver::setOutputBuffers(VAddr inBuf1, size_t inBuf1Size, VAddr inBuf2, size_t inBuf2Size)
{}

void
PMainSoundDriver::setInputBuffers(VAddr inBuf1, size_t inBuf1Size, VAddr inBuf2, size_t inBuf2Size)
{}

void
PMainSoundDriver::scheduleOutputBuffer(VAddr inBuf, size_t inBufSize)
{}

void
PMainSoundDriver::scheduleInputBuffer(VAddr inBuf, size_t inBufSize)
{}

void
PMainSoundDriver::powerOutputOn(int)
{}

void
PMainSoundDriver::powerOutputOff(void)
{
	stopOutput();
	IOPowerOff(27);
}

void
PMainSoundDriver::powerInputOn(int)
{
//	IOPowerOn(24);
//	IOPowerOn(26);
//	Wait(32);
}

void
PMainSoundDriver::powerInputOff(void)
{
	stopInput();
	IOPowerOff(26);
	IOPowerOff(24);
}

int
PMainSoundDriver::startOutput(void)
{ return 1; }

void
PMainSoundDriver::startInput(void)
{}

void
PMainSoundDriver::stopOutput(void)
{}

void
PMainSoundDriver::stopInput(void)
{}

bool
PMainSoundDriver::outputIsEnabled(void)
{ return true; }

bool
PMainSoundDriver::inputIsEnabled(void)
{ return true; }

bool
PMainSoundDriver::outputIsRunning(void)
{ return true; }

bool
PMainSoundDriver::inputIsRunning(void)
{ return true; }

void
PMainSoundDriver::currentOutputPtr(void)
{}

void
PMainSoundDriver::currentInputPtr(void)
{}

void
PMainSoundDriver::outputVolume(float)
{}

float
PMainSoundDriver::outputVolume(void)
{ return -6.0206; }

void
PMainSoundDriver::inputVolume(int)
{}

int
PMainSoundDriver::inputVolume(void)
{ return 0; }	// original uses byte for input gain

void
PMainSoundDriver::enableExtSoundSource(int)
{}

void
PMainSoundDriver::disableExtSoundSource(int)
{}

void
PMainSoundDriver::outputIntHandler(void)
{}

void
PMainSoundDriver::inputIntHandler(void)
{}


void
PMainSoundDriver::outputIntHandlerDispatcher(void)
{}

void
PMainSoundDriver::inputIntHandlerDispatcher(void)
{}

void
PMainSoundDriver::setOutputCallbackProc(SoundDriverCallbackProcPtr, void*)
{}

void
PMainSoundDriver::setInputCallbackProc(SoundDriverCallbackProcPtr, void*)
{}


#pragma mark -
/*------------------------------------------------------------------------------
	Register our sound driver.
------------------------------------------------------------------------------*/

void
RegisterSoundHardwareDriver(void)
{
	PMainSoundDriver::classInfo()->registerProtocol();	// original registers PCirrusSoundDriver
}

