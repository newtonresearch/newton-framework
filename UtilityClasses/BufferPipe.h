/*
	File:		BufferPipe.h

	Contains:	External interface to the Newton ROM buffer pipe class.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__BUFFERPIPE_H)
#define __BUFFERPIPE_H

#include "Pipes.h"

#ifndef __BUFFERSEGMENT_H
#include "BufferSegment.h"
#endif


/*----------------------------------------------------------------------
	C B u f f e r P i p e
----------------------------------------------------------------------*/

class CBufferPipe : public CPipe
{
public:
				CBufferPipe();
	virtual	~CBufferPipe();

	// initialisation

	void		init(size_t inGetBufSize, size_t inPutBufSize);
	void		init(CBufferSegment * inGetBuf, CBufferSegment * inPutBuf, bool inDoFreeBuffer);

	// pipe interface

	virtual	long		readSeek(long inOffset, int inSelector);
	virtual	long		readPosition(void) const;
	virtual	long		writeSeek(long inOffset, int inSelector);
	virtual	long		writePosition(void) const;
	virtual	void		readChunk(void * outBuf, size_t & ioSize, bool & outEOF);
	virtual	void		writeChunk(const void * inBuf, size_t inSize, bool inFlush);
	virtual	void		reset(void);
	virtual	void		resetRead();
	virtual	void		resetWrite();

	// get primitives

	int		peek(bool);
	int		next(void);
	int		skip(void);
	int		get(void);

	// put primitives

	int		put(int dataByte);

protected:
	CBufferSegment *	fGetBuf;
	CBufferSegment *	fPutBuf;
	bool		fBufsAreOurs;
	bool		f0D;
};


#endif	/*	__BUFFERPIPE_H	*/
