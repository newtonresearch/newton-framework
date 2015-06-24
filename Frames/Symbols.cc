/*
	File:		Symbols.c

	Contains:	Newton symbol functions.

	Written by:	Newton Research Group.
*/

#include <ctype.h>

#include "Objects.h"
#include "ObjHeader.h"
#include "Globals.h"
#include "Symbols.h"
#include "Frames.h"
#include "Arrays.h"


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FSymbolCompareLex(RefArg inRcvr, RefArg inSym1, RefArg inSym2);
Ref	FSymbolName(RefArg inRcvr, RefArg inSym);
}


/*----------------------------------------------------------------------
	C o n s t a n t s
----------------------------------------------------------------------*/

const ArrayIndex kCalcTableSize  = 1 << 1;
const ArrayIndex kCalcTableShift = 32 - 1;

const ArrayIndex kMinTableSize  = 1 << 5;
const ArrayIndex kMinTableShift = 32 - 5;

const ArrayIndex kInitTableSize  = 1 << 7;
const ArrayIndex kInitTableShift = 32 - 7;

const ArrayIndex kBuiltInSymbolTableSize  = 1 << 9;
const ArrayIndex kBuiltInSymbolTableShift = 32 - 9;


/*----------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------*/

extern Ref * RSsymbolTable;

Ref			gSymbolTable;
ArrayIndex	gSymbolTableSize;
ArrayIndex	gSymbolTableHashShift;

Ref *			gROMSymbolTable;
ArrayIndex	gROMSymbolTableSize;
ArrayIndex	gROMSymbolTableHashShift;

ArrayIndex	gNumSymbols;
ArrayIndex	gNumSlotsTaken;


/*----------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------*/

static bool		FindSymbol(Ref inSymbolTable[], ArrayIndex size, ArrayIndex shift, const char * name, ULong hash, ArrayIndex * index);

static void		InternExistingSymbol(Ref sym);

static void		EnlargeSymbolTable(void);
static void		AdjustSymbolTableSize(void);
static void		RehashSymbolTable(void);

static void		ResizeSymbolTable(ArrayIndex, ArrayIndex);


/*----------------------------------------------------------------------
	Initialize the symbol table.
	Args:		--
	Return:	--
----------------------------------------------------------------------*/

void
InitSymbols(void)
{
	gSymbolTableSize = kInitTableSize;
	gSymbolTableHashShift = kInitTableShift;
	gSymbolTable = AllocateArray(RA(NILREF), gSymbolTableSize);
	AddGCRoot(&gSymbolTable);

	gROMSymbolTable = Slots(RA(symbolTable));
	gROMSymbolTableSize = Length(RA(symbolTable));
	ArrayIndex targetSize = gROMSymbolTableSize;
	ArrayIndex shiftCount = kCalcTableShift;
	for (ArrayIndex size = kCalcTableSize; size < targetSize; size *= 2)
	{
		shiftCount--;
	}
	gROMSymbolTableHashShift = shiftCount;
}


/*----------------------------------------------------------------------
	Make a new symbol object.
	Args:		name		the symbol name
	Return:	Ref		the symbol object
----------------------------------------------------------------------*/

Ref
MakeSymbol(const char * name)
{
	ArrayIndex index;
	ULong hash = SymbolHashFunction(name);

	if (FindSymbol(gROMSymbolTable, gROMSymbolTableSize, gROMSymbolTableHashShift, name, hash, &index))
		// no need to intern it because it’s already in there
		return gROMSymbolTable[index];

	if (FindSymbol(Slots(gSymbolTable), gSymbolTableSize, gSymbolTableHashShift, name, hash, &index))
		// no need to intern it because it’s already in there
		return GetArraySlot(gSymbolTable, index);

	RefVar	sym(AllocateBinary(kSymbolClass, sizeof(ULong) + strlen(name) + 1));	// symbols are nul-terminated
	*(ULong *)BinaryData(sym) = hash;
	strcpy((char *)SymbolName(sym), name);

	// symbol has been created - add it to the table
	if (gNumSymbols > Length(gSymbolTable) * 85 / 100)
	{
		// table is 85% full - enlarge it
		EnlargeSymbolTable();
		InternExistingSymbol(sym);
	}
	else
	{
		SetArraySlot(gSymbolTable, index, sym);
	}

	// update table limits
	gNumSymbols++;
	gNumSlotsTaken++;
	if (gNumSymbols < gSymbolTableSize/4)
		AdjustSymbolTableSize();
	else if (gNumSlotsTaken > gSymbolTableSize * 85 / 100)
		RehashSymbolTable();

	return sym;
}

#pragma mark -

/*----------------------------------------------------------------------
	Find a symbol in the symbol table, using its hashed value.
	Args:		inSymbolTable
				inSize
				inShift
				inName			the symbol name
				inHash			the symbol hash
				outIndex			index into the symbol table
	Return:	bool				found (or not)
----------------------------------------------------------------------*/

static bool
FindSymbol(Ref inSymbolTable[], ArrayIndex inSize, ArrayIndex inShift, const char * inName, ULong inHash, ArrayIndex * outIndex)
{
	Ref	sym;
	ArrayIndex  hashIndex;

	// compute an index into the symbol table using the hash value
	hashIndex = inHash >> inShift;
	*outIndex = hashIndex;

	if (NOTNIL(sym = inSymbolTable[hashIndex]))
	{
		// check that the symbol at that index is the one we want - might be a hash collision
		ArrayIndex  incr = (hashIndex & 0x07) * 2 + 1;
		ArrayIndex  actualIndex = hashIndex;
		ArrayIndex  intIndex = kIndexNotFound;
		do
		{
			if (ISINT(sym))
				intIndex = actualIndex;
			else if (symcmp((char *)inName, SymbolName(sym)) == 0)
			{
				*outIndex = actualIndex;
				return YES;
			}
			actualIndex += incr;
			if (actualIndex >= inSize)
				actualIndex -= inSize;
		} while (NOTNIL(sym = inSymbolTable[actualIndex]));
		if (intIndex != kIndexNotFound)
			*outIndex = intIndex;
	}

	// we didn’t find it
	return NO;
}


/*----------------------------------------------------------------------
	Hash a symbol name.
	Args:		inName		C string to be made symbol
	Return:	ULong			the hash value
----------------------------------------------------------------------*/

ULong
SymbolHashFunction(const char * inName)
{
	char ch;
	ULong sum = 0;

	while ((ch = *inName++) != 0)
		sum += toupper(ch);

	return sum * 0x9E3779B9;	// 2654435769
}


/*----------------------------------------------------------------------
	Compare two symbol objects.
	Args:		sym1		a symbol
				sym2		another symbol
	Return:	int		0		if symbol names are equal
							>0		if sym1 is greater than sym2
							<0		if sym1 is less than sym2
----------------------------------------------------------------------*/

int
SymbolCompare(Ref sym1, Ref sym2)
{
	if (sym1 == sym2)
		return 0;
	if (((SymbolObject *)PTR(sym1))->hash != ((SymbolObject *)PTR(sym2))->hash)
	{
		if (((SymbolObject *)PTR(sym1))->hash > ((SymbolObject *)PTR(sym2))->hash)
			return 1;
		else
			return -1;
	}
	return symcmp(((SymbolObject *)PTR(sym1))->name,
					  ((SymbolObject *)PTR(sym2))->name);
}


int
SymbolCompareLexRef(Ref sym1, Ref sym2)
{
	return symcmp(((SymbolObject *)PTR(sym1))->name,
					  ((SymbolObject *)PTR(sym2))->name);
}


int
SymbolCompareLex(RefArg sym1, RefArg sym2)
{
	if (!IsSymbol(sym1))
		ThrowExFramesWithBadValue(kNSErrBadArgs, sym1);
	if (!IsSymbol(sym2))
		ThrowExFramesWithBadValue(kNSErrBadArgs, sym2);

	return SymbolCompareLexRef(sym1, sym2);
}


Ref
FSymbolCompareLex(RefArg inRcvr, RefArg inSym1, RefArg inSym2)
{
	return MAKEINT(SymbolCompareLex(inSym1, inSym2));
}


/*----------------------------------------------------------------------
	Compare two symbol objects.
	Unsafe in that it doesn’t check the object type.
	Args:		sym1		a symbol
				sym2		a symbol to compare it with
				hash		hash value of first symbol
	Return:	bool		YES if symbol names are equal
							NO  otherwise
----------------------------------------------------------------------*/

bool
UnsafeSymbolEqual(Ref sym1, Ref sym2, ULong hash)
{
	if (sym1 == kBadPackageRef)
		ThrowErr(exFrames, kNSErrBadPackageRef);
	if (sym1 == sym2)
		return YES;

	// if both symbols are in ROM they must have the same address to be equal
/*
	if (sym1 < ROM$$Size && sym2 < ROM$$Size)
		return NO;
*/
	if (((SymbolObject*)PTR(sym1))->hash != hash)
		return NO;
	return symcmp(((SymbolObject *)PTR(sym1))->name,
					  ((SymbolObject *)PTR(sym2))->name) == 0;
}


/*----------------------------------------------------------------------
	Return a symbol's hash value.
	Args:		sym		a symbol
	Return:	ULong	the hash value
----------------------------------------------------------------------*/

ULong
SymbolHash(Ref sym)
{
	return *(ULong*) BinaryData(sym);
}


/*----------------------------------------------------------------------
	Return a symbol's name.
	Args:		sym		a symbol
	Return:	const char *	its name
----------------------------------------------------------------------*/

const char *
SymbolName(Ref sym)
{
	return BinaryData(sym) + sizeof(ULong);
}

Ref
FSymbolName(RefArg inRcvr, RefArg inSym)
{
	if (!IsSymbol(inSym))
		ThrowExFramesWithBadValue(kNSErrBadArgs, inSym);
	return MakeStringFromCString(SymbolName(inSym));
}


#pragma mark -

/*----------------------------------------------------------------------
	Compare two symbol names. Case-insensitive.
	Args:		s1			a symbol name
				s2			another symbol name
	Return:	int		0		if symbol names are equal
							>0		if sym1 is greater than sym2
							<0		if sym1 is less than sym2
----------------------------------------------------------------------*/

int
symcmp(const char * s1, const char * s2)
{
	char c1, c2;
	for ( ; ; )
	{
		c1 = *s1++;
		c2 = *s2++;
		if (c1 == '\0')
			return (c2 == '\0') ? 0 : -1;
		if (c2 == '\0')
			break;
		if ((c1 = toupper(c1)) != (c2 = toupper(c2)))
			return (c1 > c2) ? 1 : -1;
	}
	return 1;
}
/*
int
toupper(int c)
{
	return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
}
*/
#pragma mark -

/*----------------------------------------------------------------------
	Intern a symbol in a new (probably enlarged) symbol table.
	Args:		sym		the symbol
	Return:	--
----------------------------------------------------------------------*/

static void
InternExistingSymbol(Ref sym)
{
	ArrayIndex   index;
	ULong  symHash = SymbolHash(sym);
	const char * symName = SymbolName(sym);

	if (!FindSymbol(Slots(gSymbolTable), gSymbolTableSize, gSymbolTableHashShift, symName, symHash, &index))
		SetArraySlot(gSymbolTable, index, sym);
}


/*----------------------------------------------------------------------
	Enlarge the symbol table - double its size.
	Args:		--
	Return:	--
----------------------------------------------------------------------*/

static void
EnlargeSymbolTable(void)
{
	ResizeSymbolTable(gSymbolTableSize * 2, gSymbolTableHashShift - 1);
}


/*----------------------------------------------------------------------
	Adjust the symbol table size to the next power-of-two that will
	hold its current actual size.
	Args:		--
	Return:	--
----------------------------------------------------------------------*/

static void
AdjustSymbolTableSize(void)
{
	ArrayIndex size = kMinTableSize;
	ArrayIndex targetSize = gNumSymbols * 2;
	ArrayIndex shiftCount = kMinTableShift;
	for ( ; size < targetSize; size *= 2)
	{
		shiftCount--;
	}
	if (size != targetSize)
		ResizeSymbolTable(size, shiftCount);
}


/*----------------------------------------------------------------------
	Resize the symbol table.
	Do this by creating a new table of the requested size and copying
	all symbols from the old into the new.
	Args:		newSize		requested size
				newShift		bit shift for that size
	Return:	--
----------------------------------------------------------------------*/

static void
ResizeSymbolTable(ArrayIndex newSize, ArrayIndex newShift)
{
	RefVar oldTable(gSymbolTable);
	ArrayIndex oldSize = gSymbolTableSize;

	gSymbolTable = MakeArray(newSize);
	gSymbolTableSize = newSize;
	gSymbolTableHashShift = newShift;
	for (ArrayIndex i = 0; i < oldSize; ++i)
	{
		Ref sym = GetArraySlot(oldTable, i);
		if (ISPTR(sym))
			InternExistingSymbol(sym);
	}
	gNumSlotsTaken = gNumSymbols;
}


/*----------------------------------------------------------------------
	Rehash the symbol table.
	Do this by creating a new table of the same size and copying
	all symbols from the old into the new.
	Args:		--
	Return:	--
----------------------------------------------------------------------*/

static void
RehashSymbolTable(void)
{
	RefVar oldTable(gSymbolTable);
	ArrayIndex oldSize = gSymbolTableSize;

	gSymbolTable = MakeArray(oldSize);
	for (ArrayIndex i = 0; i < oldSize; ++i)
	{
		Ref sym = GetArraySlot(oldTable, i);
		if (ISPTR(sym))
			InternExistingSymbol(sym);
	}
	gNumSlotsTaken = gNumSymbols;
}
