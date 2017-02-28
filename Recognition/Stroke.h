/*
	File:		Stroke.h

	Contains:	Public Stroke declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__STROKE_H)
#define __STROKE_H 1

#include "RecStroke.h"

/*--------------------------------------------------------------------------------
	C S t r o k e
	Was TStrokePublic.
--------------------------------------------------------------------------------*/

class CStroke
{
public:
				CStroke(CRecStroke * inStroke, bool inTakeStrokeOwnership);
				~CStroke(void);

	static CStroke * make(CRecStroke * inStroke, bool inTakeStrokeOwnership);

	size_t	count(void);
	bool		isDone(void);
	ULong		downTime(void);
	ULong		upTime(void);
	void		bounds(Rect * outBounds);

	Point		getPoint(ArrayIndex index);
	Point		firstPoint(void);
	Point		finalPoint(void);

	void		inkOn(void);
	void		inkOff(bool);
	void		inkOff(bool, bool);

	void		getInkedRect(Rect * outBounds);
	void		invalidate(void);

private:
	long				f00;
	CRecStroke *	fStroke;			// +04
	bool				fStrokeIsOurs;	// +08
	Rect				fBounds;			// +0C
};


#endif	/* __STROKE_H */
