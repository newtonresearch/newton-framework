/*
	File:		CMWorld.h

	Contains:	Communications Manager task world.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__CMWORLD_H)
#define __CMWORLD_H 1

#include "AppWorld.h"
#include "CMService.h"


/* -----------------------------------------------------------------------------
	C C M E v e n t
----------------------------------------------------------------------------- */

class CCMEvent : public CEvent
{
public:
	NewtonErr			f0C;
	CConnectedDevice	f10;
};


/* -----------------------------------------------------------------------------
	C I C H a n d l e r
	Interconnect handler.
----------------------------------------------------------------------------- */

class CICHandler
{
public:
	NewtonErr	init(ObjectId inPortId);

	void			send(ULong);
	void			sendICMessage(ULong);
	void			setTimer(Timeout);
	void			resetTimer(Timeout);
	void			sampleInterconnectStateMachine(void);
	void			ic_InterruptHandler(void);
	void			ic_TimerInterruptHandler(void);

private:
	CUAsyncMessage *	f00;
	CEvent		f04;
	ObjectId		f14;
	long			f18;
	CEvent		f1C;
	CUAsyncMessage *	f30;
	long			f34;
//	KeynesIntObject *	f38;
// size+3C
};


/* -----------------------------------------------------------------------------
	C C M E v e n t H a n d l e r
----------------------------------------------------------------------------- */

class CCMEventHandler : public CEventHandler
{
public:
						CCMEventHandler();

	NewtonErr	init(EventId inEventId, EventClass inEventClass = kNewtEventClass);

	virtual	void	eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual	void	eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);

	NewtonErr	startService(CUMsgToken * inToken, CCMEvent * inEvent);
	NewtonErr	getLastDevice(CCMEvent * inEvent);
	NewtonErr	setLastDevice(CCMEvent * inEvent);
	NewtonErr	getLastPackage(CCMEvent * inEvent);
	NewtonErr	setLastPackage(CCMEvent * inEvent);

private:
	CICHandler *	fInterconnectHandler;
//size+20
};


/* -----------------------------------------------------------------------------
	C C M S y t e m E v e n t H a n d l e r
----------------------------------------------------------------------------- */

class CCMSystemEventHandler : public CSystemEventHandler
{
public:
	void		powerOn(CEvent * inEvent);
	void		powerOff(CEvent * inEvent);
	void		appAlive(CEvent * inEvent);

private:
//size+18
};


/* -----------------------------------------------------------------------------
	C C M S C P A s y n c M e s s a g e
----------------------------------------------------------------------------- */

class CCMSCPAsyncMessage : public CUAsyncMessage
{
public:
				CCMSCPAsyncMessage();

	NewtonErr	init(ObjectId inPortId, CEventHandler * inHandler);
	void			setToken(CUMsgToken * inToken);

	NewtonErr	sendRPC(CUPort * inPort);
	NewtonErr	replyRPC(void);

private:
	CEvent		f10;		// request
	CEvent		f30;		// reply
	bool			f50;
	CUMsgToken	f54;
// size+64
};


/* -----------------------------------------------------------------------------
	C C M W o r l d
----------------------------------------------------------------------------- */

class CCMWorld : public CAppWorld
{
public:
				CCMWorld();

	virtual size_t		getSizeOf(void) const;

	virtual NewtonErr	mainConstructor(void);
	virtual void		mainDestructor(void);

	CAsyncServiceMessage *	matchPendingServiceMessage(CUMsgToken * inMsg);
	void *			matchPendingStartInfo(CCMService * inService);
	void			setDevice(CConnectedDevice * inDevice);
	void			setLastPackage(ULong, ULong);
	NewtonErr	SCPCheck(ULong);
	NewtonErr	SCPLoad(ULong inWaitPeriod, ULong inNumOfTries, ULong inFilter, CUMsgToken * inToken, ULong);

private:
	friend class CCMEventHandler;

	CCMEventHandler		f70;
	CList						f90;
	CList						fA8;
	CConnectedDevice		fDevice;	// +C0
	CCMSCPAsyncMessage *	fD8;
	ULong						fDC;		// last device
	ULong						fE0;		// last package
//	size +E4
};


#endif	/* __CMWORLD_H */
