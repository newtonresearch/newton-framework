/*
	File:		SerialTool.h

	Contains:	Serial communications tool declarations.

	Written by:	Newton Research Group, 2009.
*/

#if !defined(__SERIALTOOL_H)
#define __SERIALTOOL_H 1

#include "CommTool.h"
#include "CircleBuf.h"
#include "CRC.h"
#include "SerialOptions.h"


class CSerialChip;

/* -------------------------------------------------------------------------------
	C S e r T o o l R e p l y
------------------------------------------------------------------------------- */

class CSerToolReply : public CCommToolReply
{
public:
				CSerToolReply();

// start @10
// size +30
};


/* -------------------------------------------------------------------------------
	CSerTool
	vt @ 0002043C
	Abstract base class for serial communications tools.
------------------------------------------------------------------------------- */
struct SCCChannelInts;

class CSerTool : public CCommTool
{
public:
								CSerTool(ULong inId);
	virtual					~CSerTool();

// CUTaskWorld virtual member functions
protected:
			  NewtonErr		taskConstructor(void);
			  void			taskDestructor(void);

// CCommTool virtual member functions
public:
			  void			handleRequest(CUMsgToken& inMsgToken, ULong inMsgType);

			  void			doControl(ULong inOpCode, ULong inMsgType);
			  void			doKillControl(ULong inMsgType);

			  void			getCommEvent(void);

			  void			connectStart(void);
			  void			listenStart(void);
			  void			bindStart(void);
			  void			unbindStart(void);

			  NewtonErr		processOptionStart(COption * inOption, ULong inLabel, ULong inOpcode);
			  NewtonErr		addDefaultOptions(COptionArray * inOptions);
			  NewtonErr		addCurrentOptions(COptionArray * inOptions);

			  void			putBytes(CBufferList * inClientBuffer);
			  void			putFramedBytes(CBufferList * inClientBuffer, bool);
			  void			putComplete(NewtonErr inResult, size_t inPutBytesCount);

			  void			getBytes(CBufferList * inClientBuffer);
			  void			getFramedBytes(CBufferList * inClientBuffer);
			  void			getBytesImmediate(CBufferList * inClientBuffer, size_t inThreshold);
			  void			getComplete(NewtonErr inResult, bool inEndOfFrame = false, size_t inGetBytesCount = 0);

			  void			resArbReleaseStart(unsigned char*, unsigned char*);
			  void			resArbClaimNotification(unsigned char*, unsigned char*);

			  void			terminateComplete(void);

// CSerTool member functions
	virtual void			powerOnEvent(ULong);
	virtual void			powerOffEvent(ULong);

	virtual void			claimSerialChip(void);
	virtual NewtonErr		allocateBuffers(void) = 0;
	virtual void			bindToSerChip(void);
	virtual void			turnOnSerChip(void) = 0;

	virtual void			turnOffSerChip(void) = 0;
	virtual void			unbindToSerChip(void);
	virtual void			deallocateBuffers(void) = 0;
	virtual void			unclaimSerialChip(void);

	virtual NewtonErr		turnOn(void);
	virtual NewtonErr		turnOff(void);

	virtual void			setIOParms(CCMOSerialIOParms * inOption);
	virtual void			setSerialChipSelect(CCMOSerialHardware * inOption);
	virtual void			setSerialChipLocation(CCMOSerialHWChipLoc * inOption);
	virtual void			setSerialChipSpec(CCMOSerialChipSpec * inOption);

	virtual void			bytesAvailable(size_t & outNumOfBytes) = 0;

	virtual void			startOutput(CBufferList * inData);
	virtual void			doOutput(void) = 0;
	virtual void			startInput(CBufferList * inData);
	virtual void			doInput(void) = 0;

	virtual void			getChannelIntHandlers(SCCChannelInts*) = 0;
	virtual void			iHRequest(unsigned long);
	virtual void			iHReqHandler(void) = 0;
	virtual void			wakeUpHandler(void);

	void		controlComplete(CCommToolReply&);

	int		changeSpeed(unsigned long);

	void		sendWakeUp(unsigned long);

	void		postSerialEvent(void);

	void		setHSKo(bool);
	void		setTxDTransceiverEnable(bool);
	void		cleanUp(void);
	void		setBreak(bool);

	void		setSerialOutputs(unsigned long, unsigned long);
	void		getSerialOutputs(void);

	void		lookUpSerialChip(unsigned long);

protected:
// start @026C
	int						f26C;		// +26C
	CBufferList *			fDataToSend;			// +270
	size_t					fNumOfBytesToSend;	// +274
	bool						f278;		// +278
	bool						fIsPutFramed;			// +279
	bool						fIsFirstOutput;		// +27A

	CBufferList *			fDataRcvd;				// +27C
	size_t					fNumOfBytesToRecv;	// +280
	size_t					fThreshold;				// +284
	bool						fIsGetFramed;			// +288
	bool						fIsGetImmediate;		// +289

	bool						f28E;		// +28E
	bool						f292;		// +292

	CSerToolReply			f294;
	CUAsyncMessage			f2D8;		// could be part of something bigger
	CEvent					f2EC;		//
	CSerialChip *			fSerialChip;			// +304
	CCMOSerialChipSpec	f310;
//	CDelayTimer				f338;
	CCMOSerialIOParms		f344;
	CUAsyncMessage			f360;		// could be part of something bigger
	CEvent					f374;		//
// size +380
};


/*--------------------------------------------------------------------------------
	CAsyncSerTool
	vt @ 0001C8D4
--------------------------------------------------------------------------------*/

class CAsyncSerTool : public CSerTool
{
public:
								CAsyncSerTool(ULong inId);
	virtual					~CAsyncSerTool();

// CUTaskWorld virtual member functions
protected:
			  size_t			getSizeOf(void) const;
			  NewtonErr		taskConstructor(void);
			  void			taskDestructor(void);

// CCommTool virtual member functions
public:
			  const UChar * getToolName(void) const;

			  NewtonErr		processOptionStart(COption * inOption, ULong inLabel, ULong inOpcode);
			  NewtonErr		addDefaultOptions(COptionArray* inOptions);
			  NewtonErr		addCurrentOptions(COptionArray* inOptions);

			  void			killPut(void);
			  void			killGet(void);

// CSerTool virtual member functions
			  NewtonErr		allocateBuffers(void);
			  void			turnOnSerChip(void);
			  void			turnOffSerChip(void);
			  void			deallocateBuffers(void);

			  void			bytesAvailable(size_t & outNumOfBytes);
			  void			doOutput(void);
			  void			doInput(void);

			  void			getChannelIntHandlers(SCCChannelInts*);
			  void			iHRequest(unsigned long);
			  void			iHReqHandler(void);

// CAsyncSerTool member functions
	virtual void			doPutComplete(long);
	virtual NewtonErr		fillOutputBuffer(void);

	virtual void			doGetComplete(long, bool);
	virtual NewtonErr		emptyInputBuffer(ULong *);
			  void			emptyInFIFO(void);

	virtual void			txDataSent(void);
	virtual void			rxDataAvailable(void);

	virtual void			serialEvents(unsigned long);
	virtual void			setInputSendForIntDelay(unsigned long);
	virtual void			dataInObserver(bool);
	virtual void			restoreInputSendForIntDelay(void);

	void					continueOutputIH(bool);
	void					continueOutputST(bool);
	void					doBreakFraming(void);
	void					extStatusInt(void);
	void					flushInputBytes(void);
	size_t				flushOutputBytes(void);
	void					getStats(CCMOSerialIOStats*);
	void					handleCharIn(unsigned char, unsigned long);
	void					hskiOn(void);
	void					outputStopped(void);
	void					resetStats(void);
	void					rxCAvailInt(void);
	void					rxCSpecialInt(void);
	void					setEventEnables(CCMOSerialEventEnables*);
	void					setInputFlowControl(CCMOInputFlowControlParms*);
	void					setOutputFlowControl(CCMOOutputFlowControlParms*);
	void					startOutputST(void);
	void					doInputFlowControl(void);
	void					getMoreOutChars(unsigned char*);
	void					getNextOutChar(unsigned char*);
	void					syncInputBuffer(void);

	void					configureModemInterrupts(void);
	void					gpiOn(void);
	void					suspendTxDMA(void);
	void					txDMAInterrupt(void);
	void					txBEmptyInt(void);
	void					rxMultiByteInterrupt(unsigned long);
	void					carrierTimerInterrupt(unsigned long);

protected:
// start @380
	CCircleBuf				fSendBuf;	// +384		// transport output buffer
	CCircleBuf				fRecvBuf;	// +3AC		// transport input buffer

	CCMOSerialBuffers		f3D4;
	int						f3EC;
	int						f3F0;
	CCMOOutputFlowControlParms	f3F4;
	CCMOInputFlowControlParms	f408;
	CCMOSerialIOStats		f41C;
	CCMOBreakFraming		f43C;
	int						f454;
	CCMOSerialEventEnables	f458;
	CCMOSerialMiscConfig	f470;
	int						f498;
// size +4B0
};


/*--------------------------------------------------------------------------------
	CFramedAsyncSerTool
	vt @ 0001EC5C
--------------------------------------------------------------------------------*/

class CFramedAsyncSerTool : public CAsyncSerTool
{
public:
								CFramedAsyncSerTool(ULong inId);
	virtual					~CFramedAsyncSerTool();

// CUTaskWorld methods
protected:
	virtual size_t			getSizeOf(void) const;
	virtual NewtonErr		taskConstructor();
	virtual void			taskDestructor();

// CCommTool methods
public:
	virtual const UChar * getToolName(void) const;

	virtual NewtonErr		processOptionStart(COption * inOption, ULong inLabel, ULong inOpcode);
	virtual NewtonErr		addDefaultOptions(COptionArray * inOptions);
	virtual NewtonErr		addCurrentOptions(COptionArray * inOptions);
	
	virtual void			killPut(void);
	virtual void			killGet(void);

// CSerTool methods
	virtual NewtonErr		allocateBuffers(void);
	virtual void			deallocateBuffers(void);

// CAsyncSerTool methods
	virtual NewtonErr		fillOutputBuffer(void);
	virtual NewtonErr		emptyInputBuffer(ULong *);

// CFramedAsyncSerTool methods
	void						resetFramingStats(void);
	void						setFramingCtl(CCMOFramingParms * inParms);
	void						getFramingCtl(CCMOFramingParms * outParms);

protected:
// start @4B0
	int						fGetFrameState;		// +4B0
	int						fPutFrameState;		// +4B4
	CRC16						fGetFCS;					// +4BC
	CRC16						fPutFCS;					// +4C4
	bool						fIsGetCharEscaped;	// +4CC
	UByte						fStackedPutChar;		// +4CD
	bool						fIsPutCharStacked;	// +4CE
	UByte						fStackedGetChar;		// +4CF
	bool						fIsGetCharStacked;	// +4D0
	CCircleBuf				fPutBuffer;				// +4D4
	CCircleBuf				fGetBuffer;				// +4FC
	size_t					fBufferSize;			// +524
	CCMOFramingParms		framing;					// +528
	CCMOFramedAsyncStats	framingStats;			// +53C
// size +54C
};


#endif	/* __SERIALTOOL_H */
