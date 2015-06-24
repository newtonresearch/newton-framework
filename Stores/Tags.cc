/*
	File:		Tags.cc

	Contains:	Soup index-tags implementation.

	Written by:	Newton Research Group, 2009.
*/

#include "Objects.h"
#include "ROMResources.h"
#include "Unicode.h"
#include "RichStrings.h"
#include "UStringUtils.h"
#include "StoreWrapper.h"
#include "Indexes.h"
#include "PlainSoup.h"

extern "C" {
Ref	FSetContains(RefArg inRcvr, RefArg inArray, RefArg inMember);
}


/*------------------------------------------------------------------------------
	S K e y
------------------------------------------------------------------------------*/

void
SKey::set(ArrayIndex inSize, void * inKey)
{
	if (inSize > kSKeyBufSize)
		inSize = kSKeyBufSize;
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
SKey::operator=(int inKey)
{
	if (MISALIGNED(this))
		memmove(this, &inKey, sizeof(inKey));
	else
		*(int*)this = inKey;
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


SKey::operator int() const
{
	if (MISALIGNED(this))
	{
		int value;
		memmove(&value, this, sizeof(value));
		return value;
	}
	return *(int*)this;
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


bool
SKey::equals(const SKey & inKey) const
{
	return length == inKey.length
		 && flagBits == inKey.flagBits
		 && memcmp(buf, inKey.buf, length) == 0;
}


/*------------------------------------------------------------------------------
	S K e y   F u n c t i o n s
------------------------------------------------------------------------------*/

Ref
GetEntryKey(RefArg inEntry, RefArg inPath)
{
	if (EQRef(ClassOf(inPath), RSYMarray))
	{
		RefVar aKey;
		bool isKeyObjValid = NO;
		ArrayIndex count = Length(inPath);
		RefVar keyObj(MakeArray(count));
		for (ArrayIndex i = 0; i < count; ++i)
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


bool
GetEntrySKey(RefArg inEntry, RefArg inQuerySpec, SKey * outKey, bool * outIsComplexKey)
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
		ArrayIndex keyLen = inKey.size();
		ArrayIndex keyStrLen = keyLen + sizeof(UniChar);
		keyObj = AllocateBinary(SYMA(string), keyStrLen);
		char * keyObjData = BinaryData(keyObj);
		memmove(keyObjData, inKey.data(), keyLen);
		*(UniChar *)(keyObjData + keyLen) = 0;	// terminate the string
		*keySize = keyStrLen;
	}

	else if (EQ(inType, SYMA(int)))
	{
		*keySize = sizeof(int);
		keyObj = MAKEINT((int)inKey);
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
		ArrayIndex keyLen = inKey.size();
		ArrayIndex keySymLen = keyLen + sizeof(char);
		char * keyObjData = NewPtr(keySymLen);
		memmove(keyObjData, inKey.data(), keyLen);
		*(keyObjData + keyLen) = 0;	// terminate the string
		keyObj = MakeSymbol(keyObjData);
		FreePtr(keyObjData);
		*keySize = keySymLen;
	}

	else if (IsArray(inType))
	{
		const void * aKey = inKey.data();	// r5
		const void * aKeyLimit = (char *)inKey.data() + inKey.size();	// sp00
		short aKeyLen;
		ULong flags = inKey.flags();	// r9
		ArrayIndex count = Length(inType);
		keyObj = MakeArray(count);
		for (ArrayIndex i = 0; i < count; ++i)
		{
			if (aKey == aKeyLimit)	// we’re full
				break;
			if ((flags & 0x01) == 0)
			{
				SetArraySlot(keyObj, i, SKeyToKey(*(const SKey *)aKey, GetArraySlot(inType, i), &aKeyLen));
				aKey = (char *)aKey + WORDALIGN(aKeyLen);
			}
			flags >>= 1;
		}
	}

	else
		ThrowErr(exStore, kNSErrUnknownIndexType);

	return keyObj;
}


void
KeyToSKey(RefArg inKey, RefArg inType, SKey * outKey, short * outSize, bool * outIsComplexKey)
{
	bool isNonNumericKey = NO;	// assume it’s numeric

	if (IsArray(inType))
	{
		MultiKeyToSKey(inKey, inType, outKey);
		isNonNumericKey = YES;
	}

	else if (!((ISINT(inKey) && EQ(inType, SYMA(real))) || IsInstance(inKey, inType)))	// allow int keys where real specified
		ThrowErr(exStore, kNSErrKeyHasWrongType);

	else if (EQ(inType, SYMA(string)))
	{
		if (IsLargeBinary(inKey))
			ThrowErr(exStore, kNSErrVBOKey);
		if (IsRichString(inKey))
		{
			RichStringToSKey(inKey, outKey);
		}
		else
		{
			UniChar * s = (UniChar *)BinaryData(inKey);
			outKey->set(Ustrlen(s) * sizeof(UniChar), s);
		}
		isNonNumericKey = YES;
	}

	else if (EQ(inType, SYMA(int)))
	{
		*(int *)outKey = (int) RINT(inKey);
		if (outSize != NULL)
			*outSize = sizeof(int);
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
		const char * s = SymbolName(inKey);
		outKey->set(strlen(s), (void *)s);
		isNonNumericKey = YES;
	}

	else
		ThrowErr(exStore, kNSErrUnknownIndexType);

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
	char * keyData = (char *)outKey->data();
	bool isKeyAnArray = IsArray(inKey);
	for (ArrayIndex i = 0, count = isKeyAnArray ? Length(inKey) : 1; i < count; ++i)
	{
		keyObj = isKeyAnArray ? GetArraySlot(inKey, i) : (Ref)inKey;
		if (NOTNIL(keyObj))
		{
			//sp-5C
			SKey aKey;	// sp0C
			short aKeySize;	// sp08
			bool isComplexKey;	// sp04
			KeyToSKey(keyObj, GetArraySlot(inType, i), &aKey, &aKeySize, &isComplexKey);		// no, no, no -- surely have to verify that inType is an array?
			nextKeyLen = keyLen + WORDALIGN(aKeySize);
			if (nextKeyLen > kSKeyBufSize)
			{
				if (!isComplexKey)
					break;		// don’t need to bother with simple (numeric) keys
				aKeySize = kSKeyBufSize - keyLen;	// shorten the key to fit the available space
				if (aKeySize <= 2)
					break;	// no room for any more keys
				aKey.setSize(aKeySize - 2);
				nextKeyLen = kSKeyBufSize;
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
	ArrayIndex keyStrLen;
	UniChar * keyStr = (UniChar *)outKey->data();
	UniChar * s = (UniChar *)BinaryData(inStr);
	for (keyStrLen = 0; keyStrLen < kSKeyBufSize; keyStrLen += sizeof(UniChar))
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

#pragma mark STagsBits

/*------------------------------------------------------------------------------
	S T a g s B i t s
------------------------------------------------------------------------------*/

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


bool
STagsBits::validTest(const STagsBits & inTag, int inArg2) const
{
	if (inArg2 == 0)
		return equals(inTag);

	ArrayIndex tag1Size = size();
	ArrayIndex tag2Size = inTag.size();
	if (tag1Size != tag2Size)
	{
		if (tag1Size > tag2Size)
			tag1Size = tag2Size;
		else if (inArg2 == 1)
			return NO;
	}
	char * tag1 = (char *)data();
	char * tag2 = (char *)inTag.data();
	for (int i = tag1Size - 1; i >= 0; i--)
	{
		char ch1 = *tag1++;
		char ch2 = *tag2++;
		switch (inArg2)
		{
		case 1:
			if ((ch1 & ch2) != ch2)
				return NO;
			break;
		case 2:
			if ((ch1 & ch2) != 0)
				return YES;
			break;
		case 3:
			if ((ch1 & ch2) != 0)
				return NO;
			break;
		}
	}
	if (inArg2 != 2)
		return YES;
	return NO;
}

#pragma mark Tags

/*------------------------------------------------------------------------------
	T a g s
------------------------------------------------------------------------------*/

static bool
EncodeTags(RefArg inTagDesc, RefArg inSpec, STagsBits * outTagBits)
{
	outTagBits->setSize(0);
	bool allOK = YES;
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

	else if (IsSymbol(inSpec))
	{
		index = FSetContains(RA(NILREF), inTagDesc, inSpec);
		if (ISINT(index))
			outTagBits->setTag(RINT(index));
		else
			allOK = NO;
	}

	else
		ThrowExFramesWithBadValue(kNSErrNotASymbol, inSpec);

	return allOK;
}


static Ref
MakeTagBinary(STagsBits * inTagBits)
{
	ArrayIndex bitsLen = SKey::kOffsetToData + inTagBits->size();
	RefVar bits(AllocateBinary(SYMA(tags), bitsLen));
	memmove(BinaryData(bits), inTagBits, bitsLen);
	return bits;
}


static bool
EncodeTag(RefArg inTagDesc, RefArg inQuerySpec, RefArg inSym, int inType, RefArg outTagList)
{
	RefVar spec(GetFrameSlot(inQuerySpec, inSym));
	if (NOTNIL(spec))
	{
		STagsBits tagsBits;
		if (EncodeTags(inTagDesc, spec, &tagsBits)
		||  (inType != kTagsEqual && inType != kTagsAll))
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
	if (EncodeTag(tagDesc, inQuerySpec, SYMA(equal), kTagsEqual, tagList)
	&&  EncodeTag(tagDesc, inQuerySpec, SYMA(all), kTagsAll, tagList)
	&&  EncodeTag(tagDesc, inQuerySpec, SYMA(any), kTagsAny, tagList)
	&&  EncodeTag(tagDesc, inQuerySpec, SYMA(none), kTagsNone, tagList))
	{
		if (Length(tagList) == 0)
			ThrowErr(exStore, kNSErrInvalidTagSpec);
		return tagList;
	}
	return NILREF;
}


bool
TagsValidTest(CSoupIndex & index, RefArg inTags, PSSId inTagsId)
{
	if (ISNIL(inTags))
		return NO;

	SKey tagsKey;
	tagsKey = (int)inTagsId;
	STagsBits tagsData;
	int status = index.find(&tagsKey, &tagsKey, &tagsData, NO);
	if (status == 0)
	{
		ArrayIndex i, count = Length(inTags);
		for (i = 0; i < count; i += 2)
		{
			CDataPtr spm04(GetArraySlot(inTags, i+1));
			if (!tagsData.validTest(*(const STagsBits *)(char *)spm04, RINT(GetArraySlot(inTags, i))))
				return NO;
		}
		return YES;
	}
	else if (status == 2 || status == 3)
	{
		if (Length(inTags) != 2)
			return NO;
		int tagType = RINT(GetArraySlot(inTags, 0));
		if (tagType == kTagsNone)
			return YES;
		if (tagType == kTagsEqual)
		{
			SKey * key = (SKey *)BinaryData(GetArraySlot(inTags, 1));
			return key->size() == 0;
		}
		return NO;
	}
	else
		ThrowErr(exStore, kNSErrInvalidQueryType);
	return NO;	// just to keep the compiler quiet
}


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


ArrayIndex
CountTags(RefArg inTags)
{
	ArrayIndex count = 0;
	for (int i = Length(inTags) - 1; i >= 0; i--)
		if (NOTNIL(GetArraySlot(inTags, i)))
			count++;
	return count;
}

#pragma mark -

/*------------------------------------------------------------------------------
	T a g s   I n d e x e s
------------------------------------------------------------------------------*/

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
	||  (IsArray(inKey) && Length(inKey) > 0))
	{
		SKey tagsKey;
		tagsKey = (int) inId;
		STagsBits tagsData;
		if (!EncodeTags(inTags, inKey, &tagsData))
		{
			PlainSoupAddTags(inSoup, inKey);
			EncodeTags(inTags, inKey, &tagsData);
		}
		if (inSelector == 0)
			err = ioSoupIndex.Delete(&tagsKey, &tagsData);
		else
			err = ioSoupIndex.add(&tagsKey, &tagsData);
		if (err != noErr)
			ThrowErr(exStore, kNSErrInternalError);
	}
}


bool
UpdateTagsIndex(RefArg inSoup, RefArg indexDesc, RefArg inOldTags, RefArg inNewTags, PSSId inId)
{
	RefVar path(GetFrameSlot(indexDesc, SYMA(path)));
	RefVar oldKey(GetEntryKey(inOldTags, path));
	RefVar newKey(GetEntryKey(inNewTags, path));
	if (ISNIL(oldKey) && ISNIL(newKey))
		return NO;

	RefVar tags(GetFrameSlot(indexDesc, SYMA(tags)));
	STagsBits oldTagsData;
	STagsBits newTagsData;

	if (NOTNIL(oldKey))
		EncodeTags(tags, oldKey, &oldTagsData);

	if (NOTNIL(newKey))
	{
		if (!EncodeTags(tags, newKey, &newTagsData))
		{
			// encoding failed; try again but make sure we have the tags this time
			PlainSoupAddTags(inSoup, newKey);
			EncodeTags(tags, newKey, &newTagsData);
		}
	}

	if (!oldTagsData.equals(newTagsData))
	{
		// tags really are different; update the CSoupIndex
		CSoupIndex * soupIndex = GetSoupIndexObject(inSoup, RINT(GetFrameSlot(indexDesc, SYMA(index))));

		SKey tagsKey;
		tagsKey = (int) inId;

		NewtonErr err = noErr;
		XTRY
		{
			if (*(char *)oldTagsData.data() != 0)
				XFAIL(err = soupIndex->Delete(&tagsKey, &oldTagsData))
			if (*(char *)newTagsData.data() != 0)
				XFAIL(err = soupIndex->add(&tagsKey, &newTagsData))
		}
		XENDTRY;
		XDOFAIL(err)
		{
			ThrowErr(exStore, kNSErrInternalError);
		}
		XENDFAIL;
		return YES;
	}
	return NO;
}
