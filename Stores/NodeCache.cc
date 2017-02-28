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
	for (NodeRef * p = fPool, * pLimit = fPool + fNumOfEntries; p < pLimit; ++p)
	{
		p->id = 0;
		p->lru = 0;
		p->index = NULL;
		p->isInUse = false;
		p->node = (NodeHeader *)NewPtr(512);
		if (p->node == NULL)
			OutOfMemory();
	}
	fUseCount = 0;
	fModCount = 0;
}


CNodeCache::~CNodeCache()
{
	for (NodeRef * p = fPool, * pLimit = fPool + fNumOfEntries; p < pLimit; ++p)
		FreePtr((Ptr)p->node);
	FreePtr((Ptr)fPool);
}


NodeHeader *
CNodeCache::findNode(CSoupIndex * index, PSSId inId)
{
	for (NodeRef * p = fPool, * pLimit = fPool + fNumOfEntries; p < pLimit; ++p)
	{
		if (p->id == inId)
		{
			p->index = index;
			p->isInUse = true;
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
	bool isPoolModified = true;
	NodeHeader * theNode;
	NodeRef * aNodeToRemember = NULL;

	// look in the pool for a likely node to reuse
	for (NodeRef * p = fPool, * pLimit = fPool + fNumOfEntries; p < pLimit; ++p)
	{
		if (p->id == 0)
		{
			// this one’s free
			aNodeToRemember = p;
			isPoolModified = false;
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
			// memory situation really bad: revert size of pool
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
	aNodeToRemember->isInUse = true;
	return theNode;
}


void
CNodeCache::forgetNode(PSSId inId)
{
	for (NodeRef * p = fPool, * pLimit = fPool + fNumOfEntries; p < pLimit; ++p)
	{
		if (p->id == inId)
		{
			p->id = 0;
			p->lru = 0;
			p->index = NULL;
			p->isInUse = false;
			fModCount++;
			break;
		}
	}
}


void
CNodeCache::deleteNode(PSSId inId)
{
	for (NodeRef * p = fPool, * pLimit = fPool + fNumOfEntries; p < pLimit; ++p)
	{
		if (p->id == inId)
		{
			CSoupIndex * index = p->index;
			p->id = 0;
			p->lru = 0;
			p->index = NULL;
			p->isInUse = false;
			OSERRIF(index->store()->deleteObject(inId));
			fModCount++;
			break;
		}
	}
}


void
CNodeCache::dirtyNode(NodeHeader * inNode)
{
	for (NodeRef * p = fPool, * pLimit = fPool + fNumOfEntries; p < pLimit; ++p)
	{
		if (p->node == inNode)
		{
			p->isDirty = true;
			fModCount++;
			break;
		}
	}
}


void
CNodeCache::commit(CSoupIndex * index)
{
	bool isUsed = false;
	for (NodeRef * p = fPool, * pLimit = fPool + fNumOfEntries; p < pLimit; ++p)
	{
		if (p->index == index)
		{
			p->isInUse = false;
			if (p->isDirty)
			{
				if (p->isDup)
					index->updateDupNode(p->node);
				else
					index->updateNode(p->node);
				p->isDirty = false;
			}
		}
		else if (p->isInUse)
			isUsed = true;
	}

	if (!isUsed && fNumOfEntries > 8)
	{
		// shrink the cache
		for (NodeRef * p = fPool + 8, * pLimit = fPool + fNumOfEntries; p < pLimit; ++p)
			FreePtr((Ptr)p->node);
		fPool = (NodeRef *)ReallocPtr((Ptr)fPool, 8 * sizeof(NodeRef));
		fNumOfEntries = 8;
		fModCount++;
	}
}


void
CNodeCache::reuse(CSoupIndex * index)
{
	for (NodeRef * p = fPool, * pLimit = fPool + fNumOfEntries; p < pLimit; ++p)
	{
		if (p->index == index)
			p->isInUse = true;
			// don’t break
	}
}


void
CNodeCache::abort(CSoupIndex * index)
{
	for (NodeRef * p = fPool, * pLimit = fPool + fNumOfEntries; p < pLimit; ++p)
	{
		if (p->index == index)
		{
			p->index = NULL;
			p->isInUse = false;
			p->id = 0;
			p->isDirty = false;
			// don’t break
		}
	}
	fModCount++;
}


void
CNodeCache::clear(void)
{
	for (NodeRef * p = fPool, * pLimit = fPool + fNumOfEntries; p < pLimit; ++p)
	{
		p->index = NULL;
		p->isInUse = false;
		p->id = 0;
		p->lru = 0;
		p->isDirty = false;
	}
	fModCount++;
}

