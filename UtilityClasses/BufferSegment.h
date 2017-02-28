/*
	File:		BufferSegment.h

	Contains:	External interface to the Newton ROM buffer segment class.

	Copyright:	© 1992-1995 by Apple Computer, Inc.  All rights reserved.
*/

#ifndef __BUFFERSEGMENT_H
#define __BUFFERSEGMENT_H

#ifndef __NEWTON_H
#include "Newton.h"
#endif

#ifndef __BUFFER_H
#include "Buffer.h"
#endif

#include "UserSharedMem.h"

/*----------------------------------------------------------------------
	C B u f f e r S e g m e n t
----------------------------------------------------------------------*/

class CBufferSegment : public CBuffer
{
public:
	// in the original:
	// these member functions are intentionally declared private
	// to prevent external code from knowing the size of the object
	// NR: all very well and good, but CFramePartHandler::install allocates one of these on the stack

				CBufferSegment();
//				CBufferSegment(const CBufferSegment&);
	virtual	~CBufferSegment();

	static CBufferSegment *	make(void);
	void			destroy();

	// initialization

	NewtonErr	init(size_t len);
	NewtonErr	init(void * data, size_t len,
						 bool freeBuffer = false,
						 long validOff = 0,
						 long validCount = -1);

	NewtonErr	makeShared(ULong inPermissions);
	NewtonErr	restoreShared(ULong inPermissions);
	NewtonErr	unshare(void);

	size_t		getPhysicalSize(void);
	NewtonErr	setPhysicalSize(size_t inSize);

	// get primitives

	virtual	int		peek(void);
	virtual	int		next(void);
	virtual	int		skip(void);
	virtual	int		get(void);
	virtual	size_t	getn(UByte * p, size_t n);
	virtual	int		copyOut(UByte * p, size_t& n);

	// put primitives

	virtual	int		put(int dataByte);
	virtual	size_t	putn(const UByte * p, size_t n);
	virtual	int		copyIn(const UByte * p, size_t& n);

	// misc

	virtual	void		reset(void);

	// position and size

	virtual	size_t	getSize(void);
	virtual	bool		atEOF(void);

	virtual	long		hide(long count, int dir);
	virtual	long		seek(long offset, int dir);
	virtual	long		position(void);

private:
//	CBufferSegment& operator=(const CBufferSegment&);
//	CBufferSegment* operator&();
//	const CBufferSegment* operator&() const;

	char *	fBuffer;				// +04
	char *	fBufferLimit;		// +08
	size_t	fBufSize;			// +0C
	char *	fLoBound;			// +10
	char *	fBufPtr;				// +14
	char *	fHiBound;			// +18
	CUSharedMem	fSharedBuf;		// +1C
	bool		fBufferIsOurs;		// +24
	bool		fSharedBufIsInitialized;
};


//--------------------------------------------------------------------------------
//		ROM Glue
//--------------------------------------------------------------------------------

inline CBufferSegment * CBufferSegment::make()
{ return new CBufferSegment; }
	
inline void CBufferSegment::destroy()
{ delete this; }

#endif	/*	__BUFFERSEGMENT_H	*/
