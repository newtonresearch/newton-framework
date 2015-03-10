/*
	File:		SharedTypes.h

	Contains:	Types shared by kernel and user objects.

	Written by:	Newton Research Group.
*/

#if !defined(__SHAREDTYPES_H)
#define __SHAREDTYPES_H 1

#if !defined(__KERNELTYPES_H)
#include	"KernelTypes.h"
#endif

typedef void (*TaskProcPtr)(void*, size_t, ObjectId);			// taskInstance, taskDataSize, taskId (ie CUTaskWorld::taskEntry)
typedef NewtonErr (*MonitorProcPtr)(int, void*);		// monitorInstance, selector, msg

#if defined(correct)
/* ARM ABI */
enum TaskRegisters
{
	// result
	kResultReg				= 0,
	kcTheFrame				= 11,			// frame pointer
	kcTheStack				= 13,
	kcTheLink				= 14,
	kcThePC					= 15,
	kNumOfRegisters		= 16,

	kMonMonId				= 0,
	// MonitorDispatchSWI parameter registers
	kMonContext				= 0,
	kMonSelector			= 1,
	kMonUserRefCon			= 2,
	kMonThePC				= 3,

	kParm0					= 0,
	kParm1					= 1,
	kParm2					= 2,

	kReturnParm0			= 0,
	kReturnParm1			= 1,
	kReturnParm2			= 2,
	kReturnParm3			= 3,
	kReturnParm4			= 4,

	// Copy registers
	kCopyToBuf				= 0,
	kCopyFromBuf			= 1,
	kCopySize				= 2
};

#elif defined(__i386__)
/*	i386 ABI
	The i386 has fewer, named registers:
	0	eax
	1	ebx
	2	ecx
	3	edx
	4	esi
	5	edi
	6	ebp
	7	esp
	8	eip
*/
enum TaskRegisters
{
	// result
	kResultReg				=  0,		// eax
//	ebx, ecx, edx
	kcesi						=  4,
	kcedi						=  5,
	kcTheFrame				=  6,
	kcTheStack				=  7,
	kcThePC					=  8,
	kNumOfRegisters		= 18,		// returnParms are in this space too; but they’re not actually registers

	kMonMonId				=  9,
	// MonitorDispatchSWI parameter registers
	kMonContext				=  9,
	kMonSelector			= 10,
	kMonUserRefCon			= 11,
	kMonThePC				= 12,

	kParm0					=  9,	// need to work out how/where these are used -- might have to use the task’s stack
	kParm1					= 10,
	kParm2					= 11,

	kReturnParm1			= 13,
	kReturnParm2			= 14,
	kReturnParm3			= 15,
	kReturnParm4			= 16,

	// Copy registers
	kCopyToBuf				= 13,
	kCopyFromBuf			= 14,
	kCopySize				= 15
};

#elif defined(__x86_64__)
/*	LP64 ABI
	The x86 has a few more registers:
	 0	rax
	 1	rbx
	 2	rcx
	 3	rdx
	 4	rdi
	 5	rsi
	 6	rbp
	 7	rsp
	 8	r8
	 9	r9
	10	r10
	11	r11
	12	r12
	13	r13
	14	r14
	15	r15
*/
enum TaskRegisters
{
	// result
	kResultReg				=  0,		// rax
//	rbx, rcx, rdx
	kcesi						=  4,
	kcedi						=  5,
	kcTheFrame				=  6,
	kcTheStack				=  7,
	kcThePC					=  8,
// r8..r15
	kNumOfRegisters		= 26,		// returnParms are in this space too; but they’re not actually registers

	kMonMonId				=  5,		// rdi
	// MonitorDispatchSWI parameter registers
	kMonContext				=  5,		// rdi
	kMonSelector			=  4,		// rsi
	kMonUserRefCon			=  3,		// rdx
	kMonThePC				=  2,		// rcx

	kParm0					=  5,	// need to work out how/where these are used -- might have to use the task’s stack
	kParm1					=  4,
	kParm2					=  3,

	kReturnParm1			= 21,
	kReturnParm2			= 22,
	kReturnParm3			= 23,
	kReturnParm4			= 24,

	// Copy registers
	kCopyToBuf				= 21,
	kCopyFromBuf			= 22,
	kCopySize				= 23
};
#endif

enum TaskPriorities
{
	kIdleTaskPriority						=  0,
	kUserTaskPriority						= 10,
	kScreenTaskPriority					= 11,
	kSoundTaskPriority					= 12,
	kCommToolTaskPriority				= 13,
	kKernelTaskPriority					= 20,
	kPreemptiveSchedulerTaskPriority	= 25,
	kTimerTaskPriority					= 31
};

enum MiscItems
{
	// suspend code for monitors
	kSuspendMonitor				= -1,
	kMonitorFaultSelector		= -2,

	// exception codes for debugger
	kExcpt_SerialDebuggerStop	= 8,

	// built in ids
	kNoId								= 0,
	kSystemId,
	// built in ids for a task
	kBuiltInSMemMsgId				= 1,
	kBuiltInSMemId,
	// built in ids for a monitor
	kBuiltInSMemMonitorFaultId,					// used in fault monitors to get fault info

	// shared memory privilege flags
	kSMemReadOnly					= 0x01,			// allow only read operations (if not set, read/write)
	kSMemReadWrite					= 0x00,			// allow read/write operations
	kSMemNoSizeChangeOnCopyTo	= 0x02,			// don't allow size to change when copyto takes place (use initial size)

	// message type stuff
	kMsgType_MatchAll				= 0xFFFFFFFF,	// match any message on receive
	kMsgType_ReservedMask		= 0x00FFFFFF,	// mask to reserve system msg types
	kMsgType_CollectedReceiver	= 0x01000000,	// system: msg is collected receiver
	kMsgType_CollectedSender	= 0x02000000,	// system: msg is collected sender
	kMsgType_FromInterrupt		= 0x04000000,	// system: msg is from an interrupt
	kMsgType_NoMsgTypeSet		= 0x08000000,	// system: all messages with no message type have this set

	kMsgType_CollectorMask 		= kMsgType_CollectedReceiver | kMsgType_CollectedSender,	// tells us if we have either type of collector

	// port flags stuff
	kPortFlags_Async				= 0x00000001,	// call is async
	kPortFlags_Urgent				= 0x00000002,	// call is urgent
	kPortFlags_ReceiveOnMsgAvail=0x00000004,	// only do receive if something matches
	kPortFlags_WantDelay			= 0x00000008,	// we want delay
	kPortFlags_WantTimeout		= 0x00000010,	// we want timeout
	kPortFlags_IsMsgAvail		= 0x00000020,	// this is a peek call
	kPortFlags_ReservedMask		= 0x00FFFFFF,	// mask to reserve system port flags
	kPortFlags_TimerWanted		= 0x01000000,	// system: internal for timer wanted
	kPortFlags_CanRemoveTask	= 0x02000000,	// system: internal for task removal allowed
	kPortFlags_ScheduleOnSend	= 0x04000000,	// we want scheduling to happen on send

	// more port flags (for CUPort::reset)
	kPortFlags_Abort				= 0x00000001,	// receivers/senders should be aborted
	kPortFlags_Timeout			= 0x00000002,	// receivers/senders should be timedout

	// shared mem msg flags
	kSMemMsgFlags_Abort				= 0x00000001,	// abort the msg in progress
	kSMemMsgFlags_BlockTillDone	= 0x00000002,	// block this task till msg done

	kSMemMsgFlags_Delay				= 0x00000200,	// delay message send
	kSMemMsgFlags_Timeout			= 0x00000400,	// time out message
	kSMemMsgFlags_Timer				= 0x00000600,	// callback when timer expires
	kSMemMsgFlags_TimerMask			= 0x00000600,	// tells us if we need to time the message
};

	// port send default parameters
#define kPortSend_BufferAlreadySet	(void*)0xFFFFFFFF


// we currently only allow 16 of these
enum KernelTypes
{
	kNoType,
	kBaseType,
	kPortType,
	kTaskType,
	kEnvironmentType,
	kDomainType,
	kSemListType,
	kSemGroupType,
	kSharedMemType,
	kSharedMemMsgType,
	kMonitorType,
	kPhysType,
	kExtPhysType
};

#define	kObjectTypeBits	4
#define	kObjectTypeMask	0x0F

#if defined(__cplusplus)
inline KernelTypes	ObjectType(ObjectId inId)
{ return (KernelTypes) (inId & kObjectTypeMask); }

extern "C" {
#endif
extern bool	IsSuperMode(void);
extern bool	IsFIQMode(void);
extern void	EnterAtomic(void);
extern void	ExitAtomic(void);
extern void	EnterFIQAtomic(void);
extern void	ExitFIQAtomic(void);
#if defined(__cplusplus)
}
#endif

#endif	/* __SHAREDTYPES_H */
