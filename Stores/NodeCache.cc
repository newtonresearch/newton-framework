/*
	File:		NodeCache.cc

	Contains:	Newton object store -- soup index node cache.

	Written by:	Newton Research Group, 2009.
*/

#include "Objects.h"
#include "StoreWrapper.h"
#include "Indexes.h"
#include "OSErrors.h"


/*------------------------------------------------------------------------------
	C N o d e C a c h e
	Something to do with soup indexes.
------------------------------------------------------------------------------*/

#define kInitialPoolSize 3

CNodeCache::CNodeCache()
{
	fNumOfEntries = kInitialPoolSize;
	fPool = (NodeRef *)NewPtr(kInitialPoolSize * sizeof(NodeRef));	// was NewHandle
	if (fPool == NULL)
		OutOfMemory();
	NodeRef * p = fPool;
	NodeRef * pLimit = p + fNumOfEntries;
	for ( ; p < pLimit; p++)
	{
		p->id = 0;
		p->lru = 0;
		p->index = NULL;
		p->isInUse = NO;
		p->node = (NodeHeader *)NewPtr(512);
		if (p->node == NULL)
			OutOfMemory();
	}
	fUseCount = 0;
	fModCount = 0;
}


CNodeCache::~CNodeCache()
{
	NodeRef * p = fPool;
	NodeRef * pLimit = p + fNumOfEntries;
	for ( ; p < pLimit; p++)
		FreePtr((Ptr)p->node);
	FreePtr((Ptr)fPool);
}


NodeHeader *
CNodeCache::findNode(CSoupIndex * index, PSSId inId)
{
	NodeRef * p = fPool;
	NodeRef * pLimit = p + fNumOfEntries;
	for ( ; p < pLimit; p++)
	{
		if (p->id == inId)
		{
			p->index = index;
			p->isInUse = YES;
			p->lru = ++fUseCount;
			return p->node;
		}
	}
	return NULL;
}


NodeHeader *
CNodeCache::rememberNode(CSoupIndex * index, PSSId inId, size_t inSize, bool inDup, bool inDirty)
{
	int highestLRU = fUseCount + 1;
	bool isPoolModified = YES;
	NodeHeader * theNode;
	NodeRef * aNodeToRemember = NULL;
	// look in the pool for a likely node to reuse
	NodeRef * p = fPool;
	NodeRef * pLimit = p + fNumOfEntries;
	for ( ; p < pLimit; p++)
	{
		if (p->id == 0)
		{
			// this one’s free
			aNodeToRemember = p;
			isPoolModified = NO;
			break;
		}
		if (!p->isInUse && p->lru < highestLRU)
		{
			// this one’s inactive and the least recently used
			highestLRU = p->lru;
			aNodeToRemember = p;
		}
	}
	if (aNodeToRemember != NULL)
	{
		// use the node we found
		theNode = aNodeToRemember->node;
		if (isPoolModified)
			fModCount++;
	}
	else
	{
		// didn’t find anything; increase the size of the pool
		fPool = (NodeRef *)ReallocPtr((Ptr)fPool, (fNumOfEntries+1) * sizeof(NodeRef));
		if (MemError())
			OutOfMemory();
		// create a new node
		theNode = (NodeHeader *)NewPtr(inSize);
		if (theNode == NULL)
		{
			fPool = (NodeRef *)ReallocPtr((Ptr)fPool, fNumOfEntries * sizeof(NodeRef));
			OutOfMemory();
		}
		// and use that
		aNodeToRemember = fPool + fNumOfEntries;
		fNumOfEntries++;
		aNodeToRemember->node = theNode;
	}

	aNodeToRemember->id = inId;
	aNodeToRemember->isDup = inDup;
	aNodeToRemember->isDirty = inDirty;
	aNodeToRemember->lru = ++fUseCount;
	aNodeToRemember->index = index;
	aNodeToRemember->isInUse = YES;
	return theNode;
}


void
CNodeCache::forgetNode(PSSId inId)
{
	NodeRef * p = fPool;
	NodeRef * pLimit = p + fNumOfEntries;
	for ( ; p < pLimit; p++)
	{
		if (p->id == inId)
		{
			p->id = 0;
			p->lru = 0;
			p->index = NULL;
			p->isInUse = NO;
			fModCount++;
			break;
		}
	}
}


void
CNodeCache::deleteNode(PSSId inId)
{
	NodeRef * p = fPool;
	NodeRef * pLimit = p + fNumOfEntries;
	for ( ; p < pLimit; p++)
	{
		if (p->id == inId)
		{
			CSoupIndex * index = p->index;
			p->id = 0;
			p->lru = 0;
			p->index = NULL;
			p->isInUse = NO;
			OSERRIF(index->store()->deleteObject(inId));
			fModCount++;
			break;
		}
	}
}


void
CNodeCache::dirtyNode(NodeHeader * inNode)
{
	NodeRef * p = fPool;
	NodeRef * pLimit = p + fNumOfEntries;
	for ( ; p < pLimit; p++)
	{
		if (p->node == inNode)
		{
			p->isDirty = YES;
			fModCount++;
			break;
		}
	}
}


void
CNodeCache::commit(CSoupIndex * index)
{
	bool isUsed = NO;
	NodeRef * p = fPool;
	NodeRef * pLimit = p + fNumOfEntries;
	for ( ; p < pLimit; p++)
	{
		if (p->index == index)
		{
			p->isInUse = NO;
			if (p->isDirty)
			{
				if (p->isDup)
					index->updateDupNode(p->node);
				else
					index->updateNode(p->node);
				p->isDirty = NO;
			}
		}
		else if (p->isInUse)
			isUsed = YES;
	}
	if (!isUsed && fNumOfEntries > 8)
	{
		// shrink the cache
		for (p = fPool + 8; p < pLimit; p++)
			FreePtr((Ptr)p->node);
		fPool = (NodeRef *)ReallocPtr((Ptr)fPool, 8 * sizeof(NodeRef));
		fNumOfEntries = 8;
		fModCount++;
	}
}


void
CNodeCache::reuse(CSoupIndex * index)
{
	NodeRef * p = fPool;
	NodeRef * pLimit = p + fNumOfEntries;
	for ( ; p < pLimit; p++)
	{
		if (p->index == index)
			p->isInUse = YES;
			// don’t break
	}
}


void
CNodeCache::abort(CSoupIndex * index)
{
	NodeRef * p = fPool;
	NodeRef * pLimit = p + fNumOfEntries;
	for ( ; p < pLimit; p++)
	{
		if (p->index == index)
		{
			p->index = NULL;
			p->isInUse = NO;
			p->id = 0;
			p->isDirty = NO;
			// don’t break
		}
	}
	fModCount++;
}


void
CNodeCache::clear(void)
{
	NodeRef * p = fPool;
	NodeRef * pLimit = p + fNumOfEntries;
	for ( ; p < pLimit; p++)
	{
		p->index = NULL;
		p->isInUse = NO;
		p->id = 0;
		p->lru = 0;
		p->isDirty = NO;
	}
	fModCount++;
}

