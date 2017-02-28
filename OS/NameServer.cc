/*
	File:		NameServer.cc

	Contains:	NameServer.

	Written by:	Newton Research Group.
*/

#include "QDTypes.h"		// Gestalt needs to know screen dimensions

#include "NameServer.h"
#include "SystemEvents.h"
#include "RAMInfo.h"
#include "VirtualMemory.h"
#include "Screen.h"
#include "Tablet.h"
#include "OSErrors.h"


/*--------------------------------------------------------------------------------
	C R e s A r b i t r a t i o n I n f o
	Private to the name server.
--------------------------------------------------------------------------------*/
#include "CommTool.h"

class CResArbitrationInfo
{
public:
					CResArbitrationInfo();
					~CResArbitrationInfo();

	NewtonErr	init(CUPort * inPort);

	ULong							f00;				// flags? 0x01 = claim in progress
	CResOwnerInfo *			f04;
	CResOwnerInfo *			f08;
	CCommToolResArbRequest	f0C;
	CUAsyncMessage				f20;				// +20
	CCommToolReply				f30;				// +30	reply, size+10
	CUMsgToken					f40;				// +40
	ObjectId						f50;				// +50	owner port
	ObjectId						f54;				// +54	owner name
	CRPCInfo						f58;				// +58
	long							f60;
// size +64
};



/*--------------------------------------------------------------------------------
	D a t a
--------------------------------------------------------------------------------*/
extern ULong	gHardwareType;
extern ULong	gROMManufacturer;
extern ULong	gROMVersion;
extern ULong	gROMStage;
extern ULong	gMainCPUType;
extern float	gMainCPUClockSpeed;
extern ULong	gManufDate;
extern ULong	gNewtonScriptVersion;


// type string for port lookup
const char kUPort[] = "CUPort";

// type string for store lookup
const char kRegisteredStore[] = "CStore";

//	type string for gestalt lookup
const char kRegisteredGestalt[] = "GSLT";


CUPort *	gNameServer;


/*--------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
--------------------------------------------------------------------------------*/

NewtonErr
InitializeNameServer(void)
{
	CNameServer	nameServer;
	return nameServer.startTask(true, false, kNoTimeout, kSpawnedTaskStackSize, kUserTaskPriority, 'name');
}


bool
GetOSPortFromName(ObjectName inName, CUPort * ioPort)
{
	CUNameServer	ns;
	OpaqueRef		thing;
	OpaqueRef		spec;

	MAKE_ID_STR(inName,name);
	bool	isThere = ns.lookup(name, kUPort, &thing, &spec);
	*ioPort = (ObjectId) thing;
	return isThere;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C N a m e S e r v e r
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Return the size of this class.
	Args:		--
	Return:	number of bytes
--------------------------------------------------------------------------------*/

size_t
CNameServer::getSizeOf(void) const
{
	return sizeof(CNameServer);
}


/*--------------------------------------------------------------------------------
	Prepare to serve some names.
	Args:		--
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNameServer::taskConstructor(void)
{
	NewtonErr err;
	XTRY
	{
		fSysEvents = NULL;
		fSysEventIter = NULL;
		fInfo.fType = 2;
		XFAIL(err = fPort.init())
		XFAIL(err = fReplyMem.init())
		gNameServer = &fPort;
		fSysEvents = new CList;
		XFAILIF(fSysEvents == NULL, err = kOSErrNoMemory;)
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	The name server’s main event loop.
	It waits for CResArbitrationRequest requests to arrive on its port and
	dispatches them appropriately.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CNameServer::taskMain(void)
{
	NewtonErr	err;
	CResArbitrationRequest	request;
	CNameServerReply			reply;
	size_t		size;
	CUMsgToken	token;
	ULong			msgType;
	bool			isReplyWanted;

	for ( ; ; )
	{
	//	wait for a request to arrive
		err = fPort.receive(&size, &request, sizeof(request), &token, &msgType, kNoTimeout, (fSysEventIter == NULL) ? kMsgType_MatchAll : kMsgType_MatchAll-1);

		if ((msgType & kMsgType_CollectedSender) != 0)
		{
			CRPCInfo *	info;
			token.getUserRefCon((OpaqueRef*)&info);
			if (info->fType == 1)
			{
				resArbHandleReply((CResArbitrationInfo *)info->fData);
			}
			else if (info->fType == 2)
			{
				resArbHandleReply((CResArbitrationInfo *)info->fData);
				SysEvent *	eventInfo;
				while ((eventInfo = (SysEvent *)fSysEventIter->nextItem()) != NULL)
				{
					fSysEventPort = eventInfo->fSysEventObjId;
					if (fSysEventPort.sendRPC(&fSysEventMsg, kPortSend_BufferAlreadySet, 0, NULL, 0, eventInfo->fSysEventSendFilter, NULL, eventInfo->fSysEventTimeOut) != noErr)
						break;
				}
				if (eventInfo == NULL)
				{
				// all sends were okay
					if (fSysEventIter != NULL)
					{
						delete fSysEventIter;
						fSysEventIter = NULL;
					}
					fSysEventToken.replyRPC(kPortSend_BufferAlreadySet, 0);
				}
			}
		}
		else if (err == noErr)
		{
			isReplyWanted = true;
			switch (request.fCommand)
			{
			case kRegisterForSystemEvent:
				err = registerForSystemEvent(request.fThing, request.fSpec, request.fParam1, request.fParam2);
				token.replyRPC(&reply, sizeof(reply), err);
				break;

			case kUnregisterForSystemEvent:
				err = unregisterForSystemEvent(request.fThing, request.fSpec);
				token.replyRPC(&reply, sizeof(reply), err);
				break;

			case kSendSystemEvent:
				fSysEventToken = token;
				err = sendSystemEvent(request.fThing, request.fSpec);
				if (err == noErr)
					isReplyWanted = false;
				token.replyRPC(&reply, sizeof(reply), err);
				break;

			case kGestalt:
			//	request is NOT a CResArbitrationRequest, it’s a CGestaltRequest
				gestalt(((CGestaltRequest *)&request)->fSelector, &token);
				break;

			default:
				if ((err = buildNameAndType(request.fObjectName, request.fObjectType)) == noErr)
					switch (request.fCommand)
					{
					case kRegisterName:
						err = registerName(request.fThing, request.fSpec);
						break;
					case kUnregisterName:
						err = unregisterName();
						break;
					case kWaitForRegister:
						err = queueForRegister(&token);
						break;
					case kWaitForUnregister:
						err = queueForUnregister(&token);
						break;
					case kLookup:
						err = lookup(&reply.fThing, &reply.fSpec);
						break;
					case kResourceArbitration:
						isReplyWanted = false;
						resourceArbitration(token, &request);
						break;
					default:
						err = kOSErrBadParameters;
						break;
					}
				deleteNameAndType();
				if (isReplyWanted)
					token.replyRPC(&reply, sizeof(reply), err);
				break;
			}
		}
	}
}


/*--------------------------------------------------------------------------------
	Hash a name for indexing into the CNameObjectEntry table.
	Args:		inName		a C string
	Return:	index
--------------------------------------------------------------------------------*/

ULong
CNameServer::hash(const char * inName)
{
	ULong	n = 0;
	UChar	c;
	while ((c = *inName++) != 0)
		n += c;
	return n & kObjectNameHashBucketMask;
}


/*--------------------------------------------------------------------------------
	Build name and type buffers from shared memory objects of the same.
	This is done for every name server request that needs name/type key access.
	Args:		inName		id of name object
				inType		id of type object
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNameServer::buildNameAndType(ObjectId inName, ObjectId inType)
{
	NewtonErr err;

	XTRY
	{
		CUSharedMem	nameObject;
		CUSharedMem	typeObject;
		size_t		size;

		// name and type pointers are NULL until we’ve copied them
		fName = NULL;
		fType = NULL;
		// set up shared mem objects from ids
		nameObject = inName;
		typeObject = inType;
		// allocate name buffer and copy contents of name object
		XFAIL(err = nameObject.getSize(&size))
		XFAILNOT(fName = NewPtr(size), err = kOSErrNoMemory;)
		XFAIL(err = nameObject.copyFromShared(&size, (void*)fName, size))
		// allocate type buffer and copy contents of type object
		XFAIL(err = typeObject.getSize(&size))
		XFAILNOT(fType = NewPtr(size), err = kOSErrNoMemory;)
		err = typeObject.copyFromShared(&size, (void*)fType, size);
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Delete name and type buffers.
	This is done when the request has finished with the name/type key.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CNameServer::deleteNameAndType(void)
{
	FreePtr((Ptr)fName);
	FreePtr((Ptr)fType);
}


/*--------------------------------------------------------------------------------
	Register the thing/spec data associated with the name/type key, which has
	already been set up by buildNameAndType().
	Args:		inThing		data
				inSpec		…and more
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNameServer::registerName(OpaqueRef inThing, OpaqueRef inSpec)
{
	NewtonErr err = noErr;
	XTRY
	{
		OpaqueRef whatever;
		CObjectNameList *	db = &fDB[hash(fName)];
		XFAILIF(db->lookup(fName, fType, &whatever, &whatever, NULL) != noErr || !db->add(fName, fType, inThing, inSpec), err = kOSErrAlreadyRegistered;)
		// NULL out the key pointers so they’re not freed
		// we want them to stay in the CObjectNameEntry
		fName = fType = NULL;
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Unregister a name (and its data).
	Args:		--
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNameServer::unregisterName(void)
{
	NewtonErr err = noErr;
	XTRY
	{
		OpaqueRef whatever;
		CObjectNameList *	db = &fDB[hash(fName)];
		XFAILIF(db->lookup(fName, fType, &whatever, &whatever, NULL) == noErr || !db->remove(fName, fType), err = kOSErrNotRegistered;)
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Wait for a name to become registered.
	Args:		ioToken		for replying
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNameServer::queueForRegister(CUMsgToken * ioToken)
{
	NewtonErr err = noErr;
	XTRY
	{
		OpaqueRef thing, spec;

		if (lookup(&thing, &spec) == noErr)
		{
			//	name already exists, reply immediately
			CNameServerReply	reply;
			reply.fThing = thing;
			reply.fSpec = spec;
			ioToken->replyRPC(&reply, sizeof(reply));
		}
		else
		{
			//	name isn’t there yet; form an orderly queue to wait for it
			CRegistryListener * listener = new CRegistryListener;
			XFAILIF(listener == NULL, err = kOSErrNoMemory;)
			listener->fName = fName;
			listener->fType = fType;
			listener->fMsgToken = *ioToken;
			// thread new listener onto front of queue
			CObjectNameList *	db = &fDB[hash(fName)];
			listener->fNext = db->fRegisterListeners;
			db->fRegisterListeners = listener;
			// NULL out keys; we must retain them
			fName = NULL;
			fType = NULL;
		}
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Wait for a name to become unregistered.
	Args:		ioToken		for replying
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNameServer::queueForUnregister(CUMsgToken * ioToken)
{
	NewtonErr err = noErr;
	XTRY
	{
		OpaqueRef thing, spec;

		if (lookup(&thing, &spec) != noErr)
		{
			//	name isn’t registered; reply immediately with an error
			ioToken->replyRPC(NULL, 0, kOSErrNotRegistered);
		}
		else
		{
			//	name is still there; form an orderly queue to wait for it to go
			CRegistryListener * listener = new CRegistryListener;
			XFAILIF(listener == NULL, err = kOSErrNoMemory;)
			listener->fName = fName;
			listener->fType = fType;
			listener->fMsgToken = *ioToken;
			// thread new listener onto front of queue
			CObjectNameList *	db = &fDB[hash(fName)];
			listener->fNext = db->fUnregisterListeners;
			db->fUnregisterListeners = listener;
			// NULL out keys; we must retain them
			fName = NULL;
			fType = NULL;
		}
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Look up a name in the registry.
	Args:		outThing		where to return the data
				outSpec
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNameServer::lookup(OpaqueRef * outThing, OpaqueRef * outSpec)
{
	return fDB[hash(fName)].lookup(fName, fType, outThing, outSpec, NULL) ? noErr : kOSErrNotRegistered;
}


/*--------------------------------------------------------------------------------
	Build a resource arbitration record and add it to the given name entry.
	Args:		ioEntry				a name entry
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNameServer::resArbBuildResArbInfo(CObjectNameEntry * ioEntry)
{
	NewtonErr err = noErr;
	XTRY
	{
		// create and initialize a resource arbitration record
		ioEntry->fResArbInfo = new CResArbitrationInfo;
		XFAILIF(ioEntry->fResArbInfo == NULL, err = MemError();)
		err = ioEntry->fResArbInfo->init(&fPort);
	}
	XENDTRY;

	// if there was an error, better delete it
	XDOFAIL(err)
	{
		resArbDeleteResArbInfo(ioEntry);
	}
	XENDFAIL;
	return err;
}


/*--------------------------------------------------------------------------------
	Delete the resource arbitration record from the given name entry.
	Args:		ioEntry				a name entry
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNameServer::resArbDeleteResArbInfo(CObjectNameEntry * ioEntry)
{
	if (ioEntry->fResArbInfo != NULL)
	{
		delete ioEntry->fResArbInfo;
		ioEntry->fResArbInfo = NULL;
	}
	return noErr;
}


/*--------------------------------------------------------------------------------
	Create a resource owner record.
	Args:		ioOwner				the resource owner record
				inName				id of owner name object
				inPortId				id of port
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNameServer::resArbBuildResOwnerInfo(CResOwnerInfo *& ioOwner, ObjectId inName, ObjectId inPortId)
{
	NewtonErr err = noErr;
	XTRY
	{
		ioOwner = new CResOwnerInfo;
		XFAILIF(ioOwner == NULL, err = MemError();)
		*ioOwner = inPortId;
		if (inName != kNoId)
		{
			CUSharedMem	name(inName);
			size_t		size;

			XFAIL(err = name.getSize(&size))
			XFAILNOT(ioOwner->f08 = NewPtr(size), err = MemError();)
			err = name.copyFromShared(&size, ioOwner->f08, size);
		}
	}
	XENDTRY;

	// if there was an error, better delete it
	XDOFAIL(err)
	{
		resArbDeleteResOwnerInfo(ioOwner);
	}
	XENDFAIL;
	return err;
}


/*--------------------------------------------------------------------------------
	Delete a resource owner record.
	Args:		ioOwner				the resource owner record
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNameServer::resArbDeleteResOwnerInfo(CResOwnerInfo *& ioOwner)
{
	if (ioOwner != NULL)
	{
		delete ioOwner;
		ioOwner = NULL;
	}
	return noErr;
}


/*--------------------------------------------------------------------------------
	Handle a resource arbitration request for the current name.
	Args:		ioToken			for replying
				ioRequest		the request
	Return:	--
--------------------------------------------------------------------------------*/

void
CNameServer::resourceArbitration(CUMsgToken & ioToken, CResArbitrationRequest * ioRequest)
{
	NewtonErr			err = noErr;
	CObjectNameEntry *entry;
	CNameServerReply	reply;
	OpaqueRef			thing;
	OpaqueRef			spec;

	if (fDB[hash(fName)].lookup(fName, fType, &thing, &spec, &entry))
	{
		CResArbitrationInfo *	arbInfo = entry->fResArbInfo;
		CResOwnerInfo *			owner;
		switch (ioRequest->fRequestType)
		{
		case kResArbitrationClaim:
			if (arbInfo != NULL)
			{
				if ((arbInfo->f00 & 0x01) != 0)
				{
					err = kOSErrResourceClaimed;
					break;
				}
				if ((owner = arbInfo->f08) != NULL
				||  (owner = arbInfo->f04) != NULL)
				{
					arbInfo->f0C.fOpCode = kCommToolResArbRelease;
					arbInfo->f0C.fResNamePtr = (UChar *)entry->fName;
					arbInfo->f0C.fResTypePtr = (UChar *)entry->fType;
					if ((err = owner->sendRPC(&arbInfo->f20, &arbInfo->f0C, sizeof(CCommToolResArbRequest), &arbInfo->f30, sizeof(CCommToolReply), 30*kSeconds, NULL, 0x40/*msgType*/)) == noErr)
					{
						arbInfo->f40 = ioToken;
						arbInfo->f50 = ioRequest->fOwnerPortId;
						arbInfo->f54 = ioRequest->fOwnerName;
						arbInfo->f00 |= 0x01;
						return;
					}
				}
			}
			else
			{
			// gotta build resArbInfo first
				if ((err = resArbBuildResArbInfo(entry)) == noErr)
					break;
				arbInfo = entry->fResArbInfo;
			}

			if ((err = resArbBuildResOwnerInfo(arbInfo->f08, ioRequest->fOwnerName, ioRequest->fOwnerPortId)) != noErr)
			{
				if (arbInfo->f04 == NULL && arbInfo->f08 == NULL)
					resArbDeleteResArbInfo(entry);
			}
			break;

		case kResArbitrationUnclaim:
			if (arbInfo != NULL)
			{
				resArbDeleteResOwnerInfo(arbInfo->f08);
				if (arbInfo->f04 != NULL)
					resArbSendClaimNotification(entry);
				else
					resArbDeleteResArbInfo(entry);
			}
			break;

		case kResArbitrationPassiveClaim:
			if (arbInfo != NULL
			&& (arbInfo->f04 != NULL || arbInfo->f08 != NULL))
			{
				err = kOSErrResourceClaimed;
				break;
			}
			if (arbInfo == NULL)
			{
				if ((err = resArbBuildResArbInfo(entry)) != noErr)
					break;
				arbInfo = entry->fResArbInfo;
			}
			if ((err = resArbBuildResOwnerInfo(arbInfo->f04, ioRequest->fOwnerName, ioRequest->fOwnerPortId)) == noErr)
			{
				reply.fResult = noErr;
				ioToken.replyRPC(&reply, sizeof(reply));
				resArbSendClaimNotification(entry);
				return;
			}
			if (arbInfo->f04 == NULL && arbInfo->f08 == NULL)
				resArbDeleteResArbInfo(entry);
			break;

		case kResArbitrationPassiveUnclaim:
			if (arbInfo != NULL)
			{
				resArbDeleteResOwnerInfo(arbInfo->f04);
				if (arbInfo->f08 != NULL)
					resArbDeleteResArbInfo(entry);
			}
			break;

		default:
		//	bad request - don’t reply
			return;
		}
	}
	else
		err = kOSErrNotRegistered;

	reply.fResult = err;
	ioToken.replyRPC(&reply, sizeof(reply));
}


/*--------------------------------------------------------------------------------
	Send a resource claim notification.
	Args:		ioEntry			a name entry
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNameServer::resArbSendClaimNotification(CObjectNameEntry * ioEntry)
{
	NewtonErr err;
	XTRY
	{
		size_t						size;
		CResArbitrationInfo *	arbInfo = ioEntry->fResArbInfo;
		CResOwnerInfo *			owner = arbInfo->f04;

		arbInfo->f0C.fOpCode = 2;
		arbInfo->f0C.fResNamePtr = (UChar *)ioEntry->fName;
		arbInfo->f0C.fResTypePtr = (UChar *)ioEntry->fType;
		XFAIL(err = owner->sendRPC(&size, &arbInfo->f0C, sizeof(CCommToolResArbRequest), &arbInfo->f30, sizeof(CCommToolReply), 30*kSeconds, 0x40/*msgType*/))
		err = arbInfo->f30.fResult;
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Handle a resource claim reply.
	Args:		ioArbitrationInfo		the arbitration record
	Return:	--
--------------------------------------------------------------------------------*/

void
CNameServer::resArbHandleReply(CResArbitrationInfo * ioArbitrationInfo)
{
	NewtonErr			err;
	CNameServerReply	reply;
	if ((ioArbitrationInfo->f00 & 0x02) != 0)
	{
		reply.fResult = kOSErrNotRegistered;
		ioArbitrationInfo->f40.replyRPC(&reply, sizeof(reply));

		CObjectNameEntry	entry;
		entry.fResArbInfo = ioArbitrationInfo;
		resArbDeleteResArbInfo(&entry);
	}
	else if ((ioArbitrationInfo->f00 & 0x01) != 0)
	{
		ioArbitrationInfo->f00 &= ~0x01;
		if ((err = ioArbitrationInfo->f30.fResult) == noErr)
		{
			if (ioArbitrationInfo->f08 != NULL)
				resArbDeleteResOwnerInfo(ioArbitrationInfo->f08);
			resArbBuildResOwnerInfo(ioArbitrationInfo->f08, ioArbitrationInfo->f54, ioArbitrationInfo->f50);
		}
		reply.fResult = err;
		ioArbitrationInfo->f40.replyRPC(&reply, sizeof(reply));
	}
}


/*--------------------------------------------------------------------------------
	.
	Args:		inEvtId
				inArg2
				inArg3
				inArg4
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNameServer::registerForSystemEvent(ULong inEvtId, ULong inEventClass, ULong infEventId, ULong inSysEventType)
{
	NewtonErr err = noErr;
	XTRY
	{
		CSysEventTester	tester(inEvtId);
		ArrayIndex			index;
		CEventMasterListItem *	sysEvt;

		if ((sysEvt = (CEventMasterListItem *)fSysEvents->search(&tester, index)) == NULL)
		{
			// sys evt doesn’t exist yet so try to create one
			sysEvt = new CEventMasterListItem;
			XFAILIF(sysEvt == NULL, err = kOSErrNoMemory;)
			XFAIL(err = sysEvt->init(inEvtId))
			XFAIL(err = fSysEvents->insertFirst(sysEvt))
		}

		// add new system event to the sublist
		CEventSystemEvent *	event = new CEventSystemEvent;
		XFAILIF(event == NULL, err = kOSErrNoMemory;)
		event->fEventClass = inEventClass;
		event->fEventId = infEventId;
		event->fEventType = inSysEventType;
		XFAILNOT(sysEvt->fEventList->insertUnique(event), delete event; err = kOSErrAlreadyRegistered;)

		// if sublist is empty, delete it
		// well it could happen if mystery item creation failed
		if (sysEvt->fEventList->isEmpty())
		{
			fSysEvents->remove(sysEvt);
			delete sysEvt;
		}
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	.
	Args:		inEvtId
				inArg2
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNameServer::unregisterForSystemEvent(ULong inEvtId, ULong inArg2)
{
	NewtonErr err;
	XTRY
	{
		ULong					evtId;
		CSysEventTester	tester(inEvtId);
		ArrayIndex			index;
		CEventMasterListItem *	sysEvt;

		XFAILNOT(sysEvt = (CEventMasterListItem *)fSysEvents->search(&tester, index), err = kOSErrNotRegistered;)
		evtId = inArg2;
		sysEvt->fCmp.setTestItem(&evtId);

		CEventSystemEvent *	event;
		XFAILNOT(event = (CEventSystemEvent *)sysEvt->fEventList->search(&sysEvt->fCmp, index), err = kOSErrNotRegistered;)
		err = sysEvt->fEventList->remove(event);
		delete event;

		// if sublist is empty, delete it
		if (sysEvt->fEventList->isEmpty())
		{
			fSysEvents->remove(sysEvt);
			delete sysEvt;
		}
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	.
	Args:		inEvtId			event id?
				inMemMsg
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNameServer::sendSystemEvent(ULong inEvtId, ObjectId inMemMsg)
{
	NewtonErr err = noErr;
	XTRY
	{
		CSysEventTester	tester(inEvtId);
		ArrayIndex			index;
		CEventMasterListItem *	sysEvt;

		XFAILIF((sysEvt = (CEventMasterListItem *)fSysEvents->search(&tester, index)) == NULL
			  ||  sysEvt->fEventList->isEmpty(), err = kOSErrNotRegistered;)

		CUAsyncMessage	msg(inMemMsg, fReplyMem);
		fSysEventMsg = msg;
		fSysEventMsg.setCollectorPort(fPort);
		fSysEventMsg.setUserRefCon((OpaqueRef)&fInfo);

		fSysEventIter = new CListIterator(sysEvt->fEventList);
		XFAILIF(fSysEventIter == NULL, err = kOSErrNoMemory;)

		SysEvent *	eventInfo;
		for (eventInfo = (SysEvent *)fSysEventIter->firstItem(); eventInfo != NULL; eventInfo = (SysEvent *)fSysEventIter->nextItem())
		{
			fSysEventPort = eventInfo->fSysEventObjId;
			if (fSysEventPort.sendRPC(&fSysEventMsg, kPortSend_BufferAlreadySet, 0, NULL, 0, eventInfo->fSysEventSendFilter, NULL, eventInfo->fSysEventTimeOut) == noErr)
				break;
		}
		delete fSysEventIter;
		if (eventInfo == NULL)
			// there aren’t any SysEvents
			err = kOSErrNotRegistered;
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Return gestalt info.
	Args:		inSelector		gestalt selector
				ioToken			for replying
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CNameServer::gestalt(GestaltSelector inSelector, CUMsgToken * ioToken)
{
	NewtonErr	err = noErr;
	size_t		size;
	union
	{
		GestaltVersion					version;
		GestaltSystemInfo				system;
		GestaltRebootInfo				reboot;
		GestaltNewtonScriptVersion	nsVersion;
		GestaltPatchInfo				patch;
		GestaltPCMCIAInfo				pcmcia;
		GestaltRexInfo					rex;
	}	reply;

	switch (inSelector)
	{
	case kGestalt_Version:
		reply.version.fVersion = kGestaltImplementationVersion;
		size = sizeof(GestaltVersion);
		break;

	case kGestalt_SystemInfo:
		reply.system.fManufacturer = gROMManufacturer;
		reply.system.fMachineType = gHardwareType;
		reply.system.fROMVersion = gROMVersion;
		reply.system.fROMStage = gROMStage;
		reply.system.fRAMSize = InternalRAMInfo(kSystemRAMAlloc);

		NativePixelMap	pixmap;
		GetGrafInfo(kGrafPixelMap, &pixmap);
		reply.system.fScreenWidth = pixmap.bounds.right;
		reply.system.fScreenHeight = pixmap.bounds.bottom;
		reply.system.fScreenDepth = PixelDepth(&pixmap);

		Point		res;
		GetGrafInfo(kGrafResolution, &res);
		reply.system.fScreenResolution = res;

		float		resX, resY;
		GetTabletResolution(&resX, &resY);
		reply.system.fTabletResolution.x = resX;
		reply.system.fTabletResolution.y = resY;
#if defined(correct)
		err = GetPatchInfo(&sp28, &sp08);
		reply.system.fPatchVersion = sp28;	// short
#endif
		reply.system.fCPUType = gMainCPUType;
		reply.system.fCPUSpeed = gMainCPUClockSpeed;
		reply.system.fManufactureDate = gManufDate;
		size = sizeof(GestaltSystemInfo);
		break;

	case kGestalt_RebootInfo:
		reply.reboot.fRebootReason = gGlobalsThatLiveAcrossReboot.fRebootReason;
		reply.reboot.fRebootCount = 0;
		size = sizeof(GestaltRebootInfo);
		break;

	case kGestalt_NewtonScriptVersion:
		reply.nsVersion.fVersion = gNewtonScriptVersion;
		size = sizeof(GestaltNewtonScriptVersion);
		break;

	case kGestalt_PatchInfo:
		reply.patch.fTotalPatchPageCount = gGlobalsThatLiveAcrossReboot.fTotalPatchPageCount;
		for (ArrayIndex i = 0; i < kMaxPatchCount; ++i)
		{
			reply.patch.fPatch[i].fPatchCheckSum = gGlobalsThatLiveAcrossReboot.fPatchArray[i].fPatchCheckSum;
			reply.patch.fPatch[i].fPatchVersion = gGlobalsThatLiveAcrossReboot.fPatchArray[i].fPatchVersion;
			reply.patch.fPatch[i].fPatchPageCount = gGlobalsThatLiveAcrossReboot.fPatchArray[i].fPatchPageCount;
			reply.patch.fPatch[i].fPatchFirstPageIndex = gGlobalsThatLiveAcrossReboot.fPatchArray[i].fPatchFirstPageIndex;
		}
		size = sizeof(GestaltPatchInfo);
		break;

	case kGestalt_PCMCIAInfo:
		memset(&reply.pcmcia, 0, sizeof(GestaltPCMCIAInfo));
#if defined(correct)
		reply.pcmcia.fServicesAvailibility = kPCMCIACISParserAvailable
													  | kPCMCIACISIteratorAvailable
													  | kPCMCIAExtendedCCardPCMCIA
													  | kPCMCIAExtendedCCardSocket;
#endif
		size = sizeof(GestaltPCMCIAInfo);
		break;

	case kGestalt_RexInfo:
		memset(&reply.rex, 0, sizeof(GestaltRexInfo));
		for (ArrayIndex i = 0; i < kMaxROMExtensions; ++i)
		{
			RExHeader *	rExPtr;
			if ((rExPtr = GetRExPtr(i)) != 0)
			{
				reply.rex.fRex[i].signatureA = rExPtr->signatureA;
				reply.rex.fRex[i].signatureB = rExPtr->signatureB;
				reply.rex.fRex[i].checksum = rExPtr->checksum;
				reply.rex.fRex[i].headerVersion = rExPtr->headerVersion;
				reply.rex.fRex[i].manufacturer = rExPtr->manufacturer;
				reply.rex.fRex[i].version = rExPtr->version;
				reply.rex.fRex[i].length = rExPtr->length;
				reply.rex.fRex[i].id = rExPtr->id;
				reply.rex.fRex[i].start = rExPtr->start;
				reply.rex.fRex[i].count = rExPtr->count;
			}
		}
		size = sizeof(GestaltRexInfo);
		break;

	default:
		err = kOSErrBadParameters;
		size = 0;
		break;
	}

	return ioToken->replyRPC(&reply, size, err);
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C R e g i s t r y L i s t e n e r
--------------------------------------------------------------------------------*/

CRegistryListener::CRegistryListener()
	:	fNext(NULL), fMsgToken(), fName(NULL), fType(NULL)
{ }

#pragma mark -

/*--------------------------------------------------------------------------------
	C O b j e c t N a m e E n t r y
--------------------------------------------------------------------------------*/

CObjectNameEntry::CObjectNameEntry()
	:	fNext(NULL), fName(NULL), fType(NULL), fResArbInfo(NULL)
{ }

CObjectNameEntry::CObjectNameEntry(const char * inName, const char * inType, OpaqueRef inThing, OpaqueRef inSpec)
	:	fNext(NULL), fResArbInfo(NULL)
{
	fName = inName;
	fType = inType;
	fThing = inThing;
	fSpec = inSpec;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C O b j e c t N a m e L i s t
--------------------------------------------------------------------------------*/

CObjectNameList::CObjectNameList()
	:	fRegisterListeners(NULL), fUnregisterListeners(NULL), fRegistry(NULL)
{ }


/*--------------------------------------------------------------------------------
	Add a CObjectNameEntry to the list.
	Args:		inName			key part 1
				inType			key part 2
				inThing			data part 1
				inSpec			data part 2
	Return:	true => it was added successfully
--------------------------------------------------------------------------------*/

bool
CObjectNameList::add(const char * inName, const char * inType, OpaqueRef inThing, OpaqueRef inSpec)
{
	// create a new entry
	CObjectNameEntry *	entry = new CObjectNameEntry(inName, inType, inThing, inSpec);
	if (entry != NULL)
	{
		CRegistryListener *	listener, * prevListener, * nextListener;
		// thread it onto the registry list
		entry->fNext = fRegistry;
		fRegistry = entry;
		// if there are listeners waiting for names to be registered,
		// see if they are now satisfied
		for (prevListener = NULL, listener = fRegisterListeners; listener != NULL; prevListener = listener, listener = listener->fNext)
		{
			if (strcmp(listener->fName, inName) == 0
			&&  strcmp(listener->fType, inType) == 0)
			{
				// found one! reply with the original message token
				CNameServerReply	reply;
				reply.fThing = entry->fThing;
				reply.fSpec = entry->fSpec;
				listener->fMsgToken.replyRPC(&reply, sizeof(reply));
				// can now unthread the listener
				nextListener = listener->fNext;
				if (prevListener != NULL)
					prevListener->fNext = nextListener;
				else
					fRegisterListeners = nextListener;
				// delete the name/type buffers
				FreePtr((Ptr)listener->fName);
				FreePtr((Ptr)listener->fType);
				// and delete the listener itself
				delete listener;
				listener = (CRegistryListener *)&nextListener;
			}
		}
		return true;	// it was added okay
	}
	return false;	// we couldn’t create an entry
}


/*--------------------------------------------------------------------------------
	Remove a CObjectNameEntry from the list.
	Args:		inName			key part 1
				inType			key part 2
	Return:	true => it was removed successfully
--------------------------------------------------------------------------------*/

bool
CObjectNameList::remove(const char * inName, const char * inType)
{
	CResArbitrationInfo *	info;
	CObjectNameEntry *		prevEntry, * entry;

	for (prevEntry = NULL, entry = fRegistry; entry != NULL; prevEntry = entry, entry = entry->fNext)
	{
		if (strcmp(entry->fName, inName) == 0)
		{
			// found the name! unthread it from the list
			if (prevEntry != NULL)
				prevEntry->fNext = entry->fNext;
			else
				fRegistry = entry->fNext;
			// if there’s resArbInfo, can delete that now
			if ((info = entry->fResArbInfo) != NULL)
			{
				if ((info->f00 & 0x01) != 0)
					info->f00 |= 0x02;
				else
					delete info;
			}
			// if there are listeners waiting for names to be unregistered,
			// see if they are now satisfied
			CRegistryListener *	listener, * prevListener, * nextListener;
			for (prevListener = NULL, listener = fUnregisterListeners; listener != NULL; prevListener = listener, listener = listener->fNext)
			{
				if (strcmp(listener->fName, inName) == 0
				&&  strcmp(listener->fType, inType) == 0)
				{
					// found one! reply with the original message token
					CNameServerReply	reply;
					listener->fMsgToken.replyRPC(&reply, sizeof(reply));
					// can now unthread the listener
					nextListener = listener->fNext;
					if (prevListener != NULL)
						prevListener->fNext = nextListener;
					else
						fUnregisterListeners = nextListener;
					// delete the name/type buffers
					FreePtr((Ptr)listener->fName);
					FreePtr((Ptr)listener->fType);
					// and delete the listener itself
					delete listener;
					listener = (CRegistryListener *)&nextListener;
				}
			}
			// and finally, delete the entry
			FreePtr((Ptr)entry->fName);
			FreePtr((Ptr)entry->fType);
			delete entry;
			return true;	// it was removed okay
		}
	}
	return false;	// we couldn’t find the name
}


/*--------------------------------------------------------------------------------
	Look up a CObjectNameEntry in the list.
	Args:		inName			key part 1
				inType			key part 2
				outThing			where to put the data
				outSpec
				outEntry			optional: the actual entry
	Return:	true => it was found
--------------------------------------------------------------------------------*/

bool
CObjectNameList::lookup(const char * inName, const char * inType, OpaqueRef * outThing, OpaqueRef * outSpec, CObjectNameEntry ** outEntry)
{
	CObjectNameEntry *	entry;

	for (entry = fRegistry; entry != NULL; entry = entry->fNext)
	{
		if (strcmp(entry->fName, inName) == 0
		&&  strcmp(entry->fType, inType) == 0)
		{
			*outThing = entry->fThing;
			*outSpec = entry->fSpec;
			if (outEntry != NULL)
				*outEntry = entry;
			return true;
		}
	}
	return false;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C R e s O w n e r I n f o
--------------------------------------------------------------------------------*/

CResOwnerInfo::CResOwnerInfo()
	:	f08(NULL), f0C(0)
{ }

CResOwnerInfo::~CResOwnerInfo()
{
	if (f08 != NULL)
		FreePtr(f08);
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C R e s A r b i t r a t i o n I n f o
--------------------------------------------------------------------------------*/

CResArbitrationInfo::CResArbitrationInfo()
{
	f00 = 0;
	f04 = NULL;
	f08 = NULL;
	f58.fType = 1;
	f58.fData = (OpaqueRef) this;
}

CResArbitrationInfo::~CResArbitrationInfo()
{
	if (f04 != NULL)
		delete f04;
	if (f08 != NULL)
		delete f08;
}

NewtonErr
CResArbitrationInfo::init(CUPort * inPort)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = f20.init(true))
		XFAIL(err = f20.setUserRefCon((OpaqueRef)&f58))
		err = f20.setCollectorPort(*inPort);
	}
	XENDTRY;
	return err;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C E v e n t M a s t e r L i s t I t e m
--------------------------------------------------------------------------------*/

CEventMasterListItem::CEventMasterListItem()
{
	fEventList = NULL;
}

CEventMasterListItem::~CEventMasterListItem()
{
	if (fEventList != NULL)
		delete fEventList;
}

NewtonErr
CEventMasterListItem::init(ULong inEvtId)
{
	NewtonErr err = noErr;
	XTRY
	{
		fEvtId = inEvtId;
		fEventList = new CSortedList(&fCmp);
		XFAILIF(fEventList == NULL, err = kOSErrNoMemory;)
	}
	XENDTRY;
	return err;
}

#pragma mark -

CompareResult
CSysEventItemComparer::testItem(const void * inItem) const
{
	long	result = ((CEventMasterListItem *)fItem)->fEvtId - ((CEventMasterListItem *)inItem)->fEvtId;
	if (result < 0)
		return kItemLessThanCriteria;
	if (result > 0)
		return kItemGreaterThanCriteria;
	return kItemEqualCriteria;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C R P C I n f o
--------------------------------------------------------------------------------*/

CRPCInfo::CRPCInfo()
{
	fType = 0;
	fData = 0;
}


/*--------------------------------------------------------------------------------
	C N a m e S e r v e r R e q u e s t
--------------------------------------------------------------------------------*/

CNameServerRequest::CNameServerRequest()
{
	fCommand = kRegisterName;
}


/*--------------------------------------------------------------------------------
	C N a m e R e q u e s t
--------------------------------------------------------------------------------*/

CNameRequest::CNameRequest()
{
	fThing = 0;
	fSpec = 0;
	fObjectName = 0;
	fObjectType = 0;
}


/*--------------------------------------------------------------------------------
	C S y s E v e n t R e q u e s t
--------------------------------------------------------------------------------*/

CSysEventRequest::CSysEventRequest()
{
	fCommand = kRegisterForSystemEvent;
	fTheEvent = kSysEvent_PowerOn;
	fSysEventObjId = 0;
	fSysEventTimeOut = 0;
	fSysEventSendFilter = 0;
}


/*--------------------------------------------------------------------------------
	C R e s A r b i t r a t i o n R e q u e s t
--------------------------------------------------------------------------------*/

CResArbitrationRequest::CResArbitrationRequest()
{
	fCommand = kResourceArbitration;
	fRequestType = kResArbitrationClaim;
	fOwnerPortId = 0;
	fOwnerName = 0;
}


/*--------------------------------------------------------------------------------
	C N a m e S e r v e r R e p l y
--------------------------------------------------------------------------------*/

CNameServerReply::CNameServerReply()
{
	fThing = 0;
	fSpec = 0;
	fResult = noErr;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C U N a m e S e r v e r
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Constructor.
	Cache the port we use to communicate with the kernel name server.
	Create the shared memory objects used to pass name and type keys to it.
--------------------------------------------------------------------------------*/

CUNameServer::CUNameServer()
{
	fNameServerPort = GetPortSWI(kGetNameServerPort);
	
	if ((fMsgName = new CUSharedMem()) != NULL)
		fMsgName->init();
	if ((fMsgType = new CUSharedMem()) != NULL)
		fMsgType->init();
}


/*--------------------------------------------------------------------------------
	Destructor.
	Delete the name and type shared memory objects.
--------------------------------------------------------------------------------*/

CUNameServer::~CUNameServer()
{
	if (fMsgName != NULL)
		delete fMsgName;
	if (fMsgType != NULL)
		delete fMsgType;
}


/*--------------------------------------------------------------------------------
	Register a name with the name server.
	The name has a type, and the data stored for the key formed by that pair are
	two longwords, thing and spec.
	Args:		inName		the name under which the data is registered
				inType		a secondary key
				inThing		the data
				inSpec		…and a bit more
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CUNameServer::registerName(const char * inName, const char * inType, OpaqueRef inThing, OpaqueRef inSpec)
{
	NewtonErr err;
	XTRY
	{
		CNameRequest		request;
		CNameServerReply	reply;
		size_t				replySize;

		// set shared memory objects with name and type
		XFAILIF(fMsgName == NULL || fMsgType == NULL, err = kOSErrNoMemory;)
		fMsgName->setBuffer((void*)inName, strlen(inName) + 1);
		fMsgType->setBuffer((void*)inType, strlen(inType) + 1);

		// form the request
		request.fCommand = kRegisterName;
		request.fThing = inThing;
		request.fSpec = inSpec;
		request.fObjectName = *fMsgName;
		request.fObjectType = *fMsgType;

		// send it to the kernel name server
		err = fNameServerPort.sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply));
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Unregister a name with the name server.
	Args:		inName		the name under which the data is registered
				inType		a secondary key
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CUNameServer::unregisterName(const char * inName, const char * inType)
{
	NewtonErr err;
	XTRY
	{
		CNameRequest		request;
		CNameServerReply	reply;
		size_t				replySize;

		// set shared memory objects with name and type
		XFAILIF(fMsgName == NULL || fMsgType == NULL, err = kOSErrNoMemory;)
		fMsgName->setBuffer((void*)inName, strlen(inName) + 1);
		fMsgType->setBuffer((void*)inType, strlen(inType) + 1);

		// form the request
		request.fCommand = kUnregisterName;
		request.fObjectName = *fMsgName;
		request.fObjectType = *fMsgType;

		// send it to the kernel name server
		err = fNameServerPort.sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply));
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Wait for a name to be registered by another task, and return the data
	associated with it.
	Args:		inName		the name under which the data is registered
				inType		a secondary key
				outThing		the data
				outSpec		…and a bit more
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CUNameServer::waitForRegister(const char * inName, const char * inType, OpaqueRef * outThing, OpaqueRef * outSpec)
{
	NewtonErr err;
	XTRY
	{
		CNameRequest		request;
		CNameServerReply	reply;
		size_t				replySize;

		// set shared memory objects with name and type
		XFAILIF(fMsgName == NULL || fMsgType == NULL, err = kOSErrNoMemory;)
		fMsgName->setBuffer((void*)inName, strlen(inName) + 1);
		fMsgType->setBuffer((void*)inType, strlen(inType) + 1);

		// form the request
		request.fCommand = kWaitForRegister;
		request.fObjectName = *fMsgName;
		request.fObjectType = *fMsgType;

		// send it to the kernel name server
		XFAIL(err = fNameServerPort.sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply)))
		*outThing = reply.fThing;
		*outSpec = reply.fSpec;
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Wait for another task to unregister a name.
	Args:		inName		the name
				inType		a secondary key
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CUNameServer::waitForUnregister(const char * inName, const char * inType)
{
	NewtonErr err;
	XTRY
	{
		CNameRequest		request;
		CNameServerReply	reply;
		size_t				replySize;

		// set shared memory objects with name and type
		XFAILIF(fMsgName == NULL || fMsgType == NULL, err = kOSErrNoMemory;)
		fMsgName->setBuffer((void*)inName, strlen(inName) + 1);
		fMsgType->setBuffer((void*)inType, strlen(inType) + 1);

		// form the request
		request.fCommand = kWaitForUnregister;
		request.fObjectName = *fMsgName;
		request.fObjectType = *fMsgType;

		// send it to the kernel name server
		err = fNameServerPort.sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply));
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Look up the two longwords, thing and spec, associated with a name/type key.
	Args:		inName		name string
				inType		type string, which together with inName uniquely
								identifies a resource in the name server’s database
				outThing		pointer to thing result
				outSpec		pointer to spec result
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CUNameServer::lookup(const char * inName, const char * inType, OpaqueRef * outThing, OpaqueRef * outSpec)
{
	NewtonErr err;
	XTRY
	{
		CNameRequest		request;
		CNameServerReply	reply;
		size_t				replySize;

		// set shared memory objects with name and type
		XFAILIF(fMsgName == NULL || fMsgType == NULL, err = kOSErrNoMemory;)

		fMsgName->setBuffer((void *)inName, strlen(inName) + 1);		// should setBuffer accept const arg?
		fMsgType->setBuffer((void *)inType, strlen(inType) + 1);

		// form the request
		request.fCommand = kLookup;
		request.fObjectName = *fMsgName;
		request.fObjectType = *fMsgType;

		// send it to the kernel name server
		XFAIL(err = fNameServerPort.sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply)))
		*outThing = reply.fThing;
		*outSpec = reply.fSpec;
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Claim resources.
	Args:		--
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CUNameServer::resourceClaim(const char * inName, const char * inType, ObjectId inOwnerPortId, ObjectId inApplicationNameId)
{
	NewtonErr err;
	XTRY
	{
		CResArbitrationRequest	request;
		CNameServerReply			reply;
		size_t						replySize;

		XFAILIF(fMsgName == NULL || fMsgType == NULL, err = kOSErrNoMemory;)

		// set shared memory objects with name and type
		fMsgName->setBuffer((void*)inName, strlen(inName) + 1);
		fMsgType->setBuffer((void*)inType, strlen(inType) + 1);

		// form the request
		request.fRequestType = kResArbitrationClaim;
		request.fOwnerPortId = inOwnerPortId;
		request.fOwnerName = inApplicationNameId;
		request.fObjectName = *fMsgName;
		request.fObjectType = *fMsgType;

		// send it to the kernel name server
		XFAIL(err = fNameServerPort.sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply)))
		err = reply.fResult;
	}
	XENDTRY;
	return err;
}


NewtonErr
CUNameServer::resourceUnclaim(const char * inName, const char * inType)
{
	NewtonErr err;
	XTRY
	{
		CResArbitrationRequest	request;
		CNameServerReply			reply;
		size_t						replySize;

		XFAILIF(fMsgName == NULL || fMsgType == NULL, err = kOSErrNoMemory;)

		// set shared memory objects with name and type
		fMsgName->setBuffer((void*)inName, strlen(inName) + 1);
		fMsgType->setBuffer((void*)inType, strlen(inType) + 1);

		// form the request
		request.fRequestType = kResArbitrationUnclaim;
		request.fObjectName = *fMsgName;
		request.fObjectType = *fMsgType;

		// send it to the kernel name server
		XFAIL(err = fNameServerPort.sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply)))
		err = reply.fResult;
	}
	XENDTRY;
	return err;
}


NewtonErr
CUNameServer::resourcePassiveClaim(const char * inName, const char * inType, ObjectId inOwnerPortId, ObjectId inApplicationNameId)
{
	NewtonErr err;
	XTRY
	{
		CResArbitrationRequest	request;
		CNameServerReply			reply;
		size_t						replySize;

		XFAILIF(fMsgName == NULL || fMsgType == NULL, err = kOSErrNoMemory;)

		// set shared memory objects with name and type
		fMsgName->setBuffer((void*)inName, strlen(inName) + 1);
		fMsgType->setBuffer((void*)inType, strlen(inType) + 1);

		// form the request
		request.fRequestType = kResArbitrationPassiveClaim;
		request.fOwnerPortId = inOwnerPortId;
		request.fOwnerName = inApplicationNameId;
		request.fObjectName = *fMsgName;
		request.fObjectType = *fMsgType;

		// send it to the kernel name server
		XFAIL(err = fNameServerPort.sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply)))
		err = reply.fResult;
	}
	XENDTRY;
	return err;
}


NewtonErr
CUNameServer::resourcePassiveUnclaim(const char * inName, const char * inType)
{
	NewtonErr err;
	XTRY
	{
		CResArbitrationRequest	request;
		CNameServerReply			reply;
		size_t						replySize;

		XFAILIF(fMsgName == NULL || fMsgType == NULL, err = kOSErrNoMemory;)

		// set shared memory objects with name and type
		fMsgName->setBuffer((void*)inName, strlen(inName) + 1);
		fMsgType->setBuffer((void*)inType, strlen(inType) + 1);

		// form the request
		request.fRequestType = kResArbitrationPassiveUnclaim;
		request.fObjectName = *fMsgName;
		request.fObjectType = *fMsgType;

		// send it to the kernel name server
		XFAIL(err = fNameServerPort.sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply)))
		err = reply.fResult;
	}
	XENDTRY;
	return err;
}
