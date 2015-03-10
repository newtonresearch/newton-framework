/*
	File:		Soups.h

	Contains:	Common soup declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__SOUPS_H)
#define __SOUPS_H 1


Ref	InitQueries(void);

/*------------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	SoupIsValid(RefArg inRcvr);
Ref	SoupGetFlags(RefArg inRcvr);
Ref	SoupSetFlags(RefArg inRcvr, RefArg inFlags);
Ref	SoupGetIndexesModTime(RefArg inRcvr);
Ref	SoupGetInfoModTime(RefArg inRcvr);
Ref	CommonSoupGetName(RefArg inRcvr);
Ref	SoupAddFlushed(RefArg inRcvr, RefArg inFrame);
Ref	SoupAddFlushedWithUniqueID(RefArg inRcvr, RefArg inFrame);
Ref	CommonSoupAddEntry(RefArg inRcvr, RefArg entry, unsigned char);	// the original has a fourth, unused, unsigned char arg
}

Ref	SoupQuery(RefArg inRcvr, RefArg inQuerySpec);
Ref	SoupGetName(RefArg inRcvr);
Ref	SoupSetName(RefArg inRcvr, RefArg inName);
Ref	SoupGetSignature(RefArg inRcvr);
Ref	SoupSetSignature(RefArg inRcvr, int inSignature);
Ref	SoupGetInfo(RefArg inRcvr, RefArg inTag);
Ref	SoupSetInfo(RefArg inRcvr, RefArg inTag, RefArg info);
Ref	SoupGetAllInfo(RefArg inRcvr);
Ref	SoupSetAllInfo(RefArg inRcvr, RefArg info);
Ref	SoupCopyEntries(RefArg inRcvr, RefArg inFromSoup);
Ref	SoupRemoveAllEntries(RefArg inRcvr);
Ref	SoupRemoveFromStore(RefArg inRcvr);
Ref	SoupFlush(RefArg inRcvr);
Ref	SoupGetStore(RefArg inRcvr);
Ref	SoupAddIndex(RefArg inRcvr, RefArg index);
Ref	SoupRemoveIndex(RefArg inRcvr, RefArg index);
Ref	SoupGetIndexes(RefArg inRcvr);
Ref	SoupGetNextUId(RefArg inRcvr);
Ref	SoupAdd(RefArg inRcvr, RefArg entry);
void	SoupChanged(RefArg inRcvr, bool doFlush);
Ref	SoupAddWithUniqueID(RefArg inRcvr, RefArg);


Ref	FindSoupInCache(RefArg inCache, RefArg inName);
void	SoupCacheRemoveAllEntries(RefArg inSoup);


extern "C" {
Ref	FSetContains(RefArg inRcvr, RefArg inArray, RefArg inMember);
Ref	FSetUnion(RefArg inRcvr, RefArg ioArray, RefArg inArray2, RefArg inUnique);


}


#endif	/* __SOUPS_H */
