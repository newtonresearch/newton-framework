/*
	File:		KernelPorts.cc

	Contains:	Kernel port implementation.

	Written by:	Newton Research Group.
*/

#include "KernelPorts.h"
#include "KernelGlobals.h"
#include "Timers.h"
#include "OSErrors.h"

/*--------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
--------------------------------------------------------------------------------*/

extern ObjectId	LocalToGlobalId(ObjectId inId);

extern "C" void	PortSendKernelGlue(ObjectId inPortId, ObjectId inMsgId, ObjectId inMemId, ULong inMsgType, ULong inFlags);
extern "C" void	PortReceiveKernelGlue(ObjectId inPortId, ObjectId inMsgId, ULong inMsgFilter, ULong inFlags);
extern "C" void	PortResetFilterKernelGlue(ObjectId inPortId, ObjectId inMsgId, ULong inMsgFilter);
extern "C" void	PortResetKernelGlue(ObjectId inPortId, ULong inSenderFlags, ULong inReceiverFlags);


/*--------------------------------------------------------------------------------
	D a t a
--------------------------------------------------------------------------------*/

CUPort *	gUNullPort;


/*--------------------------------------------------------------------------------
	Constructor.
--------------------------------------------------------------------------------*/

CPort::CPort()
	:	fSenders(offsetof(CSharedMemMsg, f80)),
		fReceivers(offsetof(CSharedMemMsg, f80))
{ }


/*--------------------------------------------------------------------------------
	Destructor.
	Remove all sender and receiver task messages from their queues.
--------------------------------------------------------------------------------*/

CPort::~CPort()
{
	EnterFIQAtomic();

	CSharedMemMsg *	msg;
	while ((msg = (CSharedMemMsg *)fSenders.remove()) != NULL)
	{
		gTimerEngine->remove(msg);
		ExitFIQAtomic();
		msg->completeSender(kOSErrPortNoLongerExists);
		EnterFIQAtomic();
	}

	while ((msg = (CSharedMemMsg *)fReceivers.remove()) != NULL)
	{
		gTimerEngine->remove(msg);
		ExitFIQAtomic();
		msg->completeReceiver(NULL, kOSErrPortNoLongerExists);
		EnterFIQAtomic();
	}

	ExitFIQAtomic();
}


/*--------------------------------------------------------------------------------
	Reset the sender and receiver task queues.
	Args:		inSenderFlags			how sender task messages should be flagged
				inReceiverFlags		how receiver task messages should be flagged
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CPort::reset(ULong inSenderFlags, ULong inReceiverFlags)
{
	EnterFIQAtomic();

	if (inSenderFlags != 0)
	{
		NewtonErr status = FLAGTEST(inSenderFlags, kPortFlags_Abort) ? kOSErrCallAborted : kOSErrMessageTimedOut;
		CSharedMemMsg * msg;
		while ((msg = (CSharedMemMsg *)fSenders.remove()) != NULL)
		{
			gTimerEngine->remove(msg);
			ExitFIQAtomic();
			msg->completeSender(status);
			EnterFIQAtomic();
		}
	}

	if (inReceiverFlags != 0)
	{
		NewtonErr status = (inReceiverFlags & kPortFlags_Abort) ? kOSErrCallAborted : kOSErrMessageTimedOut;
		CSharedMemMsg * msg;
		while ((msg = (CSharedMemMsg *)fReceivers.remove()) != NULL)
		{
			gTimerEngine->remove(msg);
			ExitFIQAtomic();
			msg->completeReceiver(NULL, status);
			EnterFIQAtomic();
		}
	}

	ExitFIQAtomic();
	return noErr;
}


/*--------------------------------------------------------------------------------
	Reset a messageÕs filter.
	Args:		inMsg				the message
				inFilter			its new filter
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CPort::resetFilter(CSharedMemMsg * inMsg, ULong inFilter)
{
	NewtonErr err = noErr;
	XTRY
	{
		CSharedMemMsg * msg;

		// someone must be waiting to receive the message
		for (msg = (CSharedMemMsg *)fReceivers.peek(); msg != NULL; msg = (CSharedMemMsg *)fReceivers.getNext(msg))
			if (msg == inMsg)
				break;
		XFAILNOT(msg, err = kOSErrNoMessageWaiting;)

		// update the messageÕs filter
		inMsg->fFilter = inFilter;

		// see if someoneÕs sent a suitable message
		for (msg = (CSharedMemMsg *)fSenders.peek(); msg != NULL; msg = (CSharedMemMsg *)fSenders.getNext(msg))
			if (msg->fFilter == kMsgType_MatchAll
			|| (msg->fFilter & inMsg->fType) != 0)
				break;
		if (msg)
		{
			// they have, it can be completed immediately
			fSenders.removeFromQueue(msg);
			inMsg->completeReceiver(msg);
		}
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Tasks call here to send a message.
	Args:		inMsg				the message
				inFlags			how it should be sent
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CPort::send(CSharedMemMsg * inMsg, ULong inFlags)
{
	NewtonErr err = noErr;
	XTRY
	{
		CSharedMemMsg * msg;

		// if message is to be sent async, queue it
		if (FLAGTEST(inFlags, kPortFlags_TimerWanted) && inMsg->fTimeout != 0)
		{
			XFAILNOT(gTimerEngine->queueTimeout(inMsg), err = kOSErrMessageTimedOut;)
			inMsg->fFlags |= 0x0080;
		}

		// see if someoneÕs waiting for this message
		for (msg = (CSharedMemMsg *)fReceivers.peek(); msg != NULL; msg = (CSharedMemMsg *)fReceivers.getNext(msg))
			if (msg->fFilter == kMsgType_MatchAll
			|| (msg->fFilter & inMsg->fType) != 0)
				break;

		if (FLAGTEST(inFlags, kPortFlags_CanRemoveTask))
			UnScheduleTask(gCurrentTask);

		if (msg != NULL)
		{
			// someoneÕs waiting for this message, let the receiver have it
			if (FLAGTEST(inFlags, kPortFlags_ScheduleOnSend))
				WantSchedule();
			fReceivers.removeFromQueue(msg);
			msg->completeReceiver(inMsg);
		}
		else
		{
			// nooneÕs waiting for this message, add it to the senders
			if (FLAGTEST(inFlags, kPortFlags_Urgent))
				fSenders.addToFront(inMsg);
			else
				fSenders.add(inMsg);
		}
	}
	XENDTRY;
	XDOFAIL(err)
	{
		inMsg->fStatus = err;
	}
	XENDFAIL;
	return err;
}


/*--------------------------------------------------------------------------------
	Tasks call here to receive a message.
	Args:		inMsg				the message
				inFlags			how it should be received
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CPort::receive(CSharedMemMsg * inMsg, ULong inFlags)
{
	NewtonErr err = noErr;
	XTRY
	{
		CSharedMemMsg * msg;

		// if message is async, queue it up
		if (FLAGTEST(inFlags, kPortFlags_TimerWanted) && inMsg->fTimeout != 0)
		{
			XFAILNOT(gTimerEngine->queueTimeout(inMsg), err = kOSErrMessageTimedOut;)
		}

		// see if someoneÕs sent a suitable message
		for (msg = (CSharedMemMsg *)fSenders.peek(); msg != NULL; msg = (CSharedMemMsg *)fSenders.getNext(msg))
			if (inMsg->fFilter == kMsgType_MatchAll
			|| (inMsg->fFilter & msg->fType) != 0)
				break;

		// if the caller wants to check whether itÕs available, stop it timing out
		if (FLAGTEST(inFlags, kPortFlags_IsMsgAvail))
		{
			inMsg->fStatus = noErr;
			gTimerEngine->remove(inMsg);
			XFAILNOT(msg, err = kOSErrNoMessageWaiting;)
		}

		if (msg != NULL)
		{
			// someoneÕs sent the message weÕre waiting for
			fSenders.removeFromQueue(msg);
			inMsg->completeReceiver(msg);
		}
		else
		{
			// nooneÕs sent the messageÉ
			if (FLAGTEST(inFlags, kPortFlags_ReceiveOnMsgAvail))
			{
				// Éand weÕre not prepared to wait
				gTimerEngine->remove(inMsg);
				err = kOSErrNoMessageWaiting;
			}
			else
			{
				// ÉweÕll wait
				if (FLAGTEST(inFlags, kPortFlags_CanRemoveTask))
					// stop this task
					UnScheduleTask(gCurrentTask);
				// add message to the receivers waiting
				if (FLAGTEST(inFlags, kPortFlags_Urgent))
					fReceivers.addToFront(inMsg);
				else
					fReceivers.add(inMsg);
			}
		}
	}
	XENDTRY;
	XDOFAIL(err)
	{
		inMsg->fStatus = err;
	}
	XENDFAIL;
	return err;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Send callback from SWI.
	Args:		inPortId
				inMsgId
				inMemId
				inMsgType
				inFlags
	Return:	--
------------------------------------------------------------------------------*/

void
PortSendKernelGlue(ObjectId inPortId, ObjectId inMsgId, ObjectId inMemId, ULong inMsgType, ULong inFlags)
{
	NewtonErr err;

	// assume innocence until proven guilty
	gCurrentTask->fRegister[kResultReg] = noErr;

	XTRY
	{
		CPort *	port;
		CSharedMemMsg *	msg;

		// get port and message objects from ids
		XFAIL(err = ConvertIdToObj(kPortType, inPortId, &port))
		XFAIL(err = ConvertIdToObj(kSharedMemMsgType, inMsgId, &msg))

		if (inMemId != kNoId)
		// check shared mem actually exists
			XFAIL(err = ConvertIdToObj(kSharedMemType, inMemId, NULL))

		EnterFIQAtomic();
		// send already in progress?
		XFAILIF(msg->fStatus == kInProgress || MASKTEST(msg->fType, kMsgType_CollectorMask) != 0, ExitFIQAtomic(); err = kOSErrMessageAlreadyPosted;)
		// update msg status
		msg->fStatus = kInProgress;
		ExitFIQAtomic();

		inFlags &= kPortFlags_ReservedMask;
		msg->fReplyMem = LocalToGlobalId(inMemId);
		msg->fType = inMsgType & kMsgType_ReservedMask;
		if (msg->fType == 0)
			msg->fType = kMsgType_NoMsgTypeSet;
		msg->fSendingPort = inPortId;
		msg->fTaskToNotify = *gCurrentTask;
		msg->fFlags = 0;
		if (FLAGTEST(inFlags, kPortFlags_WantTimeout))
			inFlags |= kPortFlags_TimerWanted;
		msg->fNotify = FLAGTEST(inFlags, kPortFlags_Async) ? msg->fPortToNotify : msg->fTaskToNotify;
		if (!FLAGTEST(inFlags, kPortFlags_Async))
			inFlags |= kPortFlags_CanRemoveTask;

		if (FLAGTEST(inFlags, kPortFlags_WantDelay) && gTimerEngine->queueDelay(msg))
		{
			if (FLAGTEST(inFlags, kPortFlags_CanRemoveTask))
				UnScheduleTask(gCurrentTask);
			msg->fFilter = inFlags & ~kPortFlags_CanRemoveTask;	// set flags for NotifySend()
			break;	// donÕt send it now
		}

		err = port->send(msg, inFlags);
	}
	XENDTRY;

	// if any errors, return them to the task
	XDOFAIL(err)
	{
		gCurrentTask->fRegister[kResultReg] = err;
	}
	XENDFAIL;
}


/*------------------------------------------------------------------------------
	Receive callback from SWI.
	Args:		inPortId
				inMsgId
				inMsgFilter
				inFlags
	Return:	--
------------------------------------------------------------------------------*/

void
PortReceiveKernelGlue(ObjectId inPortId, ObjectId inMsgId, ULong inMsgFilter, ULong inFlags)
{
	NewtonErr err;

	// assume innocence until proven guilty
	gCurrentTask->fRegister[kResultReg] = noErr;

	XTRY
	{
		CPort *	port;
		CSharedMemMsg *	msg;

		// get port and message objects from ids
		XFAIL(err = ConvertIdToObj(kPortType, inPortId, &port))
		XFAIL(err = ConvertIdToObj(kSharedMemMsgType, inMsgId, &msg))

		EnterFIQAtomic();
		// send already in progress?
		XFAILIF(msg->fStatus == kInProgress || (msg->fType & kMsgType_CollectorMask) != 0, ExitFIQAtomic(); err = kOSErrMessageAlreadyPosted;)
		// update msg status
		msg->fStatus = kInProgress;
		ExitFIQAtomic();

		inFlags &= kPortFlags_ReservedMask;
		msg->fType = 0;
		msg->fFilter = inMsgFilter;
		msg->fTaskToNotify = *gCurrentTask;
		msg->fFlags = 0;
		msg->fNotify = FLAGTEST(inFlags, kPortFlags_Async) ? msg->fPortToNotify : msg->fTaskToNotify;
		if (!FLAGTEST(inFlags, kPortFlags_Async))
			inFlags |= kPortFlags_CanRemoveTask;
		if (FLAGTEST(inFlags, kPortFlags_WantTimeout))
			inFlags |= kPortFlags_TimerWanted;

		err = port->receive(msg, inFlags);
	}
	XENDTRY;

	// if any errors, return them to the task
	XDOFAIL(err)
	{
		gCurrentTask->fRegister[kResultReg] = err;
	}
	XENDFAIL;
}


/*------------------------------------------------------------------------------
	Reset filter callback from SWI.
	Args:		inPortId
				inMsgId
				inMsgFilter
	Return:	--
------------------------------------------------------------------------------*/

void
PortResetFilterKernelGlue(ObjectId inPortId, ObjectId inMsgId, ULong inMsgFilter)
{
	NewtonErr err;

	// assume innocence until proven guilty
	gCurrentTask->fRegister[kResultReg] = noErr;

	XTRY
	{
		CPort *	port;
		CSharedMemMsg *	msg;

		// get port and message objects from ids
		XFAIL(err = ConvertIdToObj(kPortType, inPortId, &port))
		XFAIL(err = ConvertIdToObj(kSharedMemMsgType, inMsgId, &msg))

		err = port->resetFilter(msg, inMsgFilter);
	}
	XENDTRY;

	// if any errors, return them to the task
	XDOFAIL(err)
	{
		gCurrentTask->fRegister[kResultReg] = err;
	}
	XENDFAIL;
}


/*------------------------------------------------------------------------------
	Reset callback from generic SWI.
	Args:		inPortId
				inSenderFlags
				inReceiverFlags
	Return:	--
------------------------------------------------------------------------------*/

void
PortResetKernelGlue(ObjectId inPortId, ULong inSenderFlags, ULong inReceiverFlags)
{
	NewtonErr err;

	// assume innocence until proven guilty
	gCurrentTask->fRegister[kResultReg] = noErr;

	XTRY
	{
		CPort *	port;

		// get port object from id and reset it
		XFAIL(err = ConvertIdToObj(kPortType, inPortId, &port))
		err = port->reset(inSenderFlags, inReceiverFlags);
	}
	XENDTRY;

	// if any errors, return them to the task
	XDOFAIL(err)
	{
		gCurrentTask->fRegister[kResultReg] = err;
	}
	XENDFAIL;
}
