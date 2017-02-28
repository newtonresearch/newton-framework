/*
	File:		REx.cc

	Contains:	Command line tool to create ROM extension defined by config file.
					Adapted from the original MPW tool, with added byte-swapping.

	TODO:			investigate patch info file structure
					page table handling
					unit import/export
					anything marked INCOMPLETE

	Written by:	The Newton Tools group.
*/

#include "Lexer.h"
#include "PackageAccessor.h"
#include "Options.h"
#include "ClauseHandlers.h"
#include "PatchTable.h"
#include "CRC16.h"
#include "Reporting.h"
#include "RExArray.h"

#include <iostream>
#include <fstream>
using namespace std;

/* -----------------------------------------------------------------------------
	T y p e s
----------------------------------------------------------------------------- */

// an element in  the global RExArray
struct Block
{
	ULong	tag;
	CChunk * data;
	SomeHandlerProc doSomething;
#if defined(hasByteSwapping)
	ByteSwapProc swap;
#endif
};


#pragma mark -
/* -----------------------------------------------------------------------------
	G l o b a l   D a t a
----------------------------------------------------------------------------- */

const char *	gCmdName;
const char *	gOutputFilename = NULL;
const char *	gInputFilename = NULL;

int	gPhysAddr = kUndefined;	// -0EAC phys start addr
int	gPatchTableAddr = kUndefined;	//-0EA8
RExArray *	gPatches = NULL;	//-0EA4
CChunk *	gGelatoPageTable = NULL;	//-0EA0
CChunk *	gPatchTablePadding = NULL;	//-0E9C
CChunk *	gPatchTablePageTable = NULL;	//-0E98
CChunk *	gCPatchTablePadding = NULL;	//-0E94
CChunk *	gCPatchTable = NULL;	//-0E90

RExArray * gConfigs = NULL;	//-0CF8
RExHeader gREx;	//-0CF4

int	g1548 = 0;	// cf version
int	g1544 = 0;	// cf manuf id

CChunk * gROMConfigOptions = NULL;	//-1528	rom
CChunk * gRAMConfigOptions = NULL;	//-1152C	ram
CChunk * gFlashConfigOptions = NULL;	//-1530	flsh
CChunk * gPackages = NULL;	//-1534	pkgl
CChunk * gDiagnosticsPadding = NULL;	//-1538	pad
CChunk * gDiagnosticsOptions = NULL;	//-153C	diag
CChunk * gRAMAllocOptions = NULL;	//-1540	ralc

CLexer * gLex = NULL;	//-138E

RExArray * g12BE;	// gExportTable?
CChunk * g12BA;	// gExportTablePadding?
RExArray * g12B6;


#pragma mark -
/* -----------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------------- */


#pragma mark -
/* -----------------------------------------------------------------------------
	Process the configuration input file.
	Args:		--			filename has been parsed into global var
	Return:	--
----------------------------------------------------------------------------- */

void
ProcessConfigFile(void)
{
	CLexer lexer(gInputFilename);
	gLex = &lexer;

	Token tokn;
	while (gLex->getToken(tokn)) {
		gLex->consumeToken();
		if (!CClauseHandler::handleClause(tokn)) {
			if (tokn.fType == kIdentifierToken) {
				FatalError("unrecognized section name “%s”", tokn.fStrValue.c_str());
			} else {
				FatalError("expected identifier");
			}
		}
	}
	gLex = NULL;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	Add configuration blocks, parsed from the input file, to our global array.
----------------------------------------------------------------------------- */

void
InitBlocks(void)
{
	gConfigs = new RExArray(sizeof(Block));
}


#if defined(hasByteSwapping)
void	AddBlock(ULong inTag, CChunk * inData, SomeHandlerProc inhandler, ByteSwapProc inSwapper)
#else
void	AddBlock(ULong inTag, CChunk * inData, SomeHandlerProc inhandler)
#endif
{
	Block block;
	block.tag = inTag;
	block.data = inData;
	block.doSomething = inhandler;
#if defined(hasByteSwapping)
	block.swap = inSwapper;
#endif
	gConfigs->fChunk.append(&block, sizeof(block));
}

bool
FindBlock(ULong inTag, Block & outBlock)
{
	for (ArrayIndex i = 0, count = gConfigs->count(); i < count; ++i) {
		Block * block = (Block *)(gConfigs->at(i));
		if (block->tag == inTag) {
			outBlock = *block;
			return true;
		}
	}
	return false;
}

bool
FindBlock(CChunk * inData, Block & outBlock)
{
	for (ArrayIndex i = 0, count = gConfigs->count(); i < count; ++i) {
		Block * block = (Block *)(gConfigs->at(i));
		if (block->data == inData) {
			outBlock = *block;
			return true;
		}
	}
	return false;
}

#pragma mark -
/* -----------------------------------------------------------------------------
	Data block alignment functions.
----------------------------------------------------------------------------- */

void
Align(CChunk * inData, size_t inSize, size_t inAlignment)
{
	size_t delta = inSize % inAlignment;
	if (delta != 0) {
		inData->append(NULL, inAlignment - delta);
	}
}

void
Align4K(CChunk * inData, long inSize)
{
	if (inSize != kUndefined) {
		Align(inData, gREx.start+inSize, 4*KByte);	// extension start addr
	}
}

void
Align1K(CChunk * inData, long inSize)
{
	if (inSize != kUndefined) {
		Align(inData, gPhysAddr+inSize, 1*KByte);	// phys start addr
	}
}

void
AlignB0(CChunk * inData, long inSize)
{
	if (inSize != kUndefined) {
		Align(inData, inSize, 0x00B0);
	}
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C l a u s e   H a n d l e r s
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Process packages for PackageHandler().
	Args:		inData		sequence of packages
				inOffset		offset from start of RExHeader to this config data
								ie self-relative address for making pointer Refs
	Return:	--
----------------------------------------------------------------------------- */

void
ProcessPackages(CChunk * inData, long inOffset)
{
	if (inOffset == kUndefined) {
		// this is our setup call
		g12B6 = new RExArray(sizeof(ExportUnit));
		size_t pkgEnd = gPackages->dataSize();	//spm08
		// iterate over exports=0, imports=1, redirects=2
		for (ArrayIndex selector = 0; selector < 3; ++selector) {
			// iterate over packages in the data chunk
			for (long pkgOffset = 0; pkgOffset < pkgEnd; ) {
				CPackageAccessor pkg(*inData, pkgOffset);
				// iterate over parts in the package
				for (ArrayIndex partNo = 0, numOfParts = pkg.numParts(); partNo < numOfParts; ++partNo) {
					// we’re only interested in NOS parts: unit import/export declaration frames
					if ((pkg.partFlags(partNo) & 0x0F) == kNOSPart) {
//						C588(0x20);
						long partOffset = pkg.partOffset(partNo);	// offset to part data in part area of package
						CFramesPartAccessor part(pkg.unsafePartDataPtr(partNo), pkg.partSize(partNo), partOffset);
						ProcessPart(selector, part, pkgOffset, partOffset);
					}
				}
				pkgOffset += pkg.size();
			}
		}
		if (g12BE) {
			if (gPatchTablePageTable == NULL) {
				gPatchTablePadding = new CChunk;
				AddBlock(kPadBlockTag, gPatchTablePadding, Align1K);
				gPatchTablePageTable = new CChunk;
				AddBlock(kPatchTablePageTableTag, gPatchTablePageTable, NULL);
				gPatchTablePageTable->append(NULL, 1*KByte);
				gGelatoPageTable = new CChunk;
				if (gGelatoPageTable == NULL) {
					FatalError("could not allocate gGelatoPageTable");
				}
				AddBlock(kGelatoPageTableTag, gGelatoPageTable, NULL);
				gGelatoPageTable->append(NULL, 1*KByte);
			}
			g12BA = new CChunk;
			AddBlock(kPadBlockTag, g12BA, Align4K);
			AddBlock(kFrameExportTableTag, &g12BE->fChunk);
		}

	} else {
		// this is our process call -- relocate frames parts
		for (long pkgOffset = 0, pkgEnd = gPackages->dataSize(); pkgOffset < pkgEnd; ) {	//d5 d7
			CPackageAccessor pkg(*inData, pkgOffset);
			for (ArrayIndex partNo = 0, numOfParts = pkg.numParts(); partNo < numOfParts; ++partNo) {
				if ((pkg.partFlags(partNo) & 0x0F) == kNOSPart) {
//					C588(0x20);
					CFramesPartAccessor part(pkg.unsafePartDataPtr(partNo), pkg.partSize(partNo), pkg.partOffset(partNo));
					part.relocate(gREx.start + inOffset + pkgOffset);
				}
			}
			pkgOffset += pkg.size();
		}

		// fix up exports to absolute Refs
		if (g12BE) {
			Ref32 * entry = (Ref32 *)g12BE->at(0);
			for (ArrayIndex i = 0, count = g12BE->count(); i < count; ++i, ++entry) {
				*entry += gREx.start + inOffset;
			}
		}
	}
}


/* -----------------------------------------------------------------------------
	Parse optional patch info file.
	Expecting:
		<file> : patch info file
	Args:		inData		package or block data
				inOffset		offset to start of data in need of patching
	Return:	--
----------------------------------------------------------------------------- */

void
TryPatchInfoFile(CChunk * inData, long inOffset)
{
	Token tokn;
	if (gLex->getToken(tokn) && tokn.fType == kStringToken) {
		gLex->consumeToken();
		if (gPatches == NULL) {
			gPatches = new RExArray(sizeof(PatchInfo));
		}

		if (gCPatchTable == NULL) {
			gCPatchTable = new CChunk;
			gCPatchTablePadding = new CChunk;
			if (gPatchTablePageTable == NULL) {
				gPatchTablePadding = new CChunk;
				AddBlock(kPadBlockTag, gPatchTablePadding, Align1K);
				gPatchTablePageTable = new CChunk;
				AddBlock(kPatchTablePageTableTag, gPatchTablePageTable);
				gPatchTablePageTable->append(NULL, 1*KByte);
				gGelatoPageTable = new CChunk;
				if (gGelatoPageTable == NULL) {
					FatalError("could not allocate gGelatoPageTable");
				}
				AddBlock(kGelatoPageTableTag, gGelatoPageTable);
				gGelatoPageTable->append(NULL, 1*KByte);
			}
		}

		PatchInfo info;
		info.fData = inData;
		info.fOffset = inOffset;
		info.fPatch = new CChunk;
		ReadPatchInfoFile(info.fPatch, tokn.fStrValue.c_str());
		gPatches->fChunk.append(&info, sizeof(info));
	}
}


/* -----------------------------------------------------------------------------
	Parse the extension id.
	Expecting:
		<int> : extension id
----------------------------------------------------------------------------- */

void
ROMExtIDHandler(void)
{
	if (gREx.id != kUndefined) {
		FatalError("multiple extension ids");
	}
	Token tokn;
	gLex->get(kIntegerToken, tokn, kMandatory);
	if (tokn.fValue < 0 || tokn.fValue > kMaxROMExtensions) {
		FatalError("extension id must be between 0 and %d", kMaxROMExtensions);
	}
	gREx.id = tokn.fValue;
}


/* -----------------------------------------------------------------------------
	Parse the physical REx start address.
	Expecting:
		<int> : virtual start address
----------------------------------------------------------------------------- */

void
ROMExtStartHandler(void)
{
	if (gREx.start != kUndefined) {
		FatalError("multiple start addresses");
	}
	Token tokn;
	gLex->get(kIntegerToken, tokn, kMandatory);
	gREx.start = tokn.fValue;
}


/* -----------------------------------------------------------------------------
	Parse the REx manufacturer id.
	Expecting:
		<int> : manufacturer id
----------------------------------------------------------------------------- */

void
ManufacturerHandler(void)
{
	if (g1544 != 0) {
		FatalError("multiple manufacturer ids");
	}
	Token tokn;
	gLex->get(kIntegerToken, tokn, kMandatory);
	gREx.manufacturer = tokn.fValue;
}


/* -----------------------------------------------------------------------------
	Parse the REx version.
	Expecting:
		<int> : version
----------------------------------------------------------------------------- */

void
VersionHandler(void)
{
	if (g1548 != 0) {
		FatalError("multiple versions");
	}
	Token tokn;
	gLex->get(kIntegerToken, tokn, kMandatory);
	gREx.version = tokn.fValue;
}


/* -----------------------------------------------------------------------------
	Parse a package.
	Expecting:
		<pkgfile> [<ptfile>] : add a package, optional patch file
----------------------------------------------------------------------------- */

void
PackageHandler(void)
{
	Token tokn;
	gLex->get(kStringToken, tokn, kMandatory);
	if (gPackages == NULL) {
		gPackages = new CChunk;
		AddBlock(kPackageListTag, gPackages, ProcessPackages);
	}
	long start = gPackages->dataSize();
	gPackages->append(tokn.fStrValue.c_str());	//data from package file
	CPackageAccessor pkg(*gPackages, start);
	if (!pkg.isPackage()) {
		FatalError("“%s” is not a package file", tokn.fStrValue.c_str());
	}
	TryPatchInfoFile(gPackages, start);
}


/* -----------------------------------------------------------------------------
	Parse diagnostics.
	Expecting:
		<file> [<int>] : diagnostics file [alignsize]
----------------------------------------------------------------------------- */

void
DiagnosticsHandler(void)
{
	if (gDiagnosticsOptions != 0) {
		FatalError("multiple diagnostics");
	}
	Token tokn;
	gLex->get(kStringToken, tokn, kMandatory);	// diagnostics filename
	gDiagnosticsPadding = new CChunk;

	Token alignment;
	if (gLex->get(kIntegerToken, alignment, kOptional)) {	// alignment size
		if (alignment.fValue == 4*KByte) {
			AddBlock(kPadBlockTag, gDiagnosticsPadding, Align4K);
		} else if (alignment.fValue == 1*KByte) {
			AddBlock(kPadBlockTag, gDiagnosticsPadding, Align1K);
		} else {
			FatalError("diagnostics align size “%04X” unknown (!= 4K/1K)", alignment.fValue);
		}
	} else {
		AddBlock(kPadBlockTag, gDiagnosticsPadding, AlignB0);
	}
	gDiagnosticsOptions = new CChunk;
	gDiagnosticsOptions->append(tokn.fStrValue.c_str());	//file
	AddBlock(kDiagnosticsTag, gDiagnosticsOptions);
}


/* -----------------------------------------------------------------------------
	Parse a raw config block.
	Expecting:
		<tag> { <file> | <raw> } [<ptfile>] : arbitrary config block, optional patch file
----------------------------------------------------------------------------- */

void
BlockHandler(void)
{
	Token tagTokn;
	gLex->get(kIntegerToken, tagTokn, kMandatory);	// tag

	Token tokn;
	if (gLex->getToken(tokn) && (tokn.fType == kStringToken || tokn.fType == kDataToken)) {
		gLex->consumeToken();
		Block blok;	//spm2A
		CChunk * blockOptions;
		if (FindBlock(tagTokn.fValue, blok)) {
			blockOptions = blok.data;
		} else {
			blockOptions = new CChunk;
			AddBlock(tagTokn.fValue, blockOptions);
		}
		long start = blockOptions->dataSize();
		if (tokn.fType == kStringToken) {
			blockOptions->append(tokn.fStrValue.c_str());	//file
		} else if (tokn.fType == kDataToken) {
			blockOptions->append((void *)tokn.fStrValue.data(), tokn.fStrValue.size());	//raw data
		}
		TryPatchInfoFile(blockOptions, start);
	} else {
		FatalError("expected filename or raw data block");
	}
}


/* -----------------------------------------------------------------------------
	Parse an arbitrary config block from a resource.
	Expecting:
		<tag> <file> : arbitrary config block from resource (resType= tag; resID= 0)
----------------------------------------------------------------------------- */

void
ResourceHandler(void)
{
	Token tagTokn;
	gLex->get(kIntegerToken, tagTokn, kMandatory);

	Token tokn;
	if (gLex->getToken(tokn) && tokn.fType == kStringToken) {
		gLex->consumeToken();
// INCOMPLETE
// we don’t use resources in OS X
	} else {
		FatalError("expected resource filename");
	}
}


/* -----------------------------------------------------------------------------
	Parse ROM configuration.
	Expecting:
		{ start <int> length <int> } : ROM config
----------------------------------------------------------------------------- */

#if defined(hasByteSwapping)
void
ROMByteSwapper(void * inData)
{
	ROMConfigTable * table = (ROMConfigTable *)inData;
	ArrayIndex count = table->count;
	table->version = BYTE_SWAP_LONG(table->version);
	table->count = BYTE_SWAP_LONG(count);
	ROMConfigEntry * entry = table->table;
	for (ArrayIndex i = 0; i < count; ++i) {
		entry->romPhysStart = BYTE_SWAP_LONG(entry->romPhysStart);
		entry->romSize = BYTE_SWAP_LONG(entry->romSize);
	}
}
#endif


void
ROMHandler(void)
{
	if (gROMConfigOptions == NULL) {
		gROMConfigOptions = new CChunk;
		AddBlock(kROMConfigTag, gROMConfigOptions, NULL, ROMByteSwapper);
		gROMConfigOptions->append(NULL, sizeof(ROMConfigTable));
		ROMConfigTable * table = (ROMConfigTable *)gROMConfigOptions->data();
		table->version = kROMConfigTableVersion;
	}
	ROMConfigEntry entry;
	COptionList options;
	options.readOptions(gLex);
	options.getOption("start", (int *)&entry.romPhysStart, 0, INT_MIN, INT_MAX);
	options.getOption("length", (int *)&entry.romSize, 0, INT_MIN, INT_MAX);
	if (options.hasOptions()) {
		cerr << "# Unrecognised options: ";
		options.printLeftovers();
		FatalError(options.position(), "bad option list");
	}
	gROMConfigOptions->append(&entry, sizeof(entry));
	ROMConfigTable * table = (ROMConfigTable *)gROMConfigOptions->data();
	++table->count;
}


/* -----------------------------------------------------------------------------
	Parse RAM configuration.
	Expecting:
		{ start <int> length <int> tag <int> lanesize <int> lanecount <int> } : RAM config
----------------------------------------------------------------------------- */

#if defined(hasByteSwapping)
void
RAMByteSwapper(void * inData)
{
	RAMConfigTable * table = (RAMConfigTable *)inData;
	ArrayIndex count = table->count;
	table->version = BYTE_SWAP_LONG(table->version);
	table->count = BYTE_SWAP_LONG(count);
	RAMConfigEntry * entry = table->table;
	for (ArrayIndex i = 0; i < count; ++i) {
		entry->ramPhysStart = BYTE_SWAP_LONG(entry->ramPhysStart);
		entry->ramSize = BYTE_SWAP_LONG(entry->ramSize);
		entry->tag = BYTE_SWAP_LONG(entry->tag);
		entry->laneSize = BYTE_SWAP_LONG(entry->laneSize);
		entry->laneCount = BYTE_SWAP_LONG(entry->laneCount);
	}
}
#endif

void
RAMHandler(void)
{
	if (gRAMConfigOptions == NULL) {
		gRAMConfigOptions = new CChunk;
		AddBlock(kRAMConfigTag, gRAMConfigOptions, NULL, RAMByteSwapper);
		gRAMConfigOptions->append(NULL, sizeof(RAMConfigTable));
		RAMConfigTable * table = (RAMConfigTable *)gRAMConfigOptions->data();
		table->version = kRAMConfigTableVersion;
	}
	RAMConfigEntry entry;
	COptionList options;
	options.readOptions(gLex);
	options.getOption("start", (int *)&entry.ramPhysStart, 0, INT_MIN, INT_MAX);
	options.getOption("length", (int *)&entry.ramSize, 0, INT_MIN, INT_MAX);
	options.getOption("tag", (int *)&entry.tag, 0, INT_MIN, INT_MAX);
	options.getOption("lanesize", (int *)&entry.laneSize, 0, INT_MIN, INT_MAX);
	options.getOption("lanecount", (int *)&entry.laneCount, 0, INT_MIN, INT_MAX);
	if (options.hasOptions()) {
		cerr << "# Unrecognised options: ";
		options.printLeftovers();
		FatalError(options.position(), "bad option list");
	}
	gRAMConfigOptions->append(&entry, sizeof(entry));
	RAMConfigTable * table = (RAMConfigTable *)gRAMConfigOptions->data();
	++table->count;
}


/* -----------------------------------------------------------------------------
	Parse flash configuration.
	Expecting:
		{ manufacturer <0-255> devicecode <0-255> internaldevicecode <0-255> needsvpp <0-1> size <int> blocksize <int> start <int> } : Flash config
----------------------------------------------------------------------------- */

#if defined(hasByteSwapping)
void
FlashByteSwapper(void * inData)
{
	FlashBankConfigTable * table = (FlashBankConfigTable *)inData;
	ArrayIndex count = table->count;
	table->version = BYTE_SWAP_LONG(table->version);
	table->count = BYTE_SWAP_LONG(count);
	FlashConfigEntry * entry = (FlashConfigEntry *)table->table;
	for (ArrayIndex i = 0; i < count; ++i) {
		entry->size = BYTE_SWAP_LONG(entry->size);
		entry->blockSize = BYTE_SWAP_LONG(entry->blockSize);
		entry->start = BYTE_SWAP_LONG(entry->start);
	}
}
#endif

void
FlashHandler(void)
{
	if (gFlashConfigOptions == NULL) {
		gFlashConfigOptions = new CChunk;
		AddBlock(kFlashConfigTag, gFlashConfigOptions, NULL, FlashByteSwapper);
		gFlashConfigOptions->append(NULL, sizeof(FlashBankConfigTable));
		FlashBankConfigTable * table = (FlashBankConfigTable *)gFlashConfigOptions->data();
		table->version = kFlashBankConfigTableVersion;
	}
	FlashConfigEntry entry;
	int byteValue;
	COptionList options;
	options.readOptions(gLex);
	options.getOption("manufacturer", &byteValue, 0, 0x00, 0xFF);
	entry.manufacturer = byteValue;
	options.getOption("devicecode", &byteValue, 0, 0x00, 0xFF);
	entry.deviceCode = byteValue;
	options.getOption("internaldevicecode", &byteValue, 0, 0x00, 0xFF);
	entry.internalDeviceCode = byteValue;
	options.getOption("needsvpp", &byteValue, 0, 0, 1);
	entry.needsVpp = byteValue;
	options.getOption("size", (int *)&entry.size, 0, INT_MIN, INT_MAX);
	options.getOption("blocksize", (int *)&entry.blockSize, 0, INT_MIN, INT_MAX);
	options.getOption("start", (int *)&entry.start, 0, INT_MIN, INT_MAX);
	if (options.hasOptions()) {
		cerr << "# Unrecognised options: ";
		options.printLeftovers();
		FatalError(options.position(), "bad option list");
	}
	gFlashConfigOptions->append(&entry, sizeof(entry));
	FlashBankConfigTable * table = (FlashBankConfigTable *)gFlashConfigOptions->data();
	++table->count;
}


/* -----------------------------------------------------------------------------
	Parse RAM allocation.
	Expecting:
		{ min <int> max <int> percent <int> } : RAM allocation
----------------------------------------------------------------------------- */

#if defined(hasByteSwapping)
void
RAMAllocByteSwapper(void * inData)
{
	RAMAllocTable * table = (RAMAllocTable *)inData;
	ArrayIndex count = table->count;
	table->version = BYTE_SWAP_LONG(table->version);
	table->count = BYTE_SWAP_LONG(count);
	RAMAllocEntry * entry = table->table;
	for (ArrayIndex i = 0; i < count; ++i, ++entry) {
		entry->min = BYTE_SWAP_LONG(entry->min);
		entry->max = BYTE_SWAP_LONG(entry->max);
		entry->pct = BYTE_SWAP_LONG(entry->pct);
	}
}
#endif

void
RAMAllocHandler(void)
{
	if (gRAMAllocOptions == NULL) {
		gRAMAllocOptions = new CChunk();
		AddBlock(kRAMAllocationTag, gRAMAllocOptions, NULL, RAMAllocByteSwapper);
		gRAMAllocOptions->append(NULL, sizeof(RAMAllocTable));
		RAMAllocTable * table = (RAMAllocTable *)gRAMAllocOptions->data();
		table->version = kRAMAllocTableVersion;
	}
	RAMAllocEntry entry;
	int pct;
	COptionList options;
	options.readOptions(gLex);
	options.getOption("min", (int *)&entry.min, 0, INT_MIN, INT_MAX);
	options.getOption("max", (int *)&entry.max, 0, INT_MIN, INT_MAX);
	options.getOption("percent", &pct, 0, INT_MIN, INT_MAX);
	entry.pct = 1024 * pct/100;
	if (options.hasOptions()) {
		cerr << "# Unrecognised options: ";
		options.printLeftovers();
		FatalError(options.position(), "bad option list");
	}
	gRAMAllocOptions->append(&entry, sizeof(entry));
	RAMAllocTable * table = (RAMAllocTable *)gRAMAllocOptions->data();
	++table->count;
}


/* -----------------------------------------------------------------------------
	Parse patch table address.
	Expecting:
		<int> : virtual patch table start address
----------------------------------------------------------------------------- */

void
PatchTableAddrHandler(void)
{
	Token tokn;
	if (gPatchTableAddr != 0) {
		FatalError("multiple patch table addresses");
	}
	gLex->get(kIntegerToken, tokn, kMandatory);
	gPatchTableAddr = tokn.fValue;
}


/* -----------------------------------------------------------------------------
	Parse physical start address.
	Expecting:
		<int> : physical start address
----------------------------------------------------------------------------- */

void
PhysStartHandler(void)
{
	Token tokn;
	if (gPhysAddr != kUndefined) {
		FatalError("multiple physical start addresses");
	}
	gLex->get(kIntegerToken, tokn, kMandatory);
	gPhysAddr = tokn.fValue;
}


/* -----------------------------------------------------------------------------
	Register all clause handlers.
----------------------------------------------------------------------------- */

void
InitHandlers(void)
{
	gREx.id = kUndefined;
	gREx.start = kUndefined;
	CNamedClauseHandler::registerHandler("id", ROMExtIDHandler, "<int> : extension id");
	CNamedClauseHandler::registerHandler("start", ROMExtStartHandler, "<int> : virtual start address");
	CNamedClauseHandler::registerHandler("physstart", PhysStartHandler, "<int> : physical start address");
	CNamedClauseHandler::registerHandler("manufacturer", ManufacturerHandler, "<int> : manufacturer id");
	CNamedClauseHandler::registerHandler("version", VersionHandler, "<int> : version");
	CNamedClauseHandler::registerHandler("package", PackageHandler, "<pkgfile> [<ptfile>] : add a package");
	CNamedClauseHandler::registerHandler("diagnostics", DiagnosticsHandler, "<file> [<int>] : diagnostics file [alignsize]");
	CNamedClauseHandler::registerHandler("rom", ROMHandler, "{ start <int> length <int> } : ROM config");
	CNamedClauseHandler::registerHandler("ram", RAMHandler, "{ start <int> length <int> tag <int> lanesize <int> lanecount <int> } : RAM config");
	CNamedClauseHandler::registerHandler("flash", FlashHandler, "{ manufacturer <0-255> devicecode <0-255> internaldevicecode <0-255> needsvpp <0-1> size <int> blocksize <int> start <int> } : Flash config");
	CNamedClauseHandler::registerHandler("ramallocation", RAMAllocHandler, "{ min <int> max <int> percent <int> } : RAM allocation");
	CNamedClauseHandler::registerHandler("block", BlockHandler, "<tag> { <file> | <raw> } [<ptfile>] : arbitrary config block");
	CNamedClauseHandler::registerHandler("resource", ResourceHandler, "<tag> <file> : arbitrary config block from resource (resType= tag; resID= 0)");
	CNamedClauseHandler::registerHandler("patchtableaddr", PatchTableAddrHandler, "<int> : virtual patch table start address");
}


#pragma mark -
/* -----------------------------------------------------------------------------
	Create the REx file.
	Args:		--			input has been parsed into global vars
	Return:	--
----------------------------------------------------------------------------- */

void
PerformOutput(void)
{
	// sanity checks
	if (gREx.id == kUndefined) {
		FatalError("must specify extension id");
	}
	if (gREx.start == kUndefined) {
		FatalError("must specify start address");
	}

	// physical address is REx address by default
	if (gPhysAddr == kUndefined) {
		gPhysAddr = gREx.start;
	}

	// set up for processing where required
	for (ArrayIndex i = 0, count = gConfigs->count(); i < count; ++i) {
		Block * block = (Block *)gConfigs->at(i);
		if (block->doSomething) {
			block->doSomething(block->data, kUndefined);
		}
	}

	if (gCPatchTable != NULL) {
		AddBlock(kPadBlockTag, gCPatchTablePadding, Align4K);
		AddBlock(kCPatchTableTag, gCPatchTable);
	}

	// create ConfigEntry array from config blocks
	ArrayIndex numOfConfigEntries = gConfigs->count();
	size_t frameExportsOffset = 0;	//d4
	size_t rexLength = sizeof(RExHeader) + numOfConfigEntries * sizeof(ConfigEntry);
	ConfigEntry * entries = new ConfigEntry[numOfConfigEntries];
	ConfigEntry * entry = entries;
	for (ArrayIndex i = 0; i < numOfConfigEntries; ++i, ++entry) {
		Block * block = (Block *)gConfigs->at(i);
		entry->tag = block->tag;
		if (block->tag == kFrameExportTableTag) {
			frameExportsOffset = rexLength;
		}
		entry->offset = (ULong)rexLength;
		block->data->roundUp(4);
		// process the block if required
		if (block->doSomething) {
			block->doSomething(block->data, rexLength);
		}
		entry->length = (ULong)block->data->dataSize();
		rexLength += entry->length;
	}
	size_t patchesOffset = 0;
	if (gCPatchTable != NULL) {
		ProcessPatchTable();
		entries[numOfConfigEntries-1].length = gCPatchTable->dataSize();
		patchesOffset = gCPatchTable->dataSize();
		rexLength += gCPatchTable->dataSize();
	}
	if (gPatchTablePageTable) {
		BuildPageTable(rexLength, patchesOffset, frameExportsOffset);
	}

	// compute CRC for config data
	CRC16 crc;
	crc.reset();
	for (ArrayIndex i = 0; i < numOfConfigEntries; ++i) {
		Block * block = (Block *)gConfigs->at(i);
#if defined(hasByteSwapping)
	// byte-swap config BEFORE computing CRC
		if (block->swap != NULL) {
			block->swap(block->data->data());
		}
#endif
		crc.computeCRC((unsigned char *)block->data->data(), block->data->dataSize());
	}

	// complete REx header -- it has already been populated by clause handlers
	gREx.signatureA = kRExSignatureA;
	gREx.signatureB = kRExSignatureB;
	gREx.checksum = crc.get();
	gREx.headerVersion = kRExHeaderVersion;
	gREx.length = (ULong)rexLength;
	gREx.count = numOfConfigEntries;

#if defined(hasByteSwapping)
	// byte-swap the directory
	ULong * p = (ULong *)&gREx;
	for (ArrayIndex i = 0; i < sizeof(RExHeader)/sizeof(ULong); ++i, ++p) {
		*p = BYTE_SWAP_LONG(*p);
	}
	// including config directory entries
	entry = entries;
	for (ArrayIndex i = 0; i < numOfConfigEntries; ++i, ++entry) {
		// byte swap directory entry
		entry->tag = BYTE_SWAP_LONG(entry->tag);
		entry->offset = BYTE_SWAP_LONG(entry->offset);
		entry->length = BYTE_SWAP_LONG(entry->length);
	}
#endif

	// write out header
	cout.write((const char *)&gREx, sizeof(RExHeader));
	// write out config entry directory
	cout.write((const char *)entries, numOfConfigEntries * sizeof(ConfigEntry));
	delete[] entries;

	// write out config entries
	for (ArrayIndex i = 0; i < numOfConfigEntries; ++i) {
//		C588(0xE0);
		Block * block = (Block *)gConfigs->at(i);
		cout << *block->data;
	}
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C o m m a n d   l i n e
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Print correct command usage.
	Args:		inMessage1		error message
				inMessage2		supplementary text, appended to inMessage1, or NULL
	Return:	--
----------------------------------------------------------------------------- */

void
Usage(const char * inMessage1, const char * inMessage2)
{
	if (inMessage1 != NULL) {
		cerr << "### " << inMessage1;
		if (inMessage2 != NULL) {
			cerr << " " << inMessage2;
		}
		cerr << endl;
	}
	cerr << "### Usage: " << gCmdName
	<< " [-help] [-o <output-file>] [<config-file>]\n"
	<< "###     -help             print this message\n"
	<< "###     -o <output-file>  set output file name\n"
	<< "### Config file clauses:\n";
	CClauseHandler::printClauseHelp();

	exit(1);
}


/* -----------------------------------------------------------------------------
	Half case-insensitive string comparison.
	Args:		inStr			the string to compare -- mixed case
				inOptionStr	reference string -- MUST be lower-case
	Return:	0 => match
----------------------------------------------------------------------------- */

int
hcistrcmp(const char * inStr, const char * inOptionStr)
{
	char  ch;
	int	delta;
	do {
		ch = *inStr++;
		if ((delta = *inOptionStr++ - tolower(ch)) != 0)
			return delta;
	} while (ch != 0);
	return 0;
}


/* -----------------------------------------------------------------------------
	Parse command line options.
	Args:		inFirstArg		pointer to first arg string, ie NOT the command
				inLastArg		pointer past end of args array
	Return:	--
----------------------------------------------------------------------------- */

void
ParseOptions(const char ** inFirstArg, const char ** inLastArg)
{
	for (const char ** argp = inFirstArg; argp < inLastArg; ++argp) {
		const char * option = *argp;
		if (option[0] == '-') {
			// must be an option; only two allowed, -help and -o
			if (hcistrcmp(option+1, "help") == 0) {
				Usage(NULL, NULL);
			} else if (hcistrcmp(option+1, "o") == 0) {
				if (argp >= inLastArg-1) {
					Usage("expected filename with ", option);
				}
				gOutputFilename = *++argp;
			}
		} else {
			// must be input filename
			if (gInputFilename != NULL) {
				char str[256];
				sprintf(str, "already have an input file (%s)--what's", gInputFilename);
				Usage(str, option);
			}
			gInputFilename = option;
		}
	}
}

/* -----------------------------------------------------------------------------
	And this is where our story REALLY starts...
	Args:		argc			number of command line args
				argv			those args in full
	Return:	exit code
----------------------------------------------------------------------------- */

int
main(int argc, const char * argv[])
{
	// init config clause handlers
	InitHandlers();

	// parse the command line
	gCmdName = argv[0];
	ParseOptions(&argv[1], &argv[argc]);

	// set up the input stream
	streambuf * cinbuf = NULL;
	ifstream * _is = NULL;
	if (gInputFilename != NULL) {
		_is = new ifstream(gInputFilename);
		if (!_is->is_open()) {
			FatalError("can't open “%s”", gInputFilename);
		}
		cinbuf = cin.rdbuf();		// save old buf
		cin.rdbuf(_is->rdbuf());	// redirect std::cin to gInputFilename
	} else {
		gInputFilename = "<stdin>";
	}
	// and the output stream
	streambuf * coutbuf = NULL;
	ofstream * _os = NULL;
	if (gOutputFilename != NULL) {
		_os = new ofstream(gOutputFilename);
		if (!_os->is_open()) {
			FatalError("can't open “%s”", gOutputFilename);
		}
		coutbuf = cout.rdbuf();		// save old buf
		cout.rdbuf(_os->rdbuf());	// redirect std::cout to gOutputFilename
	} else {
		gOutputFilename = "<stdout>";
	}

	InitBlocks();
	ProcessConfigFile();
	PerformOutput();

	// all done with i/o now
	if (_is) {
		cin.rdbuf(cinbuf);	// reset to standard input
		delete _is;
	}
	if (_os) {
		cout.rdbuf(coutbuf);	// reset to standard output
		delete _os;
	}

	return 0;
}

