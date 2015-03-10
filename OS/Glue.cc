/*
	File:		Glue.cc

	Contains:	Glue to handle SWI.
					Should really be in assembler.

	Written by:	Newton Research Group.
*/

#include "SharedTypes.h"
#include "UserGlobals.h"

extern ObjectId	GetPortInfo(int inWhat);


// SWI numbers

enum
{
	kGetPortSWI,
	kPortSendSWI,
	kPortReceiveSWI,

	kPortResetFilterSWI = 33
};

int	 gSWItch;
#define SWI(n) gSWItch = n; result = noErr

/*
#include <stdarg.h>

long
SWIBoot(...)
{
	long	result;

	va_list	args;
	va_start(args, inSelector);

	if (gSWItch < 25)
	{
		switch (gSWItch)
		{

		case kGetPortSWI:		// 0	ObjectId GetPortSWI(int inWhat)
			int	what = va_arg(args, int);
			result = GetPortInfo(what);
			break;

		case kPortSendSWI:	// 1
			CTask *	saveTask = gCurrentTask;
			PortSendKernelGlue();
			if (gCurrentTask == nil)
			{
			}
			else
				return saveTask->f10;
			
			return noErr;
		}
	}

	va_end(args);
	return result;
}
*/


/*------------------------------------------------------------------------------
	P o r t s
	All have C linkage.
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Return the ObjectId of a well-known port.
	Args:		inWhat	0 => object manager
							1 => null
							2 => name server
	Return:	ObjectId
------------------------------------------------------------------------------*/

ObjectId
GetPortSWI(int inWhat)
{
	long	result;
	SWI(kGetPortSWI);
	return result;
}


/*------------------------------------------------------------------------------
	Send a message from a port.
	Args:		inId				port id
				inMsgId			message id
				inReplyId		reply id
				inMsgType		type of message
				inFlags			flags
	Return:	error code
------------------------------------------------------------------------------*/

long
PortSendSWI(ObjectId inId, ULong inMsgId, ULong inReplyId, ULong inMsgType, ULong inFlags)
{
	long	result;
	SWI(kPortSendSWI);
	return result;
}


/*------------------------------------------------------------------------------
	Receive a message at a port.
	Args:		inId					port id
				inMsgId				message id
				inMsgFilter			types of message we’re interested in
				inFlags				flags
				outSenderMsgId		sender info - sent message id
				outReplyMemId		- sent reply id
				outMsgType			- sent message type
				outSignature		- sent signature
	Return:	error code
------------------------------------------------------------------------------*/

long
PortReceiveSWI(ObjectId inId, ULong inMsgId, ULong inMsgFilter, ULong inFlags,
					ULong * outSenderMsgId, ULong * outReplyMemId, ULong * outMsgType, ULong * outSignature)
{
	long	result;
	SWI(kPortReceiveSWI);
	return result;
}


/*------------------------------------------------------------------------------
	Reset the message filter on a port.
	Args:		inId					port id
				inMsgId				message id
				inMsgFilter			types of message we’re interested in
	Return:	error code
------------------------------------------------------------------------------*/

long
PortResetFilterSWI(ObjectId inId, ULong inMsgId, ULong inMsgFilter)
{
	long	result;
	SWI(kPortResetFilterSWI);
	return result;
}


/*------------------------------------------------------------------------------
	G e n e r i c   S W I
------------------------------------------------------------------------------*/

long
GenericSWI(int inSelector, ...)
{
//	SWI 5
	return noErr;
}

long
GenericWithReturnSWI(int inSelector, ULong inp1, ULong inp2, ULong inp3,
												 ULong * outp1, ULong * outp2, ULong * outp3)
{
//	SWI 5
	return noErr;
}

// SWI 8 = FlushMMU

/*------------------------------------------------------------------------------
	S e m a p h o r e s
------------------------------------------------------------------------------*/

long
SemOpGlue(void * inRefCon, ObjectId inListId, SemFlags inBlocking)
{
//	SWI 11
	return noErr;
}


/*------------------------------------------------------------------------------
	S h a r e d   M e m o r y
------------------------------------------------------------------------------*/

long
SMemSetBufferSWI(ObjectId inId, void * inBuffer, ULong inSize, ULong inPermissions)
{
//	SWI 13
	return noErr;
}

long
SMemGetSizeSWI(ObjectId inId, ULong * outSize, void ** outBuffer, ULong * outRefCon)
{
/*	SWI 14
	if (outSize != NULL)
	*outSize = r1;
	if (outBuffer != NULL)
	*outBuffer = r2;
	if (outRefCon != NULL)
	*outRefCon = r3;
*/
	return noErr;
}

long	SMemCopyToSharedSWI(ObjectId id, void * buffer, ULong size, ULong offset, ULong sendersMsgId, ULong signature) { /* SWI 15 */ return noErr; }
long	SMemCopyFromSharedSWI(ObjectId id, void * buffer, ULong size, ULong offset, ULong sendersMsgId, ULong signature, ULong* returnSize) { /* SWI 16 */ return noErr; }
long	SMemMsgSetTimerParmsSWI(ObjectId id, ULong timeout, ULong timeLow, ULong timeHigh) { /* SWI 17 */ return noErr; }
long	SMemMsgSetMsgAvailPortSWI(ObjectId id, ULong availPort) { /* SWI 18 */ return noErr; }
long	SMemMsgGetSenderTaskIdSWI(ObjectId id, void * senderTaskId) { /* SWI 19 */ return noErr; }
long	SMemMsgSetUserRefConSWI(ObjectId id, ULong refCon) { /* SWI 20 */ return noErr; }
long	SMemMsgGetUserRefConSWI(ObjectId id, ULong* refConPtr) { /* SWI 21 */ return noErr; }
long	SMemMsgCheckForDoneSWI(ObjectId id, ULong flags, ULong* sentById, ULong* replymemId, ULong* msgType, ULong* signature) { /* SWI 22 */ return noErr; }
long	SMemMsgMsgDoneSWI(ObjectId id, long result, ULong signature) { /* SWI 23 */ return noErr; }

/*------------------------------------------------------------------------------
	M o n i t o r s
------------------------------------------------------------------------------*/

long	MonitorDispatchSWI(ObjectId id, long selector, void * userObject) { /* SWI 27 */ return noErr; }
long	MonitorExitSWI(long monitorResult, void * continuationPC) { /* SWI 28 */ return noErr; }
long	MonitorThrowSWI(char *, void *, void *) { /* SWI 29 */ return noErr; }
long	MonitorFlushSWI(ObjectId id) { /* SWI 32 */ return noErr; }

/*------------------------------------------------------------------------------
	S c h e d u l e r
------------------------------------------------------------------------------*/

void	DoSchedulerSWI(void) { /* SWI 34 */ }

