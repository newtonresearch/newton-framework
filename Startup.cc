/*
	File:		Startup.cc

	Contains:	Startup implementation.

	Written by:	Newton Research Group, 2010.
*/

#include "UserGlobals.h"
#include "Protocols.h"


/* -----------------------------------------------------------------------------
	C S t a r t u p D r i v e r
----------------------------------------------------------------------------- */

PROTOCOL CStartupDriver : public CProtocol
{
public:
	static CStartupDriver *	make(const char * inName);
	void		destroy(void);

	void		init(void);
};


PROTOCOL CMainStartupDriver : public CStartupDriver
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CMainStartupDriver)

	CMainStartupDriver *	make(void);
	void		destroy(void);

	void		init(void);
};


/* -----------------------------------------------------------------------------
	C E r a s e P e r s i s t e n t D a t a A l e r t
----------------------------------------------------------------------------- */
#include "Alerts.h"

class CErasePersistentDataAlert : public CAlertDialog
{
public:
};


#pragma mark -
/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

CErasePersistentDataAlert gPersistentDataAlert;	// 0C1010E8.x00
int gIsPersistentDataAlertInited = NO;	// 0C1010E8.x64 = 0C10114C

CStartupDriver * gStartupDriver;			// 0C1010E8.x68 = 0C101150


/* -----------------------------------------------------------------------------
	Load the startup driver.
	I don’t believe the startup driver does anything. Or is even implemented.
----------------------------------------------------------------------------- */

void
LoadStartupDriver(void)
{
#if defined(correct)
	gStartupDriver = (CStartupDriver *)MakeByName("CStartupDriver", "CMainStartupDriver");
	if (gStartupDriver)
		gStartupDriver->init();
#endif
}


/* -----------------------------------------------------------------------------
	Ask the user whether persistent data should be erased.
----------------------------------------------------------------------------- */

void
ZapInternalStoreCheck(void)
{
#if defined(correct)
	if ((gNewtConfig & 0x00040000) != 0)
		ClobberInternalFlash();
	if (UserWantsColdBoot())
	{
		InitScreen();
		CUPort sp00;
		sp00.init();
		GetTabletCalibrationDataSWI();
		StartInker(&sp00);
		if (!gIsPersistentDataAlertInited)
		{
			ULong sp00;
			gIsPersistentDataAlertInited = YES;
			new (&gPersistentDataAlert) CErasePersistentDataAlert;
			gPersistentDataAlert.init((UniChar *)BinaryData(RA(uErasePersistentDataAlertText)),	// "Do you want to erase data completely?"
											  (UniChar *)BinaryData(RA(uErasePersistentDataButton0Str)),	// "No"
											  (UniChar *)BinaryData(RA(uErasePersistentDataButton1Str)));	// "Yes"
			if (gPersistentDataAlert.alert(&sp00) == 0 && sp00 == 1)
			{
				// confirm
				gPersistentDataAlert.init((UniChar *)BinaryData(RA(uErasePersistentConfirmAlertText)),	// "All your data will be lost. Do you want to continue?"
												  (UniChar *)BinaryData(RA(uErasePersistentConfirmButton0Str)),	// "Cancel"
												  (UniChar *)BinaryData(RA(uErasePersistentConfirmButton1Str)));	// "OK"
				if (gPersistentDataAlert.alert(&sp00) == 0 && sp00 == 1)
				{
					gGlobalsThatLiveAcrossReboot.f138 = 0;
					Rect alertTextBounds;
					alertTextBounds.top = RINT(GetFrameSlot(RA(kOSErrorAlertTextBoundsNoButtons, SYMA(top))));	// { 10, 10, 70, 182 }
					alertTextBounds.left = RINT(GetFrameSlot(RA(kOSErrorAlertTextBoundsNoButtons, SYMA(left))));
					alertTextBounds.bottom = RINT(GetFrameSlot(RA(kOSErrorAlertTextBoundsNoButtons, SYMA(bottom))));
					alertTextBounds.right = RINT(GetFrameSlot(RA(kOSErrorAlertTextBoundsNoButtons, SYMA(right))));
					gPersistentDataAlert.init(&alertTextBounds,
													  (UniChar *)BinaryData(RA(uErasePersistentStatusAlertText)),		// "Erasing data. This operation will take a couple of minutes. The system will reboot when done."
													  (UniChar *)BinaryData(RA(uErasePersistentStatusEmptyButtonStr)),	// ""
													  (UniChar *)BinaryData(RA(uErasePersistentStatusEmptyButtonStr)));	// ""
					gPersistentDataAlert.displayAlert();
					ClobberInternalFlash();
					gPersistentDataAlert.removeAlert();
					Reboot(noErr, kRebootMagicNumber, NO);	// cold boot
				}
			}
			Reboot(noErr, 0, NO);	//warm boot
		}
	}
#endif
}

