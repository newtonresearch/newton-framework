/*
	File:		SerialEndpoint.h

	Contains:	Serial endpoint declarations.

	Written by:	Newton Research Group, 2009.
*/

#if !defined(__SERIALENDPOINT_H)
#define __SERIALENDPOINT_H 1

#include "Endpoint.h"
#include "EndpointEvents.h"
#include "CommTool.h"
#include "BufferList.h"
#include "OptionArray.h"


/*--------------------------------------------------------------------------------
	CCommToolPB
--------------------------------------------------------------------------------*/

class CCommToolPB
{
public:
			CCommToolPB(CommToolRequestType inReqType, ULong inClientHandler, bool inAsync);

	CUAsyncMessage			fMsg;					// +00
 	CommToolRequestType	fReqType;			// f10
	ULong						fClientRefCon;		// +14
	bool						fIsAsync;			// +18
};


/*--------------------------------------------------------------------------------
	CCommToolBindPB
--------------------------------------------------------------------------------*/

class CCommToolBindPB : public CCommToolPB
{
public:
			CCommToolBindPB(ULong inClientHandler, ULong, bool, long);

	CCommToolBindRequest		fRequest;			// +1C
	CCommToolReply				fReply;				// +3C
	CBindCompleteEvent		fCompletionEvent;	// +4C
// size +70
};


/*--------------------------------------------------------------------------------
	CCommToolConnectPB
--------------------------------------------------------------------------------*/

class CCommToolConnectPB : public CCommToolPB
{
public:
			CCommToolConnectPB(ULong inClientHandler, long, ULong, bool inAsync);

			void	prepare(COptionArray *, CBufferSegment *, long *, bool);

	CCommToolConnectRequest	fRequest;			// +1C
	CCommToolConnectReply	fReply;				// +44
	CConnectCompleteEvent	fCompletionEvent;	// +58
// size +88
};


/*--------------------------------------------------------------------------------
	CCommToolDisconnectPB
--------------------------------------------------------------------------------*/

class CCommToolDisconnectPB : public CCommToolPB
{
public:
			CCommToolDisconnectPB(ULong inClientHandler, bool inAsync);

	CCommToolDisconnectRequest	fRequest;			// +1C
	CCommToolReply					fReply;				// +38
	CDisconnectEvent				fCompletionEvent;	// +48
// size +74
};


/*--------------------------------------------------------------------------------
	CCommToolOptMgmtPB
--------------------------------------------------------------------------------*/

class CCommToolOptMgmtPB : public CCommToolPB
{
public:
			CCommToolOptMgmtPB(ULong inClientHandler, bool inAsync);

	CCommToolOptionMgmtRequest	fRequest;			// +1C
	CCommToolReply					fReply;				// +38
	COptMgmtCompleteEvent		fCompletionEvent;	// +48
// size +74
};


/*--------------------------------------------------------------------------------
	CCommToolGetPB
--------------------------------------------------------------------------------*/

class CCommToolGetPB : public CCommToolPB
{
public:
			CCommToolGetPB(ULong inClientHandler, bool inAsync);

	CCommToolGetRequest	fRequest;			// +1C
	CCommToolGetReply		fReply;				// +38
	CRcvCompleteEvent		fCompletionEvent;	// +50

// don’t really know...
	UByte *					f6C;
	CBufferSegment *		f70;
	int						f74;
	COptionArray *			f78;

	CBufferSegment *		fMadeData;			// +84
	CBufferList *			fQueue;				// +88
	bool						f8C;
};


/*--------------------------------------------------------------------------------
	CCommToolPutPB
--------------------------------------------------------------------------------*/

class CCommToolPutPB : public CCommToolPB
{
public:
			CCommToolPutPB(ULong inClientHandler, bool inAsync);

	CCommToolPutRequest	fRequest;			// +1C
	CCommToolPutReply		fReply;				// +38
	CSndCompleteEvent		fCompletionEvent;	// +4C

	UByte *					fRawData;			// +6C
	CBufferSegment *		fData;				// +70
	int						f74;
	COptionArray *			fOptions;			// +78
	CBufferSegment *		fMadeData;			// +7C
	CBufferList *			fQueue;				// +80
};


/*--------------------------------------------------------------------------------
	CCommToolControlPB
--------------------------------------------------------------------------------*/

class CCommToolControlPB : public CCommToolPB
{
public:
			CCommToolControlPB(ULong inArg1, long inArg2, ULong inClientHandler, bool inAsync);

	CCommToolControlRequest	fRequest;			// +1C
	CCommToolReply				fReply;				// +28
	CEndpointEvent				fCompletionEvent;	// +38
// size +58
};


/*--------------------------------------------------------------------------------
	CCommToolAbortPB
--------------------------------------------------------------------------------*/

class CCommToolAbortPB : public CCommToolPB
{
public:
			CCommToolAbortPB(ULong inArg1, ULong inClientHandler, bool inAsync);

	CCommToolKillRequest	fRequest;	// +1C
	CCommToolReply			fReply;		// +28
// size +38
};


/*--------------------------------------------------------------------------------
	CCommToolEventPB
--------------------------------------------------------------------------------*/

class CCommToolEventPB : public CCommToolPB
{
public:
			CCommToolEventPB(ULong inClientHandler);

	CCommToolControlRequest	fRequest;	// +1C
	CCommToolGetEventReply	fReply;		// +28
// size +4C
};


/*--------------------------------------------------------------------------------
	CSerialEndpoint
--------------------------------------------------------------------------------*/


PROTOCOL CSerialEndpoint : public CEndpoint
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CSerialEndpoint)

	CSerialEndpoint *	make(void);
	void			destroy(void);

	NewtonErr	deleteLeavingTool(void);	// overrides NONVIRTUAL that returns ObjectId!!

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

	NewtonErr	setState(int inState);
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

protected:
	NewtonErr	initPending(void);
	void			nukePending(void);

	NewtonErr	initPutPBList(void);
	CCommToolPutPB *	grabPutPB(bool inMake);
	void			releasePutPB(CCommToolPutPB * inPB);
	void			nukePutPBList(void);

	NewtonErr	initGetPBList(void);
	CCommToolGetPB *	grabGetPB(bool inMake);
	void			releaseGetPB(CCommToolGetPB * inPB);
	void			nukeGetPBList(void);

	NewtonErr	sendBytes(CCommToolPutPB * inPB, size_t * ioSize, ULong inFlags, Timeout inTimeout = kNoTimeout, bool inSync = true, COptionArray * inOptions = NULL);
	NewtonErr	recvBytes(CCommToolGetPB * inPB, size_t * ioSize, size_t inThreshold, ULong * ioFlags, Timeout inTimeout = kNoTimeout, bool inSync = true, COptionArray * inOptions = NULL);

	NewtonErr	postEventRequest(CCommToolEventPB * inPB);
	NewtonErr	postKillRequest(ULong, bool);
	NewtonErr	prepareAbort(ULong, bool);

	NewtonErr	killKillKill(ULong, CUAsyncMessage *);

	void			handleGetReply(CCommToolGetPB * inPB);
	void			handlePutReply(CCommToolPutPB * inPB);
	void			handleAbortReply(CCommToolAbortPB * inPB);
	void			handleControlReply(CCommToolControlPB * inPB);
	void			handleConnectReply(CCommToolConnectPB * inPB);
	void			handleListenReply(CCommToolConnectPB * inPB);
	void			handleAcceptReply(CCommToolConnectPB * inPB);
	void			handleDisconnectReply(CCommToolDisconnectPB * inPB);
	void			handleReleaseReply(CCommToolControlPB * inPB);
	void			handleBindReply(CCommToolBindPB * inPB);
	void			handleUnbindReply(CCommToolBindPB * inPB);
	void			handleOptMgmtReply(CCommToolOptMgmtPB * inPB);

private:
// start @24
	CList *					fPendingList;		// +24
	CList *					fPutPBList;			// +28
	CList *					fGetPBList;			// +2C
	CCommToolEventPB *	fCommToolEvent;	// +30
	long						f34;
	CPseudoSyncState *	f38;
	CPseudoSyncState *	f3C;
	bool						f40;
	bool						f41;
	bool						f42;
	bool						f43;
// size +44
};


#endif	/* __SERIALENDPOINT_H */
