/*
	File:		GSMCodec.h

	Contains:	GSM codec interface.
					Requires the open source gsmlib.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__GSMCODEC_H)
#define __GSMCODEC_H 1

#include "SoundCodec.h"

/*------------------------------------------------------------------------------
	G S M C o d e c
------------------------------------------------------------------------------*/

PROTOCOL CGSMCodec : public CSoundCodec
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CGSMCodec)

	CGSMCodec *	make(void);
	void			destroy(void);

	NewtonErr	init(CodecBlock * inParms);
	NewtonErr	reset(CodecBlock * inParms);
	NewtonErr	produce(void*, unsigned long*, unsigned long*, CodecBlock * inParms);
	NewtonErr	consume(const void*, unsigned long*, unsigned long*, const CodecBlock * inParms);
	void			start(void);
	void			stop(int);
	bool			bufferCompleted(void);

private:
	gsm_state *	fGSMRef;				// +10
	unsigned		fRefState;			// +14
	UChar *		fComprBuffer;		// +18
	size_t		fComprBufLength;	// +1C
	ULong			fComprBufOffset;	// +20
	long			f24;
	long			f28;
	long			f2C;
//size +3C
};


#endif	/* __GSMCODEC_H */
