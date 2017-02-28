/*
	File:		SerialTool.cc

	Contains:	Serial communications tools.

	Written by:	Newton Research Group, 2009.
*/

#include "SerialTool.h"


/*--------------------------------------------------------------------------------
	CSerToolReply
--------------------------------------------------------------------------------*/

CSerToolReply::CSerToolReply()
{
	fSize = sizeof(CSerToolReply);
}

#pragma mark -

/*--------------------------------------------------------------------------------
	CSerTool
	vt @ 0002043C
--------------------------------------------------------------------------------*/

CSerTool::CSerTool(ULong inId)
	:	CCommTool(inId)
{
	f26C = 3;
}


CSerTool::~CSerTool()
{ }


void
CSerTool::connectStart(void)
{
	connectComplete(turnOn());
}


void
CSerTool::listenStart(void)
{
	listenComplete(turnOn());
}


void
CSerTool::putBytes(CBufferList * inData)
{
	f278 = false;
	fIsPutFramed = false;
	startOutput(inData);
}

void
CSerTool::putFramedBytes(CBufferList * inData, bool inArg2)
{
	f278 = inArg2;
	fIsPutFramed = true;
	startOutput(inData);
}

void
CSerTool::startOutput(CBufferList * inData)
{
	if (f28E)
	{
		fDataToSend = inData;
		inData->seek(0, kSeekFromBeginning);
		fNumOfBytesToSend = inData->getSize();
		fIsFirstOutput = true;
		doOutput();
	}

	else
		putComplete(kSerErrToolNotReady, 0);
}

void
CSerTool::putComplete(NewtonErr inStatus, ULong inCount)
{
	fDataToSend = NULL;
	CCommTool::putComplete(inStatus, inCount);
}

void
CSerTool::getBytes(CBufferList * outData)
{
	fIsGetFramed = false;
	fIsGetImmediate = false;
	startInput(outData);
}

void
CSerTool::getBytesImmediate(CBufferList * outData, size_t inThreshold)
{
	fIsGetFramed = false;
	fIsGetImmediate = true;
	fThreshold = inThreshold;
	startInput(outData);
}

void
CSerTool::getFramedBytes(CBufferList * outData)
{
	fIsGetFramed = true;
	fIsGetImmediate = false;
	startInput(outData);
}

void
CSerTool::startInput(CBufferList * outData)
{
	fDataRcvd = outData;
	outData->seek(0, kSeekFromEnd);
	fNumOfBytesToRecv = outData->getSize();
	if (f28E)
		doInput();
	else
		getComplete(kSerErrToolNotReady);
}

void
CSerTool::getComplete(NewtonErr inStatus, bool inEndOfFrame, size_t inGetBytesCount)
{
	fDataRcvd->hide(fNumOfBytesToRecv, kSeekFromEnd);
	fDataRcvd = NULL;
	CCommTool::getComplete(inStatus, inEndOfFrame, inGetBytesCount);
}

void
CCommTool::getComplete(NewtonErr inStatus, bool inEndOfFrame, size_t inGetBytesCount)
{
	CCommToolGetReply reply;
	reply.fGetBytesCount = inGetBytesCount;
	if (inEndOfFrame)
		reply.fEndOfFrame = true;
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


NewtonErr
CAsyncSerTool::allocateBuffers(void)
{
	f3F0 = 0;

	// it’s a biggy
	NewtonErr err;
	XTRY
	{
	}
	XENDTRY;
	return err;
	
}


void
CAsyncSerTool::deallocateBuffers(void)
{
	if (f28C)
	{
		f28C = false;

		f3AC.deallocate();
		f384.deallocate();
	}
	f3EC = 0;
	f3F0 = 0;

	EnterFIQAtomic();
	f330->releaseFIQTimer(f46C);
	f46C = NULL;
	ExitFIQAtomic();
}


void
CAsyncSerTool::txDataSent(void)
{
	if (fDataToSend != NULL && fSendBuf.bufferCount() == 0)
		doOutput();
}

void
CAsyncSerTool::rxDataAvailable(void)
{
	if (fDataRcvd != NULL)
		doInput();
}

void
CAsyncSerTool::bytesAvailable(size_t & outCount)
{
	outCount = fRecvBuf.bufferCount();
}

void
CAsyncSerTool::flushInputBytes(void)
{
	if (f3EC == 2)
		fSerialChip->rxDMAControl(0x14);	// CSerialChip
	fRecvBuf.flushBytes();
}

size_t
CAsyncSerTool::flushOutputBytes(void)
{
	EnterFIQAtomic();
	size_t numOfBytesFlushed = fSendBuf.bufferCount();
	if (f3F0 == 2 || f3F0 == 3)
	{
		if (fSerialChip)
			fSerialChip->txDMAControl(0x02);
		f3F0 = 1;
	}
	fSendBuf.flushBytes();
	ExitFIQAtomic();
	return numOfBytesFlushed;
}

void
CAsyncSerTool::doOutput(void)
{
	if (f470.fTxdOffUntilSend)
	{
		setTxDTransceiverEnable(true);
		f470.fTxdOffUntilSend = false;
	}

	NewtonErr status;
	if ((status = fillOutputBuffer()) == noErr)
	{
		f498 |= 0x20000000;
		if (fIsFirstOutput)
		{
			fIsFirstOutput = false;
			if (f292)
				fSerialChip->configureForOutput(true);
			startOutputST();
		}
		else
			continueOutputST(true);
	}

	else if (status == 5)
		doPutComplete(noErr);

	else
		doPutComplete(status);
}

void
CAsyncSerTool::doPutComplete(NewtonErr inStatus)
{
	f498 &= ~0x20000000;
	if (f292)
		fSerialChip->configureForOutput(false);
	putComplete(inStatus, fDataToSend->position());
}

NewtonErr
CAsyncSerTool::fillOutputBuffer(void)
{
	if (fNumOfBytesToSend == 0)
		return 5;

	if (f454)
		fSendBuf.reset();
	return fSendBuf.copyIn(fDataToSend, &fNumOfBytesToSend);
}

NewtonErr
CAsyncSerTool::emptyInputBuffer(ULong * outArg)
{
	NewtonErr status;
	if (fIsGetImmediate)
	{
		size_t numOfBytesCopied = fNumOfBytesToRecv;
		status = fRecvBuf.copyOut(fDataRcvd, &numOfBytesCopied, outArg);
		if (status == noErr && fNumOfBytesToRecv - numOfBytesCopied >= fThreshold)
			status = 6;
		fNumOfBytesToRecv -= numOfBytesCopied;
		fThreshold -= numOfBytesCopied;
	}
	else
	{
		status = fRecvBuf.copyOut(fDataRcvd, &fNumOfBytesToRecv, outArg);
		if (status == 1 && *outArg != 1000)
			status = kSerErrAsyncError;
	}
	return status;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	CFramedAsyncSerTool
--------------------------------------------------------------------------------*/

CFramedAsyncSerTool::CFramedAsyncSerTool(ULong inId)
	:	CAsyncSerTool(inId)
{ }

CFramedAsyncSerTool::~CFramedAsyncSerTool()
{ }


size_t
CFramedAsyncSerTool::getSizeOf(void) const
{ return sizeof(CFramedAsyncSerTool); }


NewtonErr
CFramedAsyncSerTool::taskConstructor()
{
	NewtonErr err;
	err = CAsyncSerTool::taskConstructor();
	fGetFrameState = 0;
	fPutFrameState = 0;
	fBufferSize = 512;
	fTransportInfo.flags = 0x38;
	return err;
}


void
CFramedAsyncSerTool::taskDestructor()
{ CAsyncSerTool::taskDestructor(); }


const UChar *
CFramedAsyncSerTool::getToolName(void) const
{ return (const UChar *)"Framed async serial tool"; }


NewtonErr
CFramedAsyncSerTool::processOptionStart(COption * ioOption, ULong inLabel, ULong inOpcode)
{
	NewtonErr err = noErr;

	if (inLabel == 'fram')
	{
		if (inOpcode == 0x0100 || inOpcode == 0x0200)
			framing.copyDataFrom(ioOption);
		else if (inOpcode = 0x0300)
		{
			CCMOFramingParms defaultFraming;
			ioOption->copyDataFrom(&defaultFraming);
		}
		else
			ioOption->copyDataFrom(&framing);
	}
	
	else if (inLabel == 'frst')
	{
		if (inOpcode == 0x0100 || inOpcode == 0x0200)
			err = -3;
		else if (inOpcode == 0x0400)
		{
			ioOption->copyDataFrom(&framingStats);
			resetFramingStats();
		}
		else
			err = -1;
	}

	else
		err = CAsyncSerTool::processOptionStart(ioOption, inLabel, inOpcode);

	return err;
}


NewtonErr
CFramedAsyncSerTool::addDefaultOptions(COptionArray * ioOptions)
{
	CCMOFramingParms defaultFraming;

	NewtonErr err;
	XTRY
	{
		XFAIL(err = ioOptions->appendOption(&defaultFraming))
		err = CAsyncSerTool::addDefaultOptions(ioOptions);
	}
	XENDTRY;
	return err;
}


NewtonErr
CFramedAsyncSerTool::addCurrentOptions(COptionArray * ioOptions)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = ioOptions->appendOption(&framing))
		err = CAsyncSerTool::addCurrentOptions(ioOptions);
	}
	XENDTRY;
	return err;
}


void
CFramedAsyncSerTool::killPut(void)
{
	fPutBuffer.flushBytes();
	CAsyncSerTool::killPut();
}


void
CFramedAsyncSerTool::killGet(void)
{
	fGetBuffer.flushBytes();
	CAsyncSerTool::killGet();
}


NewtonErr
CFramedAsyncSerTool::allocateBuffers(void)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = CAsyncSerTool::allocateBuffers())
		XFAIL(err = fGetBuffer.allocate(fBufferSize))
		err = fPutBuffer.allocate(fBufferSize);
	}
	XENDTRY;
	return err;
}


void
CFramedAsyncSerTool::deallocateBuffers(void)
{
	fGetBuffer.deallocate();
	fPutBuffer.deallocate();
	CAsyncSerTool::deallocateBuffers();
}


NewtonErr
CFramedAsyncSerTool::fillOutputBuffer(void)
{
	if (fIsPutFramed)
	{
		NewtonErr status;
		UByte ch;		// sp00
		size_t * sp04 = fNumOfBytesToSend;
		bool r10;
		size_t numOfBytesInFrame = 0;
//		r9 = &fPutBuffer
//		r8 = &fPutFCS
//		r7 = &fSendBuf
		XTRY
		{
			for (status = noErr; status == noErr; )
			{
				switch(fPutFrameState)
				{
//	init output buffer with header
				case 0:
					if (framing.fDoHeader)
					{
						fSendBuf.putNextByte(chSYN);
						fSendBuf.putNextByte(chDLE);
						fSendBuf.putNextByte(chSTX);
						numOfBytesInFrame += 3;
					}
					fPutFCS.reset();
					fIsPutCharStacked = false;
					fPutFrameState = 1;
					break;

//	append char to output buffer
				case 1:
					if (fIsPutCharStacked)
					{
						fIsPutCharStacked = false;
						ch = fStackedPutChar;
					}
					else
					{
						if (fPutBuffer.getNextByte(&ch) != 0)
						{
							// put buffer is empty -- refill it from client data
							if (fNumOfBytesToSend != 0)		// •• NOT RIGHT ••
								XFAILIF(status = fPutBuffer.copyIn(fDataToSend, sp04), fPutFrameState = 8; if (status == 5) status = kSerErrInternalError;)
							else if (f278)
								fPutFrameState = 3;
							else
								r10 = false;	// not in the original, but gotta break the loop somehow
						}
					}
					XFAILIF(fSendBuf.putNextByte(ch), fIsPutCharStacked = true; fStackedPutChar = ch;)
					if (framing.fDoPutFCS)
						fPutFCS.computeCRC(ch);
					numOfBytesInFrame++;
					if (ch == framing.fEscapeChar)
						fPutFrameState = 2;
					break;

// escape escape char
				case 2:
					XFAIL(status = fSendBuf.putNextByte(framing.fEscapeChar))
					numOfBytesInFrame++;
					fPutFrameState = 1;
					break;

//	introduce end-of-message
				case 3:
					ch = framing.fEscapeChar;
					XFAIL(status = fSendBuf.putNextByte(ch))
					numOfBytesInFrame++;
					fPutFrameState = 4;
					break;

//	put end-of-message
				case 4:
					ch = framing.fEOMChar;
					XFAIL(status = fSendBuf.putNextByte(ch))
					numOfBytesInFrame++;
					if (framing.fDoPutFCS)
					{
						fPutFCS.computeCRC(ch);
						fPutFrameState = 5;
					}
					else
						fPutFrameState = 7;
					break;

//	put first byte of FCS
				case 5:
					fPutFCS.get();
					XFAIL(status = fSendBuf.putNextByte(fPutFCS.crc16[1]))
					numOfBytesInFrame++;
					fPutFrameState = 6;
					break;

//	put second byte of FCS
				case 6:
					fPutFCS.get();
					XFAIL(status = fSendBuf.putNextByte(fPutFCS.crc16[0]))
					numOfBytesInFrame++;
					fPutFrameState = 7;
					break;

//	frame done
				case 7:
				case 8:
					// reset FSM
					fPutFrameState = 0;
					if (fNumOfBytesToSend == 0)
						// nothing more to send -- break
						XFAIL(status = 5)
					break;
				}
			}
		}
		XENDTRY;

		if (status == 3)
			status = 0;
		else if (numOfBytesInFrame == 0 && fNumOfBytesToSend == 0 && status == noErr)
			status = 5;

		return status;
	}

	else
		return CAsyncSerTool::fillOutputBuffer();
}


NewtonErr
CFramedAsyncSerTool::emptyInputBuffer(ULong * outArg)
{
	if (fIsGetFramed)
	{
		NewtonErr status;		// r6
		UByte ch;				// sp00
//		r2 = sp04 = &fNumOfBytesToRecv;
//		r7 = &fRecvBuf;
//		r9 = &fGetFCS;
//		r8 = &fGetBuffer;
		XTRY
		{
			for (status = noErr; status == noErr; )
			{
				switch (fGetFrameState)
				{
				case 0:
//	scan for SYN start-of-frame char
					fIsGetCharEscaped = false;
					fIsGetCharStacked = false;
					fGetFCS.reset();
					if (framing.fDoHeader)
					{
						do
						{
							XFAIL(status = fRecvBuf.getNextByte(&ch, outArg))
							if (ch == chSYN)
								fGetFrameState = 1;
							else
								framingStats.fPreHeaderByteCount++;
						} while (ch != chSYN);
					}
					else
						fGetFrameState = 3;
					break;

//	next start-of-frame must be DLE
				case 1:
					XFAIL(status = fRecvBuf.getNextByte(&ch, outArg))
					if (ch == chDLE)
						fGetFrameState = 2;
					else
					{
						fGetFrameState = 0;
						framingStats.fPreHeaderByteCount += 2;
					}
					break;

//	next start-of-frame must be STX
				case 2:
					XFAIL(status = fRecvBuf.getNextByte(&ch, outArg))
					if (ch == chSTX)
						fGetFrameState = 3;
					else
					{
						fGetFrameState = 0;
						framingStats.fPreHeaderByteCount += 3;
					}
					break;

//	read char from input buffer
				case 3:
					if (fIsGetCharStacked)
					{
						fIsGetCharStacked = false;
						ch = fStackedGetChar;
						status = 0;
					}
					else
						XFAIL(status = fRecvBuf.getNextByte(&ch, outArg))
					if (!fIsGetCharEscaped && ch == framing.fEscapeChar)
						fGetFrameState = 4;
					else
					{
						if ((status = fGetBuffer.putNextByte(ch)) == noErr)
						{
							if (framing.fDoGetFCS)
								fGetFCS.computeCRC(ch);
							fIsGetCharEscaped = false;
						}
						else
						{
							// get buffer is full -- flush it out to client read buffer
							fIsGetCharStacked = true;
							fStackedGetChar = ch;
							XFAIL(status = fGetBuffer.copyOut(fDataRcvd, &fNumOfBytesToRecv, NULL))
						}
					}
					break;

// unescape escape char
				case 4:
					XFAIL(status = fRecvBuf.getNextByte(&ch, outArg))
					if (ch == framing.fEOMChar)
					{
						// it’s end-of-message
						if (framing.fDoGetFCS)
						{
							fGetFCS.computeCRC(ch);
							fGetFrameState = 5;
						}
						else
							fGetFrameState = 7;
					}
					else if (ch == framing.fEscapeChar)
					{
						// it’s an escaped escape
						fIsGetCharStacked = true;
						fStackedGetChar = ch;
						fIsGetCharEscaped = true;
						fGetFrameState = 3;
					}
					else
						// it’s nonsense -- ignore it
						fGetFrameState = 3;
					break;

//	check first byte of FCS
				case 5:
					XFAIL(status = fRecvBuf.getNextByte(&ch, outArg))
					fGetFCS.get();
					if (ch == fGetFCS.crc16[1])
						fGetFrameState = 6;
					else
					{
						fGetFrameState = 0;
						fGetBuffer.flushBytes();
						status = kSerErrCRCError;
					}
					break;

//	check second byte of FCS
				case 6:
					XFAIL(status = fRecvBuf.getNextByte(&ch, outArg))
					fGetFCS.get();
					if (ch == fGetFCS.crc16[0])
						fGetFrameState = 7;
					else
					{
						fGetFrameState = 0;
						fGetBuffer.flushBytes();
						status = kSerErrCRCError;
					}
					break;

//	frame done
				case 7:
					// copy unframed buffer to client read buffer
					XFAIL((status = fGetBuffer.copyOut(fDataRcvd, &fNumOfBytesToRecv, NULL)) == 6)
					// reset FSM
					fGetFrameState = 0;
					// set status to indicate all done
					if (status == noErr)
						status = 1000;
					break;
				}
			}
		}
		XENDTRY;

		if (status == 1)
		{
			fGetFrameState = 0;
			fGetBuffer.flushBytes();
			status = kSerErrAsyncError;
		}
		else if (status == 2)
			status = 0;

		return status;
	}

	else
		return CAsyncSerTool::emptyInputBuffer(outArg);
}


void
CFramedAsyncSerTool::resetFramingStats(void)
{
	framingStats.fPreHeaderByteCount = 0;
}


void
CFramedAsyncSerTool::setFramingCtl(CCMOFramingParms * inParms)
{
	framing = *inParms;
}


void
CFramedAsyncSerTool::getFramingCtl(CCMOFramingParms * outParms)
{
	*outParms = framing;
}

#pragma mark -

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
	Copy 16-bit CRC into two chars in reverse network byte order.
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

