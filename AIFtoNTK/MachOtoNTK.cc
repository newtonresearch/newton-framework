/*
	File:		MachOtoNTK.cc

	Contains:	Command line tool to generate an NTK file containing
					native C function info from a Mach-O linked file.

	Written by:	The Newton Tools group.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
//#include <CarbonCore/Files.h>
#include <Carbon/Carbon.h>

#define kMaxExportFileCount 2
#define KByte  1024

#define BYTE_SWAP_LONG(n) (((n << 24) & 0xFF000000) | ((n <<  8) & 0x00FF0000) | ((n >>  8) & 0x0000FF00) | ((n >> 24) & 0x000000FF))


/*------------------------------------------------------------------------------
	R e f   T a g   B i t s
------------------------------------------------------------------------------*/

#define kRefTagBits		  2
#define kRefValueBits	 30
#define kRefValueMask	(-1 << kRefTagBits)
#define kRefTagMask		 ~kRefValueMask

#define kRefImmedBits	 2
#define kRefImmedMask	(-1 << kRefImmedBits)

enum
{
	kTagInteger,
	kTagPointer,
	kTagImmed,
	kTagMagicPtr
};

enum
{
	kImmedSpecial,
	kImmedChar,
	kImmedBoolean,
	kImmedReserved
};


/*------------------------------------------------------------------------------
	O b j e c t   T a g   F u n c t i o n s
------------------------------------------------------------------------------*/

#define	MAKEINT(i)				(((long) (i)) << kRefTagBits)
#define	MAKEIMMED(t, v)		((((((long) (v)) << kRefImmedBits) | ((long) (t))) << kRefTagBits) | kTagImmed)
#define	MAKECHAR(c)				MAKEIMMED(kImmedChar, (unsigned) c)
#define	MAKEBOOLEAN(b)			(b ? TRUEREF : FALSEREF)
#define	MAKEPTR(p)				((Ref)((char*)p + 1))
#define	MAKEMAGICPTR(index)	((Ref) (((long) (index)) << kRefTagBits) | kTagMagicPtr)

// constant values for comparison with a Ref
#define	NILREF		MAKEIMMED(kImmedSpecial, 0)
#define	TRUEREF		MAKEIMMED(kImmedBoolean, 1)
#define	FALSEREF		NILREF


/*------------------------------------------------------------------------------
	T y p e s
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	M a c h - O   S t r u c t u r e s
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	S y m b o l   T a b l e
------------------------------------------------------------------------------*/

class CHashTableEntry
{
public:
	CHashTableEntry * fLink;
};


class CHashTable
{
public:
								CHashTable(int inSize);
	//							~CHashTable();

	virtual unsigned long	hash(CHashTableEntry * inEntry);
	virtual int				compare(CHashTableEntry * inEntry1, CHashTableEntry * inEntry2);
	void						add(CHashTableEntry * inEntry);
	void						remove(CHashTableEntry * inEntry);
	virtual CHashTableEntry *  lookup(CHashTableEntry * inEntry);
	CHashTableEntry *		search(CHashTableEntry * inEntry);

	long						fSize;	// MUST be a power of 2
	CHashTableEntry **	fTable;  // pointers to entries
};


class CSymbol : public CHashTableEntry
{
public:
					CSymbol() {}
					CSymbol(const char * inName, CSymbol * inLink);
	//				~CSymbol();

	CSymbol *	fNext;
	CSymbol *	fPrev;
	long			fOffset;
	short			fNumArgs;
	char *		fName;
};


class CSymbolTable : public CHashTable
{
public:
								CSymbolTable(int inSize)	:  CHashTable(inSize)  { }

	virtual unsigned long	hash(CHashTableEntry * inEntry);
	virtual int				compare(CHashTableEntry * inEntry1, CHashTableEntry * inEntry2);
	CSymbol *				find(char * inName);
};


/*------------------------------------------------------------------------------
	N e w t o n   S t r e a m e d   O b j e c t   F o r m a t
------------------------------------------------------------------------------*/

struct Precedent
{
	long				id;
	size_t			size;
	const char *	name;
};

#define kPrecedentTableSize 500


typedef enum
{
	kFDImmediate,				//	Indeterminate Immediate type
	kFDCharacter,				//	ASCII Character
	kFDUnicodeCharacter,		//	Unicode Character
	kFDBinaryObject,			//	Small Binary Object ( < 32K )
	kFDArray,					//	Named Array
  	kFDPlainArray,				//	Anonymous Array
  	kFDFrame,					//	Frame
  	kFDSymbol,					//	Object Symbol
  	kFDString,					//	String (null-terminated Unicode)
  	kFDPrecedent,				//	Repeated Item
  	kFDNIL,						//	nil object
  	kFDSmallRect,				//	Small rect
	kFDLargeBinary,			//	Large Binary Object
  	kFDBoolean,					//	Boolean
  	kFDInteger					//	Integer (4 byte)
} FDObjectType;

#define kNSOFVersion 2


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

const char *	gCmdName;
bool				gDoProgress = false;
bool				gDoDump = false;
bool				gDoHelp = false;
bool				gDoslOption = false;
bool				gDosOption = false;
bool				gHasViaOption = false;
const char *	gExportFilename[kMaxExportFileCount];
long				gExportFileCount = 0;
const char *	gOutputFilename = NULL;
const char *	gImageFilename = NULL;
FILE *			gImageFile;
mach_header		gImageHeader;
bool				gIsExecutableAIF;

unsigned long *	gRelocs;
size_t			gNumOfRelocs = 0;

long				gPrecedentId = 0;
size_t			gNumOfSymbolPrecedents = 0;
Precedent		gSymbolPrecedents[kPrecedentTableSize];

#pragma mark -

/*------------------------------------------------------------------------------
	C H a s h T a b l e
------------------------------------------------------------------------------*/

#pragma mark -

CHashTable::CHashTable(int inSize)
{
	fSize = inSize;
	fTable = (CHashTableEntry **) malloc(inSize * sizeof(CHashTableEntry *));
//	ASSERT(fTable != NULL);
}

// no destructor!

unsigned long
CHashTable::hash(CHashTableEntry * inEntry)
{
	return ((unsigned long)inEntry >> 8) & (fSize - 1);
}

int
CHashTable::compare(CHashTableEntry * inEntry1, CHashTableEntry * inEntry2)
{
	return 1;
}

void
CHashTable::add(CHashTableEntry * inEntry)
{
	int	index = hash(inEntry);
	inEntry->fLink = fTable[index];
	fTable[index] = inEntry;
}

void
CHashTable::remove(CHashTableEntry * inEntry)
{
	int	index = hash(inEntry);
	CHashTableEntry * entry = fTable[index];
//	ASSERT(entry != NULL);
	if (entry == inEntry)
		fTable[index] = inEntry->fLink;
	else
	{
		CHashTableEntry * prev = fTable[index];
		for (entry = prev->fLink; entry != NULL; prev = entry, entry = entry->fLink)
			if (entry == inEntry)
				break;
//		ASSERT(entry != NULL);
		prev->fLink = inEntry->fLink;
	}
}

CHashTableEntry *
CHashTable::lookup(CHashTableEntry * inEntry)
{
	int	index = hash(inEntry);
	for (CHashTableEntry * entry = fTable[index]; entry != NULL; entry = entry->fLink)
		if (compare(entry, inEntry) == 0)		// vt04 !!
			return entry;
	return NULL;
}

CHashTableEntry *
CHashTable::search(CHashTableEntry * inEntry)
{
	for (int index = 0; index < fSize; index++)
	{
		for (CHashTableEntry * entry = fTable[index]; entry != NULL; entry = entry->fLink)
			if (compare(entry, inEntry) == 0)		// vt08 !!
				return entry;
	}
	return NULL;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S y m b o l T a b l e
------------------------------------------------------------------------------*/

unsigned long
CSymbolTable::hash(CHashTableEntry * inEntry)
{
	unsigned long  result = 0;
	const char *	symbol = static_cast<CSymbol*>(inEntry)->fName;
	for (int i = 8; *symbol != 0 && i != 0; symbol++, i--)
		result = result * 37 + tolower(*symbol);

	return result & (fSize - 1);
}

int
CSymbolTable::compare(CHashTableEntry * inEntry1, CHashTableEntry * inEntry2)
{
	return strcmp(static_cast<CSymbol*>(inEntry1)->fName,
					  static_cast<CSymbol*>(inEntry2)->fName);
}

CSymbol *
CSymbolTable::find(char * inName)
{
	CSymbol	entry;
	entry.fLink = nil;
	entry.fName = inName;
	return static_cast<CSymbol*>(lookup(&entry));	// vt14
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S y m b o l
------------------------------------------------------------------------------*/

CSymbol::CSymbol(const char * inName, CSymbol * inLink)
{
	fLink = nil;
	fNext = nil;
	fPrev = inLink;
	if (inLink != nil)
		inLink->fNext = this;
	fName = (char *) malloc(strlen(inName) + 1);
	strcpy(fName, inName);
	fOffset = -1;
	fNumArgs = -1;
}

// no destructor!

#pragma mark -

/*------------------------------------------------------------------------------
	Reporting.
------------------------------------------------------------------------------*/

void
Progress(const char * inMessage, ...)
{
	if (gDoProgress)
	{
		va_list  ap;
		va_start(ap, inMessage);
		fputs("# ", stderr);
		vfprintf(stderr, inMessage, ap);
		fputs(".\n", stderr);
		va_end(ap);
	}
}

void
vgoof(const char * inLevel, const char * inMessage, va_list ap)
{
	fprintf(stderr,  "### %s - ", gCmdName);
	fprintf(stderr,  "%s - ", inLevel);
	vfprintf(stderr, inMessage, ap);
	fputs(".\n", stderr);
}

void
WarningMessage(const char * inMessage, ...)
{
	va_list  ap;
	va_start(ap, inMessage);
	vgoof("warning", inMessage, ap);
	va_end(ap);
}

void
ErrorMessage(const char * inMessage, ...)
{
	va_list  ap;
	va_start(ap, inMessage);
	vgoof("error", inMessage, ap);
	va_end(ap);
}

void
UsageExitWithMessage(const char * inMessage, ...)
{
	va_list  ap;
	va_start(ap, inMessage);
	fputs("# ", stderr);
	vfprintf(stderr, inMessage, ap);
	fputs(".\n", stderr);
	va_end(ap);
	exit(1);
}

void
ExitWithMessage(const char * inMessage, ...)
{
	va_list  ap;
	va_start(ap, inMessage);
	fputs("# ", stderr);
	vfprintf(stderr, inMessage, ap);
	fputs(".\n", stderr);
	va_end(ap);
	exit(2);
}

#pragma mark -

/*------------------------------------------------------------------------------
	S t r e a m i n g   F u n c t i o n s
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Stream an object type; the object itself will follow.
	If a non-immediate object, bump the precedent count.
	Args:		inFile		the file to write
				inObj			the object
	Return:  --
------------------------------------------------------------------------------*/

void
StreamObject(FILE * inFile, FDObjectType inObj)
{
	if ((inObj > kFDUnicodeCharacter && inObj < kFDPrecedent)
	||   inObj > kFDNIL)
		gPrecedentId++;
	fputc(inObj, inFile);
}


/*------------------------------------------------------------------------------
	Stream a long value.
	Longs are compressed so that values < 254 occupy only one byte.
	Args:		inFile		the file to write
				inVal			the value
	Return:  --
------------------------------------------------------------------------------*/

void
StreamLong(FILE * inFile, long inVal)
{
	if (inVal >= 0 && inVal <= 0xFE)
		fputc(inVal, inFile);
	else
	{
#if defined(__LITTLE_ENDIAN__)
		inVal = BYTE_SWAP_LONG(inVal);
#endif
		fputc(0xFF, inFile);
		fwrite(&inVal, sizeof(long), 1, inFile);
	}
}


/*------------------------------------------------------------------------------
	Stream an immediate value.
	Convert C long to NS ref.
	Args:		inFile		the file to write
				inVal			the value
	Return:  --
------------------------------------------------------------------------------*/

void
StreamImmediate(FILE * inFile, long inVal)
{
	fputc(kFDImmediate, inFile);
	StreamLong(inFile, MAKEINT(inVal));
}


/*------------------------------------------------------------------------------
	Stream a symbol.
	Check the precedents table to see if the name can be reused;
	add this symbol to the table if not.
	Args:		inFile		the file to write
				inSym			the symbol name
	Return:  --
------------------------------------------------------------------------------*/

void
StreamSymbol(FILE * inFile, const char * inSym)
{
	Precedent * precedent;
	size_t  i, symLen = strlen(inSym);
	if (symLen > 0xFE)
		ExitWithMessage("function name exceeds max length for a Newton symbol");

	for (i = 0; i < symLen; i++)
	{
		char  ch = inSym[i];
		if (ch == ' ' || ch == '|' || ch == '\\')
			ExitWithMessage("invalid character (0x%02X) found in function name “%s”", ch, inSym);
	}

	for (i = 0, precedent = gSymbolPrecedents; i < gNumOfSymbolPrecedents; i++, precedent++)
	{
		if (precedent->size == symLen
		&&  memcmp(inSym, precedent->name, symLen) == 0)
		{
			fputc(kFDPrecedent, inFile);
			StreamLong(inFile, precedent->id);
			return;
		}
	}

	fputc(kFDSymbol, inFile);
	StreamLong(inFile, symLen);
	fwrite(inSym, sizeof(char), symLen, inFile);

	if (gNumOfSymbolPrecedents < kPrecedentTableSize)
	{
		precedent = gSymbolPrecedents + gNumOfSymbolPrecedents++;
		precedent->id = gPrecedentId++;
		precedent->size = symLen;
		precedent->name = inSym;
	}
}


/*------------------------------------------------------------------------------
	Stream a binary object.
	Args:		inFile		the file to write
				inClass		the object’s class
				inData		the object’s data
				inSize		the object’s size
	Return:  --
------------------------------------------------------------------------------*/

void
StreamBinary(FILE * inFile, const char * inClass, unsigned char * inData, size_t inSize)
{
	StreamObject(inFile, kFDBinaryObject);
	StreamLong(inFile, inSize);
	StreamSymbol(inFile, inClass);
	fwrite(inData, inSize, 1, inFile);
}


/*------------------------------------------------------------------------------
	Stream an array of function definition frames.
	Args:		inFile		the file to write
				inFunctions	the function set - a linked list of function definitions
	Return:  --
------------------------------------------------------------------------------*/

void
StreamFunctionSetArray(FILE * inFile, CSymbol * inFunctions)
{
	CSymbol * func;
	long  numOfFuncs = 0;
	for (func = inFunctions->fNext; func != nil; func = func->fNext)
		numOfFuncs++;

	StreamObject(inFile, kFDPlainArray);
	StreamLong(inFile, numOfFuncs);
	for (func = inFunctions->fNext; func != nil; func = func->fNext)
	{
		StreamObject(inFile, kFDFrame);
		StreamLong(inFile, 3);
		StreamSymbol(inFile, "name");
		StreamSymbol(inFile, "offset");
		StreamSymbol(inFile, "numArgs");

		StreamSymbol(inFile, func->fName);
		StreamImmediate(inFile, func->fOffset);
		StreamImmediate(inFile, func->fNumArgs);
	}
}


/*------------------------------------------------------------------------------
	Emit an NSOF file defining the Plain C Code Module (CCM).
	Args:		inFunctions	linked list of function names to emit
	Return:  --
------------------------------------------------------------------------------*/

void
EmitCCMFrameFile(CSymbol * inFunctions)
{
	FILE *	f = fopen(gOutputFilename, "w");
	if (f == NULL)
		ExitWithMessage("can’t open output file “%s” for write", gOutputFilename);

	fputc(kNSOFVersion, f);

	StreamObject(f, kFDFrame);
	StreamLong(f, 6);
	StreamSymbol(f, "class");
	StreamSymbol(f, "name");
	StreamSymbol(f, "version");
	StreamSymbol(f, "CPUType");
	StreamSymbol(f, "debugFile");
	StreamSymbol(f, "entryPoints");

	StreamSymbol(f, "PlainCModule");				// class
	StreamSymbol(f, inFunctions->fName);		// name
	StreamImmediate(f, 1);							// version
#if defined(__i386__)
	StreamSymbol(f, "x86");							// CPUType
#else
	StreamSymbol(f, "PPC");
#endif
	StreamSymbol(f, gImageFilename);				// debugFile
	StreamFunctionSetArray(f, inFunctions);	// entryPoints

	fclose(f);

// set file type/creator STRM/NTP1
	FSSpec	fileSpec;
	FInfo		finderInfo;
	OSErr		err;

	err = FSMakeFSSpec(0, 0, (const unsigned char *) gOutputFilename, &fileSpec);
	if (err == noErr
	&& (err = FSpGetFInfo(&fileSpec, &finderInfo)) == noErr)
	{
		finderInfo.fdType = 'STRM';
		finderInfo.fdCreator = 'NTP1';
		FSpSetFInfo(&fileSpec, &finderInfo);
	}
}

#pragma mark -

/*------------------------------------------------------------------------------
	Fill the symbol table.
	Args:		
	Return:  true if error
------------------------------------------------------------------------------*/

bool
FillSymbolTable(CSymbolTable * inTable, CSymbol ** outFunctionNames)
{
	CSymbol *	symbol = nil;
	size_t		numOfSymbols = 0;
	bool		isFirst = true;
	FILE *	expFile;
	long		lineLen;
	long		argCount;
	char		line[300];
	char		name[256];
	long		expFileIndex;

	for (expFileIndex = 0; expFileIndex < gExportFileCount; expFileIndex++)
	{
		const char *	expFilename = gExportFilename[expFileIndex];
		expFile = fopen(expFilename, "r");
		if (expFile == NULL)
			ExitWithMessage("can’t open -via file “%s”", expFilename);

		while (!feof(expFile))
		{
			argCount = -1;
			lineLen = 0;
			line[0] = 0;
			if (fgets(line, 300, expFile) != NULL)
			{
				if (line[0] != '\n')
				{
					int	numOfItems = sscanf(line, "%255s%ln%li", name, &lineLen, &argCount);
					if (lineLen == 255)
						ExitWithMessage("input line too long in file “%s”", expFilename);
					if (numOfItems > 0 && line[0] != ';')
					{
						if (strlen(name) > 254)
							ExitWithMessage("encountered function name longer than 254 characters in file “%s”", expFilename);
						symbol = new CSymbol(name, symbol);
						inTable->add(symbol);
						if (isFirst)
						{
							*outFunctionNames = symbol;
							isFirst = false;
						}
						else
							numOfSymbols++;
						if (numOfItems == 2 && argCount >= 0)
							symbol->fNumArgs = argCount;
					}
				}
			}
		}
		if (isFirst)
			WarningMessage("no names found in the function names file “%s”", expFilename);
		fclose(expFile);
	}

	return numOfSymbols == 0 && isFirst;
}


/*------------------------------------------------------------------------------
	Check that all functions have their offsets and arg counts defined.
	Args:		inFunctions
	Return:  true if there are undefined offsets or arg counts
------------------------------------------------------------------------------*/

bool
AnyUndefinedOffsetsOrArgCounts(CSymbol * inFunctions)
{
	bool  isUndefined = false;

	for (CSymbol * func = inFunctions->fNext; func != nil; func = func->fNext)
	{
		if (func->fOffset >= 0 && func->fNumArgs >= 0)
			Progress("symbol “%s”, offset = %ld, argCnt = %i", func->fName, func->fOffset, func->fNumArgs);
		if (func->fOffset == -1)
		{
			ErrorMessage("could not find function entry point for the function named “%s”.\n"
							 "#   Did you forget to use “extern \"C\" %s”?", func->fName, func->fName);
			isUndefined = true;
		}
		if (func->fNumArgs < 0)
		{
			ErrorMessage("could not find argument count for the function named “%s”.\n"
							 "#   Did you forget to use “extern \"C\" %s”?", func->fName, func->fName);
			isUndefined = true;
		}
	}

	return isUndefined;
}


/*------------------------------------------------------------------------------
	Read symbolic info (function arg counts and offsets) from the image file.
	Args:		inTable
				inFunctions
	Return:  --
------------------------------------------------------------------------------*/

void
FindSymbolicInfo(CSymbolTable * inTable, CSymbol * inFunctions)
{
	struct nlist * symList, * sym;
	CSymbol *		func;
	int				numOfFuncs = 0;

	for (func = inFunctions->fNext; func != nil; func = func->fNext)
		numOfFuncs++;

	symList = (struct nlist *) calloc(numOfFuncs+1, sizeof(struct nlist));
	if (symList == NULL)
		ExitWithMessage("can’t allocate %ld bytes for the symbol list", (numOfFuncs+1) * sizeof(struct nlist));

	sym = symList;
	for (func = inFunctions->fNext; func != nil; sym++, func = func->fNext)
		sym->n_un.n_name = func->fName;

	Progress("searching for symbolic function definitions within file “%s”", gImageFilename);
	if (nlist(gImageFilename, symList) != numOfFuncs)
			ExitWithMessage("couldn’t locate function definitions for all functions");

	sym = symList;
	for (func = inFunctions->fNext; func != nil; sym++, func = func->fNext)
		func->fOffset = sym->n_value;

	if (AnyUndefinedOffsetsOrArgCounts(inFunctions))
		ExitWithMessage("couldn’t define all functions");
}


/*------------------------------------------------------------------------------
	Emit an NTK file defining the function set.
	Args:		inCodeSize
				inDebugSize
				inHeaderSize
				inROSize
	Return:  --
------------------------------------------------------------------------------*/

void
EmitNTKInterface(void)
{
	CSymbol *		functionNames = nil;
	CSymbolTable * table = new CSymbolTable(4*KByte);
	if (FillSymbolTable(table, &functionNames))
		ExitWithMessage("unable to build a valid function name table");
	FindSymbolicInfo(table, functionNames);
	EmitCCMFrameFile(functionNames);
}

#pragma mark -

/*------------------------------------------------------------------------------
	Debug info reporting.
------------------------------------------------------------------------------*/

void
DumpDebugInfo(const char * inFilename, FILE * inFile,
				  size_t inCodeSectionSize, size_t inDebugSectionSize,
				  bool inHasLowLevelInfo, bool inHasSourceLevelInfo)
{
// not implemented
}

#pragma mark -

/*------------------------------------------------------------------------------
	Command line options.
------------------------------------------------------------------------------*/

void
AssureNextArg(const char ** inArg, const char ** inArgLimit, const char * inMessage)
{
	if (inArg + 1 >= inArgLimit)
		UsageExitWithMessage("no %s follows “%s”", inMessage, *inArg);
}


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


const char **
ParseOptions(const char ** inArg, const char ** inArgLimit)
{
	const char **	inputFilenameLimit = inArg;
	const char *	option;

	for ( ; inArg < inArgLimit; inArg++)
	{
		option = *inArg;
		if (hcistrcmp(option, "-help") == 0)
			gDoHelp = true;
		else if (hcistrcmp(option, "-p") == 0)
			gDoProgress = true;
		else if (hcistrcmp(option, "-dump") == 0)
			gDoDump = true;
		else if (hcistrcmp(option, "-s") == 0)
			gDosOption = true;
		else if (hcistrcmp(option, "-sl") == 0)
			gDoslOption = true;
		else if (hcistrcmp(option, "-via") == 0)
		{
			AssureNextArg(inArg, inArgLimit, "function name file");
			if (gExportFileCount >= kMaxExportFileCount)
				ExitWithMessage("only %d function name files allowed", kMaxExportFileCount);
			gHasViaOption = true;
			gExportFilename[gExportFileCount++] = *++inArg;
		}
		else if (hcistrcmp(option, "-o") == 0)
		{
			AssureNextArg(inArg, inArgLimit, "NTK output filename");
			gOutputFilename = *++inArg;
		}
		else if (*option == '-')
			UsageExitWithMessage("%s is not an option", option);
		else
			*inputFilenameLimit++ = option;
	}

	return inputFilenameLimit;
}

#pragma mark -

/*------------------------------------------------------------------------------
	main
------------------------------------------------------------------------------*/

int
main(int argc, const char * argv[])
{
// install signal handlers: kill, interrupt
	gCmdName = argv[0];
	time_t			now;
	char				timeStamp[64];
	const char **  inputFilenameLimit = ParseOptions(&argv[1], &argv[argc]);

	if (gDoHelp)
		UsageExitWithMessage("");

	if (gDoDump == false)
	{
		if (gOutputFilename == NULL)
			UsageExitWithMessage("“-o outputname” option missing");
		if (gHasViaOption == false)
			UsageExitWithMessage("“-via filename” options missing");
	}

	if (inputFilenameLimit <= &argv[1])
		UsageExitWithMessage("input filename missing");

	gImageFilename = argv[1];
	gImageFile = fopen(gImageFilename, "r");
	if (gImageFile == NULL)
		ExitWithMessage("can’t open file “%s”", gImageFilename);
	if (fread(&gImageHeader, sizeof(mach_header), 1, gImageFile) != 1)
		ExitWithMessage("can’t read %ld bytes for mach_header from file “%s”", sizeof(mach_header), gImageFilename);
	if (gImageHeader.magic != MH_MAGIC)
		ExitWithMessage("file “%s” does not seem to be in Mach-O format", gImageFilename);
	fclose(gImageFile);

	now = time(NULL);
	strftime(timeStamp, sizeof(timeStamp), "on %D at %R", localtime(&now));
	Progress("NTK C functions file converted from “%s” by MachOtoNTK %s", gImageFilename, timeStamp);

	EmitNTKInterface();

	return 0;
}
