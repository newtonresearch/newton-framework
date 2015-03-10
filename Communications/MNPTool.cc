/*
	File:		MNPTool.cc

	Contains:	MNP communications tool.

	Written by:	Newton Research Group, 2009.
*/

#include "MNPTool.h"


/*--------------------------------------------------------------------------------
	CMNP_CCB
	Circle buffer wrapper?
--------------------------------------------------------------------------------*/

CMNP_CCB::CMNP_CCB()
{
	f10 = 0;
	f90 = 30;
	f94 = 50;
	f1C = NULL;
	fCEC = 0;
	fCF0 = 0;
	fCF4 = 0;
	fCF8 = 0;
	fD10 = 0;
	f00 = 0;
	f04 = 0;
}


NewtonErr
CMNP_CCB::init()
{
	NewtonErr err;
	XTRY
	{
		// init CXmitBufDescriptors
		for (ArrayIndex i = 0; i < kMaxNumOfLTFrames; ++i)
		{
			CXmitBufDescriptor * bufDescr = &fXmitBuf[i];
			// thread prev/next pointers in a circle
			bufDescr->fNext = &fXmitBuf[(i+1)&7];
			bufDescr->fPrev = &fXmitBuf[(i-1)&7];

			XFAIL(err = bufDescr->fBufList.init(NO))				// init buffer list
			XFAIL(err = bufDescr->fBufSegment.init(&bufDescr->fBufData, sizeof(bufDescr->fBufData)))	// create buffer
			bufDescr->fBufList.insert(&bufDescr->fBufSegment);	// add it to the list
		}
		XFAIL(err)	// catch failures within for loop

		XFAIL(err = fXmitHeaderBuf.init(NO))							// init the header-send buffer list
		XFAIL(err = fXmitHeaderSegment.init(fXmitHeaderData, 10))			// create a buffer segment
		fXmitHeaderBuf.insert(&fXmitHeaderSegment);				// add it to the list
	}
	XENDTRY;
	return err;
}


#pragma mark -

/*--------------------------------------------------------------------------------
	CMNPTool
	MNP serial communications tool.
--------------------------------------------------------------------------------*/

CMNPTool::CMNPTool(ULong inId)
	:	CFramedAsyncSerTool(inId)
{ }


CMNPTool::~CMNPTool()
{ }


size_t
CMNPTool::getSizeOf(void) const
{ return sizeof(CMNPTool); }	// do we even need this any more?


NewtonErr
CMNPTool::taskConstructor()
{
	NewtonErr err;
	if ((err = CFramedAsyncSerTool::taskConstructor()) == noErr)
	{
		f55C = 2400;
		fCCB = NULL;
		f564 = 0;
		f568 = 0;
		f58C = YES;
		f3D4.fRecvSize = KByte;
		fBufferSize = 260;
		if ((f594.init(NO)) == noErr)
			f5AC = f594.getMsgId();
	}
	return err;
}


void
CMNPTool::taskDestructor()
{
	CFramedAsyncSerTool::taskDestructor();
}


const UChar *
CMNPTool::getToolName(void) const
{ return (const UChar *)"MNP Tool"; }


void
CMNPTool::handleRequest(CUMsgToken& inMsgToken, ULong inMsgType)
{
	if (f5AC != inMsgToken.getMsgId())
		CSerTool::handleRequest(inMsgToken, inMsgType);

	else
	{
//		if (f4C == 1)	// that’s what the code says..?
//			handleTickTimer();
//		else if (f4C == 2)
//			handleXmitAbortTimer();
//		else
			completeRequest(inMsgToken, kCommErrBadCommand);
	}
}


void
CMNPTool::handleTickTimer(void)
{
	if (shouldAbort(0x04000000, noErr))
		return;

	holdAbort();

	if (fCCB->fCFC > 0)
	{
		if (--fCCB->fCFC == 0)
			retransTimeOut();
	}

	if (fCCB->fD00 > 0)
	{
		if (--fCCB->fD00 == 0)
			ackTimeOut();
	}

	if (fCCB->fD04 > 0)
	{
		if (--fCCB->fD04 == 0)
			windowTimeOut();
	}

	if (fCCB->fD08 > 0)
	{
		if (--fCCB->fD08 == 0)
			inactiveTimeOut();
	}

	if (fCCB->fD0C > 0)
	{
		if (--fCCB->fD0C == 0)
			acceptorTimeOut();
	}

	setTimer(1, 1);

	allowAbort();
}


void
CMNPTool::retransTimeOut(void)
{
	if ((fState & kToolStateConnecting) != 0)
	{
		if (fCCB->fMNPStats.fLRRetransCount <= 3)
		{
			if ((fCCB->f00 & 0x02) && f568)
				f55C = changeSpeed(f560);
			xmitLR();
		}
		else
		{
			fCCB->fDisconnectReasonCode = 1;
			if (f28 == 0)
				f28 = 11;
			startAbort(kMNPErrConnectRetryLimit);
		}
	}
	else if (fCCB->fMNPStats.fLTRetransCount >= 11)
	{
		fCCB->fDisconnectReasonCode = 4;
		if (f28 == 0)
			f28 = 11;
		startAbort(kMNPErrNotConnected);
	}
	else
	{
		CXmitBufDescriptor * xmitBuf = &fCCB->fXmitBuf[fCCB->fB1];
		if (fCCB->f00 & 0x08000000)
			fCCB->fAC = xmitBuf;
		else
			fCCB->fA4 = xmitBuf;
		fCCB->f00 |= 0x01000000;
		if (fCCB->fIsOptimised && fCCB->fMNPStats.fAdaptValue > 32)
			fCCB->fMNPStats.fAdaptValue -= 24;
		xmitLT();
	}
}


void
CMNPTool::ackTimeOut(void)
{
	if ((fState & kToolStateConnected) != 0)
		xmitLA(0x08000000);
}


void
CMNPTool::windowTimeOut(void)
{
	if ((fState & kToolStateConnected) != 0)
		xmitLA(0x08000000);
}


void
CMNPTool::inactiveTimeOut(void)
{
	fCCB->fDisconnectReasonCode = 5;
	if (f28 == 0)
		f28 = 12;
	startAbort(kMNPErrNotConnected);
}


void
CMNPTool::acceptorTimeOut(void)
{
	if (f28 == 0)
		f28 = 12;
	startAbort(kMNPErrConnectTimeout);
}


void
CMNPTool::connectStart(void)
{
	NewtonErr err;
	XTRY
	{
		XFAILIF(err = connectPreflight(), startAbort(err);)
		holdAbort();
		fCCB->fCFC = fCCB->fCF4;
		xmitLR();
		setTimer(1, 1);
		allowAbort();
	}
	XENDTRY;
}


void
CMNPTool::listenStart(void)
{
	NewtonErr err;
	XTRY
	{
		XFAILIF(err = connectPreflight(), startAbort(err);)
		holdAbort();
		fCCB->f00 |= 0x02;
		fCCB->fD0C = fCCB->fCF0;
		setTimer(1, 1);
		allowAbort();
	}
	XENDTRY;
}


NewtonErr
CMNPTool::connectPreflight(void)
{
	NewtonErr err;
	CCMOFramingParms framingOpt;
	XTRY
	{
		XFAIL(err = turnOn())	// vf150
		XFAIL(err = openAlloc())
		fState |= 0x80000000;

		getFramingCtl(&fCCB->framing);
		framingOpt.fDoHeader = YES;		// but why set these? they’re the defaults anyway
		framingOpt.fDoPutFCS = YES;
		framingOpt.fDoGetFCS = YES;
		framingOpt.fEscapeChar = chDLE;
		framingOpt.fEOMChar = chETX;
		setFramingCtl(&framingOpt);

		initConnectParms();
		resetLink();
		rcvInit();
	}
	XENDTRY;
	return err;
}


NewtonErr
CMNPTool::openAlloc(void)
{
	NewtonErr err = noErr;
	XTRY
	{
		fCCB = new CMNP_CCB;
		XFAILNOT(fCCB, err = kOSErrNoMemory;)	// not original, but surely we want to report it?
		XFAIL(err = fCCB->init())

		fCCB->f0C = f54C.fCompressionType;
#if defined(correct)
		if (fCCB->f0C & 0x08)
			XFAIL(err = V42CreateCompressVars(&fCCB->f10))	// CCompressVars
		if (fCCB->f0C & 0x02)
			XFAIL(err = MNPC5Open(&fCCB->f1C))	// CMNPClass5Vars*
#endif

		XFAIL(fCCB->fRcvdFrame.init(NO))
		XFAIL(fCCB->fC98.init(256+10))
		fCCB->fRcvdFrame.insert(&fCCB->fC98);

		XFAIL(err = fCCB->fRcvdData.allocate(kMaxNumOfLTFrames*260))
		XFAIL(err = fCCB->f40.allocate(770))
		XFAIL(err = fCCB->fTxData.allocate(260))

		fCCB->fCF0 = f584;
		fCCB->fCEC = f580;
	}
	XENDTRY;
	XDOFAIL(err)
	{
		freeCCB();
	}
	XENDFAIL;
	return err;
}


#pragma mark Transmit
/* -----------------------------------------------------------------------------
	Send a LR (Link Request) frame.

		header len		x
		frame type		1		kLRFrameType
	-constant parameter 1-
							2
	-constant parameter 2-
		type				1
		length			6
		fixed				1 0 0 0 0 255
	-framing mode-
		type				2
		length			1
		mode				2 | 3
	-max number of outstanding LT frames, k-
		type				3
		length			1
		k					x
	-max information field length, N401-
		type				4
		length			2
		max length		x x
	-data phase optimisation-
		type				8
		length			1
		facilities		x
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CMNPTool::xmitLR(void)
{
	if ((fState & kToolStateConnecting) != 0 && (fState & 0x08000000) == 0)
	{
		// we can always use the first CXmitBufDescriptor for negotiation
		CXmitBufDescriptor * bufDescr = &fCCB->fXmitBuf[0];
		CBufferList * xmitBuf = &(bufDescr->fBufList);
		xmitBuf->reset();

		// calculate the size of frame we will send
		UByte frameSize = (fCCB->fIsOptimised) ? 23 : 20;
		if (fCCB->f34 != 0 && (fCCB->f04 & 0x02) == 0)
		{
			if ((fCCB->f34 & 0x06) != 0)
				frameSize += 3;
			if ((fCCB->f34 & 0x08) != 0)
				frameSize += 6;
		}
		else if ((fCCB->f34 & 0x0E) != 0)
			frameSize += 3;

		// point to data in the CXmitBufDescriptor
		UByte * frameData = bufDescr->fBufData;

		// fill it
		frameData[0] = frameSize;
		frameData[1] = kLRFrameType;

		// Constant parameter 1
		frameData[2] = 2;

		// Constant parameter 2
		frameData[3] = 1;
		frameData[4] = 6;
		frameData[5] = 1;
		frameData[6] = 0;
		frameData[7] = 0;
		frameData[8] = 0;
		frameData[9] = 0;
		frameData[10] = 0xFF;

		// Octet-oriented framing mode
		frameData[11] = 2;
		frameData[12] = 1;
		frameData[13] = 2;

		// k
		frameData[14] = 3;
		frameData[15] = 1;
		frameData[16] = fCCB->fMaxNumOfLTFramesk;

		// N401 = 64
		frameData[17] = 4;
		frameData[18] = 2;
		frameData[19] = fCCB->fN401 & 0xFF;
		frameData[20] = fCCB->fN401 >> 8;

		ArrayIndex index = 21;
		if (fCCB->fIsOptimised)
		{
			frameData[21] = 8;		// N401 = 256 & fixed LT, LA frames
			frameData[22] = 1;
			frameData[23] = 2 | ((fCCB->f04 & 0x04) ? 1 : 0);
			index = 24;
		}

		if (fCCB->f04 & 0x02)
		{
			if (fCCB->f34 & 0x0E)
			{
				frameData[index++] = 9;
				frameData[index++] = 1;
				UByte x = 0;
				if ((fCCB->f34 & 0x08) != 0 && fCCB->f14[0] == 3)
				{
					if (fCCB->f18 == 512)
						x = 4;
					else if (fCCB->f18 == 1024)
						x = 8;
					else if (fCCB->f18 == 2048)
						x = 16;
				}
				if (fCCB->f34 & 0x04)
					x |= 2;
				if (fCCB->f34 & 0x02)
					x |= 1;
				frameData[index++] = x;
			}
		}
		else
		{
			if (fCCB->f34 & 0x06)
			{
				frameData[index++] = 9;
				frameData[index++] = 1;
				UByte x = 0;
				if (fCCB->f34 & 0x04)
					x |= 2;
				if (fCCB->f34 & 0x02)
					x |= 1;
				frameData[index++] = x;
			}
			if (fCCB->f34 & 0x08)
			{
				frameData[index++] = 14;
				frameData[index++] = 4;
				frameData[index++] = fCCB->f14[0];
				frameData[index++] = fCCB->f18 >> 8;
				frameData[index++] = fCCB->f18 & 0x00FF;
				frameData[index++] = fCCB->f14[1];
			}
		}

		union
		{
			long l;
			char c[4];
		} sp00;
		sp00.l = (fCCB->f00 & 0x02) ? f568 : f564;	// is listening
		if (sp00.l)
		{
			frameData[0] += 6;			// sic
			frameData[index++] = 0xC5;
			frameData[index++] = 6;
			frameData[index++] = 1;
			frameData[index++] = 4;
#if defined(hasByteSwapping)
			frameData[index++] = sp00.c[3];
			frameData[index++] = sp00.c[2];
			frameData[index++] = sp00.c[1];
			frameData[index++] = sp00.c[0];
#else
			frameData[index++] = sp00.c[0];
			frameData[index++] = sp00.c[1];
			frameData[index++] = sp00.c[2];
			frameData[index++] = sp00.c[3];
#endif
		}

		xmitBuf->hide(266-index, kSeekFromEnd);

		fCCB->f00 |= 0x00100000;
		xmitPostRequest(xmitBuf, YES);

		fCCB->fMNPStats.fLRRetransCount++;
		fCCB->fCFC = fCCB->fCF4;
	}
}


/* -----------------------------------------------------------------------------
	Send LT (Link Transfer) data frame.
	Frame header length and type are fixed:
		header len		2(optimised) | 4(non-optimised)
		frame type		4		kLTFrameType
	-optimised data-
		N(S)				xx		tx sequence number
	-non-optimised data-
		type				1		1 => N(S)
		length			1
		N(S)				xx		tx sequence number
	Followed by up to 256 bytes of data.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CMNPTool::xmitLT(void)
{
	//r5=fCCB->fA4;
	if (fState & 0x08000000)
		return;
	if (fCCB->fA4->f11 == 0)
		return;
	if (fCCB->fA4->f11 == 3)
	{
		if ((fCCB->f00 & 0x01000000) == 0)
			return;
		if (fCCB->fA4->fSeqNo - fCCB->fB0 < 8
		&&  fCCB->fA4->fSeqNo != fCCB->fB0)
			return;
		fCCB->fA4->f12 = 1;
		fCCB->fMNPStats.fRetransTotal++;
		if (fCCB->fA4->fSeqNo == fCCB->fB2)
			fCCB->fMNPStats.fLTRetransCount++;
	}
	else if (fCCB->fB0 - fCCB->fB1 <= fCCB->fB4)
		return;
	else
		fCCB->fB4++;

	fCCB->f00 |= 0x08000000;
	fCCB->fB8 = 0;

	CBufferList * xmitBuf = &(fCCB->fA4->fBufList);
	xmitBuf->reset();

	fCCB->fA4->fBufData[fCCB->fOffsetToLTData] = fCCB->fA4->fSeqNo;

	xmitBuf->hide(266 - (fCCB->fOffsetToLTData+1), kSeekFromEnd);
	fCCB->fA8 = 2;

	xmitPostRequest(xmitBuf, NO);

	if (fCCB->fCFC == 0)
		fCCB->fCFC = fCCB->fCF4;
}


void
CMNPTool::xmitNAck(void)
{
	if (fCCB->fD10 > 0)
	{
		if (fCCB->fD00 <= 0)
		{
			fCCB->fD00 = fCCB->fD10;
			fCCB->fMNPStats.fForceAckTotal++;
			if (fCCB->fD10 < fCCB->fCF4*2)
				fCCB->fD10 <<= 1;
			if (fCCB->fD10 > fCCB->fCF4*2)
				fCCB->fD10 = fCCB->fCF4*2;
		}
	}
	else
	{
		fCCB->fMNPStats.fForceAckTotal++;
		fCCB->fD10 = fCCB->fCF4/8;
		if (fCCB->fD10 == 0)
			fCCB->fD10 = 1;
		xmitLA(0x08000000);
	}
}


/* -----------------------------------------------------------------------------
	Send LA (Link Acknowledge) frame.
	Frame header length and type are fixed:
		header len		3(optimised) | 7(non-optimised)
		frame type		5		kLAFrameType
	-optimised data-
		N(R)				xx		rx sequence number
		N(k)				xx		rx credit number
	-non-optimised data-
		type				1		1 => N(R)
		length			1
		N(R)				xx		rx sequence number
		type				2		2 => N(k)
		length			1
		N(k)				xx		rx credit number
	Args:		inFlags
	Return:	YES => success
----------------------------------------------------------------------------- */

int
CMNPTool::xmitLA(ULong inFlags)
{
	if (fState & 0x08000000)
	{
		fCCB->f00 |= inFlags;
		return NO;
	}

	fCCB->f00 &= ~0x18000000;
	fCCB->fD00 = 0;
	if (fCCB->fMaxNumOfLTFramesk > 1)
	{
		fCCB->fD04 = (fCCB->fCF8 > fCCB->fD10) ? fCCB->fCF8 : fCCB->fD10 ;
	}
	fCCB->fRcvdFrameCount = 0;

	CBufferList * xmitBuf = &fCCB->fXmitHeaderBuf;
	xmitBuf->reset();

	ArrayIndex index;
	UChar * frameData = fCCB->fXmitHeaderData;
	if (fCCB->fIsOptimised)
	{
		frameData[0] = 3;
		frameData[1] = kLAFrameType;
		index = 2;
	}
	else
	{
		frameData[0] = 7;
		frameData[1] = kLAFrameType;
		frameData[2] = 1;	// type 1
		frameData[3] = 1;	// length
		index = 4;
	}
	frameData[index] = fCCB->fNR;

	if (fCCB->fIsOptimised)
	{
		index = 3;
	}
	else
	{
		frameData[5] = 2;	// type 2
		frameData[6] = 1;	// length
		index = 7;
	}
	frameData[index] = receiveCredit();

	if (frameData[index] == 0)
		fCCB->f00 |= 0x04000000;	// no more LT frames available
	else
		fCCB->f00 &= ~0x04000000;

	xmitBuf->hide(10-(index+1), kSeekFromEnd);
	xmitPostRequest(xmitBuf, YES);

	return YES;
}


/* -----------------------------------------------------------------------------
	Send LD (Link Disconnect) frame.
	We always send both disconnect codes.
		header len		7
		frame type		2		kLDFrameType
	-required-
		type				1		1 => reason code
		length			1
		reason			0xFF
	-optional-
		type				2		2 => user code
		length			1
		reason			0xFF
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

bool
CMNPTool::xmitLD(void)
{
	if (fCCB->f00 & 0x40000000)	// already disconnected
	{
		fState &= ~0x20000000;
		return YES;
	}

	fCCB->fIsDisconnecting = YES;

	CBufferList * xmitBuf = &fCCB->fXmitHeaderBuf;
	xmitBuf->reset();

	UChar * frameData = fCCB->fXmitHeaderData;
	frameData[0] = 7;	// length
	frameData[1] = kLDFrameType;
	frameData[2] = 1;	// type 1
	frameData[3] = 1;	// length
	frameData[4] = fCCB->fDisconnectReasonCode;
	frameData[5] = 2;	// type 2
	frameData[6] = 1;	// length
	frameData[7] = fCCB->fDisconnectUserCode;

//	xmitBuf->hide(10-(8), kSeekFromEnd);	// original doesn’t -- why not?
	xmitPostRequest(xmitBuf, YES);
	setXmitAbortTimer();

	return NO;
}


/* -----------------------------------------------------------------------------
	Set up LT (Link Transfer) data frame headers.
	-optimised data-
		length			2
		type				4		kLTFrameType
		N(S)				xx		tx sequence number
	-non-optimised data-
		length			4
		type				4		kLTFrameType
		type				1		1 => N(S)
		length			1
		N(S)				xx		rt sequence number

	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CMNPTool::initFrameBufs(void)
{
	if (fCCB->fIsOptimised)
	{
		for (ArrayIndex i = 0; i < kMaxNumOfLTFrames; ++i)
		{
			UChar * bufData = fCCB->fXmitBuf[i].fBufData;
			bufData[0] = 2;	// length
			bufData[1] = kLTFrameType;
		}
		fCCB->fOffsetToLTData = 2;
	}
	else
	{
		for (ArrayIndex i = 0; i < kMaxNumOfLTFrames; ++i)
		{
			UChar * bufData = fCCB->fXmitBuf[i].fBufData;
			bufData[0] = 4;	// length
			bufData[1] = kLTFrameType;
			bufData[2] = 1;	// type
			bufData[3] = 1;	// length
		}
		fCCB->fOffsetToLTData = 4;
	}
}


void
CMNPTool::xmitStartBuffer(void)
{
	if (fCCB->f00 & 0x00200000)
		return;
	if (f94[kCommToolPutChannel].fRequestPending || fCCB->fTxData.bufferCount() > 0)
	{
		fCCB->f00 |= 0x00200000;
		if (fCCB->fC0->f11 == 0)
			xmitInitBuffer(fCCB->fC0);
		xmitBufferLT();
	}
}


void
CMNPTool::xmitInitBuffer(CXmitBufDescriptor * inBuf)
{
	inBuf->f08 = 0;
	inBuf->fSeqNo = inBuf->fPrev->fSeqNo + 1;
	inBuf->f12 = 0;
	inBuf->f11 = 1;

	xmitLT();

	ArrayIndex adaptiveBufLen;
	if (fCCB->fIsOptimised || (adaptiveBufLen = fCCB->fMaxLTFrameLen) != 0)
	{
		if (fCCB->fMNPStats.fAdaptValue < fCCB->fMaxLTFrameLen)
			fCCB->fMNPStats.fAdaptValue++;
		if (fCCB->fMNPStats.fAdaptValue <= 32)
			adaptiveBufLen = 32;
		else if (fCCB->fMNPStats.fAdaptValue <= 64)
			adaptiveBufLen = 64;
		else if (fCCB->fMNPStats.fAdaptValue <= 128)
			adaptiveBufLen = 128;
	}
	inBuf->f0C = adaptiveBufLen;
}


void
CMNPTool::xmitBufferLT(void)
{
	for ( ; ; )
	{
		CXmitBufDescriptor * xmitBuf = fCCB->fC0;
		if (!(xmitBuf->f11 == 1 || xmitBuf->f11 == 0))
			break;

		CXmitBufDescriptor * nxBuf = xmitBuf->fNext;
		if (!(nxBuf->f11 == 0 || xmitBuf->f11 == 0 || (xmitBuf->f0C - xmitBuf->f08) >= fCCB->f38))
			break;

		UChar ch;
		NewtonErr result = fCCB->fTxData.getNextByte(&ch);
		if (result == 2)
		{
			if (fCCB->fBC != NULL)
			{
				ArrayIndex len = 0;
				result = fCCB->fTxData.copyIn(fCCB->fBC, &len);
				if (result == 5)
				{
					putComplete(0, fCCB->fBC->getSize());
					fCCB->fBC = NULL;
				}
			}
			else
			{
				fCCB->fMNPStats.fWriteFlushCount++;
//				fCCB->f20->vf2C(0);	// don’t know what the object is
				break;
			}
		}
		else
		{
			fCCB->fMNPStats.fWriteBytesIn++;
//			fCCB->f20->vf28(ch);		// don’t know what the object is
			if ((fState & kToolStateWantAbort) != 0)
				break;
		}
	}
	fCCB->f00 &= ~0x00200000;
}


void
CMNPTool::xmitPostRequest(CBufferList * inData, bool inArg2)
{
	fState |= 0x08000000;
	CSerTool::putFramedBytes(inData, inArg2);
}


void
CMNPTool::putFramedBytes(CBufferList * inData, bool inArg2)
{
	CCommTool::putComplete(kCommErrBadCommand, 0);
}


void
CMNPTool::putComplete(NewtonErr inStatus, size_t inPutCount)
{
	if (fState & 0x08000000)
	{
		fDataToSend = NULL;
		if (fCCB->fIsDisconnecting)
		{
			fCCB->fIsDisconnecting = NO;
			xmitLDComplete(inStatus, inPutCount);
		}
		else
		{
			holdAbort();
			xmitFrameComplete(inStatus, inPutCount);
			allowAbort();
		}
	}

	else
		CSerTool::putComplete(inStatus, inPutCount);
}


void
CMNPTool::MNPDecompressOut(UByte inByte)
{
	fCCB->f40.putNextByte(inByte);
}

#pragma mark Receive


void
CMNPTool::rcvStartBuffer(void)
{
	fCCB->f00 &= ~0x20;
	if ((fCCB->f00 & 0x20000000) == 0  &&  fCCB->fC68 != 0)
	{
		fCCB->f00 |= 0x20000000;
		rcvBuffer();
	}
}

void
CMNPTool::rcvBuffer(void)
{
	UByte ch;	// sp00
//	r6 = 1;
//	r7 = 0;

	ArrayIndex r5 = fCCB->f40.bufferCount();
	if (fCCB->fC70 <= r5)
	{
		if (fCCB->fC70 != 0)
			fCCB->f40.copyOut(fCCB->fC68, &fCCB->fC70);
		fCCB->fMNPStats.fReadBytesOut += fCCB->fC6C;
		getComplete(noErr, NO, fCCB->fC6C);
		fCCB->fC68 = 0;
		fCCB->fC6C = 0;
		fCCB->fC70 = 0;
	}
	else
	{
		if (fCCB->f40.bufferSpace() <= fCCB->f3C)
			fCCB->f40.copyOut(fCCB->fC68, &fCCB->fC70);
		if (fCCB->fRcvdData.getNextByte(&ch) == 2)
		{
			;
		}
		else
		{
			;
		}
	}
	;

L8B7C:
	fCCB->f00 &= ~0x20000000;
	if ((fCCB->f00 & 0x04000000) != 0)
	{
		if (receiveCredit())
			xmitLA(0x08000000);
	}
}


void
CMNPTool::rcvInit(void)
{
	fCCB->fD10 = 0;
	fCCB->fC5C = 1;
	fCCB->fRcvdData.flushBytes();
	flushInputBytes();
	rcvFrame();
}

void
CMNPTool::rcvFrame(void)
{
	if ((fState & kToolStateWantAbort) == 0)
	{
		fCCB->fRcvdFrame.reset();
		fState |= 0x40000000;
		CSerTool::getFramedBytes(&fCCB->fRcvdFrame);
	}
}

void
CMNPTool::getFramedBytes(CBufferList * inBuffers)
{
	CCommTool::getComplete(kCommErrBadCommand);
}


void
CMNPTool::getComplete(NewtonErr inStatus, bool inEndOfFrame, size_t inGetCount)
{
	if (fState & 0x40000000)
	{
		fDataRcvd->hide(fNumOfBytesToRecv, kSeekFromEnd);
		fDataRcvd = NULL;

		holdAbort();
		rcvFrameComplete(inStatus, inEndOfFrame);
		allowAbort();
	}

	else
		CSerTool::getComplete(inStatus, inEndOfFrame, inGetCount);
}

void
CMNPTool::rcvFrameComplete(NewtonErr inErr, bool inEndOfFrame)
{
	NewtonErr err = noErr;
	bool isFrameOK = YES;

	if (inErr == kSerErrCRCError)
		isFrameOK = NO;
	else if (inErr == kSerErrAsyncError)
	{
		isFrameOK = NO;
		fCCB->fMNPStats.fRcvAsyncErrTotal++;
	}
	else if (inErr)
		err = inErr;
	else if (!inEndOfFrame)
		isFrameOK = NO;

	if (shouldAbort(0x40000000, err))
		return;

	if (isFrameOK)
		rcvProcessFrame();
	else
		rcvBrokenFrame(inErr);

	rcvFrame();

	if (fCCB->f00 & 0x10)
		processLA();
	if (fCCB->f00 & 0x20)
		rcvStartBuffer();
}

void
CMNPTool::rcvProcessFrame(void)
{
	fCCB->fMNPStats.fFramesRcvd++;

	fCCB->fD08 = fCCB->fCEC;
	if ((fState & kToolStateConnecting) != 0)
		fState |= 0x20000000;

	fCCB->fRcvdFrame.seek(0, kSeekFromBeginning);
	fCCB->fRcvdFrameLen = fCCB->fRcvdFrame.get();
	fCCB->fRcvdFrameType = fCCB->fRcvdFrame.get();
	switch(fCCB->fRcvdFrameType)
	{
	case kLRFrameType:
		rcvLR();
		break;
	case kLDFrameType:
		rcvLD();
		break;
	case kLTFrameType:
		rcvLT();
		break;
	case kLAFrameType:
		rcvLA();
		break;
	case kLNFrameType:
		rcvLN();
		break;
	case kLNAFrameType:
		rcvLNA();
		break;
	default:
		fCCB->fDisconnectReasonCode = 0xFE;
		if (f28 == 0)
			f28 = 14;
		startAbort(kMNPErrNotConnected);
		break;
	}
}

void
CMNPTool::rcvBrokenFrame(NewtonErr inErr)
{
	fCCB->fMNPStats.fRcvBrokenTotal++;

	if ((fState & kToolStateConnecting) != 0)
	{
		fState |= 0x20000000;
		if ((fCCB->f00 & 0x02) == 0)	// not listening
		{
			if (fCCB->fMNPStats.fLRRetransCount <= kMaxNumOfLRRetrans)
				xmitLR();
			fCCB->fDisconnectReasonCode = 4;	// byte
			if (f28 == 0)
				f28 = 0x11;
			startAbort(kMNPErrConnectRetryLimit);
		}
	}

	else if ((fState & kToolStateConnected) != 0)
		xmitNAck();
}


void
CMNPTool::rcvLR(void)
{
	if ((fState & kToolStateConnecting) != 0)
	{
		if (fCCB->f00 & 0x02)	// is listening?
		{
			if (fCCB->f00 & 0x01)
				retransTimeOut();
			else
			{
				fCCB->fD0C = 0;
				if (paramNegotiation(YES))
					listenComplete(noErr);
				else
				{
					if (f28 == 0)
						f28 = 20;
					startAbort(kMNPErrNegotiationFailure);
				}
			}
		}
		else
		{
			if (paramNegotiation(NO))
			{
				if (f568 != 0)
					f55C = changeSpeed(f568);
				enterConnectedState();
				xmitLA(0x08000000);
			}
			else
			{
				if (f28 == 0)
					f28 = 20;
				startAbort(kMNPErrNegotiationFailure);
			}
		}
	}

	else if ((fState & kToolStateConnected) != 0)
		xmitNAck();
}


/* -----------------------------------------------------------------------------
	Negotiate parameters in LR (Link Request) frame.
	We always send both disconnect codes.
		header len		x
		frame type		1		kLRFrameType
	-constant parameter 1-
							2
	-constant parameter 2-
		type				1
		length			6
		fixed				1 0 0 0 0 255
	-framing mode-
		type				2
		length			1
		mode				2 | 3
	-max number of outstanding LT frames, k-
		type				3
		length			1
		k					x
	-max information field length, N401-
		type				4
		length			2
		max length		x x
	-data phase optimisation-
		type				8
		length			1
		facilities		x
	Args:		inArg		isListening
	Return:	YES => success
----------------------------------------------------------------------------- */

bool
CMNPTool::paramNegotiation(bool inArg)
{
	bool isOK = YES;	// r5
	int sp00;
	ULong n401;								//sp04
	int sp08 = 0;
	UChar framingMode;					//sp0C
	UChar optimisation;					//sp10
	bool hasOptimisationParam = NO;	//sp14
	bool hasN401Param = NO;				//sp18
	bool haskParam = NO;					//sp1C
	bool hasFramingModeParam = NO;	//sp20
	int sp24;

	fCCB->fRcvdFrame.get();	// ignore constant parameter 1

	UChar param;
	int codeType, codeLen;
	while ((codeType = fCCB->fRcvdFrame.get()) != -1 && isOK)
	{
		codeLen = fCCB->fRcvdFrame.get();
		switch (codeType)
		{
		case 1:	// constant parameter 2
			// ignore it
			fCCB->fRcvdFrame.seek(codeLen, kSeekFromHere);	// original uses fixed length of 6
			break;

		case 2:	// octet-oriented framing mode
			param = fCCB->fRcvdFrame.get();
			if (param > 2)
				param = 2;
			framingMode = param;
			hasFramingModeParam = YES;
			break;

		case 3:	// max number of outstanding LT frames, k
			param = fCCB->fRcvdFrame.get();
			if (param < fCCB->fMaxNumOfLTFramesk)
				fCCB->fMaxNumOfLTFramesk = param;
			haskParam = YES;
			break;

		case 4:	// max information field length, N401
			n401 = fCCB->fRcvdFrame.get();
			n401 = (fCCB->fRcvdFrame.get() << 8) + n401;
			if (n401 < fCCB->fN401)
				fCCB->fN401 = n401;
			hasN401Param = YES;
			fCCB->fMaxLTFrameLen = fCCB->fN401;
			break;

		case 8:	// data phase optimisation
			optimisation = fCCB->fRcvdFrame.get();
			hasOptimisationParam = YES;
			break;

		case 9:
			;
			break;

		case 14:
			;
			break;

		case 197:	// 0xC5
			;
			break;

		default:
			// ignore this param
			fCCB->fRcvdFrame.seek(codeLen, kSeekFromHere);
			if (!inArg)
				// connecting entity is trying to negotiate a param we don’t support
				isOK = NO;
			break;
		}
	}
//	r7 = 1;
//	r6 = 0;
	if (hasFramingModeParam && framingMode == 2)
//001181EC
	{
		if (isOK)
		{
			if (!haskParam)
				fCCB->fMaxNumOfLTFramesk = 1;
			fCCB->fB0 = fCCB->fB1 + fCCB->fMaxNumOfLTFramesk;

			if (hasN401Param)
			{
				if (hasOptimisationParam && (optimisation & 0x01) != 0)	// bit 1 => max info field length of 256
				{
					if (fCCB->fN401 == 0x40)
						fCCB->fMaxLTFrameLen = 256;
					else
						fCCB->f04 &= ~0x04;
				}
			}
			else
			{
				fCCB->fN401 =
				fCCB->fMaxLTFrameLen = 260;
				fCCB->f04 &= ~0x04;
			}

			if (hasOptimisationParam)	// bit 2 => fixed field LT and LA frames
			{
				if ((optimisation & 0x02) == 0)
					fCCB->fIsOptimised = NO;
				if ((fCCB->f04 & 0x04) == 0)
					fCCB->fMNPStats.fAdaptValue = fCCB->fMaxLTFrameLen;
			}
			else
				fCCB->fIsOptimised = NO;
//001182B8
			// compression stuff
		}
	}
	else
		isOK = 0;

//00118484
	fCCB->fDisconnectReasonCode = 3;		// => kMNPErrNegotiationFailure
	if (fControlOptionInfo.fOptions)
	{
		COptionIterator iter(fControlOptionInfo.fOptions);
		CCMOMNPCompression * option;
		if ((option = (CCMOMNPCompression *)iter.findOption(kCMOMNPCompression)) != NULL)
		{
			option->fCompressionType = sp24;
			option->setOpCodeResult(fCCB->f0C == fCCB->f34 ? opSuccess : opFailure);
		}
	}
	fConnectInfo.fErrorFree = YES;
	fConnectInfo.fSupportsCallBack = YES;
	fConnectInfo.fViaAppleTalk = NO;
	fConnectInfo.fConnectBitsPerSecond = (fCCB->f34 == 10) ? f55C : (f55C * 3) / 2;

	return isOK;
}


void
CMNPTool::enterConnectedState(void)
{
	fCCB->fCFC = 0;
	fCCB->fMNPStats.fLRRetransCount = 0;
	fCCB->fD08 = fCCB->fCEC;
	if (fCCB->fMaxNumOfLTFramesk > 1)
		fCCB->fD04 = fCCB->fCF8;

	initFrameBufs();
	setRetransTimer();

	if (fCCB->f00 & 0x02)	// is listening?
	{
		fCCB->f00 &= ~0x03;
		acceptComplete(noErr);
	}
	else
	{
		connectComplete(noErr);
	}
}


/* -----------------------------------------------------------------------------
	Receive LD (Link Disconnect) frame.
	Frame header length and type have already been read from the stream.
		header len		4 | 7
		frame type		2		kLDFrameType
	-required-
		type				1		1 => reason code
		length			1
		reason			0xFF
	-optional-
		type				2		2 => user code
		length			1
		reason			0xFF
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CMNPTool::rcvLD(void)
{
	// read codes
	int codeType, codeLen;
	while ((codeType = fCCB->fRcvdFrame.get()) != -1)
	{
		codeLen = fCCB->fRcvdFrame.get();
		if (codeType == 1)		// reason code
			fCCB->fDisconnectReasonCode = fCCB->fRcvdFrame.get();
		else if (codeType == 2)	// (optional) user code
			fCCB->fDisconnectUserCode = fCCB->fRcvdFrame.get();
		else
			fCCB->fRcvdFrame.seek(codeLen, kSeekFromHere);
	}

	NewtonErr reason = kMNPErrNotConnected;
	fCCB->f00 |= 0x40000000;
	if (f28 == 0)
	{
		switch (fCCB->fDisconnectReasonCode)
		{
		case 1:		// Protocol establishment phase error, LR expected but not received
			f28 = 2;
			reason = kMNPErrHandshakeFailure;
		case 2:		// LR constant parameter 1 contains an unexpected value
			f28 = 3;
			reason = kMNPErrIncompatibleProtLvl;
		case 3:		// LR received with incompatible or unknown parameter value
			f28 = 4;
			reason = kMNPErrNegotiationFailure;
		case 4:		// reserved...
			f28 = 5;
		case 5:
			f28 = 6;
		case 6:
			f28 = 7;
		case 0xFE:	// ...reserved
			f28 = 8;
		case 0xFF:	// User-initiated disconnect
			f28 = 9;
		default:
			f28 = 1;
		}
	}
	startAbort(reason);
}


/* -----------------------------------------------------------------------------
	Receive LT (Link Transfer) frame.
	Frame header length and type have already been read from the stream.
		header len		2(optimised) | 4(non-optimised)
		frame type		4		kLTFrameType
	-optimised data-
		N(S)				xx		tx sequence number
	-non-optimised data-
		type				1		1 => N(S)
		length			1
		N(S)				xx		tx sequence number
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CMNPTool::rcvLT(void)
{
	if (((fState & kToolStateConnecting) != 0) && ((fCCB->f00 & 0x01) != 0))
	{
		enterConnectedState();
		xmitLT();
	}

	if ((fState & kToolStateConnected) != 0)
	{
		UChar NS;	// r6
		if (fCCB->fIsOptimised)
			NS = fCCB->fRcvdFrame.get();
		else
		{
			//r7=0,r5=1
			bool hasNS = NO;
			ArrayIndex i = 1;
			int codeType, codeLen;
			while (i <= fCCB->fRcvdFrameLen)
			{
				codeType = fCCB->fRcvdFrame.get();
				++i;
				codeLen = fCCB->fRcvdFrame.get();
				i += codeLen;
				if (codeType == 1)
				{
					NS = fCCB->fRcvdFrame.get();
					hasNS = YES;
				}
				else
					fCCB->fRcvdFrame.seek(codeLen, kSeekFromHere);
			}
			if (!hasNS)
			{
				fCCB->fDisconnectReasonCode = 0xFE;
				if (f28 == 0)
					f28 = 14;
				startAbort(kMNPErrNotConnected);
			}
		}
		fCCB->fRcvdFrame.hide(fCCB->fRcvdFrameLen+1, kSeekFromBeginning);
		fCCB->fRcvdInfoFieldLen = fCCB->fRcvdFrame.getSize();
		UChar expectedSeqNo = fCCB->fNR + 1;
		if (NS != expectedSeqNo)
		{
			// out-of-sequence frame
			if (NS == fCCB->fNR & (fCCB->f00 & 0x04) == 0)
				// resend of previous frame
				fCCB->f00 |= 0x04;
			else
			{
				fCCB->f00 &= ~0x04;
				xmitNAck();
			}
		}
		else if (receiveCredit() == 0)
		{
			// no more LT frames available
			fCCB->f00 &= ~0x04;
			xmitNAck();
		}
		else if (fCCB->fRcvdInfoFieldLen >= fCCB->fMaxLTFrameLen)
		{
			// wacko frame len
			fCCB->fDisconnectReasonCode = 0xFE;
			if (f28 == 0)
				f28 = 14;
			startAbort(kMNPErrNotConnected);
		}
		else
		{
			// the frame is good
			fCCB->f00 &= ~0x04;
			fCCB->fNR++;
			fCCB->fRcvdFrameCount++;

			if (fCCB->fD10 != 0)
			{
				fCCB->fD10 = 0;
				if (fCCB->fMaxNumOfLTFramesk > 1)
					fCCB->fD04 = fCCB->fCF8;
			}

			fCCB->fMNPStats.fBytesRcvd += fCCB->fRcvdInfoFieldLen;
			fCCB->fRcvdData.copyIn(&fCCB->fRcvdFrame, &fCCB->fRcvdInfoFieldLen);

			if (fCCB->fRcvdFrameCount >= fCCB->fMaxNumOfLTFramesk)
				xmitLA(0x08000000);
			else
			{
				if (xmitLA(0x10000000) == 0
				&& fCCB->fD00 == 0)
					fCCB->fD00 = fCCB->fCF4 >> 1;
			}

			fCCB->f00 |= 0x20;
		}
	}
}


/* -----------------------------------------------------------------------------
	Return N(k) -- maximum number of LT frames that can be received.
	Args:		--
	Return:	N(k)
----------------------------------------------------------------------------- */

ArrayIndex
CMNPTool::receiveCredit(void)
{
	ArrayIndex numOfFramesFree = fCCB->fRcvdData.bufferSpace() / fCCB->fMaxLTFrameLen;
	return MIN(fCCB->fMaxNumOfLTFramesk, numOfFramesFree);
}


/* -----------------------------------------------------------------------------
	Receive LA (Link Acknowledge) frame.
	Frame header length and type have already been read from the stream.
		header len		3(optimised) | 7(non-optimised)
		frame type		5		kLAFrameType
	-optimised data-
		N(R)				xx		rx sequence number
		N(k)				xx		rx credit number
	-non-optimised data-
		type				1		1 => N(R)
		length			1
		N(R)				xx		rx sequence number
		type				2		2 => N(k)
		length			1
		N(k)				xx		rx credit number
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CMNPTool::rcvLA(void)
{
	if (fCCB->f00 & 0x01)
	{
		enterConnectedState();
		xmitLT();
	}

	if ((fState & kToolStateConnected) != 0)
	{
		if (fCCB->fIsOptimised)
		{
			fCCB->fAckNR = fCCB->fRcvdFrame.get();
			fCCB->fAckNk = fCCB->fRcvdFrame.get();
		}
		else
		{
			bool hasNk = NO;
			bool hasNR = NO;
			int codeType, codeLen;
			while ((codeType = fCCB->fRcvdFrame.get()) != -1)
			{
				codeLen = fCCB->fRcvdFrame.get();
				if (codeType == 1)
				{
					fCCB->fAckNR = fCCB->fRcvdFrame.get();
					hasNR = YES;
				}
				else if (codeType == 2)
				{
					fCCB->fAckNk = fCCB->fRcvdFrame.get();
					hasNk = YES;
				}
				else
					fCCB->fRcvdFrame.seek(codeLen, kSeekFromHere);
			}
			if (!(hasNR && hasNk))
			{
				fCCB->fDisconnectReasonCode = 0xFE;
				if (f28 == 0)
					f28 = 14;
				startAbort(kMNPErrNotConnected);
			}
		}
		
		fCCB->f00 |= 0x10;
	}
}


void
CMNPTool::processLA(void)
{
	fCCB->f00 &= ~0x10;
	;
	;
	;
	fCCB->fB0 = fCCB->fAckNk + fCCB->fB1;
	if (fCCB->fAckNk != 0 && fCCB->fA4->f11 != 0 && (fState & 0x08000000) == 0)
		xmitLT();
//	if (r5)
//		xmitStartBuffer();
}


/* -----------------------------------------------------------------------------
	Receive LN (Link Attention) frame.
	We don’t do Link Attention.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CMNPTool::rcvLN(void)
{ /* this really does nothing */ }


/* -----------------------------------------------------------------------------
	Receive LNA (Link Attention Acknowledge) frame.
	We don’t do Link Attention.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CMNPTool::rcvLNA(void)
{ /* this really does nothing */ }


/*--------------------------------------------------------------------------------
	These data items are for interest only -- copied from the CDIL implementation.
--------------------------------------------------------------------------------*/

const unsigned char kFrameStart[] = { 0x16, 0x10, 0x02 };

const unsigned char kFrameEnd[] = { 0x10, 0x03 };

const unsigned char kLRFrame[] =
{
	23,		/* Length of header */
	kLRFrameType,
	/* Constant parameter 1 */
	0x02,
	0x01, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0xFF,
	/* Constant parameter 2 */
	0x02, 0x01, 0x02,			/* Octet-oriented framing mode */
	0x03, 0x01, 0x01,			/* k = 1 */
	0x04, 0x02, 0x40, 0x00,	/* N401 = 64 */
	0x08, 0x01, 0x03			/* N401 = 256 & fixed LT, LA frames */
};

const unsigned char kLDFrame[] =
{
	4,			/* Length of header */
	kLDFrameType,
	0x01, 0x01, 0xFF
};

const unsigned char kLTFrame[] =
{
	2,			/* Length of header */
	kLTFrameType,
	0			/* Sequence number */
};

const unsigned char kLAFrame[] =
{
	3,			/* Length of header */
	kLAFrameType,
	0,			/* Sequence number */
	1			/* N(k) = 1 */
};
