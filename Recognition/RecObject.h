/*
	File:		RecObject.h

	Contains:	Recognition sub-system declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__RECOBJECT_H)
#define __RECOBJECT_H 1

#include "Objects.h"

extern "C" int	REPprintf(const char * inFormat, ...);

typedef uint32_t RecType;


/*------------------------------------------------------------------------------
	C M s g

	Debug message utility class.
------------------------------------------------------------------------------*/

class CMsg
{
public:
				CMsg();
				CMsg(char * inStr);
				~CMsg();

	CMsg *	iMsg(void);

	CMsg *	msgChar(char inChar);
	CMsg *	msgLF(void);
	CMsg *	msgType(RecType inType);
	CMsg *	msgNum(ULong inNum, int inWidth);
	CMsg *	msgHex(ULong inHex, int inWidth = -1);
	CMsg *	msgStr(const char * inStr);

	void		msgFile(const char * inFilename, ULong inFilebits);
	void		msgPrintf(void);

private:
	char *	fMem;
	size_t	fLength;
	ArrayIndex	fCharsInLine;
	ArrayIndex	fNumOfLines;
};

// flag controlling amount of debug info generated
extern bool gVerbose;


/*------------------------------------------------------------------------------
	C R e c O b j e c t

	Every object in the recognition system is based on CRecObject.

	In a change from the original, we provide reference counting methods here
	in the base class. We also do some renaming for conformance with Cocoa.
	Call retain() on an object to bump its reference count. (Was clone()).
	Call release() rather than dispose() -- think [object release] in Cocoa terms.
	Call dealloc() rather than iDispose when the ref count == 0.

CRecObject
 CArray •
  CDArray
   CTypeList
   CTypeAssoc
   CRecStroke
   CRecUnitList
  CAreaList •
 CRecArea •
 CRecUnit •
  CSIUnit
   CStrokeUnit
 CRecDomain
  CGeneralShapeDomain
  CEdgeListDomain
  CWRecDomain
  CStrokeDomain
 
------------------------------------------------------------------------------*/

class CRecObject
{
public:
							CRecObject();
	virtual				~CRecObject();

// reference counting
	bool					isRetained(void) const;
	virtual CRecObject *	retain(void);
	virtual void		release(void);
	virtual void		dealloc(void);

// flags
	void					setFlags(ULong inBits);
	void					unsetFlags(ULong inBits);
	bool					testFlags(ULong inBits) const;

// cloning
	virtual NewtonErr	copyInto(CRecObject * x);

// debug
	virtual size_t		sizeInBytes(void);
	virtual void		dump(CMsg * outMsg);
	void					dumpObject(char * inStr);

protected:
	void					iRecObject(void);

	ULong			fFlags;		// +04
	int			fRefCount;
};

inline bool	CRecObject::isRetained(void) const   { return fRefCount > 0; }

inline void CRecObject::setFlags(ULong inBits)   { fFlags |= inBits; }
inline void CRecObject::unsetFlags(ULong inBits) { fFlags &= ~inBits; }
inline bool CRecObject::testFlags(ULong inBits) const  { return (fFlags & inBits) != 0; }


/*------------------------------------------------------------------------------
	C A r r a y

	Yet another array implementation, this time just for the recognition
	sub-system.
------------------------------------------------------------------------------*/

struct ArrayIterator;
typedef Ptr (*IterProcPtr)(ArrayIterator *);

struct ArrayIterator
{
	ArrayIndex	count(void) const;
	Ptr			getNext(void);
	void			removeCurrent(void);

	Ptr			fMem;				// +00	(was Handle)
	Ptr			fMemPtr;			// +04
	Ptr			fCurrent;		// +08
	size_t		fElementSize;  // +0C
	ArrayIndex  fIndex;			// +10
	ArrayIndex  fSize;			// +14
	IterProcPtr fGetNext;		// +18
	IterProcPtr fGetCurrent;	// +1C
};

inline ArrayIndex ArrayIterator::count(void) const { return fSize; }
inline Ptr ArrayIterator::getNext(void) { return fGetNext(this); }


class CArray : public CRecObject
{
public:
							CArray();
	virtual				~CArray();

	static CArray *	make(size_t inElementSize, ArrayIndex inSize);
	virtual void		dealloc(void);

// cloning
	virtual NewtonErr	copyInto(CRecObject * x);

// debug
	virtual size_t		sizeInBytes(void);
	virtual void		dump(CMsg * outMsg);
	void					dumpObject(char * inStr);

	virtual ArrayIndex	add(void);
	virtual Ptr			addEntry(void);
	virtual Ptr			getEntry(ArrayIndex index);
	virtual void		setEntry(ArrayIndex index, Ptr inData);

	virtual void		compact(void);
	virtual void		cutToIndex(ArrayIndex index);
	virtual void		clear(void);
	virtual NewtonErr	reuse(ArrayIndex index);

	virtual NewtonErr load(ULong, ULong, ULong, ULong);
	virtual NewtonErr loadFromSoup(RefArg, RefArg, ULong);
	virtual NewtonErr save(ULong, ULong, ULong, ULong);

//	void					lock(void);			we don’t use Handles any more
//	void					unlock(void);

	ArrayIndex			count(void) const;
	bool					isEmpty(void) const;
	size_t				elementSize(void) const;
	Ptr					getIterator(ArrayIterator * ioIter);

protected:
	NewtonErr			iArray(size_t inElementSize, ArrayIndex inSize);

	size_t			fElementSize;  // +08	size of a single element
	ArrayIndex		fSize;			// +0C	logical size of array
	ArrayIndex		fFree;			// +10
	ArrayIndex		fChunkSize;		// +14	grow array by this many elements
//	int				fRefCount;		// +18
	char *			fMem;				// +1C	element storage (was Handle)
};


/*------------------------------------------------------------------------------
	C A r r a y   I n l i n e s
------------------------------------------------------------------------------*/

inline ArrayIndex CArray::count(void) const
{ return fSize; }

inline bool CArray::isEmpty(void) const
{ return (fSize == 0); }

inline size_t CArray::elementSize(void) const
{ return fElementSize; }


/*------------------------------------------------------------------------------
	C D A r r a y

	Yet another dynamic array implementation.
------------------------------------------------------------------------------*/

class CDArray : public CArray
{
public:
// make
	static CDArray *	make(size_t inElementSize, ArrayIndex inSize);

	virtual ArrayIndex	insert(ArrayIndex index);
	virtual ArrayIndex	insertEntry(ArrayIndex index, Ptr inData);
	virtual ArrayIndex	insertEntries(ArrayIndex index, Ptr inData, ArrayIndex inCount);
	virtual ArrayIndex	deleteEntry(ArrayIndex index);	// was plain delete
	virtual ArrayIndex	deleteEntries(ArrayIndex index, ArrayIndex inCount);

protected:
	NewtonErr		iDArray(size_t inElementSize, ArrayIndex inSize);
};


/*------------------------------------------------------------------------------
	C T y p e L i s t 
------------------------------------------------------------------------------*/

class CTypeList : public CDArray
{
public:
	static CTypeList *	make(void);

// debug
	virtual void		dump(CMsg * outMsg);

	NewtonErr		addType(RecType inType);
	NewtonErr		addUnique(RecType inType);
	ArrayIndex		findType(RecType inType);
	RecType			getType(ArrayIndex index);

protected:
	NewtonErr		iTypeList(void);
};


/*------------------------------------------------------------------------------
	R e c D o m a i n I n f o
------------------------------------------------------------------------------*/

struct RecDomainInfo		// formerly dInfoRec
{
	long  notReally;
};


/*------------------------------------------------------------------------------
	C T y p e A s s o c
------------------------------------------------------------------------------*/
class CSIUnit;
typedef ULong (*AreaHandler)(CSIUnit *, ULong, RecDomainInfo *);	// where does this come from?
typedef ULong (*UnitHandler)(CArray*);

class CRecDomain;
struct Assoc
{
	RecType			fType;			// +00
	CRecDomain *	fDomain;			// +04
	Ptr				fInfo;			// +08	was Handle
	RecDomainInfo * fDomainInfo;	// +0C
	UnitHandler		fHandler;		// +10
	ULong				f14;
	bool				f18;
};


class CTypeAssoc : public CDArray
{
public:
	static CTypeAssoc *	make(void);
	virtual void	dealloc(void);

// cloning
	CTypeAssoc *	copy(void);

// debug
	virtual void	dump(CMsg * outMsg);

	ArrayIndex		addAssoc(Assoc * inAssoc);
	Assoc *			getAssoc(ArrayIndex index);
	NewtonErr		mergeAssoc(CTypeAssoc * inAssoc);

protected:
	NewtonErr		iTypeAssoc(void);
};


#endif	/* __RECOBJECT_H */
