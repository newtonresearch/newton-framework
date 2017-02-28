/*
	File:		DragAndDrop.cc

	Contains:	Drag and drop implementation.

	Written by:	Newton Research Group, 2014.
*/

#include "DragAndDrop.h"


CDragInfo::CDragInfo(long)
{}

CDragInfo::CDragInfo(RefArg)
{}

CDragInfo::CDragInfo(RefArg, RefArg, RefArg)
{}

void
CDragInfo::createItemFrame(long)
{}

void
CDragInfo::addDragItem(RefArg, RefArg, RefArg)
{}

void
CDragInfo::addDragItem(void)
{}

void
CDragInfo::addItemDragType(long, RefArg)
{}

void
CDragInfo::setItemDragTypes(long, RefArg)
{}

bool
CDragInfo::checkTypes(RefArg) const
{ return false; }

Ref
CDragInfo::findType(ArrayIndex, RefArg) const
{ return NILREF; }

void
CDragInfo::getItemTypes(long) const
{}

void
CDragInfo::getItemIndType(long, long) const
{}

void
CDragInfo::setItemDragLabel(long, RefArg)
{}

void
CDragInfo::getItemDragLabel(long) const
{}

void
CDragInfo::setItemDragRef(long, RefArg)
{}

void
CDragInfo::getItemDragRef(long) const
{}

void
CDragInfo::setItemView(long, CView *)
{}

CView *
CDragInfo::getItemView(long) const
{}

