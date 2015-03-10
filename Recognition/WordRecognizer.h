/*
	File:		WordRecognizer.h

	Contains:	CWRecRecognizer and support class declarations.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__WORDRECOGNIZER_H)
#define __WORDRECOGNIZER_H 1

#include "Recognizers.h"


struct WordInterpretation : public UnitInterpretation
{
	Ptr			word;		//+0C -- original seems to reuse parms
};


/* -----------------------------------------------------------------------------
	C W R e c D o m a i n
----------------------------------------------------------------------------- */
class CWRecognizer;

class CWRecDomain : public CRecDomain
{
public:
	static CWRecDomain *  make(CController * inController);

	virtual bool	group(CRecUnit * inUnit, RecDomainInfo * info);
	virtual void	classify(CRecUnit * inUnit);
	virtual void	reclassify(CRecUnit * inUnit);

	void				configureArea(RefArg, OpaqueRef);
	ULong				unitConfidence(CSIUnit *);
	void				unitInfoFreePtr(Ptr);
	void				verifyWordSymbols(UniChar *);
	void				sleep(void);
	void				wakeUp(void);
	void				signalMemoryError(void);

protected:
	NewtonErr		iWRecDomain(CController * inController);

	virtual void	domainParameter(ULong, OpaqueRef, OpaqueRef);
	virtual bool	setParameters(char ** inParms);

	CWRecognizer *	f24;
};


/* -----------------------------------------------------------------------------
	C S t d W o r d U n i t
----------------------------------------------------------------------------- */

class CStdWordUnit : public CSIUnit
{
public:
// debug
	virtual size_t		sizeInBytes(void);
	virtual void		dump(CMsg * outMsg);

	virtual void		endUnit(void);

	virtual int			deleteInterpretation(ArrayIndex index);

	virtual void		addWordInterpretation(void);
	virtual ArrayIndex	insertWordInterpretation(ArrayIndex index);

	virtual void		setCharWordString(ArrayIndex index, char * inStr);
	virtual void		setWordString(ArrayIndex index, UniChar * inStr);
	virtual char *		getString(ArrayIndex index);

	virtual void		setParam(UnitInterpretation * interpretation, ArrayIndex index, Ptr);
	virtual CArray *	getParam(ArrayIndex index);

	virtual void		getWordBase(FPoint * outPt1, FPoint * outPt2, ULong inArg3);
	virtual int			getWordSlant(ULong);
	virtual int			getWordSize(ULong);

	virtual void		reinforceWordChoice(long);

	virtual Ptr			getTrainingData(long inArg);			// was Handle
	virtual void		disposeTrainingData(Ptr inData);		// was Handle

protected:
	NewtonErr	iStdWordUnit(CRecDomain * inDomain, ULong, CArray * inAreas, ULong);

};


/* -----------------------------------------------------------------------------
	C W R e c U n i t
----------------------------------------------------------------------------- */

class CWRecUnit : public CStdWordUnit
{
public:
	static CWRecUnit *	make(CRecDomain * inDomain, ULong inType, CArray * inAreas);
	virtual void		dealloc(void);

protected:
	NewtonErr			iWRecUnit(CRecDomain * inDomain, ULong inType, CArray * inAreas);

	Ptr					f3C;
//size+40
};


/* -----------------------------------------------------------------------------
	C W R e c o g n i z e r
	is actually
	PROTOCOL CWRecognizer : public CProtocol			// interface
	PROTOCOL CRosRecognizer : public CWRecognizer	// implementation
----------------------------------------------------------------------------- */
typedef SamplePt WRecSamplePt;

class CWRecognizer
{
public:
#if 0
	static CWRecognizer *	make(char * inName);

	void	initialize(void);
	void	destroy(void);

	void	group(CStrokeUnit*);
	void	groupInkStroke(CStrokeUnit*, unsigned long, unsigned long, unsigned char);
	void	classify(CWRecUnit*);
	void	reclassify(CWRecUnit*);

	void	areaInfoConfigure(char**, RefArg);
	void	areaInfoFillDefaults(char**);
	void	areaInfoFreeDependents(char**);
	void	areaInfoGetSize(void);
	void	areaInfoSetParameters(char**);
	void	findBaseline(CRecStroke**, Point*);
	void	unitConfidence(CWRecUnit*);
	void	unitInfoFreePtr(char*);
	void	verifyWordSymbols(unsigned short*);

	void	sleep(void);
	void	wakeUp(void);

	void	addSub(CWRecUnit*, CStrokeUnit*);
	void	addWordInterpretation(CWRecUnit*);
	void	endInkStrokeGroup(CStrokeUnit**);
	void	endSubs(CWRecUnit*);
	void	getStartTime(CStrokeUnit*);
	void	getEndTime(CStrokeUnit*);
	void	getStartTime(CRecUnit*);
	void	getEndTime(CRecUnit*);
	void	getStartTime(CSIUnit*);
	void	getEndTime(CSIUnit*);
	void	getStartTime(CRecStroke*);
	void	getEndTime(CRecStroke*);
	void	getStartTime(CWRecUnit*);
	void	getEndTime(CWRecUnit*);
	void	getLabel(CWRecUnit*, unsigned long);
	void	getPartialGroup(unsigned char*);
	void	getSamplePtAddress(CStrokeUnit*, unsigned long);
	void	getSamplePtAddress(CRecStroke*, unsigned long);
	void	getScore(CWRecUnit*, unsigned long);
	void	getSub(CWRecUnit*, unsigned long);
	void	setCharWordString(CWRecUnit*, unsigned long, char*);
	void	getWordString(CWRecUnit*, unsigned long);
	void	interpretationCount(CWRecUnit*);
	void	invalidateUnit(CWRecUnit*);
	void	makeNewGroupFromStroke(CStrokeUnit*);
	void	newClassification(CWRecUnit*);
	void	rejectUnit(CWRecUnit*);
	void	setLabel(CWRecUnit*, unsigned long, unsigned long);
	void	setScore(CWRecUnit*, unsigned long, unsigned long);
	void	setWordString(CWRecUnit*, unsigned long, unsigned short*);
	void	strokeSampleX(WRecSamplePt*);
	void	strokeSampleY(WRecSamplePt*);
	void	strokeSize(CStrokeUnit*);
	void	strokeSize(CRecStroke*);
	void	strokeUnitStroke(CStrokeUnit*);
	void	subCount(CWRecUnit*);
	void	testClassifiedUnit(CWRecUnit*);
	void	testInvalidUnit(CWRecUnit*);
	void	testRejectedUnit(CWRecUnit*);
	void	unitInfoGetPtr(CWRecUnit*);
	void	unitInfoSetPtr(CWRecUnit*, char*);

private:
	CWReDomain *	f10;
#endif
};


/* -----------------------------------------------------------------------------
	C W R e c R e c o g n i z e r
----------------------------------------------------------------------------- */

class CWRecRecognizer : public CRecognizer
{
public:
	virtual ULong		unitConfidence(CUnit * inUnit);

	virtual void		sleep(void);
	virtual void		wakeUp(void);

	virtual int			configureArea(CRecArea * inArea, RefArg inArg2);

	virtual ULong		handleUnit(CUnit * inUnit);
};


/* -----------------------------------------------------------------------------
	C W o r d L i s t
----------------------------------------------------------------------------- */
#define kWordDelimiter 0xFFFF
#define kListDelimiter 0x0000

typedef bool (*CharShapeProcPtr)(UniChar);

enum { kBubbleUp, kBubbleDown };


class CWordList
{
public:
static void* operator new(size_t);
static void  operator delete(void*);

					CWordList();
					~CWordList();

	ArrayIndex	count(void);
	ULong			score(ArrayIndex index);
	ULong			label(ArrayIndex index);
	UniChar *	scanTo(ArrayIndex index);
	UniChar *	word(ArrayIndex index);

	ArrayIndex	find(UniChar * inWord);
	void			insertLast(UniChar * inWord, ULong inScore, ULong inLabel);
	UniChar *	ith(ArrayIndex index, ULong * outScore, ULong * outLabel);
	
	void			bubbleGuess(UniChar inCh, CharShapeProcPtr inTest, int inDirection);
	void			reorder(void);
	void			swapSingleCharacterGuesses(ArrayIndex index1, ArrayIndex index2);

protected:
	UShort		fScores[16];		// +00
	UShort		fLabels[16];		// +20
	UChar			fNumOfWords;		// +40
	UniChar *	fWords;				// +44	was Handle
//size+48
};


#endif	/* __WORDRECOGNIZER_H */
