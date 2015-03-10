/*
	File:		PlainSoup.h

	Contains:	Plain soup plain C interface.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__PLAINSOUP_H)
#define __PLAINSOUP_H 1

extern "C" {
Ref PlainSoupMakeKey(RefArg inRcvr, RefArg key, RefArg path);

Ref PlainSoupIndexSizes(RefArg inRcvr);
Ref PlainSoupGetIndexes(RefArg inRcvr);
Ref PlainSoupAddIndex(RefArg inRcvr, RefArg index);
Ref PlainSoupRemoveIndex(RefArg inRcvr, RefArg index);

Ref PlainSoupHasTags(RefArg inRcvr);
Ref PlainSoupGetTags(RefArg inRcvr);
Ref PlainSoupAddTags(RefArg inRcvr, RefArg inTags);
Ref PlainSoupRemoveTags(RefArg inRcvr, RefArg inTags);
Ref PlainSoupModifyTag(RefArg inRcvr, RefArg inOldTag, RefArg inNewTag);

Ref PlainSoupGetStore(RefArg inRcvr);
Ref PlainSoupSetName(RefArg inRcvr, RefArg inName);
Ref PlainSoupGetInfo(RefArg inRcvr, RefArg inSlotSym);
Ref PlainSoupSetInfo(RefArg inRcvr, RefArg inSlotSym, RefArg inValue);
Ref PlainSoupGetAllInfo(RefArg inRcvr);
Ref PlainSoupSetAllInfo(RefArg inRcvr, RefArg info);
Ref PlainSoupGetSignature(RefArg inRcvr);
Ref PlainSoupSetSignature(RefArg inRcvr, RefArg inSignature);
Ref PlainSoupGetNextUId(RefArg inRcvr);
Ref PlainSoupGetSize(RefArg inRcvr);

Ref PlainSoupCopyEntries(RefArg inRcvr, RefArg inToSoup);
Ref PlainSoupCopyEntriesWithCallBack(RefArg inRcvr, RefArg inToSoup, RefArg inArg3, RefArg inArg4);
Ref PlainSoupRemoveAllEntries(RefArg inRcvr);
Ref PlainSoupRemoveFromStore(RefArg inRcvr);

Ref PlainSoupAdd(RefArg inRcvr, RefArg inFrame);
Ref PlainSoupAddWithUniqueId(RefArg inRcvr, RefArg inFrame);

Ref PlainSoupDirty(RefArg inRcvr);
Ref PlainSoupFlush(RefArg inRcvr);
}

#endif /* __PLAINSOUP_H */
