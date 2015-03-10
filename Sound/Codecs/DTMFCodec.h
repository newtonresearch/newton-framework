/*
	File:		DTMFCodec.h

	Contains:	DTMF codec protocol definition.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__DTMFCODEC_H)
#define __DTMFCODEC_H 1

#include "SoundCodec.h"


#define kMaxTones 12

struct DTMFTone
{
	unsigned short	freqInt;				// | Fixed
	unsigned short	freqFract;			// |
	unsigned short	sustainAmplitude;
	unsigned short	leadIn;				// leading silence in ms
	unsigned short	attack;				// attack in ms
	unsigned short	decay;				// decay in ms
	unsigned short	sustain;				// sustain in ms
	unsigned short	release;				// release in ms
	unsigned short	peakAmplitude;		// peak amplitude
	unsigned short	trailOut;			// trailing silence in ms
}; 

struct DTMF
{
	unsigned short	blockType;		// MUST be 1
	unsigned short	synthType;		// 0-4
	unsigned short	reserved1;
	unsigned short	loopCount;
	unsigned short numOfTones;		// 1-12
	DTMFTone			tones[];
};


/*------------------------------------------------------------------------------
	D T M F C o d e c
------------------------------------------------------------------------------*/

PROTOCOL CDTMFCodec : public CSoundCodec
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CDTMFCodec)

	CSoundCodec *	make(void);
	void			destroy(void);

	NewtonErr	init(CodecBlock * inParms);
	NewtonErr	reset(CodecBlock * inParms);
	NewtonErr	produce(void * outBuf, size_t * ioBufSize, size_t * outDataSize, CodecBlock * ioParms);
	NewtonErr	consume(const void * inBuf, size_t * ioBufSize, size_t * outDataSize, const CodecBlock * inParms);
	void			start(void);
	void			stop(int);
	bool			bufferCompleted(void);

private:
	float			fSampleInterval;					// +10	seconds per sample
	ULong			fLeadInSamples[kMaxTones];		//	+14	number of samples to end of lead in
	ULong			fAttackSamples[kMaxTones];		//	+44	number of samples to end of attack
	ULong			fDecaySamples[kMaxTones];		//	+74	number of samples to end of decay
	ULong			fSustainSamples[kMaxTones];	//	+A4	number of samples to end of sustain
	ULong			fReleaseSamples[kMaxTones];	//	+D4	number of samples to end of release
	ULong			fTrailOutSamples[kMaxTones];	//	+104	number of samples to end of trail out
	ULong			fNumOfSamplesInSound;			// +134
	ULong			fLoopCount;							// +138
	ULong			fLoopIndex;							// +13C
	bool			fIsCompleted;						// +140
	float			fFrequency[kMaxTones];			//	+144
	float			fSustainAmplitude[kMaxTones];	//	+174
	float			fModSustainAmplitude[kMaxTones];	//	+1A4	modulator sustain amplitude
	float			fPhaseFactor[kMaxTones];		//	+1D4	radians to advance phase per sample
	float			fPhase[kMaxTones];				//	+204	phase (in radians) of sine wave
	float			fAmplitude[kMaxTones];			//	+234
	float			fModAmplitude[kMaxTones];		//	+264	modulator amplitude
	DTMF *		fComprData;							// +294
	size_t		fComprDataLength;					// +29C
	ULong			fComprDataOffset;					// +2A0
	ULong			fSampleIndex;						// +2A4
	int			fComprType;							// +2A8
	float			fSamplesPerSecond;				// +2AC
	int			fDataType;							// +2B0

//size +2C0
};


#endif	/* __DTMFCODEC_H */
