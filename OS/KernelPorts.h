/*
	File:		KernelPorts.h

	Contains:	Kernel port definitions.

	Written by:	Newton Research Group.
*/

#if !defined(__KERNELPORTS_H)
#define __KERNELPORTS_H 1

#include "KernelObjects.h"
#include "SharedMem.h"

class CPort : public CObject
{
public:
					CPort();
					~CPort();

	NewtonErr	reset(ULong inSendFlags, ULong inReceiveFlags);
	NewtonErr	resetFilter(CSharedMemMsg * inMsg, ULong inFilter);
	NewtonErr	send(CSharedMemMsg * inMsg, ULong inFlags);
	NewtonErr	receive(CSharedMemMsg * inMsg, ULong inFlags);

	CDoubleQContainer	fSenders;	// +10
	CDoubleQContainer	fReceivers;	// +24
// size +38
};

#endif	/* __KERNELPORTS_H */
