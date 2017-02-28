/*
	File:		ROMExtensions.cc

	Contains:	ROM extension functions.

	Written by:	Newton Research Group.
*/

#include "ImportExportItems.h"
#include "VirtualMemory.h"
#include "List.h"

// for reading Newton.rex
//#undef __MACTYPES__
//#include "PrivateMacTypes.h"
#include <CoreFoundation/CoreFoundation.h>

#if defined(forFramework)
#define kBundleId CFSTR("org.newton.objects")
#else
#define kBundleId CFSTR("org.newton.messagepad")
#endif


const size_t  ROM$$Size = 7*MByte;  // 0071FC4C well almost


struct FrameImportTable
{
	ULong			numOfMembers;		// +00	total number of member indexes to be imported
	RExImport	units[];				// +04	array of (variable-sized) import units
};


struct RExImportTable
{
	ULong		numOfMembers;			// +00
	Ref *		members;					// +04
} gRExImportTables[kMaxROMExtensions];

ArrayIndex  gRExExportTableCounts[kMaxROMExtensions];

CSortedList *			gMPExportList;			// 0C100E00
CSortedList *			gMPImportList;			// 0C100E04
RExPendingImport *	gMPPendingImports;	// 0C100E08


/*----------------------------------------------------------------------
	D a t a

	In this implementation we read the ROM extension from a file.
----------------------------------------------------------------------*/

char *	gREx = NULL;


/*----------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------*/

void		RegisterPendingImport(RExImport * inImport, Ref * inArg2, char * inArg3, int inArg4, int inArg5);


/*------------------------------------------------------------------------------
	Determine whether the given pointer points to a ROM extension block.
	Args:		inPtr			pointer to any old memory
	Return:  pointer to RExHeader, or NULL if not a ROM extension
------------------------------------------------------------------------------*/

RExHeader *
TestForREx(Ptr inPtr)
{
	RExHeader * rexPtr = (RExHeader *)inPtr;
	return (rexPtr->signatureA == kRExSignatureA
		  && rexPtr->signatureB == kRExSignatureB
		  && rexPtr->id < kMaxROMExtensions) ? rexPtr : NULL;
}


/*------------------------------------------------------------------------------
	Fill in persistent REx info from ROM extension blocks.
	Args:		info			pointer to persistent globals
				inAddr		pointer to potential ROM extension
	Return:  physical address after ROM extensions
------------------------------------------------------------------------------*/

PAddr
ScanForREx(SGlobalsThatLiveAcrossReboot *	info, PAddr inAddr)
{
	Ptr			p = (Ptr) inAddr;
	RExHeader * rex;

	while ((rex = TestForREx(p)) != NULL)
	{
		info->fRExPtr[rex->id] = (RExHeader *)rex->start;
		info->fPhysRExPtr[rex->id] = (RExHeader *)p;
		info->fRExSize[rex->id] = rex->length;
		p += rex->length;
	}
	return (PAddr) p;
}


/*------------------------------------------------------------------------------
	Fill in persistent REx info from ROM extension blocks.
	Should scan all potential REx locations:
		immediately after the base image (long-word aligned)
		at phys   8 MB
		at phys 256 MB ?
	…but on Mac host we only scan the loaded file.
	Args:		info			pointer to persistent globals
	Return:  --
------------------------------------------------------------------------------*/

void
RExScanner(SGlobalsThatLiveAcrossReboot *	info)
{
	for (ArrayIndex id = 0; id < kMaxROMExtensions; id++)
	{
		info->fRExPtr[id] = NULL;
		info->fPhysRExPtr[id] = NULL;
		info->fRExSize[id] = 0;
	}
	info->fRExPTPage++;

#if defined(correct)
	PAddr block = kRExPAddr0;
	if (info->fROMBanksAreSplit)
		block += 4*MByte;

	if (ScanForREx(info, block) < kRExPAddr1)
		ScanForREx(info, kRExPAddr1);

	PAddr hiBlock = ScanForREx(info, 256*MByte);
	if (hiBlock == 0)
		hiBlock = 256*MByte;
	ScanForREx(info, kRExPAddr1 + (hiBlock - TRUNC(hiBlock, kRExPAddr1)));
#else
	if (gREx == NULL)
	{
		RExHeader	rexHeader;

#if defined(forFramework)
		CFBundleRef newtBundle = CFBundleGetBundleWithIdentifier(kBundleId);
#else
		CFBundleRef newtBundle = CFBundleGetMainBundle();
#endif
#if defined(forNTK)
		CFURLRef url = CFBundleCopyResourceURL(newtBundle, CFSTR("NTK"), CFSTR("rex"), NULL);
#else
		CFURLRef url = CFBundleCopyResourceURL(newtBundle, CFSTR("Newton"), CFSTR("rex"), NULL);
#endif
		CFStringRef rexPath = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
		char pathStr[256];
		CFStringGetCString(rexPath, pathStr, 256, kCFStringEncodingUTF8);

		FILE * rexFile = fopen(pathStr, "rb");
		CFRelease(rexPath);
		CFRelease(url);
		if (rexFile != NULL)
		{
			ULong hdrLen;
			// read the header
			fread(&rexHeader, sizeof(RExHeader), 1, rexFile);
			// now we know how big it is, read the whole thing
			hdrLen = CANONICAL_LONG(rexHeader.length);
			gREx = (char *) malloc(hdrLen);
			if (gREx != NULL)
			{
				fseek(rexFile, 0, SEEK_SET);
				fread(gREx, hdrLen, 1, rexFile);
			}
			fclose(rexFile);
#if defined(hasByteSwapping)
			// byte-swap the directory
			ULong * p = (ULong *)gREx;
			for (ArrayIndex i = 0; i < sizeof(RExHeader)/sizeof(ULong); ++i, ++p) {
				*p = BYTE_SWAP_LONG(*p);
			}
			// including config directory entries
			ConfigEntry * entry = ((RExHeader *)gREx)->table;
			for (ArrayIndex i = 0; i < ((RExHeader *)gREx)->count; ++i, ++entry) {
				// byte swap directory entry
				entry->tag = BYTE_SWAP_LONG(entry->tag);
				entry->offset = BYTE_SWAP_LONG(entry->offset);
				entry->length = BYTE_SWAP_LONG(entry->length);
				// byte swap data -- we could switch on entry->tag
				if (entry->tag == kRAMAllocationTag)
				{
					RAMAllocTable * table = (RAMAllocTable *)(gREx + entry->offset);
					table->version = BYTE_SWAP_LONG(table->version);
					table->count = BYTE_SWAP_LONG(table->count);
					RAMAllocEntry * entry = table->table;
					for (ArrayIndex i = 0; i < table->count; ++i, ++entry) {
						entry->min = BYTE_SWAP_LONG(entry->min);
						entry->max = BYTE_SWAP_LONG(entry->max);
						entry->pct = BYTE_SWAP_LONG(entry->pct);
					}
				}
			}
#endif
		}
	}
	ScanForREx(info, (PAddr) gREx);
#endif
}

#pragma mark -

/*------------------------------------------------------------------------------
	Return the address of the package list.
	Args:		inId			REx block id
	Return:  its address
------------------------------------------------------------------------------*/

VAddr
GetPackageList(ULong inId)
{
	size_t length;
	return GetRExConfigEntry(inId, kPackageListTag, &length);
}

#pragma mark -

/*------------------------------------------------------------------------------
	Return a REx config entry.
	Args:		inId			REx block id
				inTag			config entry tag
				outSize		its size
	Return:  its address
------------------------------------------------------------------------------*/

VAddr
GetRExConfigEntry(ULong inId, ULong inTag, size_t * outSize)
{
#if defined(correct)
	return GenericWithReturnSWI(kGetRExConfigEntry, inId, inTag, 0, outSize, NULL, NULL);
#else
	VAddr theEntry;
	GenericWithReturnSWI(kGetRExConfigEntry, inId, inTag, 0, (OpaqueRef *)&theEntry, (OpaqueRef *)outSize, NULL);
	return theEntry;
#endif
}


/*------------------------------------------------------------------------------
	Primitive version of above.
	Args:		inId			REx block id
				inTag			config entry tag
				outSize		its size
	Return:  its address
------------------------------------------------------------------------------*/

VAddr
PrimRExConfigEntry(ULong inId, ULong inTag, size_t * outSize)
{
	if (inId >= kMaxROMExtensions)
	{
		*outSize = 0;
		return 0;
	}

#if defined(correct)
	RExHeader * rEx = gGlobalsThatLiveAcrossReboot.fRExPtr[inId];
#else
	RExHeader * rEx = gGlobalsThatLiveAcrossReboot.fPhysRExPtr[inId];
#endif
	if (rEx != NULL)
	{
		for (ArrayIndex i = 0; i < rEx->count; ++i)
		{
			ConfigEntry * entry = &rEx->table[i];
			if (entry->tag == inTag
			 && entry->offset != kUndefined)
			{
				*outSize = entry->length;
				return (VAddr)rEx + entry->offset;
			}
		}
	}
	return 0;
}


/*------------------------------------------------------------------------------
	Return the next entry in a REx block.
	Args:		inId			REx block id
				inTag			config entry tag
				outSize		its size
				ioIndex		index to entry
	Return:  its address
------------------------------------------------------------------------------*/

VAddr
PrimNextRExConfigEntry(ULong inId, ULong inTag, size_t * outSize, ArrayIndex * ioIndex)
{
	if (inId >= kMaxROMExtensions)
	{
		*outSize = 0;
		return 0;
	}

	RExHeader * rEx = gGlobalsThatLiveAcrossReboot.fRExPtr[inId];
	if (rEx != NULL)
	{
		for (ArrayIndex i = *ioIndex; i < rEx->count; ++i)
		{
			ConfigEntry * entry = &rEx->table[i];
			if (entry->tag == inTag
			 && entry->offset != kUndefined)
			{
				*outSize = entry->length;
				*ioIndex = i + 1;
				return (VAddr)rEx + entry->offset;
			}
		}
	}
	return 0;
}


/*------------------------------------------------------------------------------
	Return the last REx config entry of the specified type - ie the one that
	matters most.
	Args:		inTag			config entry tag
				outSize		its size
	Return:  its address
------------------------------------------------------------------------------*/

VAddr
GetLastRExConfigEntry(ULong inTag, size_t * outSize)
{
	if (IsSuperMode())
		return PrimLastRExConfigEntry(inTag, outSize);
	/*
	GenericWithReturnSWI is declared to return a NewtonErr.
	In a 32-bit world any value, including pointers, could be returned.
	This won’t do in a 64-bit world so we have to sanitise this call.
	*/
#if defined(correct)
	return GenericWithReturnSWI(kGetLastRExConfigEntry, inTag, 0, 0, outSize, NULL, NULL);
#else
	VAddr theEntry;
	GenericWithReturnSWI(kGetLastRExConfigEntry, inTag, 0, 0, (OpaqueRef *)&theEntry, (OpaqueRef *)outSize, NULL);
	return theEntry;
#endif
}


/*------------------------------------------------------------------------------
	Primitive version of above.
	Args:		inTag			config entry tag
				outSize		its size
	Return:  its address
------------------------------------------------------------------------------*/

VAddr
PrimLastRExConfigEntry(ULong inTag, size_t * outSize)
{
	VAddr addr = 0;
	for (int id = kMaxROMExtensions - 1; id >= 0; id--)
		if ((addr = PrimRExConfigEntry(id, inTag, outSize)) != 0)
			break;
	return addr;
}


/*------------------------------------------------------------------------------
	Return the REx header of the specified	REx block.
	Args:		inId			REx block id
	Return:  REx header pointer
------------------------------------------------------------------------------*/

RExHeader *
GetRExPtr(ULong inId)
{
	if (inId >= kMaxROMExtensions)
		return NULL;
	return (RExHeader *)GenericSWI(kGetRExPtr, inId);
}

#pragma mark -

/*----------------------------------------------------------------------
	Initialize REx magic pointer tables.
	Args:		--
	Return:	--
----------------------------------------------------------------------*/

void
InitRExMagicPointerTables(void)
{
	for (ArrayIndex id = 0; id < kMaxROMExtensions; id++)
	{
		size_t	rexlen = 1023;
		VAddr		rex;

		rex = GetRExConfigEntry(id, kFrameExportTableTag, &rexlen);
		if (rex)
			rex = rexlen/sizeof(Ref);
		gRExExportTableCounts[id] = rex;

		rex = GetRExConfigEntry(id, kFrameImportTableTag, &rexlen);
		if (rex && rexlen > 0)
		{
			VAddr			rexend = rex + rexlen;
			ULong			numOfMembers = ((FrameImportTable*)rex)->numOfMembers;	// r6 for all imported units
			RExImport * unit = ((FrameImportTable*)rex)->units;
			Ref *			members = new Ref[numOfMembers];  // r7 for all imported units
			if (members == NULL)
				OutOfMemory();
			gRExImportTables[id].numOfMembers = numOfMembers;
			gRExImportTables[id].members = members;
			for (ArrayIndex index = 0; index < numOfMembers; index++)
				members[index] = NILREF;

			for ( ; (VAddr)unit < rexend; unit = (RExImport *)((Ptr)unit + LONGALIGN(sizeof(unit) + strlen(unit->name))))
			{
				RegisterPendingImport(unit, members, unit->name, unit->majorVersion, unit->minorVersion);
				members += unit->count;
			}
		}
	}
}




void
RExPendingImport::fulfill(MPExportItem * inItem)
{
	memmove(f1C, inItem->f08, f18->count * sizeof(Ref));
}

int
RExPendingImport::match(void * inItem)
{
	return 0;
}


void
RegisterPendingImport(RExImport * inImport, Ref * inMembers, char * inName, int inMajorVersion, int inMinorVersion)
{
	RExPendingImport *	pending = new RExPendingImport;
	if (pending == NULL)
		OutOfMemory();

	pending->f08 = 1;
	pending->f0C = inName;
	pending->f18 = inImport;
	pending->f1C = inMembers;
	pending->f10 = inMajorVersion;
	pending->f14 = inMinorVersion;

	pending->f04 = gMPPendingImports;
	gMPPendingImports = pending;
}

