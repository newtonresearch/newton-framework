
/*------------------------------------------------------------------------------
	Legacy Refs for a 64-bit world.
	See also Newton/ObjectSystem.cc which does this for Refs createwd by NTK.
------------------------------------------------------------------------------*/
#if !defined(__REF32_H)
#define __REF32_H 1

typedef int32_t Ref32;

#define OBJHEADER32 \
	uint32_t size  : 24; \
	uint32_t flags :  8; \
	union { \
		struct { \
			uint32_t	locks :  8; \
			uint32_t	slots : 24; \
		} count; \
		int32_t stuff; \
		Ref32 destRef; \
	}gc;

struct ObjHeader32
{
	OBJHEADER32
}__attribute__((packed));

struct BinaryObject32
{
	OBJHEADER32
	Ref32		objClass;
	char		data[];
}__attribute__((packed));

struct ArrayObject32
{
	OBJHEADER32
	Ref32		objClass;
	Ref32		slot[];
}__attribute__((packed));

struct SymbolObject32
{
	OBJHEADER32
	Ref32		objClass;
	ULong		hash;
	char		name[];
}__attribute__((packed));

struct StringObject32
{
	OBJHEADER32
	Ref32		objClass;
	UniChar	str[];
}__attribute__((packed));


#define k4ByteAlignmentFlag 0x00000001

#endif	/* __REF32_H */
