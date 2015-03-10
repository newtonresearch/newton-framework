/*
	File:		Transformations.h

	Contains:	Affine transformations.
					See http://en.wikipedia.org/wiki/Transformation_matrix

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__TRANSFORMATIONS_H)
#define __TRANSFORMATIONS_H 1

#include "InkGeometry.h"

typedef float Matrix[3][3];

void		MxInit(Matrix * ioMx);
void		MxMove(Matrix * ioMx, float dx, float dy);
void		MxScale(Matrix * ioMx, float dx, float dy);
void		MxTransform(Matrix * inMx, FPoint * ioPt);

void		RotateMatrix(Matrix * ioMx, float inDegrees, float inAboutX, float inAboutY);
void		TransformPoints(Matrix * ioMx, int inNumOfPoints, FPoint * inPt);


#endif	/* __TRANSFORMATIONS_H */
