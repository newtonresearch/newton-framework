/*
	File:			Objects.h

	Contains:	Newton object system.
					Basic object interface.

	A function that takes more than one argument cannot safely take any Ref
	arguments, since it would be possible to invalidate a Ref argument in the
	process of evaluating another argument.  However, to avoid the overhead of
	automatic RefVar creation for critical functions such as EQ, versions are
	available that take Ref arguments (usually called something like EQRef).
	These versions should only be used when all the arguments are simple
	variable references or other expressions that cannot invalidate Refs.

	NOTE that this header declares and uses RefVar C++ classes, so is
	unsuitable for inclusion by plain C files.

	Copyright:	© 1992-1995 by Apple Computer, Inc., all rights reserved.

	<01>			06/25/95	first created for Newton C++ Tools
	<02>			09/13/04	expanded for Newton.framework
*/

#if !defined(__OBJECTS_H)
#define __OBJECTS_H 1

#if !defined(__NEWTON_H)
#include "Newton.h"
#endif

/*------------------------------------------------------------------------------
	R e f   T y p e s
------------------------------------------------------------------------------*/

typedef long Ref;

typedef struct
{
	Ref	ref;
	Ref	stackPos;
} RefHandle;

class RefVar;

typedef const RefVar & RefArg;

typedef Ref (*MapSlotsFunction)(RefArg tag, RefArg value, ULong anything);


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
#define	NILREF			MAKEIMMED(kImmedSpecial, 0)
#define	TRUEREF			MAKEIMMED(kImmedBoolean, 1)
#define	FALSEREF			NILREF
#define	INVALIDPTRREF	MAKEINT(0)

/*------------------------------------------------------------------------------
	I m m e d i a t e   C l a s s   C o n s t a n t s
------------------------------------------------------------------------------*/

#define	kWeakArrayClass		MAKEIMMED(kImmedSpecial, 1)
#define	kFaultBlockClass		MAKEIMMED(kImmedSpecial, 2)
#define	kFuncClass				MAKEIMMED(kImmedSpecial, 3)
#define	kBadPackageRef			MAKEIMMED(kImmedSpecial, 4)
#define	kUnstreamableObject	MAKEIMMED(kImmedSpecial, 5)
#define	kSymbolClass			MAKEIMMED(kImmedSpecial, 0x5555)

#define	kPlainFuncClass		0x0032
#define	kPlainCFunctionClass	0x0132
#define	kBinCFunctionClass	0x0232

#define	ISFUNCCLASS(r)	(((r) & 0xFF) == kFuncClass)
#define	FUNCKIND(r)		(((unsigned) r) >> 8)

#define	RTAG(r)			(((Ref) (r)) & kRefTagMask)
#define	RVALUE(r)		(((Ref) (r)) >> kRefTagBits)
#define	RIMMEDTAG(r)	(RVALUE(r) & ~kRefImmedMask)
#define	RIMMEDVALUE(r)	(RVALUE(r) >> kRefImmedBits)
#define	ISINT(r)			(RTAG(r) == kTagInteger)
#define	NOTINT(r)		(RTAG(r) != kTagInteger)
#define	ISPTR(r)			((BOOL)(((Ref) (r)) & kTagPointer))
#define	ISREALPTR(r)	(RTAG(r) == kTagPointer)
#define	NOTREALPTR(r)	(RTAG(r) != kTagPointer)
#define	ISMAGICPTR(r)	(RTAG(r) == kTagMagicPtr)
#define	ISIMMED(r)		(RTAG(r) == kTagImmed)
#define	ISCHAR(r)		(ISIMMED(r) && (RIMMEDTAG(r) == kImmedChar))
#define	ISBOOLEAN(r)	(ISIMMED(r) && (RIMMEDTAG(r) == kImmedBoolean))

#define	ISNIL(r)			(((Ref) (r)) == NILREF)
#define	NOTNIL(r)		(((Ref) (r)) != NILREF)
#define	ISFALSE(r)		(((Ref) (r)) == FALSEREF)
#define	ISTRUE(r)		(((Ref) (r)) != FALSEREF)

extern "C" int _RPTRError(Ref r), _RINTError(Ref r), _RCHARError(Ref r);

inline long		RINT(Ref r)		{ return ISINT(r) ? RVALUE(r) : _RINTError(r); }
inline UniChar	RCHAR(Ref r)	{ return ISCHAR(r) ? (UniChar) RIMMEDVALUE(r) : _RCHARError(r); }


/*------------------------------------------------------------------------------
	R e f V a r

	C++ stuff to keep Ref usage simple.
------------------------------------------------------------------------------*/

extern "C"
{
extern	RefHandle *		AllocateRefHandle(Ref targetObj);
extern	void				DisposeRefHandle(RefHandle * Handle);
}

class RefVar
{
public:
				RefVar();
				RefVar(Ref r);
				RefVar(const RefVar & o);
				~RefVar();

	RefVar &	operator=(Ref r);
	RefVar &	operator=(const RefVar & o);

	operator	long() const;

	RefHandle *	h;
};

inline	RefVar::RefVar()
{ h = AllocateRefHandle(NILREF); }

inline	RefVar::RefVar(Ref r)
{ h = AllocateRefHandle(r); }

inline	RefVar::RefVar(const RefVar & o)
{ h = AllocateRefHandle(o.h->ref); }

inline	RefVar::~RefVar()
{ DisposeRefHandle(h); }

inline	RefVar &	RefVar::operator=(Ref r)
{ h->ref = r; return *this; }

inline	RefVar &	RefVar::operator=(const RefVar & o)
{ h->ref = o.h->ref; return *this; }

inline				RefVar::operator	long() const
{ return h->ref; }

//______________________________________________________________________________

class RefStruct : public RefVar
{
public:
				RefStruct();
				RefStruct(const Ref r);
				RefStruct(const RefVar & o);
				RefStruct(const RefStruct & o);
				~RefStruct();

	RefStruct &	operator=(const Ref r);
	RefStruct &	operator=(const RefVar & o);
	RefStruct &	operator=(const RefStruct & o);
};

inline	RefStruct::RefStruct() : RefVar()
{ h->stackPos = 0; }

inline	RefStruct::RefStruct(const Ref r) : RefVar(r)
{ h->stackPos = 0; }

inline	RefStruct::RefStruct(const RefVar & o) : RefVar(o)
{ h->stackPos = 0; }

inline	RefStruct::RefStruct(const RefStruct & o) : RefVar(o)
{ h->stackPos = 0; }

inline	RefStruct::~RefStruct()
{ }

inline	RefStruct & RefStruct::operator=(const Ref r)
{ h->ref = r; return *this; }

inline	RefStruct & RefStruct::operator=(const RefVar & o)
{ h->ref = o.h->ref; return *this; }

inline	RefStruct & RefStruct::operator=(const RefStruct & o)
{ return operator=((const RefVar &) o); }

//______________________________________________________________________________

class CObjectPtr : public RefVar
{
public:
					CObjectPtr();
					CObjectPtr(Ref);
					CObjectPtr(RefArg);
					CObjectPtr(const RefStruct &);
					~CObjectPtr();

	CObjectPtr &	operator=(Ref);
	CObjectPtr &	operator=(const CObjectPtr &);

	operator char*() const;
};

//______________________________________________________________________________

class CDataPtr : public CObjectPtr
{
public:
					CDataPtr() : CObjectPtr() {}
					CDataPtr(Ref r) : CObjectPtr(r) {}

	CDataPtr &	operator=(Ref);
	CDataPtr &	operator=(const CDataPtr &);

	operator char*() const;
};

//______________________________________________________________________________
// Macros as Functions

extern	Ref 		MakeInt(long i);
extern	Ref	 	MakeChar(unsigned char c);
extern	Ref		MakeBoolean(int val);

extern	Boolean	IsInt(Ref r);	
extern	Boolean	IsChar(Ref r);	
extern	Boolean	IsPtr(Ref r);		
extern	Boolean	IsMagicPtr(Ref r);    
extern	Boolean	IsRealPtr(Ref r);   

extern	Ref		AddressToRef(void *);
extern	void *	RefToAddress(Ref r);
extern	long		RefToInt(Ref r);			
extern	UniChar	RefToUniChar(Ref r);		

//______________________________________________________________________________
// Object Class Functions

extern	Ref		ClassOf(Ref r);
extern	Boolean	IsArray(Ref r);
extern	Boolean	IsBinary(Ref r);
extern	Boolean	IsFrame(Ref r);
extern	Boolean	IsFunction(Ref r);								
extern	Boolean	IsNativeFunction(Ref r);								
extern	Boolean	IsNumber(Ref r);
extern	Boolean	IsReadOnly(Ref r);
extern	Boolean	IsReal(Ref r);
extern	Boolean	IsString(Ref r);
extern	Boolean	IsSymbol(Ref r);
extern	Boolean	IsInstance(Ref obj, Ref super);
extern	Boolean	IsSubclass(Ref sub, Ref super);
extern	void		SetClass(RefArg obj, RefArg theClass);

//______________________________________________________________________________
// General Object Functions

extern	Ref		AllocateBinary(RefArg theClass, long length);
extern	void		BinaryMunger(RefArg a1, long a1start, long a1count,
										 RefArg a2, long a2start, long a2count);
extern	Boolean	EQRef(Ref a, Ref b);
inline	Boolean	EQ(RefArg a, RefArg b) { return EQRef(a, b); }
extern	Ptr		SetupListEQ(Ref obj);
extern	Boolean	ListEQ(Ref a, Ref b, Ptr bPtr);
	// Make all references to target refer to replacement instead
extern	void		ReplaceObject(RefArg target, RefArg replacement);
	// Shallow clone of obj
extern	Ref		Clone(RefArg obj);
	// Deep clone of obj
extern	Ref		DeepClone(RefArg obj);
	// Really deep clone of obj (including maps and ensuring symbols are in RAM)
extern	Ref		TotalClone(RefArg obj);
	// Don't clone except as necessary to ensure maps and symbols are in RAM
extern	Ref		EnsureInternal(RefArg obj);

//______________________________________________________________________________
// Array Functions

extern	Ref		AllocateArray(RefArg theClass, long length);
extern	void		ArrayMunger(RefArg a1, long a1start, long a1count,
										RefArg a2, long a2start, long a2count);
extern	void		AddArraySlot(RefArg obj, RefArg element);
extern	long		ArrayPosition(RefArg array, RefArg element, long start, RefArg test);
extern	Boolean	ArrayRemove(RefArg array, RefArg element);
extern	void 		ArrayRemoveCount(RefArg array, long start, long removeCount);
extern	Ref		GetArraySlot(RefArg array, long slot);
extern	void		SetArraySlot(RefArg array, long slot, RefArg value);

// Sorts an array
// test = '|<|, '|>|, '|str<|, '|str>|, or any function object returning -1,0,1 (as strcmp)
// key = NILREF (use the element directly), or a path, or any function object
extern	void		SortArray(RefArg array, RefArg test, RefArg key);		

extern	Ref		FSetContains(RefArg, RefArg array, RefArg target);
extern	Ref		FSetAdd(RefArg, RefArg members, RefArg member, RefArg unique);
extern	Ref		FSetRemove(RefArg, RefArg members, RefArg member);
extern	Ref		FSetOverlaps(RefArg, RefArg array, RefArg targetArray);
extern	Ref		FSetUnion(RefArg, RefArg array1, RefArg array2, RefArg unique);
extern	Ref		FSetDifference(RefArg, RefArg array1, RefArg array2);

//______________________________________________________________________________
// Frame & Slot Functions

extern	Ref		AllocateFrame(void);
extern	Boolean	FrameHasPath(Ref obj, Ref thePath);
extern	Boolean	FrameHasSlot(Ref obj, Ref slot);
extern	Ref		GetFramePath(Ref obj, Ref thePath);
extern	Ref		GetFrameSlot(Ref obj, Ref slot);
extern	long		Length(Ref obj);		// Length in bytes or slots
	// MapSlots calls a function on each slot of an array or frame object, giving it
	// the tag (integer or symbol) and contents of each slot.  "Anything" is passed to
	// func.  If func returns anything but NILREF, MapSlots terminates.
extern	void		MapSlots(RefArg obj, MapSlotsFunction func, unsigned anything);
extern	void		RemoveSlot(RefArg frame, RefArg tag);
extern	void		SetFramePath(RefArg obj, RefArg thePath, RefArg value);
extern	void		SetFrameSlot(RefArg obj, RefArg slot, RefArg value);
extern	void		SetLength(RefArg obj, long length);

//______________________________________________________________________________
// Symbol Functions

#define SYM(name) MakeSymbol(#name)
extern	Ref		MakeSymbol(const char * name);	// Create or return a symbol
extern	char *	SymbolName(Ref sym);					// Return a symbolÕs name
extern	ULong		SymbolHash(Ref sym);					// Return a symbol's hash value
extern	int		SymbolCompareLexRef(Ref sym1, Ref sym2);
extern	int		SymbolCompareLex(RefArg sym1, RefArg sym2);
extern	int		symcmp(char * s1, char * s2);		// Case-insensitive comparison

//______________________________________________________________________________
// String Manipulation Functions

extern	Ref		ASCIIString(RefArg str);
extern	Ref		MakeStringFromASCIIString(const char * str);
extern	Ref		MakeString(const UniChar * str);
extern	int		StrBeginsWith(RefArg str, RefArg prefix);
extern	int		StrEndsWith(RefArg str, RefArg suffix);
extern	void		StrCapitalize(RefArg str);
extern	void		StrCapitalizeWords(RefArg str);
extern	void		StrUpcase(RefArg str);
extern	void		StrDowncase(RefArg str);
extern	Boolean	StrEmpty(RefArg str);
extern	void		StrMunger(RefArg s1, long s1start, long s1count,
									 RefArg s2, long s2start, long s2count);
extern	long		StrPosition(RefArg str, RefArg substr, long startPos);
extern	long		StrReplace(RefArg str, RefArg substr, RefArg replacement, long count);
extern	Ref		Substring(RefArg str, long start, long count);
extern	void		TrimString(RefArg str);

//______________________________________________________________________________
// Numeric Conversion Functions

extern	long		CoerceToInt(Ref r);
extern	double	CoerceToDouble(Ref r);
extern	double	CDouble(Ref r);

extern	Ref		MakeReal(double d);

//______________________________________________________________________________
// Exception Handling Functions

extern	void		_OSErr(NewtonErr err);

DeclareException(exFrames, exRootException);
DeclareException(exFramesData, exRootException);
DeclareException(exStore, exRootException);
DeclareException(exGraf, exRootException);
DeclareException(exInterpreter, exRootException);
DeclareException(exInterpreterData, exRootException);
DeclareException(exOutOfMemory, exRootException);	// evt.ex.outofmem
DeclareBaseException(exRefException);					// type.ref, data is a RefStruct*

extern	void		ThrowRefException(ExceptionName name, RefArg data);
extern	void		ThrowOutOfBoundsException(RefArg obj, long index);
extern	void		ThrowBadTypeWithFrameData(NewtonErr errorCode, RefArg value);
extern	void		ThrowExFramesWithBadValue(NewtonErr errorCode, RefArg value);
extern	void		ThrowExCompilerWithBadValue(NewtonErr errorCode, RefArg value);
extern	void		ThrowExInterpreterWithSymbol(NewtonErr errorCode, RefArg value);

extern	void		ExceptionNotify(Exception * inException);

inline void OutOfMemory(void)
{ Throw(exOutOfMemory, (void *) -10007, (ExceptionDestructor) 0); }

//______________________________________________________________________________
// Object Accessors

extern	unsigned	ObjectFlags(Ref r);

// Direct access (must lock/unlock before using pointers)
extern	void		LockRef(Ref r);
extern	void		UnlockRef(Ref r);
extern	Ptr		BinaryData(Ref r);
extern	Ref *		Slots(Ref r);

// DON'T USE THESE DIRECTLY!!!!
// MUST USE WITH MACROS WITH_LOCKED_BINARY AND END_WITH_LOCKED_BINARY - SEE BELOW
extern	Ptr		LockedBinaryPtr(RefArg obj);
extern	void		UnlockRefArg(RefArg obj);

//______________________________________________________________________________
// Garbage Collection Functions

extern	void		GC();

//______________________________________________________________________________
// NewtonScript

extern "C" {
Ref	DoBlock(RefArg codeBlock, RefArg args);
Ref	DoMessage(RefArg rcvr, RefArg msg, RefArg args);
Ref	DoMessageIfDefined(RefArg rcvr, RefArg msg, RefArg args, Boolean * isDefined);
long	GetFunctionArgCount(Ref fn);
}

//______________________________________________________________________________
// Iterators

class CObjectIterator
{
public:
				CObjectIterator(RefArg inObj, Boolean includeSiblings = false);
				~CObjectIterator();

	void		reset(void);
	void		resetWithObject(RefArg inObj);
	int		next(void);
	Boolean	done(void);
	Ref		tag(void);
	Ref		value(void);

private:
	RefStruct	fTag;
	RefStruct	fValue;
	RefStruct	fObj;
	Boolean		fIncludeSiblings;
	long			fIndex;
	long			fLength;
	RefStruct	fMapRef;	// NILREF indicates an Array iterator
	ExceptionCleanup	x;
};

inline Ref	CObjectIterator::tag(void)
{ return fTag; }

inline Ref  CObjectIterator::value(void)
{ return fValue; }


#define FOREACH(obj, value_var) \
	{ \
		CObjectIterator * _iter = new CObjectIterator(obj); \
		if (!_iter) OutOfMemory(); \
		RefVar value_var; \
		unwind_protect { \
			while (!_iter->done()) { \
				value_var = _iter->value();

#define FOREACH_WITH_TAG(obj, tag_var, value_var) \
	{ \
		CObjectIterator * _iter = new CObjectIterator(obj); \
		if (!_iter) OutOfMemory(); \
		RefVar tag_var; \
		RefVar value_var; \
		unwind_protect { \
			while (!_iter->done()) { \
				tag_var = _iter->tag(); \
				value_var = _iter->value();

#define END_FOREACH \
				_iter->next(); \
			} \
		} \
		on_unwind { \
			delete _iter; \
		} \
		end_unwind; \
	}

/* This is used like

	RefVar obj;
	...
	FOREACH(obj, value)
		...
		DoSomething(value);
		...
	END_FOREACH
	...

or 

	RefVar obj;
	...
	FOREACH_WITH_TAG(obj, tag, value)
		...
	if (tag == kSomething)
			DoSomething(value);
		...
	END_FOREACH
	...
*/ 


#define WITH_LOCKED_BINARY(obj, ptr_var) \
	unwind_protect { \
		void * ptr_var = LockedBinaryPtr(obj);

#define END_WITH_LOCKED_BINARY(obj) \
	} \
	on_unwind { \
		UnlockRefArg(obj); \
	} \
	end_unwind;


#endif	/* __OBJECTS_H */
