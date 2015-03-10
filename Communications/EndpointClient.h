/*
	File:		EndpointClient.h

	Contains:	Endpoint client declarations.

	Written by:	Newton Research Group, 2008.
*/


#if !defined(__ENDPOINTCLIENT_H)
#define __ENDPOINTCLIENT_H 1

#include "Unicode.h"
#include "Endpoint.h"
#include "EventHandler.h"
#include "EndpointEvents.h"
#include "BufferSegment.h"
#include "RefIO.h"


class CSerialEndpoint;


class CEndpointClient : public CEventHandler
{
public:
							CEndpointClient();
	virtual				~CEndpointClient();

	NewtonErr			init(CEndpoint *, EventId inEventId, EventClass inEventClass);
	CEndpoint *			getEndpoint(void) const;
	void					setNilEndpoint(void);

	virtual bool		testEvent(CEvent * inEvent);
	virtual void		eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual void		eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);

	virtual void		disconnect(CEndpointEvent * inEvent);
	virtual void		release(CEndpointEvent * inEvent);
	virtual void		dfault(CEndpointEvent * inEvent);
	virtual void		sndComplete(CEndpointEvent * inEvent);
	virtual void		rcvComplete(CEndpointEvent * inEvent);

	virtual void		getProtAddr(CEndpointEvent * inEvent);
	virtual void		listenComplete(CEndpointEvent * inEvent);
	virtual void		acceptComplete(CEndpointEvent * inEvent);
	virtual void		connectComplete(CEndpointEvent * inEvent);
	virtual void		optMgmtComplete(CEndpointEvent * inEvent);
	virtual void		releaseComplete(CEndpointEvent * inEvent);
	virtual void		disconnectComplete(CEndpointEvent * inEvent);
	virtual void		bindComplete(CEndpointEvent * inEvent);
	virtual void		unbindComplete(CEndpointEvent * inEvent);
	virtual void		abortComplete(CEndpointEvent * inEvent);
	virtual void		defaultComplete(CEndpointEvent * inEvent);

protected:
	CEndpoint *			fCEndpoint;		// +14
};

inline CEndpoint *	CEndpointClient::getEndpoint(void) const { return fCEndpoint; }
inline void				CEndpointClient::setNilEndpoint(void) { fCEndpoint = NULL; }



enum FormType
{
	kBadFormType,
	kExportFormType,
	kCharFormType,
	kStringFormType,
	kNumberFormType,
	kBytesFormType,
	kBinaryFormType,
	kFrameFormType,
	kTemplateFormType
};

typedef int FormUser;	// should be an enum when we know what the FormUsers are

/*------------------------------------------------------------------------------
	P S c r i p t D a t a I n
------------------------------------------------------------------------------*/

PROTOCOL PScriptDataIn : public PFrameSource
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PScriptDataIn)

	PScriptDataIn *	make(void);
	void			destroy(void);

	Ref			translate(void * inParms, CPipeCallback * inCallback);

private:
	Ref			parseInput(FormType inType, long, long, UChar * inBuf, RefArg, NewtonErr * outErr);
};

struct ScriptDataInParms
{
	UChar *	x00;
	long		x04;
	FormType	x08;
	long		x0C;
	RefVar	x10;
};


/*------------------------------------------------------------------------------
	P O p t i o n D a t a I n
------------------------------------------------------------------------------*/

PROTOCOL POptionDataIn : public PFrameSource
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(POptionDataIn)

	POptionDataIn *	make(void);
	void			destroy(void);

	Ref			translate(void * inParms, CPipeCallback * inCallback);
};

struct OptionDataInParms
{
	COptionArray *	x00;
	RefVar			x04;
	PFrameSource *	x08;
};


/*------------------------------------------------------------------------------
	P S c r i p t D a t a O u t
------------------------------------------------------------------------------*/

PROTOCOL PScriptDataOut : public PFrameSink
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PScriptDataOut)

	PScriptDataOut *	make(void);
	void			destroy(void);

	Ptr			translate(void * inParms, CPipeCallback * inCallback);

private:
	NewtonErr	parseOutputLength(RefArg, FormType, long, long*);
	NewtonErr	parseOutput(RefArg, FormType, long, long*, UByte*);
};

struct ScriptDataOutParms
{
	RefVar			x00;
	FormType			x04;
	long				x08;
	bool				x0C;
	long				x10;
};


/*------------------------------------------------------------------------------
	P O p t i o n D a t a O u t
------------------------------------------------------------------------------*/

PROTOCOL POptionDataOut : public PFrameSink
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(POptionDataOut)

	POptionDataOut *	make(void);
	void			destroy(void);

	Ptr			translate(void * inParms, CPipeCallback * inCallback);
};

struct OptionDataOutParms
{
	COptionArray *	x00;
	RefVar			x04;
	PFrameSink *	x08;
};



class CNewScriptEndpointClient : public CEndpointClient
{
public:
							CNewScriptEndpointClient();
							~CNewScriptEndpointClient();

	virtual void		eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual void		idleProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);

	virtual void		dfault(CEndpointEvent * inEvent);
	virtual void		optMgmtComplete(CEndpointEvent * inEvent);
	virtual void		bindComplete(CEndpointEvent * inEvent);
	virtual void		listenComplete(CEndpointEvent * inEvent);
	virtual void		acceptComplete(CEndpointEvent * inEvent);
	virtual void		connectComplete(CEndpointEvent * inEvent);
	virtual void		sndComplete(CEndpointEvent * inEvent);
	virtual void		rcvComplete(CEndpointEvent * inEvent);
	virtual void		releaseComplete(CEndpointEvent * inEvent);
	virtual void		disconnectComplete(CEndpointEvent * inEvent);
	virtual void		unbindComplete(CEndpointEvent * inEvent);
	virtual void		abortComplete(CEndpointEvent * inEvent);

	NewtonErr	initScriptEndpointClient(RefArg inEndpoint, RefArg inOptions, CEndpoint * inCEndpoint);

	void			optionCommandComplete(NewtonErr inErr, RefArg inQueue, COptionArray * inOptionArray);
	void			commandComplete(NewtonErr inErr, RefArg inQueue, RefArg inOptions);
	void			doCompletion(NewtonErr inErr, RefArg inQueue, RefArg inOptions);

	void			queueCallback(RefArg inCallbackSpec);
	void			unwindCallback(void);
	void			queueOptions(RefArg inOptions, RefArg inCallbackSpec);
	void			unwindOptions(void);
	static bool	getParms(RefArg inCallbackSpec, Timeout * outTimeout);
	NewtonErr	prepOptions(RefArg inOptions, COptionArray ** outArray);
	NewtonErr	convertToOptionArray(RefArg inOptions, COptionArray * ioArray);
	NewtonErr	convertFromOptionArray(RefArg outOptions, COptionArray * inArray);

	int			doState(void);
	NewtonErr	doBind(RefArg inOptions, RefArg inCallbackSpec);
	NewtonErr	doListen(RefArg inOptions, RefArg inCallbackSpec);
	NewtonErr	doAccept(RefArg inOptions, RefArg inCallbackSpec);
	NewtonErr	doConnect(RefArg inOptions, RefArg inCallbackSpec);
	NewtonErr	doOption(RefArg inOptions, RefArg inCallbackSpec);

	NewtonErr	doInputSpec(RefArg inputSpec);
	void			clearInputSpec(void);
	NewtonErr	postReceive(void);

	NewtonErr	checkForInput(long, bool);
	NewtonErr	initInputBuffers(void);
	NewtonErr	getFrameLength(void);
	int			checkEndArray(UChar * inBuf);
	NewtonErr	rawRcvComplete(CRcvCompleteEvent*);
	NewtonErr	filterRcvComplete(ULong);

	NewtonErr	doInput(void);
	NewtonErr	postInput(long, int inTerminationCondition);
	void			doFlushInput(void);

	Ref			doPartial(NewtonErr * outErr);
	Ref			getPartialData(NewtonErr * outErr);
	void			doFlushPartial(void);

	Ref			parseInput(FormType inType, long, long, UChar *, RefArg, NewtonErr * outErr);

	NewtonErr	doOutput(RefArg inData, RefArg inOptions, RefArg inCallbackSpec);
	NewtonErr	outputFrame(RefArg inData, bool inSync, ULong inFlags, Timeout inTimeout, COptionArray * inOptionArray);
	NewtonErr	outputData(RefArg inData, FormType inForm, bool inSync, ULong inFlags, Timeout inTimeout, COptionArray * inOptionArray);
	NewtonErr	outputRaw(RefArg inData, RefArg inCallbackSpec, bool inSync, ULong inFlags, Timeout inTimeout, COptionArray * inOptionArray);

	NewtonErr	doUnBind(RefArg inCallbackSpec);
	NewtonErr	doDisconnect(RefArg inCancelPending, RefArg inCallbackSpec);
	NewtonErr	doAbort(RefArg inCallbackSpec);

	PScriptDataIn *	getScriptDataInXlator(void);
	PScriptDataOut *	getScriptDataOutXlator(void);

	NewtonErr	readInputSlots(void);
	NewtonErr	readTermination(RefArg inSpec);
	NewtonErr	readTarget(RefArg inSpec);
	NewtonErr	readFilter(RefArg inSpec);

	NewtonErr	addProxyFrame(RefArg inSpec);
	NewtonErr	addProxyArrayElement(RefArg inArray, RefArg inElement);
	NewtonErr	addEndArrayElement(RefArg inSpec);

protected:
	RefStruct		fNSEndpoint;		// +18
	CharEncoding	fEncoding;			// +1C
	ULong				fRcvFlags;			// +20
	bool				f24;					// isPacketData? isLastPacket?
	RefStruct		fSendQueue;			// +28
	RefStruct		fCommandQueue;		// +2C
	RefStruct		fAbortQueue;		// +30
	RefStruct		fInputSpec;			// +34
	ArrayIndex		fByteCount;			// +38
	ArrayIndex		fDiscardAfter;		// +3C
	RefStruct		fEndSequence;		// +40
	bool				f44;					// +44
	Timeout			fTimeout;			// +48	ms
	Timeout			fPartialInterval;	// +4C	ms
	RefStruct		fByteArray;			// +50
	RefStruct		fProxyArray;		// +54
	bool				fSevenBit;			// +58
	RefStruct		fRcvOptions;		// +5C
	uint32_t			f60;
	bool				fIsTargetActive;	// +64
	RefStruct		fTarget;				// +68	binary/string
	ArrayIndex		fTargetOffset;		// +6C
	bool				f70;
	CBufferSegment	f74;
	UChar *			fInputBuffer;		// +9C
	ArrayIndex		fInputBufLen;		// +A0
	ArrayIndex		fInputBufWrIndex;	// +A4
	bool				fIsDataAvailable;	// +A8
	ArrayIndex		fInputBufRdIndex;	// +AC
	bool				fIsInputSpecActive;	// +B0
	bool				fB1;
	FormType			fForm;				// +B4
	POptionDataOut *	fOptionDataOutXlator;	// +BC
	POptionDataIn *	fOptionDataInXlator;		// +C0
	PScriptDataOut *	fDataOutXlator;			// +C4
	PScriptDataIn *	fDataInXlator;				// +C8
	PFlattenPtr *		fFrameDataOutXlator;		// +CC
	PUnflattenPtr *	fFrameDataInXlator;		// +D0
};


class CStreamingEndpointClient : public CNewScriptEndpointClient
{
public:
	Ref			doStreamIn(RefArg inSpec, NewtonErr * outErr);
	NewtonErr	doStreamOut(RefArg inData, RefArg inSpec);
};


class CStreamingCallback : public CPipeCallback
{
public:
				CStreamingCallback();
	virtual	~CStreamingCallback();

	bool	status(size_t inNumOfBytesGot, size_t inNumOfBytesPut);

	RefStruct	fReceiver;		// +0C
	int			f10;
};


CEndpoint * GetClientEndpoint(RefArg inNSEndpoint);

bool			UseModemNavigator(void);
bool			ContainsModemService(COptionArray * inOptions);
NewtonErr	RunModemNavigator(COptionArray * inOptions);


#endif	/* __ENDPOINTCLIENT_H */
