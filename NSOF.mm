
#import <Foundation/Foundation.h>

#include "Objects.h"
#include "ObjHeader.h"
#include "Iterators.h"
#include "Arrays.h"
#include "Frames.h"
#include "Funcs.h"
#include "Globals.h"
#include "Unicode.h"

#import "NSOF.h"

extern Ref MakeStringOfLength(const UniChar * str, ULong numChars);


/*------------------------------------------------------------------------------
	D e b u g g i n g
------------------------------------------------------------------------------*/

extern "C" long	GetFunctionArgCount(Ref fn);

static int	indentIndex = 0;
static char	indent[] = "                    ";

static void
DBindent(int incr)
{
	int	newIndex = indentIndex + incr;
	if (newIndex >= 0 && newIndex < 20)
	{
		indent[indentIndex] = 0x20;
		indentIndex = newIndex;
		indent[indentIndex] = 0x00;
	}
}

static void
DBprint(Ref obj)
{
	if (IsMagicPtr(obj))
		fprintf(stdout, "@%d", RVALUE(obj));

	else if (IsFunction(obj))
	{
		int n = GetFunctionArgCount(obj);
		fprintf(stdout, "func(#%d)", n);
	}

	else if (IsFrame(obj))
		fprintf(stdout, "{#%d}", Length(obj)/sizeof(Ref));

	else if (IsArray(obj))
		fprintf(stdout, "[#%d]", Length(obj)/sizeof(Ref));

	else if (IsString(obj))
	{
		char * s = (char *) malloc(Length(obj)/sizeof(UniChar));
		ConvertFromUnicode((UniChar *) BinaryData(obj), s, kDefaultEncoding, INT_MAX);
		fprintf(stdout, "\"%s\"", s);
		free(s);
	}

	else if (IsSymbol(obj))
		fprintf(stdout, "\'%s", SymbolName(obj));

	else if (IsInt(obj))
		fprintf(stdout, "%d", RVALUE(obj));

	else if (IsNumber(obj))
		fprintf(stdout, "%f", CDouble(obj));

	else if (ISIMMED(obj))
	{
		if (obj == NILREF)
			fprintf(stdout, "nil");
		else if (obj == TRUEREF)
			fprintf(stdout, "true");
		else if (IsChar(obj))
			fprintf(stdout, "$\\u%04X", RVALUE(obj));
		else
			fprintf(stdout, "weird_immediate");
	}

	else if (IsBinary(obj))
	{
		Ref r = ClassOf(obj);
		char * clas = IsSymbol(r) ? SymbolName(r) : "binary?";
		fprintf(stdout, "<%s, length #%d >", clas, Length(obj));
	}

	else
		fprintf(stdout, "something?");
}

#define TRACE(_s) fprintf(stdout, _s)
#define TRACE1(_s,_a) fprintf(stdout, _s, _a)
#define INDENT() DBindent(2)
#define OUTDENT() DBindent(-2)
#define PRINTOBJECT(r) DBprint(r);  fprintf(stdout, "\n")


/*------------------------------------------------------------------------------
	T y p e s
------------------------------------------------------------------------------*/

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


@implementation FDObjectReader

- (void) dealloc
{
	if (precedents)
		delete precedents;
	[super dealloc];
}


/*------------------------------------------------------------------------------
	Read a Newton Streamed Object File.
------------------------------------------------------------------------------*/

- (Ref) read: (NSString *) path
{
	pipe = [NSFileHandle fileHandleForReadingAtPath: path];
	if (pipe == nil)
		return NILREF;
	// Stream starts with version number
	if ([self readbyte] != kNSOFVersion)
	{
	//	SetError(-98401);
		return NILREF;
	}
	TRACE("NSOF version\n");
	precedents = new CPrecedents();
	return [self readobject];
}


/*------------------------------------------------------------------------------
	Read an object from the pipe.
------------------------------------------------------------------------------*/

- (Ref) readobject
{
	RefVar			obj = AllocRefVar(NILREF);
	FDObjectType	item;
	SInt32			i, count;

	INDENT();
	TRACE(indent);
	// if (GetError() != noErr) obj = NILREF; else
	item = (FDObjectType) [self readbyte];
	switch (item)
	{

	case kFDImmediate:
		obj->ref = [self readxlong];
		PRINTOBJECT(obj->ref);
		break;

	case kFDCharacter:
		obj->ref = MAKECHAR([self readbyte]);
		PRINTOBJECT(obj->ref);
		break;

	case kFDUnicodeCharacter:
		obj->ref = MAKECHAR([self readhalfword]);
		PRINTOBJECT(obj->ref);
		break;

	case kFDBinaryObject:
		{
			RefVar	objClass;
			BinaryObject * objPtr;

			count = [self readxlong];
			TRACE("binary\n");
			objClass = AllocRefVar([self readobject]);
			obj->ref = AllocateBinary(objClass, count);
			FreeRefVar(objClass);
			precedents->add(obj);
			objPtr = (BinaryObject *) PTR(obj->ref);
			data = [pipe readDataOfLength: count];
			memmove(objPtr->data, [data bytes], count);
		}
		break;

	case kFDArray:
	case kFDPlainArray:
		{
			RefVar	objClass;
			ArrayObject * objPtr;

			count = [self readxlong];
			TRACE("array\n");
			objClass = AllocRefVar((item == kFDArray) ? [self readobject] : SYMarray);
			obj->ref = AllocateArray(objClass, count);
			FreeRefVar(objClass);
			precedents->add(obj);
			objPtr = (ArrayObject *) PTR(obj->ref);
			for (i = 0; i < count; i++)
				objPtr->slot[i] = [self readobject];
		}
		break;

	case kFDFrame:
		{
			long		idx;
			RefVar	map;
			FrameMapObject * mapPtr;
			FrameObject * objPtr;

			count = [self readxlong];
			TRACE("frame\n");
			obj->ref = NILREF;
			idx = precedents->add(obj);
			map = AllocRefVar(AllocateMap(NILREFARG, count));
			mapPtr = (FrameMapObject *) PTR(map->ref);
			for (i = 0; i < count; i++)
				mapPtr->slot[i] = [self readobject];
			obj->ref = AllocateFrameWithMap(map);
			FreeRefVar(map);
			precedents->set(idx, obj);
			objPtr = (FrameObject *) PTR(obj->ref);
			for (i = 0; i < count; i++)
				objPtr->slot[i] = [self readobject];
		}
		break;

	case kFDSymbol:
		count = [self readxlong];
		data = [pipe readDataOfLength: count];
		memmove(buf, [data bytes], count);
		buf[count] = 0;
		obj->ref = MakeSymbol(buf);
		precedents->add(obj);
		PRINTOBJECT(obj->ref);
		break;

	case kFDString:
		count = [self readxlong];
		data = [pipe readDataOfLength: count];
		obj->ref = MakeStringOfLength((const UniChar *) [data bytes], count / sizeof(UniChar));
		precedents->add(obj);
		PRINTOBJECT(obj->ref);
		break;

	case kFDPrecedent:
		obj->ref = precedents->get([self readxlong]);
		PRINTOBJECT(obj->ref);
		break;

	case kFDNIL:
		obj->ref = NILREF;
		PRINTOBJECT(obj->ref);
		break;

	case kFDSmallRect:
		{
			Ref	r;
			TRACE("small rect ");
			obj->ref = AllocateFrame();
			r = MAKEINT([self readbyte]);
			SetFrameSlot(obj, RSYM(top),    RARG(r));
			r = MAKEINT([self readbyte]);
			SetFrameSlot(obj, RSYM(left),   RARG(r));
			r = MAKEINT([self readbyte]);
			SetFrameSlot(obj, RSYM(bottom), RARG(r));
			r = MAKEINT([self readbyte]);
			SetFrameSlot(obj, RSYM(right),  RARG(r));
		}
		PRINTOBJECT(obj->ref);
		break;

	case kFDLargeBinary:
		{
			UInt32 nameLen, parmLen;

			[self readobject];	// class
			[self readbyte];		// compressed?
			count = [self readlong];		// length of data
			nameLen = [self readlong];		// length of name
			parmLen = [self readlong];		// length or parameters
			[self readlong];					// reserved
			data = [pipe readDataOfLength: nameLen];	// compander name
			data = [pipe readDataOfLength: parmLen];	// compander parameters
			data = [pipe readDataOfLength: count];		// data
			TRACE("large binary\n");
			obj->ref = NILREF;
		}
		precedents->add(obj);
		break;

	default:
	//	SetError(-98402);
		TRACE("unrecognised type!\n");
		obj->ref = NILREF;
	}
	OUTDENT();
	return FreeRefVar(obj);
}


/*------------------------------------------------------------------------------
	Read a byte from the pipe.
------------------------------------------------------------------------------*/

- (UInt8) readbyte
{
	data = [pipe readDataOfLength: 1];
	TRACE1("0x%02X ", *(UInt8*)[data bytes]);
	return (*(UInt8*)[data bytes]);
}


/*------------------------------------------------------------------------------
	Read a halfword from the pipe.
------------------------------------------------------------------------------*/

- (UInt16) readhalfword
{
	data = [pipe readDataOfLength: 2];
	TRACE1("0x%04X ", *(UInt16*)[data bytes]);
	return (*(UInt16*)[data bytes]);
}


/*------------------------------------------------------------------------------
	Read a long from the pipe.
------------------------------------------------------------------------------*/

- (SInt32) readlong
{
	data = [pipe readDataOfLength: 4];
	TRACE1("0x%08X ", *(SInt32*)[data bytes]);
	return (*(SInt32*)[data bytes]);
}


/*------------------------------------------------------------------------------
	Read an xlong from the pipe.
------------------------------------------------------------------------------*/

- (SInt32) readxlong
{
	UInt8 x;
	x = [self readbyte];
	if (x != 0xFF)
		return (SInt32) x;
	return [self readlong];
}


@end
