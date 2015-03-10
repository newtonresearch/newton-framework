/*
	File:		Alerts.h

	Contains:	Alert event and dialog declarations.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__ALERTS_H)
#define __ALERTS_H 1

#include "AppWorld.h"
#include "Protocols.h"


/*--------------------------------------------------------------------------------
	C A l e r t D i a l o g
--------------------------------------------------------------------------------*/

class CAlertDialog
{
public:
			CAlertDialog();

// size+28
};


/*--------------------------------------------------------------------------------
	C A l e r t E v e n t
--------------------------------------------------------------------------------*/

class CAlertEvent : public CEvent
{
public:
			CAlertEvent();

	long		f08;
	long		f0C;
	void *	f10;
// size+14
};


/*--------------------------------------------------------------------------------
	C A l e r t E v e n t H a n d l e r
--------------------------------------------------------------------------------*/
class CAlertManager;

class CAlertEventHandler : public CEventHandler
{
public:
					CAlertEventHandler();
	virtual		~CAlertEventHandler();

	NewtonErr	init(CAlertManager * inMgr);

	// User supplied handler routines
	virtual void	eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual void	eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual void	idleProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);

private:
	CAlertManager * fManager;	// f14
};


/*--------------------------------------------------------------------------------
	C A l e r t M a n a g e r
--------------------------------------------------------------------------------*/

class CAlertManager : public CAppWorld
{
public:
			CAlertManager();

	virtual size_t		getSizeOf(void) const;

	virtual NewtonErr	mainConstructor(void);

private:
	CAlertEventHandler	f70;
	CUPort *					f88;
	CList						f8C;		// list of alerts
	CUAsyncMessage			fA4;
	CAlertEvent				fB4;
// size+C8
};


#endif	/* __ALERTS_H */
