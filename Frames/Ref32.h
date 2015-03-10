
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

struct ArrayObject32
{
	OBJHEADER32
	Ref32		objClass;
	Ref32		slot[];
};

struct SymbolObject32
{
	OBJHEADER32
	Ref32		objClass;
	ULong		hash;
	char		name[];
};

struct StringObject32
{
	OBJHEADER32
	Ref32		objClass;
	UniChar	str[];
};


#if defined(hasByteSwapping)
#define k4ByteAlignmentFlag 0x01000000
#else
#define k4ByteAlignmentFlag 0x00000001
#endif

#if __LP64__
#include <map>
typedef std::map<Ref32, Ref> RefMap;

extern bool		IsObjClass(Ref obj, const char * inClassName);

extern void		FixUpRef(Ref32 * inRefPtr, char * inBaseAddr);
extern size_t	ScanRef(Ref32 * inRefPtr, char * inBaseAddr);
extern void		CopyRef(ArrayObject32 * &inSrcPtr, ArrayObject * &inDstPtr, RefMap & inMap, char * inBaseAddr, ULong inAlignment);
extern void		UpdateRef(Ref * inRefPtr, RefMap & inMap);
#endif

#endif	/* __REF32_H */
