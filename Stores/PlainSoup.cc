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
			SoupChanged(soupIndexInfo, NO);
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
		SoupChanged(soupIndexInfo, NO);
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


Ref
PlainSoupHasTags(RefArg inRcvr)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	return MAKEBOOLEAN(NOTNIL(GetTagsIndexDesc(soupIndexInfo)));
}


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

	bool isTagsChanged;
	if (isTagsArray)
		for (i = 0; i < numOfNewTags; ++i)
		{
			if (AddTag(soupTags, GetArraySlot(inTags, i)))
				isTagsChanged = YES;
		}
	else
		isTagsChanged = AddTag(soupTags, inTags);

	if (isTagsChanged)
	{
		RefVar theTime(MAKEINT(RealClock() & 0x1FFFFFFF));
		SetFrameSlot(soupIndexInfo, SYMA(indexesModTime), theTime);
		SoupChanged(soupIndexInfo, NO);
		WriteFaultBlock(soupIndexInfo);
		EachSoupCursorDo(inRcvr, kSoupTagsChanged);
	}

	return NILREF;
}


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
			bool isTagsEmpty = NO;
			if (IsArray(soupTags))
			{
				for (i = count - 1; i >= 0; i--)
					ArrayRemove(soupTags, GetArraySlot(inTags, i));
				if (Length(soupTags) == 0)
					isTagsEmpty = YES;
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
		SoupChanged(soupIndexInfo, NO);
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
		SoupChanged(soupIndexInfo, NO);
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


Ref
PlainSoupSetInfo(RefArg inRcvr, RefArg inSym, RefArg inValue)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	CheckWriteProtect(PlainSoupGetStore(inRcvr));

	RefVar value(TotalClone(inValue));
	RefVar sym(EnsureInternal(inSym));
	RefVar info;
	if (FrameHasSlot(soupIndexInfo, SYMA(info)))
		info = GetFrameSlot(soupIndexInfo, SYMA(info));
	else
		info = AllocateFrame();

	SetFrameSlot(info, sym, value);
	if (EQ(inSym, SYMA(NCKLastBackupTime)))
		SetFrameSlot(soupIndexInfo, SYMA(infoModTime), MAKEINT(RealClock() & 0x1FFFFFFF));

	SoupChanged(soupIndexInfo, NO);
	WriteFaultBlock(soupIndexInfo);
	return NILREF;
}


Ref
PlainSoupGetAllInfo(RefArg inRcvr)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	RefVar info(GetFrameSlot(soupIndexInfo, SYMA(info)));
	return Clone(info);
}


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

	SoupChanged(soupIndexInfo, NO);
	WriteFaultBlock(soupIndexInfo);
	return NILREF;
}


Ref
PlainSoupGetSignature(RefArg inRcvr)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	return GetFrameSlot(soupIndexInfo, SYMA(signature));
}


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


Ref
PlainSoupGetNextUId(RefArg inRcvr)
{
	return GetFrameSlot(inRcvr, SYMA(indexNextUId));
}


Ref
PlainSoupGetStore(RefArg inRcvr)
{
	return GetFrameSlot(inRcvr, SYMA(storeObj));
}


int
GetSizeStopFn(SKey *, SKey *, void *)
{
	// INCOMPLETE!
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
	int size;
#if defined(correct)
	NewtonErr err = soupIndex->search(1, NULL, NULL, GetSizeStopFn, &size, NULL, NULL);
#else
	NewtonErr err = noErr;
	size = 0;
#endif
	if (err < 0)
		ThrowErr(exStore, err);

	RefVar indexSizes(PlainSoupIndexSizes(inRcvr));
	for (int i = Length(indexSizes) - 1; i > 0; i--)
		size += RINT(GetArraySlot(indexSizes, i));

	return MAKEINT(size);
}


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
//sp-6C
	newton_try
	{
//sp-E8
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


int
CopyEntriesStopFn(SKey *, SKey *, void *)
{
}


Ref
SlowCopyEntries(RefArg, RefArg, RefArg, unsigned long)
{
}


Ref
PlainSoupCopyEntries(RefArg inRcvr, RefArg toSoup)
{
	return PlainSoupCopyEntriesWithCallBack(inRcvr, toSoup, NILREF, NILREF);
}


Ref
PlainSoupCopyEntriesWithCallBack(RefArg inRcvr, RefArg inToSoup, RefArg arg3, RefArg arg4)
{
	RefVar fromProto(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(fromProto))
		ThrowErr(exStore, kNSErrInvalidSoup);

	RefVar toProto(GetFrameSlot(inToSoup, SYMA(_proto)));
	if (ISNIL(toProto))
		ThrowErr(exStore, kNSErrInvalidSoup);

	if (FrameHasSlot(inToSoup, SYMA(soupList)))
		ThrowErr(exStore, kNSErrCantCopyToUnionSoup);

	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(TStore));	// r7
	CheckWriteProtect(storeWrapper->store());
#if defined(correct)
	RefStruct sp14;
	if (NOTNIL(arg3))
	{
		sp14 = arg3;
		CTime sp00(RINT(arg4), 0x0E66);
		r8 = sp00.x04;
		GetGlobalTime(&sp24);
	}
	else
		r8 = 0;
	sp18 = r8;
	if (RINT(GetFrameSlot(inToSoup, SYMA(indexNextUId))) == 0
	 || CompareSoupIndexes(sp28, sp24) == 0)
		return SlowCopyEntries(inRcvr, inToSoup, arg3, arg4);

	if (RINT(GetFrameSlot(inRcvr, SYMA(indexNextUId))) == 0
	 || CompareSoupIndexes(sp28, sp24) == 0)
		return SlowCopyEntries(inRcvr, inToSoup, arg3, arg4);

	storeWrapper->lockStore();
	newton_try
	{
		storeWrapper->startCopyMaps_Symbols();
		CSoupIndex * soupIndex = GetSoupIndexObject(inRcvr, 0);
		NewtonErr err = soupIndex->search(1, NULL, NULL, CopyEntriesStopFn, NULL, NULL, NULL);
		if (err < 0)
			ThrowErr(exStore, err);
		storeWrapper->endCopyMaps_Symbols();
		qsort(sp7C, sp84, 8, anonFunc);
		CopySoupIndexes(inRcvr, inToSoup, sp7C, sp84, arg3, arg4);
		delete sp7C;
		SetFrameSlot(sp98, SYMA(lastUId), GetFrameSlot(sp9C, SYMA(lastUId)));
		SoupChanged(sp98, NO);
		WriteFaultBlock(sp98);
		SetFrameSlot(inToSoup, SYMA(indexNextUId), sp70);
	}
	newton_catch_all
	{
		storeWrapper->endCopyMaps_Symbols();
		delete sp7C;
		storeWrapper->abort();
		AbortSoupIndexes(inToSoup);
	}
	end_try;
	storeWrapper->unlockStore();
#endif
	return NILREF;
}


int
RemoveEntryStopFn(SKey *, SKey *, void *)
{
}


Ref
PlainSoupRemoveAllEntries(RefArg inRcvr)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	RefVar storeObj(GetFrameSlot(inRcvr, SYMA(storeObj)));
	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(storeObj, SYMA(TStore));
	CheckWriteProtect(storeWrapper->store());
	RefVar soupName(GetFrameSlot(inRcvr, SYMA(theName)));
	RemoveFromUnionSoup(soupName, inRcvr);
	SoupCacheRemoveAllEntries(inRcvr);

	storeWrapper->lockStore();
	newton_try
	{
		CSoupIndex * soupIndex = GetSoupIndexObject(inRcvr, 0);
#if defined(correct)
		NewtonErr err = soupIndex->search(1, NULL, NULL, RemoveEntryStopFn, NULL, NULL, NULL);
#else
		NewtonErr err = noErr;
#endif
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
		SoupChanged(soupIndexInfo, NO);
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
//sp-6C
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
		ULong r9 = RINT(GetFrameSlot(GetFrameSlot(storeObj, SYMA(_proto)), SYMA(nameIndex)));

		CSoupIndex soupIndex;	// sp50
		soupIndex.init(storeWrapper, r9, StoreGetDirSortTable(storeObj));

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


Ref
PlainSoupAdd(RefArg inRcvr, RefArg inEntry)
{
	return CommonSoupAddEntry(inRcvr, inEntry, 0x06);
}


Ref
PlainSoupAddWithUniqueId(RefArg inRcvr, RefArg inEntry)
{
	return CommonSoupAddEntry(inRcvr, inEntry, 0x00);
}


Ref
PlainSoupDirty(RefArg inRcvr)
{
	SetFrameSlot(inRcvr, SYMA(dirty), RA(TRUEREF));
	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(TStore));
	storeWrapper->dirty();
	return NILREF;
}


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
