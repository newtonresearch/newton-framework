/*
	File:		Entries.h

	Contains:	Soup entry declarations.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__ENTRIES_H)
#define __ENTRIES_H 1

// Entries
bool	IsSoupEntry(Ref r);
bool	EntryDirty(Ref inEntry);
bool	EntryIsResident(Ref inEntry);
bool	EntryValid(RefArg inEntry);
Ref	EntryStore(RefArg inEntry);
Ref	EntrySoup(RefArg inEntry);
ULong	EntryUniqueId(RefArg inEntry);
ULong	EntryModTime(RefArg inEntry);
size_t	EntrySize(RefArg inEntry);
size_t	EntrySizeWithoutVBOs(RefArg inEntry);
size_t	EntryTextSize(RefArg inEntry);
Ref	EntryCopy(RefArg inEntry, RefArg inSoup);
void	EntryMove(RefArg inEntry, RefArg inSoup);
void	EntryChange(RefArg inEntry);
void	EntryChangeVerbatim(RefArg inEntry);
void	EntryChangeWithModTime(RefArg inEntry);
void	EntryChangeCommon(RefArg inEntry, int inSelector);
void	EntryUndoChanges(RefArg inEntry);
void	EntryReplace(RefArg inEntry, RefArg inNewEntry);
void	EntryReplaceWithModTime(RefArg inEntry, RefArg inNewEntry);
void	EntryRemoveFromSoup(RefArg inEntry);
void	EntryFlush(RefArg inEntry);
void	EntryFlushWithModTime(RefArg inEntry);

// Proxies
Ref	NewProxyEntry(RefArg inEntry, RefArg in);
bool	IsProxyEntry(RefArg inEntry);
Ref	EntryCachedObject(RefArg inEntry);
void	EntrySetCachedObject(RefArg inEntry, RefArg);
Ref	EntryHandler(RefArg inEntry);
void	EntrySetHandler(RefArg inEntry, RefArg);

// Aliases
Ref	MakeEntryAlias(RefArg inEntry);
Ref	ResolveEntryAlias(RefArg inRcvr);
bool	IsEntryAlias(RefArg inEntry);
bool	CompareAliasAndEntry(RefArg inRcvr, RefArg inEntry);
bool	IsSameEntry(RefArg inRcvr, RefArg inEntry);


#endif	/* __ENTRIES_H */
