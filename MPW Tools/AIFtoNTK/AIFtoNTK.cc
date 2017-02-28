/*
	File:		AIFtoNTK.cc

	Contains:	Command line tool to generate an NTK file containing
					native C function info from an AIF linked file.

	Written by:	The Newton Tools group.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <Carbon/Carbon.h>

#define KByte  1024


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
	A I F   S t r u c t u r e s
------------------------------------------------------------------------------*/

struct AIFHeader
{
	unsigned long  decompressCode;
	unsigned long  selfRelocCode;
	unsigned long  zeroInitCode;
	unsigned long  imageEntryCode;
	unsigned long  exitCode;
	size_t			roSize;
	size_t			rwSize;
	size_t			debugSize;
	size_t			zeroInitSize;
	unsigned long  debugType;
	unsigned long  imageBase;
	size_t			workSpace;
	unsigned long  addressMode;
	unsigned long  dataBase;
	unsigned long  reserved1;
	unsigned long  reserved2;
	unsigned long  zeroInit[16];
};

struct ItemSection
{
	unsigned short		size;
	unsigned short		code;
	unsigned char		language;
	unsigned char		level;
	unsigned char		reserved1;
	unsigned char		version;
	unsigned long		codeStart;
	unsigned long		dataStart;
	size_t				codeSize;
	size_t				dataSize;
	unsigned long		fileInfo;
	size_t				debugSize;
	union
	{
		unsigned long		numOfSyms;
		struct
		{
			unsigned char		nameLength;
			char					name[3]; // actually variable length pascal string
		};
	};
};

struct ItemProcedure
{
	unsigned short		size;
	unsigned short		code;
	unsigned long		type;
	unsigned long		numOfArgs;
	unsigned long		sourcePos;
	unsigned long		startAddr;
	unsigned long		entryAddr;
	unsigned long		endProc;
	unsigned long		fileEntry;
	unsigned char		nameLength;
	char					name[3]; // actually variable length pascal string
};

// code
enum
{
	kItemSection = 1,
	kItemBeginProc,
	kItemEndProc,
	kItemVariable,
	kItemType,
	kItemStruct,
	kItemArray,
	kItemSubrange,
	kItemSet,
	kItemFileInfo,
	kItemContigEnum,
	kItemDiscontigEnum,
	kItemProcDecl,
	kItemBeginScope,
	kItemEndScope,
	kItemBitfield
};

// language
enum
{
	kLanguageNone,
	kLanguageC,
	kLanguagePascal,
	kLanguageFortran,
	kLanguageAssembler
};

// debug level
enum
{
	kDebugLineNumbers,
	kDebugVariableNames
};


struct AIFSymbol
{
	unsigned long  flags  :  8;
	unsigned long  offset : 24;
	long				value;
};


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
const char *	gExportFilename[2];
long				gExportFileCount = 0;
const char *	gOutputFilename = NULL;
const char *	gImageFilename = NULL;
FILE *			gImageFile;
AIFHeader		gImageHeader;
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
	Stream the relocation table.
	Ensure it’s in ascending address order.
	Args:		inFile		the file to write
	Return:  --
------------------------------------------------------------------------------*/

int
SortRelocs(unsigned long * a, unsigned long * b)
{
	return *a - *b;
}

void
StreamRelocs(FILE * inFile)
{
	long  relocsSize = gNumOfRelocs * sizeof(long);

	StreamObject(inFile, kFDBinaryObject);
	StreamLong(inFile, relocsSize + sizeof(long));
	StreamSymbol(inFile, "relocs");
	fwrite(&gNumOfRelocs, sizeof(long), 1, inFile);
	if (gNumOfRelocs > 0)
	{
		if (gNumOfRelocs == 2)
		{
			if (gRelocs[0] > gRelocs[1])
			{
				long  tmpReloc = gRelocs[0];
				gRelocs[0] = gRelocs[1];
				gRelocs[1] = tmpReloc;
			}
		}
		else if (gNumOfRelocs > 2)
			qsort(gRelocs, gNumOfRelocs, sizeof(long), SortRelocs);
		fwrite(gRelocs, relocsSize, 1, inFile);
	}
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
	Emit an NSOF file defining the the Native Code Module (NCM).
	Args:		inFunctions	linked list of function names to emit
	Return:  --
------------------------------------------------------------------------------*/

void
EmitNCMFrameFile(CSymbol * inFunctions)
{
	unsigned char *	image;
	FILE *	f = fopen(gOutputFilename, "w");
	if (f == NULL)
		ExitWithMessage("can’t open output file “%s” for write", gOutputFilename);

	fputc(kNSOFVersion, f);

	StreamObject(f, kFDFrame);
	StreamLong(f, 8);
	StreamSymbol(f, "class");
	StreamSymbol(f, "name");
	StreamSymbol(f, "version");
	StreamSymbol(f, "CPUType");
	StreamSymbol(f, "code");
	StreamSymbol(f, "relocations");
	StreamSymbol(f, "debugFile");
	StreamSymbol(f, "entryPoints");

	StreamSymbol(f, "NativeModule");				// class
	StreamSymbol(f, inFunctions->fName);		// name
	StreamImmediate(f, 1);							// version
	StreamSymbol(f, "ARM610");						// CPUType

	if (fseek(gImageFile, sizeof(AIFHeader), SEEK_SET) != 0)
		ExitWithMessage("can’t seek to read-only data in file “%s”", gImageFilename);
	if ((image = (unsigned char *) malloc(gImageHeader.roSize + 8)) == NULL)
		ExitWithMessage("can’t allocate %ld bytes for the RO image data", gImageHeader.roSize);
	if (fread(image, gImageHeader.roSize, 1, gImageFile) != 1)
		ExitWithMessage("can’t read %ld bytes of RO image data from file “%s”", gImageHeader.roSize, gImageFilename);

	StreamBinary(f, "code", image, gImageHeader.roSize);		// code
	free(image);

	StreamRelocs(f);									// relocations
	StreamSymbol(f, gImageFilename);				// debugFile
	StreamFunctionSetArray(f, inFunctions);	// entryPoints

	fclose(f);

// set file type/creator ntkc/NTP1
	FSSpec	fileSpec;
	FInfo		finderInfo;
	OSErr		err;

	err = FSMakeFSSpec(0, 0, gOutputFilename, &fileSpec);
	if (err == noErr
	&& (err = FSpGetFInfo(&fileSpec, &finderInfo)) == noErr)
	{
		finderInfo.fdType = 'ntkc';
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
	CSymbol *	symbol = nil;	// A4
	size_t		numOfSymbols = 0;
	bool		isFirst = true;  // 0244
	FILE *	expFile;			// 0240
	long		lineLen;			// 023C
	long		a0238;			// 0238
	char		line[300];		// 0234
	char		name[256];		// 0108
	long		expFileIndex;  // D5

	for (expFileIndex = 0; expFileIndex < gExportFileCount; expFileIndex++)
	{
		const char *	expFilename = gExportFilename[expFileIndex];
		expFile = fopen(expFilename, "r");
		if (expFile == NULL)
			ExitWithMessage("can’t open -via file “%s”", expFilename);

		while (!feof(expFile))
		{
			a0238 = 0;
			lineLen = 0;
			line[0] = 0;
			if (fgets(line, 300, expFile) != NULL)
			{
				if (line[0] != '\n')
				{
					int	numOfItems = sscanf(line, "%255s%ln%li", name, &lineLen, &a0238);
					if (lineLen == 255)
						ExitWithMessage("input line too long in file “%s”", expFilename);
					if (numOfItems > 0 && line[0] != ';')
					{
						if (strlen(name) > 254)
							ExitWithMessage("encountered function name longer than 254 characters in file “%s”", expFilename);
						symbol = new CSymbol(name, symbol);
						inTable->add(symbol); // vt0C
						if (isFirst)
						{
							*outFunctionNames = symbol;
							isFirst = false;
						}
						else
							numOfSymbols++;
						if (numOfItems == 2 && a0238 > 0)
							symbol->fNumArgs = a0238;
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
	Check that all functions have their numArgs defined.
	Args:		inFunctions
				inDoProgress
	Return:  true if there are undefined arg counts
------------------------------------------------------------------------------*/

bool
ArgCountsNotDefined(CSymbol * inFunctions, bool inDoProgress)
{
	bool  isUndefined = false;

	for (CSymbol * func = inFunctions->fNext; func != nil; func = func->fNext)
	{
		if (inDoProgress && func->fNumArgs >= 0)
			Progress("symbol “%s”, argCnt = %i", func->fName, func->fNumArgs);
		if (func->fNumArgs < 0)
		{
			if (inDoProgress)
				ErrorMessage("could not find source symbolics for the function named “%s”.\n"
								 "#   Did you forget to use “extern \"C\" %s”?", func->fName, func->fName);
			isUndefined = true;
		}
	}

	return isUndefined;
}


/*------------------------------------------------------------------------------
	Check that all functions have their offsets defined.
	Args:		inFunctions
	Return:  true if there are undefined offsets
------------------------------------------------------------------------------*/

bool
AnyUndefinedOffsets(CSymbol * inFunctions)
{
	bool  isUndefined = false;

	for (CSymbol * func = inFunctions->fNext; func != nil; func = func->fNext)
	{
		if (func->fOffset == -1)
		{
			ErrorMessage("could not find function entry point for the function named “%s”.\n"
							 "#   Did you forget to use “extern \"C\" %s”?", func->fName, func->fName);
			isUndefined = true;
		}
	}

	return isUndefined;
}


/*------------------------------------------------------------------------------
	Scan low-level symbol info (function offsets).
	Args:		inFilename		name of image file, for error reporting
				inFile			image file ref
				inItem
				inCodeSize
				ioDebugSize
				inTable
	Return:  --
------------------------------------------------------------------------------*/

void
FindProcOffsets(const char * inFilename, FILE * inFile, const ItemSection & inItem, long inCodeSize, size_t * ioDebugSize, CSymbolTable * inTable)
{
	size_t	dataSize;
	size_t	symbolTableSize;
	AIFSymbol *	symbolTable;
	size_t	stringTableSize;
	char *	stringTable;

	if (inItem.level != kDebugLineNumbers || inItem.fileInfo != 0)
		ExitWithMessage("unexpected low-level flags = 0x%02X, or fileInfo = 0x%08X", inItem.level, inItem.fileInfo);
	dataSize = inItem.debugSize - sizeof(ItemSection);
	if (dataSize != *ioDebugSize)
		ExitWithMessage("unexpected low-level debug section size = 0x%08X != 0x%08X", dataSize, *ioDebugSize);

	symbolTableSize = inItem.numOfSyms * sizeof(AIFSymbol);
	if (symbolTableSize > *ioDebugSize)
		ExitWithMessage("symbol table size = %ld too long (debug size = %ld) in file “%s”", symbolTableSize, *ioDebugSize, inFilename);
	symbolTable = (AIFSymbol *) malloc(symbolTableSize + 8);
	if (symbolTable == NULL)
		ExitWithMessage("can’t allocate %ld bytes for the symbol table", symbolTableSize);
	if (fread(symbolTable, sizeof(AIFSymbol), inItem.numOfSyms, inFile) != inItem.numOfSyms)
		ExitWithMessage("can’t read %ld bytes for symbol table from file “%s”", symbolTableSize, inFilename);
	*ioDebugSize -= symbolTableSize;

	if (fread(&stringTableSize, sizeof(size_t), 1, inFile) != 1)
		ExitWithMessage("can’t read %ld bytes for string table length from file “%s”", sizeof(size_t), inFilename);
	if (stringTableSize != *ioDebugSize)
		ExitWithMessage("unexpected string table size = %ld in file “%s”", stringTableSize, inFilename);

	stringTable = (char *) malloc(stringTableSize + 8);
	if (stringTable == NULL)
		ExitWithMessage("can’t allocate %ld bytes for the string table", stringTableSize);
	if (fread(stringTable + sizeof(long), stringTableSize - sizeof(long), 1, inFile) != 1)
		ExitWithMessage("can’t read %ld bytes for string table from file “%s”", stringTableSize, inFilename);
	*ioDebugSize -= stringTableSize;

	for (AIFSymbol * sym = symbolTable ; sym < (AIFSymbol *)((char*)symbolTable + symbolTableSize); sym++)
	{
//		main11B0(1);
		if (sym->flags & 0x01				// symbol is global
		&& (sym->flags & 0x06) == 0x02)	// symbol names code
		{
			char *		name = stringTable + sym->offset + 1;
			CSymbol *	symbol;
			if ((symbol = inTable->find(name)) != nil)
			{
				symbol->fOffset = sym->value;
				Progress("found symbol “%s”, offset = 0x%08X", name, sym->value);
			}
		}
	}

	free(stringTable);
	free(symbolTable);

	if (*ioDebugSize != 0)
		ExitWithMessage("%ld excess debug bytes following low-level debug info in file “%s”", *ioDebugSize, inFilename);
}


/*------------------------------------------------------------------------------
	Scan source-level symbol info (function arg counts).
	Args:		inFilename		name of image file, for error reporting
				inFile			image file ref
				inItem
				inCodeSize
				ioDebugSize
				inTable
	Return:  --
------------------------------------------------------------------------------*/

void
FindSourceProcs(const char * inFilename, FILE * inFile, const ItemSection & inItem, long inCodeSize, size_t * ioDebugSize, CSymbolTable * inTable)
{
	char *	symbolData, * symbolDataEnd;
	size_t	dataSize;

	if (inItem.level == kDebugLineNumbers || inItem.fileInfo == 0)
		ExitWithMessage("unexpected source-level flags = 0x%02X, or fileInfo = 0x%08X", inItem.level, inItem.fileInfo);
	dataSize = inItem.debugSize - sizeof(ItemSection);
	if (dataSize > *ioDebugSize)
		ExitWithMessage("unexpected source-level debug section size = 0x%08X > 0x%08X", dataSize, *ioDebugSize);
	dataSize = inItem.debugSize - inItem.size;
	symbolData = (char *) malloc(dataSize);
	if (symbolData == NULL)
		ExitWithMessage("can’t allocate %ld bytes for the symbol data", dataSize);
	if (fread(symbolData, dataSize, 1, inFile) != 1)
		ExitWithMessage("can’t read %ld bytes for symbol data from file “%s”", dataSize, inFilename);
	*ioDebugSize -= dataSize;

	symbolDataEnd = symbolData + dataSize;
	for (char * item = symbolData; item < symbolDataEnd; item += ((ItemProcedure *)item)->size)
	{
		if (((ItemProcedure *)item)->code == kItemBeginProc)
		{
			CSymbol *	symbol;
			if ((symbol = inTable->find(((ItemProcedure *)item)->name)) != nil)
			{
				if (((ItemProcedure *)item)->numOfArgs < 1)
					ErrorMessage("function “%s” is not defined as having at least one argument of type RefArg", ((ItemProcedure *)item)->name);
				symbol->fNumArgs = ((ItemProcedure *)item)->numOfArgs - 1;
			}
		}
//		else
//			main11B0(1);
	}
}


/*------------------------------------------------------------------------------
	Read symbolic info (function arg counts and offsets) from low-level and
	source-level sections in the source file.
	Args:		inFilename		name of image file, for error reporting
				inFile			image file ref
				inCodeSize
				inDebugSize
				inTable
				inFunctions
	Return:  --
------------------------------------------------------------------------------*/

void
FindSymbolicInfo(const char * inFilename, FILE * inFile, long inCodeSize, size_t inDebugSize, CSymbolTable * inTable, CSymbol * inFunctions)
{
	ItemSection item;
	char  symbol[256];
	int	numOfLowLevelSections;
	int	numOfSourceLevelSections;

	if (fseek(inFile, inCodeSize, SEEK_SET) != 0)
		ExitWithMessage("can’t seek to debug info in file “%s”", inFilename);
	numOfLowLevelSections = 0;
	numOfSourceLevelSections = 0;
	while (inDebugSize != 0)
	{
		unsigned long  itemSize = sizeof(ItemSection);
		if (fread(&item, itemSize, 1, inFile) != 1)
			ExitWithMessage("can’t read %ld bytes for section item header from file “%s”", itemSize, inFilename);
		inDebugSize -= itemSize;
		if (item.size > inDebugSize)
			ExitWithMessage("item length %ld exceeds remaining debug size %ld", item.size, inDebugSize);
		if (item.code != kItemSection)
			ExitWithMessage("expected section item in debug data %d: length = %d", item.code, item.size);
		if (item.language == kLanguageNone)
		{
			if (numOfLowLevelSections == 1)
				ExitWithMessage("more than one low-level section in file “%s”", inFilename);
			FindProcOffsets(inFilename, inFile, item, inCodeSize, &inDebugSize, inTable);
			numOfLowLevelSections++;
			if (AnyUndefinedOffsets(inFunctions))
				ExitWithMessage("one or more defined functions were not found in the code module");
		}
		else
		{
			memcpy(symbol, item.name, 3);
			itemSize = item.size - sizeof(ItemSection);
			if (fread(&symbol[3], 1, itemSize, inFile) != itemSize)
				ExitWithMessage("can’t read %ld bytes for section name from file “%s”", itemSize, inFilename);
			inDebugSize -= itemSize;
			Progress("searching for symbolic function definitions within file “%s”", inFilename);
			if (ArgCountsNotDefined(inFunctions, false))
			{
				FindSourceProcs(inFilename, inFile, item, inCodeSize, &inDebugSize, inTable);
				numOfSourceLevelSections++;
			}
			else
			{
				itemSize = item.size - item.debugSize;
				if (fseek(inFile, itemSize, SEEK_CUR) != 0)
					ExitWithMessage("can’t seek over source-level debug data");
				inDebugSize -= itemSize;
			}
		}
	}
	if (ArgCountsNotDefined(inFunctions, true))
		ExitWithMessage("couldn’t locate function argument counts for all functions");
}


/*------------------------------------------------------------------------------
	Read relocation info.
	Args:		inFilename		name of image file, for error reporting
				inFile			image file ref
	Return:  number of code relocations found
------------------------------------------------------------------------------*/

long
GetAIFRelocs(const char * inFilename, FILE * inFile)
{
	unsigned long  i, numOfRelocTableEntries;
	unsigned long  numOfRelocs = 0;

	gRelocs = NULL;
	if ((gImageHeader.selfRelocCode & 0xFF000000) == 0xEB000000)  // is executable AIF
		ExitWithMessage("file “%s” was not linked with linker options “-aif -bin -rel”", inFilename);
	if (gImageHeader.selfRelocCode != 0)
	{
		if (fseek(inFile, gImageHeader.selfRelocCode, SEEK_SET) != 0)
			ExitWithMessage("can’t seek to AIF relocations table in file “%s”", inFilename);
		if (fread(&numOfRelocTableEntries, sizeof(numOfRelocTableEntries), 1, gImageFile) != 1)
			ExitWithMessage("can’t read relocations count from file “%s”", inFilename);
		if (numOfRelocTableEntries > 0)
		{
			size_t	relocTableSize = numOfRelocTableEntries * sizeof(unsigned long);
			if ((gRelocs = (unsigned long *) malloc(relocTableSize)) == NULL)
				ExitWithMessage("can’t allocate %ld bytes for relocations table", relocTableSize);
			if (fread(gRelocs, relocTableSize, 1, gImageFile) != 1)
				ExitWithMessage("can’t read AIF relocations table from file “%s”", inFilename);
			unsigned long  numOfCodeRelocs = numOfRelocTableEntries;
			for (i = 0; i < numOfRelocTableEntries; i++)
			{
				if (gRelocs[i] > gImageHeader.roSize)
				{
					numOfCodeRelocs = i;
					break;
				}
				numOfRelocs++;
			}
			if (numOfCodeRelocs < numOfRelocTableEntries)
			{
				for (i = numOfCodeRelocs + 1; i < numOfRelocTableEntries; i++)
				{
					if (gRelocs[i] < gImageHeader.roSize)
					{
						gRelocs[numOfCodeRelocs++] = gRelocs[i];
						numOfRelocs++;
					}
				}
			}
		}
		else if (numOfRelocTableEntries != 0)
			ExitWithMessage("bogus relocations count (0x%08X) found in file “%s”", numOfRelocTableEntries, inFilename);
	}
	return numOfRelocs;
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
EmitNTKInterface(long inCodeSize, size_t inDebugSize, long inHeaderSize, size_t inROSize)
{
	CSymbol *		functionNames = nil;
	CSymbolTable * table = new CSymbolTable(4*KByte);
	if (FillSymbolTable(table, &functionNames))
		ExitWithMessage("unable to build a valid function name table");
	FindSymbolicInfo(gImageFilename, gImageFile, inCodeSize, inDebugSize, table, functionNames);
	gNumOfRelocs = GetAIFRelocs(gImageFilename, gImageFile);
	EmitNCMFrameFile(functionNames);
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
AssureNextArg(const char ** inArg, const char ** inArgLimit, char * inMessage)
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
			if (gExportFileCount >= 2)
				ExitWithMessage("%d is the maximum number of function name files allowed", gExportFileCount);
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
	bool				hasSourceLevelInfo, hasLowLevelInfo;
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
	if (fread(&gImageHeader, sizeof(AIFHeader), 1, gImageFile) != 1)
		ExitWithMessage("can’t read %ld bytes for aif_header from file “%s”", sizeof(AIFHeader), gImageFilename);
	if (gImageHeader.exitCode != 0xEF000011)
		ExitWithMessage("file “%s” does not seem to be in AIF format", gImageFilename);

	gIsExecutableAIF = ((gImageHeader.imageEntryCode & 0xFF000000) == 0xEB000000);
	if (gImageHeader.roSize < sizeof(long))
		ExitWithMessage("read-only code image is zero bytes long! All code was dead-stripped, probably due to missing/invalid “main”.");
	if (gIsExecutableAIF)
		ExitWithMessage("file “%s” was linked improperly for use with NTK; link with options “-aif -bin -rel”", gImageFilename);
	if (gImageHeader.imageBase != 0)
		ExitWithMessage("file “%s” can NOT be linked with a non-zero base address", gImageFilename);

	now = time(NULL);
	strftime(timeStamp, sizeof(timeStamp), "on %D at %R", localtime(&now));
	Progress("NTK C functions file converted from “%s” by AIFtoNTK %s", gImageFilename, timeStamp);
	Progress("read-only (code) size = 0x%08X", gImageHeader.roSize);

	hasLowLevelInfo = gImageHeader.debugType & 0x01;
	hasSourceLevelInfo = gImageHeader.debugType & 0x02;

	if (gDoDump)
	{
		printf("debug size = 0x%08X: %s%s%s debugging information is present.\n", gImageHeader.debugSize,
				 hasLowLevelInfo ? "low-level" : "", (hasLowLevelInfo && hasSourceLevelInfo) ? " and " : "", hasSourceLevelInfo ? "source-level" : "");
		if (gDoslOption || gDosOption)
			DumpDebugInfo(gImageFilename, gImageFile, gImageHeader.roSize + gImageHeader.rwSize + (gIsExecutableAIF ? 0 : sizeof(AIFHeader)), gImageHeader.debugSize, hasLowLevelInfo, hasSourceLevelInfo);
	}

	else if (gImageHeader.rwSize != 0)
		ExitWithMessage("file “%s” has R/W global variables which are NOT supported in Newton packages.\n"
							 "#  You can use const global variables and *const pointers", gImageFilename);

	else if (gImageHeader.debugType != 0)
	{
		if (hasLowLevelInfo == false || hasSourceLevelInfo == false)
			ExitWithMessage("file “%s” was not linked/compiled with required options for use with NTK; link with “-debug” & compile with with “-gf”", gImageFilename);
		EmitNTKInterface(gImageHeader.roSize + gImageHeader.rwSize + (gIsExecutableAIF ? 0 : sizeof(AIFHeader)), gImageHeader.debugSize, gIsExecutableAIF ? 0 : sizeof(AIFHeader), gImageHeader.roSize);
	}
	else if (gImageHeader.debugSize == 0)
		ExitWithMessage("file “%s” was not linked/compiled with required options for use with NTK; link with “-debug” & compile with with “-gf”", gImageFilename);
	else
		printf("0x%08X bytes of unexpected debugging information is present.\n", gImageHeader.debugSize);

	fclose(gImageFile);

	return 0;
}
