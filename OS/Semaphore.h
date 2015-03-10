/*
	File:		Semaphore.h

	Contains:	Semaphore declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__SEMAPHORE_H)
#define __SEMAPHORE_H 1

#include "KernelTypes.h"	// SemFlags
#include "UserObjects.h"


extern "C" ULong	Swap(ULong * inAddr, ULong inValue);

extern NewtonErr  SemGroupSetRefCon(ObjectId inSemGroupId, void * inRefCon);
extern NewtonErr  SemGroupGetRefCon(ObjectId inSemGroupId, void ** outRefCon);

/*------------------------------------------------------------------------------
	C U S e m a p h o r e O p L i s t
------------------------------------------------------------------------------*/

class CUSemaphoreOpList : public CUObject
{
public:
	NewtonErr	init(ArrayIndex inNumOfArgs, ...);
};


/*------------------------------------------------------------------------------
	C U S e m a p h o r e G r o u p
------------------------------------------------------------------------------*/

class CUSemaphoreGroup : public CUObject
{
public:
	NewtonErr	init(ULong inCount);

	NewtonErr	semOp(CUSemaphoreOpList * inList, SemFlags inBlocking);
	NewtonErr	semOp(ObjectId inListId, SemFlags inBlocking);

	NewtonErr	setRefCon(void * inRefCon);
	NewtonErr	getRefCon(void ** outRefCon);

//protected:
	ULong *	fRefCon;	// +08
};


inline NewtonErr CUSemaphoreGroup::semOp(CUSemaphoreOpList * inList, SemFlags inBlocking)
{ return SemaphoreOpGlue(fId, *inList, inBlocking); }

inline NewtonErr CUSemaphoreGroup::semOp(ObjectId inListId, SemFlags inBlocking)
{ return SemaphoreOpGlue(fId, inListId, inBlocking); }

inline NewtonErr CUSemaphoreGroup::setRefCon(void * inRefCon)
{ return SemGroupSetRefCon(fId, inRefCon); }

inline NewtonErr CUSemaphoreGroup::getRefCon(void ** outRefCon)
{ return SemGroupGetRefCon(fId, outRefCon); }


/*------------------------------------------------------------------------------
	C U L o c k i n g S e m a p h o r e
------------------------------------------------------------------------------*/

class CULockingSemaphore : public CUSemaphoreGroup
{
public:
				CULockingSemaphore();
				~CULockingSemaphore();

	static NewtonErr  staticInit(void);
	NewtonErr	init(void);
	NewtonErr	copyObject(ObjectId inId);

	NewtonErr	acquire(SemFlags inBlocking);
	NewtonErr	release(void);

protected:
	static	CUSemaphoreOpList	fAcquireOp;		// 0C104F0C
	static	CUSemaphoreOpList	fReleaseOp;		// 0C104F14
};


/*------------------------------------------------------------------------------
	C U M u t e x
	Check all instances of CULockingSemaphore: could be CUMutex.
------------------------------------------------------------------------------*/

class CUMutex : public CULockingSemaphore
{
public:
				CUMutex();

	long			f0C;
	long			f10;
};


/*------------------------------------------------------------------------------
	C U R d W r S e m a p h o r e
------------------------------------------------------------------------------*/

class CURdWrSemaphore : public CUSemaphoreGroup
{
public:
	static NewtonErr  staticInit(void);
	NewtonErr	init(void);

	void		acquireRd(SemFlags inBlocking);
	void		acquireWr(SemFlags inBlocking);
	void		releaseRd(void);
	void		releaseWr(void);

protected:
	static	CUSemaphoreOpList	fAcquireWrOp;	// 0C104F1C
	static	CUSemaphoreOpList	fReleaseWrOp;	// 0C104F24
	static	CUSemaphoreOpList	fAcquireRdOp;	// 0C104F2C
	static	CUSemaphoreOpList	fReleaseRdOp;	// 0C104F34
};


inline	void	CURdWrSemaphore::acquireRd(SemFlags inBlocking)
{ semOp(fAcquireRdOp, inBlocking); }

inline	void	CURdWrSemaphore::acquireWr(SemFlags inBlocking)
{ semOp(fAcquireWrOp, inBlocking); }

inline	void	CURdWrSemaphore::releaseRd(void)
{ semOp(fReleaseRdOp, kWaitOnBlock); }

inline	void	CURdWrSemaphore::releaseWr(void)
{ semOp(fReleaseWrOp, kWaitOnBlock); }


#endif	/* __SEMAPHORES_H */
