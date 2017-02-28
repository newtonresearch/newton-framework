/*
	File:		Controller.h

	Contains:	Recognition controller declarations.
					There is a single class, CController, that is in charge of recognition.
					All incoming information is given to the controller, and the controller
					is responsible for distributing it to the classes that actually do the
					recognition.
					A code module that performs recognition is referred to as a recognizer.
					A recognizer can be one of two types of classes:
					o  one that embodies the recognition process (a subclass of CRecDomain),
					o  one that describes the data structure that is formed as a result of
						classification (a subclass of CRecUnit).

	Written by:	Newton Research Group.
*/

#if !defined(__CONTROLLER_H)
#define __CONTROLLER_H 1

#include "Arbiter.h"


typedef   bool (*HitTestProcPtr)(CRecUnit * inUnit, CArray * inPts);
typedef   void (*ExpireStrokeProcPtr)(CRecUnit * inUnit);

struct ArbStuff
{
	CRecUnit *		x00;
	int				x04;
	ULong				x08;
	Assoc				x0C;
//size+28
};


/*------------------------------------------------------------------------------
	C C o n t r o l l e r

	The CController class provides methods for grouping and classifying
	recognition units, and for handling recognition areas.
------------------------------------------------------------------------------*/
class CStrokeUnit;

class CController : public CRecObject
{
public:
	static CController * make(void);
	virtual void	dealloc(void);

	void			initialize(void);

	void			registerDomain(CRecDomain * inDomain);
	void			newGroup(CRecUnit * inUnit);
	CRecUnitList *	getDelayList(CRecDomain * inDomain, ULong inType);

	NewtonErr	newClassification(CRecUnit * inUnit);
	void			buildGTypes(CRecArea * inArea);

	ULong			nextIdleTime(void);
	ULong			idle(void);

	bool			checkBusy(void);
	void			classifyInArea(CRecUnit * inUnit, CRecArea * inArea);
	void			cleanGroupQ(CRecUnit * inUnit);
	void			cleanUp(void);
	void			cleanupAfterError(void);
	void			cleanUpUnits(bool inNuke);
	void			clearArbiter(void);
	void			clearController(void);
	bool			controllerError(void);
	void			deletePiece(ArrayIndex index);
	void			deleteUnit(ArrayIndex index);
	int			doClassify(void);
	void			doGroup(void);
	CRecUnit *		getClickInProgress(void);
	CStrokeUnit *	getIndexedStroke(ArrayIndex index);
	CRecDomain *	getTypedDomain(RecType);
	CRecUnitList *		getUList(CRecDomain * inDomain, ULong inType, ULong, ULong);
	void			markUnits(CRecUnit * inUnit, ULong);
	bool			noEventsWithinDelay(CRecUnit * inUnit, ULong);
	void			registerArbiter(CArbiter * inArbiter);
	void			doArbitration(void);
	bool			isExternallyArbitrated(CRecUnit * inUnit);
	void			regroupSub(CRecUnit * inUnit1, CRecUnit * inUnit2);
	void			regroupUnclaimedSubs(CRecUnit * inUnit);
	void			setHitTestRoutine(HitTestProcPtr);
	void			setExpireStrokeRoutine(ExpireStrokeProcPtr);
	void			expireAllStrokes(void);
	void			signalMemoryError(void);
	void			triggerRecognition(void);
	void			updateInk(FRect*);
	bool			isLastCompleteStroke(CRecUnit * inUnit);
	void			recognizeInArea(CArray*, CRecArea * inArea, ULong (*)(CRecUnit * inUnit, ULong), ULong);

	CRecUnitList *	piecePool(void) const;
	CRecUnitList *	unitPool(void) const;
	ULong			endTime(void) const;
	void			setEndTime(ULong inTime);

	void			expireStroke(CRecUnit * inUnit);
	bool			getf21(void) const;
	void			setf2C(ULong inTime);

protected:
	NewtonErr		iController(void);

	friend NewtonErr InitControllerState(CController * inController);

	CRecUnitList *	fPiecePool;		// +08
	CRecUnitList *	fUnitPool;		// +0C
	CArray *		fDomainList;	// +10
	CArray *		f14;
	CArbiter *  fArbiter;		// +18
	short			f1C;
	ULong			f20;				// ticks...		time after which we should classify the group
	ULong			f24;
	ULong			f28;
	ULong			f2C;				// ...ticks

	HitTestProcPtr			fHitTest;		// +38
	ExpireStrokeProcPtr  fExpireStroke;	// +3C
	bool			f40;
	long			f44;
	long			f48;
	long			f4C;
	CRecArea *	f50;
	HitTestProcPtr			fSaveHitTest;	//+54
	ExpireStrokeProcPtr  fSaveExpireStroke;	//+58
	long			f5C;
// size +60
};

inline CRecUnitList *	CController::piecePool(void) const { return fPiecePool; }
inline CRecUnitList *	CController::unitPool(void) const { return fUnitPool; }
inline ULong				CController::endTime(void) const { return f20; }
inline void					CController::setEndTime(ULong inTime) { f20 = inTime; }

inline void					CController::expireStroke(CRecUnit * inUnit)  { if (fExpireStroke) fExpireStroke(inUnit); }
inline bool					CController::getf21(void) const  { return fArbiter->getf21(); }
inline void					CController::setf2C(ULong inTime)  { f2C = inTime; }

/*	CController fFlags
	kControllerError	Prevents the unit from being claimed and disposed of.
*/
#define kControllerError	0x10000000


extern CController *		gController;


#endif	/* __CONTROLLER_H */
