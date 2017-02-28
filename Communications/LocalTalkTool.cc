/*
	File:		LocalTalkTool.h

	Contains:	LocalTalk communications tool.

	Written by:	Newton Research Group, 2009.
*/


/*--------------------------------------------------------------------------------
	CLocalTalkService
	Communications Manager service for the LocalTalk tool.
--------------------------------------------------------------------------------*/

PROTOCOL CLocalTalkService : public CCMService
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CLocalTalkService)

	CLocalTalkService *	make(void);
	void			destroy(void);

	NewtonErr	start(COptionArray * inOptions, ULong inId, CServiceInfo * info);		// start the service
	NewtonErr	doneStarting(CEvent * inEvent, size_t inSize, CServiceInfo * info);	// called back when done starting

private:
// size +10
};


/*--------------------------------------------------------------------------------
	CLocalTalkService
	Communications Manager service implementation for the LocalTalk tool.
	TO DO		assembler glue
--------------------------------------------------------------------------------*/

PROTOCOL_IMPL_SOURCE_MACRO(CLocalTalkService)

CLocalTalkService *
CLocalTalkService::make(void)
{ return this; }


void
CLocalTalkService::destroy(void)
{ }


NewtonErr
CLocalTalkService::start(COptionArray * inOptions, ULong inId, CServiceInfo * info)
{
	NewtonErr err;
	XTRY
	{
		CLocalTalkTool	localTalkTool(inId);
		XFAIL(err = StartCommTool(&localTalkTool, inId, info))
		err = OpenCommTool(info->f04, inOptions, this);
	}
	XENDTRY;
	return err;
}


NewtonErr
CLocalTalkService::doneStarting(CEvent * inEvent, size_t inSize, CServiceInfo * info)
{
	return ((CEndpointEvent *)inEvent)->f08;
}

#pragma mark -


/*--------------------------------------------------------------------------------
	CLocalTalkTool
--------------------------------------------------------------------------------*/

class CLocalTalkTool : public CSerTool
{
public:
				CLocalTalkTool(ULong inId);
	virtual	~CLocalTalkTool(void);

	virtual size_t		getSizeOf(void);
	virtual NewtonErr	taskConstructor();
	virtual void		taskDestructor();

	virtual const UChar *	getToolName(void) const;

private:
// start @380
	int						f380;
	int						f384;
	CCMOLocalTalkStats	f388;
	CCircleBuf				f3A0;		// transport write buffer
	CCircleBuf				f3C8;		// transport read buffer
	CCMOSerialBuffers		f3F0;
	CCMOLocalTalkMiscConfig	f424;
// size +438
};



CLocalTalkTool::CLocalTalkTool(ULong inId)
	:	CSerTool(inId)
{ }


CLocalTalkTool::~CLocalTalkTool()
{ }


size_t
CLocalTalkTool::getSizeOf(void) const
{
	return sizeof(CLocalTalkTool);
}


const UChar *
CLocalTalkTool::getToolName(void) const
{
	return "LocalTalk Tool";
}


NewtonErr
CLocalTalkTool::taskConstructor()
{
	NewtonErr err;
	if ((err = CSerTool::taskConstructor()) == noErr)
	{
		f380 = 0;
		f384 = 0;
		resetStats();
		// lotsa zeroing
		setDefaultBufferSizes(&f3F0);
	}
	return err;
}


void
CLocalTalkTool::taskDestructor()
{
	CSerTool::taskDestructor();
}


NewtonErr
CLocalTalkTool::addDefaultOptions(COptionArray * ioOptions)
{
	CCMOSerialHardware hardware;
	CCMOSerialBuffers buffers;

	NewtonErr err;
	XTRY
	{
		setDefaultBufferSizes(&buffers);
		XFAIL(err = ioOptions->appendOption(&hardware))
		XFAIL(err = ioOptions->appendOption(&buffers))
		err = CCommTool::addDefaultOptions(ioOptions);
	}
	XENDTRY;
	return err;
}


NewtonErr
CLocalTalkTool::addCurrentOptions(COptionArray * ioOptions)
{
	CCMOSerialHardware hardware;	// sp10
	CCMOLocalTalkNodeId nodeId;

	NewtonErr err;
	XTRY
	{
		sp20 = f2FC;	// part of hardware?
		sp1C = f2F8;
		sp0C = f41C;	// byte
		XFAIL(err = ioOptions->appendOption(&hardware))
		XFAIL(err = ioOptions->appendOption(&nodeId))
		XFAIL(err = ioOptions->appendOption(&f388))
		XFAIL(err = ioOptions->appendOption(&f3F0))
		err = CCommTool::addCurrentOptions(ioOptions);
	}
	XENDTRY;
	return err;
}


NewtonErr
CLocalTalkTool::processOptionStart(COption * ioOption, ULong inLabel, ULong inArg3)
{
	NewtonErr err;
	switch (inLabel)
	{
	case 'sbps':
	case 'siop':
		err = -1;
		break;

	case 'sbuf':
		if (inArg3 == 0x0100 || inArg3 ==  0x0200)
		{
			if (!f28C)
			{
				f3F0.copyDataFrom(ioOption);
				f3F0.f0C = 604;
				err = noErr;
				break;
			}
		}
		else if (inArg3 == 0x0300)
		{
			CCMOSerialBuffers buffers;
			setDefaultBufferSizes(&buffers);
			ioOption->copyDataFrom(&buffers);
			err = noErr;
			break;
		}
		else
		{
			ioOption.copyDataFrom(&f3F0);
			err = noErr;
			break;
		}
		err = -1;
		break;

	case 'ltid':
		if (inArg3 == 0x0100 || inArg3 ==  0x0200)
		{
			if (!f41F || f430)
			{
				f41C = ioOption->f0C;
				if (f41F)
				{
					EnterFIQAtomic();
					f304->setSDLCAddress(f41C);
					ExitFIQAtomic();
					err = noErr;
				}
			}
			else
				err = -3;
		}
		else if (inArg3 == 0x0400)
		{
			ioOption->f0C = f41C;
			err = noErr;
		}
		else
			err = -1;
		break;

	case 'ltms':
		if (inArg3 == 0x0100 || inArg3 ==  0x0200)
		{
			if (!f28C)
			{
				f424.copyDataFrom(ioOption);
				err = noErr;
				break;
			}
		}
		err = -1;
		break;

	case 'ltst':
		if (inArg3 == 0x0100 || inArg3 ==  0x0200)
			err = -3;
		else if (inArg3 == 0x0400)
		{
			EnterFIQAtomic();
			ioOption->copyDataFrom(f388);
			resetStats();
			ExitFIQAtomic();
			err = noErr;
		}
		else
			err = -1;
		break;

	default:
		err = CSerTool::processOptionStart(ioOption, inLabel, inArg3);
	}
	return err;
}


void
CLocalTalkTool::setDefaultBufferSizes(CCMOSerialBuffers * inBuffers)
{
	inBuffers->f10 = 2048;		// rx buffer size
	inBuffers->f14 = 40;			// num of markers (packets)
	inBuffers->f0C = 604;		// tx buffer (== AppleTalk packet) size
}


NewtonErr
CLocalTalkTool::allocateBuffers()
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = f304->setSerialMode(2))
		f28C = true;

		XFAIL(err = f3C8.allocate(f3F0.f10, f3F0.f14, 0x02, 0x01))
		XFAIL(err = f3A0.allocate(f3F0.f0C, 0, 0x02, 0x00))

		XFAIL(err = f304->initRxDMA(f3C8, 0, CLocalTalkTool::rxDMAPacketInterrupt))
		XFAIL(err = f304->initTxDMA(f3A0, CLocalTalkTool::txDMAPacketInterrupt))

		EnterFIQAtomic();
		if (f434 == NULL)
			f434 = f330->acquireFIQTimer(CLocalTalkTool::transmitTimerInterrupt, this);
		ExitFIQAtomic();
	}
	XENDTRY;
	return err;
}


void
CLocalTalkTool::deallocateBuffers()
{
	EnterFIQAtomic();
	f330->releaseFIQTimers(this);
	f434 = NULL;
	ExitFIQAtomic();

	f28C = false;

	f3C8.deallocate();
	f3A0.deallocate();
}


void
CLocalTalkTool::doOutput(void)
{
	NewtonErr status;
	XTRY
	{
		XFAILNOT(f41F, status = kSerErrToolNotReady;)
		XFAILNOT(f274 >= 3 && f274 <= 604, status = kSerErrPacketSizeError;)

		f3A0.reset();
		XFAIL((status = f3A0.copyIn(f270, &f274)) == 5)

		EnterFIQAtomic();
		f384 |= 0x20000000;
		transmitStateMachine(1);
		ExitFIQAtomic();

		return;
	}
	XENDTRY;

	doPutComplete(status);
}


void
CLocalTalkTool::doPutComplete(NewtonErr inStatus)
{
	putComplete(inStatus, f270->position());
}


void
CLocalTalkTool::killPut(void)
{
	if (f270)
	{
		EnterFIQAtomic();
		transmitStateMachine(3);
		f3A0.reset();
		ExitFIQAtomic();
		doPutComplete(kCommErrRequestCanceled);
	}
	killPutComplete(noErr);
}


void
CLocalTalkTool::doInput(void)
{
	NewtonErr status;
	ULong sp00 = 0;
	f384 |= 0x40000000;
	if ((status = copyOut(f27C, &f280, &sp00)) != noErr)
	{
		if (status == 1)
			status = noErr;
		f384 |= 0x40000000;
		getComplete(status, true, f27C->position());
	}
}


void
CLocalTalkTool::killGet(void)
{
	if (f27C)
		getComplete(kCommErrRequestCanceled);
	f384 &= ~0x40000000;
	killGetComplete(noErr);
}


void
CLocalTalkTool::bytesAvailable(size_t & outCount)
{
	*outCount = f3C8.bufferCount();
}

