/*
	File:		SharedMem.h

	Contains:	Task interface to shared memory objects.
					Data passed between tasks is encapsulated in CSharedMem
					shared memory objects. Layered above this is CSharedMemMsg
					which handles the message passing protocol.

	Written by:	Newton Research Group.
*/

#if !defined(__SHAREDMEM_H)
#define __SHAREDMEM_H 1

#if !defined(__NEWTON_H)
#include "Newton.h"
#endif
#if !defined(__NEWTONTIME_H)
#include "NewtonTime.h"
#endif

#include "Environment.h"
#include "DoubleQ.h"

/*--------------------------------------------------------------------------------
	C S h a r e d M e m
--------------------------------------------------------------------------------*/

class CSharedMem : public CObject
{
public:
					CSharedMem(ObjectId id = 0) : CObject(id) {}
	NewtonErr	init(CEnvironment * inEnvironment);

	void *			fAddr;			// +10	address
	size_t			fBufSize;		// +14	allocated buffer size
	size_t			fCurSize;		// +18	current size used
	ULong				fPerm;			// +1C	permissions
	CEnvironment *	fEnvironment;	// +20
//	size +24
};


/*--------------------------------------------------------------------------------
	C S h a r e d M e m M s g
--------------------------------------------------------------------------------*/

typedef void (*TimeoutProcPtr)(void*);

class CSharedMemMsg : public CSharedMem
{
public:
					CSharedMemMsg();
					~CSharedMemMsg();

	NewtonErr	init(CEnvironment * inEnvironment);
	NewtonErr	completeSender(NewtonErr inErr = noErr);
	NewtonErr	completeReceiver(CSharedMemMsg * inMsg, NewtonErr inErr = noErr);
	NewtonErr	completeMsg(bool, ULong inType, NewtonErr inErr = noErr);

	Timeout				fTimeout;		// +24
	CTime					fExpiryTime;	// +28
	CDoubleQItem		fTimerQItem;	// +30	item in gTimerDeferred queue
	ULong					fFlags;			// +3C
	ObjectId				fSendingPort;	// +40	port sending this message
	NewtonErr			fStatus;			// +44
	int					fRefCon;			// +48
	ObjectId				fReplyMem;		// +4C
	ULong					fType;			// +50	message type (sender)
	ULong					fFilter;			// +54	message types accepted (receiver)
	ObjectId				fSendersMsg;	// +58
	ObjectId				fSendersMem;	// +5C	sender’s replyMemId
	ULong					fSendersType;	// +60	sender’s msgType
	ULong					fSendersSig;	// +64	sender’s signature
	ObjectId				fPortToNotify;	// +68
	ObjectId				fNotify;			//	+6C	object to notify when msg complete: task (sync) or port (async)
	ObjectId				fTaskToNotify;	// +70
	ULong					fSignature;		// +74
	ULong					fSequenceNo;	// +78
	ObjectId				fCopyTask;		// +7C	task performing low-level copy
	CDoubleQItem		f80;				// +80	item in queue of deferred sends
	CDoubleQContainer	f8C;
	void *				fCallbackData;	// +A0
	TimeoutProcPtr		fCallback;		// +A4	function to call on timeout
// size +A8
};

// non-error fStatus value
#define kInProgress 1


			  void		QueueNotify(CSharedMemMsg * inMsg);
extern "C" void		DoDeferrals(void);


#endif	/* __SHAREDMEM_H */
