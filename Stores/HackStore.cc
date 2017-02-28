/*
	File:		HackStore.cc

	Contains:	Large object store that handles CLZStoreCompander and CPixelMapCompander.
					A hack to allow NCX to handle Works documents.

	Written by:	Newton Research Group, 2007.
*/

#include "LargeObjects.h"
#include "LargeObjectStore.h"
#include "Globals.h"
#include "OSErrors.h"
#include "LZCompressor.h"
#include "LZDecompressor.h"
#include "LZStoreCompander.h"
#include "SimpleStoreCompander.h"
#include "ZippyStoreDecompression.h"
#include "RDM.h"
#include "QDTypes.h"

extern CROMDomainManager1K * gROMStoreDomainManager;
extern char *				gLZSharedBuffer;


/*------------------------------------------------------------------------------
	C H a c k S t o r e
	Not a persistent store at all.
	Objects are malloc’d and their pointers inserted into a fixed-size array.
	The index into the array is an object’s id.
------------------------------------------------------------------------------*/
#define kMaxObjectsInStore 32768

class CHackStore : public CStore
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CHackStore)
	CAPABILITIES( "LOBJ" ""
					  "rom " ""
					  "sram" ""
					  "flsh" "" )

	CHackStore *	make(void);
	void			destroy(void);

	NewtonErr	init(void * inStoreData, size_t inStoreSize, ULong inArg3, ArrayIndex inSocketNumber, ULong inFlags, void * inPSSInfo);
	NewtonErr	needsFormat(bool * outNeedsFormat);
	NewtonErr	format(void);

	NewtonErr	getRootId(PSSId * outRootId);
	NewtonErr	newObject(PSSId * outObjectId, size_t inSize);
	NewtonErr	eraseObject(PSSId inObjectId);
	NewtonErr	deleteObject(PSSId inObjectId);
	NewtonErr	setObjectSize(PSSId inObjectId, size_t inSize);
	NewtonErr	getObjectSize(PSSId inObjectId, size_t * outSize);

	NewtonErr	write(PSSId inObjectId, size_t inStartOffset, void * inBuffer, size_t inLength);
	NewtonErr	read(PSSId inObjectId, size_t inStartOffset, void * outBuffer, size_t inLength);

	NewtonErr	getStoreSize(size_t * outTotalSize, size_t * outUsedSize);
	NewtonErr	isReadOnly(bool * outIsReadOnly);
	NewtonErr	lockStore(void);
	NewtonErr	unlockStore(void);

	NewtonErr	abort(void);
	NewtonErr	idle(bool * outArg1, bool * outArg2);

	NewtonErr	nextObject(PSSId inObjectId, PSSId * outNextObjectId);
	NewtonErr	checkIntegrity(ULong * inArg1);

	NewtonErr	setBuddy(CStore * inStore);
	bool			ownsObject(PSSId inObjectId);
	VAddr			address(PSSId inObjectId);
	const char * storeKind(void);
	NewtonErr	setStore(CStore * inStore, ObjectId inEnvironment);

	bool			isSameStore(void * inData, size_t inSize);
	bool			isLocked(void);
	bool			isROM(void);

	NewtonErr	vppOff(void);
	NewtonErr	sleep(void);

	NewtonErr	newWithinTransaction(PSSId * outObjectId, size_t inSize);
	NewtonErr	startTransactionAgainst(PSSId inObjectId);
	NewtonErr	separatelyAbort(PSSId inObjectId);
	NewtonErr	addToCurrentTransaction(PSSId inObjectId);
	bool			inSeparateTransaction(PSSId inObjectId);

	NewtonErr	lockReadOnly(void);
	NewtonErr	unlockReadOnly(bool inReset);
	bool			inTransaction(void);

	NewtonErr	newObject(PSSId * outObjectId, void * inData, size_t inSize);
	NewtonErr	replaceObject(PSSId inObjectId, void * inData, size_t inSize);

	NewtonErr	calcXIPObjectSize(long inArg1, long inArg2, long * outArg3);
	NewtonErr	newXIPObject(PSSId * outObjectId, size_t inSize);
	NewtonErr	getXIPObjectInfo(PSSId inObjectId, unsigned long * outArg2, unsigned long * outArg3, unsigned long * outArg4);


private:
	int		fNextBlock;
	void *	fBlock[kMaxObjectsInStore];
	size_t	fBlockSize[kMaxObjectsInStore];
	size_t	fTotalSize;
	int		fLockCounter;
};


PROTOCOL_IMPL_SOURCE_MACRO(CHackStore)

CHackStore *
CHackStore::make(void)
{
	fNextBlock = 1;
	memset(fBlock, 0, sizeof(fBlock));
	fBlock[0] = (void *)0xFFFFFFFF;	// root
	memset(fBlockSize, 0, sizeof(fBlockSize));
	fTotalSize = 0;
	fLockCounter = 0;
	return this;
}

void
CHackStore::destroy(void)
{ }

NewtonErr
CHackStore::init(void * inStoreData, size_t inStoreSize, ULong inArg3, ArrayIndex inSocketNumber, ULong inFlags, void * inPSSInfo)
{
	return noErr;
}

NewtonErr
CHackStore::needsFormat(bool * outNeedsFormat)
{
	*outNeedsFormat = NO;
	return noErr;
}

NewtonErr
CHackStore::format(void)
{
	return noErr;
}


NewtonErr
CHackStore::getRootId(PSSId * outRootId)
{
	*outRootId = 0;
	return noErr;
}

NewtonErr
CHackStore::newObject(PSSId * outObjectId, size_t inSize)
{
	void *	obj;
	// find free block index
	int		objId = fNextBlock;
	int		firstBlock = fNextBlock;
	while (fBlock[objId] != 0)
	{
		if (++objId == kMaxObjectsInStore)
			objId = 1;
		if (objId == firstBlock)
			return kStoreErrStoreFull;
	}
	// alloc it
	obj = (inSize > 0) ? malloc(inSize) : (void *)0xFFFFFFFF;

	fBlock[objId] = obj;
	fBlockSize[objId] = inSize;
	fTotalSize += inSize;
	*outObjectId = objId;
	if (++objId == kMaxObjectsInStore)
		objId = 1;
	fNextBlock = objId;

	return noErr;
}

NewtonErr
CHackStore::eraseObject(PSSId inObjectId)
{
	return noErr;
}

NewtonErr
CHackStore::deleteObject(PSSId inObjectId)
{
	void *	obj;
	// verify id
	if (inObjectId >= kMaxObjectsInStore
	||  (obj = fBlock[inObjectId]) == NULL)
		return kStoreErrBadPSSId;
	// free it
	free(obj);
	fTotalSize -= fBlockSize[inObjectId];

	fBlock[inObjectId] = NULL;
	fBlockSize[inObjectId] = 0;

	return noErr;
}

void * gHackBlockAddr = 0;

NewtonErr
CHackStore::setObjectSize(PSSId inObjectId, size_t inSize)
{
	void *	obj;
	size_t	objSize;

	if (gHackBlockAddr)
		obj = gHackBlockAddr;
	else
	{
		// verify id
		if (inObjectId >= kMaxObjectsInStore
		||  (obj = fBlock[inObjectId]) == NULL)
			return kStoreErrBadPSSId;
		// realloc it
		if (obj == (void *)0xFFFFFFFF)
			obj = malloc(inSize);
		else
			obj = realloc(obj, inSize);
	}
	objSize = fBlockSize[inObjectId];

	fBlock[inObjectId] = obj;
	fBlockSize[inObjectId] = inSize;
	fTotalSize += (inSize - objSize);

	return noErr;
}

NewtonErr
CHackStore::getObjectSize(PSSId inObjectId, size_t * outSize)
{
	void *	obj;
	// verify id
	if (inObjectId >= kMaxObjectsInStore
	||  (obj = fBlock[inObjectId]) == NULL)
		return kStoreErrBadPSSId;

	*outSize = fBlockSize[inObjectId];

	return noErr;
}


NewtonErr
CHackStore::write(PSSId inObjectId, size_t inStartOffset, void * inBuffer, size_t inLength)
{
	NewtonErr	err = noErr;
	void *	obj;
	size_t	objSize;

	if (inObjectId >= kMaxObjectsInStore
	||  (obj = fBlock[inObjectId]) == NULL)
		return kStoreErrBadPSSId;

	objSize = fBlockSize[inObjectId];
	if (inStartOffset > objSize)
		return kStoreErrObjectOverRun;
	if (inStartOffset + inLength > objSize)
	{
		inLength = objSize - inStartOffset;
		err = kStoreErrObjectOverRun;
	}

	memmove((char *)obj + inStartOffset, inBuffer, inLength);

	return err;
}

NewtonErr
CHackStore::read(PSSId inObjectId, size_t inStartOffset, void * outBuffer, size_t inLength)
{
	NewtonErr	err = noErr;
	void *	obj;
	size_t	objSize;

	if (inObjectId >= kMaxObjectsInStore
	||  (obj = fBlock[inObjectId]) == NULL)
		return kStoreErrBadPSSId;

	objSize = fBlockSize[inObjectId];
	if (inStartOffset > objSize)
		return kStoreErrObjectOverRun;
	if (inStartOffset + inLength > objSize)
	{
		inLength = objSize - inStartOffset;
		err = kStoreErrObjectOverRun;
	}

	memmove(outBuffer, (char *)obj + inStartOffset, inLength);

	return err;
}


NewtonErr
CHackStore::getStoreSize(size_t * outTotalSize, size_t * outUsedSize)
{
	*outTotalSize = kMaxObjectsInStore*kSubPageSize;
	*outUsedSize = fTotalSize;
	return noErr;
}

NewtonErr
CHackStore::isReadOnly(bool * outIsReadOnly)
{
	*outIsReadOnly = NO;
	return noErr;
}

NewtonErr
CHackStore::lockStore(void)
{
	fLockCounter++;
	return noErr;
}

NewtonErr
CHackStore::unlockStore(void)
{
	fLockCounter--;
	return noErr;
}


NewtonErr
CHackStore::abort(void)
{
	fLockCounter = 0;
	return noErr;
}

NewtonErr
CHackStore::idle(bool * outArg1, bool * outArg2)
{
	*outArg1 = NO;
	*outArg2 = NO;
	return noErr;
}


NewtonErr
CHackStore::nextObject(PSSId inObjectId, PSSId * outNextObjectId)
{
	*outNextObjectId = 0;
	return noErr;
}

NewtonErr
CHackStore::checkIntegrity(ULong * inArg1)
{
	return noErr;
}


NewtonErr
CHackStore::setBuddy(CStore * inStore)
{
	return noErr;
}

bool
CHackStore::ownsObject(PSSId inObjectId)
{
	return true;
}

VAddr
CHackStore::address(PSSId inObjectId)
{
	void *	obj;
	if (inObjectId >= kMaxObjectsInStore
	||  (obj = fBlock[inObjectId]) == NULL)
		return (VAddr)NULL;

	return (VAddr)fBlock[inObjectId];
}

const char *
CHackStore::storeKind(void)
{
	return "Internal";
}

NewtonErr
CHackStore::setStore(CStore * inStore, ObjectId inEnvironment)
{
	return noErr;
}


bool
CHackStore::isSameStore(void * inData, size_t inSize)
{
	return true;
}

bool
CHackStore::isLocked(void)
{
	return fLockCounter != 0;
}

bool
CHackStore::isROM(void)
{
	return NO;
}


NewtonErr
CHackStore::vppOff(void)
{
	return noErr;
}

NewtonErr
CHackStore::sleep(void)
{
	return noErr;
}


NewtonErr
CHackStore::newWithinTransaction(PSSId * outObjectId, size_t inSize)
{
	return newObject(outObjectId, inSize);
}

NewtonErr
CHackStore::startTransactionAgainst(PSSId inObjectId)
{
	return noErr;
}

NewtonErr
CHackStore::separatelyAbort(PSSId inObjectId)
{
	return deleteObject(inObjectId);
}

NewtonErr
CHackStore::addToCurrentTransaction(PSSId inObjectId)
{
	return noErr;
}

bool
CHackStore::inSeparateTransaction(PSSId inObjectId)
{
	return NO;
}


NewtonErr
CHackStore::lockReadOnly(void)
{
	return noErr;
}

NewtonErr
CHackStore::unlockReadOnly(bool inReset)
{
	return noErr;
}

bool
CHackStore::inTransaction(void)
{
	return isLocked();
}


NewtonErr
CHackStore::newObject(PSSId * outObjectId, void * inData, size_t inSize)
{
	NewtonErr	err;
	if ((err = newObject(outObjectId, inSize)) == noErr)
		err = write(*outObjectId, 0, inData, inSize);
	return err;
}

NewtonErr
CHackStore::replaceObject(PSSId inObjectId, void * inData, size_t inSize)
{
	NewtonErr	err;
	if ((err = setObjectSize(inObjectId, inSize)) == noErr)
		err = write(inObjectId, 0, inData, inSize);
	return err;
}


NewtonErr
CHackStore::calcXIPObjectSize(long inArg1, long inArg2, long * outArg3)
{
	return kOSErrXIPNotPossible;
}

NewtonErr
CHackStore::newXIPObject(PSSId * outObjectId, size_t inSize)
{
	return kOSErrXIPNotPossible;
}

NewtonErr
CHackStore::getXIPObjectInfo(PSSId inObjectId, unsigned long * outArg2, unsigned long * outArg3, unsigned long * outArg4)
{
	return kOSErrXIPNotPossible;
}


#pragma mark -

/*------------------------------------------------------------------------------
	C L O H a c k S t o r e
	A large object store that contains pixel maps.
------------------------------------------------------------------------------*/

struct PixMapObj
{
	size_t	size;
	PixelMap	pixMap;
	long		x20;
	long		x24;
	long		x28;
};


PROTOCOL CLOHackStore : public CLrgObjStore
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CLOHackStore)

	CLOHackStore *	make(void);
	void			destroy(void);

	NewtonErr	init(void);
	NewtonErr	create(PSSId * outId, CStore * inStore, CPipe * inPipe, size_t inStreamSize, bool inArg5, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CLOCallback * inCallback);
	NewtonErr	createFromCompressed(PSSId * outId, CStore * inStore, CPipe * inPipe, size_t inStreamSize, bool inArg5, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CLOCallback * inCallback);
	NewtonErr	deleteObject(CStore * inStore, PSSId inId);
	NewtonErr	duplicate(PSSId * outId, CStore * inStore, PSSId inId, CStore * intoStore);
	NewtonErr	resize(CStore * inStore, PSSId inId, size_t inSize);
	size_t		storageSize(CStore * inStore, PSSId inId);
	size_t		sizeOfStream(CStore * inStore, PSSId inId, bool inCompressed);
	NewtonErr	backup(CPipe * inPipe, CStore * inStore, PSSId inId, bool inCompressed, CLOCallback * inCallback);

private:
	PixMapObj	fPixMapObj;
	long			f38;
	long			f3C;
	long			f40;
	int			fRowLongs;
};


PROTOCOL_IMPL_SOURCE_MACRO(CLOHackStore)

CLOHackStore *
CLOHackStore::make(void)
{ return this; }

void
CLOHackStore::destroy(void)
{ }


NewtonErr
CLOHackStore::init(void)
{
	fPixMapObj.size = 0;
	return noErr;
}


NewtonErr
CLOHackStore::create(PSSId * outId, CStore * inStore, CPipe * inPipe, size_t inStreamSize, bool inArg5, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CLOCallback * inCallback)
{
	return kStoreErrWriteProtected;
}


NewtonErr
CLOHackStore::createFromCompressed(PSSId * outId, CStore * inStore, CPipe * inPipe, size_t inStreamSize, bool inArg5, const char * inCompanderName, void * inCompanderParms, size_t inCompanderParmSize, CLOCallback * inCallback)
{
	NewtonErr	err = noErr;

	if (strcmp(inCompanderName, "CLZStoreCompander") == 0
	&&  inStreamSize != 0)
	{
		RefVar			largeBinary;
		char *			chunk = (char *)malloc(0x0520);

		ULong				flags;
		size_t			decompressedSize;
		size_t			actualSize;
		size_t			sizeRemaining;
		size_t			chunkIndex, numOfChunks;
		char *			decompressedData;
		char *			destination;
		CLZDecompressor decompressor;

		newton_try
		{
			size_t	itemSize;
			bool		isEOF;

			*inPipe >> flags;
			*inPipe >> decompressedSize;

			decompressor.init(NULL);

			sizeRemaining = decompressedSize;
			numOfChunks = (decompressedSize + kSubPageSize-1)/kSubPageSize;
			decompressedData = (char *)malloc(numOfChunks*kSubPageSize);
			destination = decompressedData;
			for (chunkIndex = 0; chunkIndex < numOfChunks; chunkIndex++)
			{
				size_t chunkSize;
				itemSize = sizeof(chunkSize);
				inPipe->readChunk(&chunkSize, itemSize, isEOF);
				inPipe->readChunk(chunk, chunkSize, isEOF);

				decompressor.decompress(&actualSize, destination, sizeRemaining, chunk, chunkSize);

				destination += actualSize;
				sizeRemaining -= actualSize;
			}

		}
		newton_catch(exPipe)
		{
			err = (NewtonErr)(long)CurrentException()->data;
		}
		end_try;

		free(chunk);
		*outId = (PSSId) decompressedData;
		*(outId+1) = decompressedSize;
	}

	else if (strcmp(inCompanderName, "TPixelMapCompander") == 0
		  &&  inStreamSize != 0)
	{
		RefVar			largeBinary;
		char *			chunk = (char *)malloc(0x0408);

		ULong				flags;
		size_t			decompressedSize;
		size_t			actualSize;
		size_t			sizeRemaining;
		size_t			chunkIndex, numOfChunks;
		char *			decompressedData;
		char *			destination;
		CLZDecompressor decompressor;

		newton_try
		{
			size_t	itemSize;
			bool		isEOF;

			if (inCompanderParmSize <= 44)
			{
				memmove(&fPixMapObj, inCompanderParms, inCompanderParmSize);
				fRowLongs = fPixMapObj.pixMap.rowBytes / sizeof(int32_t);
				f38 = -1;
			}

			*inPipe >> flags;
			*inPipe >> decompressedSize;

			decompressor.init(NULL);

			sizeRemaining = decompressedSize;
			numOfChunks = (decompressedSize + kSubPageSize-1)/kSubPageSize;
			decompressedData = (char *)malloc(numOfChunks*kSubPageSize);
			destination = decompressedData;
			for (chunkIndex = 0; chunkIndex < numOfChunks; chunkIndex++)
			{
				size_t chunkSize;
				itemSize = sizeof(chunkSize);
				inPipe->readChunk(&chunkSize, itemSize, isEOF);
				inPipe->readChunk(chunk, chunkSize, isEOF);

				decompressor.decompress(&actualSize, destination, sizeRemaining, chunk, chunkSize);

				if (fPixMapObj.size != 0)
				{
					int rowBytes = fPixMapObj.pixMap.rowBytes;
					fRowLongs = rowBytes / sizeof(long);
					if (f38 != kSubPageSize)
					{
						int wholeRowBytes = (kSubPageSize / rowBytes) * rowBytes;
						f3C = wholeRowBytes / sizeof(long) - fRowLongs;
						f40 = kSubPageSize - wholeRowBytes;
						f38 = kSubPageSize;
					}
					unsigned long * r6 = (unsigned long *)destination;
					unsigned long * r1 = (unsigned long *)destination + fRowLongs;
					int i;
					for (i = f3C; i >= 8; i -= 8)
					{
						*r1++ ^= *r6++;
						*r1++ ^= *r6++;
						*r1++ ^= *r6++;
						*r1++ ^= *r6++;
						*r1++ ^= *r6++;
						*r1++ ^= *r6++;
						*r1++ ^= *r6++;
						*r1++ ^= *r6++;
					}
					switch (i)
					{
					case 7:
						*r1++ ^= *r6++;
					case 6:
						*r1++ ^= *r6++;
					case 5:
						*r1++ ^= *r6++;
					case 4:
						*r1++ ^= *r6++;
					case 3:
						*r1++ ^= *r6++;
					case 2:
						*r1++ ^= *r6++;
					case 1:
						*r1 ^= *r6;
					}
				}

				destination += actualSize;
				sizeRemaining -= actualSize;
			}
		}
		newton_catch(exPipe)
		{
			err = (NewtonErr)(long)CurrentException()->data;
		}
		end_try;

		free(chunk);
		*outId = (PSSId) decompressedData;
		*(outId+1) = decompressedSize;
	}

	else
	{
		inPipe->readSeek(inStreamSize, SEEK_CUR);	// skip over compressed data
		*outId = 0;
	}

	return err;
}


NewtonErr
CLOHackStore::deleteObject(CStore * inStore, PSSId inId)
{
	return noErr;
}


NewtonErr
CLOHackStore::duplicate(PSSId * outId, CStore * inStore, PSSId inId, CStore * intoStore)
{
	return kStoreErrWriteProtected;
}


NewtonErr
CLOHackStore::resize(CStore * inStore, PSSId inId, size_t inSize)
{
	return kStoreErrWriteProtected;
}


size_t
CLOHackStore::storageSize(CStore * inStore, PSSId inId)
{
	return 0;
}


size_t
CLOHackStore::sizeOfStream(CStore * inStore, PSSId inId, bool inCompressed)
{
	return 0;
}


NewtonErr
CLOHackStore::backup(CPipe * inPipe, CStore * inStore, PSSId inId, bool inCompressed, CLOCallback * inCallback)
{
	return noErr;
}

#pragma mark -

extern Ref		RegisterStore(CStore * inStore);
extern Ref		GetStores(void);


void
RegisterHackStore(void)
{
	CHackStore::classInfo()->registerProtocol();
	CLOHackStore::classInfo()->registerProtocol();

	CHackStore * theStore = (CHackStore *)MakeByName("CStore", "CHackStore");
	RegisterStore(theStore);
}


void
InstantiateLargeObject(VAddr * ioAddr)
{
	gROMStoreDomainManager->instantiateLargeObject(ioAddr);
}


NewtonErr
Compress(VAddr inAddr)
{
	return gROMStoreDomainManager->flushLargeObject(inAddr);
}


NewtonErr
Decompress(VAddr * outAddr, /*size_t * outSize,*/ CStore * inStore, PSSId inId)
{
	NewtonErr			err;
	LargeObjectRoot	root;
	size_t				objSize;
	char *				companderName = NULL;
	char *				companderParms = NULL;
	int					delta;

//	*outSize = 0;
	*outAddr = 0;

	XTRY
	{
		XFAIL(err = inStore->read(inId, 0, &root, sizeof(LargeObjectRoot)))

		XFAIL(err = inStore->getObjectSize(root.fCompanderNameId, &objSize))
		companderName = new char[objSize+1];
		XFAIL(err = inStore->read(root.fCompanderNameId, 0, companderName, objSize))
		companderName[objSize] = 0;
	}
	XENDTRY;
REPprintf("Decompress: %s\n", companderName);
	if (strcmp(companderName, "CLZStoreCompander") == 0)
	{
		PSSId				chunkId;
		size_t			chunkIndex, numOfChunks;
		size_t			chunkSize;
		size_t			actualSize;
		size_t			sizeRemaining;
		char *			decompressedData;
		char *			destination;
		CLZDecompressor decompressor;

		newton_try
		{
			decompressor.init(NULL);

			XTRY
			{
				XFAIL(err = inStore->getObjectSize(root.fDataId, &numOfChunks))
				// convert size of array to number of entries
				numOfChunks /= sizeof(PSSId);
				sizeRemaining = numOfChunks*kSubPageSize;
//				*outSize = sizeRemaining;
				decompressedData = new char[sizeRemaining];
				destination = decompressedData;

				for (chunkIndex = 0; chunkIndex < numOfChunks; chunkIndex++)
				{
					XFAIL(err = inStore->read(root.fDataId, chunkIndex*sizeof(PSSId), &chunkId, sizeof(chunkId)))
					XFAIL(err = inStore->getObjectSize(chunkId, &chunkSize))
					XFAIL(err = inStore->read(chunkId, 0, gLZSharedBuffer, chunkSize))

					decompressor.decompress(&actualSize, destination, sizeRemaining, gLZSharedBuffer, chunkSize);

					destination += actualSize;
					sizeRemaining -= actualSize;
				}

			}
			XENDTRY;
		}
		newton_catch(exPipe)
		{
			err = (NewtonErr)(long)CurrentException()->data;
		}
		end_try;

		*outAddr = (VAddr) decompressedData;
	}

	else if (strcmp(companderName, "CLZStoreDecompressor") == 0)
	{
		PSSId				chunkId;
		size_t			chunkIndex, numOfChunks;
		size_t			sizeOfData;
		unsigned long	offset;
		char *			decompressedData;
		CLZStoreDecompressor decompressor;

		newton_try
		{
			XTRY
			{
				decompressor.init(inStore, (PSSId)gLZSharedBuffer);

				XFAIL(err = inStore->getObjectSize(root.fDataId, &numOfChunks))
				// convert size of array to number of entries
				numOfChunks /= sizeof(PSSId);
				sizeOfData = numOfChunks*kSubPageSize;
				decompressedData = new char[sizeOfData];

				for (offset = 0, chunkIndex = 0; chunkIndex < numOfChunks; chunkIndex++, offset += kSubPageSize)
				{
					XFAIL(err = inStore->read(root.fDataId, chunkIndex*sizeof(PSSId), &chunkId, sizeof(chunkId)))
					decompressor.read(chunkId, decompressedData+offset, kSubPageSize, 0);
				}
			}
			XENDTRY;
		}
		newton_catch(exPipe)
		{
			err = (NewtonErr)(long)CurrentException()->data;
		}
		end_try;

		*outAddr = (VAddr) decompressedData;
	}

	else if (strcmp(companderName, "CSimpleStoreCompander") == 0)
	{
		size_t			numOfChunks;
		size_t			sizeOfData;
		char *			decompressedData;
		CSimpleStoreCompander decompressor;

		newton_try
		{
			XTRY
			{
				decompressor.init(inStore, inId, 0, 0, NO);

				XFAIL(err = inStore->getObjectSize(root.fDataId, &numOfChunks))
				// convert size of array to number of entries
				numOfChunks /= sizeof(PSSId);
				sizeOfData = numOfChunks*kSubPageSize;
				decompressedData = new char[sizeOfData];

				for (unsigned long offset = 0; offset < sizeOfData; offset += kSubPageSize)
				{
					decompressor.read(offset, decompressedData+offset, kSubPageSize, 0);
				}
			}
			XENDTRY;
		}
		newton_catch(exPipe)
		{
			err = (NewtonErr)(long)CurrentException()->data;
		}
		end_try;

		*outAddr = (VAddr) decompressedData;
	}

	else if ((delta = strlen(companderName) - 17) > 0
		  &&  strcmp(companderName + delta, "StoreDecompressor") == 0)
	{
	//	we’re expecting
	//		CSimpleStoreDecompressor
	//		CSimpleRelocStoreDecompressor
	//		CZippyStoreDecompressor
	//		CZippyRelocStoreDecompressor
		PSSId				chunkId;
		size_t			chunkIndex, numOfChunks;
		size_t			sizeOfData;
		unsigned long	offset;
		char *			decompressedData;
		CStoreDecompressor * decompressor = (CStoreDecompressor *)MakeByName("CStoreDecompressor", companderName);

		newton_try
		{
			XTRY
			{
				decompressor->init(inStore, 0);

				XFAIL(err = inStore->getObjectSize(root.fDataId, &numOfChunks))
				// convert size of array to number of entries
				numOfChunks /= sizeof(PSSId);
				sizeOfData = numOfChunks*kSubPageSize;
				decompressedData = new char[sizeOfData];

				for (offset = 0, chunkIndex = 0; chunkIndex < numOfChunks; chunkIndex++, offset += kSubPageSize)
				{
					XFAIL(err = inStore->read(root.fDataId, chunkIndex*sizeof(PSSId), &chunkId, sizeof(chunkId)))
					decompressor->read(chunkId, decompressedData+offset, kSubPageSize, 0);
				}
			}
			XENDTRY;
		}
		newton_catch(exPipe)
		{
			err = (NewtonErr)(long)CurrentException()->data;
		}
		end_try;
		decompressor->destroy();

		*outAddr = (VAddr) decompressedData;
	}

	else if (strcmp(companderName, "CPixelMapCompander") == 0)
	{
		char *			chunk;
		PSSId				chunkId;
		size_t			chunkIndex, numOfChunks;
		size_t			chunkSize;
		size_t			actualSize;
		size_t			sizeRemaining;
		char *			decompressedData;
		char *			destination;
		CLZDecompressor decompressor;

		size_t		companderParmSize;
		PixMapObj	fPixMapObj;
		long			f38;
		long			f3C;
		long			f40;
		int			fRowLongs;

		XTRY
		{
			XFAIL(err = inStore->getObjectSize(root.fCompanderParmsId, &companderParmSize))
			companderParms = new char[companderParmSize];
			XFAIL(err = inStore->read(root.fCompanderParmsId, 0, companderParms, companderParmSize))
		}
		XENDTRY;

		newton_try
		{
			chunk = new char[0x0408];

			fPixMapObj.size = 0;
			if (companderParmSize <= 44)
			{
				memmove(&fPixMapObj, companderParms, companderParmSize);
#if defined(hasByteSwapping)
				fPixMapObj.size = BYTE_SWAP_LONG(fPixMapObj.size);
				fPixMapObj.pixMap.baseAddr = (Ptr) BYTE_SWAP_LONG((ULong)fPixMapObj.pixMap.baseAddr);
				fPixMapObj.pixMap.rowBytes = BYTE_SWAP_SHORT(fPixMapObj.pixMap.rowBytes);
				fPixMapObj.pixMap.bounds.top = BYTE_SWAP_SHORT(fPixMapObj.pixMap.bounds.top);
				fPixMapObj.pixMap.bounds.left = BYTE_SWAP_SHORT(fPixMapObj.pixMap.bounds.left);
				fPixMapObj.pixMap.bounds.bottom = BYTE_SWAP_SHORT(fPixMapObj.pixMap.bounds.bottom);
				fPixMapObj.pixMap.bounds.right = BYTE_SWAP_SHORT(fPixMapObj.pixMap.bounds.right);
				fPixMapObj.pixMap.pixMapFlags = BYTE_SWAP_LONG(fPixMapObj.pixMap.pixMapFlags);
				fPixMapObj.pixMap.deviceRes.h = BYTE_SWAP_SHORT(fPixMapObj.pixMap.deviceRes.h);
				fPixMapObj.pixMap.deviceRes.v = BYTE_SWAP_SHORT(fPixMapObj.pixMap.deviceRes.v);
#if defined(QD_Gray)
				fPixMapObj.pixMap.grayTable = (UChar *)BYTE_SWAP_LONG((ULong)fPixMapObj.pixMap.grayTable);
#endif
				fPixMapObj.x20 = BYTE_SWAP_LONG(fPixMapObj.x20);
				fPixMapObj.x24 = BYTE_SWAP_LONG(fPixMapObj.x24);
				fPixMapObj.x28 = BYTE_SWAP_LONG(fPixMapObj.x28);
#endif
				fRowLongs = fPixMapObj.pixMap.rowBytes / sizeof(long);
				f38 = -1;
			}

			decompressor.init(NULL);

			XTRY
			{
				XFAIL(err = inStore->getObjectSize(root.fDataId, &numOfChunks))
				// convert size of array to number of entries
				numOfChunks /= sizeof(PSSId);
				sizeRemaining = numOfChunks*kSubPageSize;
//				*outSize = sizeRemaining;
				decompressedData = new char[sizeRemaining];
				destination = decompressedData;

				for (chunkIndex = 0; chunkIndex < numOfChunks; chunkIndex++)
				{
					XFAIL(err = inStore->read(root.fDataId, chunkIndex*sizeof(PSSId), &chunkId, sizeof(chunkId)))
					XFAIL(err = inStore->getObjectSize(chunkId, &chunkSize))
					XFAIL(err = inStore->read(chunkId, 0, chunk, chunkSize))

					decompressor.decompress(&actualSize, destination, sizeRemaining, chunk, chunkSize);

					if (fPixMapObj.size != 0)
					{
						int rowBytes = fPixMapObj.pixMap.rowBytes;
						fRowLongs = rowBytes / sizeof(long);
						if (f38 != kSubPageSize)
						{
							int wholeRowBytes = (kSubPageSize / rowBytes) * rowBytes;
							f3C = wholeRowBytes / sizeof(long) - fRowLongs;
							f40 = kSubPageSize - wholeRowBytes;
							f38 = kSubPageSize;
						}
						unsigned long * r6 = (unsigned long *)destination;
						unsigned long * r1 = (unsigned long *)destination + fRowLongs;
						int i;
						for (i = f3C; i >= 8; i -= 8)
						{
							*r1++ ^= *r6++;
							*r1++ ^= *r6++;
							*r1++ ^= *r6++;
							*r1++ ^= *r6++;
							*r1++ ^= *r6++;
							*r1++ ^= *r6++;
							*r1++ ^= *r6++;
							*r1++ ^= *r6++;
						}
						switch (i)
						{
						case 7:
							*r1++ ^= *r6++;
						case 6:
							*r1++ ^= *r6++;
						case 5:
							*r1++ ^= *r6++;
						case 4:
							*r1++ ^= *r6++;
						case 3:
							*r1++ ^= *r6++;
						case 2:
							*r1++ ^= *r6++;
						case 1:
							*r1 ^= *r6;
						}
					}

					destination += actualSize;
					sizeRemaining -= actualSize;
				}
			}
			XENDTRY;
		}
		newton_catch(exPipe)
		{
			err = (NewtonErr)(long)CurrentException()->data;
		}
		end_try;

		delete chunk;
		*outAddr = (VAddr) decompressedData;
	}

	else
	{
REPprintf("unimplemented store compressor %s\n", companderName);
		*outAddr = 0;
	}

	delete[] companderName;
	return err;
}
