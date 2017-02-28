/*
	File:		PackageTypes.h

	Copyright:	© 1992-1996 by Apple Computer, Inc., all rights reserved.

	Derived from v1 internal, 1/8/96.
*/

#if !defined(__PACKAGETYPES_H)
#define __PACKAGETYPES_H 1

// the following are various sourcetypes bits

#define kStream 		 		0x0
#define kFixedMemory 	 	0x1
#define kRemovableStream 	0x2
#define kRemovableMemory 	0x3

#define kRemovableMask     0x2
#define kFormatMask			0x1

// device types

#define kNoDevice				0x0
#define kCardDevice			0x1
#define kSerialDevice		0x2
#define kIRDevice				0x3
#define kOtherDevice			0x4
#define kStoreDevice			0x5
#define kStoreDeviceV2		0x6

// part types

#define kProtocol 				0
#define kFrames   				1
#define kRaw      				2


#ifndef FRAM

#ifndef __NEWTONTYPES_H
#include "NewtonTypes.h"
#endif

typedef ULong	PartType;
typedef ULong	PartKind;

#endif	//	FRAM


#if defined(forNTK) || !defined(forDocker)
#ifndef FRAM

// the magic number header for a package
extern "C" const char * const	kPackageMagicNumber;
// length of kPackageMagicNumber up to version character
#define kPackageMagicLen 7
// number of version characters
#define kPackageMagicVersionCount 2

const long kMaxInfoSize					= 64;
const long kMaxCompressorNameSize	= 32;
const long kMaxPackageNameSize		= 32;


struct PartId
{
	ObjectId		packageId;
	ArrayIndex	partIndex;
};


struct SourceType
{
	UByte		format;
	UByte 	deviceKind;
	UShort 	deviceNumber;
	ULong 	deviceId;
};


inline bool IsRemovable(SourceType type)	{ return (type.format & kRemovableMask) != 0; }
inline bool IsPersistent(SourceType type)	{ return (type.format & kRemovableMask) == 0; }
inline bool IsMemory(SourceType type)		{ return (type.format & kFormatMask) != 0; }
inline bool IsStream(SourceType type)		{ return (type.format & kFormatMask) == 0; }


struct PartInfo
{
	PartKind	kind;
	PartType	type;
	ULong		size;
	ULong		sizeInMemory;
	ULong		infoSize;
	void *	info;
	const char *	compressor;
	union
	{
		ULong		streamOffset;		// stream: offset
		Ptr		address;				// ram: absolute address
	} data;
	bool		autoLoad;
	bool		autoRemove;
	bool		compressed;
	bool		notify;
	bool		autoCopy;
};


struct ExtendedPartInfo : PartInfo
{
	UniChar	packageName[kMaxPackageNameSize + 1];
};


struct StreamSource
{
	ObjectId	bufferId;
	ObjectId	messagePortId;
};


struct MemSource
{
	void *	buffer;
	size_t	size;
};


union PartSource
{
	StreamSource stream;
	MemSource	 mem;
};


#define kIterateOnAllPackages 0xFFFFFFFF


// reserved part types:
// this part type id is the id of the part handler that autoloads forms
const PartType kFormPartType  =	'form';
const PartType kPatchPartType =	'ptch';


#endif // FRAM
#endif // Docker

#endif	/* __PACKAGETYPES_H */
