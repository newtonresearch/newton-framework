/*
	File:		SIUnit.h

	Contains:	Class CSIUnit, a subclass of CRecUnit, provides the mechanism
					for adding subunits and interpretations.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__SIUNIT_H)
#define __SIUNIT_H 1

#include "RecUnit.h"
#include "RecDomain.h"


#define XXFAIL(expr)		if (expr != 0) { goto failed; }


/*--------------------------------------------------------------------------------
	U n i t I n t e r p r e t a t i o n
--------------------------------------------------------------------------------*/

struct UnitInterpretation
{
	ULong			label;
	ULong			score;
	float			angle;	// degrees
	CArray *		parms;
};

#define kNoLabel		0xFFFFFFFF
#define kBadScore		10000

extern bool		InitInterpretation(UnitInterpretation * interpretation, ULong inParamSize, ArrayIndex inNumOfParams);


/*--------------------------------------------------------------------------------
	C S I U n i t
--------------------------------------------------------------------------------*/

class CSIUnit : public CRecUnit
{
public:
							CSIUnit();
							~CSIUnit();

	static CSIUnit *  make(CRecDomain * inDomain, ULong inType, ULong inArg3, CArray * inAreaList, ULong inInterpSize);
	virtual void		dealloc(void);

// debug
	virtual size_t		sizeInBytes(void);
	virtual void		dump(CMsg * outMsg);

	ArrayIndex			openInterpList(void);
	void					closeInterpList(void);
	virtual ArrayIndex	interpretationCount(void);
	virtual ArrayIndex	getBestInterpretation(void);
	virtual ArrayIndex	addInterpretation(Ptr inPtr);

	virtual UnitInterpretation *  getInterpretation(ArrayIndex index);
	virtual int			checkInterpretationIndex(ArrayIndex index);
	virtual ArrayIndex	insertInterpretation(ArrayIndex index);
	virtual int			deleteInterpretation(ArrayIndex index);
//	virtual void		lockInterpretations(void);		// Handles -- no longer used
//	virtual void		unlockInterpretations(void);
	virtual void		compactInterpretations(void);
	virtual NewtonErr	interpretationReuse(ULong iNum, ULong inParamSize, ULong inNumOfParams);

	virtual void		claimUnit(CRecUnitList * inUnits);
	virtual NewtonErr	markUnit(CRecUnitList * inUnits, ULong inFlags);
	virtual void		doneUsingUnit(void);
	virtual void		endUnit(void);

	virtual ArrayIndex	subCount(void);
	virtual ArrayIndex	addSub(CRecUnit * inSubunit);
	virtual CRecUnit*	getSub(ArrayIndex index);
	virtual void		deleteSub(ArrayIndex index);
	virtual NewtonErr	endSubs(void);
	virtual CDArray *	getSubsCopy(void);

	virtual ArrayIndex	countStrokes(void);
	virtual CRecStroke * getStroke(ArrayIndex index);
	virtual CRecUnitList *  getAllStrokes(void);

	virtual ULong		getLabel(ArrayIndex index);
	virtual ULong		getScore(ArrayIndex index);
	virtual float		getAngle(ArrayIndex index);
	virtual CArray *	getParam(ArrayIndex index);

	virtual void		setLabel(ArrayIndex index, ULong inLabel);
	virtual void		setScore(ArrayIndex index, ULong inScore);
	virtual void		setAngle(ArrayIndex index, float inAngle);

protected:
	NewtonErr			iSIUnit(CRecDomain * inDomain, ULong inType, ULong inArg3, CArray * inAreaList, ULong inInterpSize);

	char				fHasSubUnits;			// +30
	bool				fHasInterpretations;	// +31
	CDArray *		fSubUnits;				// +34	A list of the subunits of which this unit is composed. Or a single unit. Or NULL.
	CDArray *		fInterpretations;		// +38	A list of the interpretations of the classified unit.
};


typedef ULong (*RecHandler)(CSIUnit *, ULong, RecDomainInfo *);


#endif	/* __SIUNIT_H */
