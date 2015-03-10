/*
	File:		RecUnit.h

	Contains:	Unit declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__RECUNIT_H)
#define __RECUNIT_H 1

#include "RecStroke.h"
#include "RecArea.h"


class CRecDomain;
class CRecUnitList;


/*--------------------------------------------------------------------------------
	C R e c U n i t

	Class CRecUnit is the basic recognition unit class.
	The output of a recognizer is an object of class CRecUnit or of one of the CRecUnit
	subclasses. Only primitive objects, such as clicks, keystrokes, and other
	potential forms of input to the system, are instances of this class.
	The fields and methods defined by the CRecUnit class are fundamental to the
	recognition process, however, and used by all objects that are subclassed
	from it.
--------------------------------------------------------------------------------*/

class CRecUnit : public CRecObject
{
public:
						CRecUnit();
	virtual			~CRecUnit();

	static CRecUnit * make(CRecDomain * inDomain, ULong inType, ULong inArg3, CArray * inAreas);
	virtual void	dealloc(void);

// debug
	virtual size_t		sizeInBytes(void);
	virtual void		dump(CMsg * outMsg);
	void					dumpName(CMsg * outMsg);

// interpretation
	virtual ArrayIndex	subCount(void);
	virtual ArrayIndex	interpretationCount(void);
	virtual ArrayIndex	getBestInterpretation(void);
	virtual void			claimUnit(CRecUnitList * inList);
	virtual void			invalidate(void);

	virtual NewtonErr		markUnit(CRecUnitList *, ULong);
	virtual void			doneUsingUnit(void);

	virtual ArrayIndex	countStrokes(void);
	virtual bool			ownsStroke(void);
	virtual CRecStroke *		getStroke(ArrayIndex index);
	virtual CRecUnitList *  getAllStrokes(void);

	virtual void	setContextId(ULong inId);
	virtual ULong  contextId(void);

	CRecDomain *	getDomain(void) const;
	void				setDelay(ULong inDelay);
	ULong				getDelay(void) const;
	void				setType(RecType inType);
	RecType			getType(void) const;
	void				setPriority(ULong inPriority);
	ULong				getPriority(void) const;
	void				setStartTime(ULong inTime);
	ULong				getStartTime(void) const;
	ULong				getEndTime(void) const;
	void				setDuration(ULong inDuration);

	ULong				minStroke(void) const;
	ULong				maxStroke(void) const;

/*	long				checkOverlap(CRecUnit *a, CRecUnit *b);// return overlap status of two units
	long				countStrokes(CRecUnit *a);
	long				countOverlap(CRecUnit *a, CRecUnit *b);
	void				markStrokes(CRecUnit *a, char *ap, Long min);
*/

	CRecArea *		getArea(void);
	CAreaList *		getAreas(void);
	void				setAreas(CAreaList * inAreas);
	void				setBBox(FRect * inBox);
	FRect *			getBBox(FRect * outBox);

	UChar				getf24(void) const;

protected:
	NewtonErr		iRecUnit(CRecDomain * inDomain, ULong inType, ULong inArg3, CArray * inAreas);

	friend class CController;
	friend class CSIUnit;
	friend class CUnit;

											// The fFlags field (inherited from class CRecObject) contains information about the status of a unit.
	RecType			fUnitType;		// +08	The type of unit, currently represented by a unique four character sequence (like 'STRK', 'CHAR',  'WORD').
	Rect				fBBox;			// +0C
											// A rectangle, aligned to screen coordinates, that is guaranteed to enclose completely
											// all the strokes that are owned by the unit.
											// If the object is a word for instance, this rectangle describes the area that should be erased
											// to remove the object from the screen.
											// This box does not necessarily correspond to the size or shape of the actual recognized object.
	CRecDomain *	fDomain;			// +14	The domain that created the CRecUnit (or subclass).
											// The controller uses this pointer to call back the domain's classify method.
	CRecObject *	fAreaList;		// +18	Used internally, a list of all the recognition areas hit by the unit.
	ULong				fStartTime;		// +1C	The time (currently Ticks) at which the unit was created.
//	ULong				fEndTime;		// +		
	UShort			fDuration;		// +20
	UShort			fElapsedTime;	// +22	The elapsed time at which the last sub-unit was added (using the CSIUnit::addSub method).
	UChar				f24;				// +24
//	int				fRefCount;		// +25
	UChar				fPriority;		// +26	Provided for specifying a priority value.
	UChar				fDelay;			// +27	Amount of delay for this unit.
	UShort			fNumOfStrokes;	// +28	The number of strokes of which the unit is composed. This field is set when newClassification is called.
	UShort			fMinStroke, fMaxStroke;	// +2A, +2C
											// These two fields are hints to the controller to help it efficiently determine the value of fNumOfStrokes
											// for a unit that is a parent of this unit in the future.
											// Note that fNumOfStrokes does not necessary equal fMaxStroke - fMinStroke.
// size +30
};

inline CRecDomain *	CRecUnit::getDomain(void) const				{ return fDomain; }
inline ULong			CRecUnit::getDelay(void) const				{ return fDelay; }
inline void				CRecUnit::setType(RecType inType)			{ fUnitType = inType; }
inline RecType			CRecUnit::getType(void) const					{ return fUnitType; }
inline void				CRecUnit::setPriority(ULong inPriority)	{ fPriority = inPriority; }
inline ULong			CRecUnit::getPriority(void) const			{ return fPriority; }
inline void				CRecUnit::setStartTime(ULong inTime)		{ fStartTime = inTime; }
inline ULong			CRecUnit::getStartTime(void) const			{ return fStartTime; }		// not sure
inline ULong			CRecUnit::getEndTime(void) const				{ return fStartTime + fDuration; }
inline void				CRecUnit::setDuration(ULong inDuration)	{ fDuration = inDuration; }
inline ULong			CRecUnit::minStroke(void) const				{ return fMinStroke; }
inline ULong			CRecUnit::maxStroke(void) const				{ return fMaxStroke; }
inline UChar			CRecUnit::getf24(void) const					{ return f24; }


/*	CRecUnit fFlags
	kConstantUnit		Prevents the unit from being claimed and disposed of.
	kClaimedUnit		Marks the unit, its sub-units, and any other units that have claimed units as their sub-units, as disposable. Area handlers should not set this flag directly, but should instead call the TController::ClaimUnits method.
	kPartialUnit		Marks a unit that is known not to be complete and prevents the unit from being classified. For instance, a recognizer might know that a character unit contains the first two strokes of an `E`. A recognizer that does not explicitly set a delay may want to set this flag if it needs to delay the classification of a particular unit that it has created (in its Group method) and put into the unit pool (using NewGroup).
	kDelayUnit			Delays classification of a unit until a specified amount of time has elapsed. This flag is used by the controller to maintain the delay specified by a recognizer.
	kBusyUnit			This flag prevents the controller from disposing of a unit, even after it has been claimed.
	kTryHarderUnit		Set by the controller when it passes a unit back to an area handler a second time. For example, an area might accept text or graphics. The graphics handler might not claim a unit because it seems too small. If the text handler also rejects the unit, the controller might pass the unit back to the graphics handler with the tryHarderUnit flag set.
	kNoTryHarderUnit	Prevents the controller from calling back an area handler a second time. For instance, training application might want to set this flag.

	kAreasUnit			The fAreaList contains a CAreaList* array of areas, rather than a single CRecArea instance.
*/
#define kConstantUnit		0x80000000
#define kClaimedUnit			0x40000000
#define kPartialUnit			0x20000000
#define kDelayUnit			0x10000000
#define kInvalidUnit			0x08000000
#define kBusyUnit				0x04000000
#define kTryHarderUnit		0x02000000
#define kNoTryHarderUnit	0x01000000

#define kAreasUnit			0x00020000


/*--------------------------------------------------------------------------------
	C R e c U n i t L i s t
--------------------------------------------------------------------------------*/

class CRecUnitList : public CDArray
{
public:
// make
	static CRecUnitList *	make(void);

// debug
	virtual void	dump(CMsg * outMsg);

	NewtonErr		addUnit(CRecUnit * inUnit);
	NewtonErr		addUnique(CRecUnit * inUnit);
	CRecUnit *		getUnit(ArrayIndex index);
	void				purge(void);

protected:
	NewtonErr		iRecUnitList(void);
};


#endif	/* __RECUNIT_H */
