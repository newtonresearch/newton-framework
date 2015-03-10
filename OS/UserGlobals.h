/*
	File:		UserGlobals.h

	Contains:	User glue to global variables and functions.

	Written by:	Newton Research Group.
*/

#if !defined(__USERGLOBALS_H)
#define __USERGLOBALS_H 1

#if !defined(__NEWTON_H)
#include "Newton.h"
#endif
#if !defined(__KERNELTYPES_H)
#include "KernelTypes.h"
#endif
#if !defined(__LONGTIME_H)
#include	"LongTime.h"
#endif
#include "SWI.h"


class CUPort;
class CUMonitor;

// extern globals
extern CUMonitor *	gUObjectMgrMonitor;	// use this to talk to the object manager
extern CUPort *		gUNullPort;				// 0C104F08	this is a special null port (never receives) used to create things like sleep
extern ObjectId		gCurrentTaskId;		// id of currently running task
extern ObjectId		gCurrentMonitorId;	// monitor that a task is in if any

extern "C" {

ObjectId		GetPortSWI(int what);
NewtonErr	GenericSWI(int inSelector, ...);
NewtonErr	GenericWithReturnSWI(int inSelector, ULong p1, ULong p2, ULong p3, OpaqueRef * rp1, OpaqueRef * rp2, OpaqueRef * rp3);
NewtonErr	SemaphoreOpGlue(ObjectId id, ObjectId opListId, SemFlags flags);
NewtonErr	PortSendSWI(ObjectId id, ObjectId msgId, ObjectId replyId, ULong msgType, ULong flags);
NewtonErr	PortReceiveSWI(ObjectId id, ObjectId msgId, ULong msgFilter, ULong flags, ObjectId * senderMsgId, ObjectId * replyMemId, ULong * returnMsgType, ULong * signature);
NewtonErr	PortResetFilterSWI(ObjectId id, ObjectId msgId, ULong msgFilter);

NewtonErr	SMemSetBufferSWI(ObjectId id, void * buffer, size_t size, ULong permissions);
NewtonErr	SMemGetSizeSWI(ObjectId id, size_t * returnSize, void ** returnBuffer, OpaqueRef * refConPtr);
NewtonErr	SMemCopyToSharedSWI(ObjectId id, void * buffer, size_t size, ULong offset, ULong sendersMsgId, ULong signature);
NewtonErr	SMemCopyFromSharedSWI(ObjectId id, void * buffer, size_t size, ULong offset, ULong sendersMsgId, ULong signature, size_t * returnSize);

#if defined(correct)
NewtonErr	SMemMsgSetTimerParmsSWI(ObjectId id, ULong timeout, ULong timeLow, ULong timeHigh);
#else
NewtonErr	SMemMsgSetTimerParmsSWI(ObjectId id, ULong timeout, int64_t delay);
#endif
NewtonErr	SMemMsgSetMsgAvailPortSWI(ObjectId id, ULong availPort);
NewtonErr	SMemMsgGetSenderTaskIdSWI(ObjectId id, ObjectId * senderTaskId);
NewtonErr	SMemMsgSetUserRefConSWI(ObjectId id, OpaqueRef refCon);
NewtonErr	SMemMsgGetUserRefConSWI(ObjectId id, OpaqueRef * refConPtr);
NewtonErr	SMemMsgMsgDoneSWI(ObjectId id, NewtonErr result, ULong signature);
NewtonErr	SMemMsgCheckForDoneSWI(ObjectId id, ULong flags, ObjectId * sentById, ObjectId * replyMemId, ULong * msgType, ULong * signature);

NewtonErr	MonitorDispatchSWI(ObjectId inMonitorId, int inSelector, OpaqueRef inData);
NewtonErr	MonitorExitSWI(int monitorResult, void * continuationPC);
void			MonitorThrowSWI(ExceptionName, void *, ExceptionDestructor);
NewtonErr	MonitorFlushSWI(ObjectId inMonitorId);

NewtonErr	ResetRebootReason(void);

#ifdef forARM
// Hardware status goop
// read and write volatile
inline UChar ReadVByte(const volatile void * ptr) { return *(const volatile UChar *)ptr; }
inline ULong ReadVLong(const volatile void * ptr) { return *(const volatile ULong *)ptr; }
inline void	WriteVByte(volatile void * ptr, UChar ch) { *(volatile UChar *)ptr = ch; }
inline void	WriteVLong(volatile void * ptr, ULong l) { *(volatile ULong *)ptr = l; }
#endif

/*	Return the CPU mode. */
enum
{
	kUserMode = 0x10,
	kFIQMode,
	kIRQMode,
	kSuperMode,
	kAbortMode = 0x17,
	kUndefinedMode = 0x1B,
	kModeMask = 0x1F,
//	flags in the PSR but not returned by GetCPUMode() -- they’re masked by kModeMask
	kIRQDisable = 0x40,
	kFIQDisable = 0x80
};
int		GetCPUMode(void);

// The following is an atomic and stack safe routine that will
// allow you to read a location in memory, OR in setBits, and
// clear out the clearBits, then write the new value back out
// in one operation.
void		SetAndClearBitsAtomic(ULong * ioTarget, ULong inSetBits, ULong inClearBits);

class CProcessorState;
CProcessorState *	GetProcessorFaultState();

// call-off to page manager to see how much is/could be freed
ArrayIndex	SystemFreePageCount();

}

#endif	/* __USERGLOBALS_H */
