/*
	File:		RecArea.h

	Contains:	Recognition area declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__RECAREA_H)
#define __RECAREA_H 1

#include "RecObject.h"


/*------------------------------------------------------------------------------
	C R e c A r e a

	The CRecArea class represents a screen recognition area.
	It specifies an area of the screen and the types of units to be recognized
	in that area.
------------------------------------------------------------------------------*/
class CDictChain;

class CRecArea : public CRecObject
{
public:
// make
	static CRecArea *	make(ULong inArea, ULong inFlags);
	virtual void		dealloc(void);

// debug
	virtual size_t		sizeInBytes(void);
	virtual void		dump(CMsg * outMsg);

	void					addAType(RecType inType, UnitHandler inHandler, ULong inArg3, RecDomainInfo * info);

	Ptr					getInfoFor(RecType inType, bool);
	void					paramsAllSet(RecType inType);

	CTypeAssoc *		arbitrationTypes(void) const;
	CTypeAssoc *		groupingTypes(void) const;
	void					setChain(ArrayIndex index, CDictChain * inChain);
	void					setViewId(ULong inId);
	ULong					viewId(void) const;
	long					getf10(void) const;
	long					getf14(void) const;

protected:
	friend class CAreaList;
	friend class CController;

//	int				fRefCount;  // +08
	ULong				fArea;		// +0C Used by the controller to store a structure representing the recognition area.
										// A view system may store whatever it likes in this field.
	long				f10;
	long				f14;
	CTypeAssoc *	fATypes;		// +18 A list of arbitration types--the recognized units requested by this area--
										// that's maintained by the controller. With each element in this list,
										// the controller also stores the handler routine to be called when that
										// particular type is recognized, as well as any global data to be passed back.
	CTypeAssoc *	fGTypes;		// +1C A list of grouping types--the units that must be recognized on the way to
										// recognizing the requested arbitration types--that's maintained by the
										// controller. When a grouping type unit is recognized, it is automatically
										// passed on to the next level of recognition.
	CDictChain *	f20[3];		// +20 default, priority, always areas
	ULong				fView;		// +2C Opaque view reference.
// size +30
};

inline CTypeAssoc *		CRecArea::arbitrationTypes(void) const  { return fATypes; }
inline CTypeAssoc *		CRecArea::groupingTypes(void) const  { return fGTypes; }
inline void					CRecArea::setChain(ArrayIndex index, CDictChain * inChain)  { f20[index] = inChain; }
inline void					CRecArea::setViewId(ULong inId)  { fView = inId; }
inline ULong				CRecArea::viewId(void) const  { return fView; }
inline long					CRecArea::getf10(void) const  { return f10; }
inline long					CRecArea::getf14(void) const  { return f14; }


// fFlags
#define kDefaultArea		0x80000000
#define kPriorityArea	0x40000000
#define kAlwaysArea		0x20000000


/*------------------------------------------------------------------------------
	C A r e a L i s t
------------------------------------------------------------------------------*/

class CAreaList : public CDArray
{
public:
	static CAreaList *	make(void);

	virtual CRecObject *	retain(void);
	virtual void		release(void);

	NewtonErr			addArea(CRecArea * inArea);
	CRecArea *			getArea(ArrayIndex index);
	CRecArea *			getMergedArea(void);
	bool					findMatchingView(ULong inId);

protected:
	NewtonErr			iAreaList(void);
};


#endif	/* __RECAREA_H */
