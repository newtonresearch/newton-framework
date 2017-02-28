/*
	File:		SoretdArrays.cc

	Contains:	Newton sorted array functions.

	Written by:	Newton Research Group, 2010.
*/

#include "Objects.h"
#include "ObjHeader.h"
#include "ROMResources.h"
#include "RefMemory.h"
#include "Symbols.h"
#include "Arrays.h"
#include "Frames.h"
#include "Funcs.h"
#include "RichStrings.h"
#include "NewtonScript.h"
#include "ItemTester.h"


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FLFetch(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inStart, RefArg inTest, RefArg inKey);
Ref	FLSearch(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inStart, RefArg inTest, RefArg inKey);

Ref	FSort(RefArg inRcvr, RefArg ioArray, RefArg inTest, RefArg inKey);
Ref	FInsertionSort(RefArg inRcvr, RefArg ioArray, RefArg inTest, RefArg inKey);
Ref	FStableSort(RefArg inRcvr, RefArg ioArray, RefArg inTest, RefArg inKey);
Ref	FQuickSort(RefArg inRcvr, RefArg ioArray, RefArg inTest, RefArg inKey);
Ref	FShellSort(RefArg inRcvr, RefArg ioArray, RefArg inTest, RefArg inKey);

Ref	FBFetch(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inTest, RefArg inKey);
Ref	FBFetchRight(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inTest, RefArg inKey);
Ref	FBFind(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inTest, RefArg inKey);
Ref	FBFindRight(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inTest, RefArg inKey);
Ref	FBSearchLeft(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inTest, RefArg inKey);
Ref	FBSearchRight(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inTest, RefArg inKey);
Ref	FBInsert(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3, RefArg inArg4, RefArg inArg5);
Ref	FBInsertRight(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3, RefArg inArg4, RefArg inArg5);
Ref	FBDelete(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3, RefArg inArg4, RefArg inArg5);
Ref	FBDifference(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inTest, RefArg inKey);
Ref	FBIntersect(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3, RefArg inArg4, RefArg inArg5);
Ref	FBMerge(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3, RefArg inArg4, RefArg inArg5);
}


/*------------------------------------------------------------------------------
	C G e n e r a l i z e d T e s t F n V a r

	Used for testing objects for sorting and searching.
------------------------------------------------------------------------------*/

enum SortType
{
	kSortAscending,
	kSortDescending,
	kSortEqual
};

enum KeyType
{
	kKeyNil,
	kKeyPath,
	kKeyFunc
};

class CGeneralizedTestFnVar
{
public:
			CGeneralizedTestFnVar(RefArg inTest, RefArg inKey, bool inDoSearch);

	Ref	applyKey(Ref * inElement);
	Ref	applyKey(RefArg inElement);
	int	applyTest(RefArg inObj1, Ref * inObj2);
	int	applyTest(RefArg inObj1, RefArg inObj2);

private:
	int	testClosure(RefArg inObj1, RefArg inObj2);
	int	testEQClosure(RefArg inObj1, RefArg inObj2);
	int	testEQ(RefArg inObj1, RefArg inObj2);
	int	testNumbers(RefArg inNum1, RefArg inNum2);
	int	testNumsRealUtil(RefArg inNum1, RefArg inNum2);
	int	testSymbols(RefArg inSym1, RefArg inSym2);
	int	testUniChars(RefArg inCh1, RefArg inCh2);
	int	testUniStrings(RefArg inStr1, RefArg inStr2);

	int			fSortType;  // +00
	int			fKeyType;	// +04
	RefVar		fTest;		// +08
//	RefVar		fObj1;		// +0C
//	RefVar		fObj2;		// +10
	RefVar		fKey;			// +14
	RefVar		f18;			// +18
	RefVar		f1C;			// +1C
	int			(CGeneralizedTestFnVar::*fTestFn)(RefArg inObj1, RefArg inObj2);		// +20
};

inline	int	CGeneralizedTestFnVar::applyTest(RefArg inObj1, Ref * inObj2)
{ f1C = applyKey(inObj2); return (this->*fTestFn)(inObj1, f1C); }

inline	int	CGeneralizedTestFnVar::applyTest(RefArg inObj1, RefArg inObj2)
{ return (this->*fTestFn)(inObj1, inObj2); }


CGeneralizedTestFnVar::CGeneralizedTestFnVar(RefArg inTest, RefArg inKey, bool inDoSearch)
{
	if (IsSymbol(inTest))
	{
		if (EQ(inTest, SYMA(_3C)))	// '<
		{
			fTestFn = &CGeneralizedTestFnVar::testNumbers;
			fSortType = kSortAscending;
		}
		else if (EQ(inTest, SYMA(_3E)))	// '>
		{
			fTestFn = &CGeneralizedTestFnVar::testNumbers;
			fSortType = kSortDescending;
		}
		else if (EQ(inTest, SYMA(str_3C)))	// 'str<
		{
			fTestFn = &CGeneralizedTestFnVar::testUniStrings;
			fSortType = kSortAscending;
		}
		else if (EQ(inTest, SYMA(str_3E)))	// 'str>
		{
			fTestFn = &CGeneralizedTestFnVar::testUniStrings;
			fSortType = kSortDescending;
		}
		else if (EQ(inTest, SYMA(str_3D)))	// 'str=
		{
			if (!inDoSearch)
				ThrowExFramesWithBadValue(kNSErrBadArgs, inTest);
			fTestFn = &CGeneralizedTestFnVar::testUniStrings;
			fSortType = kSortEqual;
		}
		else if (EQ(inTest, SYMA(chr_3C)))	// 'chr<
		{
			fTestFn = &CGeneralizedTestFnVar::testUniChars;
			fSortType = kSortAscending;
		}
		else if (EQ(inTest, SYMA(chr_3E)))	// 'chr>
		{
			fTestFn = &CGeneralizedTestFnVar::testUniChars;
			fSortType = kSortDescending;
		}
		else if (EQ(inTest, SYMA(sym_3C)))	// 'sym<
		{
			fTestFn = &CGeneralizedTestFnVar::testSymbols;
			fSortType = kSortAscending;
		}
		else if (EQ(inTest, SYMA(sym_3E)))	// 'sym>
		{
			fTestFn = &CGeneralizedTestFnVar::testSymbols;
			fSortType = kSortDescending;
		}
		else if (EQ(inTest, SYMA(_3D)))		// '=
		{
			if (!inDoSearch)
				ThrowExFramesWithBadValue(kNSErrBadArgs, inTest);
			fTestFn = &CGeneralizedTestFnVar::testEQ;
			fSortType = kSortEqual;
		}
		else
			ThrowExFramesWithBadValue(kNSErrBadArgs, inTest);
	}
	else
	{
		fTest = inTest;
		fTestFn = (inDoSearch) ? &CGeneralizedTestFnVar::testEQClosure : &CGeneralizedTestFnVar::testClosure;
		fSortType = kSortEqual;
	}

	if (ISNIL(inKey))
		fKeyType = kKeyNil;
	else if (ISINT(inKey) || IsSymbol(inKey) || EQ(ClassOf(inKey), SYMA(pathExpr)))
	{
		fKey = inKey;
		fKeyType = kKeyPath;
	}
	else
	{
		fKey = inKey;
		fKeyType = kKeyFunc;
	}
}

Ref
CGeneralizedTestFnVar::applyKey(Ref * inElement)
{
	switch (fKeyType)
	{
	case kKeyNil:
		return *inElement;
		break;
	case kKeyPath:
		return GetFramePath(*inElement, fKey);
		break;
	case kKeyFunc:
		f18 = *inElement;
		return NSCall(fKey, f18);
		break;
	}
	return NILREF;
}

Ref
CGeneralizedTestFnVar::applyKey(RefArg inElement)
{
	switch (fKeyType)
	{
	case kKeyNil:
		return inElement;
		break;
	case kKeyPath:
		return GetFramePath(inElement, fKey);
		break;
	case kKeyFunc:
		return NSCall(fKey, inElement);
		break;
	}
	return NILREF;
}

int
CGeneralizedTestFnVar::testClosure(RefArg inObj1, RefArg inObj2)
{
//	fObj1 = *inObj1;	// original passes Ref *; we don’t need to create RefVars
//	fObj2 = *inObj2;
	return RINT(NSCall(fTest, inObj1, inObj2));
}

int
CGeneralizedTestFnVar::testEQClosure(RefArg inObj1, RefArg inObj2)
{
//	fObj1 = *inObj1;	// original passes Ref *; we don’t need to create RefVars
//	fObj2 = *inObj2;
	Ref	result = NSCall(fTest, inObj1, inObj2);
	if (ISINT(result))
		return RINT(result);
	return NOTNIL(result) ? kItemEqualCriteria : kItemGreaterThanCriteria;
}

int
CGeneralizedTestFnVar::testEQ(RefArg inObj1, RefArg inObj2)
{
	return EQ(inObj1, inObj2) ? kItemEqualCriteria : kItemGreaterThanCriteria;
}

int
CGeneralizedTestFnVar::testNumbers(RefArg inNum1, RefArg inNum2)
{
	int	result;
	if (ISINT(inNum1) && ISINT(inNum2))
		result = RINT(inNum1) - RINT(inNum2);
	else
		result = testNumsRealUtil(inNum1, inNum2);
	if (fSortType == kSortDescending)
		result = -result;
	return result;
}

int
CGeneralizedTestFnVar::testNumsRealUtil(RefArg inNum1, RefArg inNum2)
{
	double	num1 = CoerceToDouble(inNum1);
	double	num2 = CoerceToDouble(inNum2);
	int		result = kItemEqualCriteria;
	if (num1 > num2)
		result = kItemGreaterThanCriteria;
	else 	if (num1 < num2)
		result = kItemLessThanCriteria;
	return result;
}

int
CGeneralizedTestFnVar::testSymbols(RefArg inSym1, RefArg inSym2)
{
	int		result = SymbolCompareLex(inSym1, inSym2);
	if (fSortType == kSortDescending)
		result = -result;
	return result;
}

int
CGeneralizedTestFnVar::testUniChars(RefArg inCh1, RefArg inCh2)
{
	UniChar  ch1 = RCHAR(inCh1);
	UniChar  ch2 = RCHAR(inCh2);
	int		result = ch1 - ch2;
	if (fSortType == kSortDescending)
		result = -result;
	return result;
}

int
CGeneralizedTestFnVar::testUniStrings(RefArg inStr1, RefArg inStr2)
{
	CRichString str1(inStr1);
	CRichString str2(inStr2);
	int			result = str1.compareSubStringCommon(str2, 0, -1);
	if (fSortType == kSortDescending)
		result = -result;
	return result;
}


/*------------------------------------------------------------------------------
	Q u i c k S o r t
	Based on ANSI-C qsort()
------------------------------------------------------------------------------*/

inline void Exchange(Ref * a, Ref * b)		{ Ref tmp = *a; *a = *b; *b = tmp; }
inline void Exchange(RefVar a, RefVar b)	{ Ref tmp = a; a = b; b = tmp; }

void
QSUtil(Ref * inFirstElem, Ref * inLastElem, CGeneralizedTestFnVar & inTest)
{
	Ref *		lbStack[32];
	Ref *		ubStack[32];	// sp14
	int		sp;				// sp10
	Ref *		lb;				// sp0C
	Ref *		ub;				// r5
	Ref *		m;
	RefVar	leftValue;		// sp08
	RefVar	rightValue;		// sp04
	RefVar	pivotValue;		// r8

	lbStack[0] = inFirstElem;
	ubStack[0] = inLastElem;

	for (sp = 0; sp >= 0; sp--)
	{
		lb = lbStack[sp];
		ub = ubStack[sp];
		if ((ub - lb)  > 10)
		{
			// select pivot and exchange with last element
			// which does not take part in the partitioning
			Ref * right = ub - 1;		// r7
			Ref * left = lb;				// r6
			Exchange(lb + (ub - lb)/2, ub);

			leftValue = inTest.applyKey(left);
			rightValue = inTest.applyKey(right);
			if (inTest.applyTest(leftValue, rightValue) > 0)
			{
				// *left > *right
				Exchange(left, right);
				Exchange(leftValue, rightValue);
			}
			pivotValue = inTest.applyKey(ub);
			if (inTest.applyTest(pivotValue, rightValue) > 0)
			{
				// *ub > *right
				Exchange(ub, right);
				pivotValue = rightValue;
			}
			else if (inTest.applyTest(pivotValue, leftValue) < 0)
			{
				// *ub < *left
				Exchange(ub, left);
				pivotValue = leftValue;
			}
			// partition into two segments
			while (left < right)
			{
				while (++left < right
					&&  inTest.applyTest(pivotValue, left) > 0)
					;
				while (--right > left
					&&  inTest.applyTest(pivotValue, right) < 0)
					;

				Exchange(left, right);
			}
			// pivot belongs in a[left]
			Exchange(ub, left);
			m = left;

			// keep processing smallest segment, and stack largest
			if ((m - lb) > (ub - m))
			{
				// 1st segment is larger; stack it and move lower bound up to start of next segment
				lbStack[sp] = lb;
				ubStack[sp] = m - 1;
				sp++;
				lb = m + 1;
			}
			else
			{
				// 2nd segment is larger; stack it and move upper bound down to end of prev segment
				lbStack[sp] = m + 1;
				ubStack[sp] = ub;
				sp++;
				ub = m - 1;
			}
		}

		else
		{
			// for short segments do a (stable, in-place) bubble sort
			RefVar	a;		// r9
			Ref *		p;		// r6
			for (p = lb; p <= ub; p++)
			{
				a = inTest.applyKey(*p);
				for (m = p - 1; m >= inFirstElem; Exchange(m, m+1), m--)
				{
					if (inTest.applyTest(a, m) >= 0)
						break;
				}
			}
		}
	}
}


void
QSort(RefArg ioArray, CGeneralizedTestFnVar & inTest)
{
	if (!IsArray(ioArray))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, ioArray);

	LockRef(ioArray);
	unwind_protect
	{
		Ref * array = Slots(ioArray);
		QSUtil(array, array + Length(ioArray) - 1, inTest);
	}
	on_unwind
		UnlockRef(ioArray);
	end_unwind;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Sort an array.
	In:		ioArray		the array
				inTest		'|<|, '|>|, '|str<|, '|str>|,
								or any function object returning -1,0,1 (as strcmp)
				inKey			NILREF (use the element directly),
								or a path,
								or any function object
	Return:	void			(the array object is modified)
------------------------------------------------------------------------------*/

void
SortArray(RefArg ioArray, RefArg inTest, RefArg inKey)
{
	CGeneralizedTestFnVar testFn(inTest, inKey, false);
	QSort(ioArray, testFn);
}


#pragma mark -
/* -----------------------------------------------------------------------------
	L i n e a r   S e a r c h
----------------------------------------------------------------------------- */

/*------------------------------------------------------------------------------
	Not strictly sorted, but uses CGeneralizedTestFnVar.
	Perform a linear search on an array.
	In:		inArray
				inItem
				inStart
				inTest
				inKey
	Return:	long		index of the found item; -1 if not found
------------------------------------------------------------------------------*/

int
LSearch(RefArg inArray, RefArg inItem, RefArg inStart, RefArg inTest, RefArg inKey)
{
	if (!IsArray(inArray))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, inArray);

	int	arrayLen = Length(inArray);
	int	startIndex = RINT(inStart);
	int	index = -1;

	if (startIndex >= arrayLen)
		return -1;
	if (startIndex < 0)
		ThrowExFramesWithBadValue(kNSErrOutOfBounds, inStart);

	if (IsSymbol(inTest)
	&&  ISNIL(inKey))
	{
		Ref *	arrayStart = Slots(inArray);
		Ref *	arrayEnd = arrayStart + arrayLen;
		Ref *	arrayItem;
		if (EQ(inTest, SYMA(_3D)))	// '=
		{
			for (arrayItem = arrayStart + startIndex; arrayItem < arrayEnd; arrayItem++)
				if (EQ(*arrayItem, inItem))
				{
					index = (arrayItem - arrayStart) / sizeof(Ref);
					break;
				}
		}
		return index;
	}

	LockRef(inArray);
	unwind_protect
	{
		Ref *	arrayStart = Slots(inArray);
		Ref *	arrayEnd = arrayStart + arrayLen;
		Ref *	arrayItem;
		CGeneralizedTestFnVar testFn(inTest, inKey, true);
		for (arrayItem = arrayStart + startIndex; arrayItem < arrayEnd; arrayItem++)
		{
			if (testFn.applyTest(inItem, arrayItem) == kItemEqualCriteria)
			{
				index = (arrayItem - arrayStart) / sizeof(Ref);
				break;
			}
		}
	}
	on_unwind
		UnlockRef(inArray);
	end_unwind;
	
	return index;
}


Ref
FLFetch(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inStart, RefArg inTest, RefArg inKey)
{
	int  index = LSearch(inArray, inItem, inStart, inTest, inKey);
	if (index >= 0)
		return GetArraySlot(inArray, index);
	return NILREF;
}


Ref
FLSearch(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inStart, RefArg inTest, RefArg inKey)
{
	int  index = LSearch(inArray, inItem, inStart, inTest, inKey);
	if (index >= 0)
		return MAKEINT(index);
	return NILREF;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	B i n a r y   S e a r c h
----------------------------------------------------------------------------- */

int
BSearchLeft(RefArg inArray, RefArg inItem, CGeneralizedTestFnVar & inTestFn)
{
	if (!IsArray(inArray))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, inArray);

	int result;
	int index;
	int loBound = 0;
	int hiBound = Length(inArray) - 1;
	if (hiBound >= 0)
	{
		LockRef(inArray);
		unwind_protect
		{
			Ref * p = Slots(inArray);
			while (loBound <= hiBound)
			{
				index = (loBound + hiBound) / 2;
				result = inTestFn.applyTest(inItem, p + index);
				if (result > kItemGreaterThanCriteria)
					loBound = index + 1;
				else
					hiBound = index - 1;
			}
		}
		on_unwind
		{
			UnlockRef(inArray);
		}
		end_unwind;
	}
	return loBound;
}


int
BSearchRight(RefArg inArray, RefArg inItem, CGeneralizedTestFnVar & inTestFn)
{
	if (!IsArray(inArray))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, inArray);

	int result;
	int index;
	int loBound = 0;
	int hiBound = Length(inArray) - 1;
	if (hiBound >= 0)
	{
		LockRef(inArray);
		unwind_protect
		{
			Ref * p = Slots(inArray);
			while (loBound <= hiBound)
			{
				index = (loBound + hiBound) / 2;
				result = inTestFn.applyTest(inItem, p + index);
				if (result >= kItemGreaterThanCriteria)
					loBound = index + 1;
				else
					hiBound = index - 1;
			}
		}
		on_unwind
		{
			UnlockRef(inArray);
		}
		end_unwind;
	}
	return loBound;
}


#pragma mark -
/*------------------------------------------------------------------------------
	S o r t e d   F u n c t i o n s
------------------------------------------------------------------------------*/


void
ShellSortUtil(RefArg ioArray, CGeneralizedTestFnVar & inTest, int inGap)
{
	if (!IsArray(ioArray))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, ioArray);

	LockRef(ioArray);
	unwind_protect
	{
		// see http://faculty.simpson.edu/lydia.sinapova/www/cmsc250/LN250_Weiss/L12-ShellSort.htm
		RefVar tmp;
		RefVar a1;
		Ref * a2;
		Ref * slots = Slots(ioArray);
		Ref * slotLimit = slots + Length(ioArray);
		for (int gap = inGap; gap > 0; gap /= 3)
		{
			for (Ref * p = slots + gap; p < slotLimit; ++p)
			{
				tmp = *p;
				a1 = inTest.applyKey(tmp);
				a2 = p - gap;
				for ( ; a2 >= slots; a2 -= gap)
				{
					if (inTest.applyTest(a1, a2) >= 0)
						break;
					a2[gap] = a2[0];
				}
				a2[gap] = tmp;
			}
		}
	}
	on_unwind
	{
		UnlockRef(ioArray);
	}
	end_unwind;
}


void
MergeSort(RefArg ioArray, CGeneralizedTestFnVar & inTest)
{
	if (!IsArray(ioArray))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, ioArray);

	ArrayIndex arrayLen = Length(ioArray);
	if (arrayLen < 4)
		ShellSortUtil(ioArray, inTest, 1);	// just do an insertion sort
	else
	{
		;
		// INCOMPLETE
	}
}


Ref
FSort(RefArg inRcvr, RefArg ioArray, RefArg inTest, RefArg inKey)	// is a QuickSort
{
	CGeneralizedTestFnVar testFn(inTest, inKey, false);
	QSort(ioArray, testFn);
	return ioArray;
}


Ref
FInsertionSort(RefArg inRcvr, RefArg ioArray, RefArg inTest, RefArg inKey)
{
	CGeneralizedTestFnVar testFn(inTest, inKey, false);
	ShellSortUtil(ioArray, testFn, 1);
	return ioArray;
}


Ref
FStableSort(RefArg inRcvr, RefArg ioArray, RefArg inTest, RefArg inKey)
{
	CGeneralizedTestFnVar testFn(inTest, inKey, false);
	MergeSort(ioArray, testFn);
	return ioArray;
}


Ref
FQuickSort(RefArg inRcvr, RefArg ioArray, RefArg inTest, RefArg inKey)	// the generic Sort
{
	CGeneralizedTestFnVar testFn(inTest, inKey, false);
	QSort(ioArray, testFn);
	return ioArray;
}


Ref
FShellSort(RefArg inRcvr, RefArg ioArray, RefArg inTest, RefArg inKey)
{
	CGeneralizedTestFnVar testFn(inTest, inKey, false);
// see http://www.sorting-algorithms.com/shell-sort
	ArrayIndex n = Length(ioArray);
	int h = 1;
	while (h < n)
		h = 3*h + 1;
	ShellSortUtil(ioArray, testFn, h/3);
	return ioArray;
}


Ref
FBFetch(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inTest, RefArg inKey)
{
	CGeneralizedTestFnVar testFn(inTest, inKey, false);
	int index = BSearchLeft(inArray, inItem, testFn);
	if (index < Length(inArray))
	{
		Ref element = GetArraySlot(inArray, index);
		if (testFn.applyTest(inItem, &element) == kItemEqualCriteria)
			return element;
	}
	return NILREF;
}


Ref
FBFetchRight(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inTest, RefArg inKey)
{
	CGeneralizedTestFnVar testFn(inTest, inKey, false);
	int index = BSearchRight(inArray, inItem, testFn);
	if (index >= 0)
	{
		Ref element = GetArraySlot(inArray, index);
		if (testFn.applyTest(inItem, &element) == kItemEqualCriteria)
			return element;
	}
	return NILREF;
}


Ref
FBFind(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inTest, RefArg inKey)
{
	CGeneralizedTestFnVar testFn(inTest, inKey, false);
	int index = BSearchLeft(inArray, inItem, testFn);
	if (index < Length(inArray))
	{
		Ref element = GetArraySlot(inArray, index);
		if (testFn.applyTest(inItem, &element) == kItemEqualCriteria)
			return MAKEINT(index);
	}
	return NILREF;
}


Ref
FBFindRight(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inTest, RefArg inKey)
{
	CGeneralizedTestFnVar testFn(inTest, inKey, false);
	int index = BSearchRight(inArray, inItem, testFn);
	if (index >= 0)
	{
		Ref element = GetArraySlot(inArray, index);
		if (testFn.applyTest(inItem, &element) == kItemEqualCriteria)
			return MAKEINT(index);
	}
	return NILREF;
}


Ref
FBSearchLeft(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inTest, RefArg inKey)
{
	CGeneralizedTestFnVar testFn(inTest, inKey, false);
	return MAKEINT(BSearchLeft(inArray, inItem, testFn));
}


Ref
FBSearchRight(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inTest, RefArg inKey)
{
	CGeneralizedTestFnVar testFn(inTest, inKey, false);
	return MAKEINT(BSearchRight(inArray, inItem, testFn));
}


Ref
FBInsert(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inTest, RefArg inKey, RefArg inUniqueOnly)
{
	CGeneralizedTestFnVar testFn(inTest, inKey, false);
	RefVar key(testFn.applyKey(inItem));
	int index = BSearchLeft(inArray, key, testFn);
	if (NOTNIL(inUniqueOnly) && index < Length(inArray))
	{
		Ref element = GetArraySlot(inArray, index);
		if (testFn.applyTest(key, &element) == kItemEqualCriteria)
			return EQ(inUniqueOnly, SYMA(returnElt)) ? element : NILREF;
	}
	ArrayInsert(inArray, inItem, index);
	if (EQ(inUniqueOnly, SYMA(returnElt)))
		return inItem;
	return MAKEINT(index);
}


Ref
FBInsertRight(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inTest, RefArg inKey, RefArg inUniqueOnly)
{
	CGeneralizedTestFnVar testFn(inTest, inKey, false);
	RefVar key(testFn.applyKey(inItem));
	int index = BSearchRight(inArray, key, testFn);
	if (NOTNIL(inUniqueOnly) && index >= 0)
	{
		Ref element = GetArraySlot(inArray, index);
		if (testFn.applyTest(key, &element) == kItemEqualCriteria)
			return EQ(inUniqueOnly, SYMA(returnElt)) ? element : NILREF;
	}
	ArrayInsert(inArray, inItem, index+1);
	if (EQ(inUniqueOnly, SYMA(returnElt)))
		return inItem;
	return MAKEINT(index+1);
}


Ref
FBDelete(RefArg inRcvr, RefArg inArray, RefArg inItem, RefArg inTest, RefArg inKey, RefArg inCount)
{
	CGeneralizedTestFnVar testFn(inTest, inKey, false);
	int i, index = BSearchLeft(inArray, inItem, testFn);
	int indexLimit = Length(inArray);
	if (ISINT(inCount))
	{
		int limit = index + RINT(inCount);
		if (indexLimit > limit)
			indexLimit = limit;
	}
	for (i = index; i < indexLimit; ++i)
	{
		Ref element = GetArraySlot(inArray, i);
		if (testFn.applyTest(inItem, &element) != kItemEqualCriteria)
			break;
	}
	int count = i - index;
	ArrayRemoveCount(inArray, index, count);
	return MAKEINT(count);
}

#pragma mark -

typedef int (*GOSOP)(int, int);

Ref
GenOrderedSetOp(RefArg inArray1, RefArg inArray2, RefArg inTest, RefArg inKey, int inArg5, GOSOP inOpFn, int inArg7, int inArg8, int inArg9)
{
//sp-10
	if (!IsArray(inArray1))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, inArray1);
	if (!IsArray(inArray2))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, inArray2);

	int count, spB4, spB8, spBC, spC0;
//sp-28
	CGeneralizedTestFnVar testFn(inTest, inKey, false);	// sp04
	RefVar theArray(MakeArray(inArg7));
	LockRef(inArray1);
	LockRef(inArray2);
	LockRef(theArray);
//sp-70
	unwind_protect
	{
//sp-0C
		Ref * r6 = Slots(inArray1);
		Ref * r5 = Slots(inArray2);
		Ref * r4 = Slots(theArray);
		Ref * sp08 = r6 + Length(inArray1);
		Ref * sp04 = r5 + Length(inArray2);
		Ref * sp00 = r4 + inArg7;
		if (r6 < sp08 && r5 < sp04)
		{
//sp-0C
			RefVar r8(testFn.applyKey(r6));
			RefVar r7(testFn.applyKey(r5));
			RefVar spr08;
			RefVar spr04;
			int r9 = 0, r10 = 0, spr00 = 0;
			while (!r9)
			{
//sp-04
				spr00 = inOpFn(testFn.applyTest(r8, r7), inArg5);
				if ((spr00 & 0x01) != 0)
				{
					spr08 = r8;
					spBC = spr00 & 0x10;
					spC0 = spr00 & 0x04;
					do
					{
						if (spC0 != 0)
							*r4++ = *r6++;
						if (r6 < sp08)
							r8 = testFn.applyKey(r6);
						else
							r9 = r10 = 1;
					} while (spBC != 0 && !r10 && testFn.applyTest(spr08, r8) == 0);
				}
				if ((spr00 & 0x02) != 0)
				{
					spr08 = r7;
					spB4 = spr00 & 0x20;
					spB8 = spr00 & 0x08;
					do
					{
						if (spB8 != 0)
							*r4++ = *r5++;
						if (r5 < sp04)
							r7 = testFn.applyKey(r5);
						else
							r9 = spr00 = 1;
					} while (spB4 != 0 && !spr00 && testFn.applyTest(spr04, r7) == 0);
				}
			}
		}
		if (inArg8 && r6 < sp08)
		{
			count = (sp08 - r6) / sizeof(Ref);
			memcpy(r4, r6, count * sizeof(Ref));
			r4 += count;
		}
		if (inArg9 && r5 < sp04)
		{
			count = (sp04 - r5) / sizeof(Ref);
			memcpy(r4, r5, count * sizeof(Ref));
			r4 += count;
		}
		if (r4 < sp00)
		{
			SetLength(theArray, (r4 - Slots(theArray)) / sizeof(Ref));
		}
	}
	on_unwind
	{
		UnlockRef(inArray1);
		UnlockRef(inArray2);
		UnlockRef(theArray);
	}
	end_unwind;

	return theArray;
}


int
GOSOP_Difference(int inArg1, int inArg2)
{
	if (inArg1 < 0)
		return 0x15;
	else if (inArg1 > 0)
		return 0x22;
	return 0x33;
}

Ref
FBDifference(RefArg inRcvr, RefArg inArray1, RefArg inArray2, RefArg inTest, RefArg inKey)
{
	return GenOrderedSetOp(inArray1, inArray2, inTest, inKey, 0, GOSOP_Difference, Length(inArray1), 1, 0);
}


int
GOSOP_Intersection(int inArg1, int inArg2)
{
	int op;
	if (inArg1 < 0)
		op = 0x01;
	else if (inArg1 > 0)
		op = 0x02;
	else
		op = (inArg2 == 0) ? 0x0F : 0x07;
	if (inArg2 == 0)
		op += 0x30;
	return op;
}

Ref
FBIntersect(RefArg inRcvr, RefArg inArray1, RefArg inArray2, RefArg inTest, RefArg inKey, RefArg inUniqueOnly)
{
	int array1Len = Length(inArray1);
	int array2Len = Length(inArray2);
	bool uniquely = NOTNIL(inUniqueOnly);
	return GenOrderedSetOp(inArray1, inArray2, inTest, inKey, uniquely, GOSOP_Intersection, uniquely ? MAX(array1Len, array2Len) : array1Len + array2Len, 0, 0);
}


int
GOSOP_Merge(int inArg1, int inArg2)
{
	int op;
	if (inArg1 < 0)
		op = 0x05;
	else if (inArg1 > 0)
		op = 0x0A;
	else
		op = (inArg2 == 0) ? 0x05 : 0x07;
	if (inArg2 == 0)
		op += 0x30;
	return op;
}

Ref
FBMerge(RefArg inRcvr, RefArg inArray1, RefArg inArray2, RefArg inTest, RefArg inKey, RefArg inUniqueOnly)
{
	int array1Len = Length(inArray1);
	int array2Len = Length(inArray2);
	bool uniquely = NOTNIL(inUniqueOnly);
	return GenOrderedSetOp(inArray1, inArray2, inTest, inKey, uniquely, GOSOP_Merge, array1Len + array2Len, 1, 1);
}
