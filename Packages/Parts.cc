/*
	File:		Parts.cc

	Contains:	Installation of package parts.
	
	Written by:	Newton Research Group, 2010.
*/

#include "Objects.h"
#include "Funcs.h"
#include "PackageTypes.h"
#include "PackageParts.h"
#include "ROMResources.h"

extern Ref			gFunctionFrame;

extern NewtonErr	FramesException(Exception * inException);

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

NewtonErr
InstallPart(RefArg inPartType, RefArg inPartFrame, const PartId & inPartId, SourceType inSource, PartInfo * info, RefArg ioContext)
{
	NewtonErr err = noErr;
	newton_try
	{
		RefVar pkgStyleSym;
		if (inSource.format == kFixedMemory)
		{
			if (inSource.deviceKind == kNoDevice)
				pkgStyleSym = RSYMhighROM;
		}
		else if (inSource.format == kRemovableMemory)
		{
			if (inSource.deviceKind == kStoreDevice)
				pkgStyleSym = RSYM1_2Ex;
			if (inSource.deviceKind == kStoreDeviceV2)
				pkgStyleSym = RSYMvbo;
		}
		RefVar installInfo(Clone(RA(canonicalFramePartInstallInfo)));
		SetFrameSlot(installInfo, SYMA(partType), inPartType);
		SetFrameSlot(installInfo, SYMA(partFrame), inPartFrame);
		SetFrameSlot(installInfo, SYMA(packageId), MAKEINT(inPartId.packageId));
		SetFrameSlot(installInfo, SYMA(packageName), MakeString(((ExtendedPartInfo *)info)->packageName));
		SetFrameSlot(installInfo, SYMA(partIndex), MAKEINT(inPartId.partIndex));
		SetFrameSlot(installInfo, SYMA(size), MAKEINT(info->sizeInMemory));
		SetFrameSlot(installInfo, SYMA(packageType), MAKEINT(inSource.format));
		SetFrameSlot(installInfo, SYMA(deviceKind), MAKEINT(inSource.deviceKind));
		SetFrameSlot(installInfo, SYMA(deviceNumber), MAKEINT(inSource.deviceNumber));
		SetFrameSlot(installInfo, SYMA(packageStyle), pkgStyleSym);

		RefVar args(MakeArray(1));
		SetArraySlot(args, 0, installInfo);
		RefVar result(DoBlock(GetFrameSlot(gFunctionFrame, SYMA(InstallPart)), args));

		SetFrameSlot(ioContext, SYMA(partFrame), inPartFrame);
		SetFrameSlot(ioContext, SYMA(packageStyle), pkgStyleSym);
		SetFrameSlot(ioContext, SYMA(removeCookie), result);
	}
	newton_catch(exFrames)
	{
		err = FramesException(CurrentException());
	}
	end_try;
	return err;
}



/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

NewtonErr
RemovePart(RefArg inPartType, const PartId & inPartId, RefArg ioContext)
{
	NewtonErr err = noErr;
	newton_try
	{
		RefVar removeInfo(Clone(RA(canonicalFramePartRemoveInfo)));
		SetFrameSlot(removeInfo, SYMA(partType), inPartType);
		SetFrameSlot(removeInfo, SYMA(partFrame), GetFrameSlot(ioContext, SYMA(partFrame)));
		SetFrameSlot(removeInfo, SYMA(packageId), MAKEINT(inPartId.packageId));
		SetFrameSlot(removeInfo, SYMA(partIndex), MAKEINT(inPartId.partIndex));
		SetFrameSlot(removeInfo, SYMA(packageStyle), GetFrameSlot(ioContext, SYMA(packageStyle)));

		RefVar args(MakeArray(2));
		SetArraySlot(args, 0, removeInfo);
		SetArraySlot(args, 1, GetFrameSlot(ioContext, SYMA(removeCookie)));
		DoBlock(GetFrameSlot(gFunctionFrame, SYMA(RemovePart)), args);
	}
	newton_catch(exFrames)
	{
		err = FramesException(CurrentException());
	}
	end_try;
	return err;
}

