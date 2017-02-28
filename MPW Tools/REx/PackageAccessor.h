/*
	File:		PackageAccessor.h

	Contains:	Class to access parts of a package.

	Written by:	The Newton Tools group.
*/

#ifndef __PACKAGEACCESSOR_H
#define __PACKAGEACCESSOR_H

#include "NewtonKit.h"
#include "Frames/ObjHeader.h"
#include "Frames/Ref32.h"
#include "Packages/PackageParts.h"
#include "Chunk.h"

/* -----------------------------------------------------------------------------
	C P a c k a g e A c c e s s o r
	Access to the package diretory and part directory entries.
----------------------------------------------------------------------------- */

class CPackageAccessor
{
public:
	CPackageAccessor(CChunk & inChunk, long inOffset);
	~CPackageAccessor();

	PackageDirectory *	directory(void);

	bool			isPackage(void);
	size_t		size(void);
	ArrayIndex	numParts();
	Ptr			unsafePartDataPtr(ArrayIndex index);
	long			partOffset(ArrayIndex index);
	size_t		partSize(ArrayIndex index);
	ULong			partFlags(ArrayIndex index);

private:
	PartEntry *	part(ArrayIndex index);

	CChunk & fChunk;
	long		fOffset;
	PackageDirectory *	fPkgDir;
};


/* -----------------------------------------------------------------------------
	R e f S c a n n e r
	A scanner... of Refs.
----------------------------------------------------------------------------- */

class RefScanner
{
public:
	virtual void	scanRef(Ref32 * ioRef) = 0;
};


/* -----------------------------------------------------------------------------
	C F r a m e s P a r t A c c e s s o r
	Access to 32-bit refs in a NewtonScript frame part.
----------------------------------------------------------------------------- */
struct Frame;

class CFramesPartAccessor
{
public:
				CFramesPartAccessor(Ptr inPartData, size_t inPartSize, long inRootOffset);

	ObjHeader32 *	objectPtr(Ref32 inRef);
	SymbolObject32 *	symHeader(Ref32 inRef);
	const char * symbolName(Ref32 inRef);
	bool			symTest(Ref32 inRef, const char * inName);
	ArrayIndex	arrayLength(Ref32 inRef);
	ArrayIndex	findOffset1(Ref32 inRef, const char * inSlot, bool * outExists);
	ArrayIndex	findOffset(FrameObject32 * inFrame, const char * inSlot);
	Ref32			getFrameSlot(Ref32 inRef, const char * inSlot, bool * outExists = NULL);
	Ref32			getArraySlot(Ref32 inRef, ArrayIndex index);

	void			relocate(long inOffset);
	void			scanRefs(RefScanner &, Ref32);
	void			beginRedirectionSetup(void);
	void			setRedirection(Ref32 inFromRef, Ref32 inToRef);
	void			doRedirection(void);

private:
	ArrayObject32 *	fRootObj;
	size_t		fRootSize;
	long		fPartOffset;
	bool		fIsRedirectionSetup;
	int		fAlignment;
};


struct PartRef
{
	CFramesPartAccessor * fAccessor;
	Ref32 fRef;

	bool operator==(PartRef &);
};


struct ExportUnit
{
	PartRef fObjects;		//+00
	int fMajorVersion;	//+08
	int fMinorVersion;	//+0C
	ArrayIndex fCount;	//+10	num of members of fObjects frame
	char * fName;	//+14
//size+18
};

struct ImportUnit
{
	int majorVersion;	//+00
	int minorVersion;	//+04
	int f08;				//+08
	int f0C;				//+0C
//size+10
};


struct PendingImport
{
	int f00;
	int f04;
	int f08;		// offset to ImportUnit in g12C2
//size+0C -- don’t really know
};

extern void ProcessPart(int inSelector, CFramesPartAccessor & inPart, long inArg3, long inOffset);

typedef void (*SomeHandlerProc)(CChunk * inData, long inOffset);
#if defined(hasByteSwapping)
typedef void (*ByteSwapProc)(void *);
#endif

#if defined(hasByteSwapping)
extern void	AddBlock(ULong inTag, CChunk * inData, SomeHandlerProc inhandler = NULL, ByteSwapProc inSwapper = NULL);
#else
extern void	AddBlock(ULong inTag, CChunk * inData, SomeHandlerProc inhandler = NULL);
#endif


#endif /* __PACKAGEACCESSOR_H */
