/*
	File:		CardServer.cc

	Contains:	PC card server implementation.

	Written by:	Newton Research Group, 2010.
*/

#include "Newton.h"


NewtonErr
InitCardServices(void)
{
#if defined(correct)
	NewtonErr err;
	XTRY
	{
		//sp-290C
		CCardDomains domains;	// sp29E8
		CCardServer server;		// sp00
		XFAIL(err = domains.init())
		err = server.init('cdsv', true, kSpawnedTaskStackSize);
	}
	XENDTRY;
	return err;
#else
	return noErr;
#endif
}

