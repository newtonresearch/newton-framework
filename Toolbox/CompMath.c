/*
	File:		CompMath.c

	Contains:	Composite math routines.

	Written by:	The Newton Tools group.
*/

#include "NewtonTypes.h"
#include "CompMath.h"


void	CompAdd(const Int64 * src, Int64 * dst)
{
}


void	CompSub(const Int64 * src, Int64 * dst)
{
}


void	CompNeg(Int64 * srcdst)
{
}


void	CompShift(Int64 * srcdst, long shift)
{
}


void	CompMul(long src1, long src2, Int64 * dst)
{
}


long	CompDiv(const Int64 * numerator, long denominator, long * remainder)
{
	return 0;
}


void	CompFixMul(const Int64 * compSrc, Fixed fixSrc, Int64 * compDst)
{
}


long	CompCompare(const Int64 * a, const Int64 * b)
{
	Int64	dst = *a;
	CompSub(b, &dst);
	if (dst.hi < 0)
		return -1;
	if (dst.lo > 0)
		return 1;
	return 0;
}


unsigned long CompSquareRoot(const Int64 * src)
{
	return 0;
}

