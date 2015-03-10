/*
	File:		MNPService.cc

	Contains:	MNP communications tool.

	Written by:	Newton Research Group, 2008.
*/

#include "MNPService.h"
#include "SerialTool.h"
#include "SerialOptions.h"
#include "CircleBuf.h"

/*--------------------------------------------------------------------------------
	CMNPService
	Communications Manager service implementation for the MNP Serial tool.
	TO DO		assembler glue
--------------------------------------------------------------------------------*/

PROTOCOL_IMPL_SOURCE_MACRO(CMNPService)

void
CMNPService::make(void)
{ }

void
CMNPService::destroy(void)
{ }

NewtonErr
CMNPService::start(COptionArray * inOptions, ULong inId, CServiceInfo * info)
{
	NewtonErr err;
	XTRY
	{
		CMNP	mnpTool(inId);
		XFAIL(err = StartCommTool(&mnpTool, inId, info))
		err = OpenCommTool(info->f04, inOptions, this);
	}
	XENDTRY;
	return err;
}

NewtonErr
CMNPService::doneStarting(CEvent * inEvent, size_t inSize, CServiceInfo * info)
{
	return ((CEndpointEvent *)inEvent)->f08;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	CSerTool
--------------------------------------------------------------------------------*/

void
CSerTool::putBytes(CBufferList * inData)
{
	f278 = NO;
	f279 = NO;
	startOutput(inData);
}

void
CSerTool::putFramedBytes(CBufferList * inData, BOOL inArg2)
{
	f278 = inArg2;
	f279 = YES;
	startOutput(inData);
}

void
CSerTool::startOutput(CBufferList * inData)
{
	if (f28E)
	{
		f270 = inData;
		inData->seek(0, kSeekFromBeginning);
		f274 = inData->getSize();
		f27A = YES;
		doOutput();
	}

	else
		putComplete(kSerErrToolNotReady, 0);
}

void
CSerTool::putComplete(NewtonErr inStatus, ULong inCount)
{
	f270 = nil;
	CCommTool::putComplete(inStatus, inCount);
}

void
CSerTool::getBytes(CBufferList * outData)
{
	f288 = NO;
	f289 = NO;
	startInput(outData);
}

void
CSerTool::getBytesImmediate(CBufferList * outData, size_t inThreshold)
{
	f288 = NO;
	f289 = YES;
	f284 = inThreshold;
	startInput(outData);
}

void
CSerTool::getFramedBytes(CBufferList * outData)
{
	f288 = YES;
	f289 = NO;
	startInput(outData);
}

void
CSerTool::startInput(CBufferList * outData)
{
	f27C = outData;
	outData->seek(0, kSeekFromEnd);
	f280 = outData->getSize();
	if (f28E)
		doInput();
	else
		getComplete(kSerErrToolNotReady);
}

void
CSerTool::getComplete(NewtonErr inStatus, BOOL inEndOfFrame, size_t inGetBytesCount)
{
	f27C->hide(f280, kSeekFromEnd);
	f27C = nil;
	CCommTool::getComplete(inStatus, inEndOfFrame, inGetBytesCount);
}

void
CCommTool::getComplete(NewtonErr inStatus, BOOL inEndOfFrame, size_t inGetBytesCount)
{
	CCommToolGetReply reply;
	reply.fGetBytesCount = inGetBytesCount;
	if (inEndOfFrame)
		reply.fEndOfFrame = YES;
	completeRequest(0, processOptionsCleanUp(inStatus, &fOptionInfo_2), &reply);
}

#pragma mark -

/*--------------------------------------------------------------------------------
	CAsyncSerTool
--------------------------------------------------------------------------------*/

CAsyncSerTool::CAsyncSerTool(ULong inId)
	:	CSerTool(inId)
{ }

CAsyncSerTool::~CAsyncSerTool()
{ }	// both CCircleBufs have destructors

void
CAsyncSerTool::txDataSent(void)
{
	if (f270 != nil && f384.bufferCount() != 0)
		doOutput();
}

void
CAsyncSerTool::rxDataAvailable(void)
{
	if (f27C != nil)
		doInput();
}

void
CAsyncSerTool::bytesAvailable(size_t & outCount)
{
	*outCount = f3AC.bufferCount();
}

void
CAsyncSerTool::flushInputBytes(void)
{
	if (f3EC == 2)
		f304->rxDMAControl(0x14);	// CSerialChip
	f3AC.flushBytes();
}

void
CAsyncSerTool::flushOutputBytes(void)
{
	EnterFIQAtomic();
	size_t numOfBytesFlushed = f384.bufferCount();
	if (f3F0 == 2 || f3F0 == 3)
	{
		if (f304)
			f304->txDMAControl(0x02);
		f3F0 = 1;
	}
	f384.flushBytes();
	ExitFIQAtomic();
	return numOfBytesFlushed;
}

void
CAsyncSerTool::doOutput(void)
{
	if (f482)
	{
		setTxDTransceiverEnable(YES);
		f482 = NO;
	}

	NewtonErr status;
	if ((status = fillOutputBuffer()) == noErr)
	{
		f498 |= 0x20000000;
		if (f27A)
		{
			f27A = NO;
			if (f292)
				f304->configureForOutput(YES);
			startOutputST();
		}
		else
			continueOutputST(YES);
	}

	else if (status == 5)
		doPutComplete(0);

	else
		doPutComplete(status);
}

void
CAsyncSerTool::doPutComplete(NewtonErr inStatus)
{
	f498 &= ~0x20000000;
	if (f292)
		f304->configureForOutput(NO);
	putComplete(inStatus, f270->position());
}

NewtonErr
CAsyncSerTool::fillOutputBuffer(void)
{
	if (f274 == 0)
		return 5;

	if (f454)
		f384.reset();
	return f384.copyIn(f270, &f274);
}

NewtonErr
CAsyncSerTool::emptyInputBuffer(ULong * outArg)
{
	NewtonErr status;
	if (f289)
	{
		ULong sp00 = f280;
		status = f3AC.copyOut(f27C, &sp00, outArg);
		if (status == 0 && f284 <= f280 - sp00)
			status = 6;
		f280 -= sp00;
		f284 -= sp00;
	}
	else
	{
		status = f3AC.copyOut(f27C, &f280, outArg);
		if (status == 1 && *outArg != 1000)
			status = kSerErrAsyncError;
	}
	return status;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	CFramedAsyncSerTool
--------------------------------------------------------------------------------*/

NewtonErr
CFramedAsyncSerTool::fillOutputBuffer(void)
{
	if (f279)
	{
		NewtonErr status = noErr;
		UByte ch;		// sp00
		size_t * sp04 = f274;
		BOOL r10;
		size_t r5 = 0;
//		r9 = &f4D4
//		r8 = &f4C4
//		r7 = &f384
		XTRY
		{
			for (r10 = YES; r10; )
			{
				switch(f4B4)
				{
				case 0:
					if (f528.fDoHeader)
					{
						f384.putNextByte(chSYN);
						f384.putNextByte(chDLE);
						f384.putNextByte(chSTX);
						r5 += 3;
					}
					f4C4.reset();
					f4CE = NO;
					f4B4 = 1;
					break;

				case 1:
					if (f4CE)	// isStacked
					{
						f4CE = NO;
						ch = f4CD;
					}
					else
					{
						if (f4D4.getNextByte(&ch) != 0)
						{
							if (f274 != 0)		// •• NOT RIGHT ••
								XFAILIF(status = f4D4.copyIn(f270, sp04), f4B4 = 8; if (status == 5) status = kSerErrInternalError;)
							else if (f278)
								f4B4 = 3;
							else
								r10 = NO;	// not in the original, but gotta break the loop somehow
						}
					}
					XFAILIF(f384.putNextByte(ch), f4CE = YES; f4CD = ch;)
					if (f528.fDoPutFCS)
						f4C4.computeCRC(ch);
					r5++;
					if (ch == f528.fEscapeChar)
						f4B4 = 2;
					break;

				case 2:
					XFAIL(status = f384.putNextByte(f528.fEscapeChar))	// escape frame-end-char by duplicating it
					r5++;
					f4B4 = 1;
					break;

				case 3:
					ch = f528.fEscapeChar;
					XFAIL(status = f384.putNextByte(ch))
					r5++;
					f4B4 = 4;
					break;

				case 4:
					ch = f528.fEOMChar;
					XFAIL(status = f384.putNextByte(ch))
					r5++;
					if (f528.fDoPutFCS)
					{
						f4C4.computeCRC(ch);
						f4B4 = 5;
					}
					else
						f4B4 = 7;
					break;

				case 5:
					f4C4.get();
					XFAIL(status = f384.putNextByte(f4C4.crc16[1]))
					r5++;
					f4B4 = 6;
					break;

				case 6:
					f4C4.get();
					XFAIL(status = f384.putNextByte(f4C4.crc16[0]))
					r5++;
					f4B4 = 7;
					break;

				case 7:
				case 8:
					f4B4 = 0;
					if (f274 == 0)
						XFAIL(status = 5)
					break;
				}
				XFAIL(status)
			}
		}
		XENDTRY;

		if (status == 3)
			status = 0;
		else if (r5 == 0 && f274 == 0 && status == 0)
			status = 5;

		return status;
	}

	else
		return CAsyncSerTool::fillOutputBuffer();
}


NewtonErr
CFramedAsyncSerTool::emptyInputBuffer(ULong * outArg)
{
	if (f288)
	{
		NewtonErr status = noErr;		// r6
		UByte ch;				// sp00
		BOOL r10;
//		r2 = sp04 = &f280;
//		r7 = &f3AC;
//		r9 = &f4BC;
//		r8 = &f4FC;
		XTRY
		{
			for (r10 = YES; r10; )
			{
				switch (f4B0)
				{
				case 0:
					f4CC = NO;
					f4D0 = NO;
					f4BC.reset();
					if (f528.fDoHeader)
					{
						do
						{
							XFAIL(status = f3AC.getNextByte(&ch, outArg))
							if (ch == chSYN)
								f4B0 = 1;
							else
								f548++;
						} while (ch != chSYN);
						XFAIL(status)
					}
					else
						f4B0 = 3;
					break;

				case 1:
					XFAIL(status = f3AC.getNextByte(&ch, outArg))
					if (ch == chDLE)
						f4B0 = 2;
					else
					{
						f4B0 = 0;
						f548 += 2;
					}
					break;

				case 2:
					XFAIL(status = f3AC.getNextByte(&ch, outArg))
					if (ch == chSTX)
						f4B0 = 3;
					else
					{
						f4B0 = 0;
						f548 += 3;
					}
					break;

				case 3:
					if (f4D0)	// fIsGetCharStacked
					{
						f4D0 = NO;
						ch = f4CF;
						status = 0;
					}
					else
						XFAIL(status = f3AC.getNextByte(&ch, outArg))
					if (!f4CC && ch == f528.fEscapeChar)
						f4B0 = 4;
					else
					{
						if ((status = f4FC.putNextByte(ch)) == 0)
						{
							if (f528.fDoGetFCS)
								f4BC.computeCRC(ch);
							f4CC = NO;
						}
						else
						{
							f4D0 = YES;
							f4CF = ch;
							XFAIL(f4FC.copyOut(f27C, &f280, NULL))
						}
					}
					break;

				case 4:
					XFAIL(status = f3AC.getNextByte(&ch, outArg))
					if (ch == f528.fEOMChar)
					{
						if (f528.fDoGetFCS)
						{
							f4BC.computeCRC(ch);
							f4B0 = 5;
						}
						else
							f4B0 = 7;
					}
					else if (ch == f528.fEscapeChar)
					{
						f4D0 = YES;
						f4CF = ch;
						f4B0 = 3;
						f4CC = YES;
					}
					else
						f4B0 = 3;

					break;

				case 5:
					XFAIL(status = f3AC.getNextByte(&ch, outArg))
					f4BC.get();
					if (ch == f4BC.crc16[1])
						f4B0 = 6;
					else
					{
						f4B0 = 0;
						f4FC.flushBytes();
						status = kSerErrCRCError;
					}
					break;

				case 6:
					XFAIL(status = f3AC.getNextByte(&ch, outArg))
					f4BC.get();
					if (ch == f4BC.crc16[0])
						f4B0 = 7;
					else
					{
						f4B0 = 0;
						f4FC.flushBytes();
						status = kSerErrCRCError;
					}
					break;

				case 7:
					XFAIL((status = f4FC.copyOut(f27C, &f280, NULL)) == 6)
					f4B0 = 0;
					if (status == 0)
						status = 1000;
					break;
				}
				XFAIL(status)
			}
		}
		XENDTRY;

		if (status == 1)
		{
			f4B0 = 0;
			f4FC.flushBytes();
			status = kSerErrAsyncError;
		}
		else if (status == 2)
			status = 0;

		return status;
	}

	else
		return CAsyncSerTool::emptyInputBuffer(outArg);
}


#pragma mark -

/*--------------------------------------------------------------------------------
	CMNP_CCB
	Circle buffer wrapper?
--------------------------------------------------------------------------------*/

CMNP_CCB::CMNP_CCB()
{
	f10 = 0;
	f90 = 30;
	f94 = 50;
	f1C = 0;
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
		for (int i = 0; i < 8; i++)
		{
			CXmitBufDescriptor * txBufDescr = &fC4[i];
			// thread prev/next pointers in a circle
			txBufDescr->fNext = &fC4[(i+1)&7];
			txBufDescr->fPrev = &fC4[(i-1)&7];

			XFAIL(err = txBufDescr->fQueue.init(0))				// init buffer list
			XFAIL(err = txBufDescr->fBuffer.init(&txBufDescr->fBuf, sizeof(txBufDescr->fBuf)))	// create buffer
			txBufDescr->fQueue.insertLast(&txBufDescr->fBuffer);	// add it to the list
		}
		XFAIL(err)	// catch failures within for loop

		XFAIL(err = fC2C.init(0))							// init the (disconnect?) buffer list
		XFAIL(err = fC04.init(fC4C, sizeof(fC4C)))	// create a buffer segment
		fC2C.insertLast(&fC04);								// add it to the list
	}
	XENDTRY;
	return err;
}


#pragma mark -

/*--------------------------------------------------------------------------------
	CMNP
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


/*--------------------------------------------------------------------------------
	CRC16
--------------------------------------------------------------------------------*/

static const UShort kCRC16LoTable[16] =
{
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440
};

static const UShort kCRC16HiTable[16] =
{
	0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
	0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
};


/*--------------------------------------------------------------------------------
	Reset the CRC.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CRC16::reset(void)
{
	workingCRC = 0;
}


/*--------------------------------------------------------------------------------
	Add character into CRC computation.
	Args:		inChar
	Return:	--
--------------------------------------------------------------------------------*/

void
CRC16::computeCRC(UByte inChar)
{
	ULong index = ((workingCRC & 0xFF) ^ inChar);
	ULong loCRC = kCRC16LoTable[index & 0x0F];
	ULong hiCRC = kCRC16HiTable[(index & 0xF0) >> 4];
	workingCRC = (workingCRC >> 8) ^ (hiCRC ^ loCRC);
}


/*--------------------------------------------------------------------------------
	Add characters in buffer into CRC computation.
	Args:		inData
	Return:	--
--------------------------------------------------------------------------------*/

void
CRC16::computeCRC(CBufferList& inData)
{
	int ch;
	inData.seek(0, kSeekFromBeginning);
	while ((ch = inData.get()) != -1)
	{
		ULong index = ((workingCRC & 0xFF) ^ ch);
		ULong loCRC = kCRC16LoTable[index & 0x0F];
		ULong hiCRC = kCRC16HiTable[(index & 0xF0) >> 4];
		workingCRC = (workingCRC >> 8) ^ (hiCRC ^ loCRC);
	}
}


/*--------------------------------------------------------------------------------
	Add characters in buffer into CRC computation.
	Args:		inData
				inSize
	Return:	--
--------------------------------------------------------------------------------*/

void
CRC16::computeCRC(UByte * inData, size_t inSize)
{
	for ( ; inSize > 0; inSize--)
	{
		ULong index = ((workingCRC & 0xFF) ^ *inData++);
		ULong loCRC = kCRC16LoTable[index & 0x0F];
		ULong hiCRC = kCRC16HiTable[(index & 0xF0) >> 4];
		workingCRC = (workingCRC >> 8) ^ (hiCRC ^ loCRC);
	}
}


/*--------------------------------------------------------------------------------
	Copy 16-bit CRC into two chars in network byte order.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CRC16::get(void)
{
#if defined(hasByteSwapping)
	crc16[0] = workingCRC;
	crc16[1] = workingCRC >> 8;
#else
	crc16[1] = workingCRC;
	crc16[0] = workingCRC >> 8;
#endif
}

#pragma mark -

/*--------------------------------------------------------------------------------
	CMNP
--------------------------------------------------------------------------------*/

CMNP::CMNP(Ulong inId)
	:	CFramedAsyncSerTool(inId)
{ }


CMNP::~CMNP()
{ }


size_t
CMNP::getSizeOf(void) const
{ return sizeof(CMNP); }	// do we even need this any more?


NewtonErr
CMNP::taskConstructor()
{
	NewtonErr err;
	if ((err = CFramedAsyncSerTool::taskConstructor()) == noErr)
	{
		f55C = 2400;
		fCCB = nil;
		f564 = 0;
		f568 = 0;
		f58C = YES;
		f3E4 = KByte;
		f524 = 260;
		if ((f594.init(NO)) == noErr)
			f5AC = f594.f00;
	}
	return err;
}


void
CMNP::taskDestructor()
{
	CFramedAsyncSerTool::taskDestructor();
}


NewtonErr
CMNP::openAlloc(void)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAIL((fCCB = new CMNP_CCB) == nil)
		XFAIL(err = fCCB->init())
		
	// INCOMPLETE
	}
	XENDTRY;
	XDOFAIL(err)
	{
		freeCCB();
	}
	XENDFAIL;
	return err;
}


void
CMNP::ackTimeOut(void)
{
	if (f18 & 0x02)
		xmitLA(0x08000000);
}

void
CMNP::windowTimeOut(void)
{
	if (f18 & 0x02)
		xmitLA(0x08000000);
}


void
CMNP::rcvStartBuffer(void)
{
	fCCB->f00 &= ~0x20;
	if ((fCCB->f00 & 0x20000000) == 0
	&&  fCCB->fC68 != nil)
	{
		fCCB->f00 |= 0x20000000;
		rcvBuffer();
	}
}

void
CMNP::rcvBuffer(void)
{
	UByte ch;	// sp00
//	r6 = 1;
//	r7 = 0;

	size_t r5 = fCCB->f40.bufferCount();
	if (fCCB->fC70 <= r5)
	{
		if (fCCB->fC70 != 0)
			fCCB->f40.copyOut(fCCB->fC68, &fCCB->fC70, NULL);
		fCCB->fD6C += fCCB->fC6C;
		getComplete(noErr, NO, fCCB->fC6C);
		fCCB->fC68 = 0;
		fCCB->fC6C = 0;
		fCCB->fC70 = 0;
	}
	else
	{
		if (fCCB->f40.bufferSpace() <= fCCB->f3C)
			fCCB->f40.copyOut(fCCB->fC68, &fCCB->fC70, NULL);
		if (fCCB->fCC0.getNextByte(&ch) == 2)
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
CMNP::MNPDecompressOut(UByte inByte)
{
	fCCB->f40.putNextByte(inByte);
}

void
CMNP::rcvInit(void)
{
	fCCB->fD10 = 0;
	fCCB->fC5C = 1;
	fCCB->fCC0.flushBytes();
	flushInputBytes();
	rcvFrame();
}

void
CMNP::rcvFrame(void)
{
	if ((f18 & 0x04) == 0)
	{
		fCCB->fC78.reset();
		f18 |= 0x40000000;
		CSerTool::getFramedBytes(&fCCB->fC78);
	}
}

void
CMNP::getFramedBytes(CBufferList * inBuffers)
{
	CCommTool::getComplete(kCommErrBadCommand);
}


void
CMNP::getComplete(NewtonErr inStatus, BOOL inEndOfFrame, size_t inGetCount)
{
	if (f18 & 0x40000000)
	{
		f27C->hide(f280, kSeekFromEnd);
		f27C = nil;

		holdAbort();
		rcvFrameComplete(inStatus, inEndOfFrame);
		allowAbort();
	}

	else
		CSerTool::getComplete(inStatus, inEndOfFrame, inGetCount);
}

void
CMNP::rcvFrameComplete(NewtonErr inErr, BOOL inEndOfFrame)
{
	NewtonErr err = noErr;
	BOOL isFrameOK = YES;

	if (inErr == kSerErrCRCError)
		isFrameOK = NO;
	else if (inErr == kSerErrAsyncError)
	{
		isFrameOK = NO;
		fCCB->fD28.fRcvAsyncErrTotal++;
	}
	else if (inErr)
		err = inErr;
	else if (!inEndOfFrame)
		isFrameOK = NO;

	if (!shouldAbort(0x40000000, err))
	{
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
}

void
CMNP::rcvProcessFrame(void)
{
	fCCB->fD28.fFramesRcvd++;

	fCCB->fD08 = fCCB->fCEC;
	if (f18 & 0x01)
		f18 |= 0x20000000;
	fCCB->fC78.seek(0, kSeekFromBeginning);
	fCCB->fC62 = fCCB->fC78.get();	// byte	frame length
	fCCB->fC63 = fCCB->fC78.get();	// byte	frame type
	switch(fCCB->fC63)
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
		fCCB->f98 = 0xFE;	// byte
		if (f28 == 0)
			f28 = 0x0E;
		startAbort(kMNPErrNotConnected);
		break;
	}
}

void
CMNP::rcvBrokenFrame(NewtonErr inErr)
{
	fCCB->fD28.fRcvBrokenTotal++;

	if (f18 & 0x01)
	{
		f18 |= 0x20000000;
		if ((fCCB->f00 & 0x02) == 0)	// not listening?
		{
			if (fCCB->fD3C <= 3)
				xmitLR();
			fCCB->f98 = 4;	// byte
			if (f28 == 0)
				f28 = 0x11;
			startAbort(kMNPErrConnectRetryLimit);
		}
	}

	if (f18 & 0x02)
		xmitNAck();
}


// negotiate
void
CMNP::xmitLR(void)
{
	if ((f18 & 0x01) != 0
	&&  (f18 & 0x08000000) == 0)
	{
		//r5 = fCCB->fC4[i].fBuf
		fCCB->fC4[0].fQueue->reset();
		UByte frameSize = (fCCB->f9B) ? 23 : 20;
		if (fCCB->f34 != 0 && (fCCB->f04 & 0x02) == 0)
		{
			if ((fCCB->f34 & 0x06) != 0)
				frameSize += 3;
			if ((fCCB->f34 & 0x08) != 0)
				frameSize += 6;
		}
		else if ((fCCB->f34 & 0x0E) != 0)
			frameSize += 3;

		UByte * frameBuf = fCCB->fC4[0].fBuf;
		frameBuf[0] = frameSize;
		frameBuf[1] = kLRFrameType;

		// Constant parameter 1
		frameBuf[2] = 2;
		frameBuf[3] = 1;
		frameBuf[4] = 6;
		frameBuf[5] = 1;
		frameBuf[6] = 0;
		frameBuf[7] = 0;
		frameBuf[8] = 0;
		frameBuf[9] = 0;
		frameBuf[10] = 0xFF;

		// Constant parameter 2
		frameBuf[11] = 2;		// Octet-oriented framing mode
		frameBuf[12] = 1;
		frameBuf[13] = 2;

		frameBuf[14] = 3;		// k
		frameBuf[15] = 1;
		frameBuf[16] = fCCB->f9A;

		frameBuf[17] = 4;		// N401 = 64
		frameBuf[18] = 2;
		frameBuf[19] = fCCB->f9C & 0xFF;
		frameBuf[20] = fCCB->f9C >> 8;

		int i = 21;
		if (fCCB->f9B)
		{
			frameBuf[21] = 8;		// N401 = 256 & fixed LT, LA frames
			frameBuf[22] = 1;
			frameBuf[23] = 2 | ((fCCB->f04 & 0x04) ? 1 : 0);
			i = 24;
		}

		if (fCCB->f04 & 0x02)
		{
			if (fCCB->f34 & 0x0E)
			{
				frameBuf[i++] = 9;
				frameBuf[i++] = 1;
				frameBuf[i++] = 0;
				UByte x = 0;
				if ((fCCB->f34 & 0x08) != 0 && fCCB->f14 == 3)
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
				frameBuf[i++] = x;
			}
		}
		else
		{
			if (fCCB->f34 & 0x06)
			{
				frameBuf[i++] = 9;
				frameBuf[i++] = 1;
				frameBuf[i++] = 0;
				if (fCCB->f34 & 0x04)
					frameBuf[i] = 2;
				if (fCCB->f34 & 0x02)
					frameBuf[i] |= 1;
				i++;
			}
			if (fCCB->f34 & 0x08)
			{
				frameBuf[i++] = 14;
				frameBuf[i++] = 4;
				frameBuf[i++] = fCCB->f14;
				frameBuf[i++] = fCCB->f18;
				frameBuf[i++] = fCCB->f19;
				frameBuf[i++] = fCCB->f15;
			}
		}

		union
		{
			long l;
			char c;
		} sp00;
		sp00.l = (fCCB->f00 & 0x02) ? f568 : f564;
		if (sp00.l)
		{
			frameBuf[0] += 6;			// sic
			frameBuf[i++] = 0xC5;
			frameBuf[i++] = 6;
			frameBuf[i++] = 1;
			frameBuf[i++] = 4;
#if defined(hasByteSwapping)
			frameBuf[i++] = sp00.c[3];
			frameBuf[i++] = sp00.c[2];
			frameBuf[i++] = sp00.c[1];
			frameBuf[i++] = sp00.c[0];
#else
			frameBuf[i++] = sp00.c[0];
			frameBuf[i++] = sp00.c[1];
			frameBuf[i++] = sp00.c[2];
			frameBuf[i++] = sp00.c[3];
#endif
		}

		fCCB->fC4[0].fQueue.hide(266-i, kSeekFromEnd);
		fCCB->f00 |= 0x00100000;
		xmitPostRequest(&fCCB->fC4[0].fQueue, YES);
		fCCB->fD28.fLRRetransCount++;
		fCCB->fCFC = fCCB->fCF4;
	}
}


void
CMNP::rcvLR(void)
{
	if (f18 & 0x01)
	{
		if (fCCB->f00 & 0x02)	// is listening?
		{
			if (fCCB->f00 & 0x01)
				retransTimeOut();
			fCCB->fD0C = 0;
			if (paramNegotiation(YES))
				listenComplete(noErr);
			else
			{
				if (f28 == 0)
					f28 = 0x14;
				startAbort(kMNPErrNegotiationFailure);
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
					f28 = 0x14;
				startAbort(kMNPErrNegotiationFailure);
			}
		}
	}

	else if (f18 & 0x02)
		xmitNAck();
}

// disconnect
BOOL
CMNP::xmitLD(void)
{
	if (fCCB->f00 & 0x40000000)	// already disconnected
	{
		f18 &= ~0x20000000;
		return YES;
	}

	fCCB->f08 = 1;
	fCCB->fC2C->reset();
	fCCB->fC4C[0] = 7;
	fCCB->fC4C[1] = 2;
	fCCB->fC4C[2] = 1;
	fCCB->fC4C[3] = 1;
	fCCB->fC4C[4] = fCCB->f98;
	fCCB->fC4C[5] = 2;
	fCCB->fC4C[6] = 1;
	fCCB->fC4C[7] = fCCB->f99;
	xmitPostRequest(&fCCB->fC2C, YES);
	setAbortTimer();
	return NO;
}

void
CMNP::rcvLD(void)
{
	int ch1, ch2;
	while ((ch1 = fCCB->fC78.get()) != -1)
	{
		ch2 = fCCB->fC78.get();
		if (ch1 == 1)
			fCCB->f98 = fCCB->fC78.get();
		else if (ch1 == 2)
			fCCB->f99 = fCCB->fC78.get();
		else
			fCCB->fC78.seek(ch2, kSeekFromHere);
	}

	NewtonErr reason = kMNPErrNotConnected;
	fCCB->f00 |= 0x40000000;
	if (f28 == 0)
	{
		switch (fCCB->f98)
		{
		case 1:
			f28 = 2;
			reason = kMNPErrHandshakeFailure;
		case 2:
			f28 = 3;
			reason = kMNPErrIncompatibleProtLvl;
		case 3:
			f28 = 4;
			reason = kMNPErrNegotiationFailure;
		case 4:
			f28 = 5;
		case 5:
			f28 = 6;
		case 6:
			f28 = 7;
		case 0xFE:
			f28 = 8;
		case 0xFF:
			f28 = 9;
		default:
			f28 = 1;
		}
	}
	startAbort(reason);
}

// data
void
CMNP::rcvLT(void)
{
	if ((f18 & 0x01)
	&&  (fCCB->f00 & 0x01))
	{
		enterConnectedState();
		xmitLT();
	}

	if (f18 & 0x02)
	{
		;
	}
}

// acknowledge
void
CMNP::rcvLA(void)
{
	if (fCCB->f00 & 0x01)
	{
		enterConnectedState();
		xmitLT();
	}

	if (f18 & 0x02)
	{
		if (fCCB->f9B)
		{
			fCCB->fCE9 = fCCB->fC78.get();	// byte
			fCCB->fCE8 = fCCB->fC78.get();	// byte
		}
		else
		{
			BOOL r6 = NO;
			BOOL r5 = NO;
			int ch1, ch2;
			while ((ch1 = fCCB->fC78.get()) != -1)
			{
				ch2 = fCCB->fC78.get();
				if (ch1 == 1)
				{
					fCCB->fCE9 = fCCB->fC78.get();
					r5 = YES;
				}
				else if (ch1 == 2)
				{
					fCCB->fCE8 = fCCB->fC78.get();
					r6 = YES;
				}
				else
					fCCB->fC78.seek(ch2, kSeekFromHere);
			}
			if (!(r5 && r6))
			{
				fCCB->f98 = 0xFE;	// byte
				if (f28 == 0)
					f28 = 0x0E;
				startAbort(kMNPErrNotConnected);
			}
		}
		
		fCCB->f00 |= 0x01;
	}
}

void
CMNP::rcvLN(void)
{ /* this really does nothing */ }

void
CMNP::rcvLNA(void)
{ /* this really does nothing */ }



void
CMNP::enterConnectedState(void)
{
	fCCB->fCFC = 0;
	fCCB->fD3C = 0;
	fCCB->fD08 = fCCB->fCEC;
	if (fCCB->f9A > 1)
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


void
CMNP::initFrameBufs(void)
{
	if (fCCB->f9B)
	{
		// set up LT (data) frame headers
		for (int i = 0; i < 8; i++)
		{
			fCCB->fC4[i].fBuf[0] = 2;	// i * 31 * 8 doesn’t look right for this; is decl wrong?
			fCCB->fC4[i].fBuf[1] = 4;
		}
		fCCB->fC58 = 2;
	}
	else
	{
		// set up ..? frame headers
		for (int i = 0; i < 8; i++)
		{
			fCCB->fC4[i].fBuf[0] = 4;
			fCCB->fC4[i].fBuf[1] = 4;
			fCCB->fC4[i].fBuf[2] = 1;
			fCCB->fC4[i].fBuf[3] = 1;
		}
		fCCB->fC58 = 4;
	}
}


void
CMNP::xmitPostRequest(CBufferList * inData, BOOL inArg2)
{
	f18 |= 0x08000000;
	CSerTool::putFramedBytes(inData, inArg2);
}


void
CMNP::putFramedBytes(CBufferList * inData, BOOL inArg2)
{
	CCommTool::putComplete(kCommErrBadCommand, 0);
}


void
CMNP::putComplete(NewtonErr inStatus, size_t inPutCount)
{
	if (f18 & 0x08000000)
	{
		f270 = nil;
		if (fCCB->f08)
		{
			fCCB->f08 = NO;
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

/*
	MNP CDIL
	fd -> CCircleBuf[512] -> MNP processing -> CBufferList[?] -> 

	TCP/IP CDIL
	fd -> CBufferList[?] -> 


	CDIL read thread
	->
		wait for kevent from kqueue
		put data read into CCircleBuf
	<-

	CDIL write thread
	->
		wait for kevent from kqueue
		clear isBusy
		ask tool to fillOutputBuffer (which is a tool-specific-sized CCircleBuf)
		if anything filled
			write to fd
			set isBusy
	<-
	
	CDIL write
	create CBufferSegment from buf, queue the data in CBufferList
	if not isBusy
		ask tool to fillOutputBuffer
		write to fd
		set isBusy

	CDIL is like a CEndpoint
	To write:
		instantiate a CBufferSegment with the data
		put that into a PB -- possibly adding the segment to a CBufferList
		pass it to the tool -- write thread
TCP/IP	if fd is ready 
MNP		copy data out, 256 bytes max, frame it up and write it out
			•• where is write buffer limited to 256 chars? ••
			when client buffer is empty notify CDIL
		release PB and its buffers
*/
