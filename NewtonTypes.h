/*
	File:		NewtonTypes.h

	Contains:	Global types for Newton build system.

	Written by:	Newton Research Group.
*/

#if !defined(__NEWTONTYPES_H)
#define __NEWTONTYPES_H 1

#include <stdint.h>

/* Base types */

typedef int8_t		SChar;
typedef uint8_t 	UChar;

typedef int8_t		SByte;
typedef uint8_t	UByte;

typedef int16_t	SShort;
typedef uint16_t	UShort;

typedef int32_t	SLong;
typedef uint32_t	ULong;

/* Array/String index -- also used to define length */
typedef uint32_t	ArrayIndex;
#define kIndexNotFound -1

typedef unsigned long	offs_t;

/* Error codes -- need to hold codes less than -32767 */
typedef int	NewtonErr;

/* The type of objects in the OS */
typedef uint32_t	ObjectId;
typedef uint32_t	HammerIORef;

/* Address types */
typedef unsigned long	VAddr;
typedef unsigned long	PAddr;

typedef unsigned long	OpaqueRef;

/* Ref types */

typedef long Ref;

typedef struct
{
	Ref	ref;
	Ref	stackPos;
} RefHandle;


#if !defined(__MACTYPES__)
/* From MacTypes.h */
#define __MACTYPES__ 1

typedef unsigned char						 Boolean;

typedef unsigned char                   UInt8;
typedef signed char                     SInt8;
typedef unsigned short                  UInt16;
typedef signed short                    SInt16;
#if __LP64__
typedef unsigned int                    UInt32;
typedef signed int                      SInt32;
#else
typedef unsigned long                   UInt32;
typedef signed long                     SInt32;
#endif

typedef UInt8                           Byte;
typedef SInt8                           SignedByte;

#if TARGET_RT_BIG_ENDIAN
struct wide {
  SInt32              hi;
  UInt32              lo;
};
typedef struct wide                     wide;
struct UnsignedWide {
  UInt32              hi;
  UInt32              lo;
};
typedef struct UnsignedWide             UnsignedWide;
#else
struct wide {
  UInt32              lo;
  SInt32              hi;
};
typedef struct wide                     wide;
struct UnsignedWide {
  UInt32              lo;
  UInt32              hi;
};
typedef struct UnsignedWide             UnsignedWide;
#endif  /* TARGET_RT_BIG_ENDIAN */

#if TYPE_LONGLONG
/*
  Note:   wide and UnsignedWide must always be structs for source code
           compatibility. On the other hand UInt64 and SInt64 can be
          either a struct or a long long, depending on the compiler.
         
           If you use UInt64 and SInt64 you should do all operations on 
          those data types through the functions/macros in Math64.h.  
           This will assure that your code compiles with compilers that
           support long long and those that don't.
            
           The MS Visual C/C++ compiler uses __int64 instead of long long. 
*/
    #if defined(_MSC_VER) && !defined(__MWERKS__) && defined(_M_IX86)
		typedef   signed __int64                SInt64;
		typedef unsigned __int64                UInt64;
    #else
		typedef   signed long long              SInt64;
		typedef unsigned long long              UInt64;
    #endif
#else
typedef wide                            SInt64;
typedef UnsignedWide                    UInt64;
#endif  /* TYPE_LONGLONG */

typedef float               Float32;
typedef double              Float64;

typedef SInt16                          OSErr;
typedef SInt32                          OSStatus;
typedef UInt8 *                         BytePtr;
typedef unsigned long                   ByteCount;
typedef unsigned long                   ByteOffset;
typedef SInt32                          Duration;
typedef UInt32                          OptionBits;
typedef unsigned long                   ItemCount;
typedef SInt16                          ScriptCode;
typedef SInt16                          LangCode;
typedef SInt16                          RegionCode;
typedef UInt32                          FourCharCode;
typedef FourCharCode                    OSType;
typedef FourCharCode                    ResType;

typedef UInt32                          UnicodeScalarValue;
typedef UInt32                          UTF32Char;
typedef UInt16									 UniChar;
typedef unsigned long                   UniCharCount;

typedef unsigned char *                 StringPtr;
typedef const unsigned char *           ConstStringPtr;
typedef const unsigned char *           ConstStr255Param;
typedef char                            Str255[256];
typedef unsigned char                   Str31[32];
typedef unsigned char                   Str15[16];

typedef unsigned char                   Style;

/* Graphics types */
struct Point
{
	short	v;
	short	h;
};

struct Rect
{
	short	top;
	short	left;
	short	bottom;
	short	right;
};

/* Pointer types */
typedef char * Ptr;
typedef Ptr * Handle;	// deprecated
typedef int32_t (*ProcPtr)(void*);

/* Math types */
typedef int32_t Fixed;
typedef int32_t Fract;
typedef uint32_t UnsignedFixed;

/* From FixMath.h */
#define fixed1              ((Fixed) 0x00010000L)
#define fract1              ((Fract) 0x40000000L)

#endif	/* __MACTYPES__ */

#if defined(__cplusplus)

class SingleObject {};

inline ProcPtr
ptmf2ptf(int (SingleObject::*func)(int,void*))
{
	union
	{
		int (SingleObject::*fIn)(int,void*);
		ProcPtr	fOut;
	} map;

	map.fIn = func;
	return map.fOut;
}
#define MemberFunctionCast(_t, self, fn) (_t) ptmf2ptf((int (SingleObject::*)(int,void*)) fn)

#endif	/* __cplusplus */

#endif	/* __NEWTONTYPES_H */
