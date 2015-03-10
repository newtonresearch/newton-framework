/*
	File:		Recognition.h

	Contains:	CRecognition declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__RECOGNITION_H)
#define __RECOGNITION_H 1

#include "Controller.h"
#include "StrokeCentral.h"
#include "Recognizers.h"


/*------------------------------------------------------------------------------
	C R e c o g n i t i o n M a n a g e r
------------------------------------------------------------------------------*/

class CRecognitionManager
{
public:
	NewtonErr	init(UChar inCapability);

	RecognitionState *	saveRecognitionState(bool * outFail);
	void			restoreRecognitionState(RecognitionState *);
	void			enableModalRecognition(Rect * inRect);
	void			disableModalRecognition(void);
	bool			modalRecognitionOK(Rect * inRect);

	NewtonErr	idle(void);
	CTime			nextIdle(void);

	void			ignoreClicks(Timeout inDuration);
	void			setNextClick(ULong inTime);
	void			saveClickView(CView * inView);
	void			removeClickView(CView * inView);

	NewtonErr	update(Rect * inRect);

	void			addRecognizer(CRecognizer * inRecognizer);

	CController *	controller(void) const;
	ArrayIndex		numOfRecognizers(void) const;
	CRecognizer *	getRecognizer(ArrayIndex index) const;
	CRecognizer *	findRecognizer(RecType inType) const;
	ULong				x18Value(void) const;
	void				setx1C(UChar inValue);
	UChar				x1CValue(void) const;
	CView *			prevClickView(void) const;
	CView *			clickView(void) const;
	void				setUnitHandler(UnitHandler inHandler);
	UnitHandler		unitHandler(void) const;
	void				setx38(bool inValue);
	bool				getx38(void) const;
	void				setx3C(CRecUnit * inValue);
	CRecUnit *		getx3C(void) const;

private:
	NewtonErr	initRecognizers(void);

	friend class CStrokeCentral;

	UChar					fCapability;		// +00
	CStrokeCentral *	fStrokeCentral;	// +04
	CController *		fController;		// +08
	CArbiter *			fArbiter;			// +0C
	CDArray *			x10;					// +10	areas?
	CRecognizerList *	fRecognizers;		// +14
	ULong					x18;
	UChar					x1C;
	Rect *				fModalBounds;		// +28
	CView *				fPrevClickView;	// +2C
	CView *				fClickView;			// +30
	UnitHandler			fUnitHandler;		// +34
	UChar					x38;
	CRecUnit *			x3C;
};

inline void				CRecognitionManager::addRecognizer(CRecognizer * inRecognizer)  { fRecognizers->addRecognizer(inRecognizer); }
inline CController *	CRecognitionManager::controller(void) const  { return fController; }
inline ArrayIndex		CRecognitionManager::numOfRecognizers(void) const  { return fRecognizers->count(); }
inline CRecognizer *	CRecognitionManager::getRecognizer(ArrayIndex index) const  { return fRecognizers->getRecognizer(index); }
inline CRecognizer *	CRecognitionManager::findRecognizer(RecType inType) const  { return fRecognizers->findRecognizer(inType); }
inline ULong			CRecognitionManager::x18Value(void) const  { return x18; }
inline void				CRecognitionManager::setx1C(UChar inValue)  { x1C = inValue; }
inline UChar			CRecognitionManager::x1CValue(void) const  { return x1C; }
inline CView *			CRecognitionManager::prevClickView(void) const  { return fPrevClickView; }
inline CView *			CRecognitionManager::clickView(void) const  { return fClickView; }
inline void				CRecognitionManager::setUnitHandler(UnitHandler inHandler)  { fUnitHandler = inHandler; }
inline UnitHandler	CRecognitionManager::unitHandler(void) const  { return fUnitHandler; }
inline void				CRecognitionManager::setx38(bool inValue)  { x38 = inValue; }
inline bool				CRecognitionManager::getx38(void) const  { return x38; }
inline void				CRecognitionManager::setx3C(CRecUnit * inValue)  { x3C = inValue; }
inline CRecUnit *		CRecognitionManager::getx3C(void) const  { return x3C; }


extern CRecognitionManager gRecognitionManager;

void	InitCorrection(void);

#endif	/* __RECOGNITION_H */
