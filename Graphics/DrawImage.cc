/*
	File:		DrawImage.cc

	Contains:	Picture and bitmap creation and drawing functions.

	Written by:	Newton Research Group, 2007.
*/

#include "Quartz.h"
#include "QDPatterns.h"
#include "DrawShape.h"

#include "Objects.h"
#include "Iterators.h"
#include "ROMSymbols.h"
#include "OSErrors.h"

extern "C" OSErr	PtrToHand(const void * srcPtr, Handle * dstHndl, long size);

extern int gScreenHeight;
/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern "C" {
Ref		FPictureBounds(RefArg inRcvr, RefArg inPicture);
Ref		FCopyBits(RefArg inRcvr, RefArg inImage, RefArg inX, RefArg inY, RefArg inTransferMode);
Ref		FDrawXBitmap(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3);
Ref		FGrayShrink(RefArg inRcvr, RefArg inArg1, RefArg inArg2);
Ref		FDrawIntoBitmap(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3);
Ref		FViewIntoBitmap(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3);
}

Rect *	PictureBounds(const unsigned char * inData, Rect * outRect);
void		DrawPicture(char * inPicture, size_t inPictureSize, const Rect * inRect);


/*------------------------------------------------------------------------------
	Draw a picture.
	Determine what kind of data is inIcon -
		PNG data
		Newton bitmap
		Newton shape
	Args:		inIcon
				inFrame
				inJustify
	Return:	--
------------------------------------------------------------------------------*/

void
DrawPicture(RefArg inIcon, const Rect * inFrame, ULong inJustify /*, long inTransferMode*/)
{
	Rect  iconRect;

	if (IsBinary(inIcon) && EQRef(ClassOf(inIcon), RSYMpicture))
	{
		CDataPtr	iconData(inIcon);
		DrawPicture(iconData, Length(inIcon), Justify(PictureBounds(iconData, &iconRect), inFrame, inJustify));
	}

	else if (IsFrame(inIcon) && !IsInstance(inIcon, RSYMbitmap)
			&& (FrameHasSlot(inIcon, SYMA(bits)) || FrameHasSlot(inIcon, SYMA(colorData))))
	{
		RefVar	iconBounds(GetFrameSlot(inIcon, SYMA(bounds)));
		if (!FromObject(iconBounds, &iconRect))
			ThrowMsg("bad pictBounds frame");
		DrawBitmap(inIcon, Justify(&iconRect, inFrame, inJustify));
	}

	else
	{
		if (inJustify)
			Justify(ShapeBounds(inIcon, &iconRect), inFrame, inJustify);
		else
			iconRect = *inFrame;
		RefVar	style(AllocateFrame());
	//	RefVar	mode(MAKEINT(inTransferMode));
	//	SetFrameSlot(style, SYMA(transferMode), mode);
		DrawShape(inIcon, style, *(Point *)&iconRect);
	}
}


/*------------------------------------------------------------------------------
	Draw a PNG picture.
	Pictures are always PNGs in Newton 3.0.
	NOTE	Might be able to use the QuickTime function GraphicsImportCreateCGImage
			declared in ImageCompression.h to draw PICTs.
			http://developer.apple.com/technotes/tn/tn1195.html
	Args:		inPicture
				inPictureSize
				inRect
	Return:	--
------------------------------------------------------------------------------*/
//#include <QuickTime/QuickTime.h>
// QuickTime is not available to 64-bit clients

void
DrawPicture(char * inPicture, size_t inPictureSize, const Rect * inRect)
{
#if defined(forFramework) && 0 // !defined(forNTK)
	// draw legacy PICTs
	Handle						dataRef;
	ComponentInstance			dataRefHandler = NULL;
	GraphicsImportComponent	gi;
	OSErr							err;
	ComponentResult			result;
	CGImageRef					image;

	// prefix PICT data w/ 512 byte header
	size_t pictSize = 512 + inPictureSize;
	Ptr pictPtr = NewPtr(pictSize);		// where is the FreePtr()?
	memmove(pictPtr + 512, inPicture, inPictureSize);

	// create dataRef from pointer
	PointerDataRefRecord		dataRec;
	dataRec.data = pictPtr;
	dataRec.dataLength = pictSize;
	err = PtrToHand(&dataRec, &dataRef, sizeof(PointerDataRefRecord));

	// create data handler
	err = OpenADataHandler(
			dataRef,							/* data reference */
			PointerDataHandlerSubType,	/* data ref. type */
			NULL,								/* anchor data ref. */
			(OSType)0,						/* anchor data ref. type */
			NULL,								/* time base for data handler */
			kDataHCanRead,					/* flag for data handler usage */
			&dataRefHandler);				/* returns the data handler */

	// set data ref extensions to describe the data
	Handle						extension;
	// set the data ref extension for no filename
	unsigned char				filename = 0;
	err = PtrToHand(&filename, &extension, sizeof(filename));
	err = DataHSetDataRefExtension(dataRefHandler, extension, kDataRefExtensionFileName);
	FreeHandle(extension);

	// set the data ref extension for the data handler
	OSType						picType = EndianU32_NtoB('PICT');
	err = PtrToHand(&picType, &extension, sizeof(picType));
	err = DataHSetDataRefExtension(dataRefHandler, extension, kDataRefExtensionMacOSFileType);
	FreeHandle(extension);

	// dispose old data ref handle because it does not contain our new changes
	FreeHandle(dataRef);
	dataRef = NULL;
	// re-acquire data reference from the data handler to get the new changes
	err = DataHGetDataRef(dataRefHandler, &dataRef);
	CloseComponent(dataRefHandler);

	err = GetGraphicsImporterForDataRef(dataRef, PointerDataHandlerSubType, &gi);
	result = GraphicsImportCreateCGImage(gi, &image, kGraphicsImportCreateCGImageUsingCurrentSettings);
	FreeHandle(dataRef);
	CloseComponent(gi);
#else
	// draw PNG
	CGDataProviderRef source = CGDataProviderCreateWithData(NULL, inPicture, inPictureSize, NULL);
	CGImageRef  image = CGImageCreateWithPNGDataProvider(source, NULL, true, kCGRenderingIntentDefault);
#endif
	CGContextDrawImage(quartz, MakeCGRect(*inRect), image);
	CGImageRelease(image);
}


// Read image size from PNG data.

typedef struct
{
	ULong		length;
	char		id[4];
	ULong		width;
	ULong		height;
	UChar		bitDepth;
	UChar		colorType;
	UChar		compressionMethod;
	UChar		filterMethod;
	UChar		interlaceMethod;
} PNGchunkIHDR;

ULong
PNGgetULong(const unsigned char * inData)
{
   return	((ULong)(*inData) << 24)
			 +	((ULong)(*(inData + 1)) << 16)
			 +	((ULong)(*(inData + 2)) << 8)
			 +	(ULong)(*(inData + 3));
}

Rect *
PictureBounds(const unsigned char * inData, Rect * outRect)
{
	if (memcmp(inData, "\x89PNG", 4) != 0)
		ThrowMsg("picture is not PNG");

	ULong	chunkLength;
	for (inData += 8; (chunkLength = PNGgetULong(inData)) != 0; inData += chunkLength)
	{
		if (memcmp(inData+4, "IHDR", 4) == 0)
		{
			if (chunkLength != 13)
				ThrowMsg("invalid PNG IHDR chunk");

			outRect->left = 0;
			outRect->top = 0;
			outRect->right = PNGgetULong(inData+8);
			outRect->bottom = PNGgetULong(inData+12);
			break;
		}
	}

	return outRect;
}


Ref
FPictureBounds(RefArg inRcvr, RefArg inPicture)
{
	Rect		bounds;
	CDataPtr	picData(inPicture);
	return ToObject(PictureBounds(picData, &bounds));
}


#pragma mark -
/*------------------------------------------------------------------------------
	C P i x e l O b j

	A class to convert 1-bit bitmaps to pixel maps.
------------------------------------------------------------------------------*/

/*	In a 64-bit world, the FramBitmap will no longer align with its 32-bit declaration.
	Since the FramBitmap is only built by NTK we define the baseAddr as 32-bits, not a pointer.
*/
struct FramBitmap
{
	uint32_t	baseAddr;		// +00
	short		rowBytes;		// +04
	short		reserved1;		// pads to long
	Rect		bounds;			// +08
	char		data[];			// +10
};


class CPixelObj
{
public:
						CPixelObj();
						~CPixelObj();

	void				init(RefArg inBitmap);
	void				init(RefArg inBitmap, bool);

	PixelMap *		pixMap(void)	const;
	PixelMap *		mask(void)		const;
	int				bitDepth(void)	const;

private:
	Ref				getFramBitmap(void);
	PixelMap *		framBitmapToPixMap(const FramBitmap * inFramBits);
	PixelMap *		framMaskToPixMap(const FramBitmap * inFramBits);

	RefVar		fBitmapRef; // +00
	PixelMap		fPixmap;		// +04
	PixelMap *	fPixPtr;		// +20
	PixelMap		fMask;
	PixelMap *	fMaskPtr;	// +24
	UChar *		fColorTable;// +28
	int			fBitDepth;  // +2C
	bool			fIsLocked;  // +30
};

inline	PixelMap *		CPixelObj::pixMap(void)		const		{ return fPixPtr; }
inline	PixelMap *		CPixelObj::mask(void)		const		{ return fMaskPtr; }
inline	int				CPixelObj::bitDepth(void)	const		{ return fBitDepth; }


CPixelObj::CPixelObj()
{
	fBitDepth = kOneBitDepth;
	fColorTable = NULL;
	fIsLocked = NO;
	fPixPtr = &fPixmap;
}


CPixelObj::~CPixelObj()
{
	if (fColorTable)
		FreePtr((Ptr)fColorTable);
	if (fIsLocked)
		UnlockRef(fBitmapRef);
}


void
CPixelObj::init(RefArg inBitmap)
{
	fBitmapRef = inBitmap;
	if (IsInstance(inBitmap, RSYMpicture))
		ThrowErr(exGraf, -8803);

	if (IsFrame(inBitmap))
	{
		if (IsInstance(inBitmap, RSYMbitmap))
		{
			RefVar	colorData(GetFrameSlot(inBitmap, SYMA(colorData)));
			if (ISNIL(colorData))
				fBitmapRef = GetFrameSlot(inBitmap, SYMA(data));
			else
				fBitmapRef = getFramBitmap();
		}
		else
			fBitmapRef = getFramBitmap();
	}

	LockRef(fBitmapRef);
	fIsLocked = YES;
	if (IsInstance(fBitmapRef, RSYMpixels))
	{
		fPixPtr = (PixelMap *)BinaryData(fBitmapRef);
		fBitDepth = PixelDepth(fPixPtr);
	}
	else
		fPixPtr = framBitmapToPixMap((const FramBitmap *)BinaryData(fBitmapRef));

	// we don’t mask by drawing with srcBic transferMode any more
	RefVar	theMask = GetFrameSlot(inBitmap, SYMA(mask));
	if (NOTNIL(theMask))
	{
		CDataPtr  maskData(theMask);
		fMaskPtr = framMaskToPixMap((const FramBitmap *)(char *)maskData);
	}
	else
		fMaskPtr = NULL;
}


void
CPixelObj::init(RefArg inBitmap, bool inHuh)
{
	fMaskPtr = NULL;
	fBitmapRef = inBitmap;
	if (IsInstance(inBitmap, RSYMpicture))
		ThrowErr(exGraf, -8803);

	bool  isPixelData = NO;
	RefVar	bits(GetFrameSlot(inBitmap, SYMA(data)));
	if (NOTNIL(bits))
		isPixelData = IsInstance(bits, RSYMpixels);

	LockRef(fBitmapRef);
	fIsLocked = YES;
	if (isPixelData)
	{
		fPixPtr = (PixelMap *)BinaryData(fBitmapRef);
	}
	else
	{
		bits = GetFrameSlot(inBitmap, SYMA(mask));
		if (NOTNIL(bits))
		{
			CDataPtr  obj(bits);
			fMaskPtr = framBitmapToPixMap((const FramBitmap *)(char *)obj);
		}
		if (inHuh || fMaskPtr == NULL)
			fPixPtr = framBitmapToPixMap((const FramBitmap *)BinaryData(getFramBitmap()));
	}
}


Ref
CPixelObj::getFramBitmap(void)
{
	RefVar	framBitmap;
	RefVar	colorTable;
	RefVar	colorData(GetFrameSlot(fBitmapRef, SYMA(colorData)));
	if (ISNIL(colorData))
		framBitmap = GetFrameSlot(fBitmapRef, SYMA(bits));
	else
	{
#if 0
		GrafPtr  thePort;
		GetPort(&thePort);
		int	portDepth = PixelDepth(&thePort->portBits);
#endif
		int	portDepth = 8; // for CG grayscale
		if (IsArray(colorData))
		{
		// find colorData that matches display most closely
			int	depth;
			int	maxDepth = 32767; // improbably large number
			int	minDepth = 0;
			CObjectIterator	iter(colorData);
			for ( ; !iter.done(); iter.next())
			{
				depth = RINT(GetFrameSlot(iter.value(), SYMA(bitDepth)));
				if (depth == portDepth)
				{
					fBitDepth = depth;
					framBitmap = GetFrameSlot(iter.value(), SYMA(cBits));
					colorTable = GetFrameSlot(iter.value(), SYMA(colorTable));
				}
				else if ((portDepth < depth && depth < maxDepth && (maxDepth = depth, 1))
						|| (maxDepth == 32767 && portDepth > depth && depth > minDepth && (minDepth = depth, 1)))
				{
					fBitDepth = depth;
					framBitmap = GetFrameSlot(iter.value(), SYMA(cBits));
					colorTable = GetFrameSlot(iter.value(), SYMA(colorTable));
				}
			}
		}
		else
		{
			fBitDepth = RINT(GetFrameSlot(colorData, SYMA(bitDepth)));
			framBitmap = GetFrameSlot(colorData, SYMA(cBits));
			colorTable = GetFrameSlot(colorData, SYMA(colorTable));
		}

		if (fBitDepth > 1 && NOTNIL(colorTable))
		{
			CDataPtr	colorTableData(colorTable);
			size_t	numOfColors = Length(colorTable);
			size_t	colorTableSize = 1 << fBitDepth;
			fColorTable = (UChar *)NewPtr(colorTableSize);
			if (fColorTable)
			{
				unsigned int i;
				numOfColors /= 8;
				if (numOfColors > colorTableSize)
					numOfColors = colorTableSize;
#if 0
				Ptr defaultColorPtr = GetPixelMapBits(*GetFgPattern());
				for (i = 0; i < colorTableSize; ++i)
					fColorTable[i] = *defaultColorPtr;
#endif
				ULong		r, g, b;
				short *	colorTablePtr = (short *)(char *)colorTableData;
				for (i = 0; i < numOfColors; ++i)
				{
					r = *colorTablePtr++;
					g = *colorTablePtr++;
					b = *colorTablePtr++;
					colorTablePtr++;
					fColorTable[i] = RGBtoGray(r,g,b, fBitDepth, fBitDepth);
				}
			}
		}
	}

	return framBitmap;
}


PixelMap *
CPixelObj::framBitmapToPixMap(const FramBitmap * inBitmap)
{
	fPixPtr->baseAddr = (Ptr) inBitmap->data;
#if 0	//defined(forFramework)
	fPixPtr->rowBytes = inBitmap->rowBytes;
	fPixPtr->bounds.top = inBitmap->bounds.top;
	fPixPtr->bounds.left = inBitmap->bounds.left;
	fPixPtr->bounds.bottom = inBitmap->bounds.bottom;
	fPixPtr->bounds.right = inBitmap->bounds.right;
#else
	fPixPtr->rowBytes = CANONICAL_SHORT(inBitmap->rowBytes);
	fPixPtr->bounds.top = CANONICAL_SHORT(inBitmap->bounds.top);
	fPixPtr->bounds.left = CANONICAL_SHORT(inBitmap->bounds.left);
	fPixPtr->bounds.bottom = CANONICAL_SHORT(inBitmap->bounds.bottom);
	fPixPtr->bounds.right = CANONICAL_SHORT(inBitmap->bounds.right);
#endif
	fPixPtr->pixMapFlags = kPixMapPtr + fBitDepth;
	fPixPtr->deviceRes.h =
	fPixPtr->deviceRes.v = kDefaultDPI;
	fPixPtr->grayTable = fColorTable;
	if (fColorTable)
		fPixPtr->pixMapFlags |= kPixMapGrayTable;

	return fPixPtr;
}


PixelMap *
CPixelObj::framMaskToPixMap(const FramBitmap * inBitmap)
{
	fMask.baseAddr = (Ptr) inBitmap->data;
#if 0 //defined(forFramework)
	fMask.rowBytes = inBitmap->rowBytes;
	fMask.bounds.top = inBitmap->bounds.top;
	fMask.bounds.left = inBitmap->bounds.left;
	fMask.bounds.bottom = inBitmap->bounds.bottom;
	fMask.bounds.right = inBitmap->bounds.right;
#else
	fMask.rowBytes = CANONICAL_SHORT(inBitmap->rowBytes);
	fMask.bounds.top = CANONICAL_SHORT(inBitmap->bounds.top);
	fMask.bounds.left = CANONICAL_SHORT(inBitmap->bounds.left);
	fMask.bounds.bottom = CANONICAL_SHORT(inBitmap->bounds.bottom);
	fMask.bounds.right = CANONICAL_SHORT(inBitmap->bounds.right);
#endif
	fMask.pixMapFlags = kPixMapPtr + 1;		// always 1 bit deep ??
	fMask.deviceRes.h =
	fMask.deviceRes.v = kDefaultDPI;
	fMask.grayTable = NULL;

	return &fMask;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Draw a bitmap.
	Args:		inBitmap
				inRect
	Return:	--
------------------------------------------------------------------------------*/

static const unsigned char colorTable1bit[] = { 0xFF, 0x00 };
static const unsigned char colorTable2bit[] = { 0xFF, 0xAA, 0x55, 0x00 };
static const unsigned char colorTable4bit[] = { 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88,
																0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00 };
static const unsigned char * colorTable[5] = { NULL, colorTable1bit, colorTable2bit, NULL, colorTable4bit };

void
DrawBitmap(RefArg inBitmap, const Rect * inRect)
{
	CPixelObj	pix;
	newton_try
	{
		pix.init(inBitmap);

		size_t	pixmapWidth = RectGetWidth(pix.pixMap()->bounds);
		size_t	pixmapHeight = RectGetHeight(pix.pixMap()->bounds);

		CGRect	imageRect = MakeCGRect(*inRect);
		if (RectGetWidth(*inRect) == 0)
		{
			imageRect.size.width = pixmapWidth;
			imageRect.size.height = pixmapHeight;
		}

		CGImageRef			image = NULL;
		CGColorSpaceRef	baseColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericGray);
		CGColorSpaceRef	colorSpace = CGColorSpaceCreateIndexed(baseColorSpace, (1 << pix.bitDepth())-1, colorTable[pix.bitDepth()]);
		CGDataProviderRef source = CGDataProviderCreateWithData(NULL, GetPixelMapBits(pix.pixMap()), pixmapHeight * pix.pixMap()->rowBytes, NULL);
		CGDataProviderRef maskSource;
		if (source)
		{
			image = CGImageCreate(pixmapWidth, pixmapHeight, pix.bitDepth(), pix.bitDepth(), pix.pixMap()->rowBytes,
										colorSpace, kCGImageAlphaNone, source,
										NULL, false, kCGRenderingIntentSaturation);
			CGDataProviderRelease(source);
/*
			if (pix.mask() != NULL
			&&  (maskSource = CGDataProviderCreateWithData(NULL, GetPixelMapBits(pix.mask()), pixmapHeight * pix.mask()->rowBytes, NULL)) != NULL)
			{
				CGImageRef	fullImage = image;
				CGColorSpaceRef maskColorSpace = CGColorSpaceCreateDeviceGray();
				CGImageRef	mask = CGImageCreate(pixmapWidth, pixmapHeight, 1, 1, pix.mask()->rowBytes,
															maskColorSpace, kCGImageAlphaNone, maskSource,
															NULL, false, kCGRenderingIntentDefault);
				CGColorSpaceRelease(maskColorSpace);
				CGDataProviderRelease(maskSource);
				if (mask)
				{
					image = CGImageCreateWithMask(fullImage, mask);
					CGImageRelease(fullImage);
					CGImageRelease(mask);
				}
			}
*/		}
		CGColorSpaceRelease(colorSpace);
		CGColorSpaceRelease(baseColorSpace);

		if (image)
		{
			CGContextDrawImage(quartz, imageRect, image);
			CGImageRelease(image);
		}
	}
	newton_catch_all
	{
		pix.~CPixelObj();
	}
	end_try;
}



/*------------------------------------------------------------------------------
	Draw a bitmap.
	Not part of the original Newton suite, but required by NCX to draw
	screenshots.
	Args:		inBits			bitmap data
				inHeight			height of bitmap
				inWidth			width of bitmap
				inRowBytes		row offset
				inDepth			number of bits per pixel
	Return:	--
------------------------------------------------------------------------------*/

void
DrawBits(const char * inBits, unsigned inHeight, unsigned inWidth, unsigned inRowBytes, unsigned inDepth)
{
	newton_try
	{
		CGImageRef			image = NULL;
		CGColorSpaceRef	baseColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericGray);
		CGColorSpaceRef	colorSpace = CGColorSpaceCreateIndexed(baseColorSpace, (1 << inDepth)-1, colorTable[inDepth]);
		CGDataProviderRef source = CGDataProviderCreateWithData(NULL, inBits, inHeight * inRowBytes, NULL);
		if (source)
		{
			image = CGImageCreate(inWidth, inHeight, inDepth, inDepth, inRowBytes,
										colorSpace, kCGImageAlphaNone, source,
										NULL, false, kCGRenderingIntentDefault);
			CGDataProviderRelease(source);
		}
		CGColorSpaceRelease(colorSpace);
		CGColorSpaceRelease(baseColorSpace);

		if (image)
		{
			CGRect	imageRect = CGRectMake(0.0, 0.0, inWidth, inHeight);
			CGContextDrawImage(quartz, imageRect, image);
			CGImageRelease(image);
		}
	}
	newton_catch_all
	{ }
	end_try;
}


/*------------------------------------------------------------------------------
	The dreaded CopyBits.
	Args:		inImage
				inX
				inY
				inTransferMode
	Return:	NILREF
------------------------------------------------------------------------------*/
#include "View.h"
#if !defined(forFramework)
extern CView *	FailGetView(RefArg inContext);
#endif

void
ToGlobalCoordinates(RefArg inView, short * outLeft, short * outTop, short * outRight, short * outBottom)
{
#if !defined(forFramework)
	Point globalOrigin = *(Point *)&(FailGetView(inView)->viewBounds);
	if (outLeft)
		*outLeft += globalOrigin.h;
	if (outTop)
		*outTop += globalOrigin.v;
	if (outRight)
		*outRight += globalOrigin.h;
	if (outBottom)
		*outBottom += globalOrigin.v;
#endif
}


Ref
FCopyBits(RefArg inRcvr, RefArg inImage, RefArg inX, RefArg inY, RefArg inTransferMode)
{
	if (NOTINT(inX) || NOTINT(inY))
		ThrowMsg("param not an integer");

	Rect  bounds;
	bounds.left = RINT(inX);
	bounds.top = RINT(inY);
	ToGlobalCoordinates(inRcvr, &bounds.left, &bounds.top, NULL, NULL);

	// make the frame empty - Justify will use the image’s bounds
	bounds.right = bounds.left;
	bounds.bottom = bounds.top;

	// there’s no transfer mode in Newton 3.0

	DrawPicture(inImage, &bounds, vjLeftH + vjTopV);
	return NILREF;
}


Ref		FDrawXBitmap(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3) { return NILREF; }
Ref		FGrayShrink(RefArg inRcvr, RefArg inArg1, RefArg inArg2) { return NILREF; }
Ref		FDrawIntoBitmap(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3) { return NILREF; }
Ref		FViewIntoBitmap(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3) { return NILREF; }

