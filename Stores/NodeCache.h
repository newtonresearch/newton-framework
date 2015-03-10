/*
	File:		NodeCache.h

	Contains:	Newton object store -- soup index node cache.

	Written by:	Newton Research Group, 2009.
*/

#if !defined(__NODECACHE_H)
#define __NODECACHE_H 1


class CSoupIndex;

// maybe this should be like ObjHeader, prefix to different kinds of node
// need to PACK these structs
struct NodeHeader
{
	PSSId		id;
	PSSId		nextId;				// +04
	short		bytesRemaining;	// +08
	UShort	numOfSlots;			// +0A

	UShort	keyFieldOffset[1];	// +0C	actually variable length, but always at least 1
} __attribute__ ((__packed__));

struct DupNodeHeader
{
	PSSId		id;
	PSSId		nextId;				// +04
	short		bytesRemaining;	// +08
	UShort	numOfSlots;			// +0A

	UShort	bytesUsed;			// +0C
	short		x0E;
	char		x10[];
} __attribute__ ((__packed__));


struct NodeRef
{
	PSSId				id;		// +00
	NodeHeader *	node;		// +04
	bool				isDup;	// +08
	bool				isDirty;	// +09
	bool				isInUse;	// +0A
	int				lru;		// +0C
	CSoupIndex *	index;	// +10
};


class CNodeCache
{
public:
				CNodeCache();
				~CNodeCache();

	NodeHeader *	findNode(CSoupIndex * index, PSSId inId);
	NodeHeader *	rememberNode(CSoupIndex * index, PSSId inId, size_t inSize, bool inArg4, bool inDirty);
	void		forgetNode(PSSId inId);
	void		deleteNode(PSSId inId);
	void		dirtyNode(NodeHeader * inNode);

	void		abort(CSoupIndex * index);
	void		commit(CSoupIndex * index);
	void		reuse(CSoupIndex * index);
	void		clear(void);
	bool		flush(CSoupIndex * index);
	int		modCount(void) const;

private:
	size_t		fNumOfEntries;
	NodeRef *	fPool;
	int			fUseCount;
	int			fModCount;
};

inline bool	CNodeCache::flush(CSoupIndex * inSoupIndex)
{ if (fNumOfEntries > 32) { commit(inSoupIndex); return YES; } return NO; }

inline int	CNodeCache::modCount(void) const
{ return fModCount; }


#endif	/* __NODECACHE_H */
