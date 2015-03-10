/*
	File:		Indexes.cc

	Contains:	Soup index implementation.

	Written by:	Newton Research Group, 2007.
*/

#include "Objects.h"
#include "Globals.h"
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
inline void * operator new[](size_t, CSoupIndex * addr) { return addr; }


/*----------------------------------------------------------------------
	H a c k
	Shouldn’t need this when actual GetGlobalTime() is available.
----------------------------------------------------------------------*/
#include <mach/mach.h>
#include <mach/clock.h>
#include <mach/mach_error.h>
#include <stdio.h>
#include <time.h>

extern "C" CTime
GetGlobalTime(void)
{
	kern_return_t ret;
	clock_serv_t theClock;
	mach_timespec_t theTime;

	ret = host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &theClock);
	ret = clock_get_time(theClock, &theTime);

	CTime now(theTime.tv_sec, kSeconds);
	CTime plus(theTime.tv_nsec/1000, kMicroseconds);
	return now + plus;
}


/*----------------------------------------------------------------------
	S K e y
----------------------------------------------------------------------*/

void
SKey::set(size_t inSize, void * inKey)
{
	if (inSize > 78)
		inSize = 78;
	setSize(inSize);
	memmove(buf, inKey, inSize);
}


void
SKey::setFlags(unsigned char inFlags)
{
	flagBits = inFlags;
}


void
SKey::setMissingKey(int inBit)
{
	setFlags(flagBits | (1 << inBit));
}


void
SKey::setSize(unsigned short inSize)
{
	length = inSize;
}


SKey&
SKey::operator=(const SKey & inKey)
{
	memmove(this, &inKey, sizeof(SKey));
	return *this;
}


SKey&
SKey::operator=(long inKey)
{
	if (MISALIGNED(this))
		memmove(this, &inKey, sizeof(inKey));
	else
		*(long*)this = inKey;
	return *this;
}


SKey&
SKey::operator=(UniChar inKey)
{
	if (MISALIGNED(this))
		memmove(this, &inKey, sizeof(inKey));
	else
		*(UniChar*)this = inKey;
	return *this;
}

SKey&
SKey::operator=(const double inKey)
{
	if (MISALIGNED(this))
		memmove(this, &inKey, sizeof(inKey));
	else
		*(double*)this = inKey;
	return *this;
}


SKey::operator long() const
{
	if (MISALIGNED(this))
	{
		long value;
		memmove(&value, this, sizeof(value));
		return value;
	}
	return *(long*)this;
}


SKey::operator UniChar() const
{
	if (MISALIGNED(this))
	{
		UniChar value;
		memmove(&value, this, sizeof(value));
		return value;
	}
	return *(UniChar*)this;
}


SKey::operator double() const
{
	if (MISALIGNED(this))
	{
		double value;
		memmove(&value, this, sizeof(value));
		return value;
	}
	return *(double*)this;
}


BOOL
SKey::equals(const SKey & inKey) const
{
	return length == inKey.length
		 && flagBits == inKey.flagBits
		 && memcmp(buf, inKey.buf, length) == 0;
}


/*----------------------------------------------------------------------
	S K e y   F u n c t i o n s
----------------------------------------------------------------------*/

Ref
GetEntryKey(RefArg inEntry, RefArg inPath)
{
	if (EQRef(ClassOf(inPath), RSYMarray))
	{
		RefVar aKey;
		BOOL isKeyObjValid = NO;		// r10
		int i, count = Length(inPath);	// r7, r9
		RefVar keyObj(MakeArray(count));	// r8
		for (i = 0; i < count; i++)
		{
			aKey = GetEntryKey(inEntry, GetArraySlot(inPath, i));
			if (NOTNIL(aKey))
			{
				isKeyObjValid = YES;
				SetArraySlot(keyObj, i, aKey);
			}
		}
		if (isKeyObjValid)
			return keyObj;
	}

	else if (FrameHasPath(inEntry, inPath))
		return GetFramePath(inEntry, inPath);
		
	return NILREF;
}


BOOL
GetEntrySKey(RefArg inEntry, RefArg inQuerySpec, SKey * outKey, BOOL * outIsComplexKey)
{
	RefVar keyObj(GetEntryKey(inEntry, GetFrameSlot(inQuerySpec, SYMA(path))));
	if (NOTNIL(keyObj))
	{
		short keySize;
		KeyToSKey(keyObj, GetFrameSlot(inQuerySpec, SYMA(type)), outKey, &keySize, outIsComplexKey);
		return YES;
	}
	return NO;
}


Ref
SKeyToKey(const SKey & inKey, RefArg inType, short * outp)
{
	RefVar keyObj;
	short unwanted;
	short * keySize = (outp == NULL) ? &unwanted : outp;	// r6

	if (EQ(inType, SYMA(string)))
	{
		size_t keyLen = inKey.size();
		size_t keyStrLen = keyLen + sizeof(UniChar);
		keyObj = AllocateBinary(SYMA(string), keyStrLen);
		char * keyObjData = BinaryData(keyObj);
		memmove(keyObjData, inKey.data(), keyLen);
		*(UniChar *) (keyObjData + keyLen) = 0;	// terminate the string
		*keySize = keyStrLen;
	}

	else if (EQ(inType, SYMA(int)))
	{
		*keySize = sizeof(long);
		keyObj = MAKEINT((long)inKey);
	}

	else if (EQ(inType, SYMA(real)))
	{
		*keySize = sizeof(double);
		keyObj = MakeReal((double)inKey);
	}

	else if (EQ(inType, SYMA(char)))
	{
		*keySize = sizeof(UniChar);
		keyObj = MAKECHAR((UniChar)inKey);
	}

	else if (EQ(inType, SYMA(symbol)))
	{
		size_t keyLen = inKey.size();
		size_t keySymLen = keyLen + sizeof(char);
		char * keyObjData = (char *) malloc(keySymLen);
		memmove(keyObjData, inKey.data(), keyLen);
		*(keyObjData + keyLen) = 0;	// terminate the string
		keyObj = MakeSymbol(keyObjData);
		free(keyObjData);
		*keySize = keySymLen;
	}

	else if (IsArray(inType))
	{
		const void * aKey = inKey.data();	// r5
		const void * aKeyLimit = (char *) inKey.data() + inKey.size();	// sp00
		short aKeyLen;
		ULong flags = inKey.flags();	// r9
		int i, count = Length(inType);
		keyObj = MakeArray(count);
		for (i = 0; i < count; i++)
		{
			if (aKey == aKeyLimit)	// we’re full
				break;
			if ((flags & 0x01) == 0)
			{
				SetArraySlot(keyObj, i, SKeyToKey(*(const SKey *)aKey, GetArraySlot(inType, i), &aKeyLen));
				aKey = (char *) aKey + WORDALIGN(aKeyLen);
			}
			flags >>= 1;
		}
	}

	else
		throw2(exStore, kNSErrUnknownIndexType);

	return keyObj;
}


void
KeyToSKey(RefArg inKey, RefArg inType, SKey * outKey, short * outSize, BOOL * outIsComplexKey)
{
	BOOL isNonNumericKey = NO;	// assume it’s numeric

	if (IsArray(inType))
	{
		MultiKeyToSKey(inKey, inType, outKey);
		isNonNumericKey = YES;
	}

	else if ((NOTINT(inKey) || !EQ(inType, SYMA(real)))  &&  !IsInstance(inKey, inType))
		throw2(exStore, kNSErrKeyHasWrongType);

	else if (EQ(inType, SYMA(string)))
	{
		if (IsLargeBinary(inKey))
			throw2(exStore, kNSErrVBOKey);
		if (IsRichString(inKey))
		{
			RichStringToSKey(inKey, outKey);
		}
		else
		{
			UniChar * s = (UniChar *) BinaryData(inKey);
			outKey->set(Ustrlen(s) * sizeof(UniChar), s);
		}
		isNonNumericKey = YES;
	}

	else if (EQ(inType, SYMA(int)))
	{
		*(long *)outKey = (long) RINT(inKey);
		if (outSize != NULL)
			*outSize = sizeof(long);
	}

	else if (EQ(inType, SYMA(real)))
	{
		*(double *)outKey = (double) CoerceToDouble(inKey);
		if (outSize != NULL)
			*outSize = sizeof(double);
	}

	else if (EQ(inType, SYMA(char)))
	{
		*(UniChar *)outKey = (UniChar) MAKECHAR(inKey);
		if (outSize != NULL)
			*outSize = sizeof(UniChar);
	}

	else if (EQ(inType, SYMA(symbol)))
	{
		char * s = SymbolName(inKey);
		outKey->set(strlen(s), s);
		isNonNumericKey = YES;
	}

	else
		throw2(exStore, kNSErrUnknownIndexType);

	if (outIsComplexKey)
		*outIsComplexKey = isNonNumericKey;

	if (isNonNumericKey && outSize)
		*outSize = outKey->size() + 2;
}


void
MultiKeyToSKey(RefArg inKey, RefArg inType, SKey * outKey)
{
	outKey->setFlags(0);

	RefVar keyObj;
	int keyLen = 0;
	int nextKeyLen = 0;
	char * keyData = (char *) outKey->data();
	BOOL isKeyAnArray = IsArray(inKey);
	int i, count = isKeyAnArray ? Length(inKey) : 1;	// r7, r10
	for (i = 0; i < count; i++)
	{
		keyObj = isKeyAnArray ? GetArraySlot(inKey, i) : (Ref)inKey;
		if (NOTNIL(keyObj))
		{
			//sp-5C
			SKey aKey;	// sp0C
			short aKeySize;	// sp08
			BOOL isComplexKey;	// sp04
			KeyToSKey(keyObj, GetArraySlot(inType, i), &aKey, &aKeySize, &isComplexKey);		// no, no, no -- surely have to verify that inType is an array?
			nextKeyLen = keyLen + WORDALIGN(aKeySize);
			if (nextKeyLen > 78)
			{
				if (!isComplexKey)
					break;		// don’t need to bother with simple (numeric) keys
				aKeySize = 78 - keyLen;	// shorten the key to fit the available space
				if (aKeySize <= 2)
					break;	// no room for any more keys
				aKey.setSize(aKeySize - 2);
				nextKeyLen = 78;
			}
			memmove(keyData + keyLen, &aKey, aKeySize);
			if (ODD(aKeySize))
				keyData[keyLen + aKeySize] = 0;		// pad it to short
			keyLen = nextKeyLen;
		}
		else
			outKey->setMissingKey(i);
	}
	outKey->setSize(keyLen);
}


void
RichStringToSKey(RefArg inStr, SKey * outKey)
{
	size_t keyStrLen;
	UniChar * keyStr = (UniChar *) outKey->data();
	UniChar * s = (UniChar *) BinaryData(inStr);
	for (keyStrLen = 0; keyStrLen < 78; keyStrLen += sizeof(UniChar))
	{
		UniChar ch = *s++;
		if (ch == 0)
			break;
		if (ch == kInkChar)
			ch = kKeyInkChar;
		*keyStr++ = ch;
	}
	outKey->setSize(keyStrLen);
}


#pragma mark -

/*----------------------------------------------------------------------
	C A b s t r a c t S o u p I n d e x
----------------------------------------------------------------------*/

CAbstractSoupIndex::CAbstractSoupIndex()
{ }

CAbstractSoupIndex::~CAbstractSoupIndex()
{ }

int
CAbstractSoupIndex::findPrior(SKey * arg1, SKey * arg2, SKey * arg3, BOOL inArg4, BOOL inArg5)
{
	// this: r6  args: r1 r5 r4 r3 r7
	int secondaryResult;
	int result = find(arg1, arg2, arg3, inArg4);
	if (result == 0)
	{
		if (inArg5)
			result = prior(arg2, arg3, 0, arg2, arg3);
		else
		{
			if (next(arg2, arg3, 1, arg2, arg3) == 3)
				last(arg2, arg3);
			else
				prior(arg2, arg3, 0, arg2, arg3);
		}
	}
	else if (result == 2)
	{
		if ((secondaryResult = prior(arg2, arg3, 0, arg2, arg3)) != 0)
			result = secondaryResult;
	}
	else if (result == 3)
	{
		secondaryResult = last(arg2, arg3);
		if (secondaryResult == 0)
			result = 2;
		else if (secondaryResult != 2)
			result = secondaryResult;
	}
	return result;
}

#pragma mark -

/*----------------------------------------------------------------------
	C S o u p I n d e x
----------------------------------------------------------------------*/

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


PSSId
CSoupIndex::create(CStoreWrapper * inStoreWrapper, IndexInfo * info)
{
	PSSId theId;
	info->x00 = 0;
	info->x04 = 512;
	info->x1A = 0;
	OSERRIF(inStoreWrapper->store()->newObject(&theId, (char *)info, sizeof(IndexInfo)));
	return theId;
}


CSoupIndex::CSoupIndex()
{ }


CSoupIndex::~CSoupIndex()
{ }


void
CSoupIndex::init(CStoreWrapper * inStoreWrapper, PSSId inId, const CSortingTable * inSortingTable)
{
	f04 = inStoreWrapper;
	f0C = inId;
	f40 = inSortingTable;
	f08 = inStoreWrapper->nodeCache();
	OSERRIF(readInfo());
	f2C = 0;
	f38 = fKeyCompareFns[f10.x08];	// type
	f3C = fKeyCompareFns[f10.x0C];	// class
	f30 = fKeySizes[f10.x08];
	f34 = fKeySizes[f10.x0C];
}


NewtonErr
CSoupIndex::readInfo(void)
{
	NewtonErr err;
	XTRY
	{
		size_t infoSize;
		XFAIL(err = store()->getObjectSize(f0C, &infoSize))
		if (infoSize > sizeof(IndexInfo))
			infoSize = sizeof(IndexInfo);
		else
			f10.x19 = 0;	// sic -- but just gonna read right over the top of it..?
		err = store()->read(f0C, 0, &f10, infoSize);
	}
	XENDTRY;
	return err;
}

#pragma mark -

void
CSoupIndex::createFirstRoot(void)
{
	setRootNode(newNode()->x00);
}


void
CSoupIndex::setRootNode(PSSId inId)
{
	size_t theSize;
	OSERRIF(store()->getObjectSize(f0C, &theSize));
	if (theSize > sizeof(IndexInfo))
		theSize = sizeof(IndexInfo);
	OSERRIF(store()->write(f0C, 0, &f10, theSize));
}


NodeHeader *
CSoupIndex::newNode(void)
{
	PSSId nodeId;
	OSERRIF(store()->newObject(&nodeId, 0));
	NodeHeader * theNode = f08->rememberNode(this, nodeId, f10.x04, NO, YES);
	initNode(theNode, nodeId);
	return theNode;
}


DupNodeHeader *
CSoupIndex::newDupNode(void)
{
	PSSId nodeId;
	OSERRIF(store()->newObject(&nodeId, 0));
	DupNodeHeader * theNode = (DupNodeHeader *)f08->rememberNode(this, nodeId, f10.x04, YES, YES);
	theNode->x00 = nodeId;
	theNode->x04 = 0;
	theNode->x08 = f10.x04 - 16;
	theNode->x0A = 0;
	theNode->x0C = 16;
	theNode->x0E = 0;
	return theNode;
}


void
CSoupIndex::initNode(NodeHeader * ioNode, PSSId inId)
{
	ioNode->x00 = inId;
	ioNode->x04 = 0;
	ioNode->x08 = f10.x04 - 20;
	ioNode->x0A = 0;
	ioNode->x0C[0] = f10.x04 - 2;
	KeyField * kf = firstKeyField(ioNode);
	kf->flags = kf->length = 0;
	setNodeNo(ioNode, 0, 0);
}


void
CSoupIndex::setNodeNo(NodeHeader * inNode, long index, ULong inNodeNum)
{
	ULong * nodeNumPtr = ((ULong *)keyFieldAddr(inNode, index)) - 1;
	*nodeNumPtr = inNodeNum;	// need to handle misaligned pointer
#if 0
	sp00 = keyFieldAddr(inNode, inArg2) - 4;
	union { char ch[4]; ULong ul; } sp04;
	sp04.ul = inArg3;
	if ((sp00 & 0x03) != 0)
	{
		*((char *)sp00 + 0) = sp04.ch[0];	// byte endianness?
		*((char *)sp00 + 1) = sp04.ch[1];
		*((char *)sp00 + 2) = sp04.ch[2];
		*((char *)sp00 + 3) = sp04.ch[3];
	}
	else
		*sp00 = inArg3;
#endif
}


NodeHeader *
CSoupIndex::readRootNode(BOOL inCreate)
{
	if (f10.x00 == 0)
	{
		if (!inCreate)
			return NULL;
		createFirstRoot();
	}
	return readANode(f10.x00, 0);
}


NodeHeader *
CSoupIndex::readANode(PSSId inId, ULong inArg2)
{
	NodeHeader * theNode = f08->findNode(this, inId);
	if (theNode == NULL)
	{
		size_t itsSize;
		OSERRIF(store()->getObjectSize(inId, &itsSize));
		theNode = f08->rememberNode(this, inId, f10.x04, NO, NO);
		OSERRIF(store()->read(inId, 0, theNode, itsSize));
		short * r8 = &theNode->x0C[theNode->x0A + 1];
		size_t r1 = f10.x04 - theNode->x08;
		memmove(r8 + theNode->x08, r8, r1-((char *)r8-(char *)theNode));
		memset(r8, 0, theNode->x08);
	}

	theNode->x00 = inId;
	theNode->x04 = inArg2;
	return theNode;
}


DupNodeHeader *
CSoupIndex::readADupNode(PSSId inId)
{
	NodeHeader * theNode = f08->findNode(this, inId);
	if (theNode == NULL)
	{
		size_t itsSize;
		OSERRIF(store()->getObjectSize(inId, &itsSize));
		theNode = f08->rememberNode(this, inId, f10.x04, YES, NO);
		OSERRIF(store()->read(inId, 0, theNode, itsSize));
	}

	theNode->x00 = inId;
	return (DupNodeHeader *) theNode;
}


void
CSoupIndex::deleteNode(PSSId inId)
{
	f08->deleteNode(inId);
}


void
CSoupIndex::changeNode(NodeHeader * inNode)
{
	// this really does nothing
}


void
CSoupIndex::updateNode(NodeHeader * inNode)
{
	char buf[512];
	size_t r7 = bytesInNode(inNode);
	size_t r6 = inNode->x0A * sizeof(unsigned short) + 0x0E;
	memmove(buf, inNode, r6);
	memmove(buf+r6, inNode+r6+inNode->x08, r7-r6);
	OSERRIF(store()->replaceObject(inNode->x00, buf, r7));
}


void
CSoupIndex::updateDupNode(NodeHeader * inNode)
{
	OSERRIF(store()->replaceObject(inNode->x00, inNode, bytesInNode(inNode)));
}


void
CSoupIndex::freeNodes(NodeHeader * inNode)
{
	for (int i = 0; i < inNode->x0A; i++)
	{
		PSSId lNode;
		KeyField * kf = keyFieldAddr(inNode, i);
		if (kf->flags == 0x01 && kf->length != 0)
			freeDupNodes(kf);
		if ((lNode = leftNodeNo(inNode, i)) != 0)
			freeNodes(readANode(lNode, inNode->x00));
	}
	deleteNode(inNode->x00);
}


void
CSoupIndex::freeDupNodes(KeyField * inField)
{
	PSSId dupId = kfNextDupId(inField);
	while (dupId != 0)
	{
		DupNodeHeader * dupNode = readADupNode(dupId);
		deleteNode(dupNode->x00);
		dupId = dupNode->x04;
	}
}

#pragma mark -

void
CSoupIndex::storeAborted(void)
{
	f08->abort(this);
	readInfo();
}

void
CSoupIndex::destroy(void)
{
	if (f10.x00 != 0)
	{
		newton_try
		{
			NodeHeader * theNode;
			if ((theNode = readRootNode(NO)) != NULL)
				freeNodes(theNode);
			setRootNode(0);
		}
		cleanup
		{
			f08->abort(this);
		}
		end_try;
	}
	f08->commit(this);
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

	if ((KeyField::kOffsetToData + SKey::kOffsetToData + keyLen + SKey::kOffsetToData + dataLen) > sizeof(KeyField))
		throw2(exStore, kNSErrInternalError);

	outField->flags = 0;
	outField->length = KeyField::kOffsetToData + keyLen + dataLen;
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
	if (f30 != 0)
		return f30;
	return KeyField::kOffsetToData + ((KeyField *)inBuf)->length;
}


size_t
CSoupIndex::kfSizeOfData(void * inBuf)
{
	if (f34 != 0)
		return f34;
	return KeyField::kOffsetToData + ((KeyField *)inBuf)->length;
}


void *
CSoupIndex::kfFirstDataAddr(KeyField * inField)
{
	size_t keyLen = kfSizeOfKey(inField->buf);
	ALIGNkfSIZE(keyLen);
	return inField->buf + keyLen;
}


BOOL
CSoupIndex::kfNextDataAddr(KeyField * inField, void * inData, void ** outNextData)
{
	if (inData == NULL)
	{
		// find the first
		size_t keyLen = kfSizeOfKey(inField->buf);
		ALIGNkfSIZE(keyLen);
		*outNextData = inField->buf + keyLen;
		return YES;
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
	if (inField->flags == 1)
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
	if (inField->flags == 1)
	{
		char * nextDup = (char *)inField + inField->length;
		short * dupCount = (short *)nextDup - 3;
		*dupCount = inCount;
	}
}

ULong
CSoupIndex::kfNextDupId(KeyField * inField)
{
	if (inField->flags == 1)
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
	if (inField->flags == 1)
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
	if (ioField->flags == 1)
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
	if (ioField->flags == 1)
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
	if (ioField->flags == 0)
	{
		// it’s a singleton
		ioField->length = keySize + dataSize;				// update the size
		memmove(ioField + keySize, inData, dataSize);	// copy data into data space
	}
	else if (ioField->flags == 1)
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
		if ((this->*f3C)(*(const SKey *)inData, *(const SKey *)data) == 0)
			return data;
		if (outData)
			*outData = data;
	}
	return NULL;
}

void
CSoupIndex::kfConvertKeyField(long inArg1, KeyField * inField)
{
	//size_t theSize = kfSizeOfKey(inField->buf);	// unused
	if (inField->flags == 0 && inArg1 == 1)
	{
		inField->flags = 1;
		inField->length += 6;	// 3 * sizeof(unsigned short) ?
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


BOOL
CSoupIndex::nextDupDataAddr(DupNodeHeader * inDupNode, void * inData, void ** outData)
{
	if (inData == NULL)
	{
		*outData = firstDupDataAddr(inDupNode);
		return YES;
	}
	char * limit = ((char *)inDupNode) + inDupNode->x0C;
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
	while ((dupId = dupNode->x04) != 0)
		dupNode = readADupNode(dupId);
	void * lastAddr;
	char * anAddr = (char *)firstDupDataAddr(dupNode);
	char * limit = ((char *)dupNode) + dupNode->x0C;
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


BOOL
CSoupIndex::appendDupData(DupNodeHeader * inDupNode, void * inData)
{
	char * p = (char *)inDupNode + inDupNode->x0C;
	size_t dataSize = kfSizeOfData(inData);
	ALIGNkfSIZE(dataSize);
	if (dataSize > inDupNode->x08)
		return NO;	// no room

	memmove(p, inData, dataSize);

	inDupNode->x08 -= dataSize;
	inDupNode->x0C += dataSize;
	inDupNode->x0A++;
	return YES;
}


BOOL
CSoupIndex::prependDupData(DupNodeHeader * inDupNode, void * inData)
{
	char * p = (char *)firstDupDataAddr(inDupNode);
	size_t dataSize = kfSizeOfData(inData);
	ALIGNkfSIZE(dataSize);
	if (dataSize > inDupNode->x08)
		return NO;	// no room

	char * pEnd = p + dataSize;
	char * limit = (char *)inDupNode + inDupNode->x0C;
	memmove(pEnd, p, limit - pEnd);

	memmove(p, inData, dataSize);

	inDupNode->x08 -= dataSize;
	inDupNode->x0C += dataSize;
	inDupNode->x0A++;
	return YES;
}


BOOL
CSoupIndex::deleteDupData(DupNodeHeader * inDupNode, void * inData)
{
	changeNode((NodeHeader *)inDupNode);

	size_t dataSize = kfSizeOfData(inData);
	ALIGNkfSIZE(dataSize);

	char * dataEnd = (char *)inData + dataSize;
	char * limit = (char *)inDupNode + inDupNode->x0C;
	memmove(inData, dataEnd, limit - dataEnd);

	inDupNode->x08 += dataSize;
	inDupNode->x0C -= dataSize;
	inDupNode->x0A--;
	return YES;
}


void *
CSoupIndex::findDupDataAddr(DupNodeHeader * inDupNode, void * inData, void ** outData)
{
	void * theData = NULL;
	if (outData != NULL)
		*outData = NULL;
	while (nextDupDataAddr(inDupNode, theData, &theData))
	{
		if ((this->*f3C)(*(const SKey *)inData, *(const SKey *)theData) == 0)
			return theData;
		if (outData != NULL)
			*outData = theData;
	}
	return NULL;
}


void *
CSoupIndex::findNextDupDataAddr(DupNodeHeader ** ioDupNode, void * inData, BOOL * outFound)
{
	PSSId r1;
	void * sp00;
	while ((sp00 = findDupDataAddr(*ioDupNode, inData, NULL)) == 0)
	{
		if ((r1 = (*ioDupNode)->x04) != 0)
			*ioDupNode = readADupNode(r1);
		else
		{
			if (outFound != NULL)
				*outFound = NO;
			return NULL;
		}
	}

	if (outFound != NULL)
		*outFound = YES;
	if (nextDupDataAddr(*ioDupNode, sp00, &sp00))
		return sp00;
	if ((r1 = (*ioDupNode)->x04) != 0)
	{
		*ioDupNode = readADupNode(r1);
		return firstDupDataAddr(*ioDupNode);
	}
	return NULL;
}


void *
CSoupIndex::findPriorDupDataAddr(DupNodeHeader ** ioDupNode, void * inData, BOOL * outFound)
{
	DupNodeHeader * priorDupNode;
	void * priorData;
	PSSId r1;
	void * sp00;
	while (findDupDataAddr(*ioDupNode, inData, &sp00) == 0)
	{
		priorData = sp00;
		priorDupNode = *ioDupNode;
		if ((r1 = (*ioDupNode)->x04) != 0)
			*ioDupNode = readADupNode(r1);
		else
		{
			if (outFound != NULL)
				*outFound = NO;
			return NULL;
		}
	}

	if (outFound != NULL)
		*outFound = YES;
	if (sp00 == NULL)
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

KeyField *
CSoupIndex::keyFieldAddr(NodeHeader * inNode, long index)
{
	return (KeyField *)((char *)inNode + inNode->x0C[index]);
}


ULong
CSoupIndex::leftNodeNo(NodeHeader * inNode, long index)
{
	ULong * nodeNumPtr = ((ULong *)keyFieldAddr(inNode, index)) - 1;
	return *nodeNumPtr;	// need to handle misaligned pointer
}


ULong
CSoupIndex::rightNodeNo(NodeHeader * inNode, long index)
{
	ULong * nodeNumPtr = ((ULong *)keyFieldAddr(inNode, index+1)) - 1;
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
	return leftNodeNo(inNode, inNode->x0A);
}


KeyField *
CSoupIndex::firstKeyField(NodeHeader * inNode)
{
	return keyFieldAddr(inNode, 0);
}

KeyField *
CSoupIndex::lastKeyField(NodeHeader * inNode)
{
	return keyFieldAddr(inNode, inNode->x0A - 1);
// although original says:
//	return (KeyField *)((char *)inNode + inNode->x0C[inNode->x0A - 1)]);
}

char *
CSoupIndex::keyFieldBase(NodeHeader * inNode)
{
	return (char *)&inNode->x0C[inNode->x0A] + inNode->x08 + 2;
}

#pragma mark -

void
CSoupIndex::createNewRoot(KeyField * inField, ULong inArg2)
{
	NodeHeader * theNode = newNode();
	putKeyIntoNode(inField, inArg2, theNode, 0);
	setNodeNo(theNode, 0, f10.x04);
	setRootNode(theNode->x00);
}


BOOL
CSoupIndex::insertKey(KeyField * inField, NodeHeader * inNode, ULong * ioArg3, BOOL * ioArg4)
{
	ULong theId;
	long theSlot;
	if (keyInNode(inField, inNode, &theId, &theSlot))
	{
		changeNode(inNode);
		insertDupData(inField, inNode, theSlot, ioArg3, ioArg4);
	}
	else
	{
		if (theId == 0)
		{
			*ioArg4 = YES;
			*ioArg3 = 0;
		}
		else
		{
			insertKey(inField, readANode(theId, inNode->x00), ioArg3, ioArg4);
		}
		if (*ioArg4)
		{
			keyInNode(inField, inNode, &theId, &theSlot);
			if (roomInNode(inNode, inField))
			{
				*ioArg4 = NO;
				changeNode(inNode);
				putKeyIntoNode(inField, *ioArg3, inNode, theSlot);
				f08->dirtyNode(inNode);
			}
			else
			{
				*ioArg4 = YES;
				splitANode(inField, ioArg3, inNode, theSlot);
			}
		}
	}
	return f2C == 0;
}


void
CSoupIndex::insertAfterDelete(KeyField * inField, ULong inArg2, NodeHeader * inNode)
{
	ULong theId;
	long theSlot;

	if (inNode != NULL)
	{
		while (!roomInNode(inNode, inField))
		{
			keyInNode(inField, inNode, &theId, &theSlot);
			splitANode(inField, &inArg2, inNode, theSlot);
			if (inNode->x04 == 0)
			{
				inNode = NULL;
				break;
			}
			if ((inNode = f08->findNode(this, inArg2)) == NULL)
				break;
		}
	}

	if (inNode != NULL)
	{
		changeNode(inNode);
		keyInNode(inField, inNode, &theId, &theSlot);
		putKeyIntoNode(inField, inArg2, inNode, theSlot);
		f08->dirtyNode(inNode);
	}
	else
		createNewRoot(inField, inArg2);
}


BOOL
CSoupIndex::deleteKey(KeyField * inField, NodeHeader * inNode, BOOL * outArg3)
{
	ULong theId;
	long theSlot;
	BOOL r9 = NO;

	if (keyInNode(inField, inNode, &theId, &theSlot))
	{
		if (theId == 0)
		{
			r9 = YES;
			if (fSavedKey->length != 0)
				fSavedKey->length = 0;
			else
				deleteTheKey(inNode, theSlot, inField);
			if (!(*outArg3 = nodeUnderflow(inNode)))
				f08->dirtyNode(inNode);
		}
		else
		{
			r9 = YES;
			KeyField * r10 = keyFieldAddr(inNode, theSlot);
			if (r10->flags == 0x01
			&& ((kfDupCount(r10) > 1
			  || kfNextDupId(r10) != 0)))
			{
				deleteTheKey(inNode, theSlot, inField);
				*outArg3 = NO;
			}
			else
			{
				ULong r10 = rightNodeNo(inNode, theSlot);
				*outArg3 = getLeafKey(fLeafKey, readANode(r10, inNode->x00));
				deleteTheKey(inNode, theSlot, inField);
				insertAfterDelete(fLeafKey, r10, inNode);
				if (*outArg3)
				{
					moveKey(fSavedKey, fLeafKey);
					_BTRemoveKey(fLeafKey);
					*outArg3 = NO;
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
				r9 = YES;
				fSavedKey->length = 0;
				if (!(*outArg3 = nodeUnderflow(inNode)))
					f08->dirtyNode(inNode);
			}
		}
		else
		{
			NodeHeader * aNode;
			if ((aNode = readANode(theId, inNode->x00)) != NULL)
			{
				r9 = deleteKey(inField, inNode, outArg3);
				if (*outArg3)
					*outArg3 = balanceTwoNodes(inNode, aNode, theSlot);
			}
		}
	}
	

	return r9;
}


BOOL
CSoupIndex::moveKey(KeyField * inFromField, KeyField * outToField)
{
	size_t fieldSize = inFromField->length;
	if (fieldSize != 0)
	{
		memmove(outToField, inFromField, fieldSize);
		return YES;
	}
	outToField->length = 0;
	return NO;
}


void
CSoupIndex::copyKeyFmNode(KeyField * outField, ULong * outNo, NodeHeader * inNode, long inSlot)
{
	KeyField * theField = keyFieldAddr(inNode, inSlot);
	memmove(outField, theField, theField->length);
	*outNo = leftNodeNo(inNode, inSlot);
}


KeyField *
CSoupIndex::keyBeforeNodeNo(NodeHeader * inNode, ULong inNo, long * outSlot)
{
	long slot = 0;
	while (leftNodeNo(inNode, slot) != inNo)
		slot++;
	*outSlot = slot-1;
	if (slot != 0)
		return keyFieldAddr(inNode, *outSlot);
	return NULL;
}


KeyField *
CSoupIndex::keyAfterNodeNo(NodeHeader * inNode, ULong inNo, long * outSlot)
{
	long slot = 0;
	*outSlot = slot;
	while (leftNodeNo(inNode, slot) != inNo)
		*outSlot = ++slot;
	return keyFieldAddr(inNode, *outSlot);
}


int
CSoupIndex::lastSlotInNode(NodeHeader * inNode)
{
	return inNode->x0A - 1;		// could make this inline and use it as necessary
}


size_t
CSoupIndex::bytesInNode(NodeHeader * inNode)
{
	return f10.x04 - inNode->x08;		// could make this inline and use it as necessary
}


BOOL
CSoupIndex::roomInNode(NodeHeader * inNode, KeyField * inKeyField)
{
	return (inKeyField->length + kSizeOfLengthWords) < inNode->x08;		// could make this inline and use it as necessary
}


BOOL
CSoupIndex::nodeUnderflow(NodeHeader * inNode)
{
	return inNode->x08 > f10.x04/2;
}


void
CSoupIndex::putKeyIntoNode(KeyField * inField, ULong inArg2, NodeHeader * ioNode, long inSlot)
{
	if (!roomInNode(ioNode, inField))
		throw2(exStore, kNSErrInternalError);

	// point to data area within node
	char * keyBuf = keyFieldBase(ioNode) - inField->length;
	// copy key into it
	memmove(keyBuf, inField, inField->length);
	// open up a gap for the offset to this new key
	size_t numOfSlotsToMove = (ioNode->x0A - inSlot) + 1;
	memmove(&ioNode->x0C[inSlot+1], &ioNode->x0C[inSlot], numOfSlotsToMove * sizeof(short));
	// insert offset to new key
	ioNode->x0C[inSlot] = keyBuf - (char *)ioNode;
	// insert slot into btree
	setNodeNo(ioNode, inSlot, rightNodeNo(ioNode, inSlot));
	setNodeNo(ioNode, inSlot+1, inArg2);
	// update sizes
	ioNode->x0A++;
	ioNode->x08 -= (KeyField::kOffsetToData + SKey::kOffsetToData + SKey::kOffsetToData + inField->length);
}


void
CSoupIndex::deleteKeyFromNode(NodeHeader * inNode, long inSlot)
{
	if (inSlot < 0 || inSlot >= inNode->x0A)
		throw2(exStore, kNSErrInternalError);

	// remove slot from btree
	setNodeNo(inNode, inSlot+1, leftNodeNo(inNode, inSlot));
	// point to data area
	KeyField * r8 = keyFieldAddr(inNode, inSlot);
	size_t r6 = SKey::kOffsetToData + SKey::kOffsetToData + r8->length;
	char * r1 = keyFieldBase(inNode);
	short r9 = inNode->x0C[inSlot] + r6;
	// remove key from data area
	memmove(r1+r6, r1, (char *)r8-r1-(SKey::kOffsetToData + SKey::kOffsetToData));
	// remove offset to key
	size_t numOfSlotsToMove = (inNode->x0A - inSlot) + 1;
	memmove(&inNode->x0C[inSlot], &inNode->x0C[inSlot+1], numOfSlotsToMove * sizeof(short));
	// update sizes
	inNode->x0A--;
	inNode->x08 += (KeyField::kOffsetToData + SKey::kOffsetToData + SKey::kOffsetToData + r8->length);
	// update offsets for keys
	for (int i = 0; i < inNode->x0A; i++)
	{
		short offset = inNode->x0C[i];
		if (offset < r9)
			inNode->x0C[i] = offset + r6;
	}
}


BOOL
CSoupIndex::keyInNode(KeyField * inKeyField, NodeHeader * inNode, ULong * outId, long * outIndex)
{
	// perform binary search for key
	int cmp = 0;
	long index = 0;	// r6
	long firstIndex = 0;					// r8
	long lastIndex = inNode->x0A - 1;	// r7
	while (lastIndex >= firstIndex)
	{
		index = (firstIndex + lastIndex)/2;

		KeyField * r2 = keyFieldAddr(inNode, index);
		cmp = compareKeys(*(const SKey *)inKeyField->buf, *(const SKey *)r2->buf);

		if (cmp > 0)
		// key must be in higher range
			firstIndex = index + 1;
		else if (cmp < 0)
		// key must be in lower range
			lastIndex = index - 1;
		else
		{
		// got it!
			*outIndex = index;
			*outId = leftNodeNo(inNode, index);
			return YES;
		}	
	}
	if (cmp > 0)
		index++;
	*outIndex = index;
	*outId = leftNodeNo(inNode, index);
	return NO;
}


BOOL
CSoupIndex::findFirstKey(NodeHeader * inNode, KeyField * inField)
{
	ULong nodeNo;
	while ((nodeNo = firstNodeNo(inNode)) != 0)
		readANode(nodeNo, inNode->x00);
	moveKey(firstKeyField(inNode), inField);
	return YES;
}


BOOL
CSoupIndex::findLastKey(NodeHeader * inNode, KeyField * inField)
{
	ULong nodeNo;
	while ((nodeNo = lastNodeNo(inNode)) != 0)
		readANode(nodeNo, inNode->x00);
	KeyField * kf = lastKeyField(inNode);
	moveKey(kf, inField);
	if (kf->flags == 0x01)
		kfReplaceFirstData(inField, lastDupDataAddr(kf, NULL));
	return YES;
}


BOOL		CSoupIndex::balanceTwoNodes(NodeHeader * inNode1, NodeHeader * inNode2, long) { return NO; }
void		CSoupIndex::mergeTwoNodes(KeyField * inField, NodeHeader*, NodeHeader*, NodeHeader*) {}
void		CSoupIndex::splitANode(KeyField*, ULong*, NodeHeader * inNode, long) {}
void		CSoupIndex::deleteTheKey(NodeHeader * inNode, long, KeyField*) {}


BOOL
CSoupIndex::getLeafKey(KeyField * inField, NodeHeader * inNode)
{
	ULong nodeNo;
	while ((nodeNo = firstNodeNo(inNode)) != 0)
		readANode(nodeNo, inNode->x00);
	copyKeyFmNode(inField, &nodeNo, NULL, 0);
	changeNode(inNode);
	deleteKeyFromNode(inNode, 0);
	f08->dirtyNode(inNode);

	fSavedKey->length = 0;
	BOOL isUnderflow;
	if (isUnderflow = nodeUnderflow(inNode))
		copyKeyFmNode(fSavedKey, &nodeNo, NULL, 0);
	return isUnderflow;
}


void
CSoupIndex::checkForDupData(KeyField * inField, void * inData)
{
	if (f10.x10 == 0)
		ThrowOSErr(1);

	void * data;
	if ((data = kfFindDataAddr(inField, inData, NULL)) != NULL)
		ThrowOSErr(1);

	PSSId dupId;
	if (inField->flags != 0		// sic -- shouldn’t this be == 0x01 ?
	&&  (dupId = kfNextDupId(inField)) != 0)
	{
		DupNodeHeader * dupNode = readADupNode(dupId);
		data = NULL;
		while (nextDupDataAddr(dupNode, data, &data))
		{
			if ((this->*f3C)(*(const SKey *)inData, *(const SKey *)data) == 0)
				ThrowOSErr(1);
		}
	}
}


void		CSoupIndex::storeDupData(KeyField*, void*) {}
void		CSoupIndex::insertDupData(KeyField*, NodeHeader*, long, ULong*, BOOL*) {}

#pragma mark Key Comparison

int
CSoupIndex::compareKeys(const SKey& inKey1, const SKey& inKey2)
{
	int result = (this->*f38)(inKey1, inKey2);
	if (f10.x19)	// sorting
		result = -result;
	return result;
}

int
CSoupIndex::stringKeyCompare(const SKey& inKey1, const SKey& inKey2)
{
	return CompareUnicodeText(	(const UniChar *)inKey1.data(), inKey1.size()/sizeof(UniChar),
										(const UniChar *)inKey2.data(), inKey2.size()/sizeof(UniChar),
										f40, f40 != nil );
}


int
CSoupIndex::longKeyCompare(const SKey& inKey1, const SKey& inKey2)
{
	return (long)inKey1 - (long)inKey2;
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
	for (i = 0; i < len1 && i < len2; i++)
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
	ULong multiTypes = f10.x14;
	char multiSort = f10.x18;

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

#pragma mark _BT

int
CSoupIndex::_BTEnterKey(KeyField * inField)
{
	ULong sp00;
	BOOL sp04 = NO;
	NodeHeader * theNode;
	f2C = 0;
	if ((theNode = readRootNode(YES)) != NULL
	&& insertKey(inField, theNode, &sp00, &sp04))
	{
		if (sp04)
			createNewRoot(inField, sp00);
		f2C = 0;
	}
	else
		f2C = 2;
	return f2C;
}

int
CSoupIndex::_BTGetNextKey(KeyField * inField)
{
	NodeHeader * theNode = readRootNode(NO);
	if (theNode != NULL)
	{
		long sp00;
		int cmp = searchNext(inField, &theNode, &sp00);
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
	NodeHeader * theNode = readRootNode(NO);
	if (theNode != NULL)
	{
		long sp00;
		DupNodeHeader * dup;
		int cmp = searchNextDup(inField, &theNode, &sp00, &dup);
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
	NodeHeader * theNode = readRootNode(NO);
	if (theNode != NULL)		// not in the original
	{
		long sp00;
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
	NodeHeader * theNode = readRootNode(NO);
	if (theNode != NULL)
	{
		long sp00;
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
	BOOL sp00 = NO;
	BOOL r5;
	NodeHeader * theNode;
	if ((theNode = readRootNode(NO)) != NULL
	&&  (r5 = deleteKey(inField, theNode, &sp00)))
	{
		if (sp00)
		{
			if (theNode->x0A == 0)
			{
				setRootNode(firstNodeNo(theNode));
				deleteNode(theNode->x00);
			}
			else
				f08->dirtyNode(theNode);
		}
		f2C = r5 ? 0 : 2;	// r5 MUST be set at this point
	}
	else
		f2C = 2;
	return f2C;
}

#pragma mark -

BOOL
CSoupIndex::search(KeyField * ioKeyField, NodeHeader ** ioNode, long * ioSlot)
{
	PSSId theId;
	BOOL isFound = keyInNode(ioKeyField, *ioNode, &theId, ioSlot);
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

	*ioNode = readANode(theId, (*ioNode)->x00);
	return search(ioKeyField, ioNode, ioSlot);
}


int
CSoupIndex::searchNext(KeyField * ioKeyField, NodeHeader ** ioNode, long * ioSlot)
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

	*ioNode = readANode(theId, (*ioNode)->x00);
	return searchNext(ioKeyField, ioNode, ioSlot);
}


int
CSoupIndex::searchPrior(KeyField * ioKeyField, NodeHeader ** ioNode, long * ioSlot)
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

	*ioNode = readANode(theId, (*ioNode)->x00);
	return searchPrior(ioKeyField, ioNode, ioSlot);
}


int
CSoupIndex::searchNextDup(KeyField * ioKeyField, NodeHeader ** ioNode, long * ioSlot, DupNodeHeader ** ioDupNode)
{
	PSSId theId;
	*ioDupNode = NULL;
	if (keyInNode(ioKeyField, *ioNode, &theId, ioSlot))
	{
		void * item1 = kfFirstDataAddr(ioKeyField);
		KeyField * kf = keyFieldAddr(*ioNode, *ioSlot);
		if (kf->flags == 0)
		{
			void * item2 = kfFirstDataAddr(kf);
			return ((this->*f3C)(*(const SKey *)item1, *(const SKey *)item2) == 0) ? 0 : -1;
		}

		void * sp00;
		if ((sp00 = kfFindDataAddr(ioKeyField, item1, NULL)) != NULL
		&&  kfNextDataAddr(ioKeyField, sp00, &sp00) != NULL)
		{
			kfReplaceFirstData(ioKeyField, sp00);
			return 1;
		}

		PSSId dupId;
		if ((dupId = kfNextDupId(ioKeyField)) == 0)
			return (theId == 0) ? -1 : 0;

		BOOL huh;
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

	*ioNode = readANode(theId, (*ioNode)->x00);
	return searchNextDup(ioKeyField, ioNode, ioSlot, ioDupNode);
}


int
CSoupIndex::searchPriorDup(KeyField * ioKeyField, NodeHeader ** ioNode, long * ioSlot, DupNodeHeader ** ioDupNode)
{
	PSSId theId;
	*ioDupNode = NULL;
	if (keyInNode(ioKeyField, *ioNode, &theId, ioSlot))
	{
		void * item1 = kfFirstDataAddr(ioKeyField);
		KeyField * kf = keyFieldAddr(*ioNode, *ioSlot);
		if (kf->flags == 0)
		{
			void * item2 = kfFirstDataAddr(kf);
			return ((this->*f3C)(*(const SKey *)item1, *(const SKey *)item2) == 0) ? 0 : -1;
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

		BOOL huh;
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

	*ioNode = readANode(theId, (*ioNode)->x00);
	return searchPriorDup(ioKeyField, ioNode, ioSlot, ioDupNode);
}


int		CSoupIndex::findNextKey(KeyField * inKey, NodeHeader ** ioNode, long * ioSlot) { return 0; }
int		CSoupIndex::findPriorKey(KeyField * inKey, NodeHeader ** ioNode, long * ioSlot) { return 0; }

void		CSoupIndex::findAndGetState(KeyField*, IndexState*) {}
void		CSoupIndex::findLastAndGetState(KeyField*, IndexState*) {}
void		CSoupIndex::findPriorAndGetState(KeyField*, unsigned char, IndexState*) {}
void		CSoupIndex::moveAndGetState(unsigned char, int, KeyField*, IndexState*) {}
void		CSoupIndex::moveUsingState(unsigned char, int, KeyField*, IndexState*) {}


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
		err = (NewtonErr) CurrentException()->data;
	}
	end_try;
	return err;
}


NewtonErr
CSoupIndex::addInTransaction(SKey * inKey, SKey * inData)
{
	NewtonErr err;
	kfAssembleKeyField(fKeyField, inKey, inData);
	err = _BTEnterKey(fKeyField);
	f08->flush(this);
	return err;
}


int
CSoupIndex::search(int, SKey*, SKey*, StopProcPtr, void*, SKey*, SKey*)
{ return 0; }


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
		err = (NewtonErr) CurrentException()->data;
	}
	end_try;
	if (err == noErr)
		f08->commit(this);
	else
		f08->abort(this);
	return err;
}


int
CSoupIndex::find(SKey * inKey, SKey * outKey, SKey * outData, BOOL inArg4)
{
	// this: r4  args: r5 r6 x r7
	if (f10.x00 == 0)		// no IndexInfo
		return 3;

	kfAssembleKeyField(fKeyField, inKey, NULL);

	int keyStrLen;		// r9
	if (f40 && !inArg4 && f10.x08 == 0)	// key is a string
	{
		keyStrLen = inKey->size() / sizeof(UniChar);
		f40->convertTextToLowestSort((UniChar *)(fKeyField->buf + SKey::kOffsetToData), keyStrLen);
	}
	else
		keyStrLen = 0;

	int result = 0;		// r7
	BOOL failed = NO;		// r10
	newton_try
	{
		long slot;
		NodeHeader * theNode;
		if ((theNode = readRootNode(NO)) != NULL)
		{
			if (search(fKeyField, &theNode, &slot) == 0)
				result = fKeyField->length == 0 ? 3 : 2;
			f08->commit(this);
		}
		else
			result = 2;
	}
	newton_catch_all
	{
		failed = YES;
		f08->abort(this);
		result = (int)CurrentException()->data;
	}
	end_try;

	if (!failed)
	{
		if (keyStrLen != 0 && result == 2)
		{
			if (CompareUnicodeText(	(const UniChar *)inKey->data(), keyStrLen,
											(const UniChar *)fKeyField->buf + SKey::kOffsetToData, kfSizeOfKey(fKeyField->buf)/sizeof(UniChar)-1,
											f40) == 0)
				result = 0;
		}
		if (result == 0 || result == 2)
			kfDisassembleKeyField(fKeyField, outKey, outData);
	}

	return result;
}


int
CSoupIndex::first(SKey*, SKey*)
{ return 0; }

int
CSoupIndex::last(SKey*, SKey*)
{ return 0; }

int
CSoupIndex::next(SKey*, SKey*, int, SKey*, SKey*)
{ return 0; }

int
CSoupIndex::prior(SKey*, SKey*, int, SKey*, SKey*)
{ return 0; }


#pragma mark Size

void
CSoupIndex::nodeSize(NodeHeader * inNode, size_t & ioSize)
{
	size_t theSize;
	OSERRIF(store()->getObjectSize(inNode->x00, &theSize));
	ioSize += theSize;
	for (int i = 0; i < inNode->x0A; i++)
	{
		PSSId lNode;
		KeyField * kf = keyFieldAddr(inNode, i);
		if (kf->flags == 0x01 && kf->length != 0)
			dupNodeSize(kf, ioSize);
		if ((lNode = leftNodeNo(inNode, i)) != 0)
			nodeSize(readANode(lNode, inNode->x04), ioSize);
	}
	f08->forgetNode(inNode->x00);
}


void
CSoupIndex::dupNodeSize(KeyField * inField, size_t & ioSize)
{
	PSSId dupId = kfNextDupId(inField);
	while (dupId != 0)
	{
		size_t theSize;
		DupNodeHeader * dupNode = readADupNode(dupId);
		dupId = dupNode->x04;
		OSERRIF(store()->getObjectSize(dupNode->x00, &theSize));
		ioSize += theSize;
		f08->forgetNode(dupNode->x00);
	}
}


size_t
CSoupIndex::totalSize(void)
{
	size_t theSize;
	OSERRIF(store()->getObjectSize(f0C, &theSize));
	if (f10.x00 != 0)
	{
		newton_try
		{
			NodeHeader * theNode;
			if ((theNode = readRootNode(NO)) != NULL)
				nodeSize(theNode, theSize);
			setRootNode(0);
		}
		cleanup
		{
			f08->abort(this);
		}
		end_try;
	}
	f08->commit(this);
	return theSize;
}

#pragma mark -

/*----------------------------------------------------------------------
	U n i o n I n d e x D a t a
----------------------------------------------------------------------*/

UnionIndexData::UnionIndexData()
{
	f04 = 0;
	f80 = 0;
	f7C = &f18;
}


UnionIndexData::~UnionIndexData()
{ }

#pragma mark -

/*----------------------------------------------------------------------
	C U n i o n S o u p I n d e x
----------------------------------------------------------------------*/

CUnionSoupIndex::CUnionSoupIndex(unsigned inNumOfIndexes, UnionIndexData * indexes)
{
	fNumOfSoupsInUnion = inNumOfIndexes;
	fIndexData = indexes;
	fSeqInUnion = 0;
}


CUnionSoupIndex::~CUnionSoupIndex()
{
	for (int i = fNumOfSoupsInUnion - 1; i >= 0; i--)
		delete (fIndexData + i);	// sic -- we didn’t alloc them, but we’re deleting them?
}


int
CUnionSoupIndex::find(SKey * inKey, SKey * outKey, SKey * outData, BOOL inArg4)
{
	if (fNumOfSoupsInUnion == 1)
		return fIndexData->fIndex->find(inKey, outKey, outData, inArg4);

	SKey spA4 = *inKey;
	SKey sp54;
	SKey sp04;
	int  hasAKey = 0;
	int  r8 = 3;
	for (int i = 0; i < fNumOfSoupsInUnion; i++)
	{
		UnionIndexData * indexData = fIndexData + i;
		CSoupIndex * index = indexData->fIndex;
		int r10 = index->find(&spA4, &sp54, &sp04, inArg4);
		if (((r10 == 0 && ((index->f40 && !inArg4) || r8 == 2 || r8 == 3))
		   || r10 == 2)
		&&  (hasAKey == 0 || index->compareKeys(sp54, *outKey) < 0))
		
		{
			r8 = r10;
			memmove(outKey, &sp54, index->kfSizeOfKey(&sp54));
			if (outData)
				memmove(outData, &sp04, index->kfSizeOfData(&sp04));
			setCurrentSoup(i);
			hasAKey = 1;
		}
	}

	return r8;
}


int
CUnionSoupIndex::first(SKey * outKey, SKey * outData)
{
	if (fNumOfSoupsInUnion == 1)
		return fIndexData->fIndex->first(outKey, outData);

	SKey sp50;
	SKey sp00;
	int  hasAKey = 0;
	int  r9 = 2;
	for (int i = 0; i < fNumOfSoupsInUnion; i++)
	{
		UnionIndexData * indexData = fIndexData + i;
		CSoupIndex * index = indexData->fIndex;
		int result = index->first(&sp50, &sp00);
		if (result == 0)
		{
			r9 = result;
			if (hasAKey == 0 || index->compareKeys(sp50, *outKey) < 0)
			{
				memmove(outKey, &sp50, index->kfSizeOfKey(&sp50));
				memmove(outData, &sp00, index->kfSizeOfData(&sp00));
				setCurrentSoup(i);
				hasAKey = 1;
			}
		}
	}

	return r9;
}


int
CUnionSoupIndex::last(SKey * outKey, SKey * outData)
{
	if (fNumOfSoupsInUnion == 1)
		return fIndexData->fIndex->last(outKey, outData);

	SKey sp50;
	SKey sp00;
	int  hasAKey = 0;
	int  r9 = 2;
	for (int i = 0; i < fNumOfSoupsInUnion; i++)
	{
		UnionIndexData * indexData = fIndexData + i;
		CSoupIndex * index = indexData->fIndex;
		int result = index->last(&sp50, &sp00);
		if (result == 0)
		{
			r9 = result;
			if (hasAKey == 0 || index->compareKeys(sp50, *outKey) < 0)
			{
				memmove(outKey, &sp50, index->kfSizeOfKey(&sp50));
				memmove(outData, &sp00, index->kfSizeOfData(&sp00));
				setCurrentSoup(i);
				hasAKey = 1;
			}
		}
	}

	return r9;
}


int
CUnionSoupIndex::next(SKey * inArg1, SKey * inArg2, int inArg3, SKey * inArg4, SKey * inArg5)
{
	return search(1, inArg1, inArg2, NULL, NULL, inArg4, inArg5, inArg3);
}


int
CUnionSoupIndex::prior(SKey * inArg1, SKey * inArg2, int inArg3, SKey * inArg4, SKey * inArg5)
{
	return search(0, inArg1, inArg2, NULL, NULL, inArg4, inArg5, inArg3);
}


int
CUnionSoupIndex::search(BOOL, SKey*, SKey*, int (*)(SKey*, SKey*, void*), void*, SKey*, SKey*, int)
{ return 0; }


int
CUnionSoupIndex::currentSoupGone(SKey * inArg1, SKey * inArg2, SKey * inArg3)
{
	int result;
	newton_try
	{
		if ((result = moveToNextSoup(1, 0, inArg1, 1)) == noErr)
		{
			UnionIndexData * indexData = fIndexData + fSeqInUnion;
//			indexData->fIndex->kfDisassembleKeyField(indexData->f7C, inArg2, inArg3);
		}
		else
			setCurrentSoup(0);
	}
	newton_catch_all
	{
		result = (NewtonErr) CurrentException()->data;
	}
	end_try;
	return result;
}


int
CUnionSoupIndex::moveToNextSoup(BOOL inArg1, int inArg2, SKey * inKey, BOOL inArg4)
{
#if 0
	IndexState * sp00;
	UnionIndexData * r7;
	if (inArg4)
	{
		r6 = fSeqInUnion - 1;
		r7 = NULL;
		r0 = fNumOfSoupsInUnion;
	}
	else
	{
		r6 = fSeqInUnion;
		r7 = fIndexData + fSeqInUnion;
		r0 = fNumOfSoupsInUnion - 1;
	}
	int r8 = 0;
	for (int r10 = r0 - 1; r10 >= 0; r10--)
	{
		r6++;
		if (r6 == fNumOfSoupsInUnion)
		{
			r6 = 0;
			r8 = 1;
		}
		UnionIndexData * r5 = fIndexData + r6;
		int r0 = r5->f04;
		if (r0 != 2)
		{
			if (r0 != 1)
			{
				r5->kfAssembleKeyField(r5->f7C, inKey?, NULL);
				if (inArg1)
				{
					r0 = r5->findAndGetState(r5->f7C, sp00);
					if (r0 == 0)
					{
						if (r8 != 0 || inArg2 == 1)
							r0 = r5->moveUsingState(inArg1, 1, r5->f7C, sp00);
					}
				}
				else
				{
					r0 = r5->findPriorAndGetState(r5->f7C, r8 || inArg2 == 1, &r5->f08);
				}
				if (r0 == 3)
				{
					r5->f04 = 2;
					continue;
				}
				else if (r0 == 0 || r0 == 2)
					r5->f04 = 1;
				else
					return r0;
			}
			if (r7 == NULL
			||  r7->f04 != 1)
			{
				r0 = 1;
			}
			else
			{
				r0 = r5->fIndex->compareKeys(&r5->f7C.x02, &r7->f7C.x02);
				if (inArg1)
				{
					if (r0 > 0)
						r0 = 1;
					else if (r0 != 0)
						r0 = 0;
					else if (fSeqInUnion < r6)
						r0 = 1;
					else
						r0 = 0;
				}
				else
				{
					if (r0 < 0)
						r0 = 1;
					else if (r0 != 0)
						r0 = 0;
					else if (fSeqInUnion > r6)
						r0 = 1;
					else
						r0 = 0;
				}
			}
			if (r0)
			{
				r7 = r5;
				fSeqInUnion = r6;
			}
		}
	}

	if (r7 != NULL
	&&  r7->f04 == 1)
		return 0;
#endif
	return 3;
}


void
CUnionSoupIndex::setCurrentSoup(long index)
{
	fSeqInUnion = index;
	invalidateState();
}

void
CUnionSoupIndex::invalidateState(void)
{
	for (int i = fNumOfSoupsInUnion - 1; i >= 0; i--)
		fIndexData[i].f04 = 0;
}


BOOL
CUnionSoupIndex::isValidState(SKey * inKey, SKey * inData)
{
	for (int i = fNumOfSoupsInUnion - 1; i >= 0; i--)
	{
		UnionIndexData * indexData = fIndexData + i;		// r7
		CSoupIndex * index = indexData->fIndex;
//		if (index->f08->f0C != indexData->f80)
//			return NO;

		if (fSeqInUnion == i)
		{
			void * data;
			size_t dataSize, keySize;

			if (indexData->f04 != 1)
				return NO;

			data = index->kfFirstDataAddr(indexData->f7C);
			dataSize = index->kfSizeOfData(inData);
			if (dataSize != index->kfSizeOfData(data))
				return NO;

			if (memcmp(data, inData, dataSize) != 0)
				return NO;

			keySize = index->kfSizeOfKey(inKey);
			if (keySize != index->kfSizeOfKey(&indexData->f7C->buf))
				return NO;

			if (memcmp(&indexData->f7C->buf, inKey, keySize) != 0)
				return NO;

		}
		else
		{
			if (indexData->f04 == 1)
				index->f08->reuse(index);
		}
	}
	return YES;
}

void
CUnionSoupIndex::commit(void)
{
	for (int i = fNumOfSoupsInUnion - 1; i >= 0; i--)
	{
		UnionIndexData * indexData = fIndexData + i;
		CSoupIndex * index = indexData->fIndex;
		index->f08->commit(index);
	}
}

#pragma mark -

/*----------------------------------------------------------------------
	T a g s
----------------------------------------------------------------------*/

void
STagsBits::setTag(unsigned short index)
{
	unsigned byteIndex = index / 8;
	unsigned reqSize = byteIndex + 1;
	unsigned currSize = size();
	char * dataPtr = buf;
	int delta = reqSize - currSize;
	if (delta > 0)
	{
		// fill the gap between current and required size with zeros
		setSize(reqSize);
		char * p = dataPtr + currSize;
		for ( ; delta > 0; delta--)
			*p++ = 0;
	}
	dataPtr[byteIndex] |= (1 << (index-(byteIndex*8)));
}


static BOOL
EncodeTags(RefArg inTagDesc, RefArg inSpec, STagsBits * outTagBits)
{
	outTagBits->setSize(0);
	BOOL allOK = YES;
	Ref index;
	RefVar tagSpec;
	if (IsArray(inSpec))
	{
		for (int i = Length(inSpec) - 1; i >= 0; i--)
		{
			tagSpec = GetArraySlot(inSpec, i);
			if (!IsSymbol(tagSpec))
				ThrowExFramesWithBadValue(kNSErrNotASymbol, tagSpec);
			index = FSetContains(RA(NILREF), inTagDesc, tagSpec);
			if (ISINT(index))
				outTagBits->setTag(RINT(index));
			else
				allOK = NO;
		}
	}
	else if (!IsSymbol(inSpec))
		ThrowExFramesWithBadValue(kNSErrNotASymbol, tagSpec);
	else
	{
		index = FSetContains(RA(NILREF), inTagDesc, tagSpec);
		if (ISINT(index))
			outTagBits->setTag(RINT(index));
		else
			allOK = NO;
	}
	return allOK;
}

static Ref
MakeTagBinary(STagsBits * inTagBits)
{
	size_t bitsLen = SKey::kOffsetToData + inTagBits->size();
	RefVar bits(AllocateBinary(SYMA(tags), bitsLen));
	memmove(BinaryData(bits), inTagBits, bitsLen);
	return bits;
}

static BOOL
EncodeTag(RefArg inTagDesc, RefArg inQuerySpec, RefArg inSym, int inType, RefArg outTagList)
{
	RefVar spec(GetFrameSlot(inQuerySpec, inSym));
	if (NOTNIL(spec))
	{
		STagsBits tagsBits;
		if (EncodeTags(inTagDesc, spec, &tagsBits)
		||  (inType != 0 && inType != 1))
		{
			AddArraySlot(outTagList, MAKEINT(inType));
			AddArraySlot(outTagList, MakeTagBinary(&tagsBits));
			return YES;
		}
		return NO;
	}
	return YES;
}

Ref
EncodeQueryTags(RefArg indexDesc, RefArg inQuerySpec)
{
	RefVar tagList(MakeArray(0));
	RefVar tagDesc(GetFrameSlot(indexDesc, SYMA(tags)));
	if (EncodeTag(tagDesc, inQuerySpec, SYMA(equal), 0, tagList)
	&&  EncodeTag(tagDesc, inQuerySpec, SYMA(all), 1, tagList)
	&&  EncodeTag(tagDesc, inQuerySpec, SYMA(any), 2, tagList)
	&&  EncodeTag(tagDesc, inQuerySpec, SYMA(none), 3, tagList))
	{
		if (Length(tagList) == 0)
			throw2(exStore, kNSErrInvalidTagSpec);
		return tagList;
	}
	return NILREF;
}


BOOL
TagsValidTest(CSoupIndex & index, RefArg inArg2, ULong inArg3)
{ return YES; }


int
AddTag(RefArg inTags, RefArg inNewTag)
{
	RefVar tag;
	int i, slot = -1;

	for (i = Length(inTags) - 1; i >= 0; i--)
	{
		tag = GetArraySlot(inTags, i);
		if (ISNIL(tag))
		{
		// we found an empty slot
			slot = i;
			break;			// not in the original -- but no point looking for more, surely?
		}
		else
		{
		// slot is occupied -- ensure we’re not trying to duplicate the tag
			if (EQ(tag, inNewTag))
				return NO;
		}
	}
	tag = EnsureInternal(inNewTag);
	if (slot >= 0)
		SetArraySlot(inTags, slot, tag);
	else
		AddArraySlot(inTags, tag);
	return YES;
}


int
CountTags(RefArg inTags)
{
	int i, count = 0;
	for (i = Length(inTags) - 1; i >= 0; i--)
		if (NOTNIL(GetArraySlot(inTags, i)))
			count++;
	return count;
}

#pragma mark -

/*----------------------------------------------------------------------
	I n d e x e s
----------------------------------------------------------------------*/

Ref
GetTagsIndexDesc(RefArg inSpec)
{
	RefVar indexes(GetFrameSlot(inSpec, SYMA(indexes)));
	if (NOTNIL(indexes))
	{
		RefVar indexDesc;
		int i, count = Length(indexes);
		for (i = count - 1; i >= 0; i--)
		{
			indexDesc = GetArraySlot(indexes, i);
			if (EQ(GetFrameSlot(indexDesc, SYMA(type)), SYMA(tags)))
				return indexDesc;
		}
	}
	return NILREF;
}


void
AlterTagsIndex(unsigned char inSelector, CSoupIndex & ioSoupIndex, PSSId inId, RefArg inKey, RefArg inSoup, RefArg inTags)
{
	NewtonErr err;
	if (IsSymbol(inKey)
	||  IsArray(inKey) && Length(inKey) > 0)
	{
		SKey sp50;
		SKey sp00;
		sp50 = (long) inId;
		if (!EncodeTags(inTags, inKey, (STagsBits *)&sp00))
		{
			PlainSoupAddTags(inSoup, inKey);
			EncodeTags(inTags, inKey, (STagsBits *)&sp00);
		}
		if (inSelector == 0)
			err = ioSoupIndex.Delete(&sp50, &sp00);
		else
			err = ioSoupIndex.add(&sp50, &sp00);
		if (err != noErr)
			throw2(exStore, kNSErrInternalError);
	}
}


BOOL
UpdateTagsIndex(RefArg inSoup, RefArg indexDesc, RefArg inArg3, RefArg inArg4, PSSId inId)
{
//	args: r5, r4, r7, r6, r8
	//sp-0C
	RefVar path(GetFrameSlot(indexDesc, SYMA(path)));	// sp08
	RefVar sp04(GetEntryKey(inArg3, path));
	RefVar sp00(GetEntryKey(inArg4, path));
	if (ISNIL(sp04) && ISNIL(sp00))
		return 0;

	//sp-A4
	RefVar tags(GetFrameSlot(indexDesc, SYMA(tags)));	// spA0
	SKey spm50;
	SKey spm00;
	if (NOTNIL(sp04))
		EncodeTags(tags, sp04, (STagsBits *)&spm50);
	if (NOTNIL(sp00))
	{
		if (!EncodeTags(tags, sp00, (STagsBits *)&sp00))
		{
			PlainSoupAddTags(inSoup, sp00);
			EncodeTags(tags, sp00, (STagsBits *)&sp00);
		}
	}
	if (!spm50.equals(spm00))
	{
		//sp-50
		CSoupIndex * soupIndex = GetSoupIndexObject(inSoup, RINT(GetFrameSlot(indexDesc, SYMA(index))));
		SKey spmm00;
		spmm00 = (long) inId;
		NewtonErr err = noErr;
		XTRY
		{
			if (*(BOOL *)spm50.data())	// byte
				XFAIL(err = soupIndex->Delete(&spmm00, &spm50))
			if (*(BOOL *)spm00.data())	// byte
				XFAIL(err = soupIndex->add(&spmm00, &spm00))
		}
		XENDTRY;
		XDOFAIL(err)
		{
			throw2(exStore, kNSErrInternalError);
		}
		XENDFAIL;
		return YES;
	}
	return NO;
}


void
AlterIndexes(unsigned char inSelector, RefArg inSoup, RefArg inArg3, PSSId inId)
{
	RefVar spec(GetFrameSlot(inSoup, SYMA(_proto)));
	RefVar indexes(GetFrameSlot(spec, SYMA(indexes)));	// r5
	RefVar indexDesc;	// sp00
	int i, count = Length(indexes);
	for (i = count - 1; i >= 0; i--)
	{
		indexDesc = GetArraySlot(indexes, i);
		CSoupIndex * soupIndex = GetSoupIndexObject(inSoup, RINT(GetFrameSlot(indexDesc, SYMA(index))));
		if (EQ(GetFrameSlot(indexDesc, SYMA(type)), SYMA(tags)))
		{
			//sp-08
			RefVar sp00(GetFrameSlot(indexDesc, SYMA(path)));
			RefVar sp04(GetEntryKey(inArg3, sp00));
			//sp-04
			if (NOTNIL(sp04))
				AlterTagsIndex(inSelector, *soupIndex, inId, sp04, inSoup, GetFrameSlot(indexDesc, SYMA(tags)));
		}
		else
		{
			NewtonErr err;
			SKey spm00;
			if (GetEntrySKey(inArg3, indexDesc, &spm00, NULL))
			{
				if (inSelector == 0)
					err = soupIndex->Delete(&spm00, (SKey *)&inId);
				else
					err = soupIndex->add(&spm00, (SKey *)&inId);
				if (err != noErr)
					throw2(exStore, kNSErrInternalError);
			}
		}
	}
}


BOOL
PathsEqual(RefArg inPath1, RefArg inPath2)
{
	ULong tag;
	int i, count;
	if (EQ(inPath1, inPath2))
		return YES;
	if ((tag = RTAG(ObjectFlags(inPath1))) != RTAG(ObjectFlags(inPath2)))
		return NO;
	if (NOTREALPTR(tag))
		return NO;
	if ((count = Length(inPath1)) != Length(inPath2))
		return NO;
	for (i = 0; i < count; i++)
		if (!EQRef(GetArraySlot(inPath1, i), GetArraySlot(inPath2, i)))
			return NO;
	if (!EQRef(ClassOf(inPath1), ClassOf(inPath2)))
		return NO;
	return YES;
}


BOOL
IndexPathsEqual(RefArg inPath1, RefArg inPath2)
{
	int i, count1, count2;

	if (EQRef(ClassOf(inPath1), RSYMarray))
		count1 = Length(inPath1);
	else
		count1 = 0;

	if (EQRef(ClassOf(inPath2), RSYMarray))
	{
		count2 = Length(inPath2);
		if (count1 == count2)
		{
			for (i = count1 - 1; i >= 0; i--)
			{
				if (!PathsEqual(GetArraySlot(inPath1, i), GetArraySlot(inPath2, i)))
					return NO;
			}
			return YES;
		}
	}
	else
	{
		if (count1 == 0)
			return PathsEqual(inPath1, inPath2);
	}
	
	return NO;
}


Ref
IndexPathToIndexDesc(RefArg inSpec, RefArg inPath, int * outIndex)
{
	RefVar indexes(GetFrameSlot(inSpec, SYMA(indexes)));
	RefVar indexDesc;
	int i, count = Length(indexes);
	for (i = count - 1; i >= 0; i--)
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


BOOL
UpdateIndexes(RefArg inSoup, RefArg inArg2, RefArg inArg3, PSSId inId, BOOL * outArg5)
{
	RefVar spec(GetFrameSlot(inSoup, SYMA(_proto)));
	RefVar indexes(GetFrameSlot(spec, SYMA(indexes)));	// r4
	//sp-A4
	RefVar indexDesc;		// spA0
	RefVar indexType;		// r5
	SKey sp50;
	SKey sp00;
	BOOL isChanged = NO;	// r10
	int i, count = Length(indexes);
	for (i = count - 1; i >= 0; i--)
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
			BOOL isComplex;
			BOOL r8 = GetEntrySKey(inArg2, indexDesc, &sp50, &isComplex);		// add
			BOOL r6 = GetEntrySKey(inArg3, indexDesc, &sp00, NULL);				// delete
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
					else if ((long) sp50 == (long) sp00)
						break;
				}
			}

			isChanged = YES;
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
				throw2(exStore, kNSErrInternalError);
			}
			XENDFAIL;
		}
	}

	return isChanged;
}


BOOL
CompareSoupIndexes(RefArg inArg1, RefArg inArg2)
{
	RefVar indexes(GetFrameSlot(inArg1/*spec*/, SYMA(indexes)));
	RefVar indexDesc;
	int i, count = Length(indexes);
	if (count != Length(GetFrameSlot(inArg2/*spec*/, SYMA(indexes))))
		return NO;
	for (i = count - 1; i >= 0; i--)
	{
		indexDesc = GetArraySlot(indexes, i);
		if (ISNIL(IndexPathToIndexDesc(inArg2, GetFrameSlot(indexDesc, SYMA(path)), NULL)))
			return NO;
	}
	return YES;
}


struct CopyIndexParms
{
	CSoupIndex *	soupIndex;
	size_t			idMapLen;
	PSSIdMapping *	idMap;
	BOOL				isTagsIndex;
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

int	// NewtonErr?
CopyIndexStopFn(SKey * inFromKey, SKey * inToKey, void * inParms)
{
	CopyIndexParms * parms = (CopyIndexParms *) inParms;	// r6
	long theKey = (parms->isTagsIndex) ? (long) *inFromKey : (long) *inToKey;	// sp00
	PSSIdMapping * idMap = (PSSIdMapping *) bsearch(&theKey, parms->idMap, parms->idMapLen, sizeof(PSSIdMapping), ComparePSSIdMapping);
	SKey * srcKey = (parms->isTagsIndex) ? (SKey *)(&idMap->id2) : inToKey;
	SKey * dstKey = (parms->isTagsIndex) ? inFromKey : (SKey *)(&idMap->id2);
	parms->soupIndex->addInTransaction(srcKey, dstKey);

	int result = 0;
	if (parms->cbInterval && --parms->cbCount == 0)
	{
		// do progress callback
		CTime now = GetGlobalTime();
		Timeout delta = now - parms->cbTime;
		if (delta > parms->cbInterval)
		{
			DoBlock(parms->cbCodeblock, RA(NILREF));
			parms->cbTime = GetGlobalTime();
			parms->cbCount = 100;
		}
	}
	return result;
}


void
CopySoupIndexes(RefArg inFromSoup, RefArg inToSoup, PSSIdMapping * inIdMap, size_t inIdMapLen, RefArg inCallbackFn, Timeout inCallbackInterval)
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

	int i, count = Length(srcIndexes);
	for (i = 0; i < count; i++)
	{
		indexDesc = GetArraySlot(dstIndexes, i);
		CSoupIndex * dstSoupIndex = GetSoupIndexObject(inToSoup, RINT(GetFrameSlot(indexDesc, SYMA(index))));
		indexDesc = GetArraySlot(srcIndexes, i);
		CSoupIndex * srcSoupIndex = GetSoupIndexObject(inFromSoup, RINT(GetFrameSlot(indexDesc, SYMA(index))));

		parms.soupIndex = dstSoupIndex;
		parms.isTagsIndex = EQRef(GetFrameSlot(indexDesc, SYMA(type)), RSYMtags);

		NewtonErr err;
		if ((err = srcSoupIndex->search(1, NULL, NULL, CopyIndexStopFn, &parms, NULL, NULL)) < noErr)
			throw2(exStore, err);
		dstSoupIndex->nodeCache()->commit(dstSoupIndex);
	}
}


void
AbortSoupIndexes(RefArg inSoup)
{
	RefVar spec(GetFrameSlot(inSoup, SYMA(_proto)));
	RefVar indexes(GetFrameSlot(spec, SYMA(indexes)));
	RefVar indexDesc;
	int i, count = Length(indexes);
	for (i = count - 1; i >= 0; i--)
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
		throw2(exStore, kNSErrDuplicateIndex);

	//sp-04
	RefVar indexType(GetFrameSlot(indexSpec, SYMA(type)));	//sp00
	RefVar indexStructure(GetFrameSlot(indexSpec, SYMA(structure)));	// r8

	// validate
	BOOL isMultiSlot = NO;	// r9
	if (EQ(indexStructure, SYMA(multislot)))
	{
		isMultiSlot = YES;
		if (!(EQRef(ClassOf(indexPath), RSYMarray) && IsArray(indexType)))
			throw2(exStore, kNSErrBadIndexDesc);
		if (Length(indexPath) != Length(indexType))
			throw2(exStore, kNSErrBadIndexDesc);
		if (Length(indexPath) > 6)
			throw2(exStore, kNSErrBadIndexDesc);
	}
	else
	{
		if (!EQ(indexStructure, SYMA(slot)))
			throw2(exStore, kNSErrUnknownKeyStructure);
	}

	// sp-04
	RefVar indexDesc(TotalClone(indexSpec));	// sp00
	if (EQ(indexType, SYMA(tags)))
	{
		if (NOTNIL(GetTagsIndexDesc(inSoup)))
			throw2(exStore, kNSErrDuplicateIndex);
		if (ISNIL(GetFrameSlot(indexDesc, SYMA(tags))))
			SetFrameSlot(indexDesc, SYMA(tags), MakeArray(0));
	}

	BOOL isStringIndex = EQ(indexType, SYMA(string)) || (isMultiSlot && ISINT(FSetContains(RA(NILREF), indexType, SYMA(string))));
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
	CStoreWrapper * storeWrapper = (CStoreWrapper *) GetFrameSlot(inStore, SYMA(store));
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
		int i, count = Length(inIndexes);
		for (i = 0; i < count; i++)
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
	outInfo->x0C = 1;
	outInfo->x10 = EQRef(GetFrameSlot(indexDesc, SYMA(path)), RSYM_uniqueId) ? 0 : 2;
	outInfo->x14 = 0xFFFFFFFF;
	outInfo->x18 = 0xFF;
	outInfo->x19 = 0;
	outInfo->x1A = 0;

	ULong theType;
	RefVar indexType(GetFrameSlot(indexDesc, SYMA(type)));
	RefVar indexOrder(GetFrameSlot(indexDesc, SYMA(order)));
	int i, count;
	BOOL isAnArray = IsArray(indexType);
	if (isAnArray)
	{
		count = Length(indexType);
		outInfo->x08 = 6;
	}
	else
	{
		count = 1;
		if (NOTNIL(indexOrder) && !EQ(indexOrder, SYMA(ascending)))
			outInfo->x19 = 1;
	}
	RefVar spec;
	for (i = count - 1; i >= 0; i--)
	{
		spec = isAnArray ? GetArraySlot(indexType, i) : (Ref)indexType;
		if (EQ(spec, SYMA(string)))
			theType = 0;
		else if (EQ(spec, SYMA(int)))
			theType = 1;
		else if (EQ(spec, SYMA(real)))
			theType = 3;
		else if (EQ(spec, SYMA(char)))
			theType = 2;
		else if (EQ(spec, SYMA(symbol)))
			theType = 4;
		else if (!isAnArray && EQ(spec, SYMA(tags)))
		{
			theType = 1;
			outInfo->x0C = 5;
			outInfo->x10 = 0;
		}
		else
			throw2(exStore, kNSErrUnknownIndexType);

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
			outInfo->x08 = theType;
	}
}


void
IndexEntries(RefArg inSoup, RefArg indexDesc)
{
	CStoreWrapper * storeWrapper = (CStoreWrapper *) GetFrameSlot(inSoup, SYMA(TStore));	// r9
	RefVar tags;
	RefVar path;
	BOOL isTagSpec = EQRef(GetFrameSlot(indexDesc, SYMA(type)), RSYMtags);
	if (isTagSpec)
	{
		tags = GetFrameSlot(indexDesc, SYMA(tags));
		path = GetFrameSlot(indexDesc, SYMA(path));
	}
	//sp-54
	CSoupIndex * r7 = GetSoupIndexObject(inSoup, 0);
	CSoupIndex * r6 = GetSoupIndexObject(inSoup, RINT(GetFrameSlot(indexDesc, SYMA(index))));
	SKey sp004;
	PSSId objId;
		//sp-08
		RefVar sp0004(MakeFaultBlock(inSoup, storeWrapper, 0));
		RefVar sp0000;
	int result;
	for (result = r7->first(&sp004, (SKey *)&objId); result == 0; result = r7->next(&sp004, (SKey *)&objId, 0, &sp004, (SKey *)&objId))
	{
		FaultObject * obj = (FaultObject *) NoFaultObjectPtr(sp0004);
		obj->id = MAKEINT(objId);
		obj->object = NILREF;
		if (isTagSpec)
		{
			sp0000 = GetEntryKey(sp0004, path);
			if (NOTNIL(sp0000))
				AlterTagsIndex(1, *r6, objId, sp0000, inSoup, tags);
		}
		else
		{
			//sp-50
			SKey sp00000;
			if (GetEntrySKey(sp0004, indexDesc, &sp00000, NULL))
			{
				if (r6->addInTransaction(&sp00000, (SKey *)&objId) != noErr)
					throw2(exStore, kNSErrInternalError);
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
	int i, count = Length(indexes);
	RefVar indexObjs(AllocateFramesCObject(count * sizeof(CSoupIndex), GCDeleteIndexObjects, NULL, NULL));

	CSoupIndex * indexData = (CSoupIndex *) BinaryData(indexObjs);
	new (indexData) CSoupIndex[count];
	SetFrameSlot(inSoup, SYMA(indexObjects), indexObjs);

	CStoreWrapper * storeWrapper = (CStoreWrapper *) GetFrameSlot(inSoup, SYMA(TStore));
	RefVar indexDesc;
	for (i = 0; i < count; i++, indexData++)
	{
		indexDesc = GetArraySlot(indexes, i);
		indexData->init(storeWrapper, RINT(GetFrameSlot(indexDesc, SYMA(index))), GetIndexSortTable(indexDesc));
	}
	EachSoupCursorDo(inSoup, kSoupIndexesChanged);
}


void
GCDeleteIndexObjects(void *)
{ /* this really does nothing */ }


CSoupIndex *
GetSoupIndexObject(RefArg inSoup, PSSId inId)
{
	CSoupIndex * indexData = (CSoupIndex *) BinaryData(GetFrameSlot(inSoup, SYMA(indexObjects)));
	if (inId == 0)
		return indexData;

	RefVar spec(GetFrameSlot(inSoup, SYMA(_proto)));
	CSoupIndex * limit = indexData + Length(GetFrameSlot(spec, SYMA(indexes)));
	for ( ; indexData < limit; indexData++)
		if (indexData->indexId() == inId)
			return indexData;

	return NULL;
}
