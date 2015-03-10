/*
	File:		Inker.h

	Contains:	CInker declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__INKER_H)
#define __INKER_H 1

#include "Objects.h"
#include "AppWorld.h"
#include "TabletTypes.h"
#include "BusyBox.h"


/* -------------------------------------------------------------------------------
	C I n k e r E v e n t

	Selectors:
	 2		init
	22		get calibration
	23		set calibration
	29		hobble tablet
	53		deactivate busy box
	54		activate busy box
------------------------------------------------------------------------------- */

class CInkerEvent : public CEvent
{
	friend class CInkerEventHandler;
public:
					CInkerEvent();
					CInkerEvent(int inSelector);

	void			setSelector(int inSelector);

//protected:
	int			fSelector;	// +08
	int			fResult;		// +0C
};


inline CInkerEvent::CInkerEvent() : CEvent() { }
inline CInkerEvent::CInkerEvent(int inSelector) : CEvent(kInkerEventId), fSelector(inSelector) { }
inline void CInkerEvent::setSelector(int inSelector) { fSelector = inSelector; }


/* -------------------------------------------------------------------------------
	C I n k e r C a l i b r a t i o n E v e n t
------------------------------------------------------------------------------- */

class CInkerCalibrationEvent : public CInkerEvent
{
public:
					CInkerCalibrationEvent();
					CInkerCalibrationEvent(int inSelector);

	friend class CInkerEventHandler;

//protected:
	Calibration	fCalibration;	// +10
};

inline CInkerCalibrationEvent::CInkerCalibrationEvent() : CInkerEvent() { }
inline CInkerCalibrationEvent::CInkerCalibrationEvent(int inSelector) : CInkerEvent(inSelector) { }


/* -------------------------------------------------------------------------------
	C I n k e r B o u n d s E v e n t
------------------------------------------------------------------------------- */

class CInkerBoundsEvent : public CInkerEvent
{
public:
					CInkerBoundsEvent();

	friend class CInkerEventHandler;

//protected:
	Rect	fBounds;	// +10
};

inline CInkerBoundsEvent::CInkerBoundsEvent() : CInkerEvent() { }


/* -------------------------------------------------------------------------------
	C I n k e r E v e n t H a n d l e r
------------------------------------------------------------------------------- */

class CInkerEventHandler : public CEventHandler
{
public:
	virtual	void	eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual	void	eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual	void	idleProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);

	void				inkThem(void);

private:
	CBusyBox			f14;		// +14
// size +4C
};


/* -------------------------------------------------------------------------------
	C L i v e I n k e r
------------------------------------------------------------------------------- */

class CLiveInker
{
public:
					CLiveInker();
					~CLiveInker();

	NewtonErr	init(void);
	void			resetAccumulator(void);
	void			startLiveInk(void);
	void			stopLiveInk(void);
	void			inkLine(const Point, const Point, const Point);
	bool			addPoint(const Point inLocation, const Point inSize);

	Rect			bounds(void) const;

private:
	bool			mapLCDExtent(const Rect * inRect, Rect * outRect);

	char *		fPixMem;				// +00
	PixelMap		fPixMap;				// +04
	Rect			fInkBounds;			// +20
	ArrayIndex	fNumOfPts;			// +28
	LPoint		fExtent;
	int			fExtentV;			// +2C
	int			fExtentH;			// +30
	size_t		fPixMemSize;		// +34
	ULong			fPixDepthShift;	// +38
//	size +3C
};

inline Rect CLiveInker::bounds(void) const  { return fInkBounds; }


/* -------------------------------------------------------------------------------
	C I n k e r
------------------------------------------------------------------------------- */

class CInker : public CAppWorld
{
public:
				CInker();
				~CInker();

	virtual size_t		getSizeOf(void) const;	// not in the original?
	virtual NewtonErr mainConstructor(void);

	void			setNewtPort(CUPort * inPort);
	void			setCurrentPenMode(UChar inPenMode);
	UChar			getCurrentPenMode(void);
	void			setNextPenMode(UChar inPenMode);
	UChar			getNextPenMode(void);

	bool			testForCalibrationNeeded(void);
	NewtonErr	calibrate(ULong);
	void			presCalibrate(void);

	bool			convert(void);
	void			drawInk(Point inPt1, Point inPt2, Rect * outRect, short);
	void			insertionSort(ULong *, ArrayIndex, ULong);

private:
	friend class CInkerEventHandler;

	void			iInker(void);
	void			getRawPoint(ULong *, ULong *, short, short, ULong);
	void			LCDEntry(void);

	CUAsyncMessage	fMessage;			// +70
	CUPort *			fPort;				// +80
	short				x84;
	FPoint			fPenLoc;				// +88
	FPoint			fSampleLoc;			// +90

	UChar				fCurrentPenMode;	// +C0
	UChar				fNextPenMode;		// +C1
	UChar				fPenSize;			// +C2	unused
	UChar				fPenState;			// +C3
	Rect				fInkBounds;			// +C4
	CInkerEvent		fEvent;				// +CC
	CLiveInker		fLiveInker;			// +DC
//	size +0118
};


#define gInkWorld static_cast<CInker*>(GetGlobals())

extern	  void	LoadInkerCalibration(void);
extern "C" void	StartInker(CUPort * inPort);


#endif	/* __INKER_H */
