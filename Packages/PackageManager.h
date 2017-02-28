/*
	File:		PackageManager.cc

	Contains:	Package manager implementation.

	Written by:	Newton Research Group
*/

#if !defined(__PACKAGEMANAGER_H)
#define __PACKAGEMANAGER_H 1

#include "PackageEvents.h"
#include "PackageParts.h"
#include "LargeObjectStore.h"
#include "AppWorld.h"


/*------------------------------------------------------------------------------
	C P a c k a g e M a n a g e r
------------------------------------------------------------------------------*/

class CPackageManager : public CAppWorld
{
public:
							CPackageManager() : fEventHandler(NULL) {}	 // just guessing

	virtual size_t		getSizeOf(void) const;

	virtual NewtonErr	mainConstructor(void);
	virtual void		mainDestructor(void);

	CPackageEventHandler *	fEventHandler;
	Heap						fPersistentHeap;
	Heap						fDefaultHeap;
	uint32_t					f7C;
};

#define gPkgWorld static_cast<CPackageManager*>(GetGlobals())


/* -----------------------------------------------------------------------------
	C P M I t e r a t o r
	Package Manager iterator.
----------------------------------------------------------------------------- */

class CPMIterator
{
public:
					CPMIterator();
					~CPMIterator();

	NewtonErr	init(void);
	void			nextPackage(void);
	bool			more(void);
	void			done(void);

	const UniChar *	packageName(void) const;
	ULong			packageId(void) const;
	size_t		packageSize(void) const;
	ULong			packageVersion(void) const;
	ULong			timestamp(void) const;
	bool			isCopyProtected(void) const;

private:
	ArrayIndex	fIndex;			//+00
	size_t		fPkgSize;		//+04
	ULong			fPkgId;			//+08
	UniChar		fPkgName[kMaxPackageNameSize+1];	//+0C	+1 not original
	ULong			fPkgVersion;	//+4C
	SourceType	fSource;			//+50
	ULong			fTimestamp;		//+58
	ULong			fPkgFlags;		//+5C
	uint32_t		f60;				//+60
};


inline const UniChar *	CPMIterator::packageName(void) const		{ return fPkgName; }
inline ULong				CPMIterator::packageId(void) const			{ return fPkgId; }
inline size_t				CPMIterator::packageSize(void) const		{ return fPkgSize; }
inline ULong				CPMIterator::packageVersion(void) const	{ return fPkgVersion; }
inline ULong				CPMIterator::timestamp(void) const			{ return fTimestamp; }
inline bool					CPMIterator::isCopyProtected(void) const	{ return (fPkgFlags & kCopyProtectFlag) != 0; }


/* -----------------------------------------------------------------------------
	C P r i v a t e P a c k a g e I t e r a t o r
----------------------------------------------------------------------------- */
#include "NewtonPackage.h"

class CPrivatePackageIterator
{
public:
					CPrivatePackageIterator();
					~CPrivatePackageIterator();

	NewtonErr	init(void * inPackage);
	NewtonErr	computeSizeOfEntriesAndData(ULong& inPartEntryInfoSize, ULong& inDirectoryInfoSize);
	NewtonErr	setupRelocationData(ULong inOtherInfoSize, ULong * outRelocationInfoSize);
	NewtonErr	getRelocationChunkInfo(void);

	void			disposeDirectory(void);

	NewtonErr	checkHeader(void);
	NewtonErr	verifyPackage(void);
	UniChar *	packageName(void);
	size_t		packageSize(void);

	ArrayIndex	numberOfParts(void);
	void			getPartInfo(ArrayIndex inPartIndex, PartInfo * const outInfo);
	void			getPartInfoDesc(ArrayIndex inPartIndex, PartInfo * const outInfo);
	ULong			getPartDataOffset(ArrayIndex inPartIndex);

private:
	friend class CPackageIterator;
	NewtonPackage *		fPackage;	// definitely not original -- for LP64 / byte-swapped platforms
	Ptr			fMem;						//+04
	PackageDirectory *	fPkgDir;		//+08
	PartEntry *				fPkgParts;	//+0C
	RelocationHeader *	fReloc;		//+10
	ULong *			fRelocInfo;			//+14
	Ptr			fPkgData;				//+18
	ULong			fTotalInfoSize;		//+1C
};


/* -----------------------------------------------------------------------------
	C P a c k a g e I t e r a t o r
----------------------------------------------------------------------------- */

class CPackageIterator
{
public:
					CPackageIterator(CPipe * inPipe);
					CPackageIterator(void * inPackage);
					~CPackageIterator();

	NewtonErr	init(void);
	NewtonErr	initFields(void);
	NewtonErr	computeSizeOfEntriesAndData(ULong& inPartEntryInfoSize, ULong& inDirectoryInfoSize);
	NewtonErr	setupRelocationData(ULong inOtherInfoSize, ULong * outRelocationInfoSize);
	NewtonErr	getRelocationChunkInfo(void);

	void			disposeDirectory(void);

	NewtonErr	verifyPackage(void);
	ULong			getPackageId(void);
	ULong			getVersion(void);
	ULong			packageFlags(void);
	UniChar *	packageName(void);
	size_t		packageSize(void);
	bool			forDispatchOnly(void);
	bool			copyProtected(void);
	UniChar *	copyright(void);
	ULong			creationDate(void);
	ULong			modifyDate(void);

	size_t		directorySize(void);
	ArrayIndex	numberOfParts(void);
	void			getPartInfo(ArrayIndex inPartIndex, PartInfo * const outInfo);
	ULong			getPartDataOffset(ArrayIndex inPartIndex);
	ULong			processorTypeOfPart(ArrayIndex inPartIndex);

	void			store(CStore * inStore, PSSId inId, CCallbackCompressor * inCompressor, CLOCallback * inCallback = NULL);

private:
	CPrivatePackageIterator fPrivateIter;	//+00
	bool			fIsStreamSource;
	CPipe *		fPipe;
//size+28
};


#endif	/* __PACKAGEMANAGER_H */
