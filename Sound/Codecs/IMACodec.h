/*
	File:		IMACodec.h

	Contains:	IMA codec interface.

	Written by:	Newton Research Group, 2015.
*/

#if !defined(__IMACODEC_H)
#define __IMACODEC_H 1

#include "SoundCodec.h"


/*------------------------------------------------------------------------------
	I M A C o d e c
------------------------------------------------------------------------------*/
struct IMAState
{
	int	predictor;
	int	index;
};


PROTOCOL CIMACodec : public CSoundCodec
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CIMACodec)

	CIMACodec *	make(void);
	void			destroy(void);

	NewtonErr	init(CodecBlock * inParms);
	NewtonErr	reset(CodecBlock * inParms);
	NewtonErr	produce(void * outBuf, size_t * ioBufSize, size_t * outDataSize, CodecBlock * ioParms);
	NewtonErr	consume(const void * inBuf, size_t * ioBufSize, size_t * outDataSize, const CodecBlock * inParms);
	void			start(void);
	void			stop(int);
	bool			bufferCompleted(void);

private:
	IMAState		fState;					// +10
	char *		fComprBuffer;			// +18
	size_t		fComprBufLength;		// +1C
	int			fComprBufOffset;		// +20
	int			fComprType;				// +24
	float			fSampleRate;			// +28
	int			fDataType;				// +2C
	int			f30;
	int			f34;
	int			f38;
//size +3C
};


#endif	/* __IMACODEC_H */
