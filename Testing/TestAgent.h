/*
	File:		TestAgent.h

	Contains:	Test reporting system.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__TESTAGENT_H)
#define __TESTAGENT_H 1

#include "Newton.h"
#include "Events.h"


/* -----------------------------------------------------------------------------
	T e s t   R e p o r t e r   E v e n t
----------------------------------------------------------------------------- */

#define kTestAgentMessageLen 224

class CTestReporterEvent : public CEvent
{
public:
				CTestReporterEvent(ULong inType, ULong inArg2);

	ULong		f08;
	ULong		f0C;
	ULong		f10;
	ULong		f18;
	size_t	f1C;
	char		fMsg[kTestAgentMessageLen];
// size +010C
};


/* -----------------------------------------------------------------------------
	T e s t   R e p o r t e r
----------------------------------------------------------------------------- */

class CTestReporter
{
public:
					CTestReporter(ULong, ObjectId inPortId, ULong);
					~CTestReporter();

	void			sendToTestAgent(ULong inType, const char * inMsg, long inErr);
	void			testReportError(char * inErrMsg, const char * inExplanation, NewtonErr inErr);
	void			testReportErrorValues(char * inErrMsg, const char * inExplanation, long inActualValue, long inExpectedValue);
	void			testReportMessage(char * inErrMsg, const char * inExplanation);
	void			testFPrintf(int inArg1, const char * inFormat, ...);
	NewtonErr	testReadDataFile(const char * inFileName, long inArg2, size_t * outDataSize, Ptr * outData);
	void			testFlushReportQueue(void);

protected:
	char		f60;
	ObjectId	fPortId;							// +188
	ULong		f18C;
	ULong		f190;
	ULong		fNumOfErrorsLogged;			// +198
	ULong		fNumOfErrorsReported;		// +19C
// size +1A0
};


/* -----------------------------------------------------------------------------
	T e s t   A g e n t   R e p o r t e r
----------------------------------------------------------------------------- */

class CAgentReporter : public CTestReporter
{
public:
				CAgentReporter(ULong, ObjectId inPortId, ULong);
				~CAgentReporter();

	void		agentReportError(char * inErrMsg, const char * inExplanation, NewtonErr inErr);
	void		agentReportStatus(int inSelector, const char * inTestName);
	void		reportMemoryInfo(void);
};


extern CAgentReporter *	gTestReporterForNewt;


#endif	/* __TESTAGENT_H */
