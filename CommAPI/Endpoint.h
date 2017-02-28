/*
	File:		Endpoint.h

	Contains:	Protocol Interface for CEndpoints

	Copyright:	� 1992-1995 by Apple Computer, Inc., all rights reserved.
*/

#if !defined(__ENDPOINT_H)
#define __ENDPOINT_H 1

#ifndef	__NEWTON_H
#include "Newton.h"
#endif

#ifndef __PROTOCOLS_H
#include "Protocols.h"
#endif

#ifndef __TRANSPORT_H
#include "Transport.h"
#endif

#ifndef __LONGTIME_H
#include "LongTime.h"
#endif


class CEndpointEventHandler;
class CEventHandler;
class CEvent;
class CBufferSegment;
class CUMsgToken;
class CAppWorld;
class COptionArray;
class CCMOTransportInfo;

// "which" parm to isPending()
#define	kSyncCall				(0x01)
#define	kAsyncCall				(0x02)
#define	kEitherCall				(0x03)

/*--------------------------------------------------------------------------------
	CEndpoint
--------------------------------------------------------------------------------*/

PROTOCOL CEndpoint : public CProtocol
{
public:
	static		CEndpoint *	make(const char * inName);
					void		destroy(void);

// delete the endpoint, but leave the tool running
	NONVIRTUAL	ObjectId		deleteLeavingTool(void);

// called as part of CMGetEndpoint
	NONVIRTUAL	NewtonErr	initBaseEndpoint(CEndpointEventHandler* handler);
	NONVIRTUAL	void			setClientHandler(ULong clientHandler);

// EventHandler interface
					bool			handleEvent(ULong msgType, CEvent* event, ULong msgSize);
					bool			handleComplete(CUMsgToken* msgToken, ULong* msgSize, CEvent* event);

// For handing endpoints around
// passed on to CEndpointEventHandler
					NewtonErr	addToAppWorld(void);				// assumes gAppWorld
					NewtonErr	removeFromAppWorld(void);		// assumes gAppWorld

// Client interface
					NewtonErr	open(ULong clientHandler = 0);
					NewtonErr	close(void);
					NewtonErr	abort(void);

	NONVIRTUAL	NewtonErr	getInfo(CCMOTransportInfo * info);
	NONVIRTUAL	int			getState(void) const;
	NONVIRTUAL	bool			isSync(void) const;
	NONVIRTUAL	void			getAbortParameters(ULong& eventID, ULong& refCon);

					bool			setSync(bool sync);

					NewtonErr	getProtAddr(COptionArray* bndAddr, COptionArray* peerAddr, Timeout timeOut = kNoTimeout);
					NewtonErr	optMgmt(ULong arrayOpCode, COptionArray* options, Timeout timeOut = kNoTimeout);

					NewtonErr	bind(COptionArray* addr = NULL, long* qlen = NULL, Timeout timeOut = kNoTimeout);
					NewtonErr	unbind(Timeout timeOut = kNoTimeout);
					NewtonErr	listen(COptionArray* addr = NULL, COptionArray* opt = NULL, CBufferSegment* data = NULL, long* seq = NULL, Timeout timeOut = kNoTimeout);
					NewtonErr	accept(CEndpoint* resfd, COptionArray* addr = NULL, COptionArray* opt = NULL, CBufferSegment* data = NULL, long seq = 0, Timeout timeOut = kNoTimeout);
					NewtonErr	connect(COptionArray* addr = NULL, COptionArray* opt = NULL, CBufferSegment* data = NULL, long* seq = NULL, Timeout timeOut = kNoTimeout);
					NewtonErr	disconnect(CBufferSegment* data = NULL, long reason = 0, long seq = 0);
					NewtonErr	release(Timeout timeOut = kNoTimeout);

					NewtonErr	snd(UByte* buf, ArrayIndex& nBytes, ULong flags, Timeout timeOut = kNoTimeout);
					NewtonErr	rcv(UByte* buf, ArrayIndex& nBytes, ArrayIndex thresh, ULong* flags, Timeout timeOut = kNoTimeout);

					// alternate versions of same...
					NewtonErr	snd(CBufferSegment* buf, ULong flags, Timeout timeOut = kNoTimeout);
					NewtonErr	rcv(CBufferSegment* buf, ArrayIndex thresh, ULong* flags, Timeout timeOut = kNoTimeout);

					// wait for an expected event to arrive
					NewtonErr	waitForEvent(Timeout timeOut = kNoTimeout);

// convenience functions
	NONVIRTUAL	NewtonErr	easyOpen(ULong clientHandler = 0);		// calls default Open/Bind/Connect
	NONVIRTUAL	NewtonErr	easyConnect(ULong clientHandler = 0, COptionArray* options = NULL, Timeout timeOut = kNoTimeout);
	NONVIRTUAL	NewtonErr	easyClose(void);						// calls default Disconnect/UnBind/Close

protected:
	NONVIRTUAL	void			destroyBaseEndpoint(void);		// impls must call this in ::destroy()

public:
// New calls for 2.0
					NewtonErr	nBind(COptionArray* opt = NULL, Timeout timeOut = kNoTimeout, bool sync = true);
					NewtonErr	nListen(COptionArray* opt = NULL, CBufferSegment* data = NULL, long* seq = NULL, Timeout timeOut = kNoTimeout, bool sync = true);
					NewtonErr	nAccept(CEndpoint* resfd, COptionArray* opt = NULL, CBufferSegment* data = NULL, long seq = 0, Timeout timeOut = kNoTimeout, bool sync = true);
					NewtonErr	nConnect(COptionArray* opt = NULL, CBufferSegment* data = NULL, long* seq = NULL, Timeout timeOut = kNoTimeout, bool sync = true);
					NewtonErr	nRelease(Timeout timeOut = kNoTimeout, bool sync = true);
					NewtonErr	nDisconnect(CBufferSegment* data = NULL, long reason = 0, long seq = 0, Timeout timeOut = kNoTimeout, bool sync = true);
					NewtonErr	nUnBind(Timeout timeOut = kNoTimeout, bool sync = true);
					NewtonErr	nOptMgmt(ULong arrayOpCode, COptionArray* options, Timeout timeOut = kNoTimeout, bool sync = true);
					NewtonErr	nSnd(UByte* buf, ArrayIndex* count, ULong flags, Timeout timeOut = kNoTimeout, bool sync = true, COptionArray* opt = NULL);
					NewtonErr	nRcv(UByte* buf, ArrayIndex* count, ArrayIndex thresh, ULong* flags, Timeout timeOut = kNoTimeout, bool sync = true, COptionArray* opt = NULL);
					NewtonErr	nSnd(CBufferSegment* buf, ULong flags, Timeout timeOut = kNoTimeout, bool sync = true, COptionArray* opt = NULL);
					NewtonErr	nRcv(CBufferSegment* buf, ArrayIndex thresh, ULong* flags, Timeout timeOut = kNoTimeout, bool sync = true, COptionArray* opt = NULL);
					NewtonErr	nAbort(bool sync = true);
					NewtonErr	timeout(ULong refCon);
					bool			isPending(ULong which);

	// So endpoint clients can use forks
	NONVIRTUAL	bool			useForks(bool justDoIt);		// returns old state

protected:
	int							fState;				// state of this endpoint
	CEndpointEventHandler *	fEventHandler;		// our event handler for talking to transport provider
	ULong							fClientRefCon;		// client event handler for setting up client events
	CCMOTransportInfo *		fInfo;				// information about our transport provider
	bool							fSync;				// true if we are currently synchronous
	bool							fToolIsRunning;	// true if the tool is up and running
};

inline int	CEndpoint::getState(void) const	{ return fState; }
inline bool	CEndpoint::isSync(void) const		{ return fSync; }


#endif	/* __ENDPOINT_H */
