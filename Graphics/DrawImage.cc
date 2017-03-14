/*
	File:		DrawImage.cc

	Contains:	Picture and bitmap creation and drawing functions.

	Written by:	Newton Research Group, 2007.
*/

#include "Quartz.h"
#include "Objects.h"
#include "QDPatterns.h"
#include "Geometry.h"
#include "DrawShape.h"
#include "ViewFlags.h"

#include "Iterators.h"
#include "RSSymbols.h"
#include "OSErrors.h"

extern "C" OSErr	PtrToHand(const void * srcPtr, Handle * dstHndl, long size);

extern int gScreenHeight;
/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern "C" {
Ref		FPictureBounds(RefArg inRcvr, RefArg inPicture);
Ref		FPictToShape(RefArg inRcvr, RefArg inPicture);
Ref		FCopyBits(RefArg inRcvr, RefArg inImage, RefArg inX, RefArg inY, RefArg inTransferMode);
Ref		FDrawXBitmap(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3);
Ref		FGrayShrink(RefArg inRcvr, RefArg inArg1, RefArg inArg2);
Ref		FDrawIntoBitmap(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3);
Ref		FViewIntoBitmap(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3);
Ref		FGetBitmapPixel(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3);
Ref		FHitShape(RefArg inRcvr, RefArg inShape, RefArg inX, RefArg inY);
Ref		FPtInPicture(RefArg inRcvr, RefArg inX, RefArg inY, RefArg inPicture);
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
				inTransferMode
	Return:	--
------------------------------------------------------------------------------*/

void
DrawPicture(RefArg inIcon, const Rect * inFrame, ULong inJustify, int inTransferMode)
{
	Rect  iconRect;

	if (IsBinary(inIcon) && EQ(ClassOf(inIcon), SYMA(picture)))
	{
		CDataPtr	iconData(inIcon);
//		DrawPicture(iconData, Length(inIcon), Justify(PictureBounds(iconData, &iconRect), inFrame, inJustify));
	}

	else if (IsFrame(inIcon) && !IsInstance(inIcon, SYMA(bitmap))
			&& (FrameHasSlot(inIcon, SYMA(bits)) || FrameHasSlot(inIcon, SYMA(colorData))))
	{
		RefVar	iconBounds(GetFrameSlot(inIcon, SYMA(bounds)));
		if (!FromObject(iconBounds, &iconRect))
			ThrowMsg("bad pictBounds frame");
		Justify(&iconRect, inFrame, inJustify);
		if (inTransferMode == modeMask) {
			RefVar mask(GetFrameSlot(inIcon, SYMA(mask)));
			if (NOTNIL(mask)) {
				DrawBitmap(mask, &iconRect, modeBic);
			}
			DrawBitmap(inIcon, &iconRect, modeOr);
		} else if (inTransferMode < 0) {
			RefVar mask(GetFrameSlot(inIcon, SYMA(mask)));
			DrawBitmap(mask, &iconRect, -inTransferMode);
		} else {
			DrawBitmap(inIcon, &iconRect, inTransferMode);
		}
	}

	else
	{
		if (inJustify)
			Justify(ShapeBounds(inIcon, &iconRect), inFrame, inJustify);
		else
			iconRect = *inFrame;
		RefVar	style(AllocateFrame());
		RefVar	mode(MAKEINT(inTransferMode));
		SetFrameSlot(style, SYMA(transferMode), mode);
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


Ref
FPictToShape(RefArg inRcvr, RefArg inPicture, RefArg inRect)
{
	RefVar shape;
	CDataPtr pictData(inPicture);
	PictureShape * pd = (PictureShape *)(char *)pictData;
	newton_try
	{
		Rect rect;
		if (NOTNIL(inRect))
			FromObject(inRect, &rect);
		else
			memmove(&rect, &pd->bBox, sizeof(Rect));
//		shape = DrawPicture(&pd, &rect, 1);		// original draws a PictureHandle -- need to rework to use PNG?
	}
	cleanup
	{
		pictData.~CDataPtr();
	}
	end_try;
	return shape;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C P i x e l O b j

	A class to convert 1-bit bitmaps to pixel maps.
------------------------------------------------------------------------------*/

class CPixelObj
{
public:
							CPixelObj();
							~CPixelObj();

	void					init(RefArg inBitmap);
	void					init(RefArg inBitmap, bool inUseMask);

	NativePixelMap *	pixMap(void)	const;
	NativePixelMap *	mask(void)		const;
	int					bitDepth(void)	const;

private:
	Ref					getFramBitmap(void);
	NativePixelMap *	framBitmapToPixMap(const FramBitmap * inFramBits);
	NativePixelMap *	framMaskToPixMap(const FramBitmap * inFramBits);

//	In a 64-bit world, we need to translate a 32-bit PixelMap.
	NativePixelMap *	pixMapToPixMap(const PixelMap * inPixMap);

	RefVar				fBitmapRef; // +00
	NativePixelMap		fPixmap;		// +04
	NativePixelMap *	fPixPtr;		// +20
	NativePixelMap		fMask;
	NativePixelMap *	fMaskPtr;	// +24
	UChar *				fColorTable;// +28
	int					fBitDepth;  // +2C
	bool					fIsLocked;  // +30
};

inline	NativePixelMap *	CPixelObj::pixMap(void)		const		{ return fPixPtr; }
inline	NativePixelMap *	CPixelObj::mask(void)		const		{ return fMaskPtr; }
inline	int					CPixelObj::bitDepth(void)	const		{ return fBitDepth; }


CPixelObj::CPixelObj()
{
	fBitDepth = kOneBitDepth;
	fColorTable = NULL;
	fIsLocked = false;
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
	// we cannot handle class='picture
	if (IsInstance(inBitmap, SYMA(picture)))
		ThrowErr(exGraf, -8803);

	// get a ref to the bitmap, whatever slot it’s in
	if (IsFrame(inBitmap)) {
		if (IsInstance(inBitmap, SYMA(bitmap))) {
			RefVar colorData(GetFrameSlot(inBitmap, SYMA(colorData)));
			if (ISNIL(colorData)) {
				fBitmapRef = GetFrameSlot(inBitmap, SYMA(data));
			} else {
				fBitmapRef = getFramBitmap();
			}
		} else {
			fBitmapRef = getFramBitmap();
		}
	}

	LockRef(fBitmapRef);
	fIsLocked = true;

	// create a PixelMap from the bitmap
	if (IsInstance(fBitmapRef, SYMA(pixels))) {
		fPixPtr = pixMapToPixMap((const PixelMap *)BinaryData(fBitmapRef));	// originally just points to (PixelMap *)BinaryData() but we need to convert to NativePixelMap
		fBitDepth = PixelDepth(fPixPtr);		// not original
	} else {
		fPixPtr = framBitmapToPixMap((const FramBitmap *)BinaryData(fBitmapRef));
	}

#if 0
	// we don’t mask by drawing with srcBic transferMode any more
	RefVar	theMask = GetFrameSlot(inBitmap, SYMA(mask));
	if (NOTNIL(theMask)) {
		CDataPtr  maskData(theMask);
		fMaskPtr = framMaskToPixMap((const FramBitmap *)(char *)maskData);
	} else
#endif
	fMaskPtr = NULL;
}


void
CPixelObj::init(RefArg inBitmap, bool inUseMask)
{
	fMaskPtr = NULL;
	fBitmapRef = inBitmap;
	if (IsInstance(inBitmap, SYMA(picture)))
		ThrowErr(exGraf, -8803);

	bool isPixelData = false;
	RefVar bits(GetFrameSlot(inBitmap, SYMA(data)));
	if (NOTNIL(bits)) {
		isPixelData = IsInstance(bits, SYMA(pixels));
	}

	LockRef(fBitmapRef);
	fIsLocked = true;
	if (isPixelData) {
		fPixPtr = pixMapToPixMap((const PixelMap *)BinaryData(fBitmapRef));
	} else {
		bits = GetFrameSlot(inBitmap, SYMA(mask));
		if (NOTNIL(bits)) {
			CDataPtr  obj(bits);
			fMaskPtr = framBitmapToPixMap((const FramBitmap *)(char *)obj);
		}
		if (inUseMask || fMaskPtr == NULL) {
			fPixPtr = framBitmapToPixMap((const FramBitmap *)BinaryData(getFramBitmap()));
		}
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
		if (IsArray(colorData)) {
		// find colorData that matches display most closely
			int	depth;
			int	maxDepth = 32767; // improbably large number
			int	minDepth = 0;
			CObjectIterator	iter(colorData);
			for ( ; !iter.done(); iter.next()) {
				depth = RINT(GetFrameSlot(iter.value(), SYMA(bitDepth)));
				if (depth == portDepth) {
					fBitDepth = depth;
					framBitmap = GetFrameSlot(iter.value(), SYMA(cBits));
					colorTable = GetFrameSlot(iter.value(), SYMA(colorTable));
				} else if ((portDepth < depth && depth < maxDepth && (maxDepth = depth, 1))
						|| (maxDepth == 32767 && portDepth > depth && depth > minDepth && (minDepth = depth, 1))) {
					fBitDepth = depth;
					framBitmap = GetFrameSlot(iter.value(), SYMA(cBits));
					colorTable = GetFrameSlot(iter.value(), SYMA(colorTable));
				}
			}
		} else {
			fBitDepth = RINT(GetFrameSlot(colorData, SYMA(bitDepth)));
			framBitmap = GetFrameSlot(colorData, SYMA(cBits));
			colorTable = GetFrameSlot(colorData, SYMA(colorTable));
		}

		if (fBitDepth > 1 && NOTNIL(colorTable)) {
			CDataPtr	colorTableData(colorTable);
			ArrayIndex numOfColors = Length(colorTable);
			ArrayIndex colorTableSize = 1 << fBitDepth;
			fColorTable = (UChar *)NewPtr(colorTableSize);
			if (fColorTable) {
				numOfColors /= 8;
				if (numOfColors > colorTableSize)
					numOfColors = colorTableSize;
#if 0
				Ptr defaultColorPtr = PixelMapBits(*GetFgPattern());
				for (ArrayIndex i = 0; i < colorTableSize; ++i) {
					fColorTable[i] = *defaultColorPtr;
				}
#endif
				ULong		r, g, b;
				short *	colorTablePtr = (short *)(char *)colorTableData;
				for (ArrayIndex i = 0; i < numOfColors; ++i) {
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


NativePixelMap *
CPixelObj::pixMapToPixMap(const PixelMap * inPixmap)
{
	fPixPtr->rowBytes = CANONICAL_SHORT(inPixmap->rowBytes);
	fPixPtr->reserved1 = 0;
	if (inPixmap->reserved1 == 0x11EB) {
		// it’s already been swapped
		fPixPtr->bounds.top = inPixmap->bounds.top;
		fPixPtr->bounds.left = inPixmap->bounds.left;
		fPixPtr->bounds.bottom = inPixmap->bounds.bottom;
		fPixPtr->bounds.right = inPixmap->bounds.right;
	} else {
		fPixPtr->bounds.top = CANONICAL_SHORT(inPixmap->bounds.top);
		fPixPtr->bounds.left = CANONICAL_SHORT(inPixmap->bounds.left);
		fPixPtr->bounds.bottom = CANONICAL_SHORT(inPixmap->bounds.bottom);
		fPixPtr->bounds.right = CANONICAL_SHORT(inPixmap->bounds.right);
	}
	fPixPtr->pixMapFlags = CANONICAL_LONG(inPixmap->pixMapFlags);
	fPixPtr->deviceRes.h = CANONICAL_SHORT(inPixmap->deviceRes.h);
	fPixPtr->deviceRes.v = CANONICAL_SHORT(inPixmap->deviceRes.v);

	fPixPtr->grayTable = 0;	//(UChar *)CANONICAL_LONG(inPixmap->grayTable);
printf("CPixelObj::pixMapToPixMap() PixelMap bounds=%d,%d, %d,%d", fPixPtr->bounds.top,fPixPtr->bounds.left, fPixPtr->bounds.bottom,fPixPtr->bounds.right);
	// our native pixmap baseAddr is ALWAYS a Ptr
	if ((fPixPtr->pixMapFlags & kPixMapStorage) == kPixMapOffset) {
		uint32_t offset = CANONICAL_LONG(inPixmap->baseAddr);
		fPixPtr->baseAddr = (Ptr)inPixmap + offset;
		fPixPtr->pixMapFlags = (fPixPtr->pixMapFlags & ~kPixMapStorage) | kPixMapPtr;
	} else {
printf("PixelMapBits() using baseAddr as Ptr!\n");
	}

	return fPixPtr;
}


NativePixelMap *
CPixelObj::framBitmapToPixMap(const FramBitmap * inBitmap)
{
	// ALWAYS point to the data -- ignore the bitmap’s baseAddr
	fPixPtr->baseAddr = (Ptr)inBitmap->data;

	fPixPtr->rowBytes = CANONICAL_SHORT(inBitmap->rowBytes);
	if (inBitmap->reserved1 == 0x11EB) {
		// it’s already been swapped
		fPixPtr->bounds.top = inBitmap->bounds.top;
		fPixPtr->bounds.left = inBitmap->bounds.left;
		fPixPtr->bounds.bottom = inBitmap->bounds.bottom;
		fPixPtr->bounds.right = inBitmap->bounds.right;
	} else {
		fPixPtr->bounds.top = CANONICAL_SHORT(inBitmap->bounds.top);
		fPixPtr->bounds.left = CANONICAL_SHORT(inBitmap->bounds.left);
		fPixPtr->bounds.bottom = CANONICAL_SHORT(inBitmap->bounds.bottom);
		fPixPtr->bounds.right = CANONICAL_SHORT(inBitmap->bounds.right);
	}
	fPixPtr->pixMapFlags = kPixMapPtr + fBitDepth;
	fPixPtr->deviceRes.h =
	fPixPtr->deviceRes.v = kDefaultDPI;
	fPixPtr->grayTable = fColorTable;
	if (fColorTable)
		fPixPtr->pixMapFlags |= kPixMapGrayTable;

	return fPixPtr;
}


NativePixelMap *
CPixelObj::framMaskToPixMap(const FramBitmap * inBitmap)
{
	// ALWAYS point to the data -- ignore the bitmap’s baseAddr
	fMask.baseAddr = (Ptr)inBitmap->data;

	fMask.rowBytes = CANONICAL_SHORT(inBitmap->rowBytes);
	if (inBitmap->reserved1 == 0x11EB) {
		// it’s already been swapped
		fMask.bounds.top = inBitmap->bounds.top;
		fMask.bounds.left = inBitmap->bounds.left;
		fMask.bounds.bottom = inBitmap->bounds.bottom;
		fMask.bounds.right = inBitmap->bounds.right;
	} else {
		fMask.bounds.top = CANONICAL_SHORT(inBitmap->bounds.top);
		fMask.bounds.left = CANONICAL_SHORT(inBitmap->bounds.left);
		fMask.bounds.bottom = CANONICAL_SHORT(inBitmap->bounds.bottom);
		fMask.bounds.right = CANONICAL_SHORT(inBitmap->bounds.right);
	}
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
				inTransferMode
	Return:	--
------------------------------------------------------------------------------*/

static const unsigned char colorTable1bit[] = { 0xFF, 0x00 };
static const unsigned char colorTable2bit[] = { 0xFF, 0xAA, 0x55, 0x00 };
static const unsigned char colorTable4bit[] = { 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88,
																0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00 };
static const unsigned char * colorTable[5] = { NULL, colorTable1bit, colorTable2bit, NULL, colorTable4bit };

void
DrawBitmap(RefArg inBitmap, const Rect * inRect, int inTransferMode)
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
		CGDataProviderRef source = CGDataProviderCreateWithData(NULL, PixelMapBits(pix.pixMap()), pixmapHeight * pix.pixMap()->rowBytes, NULL);
		if (source)
		{
			// create entire image
			image = CGImageCreate(pixmapWidth, pixmapHeight, pix.bitDepth(), pix.bitDepth(), pix.pixMap()->rowBytes,
										colorSpace, kCGImageAlphaNone, source,
										NULL, false, kCGRenderingIntentSaturation);
			CGDataProviderRelease(source);

			if (pix.mask() != NULL) {
				// image is masked
				CGDataProviderRef maskSource;
				if ((maskSource = CGDataProviderCreateWithData(NULL, PixelMapBits(pix.mask()), pixmapHeight * pix.mask()->rowBytes, NULL)) != NULL) {
					CGImageRef	fullImage = image;
					CGColorSpaceRef maskColorSpace = CGColorSpaceCreateDeviceGray();
					CGImageRef	mask = CGImageCreate(pixmapWidth, pixmapHeight, 1, 1, pix.mask()->rowBytes,
																maskColorSpace, kCGImageAlphaNone, maskSource,
																NULL, false, kCGRenderingIntentDefault);
					CGColorSpaceRelease(maskColorSpace);
					CGDataProviderRelease(maskSource);
					if (mask) {
						// recreate image using original full image with mask
						image = CGImageCreateWithMask(fullImage, mask);
						CGImageRelease(fullImage);
						CGImageRelease(mask);
					}
				}
			}
		}
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

	int mode = ISNIL(inTransferMode) ? modeCopy : RVALUE(inTransferMode);
	DrawPicture(inImage, &bounds, vjLeftH + vjTopV, mode);
	return NILREF;
}


Ref		FDrawXBitmap(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3) { return NILREF; }
Ref		FGrayShrink(RefArg inRcvr, RefArg inArg1, RefArg inArg2) { return NILREF; }
Ref		FDrawIntoBitmap(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3) { return NILREF; }
Ref		FViewIntoBitmap(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3) { return NILREF; }

bool
HitShape(RefArg inShape, Point inPt, RefArg ioPath)
{ return false; }

Ref
FHitShape(RefArg inRcvr, RefArg inShape, RefArg inX, RefArg inY)
{
	Point pt;
	pt.h = RINT(inX);
	pt.v = RINT(inY);
	RefVar path(AllocateArray(SYMA(pathExpr), 0));
	bool isHit = HitShape(inShape, pt, path);
	ArrayIndex pathLength = Length(path);
	if (pathLength == 0)
		return MAKEBOOLEAN(isHit);
	// reverse path slot order
	RefVar slot1, slot2;
	for (ArrayIndex i1 = 0; i1 < pathLength/2; ++i1)
	{
		ArrayIndex i2 = (pathLength - 1) - i1;
		slot1 = GetArraySlot(path, i2);
		slot2 = GetArraySlot(path, i1);
		SetArraySlot(path, i2, slot2);
		SetArraySlot(path, i1, slot1);
	}
	return path;
}


Ref
PtInPicture(RefArg inX, RefArg inY, RefArg inPicture, bool inUseMask)
{
	CPixelObj pix;
	RefVar result;
	newton_try
	{
		pix.init(inPicture, inUseMask);
		int x = RINT(inX);
		int y = RINT(inY);
		if (inUseMask)
		{
			if (pix.mask() && PtInMask(pix.mask(), x, y))
				result = MAKEINT(-1);
			else
				result = MAKEINT(PtInCPixelMap(pix.pixMap(), x, y));
		}
		else
		{
			result = MAKEBOOLEAN(PtInPixelMap(pix.mask()?pix.mask():pix.pixMap(), x, y));
		}
	}
	cleanup
	{
		pix.~CPixelObj();
	}
	end_try;
	return result;
}


Ref
FGetBitmapPixel(RefArg inRcvr, RefArg inX, RefArg inY, RefArg inBitmap)
{
	return PtInPicture(inX, inY, inBitmap, true);
}


Ref
FPtInPicture(RefArg inRcvr, RefArg inX, RefArg inY, RefArg inPicture)
{
	return PtInPicture(inX, inY, inPicture, false);
}

