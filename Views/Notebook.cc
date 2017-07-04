/*
	File:		Notebook.cc

	Contains:	Application classes

	Written by:	Newton Research Group.
*/
#include "Quartz.h"

#include "Objects.h"
#include "ROMResources.h"
#include "Arrays.h"
#include "Lookup.h"
#include "Funcs.h"
#include "Iterators.h"
#include "OSErrors.h"
#include "MagicPointers.h"
#include "Preference.h"
#include "Locales.h"
#include "Sound.h"

#include "RootView.h"
#include "Screen.h"
#include "Notebook.h"
#include "SplashScreen.h"
#include "DrawText.h"
#include "DrawShape.h"
#include "Librarian.h"
#include "Recognition.h"
#include "Inker.h"


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/
extern "C" {
Ref	FLoadFontCache(RefArg inRcvr);
}
extern void			InitPrintDrivers(void);
extern void			InitFontLoader(void);
extern void			InitScriptGlobals(void);
extern void			RunInitScripts(void);

extern "C" void	PrintObject(Ref obj, int indent);
extern Ref			InitDarkStar(RefArg inRcvr, RefArg inOptions);


void
InitFontLoader(void)
{
/*
	gFontPartHandler = new CPartHandler();	// 0C1053FC
	gFontPartHandler->init('font');
	LoadFontTable();	// which really does nothing
*/
}


Ref
FLoadFontCache(RefArg inRcvr)
{
/*
	LoadFontTable();	// which really does nothing
*/
	return NILREF;
}


/*------------------------------------------------------------------------------
	C N o t e b o o k
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clNotebook, CNotebook, CApplication)


CNotebook::CNotebook()
{ }


/*------------------------------------------------------------------------------
	Initialize.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CNotebook::init(void)
{
	CApplication::init();

	gRootView = new CRootView;
	gRootView->init(RA(viewRoot), NULL);

	InitLibrarian();
}


/*------------------------------------------------------------------------------
	Initialize toolbox components.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CNotebook::initToolbox(void)
{
	CApplication::initToolbox();

	initOffscreenBitmaps();
	InitScriptGlobals();
	initInker();

	Ref	screenOrientation(GetPreference(SYMA(screenOrientation)));
	if (NOTNIL(screenOrientation))
		SetOrientation(RINT(screenOrientation));
	else
	{
		int	grafOrientation;
		GetGrafInfo(kGrafOrientation, &grafOrientation);
		SetOrientation(grafOrientation);
	}

	drawSplashScreen();
	FPlaySoundIrregardless(RA(NILREF), RA(bootSound));

	InitPrintDrivers();
	InitFontLoader();
	InitInternationalUtils();
	gRecognitionManager.init(2);
	RunInitScripts();
	InitDarkStar(RA(gFunctionFrame), RA(NILREF));
}


/*------------------------------------------------------------------------------
	Initialize the offscreen bitmaps used for screen drawing.
	Not referenced anywhere yet.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CNotebook::initOffscreenBitmaps(void)
{
#if 0
	NewtGlobals *	newt = GetNewtGlobals();

	gScreenPort = *newt->graf;	// yes, copy the struct
	gScreenPort.visRgn = NewRgn();
	gScreenPort.clipRgn = NewRgn();
	InitPortRgns(&gScreenPort);
	// CGBitmapContextCreate(screenBuf, gScreenWidth, gScreenHeight, 8, bytesPerRow, CGColorSpaceCreateDeviceGray(), kCGImageAlphaNone);	 or is this a QD replacement?
#endif
}


/*------------------------------------------------------------------------------
	Initialize the inker task.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CNotebook::initInker(void)
{
	StartInker(GetNewtTaskPort());
}


/*------------------------------------------------------------------------------
	Draw a splash screen to be admired while the system starts up.
	0,0	 ----
			| xx |
			| xx |
			|    |
			| -- |  60,340
			 ----  320,480 
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/
extern void		QDStartDrawing(NativePixelMap * inPixmap, Rect * inBounds);
extern void		QDStopDrawing(NativePixelMap * inPixmap, Rect * inBounds);

const UniChar*	kSplashStrings[4] =
{
	(const UniChar *)"N\000e\000w\000t\000o\000n\000 \000\000",
	(const UniChar *)"\251\0002\0000\0000\0004\000-\0002\0000\0001\0005\000\000",
	(const UniChar *)"N\000e\000w\000t\000o\000n\000 \000R\000e\000s\000e\000a\000r\000c\000h\000 \000G\000r\000o\000u\000p\000.\000\000",
	(const UniChar *)"A\000l\000l\000 \000r\000i\000g\000h\000t\000s\000 \000r\000e\000s\000e\000r\000v\000e\000d\000.\000\000"
};

extern void
DrawUnicodeText(const UniChar * inStr, size_t inLength, /* inFont,*/ const Rect * inBox, CGColorRef inColor, ULong inJustify);	// SRB


void
CNotebook::drawSplashScreen(void)
{
	Rect box;

	QDStartDrawing(NULL, NULL);
	// paint the screen black
	SetRect(&box, 0, 0, gScreenWidth, gScreenHeight);
	CGContextSetFillColorWithColor(quartz, gBlackColor);
	CGContextFillRect(quartz, MakeCGRect(box));

	// give licensee a chance to make a splash
	SetRect(&box, 0, 0, gScreenWidth, gScreenHeight - 140);
	bool	isSplashed;
	CSplashScreenInfo * licenseeInfo = DrawSplashGraphic(&isSplashed, &box);
	if (!isSplashed)
		// no licensee splash -- show Newton logo
		DrawPicture(RA(bootLogoBitmap), &box, vjCenterH + vjCenterV, modeCopy);

	//	draw the splash text strings
	UniChar			strBuf[128], * str;
	ULong				strLen;
	TextOptions		txOptions;
	StyleRecord		txStyle;
	StyleRecord *	txStyles = &txStyle;
	FPoint			txLoc = { 0, 260 };

	txOptions.justification = 0.5;
	txOptions.width = gScreenWidth;
	txOptions.transferMode = 3;
	
	CreateTextStyleRecord(ROM_fontSystem9Bold, &txStyle);

	if (licenseeInfo)
	{
		//	draw screen info text - up to 3 concatenated nul-terminated strings
		str = strBuf;
		if (licenseeInfo->getText(str))
		{
			strLen = Ustrlen(str);
			txLoc.y = gScreenHeight - 140 - 30;
			DrawTextOnce(str, strLen, &txStyles, NULL, txLoc, &txOptions, NULL);
			str += (strLen + 1);
			if (*str != kEndOfString)
			{
				strLen = Ustrlen(str);
				txLoc.y += 10;
				DrawTextOnce(str, strLen, &txStyles, NULL, txLoc, &txOptions, NULL);

				str += (strLen + 1);
				if (*str != kEndOfString)
				{
					strLen = Ustrlen(str);
					txLoc.y += 10;
					DrawTextOnce(str, strLen, &txStyles, NULL, txLoc, &txOptions, NULL);
				}
			}
		}
		licenseeInfo->destroy();
	}

//	Append system version to first splash string which is "Newton "
	CUGestalt				gestalt;
	GestaltSystemInfo		sysInfo;
	if (gestalt.gestalt(kGestalt_SystemInfo, &sysInfo, sizeof(sysInfo)) == noErr)
	{
		str = strBuf;
		Ustrcpy(str, kSplashStrings[0]);
		VersionString(&sysInfo, str + Ustrlen(str));
CGContextSetStrokeColorWithColor(quartz, gWhiteColor);

	//	Draw the splash strings
		txLoc.y = gScreenHeight - 140;
		for (ArrayIndex i = 1; i <= 4; ++i)
		{
Rect box;
box.top = txLoc.y;
box.left = 0;
			txLoc.y += 10;
//			DrawTextOnce(str, Ustrlen(str), &txStyles, NULL, txLoc, &txOptions, NULL);
box.bottom = txLoc.y;
box.right = 320;
DrawUnicodeText(str, Ustrlen(str), &box, gWhiteColor, vjCenterH);
			if (i < 4)
				str = (UniChar *)kSplashStrings[i];
		}
/*
		Better use of CoreText:
		create one string with line breaks
		create rect for drawing into - this must be made into a path at some point
		DrawText function needs args:
			string
			length of string?
			styles - ROM_fontSystem9Bold
			options - justification, transfer mode?, line spacing?
			rect into which to draw
*/

	}
}


/*------------------------------------------------------------------------------
	Run the application.
	While we’re idle, update the root view.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CNotebook::run(void)
{
	ArrayIndex i = 10;
	do
	{
		idle();
		if (gRootView->needsUpdate())
			gRootView->update();
	} while (--i > 0 && needsIdle());
}


/*------------------------------------------------------------------------------
	When the global time is past our idle time we should perform the regular
	idle tasks.
	Args:		--
	Return:	bool		it’s time to idle
------------------------------------------------------------------------------*/

bool
CNotebook::needsIdle(void)
{
if (fIdleTime != CTime(0) && fIdleTime < GetGlobalTime()) printf("CNotebook::needsIdle() fIdleTime=%lld\n", (int64_t)fIdleTime);
	return fIdleTime != CTime(0) && fIdleTime < GetGlobalTime();
}


/*------------------------------------------------------------------------------
	Let the application, views and recognition manager all have idle time.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CNotebook::idle(void)
{
	CApplication::idle();

	gRecognitionManager.idle();
	fIdleTime = gRootView->idleViews();
	updateNextIdleTime(gRecognitionManager.nextIdle());
}


/*------------------------------------------------------------------------------
	When the application quits, release the offscreen bitmap.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CNotebook::quit(void)
{
#if 0
	ClosePort(&gScreenPort);	// CGContextRelease(gScreenContext);
#endif
}


#pragma mark -
/*------------------------------------------------------------------------------
	C A R M N o t e b o o k
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clARMNotebook, CARMNotebook, CNotebook)

