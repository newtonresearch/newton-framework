/*
	File:		TabletTypes.h

	Contains:	Tablet type declarations.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__TABLETTYPES_H)
#define __TABLETTYPES_H 1

#include "InkTypes.h"


// long versions of basic QD types.
// NOTE also reversal of co-ordinates.

struct LPoint
{
	int32_t	x;
	int32_t	y;
};


struct LRect
{
	int32_t	left;
	int32_t	top;
	int32_t	right;
	int32_t	bottom;
};


struct Calibration
{
	LPoint	scale;		// +00
	LPoint	origin;		// +08
	UChar		x10;
	UChar		x11;
// size+14
};


// Packed tablet point.
// There are 8 sample points per user point

#define kTabScale 8.0

struct SamplePt
{
	unsigned int	zhi:2;
	unsigned int	x:14;
	unsigned int	p:1;
	unsigned int	zlo:1;
	unsigned int	y:14;

	void		getPoint(FPoint * outPt);

	void		setFlag(void);
	void		unsetFlag(void);
	UChar		testFlag(void);
};

// the original provides C functions that accept (ULong inBits) arg
// but this seems superfluous
inline void		SamplePt::setFlag(void)   { p = 1; }
inline void		SamplePt::unsetFlag(void) { p = 0; }
inline UChar	SamplePt::testFlag(void)  { return p != 0; }

inline float	SampleX(SamplePt * p) { return (float) p->x/kTabScale; }
inline float	SampleY(SamplePt * p) { return (float) p->y/kTabScale; }
inline short	SampleP(SamplePt * p) { return (p->zhi << 1) + p->zlo; }

inline void		SamplePt::getPoint(FPoint * outPt)
{
	outPt->x = SampleX(this);
	outPt->y = SampleY(this);
}


// Packed tablet point.
// Must fit in a ULong -- these samples fill the tablet queue.

union TabletSample
{
	struct
	{
#if defined(hasByteSwapping)
		unsigned int	z:4;
		unsigned int	y:14;
		unsigned int	x:14;
#else
		unsigned int	x:14;
		unsigned int	y:14;
		unsigned int	z:4;
#endif
	};
	unsigned int intValue;
};


// sample types -- TabletSample.z
enum
{
	// 0..7 => pen pressure
	kPenMidPressureSample = 4,
	kPenMaxPressureSample = 7,
	kPenDownSample = 13,
	kPenUpSample,
	kPenInvalidSample
};

// tablet state
enum
{
	kTabletAwake,
	kTablet6 = 6,
	kTabletBypassed = 8,
	kTabletAsleep
};


#endif	/* __TABLETTYPES_H */
