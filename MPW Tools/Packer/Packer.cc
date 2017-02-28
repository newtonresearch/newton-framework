/*
	File:		Packer.cc

	Contains:	Command line tool to ... erm... pack a number of files into one package file.

	Written by:	The Newton Tools group.
*/

#include "NewtonKit.h"
#include "AIF.h"
#include "Reporting.h"
#include "StringTable.h"
#include "Relocation.h"

#include <ctype.h>


/* -----------------------------------------------------------------------------
	T y p e s
----------------------------------------------------------------------------- */

struct PartFileInfo
{
	const char *	fName;				//+00
	bool				fIsAIF;				//+04
	int32_t *		fRelocations;		//+06
	ArrayIndex		fNumOfRelocations;//+0A
	ArrayIndex		fPartIndex;			//+0E
	ArrayIndex		fBase;				//+12 18(dec)
	ArrayIndex		fOffset;				//+16
	ULong				fLogicalAddress;	//+1A
//size+1E 30(dec)
};


#pragma mark -
/* -----------------------------------------------------------------------------
	G l o b a l   D a t a
----------------------------------------------------------------------------- */

const char *	gPartTypes[5] = {"-protocol","-frames","-raw","-package",NULL};	//glob143
const char *	gPartFlags[6] = {"-autoload","-autoremove","-compressed","-notify","-autocopy",NULL};	//-1BD4 MUST be in same order as part flag bits for PartFlagsFromStringTableIndex to work

ULong				gBaseAddress = 0;			//-1B62 glob144
ULong				gPkgFlags = 0;				//-1B5E glob145
ULong				gPkgVersion = 0;			//-1B5A glob146
ULong				gPkgId = 0;					//-1B56 glob147
ArrayIndex		gNumOfPartEntries = 0;	//-1B46 glob149

ULong				gFileType = 'pkg ';		//-1B42 glob150
const char *	gFileCreator = NULL;		//-1B3E
const char *	gDumpFilename = NULL;	//-1B3A glob152
const char *	gOutputFilename = NULL;	//-1B36 glob153
const char *	gImageFilename = NULL;

bool				gDoCopyFlags = false;	//-1B26 glob154
bool				gIsAIF = false;			//-1B24 glob155
const char *	gCopyright;					//-1B22 glob156 copyright string
const char *	gPackageName;				//-1B1E?glob157 package name

const char *	gUsageStr[5] =				//-1B1A
{"usage:\n# Packer [-p] ([-o [-aif] outputFile] packageName [-packageID id] [-version vers] [-c outputCreator] [-copyright string] <miscpackageflags> <partspec>…) | -dump dumpFile)\n",
"# <miscpackageflags> :: [-invisible] [-copyprotected] [-dispatchonly] [-nocompression] [-base <address>]\n",
"# <partspec> :: ([(-protocol | -frames | -raw) [-aif] partFile [-autoLoad] [-autoRemove] [-autoCopy] [-compressed compressor] [-notify partType info] ])\n",
"                | (-package packageFile [-index number] [-copyflags])\n"};

const char *	gCmdName;					//glob175


#pragma mark -
/* -----------------------------------------------------------------------------
	Memory allocation.
	Functions that report an error on allocation failure.
----------------------------------------------------------------------------- */

void *
calloc_or_exit(unsigned int inNumItems, size_t inSize, const char * inMessage)
{
	void * p = calloc(inNumItems, inSize);
	if (p == NULL) {
		ErrorMessage(inMessage, inSize * inNumItems);	// the original only reports 1*inSize
	}
	return p;
}


void *
malloc_or_exit(size_t inSize, const char * inMessage)
{
	void * p = malloc(inSize);
	if (p == NULL) {
		ErrorMessage(inMessage, inSize);
	}
	return p;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	Misc.
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	String comparison.
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
	Look for a string within a table.
	Return its index if found, -1 if not there.
----------------------------------------------------------------------------- */

int
StringTableMatch(const char * inStr, const char ** inTable)
{
	if (*inStr != 0) {
		for (int i = 0; *inTable != NULL; ++inTable, ++i) {
			if (hcistrcmp(inStr, *inTable) == 0) {
				return i;
			}
		}
	}
	return -1;
}


/* -----------------------------------------------------------------------------
	Pad file size.
	Return:	error code
----------------------------------------------------------------------------- */

int
PadFileToLongAlignment(FILE * ioFile, size_t inSize, const char * inFilename)
{
	int err = 0;
	size_t padding = 4 - (inSize & 0x03);
	if (padding != 4) {
		uint32_t zero = 0;
		size_t numWritten = fwrite(&zero, padding, 1, ioFile);
		if (numWritten != 1)	err = EIO;
		ReportOSErr(err, "can’t write %lu bytes of padding to output file “%s”", padding, inFilename);
	}
	return err;
}


int
PadFileBy(FILE * ioFile, size_t inSize, const char * inFilename)
{
	int err = 0;
	size_t padding;
	char zero[256];
	memset(zero, 0, sizeof(zero));
	for ( ; inSize != 0; inSize -= padding) {
		padding = inSize > 256? 256 : inSize;
		size_t numWritten = fwrite(zero, padding, 1, ioFile);
		if (numWritten != 1)	err = EIO;
		if (ReportOSErr(err, "can’t write %lu bytes of padding to output file “%s”", padding, inFilename)) break;
	}
	return err;
}


void
NotifyProc(void *, long *, long *, long)
{}


#pragma mark -
/* -----------------------------------------------------------------------------
	C o m m a n d   l i n e
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Ensure there is an arg following.
	Args:		inArg			pointer to first arg string
				inArgLimit	pointer to last arg string
				inNextArg	description of next arg expected
				inUsage		array of strings describing correct usage
	Return:	--
----------------------------------------------------------------------------- */

void
AssureNextArg(const char ** inArg, const char ** inArgLimit, const char * inNextArg, const char ** inUsage)
{
	if (inArg + 1 >= inArgLimit)
		UsageErrorMessage(inUsage, "no %s follows “%s”", inNextArg, *inArg);
}


/* -----------------------------------------------------------------------------
	Parse command line options and set globals accordingly.
	Also modify the arg list so it contains only part entry specs.
	Args:		inArg			pointer to first arg string
				inArgLimit	pointer to last arg string
	Return:	pointer to last part spec arg
----------------------------------------------------------------------------- */

const char **
ParseOptions(const char ** inArg, const char ** inArgLimit)
{
	const char * partCompressor = NULL;
	const char * notifyPartType = NULL;
	const char * partIndex = NULL;
	const char ** partArg = inArg;

	for (const char ** argp = inArg; argp < inArgLimit; argp++)
	{
		const char * option = *argp;
		if (hcistrcmp(option, "-o") == 0) {
			AssureNextArg(argp, inArgLimit, "output file name", gUsageStr);	//m1B1A
			// might actually be -aif option rather than filename
			if (hcistrcmp(option, "-aif") == 0) {
				gIsAIF = true;
				AssureNextArg(argp, inArgLimit, "output file name", gUsageStr);
			}
			gOutputFilename = *++argp;
		}

		else if (hcistrcmp(option, "-dump") == 0) {
			AssureNextArg(argp, inArgLimit, "dump file name", gUsageStr);
			gDumpFilename = *++argp;
		}

		else if (hcistrcmp(option, "-p") == 0) {
			gDoProgress = true;
		}

		else if (hcistrcmp(option, "-copyright") == 0) {
			AssureNextArg(argp, inArgLimit, "copyright string", gUsageStr);
			gCopyright = *++argp;
		}

		else if (hcistrcmp(option, "-c") == 0) {
			AssureNextArg(argp, inArgLimit, "output file creator", gUsageStr);
			gFileCreator = *++argp;
		}

		else if (hcistrcmp(option, "-packageid") == 0) {
			const char * pkgIdStr;
			AssureNextArg(argp, inArgLimit, "package id", gUsageStr);
			pkgIdStr = *++argp;
			ErrorMessageIf(strlen(pkgIdStr) != 4, "package id “%s” must be 4 bytes long", pkgIdStr);
			gPkgId = (pkgIdStr[0] << 24) | (pkgIdStr[1] << 16) | (pkgIdStr[2] << 8) | pkgIdStr[3];
		}

		else if (hcistrcmp(option, "-copyprotected") == 0) {
			gPkgFlags |= kCopyProtectFlag;
		}

		else if (hcistrcmp(option, "-invisible") == 0) {
			gPkgFlags |= kInvisibleFlag;
		}

		else if (hcistrcmp(option, "-dispatchonly") == 0) {
			gPkgFlags |= 0x80;
		}

		else if (hcistrcmp(option, "-nocompression") == 0) {
			gPkgFlags |= kNoCompressionFlag;
		}

		else if (hcistrcmp(option, "-base") == 0) {
			AssureNextArg(argp, inArgLimit, "base address", gUsageStr);
			gBaseAddress = strtoul(*++argp, 0, 0);
		}

		else if (hcistrcmp(option, "-version") == 0) {
			AssureNextArg(argp, inArgLimit, "version number", gUsageStr);
			gPkgVersion = atol(*++argp);
		}
//+0344
		else if (StringTableMatch(option, gPartTypes) != -1) {	//-1B98
			AssureNextArg(argp, inArgLimit, "part file name", gUsageStr);	//m3116
			*partArg++ = option;
			// might be -aif before part file name
			if (hcistrcmp(argp[1], "-aif") == 0) {	//m3106
				++argp;
				*partArg++ = *argp;
				AssureNextArg(argp, inArgLimit, "part file name", gUsageStr);	//m3100
			}
			++argp;
			*partArg++ = *argp;
			// we have a part
			++gNumOfPartEntries;
			// reset part parms
			partCompressor = NULL;
			partIndex = NULL;
			notifyPartType = NULL;
		}
//+03E8
		else if (StringTableMatch(option, gPartFlags) != -1) {
			*partArg++ = option;
		}
//+040E
		else if (hcistrcmp(option, "-compressed") == 0) {	//m30F0
			AssureNextArg(argp, inArgLimit, "compression type", gUsageStr);	//m30E4
			if (partCompressor != NULL) {
				ErrorMessage("-compressed specified twice for one part: “%s” and “%s”", partCompressor, *++argp);	//m30D2
			}
			*partArg++ = option;
			++argp;
			*partArg++ = *argp;
			partCompressor = *argp;
		}
//+047C
		else if (hcistrcmp(option, "-index") == 0) {	//m3098
			AssureNextArg(argp, inArgLimit, "part index", gUsageStr);	//m3090
			if (partIndex != NULL) {
				ErrorMessage("-index specified twice for one part: “%s” and “%s”", partIndex, *++argp);	//m3084
			}
			*partArg++ = option;
			++argp;
			*partArg++ = *argp;
			partIndex = *argp;
		}
//+04EA
		else if (hcistrcmp(option, "-copyflags") == 0) {	//m3050
			ErrorMessageIf(gDoCopyFlags, "-copyflags specified twice");	//m3044
			gDoCopyFlags = true;
			*partArg++ = option;
		}
//+052E
		else if (hcistrcmp(option, "-notify") == 0) {	//m3028
			AssureNextArg(argp, inArgLimit, "part type", gUsageStr);	//m3020
			ErrorMessageIf(notifyPartType, "-notify specified twice for one part");	//m3016
			*partArg++ = option;
			++argp;
			*partArg++ = *argp;
			notifyPartType = *argp;
			ErrorMessageIf(strlen(notifyPartType) != 4, "part type “%s” must be 4 bytes long", notifyPartType);	//m2FF0
			AssureNextArg(argp, inArgLimit, "part info", gUsageStr);	//m2FCC
			++argp;
			*partArg++ = *argp;
		}
//+05E8
		else if (hcistrcmp(option, "-aif") == 0) {	//m2FC2
			ErrorMessage("“-aif” can only appear immediately after “-o”, “-protocol”, “-frames” or “-raw”");	//m2FBC
		}
//+060C
		else if (*option == '-') {
			UsageErrorMessage(gUsageStr, "%s is not an option", option);
		}

		else {
			ErrorMessageIf(gPackageName != NULL, "package name specified twice:  “%s” and “%s”", gPackageName, option);	//m2F6A
			gPackageName = option;
		}
	}

	return partArg;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	Return part flag bit from index into gPartFlags.
----------------------------------------------------------------------------- */

ULong
PartFlagsFromStringTableIndex(ArrayIndex index)
{
	return 1 << (4+(index&0xFF));
}


/* -----------------------------------------------------------------------------
	Convert unicode -> C string.
	Args:		inStringData	pointer to string data of package
				info				offset|length of unicode
	Return:	pointer to C string
				this will be nul-terminated iff the unicode is
----------------------------------------------------------------------------- */

char *
UnUnicode(const char * inStringData, InfoRef info)
{
	static char buf[256];
	const UniChar * ustr = (const UniChar *)(inStringData + info.offset);
	char * str = buf;
	for (ArrayIndex i = info.length; i > 0; --i, ++ustr) {
		*str++ = CANONICAL_SHORT(*ustr);
	}
	return buf;
}


/* -----------------------------------------------------------------------------
	Read part data from file into malloc’d block.
	Args:		info			part file descriptor
				inSize		amount to read
				outPartData	return pointer to data read
	Return:	error code
----------------------------------------------------------------------------- */
void *	gPartData = NULL;					//-1CD8
void		DisposePartData(void);

int
ReadPartData(const PartFileInfo * info, size_t inSize, void ** outPartData)
{
	int err = 0;
	XTRY {
		FILE * fp = fopen(info->fName, "r");
		err = (fp == NULL)? EIO:0;
		XFAIL(ReportOSErr(err, "can’t open input part file “%s”", info->fName))
		XTRY {
			err = fseek(fp, info->fOffset, SEEK_SET);
			XFAIL(ReportOSErr(err, "can’t seek to position %u of part file “%s”", info->fOffset, info->fName))
			gPartData = (Ptr)malloc(inSize);
			if (gPartData == NULL) err = ENOMEM;
			XFAIL(ReportOSErr(err, "can’t allocate the part buffer"));
			size_t numRead = fread(gPartData, inSize, 1, fp);
			if (numRead != 1) err = EIO;
			XFAIL(ReportOSErr(err, "can’t read %lu bytes for part %d from part file “%s”", inSize, info->fPartIndex, info->fName));
		} XENDTRY;

		int closErr = fclose(fp);
		ReportOSErr(closErr, "can’t close input part file");	//m2E04
		if (err == 0) {
			err = closErr;
		}
	} XENDTRY;

	if (err) {
		DisposePartData();
	} else {
		*outPartData = gPartData;
	}

	return err;
}


/* -----------------------------------------------------------------------------
	Dispose part data read from file.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
DisposePartData(void)
{
	free(gPartData);
	gPartData = NULL;
}


/* -----------------------------------------------------------------------------
	Command-line tool signal handling.
----------------------------------------------------------------------------- */

void
myexit(void)
{
	if (gPartData) {
		free(gPartData);	// the original used a Handle
		//ReportOSErr(err, "can’t dispose part buffer");
		gPartData = NULL;
	}
}


void
mysignal(int inSignal)
{
	fprintf(stderr, "### %s: Ouch!\n", gCmdName);
	exit(1);
}


/* -----------------------------------------------------------------------------
	Copy part from one file to another.
	Args:		ioFile			destination
				info				source part file descriptor
				inSize
				inArg4
				inArg5
				inPlainFrames
				inArg7
	Return:	error code
----------------------------------------------------------------------------- */

int
copyFileTo(FILE * ioFile, const PartFileInfo * info, size_t inSize, ArrayIndex inArg4, ULong inArg5, bool inPlainFrames, const CRelocationCollector * inArg7)
{
	int err;
	void * partData;
	err = ReadPartData(info, inSize, &partData);
	if (err == 0) {
		if (inPlainFrames) {
			Progress("frames relocating “%s” to 0x%08lx == 0x%08lx - 0x%08lx", info->fName, inArg5 - info->fLogicalAddress, inArg5, info->fLogicalAddress);
			RelocateFrames(partData, inSize, inArg5 - info->fLogicalAddress);
		}
		inArg7->relocate(inArg4, inArg5, (ULong *)partData, inSize);
		size_t numWritten = fwrite(partData, inSize, 1, ioFile);
		if (numWritten != 1) err = EIO;
		ReportOSErr(err, "can’t write part data to output file “s”", gOutputFilename);
	}
	return err;
}



ArrayIndex
ConvertInfoString(void * outBuf, ArrayIndex inCount, const char * infoStr)
{
	ArrayIndex i;
	char * p = (char *)outBuf;
	for (i = 0; i < inCount && *infoStr != 0; ++i) {
		if (infoStr[0] == '\\' && infoStr[1] == '$') {
		// map character encoded "\$xx" to that character
			char buf[4];
			buf[0] = infoStr[2];
			buf[1] = infoStr[3];
			buf[2] = 0;
			infoStr += 4;
			*p++ = strtol(buf, 0, 16);
		} else {
		//	copy character
			*p++ = *infoStr++;
		}
	}
	return i;
}


/* -----------------------------------------------------------------------------
	Print part notify info string.
	Args:		inStr		pointer to info string
				inLen		length of string
	Return:	--
----------------------------------------------------------------------------- */

void
PutInfoString(const char * inStr, ArrayIndex inLen)
{
	putchar('\'');
	while (inLen-- > 0) {
		int ch = *inStr++;
		if ((isgraph(ch) && ch != '"') || ch == ' ')
			putchar(ch);
		else
			printf("\\$%02x", ch);
	}
	putchar('\'');
}



/* -----------------------------------------------------------------------------
	Print package info.
	Args:		inFilename		name of package file to examine
	Return:	--
----------------------------------------------------------------------------- */

void
DumpPackageInfo(const char * inFilename)
{
	int err;
	FILE * fp = fopen(inFilename, "r");
	err = (fp == NULL)? EIO:0;
	ExitIfOSErr(err, "can’t open package file “%s”", inFilename);

	PackageDirectory pkgDir;
	size_t sizeReqd = sizeof(PackageDirectory);
	size_t numRead = fread(&pkgDir, sizeReqd, 1, fp);
	if (numRead != 1) err = EIO;
	ExitIfOSErr(err, "can’t read %lu bytes for package header package file “%s”", sizeReqd, inFilename);

#if defined(hasByteSwapping)
	pkgDir.id = BYTE_SWAP_LONG(pkgDir.id);
	pkgDir.flags = BYTE_SWAP_LONG(pkgDir.flags);
	pkgDir.version = BYTE_SWAP_LONG(pkgDir.version);
	pkgDir.copyright.offset = BYTE_SWAP_SHORT(pkgDir.copyright.offset);
	pkgDir.copyright.length = BYTE_SWAP_SHORT(pkgDir.copyright.length);
	pkgDir.name.offset = BYTE_SWAP_SHORT(pkgDir.name.offset);
	pkgDir.name.length = BYTE_SWAP_SHORT(pkgDir.name.length);
	pkgDir.size = BYTE_SWAP_LONG(pkgDir.size);
	pkgDir.creationDate = BYTE_SWAP_LONG(pkgDir.creationDate);
	pkgDir.modifyDate = BYTE_SWAP_LONG(pkgDir.modifyDate);
	pkgDir.directorySize = BYTE_SWAP_LONG(pkgDir.directorySize);
	pkgDir.numParts = BYTE_SWAP_LONG(pkgDir.numParts);
#endif

	if (memcmp(pkgDir.signature, "package0", 8) == 0) {
		printf("# package is version 0\n");
	} else if (memcmp(pkgDir.signature, "package1", 8) == 0) {
		printf("# package is version 1\n");
	} else {
		ErrorMessage("file “%s” does not seem to be a package file", inFilename);
	}

	// the original splits the package directory into a package header and parts directory header
	// reading them separately

	size_t sizeOfPartEntries = pkgDir.numParts * sizeof(PartEntry);
	PartEntry * partEntries = (PartEntry *)calloc_or_exit(pkgDir.numParts, sizeof(PartEntry), "can’t allocate %lu bytes for directory entries");
	numRead = fread(partEntries, sizeof(PartEntry), pkgDir.numParts, fp);
	if (numRead != pkgDir.numParts) err = EIO;
	ExitIfOSErr(err, "can’t read %lu bytes for part directory entries from package file “%s”", sizeOfPartEntries, inFilename);

#if defined(hasByteSwapping)
	PartEntry * pe = partEntries;
	for (ArrayIndex i = 0; i < pkgDir.numParts; ++i, ++pe)
	{
		pe->offset = BYTE_SWAP_LONG(pe->offset);
		pe->size = BYTE_SWAP_LONG(pe->size);
		pe->size2 = pe->size;
		pe->type = BYTE_SWAP_LONG(pe->type);
		pe->flags = BYTE_SWAP_LONG(pe->flags);
		pe->info.offset = BYTE_SWAP_SHORT(pe->info.offset);
		pe->info.length = BYTE_SWAP_SHORT(pe->info.length);
		pe->compressor.offset = BYTE_SWAP_SHORT(pe->compressor.offset);
		pe->compressor.length = BYTE_SWAP_SHORT(pe->compressor.length);
	}
#endif

	sizeReqd = pkgDir.directorySize - (sizeof(PackageDirectory) + sizeOfPartEntries);
	Ptr stringData = (Ptr)malloc_or_exit(sizeReqd, "can’t allocate %lu bytes for string table");	//van8
	numRead = fread(stringData, sizeReqd, 1, fp);
	if (numRead != 1) err = EIO;
	ExitIfOSErr(err, "can’t read %lu bytes for string table from “%s”", sizeReqd, inFilename);

	size_t sizeOfRelocationInfo = 0;	//d6
	RelocationHeader pkgRelo;
	if (FLAGTEST(pkgDir.flags, kRelocationFlag)) {
		sizeReqd = sizeof(RelocationHeader);
		numRead = fread(&pkgRelo, sizeReqd, 1, fp);
		if (numRead != 1) err = EIO;
		ExitIfOSErr(err, "can’t read %lu bytes for relocation header from “%s”", sizeReqd, inFilename);

#if defined(hasByteSwapping)
		pkgRelo.relocationSize = BYTE_SWAP_LONG(pkgRelo.relocationSize);
		pkgRelo.pageSize = BYTE_SWAP_LONG(pkgRelo.pageSize);
		pkgRelo.numEntries = BYTE_SWAP_LONG(pkgRelo.numEntries);
		pkgRelo.baseAddress = BYTE_SWAP_LONG(pkgRelo.baseAddress);
#endif

		sizeOfRelocationInfo = pkgRelo.relocationSize;
	}

	char pkgId[5];
	*(ULong *)pkgId = pkgDir.id;
	pkgId[4] = 0;
	printf("%s %s -o \"%s\" -packageID '%s' -version %u ∂\n", gCmdName, gDoProgress? "-p":"", inFilename, pkgId, pkgDir.version);
	printf("\"%s\"", UnUnicode(stringData, pkgDir.name));
	if (FLAGTEST(pkgDir.flags, kCopyProtectFlag)) printf(" -copyprotected");
	if (FLAGTEST(pkgDir.flags, kAutoRemoveFlag)) printf(" -dispatchonly");
	if (FLAGTEST(pkgDir.flags, kNoCompressionFlag)) printf(" -nocompression");
	if (FLAGTEST(pkgDir.flags, kInvisibleFlag)) printf(" -invisible");
	if (pkgDir.copyright.length != 0) printf(" -copyright “%s”", UnUnicode(stringData, pkgDir.copyright));
	printf("∂\n# package flags are 0x%08x ∂\n", pkgDir.flags);
	if (FLAGTEST(pkgDir.flags, kRelocationFlag)) printf("# package has %lu bytes of relocation info starting at file offset 0x%08x ∂\n", sizeOfRelocationInfo, pkgDir.directorySize);
	printf("# total size is %u, %u part%s: ∂\n", pkgDir.size, pkgDir.numParts, pkgDir.numParts == 1? "":"s");

	PartEntry * p = partEntries;
	for (ArrayIndex i = 0; i < pkgDir.numParts; ++i, ++p) {
		char typeStr[5];
		*(ULong *)typeStr = CANONICAL_LONG(p->type);
		typeStr[4] = 0;
		printf("%s \"Part %d\" ", gPartTypes[p->type & kPartTypeMask], i);
		if (FLAGTEST(p->flags, kNotifyFlag)) {
			printf("-notify '%s' ", typeStr);
			PutInfoString(stringData + p->info.offset, p->info.length);
			putchar(' ');
		}
		printf("# file offset = 0x%08lx, size = 0x%08x, partflags = 0x%08x", pkgDir.directorySize + sizeOfRelocationInfo + p->offset, p->size, p->flags);

		if (FLAGTEST(p->flags, kAutoLoadPartFlag|kAutoRemovePartFlag|kAutoCopyFlag|kCompressedFlag)) {
			printf(" ∂\n");
			if (FLAGTEST(p->flags, kAutoLoadPartFlag)) printf(" -autoLoad");
			if (FLAGTEST(p->flags, kAutoRemovePartFlag)) printf(" -autoRemove");
			if (FLAGTEST(p->flags, kAutoCopyFlag)) printf(" -autoCopy");
			if (FLAGTEST(p->flags, kCompressedFlag)) printf(" -compressed \"%s\"", UnUnicode(stringData, p->compressor));
		}
		printf((i < pkgDir.numParts - 1)? " ∂\n":"\n");
	}

	free(partEntries);
	free(stringData);
	err = fclose(fp);
	ExitIfOSErr(err, "can’t close input package file “%s”", inFilename);
}


/* -----------------------------------------------------------------------------
	Prepare package part file.
	Args:		inFilename		name of package file to examine
				info				part file descriptor
				ioPart			PartEntry to be copied from package file
				ioPackage		PackageDirectory to be copied from package file
	Return:	part size
----------------------------------------------------------------------------- */

size_t
PrepPackagePartFile(const char * inFilename, PartFileInfo * const info, PartEntry * ioPart, PackageDirectory * ioPackage)
{
	int err;
	FILE * fp = fopen(inFilename, "r");	//vao4
	err = (fp == NULL)? EIO:0;
	ExitIfOSErr(err, "can’t open input package file “%s”", inFilename);

	info->fBase = 0;
	info->fNumOfRelocations = 0;

	PackageDirectory pkgDir;	//vao5
	size_t sizeReqd = sizeof(PackageDirectory);
	size_t numRead = fread(&pkgDir, sizeReqd, 1, fp);
	if (numRead != 1) err = EIO;
	ExitIfOSErr(err, "can’t read package file “%s”", inFilename);

#if defined(hasByteSwapping)
	pkgDir.id = BYTE_SWAP_LONG(pkgDir.id);
	pkgDir.flags = BYTE_SWAP_LONG(pkgDir.flags);
	pkgDir.version = BYTE_SWAP_LONG(pkgDir.version);
	pkgDir.copyright.offset = BYTE_SWAP_SHORT(pkgDir.copyright.offset);
	pkgDir.copyright.length = BYTE_SWAP_SHORT(pkgDir.copyright.length);
	pkgDir.name.offset = BYTE_SWAP_SHORT(pkgDir.name.offset);
	pkgDir.name.length = BYTE_SWAP_SHORT(pkgDir.name.length);
	pkgDir.size = BYTE_SWAP_LONG(pkgDir.size);
	pkgDir.creationDate = BYTE_SWAP_LONG(pkgDir.creationDate);
	pkgDir.modifyDate = BYTE_SWAP_LONG(pkgDir.modifyDate);
	pkgDir.directorySize = BYTE_SWAP_LONG(pkgDir.directorySize);
	pkgDir.numParts = BYTE_SWAP_LONG(pkgDir.numParts);
#endif

	if (memcmp(pkgDir.signature, "package0", 8) != 0 && memcmp(pkgDir.signature, "package1", 8) != 0) {
		ErrorMessage("file “%s” does not seem to be a package file", inFilename);
	}
	ErrorMessageIf(FLAGTEST(pkgDir.flags, kRelocationFlag), "package file “%s” has relocations: Packer cannot yet get parts from package files with relocations", inFilename);

	// as above, the original splits the package directory into a package header and parts directory header
	// reading them separately

	size_t sizeOfPartEntries = pkgDir.numParts * sizeof(PartEntry);	//d7
	PartEntry * partEntries = (PartEntry *)calloc_or_exit(pkgDir.numParts, sizeof(PartEntry), "can’t allocate %lu bytes for directory entries");	//vao2
	PartEntry * myPartEntry = partEntries + info->fPartIndex;	//a4
	numRead = fread(partEntries, sizeof(PartEntry), pkgDir.numParts, fp);
	if (numRead != pkgDir.numParts) err = EIO;
	ExitIfOSErr(err, "can’t read package file “%s”", inFilename);

#if defined(hasByteSwapping)
	PartEntry * pe = partEntries;
	for (ArrayIndex i = 0; i < pkgDir.numParts; ++i, ++pe)
	{
		pe->offset = BYTE_SWAP_LONG(pe->offset);
		pe->size = BYTE_SWAP_LONG(pe->size);
		pe->size2 = pe->size;
		pe->type = BYTE_SWAP_LONG(pe->type);
		pe->flags = BYTE_SWAP_LONG(pe->flags);
		pe->info.offset = BYTE_SWAP_SHORT(pe->info.offset);
		pe->info.length = BYTE_SWAP_SHORT(pe->info.length);
		pe->compressor.offset = BYTE_SWAP_SHORT(pe->compressor.offset);
		pe->compressor.length = BYTE_SWAP_SHORT(pe->compressor.length);
	}
#endif

	sizeReqd = pkgDir.directorySize - (sizeof(PackageDirectory) + sizeOfPartEntries);
	Ptr stringData = (Ptr)malloc_or_exit(sizeReqd, "can’t allocate %lu bytes for string table");	//vao3
	numRead = fread(stringData, sizeReqd, 1, fp);
	if (numRead != 1) err = EIO;
	ExitIfOSErr(err, "can’t read string table from “%s”", inFilename);

	gPkgId = ioPackage->id = pkgDir.id;
	gPkgFlags = ioPackage->flags = pkgDir.flags;
	gPkgVersion = ioPackage->version = pkgDir.version;
	if (pkgDir.copyright.length != 0) {
		ioPackage->copyright = AddBytes(stringData + pkgDir.copyright.offset, pkgDir.copyright.length);
	}
	if (pkgDir.name.length != 0) {
		ioPackage->name = AddBytes(stringData + pkgDir.name.offset, pkgDir.name.length);
	}
	info->fOffset = pkgDir.directorySize + myPartEntry->offset;
	info->fLogicalAddress = info->fBase + info->fOffset;

	Progress("part %d, “%s” input offset = 0x%08x = 0x%08x + 0x%08x; logical start address = 0x%08x", info->fPartIndex, inFilename, info->fOffset, pkgDir.directorySize, myPartEntry->offset, info->fLogicalAddress);

	ioPart->type = myPartEntry->type;
	ioPart->reserved1 = myPartEntry->reserved1;
	ioPart->flags = myPartEntry->flags;
	if (myPartEntry->info.length != 0) {
		ioPart->info = AddBytes(stringData + myPartEntry->info.offset, myPartEntry->info.length);
	}
	if (myPartEntry->compressor.length != 0) {
		ioPart->compressor = AddBytes(stringData + myPartEntry->compressor.offset, myPartEntry->compressor.length);
	}

	free(partEntries);
	free(stringData);

	err = fclose(fp);
	ExitIfOSErr(err, "can’t close input package file “%s”", inFilename);

	return myPartEntry->size;
}


/* -----------------------------------------------------------------------------
	Prepare AIF part file.
	Args:		inFilename		name of AIF part file to examine
				info				part file descriptor
	Return:	part size (of AIF data)
----------------------------------------------------------------------------- */

size_t
PrepAIFFile(const char * inFilename, PartFileInfo * info)
{
	int err;
	FILE * fp = fopen(inFilename, "r");	//vap4
	err = (fp == NULL)? EIO:0;
	ExitIfOSErr(err, "can’t open input AIF part file “%s”", inFilename);

	AIFHeader header;	//vap5
	size_t numRead = fread(&header, sizeof(AIFHeader), 1, fp);
	if (numRead != 1) err = EIO;
	ExitIfOSErr(err, "can’t read AIF header of part file “%s”", inFilename);

	ErrorMessageIf(header.exitCode != 0xEF000011, "part file “%s” does not seem to be in AIF format", inFilename);
	ErrorMessageIf(MISALIGNED(header.imageBase), "bad AIF part file with unaligned image base 0x%08x", header.imageBase);
	ErrorMessageIf(MISALIGNED(header.roSize), "bad AIF part file with unaligned read-only size 0x%08x", header.roSize);
	ErrorMessageIf(MISALIGNED(header.rwSize), "bad AIF part file with unaligned read-write size 0x%08x", header.rwSize);
	WarningMessageIf(header.rwSize != 0, "part file “%s” seems to have %u bytes of initialized data", inFilename);
	WarningMessageIf(header.zeroInitSize != 0, "part file “%s” seems to have %u bytes of zero-initialized data", inFilename);

	bool isExe = *(uint8_t *)&header.imageEntryCode == 0xEB;	//d7

	Progress("“%s” is %s AIF file", inFilename, isExe? "an executable":"a non-executable");

	info->fBase = header.imageBase + (isExe? 0x80:0);
	info->fOffset = (isExe? 0:0x80);
	if (header.selfRelocCode != 0 && header.selfRelocCode != 0xE1A00000) {
		ErrorMessageIf(isExe, "part file “%s” is relocatable, executable AIF format -- I can’t handle it unless it is '-aif -bin -reloc', sorry", inFilename);

		ArrayIndex numOfRelocations;	//vap2
		err = fseek(fp, header.selfRelocCode, SEEK_SET);
		ExitIfOSErr(err, "can’t seek to position %u of AIF part file “%s”", header.selfRelocCode, inFilename);
		numRead = fread(&numOfRelocations, sizeof(numOfRelocations), 1, fp);
		if (numRead != 1) err = EIO;
		ExitIfOSErr(err, "can’t read AIF relocation count of part file “%s”", inFilename);

		Progress("AIF part file “%s” has %u relocation%s", inFilename, numOfRelocations, numOfRelocations == 1? "":"s");

		if (numOfRelocations > 0) {
			size_t sizeOfRelocations = numOfRelocations * sizeof(int32_t);	//vap3
			info->fRelocations = (int32_t *)malloc_or_exit(sizeOfRelocations, "can’t allocate %lu bytes for AIF relocations");
			numRead = fread(info->fRelocations, sizeOfRelocations, 1, fp);
			if (numRead != 1) err = EIO;
			ExitIfOSErr(err, "can’t read AIF relocations of part file “%s”", inFilename);

			ErrorMessageIf(FLAGTEST(header.addressMode, 0x0100), "I can’t yet handle AIF part files with relocations that have been linked with -data");
		}
		info->fNumOfRelocations = numOfRelocations;
	}

	err = fclose(fp);
	ExitIfOSErr(err, "can’t close input AIF part file “%s”", inFilename);
	return header.roSize + header.rwSize;
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
	int err, i;

// install signal handlers: kill, interrupt

	gCmdName = argv[0];
	const char ** firstArg = &argv[1];
	const char ** lastArg = ParseOptions(&argv[1], &argv[argc]);	//vaq_40

	if (gDumpFilename != NULL) {
		DumpPackageInfo(gDumpFilename);
		return 0;
	}

	bool isDone = false;	//vaq_41
	PackageDirectory pkgDir;	//vaq_42 onwards
	memcpy(pkgDir.signature, "package0", 8);
	pkgDir.id = 0;
	pkgDir.size = 0;
	pkgDir.creationDate = 0;
	pkgDir.modifyDate = 0;

	if (!gDoCopyFlags) {
		if (gCopyright) {
			Progress("copyright string is “%s”", gCopyright);
			pkgDir.copyright = AddUnicodeString(gCopyright);
		} else {
			Progress("no copyright string");
			pkgDir.copyright = {0,0};
		}

		if (gPackageName) {
			Progress("package name is “%s”", gPackageName);
			pkgDir.name = AddUnicodeString(gPackageName);
		} else {
			UsageErrorMessage(gUsageStr, "can’t find package name");
		}
	}
	pkgDir.id = gPkgId;
	pkgDir.flags = gPkgFlags;
	pkgDir.version = gPkgVersion;

	// allocate PartEntry array
	size_t sizeOfPartEntries = gNumOfPartEntries * sizeof(PartEntry);	//vaq_34
	PartEntry * partEntries = (PartEntry *)calloc_or_exit(gNumOfPartEntries, sizeof(PartEntry), "can’t allocate %lu bytes for directory entries");	//vaq_35 glob93
	PartEntry * partEntry = partEntries;	//vaq_36
	// allocate PartFileInfo array
	PartFileInfo * partFiles = (PartFileInfo *)malloc_or_exit(gNumOfPartEntries * sizeof(PartFileInfo), "can’t allocate %lu bytes for file infos");	//vaq_37
	PartFileInfo * partFile = partFiles;	//vaq_38
	ULong partEntryOffset = 0;	//vaq_39

	XTRY {
		CRelocationCollector relocations(gNumOfPartEntries);	//vaq_30
		// iterate over args per part
		for (const char ** argp = firstArg; argp < lastArg; ++partEntry, ++partFile) {
			InternalErrorIf(partEntry > partEntries + gNumOfPartEntries, "ran off end of partEntries");
			bool copyflags = false;	//vaq_18 -E7
			// init part file info
			partFile->fIsAIF = false;
			partFile->fRelocations = NULL;
			partFile->fNumOfRelocations = 0;
			partFile->fPartIndex = -1;
			partFile->fBase = 0;
			partFile->fOffset = 0;
			partFile->fLogicalAddress = 0;

			// expecting the type of part first
			int partType = StringTableMatch(*argp++, gPartTypes);
			if (partType == -1) {
				ErrorMessage("expecting “-protocol”, “-frames”, “-raw”, or “-package”, but found “%s”", *--argp);
			}
			// partType is 0..3 aligned with PartEntry flags
			if (partType == kPackagePart) {
				partFile->fPartIndex = 0;
				ErrorMessageIf((hcistrcmp(*argp, "-aif") == 0), "sorry, '-package -aif' is not implemented yet");
			} else {
				partEntry->flags = partType;
				partFile->fIsAIF = (hcistrcmp(*argp, "-aif") == 0);	//glob99
				if (partFile->fIsAIF) {
					++argp;
				}
			}

			// expeecting the filename of the part next
			const char * partFilename = *argp++;	//vaq_19
			partFile->fName = partFilename;
			Progress("part file name is “%s”", partFilename);

			// finally the options
			for ( ; argp < lastArg; ++argp) {
				if (StringTableMatch(*argp, gPartTypes) != -1) {
					// no spec for this part -- move along
					break;
				}
				const char * option = *argp;	//a4
				if (hcistrcmp(option, "-index") == 0) {
					if (partType == kPackagePart) {
						++argp;
						partFile->fPartIndex = atoi(*argp);
					} else {
						++argp;
						WarningMessage("Part “%s” ‘-index’ option ignored for non-package part", partFilename);
					}
				} else if (hcistrcmp(option, "-copyflags") == 0) {
					if (partType == kPackagePart) {
						copyflags = true;
					} else {
						WarningMessage("Part “%s” ‘-copyflags’ option ignored for non-package part", partFilename);
					}
				} else if (hcistrcmp(option, "-compressed") == 0) {
					if (partType == kPackagePart) {
						WarningMessage("Part “%s” ‘-compressed’ option ignored for pre-built package", partFilename);
						++argp;
					} else {
						partEntry->flags |= kCompressedFlag;
						++argp;
						partEntry->compressor = AddString(*argp);
					}
				} else if (hcistrcmp(option, "-notify") == 0) {
					if (partType == kPackagePart) {
						++argp;
						WarningMessage("Part “%s” ‘-notify’ option ignored for pre-built package", partFilename);
					} else {
						partEntry->flags |= kNotifyFlag;
						++argp;
						option = *argp;
						partEntry->type = (option[0]<<24) + (option[1]<<16) + (option[2]<<8) + option[3];	// need to think about byte-swapping
						if (partEntry->type == kPatchPartType) {
							gFileType = 'Ptch';
						}
						++argp;
						char strBuf[256];
						ArrayIndex strLen = ConvertInfoString(strBuf, 256, *argp);
						partEntry->info = AddBytes(strBuf, strLen);
					}
				} else if ((i = StringTableMatch(option, gPartFlags)) != -1) {
					if (partType == kPackagePart) {
						WarningMessage("Part “%s” ‘%s’ option ignored for pre-built package", partFilename, option);
					} else {
						partEntry->flags |= PartFlagsFromStringTableIndex(i);
					}
					break;
				} else {
					InternalErrorIf(true, "I didn’t expect to see “%s”", option);
					++argp;
				}
			}

			size_t partFileSize;	//vaq_25
			FILE * fp = fopen(partFilename, "rb");
			if ((err = (fp == NULL)? EIO:0) == 0) {
				if ((err = fseek(fp, 0, SEEK_END)) == 0) {
					partFileSize = ftell(fp);
				}
				fclose(fp);
			}
			ExitIfOSErr(err, "can’t get info on “%s”", partFilename);

			partEntry->offset = partEntryOffset;
			//vaq_28 = 0;
			//vaq_29 = NULL;
			size_t partSize;
			if (partType == kPackagePart) {
				partSize = PrepPackagePartFile(partFilename, partFile, partEntry, &pkgDir);
			} else if (partFile->fIsAIF) {
				partSize = PrepAIFFile(partFilename, partFile);
			} else {
				partSize = partFileSize;
			}
			void * framePart = NULL;	//vaq_16
			bool isPlainFrames = (FLAGTEST(partEntry->flags, kNOSPart) && !FLAGTEST(partEntry->flags, kCompressedFlag));
			if (isPlainFrames) {
				err = ReadPartData(partFile, partSize, &framePart);
				ExitIfOSErr(err, "can’t read frame part data from “%s”", partFilename);
			}

			ArrayIndex numOfRelocations = relocations.addRelocations(partFile->fBase, partSize, partFile->fNumOfRelocations, partFile->fRelocations, isPlainFrames, framePart, partFilename);

			if (isPlainFrames) {
				DisposePartData();
			}

			if (partFile->fNumOfRelocations > 0) {
				Progress("%u relocation%s kept for part", numOfRelocations, numOfRelocations == 1 ? "" : "s");
			}
			Progress("“%s” part length is %lu bytes", partFilename, partSize);
			partEntry->size = partEntry->size2 = partSize;
			pkgDir.size += partSize;
			partEntryOffset += partSize;
			pkgDir.size = LONGALIGN(pkgDir.size);
			partEntryOffset = LONGALIGN(partEntryOffset);
		}

		//vaq_29 = gStrings;
		size_t stringDataSize = gStrings.fileSize();
		size_t alignedStringDataSize = LONGALIGN(gStrings.fileSize());
		size_t pkgDirectorySize = sizeof(PackageDirectory) + sizeOfPartEntries + alignedStringDataSize;

		relocations.compute(gBaseAddress, pkgDirectorySize);
		size_t sizeOfRelocations = relocations.fileSize();
		pkgDir.size += relocations.fileSize();

		// create output package file
		FILE * fp = fopen(gOutputFilename, "w");	//vaq_33
		err = (fp == NULL)? EIO:0;
		XFAIL(ReportOSErr(err, "can’t open output file “%s”", gOutputFilename))

		XTRY {
			if (relocations.count() > 0) {
				pkgDir.flags |= kRelocationFlag;
				memcpy(pkgDir.signature, "package1", 8);
			}
			pkgDir.size += pkgDirectorySize;
			InternalErrorIf(MISALIGNED(pkgDir.size), "package size %u is not long aligned", pkgDir.size);

			Progress("package contains %u parts, package size is %u bytes", gNumOfPartEntries, pkgDir.size);
			size_t numWritten;

			if (gIsAIF) {
				// set up AIF header
				AIFHeader header;
				memset(&header, 0, sizeof(AIFHeader));
				header.imageEntryCode = gBaseAddress;
				header.exitCode = 0xEF000011;
				header.roSize = pkgDir.size;
				header.imageBase = gBaseAddress;
				header.addressMode = 0x20;
				header.zeroInit[0] = 0xEF041D41;

				Progress("writing AIF header");
				numWritten = fwrite(&header, sizeof(AIFHeader), 1, fp);
				if (numWritten != 1) err = EIO;
				XFAIL(ReportOSErr(err, "can’t write AIF header to output file “s”", gOutputFilename))
			}

			Progress("writing package header");
			// the original writes package header and parts directory header separately
			pkgDir.reserved3 = 0;
			pkgDir.directorySize = pkgDirectorySize;
			pkgDir.numParts = gNumOfPartEntries;

#if defined(hasByteSwapping)
			pkgDir.id = BYTE_SWAP_LONG(pkgDir.id);
			pkgDir.flags = BYTE_SWAP_LONG(pkgDir.flags);
			pkgDir.version = BYTE_SWAP_LONG(pkgDir.version);
			pkgDir.copyright.offset = BYTE_SWAP_SHORT(pkgDir.copyright.offset);
			pkgDir.copyright.length = BYTE_SWAP_SHORT(pkgDir.copyright.length);
			pkgDir.name.offset = BYTE_SWAP_SHORT(pkgDir.name.offset);
			pkgDir.name.length = BYTE_SWAP_SHORT(pkgDir.name.length);
			pkgDir.size = BYTE_SWAP_LONG(pkgDir.size);
			pkgDir.creationDate = BYTE_SWAP_LONG(pkgDir.creationDate);
			pkgDir.modifyDate = BYTE_SWAP_LONG(pkgDir.modifyDate);
			pkgDir.directorySize = BYTE_SWAP_LONG(pkgDir.directorySize);
			pkgDir.numParts = BYTE_SWAP_LONG(pkgDir.numParts);
#endif

			numWritten = fwrite(&pkgDir, sizeof(PackageDirectory), 1, fp);
			if (numWritten != 1) err = EIO;
			XFAIL(ReportOSErr(err, "can’t write package header to output file “s”", gOutputFilename))
			numWritten = fwrite(partEntries, sizeOfPartEntries, 1, fp);
			if (numWritten != 1) err = EIO;
			XFAIL(ReportOSErr(err, "can’t write package directory entries to output file “s”", gOutputFilename))

			Progress("writing string table, size = %lu bytes, padded to %lu bytes",  stringDataSize, alignedStringDataSize);
			err = gStrings.write(fp);
			XFAIL(ReportOSErr(err, "can’t write %lu bytes of string table to output file “s”", stringDataSize, gOutputFilename))

			XFAIL(err = PadFileToLongAlignment(fp, stringDataSize, gOutputFilename))

			if (relocations.count() > 0) {
				Progress("writing relocations = %lu bytes, %lu bytes used", sizeOfRelocations, relocations.sizeUsed());
				err = relocations.write(fp, gOutputFilename);
				XFAIL(ReportOSErr(err, "can’t write %lu bytes of relocations to output file “s”", sizeOfRelocations, gOutputFilename))
			}

			ULong logicalAddress = gBaseAddress + pkgDirectorySize + sizeOfRelocations;
			partEntry = partEntries;
			partFile = partFiles;
			for (ArrayIndex i = 0; partFile < partFiles + gNumOfPartEntries; ++partEntry, ++partFile, ++i) {
				bool isPlainFrames = (FLAGTEST(partEntry->flags, kNOSPart) && !FLAGTEST(partEntry->flags, kCompressedFlag));
				err = copyFileTo(fp, partFile, partEntry->size, i, partEntry->offset + logicalAddress, isPlainFrames, &relocations);	// static on CRelocationCollector?
				XFAIL(ReportOSErr(err, "can’t copy part file “%s”", partFile->fName))
				XFAIL(err = PadFileToLongAlignment(fp, partEntry->size, gOutputFilename))
				if (partFile->fRelocations != NULL) {
					free(partFile->fRelocations), partFile->fRelocations = NULL;
				}
			}
			XFAIL(err)
			isDone = true;
		} XENDTRY;

		// close output file
		int closErr = fclose(fp);
		ReportOSErr(closErr, "can’t close output file");
		if (err == 0) {
			err = closErr;
		}
	} XENDTRY;

	// on error clean up by deleting output file
	if (err) {
		Progress("deleting output file “%s”", gOutputFilename);
		err = remove(gOutputFilename);
		ReportOSErr(err, "can’t delete output file “%s”", gOutputFilename);
	}

	free(partFiles);
	free(partEntries);

	if (!isDone || err != 0) {
		Progress("terminated by error");	//glob139
		return 1;
	}
	Progress("finished without error");	//glob138
	return 0;
}

