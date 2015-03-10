/*
	File:		BusyBox.h

	Contains:	Busy box declarations.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__BUSYBOX_H)
#define __BUSYBOX_H 1

#include "Quartz.h"
#include "TimerQueue.h"


/* -----------------------------------------------------------------------------
	C B u s y B o x
----------------------------------------------------------------------------- */

class CBusyBox : public CTimerElement
{
public:
				CBusyBox();
				~CBusyBox();

	void		timeout(void);
	void		doCommand(long cmd);

private:
	void		showBusyBox(void);
	void		hideBusyBox(void);

#if defined(correct)
	PixelMap		fPix;			// +18	deprecated
#else
	CGImageRef	fImage;
#endif
	int			fVisible;	// +34
};


#endif	/* __BUSYBOX_H */
