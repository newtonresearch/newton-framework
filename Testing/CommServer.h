/*
	File:		CommServer.h

	Contains:	Desktop communications for the test reporting system.

	Written by:	Newton Research Group, 2012.
*/

#if !defined(__COMMSERVER_H)
#define __COMMSERVER_H 1

#include "Newton.h"
#include "Docker.h"

#define kTestServerEventId		'tsvr'
#define kTestAgentEventId		'tagt'

#define kStreamAlignment	4


/* -----------------------------------------------------------------------------
	C C o m m S e r v e r
----------------------------------------------------------------------------- */

class CCommServer : public CEzPipeProtocol
{
public:
					CCommServer(bool inArg1, bool inArg2, UniChar * inType);
					~CCommServer();

	void			setTestServerName(UniChar * inEntity, UniChar * inZone);	// args were char* !
	NewtonErr	connectToTestServer(CEzEndpointPipe ** outPipe);
	NewtonErr	disconnectFromTestServer(void);

	void			setBusy(bool inBusy);
	bool			isBusy(void);

	EventType	getResponse(size_t * outLength);
	bool			readChunk(void * inBuf, size_t inLength, bool inDone);
	NewtonErr	readString(char * inStr, size_t inLength, bool inDone);
	void			flushPadding(size_t inLength);

	void			sendCommandHeader(EventType inCommand, bool inDone);
	void			sendChunk(void * inBuf, size_t inLength, bool inDone);
	void			pad(size_t inLength);

	void			testPipeExceptionHandler(NewtonErr inErr);

private:
	bool			f0C;
	bool			f0D;
	size_t		fEvtLength;		//+10
	UniChar		fATType[32+1];		//+14
	UniChar		fATEntity[32+1];	//+56
	UniChar		fATZone[32+1];		//+98
	NewtonErr	fError;			//+DC
	bool			fIsBusy;			//+E0
};


#endif	/* COMMSERVER */
