/*
	File:		MemArch.cc

	Contains:	Memory architecture code.

	Written by:	Newton Research Group.
*/


#include "MemArch.h"
#include "MemObjManager.h"
#include "PageManager.h"
#include "StackManager.h"
#include "ListIterator.h"
#include "OSErrors.h"

extern PAddr			gPrimaryTable;

/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

CMemArchManager *		gTheMemArchManager = NULL;			// 0C100D00

CObjectTable *			gTheMemArchObjTbl = NULL;			// 0C101164

CPageManager *			gThePageManager = NULL;				// 0C1016E8
CUMonitor				gThePageManagerMonitor;				// 0C1016EC

CPageTableManager *	gThePageTableManager = NULL;		// 0C1016FC
CUMonitor				gThePageTableManagerMonitor;		// 0C101700


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern "C" {
PAddr	GetPrimaryTablePhysBaseAfterGlobalsInited(void);
void	InitPageTable(void);
void	InitFaultMonitors(void);
}

/*------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Initialize the memory architecture core.
	Set up MMU page tables, a VM page manager and page table manager.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
InitMemArchCore(void)
{
	gPrimaryTable = GetPrimaryTablePhysBaseAfterGlobalsInited();

	gTheMemArchObjTbl = new CObjectTable;
	gTheMemArchObjTbl->init();

	gThePageManager = new CPageManager();
	gThePageTableManager = new CPageTableManager();

	InitPageTable();  // MMU page table

	gTheMemArchManager = new CMemArchManager();
	InitFaultMonitors();
}


/*------------------------------------------------------------------------------
	Initialize the memory architecture objects:
	page manager and page table manager monitors, domain manager
	and system stack manager.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
InitMemArchObjs(void)
{
	gThePageManagerMonitor.init(MemberFunctionCast(MonitorProcPtr, gThePageManager, &CPageManager::monitorProc), KByte*3/4, gThePageManager, kNoId, kIsntFaultMonitor, 'PMGR');
	gThePageTableManagerMonitor.init(MemberFunctionCast(MonitorProcPtr, gThePageTableManager, &CPageTableManager::monitorProc), KByte*3/4, gThePageTableManager, kNoId, kIsntFaultMonitor, 'PTBL');

	CUDomainManager::staticInit(gThePageManagerMonitor, gThePageTableManagerMonitor);

	return MakeSystemStackManager();
}


#pragma mark -
/*------------------------------------------------------------------------------
	C M e m A r c h M a n a g e r
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Constructor.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

CMemArchManager::CMemArchManager()
{
	fEnvironmentList = NULL;
	fDomainList = NULL;
	fDomainAccess = DefaultDCR();
}


/*------------------------------------------------------------------------------
	Add an environment to our list.
	Args:		inEnv		the environment to add
	Return:	--
------------------------------------------------------------------------------*/

void
CMemArchManager::addEnvironment(CEnvironment * inEnv)
{
	inEnv->fDomainAccess = DefaultDCR();
	inEnv->fNextEnvironment = fEnvironmentList;
	fEnvironmentList = inEnv;
}


/*------------------------------------------------------------------------------
	Remove an environment from our list.
	Args:		inEnv		the environment to remove
	Return:	--
------------------------------------------------------------------------------*/

void
CMemArchManager::removeEnvironment(CEnvironment * inEnv)
{
	CEnvironment * environment, * prevEnvironment;
	for (prevEnvironment = NULL, environment = fEnvironmentList; environment != NULL; prevEnvironment = environment, environment = environment->fNextEnvironment)
	{
		if (environment == inEnv)
		{
			if (prevEnvironment != NULL)
				prevEnvironment->fNextEnvironment = environment->fNextEnvironment;
			else
				fEnvironmentList = environment->fNextEnvironment;
			environment->f24 = true;
			environment->fNextEnvironment = NULL;
		}
	}
}


/*------------------------------------------------------------------------------
	Add a domain to our list.
	Args:		inDomain		the domain to add
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CMemArchManager::addDomain(CDomain * inDomain)
{
	inDomain->fNumber = NextAvailDomainInDCR(fDomainAccess);
	if (inDomain->fNumber < 0)
		return kOSErrOutOfDomains;

	inDomain->fNextDomain = fDomainList;
	fDomainList = inDomain;
	return noErr;
}


/*------------------------------------------------------------------------------
	Add a numbered domain to our list.
	Args:		inDomain		the domain to add
				inNumber		its number
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CMemArchManager::addDomainWithDomainNumber(CDomain * inDomain, ULong inNumber)
{
	inDomain->fNumber = GetSpecificDomainFromDCR(fDomainAccess, inNumber);
	if (inDomain->fNumber < 0)
		return kOSErrBadParameters;

	inDomain->fNextDomain = fDomainList;
	fDomainList = inDomain;
	return noErr;
}


/*------------------------------------------------------------------------------
	Remove a domain from our list.
	Args:		inDomain		the domain to remove
	Return:	--
------------------------------------------------------------------------------*/

void
CMemArchManager::removeDomain(CDomain * inDomain)
{
	CDomain * domain, * prevDomain;
	for (prevDomain = NULL, domain = fDomainList; domain != NULL; prevDomain = domain, domain = domain->fNextDomain)
	{
		if (domain == inDomain)
		{
			if (prevDomain != NULL)
				prevDomain->fNextDomain = domain->fNextDomain;
			else
				fDomainList = domain->fNextDomain;
			domain->fNumber = -1;
			domain->fNextDomain = NULL;
		}
	}
}


/*------------------------------------------------------------------------------
	Determine whether a range of addresses is in any domain.
	Args:		inRangeStart		the start address
				inRangeEnd			the end address
	Return:	true/false
------------------------------------------------------------------------------*/

bool
CMemArchManager::domainRangeIsFree(VAddr inRangeStart, VAddr inRangeEnd)
{
	CDomain * domain;
	for (domain = fDomainList; domain != NULL; domain = domain->fNextDomain)
	{
		if (domain->intersects(inRangeStart, inRangeEnd))
			return false;
	}
	return true;
}

