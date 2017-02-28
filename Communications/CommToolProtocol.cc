/*
	File:		CommToolProtocol.cc

	Contains:	Comm tool protocol.

	Written by:	Newton Research Group, 2016.
*/

#include "CommToolProtocol.h"
#include "OSErrors.h"


/*------------------------------------------------------------------------------
	C C o m m T o o l P r o t o c o l
	NONVIRTUAL protocol methods.
------------------------------------------------------------------------------*/

/*	callbacks for base CommTool behavior, call as "utility" functions or treat as
	inherited calls (i.e. given function X(), inherited::X() would be CTX())
*/

NONVIRTUAL	NewtonErr			CTTaskConstructor()
{return 0;}

NONVIRTUAL	void					CTTaskDestructor()
{}

NONVIRTUAL	NewtonErr			CTGetToolPort(ObjectName toolId, CUPort& port) // private, don't use
{return 0;}

NONVIRTUAL	ObjectName			CTGetToolId() // private, don't use
{return 0;}


NONVIRTUAL	void					CTCompleteRequest(CUMsgToken& msgToken, NewtonErr result)
{}
NONVIRTUAL	void					CTCompleteRequest(CUMsgToken& msgToken, NewtonErr result, CCommToolReply& reply)
{}
NONVIRTUAL	void					CTCompleteRequest(CommToolChannelNumber channel, NewtonErr result)
{}
NONVIRTUAL	void					CTCompleteRequest(CommToolChannelNumber channel, NewtonErr result, CCommToolReply& reply)
{}

NONVIRTUAL	void					CTHandleRequest(CUMsgToken& msgToken, ULong msgType)
{}
NONVIRTUAL	void					CTHandleReply(ULong userRefCon, ULong msgType)
{}

NONVIRTUAL	void					CTDoControl(ULong opCode, ULong msgType)
{}
NONVIRTUAL	void					CTDoKillControl(ULong msgType)
{}
NONVIRTUAL	void					CTGetCommEvent()
{}
NONVIRTUAL	void					CTDoKillGetCommEvent()
{}
NONVIRTUAL	NewtonErr			CTPostCommEvent(CCommToolGetEventReply& theEvent, NewtonErr result)
{return 0;}


NONVIRTUAL	void					CTOpenContinue();	// private, don't use

NONVIRTUAL	NewtonErr			CTOpenStart(COptionArray* options)
{return 0;}

NONVIRTUAL	NewtonErr			CTOpenComplete()
{return 0;}


NONVIRTUAL	bool					CTClose()
{return 0;}
NONVIRTUAL	void					CTCloseComplete(NewtonErr result)
{}

NONVIRTUAL	void					CTConnectStart()
{}
NONVIRTUAL	void					CTConnectComplete(NewtonErr result)
{}

NONVIRTUAL	void					CTListenStart()
{}
NONVIRTUAL	void					CTListenComplete(NewtonErr result)
{}

NONVIRTUAL	void					CTAcceptStart()
{}
NONVIRTUAL	void					CTAcceptComplete(NewtonErr result)
{}

NONVIRTUAL	void					CTDisconnectComplete(NewtonErr result)
{}

NONVIRTUAL	void					CTReleaseStart()
{}
NONVIRTUAL	void					CTReleaseComplete(NewtonErr result)
{}

NONVIRTUAL	void					CTBind()
{}
NONVIRTUAL	void					CTUnbind()
{}

NONVIRTUAL	void					CTGetProtAddr();	// private, don't use

NONVIRTUAL	ULong					CTProcessOption(COption* theOption, ULong label, ULong opcode)
{return 0;}

NONVIRTUAL	NewtonErr			CTAddDefaultOptions(COptionArray* options)
{return 0;}

NONVIRTUAL	NewtonErr			CTAddCurrentOptions(COptionArray* options)
{return 0;}


NONVIRTUAL	void					CTPutComplete(NewtonErr result, ULong putBytesCount)
{}
NONVIRTUAL	void					CTKillPutComplete(NewtonErr result)
{}
NONVIRTUAL	void					CTGetComplete(NewtonErr result, bool endOfFrame = false, ULong getBytesCount = 0)
{}
NONVIRTUAL	void					CTKillGetComplete(NewtonErr result)
{}

NONVIRTUAL	void					CTKillRequestComplete(CommToolRequestType requestTypeKilled, NewtonErr killResult)
{}

NONVIRTUAL	void					CTHoldAbort()		// private, don't use
{}

NONVIRTUAL	void					CTAllowAbort()	// private, don't use
{}

NONVIRTUAL	NewtonErr			CTStartAbort(NewtonErr abortError)
{return 0;}

NONVIRTUAL	bool					CTShouldAbort(ULong stateFlag, NewtonErr result);	// private, don't use

NONVIRTUAL	void					CTTerminateConnection()
{}
NONVIRTUAL	void					CTTerminateComplete()
{}

NONVIRTUAL	NewtonErr			CTInitAsyncRPCMsg(CUAsyncMessage& asyncMsg, ULong refCon)
{return 0;}


NONVIRTUAL	NewtonErr			CTFlushChannel(CommToolRequestType filter, NewtonErr flushResult)
{return 0;}


NONVIRTUAL	void					CTSetChannelFilter(CommToolRequestType msgType, bool enable)
{}
NONVIRTUAL	CommToolChannelNumber	CTRequestTypeToChannelNumber(CommToolRequestType msgType)
{return kCommToolControlChannel;}

NONVIRTUAL	CommToolRequestType		CTChannelNumberToRequestType(CommToolChannelNumber channelNumber)
{return kCommToolRequestTypeControl;}


// getters/setters
NONVIRTUAL	ULong					CTGetToolConnectState()
{return 0;}

NONVIRTUAL	void					CTSetToolConnectState(ULong state)
{}

NONVIRTUAL	NewtonErr			CTGetAbortErr()
{return 0;}

NONVIRTUAL	void					CTSetAbortErr(NewtonErr err)
{}

NONVIRTUAL	ULong					CTGetTerminationEvent()
{return 0;}

NONVIRTUAL	void					CTSetTerminationEvent(ULong event)
{}

//NONVIRTUAL	CCMOCTConnectInfo &		CTGetConnectInfo()
//{return 0;}

NONVIRTUAL	void					CTSetConnectInfo(CCMOCTConnectInfo& info)
{}

NONVIRTUAL	ULong					CTGetRequestSize()
{return 0;}

NONVIRTUAL	UChar *				CTGetRequest()
{return 0;}


//NONVIRTUAL	CCommToolMsgContainer	CTGetRequestListItem(UChar item)	// private, don't use
//{return 0;}
//
//NONVIRTUAL  ConnectParms &	  	CTGetConnectParms()	// private, don't use
//{return 0;}
//
//
//NONVIRTUAL	TCMOTransportInfo&		CTGetCMOTransportInfo()
//{return 0;}

NONVIRTUAL	void					CTSetCMOTransportInfo(CCMOTransportInfo& info)
{}

NONVIRTUAL 	CommToolRequestType		CTGetRequestsToKill()	// private, don't use
{return kCommToolRequestTypeControl;}

NONVIRTUAL  void					CTSetRequestsToKill(CommToolRequestType rType);	// private, don't use

NONVIRTUAL	void					toolInit(CPCommTool* tool);	// private, don't use

// New Methods for 2.0
// new getters/setters
NONVIRTUAL	ULong					CTGetReceiveMessageBufSize()		// returns the size of the buffer used by the commtool to receive rpc messages
																					// Any message sent to the tool which exceeds this size will be truncated.
{return 0;}

NONVIRTUAL	CUPort *				CTGetToolPort()						// returns a pointer to the tool's port object
{return 0;}


//NONVIRTUAL	CCommToolOptionInfo&	CTGetControlOptionsInfo()		// returns the option info for the current control request
//{return 0;}
//
//NONVIRTUAL	CCommToolOptionInfo&	CTGetGetBytesOptionsInfo()		// returns the option info for the current get bytes request
//{return 0;}
//
//NONVIRTUAL	CCommToolOptionInfo&	CTGetPutBytesOptionsInfo()		// returns the option info for the current put bytes request
//{return 0;}


NONVIRTUAL	CBufferList *		CTGetCurPutData()					// returns a pointer to the current put request buffer list
{return 0;}

NONVIRTUAL	bool					CTGetCurPutFrameData();			// returns true if current put request is framed
NONVIRTUAL	bool					CTGetCurPutEndOfFrame();		// returns true if current framed put request is end of frame

NONVIRTUAL	CBufferList *		CTGetCurGetData()					// returns a pointer to the current get request buffer list
{return 0;}

NONVIRTUAL	bool					CTGetCurGetFrameData()			// returns true if current get request is framed
{return 0;}

NONVIRTUAL	bool					CTGetCurGetNonBlocking()		// returns true if current get request is non blocking (ie has a threshold)
{return 0;}

NONVIRTUAL	Size					CTGetCurGetThreshold()			// returns the value of threshold for the current nonblocking get request
{return 0;}


NONVIRTUAL	bool					CTGetPassiveClaim()				// returns true if client asked for passive claim of tool resources
{return 0;}

NONVIRTUAL	void					CTSetPassiveClaim(bool passiveClaim); // Set the value of passive claim

NONVIRTUAL	bool					CTGetPassiveState()				// returns true if in passive state (resources passively claimed, and willing to give up resources)
{return 0;}

NONVIRTUAL	void					CTSetPassiveState(bool passiveState); // Set the value of passive claim

NONVIRTUAL	bool					CTGetWaitingForResNotify()		// returns true if tool has passively claimed resources, and is waiting for notification of resource ownership
{return 0;}

NONVIRTUAL	void					CTSetWaitingForResNotify(bool waitingForResNotify); // Set the value of passive claim

NONVIRTUAL	ULong					CTGetCurRequestOpCode()		// returns the opcode of the current control request
{return 0;}


// new base class methods
NONVIRTUAL 	void					CTBindStart()
{}
NONVIRTUAL 	void					CTBindComplete(NewtonErr result)
{}

NONVIRTUAL 	void					CTUnbindStart()
{}
NONVIRTUAL 	void					CTUnbindComplete(NewtonErr result)
{}

NONVIRTUAL 	ULong					CTProcessOptionStart(COption* theOption, ULong label, ULong opcode)
{return 0;}

NONVIRTUAL 	void					CTProcessOptionComplete(ULong optResult)
{}

NONVIRTUAL 	ULong					CTProcessPutBytesOptionStart(COption* theOption, ULong label, ULong opcode)
{return 0;}

NONVIRTUAL 	void 					CTProcessPutBytesOptionComplete(ULong optResult)
{}

NONVIRTUAL 	ULong 				CTProcessGetBytesOptionStart(COption* theOption, ULong label, ULong opcode)
{return 0;}

NONVIRTUAL 	void 					CTProcessGetBytesOptionComplete(ULong optResult)
{}

NONVIRTUAL 	void					CTResArbRelease(UChar* resName, UChar* resType)
{}
NONVIRTUAL 	void					CTResArbReleaseStart(UChar* resName, UChar* resType)
{}
NONVIRTUAL 	void					CTResArbReleaseComplete(NewtonErr result)
{}

NONVIRTUAL 	void					CTResArbClaimNotification(UChar* resName, UChar* resType)
{}

NONVIRTUAL	void					CTHandleInternalEvent()
{}
