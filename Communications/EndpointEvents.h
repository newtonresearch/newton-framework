/*
	File:		EndpointEvents.h

	Contains:	Endpoint interfaces.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__ENDPOINTEVENTS_H)
#define __ENDPOINTEVENTS_H

#include "EventHandler.h"
#include "TimerQueue.h"

class COptionArray;

class CEndpointEvent : public CEvent
{
public:
				CEndpointEvent();
				CEndpointEvent(long, VAddr inClient, long);

	NewtonErr	f08;
	VAddr			f0C;	// CEndpointEventHandler * ?
	int			f10;
	int			f14;
	CTime			f18;
};


class CDefaultEvent : public CEndpointEvent
{
public:
				CDefaultEvent();

	ULong		f20;
};


class CSndCompleteEvent : public CEndpointEvent
{
public:
				CSndCompleteEvent();

	char *	f20;
	char *	f24;
	int		f28;
	COptionArray *	f2C;
};


class CRcvCompleteEvent : public CEndpointEvent
{
public:
				CRcvCompleteEvent();

	int		f20;
	int		f24;
	int		f28;
	ULong		f2C;
	COptionArray *	f30;
};


class CBindCompleteEvent : public CEndpointEvent
{
public:
				CBindCompleteEvent();
				CBindCompleteEvent(int, ULong, int);

	COptionArray *	f20;
};


class CDisconnectEvent : public CEndpointEvent
{
public:
				CDisconnectEvent();
				CDisconnectEvent(int, ULong);

	COptionArray *	f20;
	int		f24;
	int		f28;
};


class CGetProtAddrCompleteEvent : public CEndpointEvent
{
public:
				CGetProtAddrCompleteEvent(int, ULong);

	COptionArray *	f20;
	int		f24;
};


class COptMgmtCompleteEvent : public CEndpointEvent
{
public:
				COptMgmtCompleteEvent();
				COptMgmtCompleteEvent(int, ULong);

	COptionArray *	f20;
};


class CConnectCompleteEvent : public CEndpointEvent
{
public:
				CConnectCompleteEvent();
				CConnectCompleteEvent(int, ULong, int);

	COptionArray *	f20;
	COptionArray *	f24;
	int		f28;
	int		f2C;
};


class CEndpointEventHandler;
class CEndpointTimer : public CTimerElement
{
public:
				CEndpointTimer(CEndpointEventHandler * inHandler, CTimerQueue * inQueue, ULong inRefCon);

	void		timeout(void);

private:
	CEndpointEventHandler * fEventHandler;	// +18
};


// possibly belongs elsewhere
class CPseudoSyncState : public CUPort
{
public:
					CPseudoSyncState();
					~CPseudoSyncState();

	NewtonErr	init(void);
	NewtonErr	block(ULong inArg);
	void			unblock(void);
};


class CEndpoint;
class CEndpointEventHandler : public CEventHandler
{
public:
					CEndpointEventHandler(CEndpoint * inEndpoint, bool);
	virtual		~CEndpointEventHandler();

	NewtonErr	init(ObjectId inPortId, EventId inEventId, EventClass inEventClass = kNewtEventClass);

	NewtonErr	addToAppWorld(void);
	NewtonErr	removeFromAppWorld(void);

	NewtonErr	addTimer(Timeout inDelta, ULong inRefCon);
	void			killTimer(ULong inRefCon);
	bool			timeout(CEndpointTimer * inTimer);

	bool			useForks(bool);
	NewtonErr	block(ULong);
	void			unblock(void);
	NewtonErr	callService(ULong inMsgType, CUAsyncMessage * ioMessage, CEvent * inRequest, size_t inReqSize, CEvent * inReply, size_t inReplySize, Timeout inTimeout = kNoTimeout, ULong inTimerName = 0, bool inSync = false);
	NewtonErr	callServiceNoForks(ULong inMsgType, CUAsyncMessage * ioMessage, CEvent * inRequest, size_t inReqSize, CEvent * inReply, size_t inReplySize, Timeout inTimeout = kNoTimeout, ULong inTimerName = 0, bool inSync = false);
	void			callService(ULong inMsgType, CUAsyncMessage * ioMessage, CEvent * inRequest, size_t inReqSize, CEvent * inReply, size_t inReplySize, unsigned long, bool inSync = false);
	void			callServiceBlocking(ULong inMsgType, CEvent * inRequest, size_t inReqSize, CEvent * inReply, size_t inReplySize, unsigned long);

	void			doEventLoop(unsigned long);
	void			terminateEventLoop(void);
	void			handleAborts(bool);
	void			abort(void);

	bool			testEvent(CEvent * inEvent);
	void			eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	void			eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	void			idleProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);

	ObjectId		getPrivatePortId(void);
	ObjectId		getServicePortId(void);
	ObjectId		getSyncPortId(void);

private:
	CEndpoint *			f14;
	CUPort				f18;	// +18 service port
	CPseudoSyncState	f20;
	bool					f28;	// +28 isSync;
	bool					f29;
	bool					f2A;
//size +2C
};


#endif	/* __ENDPOINTEVENTS_H */
