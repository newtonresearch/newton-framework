/*
	File:		TextStyles.h

	Contains:	Text style declarations.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__TEXTSTYLES_H)
#define __TEXTSTYLES_H 1

#include "Objects.h"
#include "QDPatterns.h"

/*------------------------------------------------------------------------------
	T e x t   S t y l e
------------------------------------------------------------------------------*/

struct StyleRecord
{
						StyleRecord();
						~StyleRecord();

	RefStruct		fontFamily;		// +00
	float				fontSize;		// +04	was Fixed
	ULong				fontFace;		// +08
	Ref				fontPattern;	// +0C	x1C as integer Ref
	int				x10;				// +10
	int				x14;				// +14
	int				x18;				// +18
	PatternHandle	x1C;				// +1C	also fontPattern
};

inline StyleRecord::StyleRecord()
	: x1C(NULL)
{ }

inline StyleRecord::~StyleRecord()
{ /*if (x1C) FreeHandle((Handle)x1C);*/ }


/*------------------------------------------------------------------------------
	T e x t O p t i o n s
------------------------------------------------------------------------------*/

struct TextOptions
{
	TextOptions();

	float		alignment;			// +00	was Fract
	float		justification;		// +04	was Fract
	float		width;				// +08	was Fixed
	int		x0C;
	int		transferMode;		// +10
	int		x14;
	int		x18;
};

inline TextOptions::TextOptions()  { memset(this, 0, sizeof(TextOptions)); }


/*------------------------------------------------------------------------------
	T e x t B o u n d s I n f o
------------------------------------------------------------------------------*/

struct TextBoundsInfo
{
	int		x00;		// don’t really know
	float		width;	//+14
};


/*------------------------------------------------------------------------------
	F o n t I n f o
------------------------------------------------------------------------------*/

#ifndef __QUICKDRAWTEXT__
struct FontInfo
{
	short		ascent;
	short		descent;
	short		widMax;
	short		leading;
};
#endif


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern void		CreateTextStyleRecord(RefArg inStyle, StyleRecord * ioRec);

extern void		GetStyleFontInfo(StyleRecord * inStyle, FontInfo * outFontInfo);

extern float	ConvertToFlush(ULong inViewJustify, float * outJustification);		// was Fract


#endif	/* __TEXTSTYLES_H */
