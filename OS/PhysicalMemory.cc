/*
	File:		PhysicalMemory.cc

	Contains:	Physical memory access.

	Written by:	Newton Research Group.
*/

#include "VirtualMemory.h"
#include "MMU.h"
#include "PhysicalMemory.h"
#include "RAMInfo.h"
#include "PageManager.h"
#include "MemObjManager.h"
#include "UserTasks.h"
#include "SafeHeap.h"
#include "OSErrors.h"

#include "sys/malloc.h"
#include <new>

extern "C" ULong	LowLevelGetCPUType(void);
extern "C" void	LowLevelProcSpeed(ULong inReps);

/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern "C" {
PAddr	GetPrimaryTablePhysBaseAfterGlobalsInited(void);
PAddr GetPrimaryTablePhysBaseMMUOff(SGlobalsThatLiveAcrossReboot * info);
PAddr GetPrimaryTablePhysBase(void);
}

void		SetPGTLARAddr(KernelArea * info, SGlobalsThatLiveAcrossReboot * inGlobals);

ULong		MapTable(ULong inLastSubPage, ULong inFirstSubPage);
ULong		SizeTable(ULong inLastSubPage, ULong inFirstSubPage);
void		UseROMJumpTables(void);


/*------------------------------------------------------------------------------
	M e m o r y   M a p   M P 2 1 0 0

	Physical						Virtual

	00000000 - 007FFFFF		=								8M ROM
	00800000 - 00FFFFFF		=								8M optional licensee ROM (unused)
	...
	04000000 - 043FFFFF		0C000000 - 0C3FFFFF		4M DRAM bank 1 (heaps)
	...
	08000000 - 083FFFFF										4M DRAM bank 2 (optional)
	...
	0F000000 - 0FFFFFFF										hardware registers
	...
	34000000 - 343FFFFF		34000000 - 37FFFFFF		4M -> 64M internal flash RAM
	...
	60000000 - 603FFFFF		60000000 - 63FFFFFF		4M -> 64M package flash RAM
	...
	70000000 - 73FFFFFF										64M PC card slot 1
	...
	74000000 - 77FFFFFF										64M PC card slot 2
	...
	E0000000 - E0012BFF		=								LCD

------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	K e r n e l   G l o b a l s   M a p

	super stack 0x04004000 : 0x08004000 physical; 1K in size

	Base virtual address = 0C000000

	0C000400  int  krnlStack[];
	0C002C00  int  irqStack[0x0200];
	0C003400  int  fiqStack[0x0600];
	0C004000  int  svcStack[0x0300];		// ??
	0C004C00  int  abtStack[0x0500];
	0C006000  int  undStack[0x0500];
	0C007400  int  userStack[];

	0C100000  kernel domain heap base
	0C100800  kernel globals base

	0C1FFFFF  kernel domain heap limit
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

extern ULong *		gPrimaryTable;
extern Heap			gKernelHeap;
/*
int			krnlStack[0x0400];	// 0C00???? donÕt actually know size
int			irqStack[0x0200];		// 0C002C00
int			fiqStack[0x0300];		// 0C003400
int			svcStack[0x0300];		// 0C004000 ??
int			abtStack[0x0500];		// 0C004C00
int			undStack[0x0500];		// 0C006000
int			userStack[0x0400];	// 0C007400 donÕt actually know size
*/
KernelArea	gKernelArea;			// 0C100800

size_t		gRAMSize;				// 0C1008D8
#if defined(__i386__)
ULong			gMainCPUType = 'i386';
#else
ULong			gMainCPUType = 'x86';				// 0C1008DC
#endif
float			gMainCPUClockSpeed = 1600.0;		// 0C1008E0	1.6GHz MacBook Air

VAddr			gNextKernelVAddr;		// 0C101168
ULong			gNextPageIndex;		// 0C10116C

ULong			gFirstManagedPageIndex;	// 0C104F54

#if !defined(correct)
PAddr			gPhysRAM;
#endif

/*------------------------------------------------------------------------------
	R O M   o p e r a t i o n s
------------------------------------------------------------------------------*/

bool
ROMBanksAreSplit(void)
{
#if defined(correct)
	ULong *  bank1 = 0*MByte;
	ULong *  bank2 = 4*MByte;
	for (ArrayIndex i = 63; i > 0; --i)
		if (*bank1++  != *bank2++)
			return false;
	return true;
#else
	return false;
#endif
}

#pragma mark -

/*------------------------------------------------------------------------------
	M e m o r y   s i z e   o p e r a t i o n s
------------------------------------------------------------------------------*/

#if defined(forFramework)
// these are the only functions in this file we need for the framework
size_t
InternalRAMInfo(int inSelector, size_t inAlign)
{
	switch (inSelector)
	{
//	case kSystemRAMAlloc: return 0;
//	case kRAMStoreAlloc: return 0;
	case kFramesHeapAlloc:
#if defined(forNTK)
		return 4096*KByte;
#else
		return 256*KByte;
#endif
	case kRDMCacheAlloc: return 16*KByte;
	}
	return 0;
}

size_t
InternalStoreInfo(int inSelector)
{
	if (inSelector == 0)
		return 4*MByte;	// size of Internal flash store
	return 0;
}

#else
// provide full physical memory
/*------------------------------------------------------------------------------
	Return the amount of RAM installed on this Newton device.
	Args:		--
	Return:  the RAM size
------------------------------------------------------------------------------*/

extern "C" size_t
GetRAMSize(void)
{
	return CRAMTable::getRAMSize();
}


/*------------------------------------------------------------------------------
	Return the amount of RAM allocated to a particular resource.
	Args:		inSelector	RAMAllocSelector; see ROMExtension.h
				inAvail		available RAM
				inAlign		(sub) page size
	Return:  the RAM limit
------------------------------------------------------------------------------*/

size_t
ComputeRAMLimit(int inSelector, long inAvail, long inAlign)
{
// patch
//	if (inSelector == kFramesHeapAlloc && inAvail >= 0x00200000)	// 2MB
//		return 0x0007A120;
// end patch

	size_t	limit = 0;
	size_t	length;
	RAMAllocTable *	ramAlloc = (RAMAllocTable *)GetLastRExConfigEntry(kRAMAllocationTag, &length);
	if (ramAlloc != NULL && ramAlloc->count > inSelector)
	{
		// table has an entry for inSelector
		RAMAllocEntry *	entry = ramAlloc->table + inSelector;
		limit = inAvail * entry->pct / 1024;
		if (inAlign != 0)
			limit = ALIGN(limit, inAlign);
		if (entry->min != 0 && entry->min > limit)
			limit = entry->min;
		else if (entry->max != 0 && entry->max < limit)
			limit = entry->max;
	}
	return limit;
}


/*------------------------------------------------------------------------------
	Return info about the internal store.
	Args:		inSelector	0 => flash store size
								1 => user store size - called by InternalRAMInfo()
								2 => size of RAM store - called by WipeInternalStore()
								3 => address of RAM store - called by WipeInternalStore()
	Return:  the RAM required
------------------------------------------------------------------------------*/
size_t	g0C106520 = 4*MByte;

size_t
InternalStoreInfo(int inSelector)
{
	size_t	reqd = 0;

	if (inSelector == 3)
		reqd = 0x01100000;	// 17M

	else if (inSelector == 0)
		reqd = g0C106520;

	else
	{
		if (g0C106520 == 0)
			reqd = 0x01100000;

		else
		{
			size_t	ramSize;
			size_t	storeSize;
			size_t	byteSize = 0;
			for (ArrayIndex i = 0; i < kMaximumMemoryPartitions; ++i)
			{
				if (gGlobalsThatLiveAcrossReboot.fBank[i].tag != kUndefined
				 && gGlobalsThatLiveAcrossReboot.fBank[i].laneCount < 4)
				 	byteSize = gGlobalsThatLiveAcrossReboot.fBank[i].ramSize;
			}
			if (byteSize != 0 && byteSize <= 0x00020000)	// 128K
				storeSize = 0x00008000;	// 32K
			else
			{
				ramSize = GetRAMSize();
				storeSize = ComputeRAMLimit(kRAMStoreAlloc, ramSize, kPageSize);
				if (ramSize - storeSize < 0x00080000)	// must leave >512K for heaps etc
					storeSize = ramSize - 0x00080000;
			}

			if (inSelector == 1)
				reqd = storeSize;

			else if (inSelector == 2)
				reqd = byteSize;
		}
	}

	return reqd;
}


/*------------------------------------------------------------------------------
	Return the amount of RAM available, ie not required by the OS.
	Args:		inSelector	RAMAllocSelector; see ROMExtension.h
				inAlign		kSubPageSize if for ROM domain manager, otherwise 0
	Return:  the RAM available
------------------------------------------------------------------------------*/

size_t
InternalRAMInfo(int inSelector, size_t inAlign)
{
	size_t size = GetRAMSize() - InternalStoreInfo(1);
	return (inSelector == kSystemRAMAlloc)	? size
														: ComputeRAMLimit(inSelector, size, inAlign);
}


/*------------------------------------------------------------------------------
	Return the amount of RAM installed on this Newton device
	(less system requirements).
	Not actually referenced anywhere.
	Args:		--
	Return:  the RAM size
------------------------------------------------------------------------------*/

Size
SystemRAMSize(void)
{
	return InternalRAMInfo(kSystemRAMAlloc, 0);
}


/*------------------------------------------------------------------------------
	Return the number of bytes currently available for allocation.
	Not actually referenced anywhere.
	Args:		--
	Return:  the size
------------------------------------------------------------------------------*/

Size
TotalSystemFree(void)
{
	return SystemFreePageCount() * kPageSize;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Return the amount of RAM installed in a bank.
	Args:		--
	Return:  the RAM size
------------------------------------------------------------------------------*/

size_t
SBankInfo::normalRAMSize(void)
{
	return (laneCount == 4) ? ramSize : 0;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C R A M T a b l e
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Initialize the RAM table.
	Args:		ioBank		array of SBankInfo to be initialized - the RAM table
	Return:  --
------------------------------------------------------------------------------*/

void
CRAMTable::init(SBankInfo ioBank[])
{
	for (ArrayIndex i = 0; i < kMaximumMemoryPartitions; ++i)
	{
		ioBank[i].ramPhysStart = 0;
		ioBank[i].ramSize = 0;
		ioBank[i].tag = kUndefined;
		ioBank[i].laneSize = 0;
		ioBank[i].laneCount = 0;
	}
}


/*------------------------------------------------------------------------------
	Add a bank of RAM to the RAM table.
	Args:		ioBank		array of SBankInfo - the RAM table
				info			bank to add
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
CRAMTable::add(SBankInfo ioBank[], SBankInfo * info)
{
	for (ArrayIndex i = 0; i < kMaximumMemoryPartitions; ++i)
	{
		if (ioBank[i].tag == kUndefined)
		{
		//	this entry hasnÕt been used yet
			ioBank[i] = *info;
#if !defined(correct)
			// use allocated memory
			ioBank[i].ramPhysStart = gPhysRAM;
#endif
			return noErr;
		}
	}
	return kOSErrRAMTableFull;
}


/*------------------------------------------------------------------------------
	Remove a bank of RAM from the RAM table.
	Args:		inArg1		
				inArg2		
				inArg3		
				inArg4		
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
CRAMTable::remove(PAddr inArg1, ULong inArg2, EBankDesignation inArg3, ULong inArg4)
{
	ArrayIndex	index, usedBankIndex;
	SBankInfo *	bank = gGlobalsThatLiveAcrossReboot.fBank;	// r6
	if (PAGEOFFSET(inArg1) != 0)
		return kOSErrBadParameters;
	
	for (usedBankIndex = 0; usedBankIndex < kMaximumMemoryPartitions; usedBankIndex++)
	{
		if (bank[usedBankIndex].tag == kUndefined)
			// we didnÕt find a 'krnl' bank
			break;
	}
	if (usedBankIndex == 0)
		return kOSErrUnableToSatisfyRequest;

	//sp-04
	ULong	r7 = 0;
	long	spm04 = 0;
	ULong	sp08 = ~(inArg4 - 1);
	for (index = 0; index < usedBankIndex; index++)	// r8
	{
		ULong	r9 = 0;
		ULong	r10 = 0;
		size_t	r0 = bank[index].normalRAMSize();
		PAddr		r1 = bank[index].ramPhysStart;
		if (bank[index].tag == kUndefined)
			break;
		if (bank[index].tag == 'krnl')
		{
			r7 = r1 + r0 - inArg1;
			if (inArg4 != 0)
				r7 &= sp08;
			if (r7 > r1)
			{
				if (inArg3 == kBank0)
				{
					r9 = (r1 == 0x04000000 || r1 == 0x08000000) ? r1 + 0x00010000 : r1;
					r10 = r0;
				}
				else if (inArg3 == kBank1)
				{
					r9 = 0x04010000;
					r10 = 0x08000000;
				}
				else if (inArg3 == kBank2)
				{
					r9 = 0x08010000;
					r10 = 0xFFFFFFFF;
				}
				if (r9 <= r7 && r7 < r10)
					goto L82;
			}
		}
	}
//79
	if (spm04 == 0)
		return kOSErrUnableToSatisfyRequest;
L82:
	// INCOMPLETE
//138
	bank[index].ramPhysStart = r7;
	bank[index].ramSize = inArg1;
	bank[index].tag = inArg2;
	bank[index].laneSize = 0;
	bank[index].laneCount = 0;
	return noErr;
}


/*------------------------------------------------------------------------------
	Return the physical address of a page in the RAM table.
	Args:		inPage		the page number
				inBank		array of SBankInfo - the RAM table
	Return:  physical address
------------------------------------------------------------------------------*/

PAddr
CRAMTable::getPPage(ULong inPage, SBankInfo * inBank)
{
	size_t	accumSize = 0;
	PAddr		pageBase = inPage * kPageSize;

	for (ArrayIndex i = 0; i < kMaximumMemoryPartitions; ++i)
	{
		if (inBank[i].tag == kUndefined)
			// we didnÕt find a 'krnl' bank
			break;
		if (inBank[i].tag == 'krnl')
		{
			accumSize += inBank[i].normalRAMSize();
			if (pageBase < accumSize)
			{
				// the page address lies within this bank - return its physical address
				accumSize -= inBank[i].normalRAMSize();
				return inBank[i].ramPhysStart + pageBase - accumSize;
			}
		}
	}
	return kIllegalPAddr;
}


/*------------------------------------------------------------------------------
	Return the physical address of a tagged page in the RAM table.
	Args:		inTag		the tag
	Return:  physical address
------------------------------------------------------------------------------*/

PAddr
CRAMTable::getPPageWithTag(ULong inTag)
{
	SBankInfo * bank = gGlobalsThatLiveAcrossReboot.fBank;

	for (ArrayIndex i = 0; i < kMaximumMemoryPartitions; ++i)
	{
		if (bank[i].tag == kUndefined)
			break;
		if (bank[i].tag == inTag)
			return bank[i].ramPhysStart;
	}
	return kIllegalPAddr;
}


/*------------------------------------------------------------------------------
	Return the amount of RAM installed in all banks on this Newton device.
	Args:		--
	Return:  the RAM size
------------------------------------------------------------------------------*/

size_t
CRAMTable::getRAMSize(void)
{
	size_t ramSize = 0;
	SBankInfo * bank = gGlobalsThatLiveAcrossReboot.fBank;

	for (ArrayIndex i = 0; i < kMaximumMemoryPartitions; ++i)
	{
		if (bank[i].tag == kUndefined)
			break;
		if (bank[i].tag == 'krnl')
			ramSize += bank[i].normalRAMSize();
	}
	return ramSize;
}


#pragma mark -
/*------------------------------------------------------------------------------
	K e r n e l   A r e a
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Set up info about ROM & RAM memory banks.
	Args:		inBank1Size		size of RAM bank 1
				inBank2Size		size of RAM bank 2
	Return:  pointer to persistent globals
------------------------------------------------------------------------------*/

SGlobalsThatLiveAcrossReboot *
GetBankInfo(size_t inBank1Size, size_t inBank2Size)
{
	SGlobalsThatLiveAcrossReboot *	pGlobals;
	SBankInfo	bank1, bank2;

	bank1.ramPhysStart = 0x04000000;
	bank1.ramSize = inBank1Size;
	bank1.tag  = 'krnl';
	bank1.laneSize  = 0;
	bank1.laneCount  = 4;

	bank2.ramPhysStart = 0x08000000;
	bank2.ramSize = inBank2Size;
	bank2.tag  = 'krnl';
	bank2.laneSize  = 0;
	bank2.laneCount  = 4;

	pGlobals = MakeGlobalsPtr(inBank1Size);
	SetPGTLARAddr((KernelArea *)GetSuperStacksPhysBase(inBank1Size), pGlobals);

	SBankInfo * bankInfo = pGlobals->fBank;
	CRAMTable::init(bankInfo);
	if (inBank1Size != 0)
		CRAMTable::add(bankInfo, &bank1);
	if (inBank2Size != 0)
		CRAMTable::add(bankInfo, &bank2);

	pGlobals->fROMBanksAreSplit = ROMBanksAreSplit();
	return pGlobals;
}


void
SetPGTLARAddr(KernelArea * info, SGlobalsThatLiveAcrossReboot * inGlobals)
{
	info->pGTLAR = inGlobals;
}


/*------------------------------------------------------------------------------
	Copy RAM bank table to a known physical address so we can access it easily
	before the MMU is available.
	Args:		info			kernel area at known phys address
	Return:  --
------------------------------------------------------------------------------*/

void
CopyRAMTableToKernelArea(KernelArea * info)
{
	SBankInfo * src = (SBankInfo *)info->ptr;
	SBankInfo * dst = info->bank;

	for (ArrayIndex i = 0; i < kMaximumMemoryPartitions; ++i)
		*dst++ = *src++;
}


/*------------------------------------------------------------------------------
	Determine whether critical globals have been initialized.
------------------------------------------------------------------------------*/

bool
AreGlobalsInitialized(void)
{
	return gKernelArea.num == 'true';
}

void
SetGlobalsInitialized(void)
{
	gKernelArea.num = 'true';
}


/*------------------------------------------------------------------------------
	The following functions provide a C interface to C++ class member functions.
	NOTE  The Level One Descriptor table is also known as the Primary Table.
------------------------------------------------------------------------------*/

#pragma mark -

/*------------------------------------------------------------------------------
	Return the physical address of a page.
	Args:		--
	Return:  pageÕs physical address
------------------------------------------------------------------------------*/

PAddr
GetPhysPage(ULong inPage)
{
	return CRAMTable::getPPage(inPage, AreGlobalsInitialized()
												  ? gGlobalsThatLiveAcrossReboot.fBank
												  : gKernelArea.bank);
}


/*------------------------------------------------------------------------------
	Return the physical address of the Level One Descriptor table.
	This function can only be called once gGlobalsThatLiveAcrossReboot has been
	set up, but while the MMU is still off.
	Args:		--
	Return:  Primary Table physical address
------------------------------------------------------------------------------*/

PAddr
GetPrimaryTablePhysBaseAfterGlobalsInited(void)
{
	return CRAMTable::getPPage(0, gGlobalsThatLiveAcrossReboot.fBank);
}


/*------------------------------------------------------------------------------
	Return the physical address of the Level One Descriptor table.
	This function can only be called while the MMU is off, but with a temporary
	gGlobalsThatLiveAcrossReboot.
	Args:		--
	Return:  Primary Table physical address
------------------------------------------------------------------------------*/

PAddr
GetPrimaryTablePhysBaseMMUOff(SGlobalsThatLiveAcrossReboot * info)
{
	return CRAMTable::getPPage(0, info->fBank);
}


/*------------------------------------------------------------------------------
	Return the physical address of the Level One Descriptor table.
	Args:		--
	Return:  Primary Table physical address
------------------------------------------------------------------------------*/

PAddr
GetPrimaryTablePhysBase(void)
{
	return GetPhysPage(0);
}


#pragma mark -
/*------------------------------------------------------------------------------
	S t a c k s
------------------------------------------------------------------------------*/

PAddr
GetSuperStacksPhysBase(ULong inRAMBank1Size)
{
#if defined(correct)
	return (inRAMBank1Size != 0) ? 0x04004000 : 0x08004000;
#else
	return gPhysRAM + 0x00004000;
#endif
}


void
InitSpecialStacks(void)
{
#if defined(correct)
 	SetFIQStack(fiqStack);
	SetAbortStack(abtStack);
	SetUndefStack(undStack);
	SetIRQStack(irqStack);
	SetUserStack(userStack);
#endif
}


VAddr
GetKernelStackVirtualTop(void)
{
#if defined(correct)
	return svcStack;
#else
	return 0;
#endif
}


/*------------------------------------------------------------------------------
	K e r n e l   D o m a i n
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Return the virtual base address of the kernel heap.
	Args:		--
	Return:  virtual address
	Linkage:	C/assembler
------------------------------------------------------------------------------*/
size_t GetKernelDomainHeapSize(void);

VAddr
GetKernelDomainHeapBase(void)
{
#if defined(correct)
	return 0x0C100000;
#else
	return gPhysRAM;	// virtual addresses are physical addresses
#endif
}


/*------------------------------------------------------------------------------
	Return the virtual size of the kernel heap.
	Args:		--
	Return:  size
	Linkage:	C/assembler
------------------------------------------------------------------------------*/

size_t
GetKernelDomainHeapSize(void)
{
	return 1*MByte;
}


/*------------------------------------------------------------------------------
	Return the next available unmapped VAddr page in the kernel heap.
	Args:		--
	Return:  virtual address
------------------------------------------------------------------------------*/

VAddr
GetNextKernelVAddr(void)
{
	VAddr	base = GetKernelDomainHeapBase();
	VAddr	limit = base + GetKernelDomainHeapSize();
	bool	beenRoundAlready = false;

	do
	{
		gNextKernelVAddr += kPageSize;
		if (gNextKernelVAddr >= limit)
		{
			if (beenRoundAlready)
				Reboot(kOSErrSorrySystemError);
			beenRoundAlready = true;
			gNextKernelVAddr = base;
		}
	} while (VtoP(gNextKernelVAddr) != kIllegalPAddr);
	return gNextKernelVAddr;
}


/*------------------------------------------------------------------------------
	Return the page index of the kernel globals.
	Args:		--
	Return:  page index
------------------------------------------------------------------------------*/

ULong
GetKernelGlobalsPageIndex(void)
{
	return 7;	// 0x00007000
}


/*------------------------------------------------------------------------------
	Zero a subpage at a physical address.
	(Should be in assembler for optimum performance.)
	Args:		inAddr		physical address
	Return:  --
	Linkage:	assembler
------------------------------------------------------------------------------*/

extern "C" void
ZeroPhysSubPage(PAddr inAddr)
{
	struct ZeroBlock
	{
		long  word0;
		long  word1;
		long  word2;
		long  word3;
	};

	ZeroBlock *	p = (ZeroBlock *)inAddr;
	for (int i = kSubPageSize; i > 0; i -= sizeof(ZeroBlock), p++)
	{
		p->word0 = 0;
		p->word1 = 0;
		p->word2 = 0;
		p->word3 = 0;
	}
}


void
PhysSubPageCopy(PAddr inFromAddr, PAddr inToAddr)
{
// INCOMPLETE
}


/*------------------------------------------------------------------------------
	Zero a page at a physical address.
	Args:		inAddr		physical address
	Return:  --
------------------------------------------------------------------------------*/

void
ZeroPhysPage(PAddr inAddr)
{
	ZeroPhysSubPage(inAddr);
	ZeroPhysSubPage(inAddr + 1*KByte);
	ZeroPhysSubPage(inAddr + 2*KByte);
	ZeroPhysSubPage(inAddr + 3*KByte);
}


/*------------------------------------------------------------------------------
	Initialize the kernel heap page table.
	Args:		--
	Return:  --
	Linkage:	C/assembler
------------------------------------------------------------------------------*/

void
InitKernelHeapArea(void)
{
	PAddr level1Descriptors = GetPrimaryTablePhysBase();
	PAddr heapSubPage = GetPhysPage(6) + 2*kSubPageSize;
	ZeroPhysSubPage(heapSubPage);
	PrimSetDomainRangeWithPageTable(level1Descriptors, GetKernelDomainHeapBase(), GetKernelDomainHeapSize(), kKernelDomainHeapDomainNumber);
	AddPTableWithPageTable(level1Descriptors, GetKernelDomainHeapBase(), heapSubPage);
}


/*------------------------------------------------------------------------------
	Map a range of memory - to be used for kernel globals - to virtual memory.
	Args:		inVAddr			virtual base address of memory range
				inSize			size of memory range
				inPerm			permissions of subpages already used withinÉ
				inPageIndex		page to map to
				outNextAddr		address of start of next range
				outNextPage		page of that address
				outPerm			subpage permissions within that page
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
MapInKernelGlobals(VAddr inVAddr, int inSize, ULong inPerm, ULong inPageIndex,
						 VAddr * outNextAddr, ULong * outNextPage, ULong * outPerm)
{
	NewtonErr err = noErr;
	XTRY
	{
		ULong		lastSubPage, firstSubPage;
		ULong		perm;
		size_t	mappedSize;
		while (inSize > 0)
		{
			// determine subpages within the page
			if (inSize >= kPageSize)
				lastSubPage = 3;	// we need to fill all subpages
			else
			{
				lastSubPage = ((inSize >= 1) ? inSize - 1 : 0) / kSubPageSize;
			}

			firstSubPage = PAGEOFFSET(inVAddr) / kSubPageSize;
			perm = MapTable(lastSubPage, firstSubPage);
			mappedSize = SizeTable(lastSubPage, firstSubPage) * kSubPageSize;
			XFAIL(err = AddPgPAndPermWithPageTable(GetPrimaryTablePhysBase(), TRUNC(inVAddr,kPageSize), perm | inPerm, GetPhysPage(inPageIndex), true))
			inVAddr += mappedSize;
			inSize -= mappedSize;
			inPageIndex++;
			inPerm = 0;
		}
		*outPerm = perm;
		*outNextAddr = inVAddr;
		*outNextPage = inPageIndex;
	}
	XENDTRY;
	return err;
}


/*------------------------------------------------------------------------------
	Calculate the permissions mask for a range of subpages.
	Args:		inLastSubPage		index of last subpage
				inFirstSubPage		index of first subpage
	Return:	permissions - 2 bits set per page required
------------------------------------------------------------------------------*/

ULong
MapTable(ULong inLastSubPage, ULong inFirstSubPage)
{
	ULong	perms;
	inFirstSubPage *= 2;
	switch (inLastSubPage)
	{
	case 0:
		perms = (0x03 << inFirstSubPage);
		break;
	case 1:
		perms = (0x0F << inFirstSubPage) & 0xFF;
		break;
	case 2:
		perms = (0x3F << inFirstSubPage) & 0xFF;
		break;
	case 3:
		perms = (0xFF << inFirstSubPage) & 0xFF;
		break;
	}
	return perms;
}


/*------------------------------------------------------------------------------
	Calculate the number of subpages that will fit an aligned page.
	Args:		inLastSubPage		index of last subpage
				inFirstSubPage		index of first subpage
	Return:	number of subpages
------------------------------------------------------------------------------*/

ULong
SizeTable(ULong inLastSubPage, ULong inFirstSubPage)
{
	ULong	num;
	switch (inLastSubPage)
	{
	case 0:
		num = 1;
		break;
	case 1:
		num = (inFirstSubPage == 3) ? 1 : 2;
		break;
	case 2:
		num = (inFirstSubPage == 0) ? 3 : 4 - inFirstSubPage;
		break;
	case 3:
		num = 4 - inFirstSubPage;
		break;
	}
	return num;
}


/*------------------------------------------------------------------------------
	Initialize C globals.
	Args:		inMap
	Return:  --
	Linkage: C
------------------------------------------------------------------------------*/

extern int	gAtomicIRQNestCountFast;
extern int	gAtomicFIQNestCountFast;
extern int	gAtomicNestCount;
extern int	gAtomicFIQNestCount;

void
InitCGlobals(const MemInit * inMap)
{
	ULong			subPagePerm;
	ULong			nextPage;
	VAddr			nextVAddr;
	size_t		totalSize = 0;

	// add kernel heap to MMU tables
	InitKernelHeapArea();

	// add kernel globals to MMU tables
	for (const MemInit * p = inMap; p->tag != 0; p++)
		totalSize += (p->dataSize + p->zeroSize);
	MapInKernelGlobals(inMap->dataStart, totalSize, 0, GetKernelGlobalsPageIndex(), &nextVAddr, &nextPage, &subPagePerm);
	// update MMU
	FlushTheMMU();

#if 0
	// initialize C variables
	for (p = inMap; p->tag != 0; p++)
	{
		char *	srcPtr, dstPtr;
		srcPtr = inMap->initStart;
		dstPtr = inMap->dataStart;
		for (ArrayIndex i = 0; i < inMap->dataSize; ++i, dstPtr++, srcPtr++)
		{
			if (dstPtr < &gGlobalsThatLiveAcrossReboot || dstPtr > &letterPairs/*0C106528*/)
				*dstPtr = *srcPtr;
		}
		dstPtr = inMap->zeroStart;
		for (ArrayIndex i = 0; i < inMap->zeroSize; ++i, dstPtr++)
		{
			if (dstPtr < &gGlobalsThatLiveAcrossReboot || dstPtr > &letterPairs/*0C106528*/)
				*dstPtr = 0;
		}
	}
#else
	gAtomicIRQNestCountFast =
	gAtomicFIQNestCountFast =
	gAtomicNestCount =
	gAtomicFIQNestCount = 0;
#endif

	SetGlobalsInitialized();
	UseROMJumpTables();
//	FPE_Install();

	gPrimaryTable = (ULong *)GetPrimaryTablePhysBase();		// later updated in InitMemArchCore()

#if 0
	// initialize flash memory
	size_t	length;
	bool		r9 = false;
	bool		r8 = false;
	ULong *	someSortOfConfig = PrimLastRExConfigEntry('ctim', &length);
	if (*someSortOfConfig == 1)
	{
		if (*(someSortOfConfig + 1) != 0)
		{
			g0F280000 = *(someSortOfConfig + 1);
			r9 = true;
		}
		if (*(someSortOfConfig + 2) != 0)
		{
			g0F280400 = *(someSortOfConfig + 2);
			r8 = true;
		}
	}
//98
	// INCOMPLETE
//221
#endif	

	size_t	ramSize = GetRAMSize();
	gDomainTable = (ramSize > 1*MByte) ? g4MegDomainTable : g1MegDomainTable;
	gRAMSize = ramSize / kPageSize;

	// map in the mem obj database
	size_t	dbSize;
	ComputeMemObjDatabaseSize(&dbSize);
	gMemObjHeap = (void *)nextVAddr;
	ULong		memObjSlop = PAGEOFFSET(nextVAddr + dbSize);
	if (memObjSlop == 0)
	{
		// mem obj database fits perfectly in a page -- yeah, right!
		if (PAGEOFFSET(nextVAddr) == 0)
			MapInKernelGlobals(nextVAddr, dbSize, 0, nextPage, &nextVAddr, &nextPage, &subPagePerm);
		else
			MapInKernelGlobals(nextVAddr, dbSize, subPagePerm, nextPage - 1, &nextVAddr, &nextPage, &subPagePerm);
		gNextKernelVAddr = nextVAddr;
	}
	else
	{
		VAddr	xAddr;
		if (PAGEOFFSET(nextVAddr) == 0)
			MapInKernelGlobals(nextVAddr, dbSize + (kPageSize - memObjSlop), 0, nextPage, &xAddr, &nextPage, &subPagePerm);
		else
			MapInKernelGlobals(nextVAddr, dbSize + (kPageSize - memObjSlop), subPagePerm, nextPage - 1, &xAddr, &nextPage, &subPagePerm);
		gNextKernelVAddr = nextVAddr + dbSize;
	}
	gNextPageIndex = nextPage;

	// set up processor type and speed
	gMainCPUType = LowLevelGetCPUType();
	if (gMainCPUType == 1)							// MP130
		gMainCPUClockSpeed = 20.0;						//	20.0 MHz
	else if (gMainCPUType == 2)					// eMate 300
		gMainCPUClockSpeed = 24.88;					//	24.88 MHz
	else if (gMainCPUType == 3)					// MP2000
	{
		ULong		elapsedTime = g0F181800;
		LowLevelProcSpeed(10000);
		elapsedTime  = g0F181800 - elapsedTime;
		if (elapsedTime > 2500)
			gMainCPUClockSpeed = 99.522;			// 99.522 MHz
		else
			gMainCPUClockSpeed = 162.184;			// 162.184 MHz
	}
}


void
UseROMJumpTables(void)
{
#if 0
	size_t	length;
	BuildPatchTablePageTable(0x00016000, -1);
	for (ULong id = 0; id < kMaxROMExtensions; id++)
	{
		VAddr	r7 = PrimRExConfigEntry(id, 'ptpt', &length);
		VAddr	r0 = PrimRExConfigEntry(id, 'glpt', &length);
		if (gGlobalsThatLiveAcrossReboot.fROMBanksAreSplit != 0
		&&  r0 != 0)
			r7 += 0x00400000;
		BuildPatchTablePageTable(r7 != 0 ? r7 : 0x00016000 + id*KByte + 4*KByte, id);
	}
#endif
}


/*------------------------------------------------------------------------------
	Initialize virtual memory.
	Set up the virtual page map in the kernel heap.
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

inline ULong	PhysPageOffset(ArrayIndex i)	// offset of CLittlePhys within paging area of kernel heap
{ return sizeof(CPageTracker) + i * sizeof(CLittlePhys); }

void
VMemInit(void)
{
	void * slopBeforeMap = NULL;
	void * slopAfterMap = NULL;

	ULong	offset = PAGEOFFSET(gNextKernelVAddr);
	if (offset != 0)
	{
		// gNextKernelVAddr isnÕt page aligned; note it and align it
		slopBeforeMap = (void *)gNextKernelVAddr;
		gNextKernelVAddr += (kPageSize - offset);
	}

	gPageTracker = new ((void*)gNextKernelVAddr) CPageTracker;	// page tracker is next in kernel heapÉ
	// Éfollowed by CLittlePhys objects for each page of RAM
	
	size_t	ramSize = GetRAMSize();
	int	numOfPages = ramSize / kPageSize - gNextPageIndex;				// num of pages available
	int	numOfMapPages = PhysPageOffset(numOfPages) / kPageSize + 1;	// num of pages required to map those pages
	numOfPages -= numOfMapPages;
	offset = PAGEOFFSET(PhysPageOffset(numOfPages));
	if (offset != 0)
	{
		// CLittlePhys array wonÕt fit page exactly
		slopAfterMap = (void *)(gNextKernelVAddr + PhysPageOffset(numOfPages));
	}

	for ( ; numOfMapPages != 0; numOfMapPages--)
	{
		RememberMappingUsingPAddr(gNextKernelVAddr, 0xFF, CRAMTable::getPPage(gNextPageIndex, gGlobalsThatLiveAcrossReboot.fBank), true);
		gNextKernelVAddr += kPageSize;
		gNextPageIndex++;
	}

	gFirstManagedPageIndex = gNextPageIndex;
	gNextPageIndex += numOfPages;
	gPageTracker->initPageCount();

	NewtonErr				err;
	CPersistentDBEntry * entry;
	gPageTracker->initPages(offsetof(CPersistentDBEntry, fPages));	// init page tracker CSingleQContainer
	for (ArrayIndex i = 0; MemObjManager::getPersistentRef(i, &entry, &err); ++i)
		entry->fPages.init(offsetof(CPersistentDBEntry, fPages));	// init persistent entry CSingleQContainer

	// create a CLittlePhys for every physical page, and give it to the page tracker
	for (ArrayIndex i = 0; i < numOfPages; ++i)
	{
		gNextPageIndex--;
		PAddr	pageAddr = CRAMTable::getPPage(gNextPageIndex, gGlobalsThatLiveAcrossReboot.fBank);
		CLittlePhys * page = new ((Ptr) gPageTracker + PhysPageOffset(i)) CLittlePhys;
		page->init(pageAddr, kPageSize);
		ZeroPhysPage(page->base());
		gPageTracker->put(page);
	}

	InitSafeHeap((SSafeHeapPage **) &gKernelHeap);
	AddPartialPageToSafeHeap(slopBeforeMap, (SSafeHeapPage *)gKernelHeap);
	AddPartialPageToSafeHeap(slopAfterMap, (SSafeHeapPage *)gKernelHeap);
}

#endif
