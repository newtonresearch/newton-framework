/*
	All this seems to do is read a text file with lines in format
		"name" driverName imagingName type 1234
	and create a NSOF file of corresponding frames.
*/


#define PARTFILE_EXTENSION ".part"

// object heap
Ptr gHeap;				//-$052E		allocated block
size_t gHeapSize;		//-$0532		size of allocation
size_t gHeapLimit;	//-$0536		limit of used heap

Ref gDriverFrameMap;	//-$0542

// symbol table
struct Literal
{
	Ptr		ptr;
	size_t	len;
	Ref		ref;
};
Literal * gSymbolTable;	//-$1A4A
ArrayIndex gNumOfSymbols;	//-$1A4E

// string table
Literal * gStringTable;	//-$1746
ArrayIndex gNumOfStrings;	//-$174A


const char * gCmdName;	//-$052A
FILE * fd;	//-$0190


/* -----------------------------------------------------------------------------
	Report an error and exit.
	Args:		inError			what went wrong
				inReason			supplementary information
	Return:  --
----------------------------------------------------------------------------- */

void
FatalError(const char * inError, const char * inReason)
{
	fprintf(fd, "### %s %s%s\n", gCmdName, inError, inReason);	//-$2FE0
	exit(1);
}


/* -----------------------------------------------------------------------------
	Allocate an object on the heap.
	Objects are 8-byte aligned.
	The heap is never compacted so we can always map Ref <-> Ptr without worrying.
	Args:		inFlags		type of object to allocate
				inSize		its size
	Return:  pointer to object within heap
----------------------------------------------------------------------------- */

ObjHeader *
AllocateObject(int inFlags, size_t inSize)
{
	size_t alignedSize = ALIGN(inSize, 8);
	size_t objOffset = gHeapLimit;
	gHeapLimit += alignedSize;
	if (gHeapLimit > gHeapSize) {
		// expand the heap in 1K chunks
		gHeapSize += 1*KByte;
		gHeap = realloc(gHeap, gHeapSize);
		if (gHeap == NULL) {
			FatalError("Out of memory", "");	// $2FD0 $2FD2
		}
	}
	// object is next free space on heap
	ObjHeader * obj = (ObjHeader *)(gHeap + objOffset);
	obj->flags = inFlags;
	obj->size = inSize;
	obj->gc.stuff = 0;
	// make the object data ace bad
	size_t dataSize = alignedSize - sizeof(ObjHeader);
	ULong * p = (ULong *)(obj+1);
	for (ArrayIndex i = 0; i < dataSize/sizeof(ULong); ++i, ++p) {
		*p = 0xBEACEBAD;
	}
	return obj;	// original returned objOffset
}


/* ---------------------------------------------------------------------
	Make a string object.
	Strings are kept in a table; making a string that already exists
	returns that existing string.
	Args:		inName		C string to be made string
	Return:	Ref			the string object
--------------------------------------------------------------------- */

Ref
MakeString(const char * inStr)
{
	ArrayIndex strLen = strlen(inStr);
	// see if it already exists
	Literal * s = gStringTable;
	for (ArrayIndex i = 0; i < gNumOfStrings; ++i, ++s) {
		if (s->len == strLen
		&&  strcmp(s->ptr, inStr) == 0) {
			return s->ref;
		}
	}
	// nope: create it on the heap
	StringObject * str = (StringObject *)AllocateObject(kObjReadOnly, SIZEOF_STRINGOBJECT(strLen));
	memcpy(str->name, inName, strLen);
	if (gNumOfStrings  < 0xC0) {
		// add it to the symbol table
		s = gStringTable[gNumOfStrings];
		s->sym = inStr;
		s->len = strLen;
		s->ref = REF(str);
		++gNumOfStrings;
	}
	return REF(str);
}


/* ---------------------------------------------------------------------
	Hash a symbol name.
	Args:		inName		C string to be made symbol
	Return:	ULong			the hash value
--------------------------------------------------------------------- */

ULong
SymbolHashFunction(const char * inName)
{
	char ch;
	ULong sum = 0;

	while ((ch = *inName++) != 0)
		sum += toupper(ch);

	return sum * 0x9E3779B9;	// 2654435769
}


/* ---------------------------------------------------------------------
	Make a symbol object.
	Symbols are kept in a table; making a symbol that already exists
	returns that existing symbol.
	Args:		inName		C string to be made symbol
	Return:	Ref			the symbol object
--------------------------------------------------------------------- */

Ref
MakeSymbol(const char * inName)
{
	ArrayIndex symLen = strlen(inName);
	// see if it already exists
	Literal * s = gSymbolTable;
	for (ArrayIndex i = 0; i < gNumOfSymbols; ++i, ++s) {
		if (s->len == symLen
		&&  strcmp(s->ptr, inSym) == 0) {
			return s->ref;
		}
	}
	// nope: create it on the heap
	SymbolObject * sym = (SymbolObject *)AllocateObject(kObjReadOnly, SIZEOF_SYMBOLOBJECT(symLen));
	sym->hash = SymbolHashFunction(inName);
	memcpy(sym->name, inName, symLen);
	if (gNumOfSymbols  < 0x40) {
		// add it to the symbol table
		s = gSymbolTable[gNumOfSymbols];
		s->sym = inName;
		s->len = symLen;
		s->ref = REF(sym);
		++gNumOfSymbols;
	}
	return REF(sym);
}


/* ---------------------------------------------------------------------
	Make a printer driver frame object.
	Args:		inName		user-visible name
				inDriverName
				inImagingName
				inType
				inModel
	Return:	Ref			the driver frame
--------------------------------------------------------------------- */

Ref
MakeDriverFrame(Ref inName, Ref inDriverName, Ref inImagingName, Ref inType, int inModel)
{
	FrameObject * frame = (FrameObject *)AllocateObject(kObjReadOnly+kFrameObject, SIZEOF_FRAME(5));
	frame->map = gDriverFrameMap;
	frame->slot[0] = inName;
	frame->slot[1] = inDriverName;
	frame->slot[2] = inImagingName;
	frame->slot[3] = inType;
	frame->slot[4] = MAKEINT(inModel);
	return REF(frame);
}


Ref
SetOutputFrame(Ref * inSlots)
{
	FrameMapObject * frameMap = (FrameMapObject *)AllocateObject(kObjReadOnly+kArrayObject, SIZEOF_FRAMEMAPOBJECT(v0E46-1));
	frameMap->objClass = 0;
	frameMap->superMap = NILREF;
	char tag[64];
	for (ArrayIndex i = 0; i < v0E46; ++i) {
		sprintf(tag, "%d", i);
		frameMap->slot[i] = MakeSymbol(tag);
	}
	FrameObject * frame = (FrameObject *)AllocateObject(kObjReadOnly+kFrameObject, SIZEOF_FRAME(v0E46));
	frame->map = REF(frameMap);
	memcpy(frame->slot, inSlots, v0E46*sizeof(Ref));
	return REF(frame);
}


void
CreateObjects(void)
{
	Ref drivers[256];
	for (ArrayIndex i = 0; i < v0E46; ++i) {
		a4 = $F1BE[i];	// size 36
		drivers[i] = MakeDriverFrame(MakeString(a4->00, a4->04), MakeString(a4->08, a4->0C), MakeString(a4->10, a4->14), MakeSymbol(a4->18, a4->1C))
	}
	SetOutputFrame(drivers);
}


/* -----------------------------------------------------------------------------
	Dump the raw heap data.
	Args:		inFile		the file to write
	Return:  error code
----------------------------------------------------------------------------- */

int
DumpPart(FILE * inFile)
{
	CreateObjects();
	int x = fwrite(gHeap, 1, gHeapLimit, inFile);
	return (x == gHeapLimit) ? noErr : -1;
}


void
GetCharsUntil(FILE * inFile, char inTerminator, int, long&)
{
}
"End of file inside string"
"Odd number of bytes inside \\u sequence"
"No escapes but \\u are allowed inside \\u sequence"
"Odd number of hex digits inside \\u sequence"
"Invalid hex digit inside \\u sequence"


void
ParseInput(const char * inFilename)
{
	FILE * fd = fopen(inFilename, "r");	//-$2EDC
	if (fd == 0) {
		FatalError("Couldn’t open input file", "");	//-$2ED8 -$2EDA
	}

	while ((ch = getc(fd)) != EOF) {
		if (v0E46 > 0x40) {
			FatalError("Too many driver definitions -- recompile", gCmdName);	//-$2EBE -$052A
		}
		if (ch == '#') {
			// skip comment to end of line
			while ((ch = getc(fd)) != EOF) {
				if (ch == '\n') {
					break;
				}
			}
		} else if (ch == '"') {
			++v0E46;
			*a3 = GetCharsUntil(inFile '"', 1, vF1BE->24);

			// skip whitespace
			while ((ch = getc(fd)) != EOF) {
				if (ch == ' ' || ch == '\t') {
					continue;
				}
				if (ch == '\n') {
					FatalError("Unexpected end of line", "");	//-$2E7A -$2E7C
				}
			}
			ungetc(ch, inFile);

			a3->8 = GetCharsUntil(inFile 0, 1, a3->c);

			// skip whitespace
			while ((ch = getc(fd)) != EOF) {
				if (ch == ' ' || ch == '\t') {
					continue;
				}
				if (ch == '\n') {
					FatalError("Unexpected end of line", "");	//-$2E60 -$2E62
				}
			}
			ungetc(ch, inFile);

			a3->10 = GetCharsUntil(inFile 0, 1, a3->14);

			// skip whitespace
			while ((ch = getc(fd)) != EOF) {
				if (ch == ' ' || ch == '\t') {
					continue;
				}
				if (ch == '\n') {
					FatalError("Unexpected end of line", "");	//-$2E46 -$2E48
				}
			}
			ungetc(ch, inFile);

			a3->18 = GetCharsUntil(inFile 0, 0, a3->1c);

			// skip whitespace
			while ((ch = getc(fd)) != EOF) {
				if (ch == ' ' || ch == '\t') {
					continue;
				}
				if (ch == '\n') {
					FatalError("Unexpected end of line", "");	//-$2E2C -$2E2E
				}
			}
			ungetc(ch, inFile);

			if (fscanf(fd, "%ld", &a3->20) != 1) {	//-$2E14
				FatalError("Expected integer", "");	//-$2E0E -$2E10
			}

		} else {
			FatalError("Expected printer name", "");	//-$2E92 -$2E94
		}
	}

	fclose(fd);
}


/* -----------------------------------------------------------------------------
	Initialize heap objects.
	Args:		--
	Return:  --
----------------------------------------------------------------------------- */

void
Initialize(void)
{
	ArrayObject * d7 = (ArrayObject *)AllocateObject(kObjReadOnly+kArrayObject, SIZEOF(ARRAYOBJECT(1)));
	if (d7 == NULL)
		FatalError("Data initialization failed!" "Out of memory");	//-$2DFA,-$2DFC
	d7->objClass = NILREF;

	$053A = MakeSymbol("-$2DDC", 7);		//String?
	$053E = MakeSymbol("-$2DD4", 6);		//Array?

	FrameMapObject * driverFrameMap = (FrameMapObject *)AllocateObject(kObjReadOnly+kArrayObject, SIZEOF_FRAMEMAPOBJECT(5));
	driverFrameMap->objClass = 0;
	driverFrameMap->superMap = NILREF;
	driverFrameMap->slot[0] = MakeSymbol("name");
	driverFrameMap->slot[1] = MakeSymbol("driverName");
	driverFrameMap->slot[2] = MakeSymbol("imagingName");
	driverFrameMap->slot[3] = MakeSymbol("type");
	driverFrameMap->slot[4] = MakeSymbol("prModel");
	gDriverFrameMap = REF(driverFrameMap);
}


/* -----------------------------------------------------------------------------
	Stream a long value.
	Longs are compressed so that values < 254 occupy only one byte.
	Args:		inFile		the file to write
				inVal			the value
	Return:  --
----------------------------------------------------------------------------- */

void
StreamLong(FILE * inFile, long inVal)
{
	if (inVal >= 0 && inVal <= 0xFE) {
		fputc(inVal, inFile);
	} else {
		fputc(0xFF, inFile);
		fwrite(&inVal, sizeof(long), 1, inFile);
	}
}



StreamSymbol(FILE * inFile, const char * inSym, long inLen)
{
	for (ArrayIndex i = 0; i < -$2356; ++i) {
		;	// precedents
	}
	fputc(kFDSymbol, inFile);
	StreamLong(inFile, inLen);
	fwrite(inSym, 1, inLen, inFile);
	if (-$2356 < 0xC0) {
		;	// precedents
		++-$2356;
	}
}


void
StreamString(FILE * inFile, const char * inStr, long inLen)
{
	for (ArrayIndex i = 0; i < -$25CA; ++i) {
		;	// precedents
	}
	fputc(kFDString, inFile);
	StreamLong(inFile, inLen);
	fwrite(inStr, 1, inLen, inFile);
	if (-$25CA < 0xC0) {
		;	// precedents
	}
}


/* -----------------------------------------------------------------------------
	Save the part in Newton Streamed Object Format (NSOF).
	Args:		inFile			file descriptor, open for writing
	Return:  --
----------------------------------------------------------------------------- */

void
StreamPart(FILE * inFile)
{
	// stream always starts with version number
	fputc(kNSOFVersion, inFile);
	// this stream contains an array of printer driver spec frames
	fputc(kFDPlainArray, inFile);
	StreamLong(inFile, -$0E46);
	for (ArrayIndex i = 0; i < -$0E46; ++i) {
		// stream out a printer driver frame
		fputc(kFDFrame, inFile);
		// 5 slots
		StreamLong(inFile, 5);
		-$1A52++;
		// tags first
		StreamSymbol(inFile, "name");			//-$2DA2
		StreamSymbol(inFile, "driverName");	//-$2D9C
		StreamSymbol(inFile, "imagingName");	//-$2D90
		StreamSymbol(inFile, "type");			//-$2D84
		StreamSymbol(inFile, "prModel");		//-$2D7E
		// then corresponding values
		a3 = $F1BE[i];
		StreamString(inFile, a3->00, a3->04);
		StreamString(inFile, a3->08, a3->0C);
		StreamString(inFile, a3->10, a3->14);
		StreamSymbol(inFile, a3->18, a3->1C - 1);
		fputc(kFDImmediate, inFile);
		-$1A52++;
		StreamLong(inFile, MAKEINT(a3->20));
	}
	return 1;
}


/* -----------------------------------------------------------------------------
	Report inappropriate command line arguments and exit.
	Args:		inError			what went wrong
				inValue			the value in question
	Return:  --
----------------------------------------------------------------------------- */

void
Usage(const char * inError, const char * inValue)
{
	if (inArg1 != NULL) {
		fprintf(fd, "### %s %s\n", inError, inValue);	//-$2D76
	}
	fprintf(fd, "### Usage: %s [-o outfilename] [-s] [input]\n", gCmdName);				//-$2D6A
	fprintf(fd, "###        -s stream-format output (memory-format is default)\n");	//-$2D3C
	exit(1);
}


/* -----------------------------------------------------------------------------
	main
----------------------------------------------------------------------------- */

int
main(int argc, const char * argv[])
{
	const char * outputFilename = NULL;	//sp04
	int sp08 = 0;
	const char * inputFilename = NULL;	//sp0C
	bool isStreamFormat = false;	// D6
	gCmdName =  argv[0];
	// parse args
	for (--argc, ++argp; argc != 0; --argc, ++argv) {
		const char * argp = *argv;
		if (argp[0] = '-') {
			if (argp[1] = 'o') {
				if (argc == 0)
					Usage("expected filename with", argp);	//-$2D04
				outputFilename = ++argv;
				++argc;
			} else if (argp[0] = 's') {
				isStreamFormat = true;
			} else {
				Usage("unknown switch", argp);	//-$2CEC
			}
		} else if (inputFilename == NULL) {
			inputFilename = argp;
		} else {
			char buf[256];
			sprintf(buf, "already have an input file (%s)--what’s output", inputFilename);	//-$2CDC
			Usage(buf, argp);
		}
	}
	if (sp08 == NULL) {
		if (inputFilename != NULL) {
			a4 = strrchr(inputFilename, '.');
			if (a4) {
				;
			} else {
				d0 = strlen(inputFilename);
			}
			sp08 = new char[d0+1];
			strncpy(sp08, inputFilename, d0);
		} else {
			sp08 = -$2CB4;
		}
	}

	if (outputFilename == NULL) {
		outputFilename = new char[strlen(sp08) + strlen(PARTFILE_EXTENSION)+1];
		strcpy(outputFilename, sp08);
		strcat(outputFilename, PARTFILE_EXTENSION);
	}

	if (!isStreamFormat) {
		Initialize();
	}
	ParseInput(inputFilename);

	FILE * fd = fopen(outputFilename, "w");	//-$2CAC
	if (fd == 0) {
		FatalError("Couldn’t open output file for writing", "");	//-$2CA8 -$2CAA
	}

	if (isStreamFormat) {
		StreamPart(fd);
	} else {
		DumpPart(fd);
	}
	fclose(fd);

	if (!isStreamFormat) {
		unlink(outputFilename);
		FatalError(-$2C80, -$2C82);
	}
	fsetfileinfo(outputFilename, 'MPS ', 'TEXT');

	return 0;
}


"Couldn’t open output file for writing"
"Couldn’t write output file"
".part"

"Bug! Header offset not zero"
