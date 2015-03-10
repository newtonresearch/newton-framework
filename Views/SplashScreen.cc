/*
	File:		SplashScreen.cc

	Contains:	Splash screen class and drawing implementation.

	Written by:	Newton Research Group, 2007.
*/

#include "DrawShape.h"

#include "Objects.h"
#include "SplashScreen.h"


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern "C" {
Ref	FDisplaySplashGraphic(RefArg rcvr, RefArg inBounds);
}


/*------------------------------------------------------------------------------
	Generate a version string from gestalt system info.
	Args:		info
				outStr		buffer into which to place text
	Return:	--
------------------------------------------------------------------------------*/

void
VersionString(GestaltSystemInfo * info, UniChar * outStr)
{
	// hack in marketing version
	if (info->fROMVersion == 0x00020002
	&&  info->fROMStage == 0x00008000)
		ConvertToUnicode("2.1 (717006)", outStr);

	else
	{
		UniChar * str = outStr;
		int	stage, patch;
		int	bcd[7];
		// split the fROMVersion into 7 BCD nibls
		for (ArrayIndex i = 0; i < 7; ++i)
			bcd[i] = (info->fROMVersion >> (i*4)) & 0x0F;

		// add up to 3 major version digits to the output string
		if (bcd[6] != 0)
		{
			*str++ = '0' + bcd[6];
			*str++ = '0' + bcd[5];
			*str++ = '0' + bcd[4];
		}
		else if (bcd[5] != 0)
		{
			*str++ = '0' + bcd[5];
			*str++ = '0' + bcd[4];
		}
		else
			*str++ = '0' + bcd[4];

		// now minor version digits
		if ((info->fROMVersion & 0xFFFF) != 0)
		{
			int	decimal = 0;
			*str++ = '.';
			if (bcd[3] != 0)
				decimal = 3;
			else if (bcd[2] != 0)
				decimal = 2;
			else if (bcd[1] != 0)
				decimal = 1;
			while (decimal-- >= 0)
				*str++ = '0' + bcd[decimal];
		}

		// append build stage
		stage = (info->fROMStage >> 8) & 0xFF;
		if (stage != 0x80)
		{
			if (stage == 0x00)
			{
				*str++ = 'A';
				*str++ = 'S';
			}
			else if (stage == 0x20)
				*str++ = 'd';
			else if (stage == 0x40)
				*str++ = 'a';
			else if (stage == 0x60)
				*str++ = 'b';

			stage = info->fROMStage & 0xFF;
			if (stage != 0)
			{
				int digit;
				digit = stage >> 4;
				if (digit != 0)
					*str++ = '0' + digit;
				digit = stage & 0x0F;
				*str++ = '0' + digit;
			}
		}

		// append patch version
		div_t patchVersion;
		*str++ = '.';
		patch = info->fPatchVersion;
		if (patch > 99)
			patch = 99;
		patchVersion = div(patch, 10);
		*str++ = '0' + patchVersion.quot;
		*str++ = '0' + patchVersion.rem;

		// terminate the string
		*str = 0;
#if defined(correct)
		// give licensee a chance to add to the version string
		CVersionString * licenseeInfo = (CVersionString *)MakeByName("CVersionString", "CMainVersionString");
		if (licenseeInfo != NULL)
		{
			licenseeInfo->versionString(outStr);
			licenseeInfo->destroy();
		}
#endif
	}
}


/*------------------------------------------------------------------------------
	Display the splash graphic.
------------------------------------------------------------------------------*/

Ref
FDisplaySplashGraphic(RefArg rcvr, RefArg inBounds)
{
	CSplashScreenInfo * licenseeInfo;
	bool	isDone;
	Rect	bounds;
	FromObject(inBounds, &bounds);
	licenseeInfo = DrawSplashGraphic(&isDone, &bounds);
	if (licenseeInfo)
		licenseeInfo->destroy();
	return MAKEBOOLEAN(isDone);
}


CSplashScreenInfo *
DrawSplashGraphic(bool * outDrawn, const Rect * inRect)
{
	*outDrawn = NO;
	CSplashScreenInfo * licenseeInfo = (CSplashScreenInfo *)MakeByName("CSplashScreenInfo", "CMainSplashScreenInfo");
	if (licenseeInfo)
	{
#if defined(correct)
		PictureShape *	graphic;
		if (licenseeInfo->getBits(&graphic))
		{
			Rect		frame;
			frame = graphic->bBox;
			Justify(&frame, inRect, vjCenterH + vjCenterV);
			DrawPicture(&graphic, &frame, 0);
			*outDrawn = YES;
		}
#endif
	}
	return licenseeInfo;
}


/*------------------------------------------------------------------------------
	C M a i n V e r s i o n S t r i n g
------------------------------------------------------------------------------*/

PROTOCOL CMainVersionString : public CVersionString
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CMainVersionString)

	CMainVersionString *	make(void);
	void			destroy(void);

	bool			versionString(UniChar * outStr);
};


/*------------------------------------------------------------------------------
	C M a i n S p l a s h S c r e e n I n f o
------------------------------------------------------------------------------*/

PROTOCOL CMainSplashScreenInfo : public CSplashScreenInfo
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CMainSplashScreenInfo)

	CMainSplashScreenInfo *	make(void);
	void			destroy(void);

	bool			getBits(PictureShape ** pic);
	bool			getText(UniChar * outStr);
};

// We don’t have a separate splash screen.
