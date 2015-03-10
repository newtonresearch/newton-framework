/*
	File:		TaskSafeRingBuffer.h

	Contains:	TaskSafeRingBuffer declarations.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__TASKSAFERINGBUFFER_H)
#define __TASKSAFERINGBUFFER_H

#include "Buffer.h"
#include "Pipes.h"
#include "Semaphore.h"

/*----------------------------------------------------------------------
	C T a s k S a f e R i n g B u f f e r
----------------------------------------------------------------------*/

class CTaskSafeRingBuffer : public CBaseRingBuffer
{
public:
					CTaskSafeRingBuffer();
	virtual		~CTaskSafeRingBuffer();

	NewtonErr	init(size_t inBufLen, bool inThreaded);

	void			pause(Timeout inDelay);

	void			acquire(void);
	void			checkGetSignal(void);
	void			checkPutSignal(void);
	void			release(void);

	virtual	void		reset(void);

	virtual	int		peek(void);
	virtual	int		next(void);
	virtual	int		skip(void);
	virtual	int		get(void);
	virtual	size_t	getn(UByte * outBuf, size_t inCount);
	virtual	int		copyOut(UByte * outBuf, size_t& ioCount);

	virtual	int		put(int inByte);
	virtual	size_t	putn(const UByte * inBuf, size_t inCount);
	virtual	int		copyIn(const UByte * inBuf, size_t& ioCount);

	virtual	size_t	getSize(void);
	virtual	bool		atEOF(void);

	virtual	UByte		getCompletely(Timeout interval, Timeout inTimeout);
	virtual	void		getnCompletely(UByte * inBuf, size_t inLen, Timeout interval, Timeout inTimeout);

	virtual	void		putCompletely(int, Timeout interval, Timeout inTimeout);
	virtual	void		putnCompletely(const UByte * inBuf, size_t inLen, Timeout interval, Timeout inTimeout);

	virtual	bool		isFull(void);
	virtual	bool		isEmpty(void);
	virtual	size_t	freeCount(void);
	virtual	size_t	dataCount(void);

	virtual	long		updatePutVector(long);
	virtual	long		updateGetVector(long);
	virtual	void		computePutVectors(UByte *& outBuf1, size_t & outBuf1Len, UByte *& outBuf2, size_t & outBuf2Len);
	virtual	void		computeGetVectors(UByte *& outBuf1, size_t & outBuf1Len, UByte *& outBuf2, size_t & outBuf2Len);

private:
	UByte *		fBuf;				// +04
	UByte *		fBufEnd;			// +08
	size_t		fBufLen;			// +0C
	UByte *		fPutPtr;			// +10
	UByte *		fGetPtr;			// +14
	CULockingSemaphore *	f18;
	CULockingSemaphore *	f1C;
	NewtonErr	f20;		// +20	get signal
	NewtonErr	f24;		// +24	put signal
	int			fLockCount;		// +28
	ObjectId		fCurrentTask;	// +2C
	bool			fIsBufOurs;		// +30
	bool			fIsThreaded;	// +31
};


/*----------------------------------------------------------------------
	C T a s k S a f e R i n g P i p e
----------------------------------------------------------------------*/

class CTaskSafeRingPipe : public CPipe
{
public:
				CTaskSafeRingPipe();
				~CTaskSafeRingPipe();

	void		init(CTaskSafeRingBuffer * inRingBuf, bool inTakeRingBufOwnership, Timeout inTimeout, Timeout interval);
	void		init(size_t inBufLen, Timeout inTimeout, Timeout interval, bool inArg4);
	long		readSeek(long inOffset, int inSelector);
	long		readPosition(void) const;
	long		writeSeek(long inOffset, int inSelector);
	long		writePosition(void) const;
	void		readChunk(void * outBuf, size_t & ioSize, bool & ioEOF);
	void		writeChunk(const void * inBuf, size_t inSize, bool inFlush);
	void		flushRead(void);
	void		flushWrite(void);
	void		reset(void);
	void		overflow();
	void		underflow(long, bool&);

private:
	CTaskSafeRingBuffer *	fRingBuf;
	Timeout	fTimeout;
	Timeout	fRetryInterval;
	bool		fIsBufOurs;
};

#endif	/* __TASKSAFERINGBUFFER_H */
