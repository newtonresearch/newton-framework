/*
	File:		StreamObjects.cc

	Contains:	Implementation of Newton Streamed Objects.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "Frames.h"
#include "StreamObjects.h"
#include "LargeBinaries.h"
#include "ROMResources.h"


/*------------------------------------------------------------------------------
	L a r g e   B i n a r y   S t r e a m   D a t a
------------------------------------------------------------------------------*/

struct LBStreamData
{
	char		compressLB;
	size_t	streamSize;				// __attribute__((packed)) for 100% compatibility
	size_t	companderNameSize;
	size_t	companderParmSize;
	int32_t	reserved;
};


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

CPrecedentsForWriting * gPrecedentsForWriting = NULL;
CPrecedentsForReading * gPrecedentsForReading = NULL;
bool							gPrecedentsForWritingUsed = true;
bool							gPrecedentsForReadingUsed = true;


/*------------------------------------------------------------------------------
	C B u c k e t A r r a y
------------------------------------------------------------------------------*/

CBucketArray::CBucketArray(ArrayIndex inElementSize)
{
	fElementSize = inElementSize;
	fNumOfElements = 0;
	fNumOfBuckets = 0;
	fBucketBlock = NULL;
}


CBucketArray::~CBucketArray()
{
	ArrayIndex	i;
	void **		p;
	for (p = fBucketBlock, i = 0; i < fNumOfBuckets; ++i, ++p)
		FreePtr((Ptr)*p);
	FreePtr((Ptr)fBucketBlock);
}


void *
CBucketArray::elementAt(ArrayIndex index)
{
	if (/*index < 0 ||*/ index >= fNumOfElements)
		ThrowErr(exFrames, kNSErrOutOfRange);
	return (char *)fBucketBlock[index/kBucketSize]		// the right bucket
		  + fElementSize * (index & (kBucketSize-1));	// element within bucket
}


void
CBucketArray::setArraySize(ArrayIndex inSize)
{
	ArrayIndex	bucketIndex = inSize / kBucketSize + 1;
	if (bucketIndex < fNumOfBuckets)
	{
		// shrink array:
		// free excess buckets
		for (ArrayIndex i = bucketIndex; i < fNumOfBuckets; ++i)
			FreePtr((Ptr)fBucketBlock[i]);
		// reallocate block
		void **	p = (void **)ReallocPtr((Ptr)fBucketBlock, bucketIndex * sizeof(void*));
		if (p == NULL)
			OutOfMemory();
		fBucketBlock = p;
		fNumOfBuckets = bucketIndex;
	}
	else if (bucketIndex > fNumOfBuckets)
	{
		// grow array:
		// reallocate block
		void **	p = (void **)ReallocPtr((Ptr)fBucketBlock, bucketIndex * sizeof(void*));
		if (p == NULL)
			OutOfMemory();
		fBucketBlock = p;
		for (ArrayIndex i = fNumOfBuckets; i < bucketIndex; ++i)
			if ((fBucketBlock[i] = NewPtr(fElementSize*kBucketSize)) == NULL)
				OutOfMemory();
		fNumOfBuckets = bucketIndex;
	}
	fNumOfElements = inSize;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C P r e c e d e n t s F o r R e a d i n g
------------------------------------------------------------------------------*/

CPrecedentsForReading::CPrecedentsForReading()
	:	CBucketArray(sizeof(Ref))
{
	reset();
	DIYGCRegister(this, CPrecedentsForReading::GCMark, CPrecedentsForReading::GCUpdate);
}


CPrecedentsForReading::~CPrecedentsForReading()
{
	DIYGCUnregister(this);
}


ArrayIndex
CPrecedentsForReading::add(RefArg inObj)
{
	ArrayIndex	index = count();
	setArraySize(index + 1);
	*get(index) = inObj;
	return index;
}


void
CPrecedentsForReading::replace(ArrayIndex index, RefArg inObj)
{
	*get(index) = inObj;
}


void
CPrecedentsForReading::reset(void)
{
	setArraySize(0);
}


void
CPrecedentsForReading::markAllRefs(void)
{
	for (ArrayIndex i = 0; i < count(); ++i)
	{
		Ref * p = get(i);
		DIYGCMark(*p);
	}
}


void
CPrecedentsForReading::updateAllRefs(void)
{
	for (ArrayIndex i = 0; i < count(); ++i)
	{
		Ref * p = get(i);
		*p = DIYGCUpdate(*p);
	}
}


void
CPrecedentsForReading::GCMark(void * inContext)
{
	static_cast<CPrecedentsForReading*>(inContext)->markAllRefs();
}


void
CPrecedentsForReading::GCUpdate(void * inContext)
{
	static_cast<CPrecedentsForReading*>(inContext)->updateAllRefs();
}

#pragma mark -

/*------------------------------------------------------------------------------
	C P r e c e d e n t s F o r W r i t i n g

	When writing, we have to convert pointer refs to self-relative offsets.
------------------------------------------------------------------------------*/

CPrecedentsForWriting::CPrecedentsForWriting()
	:	CBucketArray(sizeof(WrPrec))
{
	reset();
	GCRegister(this, CPrecedentsForWriting::GCOccurred);
	DIYGCRegister(this, CPrecedentsForWriting::GCMark, CPrecedentsForWriting::GCUpdate);
}


CPrecedentsForWriting::~CPrecedentsForWriting()
{
	DIYGCUnregister(this);
	GCUnregister(this);
}


ArrayIndex
CPrecedentsForWriting::add(RefArg inObj)
{
	ArrayIndex	index = count();
	// bump the array size
	setArraySize(index + 1);
	// last element refers to added ref
	get(index)->ref = inObj;
	// build links from the added ref
	generateLinks(index);
	// return index of added ref
	return index-1;
}


void
CPrecedentsForWriting::reset(void)
{
	// only one entry, the root…
	setArraySize(1);
	// which is NULL
	WrPrec *	p = get(0);
	p->ref = NILREF;
	p->f04 = 0;
	p->f09 = 0;
	p->highestBit = 31;
}


ArrayIndex
CPrecedentsForWriting::search(RefArg inObj)
{
	Ref	r7 = inObj;
	ArrayIndex	index;

	unsigned	bitNum = get(0)->highestBit;
	if ((r7 & (1 << bitNum)) != 0)
		index = get(0)->f09;
	else
		index = get(0)->f04;

	unsigned	r6 = get(0)->highestBit;
	while ((bitNum = get(index)->highestBit) < r6)
	{
		r6 = bitNum;
		bitNum = get(index)->highestBit;
		if ((r7 & (1 << bitNum)) != 0)
			index = get(index)->f09;
		else
			index = get(index)->f04;
	}

	return index;
}


ArrayIndex
CPrecedentsForWriting::find(RefArg inObj)
{
	ArrayIndex	index = search(inObj);
	if (inObj != get(index)->ref)
		index = 0;
	return index-1;
}


void
CPrecedentsForWriting::generateLinks(ArrayIndex index)
{
	RefVar	sp00(get(index)->ref);
	ArrayIndex	r1 = search(sp00);
	int		r7 = 31;
	Ref		r8 = sp00;
	unsigned	r0 = get(r1)->ref ^ r8;
	for ( ; (r0 & 0x80000000) == 0; r0 <<= 1)	// calc next highest bit set
		r7--;
	get(index)->highestBit = r7;

	unsigned	r6;
	unsigned	r9 = 0;
	unsigned	r10 = 0;

	r0 = get(0)->highestBit;
	if ((r8 & (1 << r0)) != 0)
		r6 = get(0)->f09;
	else
		r6 = get(0)->f04;

	r10 = get(0)->highestBit;
	while ((r0 = get(r6)->highestBit) > r7 && r0 < r10)
	{
		r9 = r6;
		r10 = r0;
		r0 = get(r6)->highestBit;
		if ((r8 & (1 << r0)) != 0)
			r6 = get(r6)->f09;
		else
			r6 = get(r6)->f04;
	}

	bool	xr7 = (r8 & (1 << r7)) != 0;
	get(index)->f04 = xr7 ? r6 : index;
	if (xr7)
		r6 = index;
	get(index)->f09 = r6;

	r0 = get(r9)->highestBit;
	if ((r8 & (1 << r0)) != 0)
		get(r9)->f09 = index;
	else
		get(r9)->f04 = index;
}


void
CPrecedentsForWriting::rebuildTable(void)
{
	if (count() > 1)
	{
		WrPrec *	p = get(0);
		p->f04 = 0;
		p->f09 = 0;
		// rebuild links from every object in the table
		for (ArrayIndex i = 1; i < count(); ++i)
			generateLinks(i);
	}
}


void
CPrecedentsForWriting::markAllRefs(void)
{
	for (ArrayIndex i = 1; i < count(); ++i)
	{
		WrPrec *	p = get(i);
		DIYGCMark(p->ref);
	}
}


void
CPrecedentsForWriting::updateAllRefs(void)
{
	for (ArrayIndex i = 1; i < count(); ++i)
	{
		WrPrec *	p = get(i);
		p->ref = DIYGCUpdate(p->ref);
	}
}


void
CPrecedentsForWriting::GCOccurred(void * inContext)
{
	static_cast<CPrecedentsForWriting*>(inContext)->rebuildTable();
}


void
CPrecedentsForWriting::GCMark(void * inContext)
{
	static_cast<CPrecedentsForWriting*>(inContext)->markAllRefs();
}


void
CPrecedentsForWriting::GCUpdate(void * inContext)
{
	static_cast<CPrecedentsForWriting*>(inContext)->updateAllRefs();
}

#pragma mark -

/*------------------------------------------------------------------------------
	F l a t t e n i n g
------------------------------------------------------------------------------*/

// outPackedRect MUST be non-NULL
bool
PackSmallRect(RefArg inRect, ULong * outPackedRect)
{
	Ref	value;
	ULong	byteValue;
	*outPackedRect = 0;
	if (Length(inRect) == 4)
	{
		value = GetFrameSlot(inRect, SYMA(top));
		if (ISINT(value) && (byteValue = RVALUE(value)) <= 255)
		{
			*outPackedRect = byteValue;
			value = GetFrameSlot(inRect, SYMA(left));
			if (ISINT(value) && (byteValue = RVALUE(value)) <= 255)
			{
				*outPackedRect = (*outPackedRect << 8) | byteValue;
				value = GetFrameSlot(inRect, SYMA(bottom));
				if (ISINT(value) && (byteValue = RVALUE(value)) <= 255)
				{
					*outPackedRect = (*outPackedRect << 8) | byteValue;
					value = GetFrameSlot(inRect, SYMA(right));
					if (ISINT(value) && (byteValue = RVALUE(value)) <= 255)
					{
						*outPackedRect = (*outPackedRect << 8) | byteValue;
						return true;
					}
				}
			}
		}
	}
	return false;
}


Ref
UnpackSmallRect(ULong inPackedRect)
{
	RefVar	rect(Clone(RA(canonicalRect)));
	Ref		r;
	Ref *		RSr = &r;	// mock up a RefVar

	r = MAKEINT(inPackedRect & 0xFF);
	SetFrameSlot(rect, SYMA(right), RA(r));
	inPackedRect >>= 8;
	r = MAKEINT(inPackedRect & 0xFF);
	SetFrameSlot(rect, SYMA(bottom), RA(r));
	inPackedRect >>= 8;
	r = MAKEINT(inPackedRect & 0xFF);
	SetFrameSlot(rect, SYMA(left), RA(r));
	inPackedRect >>= 8;
	r = MAKEINT(inPackedRect & 0xFF);
	SetFrameSlot(rect, SYMA(top), RA(r));

	return rect;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C O b j e c t R e a d e r
------------------------------------------------------------------------------*/

CObjectReader::CObjectReader(CPipe & inPipe)
	:	fPipe(inPipe)
{
	fStore = NULL;
	fAllowFunctions = true;
	setPrecedentsForReading();
}


CObjectReader::CObjectReader(CPipe & inPipe, RefArg inStore)
	:	fPipe(inPipe)
{
	fStore = NULL;
	fAllowFunctions = true;
	setPrecedentsForReading();

	CStoreWrapper *	wrapper;
	if (NOTNIL(inStore)
	&&  NOTNIL(wrapper = (CStoreWrapper *)GetFrameSlot(inStore, SYMA(store))))
		fStore = wrapper->store();
}


CObjectReader::~CObjectReader()
{
	if (fPrecedents == gPrecedentsForReading)
	{
		gPrecedentsForReadingUsed = false;
		fPrecedents->reset();
	}
	else if (fPrecedents != NULL)
		delete fPrecedents;
}


void
CObjectReader::setPrecedentsForReading(void)
{
	if (gPrecedentsForReadingUsed)
	{
		if ((fPrecedents = new CPrecedentsForReading) == NULL)
			OutOfMemory();
	}
	else
	{
		fPrecedents = gPrecedentsForReading;
		gPrecedentsForReadingUsed = true;
	}
}


void
CObjectReader::setFunctionsAllowed(bool inAllowed)
{
	fAllowFunctions = inAllowed;
}


Ref
CObjectReader::read(void)
{
	unsigned char	streamVersion;
	fPipe >> streamVersion;
	if (streamVersion != kNSOFVersion)
		ThrowOSErr(kNSErrUnknownStreamFormat);
	return scan();
}


ArrayIndex
CObjectReader::size(void)
{
	long				here = fPipe.readPosition();
	ArrayIndex		objSize;
	unsigned char	streamVersion;
	fPipe >> streamVersion;
	if (streamVersion != kNSOFVersion)
		ThrowOSErr(kNSErrUnknownStreamFormat);
	objSize = sizeof(streamVersion) + scanSize();
	fPipe.readSeek(here, SEEK_SET);
	return objSize;
}


ArrayIndex
CObjectReader::scanSize(void)
{
	int				symSize;
	ArrayIndex		skipSize = 0;
	ArrayIndex		objSize;
	unsigned char	objType;
	fPipe >> objType;
	switch (objType)
	{
	case kNSImmediate:
		objSize = sizeFromPipe(NULL);			// value
		break;

	case kNSCharacter:
		objSize = skipSize = sizeof(unsigned char);
		break;

	case kNSUnicodeCharacter:
		objSize = skipSize = sizeof(UniChar);
		break;

	case kNSBinaryObject:
	case kNSString:
		{
			int	numOfChars;
			objSize = sizeFromPipe(&numOfChars);	// string length

			if (objType != kNSString)
				objSize += scanSize();			// class

			objSize += numOfChars;
			skipSize = numOfChars;
		}
		break;

	case kNSArray:
	case kNSPlainArray:
		{
			ArrayIndex	i, numOfSlots;
			objSize = sizeFromPipe((int *)&numOfSlots);

			if (objType != kNSPlainArray)
				objSize += scanSize();			// class

			for (i = 0; i < numOfSlots; ++i)
				objSize += scanSize();			// slots
		}
		break;

	case kNSFrame:
		{
			ArrayIndex	i, numOfSlots;
			objSize = sizeFromPipe((int *)&numOfSlots);

			for (i = 0; i < numOfSlots; ++i)
				objSize += scanSize();			// tags
			for (i = 0; i < numOfSlots; ++i)
				objSize += scanSize();			// slots
		}
		break;

	case kNSSymbol:
		objSize = sizeFromPipe(&symSize);	// symbol length
		objSize += symSize;
		break;

	case kNSPrecedent:
		objSize = sizeFromPipe(NULL);		// index
		break;

	case kNSNIL:
		objSize = 0;
		break;

	case kNSSmallRect:
		objSize = skipSize = 4;
		break;

	case kNSLargeBinary:
		objSize = countLargeBinary(&skipSize);
		break;

	default:
		objSize = 0;	// placate the compiler
		ThrowErr(exFrames, kNSErrBadStream);
	}
	if (skipSize > 0)
		fPipe.readSeek(skipSize, SEEK_CUR);
	return sizeof(objType) + objSize;
}


Ref
CObjectReader::scan(void)
{
	RefVar			obj;
	unsigned char	objType;
	fPipe >> objType;
	switch (objType)
	{
	case kNSImmediate:
		{
			Ref	value = longFromPipe();
			if (ISREALPTR(value))
				ThrowErr(exFrames, kNSErrBadStream);
			obj = value;
		}
		break;

	case kNSCharacter:
		{
			unsigned char	value;
			fPipe >> value;
			obj = MAKECHAR(value);
		}
		break;

	case kNSUnicodeCharacter:
		{
			UniChar	value;
			fPipe >> value;
			obj = MAKECHAR(value);
		}
		break;

	case kNSBinaryObject:
	case kNSString:
		{
			bool isEOF;
			size_t objSize = longFromPipe();

			if (objType == kNSString)
			{
				obj = AllocateBinary(SYMA(string), objSize);
				fPrecedents->add(obj);
			}
			else
			{
				obj = AllocateBinary(NILREF, objSize);
				fPrecedents->add(obj);
				((BinaryObject *)ObjectPtr(obj))->objClass = scan();
			}
			fPipe.readChunk(BinaryData(obj), objSize, isEOF);
#if defined(hasByteSwapping)
			if (IsInstance(obj, SYMA(string)))
			{
				// byte-swap unichars in the string
				UniChar * s = (UniChar *)BinaryData(obj);
				for (ArrayIndex i = objSize/sizeof(UniChar); i > 0; --i, ++s)
					*s = BYTE_SWAP_SHORT(*s);
			}
			else if (IsReal(obj))
			{
				// byte-swap double == 64 bits
				uint32_t * p = (uint32_t *)BinaryData(obj);
				uint32_t p1 = p[1];
				p[1] = BYTE_SWAP_LONG(*p);
				p[0] = BYTE_SWAP_LONG(p1);
			}
			// can byte-swap other binary classes here too
#endif
		}
		break;

	case kNSArray:
	case kNSPlainArray:
		{
			ArrayIndex numOfSlots = longFromPipe();
			obj = MakeArray(numOfSlots);
			fPrecedents->add(obj);

			if (objType != kNSPlainArray)
				((ArrayObject *)ObjectPtr(obj))->objClass = scan();

			for (ArrayIndex i = 0; i < numOfSlots; ++i)
				SetArraySlot(obj, i, scan());
		}
		break;

	case kNSFrame:
		{
			ArrayIndex numOfSlots = longFromPipe();
			ArrayIndex index = fPrecedents->add(RA(NILREF));	// placeholder

			RefVar tags(MakeArray(numOfSlots));
			for (ArrayIndex i = 0; i < numOfSlots; ++i)
				SetArraySlot(tags, i, scan());

			RefVar map(AllocateMapWithTags(RA(NILREF), tags));
			obj = AllocateFrameWithMap(map);
			fPrecedents->replace(index, obj);

			for (ArrayIndex i = 0; i < numOfSlots; ++i)
				SetArraySlot(obj, i, scan());
		}
		break;

	case kNSSymbol:
		{
			bool		isEOF;
			char		symBuf[256];
			size_t	symLen = longFromPipe();
			if (symLen > 255)
				ThrowErr(exFrames, kNSErrBadStream);

			fPipe.readChunk(symBuf, symLen, isEOF);
			symBuf[symLen] = 0;

			obj = MakeSymbol(symBuf);
			fPrecedents->add(obj);
		}
		break;

	case kNSPrecedent:
		{
			ArrayIndex	index = longFromPipe();
			obj = *fPrecedents->get(index);
		}
		break;

	case kNSNIL:
		obj = NILREF;
		break;

	case kNSSmallRect:
		{
			ULong	value;
			fPipe >> value;
			obj = UnpackSmallRect(value);
			fPrecedents->add(obj);
		}
		break;

	case kNSLargeBinary:
		obj = readLargeBinary();
		break;

	default:
		ThrowErr(exFrames, kNSErrBadStream);
	}
	if (!fAllowFunctions
	&&  IsFunction(obj))
		ThrowErr(exFrames, kNSErrFuncInStream);
	return obj;
}


Ref
CObjectReader::readLargeBinary(void)
{
	if (fStore == NULL)
		ThrowOSErr(kNSErrInvalidStore);

	ArrayIndex	index = fPrecedents->add(NILREF);	// placeholder

	RefVar			largeBinary;
	RefVar			lbClass(scan());
	LBStreamData	lb;
	char *			companderName;
	void *			companderParms;

	fPipe >> lb.compressLB;
	fPipe >> lb.streamSize;
	fPipe >> lb.companderNameSize;
	fPipe >> lb.companderParmSize;
	fPipe >> lb.reserved;

	companderName = NULL;
	if (lb.companderNameSize != 0)
	{
		if ((companderName = NewPtr(lb.companderNameSize+1)) == NULL)
			OutOfMemory();
	}
	companderParms = NULL;
	if (lb.companderParmSize != 0)
	{
		if ((companderParms = NewPtr(lb.companderParmSize)) == NULL)
		{
			if (companderName != NULL)
				FreePtr(companderName);
			OutOfMemory();
		}
	}

	NewtonErr	err = noErr;
	PSSId			id;
	VAddr			addr;

	newton_try
	{
		bool	isEOF;
		if (lb.companderNameSize != 0)
		{
			fPipe.readChunk(companderName, lb.companderNameSize, isEOF);
			companderName[lb.companderNameSize] = 0;
#if 1
			companderName[0] = 'C';
#endif
		}
		if (lb.companderParmSize != 0)
		{
			fPipe.readChunk(companderParms, lb.companderParmSize, isEOF);
		}
		err = CreateLargeObject(&id, fStore, &fPipe, lb.streamSize, false,
				companderName,
				companderParms, lb.companderParmSize,
				NULL, lb.compressLB);
	}
	end_try;
	if (err != noErr)
		ThrowErr(exFrames, err);

	err = MapLargeObject(&addr, fStore, id, false);
	if (err != noErr)
	{
		DeleteLargeObject(fStore, id);
		ThrowErr(exFrames, err);
	}

	largeBinary = WrapLargeObject(fStore, id, lbClass, addr);
	fPrecedents->replace(index, largeBinary);

	return largeBinary;
}


ArrayIndex
CObjectReader::countLargeBinary(ArrayIndex * outUnscannedSize)
{
	ArrayIndex		classSize = scanSize();
	LBStreamData	lb;

	fPipe >> lb.compressLB;
	fPipe >> lb.streamSize;
	fPipe >> lb.companderNameSize;
	fPipe >> lb.companderParmSize;
	fPipe >> lb.reserved;

	ArrayIndex	unscannedSize = lb.streamSize + lb.companderNameSize + lb.companderParmSize;
	if (outUnscannedSize != NULL)
		*outUnscannedSize = unscannedSize;
	return classSize + 17 /*sizeof(_packed_LBStreamData)*/ + unscannedSize;
}


int
CObjectReader::longFromPipe(void)
{
	int				value;
	unsigned char	byteValue;

	fPipe >> byteValue;
	if (byteValue == 0xFF)
		fPipe >> value;
	else
		value = byteValue;

	return value;
}


ArrayIndex
CObjectReader::sizeFromPipe(int * outValue)
{
	ArrayIndex		objSize;
	int				value;
	unsigned char	byteValue;

	fPipe >> byteValue;
	if (byteValue == 0xFF)
	{
		objSize = 5;
		fPipe >> value;
	}
	else
	{
		objSize = 1;
		value = byteValue;
	}

	if (outValue != NULL)
		*outValue = value;
	return objSize;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C O b j e c t W r i t e r
------------------------------------------------------------------------------*/

CObjectWriter::CObjectWriter(RefArg inObj, CPipe & inPipe, bool inFollowProtos)
	:	fPipe(inPipe)
{
	fObj = inObj;
	fObjSize = 0;
	fFollowProtos = inFollowProtos;
	fCompressLB = false;
	if (gPrecedentsForWritingUsed)
	{
		if ((fPrecedents = new CPrecedentsForWriting) == NULL)
			OutOfMemory();
	}
	else
	{
		fPrecedents = gPrecedentsForWriting;
		gPrecedentsForWritingUsed = true;
	}
}


CObjectWriter::~CObjectWriter()
{
	if (fPrecedents == gPrecedentsForWriting)
	{
		gPrecedentsForWritingUsed = false;
		fPrecedents->reset();
	}
	else if (fPrecedents != NULL)
		delete fPrecedents;
}


void
CObjectWriter::setCompressLargeBinaries(void)
{
	fCompressLB = true;
}


ArrayIndex
CObjectWriter::size(void)
{
	if (fObjSize == 0)
	{
		fPrecedents->reset();
		prescan();
		fObjSize += sizeof(unsigned char);	// for version
	}
	return fObjSize;
}


ArrayIndex
CObjectWriter::prescan(RefArg inObject)
{
	ArrayIndex	objSize, savedSize = fObjSize;
	*(fStack.top)++ = fObj;
	fObjSize = 0;
	fObj = inObject;
	prescan();
	fObj = *(--fStack.top);
	objSize = fObjSize;
	fObjSize = savedSize;
	return objSize;
}


void
CObjectWriter::prescan(void)
{
	if (ISPTR(fObj))
	{
		ArrayIndex	index;
		if ((index = fPrecedents->find(fObj)) != -1)		// precedent
			fObjSize = 1 + longToSize(index);

		else
		{
			fPrecedents->add(fObj);
			ULong flags = ObjectFlags(fObj);
			if ((flags & kObjSlotted) != 0)
			{
				if ((flags & kObjFrame) != 0)
				{
					ULong	packedRect;
					if (PackSmallRect(fObj, &packedRect))	// small rect
						fObjSize = 1 + 4;
					else												// frame
					{
						ArrayIndex	frameSize = 0;
						ArrayIndex	i, numOfSlots = Length(fObj);
						if (!fFollowProtos && FrameHasSlot(fObj, SYMA(_proto)))
							numOfSlots--;

						RefVar	map(((FrameObject*)ObjectPtr(fObj))->map);
						ArrayIndex	protoIndex = kIndexNotFound;
						for (i = 0; i < numOfSlots; ++i)
						{
							RefVar	tag(GetTag(map, i));
							if (!fFollowProtos && EQ(tag, SYMA(_proto)))
								protoIndex = i;
							else
								frameSize += prescan(tag);
						}
						for (i = 0; i < numOfSlots; ++i)
						{
							if (i != protoIndex)
								frameSize += prescan(GetArraySlot(fObj, i));
						}
						fObjSize = 1 + longToSize(numOfSlots) + frameSize;
					}
				}
				else if (IsArray(fObj))							// array
				{
					RefVar		arrayClass(ClassOf(fObj));
					ArrayIndex	classSize = EQ(arrayClass, SYMA(array)) ? 0 : prescan(arrayClass);
					ArrayIndex	arraySize = 0;
					for (ArrayIndex i = 0, numOfSlots = Length(fObj); i < numOfSlots; ++i)
						arraySize += prescan(GetArraySlot(fObj, i));
					fObjSize = 1 + longToSize(Length(fObj)) + classSize + arraySize;
				}
			}
			else if ((flags & kObjMask) == kIndirectBinaryObject)
			{
				countLargeBinary();
			}
			else if (IsSymbol(fObj))	// symbol
			{
				ArrayIndex	symLen = strlen(SymbolName(fObj));
				fObjSize = 1 + longToSize(symLen) + symLen;
			}
			else								// binary
			{
				RefVar	binClass(ClassOf(fObj));
				ArrayIndex	classSize = EQ(binClass, SYMA(string)) ? 0 : prescan(binClass);
				fObjSize = 1 + longToSize(Length(fObj)) + classSize + Length(fObj);
			}
		}
	}

	else if (ISNIL(fObj))	// NIL
		fObjSize = 1;

	else if (ISCHAR(fObj))	// character
		fObjSize = 1 + ((RIMMEDVALUE(fObj) < 256) ? sizeof(char) : sizeof(UniChar));

	else							// immediate
		fObjSize = 1 + longToSize(fObj);
}


void
CObjectWriter::countLargeBinary(void)
{
	IndirectBinaryObject *	vbo = (IndirectBinaryObject *)ObjectPtr(fObj);
	if (vbo->procs == &gLBProcs)
	{
		LBData *				largeBinary = (LBData *)&vbo->data;
		CStoreWrapper *	wrapper = largeBinary->getStore();
		size_t		companderNameSize;
		size_t		companderParmSize;
		size_t		streamSize;
		LOCompanderNameStrLen(wrapper->store(), largeBinary->fId, &companderNameSize);
		LOCompanderParameterSize(wrapper->store(), largeBinary->fId, &companderParmSize);
		streamSize = LOSizeOfStream(wrapper->store(), largeBinary->fId, fCompressLB);

		fObjSize = 1								// kNSLargeBinary
					+ prescan(ClassOf(fObj))	// class
					+ 1								// fCompressLB
					+ 4								// streamSize
					+ 4								// companderNameSize
					+ 4								// companderParmSize
					+ 4								// reserved
					+ companderNameSize			// companderName
					+ companderParmSize			// companderParms
					+ streamSize;					// stream
	}
	else
		fObjSize = 1 + longToSize(kUnstreamableObject);
}


void
CObjectWriter::write(void)
{
	fPrecedents->reset();
	fPipe << (unsigned char) kNSOFVersion;
	scan();
}


void
CObjectWriter::scan(RefArg inObject)
{
	*(fStack.top)++ = fObj;
	fObj = inObject;
	scan();
	fObj = *(--fStack.top);
}


void
CObjectWriter::scan(void)
{
	if (ISPTR(fObj))
	{
		ArrayIndex	index;
		if ((index = fPrecedents->find(fObj)) != -1)		// precedent
		{
			fPipe << (unsigned char) kNSPrecedent;
			longToPipe(index);
		}

		else
		{
			fPrecedents->add(fObj);
			ULong flags = ObjectFlags(fObj);
			if ((flags & kObjSlotted) != 0)
			{
				if ((flags & kObjFrame) != 0)
				{
					ULong	packedRect;
					if (PackSmallRect(fObj, &packedRect))	// small rect
					{
						fPipe << (unsigned char) kNSSmallRect;
						fPipe << packedRect;
					}
					else												// frame
					{
						fPipe << (unsigned char) kNSFrame;

						ArrayIndex	numOfSlots = Length(fObj);
						if (!fFollowProtos && FrameHasSlot(fObj, SYMA(_proto)))
							numOfSlots--;
						longToPipe(numOfSlots);

						RefVar	map(((FrameObject*)ObjectPtr(fObj))->map);
						ArrayIndex	i, protoIndex = kIndexNotFound;
						for (i = 0; i < numOfSlots; ++i)
						{
							RefVar	tag(GetTag(map, i));
							if (!fFollowProtos && EQ(tag, SYMA(_proto)))
								protoIndex = i;
							else
								scan(tag);
						}
						for (i = 0; i < numOfSlots; ++i)
						{
							if (i != protoIndex)
								scan(GetArraySlot(fObj, i));
						}
					}
				}
				else if (IsArray(fObj))							// array
				{
					RefVar	arrayClass(ClassOf(fObj));
					unsigned char	objType = EQ(arrayClass, SYMA(array)) ? kNSPlainArray : kNSArray;
					fPipe << objType;

					ArrayIndex	i, numOfSlots = Length(fObj);
					longToPipe(numOfSlots);
					if (objType == kNSArray)
						scan(arrayClass);
					for (i = 0; i < numOfSlots; ++i)
						scan(GetArraySlot(fObj, i));
				}
			}
			else if ((flags & kObjMask) == kIndirectBinaryObject)
			{
				writeLargeBinary();
			}
			else if (IsSymbol(fObj))	// symbol
			{
				fPipe << (unsigned char) kNSSymbol;

				const char *	sym = SymbolName(fObj);
				ArrayIndex	symLen = strlen(sym);
				longToPipe(symLen);
				fPipe.writeChunk(sym, symLen, false);
			}
			else								// binary
			{
				RefVar	binaryClass(ClassOf(fObj));
				unsigned char	objType = EQ(binaryClass, SYMA(string)) ? kNSString : kNSBinaryObject;
				fPipe << objType;

				void *	objPtr = BinaryData(fObj);
				ArrayIndex	objSize = Length(fObj);
				longToPipe(objSize);
				if (objType == kNSBinaryObject)
					// scan class of binary object
					scan(binaryClass);
#if defined(hasByteSwapping)
				UniChar * s = NULL, buf[256];
				if (IsInstance(fObj, SYMA(string)))
				{
					// byte-swap unichars in the string
					if (objSize > (int)256 * sizeof(UniChar))
						s = (UniChar *)NewPtr(objSize);
					else
						s = buf;
					memmove(s, objPtr, objSize);
					objPtr = s;
					for (ArrayIndex i = objSize/sizeof(UniChar); i > 0; i--, s++)
						*s = BYTE_SWAP_SHORT(*s);
				}
				else if (IsReal(fObj))
				{
					// byte-swap double == 64 bits
					uint32_t * p = (uint32_t *)objPtr;
					uint32_t p1 = p[1];
					p[1] = BYTE_SWAP_LONG(*p);
					p[0] = BYTE_SWAP_LONG(p1);
				}
#endif
				fPipe.writeChunk(objPtr, objSize, false);
#if defined(hasByteSwapping)
				if (s != NULL && objPtr != buf)
					FreePtr((Ptr)objPtr);
#endif
			}
		}
	}

	else if (ISNIL(fObj))		// NIL
		fPipe << (unsigned char) kNSNIL;

	else if (ISCHAR(fObj))		// character
	{
		UniChar	ch = RIMMEDVALUE(fObj);
		if (ch < 256)
		{
			fPipe << (unsigned char) kNSCharacter;
			fPipe << (unsigned char) ch;
		}
		else
		{
			fPipe << (unsigned char) kNSUnicodeCharacter;
			fPipe << ch;
		}
	}

	else								// immediate
	{
		fPipe << (unsigned char) kNSImmediate;
		longToPipe(fObj);
	}
}


void
CObjectWriter::writeLargeBinary(void)
{
	IndirectBinaryObject *	vbo = (IndirectBinaryObject *)ObjectPtr(fObj);
	if (vbo->procs == &gLBProcs)
	{
		LBData * largeBinary = (LBData *)&vbo->data;
		CStoreWrapper * wrapper = largeBinary->getStore();
		size_t companderNameSize;
		char * companderName;
		size_t companderParmSize;
		void * companderParms;
		size_t streamSize;
		LOCompanderNameStrLen(wrapper->store(), largeBinary->fId, &companderNameSize);
		LOCompanderParameterSize(wrapper->store(), largeBinary->fId, &companderParmSize);
		streamSize = LOSizeOfStream(wrapper->store(), largeBinary->fId, fCompressLB);
		companderName = NULL;
		if (companderNameSize != 0)
		{
			if ((companderName = NewPtr(companderNameSize)) == NULL)	// string does not need to be terminated
				OutOfMemory();
			LOCompanderName(wrapper->store(), largeBinary->fId, companderName);
#if 1
			companderName[0] = 'T';
#endif
		}
		companderParms = NULL;
		if (companderParmSize != 0)
		{
			if ((companderParms = NewPtr(companderParmSize)) == NULL)
			{
				if (companderName != NULL)
					FreePtr(companderName);
				OutOfMemory();
			}
			LOCompanderParameters(wrapper->store(), largeBinary->fId, companderParms);
		}
		unwind_protect
		{
			fPipe << (unsigned char)kNSLargeBinary;

			scan(ClassOf(fObj));
			fPipe << (unsigned char)fCompressLB;
			fPipe << streamSize;
			fPipe << companderNameSize;
			fPipe << companderParmSize;
			fPipe << (int)0;
			if (companderName != NULL)
				fPipe.writeChunk(companderName, companderNameSize, false);
			if (companderParms != NULL)
				fPipe.writeChunk(companderParms, companderParmSize, false);
			LOWrite(&fPipe, wrapper->store(), largeBinary->fId, fCompressLB, NULL);
		}
		on_unwind
		{
			if (companderName != NULL)
				FreePtr(companderName);
			if (companderParms != NULL)
				FreePtr((Ptr)companderParms);
		}
		end_unwind;
	}
	else
	{
		fPipe << (unsigned char)kNSImmediate;
		longToPipe(kUnstreamableObject);
	}
}


void
CObjectWriter::longToPipe(int inValue)
{
	if (inValue < 0
	||  inValue > 254)
	{
		fPipe << (unsigned char) 255;
		fPipe << inValue;
	}
	else
		fPipe << (unsigned char) inValue;
}

