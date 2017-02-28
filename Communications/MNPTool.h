/*
	File:		MNPTool.h

	Contains:	MNP serial communications tool declarations.

	Written by:	Newton Research Group, 2009.
*/

#if !defined(__MNPTOOL_H)
#define __MNPTOOL_H 1

#include "SerialTool.h"
#include "MNPOptions.h"


/*--------------------------------------------------------------------------------
	CMNPTool
	vt @ 0001F418
--------------------------------------------------------------------------------*/
class CMNP_CCB;
class CXmitBufDescriptor;

class CMNPTool : public CFramedAsyncSerTool
{
public:
								CMNPTool(ULong inId);
	virtual					~CMNPTool();

// CUTaskWorld virtual member functions
protected:
			  size_t			getSizeOf(void) const;
			  NewtonErr		taskConstructor(void);
			  void			taskDestructor(void);

// CCommTool virtual member functions
public:
			  const UChar * getToolName(void) const;

			  void			handleRequest(CUMsgToken& inMsgToken, ULong inMsgType);
			  void			doControl(ULong inOpCode, ULong inMsgType);

			  void			connectStart(void);
			  void			listenStart(void);
			  void			acceptStart(void);
			  void			releaseStart(void);

			  NewtonErr		processOptionStart(COption * inOption, ULong inLabel, ULong inOpcode);

			  void			putBytes(CBufferList * inClientBuffer);
			  void			putFramedBytes(CBufferList * inClientBuffer, bool);
			  void			putComplete(NewtonErr inResult, size_t inPutBytesCount);
			  void			killPut(void);
			  void			killPutComplete(NewtonErr inResult);

			  void			getBytes(CBufferList * inClientBuffer);
			  void			getFramedBytes(CBufferList * inClientBuffer);
			  void			getBytesImmediate(CBufferList * inClientBuffer, size_t inThreshold);
			  void			getComplete(NewtonErr inResult, bool inEndOfFrame = false, size_t inGetBytesCount = 0);
			  void			killGet(void);
			  void			killGetComplete(NewtonErr inResult);

			  void			getNextTermProc(ULong inTerminationPhase, ULong& ioTerminationFlag, TerminateProcPtr& inTerminationProc);

// CMNPTool member functions
	virtual void			disconnect(void);	// sic -- overrides CCommTool member function

	void			setTimer(unsigned long, unsigned long);
	void			cancelTimer(void);

	void			handleTickTimer(void);	// cf handleTimerTick

	void			setXmitAbortTimer(void);
	void			handleXmitAbortTimer(void);
	void			cancelXmitAbortTimer(void);

	void			setRetransTimer(void);
	void			retransTimeOut(void);
	void			windowTimeOut(void);

	void			acceptorTimeOut(void);
	void			ackTimeOut(void);
	void			inactiveTimeOut(void);

	void			initConnectParms(void);
	void			initFrameBufs(void);

	NewtonErr	connectPreflight(void);
	void			enterConnectedState(void);
	void			getBytesStart(CBufferList * inClientBuffer);
	void			resetLink(void);

	void			killWrite(void);

	void			rcvInit(void);
	void			rcvFrame(void);
	void			rcvFrameComplete(NewtonErr inResult, bool);
	void			rcvBrokenFrame(NewtonErr inResult);
	void			rcvProcessFrame(void);
	void			rcvStartBuffer(void);
	void			rcvBuffer(void);
	void			cancelRcv(void);
	void			rcvLA(void);
	void			rcvLD(void);
	void			rcvLN(void);
	void			rcvLNA(void);
	void			rcvLR(void);
	void			rcvLT(void);
	ArrayIndex	receiveCredit(void);
	void			processLA(void);

	void			xmitBufferLT(void);
	void			xmitFrameComplete(NewtonErr inResult, ULong);
	void			xmitInitBuffer(CXmitBufDescriptor *);
	void			xmitStartBuffer(void);
	void			xmitPostRequest(CBufferList*, bool);
	void			cancelXmit(void);
	int			xmitLA(ULong);
	bool			xmitLD(void);
	void			xmitLDComplete(NewtonErr inResult, ULong);
	void			xmitLR(void);
	void			xmitLT(void);
	void			xmitLTContinue(void);
	void			xmitNAck(void);

	void			cleanupCCB(void);
	void			freeCCB(void);

	NewtonErr	openAlloc(void);
	bool			paramNegotiation(bool);
	void			doCompressFile(void);

	void			MNPCompressOut(UByte inByte);
	void			MNPDecompressOut(UByte inByte);
	void			MNPNilFlush(void);

protected:
// start @54C
	CCMOMNPCompression	f54C;
	int						f55C;
	ULong						f560;
	int						f564;
	int						f568;
	CCMOMNPDebugConnect	f56C;
	int						f580;
	int						f584;
	CMNP_CCB *				fCCB;			// +588
	bool						f58C;
	CUAsyncMessage			f594;
	ObjectId					f5AC;
// size +5B0
};


/* -----------------------------------------------------------------------------
	M N P C l a s s 5 V a r s
----------------------------------------------------------------------------- */

struct MNPClass5Vars
{
// size +540
};


/* -----------------------------------------------------------------------------
	C X m i t B u f D e s c r i p t o r
----------------------------------------------------------------------------- */

class CXmitBufDescriptor
{
public:
							CXmitBufDescriptor();

//private:
	friend class CMNP_CCB;
	CXmitBufDescriptor *	fNext;		// +00
	CXmitBufDescriptor *	fPrev;		// +04
	int					f08;
	int					f0C;
	UByte					fSeqNo;			//+10
	UByte					f11;
	UByte					f12;
	CBufferList			fBufList;		// +14
	CBufferSegment		fBufSegment;	// +34
	UByte					fBufData[10+256];	// +5C
// size +168
};


/* -----------------------------------------------------------------------------
	C M N P _ C C B
----------------------------------------------------------------------------- */
#define kMaxNumOfLTFrames 8
#define kMaxNumOfLRRetrans 3

class CMNP_CCB
{
public:
							CMNP_CCB();
	NewtonErr			init();

private:
	friend class CMNPTool;

	ULong					f00;							//+00	flags
	int					f04;
	bool					fIsDisconnecting;			//+08
	int					f0C;
	int					f10;
	UChar					f14[4];
	UShort				f18;
	MNPClass5Vars *	f1C;
	ULong					f34;
	ArrayIndex			f38;
	ArrayIndex			f3C;
	CCircleBuf			f40;
	CCircleBuf			fTxData;						//+68
	int					f90;
	int					f94;
	UChar					fDisconnectReasonCode;	//+98
	UChar					fDisconnectUserCode;		//+99
	UChar					fMaxNumOfLTFramesk;		//+9A
	bool					fIsOptimised;				//+9B
	ArrayIndex			fN401;						//+9C
	ArrayIndex			fMaxLTFrameLen;			//+A0
	CXmitBufDescriptor *	fA4;						//+A4
	int					fA8;
	CXmitBufDescriptor *	fAC;						//+AC
	UChar					fB0;
	UChar					fB1;
	UChar					fB2;
	ULong					fB4;
	ULong					fB8;
	CBufferList *		fBC;
	CXmitBufDescriptor *	fC0;						//+C0
	CXmitBufDescriptor	fXmitBuf[kMaxNumOfLTFrames];	//+C4

	CBufferSegment		fXmitHeaderSegment;	//+C04
	CBufferList			fXmitHeaderBuf;		//+C2C
	UChar					fXmitHeaderData[10];	//+C4C

	ULong					fOffsetToLTData;		//+C58
	int					fC5C;
	UChar					fNR;						//+C60	N(R)	sequence number of last correctly received LT frame
	UChar					fRcvdFrameCount;		//+C61	number of frames rxd w/o ack
	UChar					fRcvdFrameLen;			//+C62
	UChar					fRcvdFrameType;		//+C63
	ArrayIndex			fRcvdInfoFieldLen;	//+C64
	CBufferList *		fC68;
	ArrayIndex			fC6C;
	ArrayIndex			fC70;

	CBufferList			fRcvdFrame;				//+C78
	CBufferSegment		fC98;
	CCircleBuf			fRcvdData;				//+CC0	unframed data

	UChar					fAckNk;					//+CE8	N(k)	rx credit number
	UChar					fAckNR;					//+CE9	N(R)	rx sequence number
	int					fCEC;
	int					fCF0;
	int					fCF4;
	int					fCF8;
	int					fCFC;
	int					fD00;
	int					fD04;
	int					fD08;
	int					fD0C;
	int					fD10;
	CCMOFramingParms	framing;			// +D14
	CCMOMNPStatistics	fMNPStats;		// +D28
// size +D74
};


/* -----------------------------------------------------------------------------
	C o n s t a n t s
----------------------------------------------------------------------------- */

enum
{
	kLRFrameType = 1,		// Link Request
	kLDFrameType,			// Link Disconnect
	kLxFrameType,
	kLTFrameType,			// Link Transfer
	kLAFrameType,			// Link Acknowledge
	kLNFrameType,			// Link attentioN
	kLNAFrameType			// Link attentioN Acknowledge
};


#endif	/* __MNPTOOL_H */
