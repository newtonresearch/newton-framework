/*
	File:		ViewObject.h

	Contains:	CViewObject declarations.
					Every object in the view system is based on CViewObject.

	Written by:	Newton Research Group.
*/

#if !defined(__VIEWOBJECT_H)
#define __VIEWOBJECT_H 1

#include "Objects.h"

/*
**	Put one of these inside the {...} of your view object implementations.
**	It declares some magic stuff you’re probably better off not puzzling over.
**
**	That is:
**
**		class CViewImpl : public CViewObject
**		{
**		public:
**			VIEW_HEADER_MACRO
**			<your methods>
**		};
**
*/

#define VIEW_HEADER_MACRO \
	virtual	ULong		classId(void) const; \
	virtual	bool		derivedFrom(long inClassId) const;

/*
**	For each of your view object implementations, put one of these in your
**	".cc" files.
**
**		#include	"ViewImpl.h"
**
**		VIEW_SOURCE_MACRO(viewClass, CViewImpl, CViewProto)
**
*/

#define VIEW_SOURCE_MACRO(classNumber, className, superclassName) \
	ULong		className::classId(void) const { return classNumber; } \
	bool		className::derivedFrom(long inClassId) const { return inClassId == classNumber || superclassName::derivedFrom(inClassId); }


class CViewObject
{
public:
// allocate view objects using the Memory Manager; zero the memory
	static	void *	operator new(size_t inSize);
	static	void		operator delete(void * inObj);

	VIEW_HEADER_MACRO

							CViewObject() {}
	virtual				~CViewObject();

//	virtual	ULong		key(void);
};


#define 	kFirstClassId	64

#endif	/* __VIEWOBJECT_H */
