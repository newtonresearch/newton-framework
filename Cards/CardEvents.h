/*
	File:		CardEvents.h

	Contains:	PC card event declarations.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__CARDEVENTS_H)
#define __CARDEVENTS_H 1

#include "EventHandler.h"
#include "SystemEvents.h"
#include "UserPorts.h"
#include "NewtonTime.h"
#include "Protocols.h"


/*--------------------------------------------------------------------------------
	C C a r d M e s s a g e
--------------------------------------------------------------------------------*/

class CCardMessage : public CEvent
{
public:
					CCardMessage();
					~CCardMessage();

	void			clear(void);
	void			messageStuff(ULong, ULong, ULong);

	ULong			f08;
	NewtonErr	f0C;
	ULong			f10;

	long			f20;

private:
	long			f14;
	long			f18;
	long			f1C;
	long			f24;
	long			f28;
	char			f2C;
	char			f2D;
	char			f2E;
	char			f2F;
	long			f30;
	long			f34;
	char			f38[128];
// size +B8
};


class CCardAsyncMsg : public CCardMessage
{
public:
					CCardAsyncMsg();

	NewtonErr	init(void);
	void			free(void);
	NewtonErr	sendRPC(CUPort * inPort, CUPort * inReplyToPort, Timeout inTimeout, CTime * inFutureTimeToSend);
	NewtonErr	send(CUPort * inPort, Timeout inTimeout, CTime * inFutureTimeToSend);

private:
	CUAsyncMessage	fMsg;
// size +CC
};


class CNewCardAsyncMsg : public CCardMessage
{
public:
					CNewCardAsyncMsg();

	NewtonErr	init(void);
	void			clear(void);
	void			sendSystemEvent(void);

private:
	CUAsyncMessage		fMsg;
	CSendSystemEvent	fEvent;
// size +F0
};


class CNewCardEvent
{
public:
	ULong			f10[4];
};


/*--------------------------------------------------------------------------------
	C N e w t C a r d E v e n t H a n d l e r
--------------------------------------------------------------------------------*/

class CNewtCardEventHandler : public CEventHandler
{
public:
	//	initialize handler to specified eventId
	NewtonErr	init(EventId inEventId, EventClass inEventClass = kNewtEventClass);

	// User supplied handler routines
	virtual	void	eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual	void	eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);

	void			readyToAcceptCardEvents(void);
	NewtonErr	handleCardEvent(CCardMessage * inEvent);

	NewtonErr	sendAyncServer(CCardAsyncMsg*, ULong);
	NewtonErr	sendServer(ULong, ULong, ObjectId, CCardMessage * inEvent);
	void			replyServer(CCardMessage * inEvent, ULong, ULong, ObjectId);

private:
	int			f10;
	CUPort *		f14;
// size +18
};


/*--------------------------------------------------------------------------------
	C C a r d A l e r t D i a l o g
--------------------------------------------------------------------------------*/

class CAlertDialog
{
public:
			CAlertDialog();

// size+28
};


// should be member function to function pointer
typedef bool (*WPAlertProc)(void *, ULong, void *);
class CFlashStore;

class CCardAlertDialog : public CAlertDialog
{
public:
			CCardAlertDialog();

	void			init(WPAlertProc inProc, CFlashStore * inStore);
	void			setFilterData(void * inData);

// size+50
};


/*--------------------------------------------------------------------------------
	C C a r d A l e r t E v e n t
--------------------------------------------------------------------------------*/

class CAlertEvent : public CEvent
{
public:
			CAlertEvent();

// size+14
};


class CCardAlertEvent : public CAlertEvent
{
public:
			CCardAlertEvent();

	CCardAlertDialog *	f10;
	CCardAlertDialog		f14;
// size+64
};


/*--------------------------------------------------------------------------------
	C C a r d H a n d l e r
	P-class interface.
--------------------------------------------------------------------------------*/
class CCardSocket;
class CCardPCMCIA;

PROTOCOL CCardHandler : public CProtocol
{
public:
	static CCardHandler *	make(const char * inName);
	void			destroy(void);

	void			recognizeCard(CCardSocket*, CCardPCMCIA*);
	void			parseUnrecognizedCard(CCardSocket*, CCardPCMCIA*);

	void			installServices(CCardSocket*, CCardPCMCIA*, ULong);
	void			removeServices(void);
	void			suspendServices(void);
	void			resumeServices(CCardSocket*, CCardPCMCIA*, ULong);
	void			emergencyShutdown(void);

	void			formatCIS(CCardSocket*, CCardPCMCIA*);
	void			cardIdString(CCardPCMCIA*);
	ULong			cardStatus(void);
	void			getNumberOfDevice(void);
	void			getDeviceInfo(ULong, ULong*, ULong*, void**, ULong*, ULong*);
	void			setCardServerPort(ULong);
	void			setRemovableHandler(unsigned char);
	void			getRemovableHandler(void);
	void			cardSpecific(ULong, void*, ULong);
};


#endif	/* __CARDEVENTS_H */
