/*
	File:		StrokeCentral.h

	Contains:	CStrokeCentral declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__STROKECENTRAL_H)
#define __STROKECENTRAL_H 1

#include "StrokeRecognizer.h"
#include "NewtonTime.h"

// time value for unexpired strokes etc.
#define kDistantFuture 0xFFFFFFFF


/*------------------------------------------------------------------------------
	R e c o g n i t i o n S t a t e
------------------------------------------------------------------------------*/

struct RecognitionState
{
	bool				f00;
	CRecStroke *	f04;
	CRecUnit *		f08;
	long				f0C;
	long				f10;
	RefStruct *		f14;
	long				f18;
	CRecUnitList *	f1C;
	long				f20;
	long				f24;
	Ptr				f28;
	bool				f2C;
	void *			f30;
	RefStruct *		f34;
	long				f38;
	int				f3C;
};


/*--------------------------------------------------------------------------------
	C S t r o k e C e n t r a l
--------------------------------------------------------------------------------*/

class CStrokeCentral
{
public:
					CStrokeCentral();
					~CStrokeCentral();

	void			init(void);
	void			initFields(void);
	void			doneFields(void);

	void			startNewStroke(CRecStroke * ioStroke);
	void			addDeferredStroke(RefArg, long, long);
	CRecStroke *	currentStroke(void);
	void			doneCurrentStroke(void);
	void			invalidateCurrentStroke(void);

	void			blockStrokes(void);
	void			unblockStrokes(void);
	bool			flushStrokes(void);
	bool			beforeLastFlush(long);

	void			addExpiredStroke(CStrokeUnit * inStroke);
	void			IGCompressGroup(CStrokeUnit ** inStrokes);
	void			expireGroup(CUnit ** inGroup);
	void			compressGroup(void);
	void			updateCompressGroup(FRect * inBox);
	void			idleCompress(void);
	void			idleCurrentStroke(void);
	void			idleStrokes(void);
	void			expireAll(void);

	OpaqueRef	saveRecognitionState(bool * outErr);
	void			restoreRecognitionState(OpaqueRef inState);

//private:
	bool				fIsValidStroke;	// +00
	CRecStroke *	fCurrentStroke;	// +04
	CRecUnit *		fCurrentUnit;		// +08
	long				f0C;
	long				f10;
	int				fBlockCount;		// +14
	int				fUnblockCount;		// +18
	long				f1C;
	RefStruct *		f20;
	ArrayIndex		f24;
	CRecUnitList * f28;
	CTime				f2C;
	Ptr				f34;		// StrokeGroupData
	bool				f38;
	void *			f3C;		// actually a function pointer
	RefStruct *		f40;
// size +44
};

extern CStrokeCentral	gStrokeWorld;


#endif	/* __STROKECENTRAL_H */
