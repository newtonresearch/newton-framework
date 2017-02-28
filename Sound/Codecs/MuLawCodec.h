/*
	File:		MuLawCodec.h

	Contains:	MuLaw codec interface.

	Written by:	Newton Research Group, 2015.
*/

#if !defined(__MULAWCODEC_H)
#define __MULAWCODEC_H 1

#include "SoundCodec.h"

/*------------------------------------------------------------------------------
	M u L a w C o d e c
------------------------------------------------------------------------------*/

PROTOCOL CMuLawCodec : public CSoundCodec
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CMuLawCodec)

	CMuLawCodec *	make(void);
	void			destroy(void);

	NewtonErr	init(CodecBlock * inParms);
	NewtonErr	reset(CodecBlock * inParms);
	NewtonErr	produce(void * outBuf, size_t * ioBufSize, size_t * outDataSize, CodecBlock * ioParms);
	NewtonErr	consume(const void * inBuf, size_t * ioBufSize, size_t * outDataSize, const CodecBlock * inParms);
	void			start(void);
	void			stop(int);
	bool			bufferCompleted(void);

private:
	void			blockConvertMuLawToLin16(void * inDst, const void * inSrc, size_t inLen);
	void			blockConvertLin16ToMuLaw(void * inDst, const void * inSrc, size_t inLen);

	UChar *		fComprBuffer;			// +10
	size_t		fComprBufLength;		// +14
	int			fComprBufOffset;		// +18
	int			fComprType;				// +1C
	int			fDataType;				// +20
	float			fSampleRate;			// +24
//size +28
};


#endif	/* __MULAWCODEC_H */
