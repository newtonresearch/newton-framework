/*
	File:		ClickRecognizer.h

	Contains:	Class CClickEventUnit, a subclass of CSIUnit.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__CLICKRECOGNIZER_H)
#define __CLICKRECOGNIZER_H 1

#include "SIUnit.h"
#include "InkGeometry.h"


/* -----------------------------------------------------------------------------
	C C l i c k U n i t
----------------------------------------------------------------------------- */

class CClickUnit : public CRecUnit
{
public:
	static CClickUnit * make(CRecDomain * inDomain, ULong inType, CRecStroke * inStroke, CArray * inAreas);
	void				dealloc(void);

// debug
	void				dump(CMsg * outMsg);

// interpretation
	NewtonErr		markUnit(CRecUnitList *, ULong);

	ULong				countStrokes(void);
	bool				ownsStroke(void);
	CRecStroke *	getStroke(ArrayIndex index);

	CRecStroke *	clikStroke(void) const;

protected:
	NewtonErr		iClickUnit(CRecDomain * inDomain, ULong inType, CRecStroke * inStroke, CArray * inAreas);

	CRecStroke *	fClikStroke;		// +30
//size+34
};

inline CRecStroke *	CClickUnit::clikStroke(void) const  { return fClikStroke; }


/* -----------------------------------------------------------------------------
	C C l i c k E v e n t U n i t
----------------------------------------------------------------------------- */
// event codes
enum
{
	kProcessedClick = 1,
	kTapClick,
	kDoubleTapClick,
	kHiliteClick,
	kTapDragClick
};


class CClickEventUnit : public CSIUnit
{
public:
	static CClickEventUnit * make(CRecDomain * inDomain, ULong inType, CArray * inAreas);

// debug
	size_t		sizeInBytes(void);
	void			dump(CMsg * outMsg);

// interpretation
	int			event(void);
	void			clearEvent(void);

protected:
	NewtonErr	iClickEventUnit(CRecDomain * inDomain, ULong inType, CArray * inAreas);

	int			fEvent;		// +3C
//size+40
};


/* -----------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------------- */

extern bool		ClickInProgress(CRecUnit * inUnit);


#endif	/* __CLICKRECOGNIZER_H */
