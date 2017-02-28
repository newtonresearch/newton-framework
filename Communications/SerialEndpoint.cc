/*
	File:		SerialEndpoint.cc

	Contains:	Serial endpoint implementation.

	Written by:	Newton Research Group, 2009.
*/

#include "SerialEndpoint.h"
#include "AppWorld.h"


/*--------------------------------------------------------------------------------
	CCommToolPB
--------------------------------------------------------------------------------*/

CCommToolPB::CCommToolPB(CommToolRequestType inReqType, ULong inClientHandler, bool inAsync)
{
	fMsg.init(true);
	fClientRefCon = inClientHandler;
	fReqType = inReqType;
	fIsAsync = inAsync;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	CCommToolGetPB
--------------------------------------------------------------------------------*/

CCommToolGetPB::CCommToolGetPB(ULong inClientHandler, bool inAsync)
	:	CCommToolPB(kCommToolRequestTypeGet, inClientHandler, inAsync)
{
	f74 = 0;
	f70 = 0;
	f80 = 0;
	f78 = 0;
	f7C = 0;
	fMadeData = NULL;
	fQueue = NULL;
	f8C = false;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	CCommToolPutPB
--------------------------------------------------------------------------------*/

CCommToolPutPB::CCommToolPutPB(ULong inClientHandler, bool inAsync)
	:	CCommToolPB(kCommToolRequestTypePut, inClientHandler, inAsync)
{
	fData = NULL;
	fRawData = NULL;
	fOptions = NULL;
	f74 = 0;
	fMadeData = NULL;
	fQueue = NULL;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	CCommToolControlPB
--------------------------------------------------------------------------------*/

CCommToolControlPB::CCommToolControlPB(ULong inArg1, long inArg2, ULong inClientHandler, bool inAsync)
	:	CCommToolPB(kCommToolRequestTypeControl, inClientHandler, inAsync)
{
	fCompletionEvent.f10 = inArg2;
	fRequest.fCmd = inArg1;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	CCommToolAbortPB
--------------------------------------------------------------------------------*/

CCommToolAbortPB::CCommToolAbortPB(ULong inWhat, ULong inClientHandler, bool inAsync)
	:	CCommToolPB(kCommToolRequestTypeKill, inClientHandler, inAsync)
{
	fRequest.fRequestsToKill = inWhat;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	CCommToolEventPB
--------------------------------------------------------------------------------*/

CCommToolEventPB::CCommToolEventPB(ULong inClientHandler)
	:	CCommToolPB(kCommToolRequestTypeGetEvent, inClientHandler, true)
{ }

#pragma mark -

/*--------------------------------------------------------------------------------
	CSerialEndpoint
--------------------------------------------------------------------------------*/

PROTOCOL_IMPL_SOURCE_MACRO(CSerialEndpoint)


CSerialEndpoint *
CSerialEndpoint::make(void)
{
	fPendingList = NULL;
	fPutPBList = NULL;
	fGetPBList = NULL;
	fCommToolEvent = NULL;
	f34 = 0;
	f38 = NULL;
	f3C = NULL;
	f40 = false;
	f41 = false;
	f42 = false;
	f43 = false;
	fToolIsRunning = false;
	fEventHandler = NULL;
	fInfo = NULL;

	return this;
}


void
CSerialEndpoint::destroy(void)
{
	nukePending();
	if (fToolIsRunning)
	{
		fState = T_UNBND;
		f42 = false;
		f40 = false;
		close();
	}
	if (fPutPBList)
		nukeGetPBList();
	if (fGetPBList)
		nukePutPBList();
	if (fCommToolEvent)
		delete fCommToolEvent;
	if (f38)
		delete f38;
	if (f3C)
		delete f3C;
	destroyBaseEndpoint();
}


NewtonErr
CSerialEndpoint::deleteLeavingTool(void)
{
	if (f42)
		return TACCESS;

	if (fState == T_UNINIT)
		return TOUTSTATE;

	NewtonErr err = noErr;
	XTRY
	{
		if (isPending(kEitherCall))
			XFAIL(err = nAbort(true))
		if (fToolIsRunning)
			XFAIL(err = killKillKill(8, &fCommToolEvent->fMsg))
		f40 = false;
		fToolIsRunning = false;
		destroy();
	}
	XENDTRY;
	return err;
}


NewtonErr
CSerialEndpoint::addToAppWorld(void)
{ return TNOTSUPPORT; }


NewtonErr
CSerialEndpoint::removeFromAppWorld(void)
{ return TNOTSUPPORT; }


NewtonErr
CSerialEndpoint::open(ULong inClientHandler)
{
	if (fState != T_UNINIT)
		return TOUTSTATE;

	if (isPending(kSyncCall))
		return TALRDYSYNC;

	if (f42)
		return TACCESS;

	NewtonErr err;
	XTRY
	{
		fState = T_INFLUX;
		XFAIL(err = initPending())
		XFAIL(err = initGetPBList())
		XFAIL(err = initPutPBList())
		fCommToolEvent = new CCommToolEventPB(0);
		XFAILIF(fCommToolEvent == NULL, err = MemError();)
		fClientRefCon = inClientHandler;
		err = postEventRequest(fCommToolEvent);	// tell us about events
	}
	XENDTRY;
	fState = err ? T_UNINIT : T_UNBND;

	return err;
}


NewtonErr
CSerialEndpoint::close(void)
{
	if (!(fState == T_UNINIT || fState == T_UNBND))
		return TOUTSTATE;

	if (isPending(kEitherCall))
		return TALRDYSYNC;

	if (f42)
		return TACCESS;

	NewtonErr err = noErr;
	if (fToolIsRunning)
	{
		if (fState != T_UNINIT)
			killKillKill(8, &fCommToolEvent->fMsg);
		CCommToolControlPB pb(2, 0, 0, false);
		f40 = true;
		err = fEventHandler->callService(4, &pb.fMsg, &pb.fRequest, sizeof(CCommToolControlRequest), &pb.f28, sizeof(CCommToolReply), kNoTimeout, 0, true);
		f40 = false;
		fToolIsRunning = false;
		if (err = noErr)
			err = pb.fReply.fResult;
	}
	fState = T_UNINIT;
	nukePending();
	nukeGetPBList();
	nukePutPBList();
	if (fCommToolEvent)
		delete fCommToolEvent;

	return err;
}


NewtonErr
CSerialEndpoint::abort(void)
{ return nAbort(true); }


NewtonErr
CSerialEndpoint::setState(int inState)
{
	if (fState != T_UNBND)
		return TOUTSTATE;

	if (inState < T_UNINIT || inState > T_INFLUX)
		return kCommErrBadParameter;

	fState = inState;
	return noErr;
}


bool
CSerialEndpoint::setSync(bool inSync)
{
	bool syncState = fSync;
	if (syncState != inSync)
	{
		if (!inSync || !isPending(kAsyncCall))
			fSync = inSync;
	}
	return syncState;
}


NewtonErr
CSerialEndpoint::getProtAddr(COptionArray * inBindAddr, COptionArray * inPeerAddr, Timeout inTimeout)
{ return TNOTSUPPORT; }


NewtonErr
CSerialEndpoint::optMgmt(ULong inOpCodes, COptionArray * inOptions, Timeout inTimeout)
{ return nOptMgmt(inOpCodes, inOptions, inTimeout, fSync); }


NewtonErr
CSerialEndpoint::bind(COptionArray * inAddr, long * ioQLen, Timeout inTimeout)
{ return nBind(inAddr, inTimeout, fSync); }


NewtonErr
CSerialEndpoint::unbind(Timeout inTimeout)
{ return nUnBind(inTimeout, fSync); }


NewtonErr
CSerialEndpoint::listen(COptionArray * inAddr, COptionArray * inOptions, CBufferSegment * inData, long * ioSeq, Timeout inTimeout)
{
	if (inAddr)
		return TBADADDR;
	return nListen(inOptions, inData, ioSeq, inTimeout, fSync);
}


NewtonErr
CSerialEndpoint::accept(CEndpoint * inResfd, COptionArray * inAddr, COptionArray * inOptions, CBufferSegment * inData, long inSeq, Timeout inTimeout)
{
	if (inAddr)
		return TBADADDR;
	return nAccept(inResfd, inOptions, inData, inSeq, inTimeout, fSync);
}


NewtonErr
CSerialEndpoint::connect(COptionArray * inAddr, COptionArray * inOptions, CBufferSegment * inData, long * ioSeq, Timeout inTimeout)
{
	if (inAddr)
		return TBADADDR;
	return nConnect(inOptions, inData, ioSeq, inTimeout, fSync);
}


NewtonErr
CSerialEndpoint::disconnect(CBufferSegment * inData, long inReason, long inSeq)
{
	fSync = true;
	return nDisconnect(inData, inReason, inSeq);
}


NewtonErr
CSerialEndpoint::release(Timeout inTimeout)
{ return nRelease(inTimeout, fSync); }


NewtonErr
CSerialEndpoint::snd(UByte * inBuffer, ArrayIndex & ioSize, ULong inFlags, Timeout inTimeout)
{
	size_t numOfBytes = ioSize;
	NewtonErr err = nSnd(inBuffer, &numOfBytes, inFlags, inTimeout, fSync, NULL);
	ioSize = numOfBytes;
	return err;
}


NewtonErr
CSerialEndpoint::snd(CBufferSegment * inBuffer, ULong inFlags, Timeout inTimeout)
{ return nSnd(inBuffer, inFlags, inTimeout, fSync, NULL); }


NewtonErr
CSerialEndpoint::rcv(UByte * outBuffer, ArrayIndex & ioSize, ArrayIndex inThreshold, ULong * ioFlags, Timeout inTimeout)
{
	size_t numOfBytes = ioSize;
	NewtonErr err = nRcv(outBuffer, &numOfBytes, inThreshold, ioFlags, inTimeout, fSync, NULL);
	ioSize = numOfBytes;
	return err;
}


NewtonErr
CSerialEndpoint::rcv(CBufferSegment * outBuffer, ArrayIndex inThreshold, ULong * ioFlags, Timeout inTimeout)
{ return nRcv(outBuffer, inThreshold, ioFlags, inTimeout, fSync, NULL); }


NewtonErr
CSerialEndpoint::waitForEvent(Timeout inTimeout)
{
	NewtonErr err;
	f41 = true;
	XTRY
	{
		if (f3C == NULL)
		{
			f3C = new CPseudoSyncState;
			XFAILIF(f3C == NULL, err = MemError();)
			XFAIL(err = f3C->init())
		}
		err = f3C->block(inTimeout);
	}
	XENDTRY;
	f41 = false;	// original doesn’t clear this on err
	return err;
}


NewtonErr
CSerialEndpoint::nBind(COptionArray * inOptions, Timeout inTimeout, bool inSync)
{ return noErr; }

NewtonErr
CSerialEndpoint::nListen(COptionArray * inOptions, CBufferSegment * inData, long * ioSeq, Timeout inTimeout, bool inSync)
{ return noErr; }

NewtonErr
CSerialEndpoint::nAccept(CEndpoint* resfd, COptionArray * inOptions, CBufferSegment * inData, long inSeq, Timeout inTimeout, bool inSync)
{ return noErr; }

NewtonErr
CSerialEndpoint::nConnect(COptionArray * inOptions, CBufferSegment * inData, long * ioSeq, Timeout inTimeout, bool inSync)
{ return noErr; }

NewtonErr
CSerialEndpoint::nRelease(Timeout inTimeout, bool inSync)
{ return noErr; }

NewtonErr
CSerialEndpoint::nDisconnect(CBufferSegment * inData, long inReason, long inSeq, Timeout inTimeout, bool inSync)
{ return noErr; }

NewtonErr
CSerialEndpoint::nUnBind(Timeout inTimeout, bool inSync)
{ return noErr; }

NewtonErr
CSerialEndpoint::nOptMgmt(ULong inOpCodes, COptionArray * inOptions, Timeout inTimeout, bool inSync)
{ return noErr; }


NewtonErr
CSerialEndpoint::nSnd(CBufferSegment * inData, ULong inFlags, Timeout inTimeout, bool inSync, COptionArray * inOptions)
{
	if (fState != T_DATAXFER)
		return TOUTSTATE;

	if (inSync && isPending(kSyncCall))
		return TALRDYSYNC;

	if (f42)
		return TACCESS;

	if (inData->getSize() == 0)
		return noErr;

	NewtonErr err;
	CCommToolPutPB * pb;
	XTRY
	{
		XFAILNOT(pb = grabPutPB(false), err = TSYSERR;)
		pb->fRawData = NULL;
		pb->fData = inData;
		pb->fOptions = inOptions;

		size_t dataSize = inData->getSize();
		XFAIL(err = pb->fQueue->insert(inData))
		err = sendBytes(pb, &dataSize, inFlags, inTimeout, inSync, inOptions);
	}
	XENDTRY;

	if (pb)
	{
		if (inSync || err)
			releasePutPB(pb);
	}
	return err;
}


NewtonErr
CSerialEndpoint::nSnd(UByte * inData, size_t * ioSize, ULong inFlags, Timeout inTimeout, bool inSync, COptionArray * inOptions)
{
	if (fState != T_DATAXFER)
		return TOUTSTATE;

	if (inSync && isPending(kSyncCall))
		return TALRDYSYNC;

	if (f42)
		return TACCESS;

	if (*ioSize == 0)
		return noErr;

	NewtonErr err;
	CCommToolPutPB * pb;
	XTRY
	{
		XFAILNOT(pb = grabPutPB(true), err = TSYSERR;)
		if (inSync)
			pb->fIsAsync = false;
		else
		{
			pb->fIsAsync = true;
			pb->fData = NULL;
			pb->fRawData = inData;
			pb->fOptions = inOptions;
		}
		XFAIL(err = pb->fMadeData->init(inData, *ioSize))
		XFAIL(err = pb->fQueue->insert(pb->fMadeData))
		err = sendBytes(pb, ioSize, inFlags, inTimeout, inSync, inOptions);
	}
	XENDTRY;

	if (pb)
	{
		if (inSync || err)
			releasePutPB(pb);
	}
	return err;
}


NewtonErr
CSerialEndpoint::sendBytes(CCommToolPutPB * inPB, size_t * ioSize, ULong inFlags, Timeout inTimeout, bool inSync, COptionArray * inOptions)
{
	NewtonErr err;
	inPB->fRequest.fOutside = false;
	if (inOptions && inOptions->count() > 0)
	{
		inPB->fRequest.fOptions = inOptions;
		inPB->fRequest.fOptionCount = inOptions->count();
	}
	else
	{
		inPB->fRequest.fOptions = NULL;
		inPB->fRequest.fOptionCount = 0;
	}
	inPB->fRequest.fValidCount = -1;
	inPB->fRequest.fData = inPB->fQueue;
	inPB->fRequest.fFrameData = (inFlags & 0x02) != 0;		// T_PACKET?
	inPB->fRequest.fEndOfFrame = (inFlags & 0x01) == 0;	// T_MORE?

	if (inSync)
		f40 = true;
	else
		inPB->fMsg.setCollectorPort(*gAppWorld->getMyPort());
	err = fEventHandler->callService(2, &inPB->fMsg, &inPB->fRequest, sizeof(CCommToolPutRequest), &inPB->fReply, sizeof(CCommToolPutReply), inTimeout, (ULong) inPB, inSync);
	if (err = kOSErrMessageTimedOut)
		killKillKill(2, &inPB->fMsg);

	if (inSync)
	{
		if (err == noErr)
			err = inPB->fReply.fResult;
		if (err == noErr)
			*ioSize = inPB->fReply.fPutBytesCount;
		if (f40)
			f40 = false;
		else if (err == noErr)
			err = kOSErrCallAborted;
	}
	else
	{
		if (err == noErr)
			fPendingList->insert(inPB);
	}

	if (err)
		*ioSize = 0;
	return err;
}


NewtonErr
CSerialEndpoint::recvBytes(CCommToolGetPB * inPB, size_t * ioSize, size_t inThreshold, ULong * ioFlags, Timeout inTimeout, bool inSync, COptionArray * inOptions)
{
	NewtonErr err;
	inPB->fRequest.fOutside = false;
	if (inOptions && inOptions->count() > 0)
	{
		inPB->fRequest.fOptions = inOptions;
		inPB->fRequest.fOptionCount = inOptions->count();
	}
	else
	{
		inPB->fRequest.fOptions = NULL;
		inPB->fRequest.fOptionCount = 0;
	}
	inPB->fRequest.fData = inPB->fQueue;
	bool isFrameData = (*ioFlags & 0x02) != 0;	// T_PACKET?
	inPB->f8C = isFrameData;
	if (isFrameData)
	{
		inPB->fRequest.fFrameData = true;
		inPB->fRequest.fThreshold = 0;
		inPB->fRequest.fNonBlocking = false;
	}
	else
	{
		inPB->fRequest.fFrameData = false;
		inPB->fRequest.fThreshold = inThreshold;
		inPB->fRequest.fNonBlocking = true;
	}

	if (inSync)
		f40 = true;
	else
		inPB->fMsg.setCollectorPort(*gAppWorld->getMyPort());
	err = fEventHandler->callService(1, &inPB->fMsg, &inPB->fRequest, sizeof(CCommToolGetRequest), &inPB->fReply, sizeof(CCommToolGetReply), inTimeout, (ULong) inPB, inSync);
	if (err = kOSErrMessageTimedOut)
		killKillKill(1, &inPB->fMsg);

	if (inSync)
	{
		if (err == noErr)
			err = inPB->fReply.fResult;
		if (err == noErr)
		{
			*ioSize = inPB->fReply.fGetBytesCount;
			ULong flags = isFrameData ? 0x02 : 0x00;
			if (!inPB->fEndOfFrame)
				flags |= 0x01;		// more
			*ioFlags = flags;
		}
		if (f40)
			f40 = false;
		else if (err == noErr)
			err = kOSErrCallAborted;
	}
	else
	{
		if (err == noErr)
			fPendingList->insert(inPB);
	}

	if (err)
		*ioSize = 0;
	return err;
}


NewtonErr
CSerialEndpoint::nAbort(bool inSync)
{
	if (f42)
		return TACCESS;
	return prepareAbort(7, inSync);
}


NewtonErr
CSerialEndpoint::timeout(ULong inRefCon)
{
	NewtonErr err = noErr;
	for (ArrayIndex i = 0, count = fPendingList->count(); i < count; ++i)
	{
		CCommToolPB * pb = (CCommToolPB *)fPendingList->at(i);
		if (pb == (CCommToolPB *)inRefCon)
		{
			err = killKillKill(pb->fReqType, NULL);
			break;
		}
	}
	return err;
}

#pragma mark -

bool
CSerialEndpoint::isPending(ULong inWhich)
{
	bool pending = false;
	if (inWhich & kSyncCall)
		pending = f40;
	if (inWhich & kAsyncCall)
	{
		if (fPendingList && fPendingList->count() > 0)
			pending |= true;
	}
	return pending;
}


NewtonErr
CSerialEndpoint::initPending(void)
{
	NewtonErr err = noErr;
	XTRY
	{
		fPendingList = new CList;
		XFAILIF(fPendingList == NULL, err = MemError();)
	}
	XENDTRY;
	return err;
}


void
CSerialEndpoint::nukePending(void)
{
	int i, count = fPendingList->count();
	for (i = count - 1; i >= 0; i--)
	{
		CCommToolEventPB * pb = (CCommToolEventPB *)fPendingList->at(i);
		fPendingList->removeAt(i);
		delete pb;
	}
	delete fPendingList;
	fPendingList = NULL;
}

#pragma mark -

NewtonErr
CSerialEndpoint::initPutPBList(void)
{
	NewtonErr err;
	XTRY
	{
		fPutPBList = new CList;
		XFAILIF(fPutPBList == NULL, err = MemError();)
		CCommToolPutPB * pb = grabPutPB(false);
		XFAILIF(pb == NULL, err = TSYSERR;)
		err = fPutPBList->insert(pb);
	}
	XENDTRY;
	return err;
}


CCommToolPutPB *
CSerialEndpoint::grabPutPB(bool inMake)
{
	NewtonErr err = noErr;
	CCommToolPutPB * pb;
	XTRY
	{
		if (fPutPBList->count() > 0)
		{
			pb = (CCommToolPutPB *)fPutPBList->last();
			fPutPBList->removeLast();
		}
		else
		{
			pb = new CCommToolPutPB(fClientRefCon, false);
			XFAILIF(pb == NULL, err = MemError();)
			pb->fQueue = CBufferList::make();
			XFAILIF(pb->fQueue == NULL, err = MemError();)
			XFAIL(err = pb->fQueue->init(false))
		}
		if (inMake)
		{
			if (pb->fMadeData == NULL) {
				pb->fMadeData = CBufferSegment::make();
				XFAILIF(pb->fMadeData == NULL, err = MemError();)
			}
		}
	}
	XENDTRY;
	XDOFAIL(err)
	{
		if (pb)
		{
			delete pb;
			pb = NULL;
		}
	}
	XENDFAIL;
	
	return pb;
}


void
CSerialEndpoint::releasePutPB(CCommToolPutPB * inPB)
{
	if (fPutPBList)
	{
		CBufferSegment * buf;
		inPB->fQueue->removeAll();
		if ((buf = inPB->fMadeData) != NULL)
			buf->init(NULL, 0);
		fPutPBList->insert(inPB);
	}

	else
		delete inPB;
}


void
CSerialEndpoint::nukePutPBList(void)
{
	int i, count = fPutPBList->count();
	for (i = count - 1; i >= 0; i--)
	{
		CCommToolPutPB * pb = (CCommToolPutPB *)fPutPBList->at(i);
		fPutPBList->removeAt(i);
		delete pb;
	}
	delete fPutPBList;
	fPutPBList = NULL;
}

#pragma mark -

NewtonErr
CSerialEndpoint::initGetPBList(void)
{
	NewtonErr err;
	XTRY
	{
		fGetPBList = new CList;
		XFAILIF(fGetPBList == NULL, err = MemError();)
		CCommToolGetPB * pb = grabGetPB(false);
		XFAILIF(pb == NULL, err = TSYSERR;)
		err = fGetPBList->insert(pb);
	}
	XENDTRY;
	return err;
}


CCommToolGetPB *
CSerialEndpoint::grabGetPB(bool inMake)
{
	NewtonErr err = noErr;
	CCommToolGetPB * pb;
	XTRY
	{
		if (fGetPBList->count() > 0)
		{
			pb = (CCommToolGetPB *)fGetPBList->last();
			fGetPBList->removeLast();
		}
		else
		{
			pb = new CCommToolGetPB(fClientRefCon, false);
			XFAILIF(pb == NULL, err = MemError();)
			pb->fQueue = CBufferList::make();
			XFAILIF(pb->fQueue == NULL, err = MemError();)
			XFAIL(err = pb->fQueue->init(false))
		}
		if (inMake)
		{
			if (pb->fMadeData == NULL) {
				pb->fMadeData = CBufferSegment::make();
				XFAILIF(pb->fMadeData == NULL, err = MemError();)
			}
		}
	}
	XENDTRY;
	XDOFAIL(err)
	{
		if (pb)
		{
			delete pb;
			pb = NULL;
		}
	}
	XENDFAIL;
	
	return pb;
}


void
CSerialEndpoint::releaseGetPB(CCommToolGetPB * inPB)
{
	if (fGetPBList)
	{
		CBufferSegment * buf;
		inPB->fQueue->removeAll();
		if ((buf = inPB->fMadeData) != NULL)
			buf->init(NULL, 0);
		fGetPBList->insert(inPB);
	}

	else
		delete inPB;
}


void
CSerialEndpoint::nukeGetPBList(void)
{
	int i, count = fGetPBList->count();
	for (i = count - 1; i >= 0; i--)
	{
		CCommToolGetPB * pb = (CCommToolGetPB *)fGetPBList->at(i);
		fGetPBList->removeAt(i);
		delete pb;
	}
	delete fGetPBList;
	fGetPBList = NULL;
}

#pragma mark -

NewtonErr
CSerialEndpoint::postEventRequest(CCommToolEventPB * inPB)
{
	if (fState == T_UNINIT)
		return TOUTSTATE;

	inPB->fRequest.fOpCode = 0;
	inPB->fMsg.setCollectorPort(*gAppWorld->getMyPort());
	return fEventHandler->callService(8, &inPB->fMsg, &inPB->fRequest, sizeof(CCommToolControlRequest), &inPB->fReply, sizeof(CCommToolGetEventReply));
}


NewtonErr
CSerialEndpoint::postKillRequest(ULong inWhat, bool inAsync)
{
	NewtonErr err = noErr;
	CCommToolAbortPB * pb;

	XTRY
	{
		pb = new CCommToolAbortPB(inWhat, f18, inAsync);
		XFAILIF(pb == NULL, err = MemError();)
		pb->fMsg.setCollectorPort(*gAppWorld->getMyPort());
		XFAILIF(err = fEventHandler->callService(16, &pb->fMsg, &pb->fRequest, sizeof(CCommToolKillRequest), &pb->fReply, sizeof(CCommToolReply)), delete pb;)
		fPendingList->insert(pb);
		f42 = true;
	}
	XENDTRY;
	return err;
}


NewtonErr
CSerialEndpoint::prepareAbort(ULong inWhat, bool inArg2)
{
	NewtonErr err = noErr;
	XTRY
	{
		if (inArg2)
		{
			if (isPending(kSyncCall) && !isPending(kAsyncCall))
			{
				f42 = true;
				err = killKillKill(inWhat, NULL);
				f42 = false;
				f40 = false;
			}
			else if (isPending(kEitherCall))
			{
				f38 = new CPseudoSyncState;
				XFAILIF(f38 == NULL, err = MemError();)
				XFAIL(err = f38->init())
				XFAIL(err = postKillRequest(inWhat, true))
				f43 = true;
				err = f38->block(0);
			}
		}

		else
			err = postKillRequest(inWhat, true);
	}
	XENDTRY;
	return err;
}

#pragma mark -

bool
CSerialEndpoint::handleEvent(ULong inMsgType, CEvent * inEvent, ULong inMsgSize)
{
	return false;
}


bool
CSerialEndpoint::handleComplete(CUMsgToken * inMsgToken, ULong * ioMsgSize, CEvent * inEvent)
{
	if (inMsgToken->f00 == fCommToolEvent->fMsg)
		handleEventReply(fCommToolEvent);

	else
	{
		CCommToolPB * pb = NULL;
		bool isPBFound = false;
		for (ArrayIndex i = 0, count = fPendingList->count(); i < count; ++i)
		{
			pb = fPendingList->at(i);
			if (pb->fMsg == inMsgToken->f00)
			{
				fPendingList->removeAt(i);
				isPBFound = true;
				break;
			}
		}
		if (isPBFound)
		{
			fEventHandler->killTimer((ULong)pb);
			switch (pb->fReqType)
			{
			case kCommToolRequestTypeGet:
				handleGetReply((CCommToolGetPB *)pb);
				break;
			case kCommToolRequestTypePut:
				handlePutReply((CCommToolPutPB *)pb);
				break;
			case kCommToolRequestTypeControl:
				handleControlReply((CCommToolControlPB *)pb);
				break;
			case kCommToolRequestTypeKill:
				handleAbortReply((CCommToolAbortPB *)pb);
				break;
			}
		}
	}
	return false;
}


void
CSerialEndpoint::handleGetReply(CCommToolGetPB * inPB)
{}


void
CSerialEndpoint::handlePutReply(CCommToolPutPB * inPB)
{
#if 0
	if (f18/*fClientRefCon?*/ && inPB->fIsAsync)
	{
		size_t sp30 = 0x30;
		;
		sp20 = inPB->f4C;	// block of 4 longs CSndCompleteEvent event?
		sp08 = inPB->f40;
		sp28 = inPB->f48;
		sp0C = f18;
		sp10 = -2;
		releasePutPB(inPB);
		f18->vf08(0, &sp30, &sp00);
	}

	else
#endif
		releasePutPB(inPB);
}


void
CSerialEndpoint::handleAbortReply(CCommToolAbortPB * inPB)
{}


void
CSerialEndpoint::handleControlReply(CCommToolControlPB * inPB)
{
	switch (inPB->fRequest.fOpCode)
	{
	case kCommToolConnect:
		handleConnectReply((CCommToolConnectPB *)inPB);
		break;
	case kCommToolListen:
		handleListenReply((CCommToolConnectPB *)inPB);
		break;
	case kCommToolAccept:
		handleAcceptReply((CCommToolConnectPB *)inPB);
		break;
	case kCommToolDisconnect:
		handleDisconnectReply((CCommToolDisconnectPB *)inPB);
		break;
	case kCommToolRelease:
		handleReleaseReply((CCommToolControlPB *)inPB);
		break;
	case kCommToolBind:
		handleBindReply((CCommToolBindPB *)inPB);
		break;
	case kCommToolUnbind:
		handleUnbindReply((CCommToolBindPB *)inPB);
		break;
	case kCommToolOptionMgmt:
		handleOptMgmtReply((CCommToolOptMgmtPB *)inPB);
		break;
	}
	delete inPB;
}


void
CSerialEndpoint::handleConnectReply(CCommToolConnectPB * inPB)
{}

void
CSerialEndpoint::handleListenReply(CCommToolConnectPB * inPB)
{}

void
CSerialEndpoint::handleAcceptReply(CCommToolConnectPB * inPB)
{}

void
CSerialEndpoint::handleDisconnectReply(CCommToolDisconnectPB * inPB)
{}

void
CSerialEndpoint::handleReleaseReply(CCommToolControlPB * inPB)
{}

void
CSerialEndpoint::handleBindReply(CCommToolBindPB * inPB)
{}

void
CSerialEndpoint::handleUnbindReply(CCommToolBindPB * inPB)
{}

void
CSerialEndpoint::handleOptMgmtReply(CCommToolOptMgmtPB * inPB)
{}


#pragma mark -

class CUnblockEvent : public CEvent
{
public:
				CUnblockEvent(void * inContext);
				~CUnblockEvent();

private:
	void *	fEventContext;
};


CUnblockEvent::CUnblockEvent(void * inContext)
{
	fEventId = 'sync';
	fEventContext = inContext;
}

CUnblockEvent::~CUnblockEvent()
{ }


#pragma mark -

CPseudoSyncState::CPseudoSyncState()
{ }

CPseudoSyncState::~CPseudoSyncState()
{ }

NewtonErr
CPseudoSyncState::init(void)
{ return CUPort::init(); }

NewtonErr
CPseudoSyncState::block(ULong inArg)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = gAppWorld->fork(NULL))
		size_t evtSize;
		CUnblockEvent unblocker(NULL);
		gAppWorld->releaseMutex();
		err = receive(&evtSize, &unblocker, sizeof(unblocker));
		gAppWorld->acquireMutex();
	}
	XENDTRY;
	return err;
}

void
CPseudoSyncState::unblock(void)
{
	CUnblockEvent unblocker(this);
	gAppWorld->releaseMutex();
	send(&unblocker, sizeof(unblocker));
	gAppWorld->acquireMutex();
}

