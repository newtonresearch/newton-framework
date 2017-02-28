/*
	File:		Indexes.cc

	Contains:	Soup index implementation.

	Written by:	Newton Research Group, 2007.
*/

#include "Objects.h"
#include "ROMResources.h"
#include "RefMemory.h"
#include "Unicode.h"
#include "NewtonTime.h"
#include "CObjectBinaries.h"
#include "FaultBlocks.h"
#include "Funcs.h"
#include "RichStrings.h"
#include "UStringUtils.h"
#include "StoreWrapper.h"
#include "PlainSoup.h"
#include "Cursors.h"
#include "OSErrors.h"

extern "C" {
Ref	FSetContains(RefArg inRcvr, RefArg inArray, RefArg inMember);
}

// may want to take sort table stuff out of Unicode.cc into a new SortTable.cc/.h
extern const CSortingTable * GetIndexSortTable(RefArg indexDesc);


// need to init CSoupIndex array at an address allocated in the frames heap
//inline void * operator new[](size_t, CSoupIndex * addr) { return addr; }
inline void * operator new(size_t, CSoupIndex * addr) { return addr; }


/*------------------------------------------------------------------------------
	C o n s t a n t s
------------------------------------------------------------------------------*/

#define kNodeBlockSize 512


#pragma mark -

/*------------------------------------------------------------------------------
	C A b s t r a c t S o u p I n d e x
------------------------------------------------------------------------------*/

CAbstractSoupIndex::CAbstractSoupIndex()
{ }

CAbstractSoupIndex::~CAbstractSoupIndex()
{ }

int
CAbstractSoupIndex::findPrior(SKey * arg1, SKey * arg2, SKey * arg3, bool inArg4, bool inArg5)
{
	// this: r6  args: r1 r5 r4 r3 r7
	int secondaryResult;
	int status = find(arg1, arg2, arg3, inArg4);
	if (status == 0)
	{
		if (inArg5)
			status = prior(arg2, arg3, 0, arg2, arg3);
		else
		{
			if (next(arg2, arg3, 1, arg2, arg3) == 3)
				last(arg2, arg3);
			else
				prior(arg2, arg3, 0, arg2, arg3);
		}
	}
	else if (status == 2)
	{
		if ((secondaryResult = prior(arg2, arg3, 0, arg2, arg3)) != 0)
			status = secondaryResult;
	}
	else if (status == 3)
	{
		secondaryResult = last(arg2, arg3);
		if (secondaryResult == 0)
			status = 2;
		else if (secondaryResult != 2)
			status = secondaryResult;
	}
	return status;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S o u p I n d e x
------------------------------------------------------------------------------*/

#define ALIGNkfSIZE(n) if (ODD(n)) n++

#define kSizeOfLengthWords (3*sizeof(short))

KeyField		keyFieldBuffer;
KeyField		savedKey;
KeyField		leafKey;

KeyCompareProcPtr		CSoupIndex::fKeyCompareFns[7] = {
	&CSoupIndex::stringKeyCompare,
	&CSoupIndex::longKeyCompare,
	&CSoupIndex::characterKeyCompare,
	&CSoupIndex::doubleKeyCompare,
	&CSoupIndex::asciiKeyCompare,
	&CSoupIndex::rawKeyCompare,
	&CSoupIndex::multiKeyCompare };
short			CSoupIndex::fKeySizes[7] = { 0, 4, 2, 8, 0, 0, 0 };	// we know the sizes of int, char, real
KeyField *	CSoupIndex::fKeyField = &keyFieldBuffer;
KeyField *	CSoupIndex::fSavedKey = &savedKey;
KeyField *	CSoupIndex::fLeafKey = &leafKey;


/*------------------------------------------------------------------------------
	Create new index info on store.
	Args:		inStoreWrapper
				info
	Return:	its id
------------------------------------------------------------------------------*/

PSSId
CSoupIndex::create(CStoreWrapper * inStoreWrapper, IndexInfo * info)
{
	PSSId theId;
	info->rootNodeId = 0;
	info->nodeSize = kNodeBlockSize;
	info->x1A = 0;
	OSERRIF(inStoreWrapper->store()->newObject(&theId, info, sizeof(IndexInfo)));
	return theId;
}


/*------------------------------------------------------------------------------
	Constructor/destructor -- do nothing.
------------------------------------------------------------------------------*/

CSoupIndex::CSoupIndex()
{ }


CSoupIndex::~CSoupIndex()
{ }


/*------------------------------------------------------------------------------
	Initialize -- read index info from store.
	Args:		inStoreWrapper
				inId					id of index info
				inSortingTable
	Return:	--
------------------------------------------------------------------------------*/

void
CSoupIndex::init(CStoreWrapper * inStoreWrapper, PSSId inId, const CSortingTable * inSortingTable)
{
	fStoreWrapper = inStoreWrapper;
	fInfoId = inId;
	fSortingTable = inSortingTable;
	fCache = inStoreWrapper->nodeCache();
	OSERRIF(readInfo());
	f_BTStatus = 0;
	fCompareKeyFn = fKeyCompareFns[fInfo.keyType];
	fCompareDataFn = fKeyCompareFns[fInfo.dataType];
	fFixedKeySize = fKeySizes[fInfo.keyType];
	fFixedDataSize = fKeySizes[fInfo.dataType];
}


/*------------------------------------------------------------------------------
	Read the index info from store.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CSoupIndex::readInfo(void)
{
	NewtonErr err;
	XTRY
	{
		size_t theSize;
		XFAIL(err = store()->getObjectSize(fInfoId, &theSize))
		if (theSize > sizeof(IndexInfo))
			theSize = sizeof(IndexInfo);
		else
			fInfo.x19 = 0;	// sic -- but just gonna read right over the top of it..?
		err = store()->read(fInfoId, 0, &fInfo, theSize);
	}
	XENDTRY;
	return err;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Create a new node and set it as the root.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CSoupIndex::createFirstRoot(void)
{
	NodeHeader * rootNode = newNode();
	setRootNode(rootNode->id);
}


/*------------------------------------------------------------------------------
	Set the id of the root node in our index info.
	Args:		inId				its id on store
	Return:	--
------------------------------------------------------------------------------*/

void
CSoupIndex::setRootNode(PSSId inId)
{
	fInfo.rootNodeId = inId;

	// make it persistent
	size_t theSize;
	OSERRIF(store()->getObjectSize(fInfoId, &theSize));
	if (theSize > sizeof(IndexInfo))
		theSize = sizeof(IndexInfo);
	OSERRIF(store()->write(fInfoId, 0, &fInfo, theSize));
}


/*------------------------------------------------------------------------------
	Create a new empty node block on store.
	Args:		--
	Return:	pointer to node header
------------------------------------------------------------------------------*/

NodeHeader *
CSoupIndex::newNode(void)
{
	PSSId nodeId;
	OSERRIF(store()->newObject(&nodeId, 0));
	NodeHeader * theNode = fCache->rememberNode(this, nodeId, fInfo.nodeSize, false, true);
	initNode(theNode, nodeId);
	return theNode;
}


/*------------------------------------------------------------------------------
	Initialize the fields in a node.
	Args:		ioNode			the node to init
				inId				its id on store
	Return:	--
------------------------------------------------------------------------------*/

void
CSoupIndex::initNode(NodeHeader * ioNode, PSSId inId)
{
	ioNode->id = inId;
	ioNode->nextId = 0;
	ioNode->bytesRemaining = fInfo.nodeSize - 20;
	ioNode->numOfSlots = 0;
	ioNode->keyFieldOffset[0] = fInfo.nodeSize - 2;

	// set up an empty key field
	KeyField * kf = firstKeyField(ioNode);
	kf->type = KeyField::kData;
	kf->length = 0;

	setNodeNo(ioNode, 0, 0);
}


/*------------------------------------------------------------------------------
	Create a new empty dup node block on store.
	Args:		--
	Return:	pointer to dup node header
------------------------------------------------------------------------------*/

DupNodeHeader *
CSoupIndex::newDupNode(void)
{
	PSSId nodeId;
	OSERRIF(store()->newObject(&nodeId, 0));
	DupNodeHeader * theNode = (DupNodeHeader *)fCache->rememberNode(this, nodeId, fInfo.nodeSize, true, true);
	theNode->id = nodeId;
	theNode->nextId = 0;
	theNode->bytesRemaining = fInfo.nodeSize - 16;
	theNode->numOfSlots = 0;
	theNode->bytesUsed = 16;
	theNode->x0E = 0;
	return theNode;
}


/*------------------------------------------------------------------------------
	Set the node number (actually id on store) for a slot; ie link to the next
	level of the B-tree.
	Args:		inNode
				inSlot
				inNodeNum
	Return:	--
------------------------------------------------------------------------------*/

void
CSoupIndex::setNodeNo(NodeHeader * inNode, int inSlot, ULong inNodeNum)
{
	KeyField * kf = keyFieldAddr(inNode, inSlot);
	ULong * nodeNumPtr = (ULong *)kf - 1;
	*nodeNumPtr = inNodeNum;	// needs to handle misaligned pointer!
}


/*------------------------------------------------------------------------------
	Set the root node, optionally creating it if it doesn’t exist.
	Args:		inCreate		true => create a root node if one doesn’t exist
	Return:	pointer to root node
				NULL => we don’t have a root node
------------------------------------------------------------------------------*/

NodeHeader *
CSoupIndex::readRootNode(bool inCreate)
{
	if (fInfo.rootNodeId == 0)
	{
		if (!inCreate)
			return NULL;
		createFirstRoot();
	}
	return readANode(fInfo.rootNodeId, 0);
}


/*------------------------------------------------------------------------------
	Read a node from store.
	NOTE	data on store is contiguous with the header so we need to move it up.
	Args:		inId
				inNextId
	Return:	pointer to the node
------------------------------------------------------------------------------*/

NodeHeader *
CSoupIndex::readANode(PSSId inId, PSSId inNextId)
{
	// look first in the cache
	NodeHeader * theNode = fCache->findNode(this, inId);
	if (theNode == NULL)
	{
		// it’s not in the cache; read it from store
		size_t itsSize;
		OSERRIF(store()->getObjectSize(inId, &itsSize));
		theNode = fCache->rememberNode(this, inId, fInfo.nodeSize, false, false);
		OSERRIF(store()->read(inId, 0, theNode, itsSize));
		// move data to end of node buf
		char * data = (char *)&theNode->keyFieldOffset[theNode->numOfSlots + 1];
		size_t headerSize = data - (char *)theNode;
		memmove(data + theNode->bytesRemaining, data, bytesInNode(theNode) - headerSize);
		// zero the unused buf
		memset(data, 0, theNode->bytesRemaining);
	}
	// set its ids
	theNode->id = inId;
	theNode->nextId = inNextId;
	return theNode;
}


/*------------------------------------------------------------------------------
	Read a dup node from store.
	Dup nodes expand from one end only, so no need for packing/unpacking.
	Args:		inId
	Return:	pointer to the dup node
------------------------------------------------------------------------------*/

DupNodeHeader *
CSoupIndex::readADupNode(PSSId inId)
{
	// look first in the cache
	NodeHeader * theNode = fCache->findNode(this, inId);
	if (theNode == NULL)
	{
		// it’s not in the cache; read it from store
		size_t itsSize;
		OSERRIF(store()->getObjectSize(inId, &itsSize));
		theNode = fCache->rememberNode(this, inId, fInfo.nodeSize, true, false);
		OSERRIF(store()->read(inId, 0, theNode, itsSize));
	}
	// set its id
	theNode->id = inId;
	return (DupNodeHeader *)theNode;
}


/*------------------------------------------------------------------------------
	Delete a node from store.
	Args:		inId
	Return:	--
------------------------------------------------------------------------------*/

void
CSoupIndex::deleteNode(PSSId inId)
{
	fCache->deleteNode(inId);
}


/*------------------------------------------------------------------------------
	Change a node.
	Doesn’t appear to do anything.
	Args:		inNode
	Return:	--
------------------------------------------------------------------------------*/

void
CSoupIndex::changeNode(NodeHeader * inNode)
{
	// this really does nothing
}


/*------------------------------------------------------------------------------
	Update a node on store.
	Close up the gap between header/offsets and data so that we can store it
	more efficiently.
	Args:		inNode
	Return:	--
------------------------------------------------------------------------------*/

void
CSoupIndex::updateNode(NodeHeader * inNode)
{
	// use a new node buffer
	char nodeBuf[kNodeBlockSize];
	size_t nodeSize = bytesInNode(inNode);
	size_t headerSize = sizeof(NodeHeader) + inNode->numOfSlots * sizeof(UShort);
	// copy the header into it
	memmove(nodeBuf, inNode, headerSize);
	// copy the data immediately after the header
	memmove(nodeBuf+headerSize, (char *)inNode+headerSize+inNode->bytesRemaining, nodeSize-headerSize);
	OSERRIF(store()->replaceObject(inNode->id, nodeBuf, nodeSize));
}


/*------------------------------------------------------------------------------
	Update a dup node on store.
	Dup nodes expand from one end only, so no need for packing/unpacking.
	Args:		inNode
	Return:	--
------------------------------------------------------------------------------*/

void
CSoupIndex::updateDupNode(NodeHeader * inNode)
{
	OSERRIF(store()->replaceObject(inNode->id, inNode, bytesInNode(inNode)));
}


/*------------------------------------------------------------------------------
	Free all the nodes referenced from a node.
	Args:		inNode
	Return:	--
------------------------------------------------------------------------------*/

void
CSoupIndex::freeNodes(NodeHeader * inNode)
{
	for (int slot = 0; slot < inNode->numOfSlots; slot++)
	{
		PSSId lNode;
		KeyField * kf = keyFieldAddr(inNode, slot);
		if (kf->type == KeyField::kDupData && kf->length != 0)
			freeDupNodes(kf);
		if ((lNode = leftNodeNo(inNode, slot)) != 0)
			freeNodes(readANode(lNode, inNode->id));
	}
	deleteNode(inNode->id);
}


/*------------------------------------------------------------------------------
	Free all the dup nodes referenced from a key field.
	Args:		inField
	Return:	--
------------------------------------------------------------------------------*/

void
CSoupIndex::freeDupNodes(KeyField * inField)
{
	PSSId dupId = kfNextDupId(inField);
	while (dupId != 0)
	{
		DupNodeHeader * dupNode = readADupNode(dupId);
		deleteNode(dupNode->id);
		dupId = dupNode->nextId;
	}
}

#pragma mark -

/*------------------------------------------------------------------------------
	Catastrophe! Abort the cache and re-read index info.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CSoupIndex::storeAborted(void)
{
	fCache->abort(this);
	readInfo();
}


/*------------------------------------------------------------------------------
	Destroy everything.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CSoupIndex::destroy(void)
{
	if (fInfo.rootNodeId != 0)
	{
		newton_try
		{
			NodeHeader * theNode;
			if ((theNode = readRootNode(false)) != NULL)
				freeNodes(theNode);
			setRootNode(0);
		}
		cleanup
		{
			fCache->abort(this);
		}
		end_try;
	}
	fCache->commit(this);
}

#pragma mark KeyField

void
CSoupIndex::kfAssembleKeyField(KeyField * outField, void * inKey, void * inData)
{
	size_t keyLen, dataLen;

	keyLen = kfSizeOfKey(inKey);
	ALIGNkfSIZE(keyLen);

	if (inData)
	{
		dataLen = kfSizeOfData(inData);
		ALIGNkfSIZE(dataLen);
	}
	else
		dataLen = 0;

	if ((SKey::kOffsetToData + keyLen + SKey::kOffsetToData + dataLen) > kKeyFieldBufSize)
		ThrowErr(exStore, kNSErrInternalError);

	outField->type = KeyField::kData;
	outField->length = KeyField::kOffsetToData + keyLen + dataLen;	// length includes (short) type/length header
	memmove(outField->buf, inKey, keyLen);
	if (dataLen)
		memmove(outField->buf + keyLen, inData, dataLen);
}


void
CSoupIndex::kfDisassembleKeyField(KeyField * inField, SKey * outKey, SKey * outData)
{
	if (outKey)
		memmove(outKey, inField->buf, kfSizeOfKey(inField->buf));
	if (outData)
	{
		void * data = kfFirstDataAddr(inField);
		memmove(outData, data, kfSizeOfData(data));
	}
}


size_t
CSoupIndex::kfSizeOfKey(void * inBuf)
{
	if (fFixedKeySize != 0)
		return fFixedKeySize;
	return SKey::kOffsetToData + ((SKey *)inBuf)->size();
}


size_t
CSoupIndex::kfSizeOfData(void * inBuf)
{
	if (fFixedDataSize != 0)
		return fFixedDataSize;
	return SKey::kOffsetToData + ((SKey *)inBuf)->size();
}


void *
CSoupIndex::kfFirstDataAddr(KeyField * inField)
{
	size_t keyLen = kfSizeOfKey(inField->buf);
	ALIGNkfSIZE(keyLen);
	return inField->buf + keyLen;
}


bool
CSoupIndex::kfNextDataAddr(KeyField * inField, void * inData, void ** outNextData)
{
	if (inData == NULL)
	{
		// find the first
		size_t keyLen = kfSizeOfKey(inField->buf);
		ALIGNkfSIZE(keyLen);
		*outNextData = inField->buf + keyLen;
		return true;
	}
	char * data, * dataLimit = (char *)inField + inField->length - kSizeOfLengthWords;
	size_t dataLen = kfSizeOfData(inData);
	ALIGNkfSIZE(dataLen);
	data = (char *)inData + dataLen;
	*outNextData = data;
	return data < dataLimit;
}


void *
CSoupIndex::kfLastDataAddr(KeyField * inField)
{
	char * lastData;
	char * data = (char *)kfFirstDataAddr(inField);
	char * dataLimit = (char *)inField + inField->length - kSizeOfLengthWords;
	do {
		lastData = data;
		size_t dataLen = kfSizeOfData(data);
		ALIGNkfSIZE(dataLen);
		data += dataLen;
	} while (data < dataLimit);
	return lastData;
}

size_t
CSoupIndex::kfDupCount(KeyField * inField)
{
	if (inField->type == KeyField::kDupData)
	{
		char * nextDup = (char *)inField + inField->length;
		short * dupCount = (short *)nextDup - 3;
		return *dupCount;
	}
	return 1;
}

void
CSoupIndex::kfSetDupCount(KeyField * inField, short inCount)
{
	if (inField->type == KeyField::kDupData)
	{
		char * nextDup = (char *)inField + inField->length;
		short * dupCount = (short *)nextDup - 3;
		*dupCount = inCount;
	}
}

ULong
CSoupIndex::kfNextDupId(KeyField * inField)
{
	if (inField->type == KeyField::kDupData)
	{
		char * nextDup = (char *)inField + inField->length;
		PSSId * nextDupId = (PSSId *)nextDup - 1;
		return *nextDupId;	// need to handle misaligned pointer
	}
	return 0;
}

void
CSoupIndex::kfSetNextDupId(KeyField * inField, PSSId inId)
{
	if (inField->type == 1)
	{
		char * nextDup = (char *)inField + inField->length;
		PSSId * nextDupId = (PSSId *)nextDup - 1;
		*nextDupId = inId;	// need to handle misaligned pointer
	}
}

void
CSoupIndex::kfInsertData(KeyField * ioField, void * inLocation, void * inData)
{
	size_t dataSize = kfSizeOfData(inData);
	ALIGNkfSIZE(dataSize);
	if (ioField->type == KeyField::kDupData)
	{
		char * fieldEnd = (char *)ioField + ioField->length;
		memmove((char *)inLocation + dataSize, inLocation, fieldEnd - (char *)inLocation);	// open up a gap
		memmove(inLocation, inData, dataSize);		// copy data into the location
		ioField->length += dataSize;
		kfSetDupCount(ioField, kfDupCount(ioField) + 1);
	}
}

void
CSoupIndex::kfDeleteData(KeyField * ioField, void * inData)
{
	size_t dataSize = kfSizeOfData(inData);
	ALIGNkfSIZE(dataSize);
	if (ioField->type == KeyField::kDupData)
	{
		char * fieldEnd = (char *)ioField + ioField->length;
		char * dataEnd = (char *)inData + dataSize;
		memmove(inData, dataEnd, fieldEnd - dataEnd);		// close up the gap
		ioField->length -= dataSize;
		kfSetDupCount(ioField, kfDupCount(ioField) - 1);
	}
}

void
CSoupIndex::kfReplaceFirstData(KeyField * ioField, void * inData)
{
	size_t keySize = SKey::kOffsetToData + kfSizeOfKey(ioField->buf);
	ALIGNkfSIZE(keySize);
	size_t dataSize = kfSizeOfData(inData);
	ALIGNkfSIZE(dataSize);
	if (ioField->type == KeyField::kData)
	{
		// it’s a singleton
		ioField->length = keySize + dataSize;				// update the size
		memmove((char *)ioField + keySize, inData, dataSize);	// copy data into data space
	}
	else if (ioField->type == KeyField::kDupData)
	{
		char * fieldData = (char *)ioField + keySize;
		size_t fieldDataSize = kfSizeOfData(fieldData);
		ALIGNkfSIZE(fieldDataSize);
		int delta = dataSize - fieldDataSize;
		if (delta != 0)
		{
			char * fieldEnd = (char *)ioField + ioField->length;
			char * fieldDataEnd = fieldData + fieldDataSize;
			memmove(fieldDataEnd + delta, fieldDataEnd, fieldEnd -  fieldDataEnd);	// adjust existing field space
		}
		memmove(fieldData, inData, dataSize);	// copy data into data space
	}
}

void *
CSoupIndex::kfFindDataAddr(KeyField * inField, void * inData, void ** outData)
{
	void * data = NULL;
	while (kfNextDataAddr(inField, data, &data))
	{
		if ((this->*fCompareDataFn)(*(const SKey *)inData, *(const SKey *)data) == 0)
			return data;
		if (outData)
			*outData = data;
	}
	return NULL;
}

void
CSoupIndex::kfConvertKeyField(int inArg1, KeyField * inField)
{
	//size_t theSize = kfSizeOfKey(inField->buf);	// unused
	if (inField->type == KeyField::kData && inArg1 == 1)
	{
		inField->type = KeyField::kDupData;
		inField->length += 6;	// sizeof(DupInfo) ?
		kfSetDupCount(inField, 1);
		kfSetNextDupId(inField, 0);
	}
}

#pragma mark DupData

void *
CSoupIndex::firstDupDataAddr(DupNodeHeader * inDupNode)
{
	return &inDupNode->x10;
}


bool
CSoupIndex::nextDupDataAddr(DupNodeHeader * inDupNode, void * inData, void ** outData)
{
	if (inData == NULL)
	{
		*outData = firstDupDataAddr(inDupNode);
		return true;
	}
	char * limit = ((char *)inDupNode) + inDupNode->bytesUsed;
	size_t dataSize = kfSizeOfData(inData);
	ALIGNkfSIZE(dataSize);
	char * nextAddr = (char *)inData + dataSize;
	*outData = nextAddr;
	return nextAddr < limit;
}


void *
CSoupIndex::lastDupDataAddr(KeyField * inKeyField, DupNodeHeader ** ioDupNode)
{
	PSSId dupId;
	if ((dupId = kfNextDupId(inKeyField)) == 0)
	{
		if (ioDupNode != NULL)
			*ioDupNode = NULL;
		return kfLastDataAddr(inKeyField);
	}
	DupNodeHeader * dupNode = readADupNode(dupId);
	while ((dupId = dupNode->nextId) != 0)
		dupNode = readADupNode(dupId);
	void * lastAddr;
	char * anAddr = (char *)firstDupDataAddr(dupNode);
	char * limit = ((char *)dupNode) + dupNode->bytesUsed;
	do {
		lastAddr = anAddr;
		size_t dataSize = kfSizeOfData(anAddr);
		ALIGNkfSIZE(dataSize);
		anAddr += dataSize;
	} while (anAddr < limit);
	if (ioDupNode)
		*ioDupNode = dupNode;
	return lastAddr;
}


bool
CSoupIndex::appendDupData(DupNodeHeader * inDupNode, void * inData)
{
	char * p = (char *)inDupNode + inDupNode->bytesUsed;
	size_t dataSize = kfSizeOfData(inData);
	ALIGNkfSIZE(dataSize);
	if (dataSize > inDupNode->bytesRemaining)
		return false;	// no room

	memmove(p, inData, dataSize);

	inDupNode->bytesRemaining -= dataSize;
	inDupNode->bytesUsed += dataSize;
	inDupNode->numOfSlots++;
	return true;
}


bool
CSoupIndex::prependDupData(DupNodeHeader * inDupNode, void * inData)
{
	char * p = (char *)firstDupDataAddr(inDupNode);
	size_t dataSize = kfSizeOfData(inData);
	ALIGNkfSIZE(dataSize);
	if (dataSize > inDupNode->bytesRemaining)
		return false;	// no room

	char * pEnd = p + dataSize;
	char * limit = (char *)inDupNode + inDupNode->bytesUsed;
	memmove(pEnd, p, limit - pEnd);

	memmove(p, inData, dataSize);

	inDupNode->bytesRemaining -= dataSize;
	inDupNode->bytesUsed += dataSize;
	inDupNode->numOfSlots++;
	return true;
}


bool
CSoupIndex::deleteDupData(DupNodeHeader * inDupNode, void * inData)
{
	changeNode((NodeHeader *)inDupNode);

	size_t dataSize = kfSizeOfData(inData);
	ALIGNkfSIZE(dataSize);

	char * dataEnd = (char *)inData + dataSize;
	char * limit = (char *)inDupNode + inDupNode->bytesUsed;
	memmove(inData, dataEnd, limit - dataEnd);

	inDupNode->bytesRemaining += dataSize;
	inDupNode->bytesUsed -= dataSize;
	inDupNode->numOfSlots--;
	return true;
}


void *
CSoupIndex::findDupDataAddr(DupNodeHeader * inDupNode, void * inData, void ** outData)
{
	void * theData = NULL;
	if (outData != NULL)
		*outData = NULL;
	while (nextDupDataAddr(inDupNode, theData, &theData))
	{
		if ((this->*fCompareDataFn)(*(const SKey *)inData, *(const SKey *)theData) == 0)
			return theData;
		if (outData != NULL)
			*outData = theData;
	}
	return NULL;
}


void *
CSoupIndex::findNextDupDataAddr(DupNodeHeader ** ioDupNode, void * inData, bool * outFound)
{
	PSSId dupId;
	void * dupDataAddr;
	while ((dupDataAddr = findDupDataAddr(*ioDupNode, inData, NULL)) == 0)
	{
		if ((dupId = (*ioDupNode)->nextId) != 0)
			*ioDupNode = readADupNode(dupId);
		else
		{
			if (outFound != NULL)
				*outFound = false;
			return NULL;
		}
	}

	if (outFound != NULL)
		*outFound = true;
	if (nextDupDataAddr(*ioDupNode, dupDataAddr, &dupDataAddr))
		return dupDataAddr;
	if ((dupId = (*ioDupNode)->nextId) != 0)
	{
		*ioDupNode = readADupNode(dupId);
		return firstDupDataAddr(*ioDupNode);
	}
	return NULL;
}


void *
CSoupIndex::findPriorDupDataAddr(DupNodeHeader ** ioDupNode, void * inData, bool * outFound)
{
	DupNodeHeader * priorDupNode;
	void * priorData;
	PSSId dupId;
	void * dupDataAddr;
	while (findDupDataAddr(*ioDupNode, inData, &dupDataAddr) == 0)
	{
		priorData = dupDataAddr;
		priorDupNode = *ioDupNode;
		if ((dupId = (*ioDupNode)->nextId) != 0)
			*ioDupNode = readADupNode(dupId);
		else
		{
			if (outFound != NULL)
				*outFound = false;
			return NULL;
		}
	}

	if (outFound != NULL)
		*outFound = true;
	if (dupDataAddr == NULL)
	{
		if (priorData != NULL)
		{
			*ioDupNode = priorDupNode;
			return priorData;
		}
	}
	return NULL;
}

#pragma mark Node Walking

ULong
CSoupIndex::leftNodeNo(NodeHeader * inNode, int inSlot)
{
	KeyField * kf = keyFieldAddr(inNode, inSlot);
	ULong * nodeNumPtr = (ULong *)kf - 1;
	return *nodeNumPtr;	// need to handle misaligned pointer
}


ULong
CSoupIndex::rightNodeNo(NodeHeader * inNode, int inSlot)
{
	KeyField * kf = keyFieldAddr(inNode, inSlot+1);
	ULong * nodeNumPtr = (ULong *)kf - 1;
	return *nodeNumPtr;	// need to handle misaligned pointer
}


ULong
CSoupIndex::firstNodeNo(NodeHeader * inNode)
{
	return leftNodeNo(inNode, 0);
}


ULong
CSoupIndex::lastNodeNo(NodeHeader * inNode)
{
	return leftNodeNo(inNode, inNode->numOfSlots);
}


KeyField *
CSoupIndex::keyFieldAddr(NodeHeader * inNode, int inSlot)
{
	return (KeyField *)((char *)inNode + inNode->keyFieldOffset[inSlot]);
}


KeyField *
CSoupIndex::firstKeyField(NodeHeader * inNode)
{
	return keyFieldAddr(inNode, 0);
}

KeyField *
CSoupIndex::lastKeyField(NodeHeader * inNode)
{
	return keyFieldAddr(inNode, lastSlotInNode(inNode));
}

char *
CSoupIndex::keyFieldBase(NodeHeader * inNode)
{
	return (char *)&inNode->keyFieldOffset[1 + inNode->numOfSlots] + inNode->bytesRemaining;
}

#pragma mark -

void
CSoupIndex::createNewRoot(KeyField * inField, ULong inArg2)
{
	NodeHeader * theNode = newNode();
	putKeyIntoNode(inField, inArg2, theNode, 0);
	setNodeNo(theNode, 0, fInfo.rootNodeId);
	setRootNode(theNode->id);
}


bool
CSoupIndex::insertKey(KeyField * inField, NodeHeader * inNode, ULong * ioArg3, bool * ioArg4)
{
	PSSId theId;
	int theSlot;
	if (keyInNode(inField, inNode, &theId, &theSlot))
	{
		changeNode(inNode);
		insertDupData(inField, inNode, theSlot, ioArg3, ioArg4);
	}
	else
	{
		if (theId == 0)
		{
			*ioArg4 = true;
			*ioArg3 = 0;
		}
		else
		{
			insertKey(inField, readANode(theId, inNode->id), ioArg3, ioArg4);
		}
		if (*ioArg4)
		{
			keyInNode(inField, inNode, &theId, &theSlot);
			if (roomInNode(inNode, inField))
			{
				*ioArg4 = false;
				changeNode(inNode);
				putKeyIntoNode(inField, *ioArg3, inNode, theSlot);
				fCache->dirtyNode(inNode);
			}
			else
			{
				*ioArg4 = true;
				splitANode(inField, ioArg3, inNode, theSlot);
			}
		}
	}
	return f_BTStatus == 0;
}


void
CSoupIndex::insertAfterDelete(KeyField * inField, ULong inArg2, NodeHeader * inNode)
{
	PSSId theId;
	int theSlot;

	if (inNode != NULL)
	{
		while (!roomInNode(inNode, inField))
		{
			keyInNode(inField, inNode, &theId, &theSlot);
			splitANode(inField, &inArg2, inNode, theSlot);
			if (inNode->nextId == 0)
			{
				inNode = NULL;
				break;
			}
			if ((inNode = fCache->findNode(this, inArg2)) == NULL)
				break;
		}
	}

	if (inNode != NULL)
	{
		changeNode(inNode);
		keyInNode(inField, inNode, &theId, &theSlot);
		putKeyIntoNode(inField, inArg2, inNode, theSlot);
		fCache->dirtyNode(inNode);
	}
	else
		createNewRoot(inField, inArg2);
}


bool
CSoupIndex::deleteKey(KeyField * inField, NodeHeader * inNode, bool * outArg3)
{
	PSSId theId;
	int theSlot;
	bool r9 = false;

	if (keyInNode(inField, inNode, &theId, &theSlot))
	{
		if (theId == 0)
		{
			r9 = true;
			if (fSavedKey->length != 0)
				fSavedKey->length = 0;
			else
				deleteTheKey(inNode, theSlot, inField);
			if (!(*outArg3 = nodeUnderflow(inNode)))
				fCache->dirtyNode(inNode);
		}
		else
		{
			r9 = true;
			KeyField * r10 = keyFieldAddr(inNode, theSlot);
			if (r10->type == KeyField::kDupData
			&& ((kfDupCount(r10) > 1
			  || kfNextDupId(r10) != 0)))
			{
				deleteTheKey(inNode, theSlot, inField);
				*outArg3 = false;
			}
			else
			{
				ULong r10 = rightNodeNo(inNode, theSlot);
				*outArg3 = getLeafKey(fLeafKey, readANode(r10, inNode->id));
				deleteTheKey(inNode, theSlot, inField);
				insertAfterDelete(fLeafKey, r10, inNode);
				if (*outArg3)
				{
					moveKey(fSavedKey, fLeafKey);
					_BTRemoveKey(fLeafKey);
					*outArg3 = false;
				}
			}
		}
	}
	else
	{
		if (theId == 0)
		{
			if (fSavedKey->length != 0)
			{
				r9 = true;
				fSavedKey->length = 0;
				if (!(*outArg3 = nodeUnderflow(inNode)))
					fCache->dirtyNode(inNode);
			}
		}
		else
		{
			NodeHeader * aNode;
			if ((aNode = readANode(theId, inNode->id)) != NULL)
			{
				r9 = deleteKey(inField, inNode, outArg3);
				if (*outArg3)
					*outArg3 = balanceTwoNodes(inNode, aNode, theSlot);
			}
		}
	}
	

	return r9;
}


bool
CSoupIndex::moveKey(KeyField * inFromField, KeyField * outToField)
{
	size_t fieldSize = inFromField->length;
	if (fieldSize != 0)
	{
		memmove(outToField, inFromField, fieldSize);
		return true;
	}
	outToField->length = 0;
	return false;
}


void
CSoupIndex::copyKeyFmNode(KeyField * outField, ULong * outNo, NodeHeader * inNode, int inSlot)
{
	KeyField * theField = keyFieldAddr(inNode, inSlot);
	memmove(outField, theField, theField->length);
	*outNo = leftNodeNo(inNode, inSlot);
}


KeyField *
CSoupIndex::keyBeforeNodeNo(NodeHeader * inNode, ULong inNo, int * outSlot)
{
	int slot = 0;
	while (leftNodeNo(inNode, slot) != inNo)
		slot++;
	*outSlot = slot-1;
	if (slot != 0)
		return keyFieldAddr(inNode, *outSlot);
	return NULL;
}


KeyField *
CSoupIndex::keyAfterNodeNo(NodeHeader * inNode, ULong inNo, int * outSlot)
{
	int slot = 0;
	*outSlot = slot;
	while (leftNodeNo(inNode, slot) != inNo)
		*outSlot = ++slot;
	return keyFieldAddr(inNode, *outSlot);
}


int
CSoupIndex::lastSlotInNode(NodeHeader * inNode)
{
	return inNode->numOfSlots - 1;		// could make this inline and use it as necessary
}


size_t
CSoupIndex::bytesInNode(NodeHeader * inNode)
{
	return fInfo.nodeSize - inNode->bytesRemaining;		// could make this inline and use it as necessary
}


bool
CSoupIndex::roomInNode(NodeHeader * inNode, KeyField * inKeyField)
{
	return (inKeyField->length + kSizeOfLengthWords) < inNode->bytesRemaining;		// could make this inline and use it as necessary
}


bool
CSoupIndex::nodeUnderflow(NodeHeader * inNode)
{
	return inNode->bytesRemaining > fInfo.nodeSize/2;
}


void
CSoupIndex::putKeyIntoNode(KeyField * inField, ULong inArg2, NodeHeader * ioNode, int inSlot)
{
	if (!roomInNode(ioNode, inField))
		ThrowErr(exStore, kNSErrInternalError);

	char * r10 = keyFieldBase(ioNode) - inField->length;
	memmove(r10, inField, inField->length);
	int numOfSlotsToMove = (ioNode->numOfSlots - inSlot) + 1;
if (numOfSlotsToMove <= 0)
	printf("huh?\n");
	// open up a gap for the offset
	memmove(&ioNode->keyFieldOffset[inSlot+1], &ioNode->keyFieldOffset[inSlot], numOfSlotsToMove * sizeof(short));
	// insert offset to new key
	ioNode->keyFieldOffset[inSlot] = r10 - (char *)ioNode;
	setNodeNo(ioNode, inSlot, rightNodeNo(ioNode, inSlot));
	setNodeNo(ioNode, inSlot+1, inArg2);
	ioNode->numOfSlots++;
	ioNode->bytesRemaining -= (KeyField::kOffsetToData + SKey::kOffsetToData + SKey::kOffsetToData + inField->length);
}


void
CSoupIndex::deleteKeyFromNode(NodeHeader * ioNode, int inSlot)
{
	if (inSlot < 0 || inSlot >= ioNode->numOfSlots)
		ThrowErr(exStore, kNSErrInternalError);

	setNodeNo(ioNode, inSlot+1, leftNodeNo(ioNode, inSlot));
	KeyField * kf = keyFieldAddr(ioNode, inSlot);
	size_t r6 = SKey::kOffsetToData + SKey::kOffsetToData + kf->length;
	char * r1 = keyFieldBase(ioNode);
	short r9 = ioNode->keyFieldOffset[inSlot] + r6;
	memmove(r1+r6, r1, (char *)kf-r1-(SKey::kOffsetToData + SKey::kOffsetToData));
	size_t numOfSlotsToMove = (ioNode->numOfSlots - inSlot) + 1;
	memmove(&ioNode->keyFieldOffset[inSlot], &ioNode->keyFieldOffset[inSlot+1], numOfSlotsToMove * sizeof(short));
	ioNode->numOfSlots--;
	ioNode->bytesRemaining += (KeyField::kOffsetToData + SKey::kOffsetToData + SKey::kOffsetToData + kf->length);
	for (int slot = 0; slot < ioNode->numOfSlots; slot++)
	{
		short offset = ioNode->keyFieldOffset[slot];
		if (offset < r9)
			ioNode->keyFieldOffset[slot] = offset + r6;
	}
}


bool
CSoupIndex::keyInNode(KeyField * inKeyField, NodeHeader * inNode, PSSId * outId, int * outSlot)
{
	// perform binary search for key
	int cmp = 0;
	int index = 0;
	int firstIndex = 0;
	int lastIndex = lastSlotInNode(inNode);
	while (lastIndex >= firstIndex)
	{
		index = (firstIndex + lastIndex)/2;

		KeyField * kf = keyFieldAddr(inNode, index);
		cmp = compareKeys(*(const SKey *)inKeyField->buf, *(const SKey *)kf->buf);

		if (cmp > 0)
		// key must be in higher range
			firstIndex = index + 1;
		else if (cmp < 0)
		// key must be in lower range
			lastIndex = index - 1;
		else
		{
		// got it!
			*outSlot = index;
			*outId = leftNodeNo(inNode, index);
			return true;
		}	
	}
	if (cmp > 0)
		index++;
	*outSlot = index;
	*outId = leftNodeNo(inNode, index);
	return false;
}


bool
CSoupIndex::findFirstKey(NodeHeader * inNode, KeyField * outKey)
{
	ULong nodeNo;
	while ((nodeNo = firstNodeNo(inNode)) != 0)
		readANode(nodeNo, inNode->id);
	moveKey(firstKeyField(inNode), outKey);
	return true;
}


bool
CSoupIndex::findLastKey(NodeHeader * inNode, KeyField * outKey)
{
	ULong nodeNo;
	while ((nodeNo = lastNodeNo(inNode)) != 0)
		readANode(nodeNo, inNode->id);
	KeyField * kf = lastKeyField(inNode);
	moveKey(kf, outKey);
	if (kf->type == KeyField::kDupData)
		kfReplaceFirstData(outKey, lastDupDataAddr(kf, NULL));
	return true;
}


bool
CSoupIndex::balanceTwoNodes(NodeHeader * inNode1, NodeHeader * inNode2, int inSlot)
{
	NodeHeader * node3;
	ULong nodeNo;
	KeyField sp00;
	KeyField * kf = keyFieldAddr(inNode1, inSlot);
	if (kf->length == 0)
	{
		inSlot = lastSlotInNode(inNode1);
		copyKeyFmNode(&sp00, &nodeNo, inNode1, inSlot);
		rightNodeNo(inNode1, inSlot);	// sic -- doesn’t do anything with the result
		node3 = inNode2;
		inNode2 = readANode(nodeNo, inNode1->id);
	}
	else
	{
		copyKeyFmNode(&sp00, &nodeNo, inNode1, inSlot);
		nodeNo = rightNodeNo(inNode1, inSlot);
		node3 = readANode(nodeNo, inNode1->id);
	}

	changeNode(inNode1);
	deleteKeyFromNode(inNode1, inSlot);
	fCache->dirtyNode(inNode1);

	changeNode(inNode2);
	changeNode(node3);
	
	return mergeTwoNodes(kf, inNode1, inNode2, node3);
}


bool
CSoupIndex::mergeTwoNodes(KeyField * inField, NodeHeader * inNode1, NodeHeader * inNode2, NodeHeader * inNode3)
{
	bool result;
	bool isNode2Underflow = nodeUnderflow(inNode2);
	if (isNode2Underflow)
	{
		putKeyIntoNode(inField, firstNodeNo(inNode3), inNode2, inNode2->numOfSlots);
	}
	else
	{
		putKeyIntoNode(inField, firstNodeNo(inNode3), inNode3, 0);
		setNodeNo(inNode3, 0, lastNodeNo(inNode2));
	}

	if (bytesInNode(inNode2) - sizeof(NodeHeader) + bytesInNode(inNode3) - sizeof(NodeHeader) <= fInfo.nodeSize - sizeof(NodeHeader))
	{
		for (int slot = 0; slot < inNode3->numOfSlots; slot++)
			putKeyIntoNode(keyFieldAddr(inNode3, slot), rightNodeNo(inNode3, slot), inNode2, inNode2->numOfSlots);
		fCache->dirtyNode(inNode2);
		deleteNode(inNode3->id);
		result = nodeUnderflow(inNode1);
		if (!result)
			fCache->dirtyNode(inNode1);
	}
	else
	{
		if (isNode2Underflow)
		{
			int slot;
			for (slot = 0; slot < inNode3->numOfSlots && nodeUnderflow(inNode2); ++slot)
			{
				putKeyIntoNode(keyFieldAddr(inNode3, 0), rightNodeNo(inNode3, 0), inNode2, inNode2->numOfSlots);
				deleteKeyFromNode(inNode3, 0);
			}
			moveKey(keyFieldAddr(inNode3, 0), inField);
			setNodeNo(inNode3, 0, rightNodeNo(inNode3, 0));
			deleteKeyFromNode(inNode3, 0);
		}
		else
		{
			int slot;
			for (slot = lastSlotInNode(inNode2); slot > 0 && nodeUnderflow(inNode3); --slot)
			{
				putKeyIntoNode(keyFieldAddr(inNode2, slot), rightNodeNo(inNode2, slot), inNode3, 0);
				deleteKeyFromNode(inNode2, slot);
			}
			moveKey(keyFieldAddr(inNode2, slot), inField);
			setNodeNo(inNode3, 0, lastNodeNo(inNode2));
			deleteKeyFromNode(inNode2, slot);
		}
		fCache->dirtyNode(inNode2);
		fCache->dirtyNode(inNode3);
		result = false;
		insertAfterDelete(inField, inNode3->id, inNode1);
	}
	
	return result;
}


void
CSoupIndex::splitANode(KeyField * inField, ULong * ioNo, NodeHeader * ioNode, int inSlot)
{
	NodeHeader * neoNode = newNode();
	if (inSlot == ioNode->numOfSlots)
		putKeyIntoNode(inField, *ioNo, neoNode, 0);

	int slot;
	for (slot = lastSlotInNode(ioNode); slot > 1 && nodeUnderflow(neoNode); --slot)
	{
		putKeyIntoNode(keyFieldAddr(ioNode, slot), rightNodeNo(ioNode, slot), neoNode, 0);
		changeNode(ioNode);
		deleteKeyFromNode(ioNode, slot);
		if (inSlot == slot)
			putKeyIntoNode(inField, *ioNo, neoNode, 0);
	}
	if (inSlot < slot)
	{
		changeNode(ioNode);
		putKeyIntoNode(inField, *ioNo, ioNode, inSlot);
		slot++;
	}
	setNodeNo(neoNode, 0, rightNodeNo(ioNode, slot));
	moveKey(keyFieldAddr(ioNode, slot), inField);
	changeNode(ioNode);
	deleteKeyFromNode(ioNode, slot);

	*ioNo = neoNode->id;

	fCache->dirtyNode(ioNode);
}


void
CSoupIndex::deleteTheKey(NodeHeader * ioNode, int inSlot, KeyField * inField)
{
	// r5 r7 r6
	//sp-08
	PSSId dupId;
	int cmp = 1;	// r9
	int sp00;
	changeNode(ioNode);
	void * r10 = kfFirstDataAddr(inField);
	void * sp04 = NULL;
	KeyField * r8 = keyFieldAddr(ioNode, inSlot);
	while (kfNextDataAddr(r8, sp04, &sp04))
	{
		if ((cmp = (this->*fCompareDataFn)(*(const SKey *)r10, *(const SKey *)sp04)) == 0)
		{
			sp00 = (char *)sp04 - (char *)r8;
			break;
		}
	}
	if (r8->type == 0)
	{
		if (cmp != 0)
			ThrowOSErr(2);
		deleteKeyFromNode(ioNode, inSlot);
		fCache->dirtyNode(ioNode);
	}
	else
	{
		if (cmp == 0)
		{
			fCache->dirtyNode(ioNode);
			if (kfDupCount(r8) > 1)
			{
				ULong rNode = rightNodeNo(ioNode, inSlot);
				moveKey(r8, inField);
				kfDeleteData(inField, (char *)inField + sp00);
				deleteKeyFromNode(ioNode, inSlot);
				putKeyIntoNode(inField, rNode, ioNode, inSlot);
			}
			else if ((dupId = kfNextDupId(r8)) != 0)
			{
				DupNodeHeader * r9 = readADupNode(dupId);
				moveKey(r8, inField);
				void * r10 = firstDupDataAddr(r9);
				kfReplaceFirstData(inField, r10);
				if (r9->numOfSlots == 1)
					kfSetNextDupId(inField, r9->nextId);
				ULong rNode = rightNodeNo(ioNode, inSlot);
				deleteKeyFromNode(ioNode, inSlot);
				putKeyIntoNode(inField, rNode, ioNode, inSlot);
				deleteDupData(r9, r10);
				if (r9->numOfSlots == 0)
					deleteNode(r9->id);
				else
					fCache->dirtyNode((NodeHeader *)r9);
			}
			else
				deleteKeyFromNode(ioNode, inSlot);
		}
		else
		{
			void * dupData;
			DupNodeHeader * r6 = NULL, * r7;
			while ((dupId = kfNextDupId(r8)) != 0)
			{
				r7 = r6;
				r6 = readADupNode(dupId);
				dupId = r6->nextId;
				if ((dupData = findDupDataAddr(r6, r10, NULL)))
				{
					deleteDupData(r6, dupData);
					if (r6->numOfSlots == 0)
					{
						if (r7 != NULL)
						{
							r7->nextId = r6->nextId;
							fCache->dirtyNode((NodeHeader *)r7);
						}
						else
						{
							kfSetNextDupId(r8, r6->nextId);
							fCache->dirtyNode(ioNode);
						}
						deleteNode(r6->id);
					}
					else
						fCache->dirtyNode((NodeHeader *)r6);
					return;
				}
			}
			ThrowOSErr(2);
		}
	}
}


bool
CSoupIndex::getLeafKey(KeyField * inField, NodeHeader * inNode)
{
	ULong nodeNo;
	while ((nodeNo = firstNodeNo(inNode)) != 0)
		readANode(nodeNo, inNode->id);
	copyKeyFmNode(inField, &nodeNo, NULL, 0);
	changeNode(inNode);
	deleteKeyFromNode(inNode, 0);
	fCache->dirtyNode(inNode);

	fSavedKey->length = 0;
	bool isUnderflow;
	if ((isUnderflow = nodeUnderflow(inNode)))
		copyKeyFmNode(fSavedKey, &nodeNo, NULL, 0);
	return isUnderflow;
}


void
CSoupIndex::checkForDupData(KeyField * inField, void * inData)
{
	if (fInfo.x10 == 0)
		ThrowOSErr(1);

	void * data;
	if ((data = kfFindDataAddr(inField, inData, NULL)) != NULL)
		ThrowOSErr(1);

	PSSId dupId;
	if (inField->type != KeyField::kData		// sic -- better to test == KeyField::kDupData?
	&&  (dupId = kfNextDupId(inField)) != 0)
	{
		DupNodeHeader * dupNode = readADupNode(dupId);
		data = NULL;
		while (nextDupDataAddr(dupNode, data, &data))
		{
			if ((this->*fCompareDataFn)(*(const SKey *)inData, *(const SKey *)data) == 0)
				ThrowOSErr(1);
		}
	}
}


void
CSoupIndex::storeDupData(KeyField * inField, void * inData)
{
	void * fieldData = NULL;
	DupNodeHeader * theNode = NULL;
	PSSId dupId;
	for (dupId = kfNextDupId(inField); dupId != 0; dupId = theNode->nextId)
		theNode = readADupNode(dupId);
	if (theNode)
	{
		changeNode((NodeHeader *)theNode);
		if (kfSizeOfData(inData) > theNode->bytesRemaining)
		{
			DupNodeHeader * neoNode = newDupNode();
			appendDupData(neoNode, inData);
			theNode->nextId = neoNode->id;
		}
		else
			appendDupData(theNode, inData);
		fCache->dirtyNode((NodeHeader *)theNode);
	}
	else
	{
		if (inField->length + kfSizeOfData(inData) < kKeyFieldBufSize)	// 100
		{
			while (kfNextDataAddr(inField, fieldData, &fieldData))
				;
			kfInsertData(inField, fieldData, inData);
		}
		else
		{
			DupNodeHeader * neoNode = newDupNode();
			appendDupData(neoNode, inData);
			kfSetNextDupId(inField, neoNode->id);
		}
	}
}


void
CSoupIndex::insertDupData(KeyField * inField, NodeHeader * inNode, int inSlot, ULong * outNodeNo, bool * outSplit)
{
	void * sp00;
	char sp04[kKeyFieldBufSize];
	*outSplit = false;
	void * fieldData = kfFirstDataAddr(inField);
	memmove(&sp04, fieldData, kfSizeOfData(fieldData));

	sp00 = &sp04;
	KeyField * r8 = keyFieldAddr(inNode, inSlot);
	checkForDupData(r8, sp00);
	if (fInfo.x10 == 1)
		ThrowOSErr(1);

	changeNode(inNode);
	*outNodeNo = rightNodeNo(inNode, inSlot);

	moveKey(r8, inField);
	deleteKeyFromNode(inNode, inSlot);
	kfConvertKeyField(1, inField);
	storeDupData(inField, sp00);
	if (roomInNode(inNode, inField))
	{
		putKeyIntoNode(inField, *outNodeNo, inNode, inSlot);
		fCache->dirtyNode(inNode);
	}
	else
	{
		*outSplit = true;
		splitANode(inField, outNodeNo, inNode, inSlot);
	}
}

#pragma mark Key Comparison

int
CSoupIndex::compareKeys(const SKey& inKey1, const SKey& inKey2)
{
	int cmp = (this->*fCompareKeyFn)(inKey1, inKey2);
	if (fInfo.x19)	// sorting
		cmp = -cmp;
	return cmp;
}

int
CSoupIndex::stringKeyCompare(const SKey& inKey1, const SKey& inKey2)
{
	return CompareUnicodeText(	(const UniChar *)inKey1.data(), inKey1.size()/sizeof(UniChar),
										(const UniChar *)inKey2.data(), inKey2.size()/sizeof(UniChar),
										fSortingTable, fSortingTable != NULL );
}


int
CSoupIndex::longKeyCompare(const SKey& inKey1, const SKey& inKey2)
{
	return (int)inKey1 - (int)inKey2;
}


int
CSoupIndex::characterKeyCompare(const SKey& inKey1, const SKey& inKey2)
{
	UniChar key1 = (UniChar)inKey1;
	UniChar key2 = (UniChar)inKey2;
	if (key1 < key2)
		return -1;
	if (key1 > key2)
		return 1;
	return 0;
}


int
CSoupIndex::doubleKeyCompare(const SKey& inKey1, const SKey& inKey2)
{
	double key1 = (double)inKey1;
	double key2 = (double)inKey2;
	if (key1 < key2)
		return -1;
	if (key1 > key2)
		return 1;
	return 0;
}


int
CSoupIndex::asciiKeyCompare(const SKey& inKey1, const SKey& inKey2)
{
	char * p1 = (char *)inKey1.data();
	size_t len1 = inKey1.size();
	char * p2 = (char *)inKey2.data();
	size_t len2 = inKey2.size();
	char ch1, ch2;
	size_t i;
	for (i = 0; i < len1 && i < len2; ++i)
	{
		ch1 = p1[i];
		if (ch1 >= 'A' && ch1 <= 'Z')
			ch1 = 'a' - 'A';
		ch2 = p2[i];
		if (ch2 >= 'A' && ch2 <= 'Z')
			ch2 = 'a' - 'A';
		if (ch1 < ch2)
			return -1;
		if (ch1 > ch2)
			return 1;
	}
	if (i < len1)
		return 1;
	if (i < len2)
		return -1;
	return 0;
}


int
CSoupIndex::rawKeyCompare(const SKey& inKey1, const SKey& inKey2)
{
	int cmp, lenCmp;
	size_t len1 = inKey1.size();
	size_t len2 = inKey2.size();
	if (len1 < len2)
		lenCmp = -1;
	else if (len1 > len2)
	{
		lenCmp = 1;
		len1 = len2;
	}
	else
		lenCmp = 0;
	if ((cmp = memcmp(inKey1.data(), inKey2.data(), len1)) == 0)
		return lenCmp;
	return cmp;	
}


int
CSoupIndex::multiKeyCompare(const SKey& inKey1, const SKey& inKey2)
{
	ULong multiTypes = fInfo.x14;
	char multiSort = fInfo.x18;

	char * key1 = (char *)inKey1.data();
	char * key1limit = key1 + inKey1.size();
	char * key2 = (char *)inKey2.data();
	char * key2limit = key2 + inKey2.size();

	char key1flags = inKey1.flags();
	char key2flags = inKey2.flags();

	int cmp;
	for (cmp = 0; cmp == 0; multiTypes <<= 4, multiSort >>= 1, key1flags >>= 1, key2flags >>= 1)
	{
		// end-of-multikeys check
		if (key1 == key1limit)
		{
			// nothing to compare!
			if (key2 == key2limit)
				return 0;
			return (key1flags & 0x80) ? 1 : -1;
		}
		if (key2 == key2limit)
		{
			return (key2flags & 0x80) ? -1 : 1;
		}

		if ((key1flags | key2flags) & 0x01)
		{
			if ((key1flags & 0x01) == 0)
				cmp = 1;
			else if ((key2flags & 0x01) == 0)
				cmp = -1;
		}

		else
		{
			int keyType = multiTypes & 0x0F;
			cmp = (this->*fKeyCompareFns[keyType])(*(const SKey *)key1, *(const SKey *)key2);
			if (cmp == 0)
			{
				size_t keySize;
				if ((keySize = fKeySizes[keyType]) == 0)
				{
					// we need to use the key’s given size to update the pointer to the next key
					keySize = SKey::kOffsetToData + ((SKey *)key1)->size();
					ALIGNkfSIZE(keySize);
					key1 += keySize;
					keySize = SKey::kOffsetToData + ((SKey *)key2)->size();
					ALIGNkfSIZE(keySize);
					key2 += keySize;
				}
				else
				{
					key1 += keySize;
					key2 += keySize;
				}
			}
		}
	}

	return (multiSort & 0x01) ? cmp : -cmp;
}

#pragma mark B-Tree

int
CSoupIndex::_BTEnterKey(KeyField * inField)
{
	ULong sp00;
	bool sp04 = false;
	NodeHeader * theNode;
	f_BTStatus = 0;
	if ((theNode = readRootNode(true)) != NULL
	&& insertKey(inField, theNode, &sp00, &sp04))
	{
		if (sp04)
			createNewRoot(inField, sp00);
		f_BTStatus = 0;
	}
	else
		f_BTStatus = 2;
	return f_BTStatus;
}

int
CSoupIndex::_BTGetNextKey(KeyField * inField)
{
	NodeHeader * theNode = readRootNode(false);
	if (theNode != NULL)
	{
		int theSlot;
		int cmp = searchNext(inField, &theNode, &theSlot);
		if (cmp == 0)
			return 3;
		if (cmp != -1)
			return 0;
	}
	return 2;
}

int
CSoupIndex::_BTGetNextDupKey(KeyField * inField)
{
	NodeHeader * theNode = readRootNode(false);
	if (theNode != NULL)
	{
		int theSlot;
		DupNodeHeader * dup;
		int cmp = searchNextDup(inField, &theNode, &theSlot, &dup);
		if (cmp == 0)
			return 3;
		if (cmp != -1)
			return 0;
	}
	return 2;
}

int
CSoupIndex::_BTGetPriorKey(KeyField * inField)
{
	NodeHeader * theNode = readRootNode(false);
	if (theNode != NULL)		// not in the original
	{
		int sp00;
		int cmp = searchPrior(inField, &theNode, &sp00);
		if (cmp == 0)
			return 3;
		if (cmp != -1)
			return 0;
	}
	return 2;
}

int
CSoupIndex::_BTGetPriorDupKey(KeyField * inField)
{
	NodeHeader * theNode = readRootNode(false);
	if (theNode != NULL)
	{
		int sp00;
		DupNodeHeader * dup;
		int cmp = searchPriorDup(inField, &theNode, &sp00, &dup);
		if (cmp == 0)
			return 3;
		if (cmp != -1)
			return 0;
	}
	return 2;
}

int
CSoupIndex::_BTRemoveKey(KeyField * inField)
{
	bool sp00 = false;
	bool r5;
	NodeHeader * theNode;
	if ((theNode = readRootNode(false)) != NULL
	&&  (r5 = deleteKey(inField, theNode, &sp00)))
	{
		if (sp00)
		{
			if (theNode->numOfSlots == 0)
			{
				setRootNode(firstNodeNo(theNode));
				deleteNode(theNode->id);
			}
			else
				fCache->dirtyNode(theNode);
		}
		f_BTStatus = r5 ? 0 : 2;	// r5 MUST be set at this point
	}
	else
		f_BTStatus = 2;
	return f_BTStatus;
}

#pragma mark -

bool
CSoupIndex::search(KeyField * ioKeyField, NodeHeader ** ioNode, int * ioSlot)
{
	PSSId theId;
	bool isFound = keyInNode(ioKeyField, *ioNode, &theId, ioSlot);
	if (isFound || theId == 0)
	{
		KeyField * kf = keyFieldAddr(*ioNode, *ioSlot);
		if (kf->length != 0)
		{
			moveKey(kf, ioKeyField);
		}
		else
		{
			*ioSlot = lastSlotInNode(*ioNode);
			findNextKey(ioKeyField, ioNode, ioSlot);
		}
		return isFound;
	}

	*ioNode = readANode(theId, (*ioNode)->id);
	return search(ioKeyField, ioNode, ioSlot);
}


int
CSoupIndex::searchNext(KeyField * ioKeyField, NodeHeader ** ioNode, int * ioSlot)
{
	PSSId theId;
	if (keyInNode(ioKeyField, *ioNode, &theId, ioSlot))
		return findNextKey(ioKeyField, ioNode, ioSlot);

	if (theId == 0)
	{
		KeyField * kf = keyFieldAddr(*ioNode, *ioSlot);
		if (kf->length != 0)
		{
			moveKey(kf, ioKeyField);
		}
		else
		{
			*ioSlot = lastSlotInNode(*ioNode);
			findNextKey(ioKeyField, ioNode, ioSlot);
		}
		return (ioKeyField->length == 0) ? 0 : -1;
	}

	*ioNode = readANode(theId, (*ioNode)->id);
	return searchNext(ioKeyField, ioNode, ioSlot);
}


int
CSoupIndex::searchPrior(KeyField * ioKeyField, NodeHeader ** ioNode, int * ioSlot)
{
	PSSId theId;
	if (keyInNode(ioKeyField, *ioNode, &theId, ioSlot))
		return findPriorKey(ioKeyField, ioNode, ioSlot);

	if (theId == 0)
	{
		if (*ioSlot != 0)
		{
			KeyField * kf = keyFieldAddr(*ioNode, *ioSlot - 1);
			moveKey(ioKeyField, kf);
		}
		else
		{
			findPriorKey(ioKeyField, ioNode, ioSlot);
		}
		return (ioKeyField->length == 0) ? 0 : -1;
	}

	*ioNode = readANode(theId, (*ioNode)->id);
	return searchPrior(ioKeyField, ioNode, ioSlot);
}


int
CSoupIndex::searchNextDup(KeyField * ioKeyField, NodeHeader ** ioNode, int * ioSlot, DupNodeHeader ** ioDupNode)
{
	PSSId theId;
	*ioDupNode = NULL;
	if (keyInNode(ioKeyField, *ioNode, &theId, ioSlot))
	{
		void * item1 = kfFirstDataAddr(ioKeyField);
		KeyField * kf = keyFieldAddr(*ioNode, *ioSlot);
		if (kf->type == 0)
		{
			void * item2 = kfFirstDataAddr(kf);
			return ((this->*fCompareDataFn)(*(const SKey *)item1, *(const SKey *)item2) == 0) ? 0 : -1;
		}

		void * sp00;
		if ((sp00 = kfFindDataAddr(ioKeyField, item1, NULL)) != NULL
		&&  kfNextDataAddr(ioKeyField, sp00, &sp00))
		{
			kfReplaceFirstData(ioKeyField, sp00);
			return 1;
		}

		PSSId dupId;
		if ((dupId = kfNextDupId(ioKeyField)) == 0)
			return (theId == 0) ? -1 : 0;

		bool huh;
		*ioDupNode = readADupNode(dupId);
		if (sp00 != NULL)
			sp00 = firstDupDataAddr(*ioDupNode);
		else
			sp00 = findNextDupDataAddr(ioDupNode, item1, &huh);
		if (sp00 != NULL)
		{
			kfReplaceFirstData(ioKeyField, sp00);
			return 1;
		}
		return huh ? 0 : -1;
	}

	if (theId == 0)
		return -1;

	*ioNode = readANode(theId, (*ioNode)->id);
	return searchNextDup(ioKeyField, ioNode, ioSlot, ioDupNode);
}


int
CSoupIndex::searchPriorDup(KeyField * ioKeyField, NodeHeader ** ioNode, int * ioSlot, DupNodeHeader ** ioDupNode)
{
	PSSId theId;
	*ioDupNode = NULL;
	if (keyInNode(ioKeyField, *ioNode, &theId, ioSlot))
	{
		void * item1 = kfFirstDataAddr(ioKeyField);
		KeyField * kf = keyFieldAddr(*ioNode, *ioSlot);
		if (kf->type == 0)
		{
			void * item2 = kfFirstDataAddr(kf);
			return ((this->*fCompareDataFn)(*(const SKey *)item1, *(const SKey *)item2) == 0) ? 0 : -1;
		}

		void * sp00;
		if (kfFindDataAddr(ioKeyField, item1, &sp00))
		{
			if (sp00 == 0)
				return 0;
			kfReplaceFirstData(ioKeyField, sp00);
			return 1;
		}

		PSSId dupId;
		if ((dupId = kfNextDupId(ioKeyField)) == 0)
			return -1;

		bool huh;
		*ioDupNode = readADupNode(dupId);
		sp00 = findPriorDupDataAddr(ioDupNode, item1, &huh);
		if (sp00 == NULL)
		{
			if (!huh)
				return -1;
			sp00 = kfLastDataAddr(ioKeyField);
		}
		kfReplaceFirstData(ioKeyField, sp00);
		return 1;
	}

	if (theId == 0)
		return -1;

	*ioNode = readANode(theId, (*ioNode)->id);
	return searchPriorDup(ioKeyField, ioNode, ioSlot, ioDupNode);
}


int
CSoupIndex::findNextKey(KeyField * ioField, NodeHeader ** ioNode, int * ioSlot)
{
	KeyField * kf;
	kf = keyFieldAddr(*ioNode, *ioSlot);
	if (kf->length == 0)
	{
		ioField->length = 0;
		return 0;
	}

	KeyField * r8;
	(*ioSlot)++;
	ULong nodeNo = leftNodeNo(*ioNode, *ioSlot);
	if (nodeNo != 0)
	{
		while (nodeNo != 0)
		{
			*ioNode = readANode(nodeNo, (*ioNode)->id);
			*ioSlot = 0;
			r8 = firstKeyField(*ioNode);
			nodeNo = firstNodeNo(*ioNode);
		}
		return moveKey(r8, ioField);
	}

	kf = keyFieldAddr(*ioNode, *ioSlot);
	if (kf->length != 0)
	{
		moveKey(kf, ioField);
		return 1;
	}

	PSSId thisId, nextId;
	while ((nextId = (*ioNode)->nextId) != 0 && kf->length == 0)
	{
		thisId = (*ioNode)->id;
		*ioNode = fCache->findNode(this, nextId);
		kf = keyAfterNodeNo(*ioNode, thisId, ioSlot);
	}
	return moveKey(kf, ioField);
}


int
CSoupIndex::findPriorKey(KeyField * inField, NodeHeader ** ioNode, int * ioSlot)
{
	KeyField * kf;
	NodeHeader * node = *ioNode;
	ULong nodeNo = leftNodeNo(node, *ioSlot);
	if (*ioSlot != 0 && nodeNo == 0)
	{
		(*ioSlot)--;
		kf = keyFieldAddr(node, *ioSlot);
	}
	else
	{
		if (nodeNo == 0)
		{
			PSSId priorId;
			for (priorId = 0; node->nextId != 0 && kf == NULL; kf = keyBeforeNodeNo(node, priorId, ioSlot))
			{
				priorId = node->id;
				node = fCache->findNode(this, node->nextId);
			}
			if (kf == NULL)
			{
				inField->length = 0;
				return 0;
			}
		}
		else
		{
			while (nodeNo)
			{
				node = readANode(nodeNo, node->id);
				kf = lastKeyField(node);
				nodeNo = lastNodeNo(node);
			}
			*ioSlot = lastSlotInNode(node);
		}
	}

	moveKey(kf, inField);
	if (kf->type == KeyField::kDupData)
		kfReplaceFirstData(inField, lastDupDataAddr(kf, NULL));
	*ioNode = node;
	return 1;
}


int
CSoupIndex::findAndGetState(KeyField * inField, IndexState * outState)
{
	int status;
	NodeHeader * rootNode = readRootNode(false);
	outState->node = rootNode;
	if (rootNode == NULL)
		return 3;

	if (search(inField, &outState->node, &outState->slot) == 0)
	{
		if (inField->length == 0)
			return 3;
		status = 2;
	}
	else
		status = 0;
	KeyField * kf = keyFieldAddr(outState->node, outState->slot);
	outState->isDup = (kf->type != KeyField::kData);
	outState->dupNode = NULL;
	return status;
}


int
CSoupIndex::findLastAndGetState(KeyField * inField, IndexState * outState)
{
	NodeHeader * rootNode = readRootNode(false);
	outState->node = rootNode;
	if (rootNode == NULL)
		return 3;

	ULong nodeNo;
	while ((nodeNo = lastNodeNo(outState->node)) != 0)
		outState->node = readANode(nodeNo, outState->node->id);
	outState->slot = lastSlotInNode(outState->node);

	KeyField * kf = lastKeyField(outState->node);
	moveKey(kf, inField);
	outState->isDup = (kf->type == KeyField::kDupData);
	if (outState->isDup)
	{
		outState->dupNode = NULL;
		kfReplaceFirstData(inField, lastDupDataAddr(kf, &outState->dupNode));
	}
	return 0;
}


int
CSoupIndex::findPriorAndGetState(KeyField * inField, bool inDoMove, IndexState * outState)
{
	// r6 r8 r4
	int status = findAndGetState(inField, outState);
	if (status == 0)
	{
		if (inDoMove)
			return moveUsingState(false, 0, inField, outState);
		if (outState->isDup)
		{
			KeyField * kf = keyFieldAddr(outState->node, outState->slot);
			kfReplaceFirstData(inField, lastDupDataAddr(kf, &outState->dupNode));
		}
	}
	else if (status == 2)
	{
		status = moveUsingState(false, 0, inField, outState);
		if (status == 0)
			status = 2;
	}
	else if (status == 3)
	{
		status = findLastAndGetState(inField, outState);
	}
	return status;
}


int
CSoupIndex::moveAndGetState(bool inDoForward, int inArg2, KeyField * inField, IndexState * outState)
{
	int status;
	NodeHeader * rootNode = readRootNode(false);
	outState->node = rootNode;
	if (rootNode == NULL)
		return 3;

	if (inArg2 == 1)
	{
		if (inDoForward)
			status = searchNext(inField, &outState->node, &outState->slot);
		else
			status = searchPrior(inField, &outState->node, &outState->slot);
		if (status == -1)
			return 2;
		if (status == 0)
			return 3;
	}
	else
	{
		if (inDoForward)
			status = searchNextDup(inField, &outState->node, &outState->slot, &outState->dupNode);
		else
			status = searchPriorDup(inField, &outState->node, &outState->slot, &outState->dupNode);
		if (status == 0)
		{
			if (inArg2 == 2)
				return 3;
			if (inDoForward)
				status = findNextKey(inField, &outState->node, &outState->slot);
			else
				status = findPriorKey(inField, &outState->node, &outState->slot);
			if (status == 0)
				return 3;
		}
		else if (status == -1)
			return 2;
		else
		{
			outState->isDup = true;
			return 0;
		}
	}
	KeyField * kf = keyFieldAddr(outState->node, outState->slot);
	outState->isDup = (kf->type != KeyField::kData);
	outState->dupNode = NULL;
	return 0;	
}


int
CSoupIndex::moveUsingState(bool inDoForward, int inArg2, KeyField * inField, IndexState * ioState)
{
	int status = 1;

	if (ioState->isDup)
	{
		if (inArg2 != 1)
		{
			//sp-04
			void * sp00;
			void * r8 = kfFirstDataAddr(inField);
			if (ioState->dupNode != NULL)
			{
				if (inDoForward)
					sp00 = findNextDupDataAddr(&ioState->dupNode, r8, NULL);
				else
				{
					//sp-04
					KeyField * kf = keyFieldAddr(ioState->node, ioState->slot);
					findDupDataAddr(ioState->dupNode, r8, &sp00);
					if (sp00 == NULL)
					{
						PSSId nextDupId = kfNextDupId(kf);
						if (nextDupId == ioState->dupNode->id)
						{
							ioState->dupNode = NULL;
							kfFindDataAddr(kf, r8, &sp00);
						}
						else
						{
							ioState->dupNode = readADupNode(nextDupId);
							sp00 = findPriorDupDataAddr(&ioState->dupNode, r8, NULL);
						}
					}
				}
			}
			else
			{
				KeyField * kf = keyFieldAddr(ioState->node, ioState->slot);
				bool r0 = inDoForward ?	kfNextDataAddr(kf, kfFindDataAddr(kf, r8, NULL), &sp00)
											 :	(kfFindDataAddr(kf, r8, &sp00) != NULL);
				if (!r0)
				{
					sp00 = NULL;
					if (inDoForward)
					{
						PSSId nextDupId = kfNextDupId(kf);
						if (nextDupId != 0)
						{
							ioState->dupNode = readADupNode(nextDupId);
							sp00 = firstDupDataAddr(ioState->dupNode);
						}
					}
					else
						return moveAndGetState(inDoForward, inArg2, inField, ioState);
				}
			}
			if (sp00 != NULL)
			{
				kfReplaceFirstData(inField, sp00);
				status = 0;
			}
			if (status == 0)
				return 0;
		}
	}

	if (inArg2 == 2)
		return 3;

	status = inDoForward ?	findNextKey(inField, &ioState->node, &ioState->slot)
								:	findPriorKey(inField, &ioState->node, &ioState->slot);
	if (status == 0)
		return 3;

	KeyField * kf = keyFieldAddr(ioState->node, ioState->slot);
	ioState->isDup = (kf->type != KeyField::kData);
	ioState->dupNode = NULL;
	return 0;	
}


NewtonErr
CSoupIndex::add(SKey * inKey, SKey * inData)
{
	NewtonErr err;
	kfAssembleKeyField(fKeyField, inKey, inData);
	newton_try
	{
		err = _BTEnterKey(fKeyField);
	}
	newton_catch_all
	{
		err = (NewtonErr)(long)CurrentException()->data;
	}
	end_try;
	if (err == noErr)
		fCache->commit(this);
	else
		fCache->abort(this);
	return err;
}


NewtonErr
CSoupIndex::addInTransaction(SKey * inKey, SKey * inData)
{
	NewtonErr err;
	kfAssembleKeyField(fKeyField, inKey, inData);
	err = _BTEnterKey(fKeyField);
	fCache->flush(this);
	return err;
}


NewtonErr
CSoupIndex::Delete(SKey * inKey, SKey * inData)
{
	NewtonErr err;
	kfAssembleKeyField(fKeyField, inKey, inData);
	newton_try
	{
		fSavedKey->length = 0;
		err = _BTRemoveKey(fKeyField);
	}
	newton_catch_all
	{
		err = (NewtonErr)(long)CurrentException()->data;
	}
	end_try;
	if (err == noErr)
		fCache->commit(this);
	else
		fCache->abort(this);
	return err;
}


int
CSoupIndex::search(int inArg1, SKey * inKey, SKey * inData, StopProcPtr inStopProc, void * ioRefCon, SKey * outKey, SKey * outData)
{
	int status;
	KeyField kf;
	if (inKey == NULL)
	{
		SKey spm00;
		SKey spm50;
		if ((status = first(&spm50, &spm00)) != 0)
			return status;
		if (inStopProc(&spm50, &spm00, ioRefCon) != 0)
			return 0;
		kfAssembleKeyField(&kf, &spm50, &spm00);
	}
	else
		kfAssembleKeyField(&kf, inKey, inData);

	newton_try
	{
		IndexState state;
		status = moveAndGetState(inArg1, 0, &kf, &state);	// sic -- looks like int/bool confusion
		while (status == 0
		&& inStopProc(kf.key(), (SKey *)kfFirstDataAddr(&kf), ioRefCon) == 0)
		{
			status = fCache->flush(this) ? moveAndGetState(inArg1, 0, &kf, &state)
												  : moveUsingState(inArg1, 0, &kf, &state);
		}
		if (status == 0)
			kfDisassembleKeyField(fKeyField, outKey, outData);
	}
	cleanup
	{
		fCache->abort(this);
	}
	end_try;

	fCache->commit(this);
	return status;
}


int
CSoupIndex::find(SKey * inKey, SKey * outKey, SKey * outData, bool inArg4)
{
	if (fInfo.rootNodeId == 0)		// no IndexInfo
		return 3;

	kfAssembleKeyField(fKeyField, inKey, NULL);

	int keyStrLen;		// r9
	if (fSortingTable != NULL && !inArg4 && fInfo.keyType == kKeyTypeString)	// key is a string
	{
		keyStrLen = inKey->size() / sizeof(UniChar);
		fSortingTable->convertTextToLowestSort((UniChar *)(fKeyField->key()), keyStrLen);
	}
	else
		keyStrLen = 0;

	int status = 0;		// r7
	bool failed = false;		// r10
	newton_try
	{
		int slot;
		NodeHeader * theNode;
		if ((theNode = readRootNode(false)) != NULL)
		{
			if (search(fKeyField, &theNode, &slot) == 0)
				status = fKeyField->length == 0 ? 3 : 2;	// not found
			fCache->commit(this);
		}
		else
			status = 2;
	}
	newton_catch_all
	{
		failed = true;
		fCache->abort(this);
		status = (long)CurrentException()->data;;
	}
	end_try;

	if (!failed)
	{
		if (keyStrLen != 0 && status == 2)
		{
			if (CompareUnicodeText(	(const UniChar *)inKey->data(), keyStrLen,
											(const UniChar *)fKeyField->buf + SKey::kOffsetToData, kfSizeOfKey(fKeyField->buf)/sizeof(UniChar)-1,
											fSortingTable) == 0)
				status = 0;
		}
		if (status == 0 || status == 2)
			kfDisassembleKeyField(fKeyField, outKey, outData);
	}

	return status;
}


int
CSoupIndex::first(SKey * outKey, SKey * outData)
{
	int status;
	bool failed = false;
	newton_try
	{
		NodeHeader * theNode;
		if ((theNode = readRootNode(false)) != NULL)
		{
			if (findFirstKey(theNode, fKeyField))
				status = 0;
			else
				status = 2;
		}
		else
			status = 2;
	}
	newton_catch_all
	{
		failed = true;
		fCache->abort(this);
		status = (long)CurrentException()->data;;
	}
	end_try;

	if (!failed)
	{
		fCache->commit(this);
		if (status == 0)
			kfDisassembleKeyField(fKeyField, outKey, outData);
	}

	return status;
}


int
CSoupIndex::last(SKey * outKey, SKey * outData)
{
	int status;
	bool failed = false;
	newton_try
	{
		NodeHeader * theNode;
		if ((theNode = readRootNode(false)) != NULL)
		{
			if (findLastKey(theNode, fKeyField))
				status = 0;
			else
				status = 2;
		}
		else
			status = 2;
	}
	newton_catch_all
	{
		failed = true;
		fCache->abort(this);
		status = (long)CurrentException()->data;;
	}
	end_try;

	if (!failed)
	{
		fCache->commit(this);
		if (status == 0)
			kfDisassembleKeyField(fKeyField, outKey, outData);
	}

	return status;
}


int
CSoupIndex::next(SKey * inKey, SKey * inData, int inArg3, SKey * outKey, SKey * outData)
{
	kfAssembleKeyField(fKeyField, inKey, inData);

	int status;
	bool failed = false;
	newton_try
	{
		if (inArg3 == 1
		|| ((status = _BTGetNextDupKey(fKeyField)) != 0 && status != 2 && inArg3 != 2))
			status = _BTGetNextKey(fKeyField);
	}
	newton_catch_all
	{
		failed = true;
		fCache->abort(this);
		status = (long)CurrentException()->data;;
	}
	end_try;

	if (!failed)
	{
		fCache->commit(this);
		if (status == 0)
			kfDisassembleKeyField(fKeyField, outKey, outData);
	}

	return status;
}


int
CSoupIndex::prior(SKey * inKey, SKey * inData, int inArg3, SKey * outKey, SKey * outData)
{
	kfAssembleKeyField(fKeyField, inKey, inData);

	int status;
	bool failed = false;
	newton_try
	{
		if (inArg3 == 0
		|| (status = _BTGetPriorDupKey(fKeyField)) != 0)
			status = _BTGetPriorKey(fKeyField);
	}
	newton_catch_all
	{
		failed = true;
		fCache->abort(this);
		status = (long)CurrentException()->data;;
	}
	end_try;

	if (!failed)
	{
		fCache->commit(this);
		if (status == 0)
			kfDisassembleKeyField(fKeyField, outKey, outData);
	}

	return status;
}


#pragma mark Size

void
CSoupIndex::nodeSize(NodeHeader * inNode, size_t & ioSize)
{
	size_t theSize;
	OSERRIF(store()->getObjectSize(inNode->id, &theSize));
	ioSize += theSize;
	for (int slot = 0; slot < inNode->numOfSlots; slot++)
	{
		PSSId lNode;
		KeyField * kf = keyFieldAddr(inNode, slot);
		if (kf->type == KeyField::kDupData && kf->length != 0)
			dupNodeSize(kf, ioSize);
		if ((lNode = leftNodeNo(inNode, slot)) != 0)
			nodeSize(readANode(lNode, inNode->nextId), ioSize);
	}
	fCache->forgetNode(inNode->id);
}


void
CSoupIndex::dupNodeSize(KeyField * inField, size_t & ioSize)
{
	PSSId dupId = kfNextDupId(inField);
	while (dupId != 0)
	{
		size_t theSize;
		DupNodeHeader * dupNode = readADupNode(dupId);
		dupId = dupNode->nextId;
		OSERRIF(store()->getObjectSize(dupNode->id, &theSize));
		ioSize += theSize;
		fCache->forgetNode(dupNode->id);
	}
}


size_t
CSoupIndex::totalSize(void)
{
	size_t theSize;
	OSERRIF(store()->getObjectSize(fInfoId, &theSize));
	if (fInfo.rootNodeId != 0)
	{
		newton_try
		{
			NodeHeader * theNode;
			if ((theNode = readRootNode(false)) != NULL)
				nodeSize(theNode, theSize);
			setRootNode(0);
		}
		cleanup
		{
			fCache->abort(this);
		}
		end_try;
	}
	fCache->commit(this);
	return theSize;
}

#pragma mark -

/*------------------------------------------------------------------------------
	U n i o n I n d e x D a t a
------------------------------------------------------------------------------*/

UnionIndexData::UnionIndexData()
{
	validity = kIsInvalid;
	cacheState = kIsInvalid;
	kf = &keyField;
}


UnionIndexData::~UnionIndexData()
{ }

#pragma mark -

/*------------------------------------------------------------------------------
	C U n i o n S o u p I n d e x
------------------------------------------------------------------------------*/

CUnionSoupIndex::CUnionSoupIndex(ArrayIndex inNumOfIndexes, UnionIndexData * indexes)
{
	fNumOfSoupsInUnion = inNumOfIndexes;
	fIndexData = indexes;
	fSeqInUnion = 0;
}


CUnionSoupIndex::~CUnionSoupIndex()
{
	delete[] fIndexData;	// sic -- we didn’t alloc them (that’s done in CCursor::createIndexes), but we’re deleting them
}


int
CUnionSoupIndex::find(SKey * inKey, SKey * outKey, SKey * outData, bool inArg4)
{
	if (fNumOfSoupsInUnion == 1)
		return fIndexData[0].index->find(inKey, outKey, outData, inArg4);

	SKey spA4 = *inKey;
	SKey sp54;
	SKey sp04;
	int  hasAKey = 0;
	int  status = 3;
	for (ArrayIndex i = 0; i < fNumOfSoupsInUnion; ++i)
	{
		CSoupIndex * index = fIndexData[i].index;

		int result = index->find(&spA4, &sp54, &sp04, inArg4);
		if (((result == 0 && ((index->fSortingTable != NULL && !inArg4) || status == 2 || status == 3))
		   || result == 2)
		&&  (hasAKey == 0 || index->compareKeys(sp54, *outKey) < 0))
		{
			status = result;
			memmove(outKey, &sp54, index->kfSizeOfKey(&sp54));
			if (outData)
				memmove(outData, &sp04, index->kfSizeOfData(&sp04));
			setCurrentSoup(i);
			hasAKey = 1;
		}
	}

	return status;
}


int
CUnionSoupIndex::first(SKey * outKey, SKey * outData)
{
	if (fNumOfSoupsInUnion == 1)
		return fIndexData[0].index->first(outKey, outData);

	SKey sp50;
	SKey sp00;
	int  hasAKey = 0;
	int  status = 2;
	for (ArrayIndex i = 0; i < fNumOfSoupsInUnion; ++i)
	{
		CSoupIndex * index = fIndexData[i].index;

		int result = index->first(&sp50, &sp00);
		if (result == 0)
		{
			status = result;
			if (hasAKey == 0 || index->compareKeys(sp50, *outKey) < 0)
			{
				memmove(outKey, &sp50, index->kfSizeOfKey(&sp50));
				memmove(outData, &sp00, index->kfSizeOfData(&sp00));
				setCurrentSoup(i);
				hasAKey = 1;
			}
		}
	}

	return status;
}


int
CUnionSoupIndex::last(SKey * outKey, SKey * outData)
{
	if (fNumOfSoupsInUnion == 1)
		return fIndexData[0].index->last(outKey, outData);

	SKey sp50;
	SKey sp00;
	int  hasAKey = 0;
	int  status = 2;
	for (ArrayIndex i = 0; i < fNumOfSoupsInUnion; ++i)
	{
		CSoupIndex * index = fIndexData[i].index;

		int result = index->last(&sp50, &sp00);
		if (result == 0)
		{
			status = result;
			if (hasAKey == 0 || index->compareKeys(sp50, *outKey) < 0)
			{
				memmove(outKey, &sp50, index->kfSizeOfKey(&sp50));
				memmove(outData, &sp00, index->kfSizeOfData(&sp00));
				setCurrentSoup(i);
				hasAKey = 1;
			}
		}
	}

	return status;
}


int
CUnionSoupIndex::next(SKey * inKey, SKey * inData, int inArg3, SKey * outKey, SKey * outData)
{
	return search(1, inKey, inData, NULL, NULL, outKey, outData, inArg3);
}


int
CUnionSoupIndex::prior(SKey * inKey, SKey * inData, int inArg3, SKey * outKey, SKey * outData)
{
	return search(0, inKey, inData, NULL, NULL, outKey, outData, inArg3);
}


int
CUnionSoupIndex::search(bool inDoForward, SKey * inKey, SKey * inData, StopProcPtr inStopProc, void * ioRefCon, SKey * outKey, SKey * outData, int inArg8)
{
	int status;
	UnionIndexData * unionInfo;	// r6
	CSoupIndex * soupIndex;			// r7

	if (fIsForwardSearch != inDoForward
	||  !isValidState(inKey, inData))
	{
		// state has changed since last search -- have to reset our index cache
		
		for (int i = fNumOfSoupsInUnion - 1; i >= 0; --i)
		{
			unionInfo = fIndexData + i;
			unionInfo->validity = kIsInvalid;
			unionInfo->cacheState = unionInfo->index->nodeCache()->modCount();
		}
	}

	fIsForwardSearch = inDoForward;
	unionInfo = fIndexData + fSeqInUnion;
	soupIndex = unionInfo->index;

	soupIndex->kfAssembleKeyField(unionInfo->kf, inKey, inData);

	newton_try
	{
		for (;;)
		{
			bool doMoveOn = true;
			status = (unionInfo->validity != kIsValid) ? soupIndex->moveAndGetState(inDoForward, inArg8, unionInfo->kf, &unionInfo->state)
																	 : soupIndex->moveUsingState(inDoForward, inArg8, unionInfo->kf, &unionInfo->state);
			if (status == 0)
			{
				unionInfo->validity = kIsValid;
				if (fNumOfSoupsInUnion == 1
				||  soupIndex->compareKeys(*inKey, *unionInfo->kf->key()) == 0)
					doMoveOn = false;
			}
			else if (status == 3)
			{
				unionInfo->validity = kIsBad;
			}
			else
			{
				unionInfo->validity = kIsInvalid;
				break;
			}

			if (doMoveOn)
			{
				if ((status = moveToNextSoup(inDoForward, inArg8, inKey, false)) != 0)
					break;
				unionInfo = fIndexData + fSeqInUnion;
				soupIndex = unionInfo->index;
			}

			if (inStopProc == NULL
			|| inStopProc(unionInfo->kf->key(), (SKey *)soupIndex->kfFirstDataAddr(unionInfo->kf), ioRefCon) != 0)
				break;

			if (soupIndex->fCache->flush(soupIndex))
				unionInfo->validity = kIsInvalid;
		}
	}
	cleanup
	{
		commit();
	}
	end_try;

	commit();
	if (status == 0)
		soupIndex->kfDisassembleKeyField(unionInfo->kf, outKey, outData);
	
	return status;
}


int
CUnionSoupIndex::currentSoupGone(SKey * inKey, SKey * outKey, SKey * outData)
{
	int status;
	newton_try
	{
		if ((status = moveToNextSoup(1, 0, inKey, 1)) == noErr)
		{
			UnionIndexData * unionInfo = fIndexData + fSeqInUnion;
			CSoupIndex * index = unionInfo->index;

			index->kfDisassembleKeyField(unionInfo->kf, outKey, outData);
		}
		else
			setCurrentSoup(0);
	}
	newton_catch_all
	{
		status = (NewtonErr)(long)CurrentException()->data;
	}
	end_try;
	return status;
}


int
CUnionSoupIndex::moveToNextSoup(bool inDoForward, int inArg2, SKey * inKey, bool inDoRebuildCurrentSoupInfo)
{
	UnionIndexData * currentInfo;	// r7
	int seq;				// r6
	int count;		// r10, r0

	if (inDoRebuildCurrentSoupInfo)
	{
		seq = fSeqInUnion - 1;
		currentInfo = NULL;
		count = fNumOfSoupsInUnion;
	}
	else
	{
		seq = fSeqInUnion;
		currentInfo = fIndexData + fSeqInUnion;
		count = fNumOfSoupsInUnion - 1;
	}

	bool isWrappedAround = false;
	for (int i = count - 1; i >= 0; --i)
	{
		if (++seq == fNumOfSoupsInUnion)
		{
			seq = 0;
			isWrappedAround = true;
		}

		UnionIndexData * unionInfo = fIndexData + seq;	// r5
		CSoupIndex * index = unionInfo->index;

		int status;
		if (unionInfo->validity != kIsBad)
		{
			if (unionInfo->validity != kIsValid)
			{
				index->kfAssembleKeyField(unionInfo->kf, inKey, NULL);
				if (inDoForward)
				{
					status = index->findAndGetState(unionInfo->kf, &unionInfo->state);
					if (status == 0)
					{
						if (isWrappedAround || inArg2 == 1)
							status = index->moveUsingState(inDoForward, 1, unionInfo->kf, &unionInfo->state);
					}
				}
				else
				{
					status = index->findPriorAndGetState(unionInfo->kf, isWrappedAround || inArg2 == 1, &unionInfo->state);
				}

				if (status == 3)
				{
					unionInfo->validity = kIsBad;
					continue;
				}
				else if (status == 0 || status == 2)
					unionInfo->validity = kIsValid;
				else
					return status;
			}

			bool doUpdate;
			if (currentInfo != NULL && currentInfo->validity == kIsValid)
			{
				int cmp = index->compareKeys(*unionInfo->kf->key(), *currentInfo->kf->key());
				if (inDoForward)
				{
					if (cmp > 0)
						doUpdate = true;
					else if (cmp != 0)
						doUpdate = false;
					else if (seq > fSeqInUnion)
						doUpdate = true;
					else
						doUpdate = false;
				}
				else
				{
					if (cmp < 0)
						doUpdate = true;
					else if (cmp != 0)
						doUpdate = false;
					else if (seq < fSeqInUnion)
						doUpdate = true;
					else
						doUpdate = false;
				}
			}
			else
				doUpdate = true;
			if (doUpdate)
			{
				currentInfo = unionInfo;
				fSeqInUnion = seq;
			}
		}
	}

	if (currentInfo != NULL && currentInfo->validity == kIsValid)
		return 0;

	return 3;
}


void
CUnionSoupIndex::setCurrentSoup(int index)
{
	fSeqInUnion = index;
	invalidateState();
}

void
CUnionSoupIndex::invalidateState(void)
{
	for (int i = fNumOfSoupsInUnion - 1; i >= 0; --i)
		fIndexData[i].validity = kIsInvalid;
}


bool
CUnionSoupIndex::isValidState(SKey * inKey, SKey * inData)
{
	for (int i = fNumOfSoupsInUnion - 1; i >= 0; --i)
	{
		UnionIndexData * unionInfo = fIndexData + i;
		CSoupIndex * index = unionInfo->index;

		if (index->nodeCache()->modCount() != unionInfo->cacheState)
			return false;

		if (fSeqInUnion == i)
		{
			void * data;
			size_t dataSize, keySize;

			if (unionInfo->validity != kIsValid)
				return false;

			data = index->kfFirstDataAddr(unionInfo->kf);
			dataSize = index->kfSizeOfData(inData);
			if (dataSize != index->kfSizeOfData(data))
				return false;

			if (memcmp(data, inData, dataSize) != 0)
				return false;

			keySize = index->kfSizeOfKey(inKey);
			if (keySize != index->kfSizeOfKey(unionInfo->kf->key()))
				return false;

			if (memcmp(unionInfo->kf->key(), inKey, keySize) != 0)
				return false;

		}
		else
		{
			if (unionInfo->validity == kIsValid)
				index->fCache->reuse(index);
		}
	}
	return true;
}

void
CUnionSoupIndex::commit(void)
{
	for (int i = fNumOfSoupsInUnion - 1; i >= 0; --i)
	{
		UnionIndexData * unionInfo = fIndexData + i;
		CSoupIndex * index = unionInfo->index;

		index->fCache->commit(index);
	}
}

#pragma mark -

/*------------------------------------------------------------------------------
	I n d e x e s
------------------------------------------------------------------------------*/

void
AlterIndexes(bool inAdd, RefArg inSoup, RefArg inEntry, PSSId inId)
{
	RefVar spec(GetFrameSlot(inSoup, SYMA(_proto)));
	RefVar indexes(GetFrameSlot(spec, SYMA(indexes)));
	RefVar indexDesc;
	for (int count = Length(indexes), i = count - 1; i >= 0; --i)
	{
		indexDesc = GetArraySlot(indexes, i);
		CSoupIndex * soupIndex = GetSoupIndexObject(inSoup, RINT(GetFrameSlot(indexDesc, SYMA(index))));
		if (EQ(GetFrameSlot(indexDesc, SYMA(type)), SYMA(tags)))
		{
			RefVar sp04(GetEntryKey(inEntry, GetFrameSlot(indexDesc, SYMA(path))));
			if (NOTNIL(sp04))
				AlterTagsIndex(inAdd, *soupIndex, inId, sp04, inSoup, GetFrameSlot(indexDesc, SYMA(tags)));
		}
		else
		{
			NewtonErr err;
			SKey theKey;
			if (GetEntrySKey(inEntry, indexDesc, &theKey, NULL))
			{
				if (inAdd)
					err = soupIndex->add(&theKey, (SKey *)&inId);
				else
					err = soupIndex->Delete(&theKey, (SKey *)&inId);
				if (err != noErr)
					ThrowErr(exStore, kNSErrInternalError);
			}
		}
	}
}


bool
PathsEqual(RefArg inPath1, RefArg inPath2)
{
	if (EQ(inPath1, inPath2))
		return true;

	ULong tag;
	if ((tag = RTAG(ObjectFlags(inPath1))) != RTAG(ObjectFlags(inPath2)))
		return false;
	if (NOTREALPTR(tag))
		return false;

	ArrayIndex count;
	if ((count = Length(inPath1)) != Length(inPath2))
		return false;
	for (ArrayIndex i = 0; i < count; ++i)
		if (!EQ(GetArraySlot(inPath1, i), GetArraySlot(inPath2, i)))
			return false;
	if (!EQ(ClassOf(inPath1), ClassOf(inPath2)))
		return false;
	return true;
}


bool
IndexPathsEqual(RefArg inPath1, RefArg inPath2)
{
	int i, count1, count2;

	if (EQ(ClassOf(inPath1), SYMA(array)))
		count1 = Length(inPath1);
	else
		count1 = 0;

	if (EQ(ClassOf(inPath2), SYMA(array)))
	{
		count2 = Length(inPath2);
		if (count1 == count2)
		{
			for (i = count1 - 1; i >= 0; --i)
			{
				if (!PathsEqual(GetArraySlot(inPath1, i), GetArraySlot(inPath2, i)))
					return false;
			}
			return true;
		}
	}
	else
	{
		if (count1 == 0)
			return PathsEqual(inPath1, inPath2);
	}
	
	return false;
}


Ref
IndexPathToIndexDesc(RefArg inSpec, RefArg inPath, int * outIndex)
{
	RefVar indexes(GetFrameSlot(inSpec, SYMA(indexes)));
	RefVar indexDesc;
	for (int count = Length(indexes), i = count - 1; i >= 0; --i)
	{
		indexDesc = GetArraySlot(indexes, i);
		if (IndexPathsEqual(inPath, GetFrameSlot(indexDesc, SYMA(path))))
		{
			if (outIndex)
				*outIndex = i;
			return indexDesc;
		}
	}
	return NILREF;
}


bool
UpdateIndexes(RefArg inSoup, RefArg inArg2, RefArg inArg3, PSSId inId, bool * outArg5)
{
	RefVar spec(GetFrameSlot(inSoup, SYMA(_proto)));
	RefVar indexes(GetFrameSlot(spec, SYMA(indexes)));	// r4
	//sp-A4
	RefVar indexDesc;		// spA0
	RefVar indexType;		// r5
	SKey sp50;
	SKey sp00;
	bool isChanged = false;	// r10
	for (int count = Length(indexes), i = count - 1; i >= 0; --i)
	{
		indexDesc = GetArraySlot(indexes, i);
		indexType = GetFrameSlot(indexDesc, SYMA(type));
		if (EQ(indexType, SYMA(tags)))
		{
			if (outArg5)
				*outArg5 = UpdateTagsIndex(inSoup, indexDesc, inArg3, inArg2, inId);
		}
		else
		{
			//sp-04
			bool isComplex;
			bool r8 = GetEntrySKey(inArg2, indexDesc, &sp50, &isComplex);		// add
			bool r6 = GetEntrySKey(inArg3, indexDesc, &sp00, NULL);				// delete
			if (!r8 && !r6)
				break;
			if (r8 && r6)
			{
				if (isComplex)
				{
					if (sp50.equals(sp00))
						break;
				}
				else
				{
					if (EQ(indexType, SYMA(real))
					&& (double) sp50 == (double) sp00)
						break;
					else if ((int) sp50 == (int) sp00)
						break;
				}
			}

			isChanged = true;
			NewtonErr err = noErr;
			XTRY
			{
				CSoupIndex * soupIndex = GetSoupIndexObject(inSoup, RINT(GetFrameSlot(indexDesc, SYMA(index))));	// r9
				if (r6)
					XFAIL(err = soupIndex->Delete(&sp00, (SKey *)&inId))
				if (r8)
					XFAIL(err = soupIndex->add(&sp50, (SKey *)&inId))
			}
			XENDTRY;
			XDOFAIL(err)
			{
				ThrowErr(exStore, kNSErrInternalError);
			}
			XENDFAIL;
		}
	}

	return isChanged;
}


bool
CompareSoupIndexes(RefArg inSpec1, RefArg inSpec2)
{
	RefVar indexes(GetFrameSlot(inSpec1, SYMA(indexes)));
	RefVar indexDesc;
	int count = Length(indexes);
	if (count != Length(GetFrameSlot(inSpec2, SYMA(indexes))))
		return false;
	for (int i = count - 1; i >= 0; --i)
	{
		indexDesc = GetArraySlot(indexes, i);
		if (ISNIL(IndexPathToIndexDesc(inSpec2, GetFrameSlot(indexDesc, SYMA(path)), NULL)))
			return false;
	}
	return true;
}


struct CopyIndexParms
{
	CSoupIndex *	soupIndex;
	ArrayIndex		idMapLen;
	PSSIdMapping *	idMap;
	bool				isTagsIndex;
	RefStruct		cbCodeblock;
	Timeout			cbInterval;
	CTime				cbTime;
	int				cbCount;
};

int
ComparePSSIdMapping(const void * inId1, const void * inId2)
{
	PSSId id1 = *(PSSId *)inId1;
	PSSId id2 = *(PSSId *)inId2;
	if (id1 > id2)
		return 1;
	if (id1 < id2)
		return -1;
	return 0;
}

int
CopyIndexStopFn(SKey * inFromKey, SKey * inToKey, void * inParms)
{
	CopyIndexParms * parms = (CopyIndexParms *)inParms;	// r6
	int theKey = (parms->isTagsIndex) ? (int) *inFromKey : (int) *inToKey;	// sp00
	PSSIdMapping * idMap = (PSSIdMapping *)bsearch(&theKey, parms->idMap, parms->idMapLen, sizeof(PSSIdMapping), ComparePSSIdMapping);
	SKey * srcKey = (parms->isTagsIndex) ? (SKey *)(&idMap->id2) : inToKey;
	SKey * dstKey = (parms->isTagsIndex) ? inFromKey : (SKey *)(&idMap->id2);
	parms->soupIndex->addInTransaction(srcKey, dstKey);

	int result = 0;
	if (parms->cbInterval && --parms->cbCount == 0)
	{
		// do progress callback
		if ((Timeout)(GetGlobalTime() - parms->cbTime) > parms->cbInterval)
		{
			DoBlock(parms->cbCodeblock, RA(NILREF));
			parms->cbTime = GetGlobalTime();
			parms->cbCount = 100;
		}
	}
	return result;
}


void
CopySoupIndexes(RefArg inFromSoup, RefArg inToSoup, PSSIdMapping * inIdMap, ArrayIndex inIdMapLen, RefArg inCallbackFn, Timeout inCallbackInterval)
{
	RefVar srcSpec(GetFrameSlot(inFromSoup, SYMA(_proto)));	//sp28
	RefVar dstSpec(GetFrameSlot(inToSoup, SYMA(_proto)));		//sp24
	RefVar srcIndexes(GetFrameSlot(srcSpec, SYMA(indexes)));	// r6
	RefVar dstIndexes(GetFrameSlot(dstSpec, SYMA(indexes)));	// r7
	RefVar indexDesc;	// r5

	CopyIndexParms parms;
	parms.idMap = inIdMap;
	parms.idMapLen = inIdMapLen;
	parms.cbInterval = inCallbackInterval;
	if (inCallbackInterval)
	{
		parms.cbCodeblock = inCallbackFn;
		parms.cbTime = GetGlobalTime();
		parms.cbCount = 100;
	}

	for (ArrayIndex i = 0, count = Length(srcIndexes); i < count; ++i)
	{
		indexDesc = GetArraySlot(dstIndexes, i);
		CSoupIndex * dstSoupIndex = GetSoupIndexObject(inToSoup, RINT(GetFrameSlot(indexDesc, SYMA(index))));
		indexDesc = GetArraySlot(srcIndexes, i);
		CSoupIndex * srcSoupIndex = GetSoupIndexObject(inFromSoup, RINT(GetFrameSlot(indexDesc, SYMA(index))));

		parms.soupIndex = dstSoupIndex;
		parms.isTagsIndex = EQ(GetFrameSlot(indexDesc, SYMA(type)), SYMA(tags));

		NewtonErr err;
		if ((err = srcSoupIndex->search(1, NULL, NULL, CopyIndexStopFn, &parms, NULL, NULL)) < noErr)
			ThrowErr(exStore, err);
		dstSoupIndex->nodeCache()->commit(dstSoupIndex);
	}
}


void
AbortSoupIndexes(RefArg inSoup)
{
	RefVar spec(GetFrameSlot(inSoup, SYMA(_proto)));
	RefVar indexes(GetFrameSlot(spec, SYMA(indexes)));
	RefVar indexDesc;
	for (int count = Length(indexes), i = count - 1; i >= 0; --i)
	{
		indexDesc = GetArraySlot(indexes, i);
		CSoupIndex * soupIndex = GetSoupIndexObject(inSoup, RINT(GetFrameSlot(indexDesc, SYMA(index))));
		soupIndex->storeAborted();
	}
}


Ref
NewIndexDesc(RefArg inSoup, RefArg inStore, RefArg indexSpec)
{
	//args: r6, r5, r4
	RefVar indexPath(GetFrameSlot(indexSpec, SYMA(path)));	// sp04
	if (NOTNIL(IndexPathToIndexDesc(inSoup, indexPath, NULL)))
		ThrowErr(exStore, kNSErrDuplicateIndex);

	//sp-04
	RefVar indexType(GetFrameSlot(indexSpec, SYMA(type)));	//sp00
	RefVar indexStructure(GetFrameSlot(indexSpec, SYMA(structure)));	// r8

	// validate
	bool isMultiSlot = false;	// r9
	if (EQ(indexStructure, SYMA(multiSlot)))
	{
		isMultiSlot = true;
		if (!(EQ(ClassOf(indexPath), SYMA(array)) && IsArray(indexType)))
			ThrowErr(exStore, kNSErrBadIndexDesc);
		if (Length(indexPath) != Length(indexType))
			ThrowErr(exStore, kNSErrBadIndexDesc);
		if (Length(indexPath) > 6)
			ThrowErr(exStore, kNSErrBadIndexDesc);
	}
	else
	{
		if (!EQ(indexStructure, SYMA(slot)))
			ThrowErr(exStore, kNSErrUnknownKeyStructure);
	}

	// sp-04
	RefVar indexDesc(TotalClone(indexSpec));	// sp00
	if (EQ(indexType, SYMA(tags)))
	{
		if (NOTNIL(GetTagsIndexDesc(inSoup)))
			ThrowErr(exStore, kNSErrDuplicateIndex);
		if (ISNIL(GetFrameSlot(indexDesc, SYMA(tags))))
			SetFrameSlot(indexDesc, SYMA(tags), MakeArray(0));
	}

	bool isStringIndex = EQ(indexType, SYMA(string)) || (isMultiSlot && ISINT(FSetContains(RA(NILREF), indexType, SYMA(string))));
	if (isStringIndex)
	{
		int sortTableId = 0;
		RefVar sortId(GetFrameSlot(indexDesc, SYMA(sortId)));
		if (ISNIL(sortId))
		{
			if ((sortTableId = DefaultSortTableId()) != 0)
				SetFrameSlot(indexDesc, SYMA(sortId), MAKEINT(sortTableId));
		}
		else
		{
			 if ((sortTableId = RINT(sortId)) == 0)
				RemoveSlot(indexDesc, SYMA(sortId));
		}
		if (sortTableId != 0)
			StoreSaveSortTable(inStore, sortTableId);
	}

	IndexInfo info;
	IndexDescToIndexInfo(indexDesc, &info);
	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inStore, SYMA(store));
	SetFrameSlot(indexDesc, SYMA(index), MAKEINT(CSoupIndex::create(storeWrapper, &info)));

	return indexDesc;
}


Ref
AddNewSoupIndexes(RefArg inSoup, RefArg inStore, RefArg inIndexes)
{
	RefVar indexes(MakeArray(0));
	SetFrameSlot(inSoup, SYMA(indexes), indexes);

	RefVar indexDesc(NewIndexDesc(inSoup, inStore, RA(indexDescPrototype)));
	AddArraySlot(indexes, indexDesc);
	if (NOTNIL(inIndexes))
	{
		for (ArrayIndex i = 0, count = Length(inIndexes); i < count; ++i)
		{
			indexDesc = NewIndexDesc(inSoup, inStore, GetArraySlot(inIndexes, i));
			AddArraySlot(indexes, indexDesc);
		}
	}
	
	return indexes;
}


void
IndexDescToIndexInfo(RefArg indexDesc, IndexInfo * outInfo)
{
	outInfo->dataType = kDataTypeNormal;
	outInfo->x10 = EQ(GetFrameSlot(indexDesc, SYMA(path)), SYMA(_uniqueId)) ? 0 : 2;
	outInfo->x14 = 0xFFFFFFFF;
	outInfo->x18 = 0xFF;
	outInfo->x19 = 0;
	outInfo->x1A = 0;

	ULong theType;
	RefVar indexType(GetFrameSlot(indexDesc, SYMA(type)));
	RefVar indexOrder(GetFrameSlot(indexDesc, SYMA(order)));
	int count;
	bool isAnArray = IsArray(indexType);
	if (isAnArray)
	{
		count = Length(indexType);
		outInfo->keyType = kKeyTypeMultislot;
	}
	else
	{
		count = 1;
		if (NOTNIL(indexOrder) && !EQ(indexOrder, SYMA(ascending)))
			outInfo->x19 = 1;
	}
	RefVar spec;
	for (int i = count - 1; i >= 0; --i)
	{
		spec = isAnArray ? GetArraySlot(indexType, i) : (Ref)indexType;
		if (EQ(spec, SYMA(string)))
			theType = kKeyTypeString;
		else if (EQ(spec, SYMA(int)))
			theType = kKeyTypeInt;
		else if (EQ(spec, SYMA(real)))
			theType = kKeyTypeReal;
		else if (EQ(spec, SYMA(char)))
			theType = kKeyTypeChar;
		else if (EQ(spec, SYMA(symbol)))
			theType = kKeyTypeSymbol;
		else if (!isAnArray && EQ(spec, SYMA(tags)))
		{
			theType = kKeyTypeInt;
			outInfo->dataType = kDataTypeTags;
			outInfo->x10 = 0;
		}
		else
			ThrowErr(exStore, kNSErrUnknownIndexType);

		if (isAnArray)
		{
			outInfo->x14 = (outInfo->x14 << 4) + theType;
			if (NOTNIL(indexOrder))
			{
				outInfo->x18 <<= 1;
				if (EQ(GetArraySlot(indexOrder, i), SYMA(ascending)))
					outInfo->x18 |= 1;
			}
		}
		else
			outInfo->keyType = theType;
	}
}


void
IndexEntries(RefArg inSoup, RefArg indexDesc)
{
	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inSoup, SYMA(TStore));	// r9
	RefVar tags;
	RefVar path;
	bool isTagSpec = EQ(GetFrameSlot(indexDesc, SYMA(type)), SYMA(tags));
	if (isTagSpec)
	{
		tags = GetFrameSlot(indexDesc, SYMA(tags));
		path = GetFrameSlot(indexDesc, SYMA(path));
	}
	//sp-54
	CSoupIndex * r7 = GetSoupIndexObject(inSoup, 0);
	CSoupIndex * r6 = GetSoupIndexObject(inSoup, RINT(GetFrameSlot(indexDesc, SYMA(index))));
	SKey entryKey;
	PSSId entryId;
		//sp-08
		RefVar sp0004(MakeFaultBlock(inSoup, storeWrapper, 0));
		RefVar sp0000;
	int status;
	for (status = r7->first(&entryKey, (SKey *)&entryId); status == 0; status = r7->next(&entryKey, (SKey *)&entryId, 0, &entryKey, (SKey *)&entryId))
	{
		FaultObject * obj = (FaultObject *)NoFaultObjectPtr(sp0004);
		obj->id = MAKEINT(entryId);
		obj->object = NILREF;
		if (isTagSpec)
		{
			sp0000 = GetEntryKey(sp0004, path);
			if (NOTNIL(sp0000))
				AlterTagsIndex(1, *r6, entryId, sp0000, inSoup, tags);
		}
		else
		{
			//sp-50
			SKey sp00000;
			if (GetEntrySKey(sp0004, indexDesc, &sp00000, NULL))
			{
				if (r6->addInTransaction(&sp00000, (SKey *)&entryId) != noErr)
					ThrowErr(exStore, kNSErrInternalError);
			}
		}
	}
	if (!isTagSpec)
		r6->nodeCache()->commit(r6);
}


void
CreateSoupIndexObjects(RefArg inSoup)
{
	RefVar spec(GetFrameSlot(inSoup, SYMA(_proto)));
	RefVar indexes(GetFrameSlot(spec, SYMA(indexes)));
	ArrayIndex count = Length(indexes);
	// allocate space for soup index objects
	RefVar indexObjs(AllocateFramesCObject(count * sizeof(CSoupIndex), GCDeleteIndexObjects, NULL, NULL));
	// initialize those soup index objects
	CSoupIndex * soupIndex = (CSoupIndex *)BinaryData(indexObjs);
	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inSoup, SYMA(TStore));
	RefVar indexDesc;
	for (ArrayIndex i = 0; i < count; ++i, ++soupIndex)
	{
		new (soupIndex) CSoupIndex;
		indexDesc = GetArraySlot(indexes, i);
		soupIndex->init(storeWrapper, RINT(GetFrameSlot(indexDesc, SYMA(index))), GetIndexSortTable(indexDesc));
	}
	// save it
	SetFrameSlot(inSoup, SYMA(indexObjects), indexObjs);
	// notify cursors of the change
	EachSoupCursorDo(inSoup, kSoupIndexesChanged);
}


void
GCDeleteIndexObjects(void *)
{ /* this really does nothing */ }


CSoupIndex *
GetSoupIndexObject(RefArg inSoup, PSSId inId)
{
	CSoupIndex * soupIndex = (CSoupIndex *)BinaryData(GetFrameSlot(inSoup, SYMA(indexObjects)));
	if (inId == 0)
		return soupIndex;	// all of them

	RefVar spec(GetFrameSlot(inSoup, SYMA(_proto)));
	RefVar indexes(GetFrameSlot(spec, SYMA(indexes)));
	CSoupIndex * limit = soupIndex + Length(indexes);
	for ( ; soupIndex < limit; soupIndex++)
		if (soupIndex->indexId() == inId)
			return soupIndex;	// with the specified id

	return NULL;
}
