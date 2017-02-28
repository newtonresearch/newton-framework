/*
	File:		Unit.h

	Contains:	 Unit declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__UNIT_H)
#define __UNIT_H 1

#include "SIUnit.h"
#include "Stroke.h"

class CView;
class CWordList;
class CGeneralShapeUnit;
class CStdWordUnit;

/*--------------------------------------------------------------------------------
	C U n i t
	Was TUnitPublic.
--------------------------------------------------------------------------------*/

class CUnit
{
public:
					CUnit(CRecUnit * inUnit, ULong inContextId);
					~CUnit();

	CSIUnit *	siUnit(void) const;
	ULong			contextId(void);
	void			bounds(Rect * outRect);
	ULong			startTime(void);
	ULong			endTime(void);

	int			gestureAngle(void);
	Point			gesturePoint(ArrayIndex index);
	ArrayIndex	countGesturePoints(void);	// was CountGesturePoints(CUnit *)

	bool			isTap(void);
	RecType		getType(void);
	int			caretType(void);
	PolygonShape *	cleanShape(void);
	PolygonShape *	roughShape(void);
	ULong			shapeType(void);

	CStroke *	stroke(void);
	Ref			strokes(void);

	Ref			trainingData(void);

	void			setWordBase(void);
	CWordList *	makeWordList(bool, bool);
	CWordList *	extractWords(void);
	void *		word(void);
	CWordList *	words(void);
	ULong			wordScore(void);
	Ref			wordInfo(void);
	void			cleanUp(void);

	ULong			inputMask(void);
	ULong			requiredMask(void);

	void			setViewHit(CView * inView, ULong inId);
	CView *		findView(ULong inId);

	void			invalidate(void);

private:
	CSIUnit *			f00;	// +00	sub/interpretation unit -- CGeneralShapeUnit etc.
	CWordList *			f04;
	
	PolygonShape *		f14;	// +14	was PolyHandle
	PolygonShape *		f18;	// +18	was PolyHandle
	CStroke *			f1C;
	
	Rect					f28;
	RefStruct			f30;
	CView *				fViewHit;		//+34
	ULong					fViewHitFlags;	//+38
// size +3C
};

inline CSIUnit *	CUnit::siUnit(void) const  { return f00; }


#endif	/* __UNIT_H */
