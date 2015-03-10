/*
	File:		CommTool.cc

	Contains:	Comm tool implementation.

	Written by:	Newton Research Group, 2009.
*/

#include "CommTool.h"
#include "CommManagerInterface.h"
#include "CMService.h"


NewtonErr
StartCommTool(CCommTool * inTool, ULong inId, CServiceInfo * info)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = inTool->startTask(YES, NO, 0, kCommToolStackSize, kCommToolTaskPriority, inId))
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
		CCommToolOpenRequest * request;
		CCommToolOpenReply * reply;
		XFAILNOT(request = new CCommToolOpenRequest, err = kOSErrNoMemory;)
		XFAILNOT(reply = new CCommToolOpenReply, err = kOSErrNoMemory;)
		request->fOptions = inOptions;
		request->fOptionCount = inOptions->count();
		request->fOutside = NO;

		CAsyncServiceMessage * msg;
		XFAILNOT(msg = new CAsyncServiceMessage, err = kOSErrNoMemory;)
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

CCommToolBindRequest::CCommToolBindRequest()
{
	fOpCode = kCommToolBind;
	fReserved1 = 0;
	fReserved2 = 0;
	fOutside = NO;
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
	fOutside = NO;
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
	fOutside = NO;
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
	fOutside = NO;
	fFrameData = NO;
	fEndOfFrame = NO;
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
	fNonBlocking = NO;
	fFrameData = NO;
	fOutside = NO;
}


CCommToolGetReply::CCommToolGetReply()
{
	fSize = sizeof(CCommToolGetReply);
	fEndOfFrame = NO;
	fGetBytesCount = 0;
}


CCommToolDisconnectRequest::CCommToolDisconnectRequest()
{
	fOpCode = kCommToolDisconnect;
	fDisconnectData = NULL;
	fSequence = 0;
	fReason = 0;
	fOutside = NO;
}

#pragma mark -

CCommToolOptionMgmtRequest::CCommToolOptionMgmtRequest()
{
	fOpCode = kCommToolOptionMgmt;

	fOptions = NULL;
	fOptionCount = 0;
	fRequestOpCode = 0;
	fOutside = NO;
	fCopyBack = NO;
}


CCommToolGetProtAddrRequest::CCommToolGetProtAddrRequest()
{
	fOpCode = kCommToolGetProtAddr;
	fBoundAddr = NULL;
	fPeerAddr = NULL;
	fOutside = NO;
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
	fRequestPending = NO;
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

/*--------------------------------------------------------------------------------
	C C o m m T o o l
--------------------------------------------------------------------------------*/

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


NewtonErr
CCommTool::taskConstructor(void)
{
	NewtonErr err = noErr;

	XTRY
	{
		// ivar zeroing
		f208 = (CommToolRequestType)-1;
		f200 = NO;
		f201 = NO;
		fState = 0;
		f218 = NULL;
		f21C = NULL;

		f174 = 0;
		f1D0 = NO;
		f1D1 = NO;
		f1D2 = NO;
		f20C = 0;
		f210 = 0;
		f214 = NULL;

		fControlOptionInfo.fChannelNum = kCommToolControlChannel;
		fPutOptionInfo.fChannelNum = kCommToolPutChannel;
		fGetOptionInfo.fChannelNum = kCommToolGetChannel;

		f13C = NULL;
		f264 = 0;
		f268 = 0;

		// create heap
		fSavedHeap = GetHeap();
		XFAIL(err = NewVMHeap(0, fHeapSize, &fHeap, 0))
		SetHeap(fHeap);

		// create port
		XFAIL(err = createPort(fName, f8C))

		// create buffers
		XFAILNOT(f218 = new CBufferList, err = MemError();)
		XFAILNOT(f21C = new CBufferList, err = MemError();)
		XFAIL(f218->init(NO))
		XFAIL(f21C->init(NO))
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
	// main event despatch loop
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

		f201 = YES;
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
		ULong thing, spec;
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

		f201 = NO;
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

void
CCommTool::initAsyncRPCMsg(CUAsyncMessage & ioMsg, ULong inRefCon)
{
	XTRY
	{
		XFAIL(ioMsg.init(YES))
		XFAIL(ioMsg.setUserRefCon(inRefCon))
		ioMsg.setCollectorPort(f8C);
	}
	XENDTRY;
}


void
CCommTool::handleInternalEvent(void)
{ }


void
CCommTool::flushChannel(CommToolRequestType, long)
{}


void
CCommTool::completeRequest(CUMsgToken&, long)
{}


void
CCommTool::completeRequest(CUMsgToken&, long, CCommToolReply&)
{}


void
CCommTool::completeRequest(CommToolChannelNumber, long)
{}


void
CCommTool::completeRequest(CommToolChannelNumber, long, CCommToolReply&)
{}


void
CCommTool::handleRequest(CUMsgToken & inMsgToken, ULong inMsgType)
{
	inMsgToken.replyRPC(NULL, 0, kOSErrBadParameters);
}


void
CCommTool::handleReply(ULong inUserRefCon, ULong inMsgType)
{
	if (inUserRefCon == kToolMsgForwardOpt)
	{
		fControlOptionInfo.fOptionsState &= ~0x0A;
//		processOptionsComplete(f214->f08, &fControlOptionInfo);
	}
}


void
CCommTool::handleTimerTick(void)
{ }

//00020810
void
CCommTool::prepGetRequest(void)
{
	;
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
CCommTool::prepPutRequest(void)
{
	;
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
CCommTool::prepControlRequest(ULong inMsgType)
{
	CCommToolControlRequest * request = (CCommToolControlRequest *)f4C;
	doControl(request->fOpCode, inMsgType);
}


void
CCommTool::prepKillRequest(void)
{
	;
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
	completeRequest(kCommToolStatusChannel, (inOpCode == 1) ? getConnectState() : kCommErrBadCommand);
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
		return NO;
	}

	for (int req = kCommToolRequestTypeGet; req <= kCommToolRequestTypeKill; req <<= 1)
		flushChannel((CommToolRequestType)req, kCommErrToolBusy);
	closeComplete(noErr);
	return YES;
}


void
CCommTool::closeComplete(NewtonErr inResult)
{
	f200 = YES;
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
			CShadowBufferSegment * buf;
			XFAILNOT(buf = new CShadowBufferSegment, err = kOSErrNoMemory;)
			XFAIL(err = buf->init((ObjectId)inReq->fData, 0, -1))
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
		f1C = 0;
		f20 = 0;
		f28 = 0;
		f2C = 0;
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
{return noErr;}

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
	f2C++;
}

void
CCommTool::allowAbort(void)
{
	if ((fState & kToolStateWantAbort) != 0
	&&  f2C == 1)
	{
		fState |= kToolStateTerminating;
		terminateConnection();
	}
	f2C--;
}

NewtonErr
CCommTool::startAbort(NewtonErr inReason)
{
	if ((fState & kToolStateWantAbort) != 0)
		return fErrStatus;
	if ((fState & (kToolStateConnecting | kToolStateConnected)) == 0)
		return kCommErrNotConnected;
	holdAbort();
	fState |= kToolStateWantAbort;
	fErrStatus = inReason;
	allowAbort();
	return noErr;
}

bool
CCommTool::shouldAbort(ULong inResetFlags, NewtonErr inReason)
{
	fState &= ~inResetFlags;
	if (inReason == noErr)
	{
		if ((inResetFlags & kToolStateWantAbort) != 0)
			return YES;
	}
	else
	{
		if (f28 == 0)
			f28 = 2;
		startAbort(inReason);
		return YES;
	}
	return NO;
}

void
CCommTool::terminateConnection(void)
{}

void
CCommTool::terminateComplete(void)
{}

void
CCommTool::getNextTermProc(ULong inTerminationPhase, ULong& ioTerminationFlag, TerminateProcPtr& inTerminationProc)
{}

#pragma mark Options

ULong
CCommTool::processOptions(COptionArray * inOptions)
{ }

ULong
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

ULong
CCommTool::processOptions(CCommToolOptionInfo * info)
{
	NewtonErr err = noErr;

	XTRY
	{
		if (info->fOptions)
		{
			if (info->fOptionsState & kOptionsOutside)
			{
				XFAILNOT(info->fOptions = new COptionArray, err = kOSErrNoMemory;)
				info->fOptionsState |= kOptionsArrayAllocated;
				XFAIL(err = info->fOptions->init((ObjectId)info->fOptions, info->fOptionCount))

				XFAILNOT(info->fOptionsIterator = new COptionIterator(info->fOptions), err = kOSErrNoMemory;)
				info->fOptionsState |= kOptionsInProgress;
				return processOptionsContinue(info);
			}
		}
	}
	XENDTRY;

	return processOptionsComplete(err, info);
}


ULong
CCommTool::processOptionsContinue(CCommToolOptionInfo * info)
{}


ULong
CCommTool::processOptionsComplete(NewtonErr inResult, CCommToolOptionInfo * info)
{}


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
		if (f20C)
			delete f20C, f20C = NULL;
		if (f210)
			delete f210, f210 = NULL;
		if (f214)
			delete f214, f214 = NULL;
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
		info->fOptionsState |= 0x02;
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


NewtonErr
CCommTool::forwardOptions(void)
{ return noErr; }	// that’s really all it does


NewtonErr
CCommTool::addDefaultOptions(COptionArray * inOptions)
{ return noErr; }	// that’s really all it does


NewtonErr
CCommTool::addCurrentOptions(COptionArray * inOptions)
{ return noErr; }	// that’s really all it does


void
CCommTool::processPutBytesOptionStart(COption * inOption, ULong inLabel, ULong inOpcode)
{
	processOptionStart(inOption, inLabel, inOpcode);
}


void
CCommTool::processPutBytesOptionComplete(ULong inStatus)
{
	processCommOptionComplete(inStatus, &fPutOptionInfo);
}

void
CCommTool::processGetBytesOptionStart(COption* inOption, ULong inLabel, ULong inOpcode)
{
	processOptionStart(inOption, inLabel, inOpcode);
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

