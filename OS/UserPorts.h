/*
	File:		UserPorts.h

	Contains:	User glue to port code.

	Written by:	Newton Research Group.
*/

#if !defined(__USERPORTS_H)
#define __USERPORTS_H 1

#if !defined(__NEWTON_H)
#include "Newton.h"
#endif

#if !defined(__USEROBJECTS_H)
#include "UserObjects.h"
#endif

#if !defined(__USERSHAREDMEM_H)
#include "UserSharedMem.h"
#endif

class CUPort;
class CUAsyncMessage;

/*--------------------------------------------------------------------------------
	C U M s g T o k e n

	Used for receive to get info about sender.
--------------------------------------------------------------------------------*/

class CUMsgToken
{
public:
					CUMsgToken();

	void			init(void);
	NewtonErr	cashMessageToken(size_t * outSize, void * inContent, size_t inSize, ULong inOffset = 0, bool inCopyDone = true);
	NewtonErr	replyRPC(void * inContent, size_t inSize, NewtonErr inReplyResult = noErr);
	NewtonErr	getUserRefCon(OpaqueRef * outRefCon);

	ObjectId		getMsgId() const;
	ObjectId		getReceiverMsgId() const;
	ObjectId		getReplyId() const;
	ObjectId		getSignature() const;

	friend class CUPort;
	friend class CUAsyncMessage;

private:
	ObjectId		fMsgId;			// message senderÕs id
	ObjectId		fReplyId;		// send reply message here
	ULong			fSignature;		// signature used to sync to message sender
	ObjectId		fRcvrMsgId;		// message receiverÕs id on async receive
};

/*------------------------------------------------------------------------------
	C U M s g T o k e n   I n l i n e s
------------------------------------------------------------------------------*/

inline				CUMsgToken::CUMsgToken()					{ init(); }
inline void			CUMsgToken::init()							{ fMsgId = 0; fRcvrMsgId = 0; fReplyId = 0; fSignature = 0; }
inline ObjectId	CUMsgToken::getMsgId() const				{ return fMsgId; }
inline ObjectId	CUMsgToken::getReceiverMsgId() const	{ return fRcvrMsgId; }
inline ObjectId	CUMsgToken::getReplyId() const			{ return fReplyId; }
inline ObjectId	CUMsgToken::getSignature() const			{ return fSignature; }


/*--------------------------------------------------------------------------------
	C U A s y n c M e s s a g e
--------------------------------------------------------------------------------*/

class CUAsyncMessage
{
public:
					CUAsyncMessage();
					CUAsyncMessage(const CUAsyncMessage & inCopy);
					CUAsyncMessage(ObjectId inMemMsg, ObjectId inReplyMem);
					~CUAsyncMessage();

	void	operator=(const CUAsyncMessage & inCopy);
	void	operator=(const CUMsgToken & inCopy);

	NewtonErr	init(bool inSendRPC = true);
	NewtonErr	setCollectorPort(ObjectId inPortId);
	NewtonErr	setUserRefCon(OpaqueRef inRefCon);
	NewtonErr	getUserRefCon(OpaqueRef * outRefCon);
	ObjectId		getMsgId(void) const;
	ObjectId		getReplyMemId(void) const;
	NewtonErr	getResult(ObjectId * sentbyId = NULL, ObjectId * replyMemId = NULL, ULong * msgType = NULL, ULong * signature = NULL);
	NewtonErr	blockTillDone(ObjectId * sentById = NULL, ObjectId * replyMemId = NULL, ULong * msgType = NULL, ULong * signature = NULL);
	NewtonErr	abort();
	NewtonErr	abort(CUMsgToken * inToken, ULong * inMsgType);

private:
	CUSharedMemMsg	fMsg;
	CUSharedMem		fReplyMem;
};

/*--------------------------------------------------------------------------------
	C U A s y n c M e s s a g e   I n l i n e s
--------------------------------------------------------------------------------*/

inline void			CUAsyncMessage::operator=(const CUAsyncMessage & inCopy)	{ fMsg = inCopy.fMsg; fReplyMem = inCopy.fReplyMem; }
inline NewtonErr	CUAsyncMessage::setCollectorPort(ObjectId inPortId)		{ return fMsg.setMsgAvailPort(inPortId); }
inline NewtonErr	CUAsyncMessage::setUserRefCon(OpaqueRef inRefCon)			{ return fMsg.setUserRefCon(inRefCon); }
inline NewtonErr	CUAsyncMessage::getUserRefCon(OpaqueRef * outRefCon)		{ return fMsg.getUserRefCon(outRefCon); }
inline ObjectId	CUAsyncMessage::getMsgId(void) const							{ return fMsg; }
inline ObjectId	CUAsyncMessage::getReplyMemId(void) const						{ return fReplyMem; }


/*--------------------------------------------------------------------------------
	C R P C I n f o
--------------------------------------------------------------------------------*/

class CRPCInfo
{
public:
					CRPCInfo();

	ULong			fType;
	OpaqueRef	fData;
};


/*--------------------------------------------------------------------------------
	C U P o r t

	When a futureTimeToSend is specified in any of the following calls, it represents a moment in time.
	A moment in time is relative to the current time.
	So, if I wanted to have a message sent in a day, I would:
		CTime futureTime = GetGlobalTime() + CTime(1*24*60,kMinutes);
														(1 day, 24 hours in a day, 60 minutes in an hour)
	gives me the time in the future to send the message.

	The timeout option below represents the duration of the call itself, and is represented by a 32 bit number.
	This means that you can have a timeout of no more than 14 minutes in the future
	(the number of 200ns time units that fit in 32 bits).

	To set the timeout paramater you can supply a number in any unit up to minutes.
	For example, if you want a 1 second timeout you could use either:
		1*kSeconds
	or:
		1000*kMilliseconds
--------------------------------------------------------------------------------*/

class CUPort : public CUObject
{
public:
					CUPort();
					CUPort(ObjectId id);
	void			operator=(const CUPort & inCopy);
	NewtonErr	init();

	NewtonErr	send(void * inContent, size_t inSize, Timeout inTimeout = kNoTimeout, ULong inMsgType = 0, bool inUrgent = false)
							{ return sendGoo(kBuiltInSMemMsgId, 0, inContent, inSize, inMsgType, 0, inUrgent, inTimeout, 0); }
	NewtonErr	sendRPC(size_t * outSize, void * inContent, size_t inSize, void * ioReplyBuf, size_t inReplySize, Timeout inTimeout = kNoTimeout, ULong inMsgType = 0, bool inUrgent = false)
							{ return sendRPCGoo(kBuiltInSMemMsgId, kBuiltInSMemId, outSize, inContent, inSize, inMsgType, 0, inUrgent, ioReplyBuf, inReplySize, inTimeout, NULL); }

				// async versions of send...
	NewtonErr	send(CUAsyncMessage * inAsync, void * inContent, size_t inSize, Timeout inTimeout = kNoTimeout, CTime * inFutureTimeToSend = NULL, ULong inMsgType = 0, bool inUrgent = false)
							{ return sendGoo(inAsync->getMsgId(), 0, inContent, inSize, inMsgType, kPortFlags_Async, inUrgent, inTimeout, inFutureTimeToSend); }
	NewtonErr	sendRPC(CUAsyncMessage* async, void * inContent, size_t inSize, void * ioReplyBuf, size_t inReplySize, Timeout inTimeout = kNoTimeout, CTime * inFutureTimeToSend = NULL, ULong inMsgType = 0, bool inUrgent = false)
							{ return sendRPCGoo(async->getMsgId(), async->getReplyMemId(), NULL, inContent, inSize, inMsgType, kPortFlags_Async, inUrgent, ioReplyBuf, inReplySize, inTimeout, inFutureTimeToSend); }

	NewtonErr	receive(size_t * outSize, void * inContent, size_t inSize, CUMsgToken * inToken = NULL, ULong * outMsgType = NULL, Timeout inTimeout = kNoTimeout, ULong inMsgFilter = kMsgType_MatchAll, bool onMsgAvail = false, bool tokenOnly = false);
	NewtonErr	receive(CUAsyncMessage * inAsync, Timeout inTimeout = kNoTimeout, ULong inMsgFilter = kMsgType_MatchAll, bool onMsgAvail = false);

	NewtonErr	isMsgAvailable(ULong inMsgFilter = kMsgType_MatchAll)
							{ return receive(NULL, (Timeout) kNoTimeout, inMsgFilter); }

	NewtonErr	resetMsgFilter(CUAsyncMessage * inAsync, ULong inMsgFilter);

	NewtonErr	reset(ULong inSendersResetFlags, ULong inReceiversResetFlags);

private:
	NewtonErr	sendGoo(ObjectId inMsgId, ObjectId inReplyId, void * inContent, size_t inSize, ULong inMsgType, ULong inFlags, bool inUrgent, Timeout inTimeout, CTime * inFutureTimeToSend);
	NewtonErr	sendRPCGoo(ObjectId inMsgId, ObjectId inReplyId, size_t * outSize, void * content, size_t size, ULong msgType, ULong flags, bool urgent,
							void * ioReplyBuf, ULong inReplySize, Timeout inTimeout, CTime * inFutureTimeToSend);
	NewtonErr	sendForSleepTill(CTime * inFutureTimeToSend)
							{ return sendGoo(kBuiltInSMemMsgId, 0, NULL, 0, 0, 0, false, kTimeOutImmediate, inFutureTimeToSend); }

	friend void SleepTill(CTime * inFutureTimeToSend);
};

/*------------------------------------------------------------------------------
	C U P o r t   I n l i n e s
------------------------------------------------------------------------------*/

inline			CUPort::CUPort() : CUObject(0) { }
inline			CUPort::CUPort(ObjectId id) : CUObject(id) { }
inline 	void	CUPort::operator=(const CUPort & inCopy) { copyObject(inCopy); }


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern NewtonErr SendForInterrupt(ObjectId inPortId, ObjectId inMsgId, ObjectId inReplyId,
						void * inContent, size_t inSize,
						ULong inMsgType = kMsgType_FromInterrupt,
						Timeout inTimeout = 0, CTime * inFutureTimeToSend = NULL,
						bool inUrgent = false);


#endif	/* __USERPORTS_H */
