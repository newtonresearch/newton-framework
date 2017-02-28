/*
	File:		ScreenDriver.cc

	Contains:	Display driver.

	Written by:	Newton Research Group.
*/
#include "Quartz.h"
#include "Geometry.h"

#include "ScreenDriver.h"
#include "ViewFlags.h"
#include "QDDrawing.h"
#include "NewtonGestalt.h"


/* -----------------------------------------------------------------------------
	S c r e e n   P a r a m e t e r s
----------------------------------------------------------------------------- */

#define kScreenWidth		480
#define kScreenHeight	320
#define kScreenDepth		  4
#define kScreenDPI		100


/*------------------------------------------------------------------------------
	B l i t R e c

	Data passed to landscape blit functions.
------------------------------------------------------------------------------*/

struct BlitRec
{
	int		srcRowOffset;	// +00
	int		dstRowOffset;	// +04
	int		x08;
	Ptr		srcAddr;			// +0C
	Ptr		dstAddr;			// +10
	int		mode;				// +14
	int		numOfRows;		// +18
	int		numOfBytes;		// +1C
//	size +20
};


/*------------------------------------------------------------------------------
	C M a i n D i s p l a y D r i v e r
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CMainDisplayDriver implementation class info.  00796564..007977FC
---------------------------------------------------------------- */

const CClassInfo *
CMainDisplayDriver::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN18CMainDisplayDriver6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN18CMainDisplayDriver4makeEv - 0b	\n"
"		.long		__ZN18CMainDisplayDriver7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CMainDisplayDriver\"	\n"
"2:	.asciz	\"CScreenDriver\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN18CMainDisplayDriver9classInfoEv - 4b	\n"
"		.long		__ZN18CMainDisplayDriver4makeEv - 4b	\n"
"		.long		__ZN18CMainDisplayDriver7destroyEv - 4b	\n"
"		.long		__ZN18CMainDisplayDriver11screenSetupEv - 4b	\n"
"		.long		__ZN18CMainDisplayDriver13getScreenInfoEP10ScreenInfo - 4b	\n"
"		.long		__ZN18CMainDisplayDriver9powerInitEv - 4b	\n"
"		.long		__ZN18CMainDisplayDriver7powerOnEv - 4b	\n"
"		.long		__ZN18CMainDisplayDriver8powerOffEv - 4b	\n"
"		.long		__ZN18CMainDisplayDriver4blitEP14NativePixelMapP4RectS3_i - 4b	\n"
"		.long		__ZN18CMainDisplayDriver10doubleBlitEP14NativePixelMapS1_P4RectS3_i - 4b	\n"
"		.long		__ZN18CMainDisplayDriver10getFeatureEi - 4b	\n"
"		.long		__ZN18CMainDisplayDriver10setFeatureEii - 4b	\n"
"		.long		__ZN18CMainDisplayDriver18autoAdjustFeaturesEv - 4b	\n"
"		.long		__ZN18CMainDisplayDriver13enterIdleModeEv - 4b	\n"
"		.long		__ZN18CMainDisplayDriver12exitIdleModeEv - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CMainDisplayDriver)

CMainDisplayDriver *
CMainDisplayDriver::make(void)
{ return this; }


void
CMainDisplayDriver::destroy(void)
{ }


void
CMainDisplayDriver::screenSetup(void)
{
	ScreenInfo	info;

	fScreenWidth = kScreenWidth;
	fScreenHeight = kScreenHeight;
	fOrientation = kPortraitFlip;
	fContrast = 0;
	fContrast2 = 0;
	fBacklight = false;
	getScreenInfo(&info);

	int	rowBytes = ALIGN(fScreenWidth, 8) / 8 * kScreenDepth;	// r6

#if defined(correct)
	CScreenMemory * r7 = (CScreenMemory *)MakeByName("CScreenMemory", "CReservedContiguousMemory");

	CUPhys * p = new CUPhys;
	f44 = p;
	r7->fBTable[5](p);

	f48 = new CLicenseeVAddress(0);	// isa CUPhys
	PAddr		addr;
	size_t	size;
	f44->base(addr);
	f44->size(size);
	f48->init(addr, size, false, false);
	f48->map(false, kReadWrite);

	fPixMap.baseAddr = f48->f10;
#else
	fPixMap.baseAddr = NewPtr(rowBytes * fScreenHeight);  // just to get things going
#endif

	fPixMap.rowBytes = rowBytes;
	fPixMap.bounds.top = 0;
	fPixMap.bounds.left = 0;
	fPixMap.bounds.bottom = fScreenHeight;
	fPixMap.bounds.right = fScreenWidth;
	fPixMap.pixMapFlags = kPixMapPtr + kScreenDepth;
	fPixMap.deviceRes.h = info.resolution.h;
	fPixMap.deviceRes.v = info.resolution.v;
	fPixMap.grayTable = NULL;

	memset(fPixMap.baseAddr, 0xA5, fScreenWidth * fScreenHeight * kScreenDepth / 8);

#if !defined(forFramework)
	// register gestalt information for screen contrast and backlight controls
	CUGestalt gestalt;
	GestaltSoftContrast * contrast = new GestaltSoftContrast;
	if (contrast)
	{
		contrast->fHasSoftContrast = true;
		contrast->fMinContrast = -16;
		contrast->fMaxContrast =  16;
		gestalt.registerGestalt(kGestalt_SoftContrast, contrast, sizeof(GestaltSoftContrast));
	}

	GestaltBacklight * backlight = new GestaltBacklight;
	if (backlight)
	{
		backlight->fHasBacklight = true;
		gestalt.registerGestalt(kGestalt_Ext_Backlight, backlight, sizeof(GestaltBacklight));
	}
#endif
}


void
CMainDisplayDriver::getScreenInfo(ScreenInfo * outInfo)
{
	if (fOrientation == kLandscape || fOrientation == kLandscapeFlip)
	{
		outInfo->width = kScreenWidth;
		outInfo->height = kScreenHeight;
	}
	else
	{
		outInfo->height = kScreenWidth;
		outInfo->width = kScreenHeight;
	}
	outInfo->depth = kScreenDepth;
	outInfo->f0C = 55;
	outInfo->f14 =
	outInfo->f18 = 32;
	outInfo->resolution.h =
	outInfo->resolution.v = kScreenDPI;
}


void
CMainDisplayDriver::powerInit(void)
{	/* this really does nothing */	}


void
CMainDisplayDriver::powerOn(void)
{
/*
	IOPowerOn(33);

	g0F141000 = 0x00000800;
	g0F140000 = 0x0004FDDF;
	g0F140400 = 0x00000000;
	g0F140C00 = 0x0000003B;
	g0F142400 = 0x0000018A;
	g0F142800 = 0x00000001;
	g0F141800 = 0x0000000D;
	g0F140800 = 0x00000001;
	g0F141C00 = 0x00000003;
//	INCOMPLETE	
*/
}


void
CMainDisplayDriver::powerOff(void)
{
//	INCOMPLETE	
}


/*------------------------------------------------------------------------------
	Blit pixmap image onto the screen.
	Optimized slightly for landscape orientation since this is how the
	hardware is organized.
	Args:		inPixmap				the image
				inSrcBounds			part of the image to use
				inDstBounds			part of the screen to update
				inTransferMode		blend effect to use
	Return:	--
------------------------------------------------------------------------------*/

static const unsigned char colorTable1bit[] = { 0xFF, 0x00 };
static const unsigned char colorTable2bit[] = { 0xFF, 0xAA, 0x55, 0x00 };
static const unsigned char colorTable4bit[] = { 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88,
																0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00 };
static const unsigned char * colorTable[5] = { NULL, colorTable1bit, colorTable2bit, NULL, colorTable4bit };

void
CMainDisplayDriver::blit(NativePixelMap * inPixmap, Rect * inSrcBounds, Rect * inDstBounds, int inTransferMode)	// 007A5EA4
{
	size_t	pixmapWidth = RectGetWidth(inPixmap->bounds);
	size_t	pixmapHeight = RectGetHeight(inPixmap->bounds);
	size_t	pixmapDepth = PixelDepth(inPixmap);

	CGImageRef			image = NULL;
	CGColorSpaceRef	baseColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericGray);
	CGColorSpaceRef	colorSpace = CGColorSpaceCreateIndexed(baseColorSpace, (1 << pixmapDepth)-1, colorTable[pixmapDepth]);
	CGDataProviderRef source = CGDataProviderCreateWithData(NULL, PixelMapBits(inPixmap), pixmapHeight * inPixmap->rowBytes, NULL);
	if (source != NULL) {
		image = CGImageCreate(pixmapWidth, pixmapHeight, pixmapDepth, pixmapDepth, inPixmap->rowBytes,
									colorSpace, kCGImageAlphaNone, source, NULL, false, kCGRenderingIntentDefault);
		CGDataProviderRelease(source);
	}
	CGColorSpaceRelease(colorSpace);
	CGColorSpaceRelease(baseColorSpace);

printf("CMainDisplayDriver::blit(pixmap={w:%lu,h:%lu}, srcBounds={t:%d,l:%d,b:%d,r:%d}, dstBounds={t:%d,l:%d,b:%d,r:%d}, mode=%d) -- ", pixmapWidth,pixmapHeight, inSrcBounds->top,inSrcBounds->left,inSrcBounds->bottom,inSrcBounds->right, inDstBounds->top,inDstBounds->left,inDstBounds->bottom,inDstBounds->right, inTransferMode);
	if (image) {
		CGRect imageRect = MakeCGRect(*inDstBounds);
		if (inTransferMode == 0) {
			CGContextDrawImage(quartz, imageRect, image);
		} else {
			// we should mask the image
			const CGFloat maskingColors[2] = {0,0};
			CGImageRef colorMaskedImage = CGImageCreateWithMaskingColors(image, maskingColors);
			CGContextDrawImage(quartz, imageRect, colorMaskedImage);
			CGImageRelease(colorMaskedImage);
		}
		CGImageRelease(image);
	} else {
printf("image=NULL -- ");
	}


#if defined(correct)
	BlitRec	blitParms;

//	g20000000 = fContrast + fContrast2 + 0x66;

	if (fOrientation == kLandscape || fOrientation == kLandscapeFlip)
	{
		// byte-aligned leftmost pixel in image
		int	srcLeft = (inSrcBounds->left - inPixmap->bounds.left) & ~0x07;
		// byte-aligned rightmost pixel in image
		int	srcRight = (inSrcBounds->right - inPixmap->bounds.left + 0x07) & ~0x07;
		// number of bytes in row to be transferred
		int	byteWd = (srcRight - srcLeft) * kScreenDepth / 8;
		// offset to first byte in image
		int	srcOffset = inPixmap->rowBytes * (inSrcBounds->top - inPixmap->bounds.top) + srcLeft * kScreenDepth / 8;
		// offset to first byte on screen
		int	dstOffset = fPixMap.rowBytes * inDstBounds->top + (inDstBounds->left & ~0x07) * kScreenDepth / 8;
		if (fOrientation != kLandscape)		// ie flipped
			dstOffset = 76796 - dstOffset; 	// 480*320*4/8-4
 
		blitParms.srcRowOffset = inPixmap->rowBytes - byteWd;
		blitParms.dstRowOffset = fPixMap.rowBytes - byteWd;
		blitParms.srcAddr = PixelMapBits(inPixmap) + srcOffset;
		blitParms.dstAddr = PixelMapBits(&fPixMap) + dstOffset;
		blitParms.mode = inTransferMode;
		blitParms.numOfRows = inDstBounds->bottom - inDstBounds->top;
		blitParms.numOfBytes = byteWd;
	}

	switch (fOrientation)
	{
	case kPortrait:
		blitPortrait(inPixmap, inSrcBounds, inDstBounds, inTransferMode, inTransferMode);
		break;
	case kLandscape:
		blitLandscape(&blitParms);
		break;
	case kPortraitFlip:
		blitPortraitFlip(inPixmap, inSrcBounds, inDstBounds, inTransferMode, inTransferMode);
		break;
	case kLandscapeFlip:
		blitLandscapeFlip(&blitParms);
		break;
	}
#endif
}


void
CMainDisplayDriver::blitLandscape(BlitRec * inBlit)	// 007A6070
{
	register int * srcPtr = (int *)inBlit->srcAddr;
	register int * dstPtr = (int *)inBlit->dstAddr;
	register int mode = inBlit->mode;
	register ArrayIndex numOfLongs = inBlit->numOfBytes / 4;	// do 32 bits at a time

	for (ArrayIndex row = inBlit->numOfRows; row > 0; row--)
	{
		if (mode == modeCopy)
			for (ArrayIndex i = numOfLongs; i > 0; i--)
				*dstPtr++ = *srcPtr++;
		else /*srcOr*/
			for (ArrayIndex i = numOfLongs; i > 0; i--)
				*dstPtr++ |= *srcPtr++;
	}
}


void
CMainDisplayDriver::blitLandscapeFlip(BlitRec * inBlit)	// 007A60E8
{
	register int * srcPtr = (int *)inBlit->srcAddr;
	register int * dstPtr = (int *)inBlit->dstAddr;
	register int mode = inBlit->mode;
	register ArrayIndex numOfLongs = inBlit->numOfBytes / 4;
	register ULong src, flipper;

	for (ArrayIndex row = inBlit->numOfRows; row > 0; row--)
	{
		if (mode == modeCopy)
			for (ArrayIndex i = numOfLongs; i > 0; i--)
			{
				src = *srcPtr++;
				flipper =   src         << 28;
				src >>= 4;
				flipper |= (src & 0x0F) << 24;
				src >>= 4;
				flipper |= (src & 0x0F) << 20;
				src >>= 4;
				flipper |= (src & 0x0F) << 16;
				src >>= 4;
				flipper |= (src & 0x0F) << 12;
				src >>= 4;
				flipper |= (src & 0x0F) <<  8;
				src >>= 4;
				flipper |= (src & 0x0F) <<  4;
				src >>= 4;
				flipper |= (src & 0x0F);
				*dstPtr-- = flipper;
			}
		else /*srcOr*/
			for (ArrayIndex i = numOfLongs; i > 0; i--)
			{
				src = *srcPtr++;
				flipper =   src         << 28;
				src >>= 4;
				flipper |= (src & 0x0F) << 24;
				src >>= 4;
				flipper |= (src & 0x0F) << 20;
				src >>= 4;
				flipper |= (src & 0x0F) << 16;
				src >>= 4;
				flipper |= (src & 0x0F) << 12;
				src >>= 4;
				flipper |= (src & 0x0F) <<  8;
				src >>= 4;
				flipper |= (src & 0x0F) <<  4;
				src >>= 4;
				flipper |= (src & 0x0F);
				*dstPtr-- |= flipper;
			}
	}
}


void
CMainDisplayDriver::blitPortrait(NativePixelMap * inPixmap, Rect * inSrcBounds, Rect * inDstBounds, int inTransferMode, int inMode)
{
return;	// don’t risk this yet
// r9: r4..
//sp-24
	int sp04 = fScreenWidth * kScreenDepth / 8;
	int sp00 = sp04 / 4;
	
	// byte-aligned leftmost pixel in image
	int	r6 = (inSrcBounds->left - inPixmap->bounds.left) & ~0x07;			//srcLeft?
	// byte-aligned rightmost pixel in image
	int	r7 = (inSrcBounds->right - inPixmap->bounds.left + 0x07) & ~0x07;	//srcRight?

	int r5 = (inSrcBounds->top - inPixmap->bounds.top) & ~0x07;					// srcTop?
	int r8 = (inSrcBounds->bottom - inPixmap->bounds.top + 7) & ~0x07;		// srcBottom?

	int r10 = (inDstBounds->left & ~0x07);
	int sp20 = (inDstBounds->top & ~0x07);

	//sp-04
	int r1 = r5 * inPixmap->rowBytes;
	int spm04 = r1 + (r6/8)*4;

	Ptr sp0C = PixelMapBits(inPixmap) + spm04;
	int r4 = inPixmap->rowBytes / 4;
	r1 = sp04 * r10;
	r10 = r1 + (fScreenWidth - sp20 - 1) / 8 * 4;

	Ptr sp08 = PixelMapBits(&fPixMap) + r10;
	r1 = (r8 - r5)/8;
	int sp1C = (r7 - r6)/8;
	int sp14 = r1;
	int sp18 = (r4 * 8) - sp1C;
	for (int sp14 = r1; sp14 != 0; sp14--)
	{
		for (int sp10 = sp1C; sp10 != 0; sp10--)
		{
			int * r1 = (int *)sp0C++;	// assuming (int *)sp0C
			int r7 = *r1; r1 += r4;
			int r6 = *r1; r1 += r4;
			int r5 = *r1; r1 += r4;
			int lk = *r1; r1 += r4;
			int r12 = *r1; r1 += r4;
			int r3 = *r1; r1 += r4;
			int r2 = *r1; r1 += r4;
			int r1x = *r1;
			for (ArrayIndex r8 = 8; r8 != 0; r8--)
			{
				int r9 = r7 >> 28; r7 <<= 4;
				r9 |= (r6 & 0xF0000000) >> 24; r6 <<= 4;
				r9 |= (r5 & 0xF0000000) >> 20; r5 <<= 4;
				r9 |= (lk & 0xF0000000) >> 16; lk <<= 4;
				r9 |= (r12 & 0xF0000000) >> 12; r12 <<= 4;
				r9 |= (r3 & 0xF0000000) >> 8; r3 <<= 4;
				r9 |= (r2 & 0xF0000000) >> 4; r2 <<= 4;
				r9 |= (r1x & 0xF0000000); r1x <<= 4;
				if (inTransferMode != 0)
					r9 |= *sp08;
				*sp08 = r9;
				sp08 += sp00;	// assuming int*
			}
		}
		sp0C += sp18 * 4;
		sp08 -= 4;
	}
}


void
CMainDisplayDriver::blitPortraitFlip(NativePixelMap * inPixmap, Rect * inSrcBounds, Rect * inDstBounds, int inTransferMode, int inMode)
{
}


void
CMainDisplayDriver::doubleBlit(NativePixelMap * inArg1, NativePixelMap * inPixmap, Rect * inSrcBounds, Rect * inDstBounds, int inTransferMode)
{	/* this really does nothing */	}


int
CMainDisplayDriver::getFeature(int inSelector)	// 01E00030
{
	switch (inSelector)
	{
	case 0:	// GetGrafInfo 3 - contrast
		return fContrast;
		break;
	case 1:	// more contrast
		return 1;
		break;
	case 2:	// GetGrafInfo 5 - backlight
		return fBacklight;
		break;
	case 3:
		return 0;
		break;
	case 4:	// GetGrafInfo 4 - orientation
		return fOrientation;
		break;
	case 5:	// GetGrafInfo 6
		return 10;
		break;
	}
	return -1;
}


void
CMainDisplayDriver::setFeature(int inSelector, int inValue)
{
/*	patch dodgy contrast
	if (inSelector == 0)
	{
		fContrast = inValue;
		long		r6 = 0
		RefVar	r5(GetPreference(MakeSymbol("UniC")));
		if (NOTNIL(r5))
			r6 = ISINT(r5) ? RVALUE(r5) : CCommToolProtocol::CTGetRequest(r5);
		g20000000 = fContrast + fContrast2 + r6 + 0x66;
		return;
	}
*/

	switch (inSelector)
	{
	case 0:	// contrast
		fContrast = inValue;	// sic
		fContrast = MINMAX(-16, inValue, 16);
	//	g20000000 = fContrast + fContrast2 + 0x66;
		break;

	case 1:	// more contrast
		{
			inValue = MINMAX(-20, inValue, 75);
		//	Fixed	dividend = FixedConst(2,6553) - FixedMultiply(FixedConst(0,5825), IntToFixed(inValue));
		//	fContrast2 = FixedTrunc(FixedDivide(dividend, FixedConst(0,2867)));
			fContrast2 = (2.6553 - (0.5825 * (float)inValue)) / 0.2867;
		}
		break;

	case 2:	// backlight
		if ((inValue == true || inValue == false) && inValue != fBacklight)
		{
			fBacklight = inValue;
/*			if (fBacklight)
				IOPowerOn(9);
			else
				IOPowerOff(9);
			bool	huh;
			GetGPIOInterfaceObject()->writeGPIOData(12, fBacklight, &huh);
*/		}
		break;

//	case 3:		fixed - can’t set it

	case 4:	// orientation
		fOrientation = inValue;
		break;

//	case 5:		fixed - can’t set it
	}
}


int
CMainDisplayDriver::autoAdjustFeatures(void)
{	/* this really does nothing */ return 0;	}


void
CMainDisplayDriver::enterIdleMode(void)
{	/* this really does nothing */	}

void
CMainDisplayDriver::exitIdleMode(void)
{	/* this really does nothing */	}

