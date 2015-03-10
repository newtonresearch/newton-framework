/*
	File:		CMService.h

	Copyright:	� 1992, 1994-1995 by Apple Computer, Inc., all rights reserved.
*/

#if !defined(__CMSERVICE_H)
#define __CMSERVICE_H 1

#ifndef	__NEWTON_H
#include "Newton.h"
#endif

#ifndef __EVENTS_H
#include "Events.h"
#endif

#ifndef __PROTOCOLS_H
#include "Protocols.h"
#endif

#ifndef	__USERPORTS_H
#include "UserPorts.h"
#endif

#ifndef	__COMMMANAGERINTERFACE_H
#include "CommManagerInterface.h"
#endif

#ifndef __OPTIONARRAY_H
#include "OptionArray.h"
#endif

#define kServiceInterfaceName		"CCMService"

class CCMService;
class CUMsgToken;
class CServiceInfo;

extern NewtonErr ServiceToPort(ULong serviceId, CUPort* port);		// DON'T USE; old style, won't work
extern NewtonErr ServiceToPort(ULong serviceId, CUPort* port, ObjectId taskId);	// task Id of service


/*--------------------------------------------------------------------------------
	CAsyncServiceMessage
--------------------------------------------------------------------------------*/

class CAsyncServiceMessage
{
public:
						CAsyncServiceMessage();
						~CAsyncServiceMessage();

	CCMService *	service();

	NewtonErr		init(CCMService * service);
	NewtonErr		send(CUPort * destination, void * message, size_t messageSize, void * reply, size_t replySize, ULong messageType = 0);

	bool				match(CUMsgToken * token);		// used internally

//private:
	CCMService *	fService;
	CUAsyncMessage	fAsyncMessage;
	void *			fMessage;
	void *			fReply;
};

inline CCMService * CAsyncServiceMessage::service()	{ return fService; }


/*--------------------------------------------------------------------------------
	CCMService

	Each communications/network service should have a CCMService implementation.
	Each service should implement:

	start:			to start the associated service
						returns
						noErr:					completed immediately with noErr
						kCall_In_Progress		in process, doneStarting will be called
						or some error			failed immediately because of reported error

	doneStarting:	called when start has completed

	** possible additions **
	terminate:		to terminate the associated service
	message:			when the system needs to inform the service of some system event
	setOptions:		to set "configuration" options
	getOptions: 	to get the current set of configuration options
--------------------------------------------------------------------------------*/

PROTOCOL CCMService : public CProtocol
{
public:

	static CCMService *	make(const char * inName);
	void			destroy(void);

	NewtonErr	start(COptionArray* options, ULong serviceId, CServiceInfo* serviceInfo);	// start the service
	NewtonErr	doneStarting(CEvent* event, ULong size, CServiceInfo* serviceInfo);			// called back when done starting
};


class CCommTool;
NewtonErr StartCommTool(CCommTool * inTool, ULong inName, CServiceInfo * info);
NewtonErr OpenCommTool(ULong inName, COptionArray * inOptions, CCMService * inService);

#endif	/* __CMSERVICE_H */
