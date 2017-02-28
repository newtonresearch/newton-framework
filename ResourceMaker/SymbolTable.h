/*
	File:		SymbolTable.h

	Contains:	Symbol table implementation.
					Symbols are plain C function names.

	Written by:	The Newton Tools group.
*/

#ifndef __SYMBOLTABLE_H
#define __SYMBOLTABLE_H 1


/*------------------------------------------------------------------------------
	H a s h   T a b l e
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

	int						fSize;	// MUST be a power of 2
	CHashTableEntry **	fTable;  // pointers to entries
};


/*------------------------------------------------------------------------------
	S y m b o l   T a b l e
------------------------------------------------------------------------------*/

class CSymbol : public CHashTableEntry
{
public:
					CSymbol() {}
					CSymbol(const char * inName, const char * inCName, CSymbol * inLink);
	//				~CSymbol();

	CSymbol *	fNext;
	CSymbol *	fPrev;
	int			fOffset;
	int			fNumArgs;
	char *		fName;
	char *		fCName;
};


class CSymbolTable : public CHashTable
{
public:
								CSymbolTable(int inSize)	:  CHashTable(inSize)  { }

	virtual unsigned long	hash(CHashTableEntry * inEntry);
	virtual int				compare(CHashTableEntry * inEntry1, CHashTableEntry * inEntry2);
	CSymbol *				find(const char * inName);
};


extern bool FillSymbolTable(CSymbolTable * inTable, CSymbol ** outFunctionNames, CSymbol ** outLastFunctionNames);


/*------------------------------------------------------------------------------
	F u n c t i o n   S y m b o l   T a b l e
------------------------------------------------------------------------------*/

class CPointer : public CHashTableEntry
{
public:
					CPointer() {}
					CPointer(uint32_t inFuncPtr, char * inName, CPointer * inLink);
	//				~CPointer();

	CPointer *	fNext;
	CPointer *	fPrev;
	uint32_t		fFuncPtr;
	char *		fName;
};


class CPointerTable : public CHashTable
{
public:
								CPointerTable(int inSize)	:  CHashTable(inSize)  { }

	virtual unsigned long	hash(CHashTableEntry * inEntry);
	virtual int				compare(CHashTableEntry * inEntry1, CHashTableEntry * inEntry2);
	CPointer *				find(const char * inName);
	CPointer *				find(uint32_t inFuncPtr);

private:
	bool	fFindName;
};

extern bool FillSymbolTable(CPointerTable * inTable);

#endif	/* __SYMBOLTABLE_H */
