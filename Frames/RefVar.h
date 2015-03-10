/*
	File:		RefVar.h

	Contains:	C++ stuff to keep Ref usage simple.

	Written by:	The Newton Tools group.
*/

#if !defined(__REFVAR_H)
#define __REFVAR_H 1

#if !defined(__NEWTONTYPES_H)
#include "NewtonTypes.h"
#endif


/*----------------------------------------------------------------------
	R e f V a r
----------------------------------------------------------------------*/

class RefVar
{
public:
	inline	RefVar();
	inline	RefVar(const Ref r);
	inline	RefVar(const RefVar & o);
	inline	~RefVar();

	RefVar &	operator=(const RefVar & o)		{ h->ref = o.h->ref; return *this; }
	RefVar &	operator=(const Ref r)				{ h->ref = r; return *this; }

	operator RefVar() const							{ return h; }
	operator long() const							{ return h->ref; }

private:
	RefHandle * h;
};


inline	RefVar::RefVar()
{ h = AllocateRefHandle(NILREF); }

inline	RefVar::RefVar(const Ref r)
{ h = AllocateRefHandle(r); }

inline	RefVar::RefVar(const RefVar & o)
{ h = AllocateRefHandle(o->ref); }

inline	RefVar::RefVar(const RefVar & o)
{ h = AllocateRefHandle(o.h->ref); }

inline	RefVar::~RefVar()
{ DisposeRefHandle(h); }


/*----------------------------------------------------------------------
	R e f S t r u c t
----------------------------------------------------------------------*/

class RefStruct : public RefVar
{
public:
	inline	RefStruct();
	inline	RefStruct(const Ref r);
	inline	RefStruct(const RefVar & o);
	inline	RefStruct(const RefStruct & o);
				~RefStruct()								{ }

	RefStruct &	operator=(const Ref r)				{ h->ref = r; return *this; }
	RefStruct &	operator=(const RefVar & o)		{ h->ref = o.h->ref; return *this; }
	RefStruct &	operator=(const RefStruct & o)	{ return operator=((const RefVar &) o); }
};


inline	RefStruct::RefStruct()
{ h->stackPos = 0; }

inline	RefStruct::RefStruct(const Ref r) : RefVar(r)
{ h->stackPos = 0; }

inline	RefStruct::RefStruct(const RefVar & o) : RefVar(o)
{ h->stackPos = 0; }

inline	RefStruct::RefStruct(const RefStruct & o) : RefVar(o)
{ h->stackPos = 0; }


/*----------------------------------------------------------------------
	C O b j e c t P t r
----------------------------------------------------------------------*/

class CObjectPtr : public RefVar
{
public:
					CObjectPtr();
					CObjectPtr(Ref);
					CObjectPtr(RefArg);
					CObjectPtr(const RefStruct &);
					~CObjectPtr();

	CObjectPtr &	operator=(Ref);
	CObjectPtr &	operator=(const CObjectPtr &);

	operator char*() const;
};


/*----------------------------------------------------------------------
	C D a t a P t r
----------------------------------------------------------------------*/

class CDataPtr : public CObjectPtr
{
public:
					CDataPtr() : CObjectPtr() {}	// for MakeStringObject
					CDataPtr(Ref r) : CObjectPtr(r) {}	// for SPrintObject

	CDataPtr &	operator=(Ref);
	CDataPtr &	operator=(const CDataPtr &);

	operator char*() const;
};


#endif	/* __REFVAR_H */
