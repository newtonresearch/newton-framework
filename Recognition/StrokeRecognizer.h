/*
	File:		StrokeRecognizer.h

	Contains:	Class CStrokeUnit, a subclass of CSIUnit.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__STROKERECOGNIZER_H)
#define __STROKERECOGNIZER_H 1

#include "SIUnit.h"
#include "InkGeometry.h"


/* -----------------------------------------------------------------------------
	C S t r o k e D o m a i n
----------------------------------------------------------------------------- */

class CStrokeDomain : public CRecDomain
{
public:
	static CStrokeDomain *	make(CController * inController);

	bool				group(CRecUnit * inUnit, RecDomainInfo * info);
	void				classify(CRecUnit * inUnit);

protected:
	NewtonErr		iStrokeDomain(CController * inController);
};


/* -----------------------------------------------------------------------------
	C S t r o k e U n i t
----------------------------------------------------------------------------- */

class CStrokeUnit	: public CSIUnit
{
public:
	static CStrokeUnit *	make(CRecDomain*, ULong, CRecStroke*, CArray*);
	virtual void	dealloc(void);

// debug
	size_t			sizeInBytes(void);
	void				dump(CMsg * outMsg);

	ULong				countStrokes(void);
	bool				ownsStroke(void);
	CRecStroke *	getStroke(ULong index);
	CRecUnitList *	getAllStrokes(void);

	void				setContextId(ULong inId);
	ULong				contextId(void);

	CArray *			getPts(void);
	bool				isCircle(FPoint*, float*, ULong*);
	bool				isEllipse(FPoint*, long*, long*, long*, ULong*);

	ULong				endTime(void) const;
	CRecStroke *	getStroke(void) const;

protected:
	NewtonErr		iStrokeUnit(CRecDomain*, ULong, CRecStroke*, CArray*);

	ULong				fContextId;		// +3C
	CRecStroke *	fStroke;			// +40
// size +44
};

inline ULong			CStrokeUnit::endTime(void) const  { return fStroke->endTime(); }
inline CRecStroke *	CStrokeUnit::getStroke(void) const  { return fStroke; }



#endif	/* __STROKERECOGNIZER_H */
