/*
	File:		CircleBuf.h

	Contains:	Circle buffer interface.

	Written by:	Newton Research Group, 2009.
*/

#if !defined(__CIRCLEBUF_H)
#define __CIRCLEBUF_H 1

#include "BufferList.h"
#include "TraceEvents.h"

struct MarkerInfo
{
	ULong	x00;
	ULong	x04;
};

/*------------------------------------------------------------------------------
	C C i r c l e B u f
	A circular FIFO buffer for communications services.
------------------------------------------------------------------------------*/

class CCircleBuf
{
public:
					CCircleBuf();
					~CCircleBuf();

	NewtonErr	allocate(ArrayIndex inSize);
	NewtonErr	allocate(ArrayIndex inSize, int inArg2, EBufferResidence inResidence, UChar inArg4);
	void			deallocate(void);

	void			reset(void);
	void			resetStart(void);

	ArrayIndex	bufferCount(void);
	ArrayIndex	bufferSpace(void);
	NewtonErr	bufferSpace(ArrayIndex inSpaceReqd);

	ArrayIndex	markerCount(void);
	ArrayIndex	markerSpace(void);

	NewtonErr	copyIn(CBufferList * inBuf, ArrayIndex * ioSize);
	NewtonErr	copyIn(UByte * inBuf, ArrayIndex * ioSize, bool inArg3 = false, ULong inArg4 = 0);

	NewtonErr	copyOut(CBufferList * outBuf, ArrayIndex * ioSize, ULong * outArg3 = NULL);
	NewtonErr	copyOut(UByte * outBuf, ArrayIndex * ioSize, ULong * outArg3 = NULL);

	void			updateStart(ArrayIndex inDelta);
	void			updateEnd(ArrayIndex inDelta);

	NewtonErr	putEOM(ULong);
	NewtonErr	putNextEOM(ULong);
	NewtonErr	putEOMMark(ULong, ULong);
	ULong			getEOMMark(ULong *);
	ArrayIndex	peekNextEOMIndex(void);
	ArrayIndex	peekNextEOMIndex(ULong *);
	NewtonErr	bufferCountToNextMarker(ULong * outCount);
	NewtonErr	flushToNextMarker(ULong *);

	NewtonErr	getBytes(CCircleBuf * inBuf);
	NewtonErr	getNextByte(UByte * outByte);
	NewtonErr	getNextByte(UByte * outByte, ULong *);
	NewtonErr	peekNextByte(UByte * outByte);
	NewtonErr	peekNextByte(UByte * outByte, ULong *);
	NewtonErr	peekFirstLong(ULong * outLong);

	NewtonErr	putFirstPossible(UByte inByte);
	NewtonErr	putNextPossible(UByte inByte);
	NewtonErr	putNextStart(void);
	NewtonErr	putNextCommit(void);
	NewtonErr	putNextByte(UByte);
	NewtonErr	putNextByte(UByte, ULong);
	void			flushBytes(void);

	void			DMABufInfo(ULong *, ULong *, UByte *, UByte *);
	void			DMAGetInfo(ULong *);
	void			DMAGetUpdate(ULong);
	void			DMAPutInfo(ULong *, ULong *);
	void			DMAPutUpdate(ULong, UByte, ULong);

private:
	void			getAlignLong(void);
	void			putAlignLong(void);

	size_t			fBufLen;				// +00
	UByte *			fBuf;					// +04
	ArrayIndex		fGetIndex;			// +08
	ArrayIndex		fPutIndex;			// +0C
	ULong				f10;
	EBufferResidence	fBufferResidence;	// +14
	UChar				f15;					// +15	flags:  0x02 => lock this in the heap  0x04 => wire it
	ArrayIndex		fNumOfMarkers;		// +18
	MarkerInfo *	fMarkers;			// +1C
	ArrayIndex		fGetMarkerIndex;	// +20
	ArrayIndex		fPutMarkerIndex;	// +24
};


#endif	/* __CIRCLEBUF_H */
