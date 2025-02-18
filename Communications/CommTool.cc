/*
	File:		CommTool.cc

	Contains:	Comm tool implementation.

	Written by:	Newton Research Group, 2009.
*/

#include "CommTool.h"
#include "CommManagerInterface.h"
#include "CMService.h"
#include "MemObject.h"


/* -------------------------------------------------------------------------------
	CCommToolProtocol stuff.
	Should maybe have its own file.
------------------------------------------------------------------------------- */

CCMOIdleTimer::CCMOIdleTimer()
{}

CCMOListenTimer::CCMOListenTimer()
{}

CCMOCTConnectInfo::CCMOCTConnectInfo()
{}

CCMOToolSpecificOptions::CCMOToolSpecificOptions()
{}

CCMOPassiveClaim::CCMOPassiveClaim()
{}

CCMOPassiveState::CCMOPassiveState()
{}


#pragma mark -
/* -------------------------------------------------------------------------------
	Initialisation.
------------------------------------------------------------------------------- */

NewtonErr
StartCommTool(CCommTool * inTool, ULong inId, CServiceInfo * info)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = inTool->startTask(true, false, 0, kCommToolStackSize, kCommToolTaskPriority, inId))
		CUPort port;
		XFAIL(err = ServiceToPort(inId, &port, inTool->getChildTaskId()))
		info->setPortId(port);
		info->setServiceId(inId);
	}
	XENDTRY;
	return err;
}


NewtonErr
OpenCommTool(ULong inId, COptionArray * inOptions, CCMService * inService)
{
	NewtonErr err;
	XTRY
	{
		CCommToolOpenRequest * request = new CCommToolOpenRequest;
		XFAILIF(request == NULL, err = kOSErrNoMemory;)
		CCommToolOpenReply * reply = new CCommToolOpenReply;
		XFAILIF(reply == NULL, err = kOSErrNoMemory;)
		request->fOptions = inOptions;
		request->fOptionCount = inOptions->count();
		request->fOutside = false;

		CAsyncServiceMessage * msg = new CAsyncServiceMessage;
		XFAILIF(msg == NULL, err = kOSErrNoMemory;)
		XFAIL(err = msg->init(inService))

		CUPort commPort(inId);
		err = msg->send(&commPort, &request, sizeof(request), &reply, sizeof(reply), 4);

		if (err == noErr)
			err = 1;			// huh?
	}
	XENDTRY;
	return err;
}


#pragma mark -
/* -------------------------------------------------------------------------------
	C C o m m T o o l   E v e n t s
------------------------------------------------------------------------------- */

CCommToolBindRequest::CCommToolBindRequest()
{
	fOpCode = kCommToolBind;
	fReserved1 = 0;
	fReserved2 = 0;
	fOutside = false;
}


CCommToolBindReply::CCommToolBindReply()
{
	fSize = sizeof(CCommToolBindReply);
	fWastedULong = 0;
}


CCommToolConnectRequest::CCommToolConnectRequest()
{
	fOpCode = kCommToolConnect;
	fReserved1 = 0;
	fReserved2 = 0;
	fOptions = NULL;
	fOptionCount = 0;
	fData = NULL;
	fSequence = 0;
	fOutside = false;
}


CCommToolConnectReply::CCommToolConnectReply()
{
	fSize = sizeof(CCommToolConnectReply);
	fSequence = 0;
}


CCommToolOpenRequest::CCommToolOpenRequest()
{
	fOpCode = kCommToolOpen;
	fOptions = NULL;
	fOptionCount = 0;
	fOutside = false;
}


CCommToolOpenReply::CCommToolOpenReply()
{
	fSize = sizeof(CCommToolOpenReply);
	fPortId = 0;
}


CCommToolPutRequest::CCommToolPutRequest()
{
	fData = NULL;
	fValidCount = 0;
	fOutside = false;
	fFrameData = false;
	fEndOfFrame = false;
}


CCommToolPutReply::CCommToolPutReply()
{
	fSize = sizeof(CCommToolPutReply);
	fPutBytesCount = 0;
}


CCommToolGetRequest::CCommToolGetRequest()
{
	fData = NULL;
	fThreshold = 0;
	fNonBlocking = false;
	fFrameData = false;
	fOutside = false;
}


CCommToolGetReply::CCommToolGetReply()
{
	fSize = sizeof(CCommToolGetReply);
	fEndOfFrame = false;
	fGetBytesCount = 0;
}


CCommToolDisconnectRequest::CCommToolDisconnectRequest()
{
	fOpCode = kCommToolDisconnect;
	fDisconnectData = NULL;
	fSequence = 0;
	fReason = 0;
	fOutside = false;
}

#pragma mark -

CCommToolOptionMgmtRequest::CCommToolOptionMgmtRequest()
{
	fOpCode = kCommToolOptionMgmt;

	fOptions = NULL;
	fOptionCount = 0;
	fRequestOpCode = 0;
	fOutside = false;
	fCopyBack = false;
}


CCommToolGetProtAddrRequest::CCommToolGetProtAddrRequest()
{
	fOpCode = kCommToolGetProtAddr;
	fBoundAddr = NULL;
	fPeerAddr = NULL;
	fOutside = false;
}


CCommToolGetEventReply::CCommToolGetEventReply()
{
	fSize = sizeof(CCommToolOpenReply);
	fEventCode = 0;
	fEventTime = 0;
	fEventData = 0;
}


CCommToolKillRequest::CCommToolKillRequest()
{
	fRequestsToKill = (CommToolRequestType)(kCommToolRequestTypeGet | kCommToolRequestTypePut | kCommToolRequestTypeControl | kCommToolRequestTypeGetEvent);
}


CCommToolStatusRequest::CCommToolStatusRequest()
{
	fStatusInfoId = 0;
}


CCommToolMsgContainer::CCommToolMsgContainer()
{
	fRequestPending = false;
	fRequestMsgSize = 0;
}


CCommToolOptionInfo::CCommToolOptionInfo()
{
	fOptionsState = 0;
	fChannelNum = kCommToolControlChannel;
	fOptions = NULL;
	fCurOptPtr = NULL;
	fOptionsIterator = NULL;
}


#pragma mark -
#pragma mark CCommTool
/* -------------------------------------------------------------------------------
	C C o m m T o o l
------------------------------------------------------------------------------- */

CCommTool::CCommTool(ULong inName)
{
	fHeapSize = kCommToolDefaultHeapSize;
	fName = inName;
	fHeap = NULL;
}


CCommTool::CCommTool(ULong inName, size_t inHeapSize)
{
	fHeapSize = inHeapSize;
	fName = inName;
	fHeap = NULL;
}


CCommTool::~CCommTool()
{
	if (fHeap)
	{
		SetHeap(fSavedHeap);
		DestroyVMHeap(fHeap);
		fHeap = NULL;
	}
}


size_t
CCommTool::getSizeOf(void) const
{
	return sizeof(CCommTool);
}


NewtonErr
CCommTool::taskConstructor(void)
{
	NewtonErr err = noErr;

	XTRY
	{
		// ivar zeroing
		f208 = (CommToolRequestType)-1;
		fIsClosed = false;
		f201 = false;
		fState = 0;
		f218 = NULL;
		f21C = NULL;

		f174 = 0;
		f1D0 = false;
		f1D1 = false;
		f1D2 = false;
		fOptionRequest = NULL;
		fOptionMessage = NULL;
		fOptionReply = NULL;

		fControlOptionInfo.fChannelNum = kCommToolControlChannel;
		fPutOptionInfo.fChannelNum = kCommToolPutChannel;
		fGetOptionInfo.fChannelNum = kCommToolGetChannel;

		f13C = NULL;
		fTimeout = kNoTimeout;
		fTimeoutRemaining = kNoTimeout;

		// create heap
		fSavedHeap = GetHeap();
		XFAIL(err = NewVMHeap(0, fHeapSize, &fHeap, 0))
		SetHeap(fHeap);

		// create port
		XFAIL(err = createPort(fName, f8C))

		// create buffers
		f218 = new CBufferList;
		XFAILIF(f218 == NULL, err = MemError();)
		f21C = new CBufferList;
		XFAILIF(f21C == NULL, err = MemError();)
		XFAIL(f218->init(false))
		XFAIL(f21C->init(false))
		f218->insert(&fBufferSegment_1);
		f21C->insert(&fBufferSegment_2);
	}
	XENDTRY;

	XDOFAIL(err)
	{
		taskDestructor();
	}
	XENDFAIL;

	return err;
}


void
CCommTool::taskDestructor(void)
{
	unregisterPort();
	if (f218)
		delete f218, f218 = NULL;
	if (f21C)
		delete f21C, f21C = NULL;
}


void
CCommTool::taskMain(void)
{
	NewtonErr err;
	ULong msgType;
	CUMsgToken msgToken;

	// main event despatch loop
	while (!fIsClosed)
	{
		bool isTimedOut = false;
		CTime startTime = GetGlobalTime();
		err = f8C.receive(&f48, &f4C, sizeof(f4C), &msgToken, &msgType, fTimeoutRemaining, f208);
		if (err == kOSErrMessageTimedOut)
		{
			handleTimerTick();
			fTimeoutRemaining = fTimeout;
			isTimedOut = true;
		}
		else if (err == noErr)
		{
			CommToolRequestType reqType = (CommToolRequestType)(msgType & kMsgType_ReservedMask);	// r5
			if (FLAGTEST(msgType, kMsgType_CollectedSender))
			{
				OpaqueRef refCon;
				if (msgToken.getUserRefCon(&refCon) == noErr)	// ULong -> OpaqueRef?
					handleReply(refCon, msgType);
			}
			else
			{
				if (reqType > 0 && reqType <= 0x40)	//kCommToolRequestTypeResArb
				{
					ArrayIndex channelNo = requestTypeToChannelNumber(reqType);
					CCommToolMsgContainer * msg = &f94[channelNo];
					msg->fRequestMsgSize = f48;
					msg->fMsgToken = msgToken;
					msg->fRequestPending = true;
					setChannelFilter(reqType, false);
					switch (channelNo)
					{
					case kCommToolGetChannel:
						prepGetRequest();
						break;
					case kCommToolPutChannel:
						prepPutRequest();
						break;
					case kCommToolControlChannel:
						prepControlRequest(reqType);
						break;
					case kCommToolGetEventChannel:
						getCommEvent();
						break;
					case kCommToolKillChannel:
						prepKillRequest();
						break;
					case kCommToolStatusChannel:
						doStatus(((CCommToolControlRequest *)f4C)->fOpCode, reqType);
						break;
					case kCommToolResArbChannel:
						prepResArbRequest();
						break;
					}
				}
			}
		}
		if (fTimeout != kNoTimeout && !isTimedOut)
		{
			// update timeout remaining until next timer tick
			CTime elapsedTime = GetGlobalTime() - startTime;
			if (fTimeoutRemaining >= elapsedTime)
				fTimeoutRemaining -= elapsedTime;
			else
			{
				handleTimerTick();
				fTimeoutRemaining = fTimeout;
			}
		}
		handleInternalEvent();
	}
	
}


#pragma mark Port


NewtonErr
CCommTool::createPort(ULong inId, CUPort & outPort)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = outPort.init())
		
		MAKE_ID_STR(inId,type);

		char name[16];		// is actually the task id
		sprintf(name, "%d", getChildTaskId());

		CUNameServer ns;
		XFAIL(err = ns.registerName(name, type, 0, 0))

		f201 = true;
	}
	XENDTRY;
	return err;
}


NewtonErr
CCommTool::getToolPort(ULong inId, CUPort & outPort)
{
	NewtonErr err;
	XTRY
	{
		OpaqueRef thing, spec;
		MAKE_ID_STR(inId,type);

		CUNameServer ns;
		XFAIL(err = ns.lookup(type, type, &thing, &spec))

		outPort = (ObjectId)thing;
	}
	XENDTRY;
	return err;
}


void
CCommTool::unregisterPort(void)
{
	if (f201)
	{
		MAKE_ID_STR(fName,type);

		char name[16];		// is actually the task id
		sprintf(name, "%d", getChildTaskId());

		CUNameServer ns;
		ns.unregisterName(name, type);

		f201 = false;
	}
}

#pragma mark Filter

void
CCommTool::setChannelFilter(CommToolRequestType inReqType, bool inDoSet)
{
	if (inDoSet)
		f208 = (CommToolRequestType)(f208 | inReqType);
	else
		f208 = (CommToolRequestType)(f208 & ~inReqType);
}


ArrayIndex
CCommTool::requestTypeToChannelNumber(CommToolRequestType inReqType)
{
	ArrayIndex channel = 0;
	int reqType = inReqType;
	while ((reqType >>= 1) != 0)
		channel++;
	return channel;
}

#pragma mark Events

NewtonErr
CCommTool::initAsyncRPCMsg(CUAsyncMessage & ioMsg, ULong inRefCon)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = ioMsg.init(true))
		XFAIL(err = ioMsg.setUserRefCon(inRefCon))
		err = ioMsg.setCollectorPort(f8C);
	}
	XENDTRY;
	return err;
}


void
CCommTool::handleInternalEvent(void)
{ /* this really does nothing */ }


NewtonErr
CCommTool::flushChannel(CommToolRequestType inType, NewtonErr inResult)
{
	size_t size;
	CUMsgToken msg;
	CCommToolControlRequest request;
	CCommToolReply reply;
	reply.fResult = inResult;
	for ( ; ; )
	{
		XFAIL(f8C.receive(&size, &request, sizeof(request), &msg, NULL, kNoTimeout, inType, true))	// okay, not in XTRY/ENDTRY but does the right thing
		if (msg.getReplyId())
			msg.replyRPC(&reply, sizeof(reply));
	}
	return noErr;
}


void
CCommTool::completeRequest(CUMsgToken& inMsg, NewtonErr inResult)
{
	if (inMsg.getReplyId() != kNoId)
	{
		CCommToolReply reply;
		reply.fResult = inResult;
		inMsg.replyRPC(&reply, reply.fSize);
	}
}


void
CCommTool::completeRequest(CUMsgToken& inMsg, NewtonErr inResult, CCommToolReply& ioReply)
{
	if (inMsg.getReplyId() != kNoId)
	{
		ioReply.fResult = inResult;
		inMsg.replyRPC(&ioReply, ioReply.fSize);
	}
}


void
CCommTool::completeRequest(CommToolChannelNumber inChannel, NewtonErr inResult)
{
	if (f94[inChannel].fRequestPending)
	{
		f94[inChannel].fRequestPending = false;
		setChannelFilter((CommToolRequestType)(1 << inChannel), true);
		CUMsgToken * msg = &f94[inChannel].fMsgToken;
		if (msg->getReplyId())
		{
			CCommToolReply reply;
			reply.fResult = inResult;
			msg->replyRPC(&reply, reply.fSize);
		}
	}
}


void
CCommTool::completeRequest(CommToolChannelNumber inChannel, NewtonErr inResult, CCommToolReply& ioReply)
{
	if (f94[inChannel].fRequestPending)
	{
		f94[inChannel].fRequestPending = false;
		setChannelFilter((CommToolRequestType)(1 << inChannel), true);
		CUMsgToken * msg = &f94[inChannel].fMsgToken;
		if (msg->getReplyId())
		{
			ioReply.fResult = inResult;
			msg->replyRPC(&ioReply, ioReply.fSize);
		}
	}
}


void
CCommTool::handleRequest(CUMsgToken & inMsg, ULong inMsgType)
{
	inMsg.replyRPC(NULL, 0, kOSErrBadParameters);
}


void
CCommTool::handleReply(ULong inUserRefCon, ULong inMsgType)
{
	if (inUserRefCon == kToolMsgForwardOpt)
	{
		fControlOptionInfo.fOptionsState &= ~0x0A;
		processOptionsComplete(fOptionReply->fResult, &fControlOptionInfo);
	}
}


void
CCommTool::handleTimerTick(void)
{ /* this really does nothing */ }


void
CCommTool::prepGetRequest(void)
{
	NewtonErr err;
	XTRY
	{
		XFAILNOT(FLAGTEST(fState, kToolStateConnected), err = kCommErrNotConnected;)
		XFAILIF(FLAGTEST(fState, kToolStateRelease), err = kCommErrReleasingConnection;)
		CCommToolGetRequest * req = (CCommToolGetRequest *)f4C;
		if (req->fOutside)
		{
			// replace ObjectId with CBufferList
			ObjectId sharedBufMem = (ObjectId)(long)req->fData;
			XFAIL(err = fBufferSegment_1.init(sharedBufMem, 0, -1))
			f218->reset();
			req->fData = f218;
		}
		if (f94[kCommToolGetChannel].fRequestMsgSize == sizeof(CCommToolGetRequest))
		{
			fGetOptionInfo.fOptionsState = 0;
			if (req->fOutside)
			{
				fGetOptionInfo.fOptionsState = kOptionsOutside;
				fGetOptionInfo.fOptionCount = req->fOptionCount;
			}
			fGetOptionInfo.fChannelNum = kCommToolGetChannel;
			fGetOptionInfo.fOptions = req->fOptions;
		}
		f1C8 = req->fData;
		f1C6 = req->fNonBlocking;
		f1C7 = req->fFrameData;
		f1CC = req->fThreshold;
		processOptions(&fGetOptionInfo);
		return;
	}
	XENDTRY;
	// if we get here there was an error
	completeRequest(kCommToolGetChannel, err);
}


void
CCommTool::getOptionsComplete(NewtonErr inResult)
{
	if (inResult == noErr)
	{
		if (f1C7)
			getFramedBytes(f1C8);
		else if (f1C6)
			getBytesImmediate(f1C8, f1CC);
		else
			getBytes(f1C8);
	}

	else
		getComplete(inResult, 0, 0);
}


void
CCommTool::getBytesImmediate(CBufferList * inQueue, size_t inSize)
{
	getBytes(inQueue);
}


void
CCommTool::getComplete(NewtonErr inResult, bool inEndOfFrame, size_t inGetBytesCount)
{}


void
CCommTool::killGetComplete(NewtonErr inResult)
{}



void
CCommTool::prepPutRequest(void)
{
	NewtonErr err;
	XTRY
	{
		XFAILNOT(FLAGTEST(fState, kToolStateConnected), err = kCommErrNotConnected;)
		XFAILIF(FLAGTEST(fState, kToolStateRelease), err = kCommErrReleasingConnection;)
		CCommToolPutRequest * req = (CCommToolPutRequest *)f4C;
		if (req->fOutside)
		{
			// replace ObjectId with CBufferList
			ObjectId sharedBufMem = (ObjectId)(long)req->fData;
			XFAIL(err = fBufferSegment_2.init(sharedBufMem, 0, req->fValidCount))
			f21C->seek(0, kSeekFromBeginning);
			req->fData = f21C;
		}
		if (f94[kCommToolPutChannel].fRequestMsgSize == sizeof(CCommToolPutRequest))
		{
			fPutOptionInfo.fOptionsState = 0;
			if (req->fOutside)
			{
				fPutOptionInfo.fOptionsState = kOptionsOutside;
				fPutOptionInfo.fOptionCount = req->fOptionCount;
			}
			fPutOptionInfo.fChannelNum = kCommToolPutChannel;
			fPutOptionInfo.fOptions = req->fOptions;
		}
		f1C0 = req->fData;
		f1C4 = req->fFrameData;
		f1C5 = req->fEndOfFrame;
		processOptions(&fPutOptionInfo);
		return;
	}
	XENDTRY;
	// if we get here there was an error
	completeRequest(kCommToolPutChannel, err);
}


void
CCommTool::putOptionsComplete(NewtonErr inResult)
{
	if (inResult == noErr)
	{
		if (f1C4)
			putFramedBytes(f1C0, f1C5);
		else
			putBytes(f1C0);
	}

	else
		putComplete(inResult, 0);
}


void
CCommTool::putComplete(NewtonErr inResult, size_t inPutBytesCount)
{}


void
CCommTool::killPutComplete(NewtonErr inResult)
{}


void
CCommTool::prepControlRequest(ULong inMsgType)
{
	CCommToolControlRequest * request = (CCommToolControlRequest *)f4C;
	doControl(request->fOpCode, inMsgType);
}


void
CCommTool::prepKillRequest(void)
{
	NewtonErr err;
	XTRY
	{
		CCommToolKillRequest * req = (CCommToolKillRequest *)f4C;
		f174 = req->fRequestsToKill;
		f1FC = noErr;

		if (FLAGTEST(f174, kCommToolRequestTypeGet))
		{
			XFAIL(err = flushChannel(kCommToolRequestTypeGet, kCommErrRequestCanceled))
			if (f94[kCommToolGetChannel].fRequestPending)
				killGet();
			else
				FLAGCLEAR(f174, kCommToolRequestTypeGet);
		}

		if (FLAGTEST(f174, kCommToolRequestTypePut))
		{
			XFAIL(err = flushChannel(kCommToolRequestTypePut, kCommErrRequestCanceled))
			if (f94[kCommToolPutChannel].fRequestPending)
				killPut();
			else
				FLAGCLEAR(f174, kCommToolRequestTypePut);
		}

		if (FLAGTEST(f174, kCommToolRequestTypeControl))
		{
			XFAIL(err = flushChannel(kCommToolRequestTypeControl, kCommErrRequestCanceled))
			if (f94[kCommToolControlChannel].fRequestPending)
			{
				CUPort * port = forwardOptions();
				if (port != NULL && FLAGTEST(fControlOptionInfo.fOptionsState, kOptionsInProgress) && FLAGTEST(fControlOptionInfo.fOptionsState, kOptionsForwardReqActive))
				{
					size_t size;
					CCommToolKillRequest request;
					CCommToolReply reply;
					request.fRequestsToKill = kCommToolRequestTypeControl;
					XFAIL(err = port->sendRPC(&size, &request, sizeof(request), &reply, sizeof(reply), kNoTimeout, 16))
				}
				doKillControl(kCommToolRequestTypeKill);
			}
			else
				FLAGCLEAR(f174, kCommToolRequestTypeControl);
		}

		if (FLAGTEST(f174, kCommToolRequestTypeGetEvent))
		{
			XFAIL(err = flushChannel(kCommToolRequestTypeGetEvent, kCommErrRequestCanceled))
			if (f94[kCommToolGetEventChannel].fRequestPending)
				doKillGetCommEvent();
			else
				FLAGCLEAR(f174, kCommToolRequestTypeGetEvent);
		}

		XFAILIF(f174 == 0, err = noErr;)
		return;
	}
	XENDTRY;
	// if we get here there was an error
	completeRequest(kCommToolKillChannel, err);
}


void
CCommTool::killRequestComplete(CommToolRequestType inReq, NewtonErr inResult)
{
	f174 &= ~inReq;
	if (inResult)
	{
		if (f1FC == noErr)
			f1FC = inResult;
	}
	if (f174 == 0)
		completeRequest(kCommToolKillChannel, f1FC);
}


void
CCommTool::prepResArbRequest(void)
{
	CCommToolResArbRequest * request = (CCommToolResArbRequest *)f4C;
	if (request->fOpCode == kCommToolResArbRelease)
		resArbRelease(request->fResNamePtr, request->fResTypePtr);
	else if (request->fOpCode == kCommToolResArbClaimNotification)
		resArbClaimNotification(request->fResNamePtr, request->fResTypePtr);
}

void
CCommTool::resArbRelease(unsigned char*, unsigned char*)
{}

void
CCommTool::resArbReleaseStart(unsigned char*, unsigned char*)
{}

void
CCommTool::resArbReleaseComplete(NewtonErr)
{}

void
CCommTool::resArbClaimNotification(unsigned char*, unsigned char*)
{}


void
CCommTool::doControl(ULong inOpCode, ULong inMsgType)
{
	f1D4 = inOpCode;

	switch (inOpCode)
	{
	case kCommToolOpen:
		open();
		break;
	case kCommToolClose:
		close();
		break;
	case kCommToolConnect:
		connect();
		break;
	case kCommToolListen:
		listen();
		break;
	case kCommToolAccept:
		accept();
		break;
	case kCommToolDisconnect:
		disconnect();
		break;
	case kCommToolRelease:
		release();
		break;
	case kCommToolBind:
		bind();
		break;
	case kCommToolUnbind:
		unbind();
		break;
	case kCommToolOptionMgmt:
		optionMgmt((CCommToolOptionMgmtRequest *)f4C);
		break;
	case kCommToolGetProtAddr:
		getProtAddr();
		break;
	default:
		completeRequest(kCommToolControlChannel, kCommErrBadCommand);
	}
}


void
CCommTool::doStatus(ULong inOpCode, ULong inMsgType)
{
	completeRequest(kCommToolStatusChannel, (inOpCode == kCommToolGetConnectState) ? getConnectState() : kCommErrBadCommand);
}


void
CCommTool::doKillControl(ULong inArg)
{
	if ((fState & (kToolStateConnecting | kToolStateRelease)) == 0)
		killRequestComplete(kCommToolRequestTypeControl, kCommErrBadCommand);
	else
	{
		if (f28 == 0)
			f28 = 2;
		startAbort(kCommErrConnectionAborted);
	}
}


void
CCommTool::doKillGetCommEvent(void)
{
	CCommToolGetEventReply reply;
	completeRequest(kCommToolGetEventChannel, kCommErrRequestCanceled, reply);
	killRequestComplete(kCommToolRequestTypeGetEvent, noErr);
}


void
CCommTool::getCommEvent(void)
{
	if (fGetEventReply.fResult != kCommErrNoEventPending)
	{
		postCommEvent(fGetEventReply, noErr);
		fGetEventReply.fResult = kCommErrNoEventPending;
	}
}


NewtonErr
CCommTool::postCommEvent(CCommToolGetEventReply & ioReply, NewtonErr inResult)
{
	if (!f94[kCommToolControlChannel].fRequestPending)
		return kCommErrNoRequestPending;

	completeRequest(kCommToolControlChannel, inResult, ioReply);
	return noErr;
}

#pragma mark Open

void
CCommTool::open(void)
{
	CCommToolOpenRequest * request = (CCommToolOpenRequest *)f4C;
	processControlOptions(request->fOutside, request->fOptions, request->fOptionCount);
}


void
CCommTool::openOptionsComplete(NewtonErr inResult)
{
	NewtonErr result;
	CCommToolOpenReply reply;
	if ((result = inResult) || (result = openStart(fControlOptionInfo.fOptions)))
	{
		processOptionsCleanUp(result, &fControlOptionInfo);
		completeRequest(kCommToolControlChannel, result);
		close();
	}

	else
		openContinue();
}


void
CCommTool::openContinue(void)
{
	NewtonErr err, result;
	CCommToolOpenReply reply;
	if ((result = openComplete()) != 1)		// huh?
	{
		err = processOptionsCleanUp(result, &fControlOptionInfo);
		if (result == noErr)
			result = err;
		reply.fPortId = f8C;
		completeRequest(kCommToolControlChannel, result, reply);
		if (result != noErr)
			close();
	}
}


NewtonErr
CCommTool::openStart(COptionArray * inOptions)
{
	return noErr;
}


NewtonErr
CCommTool::openComplete(void)
{
	return noErr;
}

#pragma mark Close

bool
CCommTool::close(void)
{
	fState |= kToolStateClosing;

	if (fState & (kToolStateConnecting | kToolStateConnected))
	{
		if (f28 == 0)
			f28 = 2;
		startAbort(kCommErrConnectionAborted);
		return false;
	}

	for (int req = kCommToolRequestTypeGet; req <= kCommToolRequestTypeKill; req <<= 1)
		flushChannel((CommToolRequestType)req, kCommErrToolBusy);
	closeComplete(noErr);
	return true;
}


void
CCommTool::closeComplete(NewtonErr inResult)
{
	fIsClosed = true;
	fState &= ~kToolStateClosing;
	unregisterPort();
	completeRequest(kCommToolControlChannel, inResult);
}

#pragma mark Connect

NewtonErr
CCommTool::importConnectPB(CCommToolConnectRequest * inReq)
{
	NewtonErr err = noErr;

	f140 = inReq->fSequence;
	f144 = inReq->fOutside;
	if (!inReq->fOutside)
		f13C = inReq->fData;
	else if (inReq->fData)
	{
		XTRY
		{
			CShadowBufferSegment * buf = new CShadowBufferSegment;
			XFAILIF(buf == NULL, err = kOSErrNoMemory;)
			ObjectId sharedBufMem = (ObjectId)(long)inReq->fData;
			XFAIL(err = buf->init(sharedBufMem, 0, -1))
			f13C = (CBufferSegment *)buf;
		}
		XENDTRY;
	}
	return err;
}


NewtonErr
CCommTool::copyBackConnectPB(NewtonErr inResult)
{
	if (f144 && f13C)
		delete f13C;
	f13C = NULL;
	return processOptionsCleanUp(inResult, &fControlOptionInfo);
}


NewtonErr
CCommTool::connectCheck(void)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILIF((fState & (kToolStateConnecting | kToolStateConnected)), err = kCommErrAlreadyConnected;)
		fState |= kToolStateConnecting;
		fTermFlag = 0;
		fTermPhase = 0;
		f28 = 0;
		fAbortLock = 0;
		fGetEventReply.fResult = noErr;
	}
	XENDTRY;
	return err;
}


void
CCommTool::connect(void)
{
	NewtonErr err;
	XTRY
	{
		CCommToolConnectRequest * request = (CCommToolConnectRequest *)f4C;
		XFAILIF(err = connectCheck(), completeRequest(kCommToolControlChannel, err);)
		XFAILIF(err = importConnectPB(request), connectComplete(err);)
		processControlOptions(fErrStatus, request->fOptions, request->fOptionCount);
	}
	XENDTRY;
}


void
CCommTool::connectOptionsComplete(NewtonErr inResult)
{
	XTRY
	{
		XFAILIF(inResult, connectComplete(inResult);)
		connectStart();
	}
	XENDTRY;
}


void
CCommTool::connectStart(void)
{
	connectComplete(noErr);
}


void
CCommTool::connectComplete(NewtonErr inResult)
{
	fState &= ~kToolStateConnecting;
	if (inResult == noErr)
	{
		fState |= kToolStateConnected;

		COption * connOpt;
		if (fControlOptionInfo.fOptions != NULL && (connOpt = fControlOptionInfo.fOptionsIterator->findOption(kCMOCTConnectInfo)) != NULL)
		{
			if (connOpt->getOpCode() == opGetCurrent)
			{
				connOpt->copyDataFrom(fPutOptionInfo.fCurOptPtr);
				connOpt->setOpCodeResult(opSuccess);
			}
			else
				connOpt->setOpCodeResult(opFailure);
			connOpt->setProcessed();
		}
	}

	NewtonErr err;
	CCommToolConnectReply reply;
	reply.fSequence = f140;
	err = copyBackConnectPB(inResult);
	completeRequest(kCommToolControlChannel, err, reply);
}


NewtonErr
CCommTool::getConnectState(void)
{
	NewtonErr err = noErr;
	XTRY
	{
		CMemObject * mem = new CMemObject;
		XFAILIF(mem == NULL, err = kOSErrNoMemory;)
		CCommToolConnectRequest * request = (CCommToolConnectRequest *)f4C;
		XFAIL(request->fReserved1 == 0)
		XFAIL(err = mem->make(request->fReserved1))
		err = mem->copyTo((Ptr)&fState, sizeof(fState));
	}
	XENDTRY;
	return err;
}

#pragma mark Listen

void
CCommTool::listen(void)
{
	NewtonErr err;
	XTRY
	{
		CCommToolConnectRequest * request = (CCommToolConnectRequest *)f4C;
		XFAILIF(err = connectCheck(), completeRequest(kCommToolControlChannel, err);)
		XFAILIF(err = importConnectPB(request), listenComplete(err);)
		processControlOptions(fErrStatus, request->fOptions, request->fOptionCount);
	}
	XENDTRY;
}


void
CCommTool::listenOptionsComplete(NewtonErr inResult)
{
	XTRY
	{
		XFAILIF(inResult, listenComplete(inResult);)
		fState |= kToolStateListenMode;
		listenStart();
	}
	XENDTRY;
}


void
CCommTool::listenStart(void)
{
	listenComplete(noErr);
}


void
CCommTool::listenComplete(NewtonErr inResult)
{
	if (inResult)
		fState &= ~(kToolStateListenMode | kToolStateConnecting);

	NewtonErr err;
	CCommToolConnectReply reply;
	reply.fSequence = f140;
	err = copyBackConnectPB(inResult);
	completeRequest(kCommToolControlChannel, err, reply);
}

#pragma mark Accept

void
CCommTool::accept(void)
{
	NewtonErr err;
	XTRY
	{
		CCommToolConnectRequest * request = (CCommToolConnectRequest *)f4C;
		XFAILNOT((fState & kToolStateListenMode), completeRequest(kCommToolControlChannel, kCommErrNotConnected);)
		XFAILIF(err = importConnectPB(request), acceptComplete(err);)
		processControlOptions(fErrStatus, request->fOptions, request->fOptionCount);
	}
	XENDTRY;
}


void
CCommTool::acceptOptionsComplete(NewtonErr inResult)
{
	XTRY
	{
		XFAILIF(inResult, acceptComplete(inResult);)
		acceptStart();
	}
	XENDTRY;
}


void
CCommTool::acceptStart(void)
{
	acceptComplete(noErr);
}


void
CCommTool::acceptComplete(NewtonErr inResult)
{
	fState &= ~kToolStateListenMode;
	connectComplete(inResult);
}

#pragma mark Disconnect

void
CCommTool::disconnect(void)
{
	XTRY
	{
		XFAILNOT((fState & (kToolStateConnecting | kToolStateConnected)), disconnectComplete(kCommErrNotConnected);)
		fState |= kToolStateDisconnectReq;
		if (f28 == 0)
			f28 = 2;
		startAbort(kCommErrConnectionAborted);
	}
	XENDTRY;
}


void
CCommTool::disconnectComplete(NewtonErr inResult)
{
	fState &= ~kToolStateDisconnectReq;
	completeRequest(kCommToolControlChannel, inResult);
}

#pragma mark Release

void
CCommTool::release(void)
{
	XTRY
	{
		XFAILNOT((fState & (kToolStateConnecting | kToolStateConnected)), completeRequest(kCommToolControlChannel, kCommErrNotConnected);)
		if (!shouldAbort(0, 0))
		{
			fState |= kToolStateRelease;
			if (f28 == 0)
				f28 = 3;
			releaseStart();
		}
	}
	XENDTRY;
}


void
CCommTool::releaseStart(void)
{
	if (!f94[kCommToolPutChannel].fRequestPending)
		startAbort(kCommErrConnectionAborted);
}


void
CCommTool::releaseComplete(NewtonErr inResult)
{
	fState &= ~kToolStateRelease;
	completeRequest(kCommToolControlChannel, inResult);
}

#pragma mark Bind

void
CCommTool::bind(void)
{
	XTRY
	{
		XFAILIF((fState & (kToolStateConnecting | kToolStateConnected)), completeRequest(kCommToolControlChannel, kCommErrAlreadyConnected);)
		XFAILIF((fState & kToolStateBound), completeRequest(kCommToolControlChannel, kCommErrAlreadyBound);)

		COptionArray * options;
		ArrayIndex numOfOptions;
		CCommToolBindRequest * request = (CCommToolBindRequest *)f4C;
		if (f94[kCommToolControlChannel].fRequestMsgSize == 0x20)
		{
			options = request->fOptions;
			numOfOptions = request->fOptionCount;
		}
		else
		{
			options = NULL;
			numOfOptions = 0;
		}
		processControlOptions(request->fOutside, options, numOfOptions);
	}
	XENDTRY;
}


void
CCommTool::bindOptionsComplete(NewtonErr inResult)
{
	XTRY
	{
		XFAILIF(inResult, bindComplete(inResult);)
		bindStart();
	}
	XENDTRY;
}


void
CCommTool::bindStart(void)
{
	bindComplete(noErr);
}


void
CCommTool::bindComplete(NewtonErr inResult)
{
	NewtonErr result;
	if ((result = processOptionsCleanUp(inResult, &fControlOptionInfo)) == noErr)
		fState |= kToolStateBound;
	completeRequest(kCommToolControlChannel, result);
}

#pragma mark Unbind

void
CCommTool::unbind(void)
{
	XTRY
	{
		XFAILIF((fState & (kToolStateConnecting | kToolStateConnected)), unbindComplete(kCommErrAlreadyConnected);)
		XFAILNOT((fState & kToolStateBound), unbindComplete(noErr);)
		unbindStart();
	}
	XENDTRY;
}


void
CCommTool::unbindStart(void)
{
	unbindComplete(noErr);
}


void
CCommTool::unbindComplete(NewtonErr inResult)
{
	if (inResult == noErr)
		fState &= ~kToolStateBound;
	completeRequest(kCommToolControlChannel, inResult);
}

#pragma mark Abort

void
CCommTool::holdAbort(void)
{
	++fAbortLock;
}

void
CCommTool::allowAbort(void)
{
	if (FLAGTEST(fState, kToolStateWantAbort)
	&&  fAbortLock == 1)
	{
		fState |= kToolStateTerminating;
		terminateConnection();
	}
	--fAbortLock;
}

NewtonErr
CCommTool::startAbort(NewtonErr inReason)
{
	if (FLAGTEST(fState, kToolStateWantAbort))
		return fErrStatus;
	if ((fState & (kToolStateConnecting | kToolStateConnected)) == 0)
		return kCommErrNotConnected;
	holdAbort();
	FLAGSET(fState, kToolStateWantAbort);
	fErrStatus = inReason;
	allowAbort();
	return noErr;
}

bool
CCommTool::shouldAbort(ULong inResetFlags, NewtonErr inReason)
{
	FLAGCLEAR(fState, inResetFlags);
	if (inReason == noErr)
	{
		if (FLAGTEST(inResetFlags, kToolStateWantAbort) != 0)
			return true;
	}
	else
	{
		if (f28 == 0)
			f28 = 2;
		startAbort(inReason);
		return true;
	}
	return false;
}

void
CCommTool::terminateConnection(void)
{
	TerminateProcPtr termProc;
	for (bool isMore = true; isMore; )
	{
		getNextTermProc(fTermPhase, fTermFlag, termProc);
		++fTermPhase;
		if (fTermFlag == 0)
		{
			terminateComplete();
			return;
		}
		if ((fState & fTermFlag) != 0)
			isMore = termProc((void *)fState);	// sic
	}
}

void
CCommTool::terminateComplete(void)
{
	NewtonErr err = fErrStatus;
	fErrStatus = noErr;
	fState &= ~0xC7;
	if (f94[kCommToolGetChannel].fRequestPending)
		getComplete(err, 0, 0);
	if (f94[kCommToolPutChannel].fRequestPending)
		putComplete(err, 0);
	if (FLAGTEST(fState, kToolStateClosing))
		closeComplete(noErr);
	else if (f94[kCommToolControlChannel].fRequestPending)
	{
		switch (f1D4)
		{
		case kCommToolConnect:
			connectComplete(err);
			break;
		case kCommToolListen:
			listenComplete(err);
			break;
		case kCommToolAccept:
			acceptComplete(err);
			break;
		case kCommToolDisconnect:
			disconnectComplete(noErr);
			break;
		case kCommToolRelease:
			releaseComplete(noErr);
			break;
//		case kCommToolBind:
//		case kCommToolUnbind:
		default:
			completeRequest(kCommToolControlChannel, err);
			break;
		}
	}
	killRequestComplete(kCommToolRequestTypeControl, noErr);
	if (f28 != 2)
	{
		fGetEventReply.fEventCode = 2;
		fGetEventReply.fEventTime = GetGlobalTime();
		fGetEventReply.fEventData = f28;
		fGetEventReply.fServiceId = fName;
		fGetEventReply.fResult = postCommEvent(fGetEventReply, noErr);
	}
}

void
CCommTool::getNextTermProc(ULong inTerminationPhase, ULong& outTerminationFlag, TerminateProcPtr& outTerminationProc)
{
	outTerminationFlag = 0;
	outTerminationProc = NULL;
}


#pragma mark Options

void
CCommTool::optionMgmt(CCommToolOptionMgmtRequest *)
{}

void
CCommTool::optionMgmtComplete(NewtonErr inResult)
{}

NewtonErr
CCommTool::addDefaultOptions(COptionArray * inOptions)
{ return noErr; }	// that’s really all it does


NewtonErr
CCommTool::addCurrentOptions(COptionArray * inOptions)
{ return noErr; }	// that’s really all it does


void
CCommTool::processOptions(COptionArray * inOptions)
{ }

void
CCommTool::processControlOptions(bool inOutside, COptionArray * inOptions, ArrayIndex inNumOfOptions)
{
	fControlOptionInfo.fOptionsState = 0;
	if (inOutside)
	{
		fControlOptionInfo.fOptionsState = kOptionsOutside;
		fControlOptionInfo.fOptionCount = inNumOfOptions;
	}
	fControlOptionInfo.fChannelNum = kCommToolControlChannel;
}

void
CCommTool::processOptions(CCommToolOptionInfo * info)
{
	NewtonErr err = noErr;

	XTRY
	{
		// on entry, info->options is an ObjectId of the shared options memory
		ObjectId sharedOptMem = (ObjectId)(long)info->fOptions;
		if (sharedOptMem != kNoId)
		{
			if (FLAGTEST(info->fOptionsState, kOptionsOutside))
			{
				// allocate our own mem for it and copy from the shared mem
				info->fOptions = new COptionArray;
				XFAILIF(info->fOptions == NULL, err = kOSErrNoMemory;)
				info->fOptionsState |= kOptionsArrayAllocated;
				XFAIL(err = info->fOptions->init(sharedOptMem, info->fOptionCount))

				info->fOptionsIterator = new COptionIterator(info->fOptions);
				XFAILIF(info->fOptionsIterator == NULL, err = kOSErrNoMemory;)
				info->fOptionsState |= kOptionsInProgress;
				processOptionsContinue(info);
			}
		}
	}
	XENDTRY;

	processOptionsComplete(err, info);
}


void
CCommTool::processOptionsContinue(CCommToolOptionInfo * info)
{
	for ( ; ; )
	{
		info->fCurOptPtr = info->fOptionsIterator->currentOption();
		if (info->fCurOptPtr == NULL)
		{
			// no more options -- we are done
			processOptionsComplete(noErr, info);
			return;
		}

		info->fOptionsIterator->nextOption();

		COption * opt = info->fCurOptPtr;
		if (!opt->isProcessed() && !opt->isService())
		{
			if (opt->label() == kCMOToolSpecificOptions)
			{
				if (((COptionExtended *)opt)->serviceLabel() == fName)
					opt->setProcessed();
				else
				{
					if (info->fOptionsIterator->more())
					{
						info->fOptionsState |= kOptionsNotProcessed;
						processOptionsComplete(noErr, info);
					}
					return;
				}
			}
			else
			{
				int opcode = opt->getOpCode();
				int result = opSuccess;
				if (opt->isServiceSpecific() && ((COptionExtended *)opt)->serviceLabel() != fName)
					result = opNotFound;
				if (!(opcode == opSetNegotiate || opcode == opSetRequired || opcode == opGetDefault || opcode == opGetCurrent))
					result = opBadOpCode;
				if (result == opSuccess)
				{
					if (info->fChannelNum == kCommToolGetChannel)
						result = processGetBytesOptionStart(opt, fName, opcode);
					else if (info->fChannelNum == kCommToolPutChannel)
						result = processPutBytesOptionStart(opt, fName, opcode);
					else if (info->fChannelNum == kCommToolControlChannel)
						result = processOptionStart(opt, fName, opcode);
				}
				if (result == opInProgress)
					return;
				else if (result == opNotFound)
				{
					info->fOptionsState |= kOptionsNotProcessed;
					info->fCurOptPtr->setOpCodeResult(opNotSupported);
				}
				else
				{
					info->fCurOptPtr->setOpCodeResult(result);
					info->fCurOptPtr->setProcessed();
				}
			}
		}
	}
}


void
CCommTool::processOptionsComplete(NewtonErr inErr, CCommToolOptionInfo * info)
{
	NewtonErr err = inErr;
	CUPort * port;
	bool isForwarding = false;
	if (err)
	{
		processOptionsCleanUp(err, info);
	}
	else if (FLAGTEST(info->fOptionsState, kToolStateConnected) && info->fChannelNum == kCommToolControlChannel && f1D4 != kCommToolOpen && (port = forwardOptions()) != NULL)
	{
		XTRY
		{
			if (!FLAGTEST(info->fOptionsState, kOptionsForwardMsgAllocated))
			{
				fOptionRequest = new CCommToolOptionMgmtRequest;
				XFAILIF(fOptionRequest == NULL, err = kOSErrNoMemory;)
				fOptionReply = new CCommToolReply;
				XFAILIF(fOptionReply == NULL, err = kOSErrNoMemory;)
				fOptionMessage = new CUAsyncMessage;
				XFAILIF(fOptionMessage == NULL, err = kOSErrNoMemory;)
				XFAIL(err = initAsyncRPCMsg(*fOptionMessage, 20000))
				fOptionRequest->fOptions = info->fOptions;
				fOptionRequest->fOutside = false;
				fOptionRequest->fRequestOpCode = opProcess;
				info->fOptionsState |= kOptionsForwardMsgAllocated;
			}
			XFAIL(err = port->sendRPC(fOptionMessage, fOptionRequest, sizeof(fOptionRequest), fOptionReply, sizeof(fOptionReply), kNoTimeout, NULL, 4))
			info->fOptionsState |= kOptionsForwardReqActive;
			isForwarding = true;
		}
		XENDTRY;
		XDOFAIL(err)
		{
			processOptionsComplete(err, info);
			return;
		}
		XENDFAIL;
	}

	if (!isForwarding)
	{
		if (info->fChannelNum == kCommToolGetChannel)
			getOptionsComplete(err);
		else if (info->fChannelNum == kCommToolPutChannel)
			putOptionsComplete(err);
		else if (info->fChannelNum == kCommToolControlChannel)
		{
			switch (f1D4)
			{
			case kCommToolOpen:
				openOptionsComplete(err);
				break;
			case kCommToolConnect:
				connectOptionsComplete(err);
				break;
			case kCommToolListen:
				listenOptionsComplete(err);
				break;
			case kCommToolAccept:
				acceptComplete(err);
				break;
			case kCommToolBind:
				bindOptionsComplete(noErr);
				break;
			case kCommToolOptionMgmt:
				optionMgmtComplete(noErr);
				break;
			}
		}
	}
}


NewtonErr
CCommTool::processOptionsCleanUp(NewtonErr inErr, CCommToolOptionInfo * info)
{
	NewtonErr err;

	if ((info->fOptionsState & kOptionsOutside) != 0
	&&  info->fOptions != NULL
	&&  inErr == noErr)
		err = info->fOptions->shadowCopyBack();

	info->fOptionsState &= ~0x3B;
	if (info->fOptionsState & kOptionsArrayAllocated)
	{
		if (info->fOptions != NULL)
			delete info->fOptions;
		info->fOptionsState &= ~kOptionsArrayAllocated;
	}
	info->fOptions = NULL;

	if (info->fOptionsIterator)
	{
		delete info->fOptionsIterator;
		info->fOptionsIterator = NULL;
	}

	if (info->fChannelNum == kCommToolControlChannel)
	{
		if (fOptionRequest)
			delete fOptionRequest, fOptionRequest = NULL;
		if (fOptionMessage)
			delete fOptionMessage, fOptionMessage = NULL;
		if (fOptionReply)
			delete fOptionReply, fOptionReply = NULL;
	}

	return err;
}


NewtonErr
CCommTool::processOptionStart(COption * ioOption, ULong inLabel, ULong inOpcode)
{
	NewtonErr err = noErr;

	if (inLabel == kCMOPassiveClaim)
	{
		if (inOpcode == 0x0100 || inOpcode == 0x0200)
		{
			if ((fState & kToolStateBound) == 0)
				f1D0 = ((CCMOPassiveClaim *)ioOption)->fPassiveClaim;
			else
				err = -1;
		}
		else if (inOpcode == 0x0300)
		{
			CCMOPassiveClaim claim;
			ioOption->copyDataFrom(&claim);
		}
		else
			((CCMOPassiveClaim *)ioOption)->fPassiveClaim = f1D0;
	}

	else if (inLabel == kCMOPassiveState)
	{
		if (inOpcode == 0x0100 || inOpcode == 0x0200)
			f1D1 = ((CCMOPassiveState *)ioOption)->fPassiveState;
		else if (inOpcode == 0x0300)
		{
			CCMOPassiveState state;
			ioOption->copyDataFrom(&state);
		}
		else
			((CCMOPassiveState *)ioOption)->fPassiveState = f1D1;
	}

	else if (inLabel == kCMOServiceIdentifier)
	{
		 if (inOpcode == 0x0300)
		{
			CCMOPassiveClaim claim;
			ioOption->copyDataFrom(&claim);
		}
		else
		{
			((CCMOServiceIdentifier *)ioOption)->fServiceId = fName;
			((CCMOServiceIdentifier *)ioOption)->fPortId = f8C;
		}
	}

	else if (inLabel == kCMOTransportInfo)
	{
		if (inOpcode == 0x0100 || inOpcode == 0x0200)
			err = -3;
		else if (inOpcode == 0x0400)
			ioOption->copyDataFrom(&fTransportInfo);
		else
			err = -1;
	}

	else
		err = -6;

	return err;
}


void
CCommTool::processOptionComplete(ULong inResult)
{
	processCommOptionComplete(inResult, &fControlOptionInfo);
}


void
CCommTool::processCommOptionComplete(ULong inStatus, CCommToolOptionInfo * info)
{
	if (inStatus == opNotFound)
	{
		info->fOptionsState |= kOptionsNotProcessed;
		info->fCurOptPtr->setOpCodeResult(opNotSupported);
	}
	else
	{
		info->fCurOptPtr->setOpCodeResult(inStatus);
		info->fCurOptPtr->setProcessed();
	}
	processOptionsContinue(info);
}

void
CCommTool::processOption(COption * inOption, ULong inLabel, ULong inOpcode)
{
	CCommTool::processOptionStart(inOption, inLabel, inOpcode);
}


CUPort *
CCommTool::forwardOptions(void)
{ return NULL; }	// that’s really all it does


NewtonErr
CCommTool::processPutBytesOptionStart(COption * inOption, ULong inLabel, ULong inOpcode)
{
	return processOptionStart(inOption, inLabel, inOpcode);
}


void
CCommTool::processPutBytesOptionComplete(ULong inStatus)
{
	processCommOptionComplete(inStatus, &fPutOptionInfo);
}

NewtonErr
CCommTool::processGetBytesOptionStart(COption* inOption, ULong inLabel, ULong inOpcode)
{
	return processOptionStart(inOption, inLabel, inOpcode);
}

void
CCommTool::processGetBytesOptionComplete(ULong inStatus)
{
	processCommOptionComplete(inStatus, &fGetOptionInfo);
}

void
CCommTool::getProtAddr(void)	// OBSOLETE
{
	completeRequest(kCommToolControlChannel, kCommErrBadCommand);
}

