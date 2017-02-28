/*
	File:		DrawInk.cc

	Contains:	Ink drawing functions.

	Changes to the original:	Fixed -> float type since we now have the hardware efficiently to handle that,
										and Quartz likes it.

	Written by:	Newton Research Group, 2007.
*/

#include "Quartz.h"
#include "Paths.h"
#include "DrawShape.h"
#include "Ink.h"

#include "Objects.h"
#include "Iterators.h"
#include "RSSymbols.h"
#include "OSErrors.h"


/*------------------------------------------------------------------------------
	C o n s t a n t s
------------------------------------------------------------------------------*/

enum
{
	kInkCoderInit = 1,
	kInkCoderRun,
	kInkCoderAddPoint,
	kInkCoderClose
};


/*------------------------------------------------------------------------------
	T y p e s
------------------------------------------------------------------------------*/

struct _CODEWORD
{
	int16_t		originalWord;
	uint16_t		codeLen;
	uint32_t		codeWord;
};


struct _CODETABLE
{
	uint16_t		size;					// SZ
	uint16_t		numOfCodewords;	// N
	int16_t		x04;					// LB		left beginning?
	int16_t		x06;					// RB		right beginning?
	int16_t		x08;					// LE		left end?
	int16_t		x0A;					// RE		right end?
	_CODEWORD	codewords[];		// W
};


struct _POINT				// NOT the same as Point!
{
	int16_t		x;
	int16_t		y;
};

struct _LPOINT
{
	int32_t		x;
	int32_t		y;
};


struct CSStrokeHeader
{
	char	stream[];		// bit stream?
};
typedef OpaqueRef CSStrokeRef;


struct tag_SKP
{
	int			isInUse;			// +00	fUseSkip
	int16_t		x04;
	int16_t		x06;
	_POINT		x08;				// CurPnt?
	_POINT		x0C;				// CurPix?
	_POINT		outPoint;		// +10	OutPnt
	int32_t		distance;		// +14	CurDist
	int32_t		prevDistance;	// +18	PrevDist
	_POINT		x1C[3];			// Pix?
	_POINT		x28[3];			// Pnt?
	int			isReady;			// +34	fReady
	int16_t		x38;
};


struct _DISPLAY
{
	float			penSize;			// +00 was short
	CGPoint		location;		// +04 was FixedPoint
	CGPoint		scale;			// +0C was FixedPoint
	int			isLive;			// +14
};


struct _DPINST					// display points instance data
{
	int			f00;			// lock
	int			f04;
	_DISPLAY *	displayParms;
	Point			lastPtOut;	//	+0C	last point drawn
	Point			lastPtIn;	//	+10	last point entered
	int			numOfPts;	// +14	number of buffered points
	Point			pts[20];		// +18
};


struct _EXPAND
{
	CGPoint		location;		// +00 was FixedPoint
	CGPoint		scale;			// +08 was FixedPoint
	float			x0C;
	Handle		x10;				// points array
	int			x14;
	int			x18;				// size of points array
	CRecStroke *	x1C;			// current stroke
	bool			x20;
	void **		x24;				// strokes array
	size_t		x28;				// number of strokes
//size +2C
};


struct _SPINST					// store points instance data
{
	int			f00;			// lock
	int			f04;
	_EXPAND *	expandParms;
// size+0C
};


struct _DCC;
typedef int (*CSDecodeProc)(short, _POINT*, _DCC*);

struct _DCC
{
	CSDecodeProc	proc;				//	lpStoreProc?
	void *			instanceData;	//	_DPINST * or _SPINST * (StorePointInstData?) instance data
	short				err;				//	+08
	int				x0C;				//	cbDecodedData?
	int				x10;				//	DecodedDataSize?
	void *			parms;			// _DISPLAY * or _EXPAND * parameters
	int				x18;
	CSStrokeHeader *	x1C;
	unsigned char *	x20;			// stroke bit stream
	int				x24;				// was isHandle flag? now unused
	int				x28;				// not referenced by decoder
	int				x2C;				// stroke stream bit index
	int				x30;				// ditto
	int				x34;				// ditto
	_LPOINT			penLocation;			// +38	CPPos
	short				scaleMultiplier;		// +40	ScMul	scaling parameters
	short				scaleDivisor;			// +42	ScDiv
	int				x44;				// KvStep2
	int				x48;				// KvStep3
	int				x4C;				// KvStepE_1
	int				x50;				// KvStepE_2
	_CODETABLE *	x54;				// pFarDX
	_CODETABLE *	x58;				// pFarDY
	_CODETABLE *	x5C;				// pNearDX
	_CODETABLE *	x60;				// pNearDY
	_CODETABLE *	x64;				// pSpX2
	_CODETABLE *	x68;				// pSpY2
	_CODETABLE *	x6C;				// pSpX3
	_CODETABLE *	x70;				// pSpY3
	short				x74;				// InMode	code book
	UShort			x76;				// OutMode
	short				isFirstStroke;	// +78	could be bool
	short				nVert;			// +7A
	int				vertX[32];		// +7C
	int				vertY[32];		// +100
	int				x184;				// SpData
	int				x188;
	int				x18C;
	int				x190;
	int				x194;
	int				x198;
	int				x19C;
	int				x1A0;
	_LPOINT			x1A4;				// LEnd
	_LPOINT			x1AC;				// REnd
	tag_SKP			skipPoint;		// +1B4	SkipPnt
// size +1F0
};
typedef OpaqueRef CSRef;
typedef OpaqueRef CSParmsRef;


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

extern _CODETABLE kCompr1, kCompr2;

static FPoint	gTabScale = { kTabScale, kTabScale };

static _CODEWORD	SEGMENT_TBL[] =
{
	{ 7, 1, 0 },
	{ 8, 1, 1 },
	{ 0, 0, 0 }
};

static _CODEWORD	STROKE_TBL[] =	
{
	{ 0, 1, 1 },
	{ 1, 2, 2 },
	{ 2, 4, 8 },
	{ 0, 0, 0 }
};


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

Ptr		CSCompress(CRecStroke ** inStrokes, int inArg2);
void		CSExpandGroup(CSStrokeHeader * inData, ULong inArg2, float inArg3, float inArg4);
void		CSRawExpandGroup(CSStrokeHeader * inData, ULong inArg2, float inArg3, float inArg4, float inArg5, float inArg6);
void		CSMakePathsGroup(CSStrokeHeader * inData, float inX, float inY);
void		CSMakePathsGroupInRect(CSStrokeHeader * inData, float inX, float inY, CGRect inRect);
void		CSDraw(CSStrokeHeader * inData, float inPenSize, CGPoint inLocation, bool inLive);
void		CSDrawInRect(CSStrokeHeader * inData, float inPenSize, CGSize inOriginalSize, CGRect inDisplayRect, bool inLive);

// internal functions use float parms
Ptr		GenericCSCompress(CRecStroke ** inStrokes, uint16_t inArg2);
Ptr		GenericCSExpandGroup(CSStrokeHeader * inData, ULong inArg2, float inArg3, float inArg4, float inArg5, float inArg6, bool inArg7);
pathsHandle *	GenericCSMakePathsGroup(CSStrokeHeader * inData, float inX, float inY, float inScaleX, float inScaleY);
void		GenericCSDraw(CSStrokeHeader * inData, float inPenSize, CGPoint inLocation,  bool inLive);
void		GenericCSDraw(CSStrokeHeader * inData, float inPenSize, CGPoint inLocation, float inScaleX, float inScaleY, bool inLive);

void		Decode(CSStrokeHeader * inData, uint16_t, void * inParms, CSDecodeProc);
_DCC *	DecoderOpen(UShort inArg1, CSStrokeRef inStroke, UShort inArg3, CSParmsRef inParms, UShort inArg5);
int		DecoderRun(_DCC * p);
int		DecoderClose(_DCC * p, int * outArg2);

int		StorePointProcDefault(short inSelector, _POINT * inPt, _DCC * inContext);
int		PGCStorePointProc(short inSelector, _POINT * inPt, _DCC * inContext);
int		BeginStroke(_EXPAND * ioData);
void		EndStroke(_EXPAND * ioData);
void		AddStrokePoint(_EXPAND * ioData, int inX, int inY);

int		PGCDrawPointProc(short inSelector, _POINT * inPt, _DCC * inContext);
void		DrawBufferedPoints(_DPINST * inPtBuffer);
	
int		ReadNewStroke(_DCC * inContext, short * outType);
int		ReadSegmentNear(_DCC * inContext, short * outType);
int		ReadShortStroke(_DCC * inContext);
int		GetSkipPoint(tag_SKP * ioSkipPoint, short inX, short inY);
int		ClearSkipPoint(tag_SKP * inSkipPoint);
void		RestoreSegment(int * inArg1, int * inArg2);
int		DecodeWord_NEW(_DCC * inContext, _CODEWORD * inCW, short * outWord);
int		DecodeWord_OLD(_DCC * inContext, _CODETABLE * inCT, short * outWord);
int		DecodeLongStroke(_DCC * inContext);
int		DecodeShortStroke(_DCC * inContext);


#pragma mark -
/*------------------------------------------------------------------------------
	S t r o k e   B u n d l e
------------------------------------------------------------------------------*/

void
DrawStrokeBundle(RefArg inStroke, Rect * inSrcRect, Rect * inDstRect)
{
// cf CView::scale()
//	CTransform	xform;
//	xform.setup(inSrcRect, inDstRect, false);

	RefVar strokes(GetFrameSlot(inStroke, SYMA(strokes)));
	for (ArrayIndex i = 0, count = Length(strokes); i < count; ++i) {
		RefVar pts(GetArraySlot(strokes, i));
		CDataPtr ptsData(pts);
		FPoint * srcPtr = (FPoint *)(char *)ptsData;

		CGPoint pt;
		CGContextBeginPath(quartz);
//		pt = xform.scale(*srcPtr);		// original does TPoint::Scale(const TTransform&)
		CGContextMoveToPoint(quartz, pt.x, pt.y);
		for (ArrayIndex j = 1, numOfPts = Length(pts) / sizeof(Point); j < numOfPts; ++j) {
			++srcPtr;
//			pt = xform.scale(*srcPtr);
			CGContextAddLineToPoint(quartz, pt.x, pt.y);
		}
		CGContextStrokePath(quartz);
	}
}

#pragma mark -
/*------------------------------------------------------------------------------
	I n k y   E x p e r i m e n t a t i o n
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------
	Make paths group.
------------------------------------------------------------------------------*/
#if 0
void
CSMakePathsGroup(CSStrokeHeader * inData, float inX, float inY)	// float <- Fixed
{
	GenericCSMakePathsGroup(inData, inX, inY, 1.0, 1.0);
}

void
CSMakePathsGroupInRect(CSStrokeHeader * inData, float inX, float inY, CGRect inRect)	// float <- Fixed
{
	GenericCSMakePathsGroup(inData, inX, inY, CGRectGetWidth(inRect)/inX, CGRectGetHeight(inRect)/inY);
}

pathsHandle *
GenericCSMakePathsGroup(CSStrokeHeader * inData, float inX, float inY, float inScaleX, float inScaleY)	// float <- Fixed
{
	pathsHandle r4, r5[] = CSRawExpandGroup(inData, 0, inX, inY, inScaleX, inScaleY);
	int r7 = 0;
	int r6;
	for (r6 = 0; (r4 = r5[r6]) != 0; r6++)
	{
		size_t r4size = GetHandleSize(r4);
		size_t r9 = r4size / 8;
		size_t r8 = (r9 + 31)/32;
		size_t sp00 = 8 + r8*4;
		if (SetHandleSize(r4, r4size + sp00) == noErr)
		{
			BlockMove(r4->f00, r4->f00 + sp00, r4size);
			r4->f00->f00 = 1;
			r4->f00->f04 = r9;
			r2 = &r4->f00->f04;
			for (i = 0; i < r8; ++i)
				*r2++ = 0;
			r4->f00->f00 &= ~0x80000000;
			r1 = (r9 - 1) & 0x1F;
			r2 = (r9 - 1) / 32;
			r0 = &r0[r2];
			r0->f04 &= ~(0x80000000 >> r1);
			r5[r7++] = r4;
		}
		DisposePaths(r4);
	}
	r5[r7] = 0;
	return r5;
}


/*------------------------------------------------------------------------------
	Compress.
------------------------------------------------------------------------------*/

Ptr
CSCompress(CRecStroke ** inStrokes, int inArg2)
{
	return GenericCSCompress(inStrokes, inArg2);
}

Ptr
GenericCSCompress(CRecStroke ** inStrokes, uint16_t inArg2)
{
	r3 = inArg2
	Ptr r10 = NULL;
	Handle r5 = 0;
	Ptr r8 = 0;
	Ptr r9 = 0;

	size_t numOfPoints = 0;			// r6
	size_t numOfStrokes = 1;		// r7
	//sp-0C
	sp08 = inArg2

	CRecStroke ** iter, * strok;
	for (iter = inStrokes; (strok = *iter) != NULL; iter++)
	{
		numOfPoints += strok->count();
		numOfStrokes++;
//		strok->lock();
	}

	XTRY
	{
		sp04 = 0x64 + numOfPoints*4;
		r6 = AllocHandle(sp04);
		XFAIL(r6 == NULL)
		r5 = EncoderOpen();
		XFAIL(r5 == NULL)
		r8 = LockHandle(r5);
		XFAIL(r8 == NULL)
		r8->x1C = PGCGetPointProc(short, _POINT*, _CDC*)
		UnlockHandle(r5);
		r8 = NULL;
		XFAIL(EncoderRun(r5) != noErr)
		XFAIL(EncoderClose(r5) == 0)
		r5 = NULL;
		r9 = LockHandle(r6);
		r0 = r1 = sp00;	// number of bits
		if (r1 < 0)
		{	r1 = -r1; r1 &= 0x07; r1 = -r1; }
		else
			r1 &= 0x07;
		if (r1 == 0)
		{
			if (r0 < 0) r0 += 7;
			r0 /= 8;
		}
		else
		{
			if (r0 < 0) r0 += 7;
			r0 /= 8;
			r0++;
		}
		r7 = r0;
		r10 = NewPtr(r7);
		XFAIL(r10 == NULL)
		memmove(r10, r9, r7);
		GlobalCompressedBytesOut = r7;	// guessing this is for debug
		GlobalCompressedBitsOut = sp00;
	}
	XENDTRY;

	for (iter = inStrokes; (strok = *iter) != NULL; iter++)
	{
//		strok->unlock();
	}

	if (r8)
		if (r5) UnlockHandle(r5);
	if (r5) DisposeHandle(r5);
	if (r9)
		if (r6) UnlockHandle(r6);
	if (r6) DisposeHandle(r6);

	return r10;
}


/*------------------------------------------------------------------------------
	Expand group.
------------------------------------------------------------------------------*/

void
CSExpandGroup(CSStrokeHeader * inData, ULong inArg2, float inArg3, float inArg4)	// float <- Fixed
{
	GenericCSExpandGroup(inData, inArg2, inArg3, inArg4, 1.0, 1.0, true);
}


void
CSRawExpandGroup(CSStrokeHeader * inData, ULong inArg2, float inArg3, float inArg4, float inArg5, float inArg6)	// float <- Fixed
{
	GenericCSExpandGroup(inData, inArg3, inArg4, inArg5, inArg6, false);
}


Ptr
GenericCSExpandGroup(CSStrokeHeader * inData, ULong inArg2, float inArg3, float inArg4, float inArg5, float inArg6, bool inArg7)	// float <- Fixed
{
	XTRY
	{
		char * r7 = NewPtr(400);	// probably 100 * sizeof()
		XFAIL(r7 == NULL)
		size_t r4 = GenericCSExpandGuts(inData, r7, inArg2, inArg3, inArg4, inArg5, inArg6, inArg7);
		char * r0 = ReallocPtr(r7, (r4+1)*4);
		XFAIL(r0 == NULL)
		r0[r4] = 0;
		return r0;
	}
	XENDTRY;
	return NULL;
}


int
GenericCSExpandGuts(CSStrokeHeader * inData, void ** inArg1, ULong inArg2, float inArg3, float inArg4, float inArg5, float inArg6, bool inArg7)	// float <- Fixed
{
	_EXPAND parms;
	parms.x00 = inArg3;
	parms.x04 = inArg4;
	parms.x08 = inArg5;
	parms.x0C = inArg6;
	parms.x10 = 0;
	parms.x14 = 0;
	parms.x18 = 0;
	parms.x1C = 0;
	parms.x24 = inArg1;
	parms.x20 = inArg7;
	Decode(inData, (inArg2 == 0 || inArg2 == 1) ? inArg2 : 0, &parms, PGCStorePointProc);
	return parms.x28;
}

#endif

/*------------------------------------------------------------------------------
	Draw at original size.
------------------------------------------------------------------------------*/

void
CSDraw(CSStrokeHeader * inData, float inPenSize, CGPoint inLocation, bool inLive)
{
	GenericCSDraw(inData, inPenSize, inLocation, inLive);
}


void
GenericCSDraw(CSStrokeHeader * inData, float inPenSize, CGPoint inLocation, bool inLive)
{
	_DISPLAY parms;
	parms.penSize = inPenSize;
	parms.location = inLocation;
	parms.scale.x = 1.0;
	parms.scale.y = 1.0;
	parms.isLive = inLive;
	Decode(inData, 0, &parms, PGCDrawPointProc);
}


/*------------------------------------------------------------------------------
	Draw scaled to fit rect.
------------------------------------------------------------------------------*/

void
CSDrawInRect(CSStrokeHeader * inData, float inPenSize, CGSize inOriginalSize, CGRect inDisplayRect, bool inLive)
{
	GenericCSDraw(inData, inPenSize, CGPointMake(inDisplayRect.origin.x, inDisplayRect.origin.y+inDisplayRect.size.height), CGRectGetWidth(inDisplayRect)/inOriginalSize.width, CGRectGetHeight(inDisplayRect)/inOriginalSize.height, inLive);
}


void
GenericCSDraw(CSStrokeHeader * inData, float inPenSize, CGPoint inLocation, float inScaleX, float inScaleY, bool inLive)
{
	_DISPLAY parms;
	parms.penSize = inPenSize;
	parms.location = inLocation;
	parms.scale.x = inScaleX;
	parms.scale.y = inScaleY;
	parms.isLive = inLive;
	Decode(inData, 0, &parms, PGCDrawPointProc);
}

#pragma mark -

/*------------------------------------------------------------------------------
	Decode points.
	Args:		inStroke
				inArg2			TDIL has no inArg2
				inParms
				inDecoder
	Return:	--
------------------------------------------------------------------------------*/

void
Decode(CSStrokeHeader * inStroke, UShort inArg2, void * inParms, CSDecodeProc inDecoder)
{
	_DCC *	context;
	if ((context = DecoderOpen(1, (CSStrokeRef)inStroke, 1, (CSParmsRef)inParms, (inArg2 == 1) ? 1 : 3)))	// TDIL says inParms->1, 1:3->3
	{
		context->proc = inDecoder;
		context->x24 = 0;		// TDIL says ((_DCC *)context)->instanceData = inParms;
		DecoderRun(context);
		DecoderClose(context, NULL);
	}
}


/*------------------------------------------------------------------------------
	Open a decoder.
	Args:		inArg1
				inStroke
				inArg3
				inParms
				inArg5
	Return:	decoder instance
------------------------------------------------------------------------------*/

_DCC *
DecoderOpen(UShort inArg1, CSStrokeRef inStroke, UShort inArg3, CSParmsRef inParms, UShort inArg5)
{
	_DCC *	context;

	XTRY
	{
		context = (_DCC *)NewPtrClear(sizeof(_DCC));	// TDIL says calloc
		XFAIL(context == NULL)

		context->err = noErr;

		XFAIL(inArg1 == 0 || inStroke == 0)
		context->x1C = (CSStrokeHeader *)inStroke;
		context->x18 = inArg1;
		context->x24 = 1;
		context->x28 = inArg1 * 8;
		context->x2C = 0;
		context->x30 = 0;
		context->x34 = 0;
		context->penLocation = (_LPOINT){0,0};

		XFAIL(inArg3 == 0 || inParms == 0)
		context->parms = (void *)inParms;
		context->x0C = inArg3;
		context->instanceData = NULL;
		context->proc = StorePointProcDefault;	// TDIL says NULL
		context->isFirstStroke = true;
		context->x76 = inArg5;

		return context;
	}
	XENDTRY;

	XDOFAIL(context)
	{
		FreePtr((Ptr)context);
	}
	XENDFAIL;

	return NULL;
}


/*------------------------------------------------------------------------------
	Run a decoder.
	Return when all strokes have been processed.
	Args:		inContext		the decoder instance
	Return:	status
------------------------------------------------------------------------------*/

int
DecoderRun(_DCC * inContext)
{
	short		strokeType;

	XTRY
	{
		XFAIL(!inContext->proc(kInkCoderInit, NULL, inContext))

		inContext->x20 = (unsigned char *)inContext->x1C;	// don’t need this, it’s a relic of the Handle implementation

		inContext->skipPoint.isInUse = true;
		inContext->skipPoint.isReady = false;
		inContext->skipPoint.x38 = -1;
		inContext->skipPoint.x06 = 8;
		inContext->skipPoint.x04 = 4;

		while (ReadNewStroke(inContext, &strokeType))
		{
			if (strokeType == 0)
				XFAIL(!DecodeLongStroke(inContext))
			else if (strokeType == 1)
				XFAIL(!DecodeShortStroke(inContext))
			else if (strokeType == 2)
				return 1;

			inContext->proc(kInkCoderRun, NULL, inContext);
		}
	}
	XENDTRY;

//	inContext->x20 = NULL;	// no need for this; nor before return 1 above
	return 0;
}


/*------------------------------------------------------------------------------
	Close a decoder instance.
	Args:		inContext		the decoder instance
				outArg2			number of points decoded?
	Return:	status
------------------------------------------------------------------------------*/

int
DecoderClose(_DCC * inContext, int * outArg2)
{
	int		result = 1;

	if (inContext->proc)
	{
		if (!inContext->proc(kInkCoderClose, NULL, inContext))
			result = 0;

		if (outArg2 != NULL)
			*outArg2 = inContext->x10;
	}
/*
	// TDIL doesn’t bother with codebooks
	if (inContext->x74 == 1)
		UnlockCodeBook(1);
	else if (inContext->x74 == 2
		  ||  inContext->x74 == 3)
		UnlockCodeBook(2);
*/
	FreePtr((Ptr)inContext);

	return result;
}


/*------------------------------------------------------------------------------
	Get n bits from the stroke stream.
------------------------------------------------------------------------------*/

unsigned int
GetNBit(_DCC * inContext, uint16_t inNumOfBits)
{
	unsigned int	bit, word = 0;

	for (ArrayIndex i = 0; i < inNumOfBits; ++i)
	{
		bit = (inContext->x20[inContext->x2C/8] >> (inContext->x2C & 0x07)) & 0x01;
		word |= (bit << i);
		inContext->x2C++;
	}
	inContext->x30 += inNumOfBits;
	inContext->x34 += inNumOfBits;

	return word;
}


/*------------------------------------------------------------------------------
	Select code book for decoding compressed stroke data.
------------------------------------------------------------------------------*/

int
DecoderSelectCodeBook(_DCC * inContext)
{
	_CODETABLE *	table = NULL;

	XTRY
	{
		if (inContext->x74 == 1)
		{
			table = &kCompr1;	// original says LockCodeBook(1);
			inContext->scaleMultiplier = 1024;
			inContext->scaleDivisor = 1;
			inContext->x44 = 0x08CC;	// 2252
			inContext->x48 = 0x0A00;	// 2560
			inContext->x4C = 0x0800;	// 2048
			inContext->x50 = 0x0800;	// 2048
		}
		else if (inContext->x74 == 2 || inContext->x74 == 3)
		{
			table = &kCompr2;	// original says LockCodeBook(2);
			inContext->scaleMultiplier = 1024;
			inContext->scaleDivisor = 1;
			inContext->x44 = 0x1D50;	// 7504
			inContext->x48 = 0x10AA;	// 4266
			inContext->x4C = 0x2000;	// 8192
			inContext->x50 = 0x2000;	// 8192
		}
		XFAIL(table == NULL)

		inContext->x54 = table;
		table = (_CODETABLE *)((char *)table + table->size);
		inContext->x58 = table;
		table = (_CODETABLE *)((char *)table + table->size);
		inContext->x5C = table;
		table = (_CODETABLE *)((char *)table + table->size);
		inContext->x60 = table;
		table = (_CODETABLE *)((char *)table + table->size);
		inContext->x64 = table;
		table = (_CODETABLE *)((char *)table + table->size);
		inContext->x68 = table;
		table = (_CODETABLE *)((char *)table + table->size);
		inContext->x6C = table;
		table = (_CODETABLE *)((char *)table + table->size);
		inContext->x70 = table;

		return 1;
	}
	XENDTRY;

	return 0;
}


/*------------------------------------------------------------------------------
	Read stroke.
------------------------------------------------------------------------------*/

int
ReadNewStroke(_DCC * inContext, short * outType)
{
	const unsigned int	kNumOfBits[4] = { 0, 8, 12, 16 };
	short	dx = 0;
	short	dy = 0;

	XTRY
	{
		if (inContext->isFirstStroke)
		{
			inContext->isFirstStroke = false;
			XFAIL(!DecodeWord_NEW(inContext, STROKE_TBL, outType))
			if (*outType == 2)
			{
				unsigned int	firstWord = GetNBit(inContext, 4);
				unsigned int	numOfBits = kNumOfBits[firstWord & 0x03];
				inContext->x74 = ((firstWord & 0x04) == 0) ? 1 : 3;
				XFAIL(!DecoderSelectCodeBook(inContext))
				dx = GetNBit(inContext, numOfBits);
				dy = GetNBit(inContext, numOfBits);
				XFAIL(inContext->err != noErr)
				XFAIL(!DecodeWord_NEW(inContext, STROKE_TBL, outType))
				XFAIL(*outType == 2)
			}
			else
			{
				// it’s a long one
				inContext->x74 = 2;
				XFAIL(!DecoderSelectCodeBook(inContext))
				dx = GetNBit(inContext, 9);
				dy = GetNBit(inContext, 9);
				XFAIL(inContext->err != noErr)
				if (*outType == 1
				&& (dx == 0x01FF || dy == 0x01FF))
				{
					XFAIL(!DecodeWord_NEW(inContext, SEGMENT_TBL, outType))
					XFAIL(*outType != 7)
					XFAIL(!DecodeWord_NEW(inContext, STROKE_TBL, outType))
					XFAIL(*outType == 2)
					inContext->penLocation.x += inContext->x50 * dx;
					inContext->penLocation.y += inContext->x50 * dy;
					XFAIL(!DecodeWord_OLD(inContext, inContext->x54, &dx))
					XFAIL(!DecodeWord_OLD(inContext, inContext->x58, &dy))
				}
			}
		}
		else
		{
			XFAIL(!DecodeWord_NEW(inContext, STROKE_TBL, outType))
			if (*outType == 2)
				return 1;
			XFAIL(!DecodeWord_OLD(inContext, inContext->x54, &dx))
			XFAIL(!DecodeWord_OLD(inContext, inContext->x58, &dy))
		}
		inContext->penLocation.x += inContext->x50 * dx;
		inContext->penLocation.y += inContext->x50 * dy;
//printf("\nnew stroke at %d, %d\n", inContext->penLocation.x/inContext->x50, inContext->penLocation.y/inContext->x50);
		return 1;
	}
	XENDTRY;

	return 0;
}


/*------------------------------------------------------------------------------
	Read segment.
------------------------------------------------------------------------------*/

int
ReadSegmentNear(_DCC * inContext, short * outType)
{
	short	dx, dy;

	XTRY
	{
		inContext->x1A4 = inContext->penLocation;

		XFAIL(!DecodeWord_OLD(inContext, inContext->x5C, &dx))
		XFAIL(!DecodeWord_OLD(inContext, inContext->x60, &dy))
		inContext->penLocation.x += inContext->x4C * dx;
		inContext->penLocation.y += inContext->x4C * dy;

		inContext->x1AC = inContext->penLocation;
//printf("segment from %d, %d to %d, %d\n", inContext->x1A4.x/inContext->x4C, inContext->x1A4.y/inContext->x4C,
//														inContext->x1AC.x/inContext->x4C, inContext->x1AC.y/inContext->x4C);

		XFAIL(!DecodeWord_OLD(inContext, inContext->x64, &dx))
		XFAIL(!DecodeWord_OLD(inContext, inContext->x68, &dy))
		inContext->x18C = inContext->x44 * dx;	// r9[2]?
		inContext->x19C = inContext->x44 * dy;	// r8[2]?

		XFAIL(!DecodeWord_OLD(inContext, inContext->x6C, &dx))
		XFAIL(!DecodeWord_OLD(inContext, inContext->x70, &dy))
		inContext->x190 = inContext->x48 * dx;	// r9[3]?
		inContext->x1A0 = inContext->x48 * dy;	// r8[3]?

		inContext->x184 = (inContext->x1A4.x + inContext->x1AC.x)/2 - inContext->x18C;
		inContext->x194 = (inContext->x1A4.y + inContext->x1AC.y)/2 - inContext->x19C;
		inContext->x188 = (inContext->x1A4.x - inContext->x1AC.x)/2 - inContext->x190;
		inContext->x198 = (inContext->x1A4.y - inContext->x1AC.y)/2 - inContext->x1A0;

		XFAIL(!DecodeWord_NEW(inContext, SEGMENT_TBL, outType))

		return 1;
	}
	XENDTRY;

	return 0;
}


/*------------------------------------------------------------------------------
	Read stroke.
------------------------------------------------------------------------------*/

int
ReadShortStroke(_DCC * inContext)
{
	short	strokeType;
	short	dx, dy;

	inContext->vertX[0] = inContext->penLocation.x;
	inContext->vertY[0] = inContext->penLocation.y;
	inContext->nVert = 1;

	while (DecodeWord_NEW(inContext, SEGMENT_TBL, &strokeType))
	{
		if (strokeType == 7)
			return 1;

		XFAIL(!DecodeWord_OLD(inContext, inContext->x5C, &dx))	// not within XTRY/XENDTRY but works anyway
		XFAIL(!DecodeWord_OLD(inContext, inContext->x60, &dy))

		inContext->penLocation.x += inContext->x4C * dx;
		inContext->penLocation.y += inContext->x4C * dy;

		inContext->vertX[inContext->nVert] = inContext->penLocation.x;
		inContext->vertY[inContext->nVert] = inContext->penLocation.y;
		inContext->nVert++;
	}

	return 0;
}


/*------------------------------------------------------------------------------
	Get skip point.
------------------------------------------------------------------------------*/

int
GetSkipPoint(tag_SKP * ioSkipPoint, short inX, short inY)
{
	if (!ioSkipPoint->isInUse)
	{
		ioSkipPoint->isReady = true;
		ioSkipPoint->outPoint.x = inX;
		ioSkipPoint->outPoint.y = inY;
		return 1;
	}

	ioSkipPoint->isReady = false;
	ioSkipPoint->x08.x = inX;			// in tablet space?
	ioSkipPoint->x08.y = inY;
	ioSkipPoint->x0C.x = inX / 8;		// in display space?
	ioSkipPoint->x0C.y = inY / 8;

	int	dx = 4 + ioSkipPoint->x0C.x * 8 - ioSkipPoint->x08.x;
	int	dy = 4 + ioSkipPoint->x0C.y * 8 - ioSkipPoint->x08.y;
	ioSkipPoint->distance = dx*dx + dy*dy;

	if (ioSkipPoint->x38 == -1)
	{
		ioSkipPoint->x38 = 0;
		ioSkipPoint->x28[0] = ioSkipPoint->x08;
		ioSkipPoint->x1C[0] = ioSkipPoint->x0C;
		ioSkipPoint->prevDistance = ioSkipPoint->distance;
		return 0;
	}

	if (ioSkipPoint->x1C[ioSkipPoint->x38].x == ioSkipPoint->x0C.x
	&&  ioSkipPoint->x1C[ioSkipPoint->x38].y == ioSkipPoint->x0C.y)
	{
		if (ioSkipPoint->distance < ioSkipPoint->prevDistance)
		{
			ioSkipPoint->x28[ioSkipPoint->x38] = ioSkipPoint->x08;
			ioSkipPoint->prevDistance = ioSkipPoint->distance;
		}
	}
	else
	{
		if (++ioSkipPoint->x38 == 3)
		{
			_POINT	d1, d2;
			ioSkipPoint->outPoint = ioSkipPoint->x28[0];
			ioSkipPoint->isReady = true;
			d1.x = ioSkipPoint->x1C[1].x - ioSkipPoint->x1C[0].x;
			d1.y = ioSkipPoint->x1C[1].y - ioSkipPoint->x1C[0].y;
			d2.x = ioSkipPoint->x1C[2].x - ioSkipPoint->x1C[1].x;
			d2.y = ioSkipPoint->x1C[2].y - ioSkipPoint->x1C[1].y;
			if (abs(d1.x) + abs(d2.x) == 1
			&&  abs(d1.y) + abs(d2.y) == 1)
			{
				ioSkipPoint->x28[0] = ioSkipPoint->x28[2];
				ioSkipPoint->x1C[0] = ioSkipPoint->x1C[2];
				ioSkipPoint->x38 = 1;
			}
			else
			{
				ioSkipPoint->x28[0] = ioSkipPoint->x28[1];
				ioSkipPoint->x1C[0] = ioSkipPoint->x1C[1];
				ioSkipPoint->x28[1] = ioSkipPoint->x28[2];
				ioSkipPoint->x1C[1] = ioSkipPoint->x1C[2];
				ioSkipPoint->x38 = 2;
			}
		}
		ioSkipPoint->x28[ioSkipPoint->x38] = ioSkipPoint->x08;
		ioSkipPoint->x1C[ioSkipPoint->x38] = ioSkipPoint->x0C;
		ioSkipPoint->prevDistance = ioSkipPoint->distance;
	}

	return ioSkipPoint->isReady;
}


/*------------------------------------------------------------------------------
	Clear skip point.
------------------------------------------------------------------------------*/

int
ClearSkipPoint(tag_SKP * inSkipPoint)
{
	if (!inSkipPoint->isInUse)
		return 0;

	if (inSkipPoint->x38 == -1)
		inSkipPoint->isReady = false;
	else
	{
		inSkipPoint->isReady = true;
		inSkipPoint->outPoint = inSkipPoint->x28[0];
		for (ArrayIndex i = 0; i < inSkipPoint->x38; ++i)
		{
			inSkipPoint->x28[i] = inSkipPoint->x28[i+1];
		}
		inSkipPoint->x38--;
	}
	return inSkipPoint->isReady;
}


/*------------------------------------------------------------------------------
	Restore segment.
------------------------------------------------------------------------------*/

void
RestoreSegment(int * inArg1, int * inArg2)
{
	int	lk = inArg2[0] >> 10;
	int	r12 = -(inArg2[1] >> 10);
	int	r2 = inArg2[2] >> 10;
	int	r3 = -(inArg2[3] >> 10);

	lk -= r2;
	r12 -= r3 * 3;
	r2 *= 2;
	r3 *= 4;
	lk <<= 16;
	r2 <<= 10;
	r3 <<= 7;
	r12 = r2 + r3 + (r12 << 13);
	r2 = r3*6 + r2*2;

	inArg1 += 8;
	*inArg1++ = lk >> 6;
	for (ArrayIndex i = 0; i < 8; ++i)
	{
		lk += r12;
		r12 += r2;
		r2 += r3;
		*inArg1++ = lk >> 6;
	}

	lk = inArg2[0] >> 10;
	r12 = inArg2[1] >> 10;
	r2 = inArg2[2] >> 10;
	r3 = inArg2[3] >> 10;

	lk -= r2;
	r12 -= r3 * 3;
	r2 *= 2;
	r3 *= 4;
	lk <<= 16;
	r2 <<= 10;
	r3 <<= 7;
	r12 = r3 + r2 + (r12 << 13);
	r2 = r3*6 + r2*2;

	inArg1 -= 10;
	for (ArrayIndex i = 0; i < 8; ++i)
	{
		lk += r12;
		r12 += r2;
		r2 += r3;
		*inArg1-- = lk >> 6;
	}
}


/*------------------------------------------------------------------------------
	Decode word.
------------------------------------------------------------------------------*/

int
DecodeWord_NEW(_DCC * inContext, _CODEWORD * inCW, short * outWord)
{
	unsigned int	bit, word = 0;

	for (ArrayIndex i = 0; ; ++i)
	{
		bit = (inContext->x20[inContext->x2C/8] >> (inContext->x2C & 0x07)) & 0x01;
		word |= (bit << i);
		inContext->x2C++;
		inContext->x30++;
		inContext->x34++;

		for (ArrayIndex j = i+1; j == inCW->codeLen; inCW++)
		{
			if (word == inCW->codeWord)
			{
				*outWord = inCW->originalWord;
				return 1;
			}
		}
		if (inCW->codeLen == 0)
		{
			*outWord = 0;
			return 0;
		}
	}
}


/*------------------------------------------------------------------------------
	Decode word.
------------------------------------------------------------------------------*/

int
DecodeWord_OLD(_DCC * inContext, _CODETABLE * inCT, short * outWord)
{
	_CODEWORD *	theCW = inCT->codewords;	// r6
	unsigned int	bit, word = 0;	// r1
	int	codewordIndex = 0;

	for (ArrayIndex i = 0; i < 32; ++i)
	{
		bit = (inContext->x20[inContext->x2C/8] >> (inContext->x2C & 0x07)) & 0x01;
		word |= (bit << i);
		inContext->x2C++;
		inContext->x30++;
		inContext->x34++;

		for (ArrayIndex j = i+1; j == theCW->codeLen; theCW++)
		{
			if (word == theCW->codeWord)
			{
				short	sp00;
				short	theWord = theCW->originalWord;	// r0
				if (theCW->originalWord == inCT->x0A)	// at the limit; recurse
				{
					if (DecodeWord_OLD(inContext, inCT, &sp00) == 0) return 0;
					theWord = inCT->x04 + sp00;
				}
				if (theCW->originalWord == inCT->x08)	// at the limit; recurse
				{
					if (DecodeWord_OLD(inContext, inCT, &sp00) == 0) return 0;
					theWord = inCT->x06 + sp00;
				}
				*outWord = theWord;
				return 1;
			}
			if (++codewordIndex == inCT->numOfCodewords)
				return 0;
		}
	}

	return 0;
}


/*------------------------------------------------------------------------------
	Decode stroke.
------------------------------------------------------------------------------*/

int
DecodeLongStroke(_DCC * inContext)
{
	short		strokeType;
	_POINT	sp04;
	_POINT	sp08;

	while (ReadSegmentNear(inContext, &strokeType))
	{
		RestoreSegment(inContext->vertX, &inContext->x184);
		RestoreSegment(inContext->vertY, &inContext->x194);

		sp08 = (_POINT){-1000,-1000};

		for (ArrayIndex i = 0; i < 16; ++i)
		{
			if (i == 0)
			{
				sp04.x = ((inContext->vertX[i] + inContext->x1A4.x)/2) >> 10;
				sp04.y = ((inContext->vertY[i] + inContext->x1A4.y)/2) >> 10;
			}
			else
			{
				sp04.x = inContext->vertX[i] >> 10;
				sp04.y = inContext->vertY[i] >> 10;
			}
			if (inContext->x76 == 1)
			{
				if (abs(sp04.x - sp08.x) > 1
				||  abs(sp04.y - sp08.y) > 1)
				{
					sp08 = sp04;
					if (inContext->proc) inContext->proc(kInkCoderAddPoint, &sp04, inContext);
				}
			}
			else
			{
				if (GetSkipPoint(&inContext->skipPoint, sp04.x, sp04.y))
				{
					sp04 = inContext->skipPoint.outPoint;

					sp08 = sp04;
					if (inContext->proc) inContext->proc(kInkCoderAddPoint, &sp04, inContext);
				}
			}
		}

		if (strokeType == 7)
		{
			sp04.x = inContext->x1AC.x >> 10;
			sp04.y = inContext->x1AC.y >> 10;
			if (inContext->x76 == 3)
			{
				if (GetSkipPoint(&inContext->skipPoint, sp04.x, sp04.y))
				{
					sp04 = inContext->skipPoint.outPoint;
					if (inContext->proc) inContext->proc(kInkCoderAddPoint, &sp04, inContext);
				}
				while (ClearSkipPoint(&inContext->skipPoint))
				{
					sp04 = inContext->skipPoint.outPoint;
					if (inContext->proc) inContext->proc(kInkCoderAddPoint, &sp04, inContext);
				}
			}
			else
			{
				if (inContext->proc) inContext->proc(kInkCoderAddPoint, &sp04, inContext);
			}

			return 1;
		}
	}

	return 0;
}


/*------------------------------------------------------------------------------
	Decode stroke.
------------------------------------------------------------------------------*/

int
DecodeShortStroke(_DCC * inContext)
{
	_POINT	pt;

	if (ReadShortStroke(inContext))
	{
		for (ArrayIndex i = 0; i < inContext->nVert; ++i)
		{
			pt.x = inContext->vertX[i] >> 10;
			pt.y = inContext->vertY[i] >> 10;
			inContext->proc(kInkCoderAddPoint, &pt, inContext);
		}
		return 1;
	}
	return 0;
}


/*------------------------------------------------------------------------------
	Decode proc for storing point data in decode context.
------------------------------------------------------------------------------*/

int
StorePointProcDefault(short inSelector, _POINT * inPt, _DCC * inParms)
{ return 0; }


#pragma mark -

/*------------------------------------------------------------------------------
	Decode proc for storing point data in decode context.
	Args:		inSelector	1 => allocate & initialize
								2 => idle? draw?
								3 => add point
								4 => free
				inPt			point to add
				inContext
	Return:	status
------------------------------------------------------------------------------*/

int
PGCStorePointProc(short inSelector, _POINT * inPt, _DCC * inContext)
{
	int			result = 0;
	_EXPAND *	expander = NULL;
	_SPINST *	instanceData = (_SPINST *)inContext->instanceData;

	if (instanceData != NULL)
		expander = instanceData->expandParms;

	switch (inSelector)
	{
	case kInkCoderInit:
		instanceData = (_SPINST *)NewPtr(sizeof(_SPINST));
		inContext->instanceData = instanceData;
		if (instanceData != NULL)
		{
			instanceData->f00 = 1;
			instanceData->f04 = 0;
			instanceData->expandParms = (_EXPAND *)inContext->parms;
			instanceData->expandParms->x28 = 0;
			instanceData->expandParms->x24[0] = NULL;
			result = 1;
		}
		break;

	case kInkCoderRun:
		if (instanceData != NULL)
		{
			if (instanceData->f00 == 0)
			{
				instanceData->f00 = 1;
				EndStroke(expander);
			}
			result = 1;
		}
		break;

	case kInkCoderAddPoint:
		if (instanceData != NULL)
		{
			if (instanceData->f04 == 0)
			{
				if (instanceData->f00 != 0)
				{
					instanceData->f00 = 0;
					if (BeginStroke(instanceData->expandParms) == 0)
						break;
				}
				
				Point	thePoint;
				float	ordinate;
				ordinate = inPt->x;
				if (ordinate < 0)
					ordinate = 0;
				ordinate = expander->location.x + (ordinate / gTabScale.x) * expander->scale.x;
				thePoint.h = ordinate;

				ordinate = inPt->y;
				if (ordinate < 0)
					ordinate = 0;
				ordinate = expander->location.y - (ordinate / gTabScale.y) * expander->scale.y;
				thePoint.v = ordinate;

				AddStrokePoint(expander, thePoint.h, thePoint.v);
			}
			result = 1;
		}
		break;

	case kInkCoderClose:
		if (instanceData != NULL)
		{
			instanceData->f00 = 1;
			if (instanceData->f04 == 0)
			{
				instanceData->f04 = 1;
				FreePtr((Ptr)instanceData);
				inContext->instanceData = NULL;
			}
			result = 1;
		}
		break;
	}

	return result;
}


int
BeginStroke(_EXPAND * ioData)
{
	XTRY
	{
		// can’t have more than 100 points in a stroke
		XFAIL(ioData->x28 >= 100)
		if (ioData->x20)
		{
			CRecStroke * strok =
#if !defined(forFramework)
			CRecStroke::make(0);
#else
			NULL;
#endif
			ioData->x1C = strok;
			XFAIL(strok == NULL)
			ioData->x24[ioData->x28] = strok;
		}
		else
		{
			Handle pt = NewHandle(0);
			ioData->x10 = pt;
			XFAIL(pt == NULL)
			ioData->x24[ioData->x28] = pt;
			ioData->x18 = 0;
			ioData->x14 = 0;
		}
		return 1;
	}
	XENDTRY;
	return 0;
}


void
EndStroke(_EXPAND * ioData)
{
	if (ioData->x20)
	{
#if !defined(forFramework)
		CRecStroke * strok = ioData->x1C;
		if (strok)
			strok->endStroke();
#endif
	}
	else
	{
		if (ioData->x18 == sizeof(_POINT))
		{
			_POINT * pt = (_POINT *)*(ioData->x10);
			AddStrokePoint(ioData, pt->x, pt->y);
		}
		SetHandleSize(ioData->x10, ioData->x18);
	}
	ioData->x28++;
	if (ioData->x28 < 100)
		ioData->x24[ioData->x28] = NULL;
}


void
AddStrokePoint(_EXPAND * ioData, int inX, int inY)	// might be Fixed
{
	XTRY
	{
		if (ioData->x20)
		{
			TabPt pt = { inX, inY /*, 0, 0*/ };
			CRecStroke * strok = ioData->x1C;
			XFAIL(strok == NULL)
#if !defined(forFramework)
			strok->addPoint(&pt);
#endif
		}
		else
		{
			XFAIL(ioData->x10 == NULL)
			int r2 = ioData->x18 + sizeof(_POINT);
			int r1 = ioData->x14;
			if (r2 > r1)
			{
//				XFAIL(SetHandleSize(ioData->x10, r1 + 0x40) != noErr)
				SetHandleSize(ioData->x10, r1 + 0x40);		// MacMemoryMgr returns void
				ioData->x14 += 0x40;
			}
			_POINT * pt = (_POINT *)(*(ioData->x10) + ioData->x18);
			pt->x = inX;
			pt->y = inY;
			ioData->x18 += sizeof(_POINT);
		}
	}
	XENDTRY;
}


/*------------------------------------------------------------------------------
	Decode proc for drawing point data in decode context.
	Args:		inSelector	1 => allocate & initialize
								2 => idle? draw?
								3 => add point
								4 => free
				inPt			point to add
				inContext
	Return:	status
------------------------------------------------------------------------------*/

int
PGCDrawPointProc(short inSelector, _POINT * inPt, _DCC * inContext)
{
	int			result = 0;
	_DISPLAY *	display = NULL;
	_DPINST *	instanceData = (_DPINST *)inContext->instanceData;

	if (instanceData != NULL)
		display = instanceData->displayParms;

	switch (inSelector)
	{
	case kInkCoderInit:
		instanceData = (_DPINST *)NewPtr(sizeof(_DPINST));
		inContext->instanceData = instanceData;
		if (instanceData != NULL)
		{
			CGContextSetLineJoin(quartz, kCGLineJoinBevel);
			instanceData->f00 = 1;
			instanceData->f04 = 0;
			instanceData->displayParms = (_DISPLAY *)inContext->parms;
			instanceData->lastPtOut = (Point){0,0};
			instanceData->numOfPts = 0;
			result = 1;
		}
		break;

	case kInkCoderRun:
		if (instanceData != NULL && display != NULL)
			instanceData->f00 = 1;
		DrawBufferedPoints(instanceData);
		result = 1;
		break;

	case kInkCoderAddPoint:
		if (instanceData != NULL && display != NULL)
		{
			if (instanceData->f04 == 0)
			{
				Point	thePoint;
				float	ordinate;
				ordinate = inPt->x;
				if (ordinate < 0)
					ordinate = 0;
				ordinate = display->location.x + (ordinate / gTabScale.x) * display->scale.x + 0.5;
				thePoint.h = ordinate;

				ordinate = inPt->y;
				if (ordinate < 0)
					ordinate = 0;
				ordinate = display->location.y - (ordinate / gTabScale.y) * display->scale.y + 0.5;
				thePoint.v = ordinate;

				if (instanceData->f00)
				{
					instanceData->f00 = 0;
//					MoveTo(thePoint.h, thePoint.v);
					instanceData->lastPtOut = (Point){-1,-1};
					instanceData->lastPtIn = (Point){-1,-1};
				}
				if (instanceData->lastPtIn.h != thePoint.h
				||  instanceData->lastPtIn.v != thePoint.v)
				{
					instanceData->pts[instanceData->numOfPts++] = thePoint;
					instanceData->lastPtIn = thePoint;
				}
				if (instanceData->numOfPts >= 20)
					DrawBufferedPoints(instanceData);
			}
			result = 1;
		}
		break;

	case kInkCoderClose:
		if (instanceData != NULL && display != NULL)
		{
			instanceData->f00 = 1;
			if (instanceData->f04 == 0)
			{
				instanceData->f04 = 1;
				FreePtr((Ptr)instanceData);
				inContext->instanceData = NULL;
			}
			result = 1;
		}
		break;
	}

	return result;
}


/*------------------------------------------------------------------------------
	Join the dots.
	Args:		inPtBuffer	struct containing points through which a stroke passed
	Return	--
------------------------------------------------------------------------------*/

void
DrawBufferedPoints(_DPINST * inPoints)
{
//printf("DrawBufferedPoints()\n");
	if (inPoints->numOfPts != 0)
	{
		ArrayIndex	index;
		Point *	lastKnownPt;
		Point *	thisPt, * nextPt;
//		Rect		inkedBox;
//		Point		pnSize;

		_DISPLAY *	display = inPoints->displayParms;
//		pnSize.h = pnSize.v = display->penSize;
		index = 0;
		thisPt = lastKnownPt = &inPoints->lastPtOut;
		if (thisPt->h == -1
		&&  thisPt->v == -1)
		{
			thisPt = inPoints->pts;
			if (inPoints->numOfPts > 1)
				index = 1;
		}
		nextPt = &inPoints->pts[index];

/*		GrafPtr thePort;
		GetPort(&thePort);
		if (!display->isLive)
			PenSize(pnSize.h, pnSize.v);
*/		SetLineWidth(display->penSize);

		CGContextBeginPath(quartz);
		CGContextMoveToPoint(quartz, thisPt->h, thisPt->v);
//printf("new path from %d, %d\n", thisPt->h, thisPt->v);

		for ( ; index < inPoints->numOfPts; ++index, ++nextPt)
		{
/*			if (display->isLive)
			{
				Inkerline(*thisPt, *nextPt, &inkedBox, pnSize.v, &thePort->portBits);
				thisPt = nextPt;
			}
			else
				LineTo(nextPt->h, nextPt->v);
*/			CGContextAddLineToPoint(quartz, nextPt->h, nextPt->v);
//printf("drawing to %d, %d\n", nextPt->h, nextPt->v);
			thisPt = nextPt;
		}
//printf("path done\n");
		CGContextStrokePath(quartz);
		inPoints->lastPtOut = *thisPt;
		inPoints->numOfPts = 0;
	}
}


#if 0
/*------------------------------------------------------------------------------
	We’re not bothering with this stuff.
------------------------------------------------------------------------------*/

struct _BOOKENTRY
{
	short	refCount;
	// size +0C
};

void *		globalCodeBookPtr;		// 0C104FC8
void *		globalCodeBookPtrInk;	// 0C104FCC
_BOOKENTRY	gBookList;					// 0C104FD0


void
InitializeParagraphCompression(void)
{
	globalCodeBookPtr = BinaryData(paragraphCodeBook1);
	globalCodeBookPtrInk = BinaryData(paragraphCodeBook2);
}


void
LockBook(const char * inName, _BOOKENTRY * ioBook)
{
	//sp-28
	if (ioBook->x08 == 0)
	{
		if (inName[4] == '1')
		
	}

	else
		ioBook->refCount++
}


void
UnlockBook(_BOOKENTRY * ioBook)
{
	if (ioBook->refCount > 0)
		ioBook->refCount--;
}


void
LockCodeBook(UShort inBook)
{
	if (inBook == 1)
		LockBook("book1", &gBookList[0]);
	else if (inBook == 2)
		LockBook("book2", &gBookList[1]);
}

void
UnlockCodeBook(UShort inBook)
{
	if (inBook == 1)
		UnlockBook(&gBookList[0]);
	else if (inBook == 2)
		UnlockBook(&gBookList[1]);
}


int
GetInkFormat(void * inData)
{
	int	inkFormat;
	int	byte = *(char *)inData;
	if ((byte & 0x0F) != 8)
		inkFormat = 2;
	else if ((byte & 0x80) != 0)
		inkFormat = 0;
	else if ((byte & 0x40) != 0)
		inkFormat = 3;
	else
		inkFormat = 1;
	return inkFormat;
}

#pragma mark -

/*------------------------------------------------------------------------------
	HWR memory.
	We’re not going to bother with handles.
	Modern memory implementations don’t use ’em -- even TDIL doesn’t.
	In any case, the handle’s locked most of the time.
------------------------------------------------------------------------------*/

typedef OpaqueRef HWRef;

static int gRecMemErrCount = 0;

HWRef
HWRMemoryAllocHandle(size_t inSize)
{
	Handle h;

	if (inSize > 300000)
		return NULL;

	h = NewHandle(sizeof(Handle) + inSize);	// NOTE NewHandle deprecated
	if (h == 0)
		gRecMemErrCount++;
	else
		NameHandle(h, 'para');

	return (HWRef) h;
}


void *
HWRMemoryLockHandle(HWRef h)
{
	void * p;
	HLock((Handle)h);
	if ((p = *(HWRef*)h) != NULL)
		*(HWRef*)p++ = h;
	return p;
}


bool
HWRMemoryUnlockHandle(HWRef h)
{
	HUnlock((Handle)h);
	return true;
}


bool
HWRMemoryFreeHandle(HWRef h)
{
	DisposeHandle((Handle)h);
	return true;
}


void *
HWRMemoryAlloc(size_t inSize)
{
	Handle h;
	void * p == NULL;

	if (inSize > 300000)
		return NULL;

	h = NewHandle(sizeof(Handle) + inSize);	// NOTE NewHandle deprecated
	if (h == NULL)
		gRecMemErrCount++;
	else
	{
		NameHandle(h, 'para');
		HLock(h);
		if ((p = *(HWRef*)h) != NULL)
			*(HWRef*)p++ = h;
		else
			DisposeHandle(h);
	}
	return p;
}


bool
HWRMemoryFree(void * p)
{
	DisposeHandle(*(Handle*)--p);
	return true;
}
#endif
