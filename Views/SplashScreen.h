/*
	File:		SplashScreen.h

	Contains:	Splash screen declarations.
					The splash screen is shown by CNotebook as it starts up.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__SPLASHSCREEN_H)
#define __SPLASHSCREEN_H 1

#include "Protocols.h"
#include "UStringUtils.h"
#include "NewtonGestalt.h"
#include "QDTypes.h"


PROTOCOL CVersionString : public CProtocol
{
public:
	static CVersionString *	make(const char * inName);
	void		destroy(void);

	bool		versionString(UniChar * outStr);
};


PROTOCOL CSplashScreenInfo : public CProtocol
{
public:
	static CSplashScreenInfo *	make(const char * inName);
	void		destroy(void);

	bool		getBits(PictureShape ** outPic);
	bool		getText(UniChar * outStr);
};


extern void						VersionString(GestaltSystemInfo * info, UniChar * outStr);
extern CSplashScreenInfo *	DrawSplashGraphic(bool * outDrawn, const Rect * inRect);


#endif	/* __SPLASHSCREEN_H */
