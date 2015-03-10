/*
	File:		Journal.h

	Contains:	Pen stroke journalling system declarations.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__JOURNAL_H)
#define __JOURNAL_H 1

#include "Objects.h"
#include "TabletTypes.h"


/* -----------------------------------------------------------------------------
	J o u r n a l   S t r o k e
----------------------------------------------------------------------------- */

struct StrokeHeader
{
	short		x00;		// unused?
	short		x02;		// sample type
	short		x04;		// num of strokes
	short		x06;
	short		x08;
};


struct JournalStroke
{
	size_t	dataSize;
	ULong		count;
	ULong		startTime;
	ULong		endTime;
	ULong		points[];	// points constituting the stroke; might actually be TabletSample or TabPts
};


/* -----------------------------------------------------------------------------
	J o u r n a l   R e p l a y   H a n d l e r
----------------------------------------------------------------------------- */

class CJournalReplayHandler
{
public:
					CJournalReplayHandler();
					~CJournalReplayHandler();

	void			initStroke(ULong inOriginX, ULong inOriginY);
	NewtonErr	playAStroke(JournalStroke*, ULong inOriginX, ULong inOriginY, bool inTakeOwnership);
	bool			isJournalReplayBusy(void);
	bool			getNextTabletSample(ULong * outSample);
	void			parseStrokeFileHeader(void);
	void			setStrokesToPlay(short);
	JournalStroke *	getNextStroke(void);

	void			setOrigin(ULong inOriginX, ULong inOriginY);

//private:
	JournalStroke *	fSampleData;	// +00
	CDataPtr		fStrokeData;			// +04
	short			fSampleType;			// +0A	sample type: 1 => TabletSample  2 => raw TabPt
	short			fNumOfStrokes;			// +08
	short			fNumOfStrokesToPlay;	// +0C
	LPoint		fOrigin;					// +10
	ULong			f18;
	short			f1A;
	short			f1C;
	short			fStrokeIndex;			// +1E
	ULong			fNumOfPoints;			// +20
	ULong			fStrokeDuration;		// +24
	ULong			fTimebase;				// +28	real time from which to base (relative) stroke times
	ULong			fStrokeStartTime;		// +2C	real time of stroke start
	ULong			f30;
	ULong			fPointIndex;			// +34
	ULong			f38;
	bool			f3C;
	bool			fIsReplaying;			// +3D
	bool			fIsStrokeAvailable;	// +3E
	ULong			fStrokeDataOffset;	// +40
	size_t		fStrokeDataSize;		// +44
// size +48
};

inline void CJournalReplayHandler::setOrigin(ULong inOriginX, ULong inOriginY)  { fOrigin.x = inOriginX; fOrigin.y = inOriginY; }


#endif	/* __JOURNAL_H */
