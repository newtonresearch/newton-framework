/*
	File:		Maths.cc

	Contains:	Mathematical functions.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "Maths.h"
#include "RichStrings.h"
#include "RSSymbols.h"


/*------------------------------------------------------------------------------
	M a t h e m a t i c a l   F u n c t i o n s
------------------------------------------------------------------------------*/

Ref
NumberAdd(RefArg a, RefArg b)
{
	return MakeReal(CoerceToDouble(a) + CoerceToDouble(b));
}


Ref
NumberSubtract(RefArg a, RefArg b)
{
	return MakeReal(CoerceToDouble(a) - CoerceToDouble(b));
}


Ref
NumberMultiply(RefArg a, RefArg b)
{
	return MakeReal(CoerceToDouble(a) * CoerceToDouble(b));
}


Ref
NumberDivide(RefArg a, RefArg b)
{
	return MakeReal(CoerceToDouble(a) / CoerceToDouble(b));
}


Ref
UnevenDivide(long a, long b)
{
	return MakeReal((double)a / (double)b);
}

#pragma mark -

Ref
FAdd(RefArg inRcvr, RefArg a, RefArg b)
{
	if (ISINT(a) && ISINT(b))
		return MAKEINT(RVALUE(a) + RVALUE(b));

	return NumberAdd(a, b);
}


Ref
FSubtract(RefArg inRcvr, RefArg a, RefArg b)
{
	if (ISINT(a) && ISINT(b))
		return MAKEINT(RVALUE(a) - RVALUE(b));

	return NumberSubtract(a, b);
}


Ref
FMultiply(RefArg inRcvr, RefArg a, RefArg b)
{
	if (ISINT(a) && ISINT(b))
		return MAKEINT(RVALUE(a) * RVALUE(b));

	return NumberMultiply(a, b);
}


Ref
FDivide(RefArg inRcvr, RefArg a, RefArg b)
{
	if (ISINT(a) && ISINT(b))
	{
		if (RVALUE(b) != 0)
		{
			div_t result = div(RVALUE(a), RVALUE(b));
			if (result.rem == 0)
				return MAKEINT(result.quot);
		}
		return UnevenDivide(RVALUE(a), RVALUE(b));
	}

	return NumberDivide(a, b);
}


Ref
FDiv(RefArg inRcvr, RefArg a, RefArg b)
{
	return MAKEINT(div(RINT(a), RINT(b)).quot);
}


Ref
FMod(RefArg inRcvr, RefArg a, RefArg b)
{
	return MAKEINT(div(RINT(a), RINT(b)).rem);
}


Ref
FNegate(RefArg inRcvr, RefArg a)
{
	if (ISINT(a))
		return MAKEINT(-RINT(a));

	else if (IsReal(a))
	{
		double	d = CDouble(a);
		return MakeReal(-d);
	}

	ThrowBadTypeWithFrameData(kNSErrNotANumber, a);
	return NILREF;
}


#pragma mark -
/*------------------------------------------------------------------------------
	Boolean logic.
------------------------------------------------------------------------------*/

Ref
FBoolAnd(RefArg inRcvr, RefArg a, RefArg b)
{
	return MAKEBOOLEAN(NOTNIL(a) && NOTNIL(b));
}


Ref
FBoolOr(RefArg inRcvr, RefArg a, RefArg b)
{
	return MAKEBOOLEAN(NOTNIL(a) || NOTNIL(b));
}


Ref
FNot(RefArg inRcvr, RefArg a)
{
	return MAKEBOOLEAN(ISNIL(a));
}

#pragma mark -

/*------------------------------------------------------------------------------
	Bitwise operators.
------------------------------------------------------------------------------*/

Ref
FBitAnd(RefArg inRcvr, RefArg a, RefArg b)
{
	return MAKEINT(RINT(a) & RINT(b));
}


Ref
FBitOr(RefArg inRcvr, RefArg a, RefArg b)
{
	return MAKEINT(RINT(a) | RINT(b));
}


Ref
FBitXor(RefArg inRcvr, RefArg a, RefArg b)
{
	return MAKEINT(RINT(a) ^ RINT(b));
}


Ref
FBitNot(RefArg inRcvr, RefArg a)
{
	return MAKEINT(~RINT(a));
}


#pragma mark -
/* -----------------------------------------------------------------------------
	Char operators.
----------------------------------------------------------------------------- */

extern "C"
Ref
FCHR(RefArg inRcvr, RefArg a)
{
	return MAKECHAR(RINT(a));
}

extern "C"
Ref
FORD(RefArg inRcvr, RefArg a)
{
	return MAKEINT(RCHAR(a));
}


#pragma mark -
/* -----------------------------------------------------------------------------
	Integer Math
----------------------------------------------------------------------------- */

Ref
Fabs(RefArg inRcvr, RefArg inArg)
{
	if (ISINT(inArg))
	{
		int x = RVALUE(inArg);
		if (x < 0)
			x = -x;
		return MAKEINT(x);
	}
	else if (IsReal(inArg))
	{
		double x = CoerceToDouble(inArg);
		if (x < 0.0)
			return MakeReal(-x);
		return x;
	}
	ThrowBadTypeWithFrameData(kNSErrNotANumber, inArg);
	return NILREF;
}

Ref
Fceiling(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	double ceilx = ceil(x);
	if (ceilx >= 1.0 && ceilx <= RVALUE(INT32_MAX))
		return MAKEINT((int)ceilx);
	return MakeReal(ceilx);
}

Ref
FMax(RefArg inRcvr, RefArg a, RefArg b)
{
	return NOTNIL(FGreaterOrEqual(inRcvr, a, b)) ? a : b;
}


Ref
FMin(RefArg inRcvr, RefArg a, RefArg b)
{
	return NOTNIL(FLessOrEqual(inRcvr, a, b)) ? a : b;
}


Ref
FLShift(RefArg inRcvr, RefArg a, RefArg inShift)
{
	return MAKEINT(RINT(a) << RINT(inShift));
}


Ref
FRShift(RefArg inRcvr, RefArg a, RefArg inShift)
{
	return MAKEINT(RINT(a) >> RINT(inShift));
}


Ref
FReal(RefArg inRcvr, RefArg inArg)
{
	double x = (double)RINT(inArg);
	return MakeReal(x);
}


#pragma mark -
/* -----------------------------------------------------------------------------
	Random numbers.
----------------------------------------------------------------------------- */

Ref
FRandom(RefArg inRcvr, RefArg a, RefArg b)
{
	int lo = RINT(a);
	int hi = RINT(b);
	if (lo > hi)
		ThrowErr(exFrames, kNSErrBadArgs);
	return MAKEINT(lo + rand()/(hi-lo+1));
}

Ref
FSetRandomSeed(RefArg inRcvr, RefArg inSeed)
{
	srand(RINT(inSeed));
	return NILREF;
}

/* -----------------------------------------------------------------------------
	Random state.
----------------------------------------------------------------------------- */
long gRandomState;	// 0C10595C

size_t sizeof_rand_state(void) { return sizeof(gRandomState); }
void set_rand_state(void * ioState) { gRandomState = *(long *)ioState; }
void get_rand_state(void * ioState) { *(long *)ioState = gRandomState; }

Ref
FGetRandomState(RefArg inRcvr)
{
	RefVar randomState(AllocateBinary(SYMA(randomState), sizeof_rand_state()));
	get_rand_state(BinaryData(randomState));
	return randomState;
}

Ref
FSetRandomState(RefArg inRcvr, RefArg inState)
{
	set_rand_state(BinaryData(inState));
	return NILREF;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	Floating Point Math
----------------------------------------------------------------------------- */

Ref
Ffabs(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(fabs(x));
}

Ref
Ffloor(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(floor(x));
}

Ref
Ffdim(RefArg inRcvr, RefArg a, RefArg b)
{
	double x = CoerceToDouble(a);
	double y = CoerceToDouble(b);
	return MakeReal(fdim(x,y));
}

Ref
Ffmax(RefArg inRcvr, RefArg a, RefArg b)
{
	double x = CoerceToDouble(a);
	double y = CoerceToDouble(b);
	return MakeReal(fmax(x,y));
}

Ref
Ffmin(RefArg inRcvr, RefArg a, RefArg b)
{
	double x = CoerceToDouble(a);
	double y = CoerceToDouble(b);
	return MakeReal(fmin(x,y));
}

Ref
Ffmod(RefArg inRcvr, RefArg a, RefArg b)
{
	double x = CoerceToDouble(a);
	double y = CoerceToDouble(b);
	return MakeReal(fmod(x,y));
}

Ref
Fgamma(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(tgamma(x));
}

Ref
Flgamma(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(lgamma(x));
}

Ref
Fpow(RefArg inRcvr, RefArg a, RefArg b)
{
	double x = CoerceToDouble(a);
	double y = CoerceToDouble(b);
	return MakeReal(pow(x,y));
}

Ref
Fsqrt(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(sqrt(x));
}

Ref
Fhypot(RefArg inRcvr, RefArg a, RefArg b)
{
	double x = CoerceToDouble(a);
	double y = CoerceToDouble(b);
	return MakeReal(hypot(x,y));
}


Ref
FIsFiniteNumber(RefArg inRcvr, RefArg inNum)
{
	return ISINT(inNum)
		 || (IsReal(inNum) && isfinite(CDouble(inNum)));
}

Ref
Fisfinite(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MAKEBOOLEAN(isfinite(x));
}

Ref
Fisnormal(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MAKEBOOLEAN(isnormal(x));
}

Ref
Fisnan(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MAKEBOOLEAN(isnan(x));
}


Ref
Ferf(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(erf(x));
}

Ref
Ferfc(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(erfc(x));
}

Ref
Fexp(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(exp(x));
}

Ref
Fexpm1(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(expm1(x));
}


Ref
Fldexp(RefArg inRcvr, RefArg a, RefArg b)
{
	double x = CoerceToDouble(a);
	double y = CoerceToDouble(b);
	return MakeReal(ldexp(x,y));
}

Ref
Flog(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(log(x));
}

Ref
Flogb(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(logb(x));
}

Ref
Flog1p(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(log1p(x));
}

Ref
Flog10(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(log10(x));
}

Ref
Fnextafterd(RefArg inRcvr, RefArg a, RefArg b)
{
	double x = CoerceToDouble(a);
	double y = CoerceToDouble(b);
	return MakeReal(nextafter(x,y));
}

Ref
Frandomx(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	RefVar result(MakeArray(2));
//	SetArraySlot(result, 0, MakeReal(randomx(&x)));	// in CarbonCore fp.h
	SetArraySlot(result, 1, MakeReal(x));
	return result;
}

Ref
Fremainder(RefArg inRcvr, RefArg a, RefArg b)
{
	double x = CoerceToDouble(a);
	double y = CoerceToDouble(b);
	return MakeReal(remainder(x,y));
}

Ref
Fremquo(RefArg inRcvr, RefArg a, RefArg b)
{
	double x = CoerceToDouble(a);
	double y = CoerceToDouble(b);
	int quo;
	RefVar result(MakeArray(2));
	SetArraySlot(result, 0, MakeReal(remquo(x, y, &quo)));
	SetArraySlot(result, 1, MAKEINT(quo));
	return result;
}

Ref
Frinttol(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MAKEINT(lrint(x));
}

Ref
Frint(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(rint(x));
}

Ref
Fnearbyint(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(nearbyint(x));
}

Ref
Fround(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(round(x));
}

Ref
Ftrunc(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MakeReal(trunc(x));
}

Ref
Fscalb(RefArg inRcvr, RefArg a, RefArg b)
{
	double x = CoerceToDouble(a);
	int n = RINT(b);
	return MakeReal(scalbn(x,n));
}

Ref
Fsignbit(RefArg inRcvr, RefArg inArg)
{
	double x = CoerceToDouble(inArg);
	return MAKEINT(signbit(x));
}

Ref
Fcopysign(RefArg inRcvr, RefArg a, RefArg b)
{
	double x = CoerceToDouble(a);
	double y = CoerceToDouble(b);
	return MakeReal(copysign(x,y));
}

Ref
Fsignum(RefArg inRcvr, RefArg inArg)
{
	if (ISINT(inArg))
	{
		int x = RVALUE(inArg);
		if (x > 0)
			return MAKEINT(1);
		else if (x < 0)
			return MAKEINT(-1);
		return MAKEINT(0);
	}
	else if (IsReal(inArg))
	{
		double x = CoerceToDouble(inArg);
		if (x > 0.0)
			return MakeReal(1.0);
		else if (x < 0.0)
			return MakeReal(-1.0);
		return MakeReal(0.0);
	}
	ThrowBadTypeWithFrameData(kNSErrNotANumber, inArg);
	return NILREF;
}


/*------------------------------------------------------------------------------
	Trigonometry.
------------------------------------------------------------------------------*/

Ref
Fsin(RefArg inRcvr, RefArg inArg)
{ return NILREF; }

Ref
Fsinh(RefArg inRcvr, RefArg inArg)
{ return NILREF; }

Ref
Fcos(RefArg inRcvr, RefArg inArg)
{ return NILREF; }

Ref
Fcosh(RefArg inRcvr, RefArg inArg)
{ return NILREF; }

Ref
Ftan(RefArg inRcvr, RefArg inArg)
{ return NILREF; }

Ref
Ftanh(RefArg inRcvr, RefArg inArg)
{ return NILREF; }


Ref
Fasin(RefArg inRcvr, RefArg inArg)
{ return NILREF; }

Ref
Fasinh(RefArg inRcvr, RefArg inArg)
{ return NILREF; }

Ref
Facos(RefArg inRcvr, RefArg inArg)
{ return NILREF; }

Ref
Facosh(RefArg inRcvr, RefArg inArg)
{ return NILREF; }

Ref
Fatan(RefArg inRcvr, RefArg inArg)
{ return NILREF; }

Ref
Fatan2(RefArg inRcvr, RefArg a, RefArg b)
{ return NILREF; }

Ref
Fatanh(RefArg inRcvr, RefArg inArg)
{ return NILREF; }


#pragma mark -
/*------------------------------------------------------------------------------
	Comparison.
------------------------------------------------------------------------------*/

Ref
FEqual(RefArg inRcvr, RefArg a, RefArg b)
{
	Ref	aRef = a;
	Ref	bRef = b;
	if (ISPTR(aRef) || ISPTR(bRef))
	{
		bool  aIsReal = IsReal(aRef);
		bool  bIsReal = IsReal(bRef);

		if (aIsReal && bIsReal)
			return MAKEBOOLEAN(CDouble(a) == CDouble(b));

		if (aIsReal || bIsReal)
		{
			if (ISINT(aRef) || ISINT(bRef))
				return MAKEBOOLEAN(CoerceToDouble(a) == CoerceToDouble(b));
			return TRUEREF;
		}

		return MAKEBOOLEAN(EQ(a, b));
	}

	return MAKEBOOLEAN(aRef == bRef);
}


Ref
FLessThan(RefArg inRcvr, RefArg a, RefArg b)
{
	Ref	aRef = a;
	Ref	bRef = b;
	bool  aIsInt = ISINT(aRef);
	bool  bIsInt = ISINT(bRef);

	if (aIsInt && bIsInt)
		return MAKEBOOLEAN(RVALUE(aRef) < RVALUE(bRef));

	else
	{
		bool  aIsReal = IsReal(aRef);
		bool  bIsReal = IsReal(bRef);

		if ((aIsReal && (bIsReal || bIsInt))
		||  (bIsReal && (aIsReal || aIsInt)))
			return MAKEBOOLEAN(CoerceToDouble(a) < CoerceToDouble(b));

		if (ISCHAR(aRef) && ISCHAR(bRef))
			return MAKEBOOLEAN(RCHAR(aRef) < RCHAR(bRef));

		if (IsString(aRef) && IsString(bRef))
		{
			CRichString aStr(a);
			CRichString bStr(b);
			return MAKEBOOLEAN(aStr.compareSubStringCommon(bStr, 0, -1) < 0);
		}
	}

	ThrowErr(exFrames, kNSErrBadArgs);
	return NILREF;
}

Ref
FLessOrEqual(RefArg inRcvr, RefArg a, RefArg b)
{
	Ref	aRef = a;
	Ref	bRef = b;
	bool  aIsInt = ISINT(aRef);
	bool  bIsInt = ISINT(bRef);

	if (aIsInt && bIsInt)
		return MAKEBOOLEAN(RVALUE(aRef) <= RVALUE(bRef));

	else
	{
		bool  aIsReal = IsReal(aRef);
		bool  bIsReal = IsReal(bRef);

		if ((aIsReal && (bIsReal || bIsInt))
		||  (bIsReal && (aIsReal || aIsInt)))
			return MAKEBOOLEAN(CoerceToDouble(a) <= CoerceToDouble(b));

		if (ISCHAR(aRef) && ISCHAR(bRef))
			return MAKEBOOLEAN(RCHAR(aRef) <= RCHAR(bRef));

		if (IsString(aRef) && IsString(bRef))
		{
			CRichString aStr(a);
			CRichString bStr(b);
			return MAKEBOOLEAN(aStr.compareSubStringCommon(bStr, 0, -1) <= 0);
		}
	}

	ThrowErr(exFrames, kNSErrBadArgs);
	return NILREF;
}

Ref
FGreaterThan(RefArg inRcvr, RefArg a, RefArg b)
{
	Ref	aRef = a;
	Ref	bRef = b;
	bool  aIsInt = ISINT(aRef);
	bool  bIsInt = ISINT(bRef);

	if (aIsInt && bIsInt)
		return MAKEBOOLEAN(RVALUE(aRef) > RVALUE(bRef));

	else
	{
		bool  aIsReal = IsReal(aRef);
		bool  bIsReal = IsReal(bRef);

		if ((aIsReal && (bIsReal || bIsInt))
		||  (bIsReal && (aIsReal || aIsInt)))
			return MAKEBOOLEAN(CoerceToDouble(a) > CoerceToDouble(b));

		if (ISCHAR(aRef) && ISCHAR(bRef))
			return MAKEBOOLEAN(RCHAR(aRef) > RCHAR(bRef));

		if (IsString(aRef) && IsString(bRef))
		{
			CRichString aStr(a);
			CRichString bStr(b);
			return MAKEBOOLEAN(aStr.compareSubStringCommon(bStr, 0, -1) > 0);
		}
	}

	ThrowErr(exFrames, kNSErrBadArgs);
	return NILREF;
}

Ref
FGreaterOrEqual(RefArg inRcvr, RefArg a, RefArg b)
{
	Ref	aRef = a;
	Ref	bRef = b;
	bool  aIsInt = ISINT(aRef);
	bool  bIsInt = ISINT(bRef);

	if (aIsInt && bIsInt)
		return MAKEBOOLEAN(RVALUE(aRef) >= RVALUE(bRef));

	else
	{
		bool  aIsReal = IsReal(aRef);
		bool  bIsReal = IsReal(bRef);

		if ((aIsReal && (bIsReal || bIsInt))
		||  (bIsReal && (aIsReal || aIsInt)))
			return MAKEBOOLEAN(CoerceToDouble(a) >= CoerceToDouble(b));

		if (ISCHAR(aRef) && ISCHAR(bRef))
			return MAKEBOOLEAN(RCHAR(aRef) >= RCHAR(bRef));

		if (IsString(aRef) && IsString(bRef))
		{
			CRichString aStr(a);
			CRichString bStr(b);
			return MAKEBOOLEAN(aStr.compareSubStringCommon(bStr, 0, -1) >= 0);
		}
	}

	ThrowErr(exFrames, kNSErrBadArgs);
	return NILREF;
}


Ref
FLessEqualOrGreater(RefArg inRcvr, RefArg a, RefArg b)
{ return NILREF; }

Ref
FLessOrGreater(RefArg inRcvr, RefArg a, RefArg b)
{ return NILREF; }


Ref
FUnordered(RefArg inRcvr, RefArg a, RefArg b)
{
	if (NOTINT(a) && NOTINT(b))
	{
		double x = CoerceToDouble(a);
		double y = CoerceToDouble(b);
//		if (relation(x, y) == 3)	// in CarbonCore fp.h
//			return TRUEREF;
	}
	return NILREF;
}


Ref
FUnorderedLessOrGreater(RefArg inRcvr, RefArg a, RefArg b)
{
	Ref	aRef = a;
	Ref	bRef = b;
	if (ISPTR(aRef) || ISPTR(bRef))
	{
		bool  aIsReal = IsReal(aRef);
		bool  bIsReal = IsReal(bRef);

		if (aIsReal && bIsReal)
			return MAKEBOOLEAN(CDouble(a) != CDouble(b));

		if (aIsReal || bIsReal)
		{
			if (ISINT(aRef) || ISINT(bRef))
				return MAKEBOOLEAN(CoerceToDouble(a) != CoerceToDouble(b));
			return TRUEREF;
		}

		return MAKEBOOLEAN(!EQ(a, b));
	}

	return MAKEBOOLEAN(aRef != bRef);
}


Ref
FUnorderedGreaterOrEqual(RefArg inRcvr, RefArg a, RefArg b)
{ return NILREF; }

Ref
FUnorderedOrEqual(RefArg inRcvr, RefArg a, RefArg b)
{ return NILREF; }

Ref
FUnorderedOrGreater(RefArg inRcvr, RefArg a, RefArg b)
{ return NILREF; }

Ref
FUnorderedOrLess(RefArg inRcvr, RefArg a, RefArg b)
{ return NILREF; }


#pragma mark -
/* -----------------------------------------------------------------------------
	Floating Point Environment
----------------------------------------------------------------------------- */

Ref
Ffeclearexcept(RefArg inRcvr, RefArg inArg)
{ return NILREF; }

Ref
Ffegetenv(RefArg inRcvr)
{ return NILREF; }

Ref
Ffegetexcept(RefArg inRcvr, RefArg inArg)
{ return NILREF; }

Ref
Ffeholdexcept(RefArg inRcvr, RefArg inArg)
{ return NILREF; }

Ref
Fferaiseexcept(RefArg inRcvr, RefArg inArg)
{ return NILREF; }

Ref
Ffesetenv(RefArg inRcvr, RefArg inArg)
{ return NILREF; }

Ref
Ffesetexcept(RefArg inRcvr, RefArg a, RefArg b)
{ return NILREF; }

Ref
Ffetestexcept(RefArg inRcvr, RefArg inArg)
{ return NILREF; }

Ref
Ffeupdateenv(RefArg inRcvr, RefArg inArg)
{ return NILREF; }


Ref
Ffegetround(RefArg inRcvr)
{ return NILREF; }

Ref
Ffesetround(RefArg inRcvr, RefArg inRound)
{ return NILREF; }


#pragma mark -
/* -----------------------------------------------------------------------------
	Financial
----------------------------------------------------------------------------- */

Ref
Fannuity(RefArg inRcvr, RefArg a, RefArg b)
{
	double x = CoerceToDouble(a);
	double y = CoerceToDouble(b);
	return NILREF;//MakeReal(annuity(x,y));
}

Ref
Fcompound(RefArg inRcvr, RefArg a, RefArg b)
{
	double x = CoerceToDouble(a);
	double y = CoerceToDouble(b);
	return NILREF;//MakeReal(compound(x,y));
}

