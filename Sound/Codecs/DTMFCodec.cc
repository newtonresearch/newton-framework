/*
	File:		DTMFCodec.cc

	Contains:	DTMF codec implementation.

	Written by:	Newton Research Group, 2007.
*/

#include "DTMFCodec.h"
#include "SoundErrors.h"
#include <math.h>

#define k2Pi 6.2831853
#define kDegreesToRadians 0.0174532925


/*------------------------------------------------------------------------------
	D T M F C o d e c
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CDTMFCodec implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CDTMFCodec::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN10CDTMFCodec6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN10CDTMFCodec4makeEv - 0b	\n"
"		.long		__ZN10CDTMFCodec7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CDTMFCodec\"	\n"
"2:	.asciz	\"CSoundCodec\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN10CDTMFCodec9classInfoEv - 4b	\n"
"		.long		__ZN10CDTMFCodec4makeEv - 4b	\n"
"		.long		__ZN10CDTMFCodec7destroyEv - 4b	\n"
"		.long		__ZN10CDTMFCodec4initEP10CodecBlock - 4b	\n"
"		.long		__ZN10CDTMFCodec5resetEP10CodecBlock - 4b	\n"
"		.long		__ZN10CDTMFCodec7produceEPvPmS1_P10CodecBlock - 4b	\n"
"		.long		__ZN10CDTMFCodec7consumeEPKvPmS2_PK10CodecBlock - 4b	\n"
"		.long		__ZN10CDTMFCodec5startEv - 4b	\n"
"		.long		__ZN10CDTMFCodec4stopEi - 4b	\n"
"		.long		__ZN10CDTMFCodec15bufferCompletedEv - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CDTMFCodec)

CSoundCodec *
CDTMFCodec::make(void)
{
	fComprData = NULL;
	fComprDataOffset = 0;
	return this;
}


void
CDTMFCodec::destroy(void)
{ }


NewtonErr
CDTMFCodec::init(CodecBlock * inParms)
{
	return noErr;
}


NewtonErr
CDTMFCodec::reset(CodecBlock * inParms)
{
	fComprData = (DTMF *)inParms->data;
	fComprType = inParms->comprType;
	fSamplesPerSecond = inParms->sampleRate;
	fDataType = inParms->dataType;
	fComprDataLength = inParms->dataSize;
	fComprDataOffset = 0;
	fLoopCount = 0;
	fLoopIndex = 0;
	fNumOfSamplesInSound = 0;
	fIsCompleted = false;
	fSampleIndex = 0;
	for (ArrayIndex i = 0; i < kMaxTones; ++i)
	{
		fPhase[i] = 0.0;
		fAmplitude[i] = 0.0;
		fModAmplitude[i] = 0.0;		// not in the original
	}
	return noErr;
}


NewtonErr
CDTMFCodec::produce(void * outBuf, size_t * ioBufSize, size_t * outDataSize, CodecBlock * ioParms)
{
	NewtonErr err;
	XTRY
	{
		XFAILIF(fComprData == NULL || CANONICAL_SHORT(fComprData->blockType) != 1, err = kSndErrNoSamples;)

		float	attackFactor[kMaxTones];	// amplitude increment / sample
		float	decayFactor[kMaxTones];
		float	releaseFactor[kMaxTones];
		float	modAttackFactor[kMaxTones];
		float	modDecayFactor[kMaxTones];
		float	modReleaseFactor[kMaxTones];
		int	i, index, numOfSamples;
		short * p = (short *)outBuf;

		fSampleInterval = 1 / fSamplesPerSecond;
		int	samplesPerMillisecond = fSamplesPerSecond / 1000.0;
		int	numOfTones = CANONICAL_SHORT(fComprData->numOfTones);
		if (numOfTones > kMaxTones)
			numOfTones = kMaxTones;
		fLoopCount = CANONICAL_SHORT(fComprData->loopCount);
		for (i = 0; i < numOfTones; ++i)
		{
			fLeadInSamples[i] = CANONICAL_SHORT(fComprData->tones[i].leadIn) * samplesPerMillisecond;
			fAttackSamples[i] = CANONICAL_SHORT(fComprData->tones[i].attack) * samplesPerMillisecond;
			fDecaySamples[i] = CANONICAL_SHORT(fComprData->tones[i].decay) * samplesPerMillisecond;
			fSustainSamples[i] = CANONICAL_SHORT(fComprData->tones[i].sustain) * samplesPerMillisecond;
			fReleaseSamples[i] = CANONICAL_SHORT(fComprData->tones[i].release) * samplesPerMillisecond;
			fTrailOutSamples[i] = CANONICAL_SHORT(fComprData->tones[i].trailOut) * samplesPerMillisecond;

			unsigned int ifreq = CANONICAL_SHORT(fComprData->tones[i].freqInt) * 65536 + CANONICAL_SHORT(fComprData->tones[i].freqFract);
			float	freq = ifreq / 65536.0;
			fFrequency[i] = freq;

			float	sustainAmpl = CANONICAL_SHORT(fComprData->tones[i].sustainAmplitude);
			fSustainAmplitude[i] = sustainAmpl;

			float	modSustainAmpl = CANONICAL_SHORT(fComprData->tones[i].sustainAmplitude);
			fModSustainAmplitude[i] = modSustainAmpl;

			float peakAmpl = CANONICAL_SHORT(fComprData->tones[i].peakAmplitude);
			float attackAmpl, modAttackAmpl;
			if (sustainAmpl > peakAmpl)
			{
				attackAmpl = sustainAmpl;
				modAttackAmpl = modSustainAmpl;
			}
			else
			{
				attackAmpl = peakAmpl;
				modAttackAmpl = peakAmpl;
			}

			fPhaseFactor[i] = k2Pi * freq / fSamplesPerSecond;		// in radians
			if (fNumOfSamplesInSound < fTrailOutSamples[i])
				fNumOfSamplesInSound = fTrailOutSamples[i];
			attackFactor[i] = attackAmpl / (fAttackSamples[i] - fLeadInSamples[i]);
			modAttackFactor[i] = modAttackAmpl / (fAttackSamples[i] - fLeadInSamples[i]);
			decayFactor[i] = (attackAmpl - fSustainAmplitude[i]) / (fDecaySamples[i] - fAttackSamples[i]);
			modDecayFactor[i] = (modAttackAmpl - fModSustainAmplitude[i]) / (fDecaySamples[i] - fAttackSamples[i]);
			releaseFactor[i] = fSustainAmplitude[i] / (fReleaseSamples[i] - fSustainSamples[i]);
			modReleaseFactor[i] = fModSustainAmplitude[i] / (fReleaseSamples[i] - fSustainSamples[i]);
		}
		// zero the remaining tone parameters
		for ( ; i < kMaxTones; ++i)
		{
			fLeadInSamples[i] = 0;
			fAttackSamples[i] = 0;
			fDecaySamples[i] = 0;
			fSustainSamples[i] = 0;
			fReleaseSamples[i] = 0;
			fTrailOutSamples[i] = 0;
			fFrequency[i] = 0.0;
			fSustainAmplitude[i] = 0.0;
			fModSustainAmplitude[i] = 0.0;
			fPhaseFactor[i] = 0.0;
			attackFactor[i] = 0.0;
			modAttackFactor[i] = 0.0;
			decayFactor[i] = 0.0;
			modDecayFactor[i] = 0.0;
			releaseFactor[i] = 0.0;
			modReleaseFactor[i] = 0.0;
		}

		numOfSamples = *ioBufSize / sizeof(short);

		switch (CANONICAL_SHORT(fComprData->synthType))
		{
		case 0:	
		// basic synthesis -- 1-12 pure sine wave tones
			for (index = 0; index < numOfSamples; index++)
			{
				float	sample = 0.0;
				fSampleIndex++;
				for (i = 0; i < numOfTones; ++i)
				{
					float phase;
					sample += sinf(fPhase[i]) * fAmplitude[i];

					phase = fPhase[i] + fPhaseFactor[i];
					if (phase > k2Pi)
						phase -= k2Pi;
					fPhase[i] = phase;

					if (fSampleIndex <= fLeadInSamples[i])
						fAmplitude[i] = 0.0;
					if (fSampleIndex > fLeadInSamples[i]
					&&  fSampleIndex <= fAttackSamples[i])
						fAmplitude[i] += attackFactor[i];
					if (fSampleIndex > fAttackSamples[i]
					&&  fSampleIndex <= fDecaySamples[i])
						fAmplitude[i] -= decayFactor[i];
					if (fSampleIndex > fDecaySamples[i]
					&&  fSampleIndex <= fSustainSamples[i])
						fAmplitude[i] = fSustainAmplitude[i];
					if (fSustainSamples[i] != fReleaseSamples[i])
					{
						if (fSampleIndex > fSustainSamples[i]
						&&  fSampleIndex <= fReleaseSamples[i])
							fAmplitude[i] -= releaseFactor[i];
					}
					if (fSampleIndex > fReleaseSamples[i]
					||  fAmplitude[i] < 0.0)
						fAmplitude[i] = 0.0;
				}
				if (fSampleIndex > fNumOfSamplesInSound)
				{
					if (fLoopIndex != fLoopCount)
					{
						fLoopIndex++;
						fSampleIndex = 0;
					}
					else
						fIsCompleted = true;
				}
				*p++ = sample;
			}
			break;

		case 1:
		// basic frequency modulation -- 1-6 frequency modulated tones, carrier + modulator
			for (index = 0; index < numOfSamples; index++)
			{
				float	sample = 0.0;
				fSampleIndex++;
				for (i = 0; i < numOfTones; i += 2)
				{
					float phase;
					float	modulator = sinf(fPhase[i+1]) * fModAmplitude[i+1] * kDegreesToRadians;
					sample += sinf(fPhase[i] + modulator) * fAmplitude[i];

					phase = fPhase[i] + fPhaseFactor[i];
					if (phase > k2Pi)
						phase -= k2Pi;
					fPhase[i] = phase;

					phase = fPhase[i+1] + fPhaseFactor[i+1];
					if (phase > k2Pi)
						phase -= k2Pi;
					fPhase[i+1] = phase;

					if (fSampleIndex <= fLeadInSamples[i])
						fAmplitude[i] = 0;
					if (fSampleIndex > fLeadInSamples[i]
					&&  fSampleIndex <= fAttackSamples[i])
						fAmplitude[i] += attackFactor[i];
					if (fSampleIndex > fAttackSamples[i]
					&&  fSampleIndex <= fDecaySamples[i])
						fAmplitude[i] -= decayFactor[i];
					if (fSampleIndex > fDecaySamples[i]
					&&  fSampleIndex <= fSustainSamples[i])
						fAmplitude[i] = fSustainAmplitude[i];
					if (fSustainSamples[i] != fReleaseSamples[i])
					{
						if (fSampleIndex > fSustainSamples[i]
						&&  fSampleIndex <= fReleaseSamples[i])
							fAmplitude[i] -= releaseFactor[i];
					}
					if (fSampleIndex > fReleaseSamples[i])
						fAmplitude[i] = 0.0;

					if (fSampleIndex <= fLeadInSamples[i+1])
						fModAmplitude[i+1] = 0.0;
					if (fSampleIndex > fLeadInSamples[i+1]
					&&  fSampleIndex <= fAttackSamples[i+1])
						fModAmplitude[i+1] += modAttackFactor[i+1];
					if (fSampleIndex > fAttackSamples[i+1]
					&&  fSampleIndex <= fDecaySamples[i+1])
						fModAmplitude[i+1] -= modDecayFactor[i+1];
					if (fSampleIndex > fDecaySamples[i+1]
					&&  fSampleIndex <= fSustainSamples[i+1])
						fModAmplitude[i+1] = fModSustainAmplitude[i+1];
					if (fSustainSamples[i+1] != fReleaseSamples[i+1])
					{
						if (fSampleIndex > fSustainSamples[i+1]
						&&  fSampleIndex <= fReleaseSamples[i+1])
							fModAmplitude[i+1] -= modReleaseFactor[i+1];
					}
					if (fSampleIndex > fReleaseSamples[i+1])
						fModAmplitude[i+1] = 0.0;

					if (fAmplitude[i] < 0.0)
						fAmplitude[i] = 0.0;
					if (fModAmplitude[i+1] < 0.0)
						fModAmplitude[i+1] = 0.0;
				}
				if (fSampleIndex > fNumOfSamplesInSound)
				{
					if (fLoopIndex != fLoopCount)
					{
						fLoopIndex++;
						fSampleIndex = 0;
					}
					else
						fIsCompleted = true;
				}
				*p++ = sample;
			}
			break;

		case 2:
		// frequency modulation -- 1-4 frequency modulated tones, carrier + modulator1 + modulator2
			for (index = 0; index < numOfSamples; index++)
			{
				float	sample = 0.0;
				fSampleIndex++;
				for (i = 0; i < numOfTones; i += 3)
				{
					float phase;
					float	modulator1 = sinf(fPhase[i+1]) * fModAmplitude[i+1] * kDegreesToRadians;
					float	modulator2 = sinf(fPhase[i+2]) * fModAmplitude[i+2] * kDegreesToRadians;
					sample += sinf(fPhase[i] + modulator1 + modulator2) * fAmplitude[i];

					phase = fPhase[i] + fPhaseFactor[i];
					if (phase > k2Pi)
						phase -= k2Pi;
					fPhase[i] = phase;

					phase = fPhase[i+1] + fPhaseFactor[i+1];
					if (phase > k2Pi)
						phase -= k2Pi;
					fPhase[i+1] = phase;
					
					phase = fPhase[i+2] + fPhaseFactor[i+2];
					if (phase > k2Pi)
						phase -= k2Pi;
					fPhase[i+2] = phase;

					if (fSampleIndex <= fLeadInSamples[i])
						fAmplitude[i] = 0;
					if (fSampleIndex > fLeadInSamples[i]
					&&  fSampleIndex <= fAttackSamples[i])
						fAmplitude[i] += attackFactor[i];
					if (fSampleIndex > fAttackSamples[i]
					&&  fSampleIndex <= fDecaySamples[i])
						fAmplitude[i] -= decayFactor[i];
					if (fSampleIndex > fDecaySamples[i]
					&&  fSampleIndex <= fSustainSamples[i])
						fAmplitude[i] = fSustainAmplitude[i];
					if (fSustainSamples[i] != fReleaseSamples[i])
					{
						if (fSampleIndex > fSustainSamples[i]
						&&  fSampleIndex <= fReleaseSamples[i])
							fAmplitude[i] -= releaseFactor[i];
					}
					if (fSampleIndex > fReleaseSamples[i])
						fAmplitude[i] = 0;

					if (fSampleIndex <= fLeadInSamples[i+1])
						fModAmplitude[i+1] = 0;
					if (fSampleIndex > fLeadInSamples[i+1]
					&&  fSampleIndex <= fAttackSamples[i+1])
						fModAmplitude[i+1] += modAttackFactor[i+1];
					if (fSampleIndex > fAttackSamples[i+1]
					&&  fSampleIndex <= fDecaySamples[i+1])
						fModAmplitude[i+1] -= modDecayFactor[i+1];
					if (fSampleIndex > fDecaySamples[i+1]
					&&  fSampleIndex <= fSustainSamples[i+1])
						fModAmplitude[i+1] = fModSustainAmplitude[i+1];
					if (fSustainSamples[i+1] != fReleaseSamples[i+1])
					{
						if (fSampleIndex > fSustainSamples[i+1]
						&&  fSampleIndex <= fReleaseSamples[i+1])
							fModAmplitude[i+1] -= modReleaseFactor[i+1];
					}
					if (fSampleIndex > fReleaseSamples[i+1])
						fModAmplitude[i+1] = 0;

					if (fSampleIndex <= fLeadInSamples[i+2])
						fModAmplitude[i+2] = 0;
					if (fSampleIndex > fLeadInSamples[i+2]
					&&  fSampleIndex <= fAttackSamples[i+2])
						fModAmplitude[i+2] += modAttackFactor[i+2];
					if (fSampleIndex > fAttackSamples[i+2]
					&&  fSampleIndex <= fDecaySamples[i+2])
						fModAmplitude[i+2] -= modDecayFactor[i+2];
					if (fSampleIndex > fDecaySamples[i+2]
					&&  fSampleIndex <= fSustainSamples[i+2])
						fModAmplitude[i+2] = fModSustainAmplitude[i+2];
					if (fSustainSamples[i+2] != fReleaseSamples[i+2])
					{
						if (fSampleIndex > fSustainSamples[i+2]
						&&  fSampleIndex <= fReleaseSamples[i+2])
							fModAmplitude[i+2] -= modReleaseFactor[i+2];
					}
					if (fSampleIndex > fReleaseSamples[i+2])
						fModAmplitude[i+2] = 0;

					if (fAmplitude[i] < 0)
						fAmplitude[i] = 0;
					if (fModAmplitude[i+1] < 0)
						fModAmplitude[i+1] = 0;
					if (fModAmplitude[i+2] < 0)
						fModAmplitude[i+2] = 0;
				}
				if (fSampleIndex > fNumOfSamplesInSound)
				{
					if (fLoopIndex != fLoopCount)
					{
						fLoopIndex++;
						fSampleIndex = 0;
					}
					else
						fIsCompleted = true;
				}
				*p++ = sample;
			}
			break;

		case 3:
		// frequency modulation -- 1-4 frequency modulated tones, carrier + modulated modulator
			for (index = 0; index < numOfSamples; index++)
			{
				float	sample = 0.0;
				fSampleIndex++;
				for (i = 0; i < numOfTones; i += 3)
				{
					float phase;
					float	modulator2 = sinf(fPhase[i+2]) * fModAmplitude[i+2] * kDegreesToRadians;
					float	modulator = sinf(fPhase[i+1] + modulator2) * fModAmplitude[i+1] * kDegreesToRadians;
					sample += sinf(fPhase[i] + modulator) * fAmplitude[i];

					phase = fPhase[i] + fPhaseFactor[i];
					if (phase > k2Pi)
						phase -= k2Pi;
					fPhase[i] = phase;

					phase = fPhase[i+1] + fPhaseFactor[i+1];
					if (phase > k2Pi)
						phase -= k2Pi;
					fPhase[i+1] = phase;

					phase = fPhase[i+2] + fPhaseFactor[i+2];
					if (phase > k2Pi)
						phase -= k2Pi;
					fPhase[i+2] = phase;

					if (fSampleIndex <= fLeadInSamples[i])
						fAmplitude[i] = 0.0;
					if (fSampleIndex > fLeadInSamples[i]
					&&  fSampleIndex <= fAttackSamples[i])
						fAmplitude[i] += attackFactor[i];
					if (fSampleIndex > fAttackSamples[i]
					&&  fSampleIndex <= fDecaySamples[i])
						fAmplitude[i] -= decayFactor[i];
					if (fSampleIndex > fDecaySamples[i]
					&&  fSampleIndex <= fSustainSamples[i])
						fAmplitude[i] = fSustainAmplitude[i];
					if (fSustainSamples[i] != fReleaseSamples[i])
					{
						if (fSampleIndex > fSustainSamples[i]
						&&  fSampleIndex <= fReleaseSamples[i])
							fAmplitude[i] -= releaseFactor[i];
					}
					if (fSampleIndex > fReleaseSamples[i])
						fAmplitude[i] = 0.0;

					if (fSampleIndex <= fLeadInSamples[i+1])
						fModAmplitude[i+1] = 0.0;
					if (fSampleIndex > fLeadInSamples[i+1]
					&&  fSampleIndex <= fAttackSamples[i+1])
						fModAmplitude[i+1] += modAttackFactor[i+1];
					if (fSampleIndex > fAttackSamples[i+1]
					&&  fSampleIndex <= fDecaySamples[i+1])
						fModAmplitude[i+1] -= modDecayFactor[i+1];
					if (fSampleIndex > fDecaySamples[i+1]
					&&  fSampleIndex <= fSustainSamples[i+1])
						fModAmplitude[i+1] = fModSustainAmplitude[i+1];
					if (fSustainSamples[i+1] != fReleaseSamples[i+1])
					{
						if (fSampleIndex > fSustainSamples[i+1]
						&&  fSampleIndex <= fReleaseSamples[i+1])
							fModAmplitude[i+1] -= modReleaseFactor[i+1];
					}
					if (fSampleIndex > fReleaseSamples[i+1])
						fModAmplitude[i+1] = 0.0;

					if (fSampleIndex <= fLeadInSamples[i+2])
						fModAmplitude[i+2] = 0.0;
					if (fSampleIndex > fLeadInSamples[i+2]
					&&  fSampleIndex <= fAttackSamples[i+2])
						fModAmplitude[i+2] += modAttackFactor[i+2];
					if (fSampleIndex > fAttackSamples[i+2]
					&&  fSampleIndex <= fDecaySamples[i+2])
						fModAmplitude[i+2] -= modDecayFactor[i+2];
					if (fSampleIndex > fDecaySamples[i+2]
					&&  fSampleIndex <= fSustainSamples[i+2])
						fModAmplitude[i+2] = fModSustainAmplitude[i+2];
					if (fSustainSamples[i+2] != fReleaseSamples[i+2])
					{
						if (fSampleIndex > fSustainSamples[i+2]
						&&  fSampleIndex <= fReleaseSamples[i+2])
							fModAmplitude[i+2] -= modReleaseFactor[i+2];
					}
					if (fSampleIndex > fReleaseSamples[i+2])
						fModAmplitude[i+2] = 0.0;

					if (fAmplitude[i] < 0.0)
						fAmplitude[i] = 0.0;
					if (fModAmplitude[i+1] < 0.0)
						fModAmplitude[i+1] = 0.0;
					if (fModAmplitude[i+2] < 0.0)
						fModAmplitude[i+2] = 0.0;
				}
				if (fSampleIndex > fNumOfSamplesInSound)
				{
					if (fLoopIndex != fLoopCount)
					{
						fLoopIndex++;
						fSampleIndex = 0;
					}
					else
						fIsCompleted = true;
				}
				*p++ = sample;
			}
			break;

		case 4:
		// complex frequency modulation -- 1-3 frequency modulated tones, carrier + modulated modulated modulator
			for (index = 0; index < numOfSamples; index++)
			{
				float	sample = 0.0;
				fSampleIndex++;
				for (i = 0; i < numOfTones; i += 4)
				{
					float phase;
					float	modulator3 = sinf(fPhase[i+3]) * fModAmplitude[i+3];
					float	modulator2 = sinf(fPhase[i+2] + modulator3) * fModAmplitude[i+2];
					float	modulator = sinf(fPhase[i+1] + modulator2) * fModAmplitude[i+1];
					float	carrier = fPhase[i];
					sample += sinf(carrier + modulator) * fAmplitude[i];

					phase = fPhase[i] + fPhaseFactor[i];
					if (phase > k2Pi)
						phase -= k2Pi;
					fPhase[i] = phase;

					phase = fPhase[i+1] + fPhaseFactor[i+1];
					if (phase > k2Pi)
						phase -= k2Pi;
					fPhase[i+1] = phase;

					phase = fPhase[i+2] + fPhaseFactor[i+2];
					if (phase > k2Pi)
						phase -= k2Pi;
					fPhase[i+2] = phase;

					phase = fPhase[i+3] + fPhaseFactor[i+3];
					if (phase > k2Pi)
						phase -= k2Pi;
					fPhase[i+3] = phase;

					if (fSampleIndex <= fLeadInSamples[i])
						fAmplitude[i] = 0;
					if (fSampleIndex > fLeadInSamples[i]
					&&  fSampleIndex <= fAttackSamples[i])
						fAmplitude[i] += attackFactor[i];
					if (fSampleIndex > fAttackSamples[i]
					&&  fSampleIndex <= fDecaySamples[i])
						fAmplitude[i] -= decayFactor[i];
					if (fSampleIndex > fDecaySamples[i]
					&&  fSampleIndex <= fSustainSamples[i])
						fAmplitude[i] = fSustainAmplitude[i];
					if (fSustainSamples[i] != fReleaseSamples[i])
					{
						if (fSampleIndex > fSustainSamples[i]
						&&  fSampleIndex <= fReleaseSamples[i])
							fAmplitude[i] -= releaseFactor[i];
					}
					if (fSampleIndex > fReleaseSamples[i])
						fAmplitude[i] = 0.0;

					if (fSampleIndex <= fLeadInSamples[i+1])
						fModAmplitude[i+1] = 0.0;
					if (fSampleIndex > fLeadInSamples[i+1]
					&&  fSampleIndex <= fAttackSamples[i+1])
						fModAmplitude[i+1] += modAttackFactor[i+1];
					if (fSampleIndex > fAttackSamples[i+1]
					&&  fSampleIndex <= fDecaySamples[i+1])
						fModAmplitude[i+1] -= modDecayFactor[i+1];
					if (fSampleIndex > fDecaySamples[i+1]
					&&  fSampleIndex <= fSustainSamples[i+1])
						fModAmplitude[i+1] = fModSustainAmplitude[i+1];
					if (fSustainSamples[i+1] != fReleaseSamples[i+1])
					{
						if (fSampleIndex > fSustainSamples[i+1]
						&&  fSampleIndex <= fReleaseSamples[i+1])
							fModAmplitude[i+1] -= modReleaseFactor[i+1];
					}
					if (fSampleIndex > fReleaseSamples[i+1])
						fModAmplitude[i+1] = 0.0;

					if (fSampleIndex <= fLeadInSamples[i+2])
						fModAmplitude[i+2] = 0.0;
					if (fSampleIndex > fLeadInSamples[i+2]
					&&  fSampleIndex <= fAttackSamples[i+2])
						fModAmplitude[i+2] += modAttackFactor[i+2];
					if (fSampleIndex > fAttackSamples[i+2]
					&&  fSampleIndex <= fDecaySamples[i+2])
						fModAmplitude[i+2] -= modDecayFactor[i+2];
					if (fSampleIndex > fDecaySamples[i+2]
					&&  fSampleIndex <= fSustainSamples[i+2])
						fModAmplitude[i+2] = fModSustainAmplitude[i+2];
					if (fSustainSamples[i+2] != fReleaseSamples[i+2])
					{
						if (fSampleIndex > fSustainSamples[i+2]
						&&  fSampleIndex <= fReleaseSamples[i+2])
							fModAmplitude[i+2] -= modReleaseFactor[i+2];
					}
					if (fSampleIndex > fReleaseSamples[i+2])
						fModAmplitude[i+2] = 0.0;

					if (fSampleIndex <= fLeadInSamples[i+3])
						fModAmplitude[i+3] = 0.0;
					if (fSampleIndex > fLeadInSamples[i+3]
					&&  fSampleIndex <= fAttackSamples[i+3])
						fModAmplitude[i+3] += modAttackFactor[i+3];
					if (fSampleIndex > fAttackSamples[i+3]
					&&  fSampleIndex <= fDecaySamples[i+3])
						fModAmplitude[i+3] -= modDecayFactor[i+3];
					if (fSampleIndex > fDecaySamples[i+3]
					&&  fSampleIndex <= fSustainSamples[i+3])
						fModAmplitude[i+3] = fModSustainAmplitude[i+3];
					if (fSustainSamples[i+3] != fReleaseSamples[i+3])
					{
						if (fSampleIndex > fSustainSamples[i+3]
						&&  fSampleIndex <= fReleaseSamples[i+3])
							fModAmplitude[i+3] -= modReleaseFactor[i+3];
					}
					if (fSampleIndex > fReleaseSamples[i+3])
						fModAmplitude[i+3] = 0.0;

					if (fAmplitude[i] < 0.0)
						fAmplitude[i] = 0.0;
					if (fModAmplitude[i+1] < 0.0)
						fModAmplitude[i+1] = 0.0;
					if (fModAmplitude[i+2] < 0.0)
						fModAmplitude[i+2] = 0.0;
					if (fModAmplitude[i+3] < 0.0)
						fModAmplitude[i+3] = 0.0;
				}
				if (fSampleIndex > fNumOfSamplesInSound)
				{
					if (fLoopIndex != fLoopCount)
					{
						fLoopIndex++;
						fSampleIndex = 0;
					}
					else
						fIsCompleted = true;
				}
				*p++ = sample;
			}
			break;
		}

		*ioBufSize = numOfSamples * sizeof(short);
		*outDataSize = 0;
		ioParms->dataType = fDataType;
		ioParms->comprType = kSampleLinear;
		ioParms->sampleRate = fSamplesPerSecond;
		err = noErr;
	}
	XENDTRY;
	return err;
}


// can’t encode DTMF!
NewtonErr
CDTMFCodec::consume(const void * inBuf, size_t * ioBufSize, size_t * outDataSize, const CodecBlock * inParms)
{
	fComprDataOffset = fComprDataLength;
	*outDataSize = 0;
	return kSndErrNoSamples;
}


void
CDTMFCodec::start(void)
{ }


void
CDTMFCodec::stop(int)
{ }


bool
CDTMFCodec::bufferCompleted(void)
{
	return fIsCompleted;
}

