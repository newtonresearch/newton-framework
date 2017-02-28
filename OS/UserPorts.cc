/*
	File:		UserPorts.cc

	Contains:	User Port implementation.

	Written by:	Newton Research Group.
*/

#include "UserPorts.h"
#include "KernelObjects.h"
#include "OSErrors.h"

extern CTime GetClock(void);

/*------------------------------------------------------------------------------
	C U M s g T o k e n
------------------------------------------------------------------------------*/

NewtonErr
CUMsgToken::cashMessageToken(size_t * outSize, void * outContent, size_t inSize, ULong inOffset, bool inCopyDone)
{
	NewtonErr err, result = noErr;

	err = SMemCopyFromSharedSWI(fMsgId, outContent, inSize, inOffset, fMsgId, fSignature, outSize);
	if (err != noErr && err != kOSErrSizeTooLargeCopyTruncated)
		result = err;
	if ((fReplyId || inCopyDone)
	 && result != noErr)
		SMemMsgMsgDoneSWI(fMsgId, result, fSignature);

	return err;
}


NewtonErr
CUMsgToken::replyRPC(void * inContent, size_t inSize, NewtonErr inReplyResult)
{
	NewtonErr err;

	if (inContent != kPortSend_BufferAlreadySet
	 && inSize > 0
	 && ((err = SMemCopyToSharedSWI(fReplyId, inContent, inSize, 0, fMsgId, fSignature)) == noErr))
		SMemMsgMsgDoneSWI(fMsgId, inReplyResult, fSignature);
	else
		err = SMemMsgMsgDoneSWI(fMsgId, inReplyResult, fSignature);
	return err;
}

NewtonErr
CUMsgToken::getUserRefCon(OpaqueRef * outRefCon)
{
	return SMemMsgGetUserRefConSWI(fRcvrMsgId ? fRcvrMsgId : fMsgId, outRefCon);
}

#pragma mark -

/*------------------------------------------------------------------------------
	C U A s y n c M e s s a g e
------------------------------------------------------------------------------*/

CUAsyncMessage::CUAsyncMessage()
{
//	fMsg, fReplyMem constructors
}

CUAsyncMessage::CUAsyncMessage(const CUAsyncMessage & inCopy)
{
	fMsg = inCopy.fMsg;
	fReplyMem = inCopy.fReplyMem;
}

CUAsyncMessage::CUAsyncMessage(ObjectId inMemMsg, ObjectId inReplyMem)
{
	fMsg = inMemMsg;
	fReplyMem = inReplyMem;
}

CUAsyncMessage::~CUAsyncMessage()
{
	fMsg = 0;
	fReplyMem = 0;
//	fMsg, fReplyMem destructors
}

void
CUAsyncMessage::operator=(const CUMsgToken & inCopy)
{
	if (inCopy.fRcvrMsgId != 0)
		fMsg = inCopy.fRcvrMsgId;
	else
	{
		fMsg = inCopy.getMsgId();
		fReplyMem = inCopy.getReplyId();
	}
}

NewtonErr
CUAsyncMessage::init(bool inSendRPC)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = fMsg.init())
		if (inSendRPC)
			XFAILIF(err = fReplyMem.init(), fMsg = 0;)
	}
	XENDTRY;
	return err;
}

NewtonErr
CUAsyncMessage::getResult(ObjectId * outSentById, ObjectId * outReplyMemId, ULong * outMsgType, ULong * outSignature)
{
	return SMemMsgCheckForDoneSWI(fMsg, 0, outSentById, outReplyMemId, outMsgType, outSignature);
}

NewtonErr
CUAsyncMessage::abort()
{
	return SMemMsgCheckForDoneSWI(fMsg, 1, NULL, NULL, NULL, NULL);
}

NewtonErr
CUAsyncMessage::abort(CUMsgToken * ioToken, ULong * inMsgType)
{
	ioToken->fRcvrMsgId = fMsg;
	return SMemMsgCheckForDoneSWI(fMsg, 1, &ioToken->fMsgId, &ioToken->fReplyId, inMsgType, &ioToken->fSignature);
}

NewtonErr
CUAsyncMessage::blockTillDone(ObjectId * outSentById, ObjectId * outReplyMemId, ULong * outMsgType, ULong * outSignature)
{
	return SMemMsgCheckForDoneSWI(fMsg, 2, outSentById, outReplyMemId, outMsgType, outSignature);
}


#pragma mark -

/*------------------------------------------------------------------------------
	C U P o r t
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Initialization.
	Ask the object manager monitor to make a port object.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CUPort::init(void)
{
	ObjectMessage	msg;
	return makeObject(kObjectPort, &msg, MSG_SIZE(0));
}


/*------------------------------------------------------------------------------
	Receive a message.
	Args:		outSize					actual size of received message
				outContent				buffer for received message
				inSize					size of that buffer
				ioToken					message token to be built
				outMsgType				type of received message
				inTimeout				time after which to give up if no message received
				inMsgFilter				type of messages weÕre interested in
				onMsgAvail				only bother if message is already available
				tokenOnly
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CUPort::receive(size_t * outSize, void * outContent, size_t inSize, CUMsgToken * ioToken, ULong * outMsgType, Timeout inTimeout, ULong inMsgFilter, bool onMsgAvail, bool tokenOnly)
{
	NewtonErr err = noErr;
	XTRY
	{
		ObjectId		senderMsgId;		// sp10
		ObjectId		replyMemId;			// sp0C
		ULong			msgType = 0;		// sp04
		ULong			signature = 0;		// sp08
		size_t		size = 0;			// sp00

		if (ioToken != NULL)
			ioToken->init();

		ULong	flags = onMsgAvail ? kPortFlags_ReceiveOnMsgAvail : 0;
		if (inTimeout != kNoTimeout)
		{
			flags |= kPortFlags_WantTimeout;
			XFAIL(err = SMemMsgSetTimerParmsSWI(kBuiltInSMemMsgId, inTimeout, 0))
//printf("CUPort::receive() id=%d timeout=%d -> %lld\n", (ObjectId)*this, inTimeout, (int64_t)TimeFromNow(inTimeout));
		}

//printf("CUPort::receive() id=%d ", (ObjectId)*this);
		XFAIL(err = PortReceiveSWI((ObjectId)*this, kBuiltInSMemMsgId, inMsgFilter, flags, &senderMsgId, &replyMemId, &msgType, &signature))
//printf(" sender=%d replyMem=%d\n", senderMsgId, replyMemId);

		if (((msgType & kMsgType_CollectedReceiver) == 0  ||  (err = SMemMsgCheckForDoneSWI(senderMsgId, 0, &senderMsgId, &replyMemId, &msgType, &signature)) == noErr)
		&&  (msgType & kMsgType_CollectedSender) == 0)
		{
			if (tokenOnly)
				err = SMemGetSizeSWI(senderMsgId, &size, NULL, NULL);
			else
				err = SMemCopyFromSharedSWI(senderMsgId, outContent, inSize, 0, senderMsgId, signature, &size);
		}

		// fill in return parameters
		if (outMsgType != NULL)
			*outMsgType = msgType;
		if (outSize != NULL)
			*outSize = size;
		if (ioToken != NULL)
		{
			ioToken->fMsgId = senderMsgId;
			ioToken->fReplyId = replyMemId;
			ioToken->fSignature = signature;
			ioToken->fRcvrMsgId = kNoId;
		}

		XFAIL((err == noErr || err == kOSErrSizeTooLargeCopyTruncated) && ioToken != NULL && !tokenOnly && replyMemId != kNoId)

		NewtonErr result = noErr;
		if (replyMemId != kNoId)
			result = kOSErrReceiverDidNotDoRPC;
		if (err)
			result = err;
		SMemMsgMsgDoneSWI(senderMsgId, result, signature);
	}
	XENDTRY;
	return err;
}


/*------------------------------------------------------------------------------
	Receive a message.
	Args:		inAsync					message area
				inTimeout				time to wait before giving up
				inMsgFilter				type of messages weÕre interested in
				onMsgAvail				only bother if message is already available
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CUPort::receive(CUAsyncMessage * inAsync, Timeout inTimeout, ULong inMsgFilter, bool onMsgAvail)
{
	NewtonErr err = noErr;
	XTRY
	{
		ULong	flags = onMsgAvail ? kPortFlags_ReceiveOnMsgAvail | kPortFlags_Async : kPortFlags_Async;
		ULong	msgId;

		if (inAsync == NULL)
		{
			flags |= kPortFlags_IsMsgAvail;
			msgId = kBuiltInSMemMsgId;
		}
		else
		{
			msgId = inAsync->getMsgId();
			if (inTimeout != kNoTimeout)
			{
				flags |= kPortFlags_WantTimeout;
				XFAIL(err = SMemMsgSetTimerParmsSWI(inAsync->getMsgId(), inTimeout, 0))
			}
		}

		err = PortReceiveSWI((ObjectId)*this, msgId, inMsgFilter, flags, NULL, NULL, NULL, NULL);
	}
	XENDTRY;
	return err;
}


/*------------------------------------------------------------------------------
	Reset the filter for messages weÕre interested in.
	Args:		inAsync					message area
				inMsgFilter				type of messages weÕre interested in
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CUPort::resetMsgFilter(CUAsyncMessage * inAsync, ULong inMsgFilter)
{
	return PortResetFilterSWI((ObjectId)*this, inAsync->getMsgId(), inMsgFilter);
}


/*------------------------------------------------------------------------------
	Reset the sender and receiver flags.
	Args:		inSendersResetFlags
				inReceiversResetFlags
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CUPort::reset(ULong inSendersResetFlags, ULong inReceiversResetFlags)
{
	return GenericSWI(kResetPortFlags, (ObjectId)*this, inSendersResetFlags, inReceiversResetFlags);
}


/*------------------------------------------------------------------------------
	Send a message.
	Args:		inMsgId
				inMemId
				inContent
				inSize
				inMsgType
				inFlags
				inUrgent
				inTimeout
				inFutureTimeToSend
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CUPort::sendGoo(ObjectId inMsgId, ObjectId inMemId, void * inContent, size_t inSize, ULong inMsgType, ULong inFlags, bool inUrgent, Timeout inTimeout, CTime * inFutureTimeToSend)
{
	NewtonErr err = noErr;
	XTRY
	{
		if (inUrgent)
			inFlags |= kPortFlags_Urgent;

		// set up shared mem to hold the request
		if (inContent != kPortSend_BufferAlreadySet)
			XFAIL(err = SMemSetBufferSWI(inMsgId, inContent, inSize, kSMemReadOnly))

		if (inTimeout != kNoTimeout)
			inFlags |= kPortFlags_WantTimeout;
		if (inFutureTimeToSend)
		{
			inFlags |= kPortFlags_WantDelay;
			XFAIL(err = SMemMsgSetTimerParmsSWI(inMsgId, inTimeout, *inFutureTimeToSend))
		}
		else if (inFlags & kPortFlags_WantTimeout)
		{
			XFAIL(err = SMemMsgSetTimerParmsSWI(inMsgId, inTimeout, 0))
		}

		err = PortSendSWI((ObjectId)*this, inMsgId, inMemId, inMsgType, inFlags);
	}
	XENDTRY;
	return err;
}


/*------------------------------------------------------------------------------
	Send a message.
	Args:		inMsgId
				inMemId
				outSize
				inContent
				inSize
				inMsgType
				inFlags
				inUrgent
				ioReplyBuf
				inReplySize
				inTimeout
				inFutureTimeToSend
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CUPort::sendRPCGoo(ObjectId inMsgId, ObjectId inMemId, size_t * outSize, void * inContent, size_t inSize, ULong inMsgType, ULong inFlags, bool inUrgent,
						void * ioReplyBuf, ULong inReplySize, Timeout inTimeout, CTime * inFutureTimeToSend)
{
	NewtonErr err = noErr;
	XTRY
	{
		// set up shared mem to hold the reply
		if (ioReplyBuf != kPortSend_BufferAlreadySet)
			XFAIL(err = SMemSetBufferSWI(inMemId, ioReplyBuf, inReplySize, kSMemReadWrite))

		// send request message
		XFAIL(err = sendGoo(inMsgId, inMemId, inContent, inSize, inMsgType, inFlags, inUrgent, inTimeout, inFutureTimeToSend))

		// if synchronous, return reply size
		if ((inFlags & kPortFlags_Async) == 0)
			err = SMemGetSizeSWI(inMemId, outSize, NULL, NULL);
	}
	XENDTRY;
	return err;
}
