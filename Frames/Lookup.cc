/*
	File:		Lookup.cc

	Contains:	Slot lookup functions.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ObjHeader.h"
#include "Lookup.h"
#include "NewtGlobals.h"
#include "Interpreter.h"
#include "RefMemory.h"
#include "ROMResources.h"

extern bool	IsFaultBlock(Ref r);


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FGetVariable(RefArg inRcvr, RefArg inObj, RefArg inTag);
Ref	FHasVariable(RefArg inRcvr, RefArg inObj, RefArg inTag);
Ref	FSetVariable(RefArg inRcvr, RefArg inObj, RefArg inTag, RefArg inValue);
}

/*----------------------------------------------------------------------
	I C a c h e
------------------------------------------------------------------------
	Interpreter cache - recently used refs for improved lookup
	performance.
----------------------------------------------------------------------*/

struct ICachedItem
{
	Ref		value;	// cached Ref; zero if entry is not in use
	Ref		slot;		// slot symbol
	ULong		hash;		// hash of that
	Ref		fr;		// context frame
	ArrayIndex	index;	// index of symbol within frame
};


class ICache
{
public:
				ICache(int pwr);
				~ICache();
	void		clear(void);
	void		clearSymbol(Ref sym, ULong hash);
	void		clearFrame(Ref fr);
	bool		lookup(Ref value, Ref sym, Ref * frPtr, Ref * valuePtr, bool * exists, ArrayIndex * indexPtr);
	bool		lookupValue(Ref value, Ref sym, Ref * valuePtr, bool * exists);
	void		insert(Ref value, Ref slot, Ref fr, ArrayIndex index);
	void		mark(void);
	void		update(void);
	static void		DIYMarkICache(void * self);
	static void		DIYUpdateICache(void * self);

private:
	ICachedItem *	cache;			// array of entries
	ArrayIndex		numOfEntries;	// number of entries in cache; always power of 2
	ArrayIndex		shift;
	ArrayIndex		mask;
};


ICache * gGetVarCache;
ICache * gFindImplCache;
ICache * gProtoCache;
ICache * gROProtoCache;


ICache::ICache(int pwr)
{
	numOfEntries = 1 << pwr;
	shift = 32 - pwr;
	mask = ~(-1 << pwr);
	cache = new ICachedItem[numOfEntries];
	if (cache == NULL)
		OutOfMemory();
	clear();
	DIYGCRegister(this, DIYMarkICache, DIYUpdateICache);
}


ICache::~ICache()
{
	delete[] cache;
	DIYGCUnregister(this);
}


void
ICache::clear(void)
{
	ICachedItem *	p = cache;
	for (ArrayIndex i = 0; i < numOfEntries; ++i, ++p)
		p->value = INVALIDPTRREF;
}


void
ICache::clearSymbol(Ref sym, ULong hash)
{
	ICachedItem *	p = cache;
	for (ArrayIndex i = 0; i < numOfEntries; ++i, ++p)
	{
		if (p->value != INVALIDPTRREF
		 && p->hash == hash
		 && UnsafeSymbolEqual(p->slot, sym, hash))
			p->value = INVALIDPTRREF;
	}
}


void
ICache::clearFrame(Ref fr)
{
	Ptr				frPtr = SetupListEQ(fr);
	ICachedItem *	p = cache;
	for (ArrayIndex i = 0; i < numOfEntries; ++i, ++p)
	{
		if (p->value != INVALIDPTRREF && ListEQ(p->fr, fr, frPtr))
			p->value = INVALIDPTRREF;
	}
}


bool
ICache::lookup(Ref value, Ref sym, Ref * frPtr, Ref * valuePtr, bool * exists, ArrayIndex * indexPtr)
{
	ULong				hash = ((SymbolObject *)PTR(sym))->hash;
	ArrayIndex		i = ((value >> 4) & mask) ^ (hash >> shift);
	ICachedItem *	p = &cache[i];
	if (p->value != INVALIDPTRREF
	 && p->hash == hash
	 && EQRef(p->value, value) && UnsafeSymbolEqual(p->slot, sym, hash))
	{
		if (p->fr == INVALIDPTRREF)
		{
			*exists = false;
			*valuePtr = NILREF;
		}
		else
		{
			*exists = true;
			*valuePtr = ((FrameObject *)ObjectPtr(p->fr))->slot[p->index];
			*frPtr = p->fr;
			*indexPtr = p->index;
		}
		return true;
	}
	return false;
}


bool
ICache::lookupValue(Ref value, Ref sym, Ref * valuePtr, bool * exists)
{
	ULong				hash = ((SymbolObject *)PTR(sym))->hash;
	ArrayIndex		i = ((value >> 4) & mask) ^ (hash >> shift);
	ICachedItem *	p = &cache[i];
	if (p->value != INVALIDPTRREF
	 && p->hash == hash
	 && EQ(p->value, value) && UnsafeSymbolEqual(p->slot, sym, hash))
	{
		if (p->fr == INVALIDPTRREF)
		{
			*exists = false;
			*valuePtr = NILREF;
		}
		else
		{
			*exists = true;
			*valuePtr = ((FrameObject *)ObjectPtr(p->fr))->slot[p->index];
		}
		return true;
	}
	return false;
}


void
ICache::insert(Ref value, Ref slot, Ref fr, ArrayIndex index)
{
	if (!IsFaultBlock(fr))
	{
		ULong				hash = ((SymbolObject *)PTR(slot))->hash;
		ArrayIndex		i = ((value >> 4) & mask) ^ (hash >> shift);
		ICachedItem *	p = &cache[i];
		p->value = value;
		p->slot = slot;
		p->hash = hash;
		p->fr = fr;
		p->index = index;
	}
}


void
ICache::mark(void)
{
	// this really does nothing
}


void
ICache::update(void)
{
	ICachedItem *	p = cache;
	for (ArrayIndex i = 0; i < numOfEntries; ++i, ++p)
	{
		if (p->value != INVALIDPTRREF)
		{
			p->value = DIYGCUpdate(p->value);
			if (ISNIL(p->value))
				p->value = INVALIDPTRREF;
			p->slot = DIYGCUpdate(p->slot);
			if (ISNIL(p->slot))
				p->slot = INVALIDPTRREF;
			p->fr = DIYGCUpdate(p->fr);
			if (ISNIL(p->fr))
				p->fr = INVALIDPTRREF;
		}
	}
}


void
ICache::DIYMarkICache(void * self)
{
	((ICache *)self)->mark();
}


void
ICache::DIYUpdateICache(void * self)
{
	((ICache *)self)->update();
}

#pragma mark -

/*----------------------------------------------------------------------
	P u b l i c   I C a c h e   I n t e r f a c e
----------------------------------------------------------------------*/

void
InitICache(void)
{
	gGetVarCache = new ICache(5);		// 32
	gProtoCache = new ICache(6);		// 64
	gROProtoCache = new ICache(6);	// 64
	gFindImplCache = new ICache(6);	// 64
}


void
ICacheClear(void)
{
	gGetVarCache->clear();
	gFindImplCache->clear();
	gProtoCache->clear();
	gROProtoCache->clear();
}


void
ICacheClearSymbol(Ref sym, ULong hash)
{
	gGetVarCache->clearSymbol(sym, hash);
	gFindImplCache->clearSymbol(sym, hash);
	gProtoCache->clearSymbol(sym, hash);
}


void
ICacheClearFrame(Ref fr)
{
	gGetVarCache->clearFrame(fr);
	gFindImplCache->clearFrame(fr);
	gProtoCache->clearFrame(fr);
}


#pragma mark -
/*----------------------------------------------------------------------
	L o o k u p   F u n c t i o n s
----------------------------------------------------------------------*/

bool
XFindImplementor(RefArg inRcvr, RefArg inTag, RefVar * outImpl, RefVar * value)
{
	if (ISNIL(inRcvr))
		return false;

	ArrayIndex slotIndex;
	bool exists;

	if (gFindImplCache->lookup(inRcvr, inTag, (Ref *)outImpl->h, (Ref *)value->h, &exists, &slotIndex))
		return exists;

	RefVar frMap;
	RefVar left(inRcvr);
	RefVar impl(inRcvr);

	if (gProtoCache->lookup(impl, inTag, (Ref *)outImpl->h, (Ref *)value->h, &exists, &slotIndex))
	{
		if (exists)
		{
			gFindImplCache->insert(inRcvr, inTag, *outImpl, slotIndex);
			return exists;	// ie true
		}

		frMap = ((FrameObject *)ObjectPtr(left))->map;
		slotIndex = FindOffset(frMap, SYMA(_parent));
		if (slotIndex == kIndexNotFound)
		{
			gFindImplCache->insert(inRcvr, inTag, INVALIDPTRREF, 0);
			return false;
		}

		left = impl = ((FrameObject *)ObjectPtr(left))->slot[slotIndex];
	}

	bool useTheCacheLuke = true;
	while (NOTNIL(impl))
	{
		Ref roProto = INVALIDPTRREF;		// R/O refs cannot move so itÕs okay to use a Ref
		while (NOTNIL(impl))
		{
			FrameObject * frPtr = (FrameObject *)ObjectPtr(impl);
			if (!ISFRAME(frPtr))
				ThrowBadTypeWithFrameData(kNSErrNotAFrame, impl);

			if (roProto == INVALIDPTRREF && ISRO(frPtr)
			&& (roProto = impl, gROProtoCache->lookup(roProto, inTag, (Ref *)outImpl->h, (Ref *)value->h, &exists, &slotIndex)))
			{
				if (exists)
				{
					// found it in the cache
					if (useTheCacheLuke)
						gProtoCache->insert(left, inTag, *outImpl, slotIndex);
					gFindImplCache->insert(inRcvr, inTag, *outImpl, slotIndex);
					return true;
				}
				frMap = frPtr->map;
			}
			else
			{
				frMap = frPtr->map;
				slotIndex = FindOffset(frMap, inTag);
				if (slotIndex != kIndexNotFound)
				{
					// found the implementor!
					*outImpl = impl;
					*value = ((FrameObject *)ObjectPtr(impl))->slot[slotIndex];

					if (roProto != INVALIDPTRREF)
						gROProtoCache->insert(roProto, inTag, impl, slotIndex);
					if (useTheCacheLuke)
						gProtoCache->insert(left, inTag, impl, slotIndex);
					gFindImplCache->insert(inRcvr, inTag, impl, slotIndex);
					return true;
				}
			}

			// slot is not in this context so keep looking up the _proto chain
			slotIndex = FindOffset(frMap, SYMA(_proto));
			if (slotIndex == kIndexNotFound)
				break;
			impl = ((FrameObject *)ObjectPtr(impl))->slot[slotIndex];
		}
		// end-of-proto-lookup

		// no _proto, or _proto is nil
		if (useTheCacheLuke)
		{
			gProtoCache->insert(left, inTag, INVALIDPTRREF, 0);
			useTheCacheLuke = false;
		}
		if (roProto != INVALIDPTRREF)
			gROProtoCache->insert(roProto, inTag, INVALIDPTRREF, 0);

		frMap = ((FrameObject *)ObjectPtr(left))->map;
		slotIndex = FindOffset(frMap, SYMA(_parent));
		if (slotIndex == kIndexNotFound)
			break;
		left = impl = ((FrameObject *)ObjectPtr(left))->slot[slotIndex];
	}
	// end-of-parent-lookup

	// couldnÕt find it
	gFindImplCache->insert(inRcvr, inTag, INVALIDPTRREF, 0);
	return false;
}


bool
XFindProtoImplementor(RefArg inRcvr, RefArg inTag, RefVar * outImpl, RefVar * value)
{
	// must have a context
	if (ISNIL(inRcvr))
		return false;

	// and it must be a frame
	RefVar impl(inRcvr);
	FrameObject * frPtr = (FrameObject *)ObjectPtr(impl);
	if (!ISFRAME(frPtr))
		ThrowBadTypeWithFrameData(kNSErrNotAFrame, impl);

	RefVar frMap(frPtr->map);
	Ref roProto = INVALIDPTRREF;		// R/O refs cannot move so itÕs okay to use a Ref
	for ( ; ; )
	{
		// look in _protoÉ
		ArrayIndex slotIndex = FindOffset(frMap, SYMA(_proto));
		if (slotIndex == kIndexNotFound)
		// _proto (and therefore implementor) wasnÕt found
			break;

		// Éwhich must be a frame
		impl = ((FrameObject *)ObjectPtr(impl))->slot[slotIndex];
		frPtr = (FrameObject *)ObjectPtr(impl);
		if (!ISFRAME(frPtr))
			ThrowBadTypeWithFrameData(kNSErrNotAFrame, impl);

		bool exists;
		if (roProto == INVALIDPTRREF && ISRO(frPtr)
		&& (roProto = impl, gROProtoCache->lookup(roProto, inTag, (Ref *)outImpl->h, (Ref *)value->h, &exists, &slotIndex)))
			return exists;

		frMap = frPtr->map;
		slotIndex = FindOffset(frMap, inTag);
		if (slotIndex != kIndexNotFound)
		{
			// found the implementor!
			*outImpl = impl;
			*value = ((FrameObject *)ObjectPtr(impl))->slot[slotIndex];

			if (roProto != INVALIDPTRREF)
				gROProtoCache->insert(roProto, inTag, impl, slotIndex);
			return true;
		}
		// else slot is not in this context so keep looking up the _proto chain
	}

	// implementor wasnÕt found
	if (roProto != INVALIDPTRREF)
		gROProtoCache->insert(roProto, inTag, INVALIDPTRREF, 0);
	return false;
}


/*----------------------------------------------------------------------
	Find the implementor of a message.
	In:		inRcvr		the context at which to start looking
				inTag			the slot tag
	Return:	Ref			the implementor
----------------------------------------------------------------------*/

Ref
FindImplementor(RefArg inRcvr, RefArg inTag)
{
	RefVar impl, value;

	if (XFindImplementor(inRcvr, inTag, &impl, &value))
		return impl;
	return NILREF;
}


Ref
FindProtoImplementor(RefArg inRcvr, RefArg inTag)
{
	// must have a context
	if (ISNIL(inRcvr))
		return NILREF;

	// try the cache first
	Ref context, value;
	bool exists;
	ArrayIndex slotIndex;
	if (gProtoCache->lookup(inRcvr, inTag, &context, &value, &exists, &slotIndex))
		return exists ? context : NILREF;

	Ref roProto = INVALIDPTRREF;		// R/O refs cannot move so itÕs okay to use a Ref
	RefVar frMap;
	RefVar impl(inRcvr);
	while (NOTNIL(impl))
	{
		FrameObject * frPtr = (FrameObject *)ObjectPtr(impl);
		if (!ISFRAME(frPtr))
			ThrowBadTypeWithFrameData(kNSErrNotAFrame, impl);

		if (roProto == INVALIDPTRREF && ISRO(frPtr)
		&& (roProto = impl, gROProtoCache->lookup(roProto, inTag, &context, &value, &exists, &slotIndex)))
		{
			return exists ? context : NILREF;
		}

		frMap = frPtr->map;
		// look for a slot with the given name in this context
		slotIndex = FindOffset(frMap, inTag);
		if (slotIndex != kIndexNotFound)
		{
			// found the implementor!
			if (roProto != INVALIDPTRREF)
				gROProtoCache->insert(roProto, inTag, impl, slotIndex);
			gProtoCache->insert(inRcvr, inTag, impl, slotIndex);
			return impl;
		}

		// not in this context - look in _proto
		slotIndex = FindOffset(frMap, SYMA(_proto));
		if (slotIndex == kIndexNotFound)
			break;
		impl = ((FrameObject *)ObjectPtr(impl))->slot[slotIndex];
	}

	// implementor wasnÕt found
	if (roProto != INVALIDPTRREF)
		gROProtoCache->insert(roProto, inTag, INVALIDPTRREF, 0);
	gProtoCache->insert(inRcvr, inTag, INVALIDPTRREF, 0);
	return NILREF;
}


/*----------------------------------------------------------------------
	Full lookup as described in the NewtonScript Bytecode Interpreter
	Specification.
	Returns nil instead of throwing nil context exception.
	In:		inRcvr		the context at which to start looking
				inTag			name of the variable
				exists		pointer to flag - does variable exist?
				lookup		if lexical lookup use _nextArgFrame
								instead of _proto for inheritance
	Return:	Ref			the value
----------------------------------------------------------------------*/

Ref
XGetVariable(RefArg inRcvr, RefArg inTag, bool * outExists, int lookup)
{
	// must have pointer to exists flag
	bool exists;
	if (outExists == NULL)
		outExists = &exists;

	// nil start context => canÕt be found
	if (ISNIL(inRcvr))
	{
		*outExists = false;
		return NILREF;
	}

	// try the get-variable cache first
	Ref value;
	if (gGetVarCache->lookupValue(inRcvr, inTag, &value, outExists))
		return value;

	// now start looking properly
	RefVar	frMap;
	ArrayIndex slotIndex;
	RefVar	left(inRcvr);
	RefVar	impl(inRcvr);
	if (lookup == kLexicalLookup)
	{
		do
		{
			frMap = ((FrameObject *)ObjectPtr(impl))->map;
			slotIndex = FindOffset(frMap, inTag);
			if (slotIndex != kIndexNotFound)
			{
				// found the slot in this frame
				*outExists = true;
				// update the get-variable cache
				gGetVarCache->insert(inRcvr, inTag, impl, slotIndex);
				return ((FrameObject *)ObjectPtr(impl))->slot[slotIndex];
			}
			// this is lexical -- try the _nextArgFrame
			impl = UnsafeGetFrameSlot(impl, SYMA(_nextArgFrame), &exists);
		} while (NOTNIL(impl) && Length(impl) > 0);
		// okay, not found in the lexical scope, try the _parent next
		left = impl = UnsafeGetFrameSlot(left, SYMA(_parent), &exists);
	}

	Ref context;
	if (NOTNIL(impl) && gProtoCache->lookup(impl, inTag, &context, &value, outExists, &slotIndex))
	{
		if (*outExists)
		{
			gGetVarCache->insert(inRcvr, inTag, context, slotIndex);
			return value;
		}
		frMap = ((FrameObject *)ObjectPtr(left))->map;
		slotIndex = FindOffset(frMap, SYMA(_parent));
		if (slotIndex == kIndexNotFound)
		{
			*outExists = false;
			gGetVarCache->insert(inRcvr, inTag, INVALIDPTRREF, 0);
			return NILREF;
		}
		left = impl = ((FrameObject *)ObjectPtr(left))->slot[slotIndex];
	}

	bool useTheCacheLuke = true;
	while (NOTNIL(impl))
	{
		Ref roProto = INVALIDPTRREF;		// R/O refs cannot move so itÕs okay to use a Ref
		FrameObject * frPtr = (FrameObject *)ObjectPtr(impl);
		if (!ISFRAME(frPtr))
			ThrowBadTypeWithFrameData(kNSErrNotAFrame, impl);

		if (roProto == INVALIDPTRREF && ISRO(frPtr)
		&& (roProto = impl, gROProtoCache->lookup(roProto, inTag, &context, &value, outExists, &slotIndex)))
		{
			if (*outExists)
			{
				gGetVarCache->insert(inRcvr, inTag, impl, slotIndex);
				if (useTheCacheLuke)
					gProtoCache->insert(left, inTag, impl, slotIndex);
				return value;
			}
			else
			{
				frMap = frPtr->map;
			}
		}
		else
		{
			frMap = frPtr->map;
			slotIndex = FindOffset(frMap, inTag);
			if (slotIndex != kIndexNotFound)
			{
				*outExists = true;
				gGetVarCache->insert(inRcvr, inTag, impl, slotIndex);
				if (useTheCacheLuke)
					gProtoCache->insert(left, inTag, impl, slotIndex);
				if (roProto != INVALIDPTRREF)
					gROProtoCache->insert(roProto, inTag, impl, slotIndex);
				return ((FrameObject *)ObjectPtr(impl))->slot[slotIndex];
			}
		}

		// slot is not in this context so keep looking up the _proto chain
		slotIndex = FindOffset(frMap, SYMA(_proto));
		if (slotIndex == kIndexNotFound
		||	 ISNIL(impl = ((FrameObject *)ObjectPtr(impl))->slot[slotIndex]))
		{
			// no _proto, or _proto is nil
			if (useTheCacheLuke)
			{
				gProtoCache->insert(left, inTag, INVALIDPTRREF, 0);
				useTheCacheLuke = false;
			}
			if (roProto != INVALIDPTRREF)
				gROProtoCache->insert(roProto, inTag, INVALIDPTRREF, 0);
			// try the next _parent
			frMap = ((FrameObject *)ObjectPtr(left))->map;
			slotIndex = FindOffset(frMap, SYMA(_parent));
			if (slotIndex == kIndexNotFound)
				break;
			left = impl = ((FrameObject *)ObjectPtr(left))->slot[slotIndex];
		}
	}

	*outExists = false;
	gGetVarCache->insert(inRcvr, inTag, INVALIDPTRREF, 0);
	return NILREF;
}


/*----------------------------------------------------------------------
	GetVariable
	Full lookup as described in the NewtonScript Bytecode Interpreter
	Specification.
	In:		inRcvr		the context at which to start looking
				inTag			name of the variable
				outExists	pointer to flag - does variable exist?
				inLookup		if lexical lookup use _nextArgFrame
								instead of _proto for inheritance
	Return:	Ref			the value
----------------------------------------------------------------------*/

Ref
GetVariable(RefArg inRcvr, RefArg inTag, bool * outExists, int inLookup)
{
	// throw exception if no context
	if (ISNIL(inRcvr))
		ThrowExInterpreterWithSymbol(kNSErrNilContext, inRcvr);

	// ensure weÕve got a vital pointer -- original checks each time itÕs set
	bool exists;
	if (outExists == NULL)
		outExists = &exists;

	// assume weÕre going to find it
	*outExists = true;

	RefVar next;
	RefVar impl(inRcvr);
	while (NOTNIL(impl))
	{
		if (FrameHasSlot(impl, inTag))
		{
			if (gInterpreter->tracing >= 2)
				gInterpreter->traceGet(inRcvr, impl, inTag);
			return GetFrameSlot(impl, inTag);
		}

		// look thru _proto/_nextArgFrame chain
		for (next = GetFrameSlot(impl, (inLookup == kLexicalLookup) ? SYMA(_nextArgFrame) : SYMA(_proto));
			  NOTNIL(next);
			  next = GetFrameSlot(next, (inLookup == kLexicalLookup) ? SYMA(_nextArgFrame) : SYMA(_proto)))
		{
			if (FrameHasSlot(next, inTag))
			{
				if (gInterpreter->tracing >= 2)
					gInterpreter->traceGet(inRcvr, next, inTag);
				return GetFrameSlot(next, inTag);
			}
		}
		// lexical lookup only applies to original context
		inLookup = kNoLookup;
		// follow the _parent chain
		impl = GetFrameSlot(impl, SYMA(_parent));
	}

	// variable wasnÕt found
	if (gInterpreter->tracing >= 2)
		gInterpreter->traceGet(inRcvr, inRcvr, inTag);
	*outExists = false;
	return NILREF;
}


/*----------------------------------------------------------------------
	GetProtoVariable
	In:		inRcvr		the context at which to start looking
				inTag			name of the variable
				outExists	pointer to flag - does variable exist?
	Return:	Ref			the value
----------------------------------------------------------------------*/

Ref
GetProtoVariable(RefArg inRcvr, RefArg inTag, bool * outExists)
{
	// throw exception if no context
	if (ISNIL(inRcvr))
		ThrowExInterpreterWithSymbol(kNSErrNilContext, inRcvr);

	// cache lookups expect this pointer
	bool exists;
	if (outExists == NULL)
		outExists = &exists;

	// try the cache first
	Ref value;
	if (gProtoCache->lookupValue(inRcvr, inTag, &value, outExists))
		return value;

	Ref roProto = INVALIDPTRREF;		// R/O refs cannot move so itÕs okay to use a Ref
	RefVar frMap;
	ArrayIndex slotIndex;
	RefVar impl(inRcvr);
	while (NOTNIL(impl))
	{
		FrameObject * frPtr = (FrameObject *)ObjectPtr(impl);
		// must have a frame
		if (!ISFRAME(frPtr))
#if defined(forNTK)
			break;
#else
			ThrowBadTypeWithFrameData(kNSErrNotAFrame, impl);
#endif

		if (roProto == INVALIDPTRREF && ISRO(frPtr)
		&& (roProto = impl, gROProtoCache->lookupValue(roProto, inTag, &value, outExists)))
		{
			return value;
		}

		// look in the impl frame map for the name of the variable
		frMap = frPtr->map;
		slotIndex = FindOffset(frMap, inTag);
		if (slotIndex != kIndexNotFound)
		{
			// found it!
			// cache it and return its value
			*outExists = true;
			if (gInterpreter->tracing >= 2)
				gInterpreter->traceGet(inRcvr, impl, inTag);
			if (roProto != INVALIDPTRREF)
				gROProtoCache->insert(roProto, inTag, impl, slotIndex);
			gProtoCache->insert(inRcvr, inTag, impl, slotIndex);
			return ((FrameObject *)ObjectPtr(impl))->slot[slotIndex];
		}

		// follow the _proto chain
		slotIndex = FindOffset(frMap, SYMA(_proto));
		if (slotIndex == kIndexNotFound)
			break;
		impl = ((FrameObject *)ObjectPtr(impl))->slot[slotIndex];
	}

	// variable wasnÕt found
	if (gInterpreter->tracing >= 2)
		gInterpreter->traceGet(inRcvr, inRcvr, inTag);
	*outExists = false;
	if (roProto != INVALIDPTRREF)
		gROProtoCache->insert(roProto, inTag, INVALIDPTRREF, 0);
	gProtoCache->insert(inRcvr, inTag, INVALIDPTRREF, 0);
	return NILREF;
}


/*----------------------------------------------------------------------
	SetVariable
	Set variable within context.
	In:		inRcvr		the context at which to start looking
				inTag			name of the variable
				inValue		the value to set
	Return:	bool			the value was set
----------------------------------------------------------------------*/

bool
SetVariable(RefArg inRcvr, RefArg inTag, RefArg inValue)
{
	return SetVariableOrGlobal(inRcvr, inTag, inValue, kSetSelfLookup);
}


/*----------------------------------------------------------------------
	SetVariableOrGlobal
	In:		inRcvr		the context at which to start looking
				inTag			name of the variable
				inValue		the value to set
				inLookup		where to lookup the var to set
	Return:	bool			the value was set
----------------------------------------------------------------------*/

bool
SetVariableOrGlobal(RefArg inRcvr, RefArg inTag, RefArg inValue, int inLookup)
{
	RefVar	left, impl;
	Ref		frMap;
	FrameObject * frPtr;
	ArrayIndex	slotIndex;
	bool		exists;

	left = inRcvr;
	if (inLookup & kLexicalLookup)
	{
		// lexical lookup
		// follow _nextArgFrame in current context
		for (impl = left; NOTNIL(impl); impl = UnsafeGetFrameSlot(impl, SYMA(_nextArgFrame), &exists))
		{
			frMap = ((FrameObject *)ObjectPtr(impl))->map;
			slotIndex = FindOffset(frMap, inTag);
			if (slotIndex != kIndexNotFound)
			{
				if (gInterpreter->tracing >= 2)
					gInterpreter->traceSet(inRcvr, impl, inTag, inValue);
				((FrameObject *)ObjectPtr(impl))->slot[slotIndex] = inValue;
				return true;
			}
		}
		// not found, try _parent next
		left = UnsafeGetFrameSlot(left, SYMA(_parent), &exists);
	}

	while (NOTNIL(left))
	{
		for (impl = left; NOTNIL(impl); )
		{
			frPtr = (FrameObject *)ObjectPtr(impl);
			if (!ISFRAME(frPtr))
				ThrowBadTypeWithFrameData(kNSErrNotAFrame, impl);

			frMap = frPtr->map;
			slotIndex = FindOffset(frMap, inTag);
			if (slotIndex != kIndexNotFound)
			{
				// found it!
				if (gInterpreter->tracing >= 2)
					gInterpreter->traceSet(inRcvr, impl, inTag, inValue);
				if (left == impl)
					// can set it in this context
					((FrameObject *)ObjectPtr(impl))->slot[slotIndex] = inValue;
				else
					// must set it in the left frame
					SetFrameSlot(left, inTag, inValue);
				return true;
			}

			// follow the _proto chain
			slotIndex = FindOffset(frMap, SYMA(_proto));
			if (slotIndex == kIndexNotFound)
				break;
			impl = ((FrameObject *)ObjectPtr(impl))->slot[slotIndex];
		}

		// follow the _parent chain
		frMap = ((FrameObject *)ObjectPtr(left))->map;
		slotIndex = FindOffset(frMap, SYMA(_parent));
		if (slotIndex == kIndexNotFound)
			break;
		left = ((FrameObject *)ObjectPtr(left))->slot[slotIndex];
	}

	if ((inLookup & kGlobalLookup) && FrameHasSlot(gVarFrame, inTag))
	{
		if (gInterpreter->tracing >= 2)
			gInterpreter->traceSet(RA(gVarFrame), RA(gVarFrame), inTag, inValue);
		DefGlobalVar(inTag, inValue);
		return true;
	}

	if (inLookup & kSetSelfLookup)
	{
		if (gInterpreter->tracing >= 2)
			gInterpreter->traceSet(inRcvr, inRcvr, inTag, inValue);
		SetFrameSlot(inRcvr, inTag, inValue);
		return true;
	}

	if (inLookup & kSetGlobalLookup)
	{
		if (gInterpreter->tracing >= 2)
			gInterpreter->traceSet(RA(gVarFrame), RA(gVarFrame), inTag, inValue);
		DefGlobalVar(inTag, inValue);
		return true;
	}

	return false;
}


/*----------------------------------------------------------------------
	Determine whether a frame is the parent of another.
	In:		fr			a frame
				start		the context at which to start looking
	Return:	bool		fr is a parent
----------------------------------------------------------------------*/

bool
IsParent(RefArg fr, RefArg start)
{
	RefVar parent(start);
	for ( ; NOTNIL(parent); parent = GetFrameSlot(parent, SYMA(_parent)))
		if (EQ(parent, fr))
			return true;
	return false;
}

#pragma mark -

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

Ref
FGetVariable(RefArg inRcvr, RefArg inObj, RefArg inTag)
{
	return GetVariable(inObj, inTag);
}


Ref
FHasVariable(RefArg inRcvr, RefArg inObj, RefArg inTag)
{
	bool  exists;
	GetVariable(inObj, inTag, &exists, kNoLookup);
	return MAKEBOOLEAN(exists);
}


Ref
FSetVariable(RefArg inRcvr, RefArg inObj, RefArg inTag, RefArg inValue)
{
	SetVariable(inObj, inTag, inValue);
	return inValue;
}


