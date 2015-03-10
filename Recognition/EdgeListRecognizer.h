/*
	File:		EdgeListRecognizer.h

	Contains:	Class CEdgeListUnit, a subclass of CSIUnit.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__EDGELIST_H)
#define __EDGELIST_H 1

#include "SIUnit.h"
#include "InkGeometry.h"


/* -----------------------------------------------------------------------------
	C E d g e L i s t D o m a i n
----------------------------------------------------------------------------- */

class CEdgeListDomain : public CRecDomain
{
public:
	static CEdgeListDomain *  make(CController * inController);

	virtual bool	group(CRecUnit * inUnit, RecDomainInfo * info);
	virtual void	classify(CRecUnit * inUnit);

private:
	NewtonErr		iEdgeListDomain(CController * inController);
	void				findCorners(CRecUnit * inUnit);
};


/* -----------------------------------------------------------------------------
	C E d g e L i s t U n i t
----------------------------------------------------------------------------- */

class CEdgeListUnit : public CSIUnit
{
public:
	static CEdgeListUnit *	make(CRecDomain * inDomain, ULong inUnitType, CArray * inAreas);
	virtual void		dealloc(void);

// debug
	virtual size_t		sizeInBytes(void);
	virtual void		dump(CMsg * outMsg);

	virtual ArrayIndex	interpretationCount(void);
	virtual ArrayIndex	addInterpretation(Ptr inPtr);
	virtual UnitInterpretation *  getInterpretation(ArrayIndex index);
	void						setInterpretation(CDArray*);

	virtual void		doneUsingUnit(void);
	virtual void		endUnit(void);

	CDArray *			getCorners(void);

protected:
	NewtonErr			iEdgeListUnit(CRecDomain * inDomain, ULong inUnitType, CArray * inAreas);

	CDArray *			fCorners;					// +3C
	ArrayIndex			fNumOfInterpretations;	// +40
	UnitInterpretation	fInterpretation;		// +44
//size+54
};

inline CDArray *		CEdgeListUnit::getCorners(void)  { return fCorners; }


#endif	/* __EDGELIST_H */
