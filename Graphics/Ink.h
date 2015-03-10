/*
	File:		Ink.h

	Contains:	Ink declarations.

	Written by:	Newton Research Group, 2009.
*/

#if !defined(__INK_H)
#define __INK_H 1

#include "RecStroke.h"


struct PackedInkWordInfo
{
	unsigned int	x0022:10;		// width
	unsigned int	x0012:10;		// height
	unsigned int	x0002:10;
	unsigned int	x0000:2;
	unsigned int	x0422:10;
	unsigned int	x0406:16;
	unsigned int	x0400:6;
};

struct InkWordInfo
{
	long		x00;		// width
	long		x04;		// height
	long		x08;
	long		x0C;
	long		x10;
	float		x14;		// scale factor
	ULong		x18;		// line width
	long		x1C;
	long		x20;
	long		x24;
	long		x28;
	long		x2C;
	long		x30;
	long		x34;
	long		x38;
//	size +3C
};



bool		IsInk(RefArg inObj);
bool		IsRawInk(RefArg inObj);
bool		IsOldRawInk(RefArg inObj);
bool		IsInkWord(RefArg inObj);


void		ExpandPackedInkWordInfo(PackedInkWordInfo * inPackedInfo, InkWordInfo * outInfo);
void		GetPackedInkWordInfo(RefArg inkWord, PackedInkWordInfo * outInfo);
void		GetInkWordInfo(RefArg inkWord, InkWordInfo * outInfo);

void		AdjustForInk(Rect * ioBounds);

// public interface uses int parms (as opposed to float used internally)
Ref		InkCompress(CRecStroke ** inStrokes, bool inWords);
void		InkExpand(RefArg inkObj, size_t inPenSize, long inArg3, long inArg4);
void		InkMakePaths(RefArg inkObj, long inX, long inY);
void		InkDraw(RefArg inkObj, size_t inPenSize, long inX, long inY, bool inLive);
void		InkDrawInRect(RefArg inkObj, size_t inPenSize, Rect * inOriginalBounds, Rect * inBounds, bool inLive);

#endif	/* __INK_H */
