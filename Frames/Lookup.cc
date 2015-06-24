/*
	File:		Lookup.cc

	Contains:	Slot lookup functions.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ObjHeader.h"
#include "Lookup.h"
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
				ICache(long pwr);
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


ICache * gGetVarCache;		// 0C105498
ICache * gFindImplCache;	// 0C10549C
ICache * gProtoCache;		// 0C1054A0
ICache * gROProtoCache;		// 0C1054A4


ICache::ICache(long pwr)
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
	for (ArrayIndex i = 0; i < numOfEntries; i++, p++)
		p->value = INVALIDPTRREF;
}


void
ICache::clearSymbol(Ref sym, ULong hash)
{
	ICachedItem *	p = cache;
	for (ArrayIndex i = 0; i < numOfEntries; i++, p++)
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
	for (ArrayIndex i = 0; i < numOfEntries; i++, p++)
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
			*exists = NO;
			*valuePtr = NILREF;
		}
		else
		{
			*exists = YES;
			*valuePtr = ((FrameObject *)ObjectPtr(p->fr))->slot[p->index];
			*frPtr = p->fr;
			*indexPtr = p->index;
		}
		return YES;
	}
	return NO;
}


bool
ICache::lookupValue(Ref value, Ref sym, Ref * valuePtr, bool * exists)
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
			*exists = NO;
			*valuePtr = NILREF;
		}
		else
		{
			*exists = YES;
			*valuePtr = ((FrameObject *)ObjectPtr(p->fr))->slot[p->index];
		}
		return YES;
	}
	return NO;
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
	for (ArrayIndex i = 0; i < numOfEntries; i++, p++)
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
	gGetVarCache = new ICache(5);	// 32
	gProtoCache = new ICache(6);	// 64
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
	RefVar	left, current; // r6, sp00
	Ref		frMap;			// r8
	FrameObject * frPtr;		// r10
	ArrayIndex	slotIndex;		// sp04
	bool		exists;			// sp08

	if (ISNIL(inRcvr))
		return NO;

	if (gFindImplCache->lookup(inRcvr, inTag, (Ref *)outImpl->h, (Ref *)value->h, &exists, &slotIndex))
		return exists;

	left = current = inRcvr;

	if (gProtoCache->lookup(current, inTag, (Ref *)outImpl->h, (Ref *)value->h, &exists, &slotIndex))
	{
		if (exists)
		{
			gFindImplCache->insert(current, inTag, *outImpl, slotIndex);
			return exists;
		}

		frMap = ((FrameObject *)ObjectPtr(left))->map;
		slotIndex = FindOffset(frMap, RSYM_parent);
		if (slotIndex == kIndexNotFound)
		{
			gFindImplCache->insert(inRcvr, inTag, INVALIDPTRREF, 0);
			return NO;
		}

		left = current = ((FrameObject *)ObjectPtr(left))->slot[slotIndex];
	}

	bool	spm00 = YES;
	while (NOTNIL(current))
	{
		Ref	r9 = INVALIDPTRREF;
		while (NOTNIL(current))
		{
			frPtr = (FrameObject *)ObjectPtr(current);
			if (NOTFRAME(frPtr))
				ThrowBadTypeWithFrameData(kNSErrNotAFrame, current);

			if (r9 == INVALIDPTRREF && ISRO(frPtr)
			&& gROProtoCache->lookup(r9 = current, inTag, (Ref *)outImpl->h, (Ref *)value->h, &exists, &slotIndex))
			{
				if (exists)
				{
					// found it in the cache
					if (spm00)
						gProtoCache->insert(left, inTag, *outImpl, slotIndex);
					gFindImplCache->insert(inRcvr, inTag, *outImpl, slotIndex);
					return YES;
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
					// found the implementor!
					*outImpl = current;
					*value = ((FrameObject *)ObjectPtr(current))->slot[slotIndex];

					if (r9 != INVALIDPTRREF)
						gROProtoCache->insert(r9, inTag, current, slotIndex);
					if (spm00)
						gProtoCache->insert(left, inTag, current, slotIndex);
					gFindImplCache->insert(inRcvr, inTag, current, slotIndex);
					return YES;
				}
			}

			// slot is not in this context so keep looking up the _proto chain
			slotIndex = FindOffset(frMap, RSYM_proto);
			if (slotIndex == kIndexNotFound)
				break;
			current = ((FrameObject *)ObjectPtr(current))->slot[slotIndex];
		}
		// end-of-proto-lookup

		if (spm00)
		{
			gProtoCache->insert(left, inTag, INVALIDPTRREF, 0);
			slotIndex = 0;
		}
		if (r9 != INVALIDPTRREF)
			gROProtoCache->insert(r9, inTag, INVALIDPTRREF, 0);

		frMap = ((FrameObject *)ObjectPtr(left))->map;
		slotIndex = FindOffset(frMap, RSYM_parent);
		if (slotIndex == kIndexNotFound)
			break;
		left = current = ((FrameObject *)ObjectPtr(left))->slot[slotIndex];
	}
	// end-of-parent-lookup

	gFindImplCache->insert(inRcvr, inTag, INVALIDPTRREF, 0);
	return NO;
}


bool
XFindProtoImplementor(RefArg inRcvr, RefArg inTag, RefVar * outImpl, RefVar * value)
{
	RefVar	left, current;
	Ref		frMap;
	FrameObject * frPtr;
	ArrayIndex	slotIndex;
	bool		exists;

	// must have a context
	if (ISNIL(inRcvr))
		return NO;

	// and it must be a frame
	current = inRcvr;
	frPtr = (FrameObject *)ObjectPtr(current);
	if (NOTFRAME(frPtr))
		ThrowBadTypeWithFrameData(kNSErrNotAFrame, current);
	frMap = frPtr->map;

	left = INVALIDPTRREF;
	for ( ; ; )
	{
		// look in _protoÉ
		slotIndex = FindOffset(frMap, RSYM_proto);
		if (slotIndex == kIndexNotFound)
		// _proto (and therefore implementor) wasnÕt found
			break;

		// Éwhich must be a frame
		current = ((FrameObject *)ObjectPtr(current))->slot[slotIndex];
		frPtr = (FrameObject *)ObjectPtr(current);
		if (NOTFRAME(frPtr))
			ThrowBadTypeWithFrameData(kNSErrNotAFrame, current);

		if (left == INVALIDPTRREF && ISRO(frPtr))
		{
			// this is first context weÕve encountered
			left = current;
			if (gROProtoCache->lookup(left, inTag, (Ref *)outImpl->h, (Ref *)value->h, &exists, &slotIndex))
				return exists;
		}

		frMap = frPtr->map;
		slotIndex = FindOffset(frMap, inTag);
		if (slotIndex != kIndexNotFound)
		{
			// found the implementor!
			*outImpl = current;
			*value = ((FrameObject *)ObjectPtr(current))->slot[slotIndex];

			if (left != INVALIDPTRREF)
				gROProtoCache->insert(left, inTag, current, slotIndex);
			return YES;
		}
		// else slot is not in this context so keep looking up the _proto chain
	}

	// implementor wasnÕt found
	if (left != INVALIDPTRREF)
		gROProtoCache->insert(left, inTag, INVALIDPTRREF, 0);
	return NO;
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
	RefVar	impl, value;

	if (XFindImplementor(inRcvr, inTag, &impl, &value))
		return impl;
	return NILREF;
}


Ref
FindProtoImplementor(RefArg inRcvr, RefArg inTag)
{
	Ref		context, value;
	RefVar	left, current;
	Ref		frMap;
	FrameObject * frPtr;
	ArrayIndex	slotIndex;
	bool		exists;

	// must have a context
	if (ISNIL(inRcvr))
		return NILREF;

	// try the cache first
	if (gProtoCache->lookup(inRcvr, inTag, &context, &value, &exists, &slotIndex))
		return exists ? context : NILREF;

	left = INVALIDPTRREF;
	for (current = inRcvr; NOTNIL(current); )
	{
		frPtr = (FrameObject *)ObjectPtr(current);
		if (NOTFRAME(frPtr))
			ThrowBadTypeWithFrameData(kNSErrNotAFrame, current);

		if (left == INVALIDPTRREF && ISRO(frPtr))
		{
			// this is first context weÕve encountered
			// try the cache
			left = current;
			if (gROProtoCache->lookup(left, inTag, &context, &value, &exists, &slotIndex))
				return exists ? context : NILREF;
		}

		frMap = frPtr->map;
		// look for a slot with the given name in this context
		slotIndex = FindOffset(frMap, inTag);
		if (slotIndex != kIndexNotFound)
		{
			// found the implementor!
			if (left == INVALIDPTRREF)
				gROProtoCache->insert(inRcvr, inTag, current, slotIndex);
			gProtoCache->insert(inRcvr, inTag, current, slotIndex);
			return current;
		}

		// not in this context - look in _proto
		slotIndex = FindOffset(frMap, RSYM_proto);
		if (slotIndex == kIndexNotFound)
		{
			// no _proto
			if (left != INVALIDPTRREF)
				gROProtoCache->insert(left, inTag, INVALIDPTRREF, 0);
			break;
		}
		current = ((FrameObject *)ObjectPtr(current))->slot[slotIndex];
	}

	// implementor wasnÕt found
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
	Ref		context, value;
	RefVar	left, current;
	Ref		frMap;
	FrameObject * frPtr;
	ArrayIndex	slotIndex;
	bool		isFound;

	// must have pointer to exists flag
	if (outExists == NULL)
		outExists = &isFound;

	// nil start context => canÕt be found
	if (ISNIL(inRcvr))
	{
		*outExists = NO;
		return NILREF;
	}

	// try the cache first
	if (gGetVarCache->lookupValue(inRcvr, inTag, &value, outExists))
		return value;

	// now start looking properly
	left = current = inRcvr;
	if (lookup == kLexicalLookup)
	{
		do
		{
			slotIndex = FindOffset(((FrameObject *)ObjectPtr(current))->map, inTag);
			if (slotIndex != kIndexNotFound)
			{
				*outExists = YES;
				gGetVarCache->insert(inRcvr, inTag, current, slotIndex);
				return ((FrameObject *)ObjectPtr(current))->slot[slotIndex];
			}
			current = UnsafeGetFrameSlot(current, RSYM_nextArgFrame, &isFound);
		} while (NOTNIL(current) && Length(current) > 0);
		left = current = UnsafeGetFrameSlot(left, RSYM_parent, &isFound);
	}

	if (NOTNIL(current) && gProtoCache->lookup(current, inTag, &context, &value, outExists, &slotIndex))
	{
		if (*outExists)
		{
			gGetVarCache->insert(inRcvr, inTag, context, slotIndex);
			return value;
		}
		slotIndex = FindOffset(((FrameObject *)ObjectPtr(current))->map, RSYM_parent);
		if (slotIndex == kIndexNotFound)
		{
			*outExists = NO;
			gGetVarCache->insert(inRcvr, inTag, INVALIDPTRREF, 0);
			return NILREF;
		}
		current = ((FrameObject *)ObjectPtr(left))->slot[slotIndex];
	}

	bool  sp = YES;
	while (NOTNIL(current))
	{
		Ref roProto = INVALIDPTRREF;
		frPtr = (FrameObject *)ObjectPtr(current);
		if (NOTFRAME(frPtr))
			ThrowBadTypeWithFrameData(kNSErrNotAFrame, current);

		if (roProto == INVALIDPTRREF && ISRO(frPtr)
		&& gROProtoCache->lookup(roProto = current, inTag, &context, &value, outExists, &slotIndex))
		{
			if (*outExists)
			{
				gGetVarCache->insert(inRcvr, inTag, current, slotIndex);
				if (sp)
					gProtoCache->insert(left, inTag, current, slotIndex);
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
				*outExists = YES;
				gGetVarCache->insert(inRcvr, inTag, current, slotIndex);
				if (sp)
					gProtoCache->insert(left, inTag, current, slotIndex);
				if (roProto != INVALIDPTRREF)
					gROProtoCache->insert(roProto, inTag, current, slotIndex);
				return ((FrameObject *)ObjectPtr(current))->slot[slotIndex];
			}
		}

		// slot is not in this context so keep looking up the _proto chain
		slotIndex = FindOffset(frMap, RSYM_proto);
		if (slotIndex != kIndexNotFound)
			current = ((FrameObject *)ObjectPtr(current))->slot[slotIndex];
		else
		{
			if (sp)
			{
				gProtoCache->insert(left, inTag, INVALIDPTRREF, 0);
				sp = NO;
			}
			if (roProto != INVALIDPTRREF)
				gROProtoCache->insert(roProto, inTag, INVALIDPTRREF, 0);
			frMap = ((FrameObject *)ObjectPtr(left))->map;
			slotIndex = FindOffset(frMap, RSYM_parent);
			if (slotIndex != kIndexNotFound)
				left = current = ((FrameObject *)ObjectPtr(left))->slot[slotIndex];
			else
				break;
		}
	}

	*outExists = NO;
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
	RefVar	left, current;
	bool		isFound;

	// throw exception if no context
	if (ISNIL(inRcvr))
		ThrowExInterpreterWithSymbol(kNSErrNilContext, inRcvr);

	// ensure weÕve got a vital pointer -- original checks each time itÕs set
	if (outExists == NULL)
		outExists = &isFound;

	// assume weÕre going to find it
	*outExists = YES;

	// look thru _parent chain
	for (left = inRcvr; NOTNIL(left); left = GetFrameSlot(left, SYMA(_parent)))
	{
		if (FrameHasSlot(left, inTag))
		{
			if (gInterpreter->tracing >= 2)
				gInterpreter->traceGet(inRcvr, left, inTag);
			return GetFrameSlot(left, inTag);
		}

		// look thru _proto/_nextArgFrame chain
		for (current = GetFrameSlot(left, (inLookup == kLexicalLookup) ? SYMA(_nextArgFrame) : SYMA(_proto));
			  NOTNIL(current);
			  current = GetFrameSlot(current, (inLookup == kLexicalLookup) ? SYMA(_nextArgFrame) : SYMA(_proto)))
		{
			if (FrameHasSlot(current, inTag))
			{
				if (gInterpreter->tracing >= 2)
					gInterpreter->traceGet(inRcvr, current, inTag);
				return GetFrameSlot(current, inTag);
			}
		}
		// lexical lookup only applies to original context
		inLookup = kNoLookup;
	}

	// variable wasnÕt found
	if (gInterpreter->tracing >= 2)
		gInterpreter->traceGet(inRcvr, left, inTag);
	*outExists = NO;
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
	Ref		value;
	RefVar	left, current;
	Ref		frMap;
	FrameObject * frPtr;
	ArrayIndex	slotIndex;
	bool		isFound;

	// throw exception if no context
	if (ISNIL(inRcvr))
		ThrowExInterpreterWithSymbol(kNSErrNilContext, inRcvr);

	// ensure weÕve got a vital pointer
	if (outExists == NULL)
		outExists = &isFound;

	// try the cache first
	if (gProtoCache->lookupValue(inRcvr, inTag, &value, outExists))
		return value;

	left = INVALIDPTRREF;
	for (current = inRcvr; NOTNIL(current); )
	{
		frPtr = (FrameObject *)ObjectPtr(current);
		// must have a frame
		if (NOTFRAME(frPtr))
#if defined(forNTK)
			break;
#else
			ThrowBadTypeWithFrameData(kNSErrNotAFrame, current);
#endif

		if (left == INVALIDPTRREF && ISRO(frPtr))
		{
			left = current;
			if (gROProtoCache->lookupValue(current, inTag, &value, outExists))
				return value;
		}

		// look in the current frame map for the name of the variable
		frMap = frPtr->map;
		slotIndex = FindOffset(frMap, inTag);
		if (slotIndex != kIndexNotFound)
		{
			// found it!
			// cache it and return its value
			*outExists = YES;
			if (gInterpreter->tracing >= 2)
				gInterpreter->traceGet(inRcvr, current, inTag);
			if (left != INVALIDPTRREF)
				gROProtoCache->insert(left, inTag, current, slotIndex);
			gProtoCache->insert(inRcvr, inTag, current, slotIndex);
			return ((FrameObject *)ObjectPtr(current))->slot[slotIndex];
		}

		// follow the _proto chain
		slotIndex = FindOffset(frMap, RSYM_proto);
		if (slotIndex == kIndexNotFound)
			break;
		current = ((FrameObject *)ObjectPtr(current))->slot[slotIndex];
	}

	// variable wasnÕt found
//	if (gInterpreter->tracing >= 2)
//		gInterpreter->traceGet(inRcvr, current, name);
	*outExists = NO;
	if (left != INVALIDPTRREF)
		gROProtoCache->insert(left, inTag, INVALIDPTRREF, 0);
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
	RefVar	left, current;
	Ref		frMap;
	FrameObject * frPtr;
	ArrayIndex	slotIndex;
	bool		exists;

	left = inRcvr;
	if (inLookup & kLexicalLookup)
	{
		// lexical lookup
		// follow _nextArgFrame in current context
		for (current = left; NOTNIL(current); current = UnsafeGetFrameSlot(current, RSYM_nextArgFrame, &exists))
		{
			frMap = ((FrameObject *)ObjectPtr(current))->map;
			slotIndex = FindOffset(frMap, inTag);
			if (slotIndex != kIndexNotFound)
			{
				if (gInterpreter->tracing >= 2)
					gInterpreter->traceSet(inRcvr, current, inTag, inValue);
				((FrameObject *)ObjectPtr(current))->slot[slotIndex] = inValue;
				return YES;
			}
		}
		// not found, try _parent next
		left = UnsafeGetFrameSlot(left, RSYM_parent, &exists);
	}

	while (NOTNIL(left))
	{
		for (current = left; NOTNIL(current); )
		{
			frPtr = (FrameObject *)ObjectPtr(current);
			if (NOTFRAME(frPtr))
				ThrowBadTypeWithFrameData(kNSErrNotAFrame, current);

			frMap = frPtr->map;
			slotIndex = FindOffset(frMap, inTag);
			if (slotIndex != kIndexNotFound)
			{
				// found it!
				if (gInterpreter->tracing >= 2)
					gInterpreter->traceSet(inRcvr, current, inTag, inValue);
				if (left == current)
					// can set it in this context
					((FrameObject *)ObjectPtr(current))->slot[slotIndex] = inValue;
				else
					// must set it in the left frame
					SetFrameSlot(left, inTag, inValue);
				return YES;
			}

			// follow the _proto chain
			slotIndex = FindOffset(frMap, RSYM_proto);
			if (slotIndex == kIndexNotFound)
				break;
			current = ((FrameObject *)ObjectPtr(current))->slot[slotIndex];
		}

		// follow the _parent chain
		frMap = ((FrameObject *)ObjectPtr(left))->map;
		slotIndex = FindOffset(frMap, RSYM_parent);
		if (slotIndex == kIndexNotFound)
			break;
		left = ((FrameObject *)ObjectPtr(left))->slot[slotIndex];
	}

	if ((inLookup & kGlobalLookup) && FrameHasSlot(gVarFrame, inTag))
	{
		if (gInterpreter->tracing >= 2)
			gInterpreter->traceSet(RA(gVarFrame), RA(gVarFrame), inTag, inValue);
		SetFrameSlot(RA(gVarFrame), inTag, inValue);
		return YES;
	}

	if (inLookup & kSetSelfLookup)
	{
		if (gInterpreter->tracing >= 2)
			gInterpreter->traceSet(inRcvr, inRcvr, inTag, inValue);
		SetFrameSlot(inRcvr, inTag, inValue);
		return YES;
	}

	if (inLookup & kSetGlobalLookup)
	{
		if (gInterpreter->tracing >= 2)
			gInterpreter->traceSet(RA(gVarFrame), RA(gVarFrame), inTag, inValue);
		SetFrameSlot(RA(gVarFrame), inTag, inValue);
		return YES;
	}

	return NO;
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
			return YES;
	return NO;
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


