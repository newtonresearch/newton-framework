/*
	File:		EndpointPipe.h

	Contains:	Endpoint pipe declarations.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__ENDPOINTPIPE_H)
#define __ENDPOINTPIPE_H 1

#include "Endpoint.h"
#include "BufferPipe.h"


class CEndpointPipe : public CBufferPipe
{
public:
				CEndpointPipe();
				~CEndpointPipe();

	// initialisation
	void		init(CEndpoint * inEndpoint, size_t inGetBufSize, size_t inPutBufSize, Timeout inTimeout, bool);
	void		init(CEndpoint * inEndpoint, size_t inGetBufSize, size_t inPutBufSize, Timeout inTimeout, bool inFraming, CPipeCallback * inCalback);

	NewtonErr	addToAppWorld(void);
	NewtonErr	removeFromAppWorld(void);

	void		abort(void);
	Timeout	getTimeout(void);
	void		setTimeout(Timeout inTimeout);
	void		useFraming(bool);
	bool		usingFraming(void);

	// pipe interface
	void		resetRead(void);
	void		resetWrite(void);
	void		flushRead(void);
	void		flushWrite(void);
	void		overflow(void);
	void		underflow(long, bool&);

protected:
	CEndpoint *			fEndpoint;				// +10
	Timeout				fTimeout;				// +14
	bool					fFraming;				// +18
	CPipeCallback *	fCallback;				// +1C
	size_t				fNumOfBytesRead;		// +20
	size_t				fNumOfBytesWritten;	// +24
	bool					isAborted;				// +28
};


#endif	/* __ENDPOINTPIPE_H */
