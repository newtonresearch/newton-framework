/*
	File:		Paths.h

	Contains:	Interface to path functions.
					Don’t know where paths are used.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__PATHS_H)
#define __PATHS_H 1

#include "QDTypes.h"

typedef FPoint	point;

struct curve
{
	point 	first;
	point 	control;
	point 	last;
};
typedef curve * curvePtr;


struct path
{
	long		vectors;
	long		controlBits[1];
	point		vector[];
};


struct paths
{
	long		contours;
	path		contour[];
};
typedef paths * pathsPtr, ** pathsHandle;


struct pathWalker
{
	int		isLine;
	curve		c;
	/* private */
	long		index;
	long		ep;
	long *	bits;
	point *	p;
};


#endif	/* __PATHS_H */
