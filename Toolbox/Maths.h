/*
	File:		Maths.h

	Contains:	Mathematical functions.

	Written by:	Newton Research Group.
*/

#if !defined(__MATHS_H)
#define __MATHS_H 1

#include <math.h>
#include <float.h>

#if defined(__cplusplus)
extern "C" {
#endif

Ref	NumberAdd(RefArg var1, RefArg var2);
Ref	NumberSubtract(RefArg var1, RefArg var2);

Ref	FAdd(RefArg inRcvr, RefArg a, RefArg b);
Ref	FSubtract(RefArg inRcvr, RefArg a, RefArg b);
Ref	FMultiply(RefArg inRcvr, RefArg a, RefArg b);
Ref	FDivide(RefArg inRcvr, RefArg a, RefArg b);
Ref	FDiv(RefArg inRcvr, RefArg a, RefArg b);
Ref	FMod(RefArg inRcvr, RefArg a, RefArg b);

Ref	FBoolAnd(RefArg inRcvr, RefArg a, RefArg b);
Ref	FBoolOr(RefArg inRcvr, RefArg a, RefArg b);
Ref	FNot(RefArg inRcvr, RefArg a);

Ref	FBitAnd(RefArg inRcvr, RefArg a, RefArg b);
Ref	FBitOr(RefArg inRcvr, RefArg a, RefArg b);
Ref	FBitXor(RefArg inRcvr, RefArg a, RefArg b);
Ref	FBitNot(RefArg inRcvr, RefArg a);

Ref	FNegate(RefArg inRcvr, RefArg a);

// Utilities - Integer Math
Ref	Fabs(RefArg inRcvr, RefArg inArg);
Ref	Fceiling(RefArg inRcvr, RefArg inArg);
Ref	Ffloor(RefArg inRcvr, RefArg inArg);
Ref	FMax(RefArg inRcvr, RefArg a, RefArg b);
Ref	FMin(RefArg inRcvr, RefArg a, RefArg b);
Ref	FLShift(RefArg inRcvr, RefArg a, RefArg inShift);
Ref	FRShift(RefArg inRcvr, RefArg a, RefArg inShift);
Ref	FReal(RefArg inRcvr, RefArg inArg);
Ref	FRandom(RefArg inRcvr, RefArg a, RefArg b);
Ref	FSetRandomSeed(RefArg inRcvr, RefArg inArg);
Ref	FSetRandomState(RefArg inRcvr, RefArg inArg);
Ref	FGetRandomState(RefArg inRcvr);

// Utilities - Floating Point Math
Ref	Fisfinite(RefArg inRcvr, RefArg inArg);
Ref	FIsFiniteNumber(RefArg inRcvr, RefArg inNum);
Ref	Fisnan(RefArg inRcvr, RefArg inArg);
Ref	Fisnormal(RefArg inRcvr, RefArg inArg);

Ref	Ffabs(RefArg inRcvr, RefArg inArg);
Ref	Fround(RefArg inRcvr, RefArg inArg);
Ref	Ftrunc(RefArg inRcvr, RefArg inArg);
Ref	Ffdim(RefArg inRcvr, RefArg a, RefArg b);
Ref	Ffmax(RefArg inRcvr, RefArg a, RefArg b);
Ref	Ffmin(RefArg inRcvr, RefArg a, RefArg b);
Ref	Ffmod(RefArg inRcvr, RefArg a, RefArg b);
Ref	Fremainder(RefArg inRcvr, RefArg a, RefArg b);
Ref	Fremquo(RefArg inRcvr, RefArg a, RefArg b);
Ref	Fnearbyint(RefArg inRcvr, RefArg inArg);
Ref	Fnextafterd(RefArg inRcvr, RefArg a, RefArg b);
Ref	Frint(RefArg inRcvr, RefArg inArg);
Ref	Frinttol(RefArg inRcvr, RefArg inArg);
Ref	Fscalb(RefArg inRcvr, RefArg a, RefArg b);
Ref	Fsignum(RefArg inRcvr, RefArg inArg);
Ref	Fsignbit(RefArg inRcvr, RefArg inArg);

Ref	Fcopysign(RefArg inRcvr, RefArg a, RefArg b);
Ref	Ferf(RefArg inRcvr, RefArg inArg);
Ref	Ferfc(RefArg inRcvr, RefArg inArg);
Ref	Fhypot(RefArg inRcvr, RefArg a, RefArg b);
Ref	Fgamma(RefArg inRcvr, RefArg inArg);
Ref	Flgamma(RefArg inRcvr, RefArg inArg);
Ref	Flog(RefArg inRcvr, RefArg inArg);	// undocumented
Ref	Flogb(RefArg inRcvr, RefArg inArg);
Ref	Flog1p(RefArg inRcvr, RefArg inArg);
Ref	Flog10(RefArg inRcvr, RefArg inArg);
Ref	Fldexp(RefArg inRcvr, RefArg a, RefArg b);
Ref	Fexp(RefArg inRcvr, RefArg inArg);
Ref	Fexpm1(RefArg inRcvr, RefArg inArg);
Ref	Fsqrt(RefArg inRcvr, RefArg inArg);
Ref	Fpow(RefArg inRcvr, RefArg a, RefArg b);
Ref	Frandomx(RefArg inRcvr, RefArg inArg);

Ref	Fsin(RefArg inRcvr, RefArg inArg);
Ref	Fsinh(RefArg inRcvr, RefArg inArg);
Ref	Fcos(RefArg inRcvr, RefArg inArg);
Ref	Fcosh(RefArg inRcvr, RefArg inArg);
Ref	Ftan(RefArg inRcvr, RefArg inArg);
Ref	Ftanh(RefArg inRcvr, RefArg inArg);

Ref	Facos(RefArg inRcvr, RefArg inArg);
Ref	Facosh(RefArg inRcvr, RefArg inArg);
Ref	Fasin(RefArg inRcvr, RefArg inArg);
Ref	Fasinh(RefArg inRcvr, RefArg inArg);
Ref	Fatan(RefArg inRcvr, RefArg inArg);
Ref	Fatan2(RefArg inRcvr, RefArg a, RefArg b);
Ref	Fatanh(RefArg inRcvr, RefArg inArg);

Ref	FLessEqualOrGreater(RefArg inRcvr, RefArg a, RefArg b);
Ref	FLessOrGreater(RefArg inRcvr, RefArg a, RefArg b);

Ref	FEqual(RefArg inRcvr, RefArg a, RefArg b);
Ref	FLessThan(RefArg inRcvr, RefArg a, RefArg b);
Ref	FLessOrEqual(RefArg inRcvr, RefArg a, RefArg b);
Ref	FGreaterThan(RefArg inRcvr, RefArg a, RefArg b);
Ref	FGreaterOrEqual(RefArg inRcvr, RefArg a, RefArg b);

Ref	FUnordered(RefArg inRcvr, RefArg a, RefArg b);
Ref	FUnorderedLessOrGreater(RefArg inRcvr, RefArg a, RefArg b);
Ref	FUnorderedGreaterOrEqual(RefArg inRcvr, RefArg a, RefArg b);
Ref	FUnorderedOrEqual(RefArg inRcvr, RefArg a, RefArg b);
Ref	FUnorderedOrGreater(RefArg inRcvr, RefArg a, RefArg b);
Ref	FUnorderedOrLess(RefArg inRcvr, RefArg a, RefArg b);

// Utilities - Floating Point Environment
Ref	Ffeclearexcept(RefArg inRcvr, RefArg inArg);
Ref	Ffegetenv(RefArg inRcvr);
Ref	Ffegetexcept(RefArg inRcvr, RefArg inArg);
Ref	Ffeholdexcept(RefArg inRcvr, RefArg inArg);
Ref	Fferaiseexcept(RefArg inRcvr, RefArg inArg);
Ref	Ffesetenv(RefArg inRcvr, RefArg inArg);
Ref	Ffesetexcept(RefArg inRcvr, RefArg a, RefArg b);
Ref	Ffetestexcept(RefArg inRcvr, RefArg inArg);
Ref	Ffeupdateenv(RefArg inRcvr, RefArg inArg);

Ref	Ffegetround(RefArg inRcvr);
Ref	Ffesetround(RefArg inRcvr, RefArg inRound);

// Utilities - Financial
Ref	Fannuity(RefArg inRcvr, RefArg a, RefArg b);
Ref	Fcompound(RefArg inRcvr, RefArg a, RefArg b);

#if defined(__cplusplus)
}
#endif


#endif	/* __MATHS_H */
