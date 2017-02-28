/*
	File:		Gestalt.cc

	Contains:	Newton gestalt functions.

	Written by:	Newton Research Group.
*/

#include "NewtonGestalt.h"
#include "NameServer.h"
#include "Unicode.h"
//#include "UStringUtils.h"
#include "OSErrors.h"


/* -------------------------------------------------------------------------------
	C U G e s t a l t
------------------------------------------------------------------------------- */

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

	// first see if there’s a registered gestalt
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

	// see if it’s already registered - can’t register it if so
	sprintf(selectorName, "%x", inSelector);
	ns.lookup(selectorName, kRegisteredGestalt, &thing, &spec);
	if (thing != 0)
		return kOSErrAlreadyRegistered;
	// also can’t allow selectors in the kGestalt_Base range
	else if (inSelector > kGestalt_Base && inSelector < kGestalt_Extended_Base)
		return kOSErrBadParameters;

	return ns.registerName(selectorName, kRegisteredGestalt, (OpaqueRef)ioParmBlock, (OpaqueRef)inParmSize);
}


/*--------------------------------------------------------------------------------
	Replace a gestalt that’s already been registered.
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

	// see if it’s already registered - must unregister it if so
	// but no matter if it doesn’t exist
	sprintf(selectorName, "%x", inSelector);
	ns.lookup(selectorName, kRegisteredGestalt, &thing, &spec);
	if (thing != 0)
		ns.unregisterName(selectorName, (char *)kRegisteredGestalt);
	// also can’t allow selectors in the kGestalt_Base range
	else if (inSelector > kGestalt_Base && inSelector < kGestalt_Extended_Base)
		return kOSErrBadParameters;

	return ns.registerName(selectorName, kRegisteredGestalt, (OpaqueRef)ioParmBlock, (OpaqueRef)inParmSize);
}


/* -------------------------------------------------------------------------------
	C G e s t a l t R e q u e s t
------------------------------------------------------------------------------- */

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


/* -------------------------------------------------------------------------------
	For argument marshaling.
------------------------------------------------------------------------------- */
extern Ref			ConstructReturnValue(void * inParmBlock, RefArg inSpec, NewtonErr * outErr, CharEncoding inEncoding);
extern NewtonErr	MarshalArgumentSize(RefArg inArray1, RefArg inArray2, size_t * outArg3, int inArg4);
extern NewtonErr	MarshalArguments(RefArg inArray1, RefArg inArray2, void ** ioParmPtr, int inArg4);


static size_t gParmBlockSize = 256;


/* -------------------------------------------------------------------------------
	Return extended gestalt information.
	Args:		inGestaltSpec	array defining gestalt struct, eg:
		constant kGestalt_ContrastInfo := '[ kGestalt_SoftContrast, [ struct, boolean, long, long ], 1 ];
		constant kGestalt_VolumeInfo := '[ kGestalt_Ext_VolumeInfo, [ struct, boolean, boolean, boolean, boolean, real, long, long ], 1 ];

		kGestalt_Ext_VolumeInfo registered by PCirrusSoundDriver

	Return:	info frame
------------------------------------------------------------------------------- */

Ref
ExtendedGestalt(RefArg inGestaltSpec)
{
	RefVar result;
	NewtonErr err = noErr;
	void * parmBlock = NULL;
	CUGestalt gestalt;
	XTRY
	{
		// first element of spec MUST be int selector
		Ref selectorRef = GetArraySlot(inGestaltSpec, 0);
		XFAIL(!ISINT(selectorRef))
		GestaltSelector selector = RVALUE(selectorRef);
		RefVar spec(GetArraySlot(inGestaltSpec, 1));
		// second element of spec MUST be structure specifier array
		XFAIL(!IsArray(spec))
		// third element of spec MUST be int
		Ref encodingRef = GetArraySlot(inGestaltSpec, 2);
		XFAIL(!ISINT(encodingRef))
		// validate selector
		XFAIL(selector < kGestalt_Extended_Base)

		size_t parmSize = gParmBlockSize;
		XFAILNOT(parmBlock = malloc(parmSize), err = MemError();)
		err = gestalt.gestalt(selector, parmBlock, &parmSize);
		if (parmSize > gParmBlockSize)
		{
			// gestalt wants a bigger parmBlock
			gParmBlockSize = parmSize;
			free(parmBlock);
			XFAILNOT(parmBlock = malloc(parmSize), err = MemError();)
			err = gestalt.gestalt(selector, parmBlock, &parmSize);
		}
		XFAIL(err)
		result = ConstructReturnValue(parmBlock, spec, &err, (CharEncoding)RVALUE(encodingRef));
	}
	XENDTRY;
	if (parmBlock)
		free(parmBlock);
	return result;
}


/* -------------------------------------------------------------------------------
	Return gestalt information.
	Args:		inRcvr
				inSelector
	Return:	info frame
------------------------------------------------------------------------------- */

Ref
FGestalt(RefArg inRcvr, RefArg inSelector)
{
	NewtonErr err;
	RefVar result;
	CUGestalt gestalt;

	if (ISINT(inSelector))
	{
		// standard gestalt
		switch (RVALUE(inSelector))
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
				SetFrameSlot(result, SYMA(ROMStage), MAKEINT(systemInfo.fROMStage));
				SetFrameSlot(result, SYMA(ROMVersion), MAKEINT(systemInfo.fROMVersion));
				SetFrameSlot(result, SYMA(RAMSize), MAKEINT(systemInfo.fRAMSize));
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
					SetFrameSlot(result, SYMA(CPUType), SYMA(strongARM));
				else if (systemInfo.fCPUType == 2)
					SetFrameSlot(result, SYMA(CPUType), SYMA(ARM710A));
				else if (systemInfo.fCPUType == 1)
					SetFrameSlot(result, SYMA(CPUType), SYMA(ARM610A));

				SetFrameSlot(result, SYMA(CPUSpeed), MakeReal(systemInfo.fCPUSpeed));

				UniChar verStr[16];
				VersionString(&systemInfo, verStr);
				SetFrameSlot(result, SYMA(ROMVersionString), MakeString(verStr));
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
					SetFrameSlot(aPatch, SYMA(fPatchCheckSum), MAKEINT(patchInfoPtr->fPatchCheckSum));
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
					RefVar rexInfoElement(Clone(RA(canonicalGestaltRExInfoArrayElement)));
					SetFrameSlot(rexInfoElement, SYMA(signatureA), MAKEINT(rexInfoPtr->signatureA));
					SetFrameSlot(rexInfoElement, SYMA(signatureB), MAKEINT(rexInfoPtr->signatureB));
					SetFrameSlot(rexInfoElement, SYMA(checksum), MAKEINT(rexInfoPtr->checksum));
					SetFrameSlot(rexInfoElement, SYMA(headerVersion), MAKEINT(rexInfoPtr->headerVersion));
					SetFrameSlot(rexInfoElement, SYMA(manufacturer), MAKEINT(rexInfoPtr->manufacturer));
					SetFrameSlot(rexInfoElement, SYMA(version), MAKEINT(rexInfoPtr->version));
					SetFrameSlot(rexInfoElement, SYMA(Length), MAKEINT(rexInfoPtr->length));
					SetFrameSlot(rexInfoElement, SYMA(id), MAKEINT(rexInfoPtr->id));
					SetFrameSlot(rexInfoElement, SYMA(start), MAKEINT(rexInfoPtr->start));
					SetFrameSlot(rexInfoElement, SYMA(count), MAKEINT(rexInfoPtr->count));
					SetArraySlot(result, i, rexInfoElement);
				}
			}
			break;
		}
	}
	else if (IsArray(inSelector))
	{
		// extended gestalt -- selector is an argument specifier
		result = ExtendedGestalt(inSelector);
	}
	return result;
}


/* -------------------------------------------------------------------------------
	Update gestalt selector.
	Args:		inSelector	int
				inArg2		array
				inArg3		array
				inArg4		int
				inUpdate		otherwise create anew
	Return:	TRUEREF => okay
------------------------------------------------------------------------------- */

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
		if (parmSize > gParmBlockSize)
			gParmBlockSize = parmSize;
		XFAIL(err = MarshalArguments(inArg2, inArg3, &parmBlock, RINT(inArg4)))
		if (inUpdate)
			err = gestalt.replaceGestalt(RINT(inSelector), parmBlock, parmSize);
		else
			err = gestalt.registerGestalt(RINT(inSelector), parmBlock, parmSize);
	}
	XENDTRY;
	return MAKEBOOLEAN(err == noErr);
}


/* -------------------------------------------------------------------------------
	Register a new gestalt selector.
	Args:		inRcvr
				inSelector
				inArg2
				inArg3
				inArg4
	Return:	TRUEREF => okay
------------------------------------------------------------------------------- */

Ref
FRegisterGestalt(RefArg inRcvr, RefArg inSelector, RefArg inArg2, RefArg inArg3, RefArg inArg4)
{
	return UpdateGestalt(inSelector, inArg2, inArg3, inArg4, false);
}


/* -------------------------------------------------------------------------------
	Replace an existing gestalt selector.
	Args:		inRcvr
				inSelector
				inArg2
				inArg3
				inArg4
	Return:	TRUEREF => okay
------------------------------------------------------------------------------- */

Ref
FReplaceGestalt(RefArg inRcvr, RefArg inSelector, RefArg inArg2, RefArg inArg3, RefArg inArg4)
{
	return UpdateGestalt(inSelector, inArg2, inArg3, inArg4, true);
}

