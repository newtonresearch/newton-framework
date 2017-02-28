/*
	File:		Packages.cc

	Contains:	Package loaders etc.
	
	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ROMResources.h"
#include "PackageTypes.h"
#include "PackageParts.h"
#include "LargeBinaries.h"
#include "NewtonScript.h"
#include "MemoryPipe.h"
#include "EndpointPipe.h"
#include "EndpointClient.h"


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

const char * const kPackageMagicNumber = "package01";


extern "C" {
Ref	FObjectPkgRef(RefArg inRcvr, RefArg inPkg);
Ref	FGetPkgRefInfo(RefArg inRcvr, RefArg inPkg);
Ref	FIsValid(RefArg inRcvr, RefArg inPkg);
}

Ref
FObjectPkgRef(RefArg inRcvr, RefArg inPkg)
{ return NILREF; }

Ref
FGetPkgRefInfo(RefArg inRcvr, RefArg inPkg)
{ return NILREF; }

Ref
FIsValid(RefArg inRcvr, RefArg inPkg)
{ return NILREF; }




/* -----------------------------------------------------------------------------
	Determine whether block of data is a package header.
	Args:		inData		pointer to data
				inSize		size of data
	Return:	true => is a package header
----------------------------------------------------------------------------- */

bool
IsPackageHeader(Ptr inData, size_t inSize)
{
	bool isOK = false;
	if (inSize >= sizeof(PackageDirectory))
	{
		PackageDirectory *	directory = (PackageDirectory *)inData;
		newton_try
		{
			ArrayIndex	i;
			for (i = 0, isOK = true; i < kPackageMagicLen && isOK; ++i)
			{
				if (directory->signature[i] != kPackageMagicNumber[i])
					isOK = false;
			}
			if (isOK)
			{
				for (isOK = false; i < kPackageMagicLen+kPackageMagicVersionCount && !isOK; ++i)
				{
					if (directory->signature[kPackageMagicLen] == kPackageMagicNumber[i])
						isOK = true;
				}
			}
		}
		newton_catch(exBusError)
			isOK = false;
		newton_catch(exPermissionViolation)
			isOK = false;
		end_try;
	}
	return isOK;
}


#pragma mark -
/* -------------------------------------------------------------------------------
	Allocate a package on a store, providing feedback.
	Args:		inPipe		the piped package
				inStore		the store on which to store the package
				inCallback	a callback frame for providing progress feedback
				inFreq		number of bytes after which we should callback
				inActivate	activate package after storing it?
	Return:	a package frame
------------------------------------------------------------------------------- */

Ref
AllocatePackage(CPipe * inPipe, RefArg inStore, RefArg inCallback, size_t inFreq, bool inActivate)
{
	NewtonErr	err;

	CLOCallback	callback;
	callback.setFunc(inCallback);
	callback.setChunk(inFreq);
	callback.setInfo(RA(NILREF));

	PSSId			id;
	RefVar		storeObject = NOTNIL(inStore) ? (Ref)inStore : NSCallGlobalFn(SYMA(GetDefaultStore));
	CStoreWrapper *	storeWrapper = (CStoreWrapper *)GetFrameSlot(storeObject, SYMA(store));
	GC();
	if ((err = StorePackage(inPipe, storeWrapper->store(), &callback, &id)) != noErr)
		ThrowErr(exFrames, err);

	return NSCallGlobalFn(SYMA(RegisterNewPackage), WrapPackage(storeWrapper->store(), id), storeObject, MAKEBOOLEAN(inActivate));
}


/* -------------------------------------------------------------------------------
	Allocate a package.
	Args:		inPipe		the piped package
				inStore		the store on which to store the package
				inParms		installation parameter frame
	Return:	a package frame
------------------------------------------------------------------------------- */

Ref
AllocatePackage(CPipe * inPipe, RefArg inStore, RefArg inParms)
{
	RefVar	cbFrame;
	Ref		cbFreq;
	size_t	freq = 4*KByte;
	bool		doActivate = true;
	if (NOTNIL(inParms))
	{
		// create AllocatePackage args from parms frame
		cbFrame = GetFrameSlot(inParms, SYMA(callback));
		doActivate = ISNIL(GetFrameSlot(inParms, SYMA(don_27tActivate)));
		cbFreq = GetFrameSlot(inParms, SYMA(callbackFreq));
		if (NOTNIL(cbFreq))
			freq = RINT(cbFreq);
	}
	return AllocatePackage(inPipe, inStore, cbFrame, freq, doActivate);
}


/* -------------------------------------------------------------------------------
	Create a new package.
	NTK calls in here to download a package.
	Args:		inPipe		the piped package
				inStore		the store on which to store the package
				inCallback	a callback frame for providing progress feedback
				inFreq		number of bytes after which we should callback
	Return:	a package frame
------------------------------------------------------------------------------- */

NewtonErr
NewPackage(CPipe * inPipe, RefArg inStore, RefArg inCallback, size_t inFreq)
{
	NewtonErr err = noErr;
	RefVar pkg;
	newton_try
	{
		pkg = AllocatePackage(inPipe, inStore, inCallback, inFreq, true);
	}
	newton_catch_all
	{
		err = (NewtonErr)(long)CurrentException()->data;;
	}
	end_try;
	return err;
}


/* -------------------------------------------------------------------------------
	Suck a package through a pipe, providing feedback.
	Args:		inPipe		the piped package
				inStore		the store on which to store the package
				inCallback	a callback frame for providing progress feedback
				inFreq		number of bytes after which we should callback
				inActivate	activate package after storing it?
	Return:	a package frame
------------------------------------------------------------------------------- */

Ref
SuckPackageThruPipe(CPipe * inPipe, RefArg inStore, RefArg inCallback, size_t inFreq, bool inActivate)
{
	return AllocatePackage(inPipe, inStore, inCallback, inFreq, inActivate);
}


/* -------------------------------------------------------------------------------
	Suck a package through a pipe.
	Args:		inPipe		the piped package
				inStore		the store on which to store the package
				inParms		installation parameters
	Return:	a package frame
------------------------------------------------------------------------------- */

Ref
SuckPackageThruPipe(CPipe * inPipe, RefArg inStore, RefArg inParms)
{
	return AllocatePackage(inPipe, inStore, inParms);
}


#pragma mark -
/* -----------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
----------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------------
	Suck a package from a binary object.
	We do this by creating a memory pipe so we can SuckPackageThruPipe()
	Args:		inRcvr		a store frame
				inBinary		the binary object containing the package
				inParms		params frame
	Return:	a package frame
------------------------------------------------------------------------------- */

Ref
StoreSuckPackageFromBinary(RefArg inRcvr, RefArg inBinary, RefArg inParms)
{
	RefVar pkg;
#if !defined(forFramework)
	size_t		binSize = Length(inBinary);
	CDataPtr		binData(inBinary);

	CBufferSegment * buf = CBufferSegment::make();
	buf->init((char *)binData, binSize);

	CMemoryPipe pipe;
	pipe.init(buf, NULL, false);

	pkg = SuckPackageThruPipe(&pipe, inRcvr, inParms);

	buf->destroy();
#endif
	return pkg;
}


/* -------------------------------------------------------------------------------
	Suck a package from an endpoint.
	Args:		inRcvr		a store frame
				inBinary		the binary object containing the package
				inParms		params frame
	Return:	a package frame
------------------------------------------------------------------------------- */

Ref
StoreSuckPackageFromEndpoint(RefArg inRcvr, RefArg inEndpoint, RefArg inParms)
{
	NewtonErr err = noErr;
	RefVar pkg;
#if 0 /* endpoints don’t work yet */
	CEndpoint * ep = GetClientEndpoint(inEndpoint);
	if (ep)
	{
		CEndpointPipe epp;
		newton_try
		{
			epp.init(ep, 2*KByte, 0, 0, false, NULL);
			pkg = AllocatePackage(&epp, inRcvr, inParms);
		}
		newton_catch_all
		{
			err = (NewtonErr)(long)CurrentException()data;
		}
		end_try;
	}
#endif
	if (err)
		ThrowErr(exFrames, err);
	
	return pkg;
}

