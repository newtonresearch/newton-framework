/*
	File:		QDTypes.h

	Contains:	QuickDraw interface to drawing functions.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__QDTYPES_H)
#define __QDTYPES_H 1

#include "NewtonTypes.h"

struct FPoint
{
	float	x;
	float	y;
};


struct FRect
{
	float	left;
	float	top;
	float	right;
	float	bottom;
};


struct RoundRectShape
{
	Rect	rect;
	short	hRadius;
	short	vRadius;
};


struct WedgeShape
{
	Rect	rect;
	short	arc1;
	short	arc2;
};


struct PolygonShape
{
	short			size;
	short			reserved1;
	Rect			bBox;
	Point			points[];
};


struct RegionShape
{
	short			size;
	short			reserved1;
	Rect			bBox;
	char			data[];
};


struct PictureShape
{
	short			size;
	short			reserved1;
	Rect			bBox;
	char			data[];
};

typedef void *		ShapePtr;


/*	In a 64-bit world, the FramBitmap MUST still align with its 32-bit definition.
	Since the FramBitmap is only built by NTK we can assume the baseAddr is a 32-bit offset, never a pointer.
*/
struct FramBitmap
{
	uint32_t		baseAddr;		// +00
	short			rowBytes;		// +04
	short			reserved1;		// pads to long
	Rect			bounds;			// +08
	char			data[];			// +10
}__attribute__((packed));


/*	In a 64-bit world, the PixelMap MUST still align with its 32-bit definition
	for handling data built by NTK or imported by NCX.
*/
struct PixelMap
{
	uint32_t		baseAddr;
	short			rowBytes;
	short			reserved1;		// pads to long
	Rect			bounds;
	ULong			pixMapFlags;
	Point			deviceRes;		// resolution of input device (0 indicates kDefaultDPI)
	uint32_t		grayTable;		// gray tone table
}__attribute__((packed));

/*	However, there are some uses (screen bitmaps, for example) where we need host-sized pointers.
*/
struct NativePixelMap
{
	Ptr			baseAddr;
	short			rowBytes;
	short			reserved1;		// pads to long
	Rect			bounds;
	ULong			pixMapFlags;
	Point			deviceRes;		// resolution of input device (0 indicates kDefaultDPI)
	UChar *		grayTable;		// gray tone table
};

// pixMapFlags bits
#define	kPixMapStorage			0xC0000000	// to mask off the appropriate bits
#define	kPixMapHandle			0x00000000	// baseAddr is a handle
#define	kPixMapPtr				0x40000000	// baseAddr is a pointer
#define	kPixMapOffset			0x80000000	// baseAddr is an offset from the PixelMap

#define	kPixMapLittleEndian	0x20000000	// pixMap is little endian
#define	kPixMapAllocated		0x10000000	// pixMap "owns" the bits memory
#define	kPixMapGrayTable		0x08000000	// grayTable field exists
#define	kPixMapNoPad			0x04000000	// direct pixel format, no pad byte
#define	kPixMapByComponent	0x02000000	// direct pixel format, stored by component
#define	kPixMapAntiAlias		0x01000000	// antialiasing ink text

#define	kPixMapVersionMask	0x0000F000	// version of this struct
#define	kPixMapDeviceType		0x00000F00	// bits 8..11 are device type code
#define	kPixMapDevScreen		0x00000000	// 	screen or offscreen bitmap
#define	kPixMapDevDotPrint	0x00000100	// 	dot matrix printer
#define	kPixMapDevPSPrint		0x00000200	// 	postscript printer
#define	kPixMapDepth			0x000000FF	// bits 0..7 are chunky pixel depth

#define	kOneBitDepth			1
#define	kDefaultDPI				72				// default value for deviceRes fields
#define	kVersionShift			12

#define	kPixMapVersion1		(0x0 << kVersionShift)
#define	kPixMapVersion2		(0x1 << kVersionShift)

// PixelMap accessors
inline bool		GrayTableExists(const PixelMap * pixmap)	{ return pixmap->pixMapFlags & kPixMapGrayTable; }
inline bool		NoPadByte(const PixelMap * pixmap)			{ return pixmap->pixMapFlags & kPixMapNoPad; }
inline bool		ByComponent(const PixelMap * pixmap)		{ return pixmap->pixMapFlags & kPixMapByComponent; }
inline bool		AntiAlias(const PixelMap * pixmap)			{ return pixmap->pixMapFlags & kPixMapAntiAlias; }

inline ULong	PixelDepth(const PixelMap * pixmap)			{ return pixmap->pixMapFlags & kPixMapDepth; }
inline ULong	PixelDepth(const NativePixelMap * pixmap)	{ return pixmap->pixMapFlags & kPixMapDepth; }
inline ULong	PixelMapVersion(const PixelMap * pixmap)	{ return pixmap->pixMapFlags & kPixMapVersionMask; }
extern Ptr		PixelMapBits(const PixelMap * pixmap);
extern Ptr		PixelMapBits(const NativePixelMap * pixmap);


// Font/Text structures elsewhere
// Patterns (colours) elsewhere


#endif	/* __QDTYPES_H */
