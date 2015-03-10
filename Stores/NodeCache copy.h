/*
	File:		NodeCache.h

	Contains:	Newton object store -- soup index node cache.

	Written by:	Newton Research Group, 2009.
*/

#if !defined(__NODECACHE_H)
#define __NODECACHE_H 1


class CSoupIndex;

// maybe this should be like ObjHeader, prefix to different kinds of node
struct NodeHeader
{
	PSSId		x00;
	ULong		x04;
	short		x08;		// size?
	short		x0A;		// num of slots

	short		x0C[];	// offsets to data?
};

struct DupNodeHeader
{
	PSSId		x00;
	ULong		x04;
	short		x08;		// bytes remaining
	short		x0A;		// num of slots

	short		x0C;		// bytes used
	short		x0E;
	char		x10[];
};


struct NodeRef
{
	PSSId				id;		// +00
	NodeHeader *	node;		// +04
	BOOL				isDup;	// +08
	BOOL				isDirty;	// +09
	BOOL				isInUse;	// +0A
	long				x0C;
	CSoupIndex *	index;	// +10
};


class CNodeCache
{
public:
				CNodeCache();
				~CNodeCache();

	NodeHeader *	findNode(CSoupIndex * index, PSSId inId);
	NodeHeader *	rememberNode(CSoupIndex * index, PSSId inId, size_t inSize, BOOL inArg4, BOOL inDirty);
	void		forgetNode(PSSId inId);
	void		deleteNode(PSSId inId);
	void		dirtyNode(NodeHeader * inNode);

	void		abort(CSoupIndex * index);
	void		commit(CSoupIndex * index);
	void		reuse(CSoupIndex * index);
	void		clear(void);
	void		flush(CSoupIndex * index);

private:
	size_t		fNumOfEntries;		// +00
	NodeRef *	fCache;				// +04
	int			f08;
	int			f0C;
};

inline void CNodeCache::flush(CSoupIndex * inSoupIndex)
{ if (fNumOfEntries > 32) commit(inSoupIndex); }


#endif	/* __NODECACHE_H */
