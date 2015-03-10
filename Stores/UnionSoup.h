/*
	File:		UnionSoup.h

	Contains:	Union soup plain C interface.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__UNIONSOUP_H)
#define __UNIONSOUP_H 1


void	AddToUnionSoup(RefArg name, RefArg soup);
void	RemoveFromUnionSoup(RefArg name, RefArg soup);


extern "C" {
Ref	CheckStoresWriteProtect(RefArg inRcvr);

Ref	FGetUnionSoup(RefArg inRcvr, RefArg inName);
Ref	FGetUnionSoupAlways(RefArg inRcvr, RefArg inName);

Ref	UnionSoupAdd(RefArg inRcvr, RefArg inFrame);
Ref	UnionSoupGetSize(RefArg inRcvr);

Ref	UnionSoupAddIndex(RefArg inRcvr, RefArg index);
Ref	UnionSoupRemoveIndex(RefArg inRcvr, RefArg index);

Ref	UnionSoupHasTags(RefArg inRcvr);
Ref	UnionSoupGetTags(RefArg inRcvr);
Ref	UnionSoupAddTags(RefArg inRcvr, RefArg inTags);
Ref	UnionSoupRemoveTags(RefArg inRcvr, RefArg inTags);
Ref	UnionSoupModifyTag(RefArg inRcvr, RefArg inOldTag, RefArg inNewTag);

Ref	UnionSoupGetSize(RefArg inRcvr);

//Ref UnionSoupFlush(RefArg inRcvr);	// discontinued
}

#endif /* __UNIONSOUP_H */
