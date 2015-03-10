/*
	File:		KernelSemaphore.h

	Contains:	Kernel semaphore declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__KERNELSEMAPHORE_H)
#define __KERNELSEMAPHORE_H 1

#include "KernelTypes.h"	// SemFlags
#include "KernelTasks.h"


/*------------------------------------------------------------------------------
	C S e m a p h o r e
------------------------------------------------------------------------------*/

class CSemaphore : public CObject, CTaskContainer
{
public:
				CSemaphore();
	virtual  ~CSemaphore();

	virtual void	remove(CTask * inTask);

	void		blockOnZero(CTask * inTask, SemFlags inBlocking);
	void		wakeTasksOnZero(void);

	void		blockOnInc(CTask * inTask, SemFlags inBlocking);
	void		wakeTasksOnInc(void);

private:
	friend class CSemaphoreGroup;

	long			fVal;				// +14	semval in unix-speak
	CTaskQueue	fZeroTasks;		// +18	tasks waiting for fValue to become zero
	CTaskQueue	fIncTasks;		// +20	tasks waiting for fValue to increase
//	size +28
};


/*------------------------------------------------------------------------------
	C S e m a p h o r e O p L i s t
------------------------------------------------------------------------------*/

class CSemaphoreOpList : public CObject
{
public:
					~CSemaphoreOpList();

	NewtonErr	init(ArrayIndex inNumOfOps, SemOp * inOps);

private:
	friend class CSemaphoreGroup;

	SemOp	*		fOpList;		// +10
	ArrayIndex	fCount;		// +14
//	size +18
};


/*------------------------------------------------------------------------------
	C S e m a p h o r e G r o u p
------------------------------------------------------------------------------*/

class CSemaphoreGroup : public CObject
{
public:
					~CSemaphoreGroup();

	NewtonErr	init(ArrayIndex inCount);

	NewtonErr	semOp(CSemaphoreOpList * inList, SemFlags inBlocking, CTask * inTask);
	void			unwindOp(CSemaphoreOpList * inList, ArrayIndex index);

	void			setRefCon(void * inRefCon);
	void *		getRefCon(void) const;

private:
	CSemaphore	*	fGroup;		// +10
	ArrayIndex		fCount;		// +14
	void *			fRefCon;		// +18
//	size +1C
};

inline void			CSemaphoreGroup::setRefCon(void * inRefCon)  { fRefCon = inRefCon; }
inline void *		CSemaphoreGroup::getRefCon(void) const  { return fRefCon; }


#endif	/* __KERNELSEMAPHORES_H */
