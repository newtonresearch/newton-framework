/*
	File:		MemoryPipe.h

	Contains:	External interface to the Newton ROM memory buffer pipe class.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__MEMORYPIPE_H)
#define __MEMORYPIPE_H

#include "BufferPipe.h"


/*----------------------------------------------------------------------
	C M e m o r y P i p e
----------------------------------------------------------------------*/

class CMemoryPipe : public CBufferPipe
{
public:
				CMemoryPipe();
	virtual	~CMemoryPipe();

	// pipe interface

	void		flushRead(void);
	void		flushWrite(void);
	void		reset(void);
	void		overflow();
	void		underflow(long, bool&);

private:
};


#endif	/*	__MEMORYPIPE_H	*/
