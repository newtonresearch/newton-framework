/*
	File:		LargeObjectStore.cc

	Contains:	Newton large object store interface.

	Written by:	Newton Research Group, 2007.
*/
#undef forDebug
#undef debugLevel

#include "Objects.h"
#include "LargeObjects.h"
#include "LargeObjectStore.h"
#include "CachedReadStore.h"
#include "StoreCompander.h"
#include "ROMResources.h"
#include "Funcs.h"
#include "OSErrors.h"
extern void DumpHex(void * inBuf, size_t inLen);


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

PackageRoot::PackageRoot()
{
	fDataId = 0;
	fCompanderNameId = 0;
	fCompanderParmsId = 0;
	flags = 1;
	fSignature = 0;
}

LargeObjectRoot::LargeObjectRoot()
{
	flags = 2;
	fActualSize = 0;
	f18 = 0;
	f1C = 0;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C a l l b a c k
	canonicalPackageCallbackInfo := {
		packageName: NULL,
		packageSize: 0,
		numberOfParts: 0,
		currentPartNumber: 0,
		amountRead: 0 };
------------------------------------------------------------------------------*/

void
CLOCallback::callback(LOCallbackInfo * info)
{
	if (ISNIL(fInfo))
	{
		fInfo = Clone(RA(canonicalPackageCallbackInfo));
		SetFrameSlot(fInfo, SYMA(packageSize), MAKEINT(info->pkgSize));
		SetFrameSlot(fInfo, SYMA(numberOfParts), MAKEINT(info->numOfParts));
		if (info->pkgName != NULL)
			SetFrameSlot(fInfo, SYMA(packageName), MakeString(info->pkgName));
	}
	else if (NOTNIL(fCallback))
	{
		RefVar args(MakeArray(1));
		SetFrameSlot(fInfo, SYMA(currentPartNumber), MAKEINT(info->partNumber));
		SetFrameSlot(fInfo, SYMA(amountRead), MAKEINT(info->sizeDone));
		SetArraySlot(args, 0, fInfo);
		newton_try
		{
			DoBlock(fCallback, args);
		}
		newton_catch("")
		{ }
		end_try;
	}
}

#pragma mark -

/*------------------------------------------------------------------------------
	A large binary object is stored in 1K (decompressed) chunks.
	We need to create a master array of ids pointing to these chunks (each chunk
	initially of zero length).
	Args:		inStore			store on which to create LBO
				outId				on return, id of chunk array
				inSize			size of object for which to create chunks
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
InitializeChunkArray(CStore * inStore, PSSId * outId, size_t inSize)
{
	NewtonErr	err;
	ArrayIndex	chunkIndex = 0;
	ArrayIndex	numOfChunks = (inSize + kSubPageSize-1)/kSubPageSize;
	ArrayIndex	chunkArraySize = numOfChunks * sizeof(PSSId);
	PSSId *		chunkArray;

PRINTF(("InitializeChunkArray(size=%lu)\n", inSize));

	XTRY
	{
		chunkArray = new PSSId[numOfChunks]();		// default constructor will zero each PSSId
		XFAIL(err = MemError())
		XFAIL(err = inStore->newWithinTransaction(outId, chunkArraySize))
		for (chunkIndex = 0; chunkIndex < numOfChunks; ++chunkIndex)
		{
			XFAIL(err = inStore->newWithinTransaction(chunkArray+chunkIndex, 0))
		}
		err = inStore->write(*outId, 0, chunkArray, chunkArraySize);
	}
	XENDTRY;

	XDOFAIL(err)
	{
		for (ArrayIndex i = 0; i < chunkIndex; ++i)
			if (chunkArray[i] != 0)
				inStore->separatelyAbort(chunkArray[i]);
	}
	XENDFAIL;

	delete[] chunkArray;
	return err;
}


/*------------------------------------------------------------------------------
	Fill the chunks array with uncompressed data read from a pipe.
	Args:		inStore					store on which the LBO resides
				inObjId					id of the final object
				inChunksId				id of the chunks of compressed data
				inPipe					source for the uncompressed data
				inSize					size of the final object
				inCompanderName		compander to use to compress source
				inCompanderParmsId	id of parms for compander
				inCallback				callback object
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
FillChunkArray(CStore * inStore, PSSId inObjId, PSSId inChunksId, CPipe * inPipe, size_t inSize, const char * inCompanderName, PSSId inCompanderParmsId, CLOCallback * inCallback)
{
	NewtonErr	err;
	size_t		chunkIndex, numOfChunks;
	size_t		chunkSize;
	char *		chunk;
	CStoreCompander * compander;

	XTRY
	{
		// create an actual 1K chunk
		chunk = new char[kSubPageSize];
		XFAIL(err = MemError())

		// create the required compander
		compander = (CStoreCompander *)MakeByName("CStoreCompander", inCompanderName);
		XFAILNOT(compander, err = kOSErrNoMemory;)
		XFAIL(err = compander->init(inStore, inObjId, inCompanderParmsId, false, false))

		// fill the chunks object we already have on store
		XFAIL(err = inStore->getObjectSize(inChunksId, &numOfChunks))
		// convert size of array to number of entries
		numOfChunks /= sizeof(PSSId);

		bool		isEOF = false;
		size_t	sizeDone = 0;
		size_t	sizeRemaining = inSize;
		size_t	cbSizeDone = 0;
		LOCallbackInfo	cbInfo;
		cbInfo.pkgSize = inSize;
		cbInfo.sizeDone = 0;
		cbInfo.pkgName = NULL;
		cbInfo.partNumber = 0;
		cbInfo.numOfParts = 0;
		for (chunkIndex = 0; chunkIndex < numOfChunks; ++chunkIndex)
		{
			chunkSize = (sizeRemaining > kSubPageSize) ? kSubPageSize : sizeRemaining;
			newton_try
			{
				inPipe->readChunk(chunk, chunkSize, isEOF);
			}
			newton_catch(exPipe)
			{
				err = (NewtonErr)(long)CurrentException()->data;;
			}
			end_try;
			XFAIL(err)

			PSSId	objId;
			// preflight check
			XFAIL(err = inStore->read(inChunksId, chunkIndex*sizeof(PSSId), &objId, sizeof(objId)))
			// write compressed chunk
			XFAIL(err = compander->write(chunkIndex*kSubPageSize, chunk, chunkSize, 0))
			sizeRemaining -= chunkSize;
			sizeDone += chunkSize;
			cbSizeDone += chunkSize;
			if (inCallback != NULL
			&&  cbSizeDone > inCallback->frequency())	// time to call back?
			{
				cbInfo.sizeDone = sizeDone;
				inCallback->callback(&cbInfo);
				cbSizeDone = 0;
			}
		}
	}
	XENDTRY;

	XDOFAIL(err)
	{
		PSSId	objId;
		for (chunkIndex = 0; chunkIndex < numOfChunks; ++chunkIndex)
		{
			XFAIL(err = inStore->read(inChunksId, chunkIndex*sizeof(PSSId), &objId, sizeof(objId)))
			XFAIL(err = inStore->separatelyAbort(objId)) 
		}
	}
	XENDFAIL;

	if (compander != NULL)
		compander->destroy();
	delete[] chunk;
	return err;
}


/*------------------------------------------------------------------------------
	Fill the chunks array with compressed data read from a pipe.
	Args:		inStore					store on which the LBO resides
				inChunksId				id of the chunks of compressed data
				inPipe					source for the compressed data
				inSize					size of the compressed data
				inCallback				callback object
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
FillChunkArrayCompressed(CStore * inStore, PSSId inChunksId, CPipe * inPipe, size_t inSize, CLOCallback * inCallback)
{
	NewtonErr	err;
	size_t		chunkIndex, numOfChunks;
	size_t		chunkSize;
	char *		chunk;

PRINTF(("FillChunkArrayCompressed(id=%u,size=%lu)\n", inChunksId,inSize));

	XTRY
	{
		// create a compressed chunk of the largest possible size
		chunk = new char[0x0520];
		XFAIL(err = MemError())

		// fill the chunks object we already have on store
		XFAIL(err = inStore->getObjectSize(inChunksId, &numOfChunks))
		// convert size of array to number of entries
		numOfChunks /= sizeof(PSSId);

		bool		isEOF = false;
		size_t	sizeDone = 0;
		size_t	cbSizeDone = 0;
		LOCallbackInfo	cbInfo;
		cbInfo.pkgSize = inSize;
		cbInfo.sizeDone = 0;
		cbInfo.pkgName = NULL;
		cbInfo.partNumber = 0;
		cbInfo.numOfParts = 0;
		for (chunkIndex = 0; chunkIndex < numOfChunks; ++chunkIndex)
		{
			newton_try
			{
				*inPipe >> chunkSize;
PRINTF(("\n%lu: pipe->readChunk(size=%lu)\n", chunkIndex,chunkSize));
				inPipe->readChunk(chunk, chunkSize, isEOF);
#if forDebug
DumpHex(chunk, chunkSize);
#endif
			}
			newton_catch(exPipe)
			{
				err = (NewtonErr)(long)CurrentException()->data;
			}
			end_try;
			XFAIL(err)

			PSSId	objId;
			XFAIL(err = inStore->read(inChunksId, chunkIndex*sizeof(PSSId), &objId, sizeof(objId)))
PRINTF(("   -> id=%d\n", objId));
			XFAIL(err = inStore->replaceObject(objId, chunk, chunkSize))
			sizeDone += (sizeof(chunkSize) + chunkSize);
			cbSizeDone += (sizeof(chunkSize) + chunkSize);
			if (inCallback != NULL
			&&  cbSizeDone > inCallback->frequency())	// time to call back?
			{
				cbInfo.sizeDone = sizeDone;
				inCallback->callback(&cbInfo);
				cbSizeDone = 0;
			}
		}
	}
	XENDTRY;

	XDOFAIL(err)
	{
		PSSId	objId;
		for (chunkIndex = 0; chunkIndex < numOfChunks; ++chunkIndex)
		{
			XFAIL(err = inStore->read(inChunksId, chunkIndex*sizeof(PSSId), &objId, sizeof(objId)))
			XFAIL(err = inStore->separatelyAbort(objId))
		}
	}
	XENDFAIL;

	delete[] chunk;
	return err;
}

#pragma mark -

NewtonErr
CopyPackageData(PackageRoot * ioRoot, CStore * inStore, CStore * intoStore, bool inTransaction)
{
	NewtonErr	err;
	PSSId			dupId = 0;
	ArrayIndex	chunkIndex = 0;
	PSSId *		dupDataIds = NULL;
	PSSId *		dataIds = NULL;
	char *		data = NULL;
	size_t		numOfChunks;

	XTRY
	{
		XFAIL(err = inStore->getObjectSize(ioRoot->fDataId, &numOfChunks))
		XFAIL(err = inTransaction ? intoStore->newWithinTransaction(&dupId, numOfChunks) : intoStore->newObject(&dupId, numOfChunks))
		numOfChunks /= sizeof(PSSId);

		dupDataIds = new PSSId[numOfChunks];
		XFAIL(err = MemError())

		dataIds = new PSSId[numOfChunks];
		XFAIL(err = MemError())

		XFAIL(err = inStore->read(ioRoot->fDataId, 0, dataIds, numOfChunks * sizeof(PSSId)))
		data = new char[sizeof(size_t) + 0x0520];
		for (chunkIndex = 0; chunkIndex < numOfChunks; ++chunkIndex)
		{
			size_t	chunkSize;
			PSSId		dupChunkId;
			XFAIL(err = inStore->getObjectSize(dataIds[chunkIndex], &chunkSize))
			XFAILIF(chunkSize > (sizeof(size_t) + 0x0520), err = kOSErrNoMemory;)
			XFAIL(err = inTransaction ? intoStore->newWithinTransaction(&dupChunkId, chunkSize) : intoStore->newObject(&dupChunkId, chunkSize))
			XFAIL(err = inStore->read(dataIds[chunkIndex], 0, data, chunkSize))
			XFAIL(err = intoStore->write(dupChunkId, 0, data, chunkSize))
		}
		XFAIL(err = intoStore->write(dupId, 0, dupDataIds, numOfChunks * sizeof(PSSId)))
		ioRoot->fDataId = dupId;
	}
	XENDTRY;

	XDOFAIL(err)
	{
		if (inTransaction)
		{
			if (dupId)
				intoStore->separatelyAbort(dupId);
			for (ArrayIndex i = 0; i < chunkIndex; ++i)
				intoStore->separatelyAbort(dupDataIds[i]);
		}
	}
	XENDFAIL;

	delete[] data;
	delete[] dataIds;
	delete[] dupDataIds;

	return err;
}


NewtonErr
DuplicatePackageData(CStore * inStore, PSSId inId, CStore * intoStore, PSSId * outId, bool inTransaction)
{
	NewtonErr	err;
	LargeObjectRoot	root;
	PSSId			dupObjId = 0;
	PSSId			dupCompanderNameId = 0;
	PSSId			dupCompanderParmsId = 0;
	char *		companderName = NULL;
	char *		companderParms = NULL;
//	char *		sp00 = NULL;	// ??
	bool			isLargeObject;
	int			objType;
	size_t		objSize;

	XTRY
	{
		XFAIL(err = inStore->read(inId, 0, &root, sizeof(PackageRoot)))
		objType = (root.flags & 0xFFFF);
		XFAILIF(objType > 2, err = kOSErrBadPackage;)
		isLargeObject = (objType == 2);
		if (isLargeObject)
			XFAIL(err = inStore->read(inId, 0, &root, sizeof(LargeObjectRoot)))

		XFAIL(err = inStore->getObjectSize(root.fCompanderNameId, &objSize))
		companderName = new char[objSize+1];
		XFAIL(err = MemError())
		XFAIL(err = inStore->read(root.fCompanderNameId, 0, companderName, objSize))
		companderName[objSize] = 0;

		if (!inTransaction)
			inStore->lockStore();

		objSize = isLargeObject ? sizeof(LargeObjectRoot) : sizeof(PackageRoot);
		XFAIL(err = inTransaction ? intoStore->newWithinTransaction(&dupObjId, objSize) : intoStore->newObject(&dupObjId, objSize))

		objSize = strlen(companderName);
		XFAIL(err = inTransaction ? intoStore->newWithinTransaction(&dupCompanderNameId, objSize) : intoStore->newObject(&dupCompanderNameId, objSize))
		XFAIL(err = intoStore->write(dupCompanderNameId, 0, companderName, strlen(companderName)))
		root.fCompanderNameId = dupCompanderNameId;

		if (root.fCompanderParmsId)
		{
			XFAIL(err = inStore->getObjectSize(root.fCompanderParmsId, &objSize))
			companderParms = new char[objSize];
			XFAIL(err = MemError())
			XFAIL(err = inTransaction ? intoStore->newWithinTransaction(&dupCompanderParmsId, objSize) : intoStore->newObject(&dupCompanderParmsId, objSize))
			XFAIL(err = inStore->read(root.fCompanderParmsId, 0, companderParms, objSize))
			XFAIL(err = intoStore->write(dupCompanderParmsId, 0, companderParms, objSize))
			root.fCompanderParmsId = dupCompanderParmsId;
		}

		XFAIL(err = CopyPackageData(&root, inStore, intoStore, inTransaction))
		objSize = isLargeObject ? sizeof(LargeObjectRoot) : sizeof(PackageRoot);
		XFAIL(err = intoStore->write(dupObjId, 0, &root, objSize))

		if (!inTransaction)
			inStore->unlockStore();
	}
	XENDTRY;

	XDOFAIL(err)
	{
		if (inTransaction)
		{
			if (dupObjId)
				intoStore->separatelyAbort(dupObjId);
			if (dupCompanderNameId)
				intoStore->separatelyAbort(dupCompanderNameId);
			if (dupCompanderParmsId)
				intoStore->separatelyAbort(dupCompanderParmsId);
		}
		else
			intoStore->abort();
	}
	XENDFAIL;

	delete[] companderName;
	delete[] companderParms;
//	delete sp00;	// ??

	*outId = dupObjId;
	return err;
}

#pragma mark -

/*------------------------------------------------------------------------------
	L O   F u n c t i o n s
------------------------------------------------------------------------------*/
NewtonErr	LOCompanderName(CStore * inStore, PSSId inId, char ** outNamePtr);

NewtonErr
GetLOAllocator(CStore * inStore, PSSId inId, CLrgObjStore ** outObjStore)
{
	NewtonErr		err;
	char *			companderName = NULL;
	CLrgObjStore *	objStore = NULL;

	XTRY
	{
		*outObjStore = objStore;
		XFAIL(err = LOCompanderName(inStore, inId, &companderName))
		objStore = (CLrgObjStore *)MakeByName("CLrgObjStore", NULL, companderName);
		*outObjStore = objStore;
		if (objStore != NULL)
			XFAILIF(err = objStore->init(), objStore->destroy();)
		else
			XFAILNOT(ClassInfoByName("CStoreCompander", companderName), err = kOSErrNoMemory;)
	}
	XENDTRY;
	
	XDOFAIL(companderName)
	{
		delete companderName;
	}
	XENDFAIL;

	return err;
}


NewtonErr
LOCompanderNameStrLen(CStore * inStore, PSSId inId, size_t * outSize)
{
	NewtonErr	err;

	XTRY
	{
		PackageRoot	root;
		XFAIL(err = inStore->read(inId, 0, &root, sizeof(root)))
		err = inStore->getObjectSize(root.fCompanderNameId, outSize);
	}
	XENDTRY;

	return err;
}


NewtonErr
LOCompanderName(CStore * inStore, PSSId inId, char * outName)
{
	NewtonErr	err;

	XTRY
	{
		size_t	objSize;
		PackageRoot	root;
		XFAIL(err = inStore->read(inId, 0, &root, sizeof(root)))
		XFAIL(err = inStore->getObjectSize(root.fCompanderNameId, &objSize))
		err = inStore->read(root.fCompanderNameId, 0, outName, objSize);
		outName[objSize] = 0;
	}
	XENDTRY;

	return err;
}


NewtonErr
LOCompanderName(CStore * inStore, PSSId inId, char ** outNamePtr)
{
	NewtonErr	err;

	XTRY
	{
		size_t	nameSize;
		XFAIL(err = LOCompanderNameStrLen(inStore, inId, &nameSize))
		*outNamePtr = new char[nameSize+1];
		XFAIL(err = MemError())
		XFAILIF(err = LOCompanderName(inStore, inId, *outNamePtr), delete[]*outNamePtr; *outNamePtr = NULL;)
	}
	XENDTRY;

	return err;
}


NewtonErr
LOCompanderParameterSize(CStore * inStore, PSSId inId, size_t * outSize)
{
	NewtonErr	err;

	XTRY
	{
		PackageRoot	root;
		XFAIL(err = inStore->read(inId, 0, &root, sizeof(root)))
		if (root.fCompanderParmsId != 0)
			err = inStore->getObjectSize(root.fCompanderParmsId, outSize);
	}
	XENDTRY;

	return err;
}


NewtonErr
LOCompanderParameters(CStore * inStore, PSSId inId, void * outParms)
{
	NewtonErr	err;

	XTRY
	{
		PackageRoot	root;
		XFAIL(err = inStore->read(inId, 0, &root, sizeof(root)))
		if (root.fCompanderParmsId != 0)
		{
			size_t	objSize;
			XFAIL(err = inStore->getObjectSize(root.fCompanderParmsId, &objSize))
			err = inStore->read(root.fCompanderParmsId, 0, outParms, objSize);
		}
	}
	XENDTRY;

	return err;
}


size_t
LOSizeOfStream(CStore * inStore, PSSId inId, bool inCompressed)
{
	size_t	streamSize = 0;
	char *	companderName = NULL;

	XTRY
	{
		XFAIL(LOCompanderName(inStore, inId, &companderName))

		CLrgObjStore * objStore = (CLrgObjStore *)MakeByName("CLrgObjStore", NULL, companderName);
		if (objStore != NULL)
		{
			XFAILIF(objStore->init(), objStore->destroy();)
			streamSize = objStore->sizeOfStream(inStore, inId, inCompressed);
			objStore->destroy();
		}
		else
		{
			XFAIL(ClassInfoByName("CStoreCompander", companderName) == NULL)
			streamSize = LODefaultStreamSize(inStore, inId, inCompressed);
		}
	}
	XENDTRY;

	if (companderName != NULL)
		delete[] companderName;

	return streamSize;
}


NewtonErr
LOWrite(CPipe * inPipe, CStore * inStore, PSSId inId, bool inCompressed, CLOCallback * inCallback)
{
	NewtonErr	err = noErr;
	char *		companderName = NULL;

	XTRY
	{
		XFAIL(err = LOCompanderName(inStore, inId, &companderName))

		CLrgObjStore * objStore = (CLrgObjStore *)MakeByName("CLrgObjStore", NULL, companderName);
		if (objStore != NULL)
		{
			XFAILIF(err = objStore->init(), objStore->destroy();)
			err = objStore->backup(inPipe, inStore, inId, inCompressed, inCallback);
			objStore->destroy();
		}
		else
		{
			XFAILNOT(ClassInfoByName("CStoreCompander", companderName), err = kOSErrNoMemory;)
			err = LODefaultBackup(inPipe, inStore, inId, inCompressed, inCallback);
		}
	}
	XENDTRY;

	if (companderName != NULL)
		delete[] companderName;

	return err;
}


NewtonErr
LODeleteByProtocol(CStore * inStore, PSSId inId)
{
	NewtonErr	err;
	CLrgObjStore * objStore;
	GetLOAllocator(inStore, inId, &objStore);
	if (objStore != NULL)
	{
		err = objStore->deleteObject(inStore, inId);
		objStore->destroy();
	}
	else
		err = LODefaultDelete(inStore, inId);
	return err;
}

#pragma mark -

/*------------------------------------------------------------------------------
	L O D e f a u l t s
------------------------------------------------------------------------------*/

NewtonErr
LODefaultCreate(PSSId * outId, CStore * inStore, CPipe * inPipe, size_t inSize, bool inReadOnly, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CLOCallback * inCallback)
{
	NewtonErr			err;
	LargeObjectRoot	root;
	PSSId					rootId = 0;

	XTRY
	{
		XFAIL(err = inStore->newWithinTransaction(&rootId, sizeof(root)))
		*outId = rootId;

		// add compander name to store
		if (inCompanderName == NULL)
			inCompanderName = "CSimpleStoreCompander";
		XFAIL(err = inStore->newWithinTransaction(&root.fCompanderNameId, strlen(inCompanderName)))
		XFAIL(err = inStore->write(root.fCompanderNameId, 0, (void *)inCompanderName, strlen(inCompanderName)))

		// add compander parameters (if any) to store
		XFAIL(err = inStore->newWithinTransaction(&root.fCompanderParmsId, inCompanderParmSize))
		if (inCompanderParmSize > 0)
			XFAIL(err = inStore->write(root.fCompanderParmsId, 0, inCompanderParms, inCompanderParmSize))

		XFAIL(err = InitializeChunkArray(inStore, &root.fDataId, inSize))
		root.flags = inReadOnly ? 0x00000002 : 0x00010002;
		root.fActualSize = inSize;
		XFAIL(err = inStore->write(rootId, 0, &root, sizeof(root)))

		if (inPipe != NULL)
			XFAIL(err = FillChunkArray(inStore, rootId, root.fDataId, inPipe, inSize, inCompanderName, root.fCompanderParmsId, inCallback))

		// at this point everything is okay so make the signature good
		root.fSignature = 'paok';
		err = inStore->write(rootId, 0, &root, sizeof(root));
	}
	XENDTRY;

	XDOFAIL(err)
	{
		if (root.fCompanderParmsId != 0)
			inStore->separatelyAbort(root.fCompanderParmsId);
		if (root.fCompanderNameId != 0)
			inStore->separatelyAbort(root.fCompanderNameId);
		if (root.fDataId != 0)
			inStore->separatelyAbort(root.fDataId);
		if (rootId != 0)
			inStore->separatelyAbort(rootId);
	}
	XENDFAIL;

	return err;
}


NewtonErr
LODefaultCreateFromCompressed(PSSId * outId, CStore * inStore, CPipe * inPipe, size_t inSize, bool inReadOnly, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CLOCallback * inCallback)
{
	NewtonErr			err = noErr;
	LargeObjectRoot	root;
	PSSId					rootId = 0;
	unsigned long		flags;
	size_t				fullSize;	// decompressed

	XTRY
	{
		// read header
		newton_try
		{
			*inPipe >> flags;
			*inPipe >> fullSize;
		}
		newton_catch(exPipe)
		{
			err = (NewtonErr)(long)CurrentException()->data;
		}
		end_try;
		XFAIL(err)

		XFAIL(err = inStore->newWithinTransaction(&rootId, sizeof(root)))
		*outId = rootId;

		// add compander name to store
		XFAIL(err = inStore->newWithinTransaction(&root.fCompanderNameId, strlen(inCompanderName)))
		XFAIL(err = inStore->write(root.fCompanderNameId, 0, (void *)inCompanderName, strlen(inCompanderName)))

		// add compander parameters (if any) to store
		XFAIL(err = inStore->newWithinTransaction(&root.fCompanderParmsId, inCompanderParmSize))
		if (inCompanderParmSize > 0)
			XFAIL(err = inStore->write(root.fCompanderParmsId, 0, inCompanderParms, inCompanderParmSize))

		XFAIL(err = InitializeChunkArray(inStore, &root.fDataId, fullSize))
		XFAIL(err = FillChunkArrayCompressed(inStore, root.fDataId, inPipe, inSize, inCallback))

		// at this point everything is okay so make the signature good
		root.flags = inReadOnly ? flags & ~0x00010000 : flags;
		root.fActualSize = fullSize;
		root.fSignature = 'paok';
		err = inStore->write(rootId, 0, &root, sizeof(root));
	}
	XENDTRY;

	XDOFAIL(err)
	{
		if (root.fCompanderParmsId != 0)
			inStore->separatelyAbort(root.fCompanderParmsId);
		if (root.fCompanderNameId != 0)
			inStore->separatelyAbort(root.fCompanderNameId);
		if (root.fDataId != 0)
			inStore->separatelyAbort(root.fDataId);
		if (rootId != 0)
			inStore->separatelyAbort(rootId);
	}
	XENDFAIL;

	return err;
}


NewtonErr
LODefaultDelete(CStore * inStore, PSSId inId)
{
	return DeallocatePackage(inStore, inId);
}


NewtonErr
LODefaultDuplicate(PSSId * outId, CStore * inStore, PSSId inId, CStore * intoStore)
{
	NewtonErr	err;
	if ((err = FlushLargeObject(inStore, inId)) == noErr)
		err = DuplicatePackageData(inStore, inId, intoStore, outId, true);
	return err;
}


NewtonErr
RemoveIndexTable(CStore * inStore, PSSId inId)
{
	NewtonErr	err = noErr;
	CCachedReadStore *	cachedStore = new CCachedReadStore(inStore, inId, kUseObjectSize);
	if (cachedStore)
	{
		PSSId *	objId;
		size_t	objSize;
		for (ArrayIndex i = 0, count = cachedStore->getDataSize() / sizeof(PSSId); i < count; ++i)
		{
			if (cachedStore->getDataPtr(i*sizeof(PSSId), sizeof(PSSId), (void**)&objId) == noErr
			&&  *objId != 0)
				inStore->deleteObject(*objId);
		}
		inStore->deleteObject(inId);
		delete cachedStore;
	}
	else
		err = kOSErrNoMemory;

	return err;
}


size_t
GetPagesSize(CStore * inStore, PSSId inId)
{
	size_t		sizeOfPages = 0;

	CCachedReadStore *	cachedStore = new CCachedReadStore(inStore, inId, kUseObjectSize);
	if (cachedStore)
	{
		PSSId *	objId;
		size_t	objSize;
		for (ArrayIndex i = 0, count = cachedStore->getDataSize() / sizeof(PSSId); i < count; ++i)
		{
			if (cachedStore->getDataPtr(i*sizeof(PSSId), sizeof(PSSId), (void**)&objId) == noErr
			&&  inStore->getObjectSize(*objId, &objSize) == noErr)
				sizeOfPages += objSize;
		}
		delete cachedStore;
	}

	return sizeOfPages;
}


size_t
LODefaultStorageSize(CStore * inStore, PSSId inId)
{
	NewtonErr	err;
	size_t		sizeOfObj;
	size_t		sizeOfParms;
	size_t		sizeOfStorage = 0;

	XTRY
	{
		PackageRoot	root;
		XFAIL(err = inStore->read(inId, 0, &root, sizeof(root)))
		XFAIL(err = inStore->getObjectSize(inId, &sizeOfStorage))
		if (root.fCompanderParmsId)
		{
			XFAIL(err = inStore->getObjectSize(root.fCompanderParmsId, &sizeOfParms))
			sizeOfStorage += sizeOfParms;
		}
		XFAIL(err = inStore->getObjectSize(root.fDataId, &sizeOfObj))
		sizeOfStorage += (GetPagesSize(inStore, root.fDataId) + sizeOfObj);
	}
	XENDTRY;

	return sizeOfStorage;
}


size_t
LODefaultStreamSize(CStore * inStore, PSSId inId, bool inCompressed)
{
	NewtonErr	err;
	size_t		sizeOfStream = 0;
	size_t		sizeOfObj;

	XTRY
	{
		if (inCompressed)
		{
			PackageRoot	root;
			XFAIL(err = inStore->read(inId, 0, &root, sizeof(root)))
			XFAIL(err = inStore->getObjectSize(root.fDataId, &sizeOfObj))
			sizeOfStream = sizeof(ULong)/*flags*/ + sizeof(size_t)/*size*/ + GetPagesSize(inStore, root.fDataId) + TRUNC(sizeOfObj, 4);
		}
		else
		{
			bool	isMapped = false;
			VAddr addr;
			if ((err = StoreToVAddr(&addr, inStore, inId)) != noErr)
			{
				XFAIL(err = MapLargeObject(&addr, inStore, inId, true))
				isMapped = true;
			}
			sizeOfStream = ObjectSize(addr);
			if (isMapped)
				UnmapLargeObject(addr);
		}
	}
	XENDTRY;

	XDOFAIL(err)
	{
		sizeOfStream = 0;
	}
	XENDFAIL;

	return sizeOfStream;
}


NewtonErr
LODefaultBackup(CPipe * inPipe, CStore * inStore, PSSId inId, bool inCompressed, CLOCallback * inCallback)
{
	NewtonErr	err = noErr;	// sp20
	VAddr			addr;				// sp1C
	size_t		objSize;			// sp18
	bool			isMapped = false;	// sp14
	char *		chunk;
	size_t		numOfChunks;

	XTRY
	{
		chunk = new char[0x0520];
		XFAILIF(chunk == NULL, err = MemError();)

		if ((err = StoreToVAddr(&addr, inStore, inId)) != noErr)
		{
			XFAIL(err = MapLargeObject(&addr, inStore, inId, true))
			isMapped = true;
		}
		objSize = ObjectSize(addr);
		if (inCompressed)
		{
			PackageRoot	root;
			XFAIL(err = inStore->read(inId, 0, &root, sizeof(root)))
			XFAIL(err = inStore->getObjectSize(root.fDataId, &numOfChunks))
			numOfChunks /= sizeof(PSSId);
			newton_try
			{
				*inPipe << root.flags;	// MUST be unsigned longs
				*inPipe << objSize;
			}
			newton_catch(exPipe)
			{
				err = (NewtonErr)(long)CurrentException()->data;
			}
			end_try;
			XFAIL(err)

			if (numOfChunks > 0)
			{
				size_t	sizeDone = 0;
				size_t	cbSizeDone = 0;
				LOCallbackInfo	cbInfo;
				cbInfo.pkgSize = objSize;
				cbInfo.sizeDone = 0;
				cbInfo.pkgName = NULL;
				cbInfo.partNumber = 0;
				cbInfo.numOfParts = 0;
				for (ArrayIndex chunkIndex = 0; chunkIndex < numOfChunks; ++chunkIndex)
				{
					PSSId objId;
					XFAIL(err = inStore->read(root.fDataId, chunkIndex*sizeof(PSSId), &objId, sizeof(objId)))
					XFAIL(err = inStore->getObjectSize(objId, &objSize))
					XFAIL(err = inStore->read(objId, 0, chunk, objSize))

					newton_try
					{
						*inPipe << objSize;
						inPipe->writeChunk(chunk, objSize, false);
					}
					newton_catch(exPipe)
					{
						err = (NewtonErr)(long)CurrentException()->data;
					}
					end_try;
					XFAIL(err)

					sizeDone += (sizeof(objSize) + objSize);
					cbSizeDone += (sizeof(objSize) + objSize);
					if (inCallback != NULL
					&&  cbSizeDone > inCallback->frequency())	// time to call back?
					{
						cbInfo.sizeDone = sizeDone;
						inCallback->callback(&cbInfo);
						cbSizeDone = 0;
					}
				}
			}
		}
		else
		{
			newton_try
			{
				inPipe->writeChunk((const void *)addr, objSize, false);
			}
			newton_catch(exPipe)
			{
				err = (NewtonErr)(long)CurrentException()->data;
			}
			end_try;
		}
	}
	XENDTRY;

	if (isMapped)
		UnmapLargeObject(addr);
	if (chunk)
		delete[] chunk;

	return err;
}


NewtonErr
LODefaultDoTransaction(CStore * inStore, PSSId, PSSId, int, bool)
{
	// this really does nothing
	return noErr;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C L O P a c k a g e S t o r e
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CLOPackageStore implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CLOPackageStore::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN15CLOPackageStore6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN15CLOPackageStore4makeEv - 0b	\n"
"		.long		__ZN15CLOPackageStore7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CLOPackageStore\"	\n"
"2:	.asciz	\"CLrgObjStore\"	\n"
"3:	.asciz	\"CZippyRelocStoreDecompressor\", \"\"	\n"
"		.asciz	\"CZippyStoreDecompressor\", \"\"	\n"
"		.asciz	\"CLZRelocStoreDecompressor\", \"\"	\n"
"		.asciz	\"CLZStoreDecompressor\", \"\"	\n"
"		.asciz	\"CSimpleRelocStoreDecompressor\", \"\"	\n"
"		.asciz	\"CSimpleStoreDecompressor\", \"\"	\n"
"		.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN15CLOPackageStore9classInfoEv - 4b	\n"
"		.long		__ZN15CLOPackageStore4makeEv - 4b	\n"
"		.long		__ZN15CLOPackageStore7destroyEv - 4b	\n"
"		.long		__ZN15CLOPackageStore4initEv - 4b	\n"
"		.long		__ZN15CLOPackageStore6createEPjP6CStoreP5CPipembPKcPvmP11CLOCallback - 4b	\n"
"		.long		__ZN15CLOPackageStore20createFromCompressedEPjP6CStoreP5CPipembPKcPvmP11CLOCallback - 4b	\n"
"		.long		__ZN15CLOPackageStore12deleteObjectEP6CStorej - 4b	\n"
"		.long		__ZN15CLOPackageStore9duplicateEPjP6CStorejS2_ - 4b	\n"
"		.long		__ZN15CLOPackageStore6resizeEP6CStorejm - 4b	\n"
"		.long		__ZN15CLOPackageStore11storageSizeEP6CStorej - 4b	\n"
"		.long		__ZN15CLOPackageStore12sizeOfStreamEP6CStorejb - 4b	\n"
"		.long		__ZN15CLOPackageStore6backupEP5CPipeP6CStorejbP11CLOCallback - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CLOPackageStore)

CLOPackageStore *
CLOPackageStore::make(void)
{ return this; }

void
CLOPackageStore::destroy(void)
{ }


NewtonErr
CLOPackageStore::init(void)
{
	return noErr;
}


NewtonErr
CLOPackageStore::create(PSSId * outId, CStore * inStore, CPipe * inPipe, size_t inSize, bool inReadOnly, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CLOCallback * inCallback)
{
	NewtonErr	err;
	CCallbackCompressor * compressor;

	*outId = 0;

	XTRY
	{
		XFAIL(err = inStore->newWithinTransaction(outId, 0))
		compressor = (CCallbackCompressor *)MakeByName("CCallbackCompressor", NULL, inCompanderName);
		XFAILIF(MemError(), err = kNoMemory;)

		if (compressor == NULL
		|| (err = compressor->init(NULL)) == noErr)
		{
			err = AllocatePackage(inPipe, inStore, *outId, inCompanderName, NULL, 0, compressor, inCallback);
		}
		if (compressor != NULL)
			compressor->destroy();
	}
	XENDTRY;

	XDOFAIL(err)
	{
		if (*outId != 0)
			inStore->separatelyAbort(*outId);
	}
	XENDFAIL;

	return err;
}


NewtonErr
CLOPackageStore::createFromCompressed(PSSId * outId, CStore * inStore, CPipe * inPipe, size_t inSize, bool inReadOnly, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CLOCallback * inCallback)
{
	return LODefaultCreateFromCompressed(outId, inStore, inPipe, inSize, inReadOnly, inCompanderName, inCompanderParms, inCompanderParmSize, inCallback);
}

NewtonErr
CLOPackageStore::deleteObject(CStore * inStore, PSSId inId)
{
	return LODefaultDelete(inStore, inId);
}


NewtonErr
CLOPackageStore::duplicate(PSSId * outId, CStore * inStore, PSSId inId, CStore * intoStore)
{
	return LODefaultDuplicate(outId, inStore, inId, intoStore);
}


NewtonErr
CLOPackageStore::resize(CStore * inStore, PSSId inId, size_t inSize)
{
	return kStoreErrWriteProtected;
}


size_t
CLOPackageStore::storageSize(CStore * inStore, PSSId inId)
{
	return LODefaultStorageSize(inStore, inId);
}


size_t
CLOPackageStore::sizeOfStream(CStore * inStore, PSSId inId, bool inCompressed)
{
	return LODefaultStreamSize(inStore, inId, inCompressed);
}


NewtonErr
CLOPackageStore::backup(CPipe * inPipe, CStore * inStore, PSSId inId, bool inCompressed, CLOCallback * inCallback)
{
	if (inCompressed)
		return LODefaultBackup(inPipe, inStore, inId, inCompressed, inCallback);
	return BackupPackage(inPipe, inStore, inId, inCallback);
}
