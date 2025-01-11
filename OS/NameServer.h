/*
	File:		NameServer.h

	Contains:	NameServer definition.

	Written by:	Newton Research Group.
*/

#if !defined(__NAMESERVER_H)
#define __NAMESERVER_H 1

#if !defined(__NEWTON_H)
#include "Newton.h"
#endif

#if !defined(__NEWTONGESTALT_H)
#include "NewtonGestalt.h"
#endif

#if !defined(__NEWTONERRORS_H)
#include "NewtonErrors.h"
#endif

#if !defined(__USERTASKS_H)
#include "UserTasks.h"
#endif

#if !defined(__OSERRORS_H)
#include "OSErrors.h"
#endif

#include "List.h"
#include "ListIterator.h"
#include "ItemComparer.h"


//	Following is a list of types that are registered with the NameServer.

extern const char kUPort[];
extern const char kRegisteredStore[];
extern const char kRegisteredGestalt[];

typedef ULong	ObjectName;


NewtonErr	InitializeNameServer(void);
bool			GetOSPortFromName(ObjectName inName, CUPort * ioPort);


/*--------------------------------------------------------------------------------
	C U N a m e S e r v e r

	User access to the name server.
--------------------------------------------------------------------------------*/

class CUNameServer
{
public:
					CUNameServer();
					~CUNameServer();

	NewtonErr	registerName(const char * inName, const char * inType, OpaqueRef inThing, OpaqueRef inSpec);
	NewtonErr	unregisterName(const char * inName, const char * inType);
	NewtonErr	waitForRegister(const char * inName, const char * inType, OpaqueRef * outThing, OpaqueRef * outSpec);
	NewtonErr	waitForUnregister(const char * inName, const char * inType);
	NewtonErr	lookup(const char * inName, const char * inType, OpaqueRef * outThing, OpaqueRef * outSpec);

	NewtonErr	resourceClaim(const char * inName, const char * inType, ObjectId inOwnerPortId, ObjectId inApplicationNameId);
	NewtonErr	resourceUnclaim(const char * inName, const char * inType);
	NewtonErr	resourcePassiveClaim(const char * inName, const char * inType, ObjectId inOwnerPortId, ObjectId inApplicationNameId);
	NewtonErr	resourcePassiveUnclaim(const char * inName, const char * inType);

private:
	CUPort			fNameServerPort;
	CUSharedMem *	fMsgName;
	CUSharedMem *	fMsgType;
};


// NameServerCommands

typedef uint32_t	NameServerCommand;
typedef uint32_t	SystemEvent;

#define	kRegisterName					1
#define	kUnregisterName				2
#define	kWaitForRegister				3
#define	kWaitForUnregister			4
#define	kLookup							5
#define	k_DEBUGGING_DumpObjectName	6
#define	kRegisterForSystemEvent		7
#define	kUnregisterForSystemEvent	8
#define	kSendSystemEvent				9
#define	kGestalt							10
#define	kResourceArbitration			11


/*--------------------------------------------------------------------------------
	Macro to set id string from id number, respecting platform endianness.
	eg 'drvl' -> "drvl"
--------------------------------------------------------------------------------*/

#if defined(hasByteSwapping)
#define MAKE_ID_STR(_n,_s) \
	char _s[5]; \
	ObjectName _n_ = _n; \
	_n_ = BYTE_SWAP_LONG(_n_); \
	*(ObjectName*)_s = _n_; \
	_s[4] = 0

#else
#define MAKE_ID_STR(_n,_s) \
	char _s[5]; \
	*(ObjectName*)_s = _n; \
	_s[4] = 0
#endif


/*--------------------------------------------------------------------------------
	Name server message block definitions.

	NOTE - if you change the size of these messages, verify that
			  kNameServerRequestSizeMax is set to the appropriate value!
--------------------------------------------------------------------------------*/

class CNameServerRequest : public SingleObject
{
public:
					CNameServerRequest();

	NameServerCommand fCommand;	// nameserver/systemEvent command
};


class CNameRequest : public CNameServerRequest
{
public:
					CNameRequest();

	OpaqueRef	fThing;				// "thing" to register.  Is returned by lookup
	OpaqueRef	fSpec;				// another thing which is registered.  Also returned by lookup
	OpaqueRef	fParam1;				// Unused
	OpaqueRef	fParam2;				// Unused
	ObjectId		fObjectName;		// shared mem Id of object name C string
	ObjectId		fObjectType;		// shared mem Id of object type C string
};


class CSysEventRequest : public CNameServerRequest
{
public:
					CSysEventRequest();

	SystemEvent	fTheEvent;
	ObjectId		fSysEventObjId;		// Object Id of port to send sys event to, or object Id of sys event shared mem message
	Timeout		fSysEventTimeOut;		// time out used for async send of sys event message
	ULong			fSysEventSendFilter;	// filter used for async send of sys event message
};


struct SysEvent
{
	ObjectId		fSysEventObjId;		// Object Id of port to send sys event to, or object Id of sys event shared mem message
	Timeout		fSysEventTimeOut;		// time out used for async send of sys event message
	ULong			fSysEventSendFilter;	// filter used for async send of sys event message
};


class CGestaltRequest : public CNameServerRequest
{
public:
					CGestaltRequest();

	GestaltSelector	fSelector;		// indicates type of gestalt request
};


// Resource Arbitration Request types

#define kResArbitrationClaim				1
#define kResArbitrationUnclaim			2
#define kResArbitrationPassiveClaim		3
#define kResArbitrationPassiveUnclaim	4


class CResArbitrationRequest : public CNameRequest
{
public:
					CResArbitrationRequest();

	ULong			fRequestType;		// indicates type of resource arbitration request (e.g. claim, unclaim)
	ObjectId		fOwnerPortId;		// Port Id of owner.  Required when claiming a resource
	ObjectId		fOwnerName;			// optionally shared mem Id of owner application (unicode, will be displayed to user)
};

class CResArbitrationInfo;

class CResOwnerInfo : public CUPort
{
public:
					CResOwnerInfo();
					~CResOwnerInfo();

    CResOwnerInfo&			operator=(ObjectId id);		// why is this not inherited from CUObject????
    CResOwnerInfo&			operator=(const CResOwnerInfo & inCopy);

	char *		f08;	// name?
	long			f0C;	// size?
};

inline CResOwnerInfo& CResOwnerInfo::operator=(ObjectId id) 
{
    copyObject(id);
    return *this;
}

inline CResOwnerInfo& CResOwnerInfo::operator=(const CResOwnerInfo & inCopy)
{
    copyObject(inCopy.fId); 
    return *this;
}


class CSysEventItemComparer : public CItemComparer
{
public:
	virtual CompareResult	testItem(const void * inItem) const;
};

class CEventMasterListItem
{
public:
					CEventMasterListItem();
					~CEventMasterListItem();

	NewtonErr	init(ULong inEvtId);

	ULong							fEvtId;			// +00
	CSysEventItemComparer	fCmp;				// +04
	CSortedList *				fEventList;		// +10
};

class CSysEventTester : public CItemTester
{
public:
					CSysEventTester(ULong inEventId);
	virtual CompareResult	testItem(const void * inItem) const;
	ULong			fEventId;
};

inline					CSysEventTester::CSysEventTester(ULong inEventId)  { fEventId = inEventId; }
inline CompareResult	CSysEventTester::testItem(const void * inItem) const  { return (fEventId == ((CEventMasterListItem *)inItem)->fEvtId ) ? kItemEqualCriteria : kItemLessThanCriteria; }


class CNameServerReply : public SingleObject
{
public:
					CNameServerReply();

	OpaqueRef	fThing;
	OpaqueRef	fSpec;
	NewtonErr	fResult;
};


class CRegistryListener
{
public:
					CRegistryListener();

	CRegistryListener *	fNext;		// +00
	CUMsgToken				fMsgToken;	// +04
	const char *			fName;		// +14
	const char *			fType;		// +18
};


class CObjectNameEntry
{
public:
					CObjectNameEntry();
					CObjectNameEntry(const char * inName, const char * inType, OpaqueRef inThing, OpaqueRef inSpec);

	CObjectNameEntry *	fNext;		// +00
	OpaqueRef				fThing;		// +04
	OpaqueRef				fSpec;		// +08
	const char *			fName;		// +0C
	const char *			fType;		// +10
	CResArbitrationInfo *fResArbInfo;// +14
};


class CObjectNameList
{
public:
					CObjectNameList();

	bool			add(const char * inName, const char * inType, OpaqueRef inThing, OpaqueRef inSpec);
	bool			remove(const char * inName, const char * inType);
	bool			lookup(const char * inName, const char * inType, OpaqueRef * outThing, OpaqueRef * outSpec, CObjectNameEntry ** outEntry);

	CRegistryListener *	fRegisterListeners;		// +00
	CRegistryListener *	fUnregisterListeners;	// +04
//	long						f08;
//	long						f0C;
	CObjectNameEntry *	fRegistry;	// +10
};

#define kObjectNameHashBucketSize		16
#define kObjectNameHashBucketMask		(kObjectNameHashBucketSize - 1)


/*--------------------------------------------------------------------------------
	C N a m e S e r v e r

	The kernel name server.
--------------------------------------------------------------------------------*/

class CNameServer : public CUTaskWorld
{
public:
	virtual size_t		getSizeOf(void) const;
	virtual NewtonErr	taskConstructor(void);
	virtual void		taskMain(void);

	ULong			hash(const char * inName);
	NewtonErr	buildNameAndType(ObjectId inName, ObjectId inType);
	void			deleteNameAndType(void);

	NewtonErr	registerName(OpaqueRef inThing, OpaqueRef inSpec);
	NewtonErr	unregisterName(void);

	NewtonErr	lookup(OpaqueRef * outThing, OpaqueRef * outSpec);
	
	NewtonErr	queueForRegister(CUMsgToken * inToken);
	NewtonErr	queueForUnregister(CUMsgToken * inToken);

	void			resourceArbitration(CUMsgToken&, CResArbitrationRequest*);
	NewtonErr	resArbBuildResArbInfo(CObjectNameEntry * ioEntry);
	NewtonErr	resArbDeleteResArbInfo(CObjectNameEntry * ioEntry);
	NewtonErr	resArbBuildResOwnerInfo(CResOwnerInfo *& ioOwnerInfo, ObjectId inName, ObjectId inPortId);
	NewtonErr	resArbDeleteResOwnerInfo(CResOwnerInfo *& ioOwnerInfo);
	NewtonErr	resArbSendClaimNotification(CObjectNameEntry * ioEntry);
	void			resArbHandleReply(CResArbitrationInfo * ioArbitrationInfo);

	NewtonErr	gestalt(GestaltSelector inSelector, CUMsgToken * ioToken);

	NewtonErr	registerForSystemEvent(ULong inEvtId, ULong inEventClass, ULong infEventId, ULong inSysEventType);
	NewtonErr	unregisterForSystemEvent(ULong inEvtId, ULong);
	NewtonErr	sendSystemEvent(ULong inEvtId, ObjectId inMemMsg);

private:
	CUPort				fPort;			// +18
	CList *				fSysEvents;		// +20
	CListIterator *	fSysEventIter;	// +24
	CUAsyncMessage		fSysEventMsg;	// +28
	CRPCInfo				fInfo;			// +38
	CUPort				fSysEventPort;	// +40
	CUMsgToken			fSysEventToken;// +48
	CUSharedMem			fReplyMem;		// +58
	const char *		fName;			// +60
	const char *		fType;			// +64
	CObjectNameList	fDB[kObjectNameHashBucketSize];	// +68
};

#define kGestaltImplementationVersion	1


#endif	/* __NAMESERVER_H */
