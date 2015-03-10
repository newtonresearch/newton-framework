/*
	File:		Buffer.h

	Contains:	Buffer declarations.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__BUFFER_H)
#define __BUFFER_H

#include "UserSharedMem.h"

enum
{
	kSeekFromBeginning = -1,
	kSeekFromHere,
	kSeekFromEnd
};

/*----------------------------------------------------------------------
	C M i n B u f f e r
	Abstract base class defining the minimum interface for a buffer.
----------------------------------------------------------------------*/

class CMinBuffer
{
public:
				CMinBuffer();
	virtual	~CMinBuffer();

	virtual	void		reset(void) = 0;

	virtual	int		peek(void) = 0;
	virtual	int		next(void) = 0;
	virtual	int		skip(void) = 0;
	virtual	int		get(void) = 0;
	virtual	size_t	getn(UByte * outBuf, size_t inCount) = 0;
	virtual	int		copyOut(UByte * outBuf, size_t& ioCount) = 0;

	virtual	int		put(int inByte) = 0;
	virtual	size_t	putn(const UByte * inBuf, size_t inCount) = 0;
	virtual	int		copyIn(const UByte * inBuf, size_t& ioCount) = 0;

	virtual	size_t	getSize(void) = 0;
	virtual	bool		atEOF(void) = 0;
};


/*----------------------------------------------------------------------
	C B u f f e r
	Abstract base class defining the interface for a buffer.
----------------------------------------------------------------------*/

class CBuffer : public CMinBuffer
{
public:
				CBuffer();
	virtual	~CBuffer();

	virtual	long		hide(long inCount, int inDirection) = 0;
	virtual	long		seek(long inOffset, int inDirection) = 0;
	virtual	long		position(void) = 0;
};


/*----------------------------------------------------------------------
	C S h a d o w B u f f e r S e g m e n t
----------------------------------------------------------------------*/

class CShadowBufferSegment : public CBuffer
{
public:
				CShadowBufferSegment();
	virtual	~CShadowBufferSegment();

	NewtonErr			init(ObjectId inSharedMemId, long inOffset, long inSize);

	virtual	void		reset(void);

	virtual	int		peek(void);
	virtual	int		next(void);
	virtual	int		skip(void);
	virtual	int		get(void);
	UByte					getByteAt(long index);
	virtual	size_t	getn(UByte * outBuf, size_t inCount);
	virtual	int		copyOut(UByte * outBuf, size_t& ioCount);

	virtual	int		put(int inByte);
	int					putByteAt(int inByte, long index);
	virtual	size_t	putn(const UByte * inBuf, size_t inCount);
	virtual	int		copyIn(const UByte * inBuf, size_t& ioCount);

	virtual	size_t	getSize(void);
	virtual	bool		atEOF(void);

	virtual	long		hide(long inCount, int inDirection);
	virtual	long		seek(long inOffset, int inDirection);
	virtual	long		position(void);

private:
	long			fLoBound;		// +04
	long			fIndex;			// +08
	long			fHiBound;		// +0C
	size_t		fBufSize;		// +10
	CUSharedMem	fSharedMem;		// +14
};


/*----------------------------------------------------------------------
	C B a s e R i n g B u f f e r
----------------------------------------------------------------------*/

class CBaseRingBuffer : public CMinBuffer
{
public:
				CBaseRingBuffer();
	virtual	~CBaseRingBuffer();

	virtual	bool		isFull(void) = 0;
	virtual	bool		isEmpty(void) = 0;
	virtual	size_t	freeCount(void) = 0;
	virtual	size_t	dataCount(void) = 0;

	virtual	long		updatePutVector(long) = 0;
	virtual	long		updateGetVector(long) = 0;
	virtual	void		computePutVectors(UByte *& outBuf1, size_t & outBuf1Len, UByte *& outBuf2, size_t & outBuf2Len) = 0;
	virtual	void		computeGetVectors(UByte *& outBuf1, size_t & outBuf1Len, UByte *& outBuf2, size_t & outBuf2Len) = 0;
};


/*----------------------------------------------------------------------
	C R i n g B u f f e r
----------------------------------------------------------------------*/
class CPipe;

class CRingBuffer : public CBaseRingBuffer
{
public:
				CRingBuffer();
	virtual	~CRingBuffer();

	NewtonErr			init(size_t inSize);
	NewtonErr			makeShared(ULong inPermissions);
	NewtonErr			unshare(void);

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

	virtual	int		copyIn(CPipe * inPipe, size_t & ioLength);
	virtual	size_t	getnAt(long inOffset, UByte * outData, size_t inLength);

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
	UByte *		fBufEnd;			// +08	points past end of buffer
	size_t		fBufSize;		// +0C
	UByte *		fWrPtr;			// +10
	UByte *		fRdPtr;			// +14
	CUSharedMem	fSharedMem;		// +18
	bool			fIsShared;		// +20
	bool			fIsBufOurs;		// +21
};


/*----------------------------------------------------------------------
	C S h a d o w R i n g B u f f e r
----------------------------------------------------------------------*/

class CShadowRingBuffer : public CBaseRingBuffer
{
public:
				CShadowRingBuffer();
	virtual	~CShadowRingBuffer();

	NewtonErr	init(ObjectId inSharedMemId, long inOffset, long inSize);

	virtual	void		reset(void);

	virtual	int		peek(void);
	virtual	int		next(void);
	virtual	int		skip(void);
	virtual	int		get(void);
	UByte					getByteAt(long index);
	virtual	size_t	getn(UByte * outBuf, size_t inCount);
	virtual	int		copyOut(UByte * outBuf, size_t& ioCount);

	virtual	int		put(int inByte);
	int					putByteAt(int inByte, long index);
	virtual	size_t	putn(const UByte * inBuf, size_t inCount);
	virtual	int		copyIn(const UByte * inBuf, size_t& ioCount);

	virtual	size_t	getSize(void);
	virtual	bool		atEOF(void);

	virtual	bool		isFull(void);
	virtual	bool		isEmpty(void);
	virtual	size_t	freeCount(void);
	virtual	size_t	dataCount(void);

	virtual	long		updatePutVector(long);
	virtual	long		updateGetVector(long);
	virtual	void		computePutVectors(UByte *& outBuf1, size_t & outBuf1Len, UByte *& outBuf2, size_t & outBuf2Len);
	virtual	void		computeGetVectors(UByte *& outBuf1, size_t & outBuf1Len, UByte *& outBuf2, size_t & outBuf2Len);

	size_t				tempDataCount(void);
	void					computeTempGetVectors(UByte *& outBuf1, size_t & outBuf1Len, UByte *& outBuf2, size_t & outBuf2Len);

private:
	long			fWrIndex;		// +04
	long			fRdIndex;		// +08
	long			fTempRdIndex;	// +0C
	size_t		fBufSize;		// +10
	CUSharedMem	fSharedMem;		// +14
};


#endif	/* __BUFFER_H */
