/*
	File:		UserDomain.h

	Contains:	Address domain object code.

	Written by:	Newton Research Group.
*/

#if !defined(__USERDOMAIN_H)
#define __USERDOMAIN_H 1

#include "UserMonitor.h"
#include "Semaphore.h"
#include "List.h"
#include "PSSTypes.h"


/*------------------------------------------------------------------------------
	C U D o m a i n

	A CUDomain object defines a range of virtual addresses, and a monitor that
	should be called when an address within that range is accessed. ??
------------------------------------------------------------------------------*/

class CUDomain : public CUObject
{
public:
					CUDomain(ObjectId id = 0);

	NewtonErr	init(ObjectId inMonitorId, VAddr inRangeStart, size_t inRangeLength);
	NewtonErr	setFaultMonitor(ObjectId inMonitorId);
};

inline			CUDomain::CUDomain(ObjectId id) : CUObject(id)  { }


/*------------------------------------------------------------------------------
	C U D o m a i n M a n a g e r
------------------------------------------------------------------------------*/
struct ProcessorState
{
	ULong		fRegister[16];	// ??
	void *	f34;				// exception data
	VAddr		fAddr;			// +44 address being accessed
	ULong		f48;				// possibly ObjectId
	VAddr		f50;
	ObjectId	f54;
	ObjectId	f58;				// current task?
	ULong		f5C;				// flags?
	ULong		f60;
//	size +64
};

extern void		GetFaultState(ProcessorState * outState);
extern void		SetFaultState(ProcessorState * inState);


class CUDomainManager : public CUObject
{
public:
				CUDomainManager();
	virtual	~CUDomainManager();

	virtual NewtonErr fault(ProcessorState * inState) = 0;
	virtual NewtonErr userRequest(int inSelector, void * inData) = 0;
	virtual NewtonErr releaseRequest(int inSelector) = 0;

	static NewtonErr	staticInit(ObjectId inPageMonitorId, ObjectId inPageTableMonitorId);

	static NewtonErr	allocatePageTable(void);
	static NewtonErr	releasePageTable(VAddr);
	static NewtonErr	registerPageMonitor(void);
	static NewtonErr	release(ObjectId inArg1);

	NewtonErr	init(ObjectId inEnvironmentId, ULong inFaultMonName, ULong inPageMonName, size_t inFaultMonStackSize, size_t inPageMonStackSize);

	NewtonErr	faultMonProc(int inSelector, void * inData);
	NewtonErr	pageMonProc(int inSelector, void * inData);

	NewtonErr	addDomain(ObjectId inDomainId);
	NewtonErr	addDomain(ObjectId & outDomainId, VAddr inRangeStart, size_t inRangeLength);
	NewtonErr	get(ObjectId&, int);
	NewtonErr	getExternal(ObjectId&, int);
	NewtonErr	copyPhysPg(PSSId inToPage, PSSId inFromPage, ULong inSubPageMask);

	static NewtonErr	remember(ObjectId, VAddr, ULong, ObjectId, bool);
	static NewtonErr	rememberPermMap(ObjectId, VAddr, ULong, Perm);
	static NewtonErr	rememberPhysMap(ObjectId, VAddr, ULong, bool);
	static NewtonErr	rememberPhysMapRange(ObjectId, ULong, ULong, ULong, ULong, bool);

	NewtonErr	remember(VAddr, ULong, ObjectId, bool);
	NewtonErr	rememberPermMap(VAddr, ULong, Perm);
	NewtonErr	rememberPhysMap(VAddr, ULong, bool);
	NewtonErr	rememberPhysMapRange(ULong, ULong, ULong, ULong, bool);

	static NewtonErr	forget(ObjectId, VAddr, ULong);
	static NewtonErr	forgetPermMap(ObjectId, VAddr, ULong);
	static NewtonErr	forgetPhysMap(ObjectId, VAddr, ULong);
	static NewtonErr	forgetPhysMapRange(ObjectId, VAddr, ULong, ULong);

	NewtonErr	forget(VAddr, ULong);
	NewtonErr	forgetPermMap(VAddr, ULong);
	NewtonErr	forgetPhysMap(VAddr, ULong);
	NewtonErr	forgetPhysMapRange(VAddr, ULong, ULong);

	NewtonErr	releasePagesForFaultHandling(ULong, ULong);
	NewtonErr	releasePagesFromOtherMonitorsForFaultHandling(ULong, ULong);

protected:
	CULockingSemaphore	fLock;	// +04
	CUMonitor	fFaultMonitor;		// +10
	CUMonitor	fPageMonitor;		// +18
	bool			f20;					// +20
	ObjectId		f24;					// +24
	CList			fDomains;			// +28
// size +40

static CUMonitor	fDomainPageMonitor;
static CUMonitor	fDomainPageTableMonitor;
};


#endif	/* __USERDOMAIN_H */
