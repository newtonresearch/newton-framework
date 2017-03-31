/*
	File:		Sound.cc

	Contains:	NewtonScript interface to sound functions.

	Written by:	Newton Research Group.
*/

#include "Sound.h"
#include "UserSoundChannel.h"
#include "Globals.h"
#include "Lookup.h"
#include "MagicPointers.h"
#include "Preference.h"
#include "ROMResources.h"
#include "cfloat"

extern Ref	SPrintObject(RefArg obj);


extern CFrameSoundChannel *	gSoundChannel;


/*------------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FVolumeToDecibels(RefArg inRcvr, RefArg inVolume);
Ref	FDecibelsToVolume(RefArg inRcvr, RefArg inDecibels);
Ref	FGetVolume(RefArg inRcvr);
Ref	FSetVolume(RefArg inRcvr, RefArg inVolume);
Ref	FGetSystemVolume(RefArg inRcvr);
Ref	FSetSystemVolume(RefArg inRcvr, RefArg inVolume);
Ref	FConvertToSoundFrame(RefArg inRcvr, RefArg inSound);
Ref	FSetInputGain(RefArg inRcvr, RefArg inGain);
Ref	FSetOutputDevice(RefArg inRcvr, RefArg inDevice);
Ref	FPlaySoundSync(RefArg inRcvr, RefArg inSound);
//Ref	FPlaySound(RefArg inRcvr, RefArg inSound);				// already declared in Sound.h
//Ref	FPlaySoundIrregardless(RefArg inRcvr, RefArg inSound);
Ref	FPlaySoundEffect(RefArg inRcvr, RefArg inSound, RefArg inArg2, RefArg inArg3);
Ref	FSoundCheck(RefArg inRcvr);
Ref	FSoundPlayEnabled(RefArg inRcvr, RefArg inSound);
}


/*------------------------------------------------------------------------------
	S o u n d
------------------------------------------------------------------------------*/

void
InitSound(void)
{ /* this really does nothing */ }


Ref
ConvertToSoundFrame(RefArg inSound)
{
	RefVar	theSound(inSound);

	if (IsString(theSound))
	{
		RefVar	sndFrame(AllocateFrame());
		SetFrameSlot(sndFrame, SYMA(_proto), RA(protoSoundFrame));
		SetFrameSlot(sndFrame, SYMA(sndFrameType), SYMA(codec));
		SetFrameSlot(sndFrame, SYMA(codecName), MakeStringFromCString("TMacintalkCodec"));
//		SetFrameSlot(sndFrame, SYMA(samples), FStripInk(RA(NILREF), Clone(theSound), RA(NILREF)));
		SetFrameSlot(sndFrame, SYMA(bufferSize), MAKEINT(5000));
		SetFrameSlot(sndFrame, SYMA(bufferCount), MAKEINT(4));
		SetFrameSlot(sndFrame, SYMA(compressionType), MAKEINT(kSampleLinear));
		SetFrameSlot(sndFrame, SYMA(dataType), MAKEINT(k16Bit));
		SetFrameSlot(sndFrame, SYMA(samplingRate), MAKEINT(21600));
		theSound = sndFrame;
	}

	else if (IsBinary(theSound))
	{
		RefVar	sndFrame(AllocateFrame());
		SetFrameSlot(sndFrame, SYMA(_proto), RA(protoSoundFrame));
		SetFrameSlot(sndFrame, SYMA(sndFrameType), SYMA(codec));
		SetFrameSlot(sndFrame, SYMA(codecName), SPrintObject(ClassOf(theSound)));
		SetFrameSlot(sndFrame, SYMA(samples), theSound);
		SetFrameSlot(sndFrame, SYMA(bufferSize), MAKEINT(5000));
		SetFrameSlot(sndFrame, SYMA(bufferCount), MAKEINT(4));
		SetFrameSlot(sndFrame, SYMA(compressionType), MAKEINT(kSampleLinear));
		SetFrameSlot(sndFrame, SYMA(dataType), MAKEINT(k16Bit));
		SetFrameSlot(sndFrame, SYMA(samplingRate), MAKEINT(21600));
		theSound = sndFrame;
	}

	return theSound;
}


void
PlaySound(RefArg inContext, RefArg inSound)
{
	RefVar	snd;

	if (IsSymbol(inSound))
		snd = GetProtoVariable(inContext, inSound);
	else
		snd = inSound;

	if (NOTNIL(snd))
		FPlaySound(inContext, snd);
}


Ref
FSoundPlayEnabled(RefArg inContext, RefArg inSound)
{
	return (((EQ(inSound, RA(click)) || EQ(inSound, RA(hiliteSound))) && NOTNIL(GetPreference(SYMA(penSoundEffects))))
			 || NOTNIL(GetPreference(SYMA(actionSoundEffects)))) ? TRUEREF : NILREF;
}


Ref
FConvertToSoundFrame(RefArg inContext, RefArg inSound)
{
	return ConvertToSoundFrame(inSound);
}


Ref
FPlaySoundEffect(RefArg inContext, RefArg inSound, RefArg inVolume, RefArg inType)
{
	if (NOTNIL(inSound))
	{
		RefVar	sndFrame(ConvertToSoundFrame(inSound));
		RefVar	theSound;
		bool		isQuiet;
		if (NOTNIL(inVolume))
		{
			theSound = Clone(sndFrame);
			SetFrameSlot(theSound, SYMA(volume), inVolume);
		}
		else
			theSound = sndFrame;
		isQuiet = (EQ(inType, SYMA(pen)) && ISNIL(GetPreference(SYMA(penSoundEffects))))
				 || (EQ(inType, SYMA(alarm)) && ISNIL(GetPreference(SYMA(alarmSoundEffects))))
				 || (EQ(inType, SYMA(action)) && ISNIL(GetPreference(SYMA(actionSoundEffects))));
		if (!isQuiet)
			return FPlaySoundIrregardless(inContext, theSound);
	}
	return NILREF;
}


Ref
FPlaySound(RefArg inContext, RefArg inSound)
{
	if (NOTNIL(FSoundPlayEnabled(inContext, inSound)))
		FPlaySoundIrregardless(inContext, inSound);
	return TRUEREF;
}


Ref
FPlaySoundSync(RefArg inContext, RefArg inSound)
{
#if !defined(forFramework)
	if (NOTNIL(inSound) && GlobalSoundChannel() != NULL)
	{
		NewtonErr	err;
		XTRY
		{
			XFAIL(err = gSoundChannel->stop(NULL, NULL))
			XFAIL(err = gSoundChannel->schedule(inSound))
			XFAIL(err = gSoundChannel->start(false))
		}
		XENDTRY;
		XDOFAIL(err)
		{
			ThrowErr(exFrames, err);
		}
		XENDFAIL;
	}
#endif
	return TRUEREF;
}


Ref
FPlaySoundIrregardless(RefArg inContext, RefArg inSound)
{
#if !defined(forFramework)
	if (NOTNIL(inSound) && GlobalSoundChannel() != NULL)
	{
		NewtonErr err = noErr;
		XTRY
		{
			XFAIL(err = gSoundChannel->stop(NULL, NULL))
			XFAIL(err = gSoundChannel->schedule(inSound))
			XFAIL(err = gSoundChannel->start(true))
		}
		XENDTRY;
		XDOFAIL(err)
		{
			ThrowErr(exFrames, err);
		}
		XENDFAIL;
	}
#endif
	return NILREF;
}

// in the following functions Fixed -> float

float
VolumeToDecibels(int inVolume)
{
	float	db;
	if (inVolume < 0)
		db = -FLT_MAX;
	else if (inVolume > 4)
		db = 0.0;
	else switch (inVolume)
	{
	case 0:					// silent
		db = -FLT_MAX;
		break;
	case 1:					// -18dB
		db = -18.0618;		// 0xFFEDF02E | 0x00120FD2
		break;
	case 2:					// -6dB
		db = -6.0206;		// 0xFFF9FABA | 0x00060546
		break;
	case 3:					// -3dB
		db = -3.0103;		// 0xFFFCFD5D | 0x000302A3
		break;
	case 4:					// 0dB
		db = 0.0;
		break;
	}
	return db;
}

Ref
FVolumeToDecibels(RefArg inRcvr, RefArg inVolume)
{
	float	db = VolumeToDecibels(RINT(inVolume));
	double ddb = (double) db;
	return MakeReal(ddb);
}

int
DecibelsToVolume(float inDecibels)
{
	if (inDecibels == -FLT_MAX)
		return 0;
	else if (inDecibels < -6.0206)
		return 1;
	else if (inDecibels < -3.0103)
		return 2;
	else if (inDecibels < 0.0)
		return 3;
	return 4;
}

Ref
FDecibelsToVolume(RefArg inRcvr, RefArg inDecibels)
{
	double ddb = CoerceToDouble(inDecibels);
	return MAKEINT(DecibelsToVolume(ddb));
}

Ref
FGetVolume(RefArg inRcvr)
{
#if !defined(forFramework)
	float	db = GlobalSoundChannel()->getVolume();
	return MAKEINT(DecibelsToVolume(db));
#else
	return MAKEINT(0);
#endif
}

Ref
FSetVolume(RefArg inRcvr, RefArg inVolume)
{
#if !defined(forFramework)
	int	vol = ISINT(inVolume) ? RINT(inVolume) : 0;
	float db = VolumeToDecibels(vol);
	GlobalSoundChannel()->setVolume(db);
#endif
	return NILREF;
}

Ref
FGetSystemVolume(RefArg inRcvr)
{
#if !defined(forFramework)
	// system volume is in dB
	float	db = GlobalSoundChannel()->getVolume();
	double ddb = (double) db;
	return MakeReal(ddb);
#else
	return MakeReal(0.0);
#endif
}

Ref
FSetSystemVolume(RefArg inRcvr, RefArg inDecibels)
{
#if !defined(forFramework)
	double ddb = CoerceToDouble(inDecibels);
	float	db = (float) ddb;
	GlobalSoundChannel()->setVolume(db);
	return MakeReal(ddb);
#else
	return MakeReal(0.0);
#endif
}

Ref
FSetInputGain(RefArg inRcvr, RefArg inGain)
{
	int	gain;
	if (ISINT(inGain))
	{
		gain = RINT(inGain);
		if (gain < 0)
			gain = 0;
		else if (gain > 255)
			gain = 255;
	}
	else
		gain = 128;
#if !defined(forFramework)
	GlobalSoundChannel()->setInputGain(gain);
#endif
	return MAKEINT(gain);
}

Ref
FSetOutputDevice(RefArg inRcvr, RefArg inDevice)
{
	int	dev = ISINT(inDevice) ? RINT(inDevice) : 0;
#if !defined(forFramework)
	GlobalSoundChannel()->setOutputDevice(dev);
#endif
	return MAKEINT(dev);
}


Ref
FClicker(RefArg inRcvr)
{
	if (NOTNIL(GetPreference(SYMA(penSoundEffects))))
	{
		RefVar	curClick(GetGlobalVar(SYMA(_curClick)));
		ArrayIndex	index = ISINT(curClick) ? RINT(curClick) : 0;
		RefVar	song(GetGlobalVar(SYMA(_clickSong)));
		if (ISNIL(song))
			song = *RS_clickSong;
// there are 6 tones in 17 notes of the song: 1,4,2,2,5,0,5,5,3,2,4,0,4,2,3,2,5
		if (++index >= Length(song))
			index = 0;
		RefVar	note(GetArraySlot(song, index));
		if (ISINT(note))
			note = GetArraySlot(RA(clicks), RINT(note));
		FPlaySoundIrregardless(RA(NILREF), note);
		DefGlobalVar(SYMA(_curClick), MAKEINT(index));
	}
	return NILREF;
}

/*
waveform := MakeBinaryFromHex("13335C8CC3EDFAFBEDD3AA753F1701000F2C5890CBF1FAF8EAD2AD71391000051430588CCCEFF9F6EAD3AB6E38100105112B568ECDEEF8F4E9DCB473390F03071229508AC7EBF9F5EBDBB1743A110B0C0E244C87BFE2F1EAE6DCB4814F2E251A111D3D6B9AC1E0F2F2DFB88958341D0F112546709AC1E0F3F0DBB78B5D3920131428456B94BDE0F1EAD2B08B67432617182C476890B8DDEEE0CAAD8F6D45281B21344C698DB2D5E0D5C2AB9170472E242A3B4E688BAECED3C9B8A591714B36323C4B576A86A4BCC0B9AFA18E6F4E40404D585F70859EAEAEA9A096876F584E4F585F6676899AA29B98958B7C665A5F64696C73808A8E8D8988877E736A686F73777A808788858280807D73", 'samples);
clicks := [ROM_click,
           {sndFrameType: 'simpleSound, samplingRate: 29401.2, dataType: 8, compressionType: 0, samples: waveform},
           {sndFrameType: 'simpleSound, samplingRate: 26193.7, dataType: 8, compressionType: 0, samples: waveform},
           {sndFrameType: 'simpleSound, samplingRate: 23336.1, dataType: 8, compressionType: 0, samples: waveform},
           {sndFrameType: 'simpleSound, samplingRate: 19623.4, dataType: 8, compressionType: 0, samples: waveform},
           {sndFrameType: 'simpleSound, samplingRate: 17482.5, dataType: 8, compressionType: 0, samples: waveform}];

(fixed sampingRate is 22254.53620 -- not as a decimal but as the two shorts of a Fixed number ≈ 22254.8)

for i := 1 to 5 do
	begin
	StrHexDump(Ref(0x003D47BD)[i].samples, 0);
	Write($\n)
	end;
*/

Ref
FSoundCheck(RefArg inRcvr)
{
//	this really does nothing
	return NILREF;
}

