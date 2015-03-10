/*
	File:		SWInterrupt.cc

	Contains:	Generic SWI  handler.
					The GenericSWI can be handled in C/C++.

	Written by:	Newton Research Group
*/

#include "SharedTypes.h"
#include "SharedMem.h"
#include "VirtualMemory.h"
#include "KernelTasks.h"
#include "KernelGlobals.h"
#include "UserGlobals.h"
#include "UserTasks.h"
#include "Scheduler.h"
#include "SWI.h"
#include "OSErrors.h"

/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

extern CTime	gTaskTimeStart;
extern ULong	gFIQInterruptOverhead;
extern ULong	gIRQInterruptOverhead;


/*----------------------------------------------------------------------
	Handle a generic SWI interrupt request.
	The generic SWI has a sub-selector.
	Args:		inSelector
				inArg1
				inArg2
				inArg3
				inArg4
	Return:	result required - usually an error code
----------------------------------------------------------------------*/

//	Tasks
extern NewtonErr	GiveObject(ObjectId inId, ObjectId inToTaskId);
extern NewtonErr	AcceptObject(ObjectId inId);
extern NewtonErr	ResetAccountTimeKernelGlue(void);
extern NewtonErr	GetNextTaskIdKernelGlue(ObjectId inLastId, ObjectId * outId);

extern "C" NewtonErr	PortResetKernelGlue(ObjectId inId, ULong inSenderFlags, ULong inReceiverFlags);

//	Virtual Memory
extern NewtonErr	RememberPhysMapping(void);
extern NewtonErr	RememberPhysMapping(ULong inDomain, VAddr inVAddr, ObjectId inPageId, bool inCacheable);
extern NewtonErr	RememberPermMapping(ULong inDomain, VAddr inVAddr, size_t inSize, Perm inPerm);
extern NewtonErr	RememberMapping(ULong inDomain, VAddr inVAddr, ULong inPerm, ObjectId inPageId, bool inCacheable);

extern NewtonErr	ForgetPhysMapping(void);
extern NewtonErr	ForgetPhysMapping(ULong inArg1, VAddr inArg2, ObjectId inPageId);
extern NewtonErr	ForgetPermMapping(ULong inDomain, VAddr inVAddr, size_t inSize);
extern NewtonErr	ForgetMapping(ULong inDomain, VAddr inVAddr, ObjectId inPageId);

extern "C" NewtonErr	AddPgPAndPermWithPageTable(PAddr inDescriptors, VAddr inVAddr, ULong inPerm, PAddr inPhysAddr, bool inCacheable);
extern "C" NewtonErr	RemovePMappings(PAddr inBase, size_t inSize);

extern "C" PAddr		GetPrimaryTablePhysBaseAfterGlobalsInited(void);
extern "C" NewtonErr	AddPageTable(ULong inArg1, ULong inArg2, ULong inArg3);
extern "C" NewtonErr	RemovePageTable(ULong inArg1, ULong inArg2);
extern "C" NewtonErr	ReleasePageTable(ULong inArg1);

extern NewtonErr	ReleasePage(ObjectId inPageId);

//	Physical Memory
extern size_t		PhysSize(ObjectId inId);
extern PAddr		PhysBase(ObjectId inId);
extern ULong		PhysAlign(ObjectId inId);
extern bool			PhysReadOnly(ObjectId inId);
extern NewtonErr	CopyPhysPgGlue(ObjectId inFromId, ObjectId inToId, ULong inSubPageMask);
extern NewtonErr	InvalidatePhys(ObjectId inId);
extern NewtonErr	MakePhysInaccessible(ObjectId inId);
extern NewtonErr	MakePhysAccessible(ObjectId inId);
extern NewtonErr	ChangeVirtualMapping(void);
extern "C" PAddr	PrimVtoP(VAddr inAddr);

//	Domains & Environments
extern "C" void	PrimSetDomainRange(PAddr inVAddr, size_t inSize, ULong inDomain);
extern "C" void	PrimClearDomainRange(PAddr inVAddr, size_t inSize);
extern NewtonErr	SetEnvironment(ObjectId inEnvId, ObjectId * outEnvId);
extern NewtonErr	GetEnvironment(ObjectId * outEnvId);
extern NewtonErr	AddDomainToEnvironment(ObjectId inEnvId, ObjectId inDomId, ULong inFlags);
extern NewtonErr	RemoveDomainFromEnvironment(ObjectId inEnvId, ObjectId inDomId);
extern NewtonErr	EnvironmentHasDomain(ObjectId inEnvId, ObjectId inDomId, bool * outHasDomain, bool * outIsManager);

//	Semaphores
extern NewtonErr	SemGroupSetRefCon(ObjectId inSemGroupId, void * inRefCon);
extern NewtonErr	SemGroupGetRefCon(ObjectId inSemGroupId, void ** outRefCon);

// Timer
extern NewtonErr	PrimRegisterDelayedFunction(ObjectId inMsgId, ULong inDelay, void * inData, TimeoutProcPtr inProc);
extern NewtonErr	PrimRemoveDelayedFunction(ObjectId inMsgId);

extern NewtonErr	RealTimeClockDispatch(void);
extern NewtonErr	PrimGetMemObjInfo(void);
extern NewtonErr	PrimGetNetworkPersistentInfo(void);
extern NewtonErr	PrimGetPatchInfo(void);
extern NewtonErr	ResetRebootReason(void);
extern void			Restart(void);
extern NewtonErr	ClearFIQAtomic(void);

//	Tablet
extern NewtonErr	SetTabletCalibrationDataSWI(void);
extern NewtonErr	GetTabletCalibrationDataSWI(void);

//	Debugger
extern void			SRegisterPackageWithDebugger(ULong, ULong);
extern void			SRegisterLoadedCodeWithDebugger(ULong, ULong, ULong);
extern void			SDeregisterLoadedCodeWithDebugger(ULong);
extern void			SInformDebuggerMemoryReloaded(ULong, ULong);

//	Patching
extern void			BackupPatch(ULong);

//	Power
extern NewtonErr	PowerOffSystemKernelGlue(void);
extern NewtonErr	PauseSystemKernelGlue(void);

//	MMU
extern "C" NewtonErr	CleanPageInIandDCacheSWIGlue(ULong);
extern "C" void		CleanDCandFlushICSWIGlue(void);
extern "C" NewtonErr	CleanPageInDCSWIGlue(ULong);
extern "C" NewtonErr	CleanRangeInDCSWIGlue(ULong, ULong);

struct CopyPageAfterStackCollisionParams;
extern void			CopyPagesAfterStackCollided(CopyPageAfterStackCollisionParams * inParams);

extern "C" long
GenericSWIHandler(int inSelector, OpaqueRef inArg1, OpaqueRef inArg2, OpaqueRef inArg3, OpaqueRef inArg4)
{
	long result = noErr;
/*
	r0 = inArg1 & 0x0F;
	r1 = TRUNC(inArg2, kPageSize);
	r2 = gObjectTable;
	r3 = 1;
	r4 = inArg1;
	r5 = inArg2;
	r6 = inArg3;
	r12 = inArg4;
	r7 = 0;	// err
	r8 = gCurrentTask;
	r9 = 0;
	r10 = kOSErrTaskDoesNotExist;
*/
	switch (inSelector)
	{
	case kTaskSetGlobals:	// 110
		gCurrentTask->fGlobals = (void *)inArg1;
		break;

//	TaskGiveObject(ObjectId inId, ObjectId inAssignToTaskId)
	case kTaskGiveObject:	// 113
		result = GiveObject((ObjectId) inArg1, (ObjectId) inArg2);
		break;

//	TaskAcceptObject(ObjectId inId)
	case kTaskAcceptObject:	// 117
		result = AcceptObject((ObjectId) inArg1);
		break;

//	GetGlobalTime(void) -> CTime
	case kGetGlobalTime:	// 120
		{
			CTime	theTime;
			if (inArg1 == kNoId)
				theTime = GetGlobalTime();
			else
			{
				// we want task time; inArg1 is task id
				CTask *	task = (CTask *)IdToObj(kTaskType, inArg1);
				if (task != NULL)
				{
					if (task == gCurrentTask)
					{
						theTime = GetGlobalTime() - gTaskTimeStart - CTime(gIRQInterruptOverhead + gFIQInterruptOverhead);
						task->fTaskTime = task->fTaskTime + theTime;
					}
					else
						theTime = task->fTaskTime;
				}
				else
				{
					theTime = CTime(0);
					result = kOSErrTaskDoesNotExist;
				}
			}
			gCurrentTask->fRegister[kReturnParm1] = theTime;
		}
		break;

//	ResetAccountTime(void)
	case kTaskResetAccountTime:	// 207
		result = ResetAccountTimeKernelGlue();
		break;

	case kTaskGetNextId:	// 209
		{
		ObjectId nextId;
		result = GetNextTaskIdKernelGlue((ObjectId) inArg1, &nextId);
		gCurrentTask->fRegister[kReturnParm1] = nextId;
		}
		break;

	case kForgetPhysMapping:	// 214
		result = ForgetPhysMapping(inArg1, inArg2, (ObjectId) inArg3);
		break;

	case kForgetPermMapping:	// 221
		result = ForgetPermMapping(inArg1, (VAddr) inArg2, (size_t) inArg3);
		break;

//	CUDomainManager::forget(ObjectId inArg1, VAddr inArg2, ULong inArg3)
	case kForgetMapping:	// 265
		result = ForgetMapping((ObjectId) inArg1, (VAddr) inArg2, (ULong) inArg3);
		break;

	case 10:	// 226
		result = RememberPhysMapping((ObjectId) inArg1, TRUNC(inArg2, kPageSize), inArg3, inArg2 & 0x01);
		break;

	case 11:	// 233
		result = RememberPermMapping((ObjectId) inArg1, TRUNC(inArg2, kPageSize), inArg3, (Perm) (inArg2 & 0x03));
		break;

	case 12:	// 238
		result = RememberMapping(inArg1, (VAddr) TRUNC(inArg2, kPageSize), inArg2 & 0xFF, (ObjectId) inArg3, (inArg2 & 0x0100) != 0);
	break;

	case 13:	// 270
		result = AddPageTable(inArg1, inArg2, inArg3);
		break;

	case 14:	// 275
		result = RemovePageTable(inArg1, inArg2);
		break;

//	CUPageManager::release(ObjectId inId)
	case kPMRelease:	// 279
		result = ReleasePage((ObjectId) inArg1);
		break;

//	CopyPhysPgGlue(ObjectId inFromId, ObjectId inToId, ULong inSubPageMask)
	case 18:	// 285
		result = CopyPhysPgGlue((ObjectId) inArg1, (ObjectId) inArg2, inArg3);
		break;

//	InvalidatePhys(ObjectId inId)
	case 19:	// 290
		result = InvalidatePhys((ObjectId) inArg1);
		break;

//	size_t PhysSize(ObjectId inId)
	case 20:	// 301
		result = PhysSize((ObjectId) inArg1);
		break;

//	PAddr PhysBase(ObjectId inId)
	case 21:	// 304
		result = PhysBase((ObjectId) inArg1);
		break;

//	ULong PhysAlign(ObjectId inId)
	case 22:	// 307
		result = PhysAlign((ObjectId) inArg1);
		break;

//	bool PhysReadOnly(ObjectId inId)
	case 23:	// 310
		result = PhysReadOnly((ObjectId) inArg1);
		break;

	case 26:	// 313
		{
			CTask *	task;
			while ((task = (CTask *)gBlockedOnMemory->remove()) != NULL)
				ScheduleTask(task);
		}
		break;

//	Yield(ObjectId toTaskId)
	case kTaskYield:	// 325
		{
			CTask *	task = (CTask *)IdToObj(kTaskType, (ObjectId) inArg1);
			if (task == NULL)
				result = kOSErrBadObject;
			else
			{
				gKernelScheduler->setCurrentTask(task);
				WantSchedule();
			}
		}
		break;

//	Reboot(NewtonErr inError, ULong inRebootType, bool inSafe)
	case kReboot:	// 345
	// call again in svc mode
		Reboot((NewtonErr) inArg1, (ULong) inArg2, (bool) inArg3);
		break;

//	Restart(void)
	case kRestart:	// 350
	// call again in svc mode
		Restart();
		break;

//	PrimRegisterDelayedFunction(ObjectId inMsgId, ULong inDelay, void * inData, TimeoutProcPtr inProc)
	case 30:	// 386
		result = PrimRegisterDelayedFunction((ObjectId) inArg1, (ULong) inArg2, (void *)inArg3, (TimeoutProcPtr) inArg4);
		break;

//	PrimRemoveDelayedFunction(ObjectId inMsgId)
	case 31:	// 392
		result = PrimRemoveDelayedFunction((ObjectId) inArg1);
		break;

//	RememberMappingUsingPAddr(VAddr inVAddr, ULong inPerm, PAddr inPhysAddr, bool inCacheable)
	case kRememberMappingUsingPAddr:	// 247
		result = AddPgPAndPermWithPageTable(GetPrimaryTablePhysBaseAfterGlobalsInited(), (VAddr) inArg1, (ULong) inArg2, (PAddr) inArg3, (bool) inArg4);
		break;

//	SetDomainRange(PAddr inVAddr, size_t inSize, ULong inDomain)
	case kSetDomainRange:	// 256
		PrimSetDomainRange((PAddr) inArg1, (size_t) inArg2, inArg3);
		break;

//	ClearDomainRange(PAddr inVAddr, size_t inSize)
	case kClearDomainRange:	// 261
		PrimClearDomainRange((PAddr) inArg1, (size_t) inArg2);
		break;

//	SetEnvironment(ObjectId inEnvId, ObjectId * outEnvId)
	case kSetEnvironment:	// 352
		result = SetEnvironment((ObjectId) inArg1, (ObjectId *)&gCurrentTask->fRegister[kReturnParm1]);
		break;

//	GetEnvironment(ObjectId * outEnvId)
	case kGetEnvironment:	// 357
		result = GetEnvironment((ObjectId *)&gCurrentTask->fRegister[kReturnParm1]);
		break;

//	AddDomainToEnvironment(ObjectId inEnvId, ObjectId inDomId, ULong inFlags)
	case kAddDomainToEnvironment:	// 361
		result = AddDomainToEnvironment((ObjectId) inArg1, (ObjectId) inArg2, (ULong) inArg3);
		break;

//	RemoveDomainFromEnvironment(ObjectId inEnvId, ObjectId inDomId)
	case kRemoveDomainFromEnvironment:	// 366
		result = RemoveDomainFromEnvironment((ObjectId) inArg1, (ObjectId) inArg2);
		break;

//	EnvironmentHasDomain(ObjectId inEnvId, ObjectId inDomId, bool * outHasDomain, bool * outIsManager)
	case kEnvironmentHasDomain:	// 370
		result = EnvironmentHasDomain((ObjectId) inArg1, (ObjectId) inArg2, (bool *)&gCurrentTask->fRegister[kReturnParm1], (bool *)&gCurrentTask->fRegister[kReturnParm2]);
		break;

//	SemGroupSetRefCon(ObjectId inSemGroupId, void * inRefCon)
	case kSemGroupSetRefCon:	// 377
		result = SemGroupSetRefCon((ObjectId) inArg1, (void *)inArg2);
		break;

//	SemGroupGetRefCon(ObjectId inSemGroupId, void ** outRefCon)
	case kSemGroupGetRefCon:	// 381
		result = SemGroupGetRefCon((ObjectId) inArg1, (void **) &gCurrentTask->fRegister[kReturnParm1]);
		break;

//	PAddr VtoP(VAddr inAddr)
	case kVtoP:	// 395
		result = PrimVtoP((VAddr) inArg1);
		break;

	case kRealTimeClockDispatch:	// 398
		result = RealTimeClockDispatch();
		break;

	case kMemObjManagerDispatch:	// 400
		result = PrimGetMemObjInfo();
		break;

//	SetNetworkPersistentInfo(ULong info) | GetNetworkPersistentInfo(ULong * outInfo)
	case 45:	// 402
	// call again in svc mode
		result = PrimGetNetworkPersistentInfo();
		break;

//	SetBequeathId(ObjectId inToTaskId)
	case kTaskBequeath:	// 404
		gCurrentTask->setBequeathId((ObjectId) inArg1);
		break;

//	RegisterPatch(void *, ULong, ULong, ULong) | GetPatchInfo(ULong *, ULong *)
	case 47:	// 408
//		result = PrimGetPatchInfo();
		break;

//	ResetRebootReason(void)
	case kResetRebootReason:	// 410
	// call again in svc mode
		result = ResetRebootReason();
		break;

//	RemovePMappings(PAddr inBase, size_t inSize)
	case kRemovePMappings:	// 412
	// call again in svc mode
		result = RemovePMappings((PAddr) inArg1, (size_t) inArg2);
		break;

//	ClearFIQAtomic(void)
	case kClearFIQ:	// 416
	// call again in svc mode
		result = ClearFIQAtomic();
		break;

	case kSetTabletCalibration:	// 418
	// call again in svc mode
		result = SetTabletCalibrationDataSWI();
		break;

	case kGetTabletCalibration:	// 420
	// call again in svc mode
		result = GetTabletCalibrationDataSWI();
		break;

	case 53:	// 426
//		SRegisterLoadedCodeWithDebugger(inArg1, inArg2, inArg3);
		break;

	case 54:	// 431
//		SDeregisterLoadedCodeWithDebugger(inArg1);
		break;

	case 55:	// 434
//		SInformDebuggerMemoryReloaded(inArg1, inArg2);
		break;

	case 56:	// 438
//		BackupPatch(inArg1);
		break;

//	MakePhysInaccessible(ObjectId inId)
	case 57:	// 293
		result = MakePhysInaccessible((ObjectId) inArg1);
		break;

//	MakePhysAccessible(ObjectId inId)
	case 58:	// 296
		result = MakePhysAccessible((ObjectId) inArg1);
		break;

//	VAddr	GetRExConfigEntry(ULong inId, ULong inTag, size_t * outSize)
	case kGetRExConfigEntry:	// 441
		gCurrentTask->fRegister[kReturnParm1] = PrimRExConfigEntry((ULong) inArg1, (ULong) inArg2, &gCurrentTask->fRegister[kReturnParm2]);
		result = noErr;
		break;

	case 60:	// 219
		result = ForgetPhysMapping();
		break;

	case 61:	// 231
		result = RememberPhysMapping();
		break;

	case 62:	// 299
		result = ChangeVirtualMapping();
		break;

	case 63:	// 282
		result = ReleasePageTable(inArg1);
		break;

//	RExHeader *	GetRExPtr(ULong inId)
	case kGetRExPtr:	// 447
		result = (long) gGlobalsThatLiveAcrossReboot.fRExPtr[inArg1];
		break;

//	VAddr	GetLastRExConfigEntry(ULong inTag, size_t * outSize)
	case kGetLastRExConfigEntry:	// 452
		gCurrentTask->fRegister[kReturnParm1] = PrimLastRExConfigEntry((ULong)inArg1, &gCurrentTask->fRegister[kReturnParm2]);
		result = noErr;
		break;

	case 66:	// 422
//		SRegisterPackageWithDebugger(inArg1, inArg2);
		break;

//	CUPort::reset(ULong inSendersResetFlags, ULong inReceiversResetFlags)
	case kResetPortFlags:	// 457
		result = PortResetKernelGlue((ObjectId) inArg1, (ULong) inArg2, (ULong) inArg3);
		break;

	case kPowerOffSystem:	// 462
		result = PowerOffSystemKernelGlue();
		break;

//	PauseSystem(void)
	case kPauseSystem:	// 464
		result = PauseSystemKernelGlue();
		break;

	case 70:	// 466
		result = CleanPageInIandDCacheSWIGlue(inArg1);
		break;

	case 71:	// 469
		EnterFIQAtomic();
		CleanDCandFlushICSWIGlue();
		ExitFIQAtomic();
		break;

	case 72:	// 474
		result = CleanPageInDCSWIGlue(inArg1);
		break;

	case 73:	// 477
		result = CleanRangeInDCSWIGlue(inArg1, inArg2);
		break;

	case 74:	// 482
		CopyPagesAfterStackCollided((CopyPageAfterStackCollisionParams *)inArg1);
		break;

//	GetTaskStackInfo(const CUTask * inTask, VAddr * outStackTop, VAddr * outStackBase)
	case kGetTaskStackInfo:
		{
			CTask *	task;
			if (inArg1 == kNoId)
				task = gCurrentTask;
			else
				task = (CTask *)IdToObj(kTaskType, (ObjectId) inArg1);
			if (task == NULL)
				result = kOSErrTaskDoesNotExist;
			else
			{
				gCurrentTask->fRegister[kReturnParm1] = task->fStackTop;
				gCurrentTask->fRegister[kReturnParm2] = task->fStackBase;
			}
		}
		break;

	default:
		result = -1;
		break;
	}

	return result;
}

