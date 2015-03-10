/*
	File:		MemArch.h

	Contains:	Memory architecture declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__MEMARCH_H)
#define __MEMARCH_H 1

#include "VirtualMemory.h"
#include "Environment.h"
#include "Semaphore.h"
#include "UserDomain.h"
#include "DoubleQ.h"


/*--------------------------------------------------------------------------------
	C M e m A r c h M a n a g e r

	The memory architecture manager maintains lists of environments and domains.
--------------------------------------------------------------------------------*/

class CMemArchManager
{
public:
					CMemArchManager();

	void			addEnvironment(CEnvironment * inEnv);
	void			removeEnvironment(CEnvironment * inEnv);
	NewtonErr	addDomain(CDomain * inDomain);
	NewtonErr	addDomainWithDomainNumber(CDomain * inDomain, ULong inDomNum);
	void			removeDomain(CDomain * inDomain);
	bool			domainRangeIsFree(VAddr inRangeStart, VAddr inRangeEnd);

private:
	CEnvironment *	fEnvironmentList;		// +00
	CDomain *		fDomainList;			// +04
	ULong				fDomainAccess;			// +08
};

extern CMemArchManager * gTheMemArchManager;



/*--------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
--------------------------------------------------------------------------------*/

extern void			InitMemArchCore(void);


#endif	/* __MEMARCH_H */
