/*
	File:		RDM.h

	Contains:	ROM Domain Manager monitor declarations.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__RDM_H)
#define __RDM_H 1

#include "MemArch.h"
#include "Store.h"
#include "StoreRootObjects.h"
#include "StoreCompander.h"

//	ROM domain manager monitor selectors
enum
{
	kRDM_MapLargeObject = 1,
	kRDM_UnmapLargeObject,
	kRDM_FreePageCount,
	kRDM_4,
	kRDM_IdToStore,
	kRDM_6,
	kRDM_7,
	kRDM_8,	// unused
	kRDM_FlushLargeObject,
	kRDM_ResizeLargeObject,
	kRDM_AbortObject,
	kRDM_CommitObject,
	kRDM_GetLargeObjectInfo,
	kRDM_StoreToVAddr,
	kRDM_LargeObjectAddress,
	kRDM_16,
	kRDM_17,
	kRDM_18,
};


//	ROM domain manager monitor parameters
struct RDMParams
{
				RDMParams();

	CStore * fStore;	// +00
	PSSId		fObjId;	// +04
	VAddr		fAddr;	// +08
	PSSId		fPkgId;	// +0C
	size_t	fSize;	// +10
	int		f14;
	bool		fRO;		// +18
	bool		fDirty;	// +19
};


/* -----------------------------------------------------------------------------
	P a g e T a b l e C h u n k
	Maps the subpage addresses for a page.
----------------------------------------------------------------------------- */

struct PageTableChunk
{
	ObjectId			fId;									// +00	kernel page object
	UShort			fPageAddr[kSubPagesPerPage];	// +04	page offset from fROMBase -- provides 64K * kPageSize = 256MB addressable memory
	bool				fIsWritable[kSubPagesPerPage];// +0C	on a per-subpage basis
};


/* -----------------------------------------------------------------------------
	L O T r a n s a c t i o n H a n d l e r
----------------------------------------------------------------------------- */

class LOTransactionHandler
{
public:
					LOTransactionHandler();
	void			free(void);

	NewtonErr	addObjectToTransaction(PSSId inId, CStore * inStore, int inTxnType);
	void			setAllInTransaction(void);
	bool			hasTransaction(void);
	NewtonErr	endTransaction(bool, CStore * inStore, PSSId inObjId, PSSId inTableId);
	NewtonErr	endAllObjectsTransaction(bool, CStore * inStore, PSSId inObjId, PSSId inTableId);
	NewtonErr	endIndexTableTransaction(bool, CStore * inStore, PSSId inTableId);

private:
	CSortedList *	fList;		// ids of objects within a transaction
	bool				f04;			// all in transaction
	bool				f05;
};


/* -----------------------------------------------------------------------------
	P a c k a g e C h u n k
	Encapsulation of a large object on a store.
----------------------------------------------------------------------------- */
struct PackageChunk
{
							PackageChunk(CStore * inStore, PSSId inObjId);

	CStore *				fStore;				// +00
	PSSId					fRootId;				// +04
	PSSId					fDataId;				// +08
	CStoreCompander * fStoreCompander;  // +0C
	int					f10;					// CUObject * subclass
	char *				fCompanderName;	// +14
	int					f18;
	PSSId					fPkgId;				// +1C
	VAddr					fAddr;				// +20
	size_t				fSize;				// +24
	NewtonErr			fError;				// +28
	int					f2C;
	bool					f30;
	LOTransactionHandler	fTxnHandler;	// +34
};


/*------------------------------------------------------------------------------
	C R O M D o m a i n M a n a g e r 1 K

	The ROM domain manager looks after virtual memory mapping for packages and
	large binary objects.
------------------------------------------------------------------------------*/

class CROMDomainManager1K
#if !defined(forFramework)
 : public CUDomainManager
#endif
{
public:
				CROMDomainManager1K();
	virtual	~CROMDomainManager1K();

	virtual NewtonErr fault(ProcessorState * inState);
	virtual NewtonErr userRequest(int inSelector, void * inData);
	virtual NewtonErr releaseRequest(int inSelector);

#if defined(forFramework)
	// beccause we don’t subclass from CUDomainManager
	NewtonErr	remember(VAddr inArg1, ULong inPermissions, ULong inArg3, bool inArg4);
#endif
// large objects
	PackageChunk *	getObjectPtr(VAddr inAddr);

	NewtonErr	objectToIndex(VAddr inAddr, ArrayIndex * outIndex);
	NewtonErr	objectToIndex(CStore * inStore, PSSId inObjId, ArrayIndex * outIndex);
	NewtonErr	packageToIndex(PSSId inPkgId, ArrayIndex * outIndex);

	NewtonErr	addPackage(CStore * inStore, PSSId inObjId, bool inReadOnly, VAddr * outAddr);
	NewtonErr	setPackageId(CStore * inStore, PSSId inObjId, PSSId inPkgId);

	NewtonErr	allocatePackageEntry(PackageChunk * ioChunk, ArrayIndex * outIndex);
	NewtonErr	deleteObjectInfo(PackageChunk * inChunk, NewtonErr inErr);
	NewtonErr	resizeObject(VAddr * outAddr, VAddr inAddr, size_t inSize, NewtonErr inErr);
	NewtonErr	mungeObject(PackageChunk * inChunk, ArrayIndex inOffset, int inDelta);

	NewtonErr	doTransactionAgainstObject(int inSelector, PackageChunk * inChunk, PSSId, int);

	NewtonErr	insertPages(PackageChunk * inChunk, ArrayIndex inStart, ArrayIndex inCount);
	NewtonErr	removePages(PackageChunk * inChunk, ArrayIndex inStart, ArrayIndex inCount);

	NewtonErr	decompressAndMap(VAddr inAddr, PackageChunk * inChunk);
	void			doAcquireDatabase(bool);
	void			doReleaseDatabase(void);

	NewtonErr	endSession(ArrayIndex index, bool);
	NewtonErr	flushCache(ArrayIndex index, bool);
	NewtonErr	endSession(CStore * inStore, PSSId inObjId);
	NewtonErr	flushCache(CStore * inStore, PSSId inObjId);
	NewtonErr	endSession(PSSId inPkgId);
	NewtonErr	flushCache(PSSId inPkgId);
	NewtonErr	flushCacheByBase(VAddr inAddr);

// XIP
	void			XIPMapInPackageSection(VAddr inAddr);
	NewtonErr	XIPAllocatePackageEntry(PackageChunk * outChunk, long*);
	NewtonErr	XIPAddPackage(CStore * inStore, PSSId inObjId, VAddr * outAddr);
	NewtonErr	XIPEndSession(ArrayIndex index);
	NewtonErr	XIPFault(ProcessorState * ioState);
	void			XIPObjectHasMoved(CStore * inStore, PSSId inObjId);
	void			XIPInvalidateStore(CStore * inStore);

// page table
	NewtonErr	reset(ArrayIndex, ArrayIndex);
	void			clearTableEntry(ArrayIndex index);

	NewtonErr	addPageTableEntry(ArrayIndex index, VAddr inAddr);
	NewtonErr	addPageTableEntry(ArrayIndex index, UShort, ULong);
	NewtonErr	releasePageTableEntry(ArrayIndex index);
	void			releasePagesFromOurWS(ArrayIndex index);

	ArrayIndex	vAddrToPageIndex(VAddr inAddr);

	void			getSourcePage(ULong);
	bool			restrictToInternalWorkingSet(void);
	NewtonErr	getWorkingSetPage(ULong * outPage);
	NewtonErr	restrictPage(ArrayIndex index);
	NewtonErr	unrestrictPage(ArrayIndex index);
	bool			restrictedPage(ArrayIndex index);
	NewtonErr	addPage(ArrayIndex * outIndex, ObjectId inPageId);

	bool			isEmptyPage(PageTableChunk * inChunk);
	bool			isValidPage(PageTableChunk * inChunk);
	bool			okIfDirty(ULong);

	NewtonErr	collect(ULong * outPermissions, ULong inSubPageMask, ArrayIndex inSubPage, PageTableChunk * inPageTableEntry);
	NewtonErr	writeOutPage(VAddr inAddr);

// sub pages
	NewtonErr	getSubPage(VAddr, ArrayIndex*, PackageChunk*);

	ULong			makeSubPageBitMap(ArrayIndex inPage, UShort inBits);
	ULong			subPageMap(ArrayIndex inPage);
	NewtonErr	findSubPage(ArrayIndex * outPage, ULong inSubPageMask);
	bool			subPageFree(ArrayIndex inPage, ArrayIndex inSubPage);
	NewtonErr	shuffleSubPages(ArrayIndex inFromPageIndex, ArrayIndex inToPageIndex, ULong inSubPageMask);
	NewtonErr	freeSubPages(ArrayIndex inPageIndex, ULong inSubPageMask);
	NewtonErr	freeAnySubPages(ArrayIndex * outPage, ULong inSubPageMask);

// permissions
	ULong			makePermissions(ArrayIndex inPage, ArrayIndex inSubPage, bool inArg3);

	friend ObjectId		GetROMDomainManagerId(void);
	friend CUMonitor *	GetROMDomainUserMonitor(void);
	friend size_t			ROMDomainManagerFreePageCount(void);

private:
	VAddr					fROMBase;			// +40	ROM domain base
	size_t				fROMSize;			// +44	ROM domain size
	CDynamicArray *	fPackageTable;		// +48	list of PackageChunks for active large objects
	PageTableChunk *	fPageTable;			// +4C
	char *				fBuffer;				// +50	LZ (de)compression buffer
	ArrayIndex			fNumOfSubPages;	// +54
	ArrayIndex			fNumOfPageTableEntries;	// +58
	ArrayIndex			f5C;
	ArrayIndex			fNumOfFreePages;	// +60	possibly actually USED pages
	ArrayIndex			fNumOfPages;		// +64
	int			f68;
	int			f6C[16];
	int			fAC;
	int			fB0;
	int			fB4;
	ArrayIndex			fFreeIndex;			// +B8
	CTime			fBC;
	ArrayIndex	fC4;
	ArrayIndex	fC8;
	bool			fIsWritingOutPage;		// +CC
	bool			fCD;
	int			fD0;
	CDecompressor *	fDecompressor;		// +D4
	bool			fD8;
	bool			fIsROCompander;			// +D9
	bool			fDA;
	VAddr					fXIPBase;			// +DC	XIP domain base
// size +E0
};


extern NewtonErr	InitROMDomainManager(void);
extern NewtonErr	RegisterROMDomainManager(void);
extern size_t		ROMDomainManagerFreePageCount(void);


#endif	/* __RDM_H */
