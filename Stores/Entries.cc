/*
	File:		Entries.cc

	Contains:	Soup entry implementation.

	Written by:	Newton Research Group, 2007.
*/

#include "Objects.h"
#include "ObjHeader.h"
#include "RefMemory.h"
#include "ROMResources.h"
#include "Funcs.h"
#include "Unicode.h"
#include "FaultBlocks.h"
#include "StoreWrapper.h"
#include "Soups.h"
#include "PlainSoup.h"
#include "Cursors.h"
#include "Entries.h"
#include "LargeBinaries.h"
#include "StoreStreamObjects.h"
#include "OSErrors.h"

DeclareException(exBadType, exRootException);

extern Ref	gPackageStores;
extern Ref	gUnionSoups;

extern void	CheckWriteProtect(CStore * inStore);
extern void	AbortLargeBinaries(RefArg inEntry);

extern ULong	RealClock(void);

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FIsSoupEntry(RefArg inRcvr, RefArg inObj);
Ref	FEntryIsResident(RefArg inRcvr, RefArg inEntry);
Ref	FEntryValid(RefArg inRcvr, RefArg inEntry);
Ref	FEntrySoup(RefArg inRcvr, RefArg inEntry);
Ref	FEntryStore(RefArg inRcvr, RefArg inEntry);
Ref	FEntrySize(RefArg inRcvr, RefArg inEntry);
Ref	FEntrySizeWithoutVBOs(RefArg inRcvr, RefArg inEntry);
Ref	FEntryTextSize(RefArg inRcvr, RefArg inEntry);
Ref	FEntryUniqueId(RefArg inRcvr, RefArg inEntry);
Ref	FEntryModTime(RefArg inRcvr, RefArg inEntry);
Ref	FEntryChange(RefArg inRcvr, RefArg inEntry);
Ref	FEntryFlush(RefArg inRcvr, RefArg inEntry);
Ref	FEntryChangeWithModTime(RefArg inRcvr, RefArg inEntry);
Ref	FEntryChangeVerbatim(RefArg inRcvr, RefArg inEntry);
Ref	FEntryUndoChanges(RefArg inRcvr, RefArg inEntry);
Ref	FEntryRemoveFromSoup(RefArg inRcvr, RefArg inEntry);
Ref	FEntryReplace(RefArg inRcvr, RefArg inEntry);
Ref	FEntryReplaceWithModTime(RefArg inRcvr, RefArg inEntry);
Ref	FEntryCopy(RefArg inRcvr, RefArg inEntry);
Ref	FEntryMove(RefArg inRcvr, RefArg inEntry);

Ref	FNewProxyEntry(RefArg inRcvr, RefArg inHandler, RefArg inObj);
Ref	FIsProxyEntry(RefArg inRcvr, RefArg inEntry);
Ref	FEntrySetHandler(RefArg inRcvr, RefArg inEntry, RefArg inHandler);
Ref	FEntryHandler(RefArg inRcvr, RefArg inEntry);
Ref	FEntrySetCachedObject(RefArg inRcvr, RefArg inEntry, RefArg inObject);
Ref	FEntryCachedObject(RefArg inRcvr, RefArg inEntry);

Ref	FMakeEntryAlias(RefArg inRcvr, RefArg inEntry);
Ref	FIsEntryAlias(RefArg inRcvr, RefArg inEntry);
Ref	FResolveEntryAlias(RefArg inRcvr, RefArg inEntry);
Ref	FIsSameEntry(RefArg inRcvr, RefArg inEntry1, RefArg inEntry2);
}

Ref	FIsSoupEntry(RefArg inRcvr, RefArg inObj) { return MAKEBOOLEAN(IsSoupEntry(inObj)); }
Ref	FEntryIsResident(RefArg inRcvr, RefArg inEntry) { return MAKEBOOLEAN(EntryIsResident(inEntry)); }
Ref	FEntryValid(RefArg inRcvr, RefArg inEntry) { return MAKEBOOLEAN(EntryValid(inEntry)); }
Ref	FEntrySoup(RefArg inRcvr, RefArg inEntry) { return EntrySoup(inEntry); }
Ref	FEntryStore(RefArg inRcvr, RefArg inEntry) { return EntryStore(inEntry); }
Ref	FEntrySize(RefArg inRcvr, RefArg inEntry) { return MAKEINT(EntrySize(inEntry)); }
Ref	FEntrySizeWithoutVBOs(RefArg inRcvr, RefArg inEntry) { return MAKEINT(EntrySizeWithoutVBOs(inEntry)); }
Ref	FEntryTextSize(RefArg inRcvr, RefArg inEntry) { return MAKEINT(EntryTextSize(inEntry)); }
Ref	FEntryUniqueId(RefArg inRcvr, RefArg inEntry) { return MAKEINT(EntryUniqueId(inEntry)); }
Ref	FEntryModTime(RefArg inRcvr, RefArg inEntry) { return MAKEINT(EntryModTime(inEntry)); }
Ref	FEntryChange(RefArg inRcvr, RefArg inEntry) { EntryChange(inEntry); return NILREF; }
Ref	FEntryFlush(RefArg inRcvr, RefArg inEntry) { EntryFlush(inEntry); return NILREF; }
Ref	FEntryChangeWithModTime(RefArg inRcvr, RefArg inEntry) { EntryChangeWithModTime(inEntry); return NILREF; }
Ref	FEntryChangeVerbatim(RefArg inRcvr, RefArg inEntry) { EntryChangeVerbatim(inEntry); return NILREF; }
Ref	FEntryUndoChanges(RefArg inRcvr, RefArg inEntry) { EntryUndoChanges(inEntry); return NILREF; }
Ref	FEntryRemoveFromSoup(RefArg inRcvr, RefArg inEntry) { EntryRemoveFromSoup(inEntry); return NILREF; }
Ref	FEntryReplace(RefArg inRcvr, RefArg inEntry, RefArg inNewEntry) { EntryReplace(inEntry, inNewEntry); return NILREF; }
Ref	FEntryReplaceWithModTime(RefArg inRcvr, RefArg inEntry, RefArg inNewEntry) { EntryReplaceWithModTime(inEntry, inNewEntry); return NILREF; }
Ref	FEntryCopy(RefArg inRcvr, RefArg inEntry, RefArg inSoup) { return EntryCopy(inEntry, inSoup); }
Ref	FEntryMove(RefArg inRcvr, RefArg inEntry, RefArg inSoup) { EntryMove(inEntry, inSoup); return NILREF; }

Ref	FNewProxyEntry(RefArg inRcvr, RefArg inHandler, RefArg inObj) { return NewProxyEntry(inHandler, inObj); }
Ref	FIsProxyEntry(RefArg inRcvr, RefArg inEntry) { return MAKEBOOLEAN(IsProxyEntry(inEntry)); }
Ref	FEntrySetHandler(RefArg inRcvr, RefArg inEntry, RefArg inHandler) { EntrySetHandler(inEntry, inHandler); return NILREF; }
Ref	FEntryHandler(RefArg inRcvr, RefArg inEntry) { return EntryHandler(inEntry); }
Ref	FEntrySetCachedObject(RefArg inRcvr, RefArg inEntry, RefArg inObject) { EntrySetCachedObject(inEntry, inObject); return NILREF; }
Ref	FEntryCachedObject(RefArg inRcvr, RefArg inEntry) { return EntryCachedObject(inEntry); }

Ref	FMakeEntryAlias(RefArg inRcvr, RefArg inEntry) { return MakeEntryAlias(inEntry); }
Ref	FIsEntryAlias(RefArg inRcvr, RefArg inEntry) { return IsEntryAlias(inEntry); }
Ref	FResolveEntryAlias(RefArg inRcvr, RefArg inEntry) { return ResolveEntryAlias(inEntry); }
Ref	FIsSameEntry(RefArg inRcvr, RefArg inEntry1, RefArg inEntry2) { return IsSameEntry(inEntry1, inEntry2); }

#pragma mark -

/*----------------------------------------------------------------------
	E n t r i e s
----------------------------------------------------------------------*/

bool
IsSoupEntry(Ref r)
{
	return IsFaultBlock(r);
}


Ref
EntryStore(RefArg inEntry)
{
	if (IsProxyEntry(inEntry))
		return ForwardEntryMessage(inEntry, SYMA(EntryStore));
	return GetFrameSlot(EntrySoup(inEntry), SYMA(storeObj));
}


Ref
EntrySoup(RefArg inEntry)
{
	if (!IsFaultBlock(inEntry))
		ThrowErr(exStore, kNSErrNotASoupEntry);

	if (IsProxyEntry(inEntry))
		return ForwardEntryMessage(inEntry, SYMA(EntrySoup));

	FaultObject * soupObj = (FaultObject *)NoFaultObjectPtr(inEntry);
	return soupObj->handler;
}


Ref
EnsureEntryInternal(RefArg inEntry)
{
	RefVar proto(GetFrameSlot(inEntry, SYMA(_proto)));
	if (ISNIL(proto))
		return EnsureInternal(inEntry);

	RefVar internalEntry;
	SetFrameSlot(inEntry, SYMA(_proto), RA(NILREF));
	newton_try
	{
		internalEntry = EnsureInternal(inEntry);
		SetFrameSlot(internalEntry, SYMA(_proto), proto);
		SetFrameSlot(inEntry, SYMA(_proto), proto);
	}
	cleanup
	{
		SetFrameSlot(inEntry, SYMA(_proto), proto);
	}
	end_try;
	return internalEntry;
}


Ref
SafeEntryAdd(RefArg inRcvr, RefArg inFrame, RefArg inId, unsigned char inFlags)
{
	RefVar entry;

	bool r6 = ((inFlags & 0x01) != 0);
	if (r6)
		entry = inFrame;
	else
		entry = EnsureEntryInternal(inFrame);

	if (inFlags & 0x02)
		SetFrameSlot(entry, SYMA(_modTime), MAKEINT(RealClock() & 0x1FFFFFFF));

	if (inFlags & 0x04)
		SetFrameSlot(entry, SYMA(_uniqueId), inId);

	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(TStore));
	PSSId storageId = kNoPSSId;
	bool sp00r;	// hasLargeBinaries?
	StorePermObject(entry, storeWrapper, storageId, NULL, &sp00r);
	if (sp00r)
		r6 = YES;
	AlterIndexes(1, inRcvr, entry, storageId);
	if (IsFaultBlock(entry))
	{
		FaultObject * fb = (FaultObject *)NoFaultObjectPtr(entry);
		RefVar existingHandler(fb->handler);
		CStoreWrapper * existingStore = (CStoreWrapper *)fb->store;
		PSSId existingId = RINT(fb->id);

		fb->handler = inRcvr;
		fb->store = (Ref) storeWrapper;
		fb->id = MAKEINT(storageId);
		if (r6)
			fb->object = NILREF;

		RefVar existingCache(GetFrameSlot(existingHandler, SYMA(cache)));
		if (!EQ(inRcvr, existingHandler))
		{
			DeleteEntryFromCache(existingCache, entry);
			PutEntryIntoCache(GetFrameSlot(inRcvr, SYMA(cache)), entry);
		}
		RefVar newEntry(MakeFaultBlock(existingHandler, existingStore, existingId));
		PutEntryIntoCache(existingCache, newEntry);
		EachSoupCursorDo(existingCache, kSoupEntryReadded, entry, newEntry);
	}
	else
	{
		RefVar newEntry(MakeFaultBlock(inRcvr, storeWrapper, storageId, r6 ? NILREF : Clone(entry)));
		ReplaceObject(entry, newEntry);
		PutEntryIntoCache(GetFrameSlot(inRcvr, SYMA(cache)), entry);
	}

	return entry;
}


void
EntryChangeCommon(RefArg inEntry, int inSelector)
{
	if (!IsFaultBlock(inEntry))
		ThrowErr(exStore, kNSErrNotASoupEntry);

	if (inSelector & 0x02)
		SetFrameSlot(inEntry, SYMA(_modTime), MAKEINT(RealClock() & 0x1FFFFFFF));

	FaultObject * obj = (FaultObject *)NoFaultObjectPtr(inEntry);
	RefVar soupObj(obj->handler);	// sp18
	RefVar frame(obj->object);		// sp14
//sp-04
	CStoreWrapper * storeWrapper = (CStoreWrapper *)obj->store;	// r6
	PSSId id = RINT(obj->id);	// sp10
//	if (storeWrapper == NULL)
//		ThrowErr(exStore, kNSErrInvalidEntry);
	CheckWriteProtect(storeWrapper->store());
//sp-08
	bool	r7;
	bool	sp0C = NO;
	RefVar proto(GetFrameSlot(soupObj, SYMA(_proto)));	// sp08
	if (ISNIL(proto))
		ThrowErr(exStore, kNSErrInvalidEntry);
	if (NOTNIL(frame))
	{
		frame = EnsureEntryInternal(frame);
		CDynamicArray * largeObjectsRead = NULL;
//sp-08
		RefVar spm00(LoadPermObject(storeWrapper, id, &largeObjectsRead));
//sp-04
		if ((inSelector & 0x01)	// !verbatim
		&& !EQRef(GetFrameSlot(frame, SYMA(_uniqueId)), GetFrameSlot(spm00, SYMA(_uniqueId))))
		{
			SetFrameSlot(frame, SYMA(_uniqueId), GetFrameSlot(spm00, SYMA(_uniqueId)));
		}

		storeWrapper->lockStore();
//sp-6C
		newton_try
		{
//sp-04
			bool sp;
			StorePermObject(frame, storeWrapper, id, largeObjectsRead, &sp);
			if (sp)
				inSelector |= 0x08;	// flush
			if (largeObjectsRead)
			{
				delete largeObjectsRead;
				largeObjectsRead = NULL;
			}
			sp0C = (inSelector & 0x04) != 0;
			r7 = UpdateIndexes(soupObj, frame, spm00, id, &sp0C);
			SoupChanged(proto, YES);
		}
		newton_catch_all
		{
			storeWrapper->abort();
			AbortSoupIndexes(soupObj);
			if (largeObjectsRead)
				delete largeObjectsRead;
		}
		end_try;
		storeWrapper->unlockStore();

		if (r7 || sp0C)
		{
//sp-04
			RefVar cursors(GetFrameSlot(soupObj, SYMA(cursors)));
			RefVar cursor;
			for (int i = Length(cursors) - 1; i >= 0; i--)
			{
				cursor = GetArraySlot(cursor, i);
				if (NOTNIL(cursor))
					CursorObj(cursor)->entryChanged(inEntry, r7, sp0C);
			}
		}

		if (inSelector & 0x08)	// flush
			((FaultObject *)NoFaultObjectPtr(inEntry))->object = NILREF;
	}
}


void
EntryChangeWithModTime(RefArg inEntry)
{
	if (IsProxyEntry(inEntry))
		ForwardEntryMessage(inEntry, SYMA(EntryChangeWithModTime));
	else
		EntryChangeCommon(inEntry, 5);
}


void
EntryChangeVerbatim(RefArg inEntry)
{
	if (IsProxyEntry(inEntry))
		ForwardEntryMessage(inEntry, SYMA(EntryChangeVerbatim));	// not in NPR
	else
		EntryChangeCommon(inEntry, 4);
}


void
EntryChange(RefArg inEntry)
{
	if (IsProxyEntry(inEntry))
		ForwardEntryMessage(inEntry, SYMA(EntryChange));
	else
		EntryChangeCommon(inEntry, 7);
}


void
EntryFlush(RefArg inEntry)
{
	if (IsProxyEntry(inEntry))
		ForwardEntryMessage(inEntry, SYMA(EntryChange));
	else
		EntryChangeCommon(inEntry, 15);
}


void
EntryFlushWithModTime(RefArg inEntry)
{
	if (IsProxyEntry(inEntry))
		ForwardEntryMessage(inEntry, SYMA(EntryChange));
	else
		EntryChangeCommon(inEntry, 13);
}


void
EntryRemoveFromSoup(RefArg inEntry)
{
	if (!IsFaultBlock(inEntry))
		ThrowErr(exStore, kNSErrNotASoupEntry);

	if (IsProxyEntry(inEntry))
		ForwardEntryMessage(inEntry, SYMA(EntryRemoveFromSoup));
	else
	{
		FaultObject * obj = (FaultObject *)NoFaultObjectPtr(inEntry);
		CStoreWrapper * storeWrapper = (CStoreWrapper *)obj->store;
		if (storeWrapper == NULL)
			ThrowErr(exStore, kNSErrInvalidEntry);
		CheckWriteProtect(storeWrapper->store());
		RefVar soupObj(obj->handler);
		PSSId id = RINT(obj->id);
		RefVar frame(obj->object);
		EachSoupCursorDo(soupObj, kEntryRemoved, inEntry);
//sp-08
		RefVar proto(GetFrameSlot(soupObj, SYMA(_proto)));
		RefVar entry;
		if (ISNIL(frame) || EntryDirty(inEntry))
			entry = LoadPermObject(storeWrapper, id, NULL);
		else
			entry = frame;

		storeWrapper->lockStore();
//sp-6C
		newton_try
		{
//sp-04
			AlterIndexes(NO, soupObj, entry, id);
			DeleteEntryFromCache(GetFrameSlot(soupObj, SYMA(cache)), inEntry);
			ReplaceObject(inEntry, NOTNIL(frame) ? frame : entry);
			DeletePermObject(storeWrapper, id);
//sp-04
			RefVar nextUId(GetFrameSlot(soupObj, SYMA(indexNextUId)));
			if (RINT(nextUId) == RINT(GetFrameSlot(inEntry, SYMA(_uniqueId))) + 1)
			{
				SetFrameSlot(proto, SYMA(lastUId), nextUId);
				WriteFaultBlock(proto);
			}
			SoupChanged(proto, YES);
		}
		newton_catch_all
		{
			storeWrapper->abort();
			AbortSoupIndexes(soupObj);
		}
		end_try;
		storeWrapper->unlockStore();
	}
}


void
EntryReplaceCommon(RefArg inEntry, RefArg inNewEntry, int inSelector)
{
	if (!IsFaultBlock(inEntry))
		ThrowErr(exStore, kNSErrNotASoupEntry);

	if (IsProxyEntry(inEntry))
		ForwardEntryMessage(inEntry, (inSelector == 0) ? SYMA(EntryReplace) : SYMA(EntryReplaceWithModTime), inNewEntry);
	else
	{
		if (!(ISPTR(inNewEntry) && (ObjectFlags(inNewEntry) & kObjFrame)))
			ThrowErr(exBadType, kNSErrNotAFrame);
		if (!EQ(inEntry, inNewEntry))
		{
			FaultObject * obj = (FaultObject *)NoFaultObjectPtr(inEntry);
			CStoreWrapper * storeWrapper = (CStoreWrapper *)obj->store;
		//	if (storeWrapper == NULL)
		//		ThrowErr(exStore, kNSErrInvalidEntry);	// not in the original, but done in EntryMove; a good idea?
			CheckWriteProtect(storeWrapper->store());
			if (IsFaultBlock(inNewEntry))
			{
				ObjectPtr(inNewEntry);	// sic -- original does nothing with it; caches entry?
				FaultObject * newObj = (FaultObject *)NoFaultObjectPtr(inNewEntry);
				RefVar soupObj(newObj->handler);
				RefVar frame(newObj->object);
				DeleteEntryFromCache(GetFrameSlot(soupObj, SYMA(cache)), inNewEntry);
				obj->object = frame;
			}
			else
				obj->object = Clone(inNewEntry);

			if (inSelector == 0)
				EntryChange(inEntry);
			else
				EntryChangeWithModTime(inEntry);
			ReplaceObject(inNewEntry, inEntry);
		}
	}
}


void
EntryReplace(RefArg inEntry, RefArg inNewEntry)
{
	EntryReplaceCommon(inEntry, inNewEntry, 0);
}


void
EntryReplaceWithModTime(RefArg inEntry, RefArg inNewEntry)
{
	EntryReplaceCommon(inEntry, inNewEntry, 1);
}


void
EntryUndoChanges(RefArg inEntry)
{
	if (!IsFaultBlock(inEntry))
		ThrowErr(exStore, kNSErrNotASoupEntry);

	if (IsProxyEntry(inEntry))
		ForwardEntryMessage(inEntry, SYMA(EntryUndoChanges));
	else
	{
		AbortLargeBinaries(inEntry);
		UncacheIfFaultBlock(inEntry);
	}
}


Ref
EntryCopy(RefArg inEntry, RefArg inSoup)
{
	if (IsProxyEntry(inEntry))
		return ForwardEntryMessage(inEntry, SYMA(EntryCopy), inSoup);
	RefVar newEntry(SoupAdd(inSoup, Clone(inEntry)));
	UncacheIfFaultBlock(newEntry);
	return newEntry;
}


void
EntryMove(RefArg inEntry, RefArg inSoup)
{
	if (!IsFaultBlock(inEntry))
		ThrowErr(exStore, kNSErrNotASoupEntry);

	if (IsProxyEntry(inEntry))
		ForwardEntryMessage(inEntry, SYMA(EntryMove), inSoup);
	else
	{
		CheckWriteProtect(PlainSoupGetStore(inSoup));
		ObjectPtr(inEntry);	// sic -- original does nothing with it; caches entry?

		FaultObject * obj = (FaultObject *)NoFaultObjectPtr(inEntry);

		CStoreWrapper * storeWrapper = (CStoreWrapper *)obj->store;
		if (storeWrapper == NULL)
			ThrowErr(exStore, kNSErrInvalidEntry);
		CheckWriteProtect(storeWrapper->store());

		RefVar soupObj(obj->handler);
		RefVar frame(obj->object);
		obj->object = NILREF;
		if (!EQ(soupObj, inSoup))
		{
			PlainSoupAdd(inSoup, frame);
			RefVar uSoup(FindSoupInCache(gUnionSoups, GetFrameSlot(soupObj, SYMA(theName))));
			if (NOTNIL(uSoup))
				EachSoupCursorDo(uSoup, kEntrySoupChanged, inEntry, frame);
			EntryRemoveFromSoup(inEntry);
			ReplaceObject(inEntry, frame);
		}
	}
}


int
GetLargeObjectSize(CStoreWrapper * inStoreWrapper, PSSId inId, StoreRef inClass, void * ioData)
{
	size_t size = StorageSizeOfLargeObject(inStoreWrapper->store(), inId);
	*(size_t *)ioData += size;
	return 0;
}


size_t
EntrySize(PSSId inId, CStoreWrapper * inStoreWrapper, bool inCountVBOs)
{
	size_t size = 0;
	OSERRIF(inStoreWrapper->store()->getObjectSize(inId, &size));

	StoreObjectHeader root;
	OSERRIF(inStoreWrapper->store()->read(inId, 0, &root, sizeof(root)));
	if (root.textBlockId != 0)
	{
		size_t textSize = 0;
		OSERRIF(inStoreWrapper->store()->getObjectSize(root.textBlockId, &textSize));
		size += textSize;
	}

	if (inCountVBOs && (root.flags & 0x01))
	{
		CStoreObjectReader reader(inStoreWrapper, inId, NULL);
		newton_try
		{
			reader.eachLargeObjectDo(GetLargeObjectSize, &size);
		}
		cleanup
		{
			reader.~CStoreObjectReader();
		}
		end_try;
	}

	return size;
}


size_t
EntrySize(RefArg inEntry)
{
	if (!IsFaultBlock(inEntry))
		ThrowErr(exStore, kNSErrNotASoupEntry);

	FaultObject * obj = (FaultObject *)NoFaultObjectPtr(inEntry);
	if (ISNIL(obj->store))
		return RINT(ForwardEntryMessage(inEntry, SYMA(EntrySize)));
	return EntrySize(RINT(obj->id), (CStoreWrapper *)obj->store, YES);
}


size_t
EntrySizeWithoutVBOs(RefArg inEntry)
{
	if (!IsFaultBlock(inEntry))
		ThrowErr(exStore, kNSErrNotASoupEntry);

	FaultObject * obj = (FaultObject *)NoFaultObjectPtr(inEntry);
	if (ISNIL(obj->store))
		return RINT(ForwardEntryMessage(inEntry, SYMA(EntrySize)));
	return EntrySize(RINT(obj->id), (CStoreWrapper *)obj->store, NO);
}


size_t
EntryTextSize(RefArg inEntry)
{
	if (!IsFaultBlock(inEntry))
		ThrowErr(exStore, kNSErrNotASoupEntry);

	FaultObject * obj = (FaultObject *)NoFaultObjectPtr(inEntry);
	CStoreWrapper * storeWrapper = (CStoreWrapper *)obj->store;
	if (ISNIL(storeWrapper))
		return RINT(ForwardEntryMessage(inEntry, SYMA(EntryTextSize)));

	size_t textSize = 0;
	StoreObjectHeader root;
	OSERRIF(storeWrapper->store()->read(RINT(obj->id), 0, &root, sizeof(root)));
	if (root.textBlockId != 0)
		OSERRIF(storeWrapper->store()->getObjectSize(root.textBlockId, &textSize));
	return textSize;
}


ULong
EntryUniqueId(RefArg inEntry)
{
	if (!IsFaultBlock(inEntry))
		ThrowErr(exStore, kNSErrNotASoupEntry);

	FaultObject * obj = (FaultObject *)NoFaultObjectPtr(inEntry);
	CStoreWrapper * storeWrapper = (CStoreWrapper *)obj->store;
	if (ISNIL(storeWrapper))
		return RINT(ForwardEntryMessage(inEntry, SYMA(EntryUniqueId)));

	ULong uniqueId;
	OSERRIF(storeWrapper->store()->read(RINT(obj->id), offsetof(StoreObjectHeader, uniqueId), &uniqueId, sizeof(uniqueId)));
	return uniqueId;
}


ULong
EntryModTime(RefArg inEntry)
{
	if (!IsFaultBlock(inEntry))
		ThrowErr(exStore, kNSErrNotASoupEntry);

	FaultObject * obj = (FaultObject *)NoFaultObjectPtr(inEntry);
	CStoreWrapper * storeWrapper = (CStoreWrapper *)obj->store;
	if (ISNIL(storeWrapper))
		return RINT(ForwardEntryMessage(inEntry, SYMA(EntryUniqueId)));

	ULong modTime;
	OSERRIF(storeWrapper->store()->read(RINT(obj->id), offsetof(StoreObjectHeader, modTime), &modTime, sizeof(modTime)));
	if (modTime == 0xFFFFFFFF)
		modTime = 0;
	return modTime;
}


struct EntryDirtyLink
{
	EntryDirtyLink * next;
	ObjHeader * obj;
};

bool
EntryDirty1(ObjHeader * inObj, EntryDirtyLink * inLink)
{
	EntryDirtyLink * link;
	for (link = inLink; link != NULL; link = link->next)
		if (link->obj == inObj)
			return NO;

	if (inObj->flags & kObjDirty)
		return YES;

	if (inObj->flags & kObjSlotted)
	{
		EntryDirtyLink aLink = { inLink, inObj };
		for (ArrayIndex i = 0, count = ARRAYLENGTH(inObj); i < count; ++i)
		{
			Ref r = ((ArrayObject *)inObj)->slot[i];
			if (ISREALPTR(r) && EntryDirty1(ObjectPtr(r), &aLink))
				return YES;
		}
	}

	return NO;
}


bool
EntryDirty(Ref inEntry)
{
	if (ISREALPTR(inEntry))
	{
		if (IsFaultBlock(inEntry)
		&&  NOTNIL(((FaultObject *)NoFaultObjectPtr(inEntry))->object))
			return EntryDirty1(ObjectPtr(inEntry), NULL);
	}
	return NO;
}


bool
EntryIsResident(Ref inEntry)
{
	if (!IsFaultBlock(inEntry))
		ThrowErr(exStore, kNSErrNotASoupEntry);
	return NOTNIL(((FaultObject *)NoFaultObjectPtr(inEntry))->object);
}


bool
EntryValid(RefArg inEntry)
{
	if (IsFaultBlock(inEntry))
	{
		FaultObject * soupObj = (FaultObject *)NoFaultObjectPtr(inEntry);
		CStoreWrapper * storeWrapper = (CStoreWrapper *)soupObj->store;
		if (ISNIL(storeWrapper))	// yup
			return NOTNIL(ForwardEntryMessage(inEntry, SYMA(EntryValid)));
		if (IsValidStore(storeWrapper->store()))
			return YES;
		for (int i = Length(gPackageStores) - 1; i >= 0; i--)
		{
			if (storeWrapper == (CStoreWrapper *)GetFrameSlot(GetArraySlot(gPackageStores, i), SYMA(store)))
				return YES;
		}
	}
	return NO;
}

#pragma mark -

/*----------------------------------------------------------------------
	E n t r y   P r o x i e s
----------------------------------------------------------------------*/

void
CheckProxyEntry(RefArg inEntry)
{
	if (!(IsFaultBlock(inEntry) && IsProxyEntry(inEntry)))
		ThrowErr(exStore, kNSErrNotASoupEntry);
}


Ref
NewProxyEntry(RefArg inHandler, RefArg inObj)
{
	return MakeFaultBlock(inHandler, (CStoreWrapper *)NILREF, 0, inObj);
}


bool
IsProxyEntry(RefArg inEntry)
{
	return ISNIL(((FaultObject *)NoFaultObjectPtr(inEntry))->store);
}


void
EntrySetHandler(RefArg inEntry, RefArg inHandler)
{
	CheckProxyEntry(inEntry);
	((FaultObject *)NoFaultObjectPtr(inEntry))->handler = inHandler;
}


Ref
EntryHandler(RefArg inEntry)
{
	CheckProxyEntry(inEntry);
	return ((FaultObject *)NoFaultObjectPtr(inEntry))->handler;
}


void
EntrySetCachedObject(RefArg inEntry, RefArg inObject)
{
	CheckProxyEntry(inEntry);
	((FaultObject *)NoFaultObjectPtr(inEntry))->object = inObject;
}


Ref
EntryCachedObject(RefArg inEntry)
{
	CheckProxyEntry(inEntry);
	return ((FaultObject *)NoFaultObjectPtr(inEntry))->object;
}

#pragma mark -

/*----------------------------------------------------------------------
	E n t r y   A l i a s e s
----------------------------------------------------------------------*/

extern Ref gStores;
extern Ref gPackageStores;
extern bool gNeedsInternalSignatures;

Ref
MakeEntryAlias(RefArg inEntry)
{
	RefVar soup(EntrySoup(inEntry));

	if (gNeedsInternalSignatures)
	{
		if (EQRef(GetFrameSlot(soup, SYMA(storeObj)), GetArraySlot(gStores, 0)))
			gNeedsInternalSignatures = NO;
	}

	RefVar	alias(AllocateArray(SYMA(alias), 4));
	SetArraySlot(alias, 1, SoupGetSignature(soup));
	SetArraySlot(alias, 2, MAKEINT(EntryUniqueId(inEntry)));
	SetArraySlot(alias, 3, CommonSoupGetName(soup));
	return alias;
}


bool
IsEntryAlias(RefArg inEntry)
{
	return IsFaultBlock(inEntry) && EQRef(ClassOf(inEntry), RSYMalias);
}


Ref
ResolveEntryAliasInStores(RefArg inRcvr, RefArg inStores)
{
	if (NOTNIL(inStores))
	{
		RefVar soup;
		RefVar aliasSoupSignature(GetArraySlot(inRcvr, 1));
		for (int i = Length(inStores) - 1; i >= 0; --i)
		{
			RefVar aliasSoupName(GetArraySlot(inRcvr, 3));
			RefVar store(GetArraySlot(inStores, i));
			soup = StoreGetSoup(store, aliasSoupName);
			if (NOTNIL(soup) && EQ(SoupGetSignature(soup), aliasSoupSignature))
			{
				SKey entryIdKey;
				PSSId entryId;
				CSoupIndex * soupIndex = GetSoupIndexObject(soup, 0);
				entryIdKey = (int)RINT(GetArraySlot(inRcvr, 2));
				if (soupIndex->find(&entryIdKey, NULL, (SKey *)&entryId, NO) == noErr)
					return GetEntry(soup, entryId);
				else
					break;
			}
			
		}
	}
	return NILREF;
}


Ref
ResolveEntryAlias(RefArg inRcvr)
{
	RefVar entry;
	if (IsEntryAlias(inRcvr))
	{
		entry = ResolveEntryAliasInStores(inRcvr, gStores);
		if (ISNIL(entry))
			entry = ResolveEntryAliasInStores(inRcvr, gPackageStores);
	}
	return entry;
}


bool
CompareAliasAndEntry(RefArg inRcvr, RefArg inEntry)
{
	bool isSame = NO;
	if (IsSoupEntry(inEntry)
	&&  EntryUniqueId(inEntry) == RINT(GetArraySlot(inRcvr, 2)))
	{
		RefVar theSoup(EntrySoup(inEntry));
		if (EQ(SoupGetSignature(EntrySoup(inEntry)), GetArraySlot(inRcvr, 1)))
			isSame = YES;
	}
	return isSame;
}


bool
IsSameEntry(RefArg inRcvr, RefArg inEntry)
{
	bool isRcvrAlias = IsEntryAlias(inRcvr);
	bool isEntryAlias = IsEntryAlias(inEntry);
	if (isRcvrAlias)
	{
		if (!isEntryAlias)
			return CompareAliasAndEntry(inRcvr, inEntry);
		return EQ(GetArraySlot(inRcvr, 2), GetArraySlot(inEntry, 2))
			 && EQ(GetArraySlot(inRcvr, 1), GetArraySlot(inEntry, 1));
	}
	else if (isEntryAlias)
	{
		return CompareAliasAndEntry(inEntry, inRcvr);
	}
	return EQ(inRcvr, inEntry);
}
