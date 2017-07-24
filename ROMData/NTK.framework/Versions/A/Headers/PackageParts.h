/*
	File:		PackageParts.h

	Contains:	Package structures

	Written by:	Newton Research Group
*/

#if !defined(__PACKAGEPARTS_H)
#define __PACKAGEPARTS_H 1

#include "NewtonTypes.h"

typedef ULong	Date;

struct InfoRef
{
	UShort	offset;
	UShort	length;
};


/*
	Part Entries
	------------
	Following the PackageDirectory, each part is represented by a PartEntry structure.
	Parts are often referred to by number (part 0, part 1, and so forth); the number is
	determined by the order of the PartEntry structures. The first PartEntry corresponds
	to part 0.
*/

struct PartEntry
{
	ULong		offset;				//	offset to part, longword aligned
	ULong		size;					//	size of part
	ULong		size2;				//	= size
	ULong		type;					//	'form', 'book', 'auto' etc.
	ULong		reserved1;
	ULong		flags;				//	defined below
	InfoRef	info;					//	data passed to part when activated
	InfoRef	compressor;			// compressor name
};

enum
{
	kProtocolPart				= 0x00000000,
	kNOSPart						= 0x00000001,
	kRawPart						= 0x00000002,
	kPackagePart				= 0x00000003,
	kPartTypeMask				= 0x00000003,

	kAutoLoadPartFlag			= 0x00000010,	//	kAutoLoadFlag renamed for consistency with...
	kAutoRemovePartFlag		= 0x00000020,	//	kAutoRemoveFlag renamed to avoid clash with package flag
	kCompressedFlag			= 0x00000040,
	kNotifyFlag					= 0x00000080,
	kAutoCopyFlag				= 0x00000100
};


/*
	The part entries are followed by the variable-length data area referred to by the
	InfoRef values. Most data in this area consists of ASCII or Unicode strings. Note that
	although the length of the string is provided in the InfoRef, null terminators are
	necessary and should be included at the end of the strings.
*/


/*
	Relocation Information
	----------------------
	If the package signature is "package1", and the package directory kRelocationFlag
	is set, the package contains a relocation information area after the package directory. This
	information is used to relocate package-relative addresses to compensate for the virtual
	address that gets assigned to the package when it is activated. This allows native code in
	the package to use absolute addresses.

	The relocation information begins with a fixed set of fields represented by the
	RelocationHeader structure.
*/

struct RelocationHeader
{
	ULong		reserved;			//	must be zero
	ULong		relocationSize;	//	total size of relocation information area including this header
	ULong		pageSize;			//	size in bytes of a relocation page - must be 1024
	ULong		numEntries;			//	number of relocation entries following the header
	ULong		baseAddress;		//	original base address of the package
};

/*
	The RelocationHeader is followed by a series of RelocationEntry structures. Each
	RelocationEntry must be padded to a multiple of four bytes.
*/

struct RelocationEntry
{
	UShort	pageNumber;			//	zero-based index of the page in the package
	UShort	offsetCount;		//	number of offset bytes in offsets
	UByte		offsets[];			/*	offsets of the words to be relocated.
											Each byte is the zero-based index of a word in the page
											to be relocated. For example, the offset 0x10 means
											to relocate a word 0x40 bytes into the page. */
};


/*
	Package Directory
	-----------------
	The format of the package directory is defined as a PackageDirectory structure,
	followed by zero or more PartEntry structures (one per part), followed by the
	variable-length data area.

	The package directory begins with a fixed set of fields represented by the
	PackageDirectory structure.
*/

struct PackageDirectory
{
// package header
	char		signature[8];		//	'package0' or 'package1'
	ULong		id;					// reserved1 according to “Newton Formats 1.1” -- typically xxxx
	ULong		flags;				//	defined below
	ULong		version;				//	arbitrary number
	InfoRef	copyright;			//	Unicode copyright notice - optional
	InfoRef	name;					//	Unicode package name - unique
	ULong		size;					//	total size of package including this directory
	Date		creationDate;
	Date		modifyDate;			// reserved2 according to “Newton Formats 1.1” -- typically 0
// parts directory header
	ULong		reserved3;
	ULong		directorySize;		//	size of this directory including part entries & data
	ULong		numParts;			//	number of parts in the package
// part directory entries
	PartEntry parts[];
};

enum
{
	kAutoRemoveFlag				= 0x80000000,
	kCopyProtectFlag				= 0x40000000,
	kInvisibleFlag					= 0x20000000,		// undocumented -- set by Packer MPW tool
	kNoCompressionFlag			= 0x10000000,
	kRelocationFlag				= 0x04000000,		//	2.0 only
	kUseFasterCompressionFlag	= 0x02000000		//	2.0 only
};


#endif	/* __PACKAGEPARTS_H */
