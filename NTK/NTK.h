/*
	File:		NTK.h

	Contains:	Newton ToolKit interface.

	Written by:	Newton Research Group.
*/

#if !defined(__NTK_H)
#define __NTK_H

#include "REPTranslators.h"
#include "TaskSafeRingBuffer.h"
#include "EventHandler.h"
#include "NTKProtocol.h"


/*------------------------------------------------------------------------------
	P S e r i a l I n T r a n s l a t o r
	Input from serial port.
------------------------------------------------------------------------------*/

struct SerialTranslatorInfo
{
	CTaskSafeRingBuffer *	ringBuf;
	ArrayIndex	bufSize;
};


PROTOCOL PSerialInTranslator : public PInTranslator
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PSerialInTranslator)

	PSerialInTranslator *	make(void);
	void			destroy(void);

	NewtonErr	init(void * inContext);
	Timeout		idle(void);
	bool			frameAvailable(void);
	Ref			produceFrame(int inLevel);

private:
	CTaskSafeRingBuffer *	fRingBuf;
	char *		fBuf;
	ArrayIndex	fBufSize;
};


/*------------------------------------------------------------------------------
	P S e r i a l O u t T r a n s l a t o r
	Output to serial port.
------------------------------------------------------------------------------*/

PROTOCOL PSerialOutTranslator : public POutTranslator
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PSerialOutTranslator)

	PSerialOutTranslator *	make(void);
	void			destroy(void);

	NewtonErr	init(void * inContext);
	Timeout		idle(void);
	void			consumeFrame(RefArg inObj, int inDepth, int indent);
	void			prompt(int inLevel);
	int			print(const char * inFormat, ...);
	int			vprint(const char * inFormat, va_list args);
	int			putc(int inCh);
	void			flush(void);

	void			enterBreakLoop(int);
	void			exitBreakLoop(void);

	void			stackTrace(void * interpreter);
	void			exceptionNotify(Exception * inException);

private:
	CTaskSafeRingBuffer *	fRingBuf;
	char *		fBuf;
	ArrayIndex	fBufSize;
};


/*------------------------------------------------------------------------------
	P N T K I n T r a n s l a t o r
	Input from Newton Toolkit.
------------------------------------------------------------------------------*/

struct NTKTranslatorInfo
{
	CTaskSafeRingBuffer *	x00;
	unsigned			x04;
	Timeout			x08;
	unsigned			x0C;	// used by NTKOutTranslator only
};


PROTOCOL PNTKInTranslator : public PInTranslator
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PNTKInTranslator)

	PNTKInTranslator *	make(void);
	void			destroy(void);

	NewtonErr	init(void * inContext);
	Timeout		idle(void);
	bool			frameAvailable(void);
	Ref			produceFrame(int inLevel);
	void			readHeader(EventType*, size_t*);
	void			readData(void * inData, size_t inLength);
	NewtonErr	loadPackage(void);
	void			setTimeout(Timeout inTimeout);

private:
	Timeout			fRetryInterval;
	Timeout			fTimeout;
	CTaskSafeRingBuffer *	fBuffer;
	CTaskSafeRingPipe *	fPipe;
	bool				fIsFrameAvailable;
};


/*------------------------------------------------------------------------------
	P N T K O u t T r a n s l a t o r
	Output to Newton Toolkit.
------------------------------------------------------------------------------*/

PROTOCOL PNTKOutTranslator : public POutTranslator
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PNTKOutTranslator)

	PNTKOutTranslator *	make(void);
	void			destroy(void);

	NewtonErr	init(void * inContext);
	Timeout		idle(void);
	void			consumeFrame(RefArg inObj, int inDepth, int indent);
	void			prompt(int inLevel);
	int			print(const char * inFormat, ...);
	int			vprint(const char * inFormat, va_list args);
	int			putc(int inCh);
	void			flush(void);

	void			enterBreakLoop(int);
	void			exitBreakLoop(void);

	void			stackTrace(void * interpreter);
	void			exceptionNotify(Exception * inException);

private:
	friend class CNTKNub;
	void			sendHeader(EventClass inEvtClass = kNewtEventClass, EventId inEvtId = kToolkitEventId);
	void			sendCommand(EventType inCmd, size_t inLength);
	void			sendData(void * inData, size_t inLength);
	void			consumeFrameReally(RefArg inRef);
	void			consumeExceptionFrame(RefArg inRef, const char * inMessage);
	void			flushText(void);
	void			setTimeout(Timeout inTimeout);

	Timeout			fRetryInterval;
	Timeout			fTimeout;
	CTaskSafeRingBuffer *	fBuffer;
	char *			fBuf;
	ArrayIndex		fBufLen;
	CTaskSafeRingPipe *	fPipe;
	char *			fBufPtr;
	ArrayIndex		fBufRemaining;
};


/*------------------------------------------------------------------------------
	C N T K E n d p o i n t C l i e n t
------------------------------------------------------------------------------*/
class COption;
class COptionArray;
class CEndpointEvent;

class CNTKEndpointClient /* : public CEndpointClient */
{
public:
					CNTKEndpointClient();
					~CNTKEndpointClient();

	NewtonErr	init(COptionArray*, COptionArray*, COptionArray*, CTaskSafeRingBuffer*, CTaskSafeRingBuffer*, long, long);
	NewtonErr	idleProc(CUMsgToken*, unsigned long*, CEvent*);
	NewtonErr	checkSend(void);
	NewtonErr	makeYourPeace(void);
	NewtonErr	bindComplete(CEndpointEvent * inEndpoint);
	NewtonErr	connectComplete(CEndpointEvent * inEndpoint);
	NewtonErr	abortComplete(CEndpointEvent * inEndpoint);
	NewtonErr	rcvComplete(CEndpointEvent * inEndpoint);
	NewtonErr	sndComplete(CEndpointEvent * inEndpoint);
	NewtonErr	disconnect(CEndpointEvent * inEndpoint);
	NewtonErr	disconnectComplete(CEndpointEvent * inEndpoint);
	NewtonErr	unBindComplete(CEndpointEvent * inEndpoint);

private:
// size +44
};


/*------------------------------------------------------------------------------
	C K i l l E v e n t
------------------------------------------------------------------------------*/

class CKillEvent : public CEvent
{
public:
					CKillEvent();
};


/*--------------------------------------------------------------------------------
	C K i l l E v e n t H a n d l e r
--------------------------------------------------------------------------------*/

class CKillEventHandler : public CEventHandler
{
public:
					CKillEventHandler(CNTKEndpointClient * inEndpoint);

	NewtonErr	init();
	void			eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);

private:
	CNTKEndpointClient *	f14;
};


/*------------------------------------------------------------------------------
	C N T K T a s k
------------------------------------------------------------------------------*/
#include "AppWorld.h"

class CNTKTask : public CAppWorld
{
public:
					CNTKTask();
	virtual		~CNTKTask();

	size_t		getSizeOf(void) const;

	NewtonErr	mainConstructor(void);
	void			mainDestructor(void);
	NewtonErr	preMain(void);
	void			postMain(void);

	NewtonErr	initNTK(COptionArray * inOpt1, COptionArray * inOpt2, COptionArray * inOpt3, CTaskSafeRingBuffer * inBuf1, CTaskSafeRingBuffer * inBuf2, size_t inBuf1Len, size_t inBuf2Len);

private:
	CTaskSafeRingBuffer *	f70;		// +70
	CTaskSafeRingBuffer *	f74;		// +74
	size_t						f78;		// +78
	size_t						f7C;		// +7C
	COptionArray *				f80;		// +80
	COptionArray *				f84;		// +84
	COptionArray *				f88;		// +88
};


/*------------------------------------------------------------------------------
	C N T K N u b
------------------------------------------------------------------------------*/

class CNTKNub
{
public:
				CNTKNub();
				~CNTKNub();

	NewtonErr	init(COptionArray * inOpt1, COptionArray * inOpt2, COptionArray * inOpt3, const char * inInTranslator, const char * inOutTranslator, bool inSerial);

	NewtonErr	startListener(void);
	NewtonErr	stopListener(void);

	NewtonErr	readCommand(EventType * outCmd, size_t * outLength);
	NewtonErr	doCommand(void);
	NewtonErr	downloadPackage(void);
	NewtonErr	deletePackage(size_t inNameLen);
	NewtonErr	handleCodeBlock(size_t inLength);

	NewtonErr	sendTextHeader(size_t inLength);
	NewtonErr	sendResult(NewtonErr inResult);
	NewtonErr	sendEOM(void);
	NewtonErr	sendRef(EventType	inCmd, RefArg inRef);

	NewtonErr	enterBreakLoop(int inLevel);
	NewtonErr	exitBreakLoop(void);

	NewtonErr	exceptionNotify(Exception * inException);
	NewtonErr	sendExceptionData(const char * inExceptionName, char * inMessage);
	NewtonErr	sendExceptionData(const char * inExceptionName, RefArg inData);
	NewtonErr	sendExceptionData(const char * inExceptionName, NewtonErr inError);
	NewtonErr	sendExceptionHeader(EventType inCmd);

private:
	PInTranslator *		fSavedREPIn;
	POutTranslator *		fSavedREPOut;
	PInTranslator *		fREPIn;
	POutTranslator *		fREPOut;
	PNTKInTranslator *	fNTKIn;
	PNTKOutTranslator *	fNTKOut;
	CTaskSafeRingBuffer *fInputBuffer;
	CTaskSafeRingBuffer *fOutputBuffer;
	CUPort					fNTKPort;
	bool						fIsNTKTranslator;
	bool						fIsConnected;
};


/*----------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------*/

extern void		NTKInit(void);

#endif	/* __NTK_H */
