/*
	File:		StreamObjects.h

	Contains:	Private interface to the Newton Streamed Object Format (NSOF)
					classes.

	Written by:	Newton Research Group.
*/

#if !defined(__STREAMOBJECTS_H)
#define __STREAMOBJECTS_H 1

#include "PrecedentsForIO.h"
#include "Pipes.h"
#include "RefStack.h"
#include "Store.h"


/*------------------------------------------------------------------------------
	Current Newton Streamed Object Format version.
------------------------------------------------------------------------------*/

#define kNSOFVersion 2


/*------------------------------------------------------------------------------
	T y p e s
------------------------------------------------------------------------------*/

typedef enum
{
	kNSImmediate,				//	Indeterminate Immediate type
	kNSCharacter,				//	ASCII Character
	kNSUnicodeCharacter,		//	Unicode Character
	kNSBinaryObject,			//	Small Binary Object ( < 32K )
	kNSArray,					//	Named Array
  	kNSPlainArray,				//	Anonymous Array
  	kNSFrame,					//	Frame
  	kNSSymbol,					//	Object Symbol
  	kNSString,					//	String (null-terminated Unicode)
  	kNSPrecedent,				//	Repeated Item
  	kNSNIL,						//	nil object
  	kNSSmallRect,				//	Small rect
	kNSLargeBinary,			//	Large Binary Object
  	kNSBoolean,					//	Boolean
  	kNSInteger					//	Integer (4 byte)
} NSObjectType;


/*------------------------------------------------------------------------------
	C O b j e c t R e a d e r

	The reader that constructs Newton objects from streamed data.
------------------------------------------------------------------------------*/

class CObjectReader
{
public:
					CObjectReader(CPipe & inPipe);
					CObjectReader(CPipe & inPipe, RefArg inOptions);
					~CObjectReader();

	void			setPrecedentsForReading(void);
	void			setFunctionsAllowed(bool inAllowed);
	Ref			read(void);
	ArrayIndex	size(void);								// NRG

private:
	ArrayIndex	scanSize(void);						// NRG
	Ref			scan(void);
	Ref			readLargeBinary(void);
	ArrayIndex	countLargeBinary(ArrayIndex * outUnscannedSize);
	int			longFromPipe(void);
	ArrayIndex	sizeFromPipe(int * outValue);	// NRG

	CPrecedentsForReading *	fPrecedents;			// +00
	CPipe &		fPipe;					// +04
	CStore *		fStore;					// +08
	bool			fAllowFunctions;		// +0C
};


/*------------------------------------------------------------------------------
	O b j e c t W r i t e r

	The writer that streams Newton objects to a communications pipe.
------------------------------------------------------------------------------*/

class CObjectWriter
{
public:
					CObjectWriter(RefArg inObj, CPipe & inPipe, bool inFollowProtos);
					~CObjectWriter();

	void			setCompressLargeBinaries(void);
	ArrayIndex	size(void);
	void			write(void);

private:
	void			prescan(void);
	ArrayIndex	prescan(RefArg inObj);
	void			countLargeBinary(void);
	ArrayIndex	longToSize(long inValue) const;
	void			scan(void);
	void			scan(RefArg inObj);
	void			writeLargeBinary(void);
	void			longToPipe(int inValue);

	CPrecedentsForWriting *	fPrecedents;	// +00
	RefStack		fStack;				// +04
	RefStruct	fObj;					// +14
	CPipe &		fPipe;				// +18
	ArrayIndex	fObjSize;			// +1C
	bool			fFollowProtos;		// +20
	bool			fCompressLB;		// +24
};

inline ArrayIndex	CObjectWriter::longToSize(long inValue) const
{ return (inValue < 0 || inValue > 254) ? 1 + 4 : 1; }


#endif	/* __STREAMOBJECTS */
