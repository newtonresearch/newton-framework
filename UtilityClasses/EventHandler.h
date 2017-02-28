/*
	File:		EventHandler.h

	Contains:	CEventHandler definition.

	Written by:	Newton Research Group.
*/

#if !defined(__EVENTHANDLER_H)
#define __EVENTHANDLER_H 1

#if !defined(__NEWTON_H)
#include "Newton.h"
#endif

#if !defined(__EVENTS_H)
#include "Events.h"
#endif

#if !defined(__TIMERQUEUE_H)
#include "TimerQueue.h"
#endif

#if !defined(__USERPORTS_H)
#include "UserPorts.h"
#endif

class CEventHandler;


/*--------------------------------------------------------------------------------
	C E v e n t H a n d l e r C o m p a r e r
--------------------------------------------------------------------------------*/

class CEventHandlerComparer : public CItemComparer
{
public:
						CEventHandlerComparer();
	CompareResult	testItem(const void * inItem) const;
};

typedef CItemComparer CEventTimerComparer;


/*--------------------------------------------------------------------------------
	C E v e n t I d l e T i m e r
--------------------------------------------------------------------------------*/

class CEventIdleTimer : public CTimerElement
{
public:
				CEventIdleTimer(CTimerQueue * inQ, ULong inRefCon, CEventHandler * inHandler, Timeout inIdle);

	bool		start(void);
	bool		primeme(void);
	bool		stop(void);
	bool		reset(void);
	bool		reset(Timeout inIdle);
	void		timeout(void);

private:
	CEventHandler *	fHandler;
	Timeout				fTimeout;
};

inline bool CEventIdleTimer::start()  { return prime(fTimeout); }
inline bool CEventIdleTimer::primeme()  { return prime(fTimeout); }
inline bool CEventIdleTimer::stop()  { return cancel(); }
inline bool CEventIdleTimer::reset()  { return cancel() && primeme(); }
inline bool CEventIdleTimer::reset(Timeout inIdle)  { fTimeout = inIdle; return cancel() && primeme(); }


/*--------------------------------------------------------------------------------
	C E v e n t H a n d l e r
--------------------------------------------------------------------------------*/

class CEventHandler : public SingleObject
{
public:
						CEventHandler();
	virtual			~CEventHandler();

	//	initialize handler to specified eventId
	//	will now call EventHandlerProc for all events of this type
	NewtonErr		init(EventId inEventId, EventClass inEventClass = kNewtEventClass);

	EventClass		getEventClass(void)	const;
	EventId			getEventId(void)		const;

	void				deferReply(void);											// Defer reply to message we just received.
	NewtonErr		replyImmed(void);											// Do an immediate reply.
	void				setReply(ULong inReply);
	void				setReply(size_t inSize, CEvent * inEvent);			// Set reply size and reply
	void				setReply(CUMsgToken * inToken);						// Set reply message token
	void				setReply(CUMsgToken * inToken, size_t inSize, CEvent * inEvent);	// Set all the reply parameters

	// User supplied test of event - true if event is for this handler
	virtual	bool	testEvent(CEvent * inEvent);

	// User supplied handler routines
	virtual	void	eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual	void	eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual	void	idleProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);

	// initialize idler for time specified
	// will now call IdleProc
	NewtonErr		initIdler(Timeout inIdle, ULong inRefCon = 0, bool inStart = true);
	NewtonErr		initIdler(ULong inIdleAmount, TimeUnits inIdleUnits, ULong inRefCon = 0, bool inStart = true);

	NewtonErr		startIdle(void);					// start the timer
	NewtonErr		stopIdle(void);						// stop the timer
	NewtonErr		resetIdle(void);							// stop the timer if running, start again, using original delay value
	NewtonErr		resetIdle(ULong inIdleAmount, TimeUnits inIdleUnits);	// stop the timer if running, and setup new values, start again
	NewtonErr		resetIdle(Timeout inIdle);					// stop the timer if running, and setup new values, start again

private:
	friend class	CEventComparer;
	friend class	CEventHandlerComparer;
	friend class	CEventHandlerIterator;
	friend class	CAppWorld;

	CEventHandler *	addHandler(CEventHandler * inHeadOfChain);
	// Adds SELF from the chain of handlers starting at headOfChain.

	CEventHandler *	removeHandler(CEventHandler * inHeadOfChain);
	// Removes SELF from the chain of handlers starting at headOfChain. Returns the
	// new head of chain (if self was the head then there _must_ be a new head).

	NewtonErr			doEvent(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	// Execute the event if it belongs to this handler
	// else pass it along to the next handler

	void 					doComplete(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	// Execute the event if it belongs to this handler
	// else pass it along to the next handler

	CEventHandler *	getNextHandler(void)	const;
	// Return the next handler in the chain

	CEventHandler *	fNext;
	EventClass 			fEventClass;
	EventId 				fEventId;
	CEventIdleTimer *	fIdler;
};

inline EventClass			CEventHandler::getEventClass(void) const  { return fEventClass; }
inline EventId				CEventHandler::getEventId(void) const  { return fEventId; }
inline CEventHandler *	CEventHandler::getNextHandler(void) const  { return fNext; }


/*--------------------------------------------------------------------------------
	C S y s t e m E v e n t H a n d l e r
--------------------------------------------------------------------------------*/

class CSystemEventHandler : public CEventHandler
{
public:
						CSystemEventHandler();

	// call init for each type of system event you're interested in…
	NewtonErr		init(ULong inSystemEvent, ULong inSendFilter = 0);

	// handler
	virtual	void	eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);

	// override to get all system events…
	virtual void	anySystemEvents(CEvent * inEvent);

	// override to receive system events of their respective types…
	virtual void	powerOn(CEvent * inEvent);
	virtual void	powerOff(CEvent * inEvent);
	virtual void	newCard(CEvent * inEvent);
	virtual void	appAlive(CEvent * inEvent);
	virtual void	deviceNotification(CEvent * inEvent);
	virtual void	powerOffPending(CEvent * inEvent);

private:
	bool			fInited;
};


/*--------------------------------------------------------------------------------
	C P o w e r E v e n t H a n d l e r
--------------------------------------------------------------------------------*/

class CPowerEventHandler : public CSystemEventHandler
{
public:
	virtual	void	eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual	void	eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
};


/*--------------------------------------------------------------------------------
	C E v e n t H a n d l e r I t e r a t o r
--------------------------------------------------------------------------------*/

class CEventHandlerIterator : public SingleObject
{
public:
	CEventHandlerIterator(CEventHandler* inHead);

	void	reset(void);
	// Resets the iterator to begin again

	bool	more(void);
	// Returns true if there are more elements to iterate over

	CEventHandler *	firstHandler(void);
	// Resets the iterator to begin again and returns the first handler in the handler chain

	CEventHandler *	currentHandler(void);
	// returns the current handler in the handler chain

	CEventHandler *	nextHandler(void);
	// returns the next handler in the handler chain

private:
	void	advance(void);
	// Advances the iteration

	CEventHandler *	fFirstHandler;
	CEventHandler *	fCurrentHandler;
	CEventHandler *	fNextHandler;
};

inline bool					CEventHandlerIterator::more(void)  { return (fCurrentHandler != NULL); }
inline CEventHandler *	CEventHandlerIterator::firstHandler(void)  { reset(); return fFirstHandler; }
inline CEventHandler *	CEventHandlerIterator::currentHandler(void)  { return fCurrentHandler; }
inline CEventHandler *	CEventHandlerIterator::nextHandler(void)  { advance(); return fCurrentHandler; }


#endif	/* __EVENTHANDLER_H */