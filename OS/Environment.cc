/*
	File:		Environment.cc

	Contains:	Domain and environment implementation.

	Written by:	Newton Research Group.
*/

#include "Environment.h"
#include "MemArch.h"
#include "MemObjManager.h"
#include "KernelGlobals.h"
#include "PageManager.h"
#include "MemMgr.h"
#include "RAMInfo.h"
#include "SWI.h"
#include "OSErrors.h"

extern Heap					gKernelHeap;

size_t						gInRamStoreSize;			// 0C100F78


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

NewtonErr		InitDomainsAndEnvironments(void);
void				InitDomainPrimaryTable(ULong inStart, ULong inSize, ULong inDomainNumber);
void				ClearDomainPrimaryTable(ULong inStart, ULong inSize);
void				SetDomainRange(ULong inStart, ULong inSize, ULong inDomainNumber);
void				ClearDomainRange(ULong inStart, ULong inSize);

static NewtonErr	BuildDomainsAndHeaps(ObjectId inEnvId);
static NewtonErr	BuildEnvironments(void);


/*------------------------------------------------------------------------------
	L i c e n s e e   I n t e r f a c e
------------------------------------------------------------------------------*/

struct LicenseeDomainInfo
{
	ObjectId	domId;
	ULong		base;
	size_t	size;
	ULong		base2;
	CULockingSemaphore *	sem;
} gLicenseeDomainInfo;		// 0C1078C4

void
InitLicenseeDomain(void)
{
	NewtonErr err;
	XTRY
	{
		CDomainInfo info;
		XFAIL(err = MemObjManager::findDomainId('LicD', &gLicenseeDomainInfo.domId))
		XFAIL(err = MemObjManager::getDomainInfoByName('LicD', &info))
		XFAILNOT(gLicenseeDomainInfo.sem = new CULockingSemaphore, err = -1;)
		gLicenseeDomainInfo.sem->init();
		gLicenseeDomainInfo.base = info.base();
		gLicenseeDomainInfo.base2 = info.base();
		gLicenseeDomainInfo.size = info.size();
	}
	XENDTRY;
	XDOFAIL(err)
	{
		gLicenseeDomainInfo.domId = kNoId;
		gLicenseeDomainInfo.base = 0;
		gLicenseeDomainInfo.size = 0;
		gLicenseeDomainInfo.base2 = 0;
		gLicenseeDomainInfo.sem = NULL;
	}
	XENDFAIL;
}


#pragma mark -
/*------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Build the environments in the object table, the domains within them
	and the heaps within them.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
InitDomainsAndEnvironments(void)
{
	NewtonErr err;
	XTRY
	{
		ObjectId envId;
		XFAIL(err = MemObjManager::findEnvironmentId('krnl', &envId))
		XFAIL(err = BuildDomainsAndHeaps(envId))
		err = BuildEnvironments();
	}
	XENDTRY;
	return err;
}


/*------------------------------------------------------------------------------
	Iterate over the domains defined in the object table
	and build the heaps defined for those domains.
	Args:		inEnvId			environment in which to build (ie kernel)
	Return:	error code
------------------------------------------------------------------------------*/

static NewtonErr
BuildDomainsAndHeaps(ObjectId inEnvId)
{
	NewtonErr		err;			// sp30
	CDomainInfo		domain;		// sp04
	ObjectId			domainId;	// sp00
	CUEnvironment *	environment = new CUEnvironment(inEnvId);	// r6
	VAddr				heapAreaStart, heapAreaEnd;
	ULong				options;

	XTRY
	{
		// iterate over all domains in the object manager
		for (ArrayIndex index = 0; MemObjManager::getDomainInfo(index, &domain, &err); index++)
		{
			// build only non-kernel domains
			if (domain.name() != 'krnl')
			{
				// domain MUST be aligned on MByte
				XFAILIF((domain.base() & (kSectionSize-1)) != 0
					 ||  (domain.size() & (kSectionSize-1)) != 0, err = kOSErrBadParameters;)
				if (domain.hasGlobals()
				||  domain.hasHeap()
				||  domain.makeHeapDomain())
				{
//printf("\nBuildDomainsAndHeaps(%d) -- %c%c%c%c domain heap\n", inEnvId, domain.name() >> 24, domain.name() >> 16, domain.name() >> 8, domain.name());
					// the domain has a heap - create a domain for the heap
					XFAIL(err = NewHeapDomain(domain.base() / kSectionSize, domain.size() / kSectionSize, &domainId))
					// add it to our environment
					XFAIL(err = environment->add(domainId, domain.isReadOnly()/*manager*/, NO/*stack*/, NO/*heap*/))

					if (domain.hasGlobals())
					{
						// the domain has global variables - create a heap area for them
						options = (domain.isReadOnly() ? kNHOpts_ReadOnly : 0)
								  + (domain.isCacheable() ? 0 : kNHOpts_NotCacheable)
								  + (domain.exceptOnNoMem() ? kNHOpts_ExceptionOnNoMem : 0);
//printf("-- GLOBALS\n");
						XFAIL(err = NewHeapArea(domainId, domain.globalBase(), domain.globalSize(), options, &heapAreaStart, &heapAreaEnd))
						XFAILIF(domain.globalBase() != heapAreaStart, err = kOSErrBadParameters;)
						// innitialize the variables by copying their initial values
						XFAIL(err = LockHeapRange(heapAreaStart, heapAreaStart + domain.globalSize() - 1))
						Ptr	p = (Ptr) heapAreaStart;
						memmove(p, (const void *)domain.globalROMBase(), domain.globalInitSize());
						p += domain.globalInitSize();
						for (size_t size = domain.globalZeroSize(); size > 0; size--)
							*p++ = 0;
						XFAIL(err = UnlockHeapRange(heapAreaStart, heapAreaEnd))
					}
					if (domain.hasHeap())
					{
						// the domain has a heap
						//sp-10
						Heap		heap;		//sp0C
						VAddr		heapStart, heapEnd;	// sp04, sp00

						// can’t have persistent globals!
						XFAILIF(domain.isPersistent() && domain.hasGlobals(), err = kOSErrBadParameters;)
						if (domain.isPersistent())
						{
							// domain lives across reboot so reconstruct it
							//sp-24
							ObjectId					pageId;	//sp08 in prev stack frame
							CLittlePhys *			page;				// r4
							CPersistentDBEntry	persistentEntry;	// sp00
							// get the domain’s persistent entry from the object manager
							MemObjManager::findEntryByName(kMemObjPersistent, domain.name(), &persistentEntry);

							if ((page = (CLittlePhys *)persistentEntry.fPages.peek()) != NULL)
							{
								// it has saved pages
								options = (domain.isPersistent() ? kNHOpts_Persistent : 0)
										  + (domain.isReadOnly() ? kNHOpts_ReadOnly : 0)
										  + (domain.isCacheable() ? 0 : kNHOpts_NotCacheable);
//printf("-- PERSISTENT HEAP\n");
								XFAIL(err = NewHeapArea(domainId, persistentEntry.fBase, persistentEntry.fSize, options, &heapAreaStart, &heapAreaEnd))
								for ( ; page != NULL; page = (CLittlePhys *)persistentEntry.fPages.getNext(page))
								{
									if (page->f18 != kIllegalVAddr)
									{
										CPageManager::make(pageId, kNoId, page);
										XFAIL(err = AddPageMappingToDomain(domainId, page->f18, pageId))
									}
								}
								XFAIL(err)
								if (!domain.isHunkOMemory())
									ResurrectVMHeap(persistentEntry.fHeap);
							}
							else
							{
								if (domain.isHunkOMemory())
								{
									size_t	heapSize = domain.heapSize();
									if (domain.name() == 'rams')
										heapSize = gInRamStoreSize = InternalStoreInfo(1);

									options = (domain.isReadOnly() ? kNHOpts_ReadOnly : 0)
											  + (domain.isCacheable() ? 0 : kNHOpts_NotCacheable);
//printf("-- RAM STORE\n");
									XFAIL(err = NewHeapArea(domainId, domain.base() + 32*KByte, heapSize, options, &heapStart, &heapEnd))

									HeapPtr(heap)->start = heapStart;
									err = SetHeapLimits(heapStart, heapStart + heapSize - 1);
									err = LockHeapRange(heapStart, heapStart + heapSize - 1);
									err = UnlockHeapRange(heapStart, heapStart + heapSize - 1);
								}
								else
								{
									options = (domain.isPersistent() ? kNHOpts_Persistent : 0)
											  + (domain.isReadOnly() ? kNHOpts_ReadOnly : 0)
											  + (domain.isCacheable() ? 0 : kNHOpts_NotCacheable);
//printf("-- PERSISTENT VM HEAP\n");
									XFAIL(err = NewVMHeap(domainId, domain.heapSize(), &heap, options))
									XFAIL(err = GetHeapAreaInfo(HeapPtr(heap)->start, &heapStart, &heapEnd))
								}
								persistentEntry.fHeap = heap;
								persistentEntry.fBase = heapStart;
								persistentEntry.fSize = heapEnd - heapStart;
							}
							// register the fully built persistent entry
							MemObjManager::registerEntryByName(kMemObjPersistent, domain.name(), &persistentEntry);
						}
						else	// domain is not persistent
						{
							if (domain.isSegregated())
							{
								options = (domain.isPersistent() ? kNHOpts_Persistent : 0)
										  + (domain.isReadOnly() ? kNHOpts_ReadOnly : 0)
										  + (domain.isCacheable() ? 0 : kNHOpts_NotCacheable);
//printf("-- SEGREGATED VM HEAP\n");
								XFAIL(err = NewSegregatedVMHeap(domainId, domain.heapSize(), domain.handleHeapSize(), &heap, options))
							}
							else
							{
								options = (domain.isPersistent() ? kNHOpts_Persistent : 0)
										  + (domain.isReadOnly() ? kNHOpts_ReadOnly : 0)
										  + (domain.isCacheable() ? 0 : kNHOpts_NotCacheable);
//printf("-- VM HEAP\n");
								XFAIL(err = NewVMHeap(domainId, domain.heapSize(), &heap, options))
							}
							// register the fully built heap
							XFAIL(err = MemObjManager::registerHeapRef(domain.name(), heap))
						}
					}
					// remove the domain from our (kernel) environment
					// it was only a temporary home
					XFAIL(err = environment->remove(domainId))
				}
				else	// domain has no heaps
				{
					CUDomain	realDomain;
					XFAIL(err = realDomain.init(kNoId, domain.base(), domain.size()))
					realDomain.denyOwnership();
					domainId = realDomain;
				}
				// register the fully built domain
				XFAIL(err = MemObjManager::registerDomainId(domain.name(), domainId))
			}
		}
		XFAIL(err)

		CPersistentDBEntry *	persistentEntry;	// sp14
		CDomainDBEntry		domainEntry;		// sp0C
		ObjectId				pageId;			//sp08
		CLittlePhys *		page;
		for (ArrayIndex index = 0; MemObjManager::getPersistentRef(index, &persistentEntry, &err); index++)
		{
			if ((persistentEntry->fFlags & 0x40) == 0
			&&  persistentEntry->fName != 'emty')
			{
				if (MemObjManager::getDomainInfo((persistentEntry->fFlags & 0xFF00) >> 8, &domain, &err)
				&&  persistentEntry->fDomainName == domain.name())
				{
					XFAIL(err = MemObjManager::findEntryByName(kMemObjDomain, domain.name(), &domainEntry))
					domainId = domainEntry.fId;
					XFAIL(err = environment->add(domainId, NO/*manager*/, NO/*stack*/, NO/*heap*/))
					options = kNHOpts_Persistent
							  + (domain.isReadOnly() ? kNHOpts_ReadOnly : 0)
							  + (domain.isCacheable() ? 0 : kNHOpts_NotCacheable);
//printf("-- PERSISTENT ENTRY\n");
					NewHeapArea(domainId, persistentEntry->fBase, persistentEntry->fSize, options, &heapAreaStart, &heapAreaEnd);
					// no err check?

					if ((page = (CLittlePhys *)persistentEntry->fPages.peek()) != NULL)
					{
						for ( ; page != NULL; page = (CLittlePhys *)persistentEntry->fPages.getNext(page))
						{
							if (page->f18 != kIllegalVAddr)
							{
								CPageManager::make(pageId, kNoId, page);
								XFAIL(err = AddPageMappingToDomain(domainId, page->f18, pageId))
							}
						}
						XFAIL(err)
						if (!domain.isHunkOMemory())
							ResurrectVMHeap(persistentEntry->fHeap);
					}
					else
					{
						persistentEntry->init('emty');
						persistentEntry->fFlags &= ~0x40;
					}
					XFAIL(err = environment->remove(domainId))
				}
				for (page = (CLittlePhys *)persistentEntry->fPages.peek(); page != NULL; page = (CLittlePhys *)persistentEntry->fPages.getNext(page))
				{
					if (page->f18 == kIllegalVAddr)
						gPageTracker->put(page);
				}
			}
		}
	}
	XENDTRY;

	// no need for the temporary environment any more
	if (environment != NULL)
		delete environment;
	// errors are really, really serious
	if (err != noErr)
		Reboot(kOSErrSorrySystemFailure, kRebootMagicNumber);

	CLittlePhys *			page;
	CPersistentDBEntry *	persistentEntry;
	for (ArrayIndex index = 0; MemObjManager::getPersistentRef(index, &persistentEntry, &err); index++)
	{
		for (page = (CLittlePhys *)persistentEntry->fPages.peek(); page != NULL; page = (CLittlePhys *)persistentEntry->fPages.getNext(page))
		{
			if (page->f18 == kIllegalVAddr)
				gPageTracker->put(page);
		}
	}
	
	return err;
}


/*------------------------------------------------------------------------------
	Iterate over the environments defined in the object table
	and build those environments.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

static NewtonErr
BuildEnvironments(void)
{
	NewtonErr err;

	XTRY
	{
		CEnvironmentInfo env;		// sp00
		for (ArrayIndex index = 0; MemObjManager::getEnvironmentInfo(index, &env, &err); index++)
		{
			ULong				envName = env.name();	// r6
			//sp-0C
			Heap				heap = NULL;
			CUEnvironment	environment;
			if (envName == 'krnl')
			{
				ObjectId	envId;
				XFAIL(err = MemObjManager::findEnvironmentId(envName, &envId))
				environment = envId;
			}
			else
			{
				if (env.defaultHeap() == 'krnl')
				{
					heap = gKernelHeap;
				}
				else if (env.defaultHeap() != 0)
				{
					XFAIL(err = MemObjManager::findHeapRef(env.defaultHeap(), &heap))
				}
				XFAIL(err = environment.init(heap))
				environment.denyOwnership();
			}

			ULong			domainName;	// sp0C
			ObjectId		domainId;	// sp08
			bool			isManager;	// sp04
			NewtonErr	domainErr;	// sp00
			for (ArrayIndex domainIndex = 0; env.domains(domainIndex, &domainName, &isManager, &domainErr); domainIndex++)
			{
				XFAIL(err = MemObjManager::findDomainId(domainName, &domainId))
				XFAIL(err = environment.add(domainId, isManager, env.defaultHeapDomain() == domainName, env.defaultStackDomain() == domainName))
			}
			XFAIL(err)
			XFAIL(domainErr)
			XFAIL(err = MemObjManager::registerEnvironmentId(envName, environment))
		}
	}
	XENDTRY;

	return err;
}

#pragma mark -

void
InitDomainPrimaryTable(ULong inStart, ULong inSize, ULong inDomainNumber)
{
	SetDomainRange(inStart, inSize, inDomainNumber);
}


void
ClearDomainPrimaryTable(ULong inStart, ULong inSize)
{
	ClearDomainRange(inStart, inSize);
}


void
SetDomainRange(ULong inStart, ULong inSize, ULong inDomainNumber)
{
	GenericSWI(kSetDomainRange, inStart, inSize, inDomainNumber);
}


void
ClearDomainRange(ULong inStart, ULong inSize)
{
	GenericSWI(kClearDomainRange, inStart, inSize);
}


/*------------------------------------------------------------------------------
	Set the current task’s environment;
	return the previous one.
	Args:		inEnvId			id of new environment
				outEnvId			id of old environment
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
SetEnvironment(ObjectId inEnvId, ObjectId * outEnvId)
{
	if (IsSuperMode())
	{
		CEnvironment * env;
		if ((env = (CEnvironment *)IdToObj(kEnvironmentType, inEnvId)) == NULL)
			return kOSErrBadObjectId;

		CEnvironment * oldEnv = gCurrentTask->fEnvironment;
		oldEnv->decrRefCount();
		*outEnvId = *oldEnv;
		gCurrentTask->fEnvironment = env;
		env->incrRefCount();
		return noErr;
	}

	return GenericWithReturnSWI(kSetEnvironment, inEnvId, 0, 0, (OpaqueRef *)outEnvId, 0, 0);
}


/*------------------------------------------------------------------------------
	Return the id of the current task’s environment.
	Args:		outEnvId			id of environment
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
GetEnvironment(ObjectId * outEnvId)
{
	if (IsSuperMode())
	{
		CEnvironment * env = gCurrentTask->fEnvironment;
		*outEnvId = *env;
		return noErr;
	}

	return GenericWithReturnSWI(kGetEnvironment, 0, 0, 0, (OpaqueRef *)outEnvId, 0, 0);
}


/*------------------------------------------------------------------------------
	Environment Add/Remove/Test glue (eventually calls GenericSWI)
------------------------------------------------------------------------------*/

NewtonErr
AddDomainToEnvironment(ObjectId inEnvId, ObjectId inDomId, ULong inFlags)
{
	if (IsSuperMode())
	{
		CEnvironment * env;
		CDomain * dom;
		if ((env = (CEnvironment *)IdToObj(kEnvironmentType, inEnvId)) == NULL
		 || (dom = (CDomain *)IdToObj(kDomainType, inDomId)) == NULL)
			return kOSErrBadParameters;
		return env->add(dom, (inFlags & 0x04) != 0, (inFlags & 0x02) != 0, (inFlags & 0x01) != 0);
	}

	return GenericSWI(kAddDomainToEnvironment, inEnvId, inDomId, inFlags);
}


NewtonErr
RemoveDomainFromEnvironment(ObjectId inEnvId, ObjectId inDomId)
{
	if (IsSuperMode())
	{
		CEnvironment * env;
		CDomain * dom;
		if ((env = (CEnvironment *)IdToObj(kEnvironmentType, inEnvId)) == NULL
		 || (dom = (CDomain *)IdToObj(kDomainType, inDomId)) == NULL)
			return kOSErrBadParameters;
		return env->remove(dom);
	}

	return GenericSWI(kRemoveDomainFromEnvironment, inEnvId, inDomId);
}


NewtonErr
EnvironmentHasDomain(ObjectId inEnvId, ObjectId inDomId, bool * outHasDomain, bool * outIsManager)
{
	if (IsSuperMode())
	{
		CEnvironment * env;
		CDomain * dom;
		if ((env = (CEnvironment *)IdToObj(kEnvironmentType, inEnvId)) == NULL
		 || (dom = (CDomain *)IdToObj(kDomainType, inDomId)) == NULL)
			return kOSErrBadObjectId;
		return env->hasDomain(dom, outHasDomain, outIsManager);
	}

	return GenericWithReturnSWI(kEnvironmentHasDomain, inEnvId, inDomId, 0, (OpaqueRef *)outHasDomain, (OpaqueRef *)outIsManager, 0);
}


#pragma mark -
/*------------------------------------------------------------------------------
	Domain Control Register (DCR) manipulation

	The DCR holds 2 bits per 16 domains = 32 bits.
------------------------------------------------------------------------------*/

ULong
DefaultDCR(void)
{
	return (kClientDomain << kDomainBits)
			| kClientDomain;	// 0x00000005;	// 2 client domains
}


ULong
AddClientToDCR(ULong inBits, int inDomNum)
{
	if (inDomNum >= 0)
		inBits |= (kClientDomain << (inDomNum*kDomainBits));
	return inBits;
}


ULong
AddManagerToDCR(ULong inBits, int inDomNum)
{
	if (inDomNum >= 0)
		inBits |= (kManagerDomain << (inDomNum*kDomainBits));
	return inBits;
}


ULong
RemoveFromDCR(ULong inBits, int inDomNum)
{
	if (inDomNum >= 0)
		inBits &= ~(kDomainMask << (inDomNum*kDomainBits));
	return inBits;
}


int
GetSpecificDomainFromDCR(ULong & ioBits, int inDomNum)
{
	ULong	mask = kDomainMask << (inDomNum*kDomainBits);
	if ((ioBits & mask) == 0)
	{
		ioBits |= mask;
		return inDomNum;
	}
	return -1;
}


int
NextAvailDomainInDCR(ULong & ioBits)
{
	for (int number = 0; number < kNumOfDomains; number++)
	{
		ULong	mask = kDomainMask << (number*kDomainBits);
		if ((ioBits & mask) == 0)
		{
			ioBits |= mask;
			return number;
		}
	}
	return -1;
}


#pragma mark -
/*------------------------------------------------------------------------------
	F a u l t   M o n i t o r s
------------------------------------------------------------------------------*/

struct FaultMonitorDomain
{
	ObjectId	fMonitor;
	ObjectId	fDomain;
} gFault[kNumOfDomains];		// 0C105FF4
/* {
	{ 0x0000, 0x0000 },	//  0
	{ 0x0000, 0x0000 },
	{ 0x0000, 0x1025 },
	{ 0x121A, 0x1355 },
	{ 0x121A, 0x13A5 },	//  4
	{ 0x121A, 0x1415 },
	{ 0x121A, 0x1465 },
	{ 0x151A, 0x1475 },
	{ 0x1D4A, 0x1485 },	//  8
	{ 0x1D4A, 0x1495 },
	{ 0x0000, 0x14A5 },
	{ 0x0000, 0x0000 },
	{ 0x0000, 0x0000 },	// 12
	{ 0x0000, 0x0000 },
	{ 0x0000, 0x0000 },
	{ 0x0000, 0x0000 },
}; */


extern "C" void
InitFaultMonitors(void)
{
	for (ArrayIndex i = 0; i < kNumOfDomains; ++i)
	{
		gFault[i].fDomain = kNoId;
		gFault[i].fMonitor = kNoId;
	}
}


void
RegisterFaultMonitor(int inDomainNumber, ObjectId inDomainId, ObjectId inMonitorId)
{
	EnterAtomic();
	gFault[inDomainNumber].fDomain = inDomainId;
	if (inMonitorId)
		gFault[inDomainNumber].fMonitor = inMonitorId;
	ExitAtomic();
}


void
DeregisterFaultMonitorByDomainNumber(int inDomainNumber)
{
	EnterAtomic();
	gFault[inDomainNumber].fDomain = kNoId;
	gFault[inDomainNumber].fMonitor = kNoId;
	ExitAtomic();
}


ObjectId
GetDomainFromDomainNumber(int inDomainNumber)
{
	return gFault[inDomainNumber].fDomain;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C D o m a i n I n f o
------------------------------------------------------------------------------*/

void
CDomainInfo::initDomainInfo(ULong inName, ULong in04, VAddr inBase, size_t inSize)
{
	fName = inName;
	f04 = in04;
	fBase = inBase;
	fSize = inSize;
}


void
CDomainInfo::initGlobalInfo(VAddr inBase, VAddr inROMBase, size_t inInitSize, size_t inZeroSize)
{
	fGlobalBase = inBase;
	fGlobalROMBase = inROMBase;
	fGlobalInitSize = inInitSize;
	fGlobalZeroSize = inZeroSize;
}


void
CDomainInfo::initHeapInfo(size_t inSize, size_t inHandleSize, ULong inFlags)
{
	fHeapSize = inSize;
	fHandleHeapSize = inHandleSize;
	fHeapFlags = inFlags;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C D o m a i n
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Constructor.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

CDomain::CDomain()
{
	fStart = 0;
	fLength = 0;
	fFaultMonitor = kNoId;
	fNumber = -1;
	fNextDomain = NULL;
}


/*------------------------------------------------------------------------------
	Destructor.
	Remove the domain from the manager.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

CDomain::~CDomain()
{
	DeregisterFaultMonitorByDomainNumber(fNumber);
	ClearDomainPrimaryTable(fStart, fLength);
	gTheMemArchManager->removeDomain(this);
}


/*------------------------------------------------------------------------------
	Initialize the domain.
	Args:		inMonitor		id of fault monitor
				inRangeStart	start of domain
				inRangeLength	size of domain
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CDomain::init(ObjectId inMonitor, VAddr inRangeStart, size_t inRangeLength)
{
	NewtonErr err;

	fStart = inRangeStart;
	fLength = inRangeLength;
	VAddr rangeEnd = inRangeStart + inRangeLength - 1;

	if (inRangeStart > rangeEnd					// range must be non-zero length
#if defined(correct)
	 || !((inRangeStart & (kSectionSize-1)) == 0		// must be section aligned
	   && (inRangeLength & (kSectionSize-1)) == 0)	// and an exact number of sections
#endif
	 || !gTheMemArchManager->domainRangeIsFree(inRangeStart, rangeEnd))	// and must be free
		return kOSErrIllFormedDomain;

	err = gTheMemArchManager->addDomain(this);
	InitDomainPrimaryTable(inRangeStart, inRangeLength, fNumber);
	setFaultMonitor(inMonitor);
	return err;
}


/*------------------------------------------------------------------------------
	Initialize a specific domain.
	Args:		inMonitor		id of fault monitor
				inRangeStart	start of domain
				inRangeLength	size of domain
				inDomNum			the domain number
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CDomain::initWithDomainNumber(ObjectId inMonitor, VAddr inRangeStart, size_t inRangeLength, ULong inDomNum)
{
	NewtonErr err;

	fStart = inRangeStart;
	fLength = inRangeLength;
	VAddr rangeEnd = inRangeStart + inRangeLength - 1;

	if (inRangeStart > rangeEnd					// range must be non-zero length
#if defined(correct)
	 || !((inRangeStart & (kSectionSize-1)) == 0		// must be section aligned
	   && (inRangeLength & (kSectionSize-1)) == 0)	// and an exact number of sections
#endif
	 || !gTheMemArchManager->domainRangeIsFree(inRangeStart, rangeEnd))	// and must be free
		return kOSErrIllFormedDomain;

	err = gTheMemArchManager->addDomainWithDomainNumber(this, inDomNum);
	setFaultMonitor(inMonitor);
	return err;
}


/*------------------------------------------------------------------------------
	Set the domain’s fault monitor.
	Args:		inMonitor		id of fault monitor
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CDomain::setFaultMonitor(ObjectId inMonitor)
{
	fFaultMonitor = inMonitor;
	RegisterFaultMonitor(fNumber, fId, inMonitor);
	return noErr;
}


/*------------------------------------------------------------------------------
	Determine whether the given address range intersects with this domain.
	Args:		inRangeStart	start of range
				inRangeEnd		end of range
	Return:	error code
------------------------------------------------------------------------------*/

bool
CDomain::intersects(VAddr inRangeStart, VAddr inRangeEnd)
{
	return fStart <= inRangeEnd && (fStart + fLength) > inRangeStart;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C E n v i r o n m e n t I n f o
------------------------------------------------------------------------------*/

void
CEnvironmentInfo::init(ULong inName, ObjectId inId, ULong inHeap, ObjectId inHeapDomain, ObjectId inStackDomain)
{
	fName = inName;
	fId = inId;
	fDefaultHeap = inHeap;
	fDefaultHeapDomain = inHeapDomain;
	fDefaultStackDomain = inStackDomain;
}


bool
CEnvironmentInfo::domains(ArrayIndex index, ULong * outName, bool * outIsManager, NewtonErr * outError)
{
	return MemObjManager::getEnvDomainName(fName, index, outName, outIsManager, outError);
}


#pragma mark -
/*------------------------------------------------------------------------------
	C E n v i r o n m e n t
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Destructor.
	Remove the environment from the manager.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

CEnvironment::~CEnvironment()
{
	gTheMemArchManager->removeEnvironment(this);
}


/*------------------------------------------------------------------------------
	Initialize.
	Add the environment to the manager.
	Args:		inHeap		heap for this environment
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CEnvironment::init(Heap inHeap)
{
	fHeap = inHeap;
	fStackDomainId = kNoId;
	fHeapDomainId = kNoId;
	fRefCount = 0;
	f24 = NO;
	gTheMemArchManager->addEnvironment(this);
	return noErr;
}


/*------------------------------------------------------------------------------
	Add a domain to this environment.
	Args:		inDomain		the domain to add
				inIsManager	is this domain a manager?
				inHasStack	does this domain have a stack?
				inHasHeap	does this domain have a heap?
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CEnvironment::add(CDomain * inDomain, bool inIsManager, bool inHasStack, bool inHasHeap)
{
	if (inIsManager)
		fDomainAccess = AddManagerToDCR(fDomainAccess, inDomain->fNumber);
	else
		fDomainAccess = AddClientToDCR(fDomainAccess, inDomain->fNumber);
	if (inHasStack)
		fStackDomainId = *inDomain;
	if (inHasHeap)
		fHeapDomainId = *inDomain;
	return noErr;
}


/*------------------------------------------------------------------------------
	Remove a domain from this environment.
	Args:		inDomain		the domain to remove
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CEnvironment::remove(CDomain * inDomain)
{
	fDomainAccess = RemoveFromDCR(fDomainAccess, inDomain->fNumber);
	return noErr;
}


/*------------------------------------------------------------------------------
	Determine whether this environment has the given domain.
	Args:		inDomain			the domain to add
				outHasDomain	the result of the test
				outIsManager	in addition, whether the domain is a manager
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CEnvironment::hasDomain(CDomain * inDomain, bool * outHasDomain, bool * outIsManager)
{
	switch ((fDomainAccess >> (inDomain->fNumber * kDomainBits)) & kDomainMask)
	{
	case 0x01:
		*outHasDomain = YES;
		*outIsManager = NO;
		break;
	case 0x03:
		*outHasDomain = YES;
		*outIsManager = YES;
		break;
	default:
		*outHasDomain = NO;
		*outIsManager = NO;
		break;
	}
	return noErr;
}


/*------------------------------------------------------------------------------
	Increment the reference count for this environment.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CEnvironment::incrRefCount(void)
{
	fRefCount++;
}


/*------------------------------------------------------------------------------
	Decrement the reference count for this environment.
	Args:		--
	Return:	YES if the environment is no longer referenced
------------------------------------------------------------------------------*/

bool
CEnvironment::decrRefCount(void)
{
	long count = fRefCount--;
	if (f24 && count == 0)
	{
		fOwnerId = kNoId;
		return YES;
	}
	return NO;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C U E n v i r o n m e n t
------------------------------------------------------------------------------*/

NewtonErr
CUEnvironment::init(Heap inHeap)
{
	ObjectMessage	msg;
	msg.request.environment.heap = inHeap;
	return makeObject(kObjectEnvironment, &msg, MSG_SIZE(1));
}


NewtonErr
CUEnvironment::add(ObjectId inDomId, bool inIsManager, bool inHasStack, bool inHasHeap)
{
	ULong	flags = 0;

	if (inIsManager)
		flags |= 0x04;
	if (inHasStack)
		flags |= 0x02;
	if (inHasHeap)
		flags |= 0x01;

	return AddDomainToEnvironment(fId, inDomId, flags);
}


NewtonErr
CUEnvironment::remove(ObjectId inDomId)
{
	return RemoveDomainFromEnvironment(fId, inDomId);
}


NewtonErr
CUEnvironment::hasDomain(ObjectId inDomId, bool * outHasDomain, bool * outIsManager)
{
	return EnvironmentHasDomain(fId, inDomId, outHasDomain, outIsManager);
}

