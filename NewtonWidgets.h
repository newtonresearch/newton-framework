/*
	File:		NewtonWidgets.h

	Contains:	Handy stuff.

	Written by:	Newton Research Group.
*/

#if !defined(__NEWTONWIDGETS_H)
#define __NEWTONWIDGETS_H 1

#if !defined(__NEWTONTYPES_H)
#include "NewtonTypes.h"
#endif

#if !defined(EOF)
#define EOF (-1)
#endif

#if !defined(EOM)
#define EOM (-2)
#endif

#if defined(__cplusplus)
enum PositionMode 
{
	kPosBeg = -1,
	kPosCur = 0,
	kPosEnd = 1
};
#endif


//--------------------------------------------------------------------------------
//		Byte swapping
//--------------------------------------------------------------------------------

// Watch out for these macros...they can silently turn your values into different types...

#define BYTE_SWAP_CHAR(n) (n)
#define BYTE_SWAP_SHORT(n) (((n << 8) & 0xFF00) | ((n >> 8) & 0x00FF))
#define BYTE_SWAP_LONG(n) (((n << 24) & 0xFF000000) | ((n <<  8) & 0x00FF0000) | ((n >>  8) & 0x0000FF00) | ((n >> 24) & 0x000000FF))

// the following macros are for code with static data that
// assumes ordering different from native ordering

#if defined(hasByteSwapping)
#define CANONICAL_LONG BYTE_SWAP_LONG
#define CANONICAL_SHORT BYTE_SWAP_SHORT
#define CANONICAL_CHAR BYTE_SWAP_CHAR
#else
#define CANONICAL_LONG(n) (n)
#define CANONICAL_SHORT(n) (n)
#define CANONICAL_CHAR(n) (n)
#endif

void ByteSwap(void * p, int count, int swapSize);
	// implementation of ByteSwap is in UtilityClasses:Widgets.c


//--------------------------------------------------------------------------------
//		Comparison Macros
//--------------------------------------------------------------------------------

#if !defined(ODD)
#define ODD(x)    ( (x) & 1 )
#endif
#if !defined(EVEN)
#define EVEN(x)   ( !((x) & 1) )
#endif
#if !defined(ABS)
#define ABS(a)    ( ((a) <  0) ? -(a) : (a) )
#endif
#if !defined(MAX)
#define MAX(a, b) ( ((a) > (b)) ? (a) : (b) )
#endif
#if !defined(MIN)
#define MIN(a, b) ( ((a) < (b)) ? (a) : (b) )
#endif
#if !defined(MINMAX)
#define MINMAX(min, expr, max) ( MIN(MAX(min, expr), max) )
#endif


//--------------------------------------------------------------------------------
//		Bitwiddling Macros
//--------------------------------------------------------------------------------

//	BIT(N)		the Nth bit set
//	BITS(N,M)	bits N through M set, inclusive.  N > M  (e.g. "BITS(31,0)").

#define BIT(N)		(1<<(N))								/* bit N				*/
#define BITS(N,M)	((BIT((M)-(N)+1)-1)<<(N))		/* bits N..M inclusive	*/

#define MASKTEST(data, mask)   ( (data) & (mask) )
#define MASKSET(data, mask)    ( (data) | (mask) )
#define MASKCLEAR(data, mask)  ( (data) & (~(mask)) )
#define MASKTOGGLE(data, mask) ( ((data) & (mask)) ? ((data) & (~(mask))) : ((data) | (mask)) )

#define FLAGTEST(data, bit)	 ( (data & bit) != 0 )
#define FLAGSET(data, bit)		 ( data = data | (bit) )
#define FLAGCLEAR(data, bit)	 ( data = data & ~(mask) )


//--------------------------------------------------------------------------------
//		Alignment Macros
//--------------------------------------------------------------------------------

// some useful sizes

#define KByte					(1024)
#define MByte					(1024*1024)

//	ARM MMU Sizes

#define	kSubPageSize		KByte
#define	kSubPagesPerPage	(4)
#define	kPageSize			(4 * KByte)
#define	kBigPageSize		(64 * KByte)
#define	kBigSubPageSize	(16 * KByte)
#define	kSectionSize		MByte

//	ALIGN(amount, boundary)	round 'amount' up to nearest 'boundary' (which
//									must be a power of two)
//
//	WORDALIGN(amount)			round 'amount' up to a multiple of two
//
//	LONGALIGN(amount)			round 'amount' up to a multiple of four
//
//	PAGEALIGN(amount)			ditto, for 4K and 1K (ARM)
//	SUBPAGEALIGN(amount)

#define TRUNC(N,B)			(((long)(N)) & ~((long)(B)-1L))
#define ALIGN(N,B)			((((long)(N))+(long)(B)-1)&~((long)(B)-1L))
#define WORDALIGN(n)			ALIGN((n),2L)
#define LONGALIGN(n)			ALIGN((n),4L)
#define PAGEALIGN(n)			ALIGN((n),kPageSize)
#define SUBPAGEALIGN(n)		ALIGN((n),kSubPageSize)
#define ALIGNED(n,B)			(((long)(n)) == ALIGN(((long)(n)),(long)(B)))
#define MISALIGNED(n)		(((long)(n) & 3L) != 0)

#define PAGEOFFSET(n)		((long)(n) & (kPageSize-1))


#endif	/* __NEWTONWIDGETS_H */
