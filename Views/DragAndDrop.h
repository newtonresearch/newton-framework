/*
	File:		DragAndDrop.h

	Contains:	Drag and drop declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__DRAGANDDROP_H)
#define __DRAGANDDROP_H 1

#include "Objects.h"
class CView;

class CDragInfo
{
public:
				CDragInfo(long);
				CDragInfo(RefArg);
				CDragInfo(RefArg, RefArg, RefArg);

	void		createItemFrame(long);
	void		addDragItem(RefArg, RefArg, RefArg);
	void		addDragItem(void);
	void		addItemDragType(long, RefArg);
	void		setItemDragTypes(long, RefArg);
	void		checkTypes(RefArg) const;
	void		findType(long, RefArg) const;

	void		getItemTypes(long) const;
	void		getItemIndType(long, long) const;

	void		setItemDragLabel(long, RefArg);
	void		getItemDragLabel(long) const;

	void		setItemDragRef(long, RefArg);
	void		getItemDragRef(long) const;

	void		setItemView(long, CView *);
	CView *	getItemView(long) const;
};


#endif	/* __DRAGANDDROP_H */
