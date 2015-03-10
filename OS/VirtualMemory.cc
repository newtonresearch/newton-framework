/*
	File:		VirtualMemory.cc

	Contains:	Interfaces to virtual memory functions.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "MemArch.h"
#include "StackManager.h"
#include "UserTasks.h"

#pragma mark Stack
/* -----------------------------------------------------------------------------
	S t a c k
----------------------------------------------------------------------------- */
/* -----------------------------------------------------------------------------
	Create a new stack.
	Args:		inDomainId
				inMaxSize
				inOwnerId
				outTopOfStack
				outBottomOfStack
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
NewStack(ObjectId inDomainId, size_t inMaxSize, ObjectId inOwnerId, VAddr * outTopOfStack, VAddr * outBottomOfStack)
{
	NewtonErr	err;
	FM_NewStack_Parms args;

	args.domainId = inDomainId;
	args.addr = 0;
	args.maxSize = inMaxSize;
	args.ownerId = inOwnerId;
	err = InvokeSMMRoutine(kSM_NewStack, &args);
	if (err == noErr)
	{
		*outTopOfStack = args.stackTop;
		*outBottomOfStack = args.stackBottom;
	}

	return err;
}


NewtonErr
LockStack(StackLimits * ioLimits, size_t inAdditionalSpace)
{
	VAddr stack;
	VAddr start;
	VAddr end;

	stack = (VAddr)&stack - inAdditionalSpace;
	ioLimits->start = start = MAX(TaskSwitchedGlobals()->fStackBottom, stack);
	ioLimits->end = end = TaskSwitchedGlobals()->fStackTop;
	return LockHeapRange(start, end);
}


NewtonErr
UnlockStack(StackLimits * inLimits)
{
	return UnlockHeapRange(inLimits->start, inLimits->end);
}


#pragma mark Heap
/* -----------------------------------------------------------------------------
	H e a p
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Create a new heap.
	Args:		inDomainId
				inRequestedAddr
				inMaxSize
				inOptions
				outAreaStart
				outAreaEnd
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
NewHeapArea(ObjectId inDomainId, VAddr inRequestedAddr, size_t inMaxSize, ULong inOptions, VAddr * outAreaStart, VAddr * outAreaEnd)
{
	NewtonErr	err;
	FM_NewHeapArea_Parms args;

	args.domainId = inDomainId;
	args.addr = inRequestedAddr;
	args.maxSize = inMaxSize;
	args.options = inOptions;
	err = InvokeSMMRoutine(kSM_NewHeapArea, &args);
	if (err == noErr)
	{
		*outAreaStart = args.areaStart;
		*outAreaEnd = args.areaEnd;
	}

	return err;
}


NewtonErr
SetHeapLimits(VAddr inStart, VAddr inEnd)
{
	FM_SetHeapLimits_Parms args;

	args.start = inStart;
	args.end = inEnd;
	return InvokeSMMRoutine(kSM_SetHeapLimits, &args);
}


NewtonErr
FreePagedMem(VAddr inAddressInArea)
{
	FM_Free_Parms args;

	args.addr = inAddressInArea;
	return InvokeSMMRoutine(kSM_FreePagedMem, &args);
}


NewtonErr
LockHeapRange(VAddr inStart, VAddr inEnd, bool inWire)
{
	FM_LockHeapRange_Parms args;

	args.start = inStart;
	args.end = inEnd - 1;
	args.wire = inWire;
	return InvokeSMMRoutine(kSM_LockHeapRange, &args);
}


NewtonErr
UnlockHeapRange(VAddr inStart, VAddr inEnd)
{
	FM_SetHeapLimits_Parms args;

	args.start = inStart;
	args.end = inEnd - 1;
	return InvokeSMMRoutine(kSM_UnlockHeapRange, &args);
}


NewtonErr
NewHeapDomain(ULong inSectionBase, ULong inSectionCount, ObjectId * outDomainId)
{
	NewtonErr	err;
	FM_NewHeapDomain_Parms args;

	args.sectionBase = inSectionBase;
	args.sectionCount = inSectionCount;
	err = InvokeSMMRoutine(kSM_NewHeapDomain, &args);
	*outDomainId = args.domainId;
	return err;
}


NewtonErr
AddPageMappingToDomain(ObjectId inDomainId, VAddr inAddr, ObjectId inPageId)
{
	FM_AddPageMappingToDomain_Parms args;

	args.domainId = inDomainId;
	args.addr = inAddr;
	args.pageId = inPageId;
	return InvokeSMMRoutine(kSM_AddPageMappingToDomain, &args);
}


NewtonErr
SetRemoveRoutine(VAddr inAddr, ReleaseProcPtr inProc, void * inRefCon)
{
	FM_SetRemoveRoutine_Parms args;

	args.addr = inAddr;
	args.proc = inProc;
	args.refCon = inRefCon;
	return InvokeSMMRoutine(kSM_SetRemoveRoutine, &args);
}


NewtonErr
GetHeapAreaInfo(VAddr inAddr, VAddr * outStart, VAddr * outEnd)
{
	NewtonErr	err;
	FM_GetHeapAreaInfo_Parms args;

	args.addr = inAddr;
	err = InvokeSMMRoutine(kSM_GetHeapAreaInfo, &args);
	*outStart = args.areaStart;
	*outEnd = args.areaEnd;
	return err;
}


NewtonErr
GetSystemReleasable(ULong * outReleaseable, ULong * outStackSpaceUsed, ULong * outTotalPagesUsed)
{
	NewtonErr  err;
	FM_GetSystemReleaseable_Parms args;

	err = InvokeSMMRoutine(kSM_GetSystemReleasable, &args);
	if (outReleaseable != NULL)
		*outReleaseable = args.releasable;
	if (outStackSpaceUsed != NULL)
		*outStackSpaceUsed = args.stackUsed;
	if (outTotalPagesUsed != NULL)
		*outTotalPagesUsed = args.pagesUsed;
	return err;
}

