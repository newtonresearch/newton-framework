/*
	File:		Gestalt.cc

	Contains:	Newton gestalt functions.

	Written by:	Newton Research Group.
*/

#include "NewtonGestalt.h"
#include "NameServer.h"
#include "OSErrors.h"


/*--------------------------------------------------------------------------------
	C U G e s t a l t
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Constructor.
	Talk to the name server for gestalt information.
--------------------------------------------------------------------------------*/

CUGestalt::CUGestalt()
{
	fGestaltPort = GetPortSWI(kGetNameServerPort);
}


/*--------------------------------------------------------------------------------
	Return gestalt information.
	Args:		inSelector		which type of information to return
				ioParmBlock		where to put it; actually a gestalt info struct
				inParmSize		the size of the paramblock
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CUGestalt::gestalt(GestaltSelector inSelector, void * ioParmBlock, size_t inParmSize)
{
	return gestalt(inSelector, ioParmBlock, &inParmSize);
}


/*--------------------------------------------------------------------------------
	Return gestalt information.
	Args:		inSelector		which type of information to return
				ioParmBlock		where to put it; actually a gestalt info struct
				ioParmSize		the size of the paramblock
	Return:	error code
				ioParmSize		the actual size of the gestalt info struct
--------------------------------------------------------------------------------*/

NewtonErr
CUGestalt::gestalt(GestaltSelector inSelector, void * ioParmBlock, size_t * ioParmSize)
{
	CUNameServer	ns;
	char			selectorName[8];
	OpaqueRef	thing = 0;
	OpaqueRef	spec = 0;

	// first see if thereÕs a registered gestalt
	// its name is the selector expressed as a hex number
	sprintf(selectorName, "%x", inSelector);
	ns.lookup(selectorName, kRegisteredGestalt, &thing, &spec);
	if (thing != 0)
	{
		if (*ioParmSize > spec)
			*ioParmSize = spec;
		newton_try
		{
			memmove(ioParmBlock, (Ptr) thing, *ioParmSize);
		}
		newton_catch(exAbort)
		{ }
		newton_catch(exBusError)
		{ }
		end_try;
		*ioParmSize = spec;
		return noErr;
	}

	// no registered gestalt - do it the proper way
	//	send a gestalt request to the name server
	CGestaltRequest	request;
	size_t				replySize;
	request.fSelector = inSelector;
	return fGestaltPort.sendRPC(&replySize, &request, sizeof(request), ioParmBlock, *ioParmSize);
}


/*--------------------------------------------------------------------------------
	Register a new gestalt.
	Args:		inSelector		its unique identifier
				ioParmBlock		its data
				inParmSize		its size
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CUGestalt::registerGestalt(GestaltSelector inSelector, void * ioParmBlock, size_t inParmSize)
{
	CUNameServer	ns;
	char			selectorName[8];
	OpaqueRef	thing = 0;
	OpaqueRef	spec = 0;

	// see if itÕs already registered - canÕt register it if so
	sprintf(selectorName, "%x", inSelector);
	ns.lookup(selectorName, kRegisteredGestalt, &thing, &spec);
	if (thing != 0)
		return kOSErrAlreadyRegistered;
	// also canÕt allow selectors in the kGestalt_Base range
	else if (inSelector > kGestalt_Base && inSelector < kGestalt_Extended_Base)
		return kOSErrBadParameters;

	return ns.registerName(selectorName, kRegisteredGestalt, (OpaqueRef)ioParmBlock, (OpaqueRef)inParmSize);
}


/*--------------------------------------------------------------------------------
	Replace a gestalt thatÕs already been registered.
	Args:		inSelector		its unique identifier
				ioParmBlock		its data
				inParmSize		its size
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CUGestalt::replaceGestalt(GestaltSelector inSelector, void * ioParmBlock, size_t inParmSize)
{
	CUNameServer	ns;
	char			selectorName[8];
	OpaqueRef	thing = 0;
	OpaqueRef	spec = 0;

	// see if itÕs already registered - must unregister it if so
	// but no matter if it doesnÕt exist
	sprintf(selectorName, "%x", inSelector);
	ns.lookup(selectorName, kRegisteredGestalt, &thing, &spec);
	if (thing != 0)
		ns.unregisterName(selectorName, (char *)kRegisteredGestalt);
	// also canÕt allow selectors in the kGestalt_Base range
	else if (inSelector > kGestalt_Base && inSelector < kGestalt_Extended_Base)
		return kOSErrBadParameters;

	return ns.registerName(selectorName, kRegisteredGestalt, (OpaqueRef)ioParmBlock, (OpaqueRef)inParmSize);
}


/*--------------------------------------------------------------------------------
	C G e s t a l t R e q u e s t
--------------------------------------------------------------------------------*/

CGestaltRequest::CGestaltRequest()
{
	fCommand = kGestalt;
	fSelector = kGestalt_SystemInfo;
}


#pragma mark -

/* -----------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
----------------------------------------------------------------------------- */
#include "Objects.h"
#include "ROMResources.h"

extern "C"
{
Ref	FGestalt(RefArg inRcvr, RefArg inSelector);
Ref	FRegisterGestalt(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3, RefArg inArg4);
Ref	FReplaceGestalt(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3, RefArg inArg4);
}

extern void	VersionString(GestaltSystemInfo * info, UniChar * outStr);


Ref
FGestalt(RefArg inRcvr, RefArg inSelector)
{
	NewtonErr err;
	RefVar result;
	CUGestalt gestalt;

	switch (RINT(inSelector))
	{
	case kGestalt_Version:
		{
			GestaltVersion versionInfo;
			XFAIL(err = gestalt.gestalt(kGestalt_Version, &versionInfo, sizeof(versionInfo)))
			result = Clone(RA(canonicalGestaltVersion));
			SetFrameSlot(result, SYMA(version), MAKEINT(versionInfo.fVersion));
		}
		break;

	case kGestalt_SystemInfo:
		{
			GestaltSystemInfo systemInfo;
			XFAIL(err = gestalt.gestalt(kGestalt_SystemInfo, &systemInfo, sizeof(systemInfo)))
			result = Clone(RA(canonicalGestaltSystemInfo));
			SetFrameSlot(result, SYMA(manufacturer), MAKEINT(systemInfo.fManufacturer));
			SetFrameSlot(result, SYMA(machineType), MAKEINT(systemInfo.fMachineType));
			SetFrameSlot(result, SYMA(ROMstage), MAKEINT(systemInfo.fROMStage));
			SetFrameSlot(result, SYMA(ROMversion), MAKEINT(systemInfo.fROMVersion));
			SetFrameSlot(result, SYMA(RAMsize), MAKEINT(systemInfo.fRAMSize));
			SetFrameSlot(result, SYMA(screenWidth), MAKEINT(systemInfo.fScreenWidth));
			SetFrameSlot(result, SYMA(screenHeight), MAKEINT(systemInfo.fScreenHeight));
			SetFrameSlot(result, SYMA(screenResolutionX), MAKEINT(systemInfo.fScreenResolution.h));
			SetFrameSlot(result, SYMA(screenResolutionY), MAKEINT(systemInfo.fScreenResolution.v));
			SetFrameSlot(result, SYMA(screenDepth), MAKEINT(systemInfo.fScreenDepth));
			SetFrameSlot(result, SYMA(patchVersion), MAKEINT(systemInfo.fPatchVersion));
			SetFrameSlot(result, SYMA(tabletResolutionX), MAKEINT((long)systemInfo.fTabletResolution.x));
			SetFrameSlot(result, SYMA(tabletResolutionY), MAKEINT((long)systemInfo.fTabletResolution.y));
			SetFrameSlot(result, SYMA(manufactureDate), MAKEINT(systemInfo.fManufactureDate/60));	// minutes <- seconds

			if (systemInfo.fCPUType == 3)
				SetFrameSlot(result, SYMA(CPUtype), SYMA(strongARM));
			else if (systemInfo.fCPUType == 2)
				SetFrameSlot(result, SYMA(CPUtype), SYMA(ARM710A));
			else if (systemInfo.fCPUType == 1)
				SetFrameSlot(result, SYMA(CPUtype), SYMA(ARM610A));

			SetFrameSlot(result, SYMA(CPUspeed), MakeReal(systemInfo.fCPUSpeed));

			UniChar verStr[16];
			VersionString(&systemInfo, verStr);
			SetFrameSlot(result, SYMA(ROMversionString), MakeString(verStr));
		}
		break;

	case kGestalt_RebootInfo:
		{
			GestaltRebootInfo rebootInfo;
			XFAIL(err = gestalt.gestalt(kGestalt_RebootInfo, &rebootInfo, sizeof(rebootInfo)))
			result = Clone(RA(canonicalGestaltRebootInfo));
			SetFrameSlot(result, SYMA(rebootReason), MAKEINT(rebootInfo.fRebootReason));
			SetFrameSlot(result, SYMA(rebootCount), MAKEINT(rebootInfo.fRebootCount));
		}
		break;

	case kGestalt_NewtonScriptVersion:
		{
			GestaltNewtonScriptVersion versionInfo;
			XFAIL(err = gestalt.gestalt(kGestalt_NewtonScriptVersion, &versionInfo, sizeof(versionInfo)))
			result = Clone(RA(canonicalGestaltVersion));
			SetFrameSlot(result, SYMA(version), MAKEINT(versionInfo.fVersion));
		}
		break;

	case kGestalt_PatchInfo:
		{
			GestaltPatchInfo patchInfo;
			XFAIL(err = gestalt.gestalt(kGestalt_PatchInfo, &patchInfo, sizeof(patchInfo)))
			result = Clone(RA(canonicalGestaltPatchInfo));
			SetFrameSlot(result, SYMA(fTotalPatchPageCount), MAKEINT(patchInfo.fTotalPatchPageCount));
			RefVar patches(MakeArray(5));
			_PatchInfo * patchInfoPtr = patchInfo.fPatch;
			for (ArrayIndex i = 0; i < 5; ++i)
			{
				RefVar aPatch(Clone(RA(canonicalGestaltPatchInfoArrayElement)));
				SetFrameSlot(aPatch, SYMA(fPatchChecksum), MAKEINT(patchInfoPtr->fPatchCheckSum));
				SetFrameSlot(aPatch, SYMA(fPatchVersion), MAKEINT(patchInfoPtr->fPatchVersion));
				SetFrameSlot(aPatch, SYMA(fPatchPageCount), MAKEINT(patchInfoPtr->fPatchPageCount));
				SetFrameSlot(aPatch, SYMA(fPatchFirstPageIndex), MAKEINT(patchInfoPtr->fPatchFirstPageIndex));
				SetArraySlot(patches, i, aPatch);
			}
			SetFrameSlot(result, SYMA(fPatch), patches);
		}

		break;

	case kGestalt_SoundInfo:
		// no NS interface
		break;

	case kGestalt_PCMCIAInfo:
		// no NS interface
		break;

	case kGestalt_RexInfo:
		{
			GestaltRexInfo rexInfo;
			XFAIL(err = gestalt.gestalt(kGestalt_RexInfo, &rexInfo, sizeof(rexInfo)))
			result = MakeArray(4);
			_RexInfo * rexInfoPtr = rexInfo.fRex;
			for (ArrayIndex i = 0; i < 4; i++, rexInfoPtr++)
			{
				RefVar rexInfoElement(Clone(RA(canonicalGestaltRexInfoArrayElement)));
				SetFrameSlot(rexInfoElement, SYMA(signatureA), MAKEINT(rexInfoPtr->signatureA));
				SetFrameSlot(rexInfoElement, SYMA(signatureB), MAKEINT(rexInfoPtr->signatureB));
				SetFrameSlot(rexInfoElement, SYMA(checksum), MAKEINT(rexInfoPtr->checksum));
				SetFrameSlot(rexInfoElement, SYMA(headerVersion), MAKEINT(rexInfoPtr->headerVersion));
				SetFrameSlot(rexInfoElement, SYMA(manufacturer), MAKEINT(rexInfoPtr->manufacturer));
				SetFrameSlot(rexInfoElement, SYMA(version), MAKEINT(rexInfoPtr->version));
				SetFrameSlot(rexInfoElement, SYMA(length), MAKEINT(rexInfoPtr->length));
				SetFrameSlot(rexInfoElement, SYMA(id), MAKEINT(rexInfoPtr->id));
				SetFrameSlot(rexInfoElement, SYMA(start), MAKEINT(rexInfoPtr->start));
				SetFrameSlot(rexInfoElement, SYMA(count), MAKEINT(rexInfoPtr->count));
				SetArraySlot(result, i, rexInfoElement);
			}
		}
		break;
	}
	return result;
}

size_t g0C104C54;

NewtonErr
MarshalArgumentSize(RefArg, RefArg, size_t*, int)
{ return noErr; }

NewtonErr
MarshalArguments(RefArg, RefArg, void **, int)
{ return noErr; }


Ref
UpdateGestalt(RefArg inSelector, RefArg inArg2, RefArg inArg3, RefArg inArg4, bool inUpdate)
{
	CUGestalt gestalt;
	NewtonErr err = 1;
	XTRY
	{
		XFAIL(!ISINT(inSelector))
		XFAIL(!ISINT(inArg4))
		XFAIL(!IsArray(inArg2))
		XFAIL(!IsArray(inArg3))
		size_t parmSize;
		void * parmBlock;
		XFAIL(err = MarshalArgumentSize(inArg2, inArg3, &parmSize, RINT(inArg4)))
		if (parmSize > g0C104C54)
			g0C104C54 = parmSize;
		XFAIL(err = MarshalArguments(inArg2, inArg3, &parmBlock, RINT(inArg4)))
		if (inUpdate)
			err = gestalt.replaceGestalt(RINT(inSelector), parmBlock, parmSize);
		else
			err = gestalt.registerGestalt(RINT(inSelector), parmBlock, parmSize);
	}
	XENDTRY;
	return MAKEBOOLEAN(err == noErr);
}


Ref
FRegisterGestalt(RefArg inRcvr, RefArg inSelector, RefArg inArg2, RefArg inArg3, RefArg inArg4)
{
	return UpdateGestalt(inSelector, inArg2, inArg3, inArg4, NO);
}


Ref
FReplaceGestalt(RefArg inRcvr, RefArg inSelector, RefArg inArg2, RefArg inArg3, RefArg inArg4)
{
	return UpdateGestalt(inSelector, inArg2, inArg3, inArg4, YES);
}

