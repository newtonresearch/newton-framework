/*
	File:		KernelPhys.cc

	Contains:	Virtual to physical address mapping.

	Written by:	Newton Research Group.
*/

#include "KernelPhys.h"
#include "MMU.h"
#include "VirtualMemory.h"
#include "PageManager.h"
#include "OSErrors.h"

extern ObjectId	GetDomainFromDomainNumber(int inNumber);


PAddr
PhysBase(ObjectId inId)
{
	CPhys *	page;
	if (GetPhys(inId, page))
		return page->base();
	return 0;
}


size_t
PhysSize(ObjectId inId)
{
	CPhys *	page;
	if (GetPhys(inId, page))
		return page->size();
	return 0;
}


ULong
PhysAlign(ObjectId inId)
{
	CPhys *	page;
	if (GetPhys(inId, page))
		return TRUNC(page->base(), 4*MByte);
	return 0;
}


bool
PhysReadOnly(ObjectId inId)
{
	CPhys *	page;
	if (GetPhys(inId, page))
		return page->isReadOnly();
	return YES;		// original returns -1;
}


NewtonErr
CopyPhysicalPage(PAddr inFromAddr, PAddr inToAddr, ULong inSubPageMask)
{
	ULong	subPageBase = 0;
	for (int i = 0; i < 4; i++, subPageBase += kSubPageSize)
	{
		if ((inSubPageMask & (1 << i)) != 0)
		{
			PAddr	fromAddr = inFromAddr + subPageBase;
			PAddr	toAddr = inToAddr + subPageBase;
			for (ULong offset = 0; i < kSubPageSize; i += 128)	// lock out FIQ in short bursts
			{
				EnterFIQAtomic();
				PhysSubPageCopy(fromAddr + offset, toAddr + offset);
				ExitFIQAtomic();
			}
		}
	}
	return noErr;
}


NewtonErr
CopyPhysPgGlue(ObjectId inFromId, ObjectId inToId, ULong inSubPageMask)
{
	CPhys *	fromPage;
	CPhys *	toPage;
	if (GetPhys(inFromId, fromPage)
	&&  GetPhys(inToId, toPage))
		return CopyPhysicalPage(fromPage->base(), toPage->base(), inSubPageMask);
	return kOSErrBadObjectId;
}


NewtonErr
InvalidatePhys(ObjectId inId)
{
	NewtonErr err = noErr;
	XTRY
	{
		CPhys *	page;
		XFAILNOT(GetPhys(inId, page), err = kOSErrBadObjectId;)
		page->invalidate();
	}
	XENDTRY;
	return err;
}


NewtonErr
MakePhysInaccessible(ObjectId inId)
{
	NewtonErr err = noErr;
	XTRY
	{
		CPhys *	page;
		XFAILNOT(GetPhys(inId, page), err = kOSErrBadObjectId;)
		page->makeInaccessible();
	}
	XENDTRY;
	return err;
}


NewtonErr
MakePhysAccessible(ObjectId inId)
{
	NewtonErr err = noErr;
	XTRY
	{
		CPhys *	page;
		XFAILNOT(GetPhys(inId, page), err = kOSErrBadObjectId;)
		page->makeAccessible();
	}
	XENDTRY;
	return err;
}


NewtonErr
ChangeVirtualMapping(ObjectId inId, VAddr inVAddr, size_t inVSize, PhysicalChange inAccess)
{
	NewtonErr err = noErr;
	XTRY
	{
		CPhys *	page;
		XFAILNOT(GetPhys(inId, page), err = kOSErrBadObjectId;)
		page->changeVirtualMapping(inVAddr, inVSize, inAccess);
	}
	XENDTRY;
	return err;
}


NewtonErr
ChangeVirtualMapping(void)
{
	TaskGlobals *	taskGlobals = TaskSwitchedGlobals();
	return ChangeVirtualMapping((ObjectId) taskGlobals->fParams[0], (VAddr) taskGlobals->fParams[1], (size_t) taskGlobals->fParams[2], (PhysicalChange) taskGlobals->fParams[3]);
}


NewtonErr
ChangePRangeAccessibility(PAddr inPAddr, size_t inSize, PhysicalChange inChange)
{
	// WHOPPER
	return noErr;
}


NewtonErr
ChangeVRangeAccessibility(VAddr inVAddr, size_t inVSize, PAddr inPAddr, size_t inPSize, PhysicalChange inChange)
{
	// WHOPPER
	return noErr;
}

#pragma mark -

bool
GetPhys(ObjectId inPageId, CPhys * &outPage)
{
	CPhys *	page = PhysIdToObj(kPhysType, inPageId);
	outPage = page;
	if (page == NULL)
	{
		page = PhysIdToObj(kExtPhysType, inPageId);
		outPage = page;
		if (page == NULL)
		{
			page = (CPhys *)IdToObj(kPhysType, inPageId);
			outPage = page;
		}
		if (page == NULL)
			return NO;
	}
	return YES;
}


NewtonErr
VerifyPhysMappingParams(ULong inDomain, ULong inVAddr, ObjectId inPageId, ULong inOffset, size_t inSize)
{
	NewtonErr err = noErr;
	XTRY
	{
		CPhys *	page;
		XFAILNOT(GetPhys(inPageId, page), err = kOSErrBadObjectId;)

		int		domain;
		XFAILIF(inOffset + inSize >= page->size()
			  || (domain = CheckVAddrRange(inOffset + inVAddr, inSize != 0 ? inSize : page->size())) < 0
			  || GetDomainFromDomainNumber(domain) != inDomain, err = kOSErrBadParameters;)
	}
	XENDTRY;
	return err;
}


int
ComputePhysUnit(VAddr inVAddr, PAddr inPAddr, size_t inSize)
{
	if ((inVAddr & (kSectionSize-1)) == 0
	&&  (inPAddr & (kSectionSize-1)) == 0
	&&  inSize >= kSectionSize)
		return 0;

	if ((inVAddr & (kBigPageSize-1)) == 0
	&&  (inPAddr & (kBigPageSize-1)) == 0
	&&  inSize >= kBigPageSize)
		return 1;

	return 3;	// assume kPageSize
}


int
ComputePermUnit(VAddr inVAddr, size_t inSize)
{
	if ((inVAddr & (kSectionSize-1)) == 0
	&&  inSize >= kSectionSize)
		return 0;

	if ((inVAddr & (kBigPageSize-1)) == 0
	&&  inSize >= kBigPageSize)
		return 1;

	if ((inVAddr & (kBigSubPageSize-1)) == 0
	&&  inSize >= kBigSubPageSize)
		return 2;

	if ((inVAddr & (kPageSize-1)) == 0
	&&  inSize >= kPageSize)
		return 3;

	return 4;	// assume sub kPageSize
}


NewtonErr
MakeConforming(VAddr inVAddr, int * ioUnit)
{
	NewtonErr err = noErr;

	switch (VtoUnit(inVAddr))
	{
	case 0:
		switch (*ioUnit)
		{
		case 1:
		case 2:
			err = TransformSectionIntoPageTable(inVAddr);
			break;
		case 3:
		case 4:
			if ((err = TransformSectionIntoPageTable(inVAddr)) == noErr)
				err = TransformBigPageToPages(inVAddr);
			break;
		}
		break;

	case 1:
		switch (*ioUnit)
		{
		case 0:
			*ioUnit = 1;
			break;
		case 3:
		case 4:
			err = TransformBigPageToPages(inVAddr);
			break;
		}
		break;

	case 3:
		*ioUnit = 3;
		break;

	case 5:
		switch (*ioUnit)
		{
		case 1:
		case 2:
		case 3:
		case 4:
			err = AllocatePageTable(inVAddr);
			break;
		}
		break;

	case 6:
		switch (*ioUnit)
		{
		case 0:
		case 1:
		case 2:
			*ioUnit = CanUseBigPage(inVAddr) ? 1 : 3;
			break;
		case 3:
		case 4:
			*ioUnit = 3;
			break;
		}
		break;
	}
	return err;
}

#pragma mark -

NewtonErr
PrimRememberPhysMapping(VAddr inVAddr, CPhys * inPage, ULong inOffset, size_t inSize, bool inCacheable)
{
	NewtonErr	err;
	int			unit;
	ULong			unitSize;

	PAddr		physAddr = inPage->base();
	size_t	size = (inSize != 0) ? inSize : inPage->size();
	do
	{
		unit = ComputePhysUnit(inVAddr, physAddr, size);
		if ((err = MakeConforming(inVAddr, &unit)) != noErr)
			return err;

		switch (unit)
		{
		case 0:
			AddSecP(inVAddr, physAddr, inCacheable);
			unitSize = kSectionSize;
			break;
		case 1:
			AddBigPgP(inVAddr, physAddr, inCacheable);
			unitSize = kBigPageSize;
			break;
		case 3:
			AddPgP(inVAddr, physAddr, inCacheable);
			unitSize = kPageSize;
			break;
		}
		inVAddr += unitSize;
		physAddr += unitSize;
		size -= unitSize;
	} while (size != 0);

	return noErr;
}


NewtonErr
RememberPhysMapping(ULong inDomain, VAddr inVAddr, ObjectId inPageId, bool inCacheable)
{
	NewtonErr err = noErr;
	XTRY
	{
		CPhys * page;
		XFAIL(err = VerifyPhysMappingParams(inDomain, inVAddr, inPageId, 0, 0))
		GetPhys(inPageId, page);
		err = PrimRememberPhysMapping(inVAddr, page, 0, 0, inCacheable);
	}
	XENDTRY;
	return err;
}


NewtonErr
RememberPhysMapping(void)
{
	NewtonErr err = noErr;
	XTRY
	{
		CPhys * page;
		TaskGlobals *	taskGlobals = TaskSwitchedGlobals();
		ULong			vAddr = taskGlobals->fParams[1];
		ObjectId		pageId = taskGlobals->fParams[2];
		ULong			offset = taskGlobals->fParams[3];
		ULong			size = taskGlobals->fParams[4];
		bool			isCacheable = taskGlobals->fParams[5];
		XFAIL(err = VerifyPhysMappingParams(taskGlobals->fParams[0], vAddr, pageId, offset, size))
		GetPhys(pageId, page);
		err = PrimRememberPhysMapping(vAddr, page, offset, size, isCacheable);
	}
	XENDTRY;
	return err;
}


NewtonErr
PrimRememberPermMapping(VAddr inVAddr, size_t inSize, Perm inPerm)
{
	NewtonErr	err;
	int			unit;
	ULong			unitSize;

	do
	{
		unit = ComputePermUnit(inVAddr, inSize);
		if ((err = MakeConforming(inVAddr, &unit)) != noErr)
			return err;

		switch (unit)
		{
		case 0:
			AddSecPerm(inVAddr, inPerm);
			unitSize = kSectionSize;
			break;
		case 1:
			AddBigPgPerm(inVAddr, inPerm);
			unitSize = kBigPageSize;
			break;
		case 2:
			AddBigSubPgPerm(inVAddr, inPerm);
			unitSize = kBigSubPageSize;
			break;
		case 3:
			AddPgPerm(inVAddr, inPerm);
			unitSize = kPageSize;
			break;
		case 4:
			AddSubPgPerm(inVAddr, inPerm);
			unitSize = kSubPageSize;
			break;
		}
		inVAddr += unitSize;
		inSize -= MIN(inSize, unitSize);
	} while (inSize != 0);

	return noErr;
}


NewtonErr
RememberPermMapping(ULong inDomain, VAddr inVAddr, size_t inSize, Perm inPerm)
{
	NewtonErr err = noErr;
	XTRY
	{
		int domain;
		XFAILIF((domain = CheckVAddrRange(inVAddr, inSize)) < 0, err = kOSErrBadParameters;)
		XFAILIF(GetDomainFromDomainNumber(domain) != inDomain, err = kOSErrBadParameters;)
		err = PrimRememberPermMapping(inVAddr, inSize, inPerm);
	}
	XENDTRY;
	return err;
}


NewtonErr
PrimRememberMapping(VAddr inVAddr, ULong inPerm, CPhys * inPage, bool inCacheable)
{
	NewtonErr err = noErr;	// Newton doesnÕt set this! Maybe it never checks.

	switch (VtoUnit(inVAddr))
	{
	case 2:
	case 4:
		break;
	case 5:
		if ((err = AllocatePageTable(inVAddr)) != noErr)
			break;
	case 6:
	case 3:
		err = AddPgPAndPerm(inVAddr, inPerm, inPage->base(), inCacheable);
		break;
	default:
		err = kOSErrBadParameters;
		break;
	}

	return err;
}


NewtonErr
RememberMapping(ULong inDomain, VAddr inVAddr, ULong inPerm, ObjectId inPageId, bool inCacheable)
{
	NewtonErr err = noErr;
	XTRY
	{
		int domain;
		CPhys * page;
		XFAILNOT(GetPhys(inPageId, page), err = kOSErrBadObjectId;)
		XFAILIF((domain = CheckVAddrRange(inVAddr, page->size())) < 0, err = kOSErrBadParameters;)
		XFAILIF(page->size() != kPageSize, err = kOSErrBadParameters;)
		XFAILIF(GetDomainFromDomainNumber(domain) != inDomain, err = kOSErrBadParameters;)
		err = PrimRememberMapping(inVAddr, inPerm, page, inCacheable);
	}
	XENDTRY;
	return err;
}

#pragma mark -

NewtonErr
PrimForgetPhysMapping(VAddr inVAddr, CPhys * inPage, size_t inSize)
{
	NewtonErr	err;
	int			unit;
	ULong			unitSize;

	PAddr		physAddr = PrimVtoP(inVAddr);
	PAddr		pageBase = inPage->base();
	size_t	pageSize = inPage->size();
	size_t	size = (inSize != 0) ? inSize : pageSize;
	if (physAddr < pageBase
	||  physAddr + inSize > pageBase + pageSize)
		return kOSErrBadParameters;

	do
	{
		unit = ComputePhysUnit(inVAddr, physAddr, size);
		if ((err = MakeConforming(inVAddr, &unit)) != noErr)
			return err;

		switch (unit)
		{
		case 0:
			RemoveSecP(inVAddr);
		case 5:
			unitSize = kSectionSize;
			break;
		case 1:
			RemoveBigPgP(inVAddr);
			unitSize = kBigPageSize;
			break;
		case 3:
			RemovePgP(inVAddr);
		case 6:
			unitSize = kPageSize;
			break;
		}
		inVAddr += unitSize;
		physAddr += unitSize;
		size -= unitSize;
	} while (size != 0);

	return noErr;
}


// Generic SWI 7 callback
NewtonErr
ForgetPhysMapping(ULong inDomain, VAddr inArg2, ObjectId inPageId)
{
	NewtonErr err = noErr;
	XTRY
	{
		CPhys * page;
		XFAIL(err = VerifyPhysMappingParams(inDomain, inArg2, inPageId, 0, 0))
		GetPhys(inPageId, page);
		err = PrimForgetPhysMapping(inArg2, page, 0);
	}
	XENDTRY;
	return err;
}


// Generic SWI 60 callback
NewtonErr
ForgetPhysMapping(void)
{
	NewtonErr err = noErr;
	XTRY
	{
		CPhys * page;
		TaskGlobals *	taskGlobals = TaskSwitchedGlobals();
		ULong			vAddr = taskGlobals->fParams[1];
		ObjectId		pageId = taskGlobals->fParams[2];
		ULong			size = taskGlobals->fParams[4];
		XFAIL(err = VerifyPhysMappingParams(taskGlobals->fParams[0], vAddr, pageId, 0, size))
		GetPhys(pageId, page);
		err = PrimForgetPhysMapping(vAddr, page, size);
	}
	XENDTRY;
	return err;
}


NewtonErr
PrimForgetPermMapping(VAddr inVAddr, size_t inSize)
{
	NewtonErr	err;
	int			unit;
	ULong			unitSize;

	do
	{
		unit = ComputePermUnit(inVAddr, inSize);
		if ((err = MakeConforming(inVAddr, &unit)) != noErr)
			return err;

		switch (unit)
		{
		case 0:
		case 5:
			RemoveSecPerm(inVAddr);
			unitSize = kSectionSize;
			break;
		case 1:
			RemoveBigPgPerm(inVAddr);
			unitSize = kBigPageSize;
			break;
		case 2:
			RemoveBigSubPgPerm(inVAddr);
			unitSize = kBigSubPageSize;
			break;
		case 3:
		case 6:
			RemovePgPerm(inVAddr);
			unitSize = kPageSize;
			break;
		case 4:
			RemoveSubPgPerm(inVAddr);
			unitSize = kSubPageSize;
			break;
		}
		inVAddr += unitSize;
		inSize -= MIN(inSize, unitSize);
	} while (inSize != 0);

	return noErr;
}


NewtonErr
ForgetPermMapping(ULong inDomain, VAddr inVAddr, size_t inSize)
{
	NewtonErr err = noErr;
	XTRY
	{
		int domain;
		XFAILIF((domain = CheckVAddrRange(inVAddr, inSize)) < 0, err = kOSErrBadParameters;)
		XFAILIF(GetDomainFromDomainNumber(domain) != inDomain, err = kOSErrBadParameters;)
		err = PrimForgetPermMapping(inVAddr, inSize);
	}
	XENDTRY;
	return err;
}


NewtonErr
PrimForgetMapping(VAddr inVAddr, CPhys * inPage)
{
	RemovePgPAndPerm(inVAddr);
	return noErr;
}


NewtonErr
ForgetMapping(ULong inDomain, VAddr inVAddr, ObjectId inPageId)
{
	NewtonErr err = noErr;
	XTRY
	{
		int domain;
		CPhys * page;
		XFAILNOT(GetPhys(inPageId, page), err = kOSErrBadObjectId;)
		XFAILIF((domain = CheckVAddrRange(inVAddr, page->size())) < 0, err = kOSErrBadParameters;)
		XFAILIF(GetDomainFromDomainNumber(domain) != inDomain, err = kOSErrBadParameters;)
		err = PrimForgetMapping(inVAddr, page);
	}
	XENDTRY;
	return err;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C P h y s
------------------------------------------------------------------------------*/

CPhys::CPhys()
{
	fIsWhatever = NO;
	fIsLittle = YES;		// not in the original, but surely init() relies on this?
}


CPhys::~CPhys()
{ }


NewtonErr
CPhys::init(PAddr inPAddr, size_t inSize, bool inReadOnly, bool inCache)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILIF((inSize > MByte && !fIsLittle)									// CLittlePhys must be < 1MByte in size
			  ||  PAGEOFFSET(inPAddr) != 0										// address must be page aligned
			  ||  inSize >= 256 * MByte, err = kOSErrBadParameters;)		// must not be larger than 256MByte
		initState(inPAddr, inSize, inReadOnly, inCache);
	}
	XENDTRY;
	return err;
}


void
CPhys::initState(PAddr inPAddr, size_t inSize, bool inReadOnly, bool inCache)
{
	fPageAddr = inPAddr / kPageSize;
	fIsReadOnly = inReadOnly;
	fIsCached = inCache;
	if (inSize >= MByte)
		fSize = inSize;
	else
		fNumOfPages = inSize / kPageSize;
}


NewtonErr
CPhys::invalidate(void)
{
	return ChangePRangeAccessibility(fPageAddr * kPageSize, fIsLittle ? fNumOfPages * kPageSize: fSize, kMakeRangeInvalid);
}


NewtonErr
CPhys::makeInaccessible(void)
{
	return ChangePRangeAccessibility(fPageAddr * kPageSize, fIsLittle ? fNumOfPages * kPageSize: fSize, kMakeRangeInaccessible);
}


NewtonErr
CPhys::makeAccessible(void)
{
	return ChangePRangeAccessibility(fPageAddr * kPageSize, fIsLittle ? fNumOfPages * kPageSize: fSize, kMakeRangeAccessible);
}


NewtonErr
CPhys::changeVirtualMapping(VAddr inVAddr, size_t inVSize, PhysicalChange inAccess)
{
	return ChangeVRangeAccessibility(inVAddr, inVSize, fPageAddr * kPageSize, fIsLittle ? fNumOfPages * kPageSize: fSize, inAccess);
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C L i t t l e P h y s
	Represents < 1MByte of memory.
--------------------------------------------------------------------------------*/

CLittlePhys::CLittlePhys()
{
	fIsWhatever = NO;
	fIsLittle = YES;
}

