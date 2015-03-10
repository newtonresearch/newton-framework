/*
	File:		PhysicalMemory.h

	Contains:	Physical memory addresses and declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__PHYSICALMEMORY_H)
#define __PHYSICALMEMORY_H 1

#include "VirtualMemory.h"

struct KernelArea
{
	void *		ptr;
	long			num;
	SBankInfo	bank[kMaximumMemoryPartitions];
	SGlobalsThatLiveAcrossReboot *	pGTLAR;
};

extern PAddr		GetSuperStacksPhysBase(ULong inRAMBank1Size);
extern void			CopyRAMTableToKernelArea(KernelArea * info);


/*------------------------------------------------------------------------------
	Return physical address of globals that live across reboot.
	For use before MMU and virtual addresses are available.
	Args:		inBank		non-zero if RAM bank 1 exists
	Return:  physical address of globals that live across reboot
------------------------------------------------------------------------------*/

inline SGlobalsThatLiveAcrossReboot *
MakeGlobalsPtr(ULong inBank1)
{
#if defined(correct)
	return (SGlobalsThatLiveAcrossReboot *)(PAddr)
		((&gGlobalsThatLiveAcrossReboot - TRUNC(0x0C100800, kPageSize) + 7*kPageSize)	// offset
	 | ((inBank1 != 0) ? 0x04000000 : 0x08000000));	// physical base -> 0x04007800 + offset to gGlobalsThatLiveAcrossReboot
#else
	return &gGlobalsThatLiveAcrossReboot;
#endif
}


#endif	/* __PHYSICALMEMORY_H */
