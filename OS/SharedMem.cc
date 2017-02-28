/*
	File:		SharedMem.cc

	Contains:	Task interface to shared memory objects.

	Written by:	Newton Research Group.
*/

#include "SharedMem.h"
#include "KernelGlobals.h"
#include "PageManager.h"
#include "OSErrors.h"


/*--------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s

	Glue functions must have C linkage; they’re called from assembler.
--------------------------------------------------------------------------------*/

extern "C"
{
	NewtonErr	SMemSetBufferKernelGlue(ObjectId inId, void * inBuffer, size_t inSize, ULong inPermissions);
	NewtonErr	SMemGetSizeKernelGlue(ObjectId inId);
	NewtonErr	SMemCopyToKernelGlue(ObjectId inId, void * inBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature);
	NewtonErr	SMemCopyFromKernelGlue(ObjectId inId, void * outBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature);
	NewtonErr	SMemMsgSetTimerParmsKernelGlue(ObjectId inId, Timeout inTimeout, int64_t inDelay);
	NewtonErr	SMemMsgSetMsgAvailPortKernelGlue(ObjectId inId, ObjectId inPortId);
	NewtonErr	SMemMsgGetSenderTaskIdKernelGlue(ObjectId inId);
	NewtonErr	SMemMsgSetUserRefConKernelGlue(ObjectId inId, ULong inRefCon);
	NewtonErr	SMemMsgGetUserRefConKernelGlue(ObjectId inId);
	NewtonErr	SMemMsgCheckForDoneKernelGlue(ObjectId inId, ULong inFlags);
	NewtonErr	SMemMsgMsgDoneKernelGlue(ObjectId inId, long inResult, ULong inSignature);
	NewtonErr	LowLevelCopyDoneFromKernelGlue(NewtonErr inErr, CTask * inTask, VAddr inReturn);
}

/*--------------------------------------------------------------------------------
	D a t a
--------------------------------------------------------------------------------*/

bool  gWantDeferred = false;


/*--------------------------------------------------------------------------------
	G l u e
--------------------------------------------------------------------------------*/
extern NewtonErr	ConvertMemOrMsgIdToObj(ObjectId inId, CSharedMem ** outObj);

NewtonErr
SMemSetBufferKernelGlue(ObjectId inId, void * inBuffer, size_t inSize, ULong inPermissions)
{
	NewtonErr err;
	XTRY
	{
		CSharedMem *	mem;
		XFAIL(err = ConvertMemOrMsgIdToObj(inId, &mem))
		mem->fAddr = inBuffer;
		mem->fBufSize =
		mem->fCurSize = inSize;
		mem->fPerm = inPermissions;
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Get the size (and other info) of a shared memory object.
	Supervisor mode callback from SWI.
	Args:		inId				id of CSharedMem
	Return:	error code
				gCurrentTask->fRegister[kReturnParm1]	-> message size
				gCurrentTask->fRegister[kReturnParm2]	-> message address
				gCurrentTask->fRegister[kReturnParm3]	-> refCon
--------------------------------------------------------------------------------*/

NewtonErr
SMemGetSizeKernelGlue(ObjectId inId)
{
	NewtonErr err;
	XTRY
	{
		CSharedMemMsg *	msg;
		CSharedMem *		mem;

		XFAIL(err = ConvertMemOrMsgIdToObj(inId, &mem))
		gCurrentTask->fRegister[kReturnParm1] = mem->fCurSize;

		ObjectId	memOwner = mem->owner();
		bool		isMemOwnedByTask;
		while (!(isMemOwnedByTask = (memOwner == *gCurrentTask)) && memOwner != kNoId)
		{
		// current task doesn’t own the memory
		// look for the task to whom the memory has been bequeathed
			CTask *	task;
			XFAIL(err = ConvertIdToObj(kTaskType, memOwner, &task))
			memOwner = task->fBequeathId;
		}
		gCurrentTask->fRegister[kReturnParm2] = isMemOwnedByTask ? (VAddr) mem->fAddr : 0;
		gCurrentTask->fRegister[kReturnParm3] = (ConvertIdToObj(kSharedMemMsgType, inId, &msg) == noErr)? msg->fRefCon : 0;	// refCon
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Copy a block of memory into a shared memory object.
	Supervisor mode callback from SWI.
	Args:		inId				id of CSharedMem
				inBuffer			address of block of memory
				inSize			size of the block to be copied
				inOffset			offset to start of the block to be copied
				inSendersMsgId	id of the sending task’s message
				inSendersSignature	signature of the sending task
	Return:	< 0 => error code
				else copy chunk size (byte alignment) 1 or 4
				gCurrentTask->fRegister[kCopySize]		-> size of buffer
				gCurrentTask->fRegister[kCopyFromBuf]	-> source
				gCurrentTask->fRegister[kCopyToBuf]		-> destination
--------------------------------------------------------------------------------*/

NewtonErr
SMemCopyToKernelGlue(ObjectId inId, void * inBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature)
{
	NewtonErr err;
	XTRY
	{
		CSharedMemMsg *	sendersMsg = NULL;
		CSharedMem *		mem;

		gCurrentTask->fRegister[kCopySize] = 0;

		XFAIL(err = ConvertMemOrMsgIdToObj(inId, &mem))
		XFAILIF(FLAGTEST(mem->fPerm, kSMemReadOnly), err = kOSErrSMemModeViolation;)
		if (inSendersMsgId != kNoId)
		{
			XFAIL(err = ConvertIdToObj(kSharedMemMsgType, inSendersMsgId, &sendersMsg))
			XFAILIF(sendersMsg->fCopyTask != kNoId, err = kOSErrCallAlreadyInProgress;)
			XFAILIF(sendersMsg->fStatus != kInProgress, err = kOSErrCallNotInProgress;)
			XFAILIF(sendersMsg->fSignature != inSendersSignature, err = kOSErrBadSignature;)
		}
		if (inOffset + inSize > mem->fBufSize)
		{
			err = kOSErrSizeTooLargeCopyTruncated;
			inSize = mem->fBufSize - inOffset;
			if (!FLAGTEST(mem->fPerm, kSMemNoSizeChangeOnCopyTo))
				mem->fCurSize = mem->fBufSize;
		}
		else if (!FLAGTEST(mem->fPerm, kSMemNoSizeChangeOnCopyTo))
			mem->fCurSize = inOffset + inSize;

		if (inSize != 0)
		{
			if (sendersMsg != NULL)
				sendersMsg->fCopyTask = *gCurrentTask;
			gCurrentTask->fCopySavedMemMsgId = inSendersMsgId;
			gCurrentTask->fCopySavedPC = gCurrentTask->fRegister[kcThePC];
#if defined(__i386__) || defined(__x86_64__)
			gCurrentTask->fCopySavedesi = gCurrentTask->fRegister[kcesi];
			gCurrentTask->fCopySavededi = gCurrentTask->fRegister[kcedi];
#endif
			gCurrentTask->fCopiedSize = 0;
			gCurrentTask->fRegister[kCopySize] = inSize;
			gCurrentTask->fRegister[kCopyFromBuf] = (VAddr) inBuffer;
			gCurrentTask->fRegister[kCopyToBuf] = (VAddr) mem->fAddr + inOffset;
			gCurrentTask->fSMemEnvironment = mem->fEnvironment;
			gCopyTasks->add(gCurrentTask);
			gCurrentTask->fCopySavedErr = err;
			gCurrentTask->fCopySavedMemId = inId;
			err = ((gCurrentTask->fRegister[kCopySize] & 0x03) == 0
				&&  (gCurrentTask->fRegister[kCopyFromBuf] & 0x03) == 0
				&&  (gCurrentTask->fRegister[kCopyToBuf] & 0x03) == 0) ? 4 : 1;
		}
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Copy a block of memory out of a shared memory object.
	Supervisor mode callback from SWI.
	Args:		inId				id of CSharedMem
				outBuffer		address of block of memory
				inSize			size of the block to be copied
				inOffset			offset to start of the block to be copied
				inSendersMsgId	id of the sending task’s message
				inSendersSignature	signature of the sending task
	Return:	error code
				1 => perform copy, byte-at-a-time
				4 => perform copy, long-at-a-time
				gCurrentTask->fRegister[kCopySize]		-> size of buffer
				gCurrentTask->fRegister[kCopyFromBuf]	-> source
				gCurrentTask->fRegister[kCopyToBuf]		-> destination
--------------------------------------------------------------------------------*/

NewtonErr
SMemCopyFromKernelGlue(ObjectId inId, void * outBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature)
{
	NewtonErr err;
	XTRY
	{
		CSharedMemMsg *	sendersMsg = NULL;
		CSharedMem *		mem;

		gCurrentTask->fRegister[kCopySize] = 0;

		XFAIL(err = ConvertMemOrMsgIdToObj(inId, &mem))
		if (inSendersMsgId != kNoId)
		{
			XFAIL(err = ConvertIdToObj(kSharedMemMsgType, inSendersMsgId, &sendersMsg))
			XFAILIF(sendersMsg->fCopyTask != kNoId, err = kOSErrCallAlreadyInProgress;)
			XFAILIF(sendersMsg->fStatus != kInProgress, err = kOSErrCallNotInProgress;)
			XFAILIF(sendersMsg->fSignature != inSendersSignature, err = kOSErrBadSignature;)
		}
		if (inSize != 0
		&&  inOffset < mem->fCurSize)
		{
			if (inOffset + inSize > mem->fCurSize)
				inSize = mem->fCurSize - inOffset;
			if (inSize != 0)
			{
				if (sendersMsg != NULL)
					sendersMsg->fCopyTask = *gCurrentTask;
				gCurrentTask->fCopySavedMemMsgId = inSendersMsgId;
				gCurrentTask->fCopySavedPC = gCurrentTask->fRegister[kcThePC];
#if defined(__i386__) || defined(__x86_64__)
				gCurrentTask->fCopySavedesi = gCurrentTask->fRegister[kcesi];
				gCurrentTask->fCopySavededi = gCurrentTask->fRegister[kcedi];
#endif
				gCurrentTask->fCopiedSize = inSize;
				gCurrentTask->fRegister[kCopySize] = inSize;
				gCurrentTask->fRegister[kCopyFromBuf] = (VAddr) mem->fAddr + inOffset;
				gCurrentTask->fRegister[kCopyToBuf] = (VAddr) outBuffer;
				gCurrentTask->fSMemEnvironment = mem->fEnvironment;
				gCopyTasks->add(gCurrentTask);
				gCurrentTask->fCopySavedErr = err;
				gCurrentTask->fCopySavedMemId = inId;
				err = ((gCurrentTask->fRegister[kCopySize] & 0x03) == 0
					&&  (gCurrentTask->fRegister[kCopyFromBuf] & 0x03) == 0
					&&  (gCurrentTask->fRegister[kCopyToBuf] & 0x03) == 0) ? 4 : 1;
			}
		}
	}
	XENDTRY;
	return err;
}


NewtonErr
SMemMsgSetTimerParmsKernelGlue(ObjectId inId, ULong inTimeout, int64_t inDelay)
{
	NewtonErr err;
	XTRY
	{
		CSharedMemMsg *	msg;

		XFAIL(err = ConvertIdToObj(kSharedMemMsgType, inId, &msg))
		XFAILIF(FLAGTEST(msg->fFlags, kSMemMsgFlags_TimerMask), err = kOSErrCallAlreadyInProgress;)
		msg->fTimeout = inTimeout;
		msg->fExpiryTime = inDelay;
	}
	XENDTRY;
	return err;
}


NewtonErr
SMemMsgSetMsgAvailPortKernelGlue(ObjectId inMsgId, ObjectId inPortId)
{
	NewtonErr err;
	XTRY
	{
		CSharedMemMsg *	msg;

		// validate msg id
		XFAIL(err = ConvertIdToObj(kSharedMemMsgType, inMsgId, &msg))
		if (inPortId != kNoId)
		{
			// validate port id
			XFAIL(err = ConvertIdToObj(kPortType, inPortId, NULL))
			msg->fPortToNotify = inPortId;
		}
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Get shared mem mesage sender task’s id.
	Supervisor mode callback from SWI.
	Args:		inId			message’s id
				inRefCon		the refCon
	Return:	error code
				gCurrentTask->fRegister[kReturnParm1]	-> refCon
--------------------------------------------------------------------------------*/

NewtonErr
SMemMsgGetSenderTaskIdKernelGlue(ObjectId inId)
{
	NewtonErr err;
	XTRY
	{
		CSharedMemMsg *	msg;

		XFAIL(err = ConvertIdToObj(kSharedMemMsgType, inId, &msg))
		gCurrentTask->fRegister[kReturnParm1] = msg->fTaskToNotify;
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Set shared mem mesage refCon.
	Args:		inId			message’s id
				inRefCon		the refCon
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
SMemMsgSetUserRefConKernelGlue(ObjectId inId, ULong inRefCon)
{
	NewtonErr err;
	XTRY
	{
		CSharedMemMsg *	msg;

		XFAIL(err = ConvertIdToObj(kSharedMemMsgType, inId, &msg))
		msg->fRefCon = inRefCon;
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Get shared mem mesage refCon.
	Args:		inId			message’s id
	Return:	error code
				gCurrentTask->fRegister[kReturnParm1]	-> refCon
--------------------------------------------------------------------------------*/

NewtonErr
SMemMsgGetUserRefConKernelGlue(ObjectId inId)
{
	NewtonErr err;
	XTRY
	{
		CSharedMemMsg *	msg;

		XFAIL(err = ConvertIdToObj(kSharedMemMsgType, inId, &msg))
		gCurrentTask->fRegister[kReturnParm1] = msg->fRefCon;
	}
	XENDTRY;
	return err;
}


/* -------------------------------------------------------------------------------
------------------------------------------------------------------------------- */

NewtonErr
SMemMsgCheckForDoneKernelGlue(ObjectId inId, ULong inFlags)
{
	NewtonErr err;
	XTRY
	{
		CSharedMemMsg *	msg;

		gCurrentTask->fRegister[kResultReg] = noErr;

		XFAIL(err = ConvertIdToObj(kSharedMemMsgType, inId, &msg))
		gCurrentTask->fRegister[kReturnParm1] = msg->fSendersMsg;
		gCurrentTask->fRegister[kReturnParm2] = msg->fSendersMem;
		gCurrentTask->fRegister[kReturnParm3] = msg->fSendersType;
		gCurrentTask->fRegister[kReturnParm4] = msg->fSendersSig;
		if (msg->fStatus == kInProgress  ||  FLAGTEST(msg->fType, kMsgType_CollectorMask))
		{
			if (FLAGTEST(msg->fType, kMsgType_CollectorMask))
			{
				if (msg->fStatus != kInProgress)
					err = msg->fStatus;
				msg->fNotify = *gCurrentTask;
				err = msg->completeMsg(false, msg->fType, err);
			}
			else if (FLAGTEST(inFlags, kSMemMsgFlags_BlockTillDone))
			{
				if (ObjectType(msg->fNotify) == kTaskType)
					gCurrentTask->fRegister[kResultReg] = kOSErrAnotherTaskAlreadyBlocking;
				else
				{
					msg->fNotify = *gCurrentTask;
					UnScheduleTask(gCurrentTask);
				}
			}
			else if (FLAGTEST(inFlags, kSMemMsgFlags_Abort))
			{
				msg->fNotify = *gCurrentTask;
				err = msg->completeMsg(true, 0, kOSErrCallAborted);
			}
			else
			{
				err = msg->fStatus;
				if (err != noErr)
					gCurrentTask->fRegister[kResultReg] = err;
			}
		}
		else
		{
			err = msg->fStatus;
			if (err != noErr)
				gCurrentTask->fRegister[kResultReg] = err;
		}
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Shared mem message is done - clean up.
	Args:		inMsgId			message id
				inResult			result of message send
				inSignature		sanity check
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
SMemMsgMsgDoneKernelGlue(ObjectId inMsgId, long inResult, ULong inSignature)
{
	NewtonErr err;
	XTRY
	{
		CSharedMemMsg *	msg;

		XFAIL(err = ConvertIdToObj(kSharedMemMsgType, inMsgId, &msg))
		XFAILIF(msg->fStatus != kInProgress, err = kOSErrCallNotInProgress;)
		XFAILIF(msg->fSignature != inSignature, err = kOSErrBadSignature;)
		XFAILIF(msg->f80.fContainer == NULL, err = kOSErrMsgDoneNotExpected;)
		msg->f80.fContainer->deleteFromQueue(msg);
		msg->completeSender(inResult);
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
--------------------------------------------------------------------------------*/

NewtonErr
LowLevelCopyDoneFromKernelGlue(NewtonErr inErr, CTask * inTask, VAddr inReturn)
{
	if (!gCopyTasks->removeFromQueue(inTask))
	{
		inTask->fRegister[kcThePC] = inReturn;
		return kOSErrBadParameters;
	}

	CSharedMemMsg *	msg = NULL;
	if (inTask == gCurrentTask)
		gCopyDone = true;
	if (inErr == noErr)
		inErr = inTask->fCopySavedErr;
	ConvertIdToObj(kSharedMemMsgType, inTask->fCopySavedMemMsgId, &msg);
	if (msg)
		msg->fCopyTask = kNoId;
	inTask->fSMemEnvironment = NULL;
	inTask->fRegister[kReturnParm1] = inTask->fCopiedSize;
	inTask->fRegister[kcThePC] = inTask->fCopySavedPC;
#if defined(__i386__) || defined(__x86_64__)
	gCurrentTask->fRegister[kcesi] = gCurrentTask->fCopySavedesi;
	gCurrentTask->fRegister[kcedi] = gCurrentTask->fCopySavededi;
#endif
	inTask->fRegister[kResultReg] = inErr;
	inTask->fCopySavedMemId = kNoId;
	inTask->fCopySavedMemMsgId = kNoId;
	if (inTask->fMonitorQItem.fContainer != NULL)
		inTask->fMonitorQItem.fContainer->removeFromQueue(inTask);
	return noErr;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C S h a r e d M e m
	
	Initialize an instance in its environment.
	Args:		inEnvironment
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CSharedMem::init(CEnvironment * inEnvironment)
{
	fAddr = NULL;
	fBufSize = 0;
	fPerm = kSMemReadWrite;
	fEnvironment = inEnvironment;

	return noErr;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C S h a r e d M e m M s g
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Constructor.
--------------------------------------------------------------------------------*/

CSharedMemMsg::CSharedMemMsg(void)
	: f8C(offsetof(CSharedMemMsg, f80))
{ }


/*--------------------------------------------------------------------------------
	Destructor.
	Inform all tasks trying to send this message that there’s noone to receive it.
--------------------------------------------------------------------------------*/

CSharedMemMsg::~CSharedMemMsg(void)
{
	EnterFIQAtomic();
	for (CSharedMemMsg * msg; (msg = (CSharedMemMsg *)f8C.remove()) != NULL; )
	{
		gTimerEngine->remove(msg);
		ExitFIQAtomic();
		msg->completeSender(kOSErrReceiverObjectNoLongerExists);
		EnterFIQAtomic();
	}
	ExitFIQAtomic();

	gTimerEngine->remove(this);
	if (fStatus == kInProgress  || FLAGTEST(fType, kMsgType_CollectorMask))
		completeMsg(true, 0, kOSErrSharedMemMsgNoLongerExists);
}


/*--------------------------------------------------------------------------------
	Initialize an instance in its environment.
	Args:		inEnvironment
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CSharedMemMsg::init(CEnvironment * inEnvironment)
{
	CSharedMem::init(inEnvironment);

	fTimeout = 0;
	fExpiryTime = 0;
	fFlags = 0;
	fSendingPort = kNoId;
	fStatus = noErr;
	fRefCon = 0;
	fReplyMem = kNoId;
	fType = 0;
	fFilter = 0;
	fSendersMsg = kNoId;
	fSendersMem = kNoId;
	fSendersType = 0;
	fSendersSig = 0;
	fPortToNotify = kNoId;
	fTaskToNotify = kNoId;
	fNotify = kNoId;
	fSignature = 0;
	fSequenceNo = 1;
	fCopyTask = kNoId;

	return noErr;
}


/*--------------------------------------------------------------------------------
	Complete a message for a sender.
	Args:		inErr				the message’s status
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CSharedMemMsg::completeSender(NewtonErr inErr)
{
	return completeMsg(false, kMsgType_CollectedSender, inErr);
}


/*--------------------------------------------------------------------------------
	Complete a message for a receiver.
	Args:		inMsg				
				inErr				the message’s status
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CSharedMemMsg::completeReceiver(CSharedMemMsg * inMsg, NewtonErr inErr)
{
	bool	isCollector = false;

	if (inErr == noErr)
	{
		fSendersMsg = *inMsg;
		fSendersMem = inMsg->fReplyMem;
		fSendersType = inMsg->fType;
		fSendersSig = 0;
		if ((isCollector = FLAGTEST(inMsg->fType, kMsgType_CollectorMask)))
			inMsg->fType = 0;
		else
		{
			f8C.add(inMsg);		// what’s the point? it gets deleted in completeMsg next
			fSendersSig = inMsg->fSignature = inMsg->fSequenceNo;	// sync signatures
			if (++inMsg->fSequenceNo == 0)			// bump sender’s sequence number (uid)
				inMsg->fSequenceNo = 1;
		}
	}
	return completeMsg(isCollector, kMsgType_CollectedReceiver, inErr);
}


/*--------------------------------------------------------------------------------
	Complete a message.
	Args:		inCollector
				inType			the message’s type
				inErr				the message’s status
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CSharedMemMsg::completeMsg(bool inCollector, ULong inType, NewtonErr inErr)
{
	NewtonErr	err;
	CTask *		task;
	CPort *		port;

	fType = 0;

	// the message should no longer time out
	gTimerEngine->remove(this);

	// it’s no longer queued
	if (f80.fContainer != NULL)
		f80.fContainer->deleteFromQueue(this);

	if (fCopyTask != kNoId  &&  ConvertIdToObj(kTaskType, fCopyTask, &task) == noErr)
		// message completed before low-level copy finished
		LowLevelCopyDoneFromKernelGlue(inErr ? inErr : kOSErrCopyAborted, task, 0);
	fCopyTask = kNoId;
	fSignature = 0;

	switch (ObjectType(fNotify))
	{
	case kNoType:
		fStatus = inErr;
		err = noErr;
		break;

	case kPortType:
		// async: send notification to port
		if ((err = ConvertIdToObj(kPortType, fNotify, &port)) == noErr)
		{
			if (inCollector)
				fStatus = err = kOSErrNestedCollection;
			else
			{
				fStatus = inErr;
				fType = inType;
				err = port->send(this, 0);
			}
		}
		else
			fStatus = err;
		break;

	case kTaskType:
		// sync: continue task
		if ((err = ConvertIdToObj(kTaskType, fNotify, &task)) == noErr)
		{
			task->fRegister[kResultReg] = inErr;
			task->fRegister[kReturnParm1] = fSendersMsg;
			task->fRegister[kReturnParm2] = fSendersMem;
			task->fRegister[kReturnParm3] = fSendersType;
			task->fRegister[kReturnParm4] = fSendersSig;
			gKernelScheduler->setCurrentTask(task);
			ScheduleTask(task);
		}
		fStatus = err;
		break;

	default:
		fStatus = err = kOSErrImTotallyConfused;
	}

	return err;
}

#pragma mark -

/* -----------------------------------------------------------------------------
	Set up interrupt for timed message send.
	Args:		inPortId
				inMsgId
				inReplyId
				inContent
				inSize
				inMsgType
				inTimeout
				inFutureTimeToSend
				inUrgent
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
SendForInterrupt(ObjectId inPortId, ObjectId inMsgId, ObjectId inReplyId,
					void * inContent, size_t inSize,
					ULong inMsgType,
					Timeout inTimeout, CTime * inFutureTimeToSend,
					bool inUrgent)
{
//xx r1 r4 r3 r9 r10 r8 r7 sp00
	NewtonErr err = noErr;
	XTRY
	{
		CSharedMemMsg * msg = (ObjectType(inMsgId) == kSharedMemMsgType) ? (CSharedMemMsg *)gObjectTable->get(inMsgId) : NULL;	// r5
		XFAILNOT(msg, err = kOSErrBadObjectId;)
		if (inReplyId)
			XFAIL(err = ConvertIdToObj(kSharedMemType, inReplyId, NULL))

		EnterFIQAtomic();
		XFAILIF(msg->fStatus == kInProgress || FLAGTEST(msg->fType, kMsgType_CollectorMask), ExitFIQAtomic(); err = kOSErrMessageAlreadyPosted;)
		msg->fStatus = kInProgress;
		gWantDeferred = true;
		gDeferredSends->add(msg);
#if defined(correct)
		if (IsFIQMode())
			FireAlarm();
#endif
		ExitFIQAtomic();

		msg->fFilter = inUrgent	? kPortFlags_ScheduleOnSend | kPortFlags_TimerWanted | kPortFlags_Async | kPortFlags_Urgent
										: kPortFlags_ScheduleOnSend | kPortFlags_TimerWanted | kPortFlags_Async;
		if (inFutureTimeToSend)
		{
			msg->fFlags = 0x20;
			msg->fTimeout = inTimeout;
			msg->fExpiryTime = *inFutureTimeToSend;
		}
		else
		{
			msg->fFlags = 0x00;
			msg->fTimeout = inTimeout;
			msg->fExpiryTime = 0;
		}
		msg->fAddr = inContent;
		msg->fBufSize = inSize;
		msg->fReplyMem = inReplyId;
		msg->fType = inMsgType;
		msg->fPerm = kSuperReadWrite;
		msg->fCurSize = inSize;
		msg->fSendingPort = inPortId;
		msg->fTaskToNotify = kNoId;
		msg->fNotify = msg->fPortToNotify;
	}
	XENDTRY;
	return err;
}


#pragma mark - Deferred message handling
/* -------------------------------------------------------------------------------
	Deferred message handling.
------------------------------------------------------------------------------- */

void		DeferredNotify(void);
void		PortDeferredSendNotify(void);
void		NotifySend(CSharedMemMsg * inMsg);
void		NotifyTimeout(CSharedMemMsg * inMsg);


void
QueueNotify(CSharedMemMsg * inMsg)
{
	gWantDeferred = true;
	gTimerDeferred->add(inMsg);
}


/* -------------------------------------------------------------------------------
	Handle deferred messages.
	Called from every SWI if gWantDeferred.
	Args:		--
	Return:	--
------------------------------------------------------------------------------- */

void
DoDeferrals(void)
{
	EnterAtomic();
	if (gWantDeferred)
	{
		gWantDeferred = false;
		ExitAtomic();
		PortDeferredSendNotify();
		DeferredNotify();
		if (gExtPageTrackerMgr != NULL)
			gExtPageTrackerMgr->doDeferral();
		EnterAtomic();
	}
	ExitAtomic();
}


/* -------------------------------------------------------------------------------
	Send message.
	Args:		--
	Return:	--
------------------------------------------------------------------------------- */

void
PortDeferredSendNotify(void)
{
	// iterate over messages in the send queue
	CSharedMemMsg * msg;
	while (EnterFIQAtomic(), msg = (CSharedMemMsg *)gDeferredSends->remove(), ExitFIQAtomic(), msg != NULL)
	{
		if (FLAGTEST(msg->fFlags, 0x0020))
		{
			if (!gTimerEngine->queueDelay(msg))
				NotifySend(msg);
		}
		else
			// send now
			NotifySend(msg);
	}
}


/* -------------------------------------------------------------------------------
	Handle deferred messages.
	Args:		--
	Return:	--
------------------------------------------------------------------------------- */

void
DeferredNotify(void)
{
	// iterate over messages in the timer queue
	CSharedMemMsg * msg;
	while (EnterAtomic(), msg = (CSharedMemMsg *)gTimerDeferred->remove(), ExitAtomic(), msg != NULL)
	{
		if (FLAGTEST(msg->fFlags, kSMemMsgFlags_Timeout))
		{
			// message has timeout => waiting to receive something
			msg->fFlags &= ~kSMemMsgFlags_TimerMask;
			NotifyTimeout(msg);
		}
		else
		{
			// else must be waiting to send something
			msg->fFlags &= ~kSMemMsgFlags_TimerMask;
			NotifySend(msg);
		}
	}
}


/* -------------------------------------------------------------------------------
	Send message.
	Args:		inMsg		the message
	Return:	--
------------------------------------------------------------------------------- */

void
NotifySend(CSharedMemMsg * inMsg)
{
	NewtonErr err;
	XTRY
	{
		CPort * port;
		XFAIL(err = ConvertIdToObj(kPortType, inMsg->fSendingPort, &port))
		err = port->send(inMsg, inMsg->fFilter);
	}
	XENDTRY;
	XDOFAIL(err)
	{
		inMsg->completeSender(err);
	}
	XENDFAIL;
}


/* -------------------------------------------------------------------------------
	Message has timed out.
	Receiver => timed out, no message received
	Sender => future-time-to-send has elapsed, send message now
	Args:		inMsg		the message
	Return:	--
------------------------------------------------------------------------------- */

void
NotifyTimeout(CSharedMemMsg * inMsg)
{
	if (FLAGTEST(inMsg->fFlags, 0x0080))
		inMsg->completeSender();
	else
		inMsg->completeReceiver(NULL, kOSErrMessageTimedOut);
}

