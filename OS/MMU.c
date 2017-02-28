/*
	File:		MMU.c

	Contains:	Memory Management Unit code.

	Written by:	Newton Research Group.
*/
typedef char bool;

#include "MMU.h"
#include "SWI.h"
#include "OSErrors.h"

#undef true
#define true 1
#undef false
#define false 0

extern NewtonErr	GenericSWI(int inSelector, ...);

extern PAddr	GetPrimaryTablePhysBase(void);
extern PAddr	GetPrimaryTablePhysBaseMMUOff(SGlobalsThatLiveAcrossReboot * info);
extern PAddr	GetPrimaryTablePhysBaseAfterGlobalsInited(void);

extern void		ZeroPhysSubPage(PAddr inAddr);


/* -----------------------------------------------------------------------------
	When the MMU is on, the top 12 bits of each virtual address index into
	the Level One Descriptor table (which is therefore 4 KWords of descriptors).
	This means each MByte or Section of address space has a Level One Descriptor.
	The descriptor indicates whether the section is:
		00		invalid		section translation fault is generated
		01		page			Level Two lookup required
		10		section		no further lookup required
		11		reserved		behaves like invalid
----------------------------------------------------------------------------- */

typedef uint32_t MMUDescriptor;

// Level 1 Descriptor bits
enum
{
	kInvalidDescriptorType,
	kPageDescriptorType,
	kSectionDescriptorType,
	kReservedDescriptorType,

	kBufferable				= 1 << 2,
	kCacheable				= 1 << 3,
	kUpdateable				= 1 << 4,

	kDomainShift			=  5,
	kPageTableBaseShift	= 10,
	kSectionPermShift		= 10,
	kSectionBaseShift		= 20
};

#define kDescriptorTypeMask				 0x03
#define kDomainMask							(0x0F << kDomainShift)
#define kPageTableBaseMask					(~0 << kPageTableBaseShift)
#define kSectionBaseMask					(~0 << kSectionBaseShift)
#define kPageDescriptorAccessMask		(kDomainMask | kUpdateable)
#define kSectionDescriptorAccessMask	(kDomainMask | kUpdateable | kCacheable | kBufferable)
#define kSectionPermMask					(0x03 << kSectionPermShift)

// Level 2 Descriptor bits
enum
{
	kLargePageDescriptorType = 1,
	kSmallPageDescriptorType,

	// kBufferable and kCacheable shared from Level 1

	kPermissionShift		=  4,
	kSmallPageBaseShift  = 12,
	kLargePageBaseShift	= 16
};

#define kPermissionMask						(0xFF << kPermissionShift)
#define kSmallPageBaseMask					(~0 << kSmallPageBaseShift)
#define kLargePageBaseMask					(~0 << kLargePageBaseShift)


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

PAddr		gPrimaryTable;						// 0C1016D8

ULong		gLevel2PermissionBits[4];		// 0C106830 = { 0x0000, 0x0550, 0x0AA0, 0x0FF0 };


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

ULong		LoadFromPhysAddress(PAddr inAddr);
void		StoreToPhysAddress(PAddr inAddr, ULong inData);

void		CleanPageInDCache(PAddr inAddr);
void		_FlushMMU(void);


/*------------------------------------------------------------------------------
	Initialize the MMU page table.
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

void
InitPageTable(void)
{
	InitPagePermissions();
}


/*------------------------------------------------------------------------------
	Initialize the table of MMU level 2 descriptor permission bits
	to R/W only in supervisor mode.
	Dunno why gLevel2PermissionBits can’t be const data.
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

void
InitPagePermissions(void)
{
	ArrayIndex  i;
	for (i = 0; i < 4; ++i)
		gLevel2PermissionBits[i] = (0x55*i) << kPermissionShift;
}


/*------------------------------------------------------------------------------
	Load / store to physical address.
	Args:		--
	Return:  --
	Linkage: assembler
------------------------------------------------------------------------------*/

ULong
LoadFromPhysAddress(PAddr inAddr)
{
#if defined(correct)
	return *(ULong*)inAddr;
#else
	return 0;
#endif
}


void
StoreToPhysAddress(PAddr inAddr, ULong inData)
{
// inAddr is a physical Newton address: don’t use it, bad things will happen
#if defined(correct)
	*(ULong*)inAddr = inData;
#endif
}

#pragma mark -

/*------------------------------------------------------------------------------
	Initialize the MMU’s Level One Descriptor table.
	Args:		true, false, descriptorDefs, persistenGlobals
	Return:  --
------------------------------------------------------------------------------*/

void
InitTheMMUTables(bool inAll, bool inMMUOn, PAddr inPrimaryDef, SGlobalsThatLiveAcrossReboot * info)
{
	PAddr level1Descriptors = inMMUOn ? GetPrimaryTablePhysBase() : GetPrimaryTablePhysBaseMMUOff(info);
	MakePrimaryMMUTable(inPrimaryDef, level1Descriptors, inAll, inMMUOn, info);
}


/*------------------------------------------------------------------------------
	Make the MMU’s Level One Descriptor table.
	Map kSectionSize (MByte) blocks virtual -> physical addresses.
	Args:		inPrimaryDef		definition of Level One Descriptor table
				ioDescriptors		physical address of Level One Descriptor table
				inFill				true -> fill unused sections with kInvalidDescriptorType
				inMMUOn				true -> MMU is on
				info					persistent globals
	Return:  physical address of Level One Descriptor table
------------------------------------------------------------------------------*/

PAddr
MakePrimaryMMUTable(PAddr inPrimaryDef, PAddr ioDescriptors, bool inFill, bool inMMUOn, SGlobalsThatLiveAcrossReboot * info)
{
	// save ALL registers
	ULong *  descriptorPtr = (ULong *)ioDescriptors;
	PAddr		physAddr = 0;
	PAddr		physOffset;
	SectionDef *	def;

	for (def = (SectionDef *)inPrimaryDef; def->start != 0xFFFFFFFF; def++)
	{
	// fill the table up to this address with translation faults
		while (physAddr < def->start)
		{
			if (inFill)
			{
				if (inMMUOn)
					StoreToPhysAddress(descriptorPtr, kInvalidDescriptorType);
				else
					*descriptorPtr = kInvalidDescriptorType;
			}
			++descriptorPtr;
			physAddr += kSectionSize;
		}
//31
		for (physOffset = 0; physOffset < def->size; physOffset += kSectionSize)
		{
			ULong	descr;
			if (def->bits & kPageDescriptorType)
				descr = def->x04 | def->bits;
			else
				descr = def->x04 + physOffset;
			if (inMMUOn)
				StoreToPhysAddress(descriptorPtr, descr);
			else
				*descriptorPtr = descr;
			++descriptorPtr;
			physAddr += kSectionSize;
		}
	}

//62
	if (inFill)
	{
	// fill the remainder of the table with translation faults
		while (descriptorPtr < (ULong *)((char *)ioDescriptors + 16*KByte))
		{
			if (inMMUOn)
				StoreToPhysAddress(descriptorPtr, kInvalidDescriptorType);
			else
				*descriptorPtr = kInvalidDescriptorType;
			++descriptorPtr;
		}
	}

#if 0
//80
	for (i = 0; i < kMaxROMExtensions; ++i)	// r6
	{
		if (r1 = info->fRExPtr[i])
		{
			r5 = r1 - (r1 & kPageSize);
			def = info->fPhysRExPtr[i]
	// INCOMPLETE
			if (inMMUOn)
				AddNewSecPNJT(r5, def, 0, kReadOnly, true); // (ULong, ULong, ULong, Perm, bool)
			else
				AddNewSecPNJTMMUWithOff(r5, def, 0, kReadOnly, true, info);
		}
	}
#endif

	if (IsSuperMode())
		FlushTheMMU();
	else
		_FlushMMU();

	return ioDescriptors;
}


/*------------------------------------------------------------------------------
	Flush the MMU.
	Args:		--
	Return:  --
	Linkage: assembler
------------------------------------------------------------------------------*/

void
FlushTheMMU(void)
{
/*
		MRC	Internal, R0, CR0
		BIC	R0, R0, #&0000000F
		EOR	R0, R0, #&44000000
		EOR	R0, R0, #&00010000
		EORS	R0, R0, #&0000A100
		MCREQ Internal, R0, CR8
		MCRNE Internal, R0, Fault Status
		MOV	PC, LK
*/
}

void
_FlushMMU(void)
{
/*
		SWI	&8
		MOV	PC, LK
*/
}

void
FlushTheCache(void)
{
/*
		MRC	Internal, R0, CR0
		BIC	R0, R0, #&0000000F
		EOR	R0, R0, #&44000000
		EOR	R0, R0, #&00010000
		EORS	R0, R0, #&0000A100
		BNE	L11
		MOV	R0, #&04000000
		ADD	R0, R0, #&00004000
L8		LDR	R1, [R0], #-&00000020
		TEQ	R0, #&04000000
		BNE	L8
L11	MCR Internal, R0, CR7
		MOV	PC, LK
*/
}

void
FlushDCache(void)
{
/*
		MRC	Internal, R0, CR0
		BIC	R0, R0, #&0000000F
		EOR	R0, R0, #&44000000
		EOR	R0, R0, #&00010000
		EORS	R0, R0, #&0000A100
		MOVNE	PC, LK

		MOV	R0, #&04000000
		ADD	R0, R0, #&00004000
L8		LDR	R1, [R0], #-&00000020
		TEQ	R0, #&04000000
		BNE	L8
		MCR Internal, R0, CR7
		MOV	PC, LK
*/
}

void
FlushEntireTLB(void)
{
/*
		MRC	Internal, R1, CR0
		BIC	R1, R1, #&0000000F
		EOR	R1, R1, #&44000000
		EOR	R1, R1, #&00010000
		EORS	R1, R1, #&0000A100
		MCREQ Internal, R0, CR8
		MCRNE Internal, R0, Fault Status
		MOV	PC, LK
*/
}

#pragma mark -

PAddr
PrimVtoP(VAddr inVAddr)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor, level2Descriptor;
	int		descriptorType;
	PAddr		physAddr;

	EnterFIQAtomic();
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorType = level1Descriptor & kDescriptorTypeMask;
	if (descriptorType == kPageDescriptorType)
	{
	//	level 2 lookup required
		descriptorPtr = (level1Descriptor & kPageTableBaseMask) + ((inVAddr / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
		level2Descriptor = LoadFromPhysAddress(descriptorPtr);
		ExitFIQAtomic();
		descriptorType = level2Descriptor & kDescriptorTypeMask;
		if (descriptorType == kLargePageDescriptorType
		||  descriptorType == kSmallPageDescriptorType)
			physAddr = (level2Descriptor & ~(kPageSize-1)) + (inVAddr & (kPageSize-1));
		else
			physAddr = kIllegalPAddr;
	}
	else if (descriptorType == kSectionDescriptorType)
	{
	//	no further lookup required - just map to physical section
		ExitFIQAtomic();
		physAddr = (level1Descriptor & ~(kSectionSize-1)) + (inVAddr & (kSectionSize-1));
	}
	else
	{
	//	invalid section
		ExitFIQAtomic();
		physAddr = kIllegalPAddr;
	}

	return physAddr;
}


PAddr
VtoP(VAddr inVAddr)
{
	if (IsSuperMode())
		return PrimVtoP(inVAddr);
	return (PAddr) GenericSWI(kVtoP, inVAddr);
}


size_t
VtoSizeWithP(VAddr inVAddr, PAddr * outPAddr)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor, level2Descriptor;
	int		descriptorType;
	PAddr		physAddr = kIllegalPAddr;
	size_t	size = kSectionSize;	// size remaining

	EnterFIQAtomic();
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorType = level1Descriptor & kDescriptorTypeMask;

	if (descriptorType == kPageDescriptorType)
	{
	//	level 2 lookup required
		descriptorPtr = (level1Descriptor & kPageTableBaseMask) + ((inVAddr / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
		level2Descriptor = LoadFromPhysAddress(descriptorPtr);
		ExitFIQAtomic();
		descriptorType = level2Descriptor & kDescriptorTypeMask;
		if (descriptorType == kLargePageDescriptorType)
		{
			physAddr = (level2Descriptor & kLargePageBaseMask) + (inVAddr & ~kLargePageBaseMask);
			size = kBigPageSize - (inVAddr & ~kLargePageBaseMask);
		}
		else if (descriptorType == kSmallPageDescriptorType)
		{
			physAddr = (level2Descriptor & kSmallPageBaseMask) + (inVAddr & ~kSmallPageBaseMask);
			size = kPageSize - (inVAddr & ~kSmallPageBaseMask);
		}
		else
			size = kPageSize;
	}
	else if (descriptorType == kSectionDescriptorType)
	{
	//	no further lookup required
		ExitFIQAtomic();
		physAddr = level1Descriptor & kSectionBaseMask;
		size = kSectionSize - (inVAddr & ~kSectionBaseMask);
	}
	else
	{
	//	invalid section
		ExitFIQAtomic();
	}

	*outPAddr = physAddr;
	return size;
}


int
VtoUnit(VAddr inVAddr)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor, level2Descriptor;
	int		descriptorType;
	int		unit;

	EnterFIQAtomic();
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorType = level1Descriptor & kDescriptorTypeMask;

	if (descriptorType == kPageDescriptorType)
	{
	//	level 2 lookup required
		descriptorPtr = (level1Descriptor & kPageTableBaseMask) + ((inVAddr / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
		level2Descriptor = LoadFromPhysAddress(descriptorPtr);
		ExitFIQAtomic();
		descriptorType = level2Descriptor & kDescriptorTypeMask;
		if (descriptorType == kLargePageDescriptorType)
			unit = 1;
		else if (descriptorType == kSmallPageDescriptorType)
			unit = 3;
		else
			unit = 6;
	}
	else if (descriptorType == kSectionDescriptorType)
	{
	//	no further lookup required
		ExitFIQAtomic();
		unit = 0;
	}
	else
	{
	//	invalid section
		ExitFIQAtomic();
		unit = 5;
	}

	return unit;
}


int
CheckVAddrRange(VAddr inVAddr, size_t inSize)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor;
	int		domain;

	EnterFIQAtomic();
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);			// start of range
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	domain = (level1Descriptor >> kDomainShift) & 0x0F;

	descriptorPtr = gPrimaryTable + ((inVAddr + inSize - 1) / kSectionSize) * sizeof(MMUDescriptor);	// end of range
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	if (domain != ((level1Descriptor >> kDomainShift) & 0x0F))			// must be in same domain!
		domain = -1;
	ExitFIQAtomic();

	return domain;
}

#pragma mark -

//	Generic SWI 13 callback
NewtonErr
AddPageTable(ULong inArg1, ULong inArg2, ULong inArg3)
{
	return noErr;	// Newton really does this!
}

//	Generic SWI 14 callback
NewtonErr
RemovePageTable(ULong inArg1, ULong inArg2)
{
	return -1;		// Newton really does this!
}

//	Generic SWI 15 callback
//	Seems kinda irrelevant
NewtonErr
ReleasePageTable(VAddr inVAddr)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor;

	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	return ((level1Descriptor & kDescriptorTypeMask) != kPageDescriptorType) ? kOSErrBadParameters : noErr;
}


NewtonErr
TransformSectionIntoPageTable(VAddr inVAddr)
{ return noErr; }

NewtonErr
TransformBigPageToPages(VAddr inVAddr)
{ return noErr; }

bool
CanUseBigPage(VAddr inVAddr)
{ return false; }


/*------------------------------------------------------------------------------
	Set a range of virtual memory into a domain.
	Args:		inDescriptors	Level One Descriptor table
				inVAddr			start of virtual address range
				inSize			Length of virtual address range
				inDomain			domain to associate with that range
	Return:  --
------------------------------------------------------------------------------*/

void
PrimSetDomainRangeWithPageTable(PAddr inDescriptors, PAddr inVAddr, size_t inSize, ULong inDomain)
{
	PAddr		descriptorPtr = inDescriptors + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	ULong		level1Descriptor = ((inDomain << kDomainShift) & kDomainMask)
										| kUpdateable
										| kInvalidDescriptorType;
	VAddr		endAddr;

	for (endAddr = inVAddr + inSize - 1; inVAddr < endAddr; inVAddr += kSectionSize)
	{
		StoreToPhysAddress(descriptorPtr, level1Descriptor);
		descriptorPtr += sizeof(MMUDescriptor);
	}
}


void
PrimSetDomainRange(PAddr inVAddr, size_t inSize, ULong inDomain)
{
	PAddr		descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	ULong		level1Descriptor = ((inDomain << kDomainShift) & kDomainMask)
										| kUpdateable
										| kInvalidDescriptorType;
	VAddr		endAddr;

	EnterFIQAtomic();
	for (endAddr = inVAddr + inSize - 1; inVAddr < endAddr; inVAddr += kSectionSize)
	{
		StoreToPhysAddress(descriptorPtr, level1Descriptor);
		descriptorPtr += sizeof(MMUDescriptor);
	}
	ExitFIQAtomic();
}


void
PrimClearDomainRange(PAddr inVAddr, size_t inSize)
{
	PAddr		descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	ULong		level1Descriptor =  kUpdateable
										| kInvalidDescriptorType;
	VAddr		endAddr;

	EnterFIQAtomic();
	for (endAddr = inVAddr + inSize - 1; inVAddr < endAddr; inVAddr += kSectionSize)
	{
		StoreToPhysAddress(descriptorPtr, level1Descriptor);
		descriptorPtr += sizeof(MMUDescriptor);
	}
	ExitFIQAtomic();
}


/*------------------------------------------------------------------------------
	Set up a Level Two descriptor table.
	Args:		inDescriptors	Level One Descriptor table
				inVAddr			virtual page address
				inPageTable		address of Level Two descriptor table
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
AddPTableWithPageTable(PAddr inDescriptors, VAddr inVAddr, PAddr inPageTable)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor;

	descriptorPtr = inDescriptors + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	StoreToPhysAddress(descriptorPtr,  (level1Descriptor & kPageDescriptorAccessMask)
												| (inPageTable & kPageTableBaseMask)
												| kPageDescriptorType);

	return noErr;
}


NewtonErr
AddPTable(VAddr inVAddr, PAddr inPageTable, bool inClear)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor;

	EnterFIQAtomic();
	if (inClear)
		ZeroPhysSubPage(inPageTable);
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	StoreToPhysAddress(descriptorPtr,  (level1Descriptor & kPageDescriptorAccessMask)
												| (inPageTable & kPageTableBaseMask)
												| kPageDescriptorType);
	ExitFIQAtomic();

	return noErr;
}

#pragma mark -

void
CleanPageInDCache(VAddr inVAddr)
{
/*
		MOV	R2, R0
		MRC	Internal, R0, CR0
		BIC	R0, R0, #&0F
		EOR	R0, R0, #&44000000
		EOR	R0, R0, #&00010000
		EORS	R0, R0, #&0000A100
		MOVNE	PC, LK					; only applies if CPU = D-1-A10

		MOV	R1, #&FFFFFF00
		BIC	R1, R1, #&00000F00
		AND	R2, R2, R1
		ADD	R1, R2, #&00001000
L11:  MCR	Internal, R2, CR7
		MCR	Internal, R2, CR7
		MCR	Internal, R2, CR7
		ADD	R2, R2, #&20
		TEQ	R2, R1
		BNE	L11
		MOV	PC,LK
*/
}

NewtonErr
CleanPageInIandDCacheSWIGlue(ULong inArg1)
{ return noErr; }

void
CleanDCandFlushICSWIGlue(void)
{ }

NewtonErr
CleanPageInDCSWIGlue(ULong inArg1)
{ return noErr; }

NewtonErr
CleanRangeInDCSWIGlue(ULong inArg1, ULong inArg2)
{ return noErr; }


void
PurgePageFromTLB(VAddr inVAddr)
{
/*
		MRC	Internal, R0, CR0
		BIC	R0, R0, #&0F
		EOR	R0, R0, #&44000000
		EOR	R0, R0, #&00010000
		EORS	R0, R0, #&0000A100
												; only applies if CPU = D-1-A10
		MCREQ	Internal, R0, CR8
		MCREQ	Internal, R0, CR8
		MCRNE	Internal, R0, Fault Address
		MOV	PC,LK
*/
}

#pragma mark -

/*------------------------------------------------------------------------------
	Add a permissions mapping.
	Args:		inVAddr			virtual page address
				inPerm			new permissions for that page
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
AddSecPerm(VAddr inVAddr, Perm inPerm)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor;

	EnterFIQAtomic();
	FlushDCache();
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	StoreToPhysAddress(descriptorPtr, (level1Descriptor & ~kSectionPermMask)	// scrub existing permissions
												| (inPerm << kSectionPermShift));			// use new permissions
	ExitFIQAtomic();

	return noErr;
}


NewtonErr
AddBigPgPerm(VAddr inVAddr, Perm inPerm)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor, level2Descriptor;
	int		i;

	EnterFIQAtomic();
	FlushDCache();
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorPtr = (level1Descriptor & kPageTableBaseMask) + (((inVAddr & kLargePageBaseMask) / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
	// there are 16 big pages to a section
	for (i = 0; i < 16; ++i, ++descriptorPtr)
	{
		level2Descriptor = LoadFromPhysAddress(descriptorPtr);
		StoreToPhysAddress(descriptorPtr, (level2Descriptor & ~kPermissionMask)	// scrub existing permissions
													| gLevel2PermissionBits[inPerm]);		// use new permissions
	}
	ExitFIQAtomic();

	return noErr;
}


NewtonErr
AddBigSubPgPerm(VAddr inVAddr, Perm inPerm)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor, level2Descriptor;
	ULong		permMask, permBits;
	int		i, permShift;

	EnterFIQAtomic();
	FlushDCache();
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorPtr = (level1Descriptor & kPageTableBaseMask) + (((inVAddr & kLargePageBaseMask) / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
	permShift = kPermissionShift + (((inVAddr / kBigSubPageSize) & 0x03) * 2);
	permMask = 0x03 << permShift;
	permBits = inPerm << permShift;
	// there are 16 pages to a big page
	for (i = 0; i < 16; ++i, ++descriptorPtr)
	{
		level2Descriptor = LoadFromPhysAddress(descriptorPtr);
		StoreToPhysAddress(descriptorPtr, (level2Descriptor & ~permMask)	// scrub existing permissions
													| permBits);							// use new permissions
	}
	ExitFIQAtomic();

	return noErr;
}


NewtonErr
AddPgPerm(VAddr inVAddr, Perm inPerm)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor, level2Descriptor;

	EnterFIQAtomic();
	CleanPageInDCache(inVAddr);
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorPtr = (level1Descriptor & kPageTableBaseMask) + ((inVAddr / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
	level2Descriptor = LoadFromPhysAddress(descriptorPtr);
	StoreToPhysAddress(descriptorPtr, (level2Descriptor & ~kPermissionMask)	// scrub existing permissions
												| gLevel2PermissionBits[inPerm]);		// use new permissions
	ExitFIQAtomic();

	return noErr;
}


NewtonErr
AddSubPgPerm(VAddr inVAddr, Perm inPerm)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor, level2Descriptor;
	ULong		permMask, permBits;
	int		permShift;

	EnterFIQAtomic();
	CleanPageInDCache(inVAddr);
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorPtr = (level1Descriptor & kPageTableBaseMask) + ((inVAddr / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
	permShift = kPermissionShift + (((inVAddr / kSubPageSize) & 0x03) * 2);
	permMask = 0x03 << permShift;
	permBits = inPerm << permShift;
	level2Descriptor = LoadFromPhysAddress(descriptorPtr);
	StoreToPhysAddress(descriptorPtr, (level2Descriptor & ~permMask)	// scrub existing permissions
												| permBits);							// use new permissions
	ExitFIQAtomic();

	return noErr;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Remove a permissions mapping.
	Args:		inVAddr			virtual page address
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
RemoveSecPerm(VAddr inVAddr)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor;

	EnterFIQAtomic();
	FlushDCache();
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	StoreToPhysAddress(descriptorPtr, level1Descriptor & ~kSectionPermMask);
	ExitFIQAtomic();

	return noErr;
}


NewtonErr
RemoveBigPgPerm(VAddr inVAddr)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor, level2Descriptor;
	int		i;

	EnterFIQAtomic();
	FlushDCache();
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorPtr = (level1Descriptor & kPageTableBaseMask) + (((inVAddr & kLargePageBaseMask) / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
	level2Descriptor = LoadFromPhysAddress(descriptorPtr);
	// there are 16 big pages to a section
	for (i = 0; i < 16; ++i, ++descriptorPtr)
	{
		StoreToPhysAddress(descriptorPtr, (level2Descriptor & ~kPermissionMask));
	}
	ExitFIQAtomic();

	return noErr;
}


NewtonErr
RemoveBigSubPgPerm(VAddr inVAddr)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor, level2Descriptor;
	ULong		permMask;
	int		i, permShift;

	EnterFIQAtomic();
	FlushDCache();
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorPtr = (level1Descriptor & kPageTableBaseMask) + (((inVAddr & kLargePageBaseMask) / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
	level2Descriptor = LoadFromPhysAddress(descriptorPtr);
	permShift = kPermissionShift + (((inVAddr / kBigSubPageSize) & 0x03) * 2);
	permMask = 0x03 << permShift;
	// there are 16 big pages to a section
	for (i = 0; i < 16; ++i, ++descriptorPtr)
	{
		StoreToPhysAddress(descriptorPtr, (level2Descriptor & ~permMask));
	}
	ExitFIQAtomic();

	return noErr;
}


NewtonErr
RemovePgPerm(VAddr inVAddr)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor, level2Descriptor;

	EnterFIQAtomic();
	CleanPageInDCache(inVAddr);
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorPtr = (level1Descriptor & kPageTableBaseMask) + ((inVAddr / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
	level2Descriptor = LoadFromPhysAddress(descriptorPtr);
	StoreToPhysAddress(descriptorPtr, level2Descriptor & ~kPermissionMask);
	ExitFIQAtomic();

	return noErr;
}


NewtonErr
RemoveSubPgPerm(VAddr inVAddr)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor, level2Descriptor;
	ULong		permMask;
	int		permShift;

	EnterFIQAtomic();
	CleanPageInDCache(inVAddr);
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorPtr = (level1Descriptor & kPageTableBaseMask) + ((inVAddr / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
	permShift = kPermissionShift + (((inVAddr / kSubPageSize) & 0x03) * 2);
	permMask = 0x03 << permShift;
	level2Descriptor = LoadFromPhysAddress(descriptorPtr);
	StoreToPhysAddress(descriptorPtr, (level2Descriptor & ~permMask));
	ExitFIQAtomic();

	return noErr;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Add address mapping for flash memory.
	Args:		inVAddr			virtual page address
				inPhysAddr		physical address to map that page to
				inDomain
				inPerm
				inCacheable		
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
AddNewSecPNJT(VAddr inVAddr, PAddr inPhysAddr, ULong inDomain, Perm inPerm, bool inCacheable)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor;

	descriptorPtr = GetPrimaryTablePhysBase() + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = ((inDomain << kDomainShift) & kDomainMask);	// set up domain
	level1Descriptor |= kUpdateable;											// huh?
	level1Descriptor = (level1Descriptor & ~kSectionPermMask)		// scrub existing permissions
							| (inPerm << kSectionPermShift);				// use new permissions
	StoreToPhysAddress(descriptorPtr, (level1Descriptor & 0x0FE0)	// (kSectionPermMask | kDomainMask | 0x0100)
												| (inPhysAddr & kSectionBaseMask)
												| (inCacheable ? (kUpdateable | kCacheable | kBufferable) : 0)
												| kSectionDescriptorType);
	return noErr;
}


/*------------------------------------------------------------------------------
	Add a physical address mapping.
	Args:		inVAddr			virtual page address
				inPhysAddr		physical address to map that page to
				inCacheable		
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
AddSecP(VAddr inVAddr, PAddr inPhysAddr, bool inCacheable)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor;

	EnterFIQAtomic();
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	if ((level1Descriptor & kSectionBaseMask) != inPhysAddr)
		FlushDCache();
	StoreToPhysAddress(descriptorPtr,  (level1Descriptor & 0x0FE0)	// (kSectionPermMask | kDomainMask | 0x0100)
												| (inPhysAddr & kSectionBaseMask)
												| (inCacheable ? (kUpdateable | kCacheable | kBufferable) : 0)
												| kSectionDescriptorType);
	ExitFIQAtomic();

	return noErr;
}


NewtonErr
AddBigPgP(VAddr inVAddr, PAddr inPhysAddr, bool inCacheable)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor, level2Descriptor;
	ArrayIndex	i;
	bool		isFirstTime;

	EnterFIQAtomic();
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorPtr = (level1Descriptor & kPageTableBaseMask) + (((inVAddr & kLargePageBaseMask) / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
	isFirstTime = true;
	// there are 16 big pages to a section
	for (i = 0; i < 16; ++i, ++descriptorPtr)
	{
		level2Descriptor = LoadFromPhysAddress(descriptorPtr);
		if (isFirstTime
		&& (level2Descriptor & kLargePageBaseMask) != inPhysAddr)
		{
			FlushDCache();
			isFirstTime = false;
		}
		StoreToPhysAddress(descriptorPtr,  (level2Descriptor & kPermissionMask)
													| (inPhysAddr & kLargePageBaseMask)
													| (inCacheable ? (kCacheable | kBufferable) : 0)
													| kLargePageDescriptorType);
	}
	ExitFIQAtomic();

	return noErr;
}


NewtonErr
AddPgP(VAddr inVAddr, PAddr inPhysAddr, bool inCacheable)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor, level2Descriptor;

	EnterFIQAtomic();
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorPtr = (level1Descriptor & kPageTableBaseMask) + ((inVAddr / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
	level2Descriptor = LoadFromPhysAddress(descriptorPtr);
	if ((level2Descriptor & kSmallPageBaseMask) != inPhysAddr)
		CleanPageInDCache(inVAddr);	// address has changed
	StoreToPhysAddress(descriptorPtr,  (level2Descriptor & kPermissionMask)	// keep existing permissions
												| (inPhysAddr & kSmallPageBaseMask)		// use new physical base
												| (inCacheable ? (kCacheable | kBufferable) : 0)
												| kSmallPageDescriptorType);
	ExitFIQAtomic();

	return noErr;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Remove a physical address mapping.
	Args:		inVAddr			virtual page address
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
RemoveSecP(VAddr inVAddr)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor;

	EnterFIQAtomic();
	FlushDCache();
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	StoreToPhysAddress(descriptorPtr, level1Descriptor & (kDomainMask | 0x200));	// sic
	ExitFIQAtomic();

	return noErr;
}


NewtonErr
RemoveBigPgP(VAddr inVAddr)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor, level2Descriptor;
	ArrayIndex	i;

	EnterFIQAtomic();
	FlushDCache();
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorPtr = (level1Descriptor & kPageTableBaseMask) + (((inVAddr & kLargePageBaseMask) / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
	level2Descriptor = LoadFromPhysAddress(descriptorPtr);
	// there are 16 big pages to a section
	for (i = 0; i < 16; ++i, ++descriptorPtr)
	{
		StoreToPhysAddress(descriptorPtr, (level2Descriptor & kPermissionMask));
	}
	ExitFIQAtomic();

	return noErr;
}


NewtonErr
RemovePgP(VAddr inVAddr)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor, level2Descriptor;

	EnterFIQAtomic();
	CleanPageInDCache(inVAddr);
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorPtr = (level1Descriptor & kPageTableBaseMask) + ((inVAddr / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
	level2Descriptor = LoadFromPhysAddress(descriptorPtr);
	StoreToPhysAddress(descriptorPtr, level2Descriptor & kPermissionMask);
	ExitFIQAtomic();

	return noErr;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Add a physical address and permissions mapping.
	Args:		inVAddr			virtual page address
				inPerm			
				inPhysAddr		physical address to map that page to
				inCacheable		
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
AddPgPAndPerm(VAddr inVAddr, ULong inPerm, PAddr inPhysAddr, bool inCacheable)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor;

	EnterFIQAtomic();
	CleanPageInDCache(inVAddr);
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorPtr = (level1Descriptor & kPageTableBaseMask) + ((inVAddr / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
	StoreToPhysAddress(descriptorPtr,  (inPhysAddr & kSmallPageBaseMask)			// use new physical base
												| ((inPerm & 0xFF) << kPermissionShift)	// use new permissions
												| (inCacheable ? (kCacheable | kBufferable) : 0)
												| kSmallPageDescriptorType);
	PurgePageFromTLB(inVAddr);
	ExitFIQAtomic();

	return noErr;
}


/*------------------------------------------------------------------------------
	Add a physical address and permissions mapping in the sepcified Level One
	Descriptor table.
	Args:		inDescriptors
				inVAddr			virtual page address
				inPerm			
				inPhysAddr		physical address to map that page to
				inCacheable		
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
AddPgPAndPermWithPageTable(PAddr inDescriptors, VAddr inVAddr, ULong inPerm, PAddr inPhysAddr, bool inCacheable)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor;

//	EnterFIQAtomic();		// no need because interrupts aren’t enabled yet
	CleanPageInDCache(inVAddr);
	descriptorPtr = inDescriptors + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorPtr = (level1Descriptor & kPageTableBaseMask) + ((inVAddr / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
	StoreToPhysAddress(descriptorPtr,  (inPhysAddr & kSmallPageBaseMask)			// use new physical base
												| ((inPerm & 0xFF) << kPermissionShift)	// use new permissions
												| (inCacheable ? (kCacheable | kBufferable) : 0)
												| kSmallPageDescriptorType);
	PurgePageFromTLB(inVAddr);
//	ExitFIQAtomic();

	return noErr;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Remove a physical address and permissions mapping.
	Args:		inVAddr			virtual page address
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
RemovePgPAndPerm(VAddr inVAddr)
{
	PAddr		descriptorPtr;
	ULong		level1Descriptor, level2Descriptor;

	EnterFIQAtomic();
	CleanPageInDCache(inVAddr);
	descriptorPtr = gPrimaryTable + (inVAddr / kSectionSize) * sizeof(MMUDescriptor);
	level1Descriptor = LoadFromPhysAddress(descriptorPtr);
	descriptorPtr = (level1Descriptor & kPageTableBaseMask) + ((inVAddr / kPageSize) & 0xFF) * sizeof(MMUDescriptor);
	level2Descriptor = LoadFromPhysAddress(descriptorPtr);
	StoreToPhysAddress(descriptorPtr, 0);
	PurgePageFromTLB(inVAddr);
	ExitFIQAtomic();

	return noErr;
}


NewtonErr
RememberMappingUsingPAddr(VAddr inVAddr, ULong inPerm, PAddr inPhysAddr, bool inCacheable)
{
	if (IsSuperMode())
		return AddPgPAndPermWithPageTable(GetPrimaryTablePhysBaseAfterGlobalsInited(), inVAddr, inPerm, inPhysAddr, inCacheable);

	return GenericSWI(kRememberMappingUsingPAddr, inVAddr, inPerm, inPhysAddr, inCacheable);	// actually RememberMappingUsingPAddrGlue which preserves R4
}


NewtonErr
PrimRemovePMappings(PAddr inBase, size_t inSize)
{
	size_t	size;
	PAddr		physAddr;
	PAddr		limit = inBase + inSize;
	VAddr		vPage;
/*
	for (vPage = 0; vPage < 0xC0000000; vPage += size)
	{
		size = VtoSizeWithP(vPage, &physAddr);
		if (physAddr >= inBase && physAddr < limit)
			RemovePgPAndPerm(vPage);
	}
*/
	return noErr;
}


NewtonErr
RemovePMappings(PAddr inBase, size_t inSize)
{
	if (IsSuperMode())
		return PrimRemovePMappings(inBase, inSize);

	return GenericSWI(kRemovePMappings, inBase, inSize);
}

