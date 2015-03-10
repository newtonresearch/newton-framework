/*
	File:		RefStack.h

	Contains:	Stack that holds GC Refs.
					Used by the interpreter VM and when object streaming.

	Written by:	Newton Research Group.
*/

#if !defined(__REFSTACK_H)
#define __REFSTACK_H 1


class RefStack
{
public:
				RefStack();
				~RefStack();

	void		reset(ArrayIndex);
	void		pushNILs(ArrayIndex);

//protected:
	Ref *		top;		// +00
	Ref *		base;		// +04
	Ref *		limit;	// +08
	ArrayIndex	size;		// +0C	number of Refs

friend void MarkRefStack(RefStack *);
friend void UpdateRefStack(RefStack *);
friend bool ReleaseRefStackProc(void *, OpaqueRef *, OpaqueRef *, bool);
};


#endif	/* __REFSTACK_H */
