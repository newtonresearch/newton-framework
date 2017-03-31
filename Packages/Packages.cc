/*
	File:		Packages.cc

	Contains:	Package loaders etc.
	
	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ROMResources.h"
#include "PackageTypes.h"
#include "PackageParts.h"
#include "LargeObjects.h"
#include "LargeBinaries.h"
#include "NewtonScript.h"
#include "MemoryPipe.h"
#include "EndpointPipe.h"
#include "EndpointClient.h"

#include "PackageManager.h"


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

const char * const kPackageMagicNumber = "package01";

extern Ref	ToObject(CStore * inStore);

Ref
GetPkgInfoFromVAddr(Ptr inAddr)
{
	RefVar info(Clone(RA(canonicalPackageFrame)));	//sp0C
	CStore * store;	//sp08
	PSSId storeId;	//sp04
	PSSId objId;	//sp00
	bool r5 = false;
	if (VAddrToStore(&store, &storeId, (VAddr)inAddr) == noErr) {
		SetFrameSlot(info, SYMA(store), ToObject(store));
		SetFrameSlot(info, SYMA(pssid), MAKEINT(storeId));
	}
	if (StoreToId(store, storeId, &objId) == noErr) {
		SetFrameSlot(info, SYMA(id), MAKEINT(objId));
	}
	//sp-28
#if 0
	CPackageIterator iter(inAddr);
	if (iter.init() == noErr) {
		//sp-30
		SetFrameSlot(info, SYMA(size), MAKEINT(iter.packageSize()));
		SetFrameSlot(info, SYMA(title), MakeString(iter.packageName()));
		SetFrameSlot(info, SYMA(version), MAKEINT(iter.getVersion()));
		SetFrameSlot(info, SYMA(timestamp), MAKEINT(iter.modifyDate()));
		SetFrameSlot(info, SYMA(creationdate), MAKEINT(iter.creationDate()/60));
		SetFrameSlot(info, SYMA(dispatchonly), MAKEBOOLEAN(iter.forDispatchOnly()));
		SetFrameSlot(info, SYMA(copyProtection), MAKEBOOLEAN(iter.copyProtected()));
		SetFrameSlot(info, SYMA(flags), MAKEINT(iter.packageFlags()>>4));
		SetFrameSlot(info, SYMA(copyright), MakeString(iter.copyright()));
		SetFrameSlot(info, SYMA(compressed), MAKEBOOLEAN((iter.packageFlags() & kNoCompressionFlag) == 0));
		SetFrameSlot(info, SYMA(cmprsdSz), MAKEINT(StorageSizeOfLargeObject((VAddr)inAddr)));
		SetFrameSlot(info, SYMA(numparts), MAKEINT(iter.numberOfParts()));

		RefVar partsArray(MakeArray(0));
		RefVar partTypesArray(MakeArray(0));
		SetFrameSlot(info, SYMA(parts), partsArray);
		SetFrameSlot(info, SYMA(parttypes), partTypesArray);

		for (ArrayIndex i = 0, count = iter.numberOfParts(); i < count; ++i) {
			PartInfo part;
			iter.getPartInfo(i, &part);
			//sp-08
			RefVar partType, thisPart;
			if (part.notify) {
				char name[5];
				*(ULong *)name = part.type;
				name[4] = 0;
				partType = MakeSymbol(name);
			}
			switch (part.kind) {
			case kProtocol:
//				spm00 = MakeStringFromCString(*((CProtocol *)part.data.address)::classInfo->implementationName());
				if (ISNIL(partType)) {
					partType = SYMA(protocol);
				}
				break;
			case kFrames:
				if (ISNIL(partType)) {
					partType = SYMA(frame);
				}
				break;
			case kRaw:
//				thisPart = MAKEINT();
				if (ISNIL(partType)) {
					partType = SYMA(raw);
				}
				break;
			}
			AddArraySlot(partTypesArray, partType);
			AddArraySlot(partsArray, thisPart);
		}
	}
#endif
	return info;
}


/* -----------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n s
----------------------------------------------------------------------------- */
bool	IsPackageHeader(Ptr inData, size_t inSize);

extern "C" {
Ref	FObjectPkgRef(RefArg rcvr, RefArg inPkg);
Ref	FGetPkgRefInfo(RefArg rcvr, RefArg inPkg);
Ref	FGetPkgInfoFromPSSid(RefArg rcvr, RefArg inArg1, RefArg inArg2);
Ref	FIsValid(RefArg rcvr, RefArg inPkg);
}

Ref
FObjectPkgRef(RefArg rcvr, RefArg inPkg)
{ return NILREF; }

Ref
FGetPkgRefInfo(RefArg rcvr, RefArg inPkg)
{
	ArrayIndex pkgSize = Length(inPkg);
	if (!IsPackageHeader(BinaryData(inPkg), pkgSize)) {
		ThrowBadTypeWithFrameData(kNSErrNotAPackage, inPkg);
	}
	RefVar info;
	unwind_protect
	{
		LockRef(inPkg);
		info = GetPkgInfoFromVAddr(BinaryData(inPkg));
	}
	on_unwind
	{
		UnlockRef(inPkg);
	}
	end_unwind;
	return info;
}

Ref
FGetPkgInfoFromPSSid(RefArg rcvr, RefArg inArg1, RefArg inArg2)
{ return NILREF; }

Ref
FIsValid(RefArg rcvr, RefArg inPkg)
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
	Args:		rcvr			a store frame
				inBinary		the binary object containing the package
				inParms		params frame
	Return:	a package frame
------------------------------------------------------------------------------- */

Ref
StoreSuckPackageFromBinary(RefArg rcvr, RefArg inBinary, RefArg inParms)
{
	RefVar pkg;
#if !defined(forFramework)
	size_t		binSize = Length(inBinary);
	CDataPtr		binData(inBinary);

	CBufferSegment * buf = CBufferSegment::make();
	buf->init((char *)binData, binSize);

	CMemoryPipe pipe;
	pipe.init(buf, NULL, false);

	pkg = SuckPackageThruPipe(&pipe, rcvr, inParms);

	buf->destroy();
#endif
	return pkg;
}


/* -------------------------------------------------------------------------------
	Suck a package from an endpoint.
	Args:		rcvr			a store frame
				inBinary		the binary object containing the package
				inParms		params frame
	Return:	a package frame
------------------------------------------------------------------------------- */

Ref
StoreSuckPackageFromEndpoint(RefArg rcvr, RefArg inEndpoint, RefArg inParms)
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
			pkg = AllocatePackage(&epp, rcvr, inParms);
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

