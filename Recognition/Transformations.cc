/*
	File:		Transformations.cc

	Contains:	Affine transformations.
					See http://en.wikipedia.org/wiki/Transformation_matrix

	Written by:	Newton Research Group, 2010.
*/

#include "Transformations.h"
#include <string.h>
#include <math.h>

/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

const Matrix idMatrix = { { 1.0, 0.0, 0.0 },
								  { 0.0, 1.0, 0.0 },
								  { 0.0, 0.0, 1.0 } };

/*------------------------------------------------------------------------------
	M a t r i c e s
------------------------------------------------------------------------------*/

void
MxInit(Matrix * ioMx)
{
	memcpy(ioMx, &idMatrix, sizeof(Matrix));
}


void
MxCopy(const Matrix * inSrc, Matrix * inDst)
{
	if (inSrc != inDst)
		memcpy(inDst, &inSrc, sizeof(Matrix));
}


void
Concatenate(Matrix * ioMx1, Matrix * ioMx2)
{
	Matrix mx;
	for (int row = 0; row < 3; row++)
	{
		for (int col = 0; col < 3; col++)
		{
			float sum = 0.0;
			for (ArrayIndex i = 0; i < 3; ++i)
				sum += *ioMx1[row][i] * *ioMx2[i][col];
			mx[row][col] = sum;
		}
	}
	MxCopy(&mx, ioMx2);
}


void
MxRotate(Matrix * ioMx, int inRadians)
{
	Matrix mx;
	MxCopy(&idMatrix, &mx);
	float sinTheta = sinf(inRadians);
	float cosTheta = cosf(inRadians);
	mx[0][0] = cosTheta;
	mx[0][1] = -sinTheta;
	mx[1][0] = sinTheta;
	mx[1][1] = cosTheta;
	Concatenate(&mx, ioMx);
}


void
MxScale(Matrix * ioMx, float dx, float dy)
{
	Matrix mx;
	MxCopy(&idMatrix, &mx);
	mx[1][1] = dy;
	mx[0][0] = dx;
	Concatenate(&mx, ioMx);
}


void
MxMove(Matrix * ioMx, float dx, float dy)
{
	Matrix mx;
	MxCopy(&idMatrix, &mx);
	mx[2][1] = dy;
	mx[2][0] = dx;
	Concatenate(&mx, ioMx);
}


void
MxTransform(Matrix * inMx, FPoint * ioPt)
{
	float x = ioPt->x;
	float y = ioPt->y;
	ioPt->x = *inMx[0][0] * x + *inMx[1][0] * y + *inMx[2][0];
	ioPt->y = *inMx[0][1] * x + *inMx[1][1] * y + *inMx[2][1];
}


void
RotateMatrix(Matrix * ioMx, float inDegrees, float inAboutX, float inAboutY)
{
	MxMove(ioMx, -inAboutX, -inAboutY);
	MxRotate(ioMx, DegToRad(-inDegrees));
	MxMove(ioMx, inAboutX, inAboutY);
}


void
TransformPoints(Matrix * ioMx, int inNumOfPoints, FPoint * inPt)
{
	for (int i = 0; i < inNumOfPoints; i++, inPt++)
		MxTransform(ioMx, inPt);
}
