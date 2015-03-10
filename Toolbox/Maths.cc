/*
	File:		Maths.cc

	Contains:	Mathematical functions.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "Maths.h"
#include "RichStrings.h"


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

#pragma mark -

/*------------------------------------------------------------------------------
	Comparison.
------------------------------------------------------------------------------*/

Ref
FIsFiniteNumber(RefArg inRcvr, RefArg inNum)
{
	return ISINT(inNum)
		 || (IsReal(inNum) && isfinite(CDouble(inNum)));
}


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
	return MAKEBOOLEAN(NO);	// just to keep the compiler quiet
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
	return MAKEBOOLEAN(NO);	// just to keep the compiler quiet
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
	return MAKEBOOLEAN(NO);	// just to keep the compiler quiet
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
	return MAKEBOOLEAN(NO);	// just to keep the compiler quiet
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

/*------------------------------------------------------------------------------
	max and min.
------------------------------------------------------------------------------*/

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

#pragma mark -

/*------------------------------------------------------------------------------
	Char operators.
------------------------------------------------------------------------------*/

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
