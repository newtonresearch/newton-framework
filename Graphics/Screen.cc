/*
	File:		Screen.cc

	Contains:	Display driver.

	Written by:	Newton Research Group.

	Newton (QuickDraw)
	all drawing is done into the screenÕs GrafPort (bitmap)
	which is blitted to the LCD bitmap @ ~33fps (actually only dirty rect is blited)

	Quartz
	all drawing is done into a windowÕs quartz context (presumably off-screen bitmap)
	the quartz compositor blends this context onto the display
so...
	weÕll just draw into the windowÕs quartz context
	at some point we could create a quartz context for each child view of the root, making app views more manageable
alternatively...
	create a CGLayer and draw into its context
	screen task should blit that into windowÕs context
	forFramework, must continue to draw into quartz context
*/

#include "Quartz.h"

#include "Objects.h"
#include "NewtonGestalt.h"
#include "NewtGlobals.h"
#include "UserTasks.h"
#include "Semaphore.h"

#include "Screen.h"
#include "ScreenDriver.h"
#include "QDDrawing.h"
#include "Tablet.h"

extern void SetEmptyRect(Rect * r);


/* -----------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
----------------------------------------------------------------------------- */

extern "C" {
Ref	FGetLCDContrast(RefArg inRcvr);
Ref	FSetLCDContrast(RefArg inRcvr, RefArg inContrast);
Ref	FGetOrientation(RefArg inRcvr);
Ref	FSetOrientation(RefArg inRcvr, RefArg inOrientation);
Ref	FLockScreen(RefArg inRcvr, RefArg inDoIt);
}


/*------------------------------------------------------------------------------
	D i s p l a y   D a t a
	Definition of the display: its PixelMap
------------------------------------------------------------------------------*/

const ScreenParams gScreenConstants =		// was qdConstants 00380BCC
{	{ 0, 5, 4, 0, 3 },
	{ 0, 3, 2, 0, 1 },
	{ 0, 0x1F, 0x0F, 0, 0x07 } };

NativePixelMap	gScreenPixelMap;				// was qd.pixmap


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

CGContextRef	quartz;

int				gScreenWidth;			// 0C104C58
int				gScreenHeight;			// 0C104C5C

struct
{
	CScreenDriver *		driver;			// +00	 actually CMainDisplayDriver protocol
	CScreenDriver *		auxDriver;		// +04
	int						f08;				// +08
	Rect						dirtyBounds;	// +0C
	CUTask *					updateTask;		// +14
	CUSemaphoreGroup *	semaphore;		// +18
	CUSemaphoreOpList *	f1C;				// acquire display
	CUSemaphoreOpList *	f20;				// release display
	CUSemaphoreOpList *	f24;
	CUSemaphoreOpList *	f28;
	CUSemaphoreOpList *	f2C;
	CUSemaphoreOpList *	f30;
	CUSemaphoreOpList *	f34;
	CUSemaphoreOpList *	f38;
	CULockingSemaphore *	lock;				// +3C
} gScreen;	// 0C101A4C


struct AlertScreenInfo
{
	CScreenDriver *	display;
	NativePixelMap		pixmap;
	int					orientation;
};

AlertScreenInfo gAlertScreenInfo;		// 0C105EF0



/* -----------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------------- */

static void	SetupScreen(void);
static void	SetupScreenPixelMap(void);

void	SetScreenInfo(void);
void	SetAlertScreenInfo(AlertScreenInfo * info);

void	InitScreenTask(void);
void	ScreenUpdateTask(void * inTask, size_t inSize, ObjectId inArg);
extern "C" void	UpdateHardwareScreen(void);
void	BlitToScreens(NativePixelMap * inPixmap, Rect * inSrcBounds, Rect * inDstBounds, int inTransferMode);


#pragma mark -
/*------------------------------------------------------------------------------
	Set up the Quartz graphics system.
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

extern "C" void
SetupQuartzContext(CGContextRef inContext)
{
	quartz = inContext;
#if 1
	CGContextTranslateCTM(quartz, 10.0, 20.0);
#else
	CGContextTranslateCTM(quartz, 10.0, 20.0+480.0);	// gScreenHeight
	CGContextScaleCTM(quartz, 1.0, -1.0);
#endif
}


#if defined(forFramework)
void
InitDrawing(CGContextRef inContext, int inScreenHeight)
{
	quartz = inContext;
	gScreenHeight = inScreenHeight;
}
#endif


#pragma mark -
/*------------------------------------------------------------------------------
	Initialize the screen.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
InitScreen(void)
{
#if !defined(forFramework)
	// initialize the driver(s)
	SetupScreen();
	gScreen.driver->screenSetup();
	gScreen.driver->powerInit();

	// initialize the QD pixmap
	SetupScreenPixelMap();
	if (gScreenPixelMap.baseAddr == NULL)
	{
	// allocate enough memory for either screen orientation
		ScreenInfo info;
		int			landscapePix, portraitPix, screenPix;
		int			pixAlignment, screenBytes;

		gScreen.driver->getScreenInfo(&info);
		pixAlignment = 8 * 8 / info.depth;
		landscapePix = (((info.width + pixAlignment - 1) & -pixAlignment) / 8) * info.height;
		portraitPix = (((info.height + pixAlignment - 1) & -pixAlignment) / 8) * info.width;
		screenPix = (landscapePix > portraitPix) ? landscapePix : portraitPix;
		screenBytes = screenPix * info.depth;
		gScreenPixelMap.baseAddr = NewPtr(screenBytes);
		memset(gScreenPixelMap.baseAddr, 0, screenBytes);
		SetScreenInfo();
	}

	SetEmptyRect(&gScreen.dirtyBounds);
	InitScreenTask();
#endif
}


/*------------------------------------------------------------------------------
	Set up the screen driver(s).
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

static void
SetupScreen(void)
{
	gScreen.driver = (CScreenDriver *)MakeByName("CScreenDriver", "CMainDisplayDriver");
	gScreen.auxDriver = NULL;
	gScreen.f08 = 0;
}


/*------------------------------------------------------------------------------
	Set up the screenÕs pixmap.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

static void
SetupScreenPixelMap(void)
{
	ScreenInfo info;
	int			pixAlignment;

	gScreen.driver->getScreenInfo(&info);
	pixAlignment = 8 * 8 / info.depth;
	gScreenPixelMap.rowBytes = (((info.width + pixAlignment - 1) & -pixAlignment) * info.depth) / 8;
	SetRect(&gScreenPixelMap.bounds, 0, 0, info.width, info.height);
	gScreenPixelMap.pixMapFlags = kPixMapPtr + info.depth;
	gScreenPixelMap.deviceRes.h = info.resolution.h;
	gScreenPixelMap.deviceRes.v = info.resolution.v;
	gScreenPixelMap.grayTable = NULL;

	SetScreenInfo();
}


/*------------------------------------------------------------------------------
	Set up the screen info record.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
SetScreenInfo(void)
{
	AlertScreenInfo info;

	info.display = gScreen.driver;

	if (gScreenPixelMap.baseAddr)
		info.pixmap = gScreenPixelMap;	// sic
	info.pixmap.rowBytes = gScreenPixelMap.rowBytes;
	info.pixmap.bounds.top = gScreenPixelMap.bounds.top;
	info.pixmap.bounds.bottom = gScreenPixelMap.bounds.bottom;
	info.pixmap.bounds.left = gScreenPixelMap.bounds.left;
	info.pixmap.bounds.right = gScreenPixelMap.bounds.right;
	info.pixmap.pixMapFlags = gScreenPixelMap.pixMapFlags;
	info.pixmap.deviceRes = gScreenPixelMap.deviceRes;
	info.pixmap.grayTable = gScreenPixelMap.grayTable;

	info.orientation = gScreen.driver->getFeature(4);
	
	SetAlertScreenInfo(&info);
}

void
SetAlertScreenInfo(AlertScreenInfo * info)
{
	// the original does field-by-field copy
	gAlertScreenInfo = *info;
}


#pragma mark -

/*------------------------------------------------------------------------------
	Set the physical orientation of the screen.
	Args:		inOrientation	the orientation
	Return:	--
------------------------------------------------------------------------------*/
Ref
FSetOrientation(RefArg inRcvr, RefArg inOrientation)
{
	SetOrientation(RINT(inOrientation));
	return NILREF;
}


void
SetOrientation(int inOrientation)
{
	SetGrafInfo(kGrafOrientation, inOrientation);
#if !defined(forFramework)
	TabSetOrientation(inOrientation);
#endif
#if 0
	GetGrafInfo(kGrafPixelMap, &gScreenPort.portBits);
	gScreenPort.portRect = gScreenPort.portBits.bounds;
	InitPortRgns(&gScreenPort);
#endif

#if !defined(forFramework)
	NewtGlobals *	newt;
/*	if ((newt = GetNewtGlobals()) != NULL)
	{
		GrafPtr	newtGraf = newt->graf;
		GetGrafInfo(kGrafPixelMap, newtGraf);
		newtGraf->portRect = newtGraf->portBits.bounds;
		InitPortRgns(newtGraf);
	}
*/
	CUGestalt				gestalt;
	GestaltSystemInfo		sysInfo;
	gestalt.gestalt(kGestalt_SystemInfo, &sysInfo, sizeof(sysInfo));
	bool	isWideScreenHardware = (sysInfo.fScreenWidth > sysInfo.fScreenHeight);
	if (inOrientation == kLandscape || inOrientation == kLandscapeFlip)
	{
		if (isWideScreenHardware)
		{
			gScreenWidth = sysInfo.fScreenWidth;
			gScreenHeight = sysInfo.fScreenHeight;
		}
		else
		{
			gScreenWidth = sysInfo.fScreenHeight;
			gScreenHeight = sysInfo.fScreenWidth;
		}
	}
	else
	{
		if (isWideScreenHardware)
		{
			gScreenWidth = sysInfo.fScreenHeight;
			gScreenHeight = sysInfo.fScreenWidth;
		}
		else
		{
			gScreenWidth = sysInfo.fScreenWidth;
			gScreenHeight = sysInfo.fScreenHeight;
		}
	}
#endif
}


Ref
FGetOrientation(RefArg inRcvr)
{
	int	grafOrientation;
	GetGrafInfo(kGrafOrientation, &grafOrientation);
	return MAKEINT(grafOrientation);
}


Ref
FGetLCDContrast(RefArg inRcvr)
{
	int	contrast;
	GetGrafInfo(kGrafContrast, &contrast);
	return MAKEINT(contrast);
}


Ref
FSetLCDContrast(RefArg inRcvr, RefArg inContrast)
{
	SetGrafInfo(kGrafContrast, RINT(inContrast));
	return NILREF;
}


/*------------------------------------------------------------------------------
	Set info about the screen.
	Args:		inSelector		the type of info
				inInfo			the info
	Return:	error code
------------------------------------------------------------------------------*/

void
SetGrafInfo(int inSelector, int inValue)
{
	switch (inSelector)
	{
	case kGrafContrast:
		if (gScreen.driver)
			gScreen.driver->setFeature(0, inValue);
/*		CADC *	adc = GetADCObject();
		if (adc)
			adc->getSample(10, 1000, ContrastTempSample, NULL);
*/		break;

	case kGrafOrientation:
		if (gScreen.driver)
			gScreen.driver->setFeature(4, inValue);
		SetupScreenPixelMap();
		memset(GetPixelMapBits(&gScreenPixelMap), 0, (gScreenPixelMap.bounds.bottom - gScreenPixelMap.bounds.top) * gScreenPixelMap.rowBytes);
		break;

	case kGrafBacklight:
		if (gScreen.driver)
			gScreen.driver->setFeature(2, inValue);
		break;
	}
}


/*------------------------------------------------------------------------------
	Return info about the screen.
	Args:		inSelector		the type of info required
				outInfo			pointer to result
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
GetGrafInfo(int inSelector, void * outInfo)
{
	NewtonErr err = noErr;

	switch (inSelector)
	{
	case kGrafPixelMap:
		*(NativePixelMap *)outInfo = gScreenPixelMap;
		break;

	case kGrafResolution:
		*(Point *)outInfo = gScreenPixelMap.deviceRes;
		break;

	case kGrafPixelDepth:
		*(ULong *)outInfo = PixelDepth(&gScreenPixelMap);
		break;

	case kGrafContrast:
		if (gScreen.driver)
			*(int*)outInfo = gScreen.driver->getFeature(0);
		else
		{
			*(int*)outInfo = 0;
			err = -1;
		}
		break;

	case kGrafOrientation:
		if (gScreen.driver)
			*(int*)outInfo = gScreen.driver->getFeature(4);
		else
		{
			*(int*)outInfo = 2;	// 1 in the original, but we prefer portrait
			err = -1;
		}
		break;

	case kGrafBacklight:
		if (gScreen.driver)
			*(int*)outInfo = gScreen.driver->getFeature(2);
		else
		{
			*(int*)outInfo = 0;	// no display, no backlight
			err = -1;
		}
		break;

	case 6:
		if (gScreen.driver)
			*(int*)outInfo = gScreen.driver->getFeature(5);
		else
		{
			*(int*)outInfo = 10;
			err = -1;
		}
		break;

	case kGrafScreen:
		if (gScreen.driver)
			gScreen.driver->getScreenInfo((ScreenInfo *)outInfo);
		else
			err = -1;
		break;
	default:
		err = -1;
	}

	return err;
}

#pragma mark -

void
InitScreenTask(void)
{
#if !defined(forFramework)
	if (gScreen.semaphore == NULL)
	{
		CUSemaphoreGroup *	semGroup;	// size +08
		semGroup = new CUSemaphoreGroup;
		gScreen.semaphore = semGroup;
		semGroup->init(3);

		CULockingSemaphore *	semLock;		// size +0C
		semLock = new CULockingSemaphore;
		gScreen.lock = semLock;
		semLock->init();

		CUSemaphoreOpList *	semList;		// size +08
		semList = new CUSemaphoreOpList;
		gScreen.f24 = semList;
		semList->init(1, MAKESEMLISTITEM(2,1));

		semList = new CUSemaphoreOpList;
		gScreen.f28 = semList;
		semList->init(1, MAKESEMLISTITEM(2,-1));

		semList = new CUSemaphoreOpList;
		gScreen.f38 = semList;
		semList->init(3, MAKESEMLISTITEM(2,-1), MAKESEMLISTITEM(2,0), MAKESEMLISTITEM(2,1));

		semList = new CUSemaphoreOpList;
		gScreen.f2C = semList;
		semList->init(1, MAKESEMLISTITEM(1,1));

		semList = new CUSemaphoreOpList;
		gScreen.f1C = semList;
		semList->init(2, MAKESEMLISTITEM(0,0), MAKESEMLISTITEM(0,1));

		semList = new CUSemaphoreOpList;
		gScreen.f20 = semList;
		semList->init(1, MAKESEMLISTITEM(0,-1));

		semList = new CUSemaphoreOpList;
		gScreen.f30 = semList;
		semList->init(4, MAKESEMLISTITEM(0,0), MAKESEMLISTITEM(0,1), MAKESEMLISTITEM(1,-1), MAKESEMLISTITEM(2,0));

		semList = new CUSemaphoreOpList;
		gScreen.f34 = semList;
		semList->init(1, MAKESEMLISTITEM(0,-1));

// we donÕt use this frame-buffering scheme
#if defined(correct)
		CUTask * task = new CUTask;
		gScreen.updateTask = task;
		task->init(ScreenUpdateTask, 4*KByte, 0, NULL, kScreenTaskPriority, 'scrn');

		gScreen.driver->powerOn();
		task->start();
#endif
	}
#endif
}


void
ScreenUpdateTask(void * inTask, size_t inSize, ObjectId inArg)
{
#if defined(correct)
	CTime		nextUpdateTime;
//	CADC *	adc = GetADCObject();

	for (;;)
	{
		gScreen.semaphore->semOp(gScreen.f30, kWaitOnBlock);
		gScreen.lock->acquire(kWaitOnBlock);

		nextUpdateTime = TimeFromNow(30*kMilliseconds);
		UpdateHardwareScreen();

		gScreen.lock->release();
		gScreen.semaphore->semOp(gScreen.f34, kWaitOnBlock);

		if (GetGlobalTime() > nextUpdateTime)
		{
			nextUpdateTime = TimeFromNow(30*kMilliseconds);
/*			if (adc)
				adc->getSample(10, 1000, ContrastTempSample, NULL);
			else
				adc = GetADCObject();
*/		}

		SleepTill(&nextUpdateTime);
	}
#endif
}


/*------------------------------------------------------------------------------
	Update the display.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
UpdateHardwareScreen(void)
{
	Rect r;

	// only bother if the dirty area is actually on-screen
	if (SectRect(&gScreen.dirtyBounds, &gScreenPixelMap.bounds, &r))
		BlitToScreens(&gScreenPixelMap, &r, &r, 0);	// srcCopy
	// it's no longer dirty
	SetEmptyRect(&gScreen.dirtyBounds);
}


/*------------------------------------------------------------------------------
	Blit a pixmap to the display.
	If thereÕs an external display connected, blit to that too.
	Args:		inPixmap			the pix map to be blitted
				inSrcBounds		bounds  of inPixmap to use
				inDstBounds		bounds to blit into
				inTransferMode
	Return:	--
------------------------------------------------------------------------------*/

void
BlitToScreens(NativePixelMap * inPixmap, Rect * inSrcBounds, Rect * inDstBounds, int inTransferMode)
{
/*
	gScreen.driver->blit(inPixmap, inSrcBounds, inDstBounds, inTransferMode);

	if (gScreen.auxDriver)
		gScreen.auxDriver->blit(inPixmap, inSrcBounds, inDstBounds, inTransferMode);
*/
}

#pragma mark -

void
StartDrawing(PixelMap * inPixmap, Rect * inBounds)
{
	if (inPixmap == NULL)
#if defined(correct)
		inPixmap = &GetCurrentPort()->portBits;
#else
		/*inPixmap = &gScreenPixelMap*/;
#endif

#if !defined(forFramework)
	if (GetPixelMapBits(inPixmap) == GetPixelMapBits(&gScreenPixelMap))
		gScreen.semaphore->semOp(gScreen.f24, kWaitOnBlock);
#endif
}


void
StopDrawing(PixelMap * inPixmap, Rect * inBounds)
{
	if (inPixmap == NULL)
#if defined(correct)
		inPixmap = &GetCurrentPort()->portBits;
#else
//		inPixmap = &gScreenPixelMap;
#endif

	if (GetPixelMapBits(inPixmap) == GetPixelMapBits(&gScreenPixelMap))
	{
		if (inBounds)
		{
			Rect	bbox = *inBounds;
			OffsetRect(&bbox, -inPixmap->bounds.left, -inPixmap->bounds.top);
			UnionRect(&bbox, &gScreen.dirtyBounds, &gScreen.dirtyBounds);
		}

#if !defined(forFramework)
		if (gScreen.semaphore->semOp(gScreen.f38, kNoWaitOnBlock) == 0)
		{
			gScreen.semaphore->semOp(gScreen.f1C, kWaitOnBlock);
			UpdateHardwareScreen();
			gScreen.lock->release();
			gScreen.semaphore->semOp(gScreen.f20, kWaitOnBlock);
		}
		gScreen.semaphore->semOp(gScreen.f28, kNoWaitOnBlock);
#endif
	}

//	CGContextFlush(quartz);
}


void
QDStartDrawing(PixelMap * inPixmap, Rect * inBounds)
{
	if (inPixmap == NULL)
#if defined(correct)
		inPixmap = &GetCurrentPort()->portBits;
#else
		/*inPixmap = &gScreenPixelMap*/;
#endif

#if !defined(forFramework)
	if (GetPixelMapBits(inPixmap) == GetPixelMapBits(&gScreenPixelMap))
		//	weÕre drawing to the display so acquire a lock
		gScreen.lock->acquire(kWaitOnBlock);
#endif
}


void
QDStopDrawing(PixelMap * inPixmap, Rect * inBounds)
{
	if (inPixmap == NULL)
#if defined(correct)
		inPixmap = &GetCurrentPort()->portBits;
#else
//		inPixmap = &gScreenPixelMap;
#endif

	if (GetPixelMapBits(inPixmap) == GetPixelMapBits(&gScreenPixelMap))
	{
		bool	isAnythingToDraw = !EmptyRect(&gScreen.dirtyBounds);
		if (inBounds)
		{
			Rect	bbox = *inBounds;
			OffsetRect(&bbox, -inPixmap->bounds.left, -inPixmap->bounds.top);
			UnionRect(&bbox, &gScreen.dirtyBounds, &gScreen.dirtyBounds);
		}

#if !defined(forFramework)
		if (!isAnythingToDraw)
			gScreen.semaphore->semOp(gScreen.f2C, kWaitOnBlock);
		//	weÕre drawing to the display so release the lock
		gScreen.lock->release();
#endif
	}
}


void
BlockLCDActivity(bool doIt)
{
#if !defined(forFramework)
	gScreen.semaphore->semOp(doIt ? gScreen.f1C : gScreen.f20, kWaitOnBlock);
#endif
}


void
ReleaseScreenLock(void)
{
#if !defined(forFramework)
	while (gScreen.semaphore->semOp(gScreen.f28, kNoWaitOnBlock) == 0)
		;
#endif
}


Ref
FLockScreen(RefArg inRcvr, RefArg inDoIt)
{
	if (NOTNIL(inDoIt))
		StartDrawing(NULL, NULL);
	else
		StopDrawing(NULL, NULL);
	return NILREF;
}

