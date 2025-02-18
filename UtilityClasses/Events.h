/*
	File:		Events.h

	Contains:	Implementation of main interface calls.

	Written by:	Newton Research Group.
*/

#if !defined(__EVENTS_H)
#define __EVENTS_H 1

#if !defined(__NEWTON_H)
#include "Newton.h"
#endif

#if !defined(__ITEMCOMPARER_H)
#include "ItemComparer.h"
#endif


typedef uint32_t EventClass;
typedef uint32_t EventId;
typedef uint32_t EventType;

#define kTypeWildCard 				'****'

#define kMAXEVENTSIZE	(256)


//	NEWTON specific follows...

// ADD YOUR TYPES HERE  (please use all lower case)

#define kNewtEventClass				'newt'

#define kSystemEventId				'sysm'
#define kTimerEventId				'timr'
#define kIdleEventId					'idle'

//  System Message id
#define kSystemMessageId			'sysm'

//  Debugger
#define kDebuggerEventId			'dbug'

//  AppleTalk ids
#define kAppleTalkId					'atlk'
#define kAppleTalkPAPId				'pap '
#define kAppleTalkATPId				'atp '
#define kAppleTalkADSPId			'adsp'
#define kAppleTalkDDPId				'ddp '
#define kAppleTalkLAPId				'lap '
#define kAppleTalkRTMPId			'rtmp'
#define kAppleTalkAEPId				'aep '
#define kAppleTalkNBPId				'nbp '

//  Communications Manager
#define kCommManagerId				'comg'

//  Comm Tool
#define kCommToolEventId			'comt'

//	inker event
#define kInkerEventId				'inkr'

// card server event
#define kCardServerId				'cdsv'
#define kCardServerClientId		'cdcl'
#define kCardServerUIId				'cdui'

#define kCardEventId					'card'

// sound server event
#define kSndEventId 					'snde'

// endpoint event
#define kEndpointEventId			'endp'

// package manager event
#define kPackageEventId			 	'pckm'


/*--------------------------------------------------------------------------------
	C E v e n t

	Base event type in the Newton system from which all others are derived.
	All events (messages) should be subclassed from CEvent.
	No virtuals allowed.
--------------------------------------------------------------------------------*/

class CEvent : public SingleObject
{
public:
					CEvent();
					CEvent(EventId inId, EventClass inClass = kNewtEventClass);

	EventClass 	fEventClass;
	EventId 		fEventId;
};

inline CEvent::CEvent() : fEventClass(kNewtEventClass) { }
inline CEvent::CEvent(EventId inId, EventClass inClass) : fEventClass(inClass), fEventId(inId) { }


/*--------------------------------------------------------------------------------
	C E v e n t S y s t e m E v e n t
	Was TAESystemEvent
--------------------------------------------------------------------------------*/

class CEventSystemEvent : public CEvent
{
public:
					CEventSystemEvent();
					CEventSystemEvent(EventType inType);

	EventType	fEventType;
};

inline CEventSystemEvent::CEventSystemEvent() : CEvent(kSystemEventId), fEventType(0) { }
inline CEventSystemEvent::CEventSystemEvent(EventType inType) : CEvent(kSystemEventId), fEventType(inType) { }


/*--------------------------------------------------------------------------------
	C P o w e r E v e n t
--------------------------------------------------------------------------------*/

class CPowerEvent : public CEventSystemEvent
{
public:
					CPowerEvent();
					CPowerEvent(EventType inType, ULong inReason);

	ULong			fReason;
};

inline CPowerEvent::CPowerEvent() : fReason(0) { }
inline CPowerEvent::CPowerEvent(EventType inType, ULong inReason) : CEventSystemEvent(inType), fReason(inReason) { }


/* -----------------------------------------------------------------------------
	C I d l e E v e n t
----------------------------------------------------------------------------- */

class CIdleEvent : public CEvent
{
public:
					CIdleEvent();
					CIdleEvent(EventType inType);

	EventType	fEventType;
};

inline CIdleEvent::CIdleEvent() : CEvent(kIdleEventId) { }
inline CIdleEvent::CIdleEvent(EventType inType) : fEventType(inType) { }


/*--------------------------------------------------------------------------------
	C T i m e r E v e n t

	Event type for idle handlers.
	Private to the idler implementation - do not subclass.
--------------------------------------------------------------------------------*/
class CEventIdleTimer;

class CTimerEvent : public CEvent
{
public:
	CEventIdleTimer * fTimer;
	ULong					fTimerRefCon;
};


/*--------------------------------------------------------------------------------
	C E v e n t C o m p a r e r

	Compares CEvent with a CEventHandler.
--------------------------------------------------------------------------------*/

class CEventComparer : public CItemComparer
{
public:
									CEventComparer();
	virtual CompareResult	testItem(const void * inItem) const;
};


#endif /* __EVENTS_H */
