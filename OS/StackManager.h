/*
	File:		StackManager.h

	Contains:	Stack manager declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__STACKMANAGER_H)
#define __STACKMANAGER_H 1

#include "Semaphore.h"
#include "Environment.h"
#include "UserDomain.h"
#include "DoubleQ.h"
#include "VirtualMemory.h"


/* -------------------------------------------------------------------------------
	C S t a c k P a g e
	A kPageSize area of memory used as stack.
------------------------------------------------------------------------------- */
class CStackInfo;

class CStackPage
{
public:
						CStackPage();
						~CStackPage();

	NewtonErr		init(CUDomainManager * inManager, ObjectId inPageId);

						operator	ObjectId()	const			{ return fPageId; }

//private:
	ObjectId			fPageId;			// +00
	CDoubleQItem	f04;				// +04
	CStackInfo *	fStackInfo[kSubPagesPerPage];	// f10
	UShort			fPageNo[kSubPagesPerPage];		// f20
	UByte				f28[kSubPagesPerPage];
	UByte				f2C[kSubPagesPerPage];
	ULong				fFreeSubPages : 4;	// +30	flags; bits 28..31 = sub page mask
	ULong				f30_8 : 1;				//					 bit 08000000
	ULong				fIsPageOurs : 1;		//					 bit 04000000
// size +34
};


/* -------------------------------------------------------------------------------
	C S t a c k I n f o
	Information about a 32K stack region - address limits & allocated pages.
------------------------------------------------------------------------------- */

class CStackInfo
{
public:
						CStackInfo();
						~CStackInfo();

	NewtonErr		init(VAddr inStackTop, VAddr inStackBottom, ArrayIndex inNumOfPages, ObjectId inOwnerId, VAddr inRockBottom, ObjectId inDomainId);

	VAddr				fAreaEnd;		// +00	end of memory area
	VAddr				fAreaStart;		// +04	start of memory area
	ArrayIndex		fNumOfPages;	// +08	number of stack pages
	ObjectId			fOwnerId;		// +0C	owner id
	CStackPage **	fPage;			// +10	stack pages
	VAddr				fBase;			// +14	mem start address
	VAddr				fStackStart;	// +18	stack start address, after 3K safety zone
	VAddr				fStackEnd;		// +1C	mem end address
	ULong				fOptions;		// +20
	ObjectId			fDomainId;		// +24	domain id
	ReleaseProcPtr fProc;			// +28
	void *			fRefCon;			// +2C
// size +30
};


/* -------------------------------------------------------------------------------
	C S t a c k M a n a g e r

	The stack manager is a domain manager that allows its domain to grow when
	addresses just beyond the bottom of the stack are accessed.
------------------------------------------------------------------------------- */

//	Stack manager monitor selectors
enum
{
	kSM_NewStack = 1,
	kSM_NewHeapArea,
	kSM_SetHeapLimits,
	kSM_FreePagedMem,
	kSM_5,
	kSM_LockHeapRange,
	kSM_UnlockHeapRange,
	kSM_NewHeapDomain,
	kSM_AddPageMappingToDomain,
	kSM_SetRemoveRoutine,
	kSM_GetHeapAreaInfo,
	kSM_GetSystemReleasable
};

//	Stack manager monitor parameters
struct FM_NewStack_Parms
{
	ObjectId	domainId;
	VAddr		addr;
	size_t	maxSize;
	ObjectId	ownerId;
	VAddr		stackTop;
	VAddr		stackBottom;
};

struct FM_NewHeapArea_Parms
{
	ObjectId	domainId;
	VAddr		addr;
	size_t	maxSize;
	ULong		options;
	VAddr		areaStart;
	VAddr		areaEnd;
};

struct FM_SetHeapLimits_Parms
{
	VAddr	start;
	VAddr	end;
};

struct FM_Free_Parms
{
	VAddr	addr;
};

struct FM_LockHeapRange_Parms
{
	VAddr start;
	VAddr end;
	bool  wire;
};

struct FM_NewHeapDomain_Parms
{
	ArrayIndex	sectionBase;
	ArrayIndex	sectionCount;
	ObjectId		domainId;
};

struct FM_AddPageMappingToDomain_Parms
{
	ObjectId	domainId;
	VAddr		addr;
	ObjectId	pageId;
};

struct FM_SetRemoveRoutine_Parms
{
	VAddr				addr;
	ReleaseProcPtr proc;
	void *			refCon;
};

struct FM_GetHeapAreaInfo_Parms
{
	VAddr		addr;
	VAddr		areaStart;
	VAddr		areaEnd;
};

struct FM_GetSystemReleaseable_Parms
{
	ArrayIndex	releasable;
	size_t		stackUsed;
	ArrayIndex	pagesUsed;
};

struct CopyPageAfterStackCollisionParams;


/* -------------------------------------------------------------------------------
	C S t a c k M a n a g e r
------------------------------------------------------------------------------- */
class CUEnvironment;
class CHeapDomain;

class CStackManager : public CUDomainManager
{
public:
					CStackManager();
					~CStackManager();

	virtual NewtonErr fault(ProcessorState * ioState);
	virtual NewtonErr userRequest(int inSelector, void * inData);
	virtual NewtonErr releaseRequest(int inSelector);
	virtual NewtonErr releaseRequest(int inSelector, bool, ArrayIndex * outNumOfFreePages);

	NewtonErr	init(void);
	NewtonErr	safeUserRequestEntry(int inSelector, void * inData);

	CStackPage *	allocNewPage(ObjectId inId);

	void			setRestrictedPage(ArrayIndex index, CStackPage * inPage);
	bool			checkRestrictedPage(CStackPage * inPage);
	void			copyPageState(CStackPage*, CStackPage*, ULong);
	void			updatePageState(CStackPage * inPage);

	NewtonErr	resolveFault(CStackInfo * info);

	void			findOrAllocPage_ReturnUnLockedOnNoPage(CStackInfo * info, ULong, ULong);

	ULong			buildPerms(CStackInfo * info, ULong inArg2, CStackPage * inPage);
	void			rememberMappings(CStackInfo * info, ULong, CStackPage*);
	void			forgetMappings(CStackInfo * info, CStackPage*, VAddr);
	void			forgetMappings(CStackInfo * info, ULong, CStackPage*);
	void			removeOwnerFromPage(CStackInfo * info, ULong, CStackPage*);

	void			freeSubPagesAbove(CStackInfo * info, VAddr, bool, ULong*, bool);
	void			freeSubPagesBelow(CStackInfo * info, VAddr, bool, ULong*, bool);
	NewtonErr	freeSubPagesBetween(CStackInfo * info, VAddr, VAddr, bool, ULong*, bool);
	void			unlockSubPagesBetween(CStackInfo * info, VAddr, VAddr);

	CStackInfo *	getStackInfo(VAddr inAddr);
	CHeapDomain *	getDomainForAddress(VAddr inAddr);
#if !defined(correct)
	CHeapDomain *	getDomainForCorrectAddress(VAddr inAddr);
#endif
	CHeapDomain *	getMatchingDomain(ObjectId inId);
	CStackPage *	getMatchingPage(CStackInfo * info, ULong, ULong);

	size_t		gatherFreePages(CDoubleQContainer&, bool);
	ArrayIndex	releasePagesInOneStack(CHeapDomain*, CUEnvironment, CStackInfo * info, bool);
	ArrayIndex	roundRobinPageRelease(CUEnvironment, bool);
	void			setSubPageInfo(CStackInfo * info, ULong, CStackPage*, ULong);
	void			copyPagesAfterStackCollided(CopyPageAfterStackCollisionParams *);
	void			pageMatchFound(CStackInfo * info, ULong, ULong, CStackPage*);
	ArrayIndex	countMatches(CStackInfo * info, ArrayIndex, CStackPage*, ULong*, bool*, bool*);

	NewtonErr	FMNewStack(FM_NewStack_Parms * ioParms, CStackInfo ** outInfo);
	NewtonErr	FMNewHeapArea(FM_NewHeapArea_Parms * ioParms);
	NewtonErr	validateHeapLimitsParms(FM_SetHeapLimits_Parms * ioParms);
	NewtonErr	checkRange(CStackInfo * info, VAddr inAddr);
	NewtonErr	FMSetHeapLimits(FM_SetHeapLimits_Parms * ioParms);
	NewtonErr	FMFree(FM_Free_Parms * ioParms);
	NewtonErr	FMFreeHeapRange(FM_SetHeapLimits_Parms * ioParms);
	NewtonErr	FMLockHeapRange(FM_LockHeapRange_Parms * ioParms);
	NewtonErr	FMUnlockHeapRange(FM_SetHeapLimits_Parms * ioParms);
	NewtonErr	FMNewHeapDomain(FM_NewHeapDomain_Parms * ioParms);
	NewtonErr	FMAddPageMappingToDomain(FM_AddPageMappingToDomain_Parms * ioParms);
	NewtonErr	FMSetRemoveRoutine(FM_SetRemoveRoutine_Parms * ioParms);
	NewtonErr	FMGetHeapAreaInfo(FM_GetHeapAreaInfo_Parms * ioParms);
	NewtonErr	FMGetSystemReleaseable(FM_GetSystemReleaseable_Parms * ioParms);

private:
	friend NewtonErr InvokeSMMRoutine(int inSelector, void * inArgs);

	ProcessorState *		fState;		// +40
	CDoubleQContainer		f44;			// list of CStackPage stack pages
	CDoubleQContainer		f58;			// ditto
	int						f6C[16];		// cf CRomDomainManager1K -- interestingly, also at the same address!
	int						fAC;
	CStackPage *			fB0[2];			// +B0 restricted pages
	CUMonitor				fStackMonitor;	// +B8
	bool						fC0;
	CULockingSemaphore	fC4;
	CDoubleQContainer		fDomains;		// +D0
	CULockingSemaphore	fE4;
	CHeapDomain *			fRoundRobinDomain;		// +F0
	ArrayIndex				fRoundRobinStackRegion;	// +F4
// size  +F8
};

extern CStackManager *	gStackManager;

inline NewtonErr	InvokeSMMRoutine(int inSelector, void * inArgs)
{ return gStackManager->fStackMonitor.invokeRoutine(inSelector, inArgs); }


/* -------------------------------------------------------------------------------
	C H e a p D o m a i n
------------------------------------------------------------------------------- */

class CHeapDomain
{
public:
					CHeapDomain();
					~CHeapDomain();
	NewtonErr	init(CStackManager * inManager, ULong inBaseMegs, ULong inNumMegs);
	NewtonErr	getStackInfo(VAddr inAddr, CStackInfo ** outInfo);
#if !defined(correct)
	NewtonErr	getCorrectStackInfo(VAddr inAddr, CStackInfo ** outInfo);
#endif

					operator	ObjectId()	const			{ return fId; }

	ObjectId			fId;					// +00	domain id
	CDoubleQItem	f04;					// +04
#if !defined(correct)
	VAddr				fCorrectBase;
	VAddr				fCorrectLimit;
#endif
	VAddr				fBase;				// +10	base address
	VAddr				fLimit;				// +14	(one past) end address
	ArrayIndex		fStackRegionCount;	// +18	number of…
	CStackInfo **	fStackRegion;			// +1C	one for each 33K
};


/* -------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------- */

extern NewtonErr  MakeSystemStackManager(void);


#endif	/* __STACKMANAGER_H */
