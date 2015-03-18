/*
	File:		PlainSoup.cc

	Contains:	Plain soup functions.
					A NewtonScript plain soup object looks like:
					{
						class: 'PlainSoup,
						theName: "Calls",		// or wahetever
						TStore: xxxx,			// (CStoreWrapper *)store object wrapper
						storeObj: {},			// (Ref) store to which this soup belongs
						...
						_parent:	{},			// plain soup functions
						_proto: {},				// signature, indexes and info
	
					}

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "Globals.h"
#include "Unicode.h"
#include "NewtonTime.h"
#include "UStringUtils.h"
#include "ObjHeader.h"
#include "RefMemory.h"
#include "FaultBlocks.h"
#include "Stores.h"
#include "Soups.h"
#include "Cursors.h"
#include "Entries.h"
#include "PlainSoup.h"
#include "OSErrors.h"

DeclareException(exBadType, exRootException);

extern void		AddToUnionSoup(RefArg name, RefArg soup);
extern void		RemoveFromUnionSoup(RefArg name, RefArg soup);

extern bool		IsRichString(RefArg inObj);
extern unsigned long	RealClock(void);


#pragma mark Indexes
/* -----------------------------------------------------------------------------
	Construct the index key that would be used for one or more specified values.
	Native function for soup._parent.MakeKey
	Args:		inRcvr		soup object frame
				inKey			string for which to make the key
				inPath		index path associated with that key value
	Return:	key object
----------------------------------------------------------------------------- */

Ref
PlainSoupMakeKey(RefArg inRcvr, RefArg inKey, RefArg inPath)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	RefVar indexDesc(IndexPathToIndexDesc(soupIndexInfo, inPath, NULL));
	if (ISNIL(indexDesc))
		ThrowErr(exStore, kNSErrNoSuchIndex);

	SKey sKey;
	RefVar indexType(GetFrameSlot(indexDesc, SYMA(type)));
	KeyToSKey(inKey, indexType, &sKey, NULL, NULL);
	return SKeyToKey(sKey, indexType, NULL);
}


/* -----------------------------------------------------------------------------
	Return all the indexes on this soup.
	Native function for soup._parent.GetIndexes
	Args:		inRcvr		soup object frame
	Return:	array of index spec frames
----------------------------------------------------------------------------- */

Ref
PlainSoupGetIndexes(RefArg inRcvr)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	RefVar soupIndexes(GetFrameSlot(soupIndexInfo, SYMA(indexes)));
	ArrayIndex count = Length(soupIndexes);
	RefVar indexes(MakeArray(count - 1));	// don’t need _uniqueId
	RefVar index;
	for (ArrayIndex i = 0, ii = 0; i < count; ++i)
	{
		index = GetArraySlot(soupIndexes, i);
		if (!EQRef(GetFrameSlot(index, SYMA(path)), RSYM_uniqueId))
			SetArraySlot(indexes, ii++, Clone(index));
	}
	return indexes;
}


/* -----------------------------------------------------------------------------
	Return the size of all indexes on this soup.
	Native function for soup._parent.IndexSizes
	Args:		inRcvr		soup object frame
	Return:	array of integer
----------------------------------------------------------------------------- */

Ref
PlainSoupIndexSizes(RefArg inRcvr)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	RefVar soupIndexes(GetFrameSlot(soupIndexInfo, SYMA(indexes)));
	ArrayIndex ii, count = Length(soupIndexes);
	RefVar indexSizes(MakeArray(count));
	RefVar index;
	for (ArrayIndex i = 0; i < count; ++i)
	{
		index = GetArraySlot(soupIndexes, i);
		ii = RINT(GetFrameSlot(index, SYMA(index)));
		SetArraySlot(indexSizes, i, MAKEINT(GetSoupIndexObject(inRcvr, ii)->totalSize()));
	}
	return indexSizes;
}


/* -----------------------------------------------------------------------------
	Add an index to this soup.
	Native function for soup._parent.AddIndex
	Args:		inRcvr		soup object frame
				index			index frame to add
	Return:	nil
----------------------------------------------------------------------------- */

Ref
PlainSoupAddIndex(RefArg inRcvr, RefArg index)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	if (NOTNIL(IndexPathToIndexDesc(soupIndexInfo, GetFrameSlot(index, SYMA(path)), NULL)))
	{
		CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(TStore));
		CheckWriteProtect(storeWrapper->store());

		RefVar soupIndexes(GetFrameSlot(soupIndexInfo, SYMA(indexes)));
		ArrayIndex originalNumOfIndexes = Length(soupIndexes);

		storeWrapper->lockStore();
		newton_try
		{
			RefVar newIndex(NewIndexDesc(soupIndexInfo, RefVar(GetFrameSlot(inRcvr, SYMA(storeObj))), index));
			AddArraySlot(soupIndexes, newIndex);
			SetFrameSlot(soupIndexInfo, SYMA(indexesModTime), MAKEINT(RealClock() & 0x1FFFFFFF));
			SoupChanged(soupIndexInfo, false);
			WriteFaultBlock(soupIndexInfo);
			CreateSoupIndexObjects(inRcvr);
			IndexEntries(inRcvr, newIndex);
		}
		newton_catch_all
		{
			if (Length(soupIndexes) != originalNumOfIndexes)
				SetLength(soupIndexes, originalNumOfIndexes);
			storeWrapper->abort();
		}
		end_try;
		storeWrapper->unlockStore();
	}
	return NILREF;
}


/* -----------------------------------------------------------------------------
	Remove an index from this soup.
	Native function for soup._parent.RemoveIndex
	Args:		inRcvr		soup object frame
				index			index frame to remove
	Return:	nil
----------------------------------------------------------------------------- */

Ref
PlainSoupRemoveIndex(RefArg inRcvr, RefArg index)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	if (EQ(index, SYMA(_uniqueId)))
		ThrowErr(exStore, kNSErrCantRemoveUIdIndex);

	int huh;
	RefVar indexDesc(IndexPathToIndexDesc(soupIndexInfo, index, &huh));
	if (ISNIL(indexDesc))
		ThrowErr(exStore, kNSErrNoSuchIndex);

	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(TStore));
	CheckWriteProtect(storeWrapper->store());

	ArrayIndex indexi = RINT(GetFrameSlot(indexDesc, SYMA(index)));

	storeWrapper->lockStore();
	CSoupIndex * soupIndex = GetSoupIndexObject(inRcvr, indexi);
	newton_try
	{
		soupIndex->destroy();
		storeWrapper->store()->deleteObject(indexi);
		RefVar soupIndexes(GetFrameSlot(soupIndexInfo, SYMA(indexes)));
		ArrayMunger(soupIndexes, indexi, 1, NILREF, 0, 0);
		SetFrameSlot(soupIndexInfo, SYMA(indexesModTime), MAKEINT(RealClock() & 0x1FFFFFFF));
		SoupChanged(soupIndexInfo, false);
		WriteFaultBlock(soupIndexInfo);
		RefVar sortId(GetFrameSlot(indexDesc, SYMA(sortId)));
		if (NOTNIL(sortId))
			StoreRemoveSortTable(GetFrameSlot(inRcvr, SYMA(storeObj)), RINT(sortId));
	}
	newton_catch_all
	{
		storeWrapper->abort();
		soupIndex->storeAborted();
	}
	end_try;
	storeWrapper->unlockStore();

	EachSoupCursorDo(inRcvr, kSoupIndexRemoved, indexDesc);
	CreateSoupIndexObjects(inRcvr);

	return NILREF;
}


#pragma mark Tags
/* -----------------------------------------------------------------------------
	Does this soup have any tags?
	Native function for soup._parent.HasTags
	Args:		inRcvr		soup object frame
	Return:	true => tags exist
----------------------------------------------------------------------------- */

Ref
PlainSoupHasTags(RefArg inRcvr)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	return MAKEBOOLEAN(NOTNIL(GetTagsIndexDesc(soupIndexInfo)));
}


/* -----------------------------------------------------------------------------
	Return the tags for this soup.
	Native function for soup._parent.GetTags
	Args:		inRcvr		soup object frame
	Return:	tags array
----------------------------------------------------------------------------- */

Ref
PlainSoupGetTags(RefArg inRcvr)
{
	RefVar tags;
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	RefVar tagsIndexDesc(GetTagsIndexDesc(soupIndexInfo));
	if (NOTNIL(tagsIndexDesc))
	{
		RefVar aTag;
		RefVar soupTags(GetFrameSlot(tagsIndexDesc, SYMA(tags)));
		tags = MakeArray(0);
		for (ArrayIndex i = 0, count = Length(soupTags); i < count; ++i)
		{
			aTag = GetArraySlot(soupTags, i);
			if (NOTNIL(aTag))
				AddArraySlot(tags, aTag);
		}
	}
	return tags;
}


/* -----------------------------------------------------------------------------
	Add tags to this soup.
	Native function for soup._parent.AddTags
	Args:		inRcvr		soup object frame
				inTags		array of tag symbols to add
	Return:	nil
----------------------------------------------------------------------------- */

Ref
PlainSoupAddTags(RefArg inRcvr, RefArg inTags)
{
	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(TStore));
	CheckWriteProtect(storeWrapper->store());

	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	RefVar tagsIndexDesc(GetTagsIndexDesc(soupIndexInfo));
	if (ISNIL(tagsIndexDesc))
		ThrowErr(exStore, kNSErrNoTags);

	RefVar soupTags(GetFrameSlot(tagsIndexDesc, SYMA(tags)));
	bool isTagsArray = IsArray(inTags);
	ArrayIndex i, numOfNewTags = isTagsArray ? Length(soupTags) : 1;
	if (CountTags(soupTags) + numOfNewTags > 624)
		ThrowErr(exStore, kNSErrInvalidTagsCount);

	bool isTagsChanged = false;
	if (isTagsArray)
		for (i = 0; i < numOfNewTags; ++i)
		{
			if (AddTag(soupTags, GetArraySlot(inTags, i)))
				isTagsChanged = true;
		}
	else
		isTagsChanged = AddTag(soupTags, inTags);

	if (isTagsChanged)
	{
		RefVar theTime(MAKEINT(RealClock() & 0x1FFFFFFF));
		SetFrameSlot(soupIndexInfo, SYMA(indexesModTime), theTime);
		SoupChanged(soupIndexInfo, false);
		WriteFaultBlock(soupIndexInfo);
		EachSoupCursorDo(inRcvr, kSoupTagsChanged);
	}

	return NILREF;
}


/* -----------------------------------------------------------------------------
	Remove tags from this soup.
	Native function for soup._parent.RemoveTags
	Args:		inRcvr		soup object frame
				inTags		array of tag symbols to remove
	Return:	nil
----------------------------------------------------------------------------- */

Ref
PlainSoupRemoveTags(RefArg inRcvr, RefArg inTags)
{
	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(TStore));
	CheckWriteProtect(storeWrapper->store());

	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	RefVar tagsIndexDesc(GetTagsIndexDesc(soupIndexInfo));
	if (ISNIL(tagsIndexDesc))
		ThrowErr(exStore, kNSErrNoTags);

	RefVar cursor(QueryEntriesWithTags(inRcvr, inTags));
	int i, count = Length(inTags);
	RefVar tagsPath(GetFrameSlot(tagsIndexDesc, SYMA(path)));
	bool isTagsPathSymbol = IsSymbol(tagsPath);
	RefVar theTime;

	OSERRIF(storeWrapper->lockStore());
	newton_try
	{
		RefVar soupTags;
		RefVar entry(CursorEntry(cursor));
		for ( ; NOTNIL(entry); entry = CursorNext(cursor))	// original says CursorEntry(cursor) -- but cf PlainSoupModifyTag() below
		{
			soupTags = GetFramePath(entry, tagsPath);
			bool isTagsEmpty = false;
			if (IsArray(soupTags))
			{
				for (i = count - 1; i >= 0; i--)
					ArrayRemove(soupTags, GetArraySlot(inTags, i));
				if (Length(soupTags) == 0)
					isTagsEmpty = true;
			}
			if (!IsArray(soupTags) || isTagsEmpty)
			{
				if (isTagsPathSymbol)
					RemoveSlot(entry, tagsPath);
				else
					SetFramePath(entry, tagsPath, RA(NILREF));
			}
			EntryChange(entry);
		}
		soupTags = GetFrameSlot(tagsIndexDesc, SYMA(tags));
		for (i = count - 1; i >= 0; i--)
		{
			RefVar index(FSetContains(RA(NILREF), soupTags, GetArraySlot(inTags, i)));
			if (ISINT(index))
				SetArraySlot(soupTags, RINT(index), RA(NILREF));
		}
		
		theTime = MAKEINT(RealClock() & 0x1FFFFFFF);
		SetFrameSlot(soupIndexInfo, SYMA(indexesModTime), theTime);
		SoupChanged(soupIndexInfo, false);
		WriteFaultBlock(soupIndexInfo);
	}
	newton_catch_all
	{
		OSERRIF(storeWrapper->abort());
	}
	end_try;
	OSERRIF(storeWrapper->unlockStore());

	EachSoupCursorDo(inRcvr, kSoupTagsChanged, theTime);
	return NILREF;
}


/* -----------------------------------------------------------------------------
	Modify a tag in this soup.
	Native function for soup._parent.ModifyTag
	Args:		inRcvr		soup object frame
				inOldTag		existing tag symbol
				inNewTag		required tag symbol
	Return:	nil
----------------------------------------------------------------------------- */

Ref
PlainSoupModifyTag(RefArg inRcvr, RefArg inOldTag, RefArg inNewTag)
{
	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(TStore));
	CheckWriteProtect(storeWrapper->store());

	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	RefVar tagsIndexDesc(GetTagsIndexDesc(soupIndexInfo));
	if (ISNIL(tagsIndexDesc))
		ThrowErr(exStore, kNSErrNoTags);

	if (EQ(inOldTag, inNewTag))
		return NILREF;

	RefVar soupTags(GetFrameSlot(tagsIndexDesc, SYMA(tags)));
	if (ISINT(FSetContains(RA(NILREF), soupTags, inNewTag)))
		ThrowErr(exStore, kNSErrInvalidTagSpec);

	RefVar tagIndex(FSetContains(RA(NILREF), soupTags, inOldTag));
	if (NOTINT(tagIndex))
		return NILREF;

	RefVar cursor(QueryEntriesWithTags(inRcvr, inOldTag));
	RefVar tagsPath(GetFrameSlot(tagsIndexDesc, SYMA(path)));
	RefVar theTags;
	RefVar index;

	OSERRIF(storeWrapper->lockStore());
	newton_try
	{
		RefVar entry(CursorEntry(cursor));
		for ( ; NOTNIL(entry); entry = CursorNext(cursor))
		{
			theTags = GetFramePath(entry, tagsPath);
			if (IsArray(theTags))
			{
				index = FSetContains(RA(NILREF), theTags, inOldTag);
				SetArraySlot(theTags, RINT(index), inNewTag);
			}
			else
				SetFramePath(entry, tagsPath, inNewTag);

			EntryChangeCommon(entry, 3);
		}
		SetArraySlot(soupTags, RINT(tagIndex), inNewTag);
		SetFrameSlot(soupIndexInfo, SYMA(indexesModTime), MAKEINT(RealClock() & 0x1FFFFFFF));
		SoupChanged(soupIndexInfo, false);
		WriteFaultBlock(soupIndexInfo);
	}
	newton_catch_all
	{
		OSERRIF(storeWrapper->abort());
	}
	end_try;
	OSERRIF(storeWrapper->unlockStore());

	return NILREF;
}


#pragma mark Soup info
/* -----------------------------------------------------------------------------
	Return a slot in the soup’s info frame.
	Native function for soup._parent.SetInfo
	Args:		inRcvr		soup object frame
				inTag			info frame slot
	Return:	info slot value
----------------------------------------------------------------------------- */

Ref
PlainSoupGetInfo(RefArg inRcvr, RefArg inTag)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	RefVar info;
	if (FrameHasSlot(soupIndexInfo, SYMA(info)))
		info = GetFrameSlot(GetFrameSlot(soupIndexInfo, SYMA(info)), inTag);
	return info;
}


/* -----------------------------------------------------------------------------
	Set a slot in the soup’s info frame.
	Native function for soup._parent.SetInfo
	Args:		inRcvr		soup object frame
				inTag			info frame slot
				inValue		info value
	Return:	nil
----------------------------------------------------------------------------- */

Ref
PlainSoupSetInfo(RefArg inRcvr, RefArg inTag, RefArg inValue)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	CheckWriteProtect(PlainSoupGetStore(inRcvr));

	RefVar value(TotalClone(inValue));
	RefVar tag(EnsureInternal(inTag));
	RefVar info;
	if (FrameHasSlot(soupIndexInfo, SYMA(info)))
		info = GetFrameSlot(soupIndexInfo, SYMA(info));
	else
		info = AllocateFrame();

	SetFrameSlot(info, tag, value);
	if (EQ(inTag, SYMA(NCKLastBackupTime)))
		SetFrameSlot(soupIndexInfo, SYMA(infoModTime), MAKEINT(RealClock() & 0x1FFFFFFF));

	SoupChanged(soupIndexInfo, false);
	WriteFaultBlock(soupIndexInfo);
	return NILREF;
}


/* -----------------------------------------------------------------------------
	Return the soup’s info frame.
	Native function for soup._parent.GetAllInfo
	Args:		inRcvr		soup object frame
	Return:	info frame
----------------------------------------------------------------------------- */

Ref
PlainSoupGetAllInfo(RefArg inRcvr)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	RefVar info(GetFrameSlot(soupIndexInfo, SYMA(info)));
	return Clone(info);
}


/* -----------------------------------------------------------------------------
	Set the soup’s info frame.
	Native function for soup._parent.SetAllInfo
	Args:		inRcvr		soup object frame
				info			soup info frame
	Return:	nil
----------------------------------------------------------------------------- */

Ref
PlainSoupSetAllInfo(RefArg inRcvr, RefArg info)
{
	if (!IsFrame(info))
		ThrowErr(exBadType, kNSErrNotAFrame);

	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	CheckWriteProtect(PlainSoupGetStore(inRcvr));

	SetFrameSlot(soupIndexInfo, SYMA(info), TotalClone(info));
	SetFrameSlot(soupIndexInfo, SYMA(infoModTime), MAKEINT(RealClock() & 0x1FFFFFFF));

	SoupChanged(soupIndexInfo, false);
	WriteFaultBlock(soupIndexInfo);
	return NILREF;
}


/* -----------------------------------------------------------------------------
	Return the soup’s signature.
	Native function for soup._parent.GetSignature
	Args:		inRcvr		soup object frame
	Return:	the signature: an integer
----------------------------------------------------------------------------- */

Ref
PlainSoupGetSignature(RefArg inRcvr)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	return GetFrameSlot(soupIndexInfo, SYMA(signature));
}


/* -----------------------------------------------------------------------------
	Set the soup’s signature.
	Native function for soup._parent.SetSignature
	Args:		inRcvr		soup object frame
				inSignature	an integer
	Return:	the signature
----------------------------------------------------------------------------- */

Ref
PlainSoupSetSignature(RefArg inRcvr, RefArg inSignature)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	CheckWriteProtect(PlainSoupGetStore(inRcvr));

	SetFrameSlot(soupIndexInfo, SYMA(info), inSignature);

	WriteFaultBlock(soupIndexInfo);
	return inSignature;
}


/* -----------------------------------------------------------------------------
	Return the next available unique entry id for this soup.
	Native function for soup._parent.GetNextUId
	Args:		inRcvr		soup object frame
	Return:	integer
----------------------------------------------------------------------------- */

Ref
PlainSoupGetNextUId(RefArg inRcvr)
{
	return GetFrameSlot(inRcvr, SYMA(indexNextUId));
}


/* -----------------------------------------------------------------------------
	Return the store on which this soup resides.
	Native function for soup._parent.GetStore
	Args:		inRcvr		soup object frame
	Return:	store object frame
----------------------------------------------------------------------------- */

Ref
PlainSoupGetStore(RefArg inRcvr)
{
	return GetFrameSlot(inRcvr, SYMA(storeObj));
}


/* -----------------------------------------------------------------------------
	Return the size of the soup.
	Native function for soup._parent.GetSize
	Args:		inRcvr		soup object frame
	Return:	size in bytes
----------------------------------------------------------------------------- */
extern size_t EntrySize(PSSId inId, CStoreWrapper * inStoreWrapper, bool inCountVBOs);

struct GetSizeParms
{
	CStoreWrapper * storeWrapper;
	size_t size;
};

// search iterator function: return size of object
int
GetSizeStopFn(SKey * inKey, SKey * inData, void * inParms)
{
	GetSizeParms * p = (GetSizeParms *)inParms;
	p->size += EntrySize((int)*inData, p->storeWrapper, true);
	return 0;
}


Ref
PlainSoupGetSize(RefArg inRcvr)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(TStore));
	CSoupIndex * soupIndex = GetSoupIndexObject(inRcvr, 0);
	GetSizeParms parms = { storeWrapper, 0 };
	NewtonErr err = soupIndex->search(1, NULL, NULL, GetSizeStopFn, &parms, NULL, NULL);
	if (err < 0)
		ThrowErr(exStore, err);

	// add in sizes of indexes
	RefVar indexSizes(PlainSoupIndexSizes(inRcvr));
	for (int i = Length(indexSizes) - 1; i > 0; i--)
		parms.size += RINT(GetArraySlot(indexSizes, i));

	return MAKEINT(parms.size);
}


/* -----------------------------------------------------------------------------
	Set soup’s name.
	Native function for soup._parent.SetName
	Args:		inRcvr		soup object frame
				inName		name string
	Return:	the name
----------------------------------------------------------------------------- */

Ref
PlainSoupSetName(RefArg inRcvr, RefArg inName)
{
	if (!IsString(inName))
		ThrowErr(exStore, kNSErrNotAString);
	if (IsRichString(inName))
		ThrowBadTypeWithFrameData(kNSErrNotAPlainString, inName);
	if (Length(inName) / sizeof(UniChar) > 40)
	{
		RefVar errFrame(AllocateFrame());
		SetFrameSlot(errFrame, SYMA(errorCode), MAKEINT(kNSErrInvalidSoupName));
		SetFrameSlot(errFrame, SYMA(value), inName);
		ThrowRefException(exFramesData, errFrame);
	}
	RefVar soupName(GetFrameSlot(inRcvr, SYMA(theName)));
	if (CompareStringNoCase(GetUString(soupName), GetUString(inName)) == 0)
		return inName;

	if (ISTRUE(StoreHasSoup(PlainSoupGetStore(inRcvr), inName)))
		ThrowErr(exStore, kNSErrDuplicateSoupName);

	FaultObject * soupObj = (FaultObject *)NoFaultObjectPtr(GetFrameSlot(inRcvr, SYMA(_proto)));
	CStoreWrapper * storeWrapper = (CStoreWrapper *)soupObj->store;
	PSSId soupObjId = RINT(soupObj->id);	// r8

	CheckWriteProtect(storeWrapper->store());

	OSERRIF(storeWrapper->lockStore());
	newton_try
	{
		NewtonErr err;
		RefVar storeObj(GetFrameSlot(inRcvr, SYMA(storeObj)));	// spE4
		ULong index = RINT(GetFrameSlot(GetFrameSlot(storeObj, SYMA(_proto)), SYMA(nameIndex)));	// r10

		CSoupIndex soupIndex;	// spA0
		soupIndex.init(storeWrapper, index, StoreGetDirSortTable(storeObj));

		SKey	key1;				// sp50
		SKey	key2;				// sp00
		UniChar * name;

		name = GetUString(inName);
		key1.set(Ustrlen(name) * sizeof(UniChar), name);
		key2 = (int)soupObjId;
		err = soupIndex.add(&key1, &key2);
		if (err != noErr)
		{
			if (err > noErr)
				err = kNSErrInternalError;
			ThrowErr(exStore, err);
		}

		name = GetUString(soupName);	// sp158
		key1.set(Ustrlen(name) * sizeof(UniChar), name);
		key2 = (int)soupObjId;
		err = soupIndex.Delete(&key1, &key2);
		if (err != noErr)
		{
			if (err > noErr)
				err = kNSErrInternalError;
			ThrowErr(exStore, err);
		}
	}
	newton_catch_all
	{
		OSERRIF(storeWrapper->abort());
	}
	end_try;
	OSERRIF(storeWrapper->unlockStore());

	RemoveFromUnionSoup(soupName, inRcvr);

	RefVar theName(TotalClone(inName));
	SetClass(theName, SYMA(string_2Enohint));
	SetFrameSlot(inRcvr, SYMA(theName), theName);
	AddToUnionSoup(inName, inRcvr);

	return inName;
}


/* -----------------------------------------------------------------------------
	Remove soup from its store.
	Native function for soup._parent.RemoveFromStore
	Args:		inRcvr		soup object frame
	Return:	nil
----------------------------------------------------------------------------- */

Ref
PlainSoupRemoveFromStore(RefArg inRcvr)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	FaultObject * soupObj = (FaultObject *)NoFaultObjectPtr(soupIndexInfo);
	CStoreWrapper * storeWrapper = (CStoreWrapper *)soupObj->store;
	PSSId soupObjId = RINT(soupObj->id);	// sp00
	CheckWriteProtect(storeWrapper->store());
//sp-04
	RefVar soupName(GetFrameSlot(inRcvr, SYMA(theName)));	// sp00
	PlainSoupRemoveAllEntries(inRcvr);
	RemoveFromUnionSoup(soupName, inRcvr);
	EachSoupCursorDo(inRcvr, kSoupRemoved, inRcvr);

	OSERRIF(storeWrapper->lockStore());
	newton_try
	{
//sp-04
		NewtonErr err;
		RefVar storeObj(GetFrameSlot(inRcvr, SYMA(storeObj)));	// sp00
		RefVar soupIndexes(GetFrameSlot(soupIndexInfo, SYMA(indexes)));	// r7
//sp-04
		RefVar index;	// r8
		Ref id;
		for (ArrayIndex i = 0, count = Length(soupIndexes); i < count; ++i)
		{
			index = GetArraySlot(soupIndexes, i);
			storeWrapper->store()->deleteObject(RINT(GetFrameSlot(index, SYMA(index))));
			id = GetFrameSlot(index, SYMA(sortId));
			if (NOTNIL(id))
				StoreRemoveSortTable(storeObj, RINT(id));
		}
//sp-94
		ULong nameIndex = RINT(GetFrameSlot(GetFrameSlot(storeObj, SYMA(_proto)), SYMA(nameIndex)));		// r9

		CSoupIndex soupIndex;	// sp50
		soupIndex.init(storeWrapper, nameIndex, StoreGetDirSortTable(storeObj));

		SKey	key1;	// sp00
		UniChar * name = GetUString(soupName);
		key1.set(Ustrlen(name) * sizeof(UniChar), name);

//sp-50
		SKey	key2;	// sp00r
		key2 = (int)soupObjId;
		err = soupIndex.Delete(&key1, &key2);
//sp-08
		if (err != noErr)
		{
			if (err > 0)
				err = kNSErrInternalError;
			ThrowErr(exStore, err);
		}

		SetFrameSlot(inRcvr, SYMA(_proto), RA(NILREF));
		DeleteEntryFromCache(GetFrameSlot(storeObj, SYMA(soups)), inRcvr);
		DeletePermObject(storeWrapper, soupObjId);
	}
	newton_catch_all
	{
		OSERRIF(storeWrapper->abort());
	}
	end_try;
	OSERRIF(storeWrapper->unlockStore());

	return NILREF;
}


#pragma mark Add/Remove entries
/* -----------------------------------------------------------------------------
	Add entry to soup.
	Native function for soup._parent.Add
	Args:		inRcvr		soup object frame
				inFrame		frame to add
	Return:	soup entry frame
----------------------------------------------------------------------------- */

Ref
PlainSoupAdd(RefArg inRcvr, RefArg inFrame)
{
	return CommonSoupAddEntry(inRcvr, inFrame, 0x06);
}


/* -----------------------------------------------------------------------------
	Add entry to soup, preserving its _uniqueId.
	Native function for soup._parent.AddWithUniqueId
	Args:		inRcvr		soup object frame
				inFrame		frame to add
	Return:	soup entry frame
----------------------------------------------------------------------------- */

Ref
PlainSoupAddWithUniqueId(RefArg inRcvr, RefArg inFrame)
{
	return CommonSoupAddEntry(inRcvr, inFrame, 0x00);
}


/* -----------------------------------------------------------------------------
	Remove all entries from soup.
	Native function for soup._parent.RemoveAllEntries
	Args:		inRcvr		soup object frame
	Return:	nil
----------------------------------------------------------------------------- */
// search iterator function: delete object
int
RemoveEntryStopFn(SKey * inKey, SKey * inData, void * inParms)
{
	DeletePermObject((CStoreWrapper *)inParms, (int)*inData);
	return 0;
}


Ref
PlainSoupRemoveAllEntries(RefArg inRcvr)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(TStore));
	CheckWriteProtect(storeWrapper->store());

	RefVar soupName(GetFrameSlot(inRcvr, SYMA(theName)));
	RemoveFromUnionSoup(soupName, inRcvr);
	SoupCacheRemoveAllEntries(inRcvr);

	storeWrapper->lockStore();
	newton_try
	{
		CSoupIndex * soupIndex = GetSoupIndexObject(inRcvr, 0);
		NewtonErr err = soupIndex->search(1, NULL, NULL, RemoveEntryStopFn, storeWrapper, NULL, NULL);
		if (err < 0)
			ThrowErr(exStore, err);
		RefVar soupIndexes(GetFrameSlot(soupIndexInfo, SYMA(indexes)));
		RefVar index;
		for (ArrayIndex i = 0, count = Length(soupIndexes); i < count; ++i)
		{
			index = GetArraySlot(soupIndexes, i);
			soupIndex = GetSoupIndexObject(inRcvr, RINT(GetFrameSlot(index, SYMA(index))));
			soupIndex->destroy();
		}
		AddToUnionSoup(soupName, inRcvr);
		SetFrameSlot(soupIndexInfo, SYMA(lastUId), GetFrameSlot(inRcvr, SYMA(indexNextUId)));
		SoupChanged(soupIndexInfo, false);
		WriteFaultBlock(soupIndexInfo);
	}
	newton_catch_all
	{
		storeWrapper->abort();
		AbortSoupIndexes(inRcvr);
	}
	end_try;
	storeWrapper->unlockStore();

	return NILREF;
}


/* -----------------------------------------------------------------------------
	Copy all entries from this soup to another.
	Native function for soup._parent.CopyEntries
	Args:		inRcvr		soup object frame
				intoSoup		destination soup
	Return:
----------------------------------------------------------------------------- */

Ref
PlainSoupCopyEntries(RefArg inRcvr, RefArg intoSoup)
{
	return PlainSoupCopyEntriesWithCallBack(inRcvr, intoSoup, NILREF, NILREF);
}


/* -----------------------------------------------------------------------------
	Copy all entries from this soup to another.
	Call back with progress.
	Native function for soup._parent.CopyEntriesWithCallBack
	Args:		inRcvr		soup object frame
	Return:
----------------------------------------------------------------------------- */
extern int ComparePSSIdMapping(const void * inId1, const void * inId2);
extern PSSId CopyPermObject(PSSId inId, CStoreWrapper * inFromStore, CStoreWrapper * inToStore);
extern "C" Ref DoBlock(RefArg codeBlock, RefArg args);

struct CopyEntriesParms
{
	CStoreWrapper * storeWrapper;
	CStoreWrapper * toStoreWrapper;
	PSSIdMapping * map;
	ArrayIndex aCount;
	ArrayIndex aIndex;
	RefStruct cbCodeblock;
	Timeout cbInterval;
	CTime cbTime;
};

// search iterator function: copy object
int
CopyEntriesStopFn(SKey * inKey, SKey * inData, void * inParms)
{
	CopyEntriesParms * parms = (CopyEntriesParms *)inParms;
	if (parms->aIndex >= parms->aCount)
		// map is full
		return 1;

	parms->map[parms->aIndex].id1 = (int)*inData;
	parms->map[parms->aIndex].id2 = CopyPermObject((int)*inData, parms->storeWrapper, parms->toStoreWrapper);
	++(parms->aIndex);

	if (parms->cbInterval)
	{
		if ((Timeout)(GetGlobalTime() - parms->cbTime) > parms->cbInterval)
		{
			DoBlock(parms->cbCodeblock, RA(NILREF));
			parms->cbTime = GetGlobalTime();
		}
	}

	return 0;
}


Ref
SlowCopyEntries(RefArg inFromSoup, RefArg inToSoup, RefArg inCallbackFn, Timeout inCallbackInterval)
{
	CStoreWrapper * fromStore = (CStoreWrapper *)GetFrameSlot(inFromSoup, SYMA(TStore));
	CStoreWrapper * toStore = (CStoreWrapper *)GetFrameSlot(inToSoup, SYMA(TStore));
	SKey indexKey;
	SKey indexData;
	RefVar entry;
	CTime cbTime;
	if (inCallbackInterval != 0)
		cbTime = GetGlobalTime();
	int indexNextUId = -1;
	CSoupIndex * soupIndex = GetSoupIndexObject(inFromSoup, 0);

	toStore->lockStore();
	newton_try
	{
		if (soupIndex->first(&indexKey, &indexData) == 0)
		{
			for (int nextErr = 0; nextErr == 0; )
			{
				entry = LoadPermObject(fromStore, (int)indexData, NULL);
				int uid = RINT(GetFrameSlot(entry, SYMA(_uniqueId)));
				if (uid > indexNextUId)
					indexNextUId = uid;
				PSSId id = kNoPSSId;
				StorePermObject(entry, toStore, id, NULL, NULL);
				AlterIndexes(1, inToSoup, entry, id);
				nextErr = soupIndex->next(&indexKey, &indexData, 0, &indexKey, &indexData);
				if (inCallbackInterval)
				{
					if ((Timeout)(GetGlobalTime() - cbTime) > inCallbackInterval)
					{
						DoBlock(inCallbackFn, RA(NILREF));
						cbTime = GetGlobalTime();
					}
				}
			}
		}
		if (RINT(GetFrameSlot(inToSoup, SYMA(indexNextUId))) < indexNextUId)
		{
			SetFrameSlot(inToSoup, SYMA(indexNextUId), MAKEINT(indexNextUId+1));
			RefVar toProto(GetFrameSlot(inToSoup, SYMA(_proto)));
			SetFrameSlot(toProto, SYMA(lastUId), RA(NILREF));
			SoupChanged(toProto, false);
			WriteFaultBlock(toProto);
		}
	}
	newton_catch_all
	{
		toStore->abort();
		AbortSoupIndexes(inToSoup);
	}
	end_try;
	toStore->unlockStore();

	return NILREF;
}



Ref
PlainSoupCopyEntriesWithCallBack(RefArg inRcvr, RefArg intoSoup, RefArg inCallbackFn, RefArg inCallbackInterval)
{
	RefVar fromProto(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(fromProto))
		ThrowErr(exStore, kNSErrInvalidSoup);

	RefVar toProto(GetFrameSlot(intoSoup, SYMA(_proto)));
	if (ISNIL(toProto))
		ThrowErr(exStore, kNSErrInvalidSoup);

	if (FrameHasSlot(intoSoup, SYMA(soupList)))
		ThrowErr(exStore, kNSErrCantCopyToUnionSoup);

	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(TStore));
	CheckWriteProtect(storeWrapper->store());

	CopyEntriesParms parms;
	Timeout callbackInterval;
	if (NOTNIL(inCallbackFn))
	{
		parms.cbCodeblock = inCallbackFn;
		CTime cbi(RINT(inCallbackInterval), kMilliseconds);
		callbackInterval = (Timeout)cbi;
		parms.cbTime = GetGlobalTime();
	}
	else
		callbackInterval = 0;
	parms.cbInterval = callbackInterval;

	if (RINT(GetFrameSlot(intoSoup, SYMA(indexNextUId))) != 0
	||  CompareSoupIndexes(fromProto, toProto) == 0)
		return SlowCopyEntries(inRcvr, intoSoup, inCallbackFn, callbackInterval);

	RefVar indexNextUId = GetFrameSlot(inRcvr, SYMA(indexNextUId));
	parms.storeWrapper = storeWrapper;
	parms.toStoreWrapper = (CStoreWrapper *)GetFrameSlot(intoSoup, SYMA(TStore));
	parms.aIndex = 0;
	parms.aCount = RINT(indexNextUId);
	parms.map = new PSSIdMapping[parms.aCount];
	if (parms.map == NULL)
		return SlowCopyEntries(inRcvr, intoSoup, inCallbackFn, callbackInterval);

	storeWrapper->lockStore();
	newton_try
	{
		storeWrapper->startCopyMaps_Symbols();
		CSoupIndex * soupIndex = GetSoupIndexObject(inRcvr, 0);
		NewtonErr err = soupIndex->search(1, NULL, NULL, CopyEntriesStopFn, &parms, NULL, NULL);
		if (err < 0)
			ThrowErr(exStore, err);
		storeWrapper->endCopyMaps_Symbols();

		qsort(parms.map, parms.aIndex, sizeof(PSSIdMapping), ComparePSSIdMapping);
		CopySoupIndexes(inRcvr, intoSoup, parms.map, parms.aIndex, inCallbackFn, callbackInterval);
		delete parms.map;

		SetFrameSlot(toProto, SYMA(lastUId), GetFrameSlot(fromProto, SYMA(lastUId)));
		SoupChanged(toProto, false);
		WriteFaultBlock(toProto);
		SetFrameSlot(intoSoup, SYMA(indexNextUId), indexNextUId);
	}
	newton_catch_all
	{
		storeWrapper->endCopyMaps_Symbols();
		delete parms.map;
		storeWrapper->abort();
		AbortSoupIndexes(intoSoup);
	}
	end_try;
	storeWrapper->unlockStore();

	return NILREF;
}


#pragma mark DEPRECATED
/* -----------------------------------------------------------------------------
	Mark the soup as dirty (and therefore also its store).
	Native function for soup._parent.Dirty -- does this even exist?
	Args:		inRcvr		soup object frame
	Return:	nil
----------------------------------------------------------------------------- */

Ref
PlainSoupDirty(RefArg inRcvr)
{
	SetFrameSlot(inRcvr, SYMA(dirty), RA(TRUEREF));
	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(TStore));
	storeWrapper->dirty();
	return NILREF;
}


/* -----------------------------------------------------------------------------
	Flush dirty soup entries to backing store.
	Native function for soup._parent.Flush
	DEPRECATED in NOS2
	Args:		inRcvr		soup object frame
	Return:	true => something was flushed
----------------------------------------------------------------------------- */

Ref
PlainSoupFlush(RefArg inRcvr)
{
	Ref isFlushed = NILREF;

	if (NOTNIL(GetFrameSlot(inRcvr, SYMA(dirty))))
	{
		RefVar entry;
		RefVar cache(GetFrameSlot(inRcvr, SYMA(cache)));
		for (ArrayIndex i = 0, count = Length(cache); i < count; ++i)
		{
			entry = GetArraySlot(cache, i);
			if (NOTNIL(entry) && EntryDirty(entry))
			{
				EntryChange(entry);
				isFlushed = TRUEREF;
			}
		}
		SetFrameSlot(inRcvr, SYMA(dirty), RA(NILREF));
	}

	return isFlushed;
}
