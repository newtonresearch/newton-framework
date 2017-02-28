/*
	File:		CommTool.h

	Contains:	Base class for communications tools.

	Copyright:	� 1992-1995 by Apple Computer, Inc., all rights reserved.
*/

#if !defined(__COMMTOOL_H)
#define __COMMTOOL_H 1

#ifndef __CONFIGCOMMUNICATIONS_H
#include "ConfigCommunications.h"
#endif

#include "CommErrors.h"
#include "OSErrors.h"

#ifndef	__USERPORTS_H
#include "UserPorts.h"
#endif

#ifndef	__USERTASKS_H
#include "UserTasks.h"
#endif

#ifndef	__NAMESERVER_H
#include "NameServer.h"
#endif

#ifndef	__BUFFERLIST_H
#include "BufferList.h"
#endif

#ifndef	__BUFFERSEGMENT_H
#include "BufferSegment.h"
#endif

#ifndef	__EVENTS_H
#include "Events.h"
#endif

#ifndef	__OPTIONARRAY_H
#include "OptionArray.h"
#endif

#ifndef	__COMMOPTIONS_H
#include "CommOptions.h"
#endif

#ifndef	__COMMTOOLOPTIONS_H
#include "CommToolOptions.h"
#endif

#ifndef	__COMMADDRESSES_H
#include "CommAddresses.h"
#endif

#ifndef	__TRANSPORT_H
#include "Transport.h"
#endif

#ifndef __TRACEEVENTS_H
//#include "TraceEvents.h"
#endif


//--------------------------------------------------------------------------------
//		CommTool Tracing
//--------------------------------------------------------------------------------

// All comm tools descend from CCommTool, so we define a trace buffer that all
// subclasses can use. It is defined if hasBasicCommTracing is true.
// Macros for using this buffer are defined here as well.
//
// If hasBasicCommTracing is defined, the COMM_BASIC_ADDR_TRACE and COMM_BASIC_TRACE_EVENT
// generate code. If hasFullCommTracing is defined, COMM_TRACE_EVENT and COMM_ADDR_TRACE
// are also active.
//
// NOTE: Subclass tools should use their own "traceXXXTool" to control tracing in that
// tool by defining away any of these macros used as follows:
//
// #if defined(traceXXXTool)
//  	#if defined(hasFullCommTracing)
//		EventTraceCauseDesc XXXToolTraceEvents[] = {
//		etc...
//		};
//  	#endif // hasFullCommTracing
// #else
// 		#define	COMM_BASIC_ADDR_TRACE()
//		#define	COMM_BASIC_TRACE_EVENT(x, y)
//		#define	COMM_ADDR_TRACE()
//		#define	COMM_TRACE_EVENT(x, y)
// #endif // traceXXXTool
//
// ... and later in the constructor:
//
//	#if defined(traceXXXTool) && defined(hasFullCommTracing)
//	fEventBuffer->addDescriptions(XXXToolTraceEvents, sizeof(XXXToolTraceEvents) / sizeof(EventTraceCauseDesc));
//	#endif
//
// Direct subclasses can add event buffer descriptors (addDescriptions) for events
// defined starting at kLastCommTraceEvent...

#ifdef hasBasicCommTracing
#define	COMM_BASIC_ADDR_TRACE() fEventBuffer->addAddress()
#define	COMM_BASIC_TRACE_EVENT(x, y) fEventBuffer->add((uint32_t)(((uint32_t)((x) & 0xFF) << 24) + ((uint32_t)((y) & 0xFF) << 16) + (uint32_t)((y) & 0xFFFF)))
#define	COMM_BASIC_TRACE_EVENT2(x, y, z) fEventBuffer->add((uint32_t)(((uint32_t)((x) & 0xFF) << 24) + ((uint32_t)((y) & 0xFF) << 16) + (uint32_t)((z) & 0xFFFF)))
#else
#define	COMM_BASIC_ADDR_TRACE()
#define	COMM_BASIC_TRACE_EVENT(x, y)
#define	COMM_BASIC_TRACE_EVENT2(x, y, z)
#endif

#ifdef hasFullCommTracing
#define	COMM_ADDR_TRACE() fEventBuffer->addAddress()
#define	COMM_TRACE_EVENT(x, y) fEventBuffer->add((uint32_t)(((uint32_t)((x) & 0xFF) << 24) + ((uint32_t)((y) & 0xFF) << 16) + (uint32_t)((y) & 0xFFFF)))
#define	COMM_TRACE_EVENT2(x, y, z) fEventBuffer->add((uint32_t)(((uint32_t)((x) & 0xFF) << 24) + ((uint32_t)((y) & 0xFF) << 16) + (uint32_t)((z) & 0xFFFF)))
#else
#define	COMM_ADDR_TRACE()
#define	COMM_TRACE_EVENT(x, y)
#define	COMM_TRACE_EVENT2(x, y, z)
#endif

#define kLastCommTraceEvent (96)

extern NewtonErr InitCommTools();


// Tool types
const ULong		kConnectionCommTool			= 'cnct';
const ULong		kTranslationCommTool			= 'trsl';

// Tool sub-types
const ULong		kSessionCnctCommTool			= 'sson';
const ULong		kSessionlessCnctCommTool	= 'sson';
const ULong		kBitMapTrslCommTool			= 'bitm';

const size_t	kCommToolStackSize			=  6000;
const size_t	kCommToolMaxRequestSize		=    64;

const size_t	kCommToolDefaultHeapSize	= 50000;


//--------------------------------------------------------------------------------
//		CommToolOpCodes
//
// The standard CommToolOpCodes correspond to standard Comm Tool control calls.
// These opcodes, as well as the message types, define the interface for sending
// events FROM THE CLIENT TO THE TOOL.
//--------------------------------------------------------------------------------

enum CommToolOpCodes
{
	kCommToolOpen = 1,				// open comm tool (create instance variables)
	kCommToolClose,					// dispose connection end instance variables
	kCommToolConnect,					// for tools that support connection, initiate connection (active)
	kCommToolListen,					// for tools that support connection, listen for connection (passive)
	kCommToolAccept,					// for tools that support connection, accept connection request (passive)
	kCommToolDisconnect,				// terminate current connection (aborting current data) (active),  or reject conn. request (passive)
	kCommToolRelease,					// orderly connection shut down
	kCommToolBind,						// bind tool connection end to address (passive)
	kCommToolUnbind,					// unbind tool connection end to address (passive)
	kCommToolOptionMgmt,				// option management
	kCommToolGetProtAddr,			// get local and remote protocol address of connection
	kCommToolCreateConnectionEnd,	// create a connection end
	kCommToolDisposeConnectionEnd,// destroy a connection end
	kCommToolOpenMux					// Open a multiplexting commTool (aka MuxTool)
};

// NOTE - Control call OpCodes are shared by CCommTool as well as tool implementations.
// Tool implementation begin defining their opcodes at kCommToolCmdMax.
// So if CCommTool ever needs more than 256 commands, this will effect all
// tool implementations which define their own control call opcodes.

#define kCommToolCmdMax		(256)			// must be last command


enum CommToolStatusOpCodes
{
	kCommToolGetConnectState = 1,			// returns current tool connect state
	kCommToolDumpState,						// for testing/debug, returns more than you ever wanted to know
	kCommToolStatusMax
};


enum CommToolResArbOpCodes
{
	kCommToolResArbRelease = 1,			// request to release resource
	kCommToolResArbClaimNotification		// notification of active ownership of passive resource
};


//--------------------------------------------------------------------------------
//		CommToolEvents
//--------------------------------------------------------------------------------

enum CommToolEvents
{
	kCommToolEventDisconnected = T_DISCONNECT,	// current connection gone, this event can be returned after a connect call has completed successfully

	kCommToolEventSerialEvent = T_SPECEVT,			// serial tool event, e.g., DCD change, break rcvd., etc.

	kCommToolEventProgressEvent = T_PROGRESSEVT	// progress event, e.g., N fax lines have been received.

	// Motorola Radio Tool:  32000 to 32999
};


//--------------------------------------------------------------------------------
//		CommToolRequestType
//		MUST match order of CommToolChannelNumber
//--------------------------------------------------------------------------------

enum CommToolRequestType
{
	kCommToolRequestTypeGet				= 0x00000001,		// get requests use this channel
	kCommToolRequestTypePut				= 0x00000002,		// put requests use this channel
	kCommToolRequestTypeControl		= 0x00000004,		// control calls use this channel
	kCommToolRequestTypeGetEvent		= 0x00000008,		// Get Comm Event requests use this channel
	kCommToolRequestTypeKill			= 0x00000010,		// kill request
	kCommToolRequestTypeStatus			= 0x00000020,		// status calls use this channel
	kCommToolRequestTypeResArb			= 0x00000040,		// resource arbitration requests use this channel

	kCommToolRequestTypeMask			= 0x000000FF		// mask for CommTool requests
};


//--------------------------------------------------------------------------------
//		CommToolChannelNumber
//		MUST match order of CommToolRequestType
//--------------------------------------------------------------------------------

enum CommToolChannelNumber
{
	kCommToolGetChannel,
	kCommToolPutChannel,
	kCommToolControlChannel,
	kCommToolGetEventChannel,
	kCommToolKillChannel,
	kCommToolStatusChannel,
	kCommToolResArbChannel,
	kCommToolNumChannels
};


// CommTool reserved TUAsyncMessage refCons
enum
{
	kToolMsgForwardOpt					= 20000
};


//--------------------------------------------------------------------------------
//		CommToolConnectInfo
//--------------------------------------------------------------------------------

enum CommToolConnectInfo
{
	kCommConnectionSupportsCallBack	= 0x00000001,			// set if the connection supports call back security
	kCommConnectionViaAppleTalk		= 0x00000002,			// set if the connection is over AppleTalk
	kCommConnectionReliable				= 0x00000004			// set if the connection is reliable (guaranteed in order delivery)
};

//	fState
enum
{
	kToolStateConnecting				= 0x00000001,			// tool is trying to establish a connection
	kToolStateConnected				= 0x00000002,			// tool is connected
	kToolStateWantAbort				= 0x00000004,			// aborting or waiting to abort task
	kToolStateRelease					= 0x00000008,			// orderly connection shut down.
	kToolStateClosing					= 0x00000010,			// set when tool is closing down
	kToolStateDisconnectReq			= 0x00000020,			// set when disconnect request is pending
	kToolStateListenMode				= 0x00000040,
	kToolStateTerminating			= 0x00000080,			// set when connection is being terminated
	kToolStateBound					= 0x00000100			// set when the endpoint is in bound state
};

// fOptionsState
enum
{
	kOptionsInProgress				= 0x00000001,			// set if option array is being processed
	kOptionsNotProcessed				= 0x00000002,			// set when an option is not processed, attempt to forward request
	kOptionsArrayAllocated			= 0x00000004,			// set when option array is allocated for an "outside" request
	kOptionsForwardReqActive		= 0x00000008,			// set when option request has been forwarded, and RPC is in progress
	kOptionsForwardMsgAllocated	= 0x00000010,			// set when message blocks for options forward request have been allocated
	kOptionsOutside					= 0x00000020			// set when options request is outside (i.e. uses shared memory object for option array)
};


//--------------------------------------------------------------------------------
//		CommTool Trace Events
//--------------------------------------------------------------------------------

// First subclass can add events starting at kLastCommTraceEvent constant below


//--------------------------------------------------------------------------------
//		CCommToolEvent
//--------------------------------------------------------------------------------

class CCommToolEvent : public CEvent
{
public:
						CCommToolEvent();
};

inline CCommToolEvent::CCommToolEvent() { fEventId = kCommToolEventId; }


//--------------------------------------------------------------------------------
//		CCommToolControlRequest
//--------------------------------------------------------------------------------

class CCommToolControlRequest : public CCommToolEvent
{
public:
						CCommToolControlRequest();

	ULong				fOpCode;					// request operation code, used to differentiate control calls
};

inline CCommToolControlRequest::CCommToolControlRequest() { fOpCode = 0; }


//--------------------------------------------------------------------------------
//		CCommToolReply
//
// For most CommTool RPC requests, the reply is a long indicating the result
// of the request.  Other requests, such as Get, return more information.
//--------------------------------------------------------------------------------

class CCommToolReply : public CCommToolEvent
{
public:
						CCommToolReply();

	NewtonErr		fResult;					// indicates result of request
	size_t			fSize;					// size of this reply
};

inline CCommToolReply::CCommToolReply() { fResult = noErr; fSize = sizeof(CCommToolReply); }


//--------------------------------------------------------------------------------
//		CCommToolPutRequest
//--------------------------------------------------------------------------------

class CCommToolPutRequest : public CCommToolEvent
{
public:
						CCommToolPutRequest();

	CBufferList *	fData;					// data list, or shared memory ObjectId

	// these fields are for clients outside our domain

	ArrayIndex		fValidCount;			// count of valid bytes in fData; kIndexNotFound => "all"
													// data is assumed to start at offset 0
	bool				fOutside;				// true, fData is a shared memory object id

	// these fields are for clients using framing
	bool				fFrameData;				// true, use framing
	bool				fEndOfFrame;			// true, close the frame

	// Optional options for Put request
	COptionArray *	fOptions;				// 2.0 // COptionArray
	ArrayIndex		fOptionCount;			// 2.0 // number of options specified in the request option array, only used if outside
};


//--------------------------------------------------------------------------------
//		CCommToolPutReply
//--------------------------------------------------------------------------------

class CCommToolPutReply : public CCommToolReply
{
public:
						CCommToolPutReply();

	ArrayIndex		fPutBytesCount;		// number of bytes put
};


//--------------------------------------------------------------------------------
//		CCommToolGetRequest
//--------------------------------------------------------------------------------

class CCommToolGetRequest : public CCommToolEvent
{
public:
						CCommToolGetRequest();

	CBufferList *	fData;				// data list, or shared memory ObjectId
	ArrayIndex		fThreshold;			// for fNonBlocking true, do not complete until
												// at least this many bytes have arrived
												// if zero, then complete immediately
	bool				fNonBlocking;		// true, return as soon as fThreshold is satisfied
	bool				fFrameData;			// true, deframe the data as it arrives
	bool				fOutside;			// true, fData is a shared memory object id
	COptionArray *	fOptions;			// 2.0 // COptionArray
	ArrayIndex		fOptionCount;		// 2.0 // number of options specified in the request option array, only used if outside
};


//--------------------------------------------------------------------------------
//		CCommToolGetReply
//--------------------------------------------------------------------------------

class CCommToolGetReply : public CCommToolReply
{
public:
						CCommToolGetReply();

	bool				fEndOfFrame;
	ArrayIndex		fGetBytesCount;		// number of bytes got
};


//--------------------------------------------------------------------------------
//		CCommToolOpenRequest
//--------------------------------------------------------------------------------

class CCommToolOpenRequest : public CCommToolControlRequest
{
public:
						CCommToolOpenRequest();

	COptionArray *	fOptions;				// open options array
	ArrayIndex		fOptionCount;			// number of options specified in the option array

	bool				fOutside;				// fOptionArray is actually a sharedId
};


//--------------------------------------------------------------------------------
//		CCommToolOpenReply
//--------------------------------------------------------------------------------

class CCommToolOpenReply : public CCommToolReply
{
public:
						CCommToolOpenReply();

	ObjectId			fPortId;					// id of port for tool or connection end
};


//--------------------------------------------------------------------------------
//		CCommToolConnectRequest
//
//	used for listen, accept, and connect
//--------------------------------------------------------------------------------

class CCommToolConnectRequest : public CCommToolControlRequest
{
public:
						CCommToolConnectRequest();

	ULong				fReserved1;				// +0C reserved, don't use
	ULong				fReserved2;				// +10 reserved, don't use
	COptionArray *	fOptions;				// COptionArray
	ArrayIndex		fOptionCount;			// number of options specified in the request option array
	CBufferSegment *	fData;
	ULong				fSequence;				// from CCommToolListenReply

	bool				fOutside;				// if true, the above are ObjectIds
};


//--------------------------------------------------------------------------------
//		CCommToolConnectReply
//
//	used for listen, accept, and connect
//--------------------------------------------------------------------------------

class CCommToolConnectReply : public CCommToolReply
{
public:
						CCommToolConnectReply();

	ULong				fSequence;
};


//--------------------------------------------------------------------------------
//		CCommToolDisconnectRequest
//--------------------------------------------------------------------------------

class CCommToolDisconnectRequest : public CCommToolControlRequest
{
public:
						CCommToolDisconnectRequest();

	CBufferSegment *	fDisconnectData;
	ULong				fSequence;				// from CCommToolListenReply, zero otherwise
	ULong				fReason;					// reason code

	bool				fOutside;				// if true, the above *'s are ObjectIds
};


//--------------------------------------------------------------------------------
//		CCommToolBindRequest
//--------------------------------------------------------------------------------

class CCommToolBindRequest : public CCommToolControlRequest
{
public:
						CCommToolBindRequest();

	ULong				fReserved1;				// reserved, don't use
	ULong				fReserved2;				// reserved, don't use

	bool				fOutside;				// if true, the above *'s are ObjectIds
	bool				fCopyBack;				// if true and fOutside true, copy back the data on reply

	COptionArray *	fOptions;				// used as both a request and reply parameter
	ArrayIndex		fOptionCount;			// number of options specified in the request option array

};


//--------------------------------------------------------------------------------
//		CCommToolBindReply
//--------------------------------------------------------------------------------

class CCommToolBindReply : public CCommToolReply
{
public:
						CCommToolBindReply();

	ULong				fWastedULong;
};



//--------------------------------------------------------------------------------
//		CCommToolOptionMgmtRequest
//--------------------------------------------------------------------------------

class CCommToolOptionMgmtRequest : public CCommToolControlRequest
{
public:
						CCommToolOptionMgmtRequest();

	COptionArray *	fOptions;				// used as both a request and reply parameter
	ArrayIndex		fOptionCount;			// number of options specified in the request option array
	ULong				fRequestOpCode;

	bool				fOutside;				// if true, the above *'s are ObjectIds
	bool				fCopyBack;				// if true and fOutside true, copy back the data on reply
};


//--------------------------------------------------------------------------------
//		CCommToolGetProtAddrRequest
//--------------------------------------------------------------------------------

class CCommToolGetProtAddrRequest : public CCommToolControlRequest
{
public:
						CCommToolGetProtAddrRequest();

	// NOTE: these fields are used as return values,
	// and are valid only after the GetProtAddr request has completed

	COptionArray *	fBoundAddr;
	COptionArray *	fPeerAddr;

	bool				fOutside;					// if true, the above *'s are ObjectIds
};


//--------------------------------------------------------------------------------
//		CCommToolGetEventReply
//--------------------------------------------------------------------------------

class CCommToolGetEventReply : public CCommToolReply
{
public:
						CCommToolGetEventReply();

	ULong				fEventCode;					// the event
	CTime				fEventTime;					// global time event occurred
	ULong				fEventData;					// event data
	ULong				fServiceId;					// Service Id of tool which originated the event
};


//--------------------------------------------------------------------------------
//		CCommToolKillRequest
//--------------------------------------------------------------------------------

class CCommToolKillRequest : public CCommToolEvent
{
public:
						CCommToolKillRequest();

														// defaults to all channels
	CommToolRequestType	fRequestsToKill;	// bit map of request types to kill
	// NOTE:  kill is not valid on status or timer or kill channel
};


//--------------------------------------------------------------------------------
//		CCommToolStatusRequest
//--------------------------------------------------------------------------------

class CCommToolStatusRequest : public CCommToolControlRequest
{
public:
						CCommToolStatusRequest();

	ObjectId			fStatusInfoId;				// shared memory id of container for status info
};


//--------------------------------------------------------------------------------
//		CCommToolResArbRequest
//--------------------------------------------------------------------------------

class CCommToolResArbRequest : public CCommToolControlRequest
{
public:
						CCommToolResArbRequest();

	UChar *			fResNamePtr;				// pointer to resource name "C" string
	UChar *			fResTypePtr;				// pointer to resource type "C" string
};

inline CCommToolResArbRequest::CCommToolResArbRequest() { fResNamePtr = NULL; fResTypePtr = NULL; }


//--------------------------------------------------------------------------------
//		CCommToolMsgContainer
//--------------------------------------------------------------------------------

class CCommToolMsgContainer : public SingleObject
{
public:
						CCommToolMsgContainer();

	bool				fRequestPending;
	ULong				fRequestMsgSize;
	CUMsgToken		fMsgToken;
};


//--------------------------------------------------------------------------------
//		CCommToolOptionInfo
//--------------------------------------------------------------------------------

class CCommToolOptionInfo : public SingleObject
{
public:
						CCommToolOptionInfo();

	ULong				fOptionsState;
	CommToolChannelNumber	fChannelNum;	// channel number of options request
	ArrayIndex		fOptionCount;				// used for outside requests only, indicates number of options in option array
	COptionArray * fOptions;					// pointer to request options, or ObjectId
	COption *		fCurOptPtr;					// pointer to current option
	COptionIterator *	fOptionsIterator;		// pointer to option array iterator for current options request
};


typedef bool (*TerminateProcPtr)(void* refPtr);


//--------------------------------------------------------------------------------
//		ConnectParms
//--------------------------------------------------------------------------------

struct ConnectParms
{
	CBuffer *	udata;
	ULong			sequence;
	bool			outside;
};

typedef struct ConnectParms ConnectParms, *ConnectParmsPtr;


/*--------------------------------------------------------------------------------
	CCommTool
	vt @ 00020810
	Abstract base class for all communication tools.
	Each tool runs in its own task world.
	ASIDE	How does this tie in with CCommToolProtocol?
--------------------------------------------------------------------------------*/

class CCommTool : public CUTaskWorld
{
public:
								CCommTool(ULong inName);
								CCommTool(ULong inName, size_t inHeapSize);
	virtual					~CCommTool();

// CUTaskWorld virtual member functions
protected:
			  size_t			getSizeOf(void) const;
			  NewtonErr		taskConstructor(void);
			  void			taskDestructor(void);
			  void			taskMain(void);

// CCommTool member functions
public:
	virtual const UChar * getToolName(void) const = 0;

	virtual void			handleInternalEvent(void);
	virtual void			handleRequest(CUMsgToken & inMsgToken, ULong inMsgType);
	virtual void			handleReply(ULong inUserRefCon, ULong inMsgType);
	virtual void			handleTimerTick(void);

	virtual void			doControl(ULong inOpCode, ULong inMsgType);
	virtual void			doKillControl(ULong inMsgType);
	virtual void			doStatus(ULong inOpCode, ULong inMsgType);
	
	virtual void			getCommEvent(void);
	virtual void			doKillGetCommEvent(void);
	virtual NewtonErr		postCommEvent(CCommToolGetEventReply& ioEvent, NewtonErr inResult);

			  void			open(void);
			  void			openOptionsComplete(NewtonErr inResult);
	virtual NewtonErr		openStart(COptionArray * inOptions);
			  void			openContinue(void);
	virtual NewtonErr		openComplete(void);

	virtual bool			close(void);
	virtual void			closeComplete(NewtonErr inResult);

			  NewtonErr		importConnectPB(CCommToolConnectRequest * inReq);
			  NewtonErr		copyBackConnectPB(NewtonErr inResult);
			  NewtonErr		connectCheck(void);
			  void			connect(void);
			  void			connectOptionsComplete(NewtonErr inResult);
	virtual void			connectStart(void);
	virtual void			connectComplete(NewtonErr inResult);

			  void			listen(void);
			  void			listenOptionsComplete(NewtonErr inResult);
	virtual void			listenStart(void);
	virtual void			listenComplete(NewtonErr inResult);

			  void			accept(void);
			  void			acceptOptionsComplete(NewtonErr inResult);
	virtual void			acceptStart(void);
	virtual void			acceptComplete(NewtonErr inResult);

			  void			disconnect(void);
	virtual void			disconnectComplete(NewtonErr inResult);

			  void			release(void);
	virtual void			releaseStart(void);
	virtual void			releaseComplete(NewtonErr inResult);

			  void			bind(void);
			  void			bindOptionsComplete(NewtonErr inResult);
	virtual void			bindStart(void);
	virtual void			bindComplete(NewtonErr inResult);

			  void			unbind(void);
	virtual void			unbindStart(void);
	virtual void			unbindComplete(NewtonErr inResult);

	virtual void			getProtAddr(void);

			  NewtonErr		getConnectState(void);
			  NewtonErr		flushChannel(CommToolRequestType inType, NewtonErr inResult);
			  void			completeRequest(CUMsgToken&, NewtonErr);
			  void			completeRequest(CUMsgToken&, NewtonErr, CCommToolReply&);
			  void			completeRequest(CommToolChannelNumber, NewtonErr);
			  void			completeRequest(CommToolChannelNumber, NewtonErr, CCommToolReply&);

	virtual void		 	optionMgmt(CCommToolOptionMgmtRequest *);
	virtual void		 	optionMgmtComplete(NewtonErr inResult);

			  void			processOptions(COptionArray*);
			  void			processControlOptions(bool, COptionArray*, ULong);
	virtual void			processOptions(CCommToolOptionInfo*);
	virtual void			processOptionsContinue(CCommToolOptionInfo*);
	virtual void			processOptionsComplete(NewtonErr, CCommToolOptionInfo*);
	virtual NewtonErr		processOptionsCleanUp(NewtonErr, CCommToolOptionInfo*);

	virtual void			processCommOptionComplete(ULong inStatus, CCommToolOptionInfo * info);
	virtual NewtonErr		processOptionStart(COption * inOption, ULong inLabel, ULong inOpcode);
	virtual void			processOptionComplete(ULong inResult);
	virtual void			processOption(COption * inOption, ULong inLabel, ULong inOpcode);
	
	virtual CUPort *		forwardOptions(void);
	virtual NewtonErr		addDefaultOptions(COptionArray* inOptions);
	virtual NewtonErr		addCurrentOptions(COptionArray* inOptions);

	virtual NewtonErr		processPutBytesOptionStart(COption* theOption, ULong inLabel, ULong inOpcode);
	virtual void			processPutBytesOptionComplete(ULong);
	virtual NewtonErr		processGetBytesOptionStart(COption* theOption, ULong inLabel, ULong inOpcode);
	virtual void			processGetBytesOptionComplete(ULong);
	
	virtual void			putBytes(CBufferList *)= 0;
	virtual void			putFramedBytes(CBufferList *, bool) = 0;
	virtual void			putComplete(NewtonErr inResult, size_t inPutBytesCount);
	virtual void			killPut(void) = 0;
	virtual void			killPutComplete(NewtonErr inResult);

	virtual void			getBytes(CBufferList *)= 0;
	virtual void			getFramedBytes(CBufferList *)= 0;
	virtual void			getBytesImmediate(CBufferList * inClientBuffer, size_t inThreshold);
	virtual void			getComplete(NewtonErr inResult, bool inEndOfFrame = false, size_t inGetBytesCount = 0);
	virtual void			killGet(void) = 0;
	virtual void			killGetComplete(NewtonErr inResult);

	virtual void			prepGetRequest(void);
	virtual void			getOptionsComplete(NewtonErr inResult);

	virtual void			prepPutRequest(void);
	virtual void			putOptionsComplete(NewtonErr inResult);
	
			  void			prepControlRequest(ULong inMsgType);

			  void			prepKillRequest(void);
			  void			killRequestComplete(CommToolRequestType inReq, NewtonErr inResult);

			  void			prepResArbRequest(void);
	virtual void			resArbRelease(unsigned char*, unsigned char*);
	virtual void			resArbReleaseStart(unsigned char*, unsigned char*);
	virtual void			resArbReleaseComplete(NewtonErr);
	virtual void			resArbClaimNotification(unsigned char*, unsigned char*);

			  void			holdAbort(void);
			  void			allowAbort(void);
	virtual NewtonErr		startAbort(NewtonErr inReason);
			  bool			shouldAbort(ULong inResetFlags, NewtonErr inReason);

	virtual void			terminateConnection(void);
	virtual void			terminateComplete(void);
	virtual void			getNextTermProc(ULong inTerminationPhase, ULong& ioTerminationFlag, TerminateProcPtr& inTerminationProc);
	virtual void			setChannelFilter(CommToolRequestType inFilter, bool inDoSet);
	ArrayIndex				requestTypeToChannelNumber(CommToolRequestType inReqType);


protected:
	NewtonErr				createPort(ULong inId, CUPort & outPort);
	NewtonErr				getToolPort(ULong inId, CUPort & outPort);
	void						unregisterPort(void);
	NewtonErr				initAsyncRPCMsg(CUAsyncMessage & ioMsg, ULong inRefCon);

	ULong						fState;					// +18
	ULong						fTermFlag;				// +1C	termination flag
	ULong						fTermPhase;				// +20	termination phase sequence number
	NewtonErr				fErrStatus;				// +24
	int						f28;						// +28
	int						fAbortLock;				// +002C

	CCMOCTConnectInfo 	fConnectInfo;			// +0030
															// +0044
	size_t					f48;						// +0048 size of message following
	char						f4C[0x40];				// +004C	enough space for the largest CCommToolxxxRequest class
	CUPort					f8C;						// +008C
	CCommToolMsgContainer	f94[kCommToolNumChannels];				// +0094	size+18
	CBufferSegment *		f13C;						// +013C
	ULong						f140;						// +0140	sequence
	bool						f144;						// +0144	isOutside
	CCMOTransportInfo		fTransportInfo;		// +0148

	long						f174;						// +0174
	CCommToolOptionInfo 	fControlOptionInfo;	// +0178
	CCommToolOptionInfo 	fGetOptionInfo;		// +0190
	CCommToolOptionInfo 	fPutOptionInfo;		// +01A8

	CBufferList *			f1C0;						// +01C0	put queue
	bool						f1C4;						// +01C4
	bool						f1C5;						// +01C5
	bool						f1C6;						// +01C6
	bool						f1C7;						// +01C7

	CBufferList *			f1C8;						// +01C8	get queue
	ArrayIndex				f1CC;						// +01CC	threshold?
	bool						f1D0;						// +01D0
	bool						f1D1;						// +01D1
	bool						f1D2;						// +01D2

	ULong						f1D4;						// +01D4	opcode
	CCommToolGetEventReply	fGetEventReply;	// +01D8
	int						f1F8;						// +01F8
	NewtonErr				f1FC;						// +01FC
	bool						fIsClosed;				// +0200
	bool						f201;						// +0201	isPortRegistered
	ULong						fName;					// +0204
	CommToolRequestType	f208;						// +0208	channel filter
	CCommToolOptionMgmtRequest *	fOptionRequest;	// +020C
	CUAsyncMessage *		fOptionMessage;		// +0210
	CCommToolReply *		fOptionReply;			// +0214
	CBufferList *			f218;						// +0218
	CBufferList *			f21C;						// +021C
	CShadowBufferSegment	fBufferSegment_1;		// +0220
	CShadowBufferSegment	fBufferSegment_2;  	// +023C
															// +0248
	Heap						fSavedHeap;				// +0258
	Heap						fHeap;					// +025C
	size_t					fHeapSize;				// +0260
	Timeout					fTimeout;				// +0264
	Timeout					fTimeoutRemaining;	// +0268
// size +026C
};


#endif	/* __COMMTOOL_H */
