/*
	File:		Arbiter.h

	Contains:	Recognition controller declarations.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__ARBITER_H)
#define __ARBITER_H 1

#include "RecDomain.h"


struct BestMatch
{
	CSIUnit *	x00;
	int			x08;
//size+28
};


/*------------------------------------------------------------------------------
	C A r b i t e r
------------------------------------------------------------------------------*/
class CController;

class CArbiter
{
public:
	static CArbiter *	make(CController * inController);

	int		allUnitsPresent(CRecArea * inArea, BestMatch * inArg2);
	int		gatherUnits(ULong, bool, CArray *);
	int		waitingForOtherUnits(CRecArea * inArea, BestMatch * inArg2);
	int		arbitrateUnits(CRecArea * inArea);
	void		arbitrateGraphicsWords(CArray * inArg1);
	void		doArbitration(void);
	void		cleanUp(void);

	void		setf20(bool inArg);
	bool		getf21(void) const;
	long		getf24(void) const;
	void		setf24(long inArg);

protected:
	NewtonErr		iArbiter(CController * inController);
	friend NewtonErr InitArbiterState(CArbiter * inArbiter);
	friend class CController;

	CController *  fController;	// +00
	CDArray *		f04;
	CDArray *		f08;
	CArray *			f0C;
	CArray *			f10;
	CDArray *		f14;
	CDArray *		f18;
	CDArray *		f1C;
	bool				f20;
	bool				f21;
	long				f24;
};

inline void		CArbiter::setf20(bool inArg)  { f20 = inArg; }
inline bool		CArbiter::getf21(void) const  { return f21; }
inline long		CArbiter::getf24(void) const  { return f24; }
inline void		CArbiter::setf24(long inArg)  { f24 = inArg; }

extern CArbiter *			gArbiter;


extern bool		IsOfType(RecType inType1, RecType inType2);


#endif	/* __ARBITER_H */
