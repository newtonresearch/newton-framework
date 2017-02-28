/*
	File:		UnionSoup.cc

	Contains:	Union soup functions.

	Written by:	Newton Research Group, 2007.
*/

#include "Objects.h"
#include "ROMResources.h"
#include "StoreWrapper.h"
#include "Cursors.h"
#include "FaultBlocks.h"
#include "Soups.h"
#include "PlainSoup.h"
#include "UnionSoup.h"
#include "OSErrors.h"
#include "NewtonScript.h"


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

Ref gUnionSoups;


/*------------------------------------------------------------------------------
	I n t e r f a c e s
------------------------------------------------------------------------------*/

// from Stores.cc
extern Ref	gStores;
extern void	CheckWriteProtect(CStore * inStore);



Ref
CheckStoresWriteProtect(RefArg inRcvr)
{
	RefVar	soupList(GetFrameSlot(inRcvr, SYMA(soupList)));
	RefVar	theSoup;
	for (int i = Length(soupList) - 1; i >= 0; i--)
	{
		theSoup = GetArraySlot(soupList, i);
		CStoreWrapper *	storeWrapper = (CStoreWrapper *)GetFrameSlot(theSoup, SYMA(TStore));
		CheckWriteProtect(storeWrapper->store());
	}
	return soupList;
}


Ref
CheckSoupsSortTables(RefArg inSoup1, RefArg inSoup2)
{
	RefVar	indexes1(GetFrameSlot(inSoup1, SYMA(indexes)));
	RefVar	path, index1, index2;
	int		i, count = Length(indexes1);
	for (i = count - 1; i >= 0; i--)
	{
		index1 = GetArraySlot(indexes1, i);
		path = GetFrameSlot(index1, SYMA(path));
		index2 = IndexPathToIndexDesc(inSoup2, path, NULL);
		if (NOTNIL(index2))
		{
			if (EQ(GetFrameSlot(index1, SYMA(sortId)), GetFrameSlot(index2, SYMA(sortId))))
				return path;
		}
	}
	return NILREF;
}


static NewtonErr
CheckUnion(RefArg inUnionSoup)
{
	RefVar	soupList(GetFrameSlot(inUnionSoup, SYMA(soupList)));
	for (int i = Length(soupList) - 1; i >= 1; i--)
	{
		RefVar pSoup2(GetFrameSlot(GetArraySlot(soupList, i-1), SYMA(_proto)));
		RefVar pSoup1(GetFrameSlot(GetArraySlot(soupList, i), SYMA(_proto)));
		if (NOTNIL(CheckSoupsSortTables(pSoup1, pSoup2)))
			return kNSErrInvalidUnion;
	}
	return noErr;
}


Ref
GetUnionSoup(RefArg inName)
{
	RefVar	uSoup(FindSoupInCache(gUnionSoups, inName));
	if (ISNIL(uSoup))
	{
		NewtonErr	err;
		uSoup = Clone(RA(unionSoupPrototype));
		RefVar	soupList(MakeArray(0));
		RefVar	store;
		ArrayIndex count = Length(gStores);
		for (ArrayIndex i = 0; i < count; ++i)
		{
			store = GetArraySlot(gStores, i);
			if (NOTNIL(StoreHasSoup(store, inName)))
				AddArraySlot(soupList, StoreGetSoup(store, inName));
		}
		if (Length(soupList) > 0)
		{
			SetFrameSlot(uSoup, SYMA(soupList), soupList);
			SetFrameSlot(uSoup, SYMA(theName), SoupGetName(GetArraySlot(soupList, 0)));
			SetFrameSlot(uSoup, SYMA(cursors), MakeEntryCache());
			if (count > 0)	// under just what circumstances will there be no stores?
				PutEntryIntoCache(gUnionSoups, uSoup);
			if ((err = CheckUnion(uSoup)) != noErr)
				SetFrameSlot(uSoup, SYMA(errorCode), MAKEINT(err));
		}
		else
		// named soup doesnÕt exist on any store
			uSoup = NILREF;
	}
	return uSoup;
}


Ref
FGetUnionSoup(RefArg inRcvr, RefArg inName)
{
	return GetUnionSoup(inName);
}


Ref
GetUnionSoupAlways(RefArg inName)
{
	RefVar	uSoup(GetUnionSoup(inName));
	if (ISNIL(uSoup))
	{
		uSoup = Clone(RA(unionSoupPrototype));
		SetFrameSlot(uSoup, SYMA(soupList), MakeArray(0));
		SetFrameSlot(uSoup, SYMA(theName), TotalClone(inName));
		SetFrameSlot(uSoup, SYMA(cursors), MakeEntryCache());
		PutEntryIntoCache(gUnionSoups, uSoup);
	}
	return uSoup;
}


Ref
FGetUnionSoupAlways(RefArg inRcvr, RefArg inName)
{
	return GetUnionSoupAlways(inName);
}


void
AddToUnionSoup(RefArg inUnionSoup, RefArg newSoup)
{
	NewtonErr	err;
	RefVar		uSoup;

	if (IsString(inUnionSoup))
	{
		// can specify by name
		uSoup = FindSoupInCache(gUnionSoups, inUnionSoup);
		if (ISNIL(uSoup))
			return;
	}
	else
		uSoup = inUnionSoup;

	RefVar	soupList(GetFrameSlot(uSoup, SYMA(soupList)));
	AddArraySlot(soupList, newSoup);
	if ((err = CheckUnion(uSoup)) != noErr)
		SetFrameSlot(uSoup, SYMA(errorCode), MAKEINT(err));

	EachSoupCursorDo(uSoup, err ? kSoupSet : kSoupAdded, newSoup);
}


void
RemoveFromUnionSoup(RefArg inUnionSoup, RefArg oldSoup)
{
	NewtonErr	err;
	RefVar		uSoup;

	if (IsString(inUnionSoup))
	{
		// can specify by name
		uSoup = FindSoupInCache(gUnionSoups, inUnionSoup);
		if (ISNIL(uSoup))
			return;
	}
	else
		uSoup = inUnionSoup;

	RefVar	soupList(GetFrameSlot(uSoup, SYMA(soupList)));
	int		i;
	for (i = Length(soupList) - 1; i >= 0; i--)
	{
		if (EQ(GetArraySlot(soupList, i), oldSoup))
		{
			ArrayMunger(soupList, i, 1, RA(NILREF), 0, 0);
			break;
		}
	}
	if (i > 0)
	{
		if ((err = CheckUnion(uSoup)) != noErr
		||  NOTNIL(GetFrameSlot(uSoup, SYMA(errorCode))))
		{
			RefVar errRef, sp;
			if (err)
			{
				errRef = MAKEINT(err);
				sp = GetArraySlot(soupList, Length(soupList) - 1);
			}
			else
				sp = uSoup;
			SetFrameSlot(uSoup, SYMA(errorCode), errRef);
		EachSoupCursorDo(uSoup, kSoupSet, sp);
		}
	}
	EachSoupCursorDo(uSoup, kSoupRemoved, oldSoup);
}

#pragma mark -

Ref
UnionSoupAdd(RefArg inRcvr, RefArg inFrame)
{
	RefVar	soupList(GetFrameSlot(inRcvr, SYMA(soupList)));
	RefVar	theSoup(GetArraySlot(soupList, 0));
	SoupAdd(theSoup, inFrame);
	return inFrame;
}


Ref
UnionSoupGetSize(RefArg inRcvr)
{
	RefVar	soupList(GetFrameSlot(inRcvr, SYMA(soupList)));
	RefVar	theSoup;
	unsigned	unionSize = 0;
	for (int i = Length(soupList) - 1; i >= 0; i--)
	{
		theSoup = GetArraySlot(soupList, i);
		unionSize += RINT(PlainSoupGetSize(theSoup));
	}
	return MAKEINT(unionSize);
}

#pragma mark -

Ref
UnionSoupAddIndex(RefArg inRcvr, RefArg indexSpec)
{
	CheckStoresWriteProtect(inRcvr);

	RefVar	soupList(GetFrameSlot(inRcvr, SYMA(soupList)));
	RefVar	theSoup;
	for (ArrayIndex i = 0, count = Length(soupList); i < count; ++i)
	{
		theSoup = GetArraySlot(soupList, i);
		PlainSoupAddIndex(theSoup, indexSpec);
	}
	return NILREF;
}


Ref
UnionSoupRemoveIndex(RefArg inRcvr, RefArg indexPath)
{
	CheckStoresWriteProtect(inRcvr);

	RefVar	soupList(GetFrameSlot(inRcvr, SYMA(soupList)));
	RefVar	theSoup;
	for (ArrayIndex i = 0, count = Length(soupList); i < count; ++i)
	{
		theSoup = GetArraySlot(soupList, i);
		PlainSoupRemoveIndex(theSoup, indexPath);
	}
	return NILREF;
}

#pragma mark -

Ref
UnionSoupAddTags(RefArg inRcvr, RefArg inTags)
{
	CheckStoresWriteProtect(inRcvr);
	if (ISFALSE(UnionSoupHasTags(inRcvr)))
		ThrowErr(exStore, kNSErrNoTags);

	RefVar	soupList(GetFrameSlot(inRcvr, SYMA(soupList)));
	RefVar	theSoup;
	for (int i = Length(soupList) - 1; i >= 0; i--)
	{
		theSoup = GetArraySlot(soupList, i);
		PlainSoupAddTags(theSoup, inTags);
	}
	return NILREF;
}


Ref
UnionSoupHasTags(RefArg inRcvr)
{
	RefVar	soupList(GetFrameSlot(inRcvr, SYMA(soupList)));
	RefVar	theSoup;
	ArrayIndex count = Length(soupList);

	if (count == 0)
	{
		// no actual soups in the union; check the indexes
		RefVar	args(MakeArray(1));
		SetArraySlot(args, 1, CommonSoupGetName(inRcvr));
		RefVar	soupDef(NSCallGlobalFn(SYMA(GetSoupDef), args));
		if (NOTNIL(soupDef))
		{
			RefVar	indexes(GetFrameSlot(soupDef, SYMA(indexes)));
			RefVar	theIndex;
			if (NOTNIL(indexes))
			{
				count = Length(indexes);
				for (ArrayIndex i = 0; i < count; ++i)
				{
					theIndex = GetArraySlot(indexes, i);
					if (EQ(GetFrameSlot(theIndex, SYMA(type)), SYMA(tags)))
						return TRUEREF;	// doesnÕt look right, given the stuff below, but this is what the NS does
				}
			}
		}
		return NILREF;
	}

	for (ArrayIndex i = 0; i < count; ++i)
	{
		theSoup = GetArraySlot(soupList, i);
		if (!PlainSoupHasTags(theSoup))
			return NILREF;
	}
	return TRUEREF;
}


Ref
UnionSoupGetTags(RefArg inRcvr)
{
	RefVar	soupList(GetFrameSlot(inRcvr, SYMA(soupList)));
	RefVar	theSoup;
	RefVar	unionTags, theTags;
	for (ArrayIndex i = 0, count = Length(soupList); i < count; ++i)
	{
		theSoup = GetArraySlot(soupList, i);
		theTags = PlainSoupGetTags(theSoup);
		if (ISNIL(theTags))
			return NILREF;
		if (ISNIL(unionTags))
			unionTags = theTags;
		else
			unionTags = FSetUnion(RA(NILREF), unionTags, theTags, RA(TRUEREF));
	}
	return unionTags;
}


Ref
UnionSoupModifyTag(RefArg inRcvr, RefArg inOldTag, RefArg inNewTag)
{
	CheckStoresWriteProtect(inRcvr);
	if (ISFALSE(UnionSoupHasTags(inRcvr)))
		ThrowErr(exStore, kNSErrNoTags);

	RefVar	soupList(GetFrameSlot(inRcvr, SYMA(soupList)));
	RefVar	theSoup;
	for (int i = Length(soupList) - 1; i >= 0; --i)
	{
		theSoup = GetArraySlot(soupList, i);
		PlainSoupModifyTag(theSoup, inOldTag, inNewTag);
	}
	return NILREF;
}


Ref
UnionSoupRemoveTags(RefArg inRcvr, RefArg inTags)
{
	CheckStoresWriteProtect(inRcvr);
	if (ISFALSE(UnionSoupHasTags(inRcvr)))
		ThrowErr(exStore, kNSErrNoTags);

	RefVar	soupList(GetFrameSlot(inRcvr, SYMA(soupList)));
	RefVar	theSoup;
	for (int i = Length(soupList) - 1; i >= 0; --i)
	{
		theSoup = GetArraySlot(soupList, i);
		PlainSoupRemoveTags(theSoup, inTags);
	}
	return NILREF;
}

#pragma mark -

// discontinued
Ref
FlushSoupList(RefArg inSoupList)
{
	Ref		isFlushed = NILREF;
	RefVar	theSoup;
	for (int i = Length(inSoupList) - 1; i >= 0; --i)
	{
		theSoup = GetArraySlot(inSoupList, i);
		if (ISTRUE(PlainSoupFlush(theSoup)))
			isFlushed = TRUEREF;
	}
	return isFlushed;
}


Ref
UnionSoupFlush(RefArg inRcvr)
{
	RefVar	soupList(GetFrameSlot(inRcvr, SYMA(soupList)));
	return FlushSoupList(soupList);
}
