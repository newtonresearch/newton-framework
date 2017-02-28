/*
	File:		TestAgent.cc

	Contains:	Test reporting system.

	Written by:	Newton Research Group, 2010.
*/

#include "TestAgent.h"
#include "Debugger.h"
#include "Funcs.h"
#include "Dates.h"
#include "Unicode.h"
#include "UserPorts.h"

#include <stdarg.h>


/* -----------------------------------------------------------------------------
	D a t a

0c1017b4	gDebuggeeState
0c1017b8	gPCSampleProc
0c1017bc	gPCSampleInterval
0c1017c0	gPCSampleEnumerateOS
0c1017c4	gWantSerialDebugging
0c1017cc	gDebugLink
0c1017e8	gTestReceiveEchoIntCount
0c1017ec	gTestReceiveEchoSendCount
0c1017f0	gTestReceiveEchoError
0c1017f4	gReceiveDebugFrameErrorCount
0c1017f8	gReceiveDebugFrameLastError
----------------------------------------------------------------------------- */

ULong					gDebuggerBits = 8;			// 000013F4
															// 0x00000001 => debugger is present
															// 0x00000008 => use ARMistice card as tablet
															// 0x00000020 => use external serial debug
															// 0x00000040 => use GeoPort debug
ULong					gNewtTests = 0;				// 000013F8

bool					gWantSerialDebugging = false;	//	0C1017C4

CAgentReporter *	gTestReporterForNewt;		// 0C104D48
Ref					gTestMgrAppContext;			// 0C104D50

void *				gTestAgentMessageQueue;		// 0C104D54
void *				gtspsPartHandler;				// 0C104D58
void *				gTestCasePartAddress;		// 0C104D60


/* -----------------------------------------------------------------------------
	T e s t   A g e n t
----------------------------------------------------------------------------- */
#if 0
class CTestAgent : public CAppWorld
{
public:
							CTestAgent();

	virtual size_t		getSizeOf(void) const;

	virtual NewtonErr	forkInit(CForkWorld * inWorld);
	virtual NewtonErr	forkConstructor(CForkWorld * inWorld);
	virtual void		forkDestructor(void);

	virtual NewtonErr	mainConstructor(void);
	virtual void		mainDestructor(void);

	void		allocateATestReporter(CAgentReporter**);
	void		agentReportDirect(unsigned long, char*);
	void		setup(char*, char*);	// creates a CCommServer to talk to desktop test controller
	void		startCTestCase(void);
	void		startACardTestCase(void);
	void		processTestServerCommand(unsigned long, long);
	void		makeTestStore(bool);
	void		storeTestCommand(unsigned long);
	void		storeTestOrDataFile(unsigned long, unsigned long);
	void		doRunTestsFromStore(bool);
	void		doDropConnection(void);
	void		processTestMgrParameters(long);
	void		eventHandlerProc(CUMsgToken*, unsigned long*, CTestAgentEvent*);
	void		idleProc(void);

protected:
	virtual CForkWorld *	makeFork(void);
};

class CTestAgentEventHandler : public CEventHandler
{
public:
	void		eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	void		eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	void		idleProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
};


class CNewtTestScriptEventHandler : public CEventHandler
{
public:
	void		eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	void		eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
};


Ref		FActivateTestAgent(RefArg inRcvr);
Ref		FDeactivateTestAgent(RefArg inRcvr);
void		DoNewtCTestCase(ClassInfo*, char*);

class CtspsPart : public CPartHandler
{
public:
							CtspsPart();
							~CtspsPart();

	virtual NewtonErr	install(const PartId & inPartId, SourceType inSource, PartInfo * info);
	virtual NewtonErr	remove(const PartId & inPartId, PartType inType, RemoveObjPtr inContext);

	virtual NewtonErr	installFrame(RefArg inPartFrame, const PartId & inPartId, SourceType inSource, PartInfo * info) = 0;
	virtual NewtonErr	removeFrame(RefArg inContext, const PartId & inPartId, PartType inSource) = 0;
};

FTestGetParameterString
FTestGetParameterArray
FTestReportError
FTestReportMessage
FTestReadTextFile
FTestReadDataFile
FTestFlushReportQueue
FTestMSetParameterString
FTestWillCallExit
FTestExit
FTestMStartTestFrame
FTestMStartTestCase
FTestMGetReportMsg
FTestMDropConnection

class CTestCommandQueue
{
public:
				CTestCommandQueue(CStore*);

	void		enqueueTestCommand(unsigned long, char*, char*);
	void		dequeueTestCommand(unsigned long*, char*, char*);
};

class CTestStoreFileList
{
public:
				CTestStoreFileList(CStore*);
				~CTestStoreFileList();

	void		add(char*, char*, unsigned long);
	void		get(char*, char**, unsigned long*, unsigned long, unsigned long);
	void		deleteAll(void);
	void		fileNameSum(char*);
	void		fileNamesEqual(char*, char*);
};

class CTestCaseTask : public CAppWorld
{
public:
	virtual size_t		getSizeOf(void) const;

	virtual NewtonErr	mainConstructor(void);
	virtual void		mainDestructor(void);

	void		init2(unsigned long, ClassInfo*, CAgentReporter*, char*, char*);
	void		getArgcArgv(short*, char**);
	void		eventHandlerProc(CTestCaseTaskEvent*);

protected:
	virtual CForkWorld *	makeFork(void);
};

class CTestCaseTaskEventHandler : public CEventHandler
{
public:
	void		eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	void		eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
};

#endif


bool
IsDebuggerPresent(void)
{
	return FLAGTEST(gDebuggerBits, kDebuggerIsPresent)
		 || gWantSerialDebugging;
}

void
SRegisterPackageWithDebugger(void * inAddr, ULong inPkgId)
{}

void
SRegisterLoadedCodeWithDebugger(void * inAddr, const char * infoStr, ULong inId)
{}

void
SDeregisterLoadedCodeWithDebugger(ULong inId)
{}

void
SInformDebuggerMemoryReloaded(void * inAddr, ULong inSize)
{}


NewtonErr
InitTestAgent(void)
{
#if defined(correct)
	NewtonErr err;
	XTRY
	{
		CUNameServer ns;	// sp08
		OpaqueRef thing, spec;
		XFAIL((err = ns.lookup("tagt", kUPort, &thing, &spec)) == noErr)	// haha!
		//sp-178
		CTestAgent testAgent;
		XFAILIF(err = testAgent.init('tagt', true, kSpawnedTaskStackSize), printf("Could not init CTestAgent\n"))
	}
	XENDTRY;
	return err;
#else
	return noErr;
#endif
}


void
HandleTestAgentEvent()
{ /* this really does nothing */ }


#if !defined(forFramework)
#pragma mark -
/* -----------------------------------------------------------------------------
	T e s t   R e p o r t e r
----------------------------------------------------------------------------- */

CTestReporterEvent::CTestReporterEvent(ULong inType, ULong inArg2)
	:	CEvent('tste'), f08(inType), f10(inArg2)
{ }

#pragma mark -

CTestReporter::CTestReporter(ULong inArg1, ObjectId inPortId, ULong inArg3)
{
	f190 = 1;
	fPortId = inPortId;
	f18C = inArg1;
	f60 = 0;
	fNumOfErrorsLogged = 0;
	fNumOfErrorsReported = 0;
}


CTestReporter::~CTestReporter()
{ }


void
CTestReporter::sendToTestAgent(ULong inType, const char * inMsg, long inErr)
{
	CUPort port(fPortId);
	CTestReporterEvent evt(inType, f18C);
	if (inMsg)
		strncpy(evt.fMsg, inMsg, kTestAgentMessageLen);
	else
		evt.fMsg[0] = 0;

	port.send(&evt, sizeof(CTestReporterEvent));
}


void
CTestReporter::testReportError(char * inErrMsg, const char * inExplanation, NewtonErr inErr)
{
	char buf[256];

	if (strlen(inErrMsg) > 200)
		inErrMsg[200] = 0;

	fNumOfErrorsReported++;

	if (inExplanation)
		sprintf(buf, "Test Case ERR: %d\t%s\t%s\n", inErr, inExplanation, inErrMsg);
	else
	{
		if (inErr)
			sprintf(buf, "Test Case ERR: %d\t%s\n", inErr, inErrMsg);
		else
			sprintf(buf, "Test Case ERR: %s\n", inErrMsg);
	}

	sendToTestAgent(4, buf, 0);
}


void
CTestReporter::testReportErrorValues(char * inErrMsg, const char * inExplanation, long inActualValue, long inExpectedValue)
{
	char buf[256];
	char errs[128];

	if (strlen(inErrMsg) > 200)
		inErrMsg[200] = 0;

	fNumOfErrorsReported++;

	sprintf(errs, "               got: %ld (0x%016lX), expected %ld (0x%016lX)", inActualValue, inActualValue, inExpectedValue, inExpectedValue);
	if (inExplanation)
		sprintf(buf, "Test Case ERR: %s\t%s\n%s\n", inExplanation, inErrMsg, errs);
	else
		sprintf(buf, "Test Case ERR: %s\n%s\n", inErrMsg, errs);

	sendToTestAgent(4, buf, 0);
}


void
CTestReporter::testReportMessage(char * inErrMsg, const char * inExplanation)
{
	char buf[256];
	
	if (strlen(inErrMsg) > 200)
		inErrMsg[200] = 0;

	if (inExplanation)
		sprintf(buf, "Test Case MSG: %s\t%s\n", inExplanation, inErrMsg);
	else
		sprintf(buf, "Test Case MSG: %s\n", inErrMsg);

	sendToTestAgent(3, buf, 0);
}


void
CTestReporter::testFPrintf(int inArg1, const char * inFormat, ...)
{
	char buf[256];

	va_list args;
	va_start(args, inFormat);
	vsprintf(buf, inFormat, args);
	va_end(args);

	if (inArg1 == 2)
		sendToTestAgent(3, buf, 0);
	else
	{
		fNumOfErrorsReported++;
		sendToTestAgent(4, buf, 0);
	}
}


NewtonErr
CTestReporter::testReadDataFile(const char * inFileName, long inArg2, size_t * outDataSize, Ptr * outData)
{
	NewtonErr err = noErr;
	XTRY
	{
		if (inArg2 == -1)
		{
//			StdioOn();
			FILE * fd;
			XFAILIF((fd = fopen(inFileName, "r")) == 0, err = -2;)
			fseek(fd, 0, SEEK_END);
			*outDataSize = ftell(fd);
			fseek(fd, 0, SEEK_SET);
			*outData = NewPtr(*outDataSize + 4);	// original does no XFAIL testing
			fread(*outData, 1, *outDataSize, fd);
			fclose(fd);
//			StdioOff();
		}

		else if (NOTNIL(gTestMgrAppContext) && this == gTestReporterForNewt)		// original tests != nil
		{
			UniChar filenameStr[256];
			RefVar data;
			ConvertToUnicode(inFileName, filenameStr);
			RefVar args(MakeArray(3));
			SetArraySlot(args, 0, MakeString(filenameStr));
			SetArraySlot(args, 1, MAKEINT(inArg2));
			SetArraySlot(args, 1, MAKEINT(*outDataSize));
			data = DoMessage(gTestMgrAppContext, MakeSymbol("testMgrReadDataFile"), args);
			*outDataSize = Length(data);
			XFAIL((*outData = NewPtr(*outDataSize + 4)) == NULL)
			memmove(*outData, BinaryData(data), *outDataSize);
		}

		else
		{
			struct
			{
				int		x00;
				size_t	x04;
				Ptr		x08;
			} reply;
			size_t replySize;
			CTestReporterEvent request(9, f18C);
			request.f18 = inArg2;
			request.f0C = 0;
			request.f1C = *outDataSize;
			if (inFileName)
				strncpy(request.fMsg, inFileName, kTestAgentMessageLen);
			else
				request.fMsg[0] = 0;

			CUPort thePort(fPortId);
			XFAIL(err = thePort.sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply)))

			*outDataSize = reply.x04;
			*outData = reply.x08;
		}
	}
	XENDTRY;
	return err;
}


void
CTestReporter::testFlushReportQueue(void)
{
	sendToTestAgent(10, NULL, 0);
}


#pragma mark -
/* -----------------------------------------------------------------------------
	T e s t   A g e n t   R e p o r t e r
----------------------------------------------------------------------------- */

CAgentReporter::CAgentReporter(ULong inArg1, ObjectId inPortId, ULong inArg3)
	:	CTestReporter(inArg1, inPortId, inArg3)
{ }


CAgentReporter::~CAgentReporter()
{ }


void
CAgentReporter::agentReportError(char * inErrMsg, const char * inExplanation, NewtonErr inErr)
{
	fNumOfErrorsReported++;

	char buf[256];
	sprintf(buf, "Test Agent ERR: %d\t%s\t%s\n", inErr, inExplanation, inErrMsg);
	sendToTestAgent(2, buf, 0);
}


void
CAgentReporter::agentReportStatus(int inSelector, const char * inTestName)
{
	int len;
	char buf[256];
	UniChar uDateStr[32];
	UniChar uTimeStr[32];
	char dateStr[32];
	char timeStr[32];

	CDate now;
	now.setCurrentTime();
	now.shortDateString(kIncludeAllElements, uDateStr, 32);
	now.timeString(kIncludeAllElements, uTimeStr, 32);

	ConvertFromUnicode(uDateStr, &dateStr, 32);
	ConvertFromUnicode(uTimeStr, &timeStr, 32);

	if (inTestName == NULL)
		inTestName = "";

	switch (inSelector)
	{
	case 2:
	case 5:
	case 8:
		sprintf(buf, "Test Agent MSG: test %s started in %s at %s, %s\n", inTestName, (inSelector == 5) ? "ttsk" : "newt", timeStr, dateStr);
		break;

	case 4:
	case 6:
	case 9:
		reportMemoryInfo();
		len = sprintf(buf, "Test Agent MSG: test %s finished at %s, %s\n", inTestName, timeStr, dateStr);
		sprintf(buf + len, ".....%d errors reported, %d errors logged\n", fNumOfErrorsReported, MIN(fNumOfErrorsLogged, fNumOfErrorsReported));
		break;

	default:
		strncpy(buf, inTestName, kTestAgentMessageLen);
		break;
	}
	sendToTestAgent(5, buf, inSelector);
	if (inSelector == 2 || inSelector == 5)
		reportMemoryInfo();
}


void
CAgentReporter::reportMemoryInfo(void)
{
	char buf[256];
	sprintf(buf, "Test Agent MSG: TotalSystemFree=%ld\n", TotalSystemFree());
	sendToTestAgent(1, buf, 0);
}

#endif
