/*
	File:		Soups.cc

	Contains:	Common soup functions.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ROMResources.h"
#include "Funcs.h"
#include "Unicode.h"
#include "FaultBlocks.h"
#include "RefMemory.h"
#include "StoreWrapper.h"
#include "Soups.h"
#include "PlainSoup.h"
#include "Cursors.h"

extern ULong	RealClock(void);
extern void		CheckWriteProtect(CStore * inStore);
extern Ref		SafeEntryAdd(RefArg inRcvr, RefArg inFrame, RefArg inId, unsigned char inFlags);

DeclareException(exBadType, exRootException);


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C"
{
Ref	FQuery(RefArg inRcvr, RefArg inSoup, RefArg inQuerySpec);
}


#pragma mark Entry cache
/*------------------------------------------------------------------------------
	E n t r y   C a c h e
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Allocate a soup cache.
	Args:		--
	Return:	a weak array
------------------------------------------------------------------------------*/

Ref
MakeEntryCache(void)
{
	return AllocateArray(kWeakArrayClass, 8);
}


Ref
GetEntry(RefArg inSoup, PSSId inId)
{
	RefVar theCache(GetFrameSlot(inSoup, SYMA(cache)));
	RefVar theEntry(FindEntryInCache(theCache, inId));
	if (ISNIL(theEntry))
	{
		theEntry = MakeFaultBlock(inSoup, (CStoreWrapper *)GetFrameSlot(inSoup, SYMA(TStore)), inId);
		PutEntryIntoCache(theCache, theEntry);
	}
	return theEntry;
}


void
PutEntryIntoCache(RefArg inCache, RefArg inEntry)
{
	RefVar	entry;
	ArrayIndex indexOfLastNilSlot = 0;
	ArrayIndex count = Length(inCache);
	for (ArrayIndex i = 0; i < count; ++i)
	{
		entry = GetArraySlot(inCache, i);
		if (NOTNIL(entry))
		{
			if (i != indexOfLastNilSlot)
			{
				SetArraySlot(inCache, indexOfLastNilSlot, entry);
				SetArraySlot(inCache, i, NILREF);
			}
			indexOfLastNilSlot++;
		}
	}
	if (indexOfLastNilSlot == count)
		SetLength(inCache, count+8);
	else if (indexOfLastNilSlot < 8)
		SetLength(inCache, 8);
	SetArraySlot(inCache, indexOfLastNilSlot, inEntry);
}


Ref
FindEntryInCache(RefArg inCache, PSSId inId)
{
	RefVar	theEntry;
	for (ArrayIndex i = 0, count = Length(inCache); i < count; ++i)
	{
		theEntry = GetArraySlot(inCache, i);
		if (NOTNIL(theEntry))
		{
			FaultObject * soupObj = (FaultObject *)NoFaultObjectPtr(theEntry);
			PSSId soupObjId = RINT(soupObj->id);
			if (soupObjId == inId)
				return theEntry;
		}
	}
	return NILREF;
}


Ref
FindSoupInCache(RefArg inCache, RefArg inName)
{
	RefVar	theSoup;
	for (ArrayIndex i = 0, count = Length(inCache); i < count; ++i)
	{
		theSoup = GetArraySlot(inCache, i);
		if (NOTNIL(theSoup))
		{
			RefVar soupName(GetFrameSlot(theSoup, SYMA(theName)));
			if (CompareStringNoCase(GetUString(inName), GetUString(soupName)) == 0)
				return theSoup;
		}
	}
	return NILREF;
}


void
DeleteEntryFromCache(RefArg inCache, RefArg inEntry)
{
	for (ArrayIndex i = 0, count = Length(inCache); i < count; ++i)
	{
		if (EQ(GetArraySlot(inCache, i), inEntry))
			SetArraySlot(inCache, i, RA(NILREF));
	}
}


void
InvalidateCacheEntries(RefArg inCache)
{
	RefVar	theEntry;
	for (ArrayIndex i = 0, count = Length(inCache); i < count; ++i)
	{
		theEntry = GetArraySlot(inCache, i);
		if (NOTNIL(theEntry))
		{
			InvalFaultBlock(theEntry);
			SetArraySlot(inCache, i, RA(NILREF));
		}
	}
}


#pragma mark -
/*----------------------------------------------------------------------
	S o u p   F u n c t i o n s
----------------------------------------------------------------------*/

void
SoupChanged(RefArg inSoup, bool inFlush)
{
	int flags;
	Ref flagsSlot = GetFrameSlot(inSoup, SYMA(flags));
	if (ISNIL(flagsSlot))
		flags = 0;
	else
	{
		flags = RINT(flagsSlot);
		if (flags == 0x03)
			return;
	}
	SetFrameSlot(inSoup, SYMA(flags), MAKEINT(flags | 0x03));
	if (inFlush)
		WriteFaultBlock(inSoup);
}


void
SoupCacheRemoveAllEntries(RefArg inSoup)
{
	RefVar theCache(GetFrameSlot(inSoup, SYMA(cache)));
	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(GetFrameSlot(inSoup, SYMA(storeObj)), SYMA(store));
	RefVar cachedEntry;
	RefVar realSoup;
	for (int i = Length(theCache) - 1; i >= 0; i--)
	{
		cachedEntry = GetArraySlot(theCache, i);
		if (NOTNIL(cachedEntry))
		{
			FaultObject * soupObj = (FaultObject *)NoFaultObjectPtr(cachedEntry);
			PSSId soupObjId = RINT(soupObj->id);
			realSoup = soupObj->object;
			SetArraySlot(theCache, i, RA(NILREF));
			if (NOTNIL(realSoup))
				ReplaceObject(cachedEntry, realSoup);
			else
				ReplaceObject(cachedEntry, LoadPermObject(storeWrapper, soupObjId, NULL));
		}
	}
}


#pragma mark -
/*----------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
----------------------------------------------------------------------*/

Ref
SoupIsValid(RefArg inRcvr)
{
	return MAKEBOOLEAN(NOTNIL(GetFrameSlot(inRcvr, SYMA(_proto)))						// must have a soupIndexInfo frame
							 && StoreIsValid(GetFrameSlot(inRcvr, SYMA(storeObj))));		// and a valid store frame
}


Ref
SoupGetFlags(RefArg inRcvr)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);
	return GetFrameSlot(soupIndexInfo, SYMA(flags));
}


Ref
SoupSetFlags(RefArg inRcvr, RefArg inFlags)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);
	CheckWriteProtect(PlainSoupGetStore(inRcvr));
	
	SetFrameSlot(soupIndexInfo, SYMA(flags), inFlags);
	WriteFaultBlock(soupIndexInfo);
	return NILREF;
}


Ref
SoupGetIndexesModTime(RefArg inRcvr)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);
	return GetFrameSlot(soupIndexInfo, SYMA(indexesModTime));
}


Ref
SoupGetInfoModTime(RefArg inRcvr)
{
	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);
	return GetFrameSlot(soupIndexInfo, SYMA(infoModTime));
}


Ref
CommonSoupGetName(RefArg inRcvr)
{
	return GetFrameSlot(inRcvr, SYMA(theName));
}


Ref
SoupAddFlushed(RefArg inRcvr, RefArg inFrame)
{
	return CommonSoupAddEntry(inRcvr, inFrame, 0x07);
}


Ref
SoupAddFlushedWithUniqueID(RefArg inRcvr, RefArg inFrame)
{
	return CommonSoupAddEntry(inRcvr, inFrame, 0x01);
}


Ref
CommonSoupAddEntry(RefArg inRcvr, RefArg inFrame, unsigned char inFlags)	// the original has a fourth, unused, unsigned char arg
{
	ULong objFlags = ObjectFlags(inFrame);
	if ((objFlags & kObjFrame) == 0)
		ThrowErr(exBadType, kNSErrNotAFrame);
	if ((objFlags & kObjReadOnly) != 0)
		ThrowExFramesWithBadValue(kNSErrObjectReadOnly, inFrame);

	RefVar soupIndexInfo(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(soupIndexInfo))
		ThrowErr(exStore, kNSErrInvalidSoup);

	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(TStore));
	CheckWriteProtect(storeWrapper->store());

	RefVar entry;
	RefVar indexNextUId(GetFrameSlot(inRcvr, SYMA(indexNextUId)));
	storeWrapper->lockStore();
	newton_try
	{
		entry = SafeEntryAdd(inRcvr, inFrame, indexNextUId, inFlags);
		int nextUId = RINT(indexNextUId);
		bool updateNextUId = ((inFlags & 0x04) != 0);
		if (!updateNextUId)
		{
			int uid = RINT(GetFrameSlot(inFrame, SYMA(_uniqueId)));
			if (uid >= nextUId)
			{
				nextUId = uid;
				updateNextUId = true;
			}
		}
		if (updateNextUId)
			SetFrameSlot(inRcvr, SYMA(indexNextUId), MAKEINT(nextUId+1));
		if (NOTNIL(GetFrameSlot(soupIndexInfo, SYMA(lastUId))))
		{
			SetFrameSlot(soupIndexInfo, SYMA(lastUId), RA(NILREF));
			SoupChanged(soupIndexInfo, false);
			WriteFaultBlock(soupIndexInfo);
		}
		else
			SoupChanged(soupIndexInfo, true);
	}
	cleanup
	{
		storeWrapper->abort();
		AbortSoupIndexes(inRcvr);
	}
	end_try;
	storeWrapper->unlockStore();

	return entry;
}


#pragma mark -
/*----------------------------------------------------------------------
	S o u p   F u n c t i o n s
	These call back into the NewtonScript functions in the soup frame.
----------------------------------------------------------------------*/

Ref
SoupQuery(RefArg inSoup, RefArg inQuerySpec)
{
	RefVar args(MakeArray(1));
	SetArraySlot(args, 0, inQuerySpec);
	return DoMessage(inSoup, SYMA(Query), args);
}


Ref
FQuery(RefArg inRcvr, RefArg inSoup, RefArg inQuerySpec)
{
	return SoupQuery(inSoup, inQuerySpec);
}

#pragma mark Soup Info

Ref
SoupSetName(RefArg inRcvr, RefArg inName)
{
	RefVar args(MakeArray(1));
	SetArraySlot(args, 0, inName);
	return DoMessage(inRcvr, SYMA(SetName), args);
}


Ref
SoupGetName(RefArg inRcvr)
{
	return DoMessage(inRcvr, SYMA(GetName), RA(NILREF));
}


Ref
SoupSetSignature(RefArg inRcvr, int inSignature)
{
	RefVar args(MakeArray(1));
	SetArraySlot(args, 0, MAKEINT(inSignature));
	return DoMessage(inRcvr, SYMA(SetSignature), args);
}


Ref
SoupGetSignature(RefArg inRcvr)
{
	return DoMessage(inRcvr, SYMA(GetSignature), RA(NILREF));
}


Ref
SoupSetInfo(RefArg inRcvr, RefArg inTag, RefArg info)
{
	RefVar args(MakeArray(2));
	SetArraySlot(args, 0, inTag);
	SetArraySlot(args, 1, info);
	return DoMessage(inRcvr, SYMA(SetInfo), args);
}


Ref
SoupGetInfo(RefArg inRcvr, RefArg inTag)
{
	RefVar args(MakeArray(1));
	SetArraySlot(args, 0, inTag);
	return DoMessage(inRcvr, SYMA(GetInfo), args);
}


Ref
SoupSetAllInfo(RefArg inRcvr, RefArg info)
{
	RefVar args(MakeArray(1));
	SetArraySlot(args, 0, info);
	return DoMessage(inRcvr, SYMA(SetAllInfo), args);
}


Ref
SoupGetAllInfo(RefArg inRcvr)
{
	return DoMessage(inRcvr, SYMA(GetAllInfo), RA(NILREF));
}


Ref
SoupGetStore(RefArg inRcvr)
{
	return DoMessage(inRcvr, SYMA(GetStore), RA(NILREF));
}


Ref
SoupGetNextUId(RefArg inRcvr)
{
	return DoMessage(inRcvr, SYMA(GetNextUId), RA(NILREF));
}


Ref
SoupRemoveFromStore(RefArg inRcvr)
{
	return DoMessage(inRcvr, SYMA(RemoveFromStore), RA(NILREF));
}

#pragma mark Update indexes

Ref
SoupAddIndex(RefArg inRcvr, RefArg index)
{
	RefVar args(MakeArray(1));
	SetArraySlot(args, 0, index);
	return DoMessage(inRcvr, SYMA(AddIndex), args);
}


Ref
SoupRemoveIndex(RefArg inRcvr, RefArg index)
{
	RefVar args(MakeArray(1));
	SetArraySlot(args, 0, index);
	return DoMessage(inRcvr, SYMA(RemoveIndex), args);
}


Ref
SoupGetIndexes(RefArg inRcvr)
{
	return DoMessage(inRcvr, SYMA(GetIndexes), RA(NILREF));
}

#pragma mark Add/Remove entries

Ref
SoupAdd(RefArg inRcvr, RefArg inFrame)
{
	RefVar args(MakeArray(1));
	SetArraySlot(args, 0, inFrame);
	return DoMessage(inRcvr, SYMA(Add), args);
}


Ref
SoupAddWithUniqueID(RefArg inRcvr, RefArg inFrame)
{
	RefVar args(MakeArray(1));
	SetArraySlot(args, 0, inFrame);
	return DoMessage(inRcvr, SYMA(AddWithUniqueId), args);
}


Ref
SoupCopyEntries(RefArg inRcvr, RefArg inSoup)
{
	RefVar args(MakeArray(1));
	SetArraySlot(args, 0, inSoup);
	return DoMessage(inRcvr, SYMA(CopyEntries), args);
}


Ref
SoupRemoveAllEntries(RefArg inRcvr)
{
	return DoMessage(inRcvr, SYMA(RemoveAllEntries), RA(NILREF));
}

#pragma mark DEPRECATED

Ref
SoupFlush(RefArg inRcvr)
{
	return DoMessage(inRcvr, SYMA(Flush), RA(NILREF));
}

