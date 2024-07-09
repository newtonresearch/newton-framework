/*
	File:		Objects.cc

	Contains:	Basic Newton object functions.

	Written by:	Newton Research Group.
*/

#include <ctype.h>	// tolower

#include "Objects.h"
#include "ObjHeader.h"
#include "LargeBinaries.h"
#include "Globals.h"
#include "RefMemory.h"
#include "Symbols.h"
#include "Lookup.h"
#include "OSErrors.h"
#include "ROMResources.h"

extern bool	IsFaultBlock(Ref r);
extern bool	IsPackageHeader(Ptr inData, size_t inSize);


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	Fref(RefArg inRcvr, RefArg inObj);
Ref	FrefOf(RefArg inRcvr, RefArg inObj);
Ref	FSetClass(RefArg inRcvr, RefArg inObj, RefArg inClass);
Ref	FClassOf(RefArg inRcvr, RefArg inObj);
Ref	FPrimClassOf(RefArg inRcvr, RefArg inObj);
Ref	FIsSubclass(RefArg inRcvr, RefArg inObj, RefArg inSuper);
Ref	FIsArray(RefArg inRcvr, RefArg inObj);
Ref	FIsBinary(RefArg inRcvr, RefArg inObj);
Ref	FIsCharacter(RefArg inRcvr, RefArg inObj);
Ref	FIsDirty(RefArg inRcvr, RefArg inObj);
Ref	FIsFrame(RefArg inRcvr, RefArg inObj);
Ref	FIsFunction(RefArg inRcvr, RefArg inObj);
Ref	FIsNativeFunction(RefArg inRcvr, RefArg inObj);
Ref	FIsImmediate(RefArg inRcvr, RefArg inObj);
Ref	FIsInstance(RefArg inRcvr, RefArg inObj, RefArg inOfObj);
Ref	FIsInteger(RefArg inRcvr, RefArg inObj);
Ref	FIsMagicPtr(RefArg inRcvr, RefArg inObj);
Ref	FIsNumber(RefArg inRcvr, RefArg inObj);
Ref	FIsPackage(RefArg inRcvr, RefArg inObj);
Ref	FIsReadOnly(RefArg inRcvr, RefArg inObj);
Ref	FIsReal(RefArg inRcvr, RefArg inObj);
Ref	FIsString(RefArg inRcvr, RefArg inObj);
Ref	FIsSymbol(RefArg inRcvr, RefArg inObj);
Ref	FIsVBO(RefArg inRcvr, RefArg inObj);
}


/*------------------------------------------------------------------------------
	C   - >   N S   C o n s t r u c t o r s
------------------------------------------------------------------------------*/

Ref	MakeInt(long i) { return MAKEINT(i); }
Ref	MakeChar(unsigned char c) { return MAKECHAR(c); }
Ref	MakeBoolean(int b) { return MAKEBOOLEAN(b); }
Ref	MakeReal(double d) { Ref r = AllocateBinary(SYMA(real), sizeof(double)); memmove(BinaryData(r), &d, sizeof(double)); return r; }
Ref	MakeImmediate(unsigned type, unsigned value) { return MAKEIMMED(type, value); }
Ref	MakeUniChar(UniChar c) { return MakeImmediate(kImmedChar, c); }

#pragma mark -

/*------------------------------------------------------------------------------
	P r e d i c a t e s
------------------------------------------------------------------------------*/

bool	IsInt(Ref r) { return ISINT(r); }
bool	IsChar(Ref r) { return ISCHAR(r); }
bool	IsPtr(Ref r) { return ISPTR(r); }
bool	IsMagicPtr(Ref r) { return RTAG(r) == kTagMagicPtr; }
bool	IsRealPtr(Ref r) { return ISREALPTR(r); }

#pragma mark -

/*------------------------------------------------------------------------------
	N S   - >   C   C o n s t r u c t o r s
------------------------------------------------------------------------------*/

int	_RPTRError(Ref r) { ThrowBadTypeWithFrameData(kNSErrNotAPointer, r); return 0; }
int	_RINTError(Ref r) { ThrowBadTypeWithFrameData(kNSErrNotAnInteger, r); return 0; }
int	_RCHARError(Ref r) { ThrowBadTypeWithFrameData(kNSErrNotACharacter, r); return 0; }

Ref		AddressToRef(void * inAddr) { return (Ref) inAddr & kRefValueMask; }
void *	RefToAddress(Ref r) { return (void *)(ISINT(r) ? r & kRefValueMask : _RINTError(r)); }
int		RefToInt(Ref r) { return RINT(r); }
UniChar	RefToUniChar(Ref r) { return RCHAR(r); }

#pragma mark -

/*------------------------------------------------------------------------------
	O b j e c t   C l a s s   F u n c t i o n s
------------------------------------------------------------------------------*/

void
SetClass(RefArg obj, RefArg theClass)
{
	ObjHeader *	oPtr = ObjectPtr(obj);
	int			flags = oPtr->flags;

	if (flags & kObjReadOnly)
		ThrowExFramesWithBadValue(kNSErrObjectReadOnly, obj);

	if (flags & kObjSlotted)
	{
		if (flags & kObjFrame)
			SetFrameSlot(obj, SYMA(class), theClass);
		else
			((ArrayObject *)oPtr)->objClass = theClass;
	}
	else
	{
		((BinaryObject *)oPtr)->objClass = theClass;
		if (flags & kIndirectBinaryObject)
		{
			IndirectBinaryObject * vbo = (IndirectBinaryObject *)oPtr;
			vbo->procs->SetClass(vbo->data, theClass);
		}
	}
	DirtyObject(obj);
}


Ref
ClassOf(Ref r)
{
	int tag = RTAG(r);

	if (tag == kTagInteger)
		return SYMA(int);

	if (tag == kTagImmed)
	{
		if (RIMMEDTAG(r) == kImmedChar)
			return SYMA(char);
		else if (RIMMEDTAG(r) == kImmedBoolean)
			return SYMA(boolean);
		return SYMA(weird_immediate);
	}

	if (tag & kTagPointer)
	{
		unsigned flags = ObjectFlags(r);
		if (FLAGTEST(flags, kObjSlotted))
		{
			if (FLAGTEST(flags, kObjFrame))
			{
				Ref theClass = GetProtoVariable(r, SYMA(class));
				if (NOTNIL(theClass))
				{
					if (theClass == kPlainFuncClass)
						return SYMA(_function);
					else if (theClass == kPlainCFunctionClass
							|| theClass == kBinCFunctionClass)
						return SYMA(_function_2Enative);
					return theClass;
				}
				return SYMA(frame);
			}
			return ((ArrayObject *)ObjectPtr(r))->objClass;
		}
		else if (((BinaryObject *)ObjectPtr(r))->objClass == kWeakArrayClass)
			return SYMA(_weakarray);
		else if (IsSymbol(r))
			return SYMA(symbol);
		return ((BinaryObject *)ObjectPtr(r))->objClass;
	}

	return NILREF;	//	should never happen
}


bool
IsInstance(Ref obj, Ref ofClass)
{
	Ref	objClass = ClassOf(obj);
	return NOTNIL(objClass)
	&& IsSubclass(objClass, ofClass);
}


bool
IsBinary(Ref r)
{
	return ISPTR(r)
	&& !IsFaultBlock(r)
	&& !(ObjectFlags(r) & kObjSlotted);
}


bool
IsArray(Ref r)
{
	return ISPTR(r)
	&& !IsFaultBlock(r)
	&& (ObjectFlags(r) & kObjMask) == kArrayObject;
}


bool
IsFrame(Ref r)
{
	return ISPTR(r)
	&& !IsFaultBlock(r)
	&& (ObjectFlags(r) & kObjMask) == kFrameObject;
}


bool
IsFunction(Ref r)
{
	bool isFunction = false;

	if (ISPTR(r)
	&& ObjectFlags(r) & kObjSlotted
	&& Length(r) != 0)
	{
		Ref theClass = GetArraySlot(r, 0);
		if (theClass == kPlainFuncClass
		 || theClass == kPlainCFunctionClass
		 || theClass == kBinCFunctionClass
		 || EQ(theClass, SYMA(CodeBlock))
		 || EQ(theClass, SYMA(binCFunction)))
			isFunction = true;
	}
	return isFunction;	
}


bool
IsNativeFunction(Ref r)
{
	Ref fnClass = ClassOf(r);
	return EQ(fnClass, SYMA(_function_2Enative))
		 || EQ(fnClass, SYMA(binCFunction));
}


bool
IsNumber(Ref r)
{
	return ISINT(r)
	|| IsReal(r);
}


bool
IsPackage(RefArg r)
{
	return IsLargeBinary(r)
	&& EQ(ClassOf(r), SYMA(package))
	&& IsPackageHeader(BinaryData(r), Length(r))
/*	&& IsOnStoreAsPackage(BinaryData(r)) */ ;
}


bool
IsReadOnly(Ref r)
{
	return ObjectFlags(r) & kObjReadOnly;
}


bool
IsReal(Ref r)		// called ISREAL in Newton
{
	return ISPTR(r)
	&& !IsFaultBlock(r)
	&& !(ObjectFlags(r) & kObjSlotted)
	&& EQ(ClassOf(r), SYMA(real));
}


bool
IsString(Ref r)
{
	return ISPTR(r)
	&& !IsFaultBlock(r)
	&& !(ObjectFlags(r) & kObjSlotted)
	&& IsInstance(r, SYMA(string));
}


bool
IsSymbol(Ref r)
{
	return ISPTR(r)
	&& ((SymbolObject *)NoFaultObjectPtr(r))->objClass == kSymbolClass;
}



/*------------------------------------------------------------------------------
	S t r i n g   C l a s s e s
------------------------------------------------------------------------------*/

Ref	gInheritanceFrame;			// 0C105184
/* {
	address: 'string,
	beeperPhone: 'phone,
	carPhone: 'phone,
	company: 'string,
	faxPhone: 'phone;
	homeFaxPhone: 'phone,
	homePhone: 'phone,
	mobilePhone: 'phone,
	name: 'string,
	otherPhone: 'phone,
	phone: 'string,
	title: 'string,
	workPhone: 'phone
} */


void
InitClasses(void)
{
	gInheritanceFrame = AllocateFrame();
	AddGCRoot(&gInheritanceFrame);
}


void
RegisterClass(const char * name, const char * superName)
{
	RefVar	nameRef(MakeSymbol(name));
	RefVar	superNameRef(MakeSymbol(superName));
	SetFrameSlot(gInheritanceFrame, nameRef, superNameRef);
}


bool
IsSubclass(Ref obj, Ref super)
{
	if (!IsSymbol(obj) || !IsSymbol(super))
		return EQRef(obj, super);

	if (EQRef(obj, super))
		return true;

	const char * superName, * subName;
	const char * dot;

	superName = SymbolName(super);
	if (*superName == 0)
		return false;
	subName = SymbolName(obj);
	dot = strchr(subName, '.');
	if (dot)
	{
		for ( ; *subName && *superName; subName++, superName++)
		{
			if (tolower(*subName) != tolower(*superName))
				return false;
		}
		return (*superName == 0 && (*subName == 0 || *subName == '.'));
	}

	while (!EQRef(obj, super))
	{
		if (ISNIL(obj = GetFrameSlot(gInheritanceFrame, obj)))
			return false;
	}

	return true;
}

#pragma mark -

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

Ref	Fref(RefArg inRcvr, RefArg inObj) { return RVALUE(inObj); }
Ref	FrefOf(RefArg inRcvr, RefArg inObj) { return MAKEINT(inObj); }
Ref	FSetClass(RefArg inRcvr, RefArg inObj, RefArg inClass) { SetClass(inObj, inClass); return inObj; }
Ref	FClassOf(RefArg inRcvr, RefArg inObj) { return ClassOf(inObj); }

Ref
FPrimClassOf(RefArg inRcvr, RefArg inObj)
{
	if (ISPTR(inObj))
	{
		unsigned flags = ObjectFlags(inObj);
		if ((flags & kObjSlotted) == 0)
			return SYMA(binary);
		else if ((flags & kObjFrame) == 0)
			return SYMA(array);
		else
			return SYMA(frame);
	}
	return SYMA(immediate);
}

Ref	FIsSubclass(RefArg inRcvr, RefArg inObj, RefArg inSuper) { return MAKEBOOLEAN(IsSubclass(inObj, inSuper)); }
Ref	FIsArray(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(IsArray(inObj)); }
Ref	FIsBinary(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(ISPTR(inObj) && !(ObjectFlags(inObj) & kObjSlotted)); }
Ref	FIsCharacter(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(ISCHAR(inObj)); }
Ref	FIsDirty(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(ObjectFlags(inObj) & kObjDirty); }
Ref	FIsFrame(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(IsFrame(inObj)); }
Ref	FIsFunction(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(IsFunction(inObj)); }
Ref	FIsNativeFunction(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(IsNativeFunction(inObj)); }
Ref	FIsImmediate(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(!ISPTR(inObj)); }
Ref	FIsInstance(RefArg inRcvr, RefArg inObj, RefArg inOfObj) { return MAKEBOOLEAN(IsInstance(inObj, inOfObj)); }
Ref	FIsInteger(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(ISINT(inObj)); }
Ref	FIsMagicPtr(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(ISMAGICPTR(inObj)); }
Ref	FIsNumber(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(IsNumber(inObj)); }
Ref	FIsPackage(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(IsPackage(inObj)); }
Ref	FIsReadOnly(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(ObjectFlags(inObj) & kObjReadOnly);}
Ref	FIsReal(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(IsReal(inObj)); }
Ref	FIsString(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(IsString(inObj)); }
Ref	FIsSymbol(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(IsSymbol(inObj)); }
Ref	FIsVBO(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(IsLargeBinary(inObj)); }	// originally FIsLargeBinary
/*
Ref	IsRichString
Ref	IsValidString
*/


#pragma mark -
/*------------------------------------------------------------------------------
	N u m e r i c   C o n v e r s i o n   F u n c t i o n s
------------------------------------------------------------------------------*/

int
CoerceToInt(Ref r)
{
	if (ISINT(r))
		return RVALUE(r);

	if (IsReal(r))
		return (int) *(double *)BinaryData(r);

	ThrowBadTypeWithFrameData(kNSErrNotANumber, r);
	return 0;
}


double
CoerceToDouble(Ref r)
{
	if (ISINT(r))
		return (double) RVALUE(r);

	if (IsReal(r))
		return *(double *)BinaryData(r);

	ThrowBadTypeWithFrameData(kNSErrNotANumber, r);
	return 0.0;
}


double
CDouble(Ref r)
{
	if (EQ(ClassOf(r), SYMA(real)))
		return *(double *)BinaryData(r);

	ThrowBadTypeWithFrameData(kNSErrNotANumber, r);
	return 0.0;
}


#pragma mark -
/*------------------------------------------------------------------------------
	O b j e c t   C o n v e r s i o n   F u n c t i o n s
------------------------------------------------------------------------------*/

Ref
ToObject(RefArg inClass, const char * inData, size_t inSize)
{
	RefVar theObject(AllocateBinary(inClass, inSize));
	CDataPtr objData(theObject);
	memcpy((char *)objData, inData, inSize);
	return theObject;
}


bool
FromObject(RefArg inObj, char * outData, size_t& outSize, size_t inBufferSize)
{
	outSize = Length(inObj);
	CDataPtr objData(inObj);
	memcpy(outData, (char *)objData, MIN(inBufferSize, outSize));
	return true;
}

