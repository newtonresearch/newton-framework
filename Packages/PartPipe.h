/*
	File:		PartPipe.h

	Contains:	Part pipe interface.

	Written by:	Newton Research Group, 2016.
*/

#if !defined(__PARTPIPE_H)
#define __PARTPIPE_H 1

#include "Pipes.h"
#include "Buffer.h"


/* -----------------------------------------------------------------------------
	C P a r t P i p e
----------------------------------------------------------------------------- */

class CPartPipe : public CPipe
{
public:
				CPartPipe();
	virtual	~CPartPipe();

	void		init(ObjectId inPortId, CShadowRingBuffer * inBuffer, bool inOwnBuffer);
	void		setStreamSize(size_t inSize);

	long		readSeek(long inOffset, int inSelector);
	long		readPosition(void) const;
	long		writeSeek(long inOffset, int inSelector);
	long		writePosition(void) const;
	void		readChunk(void * outBuf, size_t & ioSize, bool & outEOF);
	void		writeChunk(const void * inBuf, size_t inSize, bool inFlush);
	void		flushRead(void);
	void		flushWrite(void);
	void		reset(void);
	void		overflow();
	void		underflow(long, bool&);

	void		seekEOF(void);
	void		close(void);

private:
	CShadowRingBuffer *	f04;
	bool			f08;
	CUPort *		f0C;
	size_t		f10;
// size+14
};

#endif	/* __PARTPIPE_H */
