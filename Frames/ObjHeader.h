/*
	File:		ObjHeader.h

	Contains:	Newton object structures

	Written by:	Newton Research Group
*/

#if !defined(__OBJHEADER_H)
#define __OBJHEADER_H 1

#if !defined(__NEWTONTYPES_H)
#include "NewtonTypes.h"
#endif

/*----------------------------------------------------------------------
	O b j e c t   H e a d e r
----------------------------------------------------------------------*/

#define OBJHEADER \
	uint32_t size  : 24; \
	uint32_t flags :  8; \
	union { \
		struct { \
			uint32_t	locks :  8; \
			uint32_t	slots : 24; \
		} count; \
		Ref stuff; \
		Ref destRef; \
	}gc;

struct ObjHeader
{
	OBJHEADER
}__attribute__((packed));


/*----------------------------------------------------------------------
	S l o t t e d   O b j e c t
----------------------------------------------------------------------*/

struct SlottedObject
{
	OBJHEADER

	Ref		slot[];
}__attribute__((packed));


/*----------------------------------------------------------------------
	F o r w a r d i n g   O b j e c t
----------------------------------------------------------------------*/

struct ForwardingObject
{
	OBJHEADER

	Ref		obj;
}__attribute__((packed));


/*----------------------------------------------------------------------
	B i n a r y   O b j e c t
----------------------------------------------------------------------*/

struct BinaryObject
{
	OBJHEADER

	Ref		objClass;
	char		data[];
}__attribute__((packed));

#define SIZEOF_BINARYOBJECT (sizeof(ObjHeader) + sizeof(Ref))

#define kMaxObjSize (size_t)(0x00FFFFFFL - SIZEOF_BINARYOBJECT)


/*----------------------------------------------------------------------
	I n d i r e c t   B i n a r y   O b j e c t
----------------------------------------------------------------------*/

struct IndirectBinaryProcs;
struct IndirectBinaryObject
{
	OBJHEADER

	Ref		objClass;
	const IndirectBinaryProcs * procs;
	char		data[];
}__attribute__((packed));

#define SIZEOF_INDIRECTBINARYOBJECT (sizeof(ObjHeader) + sizeof(Ref) + sizeof(const IndirectBinaryProcs *))


/*----------------------------------------------------------------------
	A r r a y   O b j e c t
----------------------------------------------------------------------*/

struct ArrayObject
{
	OBJHEADER

	Ref		objClass;
	Ref		slot[];
}__attribute__((packed));

#define SIZEOF_ARRAYOBJECT (sizeof(ObjHeader) + sizeof(Ref))

#define kMaxSlottedObjSize (size_t)(kMaxObjSize / sizeof(Ref))


/*----------------------------------------------------------------------
	F r a m e   O b j e c t
----------------------------------------------------------------------*/

struct FrameObject
{
	OBJHEADER

	Ref		map;
	Ref		slot[];
}__attribute__((packed));

#define SIZEOF_FRAMEOBJECT (sizeof(ObjHeader) + sizeof(Ref))


/*----------------------------------------------------------------------
	F r a m e   M a p   O b j e c t
----------------------------------------------------------------------*/

struct FrameMapObject
{
	OBJHEADER

	Ref		objClass;
	Ref		supermap;
	Ref		slot[];
}__attribute__((packed));

#define SIZEOF_FRAMEMAPOBJECT (sizeof(ObjHeader) + sizeof(Ref) + sizeof(Ref))


/*----------------------------------------------------------------------
	S y m b o l   O b j e c t
----------------------------------------------------------------------*/

struct SymbolObject
{
	OBJHEADER

	Ref		objClass;
	ULong		hash;
	char		name[];
}__attribute__((packed));


/*----------------------------------------------------------------------
	S t r i n g   O b j e c t
----------------------------------------------------------------------*/

struct StringObject
{
	OBJHEADER

	Ref		objClass;
	UniChar	str[];
}__attribute__((packed));


/*----------------------------------------------------------------------
	O b j e c t   F l a g s
----------------------------------------------------------------------*/

typedef enum
{
	kObjSlotted		= 0x01,
	kObjFrame		= 0x02,
	kObjFree			= 0x04,
	kObjMarked		= 0x08,
	kObjLocked		= 0x10,
	kObjForward		= 0x20,
	kObjReadOnly	= 0x40,
	kObjDirty		= 0x80,

// the lowest 2 bits indicate the object type,
// distinguished primarily by kObjSlotted
	kBinaryObject	= 0x00,
	kIndirectBinaryObject = 0x02,
	kArrayObject	= kObjSlotted + 0x00,
	kFrameObject	= kObjSlotted + 0x02,
	kObjMask			= 0x03
} ObjFlags;

#define ISLARGEBINARY(_o)	((_o->flags & kObjMask) == kIndirectBinaryObject)
#define ISARRAY(_o)	((_o->flags & kObjMask) == kArrayObject)
#define ISFRAME(_o)	((_o->flags & kObjMask) == kFrameObject)
#define NOTFRAME(_o)	((_o->flags & kObjMask) != kFrameObject)

#define kMapPlain		MAKEINT(0)
#define kMapSorted	MAKEINT(1 << 0)
#define kMapShared	MAKEINT(1 << 1)
#define kMapProto		MAKEINT(1 << 2)


/*----------------------------------------------------------------------
	R e f   < - >   P o i n t e r   C o n v e r s i o n
----------------------------------------------------------------------*/

#if defined(__cplusplus)
extern "C" {
#endif
ObjHeader *	ObjectPtr(Ref r);
#if defined(__cplusplus)
}
#endif

#define REF(_o) ((Ref)((Ref)_o + 1))
#define PTR(_o) ((ObjHeader*)((Ref)_o - 1))
#define INC(_o,_i) ((ObjHeader*)((char*)_o + _i))

#define SLOTCOUNT(_o) (ArrayIndex)((_o->size - sizeof(ObjHeader)) / sizeof(Ref))
#define ARRAYLENGTH(_o) (ArrayIndex)((_o->size - SIZEOF_ARRAYOBJECT) / sizeof(Ref))
#define BINARYLENGTH(_o) (size_t)(_o->size - SIZEOF_BINARYOBJECT)


#endif	/* __OBJHEADER_H */
