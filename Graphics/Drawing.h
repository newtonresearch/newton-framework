/*
	File:		Drawing.h

	Contains:	Miscellaneous screen drawing declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__DRAWING_H)
#define __DRAWING_H 1

#if 0
#include "Objects.h"
#include "QDTypes.h"


/*------------------------------------------------------------------------------
	C B i t s
------------------------------------------------------------------------------*/
class CBitsPort;

class CBits
{
public:
					CBits();
					~CBits();

	bool			init(const Rect * inBounds);
	void			init(const PixelMap * inPixMap);
	static size_t	initBitMap(const Rect * inBounds, PixelMap * ioPixMap);
	void			cleanUp(void);
	void			beginDrawing(Point inOrigin);
	void			endDrawing(void);
	void			draw(const Rect * inBounds, long inTransferMode, RgnHandle inMaskRgn);
	void			draw(const Rect * inSrcBounds, const Rect * inDstBounds, long inTransferMode, RgnHandle inMaskRgn);
	void			copyFromScreen(const Rect * inSrcBounds, const Rect * inDstBounds, long inTransferMode, RgnHandle inMaskRgn);
	void			copyIntoBitmap(PixelMap * inPixMap, long inTransferMode, RgnHandle inMaskRgn);
	void			fill(long inPattern);
	void			setPort(void);
	void			restorePort(void);
	void			setBounds(const Rect * inBounds);

	PixelMap				fPixMap;					// +00
private:
	CBitsPort *			fBitsPort;				// +1C
	bool					fIsDirty;				// +20
	bool					fIsPixMapCreatedByUs;// +21
	ExceptionCleanup	fException;				// +24
};


/*------------------------------------------------------------------------------
	C B i t s P o r t
------------------------------------------------------------------------------*/

class CBitsPort
{
public:
//				CBitsPort()	{ } probably inline
				~CBitsPort();

	void		init(CBits * inBits, Point inOrigin, bool inClear);

private:
	friend class CBits;

	CBits *	fBits;		// +00
	GrafPtr	fPort;		// +04
	GrafPtr	fSavedPort;	// +08
};


/*------------------------------------------------------------------------------
	C S a v e S c r e e n B i t s
------------------------------------------------------------------------------*/

class CSaveScreenBits
{
public:
				CSaveScreenBits(void);
				~CSaveScreenBits(void);

	bool		allocateBuffers(Rect * inBounds);
	void		saveScreenBits(void);
	void		restoreScreenBits(Rect * inBounds, RgnHandle inMask);

private:
	PixelMap				fPixMap;		// +00
	ExceptionCleanup	fException;	// +1C
//	size +2C
};
#endif


#endif	/* __DRAWING_H */
