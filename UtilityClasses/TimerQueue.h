/*
	File:		TimerQueue.h

	Contains:	Interface to the CTimerQueue class.

	Written by:	Newton Research Group.
*/

#if !defined(__TIMERQUEUE_H)
#define __TIMERQUEUE_H 1

// name clash with AssertMacros.h
#undef check

#if !defined(__LONGTIME_H)
#include "LongTime.h"
#endif

#if !defined(__USERPORTS_H)
#include "UserPorts.h"
#endif


/*--------------------------------------------------------------------------------
	C T i m e r E l e m e n t
--------------------------------------------------------------------------------*/

class CTimerQueue;

class CTimerElement : public SingleObject
{
public:
							CTimerElement(CTimerQueue * inQ, ULong refCon);
	virtual				~CTimerElement();

	bool					prime(Timeout delta);
	bool					cancel(void);
	bool					isPrimed(void)		const;

	virtual void		timeout(void) = 0;

	ULong					getRefCon(void)	const;
	void					setRefCon(ULong refCon);

private:
	friend class CTimerQueue;

	CTimerQueue *		fQueue;
	CTimerElement *	fNext;
	Timeout				fDelta;
	ULong					fRefCon;
	bool					fPrimed;
};

inline bool		CTimerElement::isPrimed(void) const  { return fPrimed; }
inline ULong	CTimerElement::getRefCon(void) const  { return fRefCon; }
inline void		CTimerElement::setRefCon(ULong refCon)  { fRefCon = refCon; }


/*--------------------------------------------------------------------------------
	C T i m e r Q u e u e
--------------------------------------------------------------------------------*/

class CTimerQueue : public SingleObject
{
public:
							CTimerQueue();
							~CTimerQueue();

	Timeout				check(void);
	bool					isEmpty(void)	const;

	CTimerElement *	cancel(ULong refCon);

private:
	friend class CTimerElement;

	CTimerElement *	enqueue(CTimerElement * inItem);
	CTimerElement *	dequeue(CTimerElement * inItem, bool inAdjust);
	void					calibrate(void);

	CTimerElement *	fHead;
	CTime					fLastCalibrate;
	bool					fTimeoutInProgress;
};

inline bool		CTimerQueue::isEmpty(void) const  { return (fHead == NULL); }


/*--------------------------------------------------------------------------------
	C T i m e r P o r t
--------------------------------------------------------------------------------*/

class CTimerPort : public CUPort
{
public:
						CTimerPort();
						~CTimerPort();

	NewtonErr		init(void);
	NewtonErr		timedReceive(	size_t * outSize,
											void * content,
											size_t inSize,
											CUMsgToken * token = NULL,
											ULong * outMsgType = NULL,
											ULong inMsgFilter = kMsgType_MatchAll,
											bool onMsgAvail = false,
											bool tokenOnly = false);
	CTimerQueue *	getQueue(void);

private:
	CTimerQueue *	fQueue;

};

inline CTimerQueue * CTimerPort::getQueue(void)  { return fQueue; }


#endif	/*	__TIMERQUEUE_H	*/
