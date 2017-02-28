/*
	File:		Alerts.cc

	Contains:	Alert event and dialog implementations.

	Written by:	Newton Research Group, 2010.
*/

#include "Alerts.h"
#include "OSErrors.h"

/*--------------------------------------------------------------------------------
	A l e r t M a n a g e r
--------------------------------------------------------------------------------*/

NewtonErr
InitAlertManager(void)
{
	NewtonErr err;
	XTRY
	{
		CAlertManager * alertMgr = new CAlertManager;
		XFAILIF(alertMgr == NULL, err = kOSErrNoMemory;)
		err = alertMgr->init('alrt', true, kSpawnedTaskStackSize);
	}
	XENDTRY;
	return err;
}


#pragma mark -
/*--------------------------------------------------------------------------------
	C A l e r t M a n a g e r
--------------------------------------------------------------------------------*/

CAlertManager::CAlertManager()
{ }


/*--------------------------------------------------------------------------------
	Return the size of this class.
	Args:		--
	Return:	number of bytes
--------------------------------------------------------------------------------*/

size_t
CAlertManager::getSizeOf(void) const
{
	return sizeof(CAlertManager);
}


/*--------------------------------------------------------------------------------
	Construct the main thing.
	Args:		--
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CAlertManager::mainConstructor(void)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = CAppWorld::mainConstructor())
		f88 = getMyPort();
		XFAIL(f70.init(this))
		fA4.init(false);
	}
	XENDTRY;
	return err;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C A l e r t E v e n t
--------------------------------------------------------------------------------*/

CAlertEvent::CAlertEvent()
	:	CEvent('alrt'), fError(noErr), f08(1), f10(NULL)
{ }


#pragma mark -

/*--------------------------------------------------------------------------------
	C A l e r t E v e n t H a n d l e r
--------------------------------------------------------------------------------*/

CAlertEventHandler::CAlertEventHandler()
{ }

CAlertEventHandler::~CAlertEventHandler()
{ }

NewtonErr
CAlertEventHandler::init(CAlertManager * inMgr)
{
	NewtonErr err;
	XTRY
	{
		fManager = inMgr;
		XFAIL(err = CEventHandler::init('alrt'))
		err = initIdler(200, kMilliseconds, 0, true);
	}
	XENDTRY;
	return err;
}

void
CAlertEventHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	XTRY
	{
		CAlertEvent * alertEvent = (CAlertEvent *)inEvent;
		CAlertDialog * currentAlert;
		void * r5;

		alertEvent->fError = noErr;
		r5 = alertEvent->f10;
		XFAILIF(r5 == NULL, alertEvent->fError = kOSErrBadParameters;)
		XFAIL(alertEvent->f08 != 1)
#if defined(correct)
		if ((currentAlert = (CAlertDialog *)fManager->f8C.at(0)) == NULL
		||  memcmp(r5, currentAlert, r5->f18) != 0)
		{
			CAlertDialog * newAlert;
			XFAIL((newAlert = (CAlertDialog *)NewPtr(r5->f18)) == NULL)
			memmove(newAlert, r5, r5->f18);
			alertEvent->fError = fManager->f8C.insertAt(0, newAlert);
			if (currentAlert)
				currentAlert->removeAlert();
			newAlert->displayAlert();
			resetIdle();
		}
#endif
	}
	XENDTRY;
}

void
CAlertEventHandler::eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{ }

void
CAlertEventHandler::idleProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	CAlertDialog * currentAlert;
	bool r7 = true;
	bool r6 = true;
	ULong sp00;
#if defined(correct)
	while ((currentAlert = (CAlertDialog *)fManager->f8C.at(0)) != NULL)
	{
		if (!r7)
		{
			// this ain’t right...
			if (currentAlert->f0C != 0)
			{
				currentAlert->displayAlert();
				r6 = true;
			}
			else if (!currentAlert->checkAlertDone(&sp00))
			{
				currentAlert->displayAlert();
				r6 = true;
			}
		}
		if (!currentAlert->checkAlertDone(&sp00))
			break;

		if (r6)
		{
			currentAlert->removeAlert();
			r6 = false;
		}

		fManager->f8C.remove(currentAlert);
		r7 = false;
	}

	if (currentAlert)
		resetIdle();

	else
	{
		char info[0x1C];
		GetGrafInfo(0, info);
		fManager->fB4.f04 = kIdleEventId;
		fManager->fB4.f08 = 'draw';
		gNewtPort->send(...);
	}
#endif
}


#pragma mark -
/*--------------------------------------------------------------------------------
	C A l e r t D i a l o g
--------------------------------------------------------------------------------*/

CAlertDialog::CAlertDialog()
{
	// INCOMPLETE
}

