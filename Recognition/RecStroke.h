/*
	File:		RecStroke.h

	Contains:	Stroke declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__RECSTROKE_H)
#define __RECSTROKE_H 1

#include "RecObject.h"
#include "InkGeometry.h"
#include "TabletTypes.h"


/*--------------------------------------------------------------------------------
	T a b P t

	The stroke recognition system’s idea of a tablet point.
--------------------------------------------------------------------------------*/

struct TabPt
{
	float		x;
	float		y;
	short		z;		// type
	short		p;		// pressure? or just packing
};


/*--------------------------------------------------------------------------------
	C R e c S t r o k e

	An array of Points.
--------------------------------------------------------------------------------*/

class CRecStroke : public CDArray
{
public:
	static CRecStroke *	make(ArrayIndex inNumOfPts);
	virtual void	dealloc(void);

// debug
	virtual size_t		sizeInBytes(void);

	SamplePt *	tryToAddPoint(void);
	NewtonErr	addPoint(TabPt * inPt);
	void			bifurcate(void);
	void			endStroke(void);
	bool			isDone(void);

	SamplePt *	getPoint(ArrayIndex index);
	void			getTabPt(ArrayIndex index, TabPt * outPt);
	void			getFPoint(ArrayIndex index, FPoint * outPt);
	void			getStrokeRect(Rect * outRect);

	void			offset(float dx, float dy);
	void			rotate(float inDegrees);
	void			scale(float dx, float dy);
	void			map(FRect * inBox);
	void			updateBBox(void);
	void			draw(void);

	FRect *		bounds(void);
	void			setStartTime(ULong inStartTime);
	ULong			startTime(void) const;
	void			setEndTime(ULong inEndTime);
	ULong			endTime(void) const;
	ULong			f40Time(void) const;
	ULong			duration(void) const;
	ULong			getTapEvent(void) const;
	void			setTapEvent(ULong inEvt);

protected:
	NewtonErr		iStroke(ArrayIndex inNumOfPts);

	friend class CStrokeCentral;
//	friend class CStrokeDomain;

	FRect		fBBox;				// +20
	long		f30;					// +30	number of points?
	ULong		fStartTime;			// +34	time pen went down
	ULong		fEndTime;			// +38	time pen went up
	long		f3C;
	ULong		f40;					// +40
	short		f44;
	short		f46;
	ULong		fEvt;					// +48
// size +4C
};

inline FRect *		CRecStroke::bounds(void)  { return &fBBox; }
inline ULong		CRecStroke::startTime(void) const  { return fStartTime; }
inline void			CRecStroke::setStartTime(ULong inStartTime)  { fStartTime = inStartTime; }
inline ULong		CRecStroke::endTime(void) const  { return fEndTime; }
inline void			CRecStroke::setEndTime(ULong inEndTime)  { fEndTime = inEndTime; }
inline ULong		CRecStroke::duration(void) const  { return fEndTime - fStartTime; }
inline void			CRecStroke::getStrokeRect(Rect * outRect)  { UnfixRect(&fBBox, outRect); }
inline ULong		CRecStroke::f40Time(void) const  { return f40; }
inline ULong		CRecStroke::getTapEvent(void) const  { return fEvt; }
inline void			CRecStroke::setTapEvent(ULong inEvt)  { fEvt = inEvt; }


// CRecStroke fFlags
// pen size in bits 8..
#define kDoneStroke		0x40000000


/* -----------------------------------------------------------------------------
	C S S t r o k e
----------------------------------------------------------------------------- */

class CSStroke : public CRecStroke
{
public:
	static CSStroke *	make(ArrayIndex inNumOfPts);

	NewtonErr	addPoint(TabPt * inPt);

private:
	Point			f4C;
	Point			f50;
	bool			f54;
// size +58
};


/* -----------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------------- */

bool		AcquireStroke(CRecStroke * inStroke);
void		ReleaseStroke(void);

ULong		GetTicks(void);

void		DisposeRecStrokes(CRecStroke ** inStrokes);
ULong		CountRecStrokes(CRecStroke ** inStrokes);
void		OffsetStrokes(CRecStroke ** inStrokes, float dx, float dy);
void		GetStrokeRect(CRecStroke * inStroke, Rect * outBounds);
void		UnionBounds(CRecStroke ** inStrokes, Rect * outBounds);
void		InkBounds(CRecStroke ** inStrokes, Rect * outBounds);

PolygonShape *	AsPolygon(CRecStroke * inStroke);

extern void		StrokeTime(void);
extern CSStroke *	StrokeGet(void);


#endif	/* __RECSTROKE_H */
