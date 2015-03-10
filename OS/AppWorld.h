/*
	File:		AppWorld.h

	Contains:	App world declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__APPWORLD_H)
#define __APPWORLD_H 1

#include "UserTasks.h"
#include "EventHandler.h"
#include "List.h"
#include "Semaphore.h"


/*------------------------------------------------------------------------------
	C F o r k W o r l d
------------------------------------------------------------------------------*/

class CForkWorld : public CUTaskWorld
{
public:
					CForkWorld();
	virtual		~CForkWorld();

	bool				enableForking(bool inEnable);
	NewtonErr		fork(CForkWorld * inWorld);
	void				yield(void);

	virtual size_t		getSizeOf(void) const;
	virtual NewtonErr	taskConstructor(void);
	virtual void		taskDestructor(void);
	virtual void		taskMain(void);

	virtual NewtonErr	forkInit(CForkWorld * inWorld);
	virtual NewtonErr	forkConstructor(CForkWorld * inWorld);
	virtual void		forkDestructor(void);

	virtual NewtonErr	mainInit(ULong inName, size_t inStackSize);
	virtual NewtonErr	mainInit(ULong inName, size_t inStackSize, ULong inPriority, ObjectId inEnvironment);
	virtual NewtonErr	mainConstructor(void);
	virtual void		mainDestructor(void);
	virtual NewtonErr	preMain(void);
	virtual void		theMain(void) = 0;
	virtual void		postMain(void);

	NewtonErr		acquireMutex(void);
	NewtonErr		releaseMutex(void);

protected:
	virtual void			forkSwitch(bool inDoIt);
	virtual CForkWorld *	makeFork(void);

	CUMutex *		fMutex;				// +18
	bool				fIsFirstInstance;	// +1C	is main, ie not forked task
	bool				fIsTasked;			// +1D
	bool				fIsReady;			// +1E
	bool				fIsEnabled;			// +1F
	CForkWorld *	fParent;				// +20
	size_t			fStackSize;			// +24
	ULong				fPriority;			// +28
	ULong				fName;				// +2C
//	size +30
};


/*------------------------------------------------------------------------------
	C A p p W o r l d S t a t e
------------------------------------------------------------------------------*/

class CAppWorldState
{
public:
						CAppWorldState();
						~CAppWorldState();

	NewtonErr		init(void);
	NewtonErr		init(ULong);
	NewtonErr		init(CUPort * inPort);

	NewtonErr		waitForEvent(Timeout inTimeout);
	NewtonErr		getError(void)	const;
	CUPort *			getPort(void)	const;

	void				nestedEventLoop(void);
	void				terminateNestedEventLoop(void);

	NewtonErr		fErr;				// +00
	CUPort *			fPort;			// +04
	CUMsgToken		fMsgTokenBuf;  // +08
	size_t			fMsgSize;		// +18
	size_t			fEventSize;		// +1C
	ULong				fMsgType;		// +20
	ULong				fFilter;			// +24
	bool				fDone;			// +28
	bool				fIsTokenOnly;	// +29
	bool				fOnMsgAvail;	// +2A
	CEvent *			fEvent;			// +2C
	CUMsgToken *	fMsgToken;		// +30
	char				fEventBuf[256];// +34
// size +0134
};

inline NewtonErr	CAppWorldState::waitForEvent(Timeout inTimeout)	{ return fPort->receive(&fMsgSize, fEventBuf, sizeof(fEventBuf), fMsgToken, &fMsgType, inTimeout, fFilter, fOnMsgAvail, fIsTokenOnly); }
inline NewtonErr  CAppWorldState::getError(void)	const	{ return fErr; }
inline CUPort *	CAppWorldState::getPort(void)		const	{ return fPort; }


/*------------------------------------------------------------------------------
	C A p p W o r l d
------------------------------------------------------------------------------*/
class CEventHandler;

class CAppWorld : public CForkWorld
{
public:
					CAppWorld();
	virtual		~CAppWorld();

	void			setFilter(ULong inBits);
	void			clearFilter(ULong inBits);
	void			setTokenOnly(bool inTokenOnly);
	bool			tokenOnly(void)	const;
	NewtonErr	getError(void)		const;
	CUPort *		getMyPort(void)	const;

	virtual size_t		getSizeOf(void) const;

	virtual NewtonErr	forkInit(CForkWorld * inWorld);
	virtual NewtonErr	forkConstructor(CForkWorld * inWorld);
	virtual void		forkDestructor(void);

	virtual NewtonErr	mainConstructor(void);
	virtual void		mainDestructor(void);
	virtual void		theMain(void);

	virtual NewtonErr	init(ULong inName, bool inHuh, size_t inStackSize);
	virtual NewtonErr	init(ULong inName, bool inHuh, size_t inStackSize, ULong inPriority, ObjectId inEnvironment);

//protected:
	virtual void		interruptHandler(ULong*, CEvent*);
	virtual NewtonErr	eventDispatch(ULong, CUMsgToken*, size_t*, CEvent*);

	CEventHandler *	eventFindHandler(EventClass inClass, EventId inId);
	CEvent *			eventGetEvent(void);
	NewtonErr		eventGetCollectedEvent(ULong inFlags, CUMsgToken * inMsgToken, size_t * outSize, CEvent ** outEvent, OpaqueRef * outRefCon);
	size_t			eventGetMsgSize(void);
	CUMsgToken *	eventGetMsgToken(void);
	long				eventGetMsgType(void);
	NewtonErr		eventInstallHandler(CEventHandler * inHandler);
	NewtonErr		eventRemoveHandler(CEventHandler * inHandler);
	void				eventInstallIdleHandler(CEventHandler * inHandler);
	void				eventRemoveIdleHandler(CEventHandler * inHandler);
	void				eventSetReply(size_t inReply);
	void				eventSetReply(size_t inReply, CEvent * inEvent);
	void				eventSetReply(CUMsgToken * inMsgToken);
	void				eventSetReply(CUMsgToken * inMsgToken, size_t inReply, CEvent * inEvent);
	NewtonErr		eventReplyImmed(void);
	void				eventDeferReply(void);
	void				eventLoop(void);
	void				eventLoop(CAppWorldState * inState);
	void				eventLoop(CAppWorldState * inState, CUMsgToken * inMsgToken);
	void				eventTerminateLoop(void);

	ULong							fPortName;		// +30
	CAppWorldState *			fWorldState;	// +34
	CAppWorldState *			fAppState;		// +38
	CItemComparer				fItemCmp;		// +3C	not really used
	CEventComparer				fEventCmp;		// +48
	CEventHandlerComparer	fHandlerCmp;	// +54
	CSortedList *				fHandlers;		// +60
	CTimerQueue *				fTimers;			// +64
	ULong							fEventId;		// +68
	ULong							fEventClass;	// +6C
// size +70
};

inline void			CAppWorld::setFilter(ULong inBits)			{ fAppState->fFilter |= inBits; }
inline void			CAppWorld::clearFilter(ULong inBits)		{ fAppState->fFilter &= ~inBits; }
inline void			CAppWorld::setTokenOnly(bool inTokenOnly)	{ fAppState->fIsTokenOnly = inTokenOnly; }
inline bool			CAppWorld::tokenOnly(void)		const			{ return fAppState->fIsTokenOnly; }
inline NewtonErr  CAppWorld::getError(void)		const			{ return fAppState->fErr; }
inline CUPort *	CAppWorld::getMyPort(void)		const			{ return fAppState->getPort(); }


NewtonErr	SendRPC(CEventHandler * inHandler, CUPort * inPort, CUAsyncMessage * ioMessage, void * inRequest, size_t inReqSize, void * inReply, size_t inReplySize, Timeout inTimeout, CTime * inTimeToSend, ULong inMsgType, bool inUrgent);


#define gAppWorld static_cast<CAppWorld*>(GetGlobals())

#endif	/* __APPWORLD_H */
