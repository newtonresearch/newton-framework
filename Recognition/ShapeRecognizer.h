/*
	File:		ShapeRecognizer.h

	Contains:	Class CGeneralShapeUnit, a subclass of CSIUnit.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__GENERALSHAPE_H)
#define __GENERALSHAPE_H 1

#include "SIUnit.h"
#include "InkGeometry.h"


typedef OpaqueRef GSType;

struct GSHuh
{
	CSIUnit *		x00;
	int32_t			x04;
	int32_t			x08;
	int32_t			x0C;
};

struct GSBlock
{
	char		x00[8];
	char		x08[8];
	int32_t	x50[2];
	int32_t	x58;
	GSHuh		x5C[2];
//size+7C
};


/*--------------------------------------------------------------------------------
	S h a p e I n t e r p r e t a t i o n
--------------------------------------------------------------------------------*/

struct ShapeInterpretation : public UnitInterpretation
{
	CArray *		parms;
	CDArray *	x10;
	int			x14[6];
};


/* -----------------------------------------------------------------------------
	C G e n e r a l S h a p e D o m a i n
----------------------------------------------------------------------------- */

class CGeneralShapeDomain : public CRecDomain
{
public:
	static CGeneralShapeDomain *  make(CController * inController);

	virtual NewtonErr	preGroup(CRecUnit * inUnit);
	virtual bool		group(CRecUnit * inUnit, RecDomainInfo * info);
	virtual void		classify(CRecUnit * inUnit);

protected:
	NewtonErr		iGeneralShapeDomain(CController * inController);
};


/* -----------------------------------------------------------------------------
	C G e n e r a l S h a p e U n i t
----------------------------------------------------------------------------- */

class CGeneralShapeUnit	: public CSIUnit
{
public:
	static CGeneralShapeUnit *	make(CRecDomain*, ULong, CArray*);
	virtual void		dealloc(void);

// debug
	virtual size_t		sizeInBytes(void);
	virtual void		dump(CMsg * outMsg);

	virtual ArrayIndex	interpretationCount(void);
	virtual ArrayIndex	addInterpretation(Ptr inPtr);

	virtual UnitInterpretation *  getInterpretation(ArrayIndex index);
	void					newInterpretation(CDArray * inArray);

	virtual void		doneUsingUnit(void);
	virtual void		endUnit(void);

	virtual void		setContextId(ULong inId);
	virtual ULong		contextId(void);

	CRecStroke *		getGSAsStroke(void);
	CRecStroke *		getEllipseAsStroke(void);
	CDArray *			getGeneralShape(void);
	void					setGeneralShape(CDArray * inArray);

	void					setf70(long inArg);
	void					setf74(long inArg);
	int32_t				getf74(void) const;
	int32_t				getf78(void) const;

	GSBlock *			getFD(void) const;
	void					disposeFD(void);

protected:
	NewtonErr			iGeneralShapeUnit(CRecDomain*, ULong, CArray*);

	ULong			fContextId;						// +3C
	GSBlock *	f40;								// +40
	ULong			fInterpretationCount;		// +44
	ShapeInterpretation	fInterpretation;	// +48
	int32_t		f70;
	int32_t		f74;
	int32_t		f78;
// size +7C
};

inline void				CGeneralShapeUnit::setf70(long inArg)  { f70 = inArg; }
inline void				CGeneralShapeUnit::setf74(long inArg)  { f74 = inArg; }
inline int32_t			CGeneralShapeUnit::getf74(void) const  { return f74; }
inline int32_t			CGeneralShapeUnit::getf78(void) const  { return f78; }

inline GSBlock *		CGeneralShapeUnit::getFD(void) const  { return f40; }
inline void				CGeneralShapeUnit::disposeFD(void)  { free(f40); f40 = NULL; }


extern ULong		gRecognitionTimeout;


#endif	/* __GENERALSHAPE_H */
