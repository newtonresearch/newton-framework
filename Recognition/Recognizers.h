/*
	File:		Recognizers.h

	Contains:	CRecognizer declarations.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__RECOGNIZERS_H)
#define __RECOGNIZERS_H 1


/* -----------------------------------------------------------------------------
	C R e c o g n i z e r
----------------------------------------------------------------------------- */

class CRecognizer
{
public:
							CRecognizer();
	virtual				~CRecognizer();

	virtual void		init(CRecDomain * inDomain, ULong inId, ULong inCmd, UChar inFlags, ULong inArbTime);
	virtual void		initServices(ULong inPossible, ULong inEnabled);
	virtual CRecDomain *	domain(void);
	virtual ULong		id(void);
	virtual ULong		command(void);
	virtual UChar		flags(void);
	virtual bool		testFlags(UChar inMask);
	virtual ULong		servicesPossible(void);
	virtual ULong		servicesEnabled(void);
	virtual ULong		unitConfidence(CUnit * inUnit);
	virtual void		sleep(void);
	virtual void		wakeUp(void);
	virtual ULong		arbitrateTime(void);
	virtual void		buildConfig(RefArg, CView *, ULong);
	virtual int			enableArea(CRecArea * inArea, RefArg inArg2);
	virtual int			configureArea(CRecArea * inArea, RefArg inArg2);
	virtual ULong		handleUnit(CUnit * inUnit);
	virtual Ref			getLearningData(CUnit * inUnit);
	virtual void		doLearning(RefArg, long);

private:
	CRecDomain *	fDomain;			// +04
	ULong				fId;				// +08
	ULong				fCmd;				// +0C	aeCode
	UChar				fFlags;			// +10
	ULong				fArbTime;		// +14
	ULong				fPossibleServices;	// +18
	ULong				fEnabledServices;		// +1C
// size+20
};


/* -----------------------------------------------------------------------------
	C E v e n t R e c o g n i z e r
----------------------------------------------------------------------------- */

class CEventRecognizer : public CRecognizer
{
public:
	ULong				id(void);
	ULong				handleUnit(CUnit * inUnit);
};


/* -----------------------------------------------------------------------------
	C C l i c k R e c o g n i z e r
----------------------------------------------------------------------------- */

class CClickRecognizer : public CRecognizer
{
public:
	ULong				handleUnit(CUnit * inUnit);
};


/* -----------------------------------------------------------------------------
	C S c r u b R e c o g n i z e r
----------------------------------------------------------------------------- */

class CScrubRecognizer : public CRecognizer
{
public:
	ULong				handleUnit(CUnit * inUnit);
};


/* -----------------------------------------------------------------------------
	C S h a p e R e c o g n i z e r
----------------------------------------------------------------------------- */

class CShapeRecognizer : public CRecognizer
{
public:
	ULong				handleUnit(CUnit * inUnit);
};


/*------------------------------------------------------------------------------
	C R e c o g n i z e r L i s t
------------------------------------------------------------------------------*/

class CRecognizerList : public CArray
{
public:
	static CRecognizerList *	make(void);
	NewtonErr		iRecognizerList(void);

	void				addRecognizer(CRecognizer * inRecognizer);
	CRecognizer *	getRecognizer(ArrayIndex index);
	CRecognizer *	findRecognizer(RecType inId);
};

#endif	/* __RECOGNIZERS_H */
