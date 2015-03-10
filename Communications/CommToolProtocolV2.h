/*
	File:		CommToolProtocolV2.h

	Contains:	Protocol common to CCommTools.

	Copyright:	© 1993-1995 by Apple Computer, Inc., all rights reserved.
*/

#if !defined(__COMMTOOLPROTOCOLV2_H)
#define __COMMTOOLPROTOCOLV2_H 1

#ifndef __PROTOCOLS_H
#include "Protocols.h"
#endif

#ifndef	__OPTIONARRAY_H
#include "OptionArray.h"
#endif

#ifndef	__COMMTOOL_H
#include "CommTool.h"
#endif

class CServiceInfo;
class CPCommTool;
class CCMService;


// If you implement CCommToolProtocol as defined below, you must include a kCTInterfaceVersionType
// with value kCTPInterfaceVersion2Str
// The macro should be used as follows:
//	CAPABILITIES((kCTInterfaceVersionType kCTPInterfaceVersion2Str))

#define kCTInterfaceName				"CCommToolProtocol"
#define kCTInterfaceVersionType		"ctiv"
#define kCTPInterfaceVersion2Str		"2"

const long	kCTPInterfaceVersion2 = 2;


PROTOCOL CCommToolProtocol : public CProtocol
{
public:
	static		CCommToolProtocol *	New(char*);
					void						Delete();

					NewtonErr			taskConstructor();
					void					taskDestructor();

					UChar *				getToolName();

					void					handleRequest(CUMsgToken& msgToken, ULong msgType);
					void					handleReply(ULong userRefCon, ULong msgType);

					void					doControl(ULong opCode, ULong msgType);
					void					doKillControl(ULong msgType);
					void					getCommEvent();
					void					doKillGetCommEvent();
					NewtonErr			postCommEvent(CCommToolGetEventReply& theEvent, NewtonErr result);

					NewtonErr			openStart(COptionArray* options);
					NewtonErr			openComplete();
					bool					close();
					void					closeComplete(NewtonErr result);

					void					connectStart();
					void					connectComplete(NewtonErr result);

					void					listenStart();
					void					listenComplete(NewtonErr result);

					void					acceptStart();
					void					acceptComplete(NewtonErr result);

					void					disconnectComplete(NewtonErr result);

					void					releaseStart();
					void					releaseComplete(NewtonErr result);

/* OBSOLETE, leave implementation empty
					void					bind();
					void					unbind();

					void					getProtAddr();

					ULong					processOption(COption* theOption, ULong label, ULong opcode);
*/
					NewtonErr			addDefaultOptions(COptionArray* options);
					NewtonErr			addCurrentOptions(COptionArray* options);

					void					putBytes(CBufferList* clientBuffer);
					void					putFramedBytes(CBufferList* clientBuffer, bool endOfFrame);
					void					putComplete(NewtonErr result, ULong putBytesCount);

					void					killPut();
					void					killPutComplete(NewtonErr result);

					void					getBytes(CBufferList* clientBuffer);
					void					getFramedBytes(CBufferList* clientBuffer);
					void					getComplete(NewtonErr result, bool endOfFrame = false, ULong getBytesCount = 0);
					void					getBytesImmediate(CBufferList* clientBuffer, Size threshold);

					void					killGet();
					void					killGetComplete(NewtonErr result);

					void					terminateConnection();
					void					terminateComplete();
// OBSOLETE, leave implementation empty
//					void					getNextTermProc(ULong terminationPhase,ULong& terminationFlag,CerminateProcPtr& CerminationProc);

					void					setChannelFilter(int msgType, bool enable);

	/*	callbacks for base CommTool behavior, call as "utility" functions or treat as
		inherited calls (i.e. given function X(), inherited::X() would be CTX())
	*/

	NONVIRTUAL	NewtonErr			CTTaskConstructor();
	NONVIRTUAL	void					CTTaskDestructor();
	NONVIRTUAL	NewtonErr			CTGetToolPort(CObjectName toolId, CUPort& port); // private, don't use
	NONVIRTUAL	CObjectName			CTGetToolId(); // private, don't use

	NONVIRTUAL	void					CTCompleteRequest(CUMsgToken& msgToken, NewtonErr result);
	NONVIRTUAL	void					CTCompleteRequest(CUMsgToken& msgToken, NewtonErr result, CCommToolReply& reply);
	NONVIRTUAL	void					CTCompleteRequest(CommToolChannelNumber channel, NewtonErr result);
	NONVIRTUAL	void					CTCompleteRequest(CommToolChannelNumber channel, NewtonErr result, CCommToolReply& reply);

	NONVIRTUAL	void					CTHandleRequest(CUMsgToken& msgToken, ULong msgType);
	NONVIRTUAL	void					CTHandleReply(ULong userRefCon, ULong msgType);

	NONVIRTUAL	void					CTDoControl(ULong opCode, ULong msgType);
	NONVIRTUAL	void					CTDoKillControl(ULong msgType);
	NONVIRTUAL	void					CTGetCommEvent();
	NONVIRTUAL	void					CTDoKillGetCommEvent();
	NONVIRTUAL	NewtonErr			CTPostCommEvent(CCommToolGetEventReply& theEvent, NewtonErr result);

	NONVIRTUAL	void					CTOpenContinue();	// private, don't use

	NONVIRTUAL	NewtonErr			CTOpenStart(COptionArray* options);
	NONVIRTUAL	NewtonErr			CTOpenComplete();

	NONVIRTUAL	bool					CTClose();
	NONVIRTUAL	void					CTCloseComplete(NewtonErr result);

	NONVIRTUAL	void					CTConnectStart();
	NONVIRTUAL	void					CTConnectComplete(NewtonErr result);

	NONVIRTUAL	void					CTListenStart();
	NONVIRTUAL	void					CTListenComplete(NewtonErr result);

	NONVIRTUAL	void					CTAcceptStart();
	NONVIRTUAL	void					CTAcceptComplete(NewtonErr result);

	NONVIRTUAL	void					CTDisconnectComplete(NewtonErr result);

	NONVIRTUAL	void					CTReleaseStart();
	NONVIRTUAL	void					CTReleaseComplete(NewtonErr result);

	NONVIRTUAL	void					CTBind();
	NONVIRTUAL	void					CTUnbind();

	NONVIRTUAL	void					CTGetProtAddr();	// private, don't use

	NONVIRTUAL	ULong					CTProcessOption(COption* theOption, ULong label, ULong opcode);
	NONVIRTUAL	NewtonErr			CTAddDefaultOptions(COptionArray* options);
	NONVIRTUAL	NewtonErr			CTAddCurrentOptions(COptionArray* options);

	NONVIRTUAL	void					CTPutComplete(NewtonErr result, ULong putBytesCount);
	NONVIRTUAL	void					CTKillPutComplete(NewtonErr result);
	NONVIRTUAL	void					CTGetComplete(NewtonErr result, bool endOfFrame = false, ULong getBytesCount = 0);
	NONVIRTUAL	void					CTKillGetComplete(NewtonErr result);

	NONVIRTUAL	void					CTKillRequestComplete(CommToolRequestType requestTypeKilled, NewtonErr killResult);

	NONVIRTUAL	void					CTHoldAbort();		// private, don't use
	NONVIRTUAL	void					CTAllowAbort();	// private, don't use
	NONVIRTUAL	NewtonErr			CTStartAbort(NewtonErr abortError);
	NONVIRTUAL	bool					CTShouldAbort(ULong stateFlag, NewtonErr result);	// private, don't use

	NONVIRTUAL	void					CTTerminateConnection();
	NONVIRTUAL	void					CTTerminateComplete();

	NONVIRTUAL	NewtonErr			CTInitAsyncRPCMsg(CUAsyncMessage& asyncMsg, ULong refCon);

	NONVIRTUAL	NewtonErr			CTFlushChannel(CommToolRequestType filter, NewtonErr flushResult);

	NONVIRTUAL	void					CTSetChannelFilter(CommToolRequestType msgType, bool enable);
	NONVIRTUAL	CommToolChannelNumber	CTRequestTypeToChannelNumber(CommToolRequestType msgType);
	NONVIRTUAL	CommToolRequestType		CTChannelNumberToRequestType(CommToolChannelNumber channelNumber);

	// getters/setters
	NONVIRTUAL	ULong					CTGetToolConnectState();
	NONVIRTUAL	void					CTSetToolConnectState(ULong state);

	NONVIRTUAL	NewtonErr			CTGetAbortErr();
	NONVIRTUAL	void					CTSetAbortErr(NewtonErr err);

	NONVIRTUAL	ULong					CTGetTerminationEvent();
	NONVIRTUAL	void					CTSetTerminationEvent(ULong event);

	NONVIRTUAL	CCMOCTConnectInfo &		CTGetConnectInfo();
	NONVIRTUAL	void					CTSetConnectInfo(CCMOCTConnectInfo& info);

	NONVIRTUAL	ULong					CTGetRequestSize();
	NONVIRTUAL	UChar *				CTGetRequest();

	NONVIRTUAL	CCommToolMsgContainer	CTGetRequestListItem(UChar item);	// private, don't use
	NONVIRTUAL  ConnectParms &	  	CTGetConnectParms();	// private, don't use

	NONVIRTUAL	TCMOTransportInfo&		CTGetCMOTransportInfo();
	NONVIRTUAL	void					CTSetCMOTransportInfo(CCMOTransportInfo& info);

	NONVIRTUAL 	CommToolRequestType		CTGetRequestsToKill();	// private, don't use
	NONVIRTUAL  void					CTSetRequestsToKill(CommToolRequestType rType);	// private, don't use

	NONVIRTUAL	void					toolInit(CPCommTool* tool);	// private, don't use

	// New Methods for 2.0
	// new getters/setters
	NONVIRTUAL	ULong					CTGetReceiveMessageBufSize();		// returns the size of the buffer used by the commtool to receive rpc messages
																						// Any message sent to the tool which exceeds this size will be truncated.
	NONVIRTUAL	CUPort *				CTGetToolPort();						// returns a pointer to the tool's port object

	NONVIRTUAL	CCommToolOptionInfo&	CTGetControlOptionsInfo();		// returns the option info for the current control request
	NONVIRTUAL	CCommToolOptionInfo&	CTGetGetBytesOptionsInfo();	// returns the option info for the current get bytes request
	NONVIRTUAL	CCommToolOptionInfo&	CTGetPutBytesOptionsInfo();	// returns the option info for the current put bytes request

	NONVIRTUAL	CBufferList *		CTGetCurPutData();				// returns a pointer to the current put request buffer list
	NONVIRTUAL	bool					CTGetCurPutFrameData();			// returns true if current put request is framed
	NONVIRTUAL	bool					CTGetCurPutEndOfFrame();		// returns true if current framed put request is end of frame

	NONVIRTUAL	CBufferList *		CTGetCurGetData();				// returns a pointer to the current get request buffer list
	NONVIRTUAL	bool					CTGetCurGetFrameData();			// returns true if current get request is framed
	NONVIRTUAL	bool					CTGetCurGetNonBlocking();		// returns true if current get request is non blocking (ie has a threshold)
	NONVIRTUAL	Size					CTGetCurGetThreshold();			// returns the value of threshold for the current nonblocking get request

	NONVIRTUAL	bool					CTGetPassiveClaim();				// returns true if client asked for passive claim of tool resources
	NONVIRTUAL	void					CTSetPassiveClaim(bool passiveClaim); // Set the value of passive claim

	NONVIRTUAL	bool					CTGetPassiveState();				// returns true if in passive state (resources passively claimed, and willing to give up resources)
	NONVIRTUAL	void					CTSetPassiveState(bool passiveState); // Set the value of passive claim

	NONVIRTUAL	bool					CTGetWaitingForResNotify();	// returns true if tool has passively claimed resources, and is waiting for notification of resource ownership
	NONVIRTUAL	void					CTSetWaitingForResNotify(bool waitingForResNotify); // Set the value of passive claim

	NONVIRTUAL	ULong					CTGetCurRequestOpCode();		// returns the opcode of the current control request

	// new base class methods
	NONVIRTUAL 	void					CTBindStart();
	NONVIRTUAL 	void					CTBindComplete(NewtonErr result);

	NONVIRTUAL 	void					CTUnbindStart();
	NONVIRTUAL 	void					CTUnbindComplete(NewtonErr result);

	NONVIRTUAL 	ULong					CTProcessOptionStart(COption* theOption, ULong label, ULong opcode);
	NONVIRTUAL 	void					CTProcessOptionComplete(ULong optResult);

	NONVIRTUAL 	ULong					CTProcessPutBytesOptionStart(COption* theOption, ULong label, ULong opcode);
	NONVIRTUAL 	void 					CTProcessPutBytesOptionComplete(ULong optResult);

	NONVIRTUAL 	ULong 				CTProcessGetBytesOptionStart(COption* theOption, ULong label, ULong opcode);
	NONVIRTUAL 	void 					CTProcessGetBytesOptionComplete(ULong optResult);

	NONVIRTUAL 	void					CTResArbRelease(UChar* resName, UChar* resType);
	NONVIRTUAL 	void					CTResArbReleaseStart(UChar* resName, UChar* resType);
	NONVIRTUAL 	void					CTResArbReleaseComplete(NewtonErr result);

	NONVIRTUAL 	void					CTResArbClaimNotification(UChar* resName, UChar* resType);

	NONVIRTUAL void					CTHandleInternalEvent();

	// new implementation methods
					void					bindStart();
					void					bindComplete(NewtonErr result);

					void					unbindStart();
					void					unbindComplete(NewtonErr result);

					NewtonErr			processOptionStart(COption* theOption, ULong label, ULong opcode);
					void					processOptionComplete(ULong optResult);

					ULong					processPutBytesOptionStart(COption* theOption, ULong label, ULong opcode);
					void 					processPutBytesOptionComplete(ULong optResult);

					ULong 				processGetBytesOptionStart(COption* theOption, ULong label, ULong opcode);
					void 					processGetBytesOptionComplete(ULong optResult);

					void					resArbRelease(UChar* resName, UChar* resType);
					void					resArbReleaseStart(UChar* resName, UChar* resType);
					void					resArbReleaseComplete(NewtonErr result);

					void					resArbClaimNotification(UChar* resName, UChar* resType);

					CUPort *				forwardOptions();

					void					handleInternalEvent();

//private:
	CPCommTool *	fCommTool;
};


NewtonErr StartCommToolProtocol(COptionArray* options, ULong serviceId, CServiceInfo* serviceInfo, CCMService* service, CCommToolProtocol* ctProtocol);

// 2.0:	New version of StartCommToolProtocol accepts heapSize parameter.
// 		HeapSize is used to set the size of the heap for the CCommToolProtocol implementation.
NewtonErr StartCommToolProtocol(COptionArray* options, ULong serviceId, CServiceInfo* serviceInfo, CCMService* service, CCommToolProtocol* ctProtocol, size_t heapSize);


#endif	/* __COMMTOOLPROTOCOLV2_H */
