/*
	File:		PickView.cc

	Contains:	CPickView implementation.
					See NPG 6-37 for info on the pick items. Basically, items can be:
						simple string
						icon with string
						bitmap
						2D grid
						separator line
					See NPG2.1 4-31 for command-key info.

	Written by:	Newton Research Group.
*/

#include "PickView.h"
#include "RootView.h"
#include "Lookup.h"
#include "Funcs.h"
#include "NewtonScript.h"
#include "Strings.h"
#include "RichStrings.h"
#include "UStringUtils.h"
#include "ROMResources.h"
#include "KeyboardKeys.h"

#include "Globals.h"
#include "NewtGlobals.h"
#include "Application.h"

#include "Unit.h"
#include "QDDrawing.h"
#include "DrawText.h"
#include "DrawShape.h"
#include "Sound.h"
#include "Preference.h"

#define kMaxPickItemHeight				28
#define kPickSeparatorHeight			 6

#define kDefaultPickTextItemHeight	13
#define kDefaultPickLeftMargin		 4
#define kDefaultPickRightMargin		 5
#define kDefaultPickTopMargin			 2
#define kDefaultPickBottomMargin		 2
#define kDefaultPickMarkWidth			10


extern "C" {
Ref	FPtInPicture(RefArg inRcvr, RefArg inX, RefArg inY, RefArg inPicture);
Ref	FStyledStrTruncate(RefArg inRcvr, RefArg inStr, RefArg inWidth, RefArg inFont);
Ref	FStrLen(RefArg inRcvr, RefArg inStr);
}

extern void	GetAppAreaBounds(Rect * outBounds);

Ref	ToObject(RefArg inClass, const char * inData, size_t inSize);
bool	FromObject(RefArg inObj, char * outData, size_t& outSize, size_t inBufferSize);

void	DrawStrokeBundle(RefArg inStroke, Rect * inSrcRect, Rect * inDstRect);		// <-- DrawInk.cc needs CTransform


/* -----------------------------------------------------------------------------
	F u n c t i o n s
----------------------------------------------------------------------------- */

bool
AdjustPopupInRect(Rect&, int, int, const Rect&, int)
{}


#pragma mark -
/* -----------------------------------------------------------------------------
	C P i c k V i e w
----------------------------------------------------------------------------- */

VIEW_SOURCE_MACRO(clPickView, CPickView, CView)


/* -----------------------------------------------------------------------------
	Constructor.
	NULL-out lists.
	Args:		inContext
				inView
	Return:	--
----------------------------------------------------------------------------- */

void
CPickView::init(RefArg inContext, CView * inView)
{
	fItemBottoms = NULL;
	fItemInfo = NULL;
	fItemGrid = NULL;
	fPicked.index = kIndexNotFound;

	CView::init(inContext, inView);
}


/* -----------------------------------------------------------------------------
	Destructor.
	Free malloc’d lists.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

CPickView::~CPickView()
{
	if (fItemBottoms != NULL) {
		free(fItemBottoms);
	}
	if (fItemInfo != NULL) {
		free(fItemInfo);
	}
	if (fItemGrid != NULL) {
		for (ArrayIndex i = 0; i < fNumOfItems; ++i) {
			if (fItemGrid[i] != NULL) {
				delete fItemGrid[i];
			}
		}
		free(fItemGrid);
	}
}


/* -----------------------------------------------------------------------------
	Handle an event.
	We’re only interested in clicks which we package up into an aePickItem event
	and send to ourself.
	Args:		inCmd		the event
	Return:	true => we handled it
----------------------------------------------------------------------------- */

bool
CPickView::realDoCommand(RefArg inCmd)
{
	PickStuff selection;
	RefVar cmd;
	RefVar cmdParm;
	bool isHandled = false;

	switch (CommandId(inCmd))
	{
	case aeClick:
		// audible feedback
		FClicker(RA(NILREF));
		// track the pen
		trackStroke(((CUnit *)CommandParameter(inCmd))->stroke(), &selection);
		// dispatch pick command, parm = PickStuff selection info
		cmd = MakeCommand(aePickItem, this, (OpaqueRef)&selection);
		cmdParm = ToObject(SYMA(string), (const char *)&selection, sizeof(selection));
		CommandSetFrameParameter(cmd, cmdParm);
		gApplication->dispatchCommand(cmd);
		CommandSetResult(inCmd, 1);
		isHandled = true;
		break;

	case aePickItem: {
		fIsPicked = true;
		SetFrameSlot(fContext, SYMA(allowKeysThrough), RA(TRUEREF));
		// extract PickStuff selection info
		cmdParm = CommandFrameParameter(inCmd);
		if (NOTNIL(cmdParm)) {
			size_t size;
			FromObject(cmdParm, (char *)&selection, size, sizeof(selection));
		} else {
			selection.index = CommandParameter(inCmd);
			selection.isGrid = false;
		}
		CView * vwContext = pickItem(&selection);
		if (vwContext != NULL && fAutoClose) {
			gApplication->dispatchCommand(MakeCommand(aeDropChild, vwContext->fParent, (OpaqueRef)vwContext));
		}
		CommandSetResult(inCmd, 1);
		isHandled = true;
		}
		break;

	default:
		isHandled = CView::realDoCommand(inCmd);
		break;
	}

	return isHandled;
}


/* -----------------------------------------------------------------------------
	Draw the picker.
	Args:		inRect		area of interest -- actually we do it all
	Return:	--
----------------------------------------------------------------------------- */

void
CPickView::realDraw(Rect& inRect)
{
	//sp-30
	StyleRecord * stylePtr = &fTextStyle;	//sp2C
//	StyleRecord * sp08 = &fTextStyle;
	int indent = -1;	//r8
	bool hasCmdKeys = gRootView->commandKeyboardConnected() && NOTNIL(fCommandKeys);
	Point origin = childOrigin();	//sp00

	//sp28 = fItems
	//sp24 = 'pickLeftMargin
	//sp20 = commandKeyIcon
	//sp1C = shiftKeyIcon
	//sp18 = optionKeyIcon
	//sp14 = controlKeyIcon

	RefVar displayItem;	//sp10
	RefVar item;		//sp0C
	for (ArrayIndex i = 0; i < fNumOfItems; ++i) {
		//sp-08
		item = GetArraySlot(fItems, i);
		int itemIndent = getDisplayIndent(item);
		if (itemIndent > indent) {
			indent = itemIndent;	// indent all subsequent text items by this amount to allow for fat icons
		}
		bool isPickable; //sp04
		UniChar markCh; // sp00
		displayItem = getDisplayItem(i, &isPickable, &markCh);
		int itemTop = viewBounds.top + origin.v;	//r6
		if (i > 0) {
			itemTop += fItemBottoms[i-1];
		}
		int itemBottom = viewBounds.top + origin.v + fItemBottoms[i];	//r10
		int baseline = fTopMargin + (itemTop + itemBottom - (f44.ascent + f44.descent))/2 - 1;	//r9
		int markBaseline = 0;	//r7
		if (IsString(displayItem)) {
			// it’s plain text, possibly with an icon
			//sp-04
			FPoint txLoc;
			txLoc.x = viewBounds.left + fLeftMargin;
			txLoc.y = baseline;
			RefVar icon(getDisplayIcon(item));
			if (NOTNIL(icon)) {
				// yup, item has an icon too
				//sp-08
				Rect iconRect;
				iconRect.left = viewBounds.left + fLeftMargin;
				iconRect.top = itemTop + fTopMargin;
				//sp-0C
				int iconWd;	//r6
				Rect iconBounds;
				FromObject(GetFrameSlot(icon, SYMA(bounds)), &iconBounds);
				if (indent >= 0) {
					// centre the icon in this space
					iconRect.left -= RINT(getProto(SYMA(pickLeftMargin)))/2;
					iconWd = indent;
				} else {
					iconWd = RectGetWidth(iconBounds) + 2;
				}
				iconRect.right = iconRect.left + iconWd;
				iconRect.bottom = itemBottom;
				DrawPicture(icon, &iconRect, vjCenterH|vjCenterV, 0);	//srcCopy
				txLoc.x += iconWd;	//short/Fixed
			}
//0018689C
			//sp-44
			CRichString str(displayItem);	//sp1C
			int txWd = getItemLength(i);
			TextBoundsInfo txBounds;
			DrawRichString(str, 0, txWd>=0? txWd : -txWd, stylePtr, txLoc, NULL, &txBounds);
			if (txWd < 0) {
				//sp-04
				UniChar ch = 0x2026;	//ellipsis
//				txLoc.x += txBounds.x14;	// presumably text width
				DrawTextOnce(&ch, 1, &stylePtr, NULL, txLoc, NULL, NULL);
			}
			markBaseline = baseline;
		} else if (IsSymbol(displayItem)) {
			// it’s a separator
			CGContextSaveGState(quartz);
			if (EQ(displayItem, SYMA(pickSeparator))) {
				//PatHandle savedPat = GetFgPattern();
				//SetFgPattern(GetStdPattern(grayPat));
//				MoveTo(viewBounds.left+1, itemTop);
//				LineTo(viewBounds.right-1, itemTop);
				Point pt1 = {viewBounds.left+1, itemTop};
				Point pt2 = {viewBounds.right-1, itemTop};
				SetPattern(grayPat);
				StrokeLine(pt1, pt2);
				//SetFgPattern(savedPat);
			} else if (EQ(displayItem, SYMA(pickSolidSeparator))) {
				// PenState savedPen; GetPenState(&savedPen);
				//SetFgPattern(GetStdPattern(blackPat));
//				PenSize(1,2);
//				MoveTo(viewBounds.left+1, itemTop);	// original doesn’t add 1
//				LineTo(viewBounds.right-1, itemTop);
				Point pt1 = {viewBounds.left+1, itemTop};
				Point pt2 = {viewBounds.right-1, itemTop};
				SetPattern(blackPat);
				SetLineWidth(2);
				StrokeLine(pt1, pt2);
				//SetPenState(savedPen);
			}
			CGContextRestoreGState(quartz);
		} else if (IsFrame(displayItem)) {
			// it’s a graphic of some kind
			//sp-08
			Rect iconRect;
			iconRect.left = viewBounds.left + fLeftMargin;
			iconRect.top = itemTop + fTopMargin;
			if (FrameHasSlot(displayItem, SYMA(bits)) || FrameHasSlot(displayItem, SYMA(colorData))) {
				DrawPicture(displayItem, &iconRect, vjLeftH|vjTopV, 0);	//srcCopy
			} else if (FrameHasSlot(displayItem, SYMA(picture))) {
				iconRect.right = iconRect.left;
				iconRect.bottom = iconRect.top;
				DrawPicture(GetFrameSlot(displayItem, SYMA(picture)), &iconRect, vjLeftH|vjTopV, 0);	//srcCopy
			} else if (FrameHasSlot(displayItem, SYMA(strokeList))) {
				//sp-10
				RefVar stroke(GetFrameSlot(displayItem, SYMA(strokeList)));
				Rect strokeBounds;
				if (!FromObject(GetFrameSlot(stroke, SYMA(bounds)), &strokeBounds)) {
					ThrowMsg("bad strokeBounds frame");
				}
				int strokeHt = RectGetHeight(strokeBounds);
				int strokeWd = RectGetWidth(strokeBounds);
				if (strokeHt > kMaxPickItemHeight) {
					// limit height to fit menu item -- scale stroke
					strokeWd = strokeWd * kMaxPickItemHeight / strokeHt;
					strokeHt = kMaxPickItemHeight;
				}
				iconRect.right = iconRect.left + strokeWd;
				iconRect.bottom = iconRect.top + strokeHt;
				DrawStrokeBundle(stroke, &strokeBounds, &iconRect);
			}
			markBaseline = itemTop + (itemBottom - itemTop)/2 + f44.ascent/2;	//f44 is not FontInfo as we know it
		}

//00186CB4
		if (fHasMark && markCh != 0 && markCh != ' ') {
			// item has a mark eg √
			FPoint txLoc;
			txLoc.x = fMarkIndent;
			txLoc.y = markBaseline;
			DrawTextOnce(&markCh, 1, &stylePtr, NULL, txLoc, NULL, NULL);
		}
		RefVar cmdKey(getKeyCommand(i));
		if (hasCmdKeys && NOTNIL(cmdKey)) {
			// item has a cmd-key equivalent
			UniChar cmdChar = GetDisplayCmdChar(cmdKey);
			if (cmdChar != 0) {
				FPoint txLoc;
				txLoc.x = viewBounds.right - fRightMargin - fCommandKeySpace;
				txLoc.y = baseline;
				// draw the char
				UpperCaseText(&cmdChar, 1);
				DrawTextOnce(&cmdChar, 1, &stylePtr, NULL, txLoc, NULL, NULL);

				// and its modifiers, right-left from the left edge of the item text
				Rect cmdKeyBox;
				cmdKeyBox.top = baseline - 7;
				cmdKeyBox.bottom = baseline;
				cmdKeyBox.right = viewBounds.right - fRightMargin - fCommandKeySpace - 2;
				ULong modifiers = KeyCommandModifiers(cmdKey);
#if 0
				if (FLAGTEST(modifiers, kCmdModifier)) {
					cmdKeyBox.left = cmdKeyBox.right - 9;		// command key icon width
					DrawBitmap(RA(commandKeyIcon), &cmdKeyBox, modeCopy);
					cmdKeyBox.right -= 11;	// command key icon width + 2
				}
				if (FLAGTEST(modifiers, kShiftModifier)) {
					cmdKeyBox.left = cmdKeyBox.right - 9;
					DrawBitmap(RA(shiftKeyIcon), &cmdKeyBox, modeCopy);
					cmdKeyBox.right -= 11;
				}
				if (FLAGTEST(modifiers, kOptionModifier)) {
					cmdKeyBox.left = cmdKeyBox.right - 9;			// should this be 10?
					DrawBitmap(RA(optionKeyIcon), &cmdKeyBox, modeCopy);
					cmdKeyBox.right -= 12;								// or should this be 11?
				}
				if (FLAGTEST(modifiers, kControlModifier)) {
					cmdKeyBox.left = cmdKeyBox.right - 5;
					DrawBitmap(RA(controlKeyIcon), &cmdKeyBox, modeCopy);
				}
#endif
			}
		}
	}
	if (fPicked.index != kIndexNotFound) {
		invertItem(&fPicked);
	}
}


/* -----------------------------------------------------------------------------
	Hide the picker.
	If it’s being dismissed, call its pickCancelledScript.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CPickView::hide(void)
{
	bool isCancelReqd = FLAGTEST(viewFlags, vVisible) && !fIsPicked && fAutoClose;
	SetFrameSlot(fContext, SYMA(allowKeysThrough), RA(TRUEREF));

	CView::hide();

	if (isCancelReqd) {
		RefVar scriptContext(getProto(SYMA(callbackContext)));
		if (NOTNIL(scriptContext)) {
			bool exists;
			GetVariable(scriptContext, SYMA(pickCancelledScript), &exists);
			if (exists) {
				DoMessage(scriptContext, SYMA(pickCancelledScript), RA(NILREF));
			}
		} else {
			// no separate context -- run script in self
			runScript(SYMA(pickCancelledScript), RA(NILREF), true);
		}
	}
	// selection is no longer valid
	fPicked.index = kIndexNotFound;
}


/* -----------------------------------------------------------------------------
	Feedback -- flash the currently picked item.
	Args:		inPicked			descriptor of the currently picked item
	Return:	--
----------------------------------------------------------------------------- */

void
CPickView::flashItem(PickStuff * inPicked)
{
	if (inPicked->index != kIndexNotFound) {
		for (ArrayIndex i = 0; i < 3; ++i) {
			Wait(5);
			invertItem(inPicked);
		}
	}
}


void
CPickView::invertItem(PickStuff * inPicked)
{
	Rect itemRect;
	getGridItemRect(inPicked, &itemRect);
//	InvertRect(&itemRect);
}


/* -----------------------------------------------------------------------------
	Return the rect enclosing the picked item IN A GRID, in screen coordinates.
	Args:		inPicked		descriptor of picked item
				outRect		the result
	Return:	--
----------------------------------------------------------------------------- */

void
CPickView::getGridItemRect(PickStuff * inPicked, Rect * outRect)
{
	// get the bounds of the grid
	getItemRect(inPicked, outRect);
	// narrow it down to the cell
	GridInfo * grid = fItemGrid[inPicked->index];
	if (grid != NULL) {
		outRect->left = fLeftMargin + grid->outerFrameWd + outRect->left + (inPicked->gridX * (grid->cellWd + grid->cellFrameWd));
		outRect->top = fTopMargin + grid->outerFrameWd + outRect->top + (inPicked->gridY * (grid->cellHt + grid->cellFrameWd));
		outRect->right = outRect->left + grid->cellWd;
		outRect->bottom = outRect->top + grid->cellHt;
	}
}


/* -----------------------------------------------------------------------------
	Return the rect enclosing the picked item, in screen coordinates.
	Args:		inPicked		descriptor of picked item
				outRect		the result
	Return:	--
----------------------------------------------------------------------------- */

void
CPickView::getItemRect(PickStuff * inPicked, Rect * outRect)
{
	int itemTop = 0;
	if (inPicked->index != 0) {
		itemTop = fItemBottoms[inPicked->index - 1];
	}
	int itemBottom = fItemBottoms[inPicked->index];
	Point origin = childOrigin();	// because it might be scrolled
	outRect->left = viewBounds.left;
	outRect->top = origin.v + viewBounds.top + itemTop;
	outRect->right = viewBounds.right;
	outRect->bottom = origin.v + viewBounds.top + itemBottom;
}


/* -----------------------------------------------------------------------------
	Return fixed height for string or bitmap item.
	Args:		inItem		popup item frame
	Return:	height		-1 => height is not fixed
----------------------------------------------------------------------------- */

int
CPickView::getDisplayFixedHeight(RefArg inItem)
{
	if (IsFrame(inItem) ) {
		Ref height = GetFrameSlot(inItem, SYMA(fixedHeight));
		if (NOTNIL(height)) {
			return RINT(height);
		}
	}
	return -1;
}


/* -----------------------------------------------------------------------------
	Return indent for string with icon item.
	Args:		inItem		popup item frame
	Return:	indent
----------------------------------------------------------------------------- */

int
CPickView::getDisplayIndent(RefArg inItem)
{
	if (IsFrame(inItem) ) {
		Ref indent = GetFrameSlot(inItem, SYMA(indent));
		if (NOTNIL(indent)) {
			return RINT(indent);
		}
	}
	return -1;
}


/* -----------------------------------------------------------------------------
	Return icon for string with icon item.
	Args:		inItem		popup item frame
	Return:	icon
----------------------------------------------------------------------------- */

Ref
CPickView::getDisplayIcon(RefArg inItem)
{
	if (IsFrame(inItem) ) {
		return GetFrameSlot(inItem, SYMA(icon));
	}
	return NILREF;
}


/* -----------------------------------------------------------------------------
	Return grid info for 2D grid item.
	Args:		inItem		popup item frame
	Return:	grid info struct -- caller must release it
----------------------------------------------------------------------------- */

GridInfo *
CPickView::getGridInfo(RefArg inItem, Rect * inBounds)
{
	GridInfo * info = NULL;

	if (IsFrame(inItem) ) {
		RefVar value(GetFrameSlot(inItem, SYMA(width)));
		if (NOTNIL(value)) {
			info = new GridInfo;
			if (info == NULL) {
				OutOfMemory();
			}
			int numOfColumns, numOfRows;
			numOfColumns = RINT(value);
			value = GetFrameSlot(inItem, SYMA(height));
			numOfRows = RINT(value);
			value = GetFrameSlot(inItem, SYMA(cellFrame));
			info->cellFrameWd = ISNIL(value) ? 1 : RINT(value);	// default cell frame width is 1
			value = GetFrameSlot(inItem, SYMA(outerFrame));
			info->outerFrameWd = ISNIL(value) ? 2 : RINT(value);	// default outer frame width is 2
			info->numOfRows = numOfRows;
			info->numOfColumns = numOfColumns;
			info->cellWd = ((RectGetWidth(*inBounds) - info->outerFrameWd*2) + info->cellFrameWd) / numOfColumns - info->cellFrameWd;
			info->cellHt = ((RectGetHeight(*inBounds) - info->outerFrameWd*2) + info->cellFrameWd) / numOfRows - info->cellFrameWd;
		}
	}

	return info;
}


void
CPickView::setItemFlags(PickStuff * inPicked, bool inPickable, UniChar inMark)
{
	ItemInfo item;
	item.isPickable = inPickable;
	item.markCharacter = inMark;
	fItemInfo[inPicked->index] = item;
}


void
CPickView::getItemFlags(PickStuff * inPicked, bool * outPickable, UniChar * outMark)
{
	ItemInfo item = fItemInfo[inPicked->index];
	*outPickable = item.isPickable;
	*outMark = item.markCharacter;
}


void
CPickView::setItemLength(PickStuff * inPicked, int inLength)
{
	fItemInfo[inPicked->index].length = inLength;
}


int
CPickView::getItemLength(ArrayIndex index)
{
	return fItemInfo[index].length;
}


bool
CPickView::isItemNoPickable(ArrayIndex index)
{
	return fItemInfo[index].isPickable;
}


/* -----------------------------------------------------------------------------
	Track the stroke over the picker.
	Args:		inStroke		the stroke
				outPicked	the result
	Return:	--
----------------------------------------------------------------------------- */

void
CPickView::trackStroke(CStroke * inStroke, PickStuff * outPicked)
{
	inStroke->inkOff(true);
	gRootView->update();
	// don’t show the busy box while tracking
	BusyBoxSend(53);
	Point pt = inStroke->firstPoint();
	pickableItem(pt, outPicked);
	if (outPicked->index != kIndexNotFound) {
		invertItem(outPicked);
	}
	while (!inStroke->isDone()) {
		Point finalPt = inStroke->finalPoint();
		PickStuff currentPick;
		pickableItem(finalPt, &currentPick);
		if (currentPick.index == outPicked->index
		&&  currentPick.gridX == outPicked->gridX
		&&  currentPick.gridY == outPicked->gridY) {
			// selection hasn’t changed
			Wait(1);	// could probably wait longer
		} else {
			if (outPicked->index != kIndexNotFound) {
				// unhilite previous item
				invertItem(outPicked);
			}
			if (currentPick.index != kIndexNotFound) {
				// hilite new selection
				invertItem(&currentPick);
			}
			*outPicked = currentPick;
		}
	}
	BusyBoxSend(54);
}


/* -----------------------------------------------------------------------------
	An item has been tapped. Pick it.
	Args:		inPicked			descriptor of the currently picked item
	Return:	this?
----------------------------------------------------------------------------- */

CView *
CPickView::pickItem(PickStuff * inPicked)
{
	fPicked = *inPicked;
	// trivial rejection
	if (inPicked->index == kIndexNotFound) {
		fIsPicked = false;
		return this;
	}

	RefVar savedState(GetProtoVariable(fContext, SYMA(allowKeysThrough)));
	SetFrameSlot(fContext, SYMA(allowKeysThrough), RA(TRUEREF));

	// provide selection feedback
	flashItem(inPicked);
	if (fAutoClose) {
		hide();
	}

	CView * target;
	bool isCmdKeyActioned = false;
	RefVar scriptContext(getProto(SYMA(callbackContext)));
	RefVar isActionReqd;
	if (NOTNIL(scriptContext)) {
		isActionReqd = GetProtoVariable(scriptContext, SYMA(alwaysCallPickActionScript));
	}
	if (ISNIL(isActionReqd)) {
		RefVar keyCmdFrame(getKeyCommand(inPicked->index));
		if (NOTNIL(keyCmdFrame)) {
			RefVar msg(GetFrameSlot(keyCmdFrame, SYMA(keyMessage)));
			if (NOTNIL(msg)) {
				target = gRootView->caretView();
				if (target == NULL) {
					target = GetView(scriptContext);
				}
				if (target == NULL) {
					target = this;
				}
				if (target != NULL) {
					SendKeyMessage(target, msg);
					isCmdKeyActioned = true;
					target = GetView(scriptContext);
					if (target != NULL) {
						target->select(false, false);
					}
				}
			}
		}
	}
	if (!isCmdKeyActioned) {
		Ref topIndexRef = getProto(SYMA(topItem));
		Ref indexRef = MAKEINT(RINT(topIndexRef) + inPicked->index);
		RefVar index(indexRef);
		if (inPicked->isGrid) {
			index = Clone(RA(protoGridItem));
			SetFrameSlot(index, SYMA(index), indexRef);
			SetFrameSlot(index, SYMA(x), MAKEINT(inPicked->gridX));
			SetFrameSlot(index, SYMA(y), MAKEINT(inPicked->gridY));
		}
		RefVar args(MakeArray(1));
		SetArraySlot(args, 0, index);
		if (NOTNIL(scriptContext)) {
			DoMessage(scriptContext, SYMA(pickActionScript), args);
		} else {
			runScript(SYMA(pickActionScript), args, true);
		}
	}
	target = GetView(fContext);
	if (target != NULL) {
		fPicked.index = kIndexNotFound;
		SetFrameSlot(fContext, SYMA(allowKeysThrough), savedState);
	}
}


/* -----------------------------------------------------------------------------
	An item in a grid has been tapped. Update the pick stuff to indicate which.
	Args:		inPt				where tapped
				ioPicked			descriptor of the currently picked item
	Return:	--
----------------------------------------------------------------------------- */

void
CPickView::subItem(Point & inPt, PickStuff * ioPicked)
{
	GridInfo * grid = fItemGrid[ioPicked->index];
	if (grid != NULL) {
		Rect cellRect;
		ioPicked->isGrid = true;
		for (ArrayIndex row = 0; row < grid->numOfRows; ++row) {
			ioPicked->gridY = row;
			for (ArrayIndex col = 0; col < grid->numOfColumns; ++col) {
				ioPicked->gridX = col;
				getGridItemRect(ioPicked, &cellRect);
				if (PtInRect(inPt, &cellRect)) {
					return;
				}
			}
		}
		ioPicked->index = kIndexNotFound;
	}
}


/* -----------------------------------------------------------------------------
	An item has been tapped. Update the pick stuff to indicate which.
	Args:		inPt				where tapped
				ioPicked			descriptor of the currently picked item
	Return:	--
----------------------------------------------------------------------------- */

void
CPickView::item(Point & inPt, PickStuff * ioPicked)
{
	ioPicked->index = kIndexNotFound;
	ioPicked->isGrid = false;
	if (PtInRect(inPt, &viewBounds)) {
		Rect itemRect;
		for (ArrayIndex i = 0; i < fNumOfItems; ++i) {
			ioPicked->index = i;
			getItemRect(ioPicked, &itemRect);
			if (PtInRect(inPt, &itemRect)) {
				subItem(inPt, ioPicked);
				return;
			}
		}
	}
}


/* -----------------------------------------------------------------------------
	Identify the picked item from pen location.
	Update the pick stuff to indicate which.
	Args:		inPt				where tapped
				ioPicked			descriptor of the currently picked item
	Return:	--
----------------------------------------------------------------------------- */

void
CPickView::pickableItem(Point & inPt, PickStuff * inPicked)
{
	item(inPt, inPicked);
	// trivial rejection
	if (inPicked->index == kIndexNotFound) {
		return;
	}

	bool isPickable = false;
	UniChar ch;
	getItemFlags(inPicked, &isPickable, &ch);

	if (!isPickable) {
		Point pt = inPt;
		Rect itemRect;
		getItemRect(inPicked, &itemRect);
		Point midPt = MidPoint(&itemRect);	// sic -- unused
		pt.v -= RectGetHeight(itemRect);
		pickableItem(pt, inPicked);	// try the previous
	} else if (inPicked->isGrid) {
		RefVar picFrame(getDisplayItem(inPicked->index, &isPickable, &ch));
		if (FrameHasSlot(picFrame, SYMA(mask))) {
			// get top-left of item
			Rect itemRect;
			PickStuff picked = *inPicked;
			picked.gridY = 0;
			picked.gridX = 0;
			getGridItemRect(&picked, &itemRect);
			GridInfo * grid = fItemGrid[inPicked->index];
			// so we can calculate position of pen relative to picture
			int ptX = inPt.h - (itemRect.left - grid->outerFrameWd);
			int ptY = inPt.v - (itemRect.top - grid->outerFrameWd);
			if (ISNIL(FPtInPicture(RA(NILREF), MAKEINT(ptX), MAKEINT(ptY), picFrame))) {
				inPicked->index = kIndexNotFound;
			}
		}
	}
}


/* -----------------------------------------------------------------------------
	Return the object to be displayed.
	Args:		index				index into list of items
				outPickable		is pickable
				outMark			the mark, 0 => not specified
	Return:	Ref
----------------------------------------------------------------------------- */

Ref
CPickView::getDisplayItem(ArrayIndex index, bool * outPickable, UniChar * outMark)
{
	if (outPickable != NULL) {
		*outPickable = true;
	}
	if (outMark != NULL) {
		*outMark = 0;
	}

	RefVar item(GetArraySlot(fItems, index));
	if (IsFrame(item)) {
		RefVar value(GetFrameSlot(item, SYMA(item)));
		if (ISNIL(value)) {
			if (NOTNIL(fCommandKeys)) {
				value = GetArraySlot(fCommandKeys, index);
				if (NOTNIL(value)) {
					item = GetFrameSlot(value, SYMA(name));
				}
				if (ISNIL(item)) {
					item = RA(emptyString);
				}
			}
		} else {
			item = value;
		}
		if (outMark != NULL && (value = GetFrameSlot(item, SYMA(mark)), NOTNIL(value))) {
			*outMark = RCHAR(value);
		}
		if (outPickable != NULL && FrameHasSlot(item, SYMA(pickable))) {
			*outPickable = NOTNIL(GetFrameSlot(item, SYMA(pickable)));
		}

	} else if (outPickable != NULL && IsSymbol(item)) {
		if (EQ(item, SYMA(pickSeparator)) || EQ(item, SYMA(pickSolidSeparator))) {
			*outPickable = false;
		}
	}
	return item;
}


/* -----------------------------------------------------------------------------
	Return the text object to be displayed.
	Args:		index				index into list of items
	Return:	Ref				nil => is not pickable or is not a string
----------------------------------------------------------------------------- */

Ref
CPickView::getItemNoText(ArrayIndex index)
{
	bool isPickable;
	RefVar item(getDisplayItem(index, &isPickable, NULL));
	if (isPickable && IsString(item)) {
		return item;
	}
	return NILREF;
}


void
CPickView::keyToNextItem(ArrayIndex index)
{
	GridInfo * grid;
	Rect itemRect;
	PickStuff nextPicked;
	nextPicked.index = index;
	// find next pickable item
	while (nextPicked.index < fNumOfItems && !isItemNoPickable(nextPicked.index)) {
		++nextPicked.index;
	}

	if (nextPicked.index < fNumOfItems) {
		// set up grid info
		grid = fItemGrid[nextPicked.index];
		if (grid != NULL) {
			nextPicked.isGrid = true;
			nextPicked.gridY = 0;
			nextPicked.gridX = 0;
		} else {
			nextPicked.isGrid = false;
		}
		getGridItemRect(&nextPicked, &itemRect);
	}

	while (fPicked.index == kIndexNotFound && nextPicked.index < fNumOfItems && viewBounds.top > itemRect.top) {
		if (nextPicked.isGrid && nextPicked.gridY < grid->numOfRows) {
			++nextPicked.gridY;
		} else {
			do {
				++nextPicked.index;
			} while (nextPicked.index < fNumOfItems && !isItemNoPickable(nextPicked.index));
			if (nextPicked.index < fNumOfItems) {
				grid = fItemGrid[nextPicked.index];
				if (grid != NULL) {
					nextPicked.isGrid = true;
					nextPicked.gridY = 0;
					nextPicked.gridX = 0;
				} else {
					nextPicked.isGrid = false;
				}
			}
		}
		if (nextPicked.index < fNumOfItems) {
			getGridItemRect(&nextPicked, &itemRect);
		}
	}

	// update currently-picked info
	if (nextPicked.index != fNumOfItems) {
		fPicked.index = nextPicked.index;
		fPicked.isGrid = nextPicked.isGrid;
		if (nextPicked.isGrid) {
			fPicked.gridY = nextPicked.gridY;
			fPicked.gridX = nextPicked.gridX;
		}
	}
}


void
CPickView::keyToPrevItem(ArrayIndex index)
{
	GridInfo * grid;
	Rect itemRect;
	PickStuff nextPicked;
	nextPicked.index = index;
	// find next pickable item
	while (nextPicked.index != kIndexNotFound && !isItemNoPickable(nextPicked.index)) {		// yes, relies on kIndexNotFound == -1
		--nextPicked.index;
	}

	if (nextPicked.index != kIndexNotFound) {
		// set up grid info
		grid = fItemGrid[nextPicked.index];
		if (grid != NULL) {
			nextPicked.isGrid = true;
			nextPicked.gridY = grid->numOfRows - 1;
			nextPicked.gridX = 0;
		} else {
			nextPicked.isGrid = false;
		}
		getGridItemRect(&nextPicked, &itemRect);
	}

	while (fPicked.index == kIndexNotFound && nextPicked.index != kIndexNotFound && viewBounds.bottom < itemRect.bottom) {
		if (nextPicked.isGrid && nextPicked.gridY > 0) {
			--nextPicked.gridY;
		} else {
			do {
				--nextPicked.index;
			} while (nextPicked.index != kIndexNotFound && !isItemNoPickable(nextPicked.index));
			if (nextPicked.index != kIndexNotFound) {
				grid = fItemGrid[nextPicked.index];
				if (grid != NULL) {
					nextPicked.isGrid = true;
					nextPicked.gridY = grid->numOfRows - 1;
					nextPicked.gridX = 0;
				} else {
					nextPicked.isGrid = false;
				}
			}
		}
		if (nextPicked.index != kIndexNotFound) {
			getGridItemRect(&nextPicked, &itemRect);
		}
	}

	// update currently-picked info
	if (nextPicked.index != kIndexNotFound) {
		fPicked.index = nextPicked.index;
		fPicked.isGrid = nextPicked.isGrid;
		if (nextPicked.isGrid) {
			fPicked.gridY = nextPicked.gridY;
			fPicked.gridX = nextPicked.gridX;
		}
	}
}


/* -----------------------------------------------------------------------------
	Handle a key press.
	Picker items can be selected by cmd-char or by type select.
	Args:		inChar			the character typed
				inKeyFlags		the key code and modifier flags
	Return:	true => we handled it
----------------------------------------------------------------------------- */

bool
CPickView::handleKeyDown(UniChar inChar, ULong inKeyFlags)
{
	ULong modifiers = inKeyFlags & 0x3E000000;
	int keyCode = inKeyFlags & 0x0000FFFF;	//r7
	GridInfo * grid = NULL;	//r1
	if (fPicked.index != kIndexNotFound) {
		grid = fItemGrid[fPicked.index];
	}
	bool isUpdated = false;	//r5
	bool isDismissed = false;	//r8
	ArrayIndex index;

	switch (keyCode) {
	case 0x03:	//kEnterKey
	case 0x0D:	//kReturnKey
		isUpdated = true;
		if (fPicked.index != kIndexNotFound) {
			FClicker(RA(NILREF));
			RefVar cmd(MakeCommand(aePickItem, this, (OpaqueRef)fPicked.index));
			RefVar cmdParm(ToObject(SYMA(string), (const char *)&fPicked, sizeof(fPicked)));
			CommandSetFrameParameter(cmd, cmdParm);
			SetFrameSlot(fContext, SYMA(allowKeysThrough), RA(TRUEREF));
			gApplication->dispatchCommand(cmd);
			isDismissed = true;
		}
		break;

	case 0x09:	//kTabKey
	case 0x1F:	//kDownArrowKey
		isUpdated = true;
		if (grid != NULL && fPicked.gridY != fNumOfItems - 1) {
			++fPicked.gridY;
		} else {
			index = (fPicked.index == kIndexNotFound)? 0 : fPicked.index + 1;
			keyToPrevItem(index);
		}
		break;

	case 0x1E:	//kUpArrowKey
		isUpdated = true;
		if (grid != NULL && fPicked.gridY != 0) {
			--fPicked.gridY;
		} else {
			index = (fPicked.index == kIndexNotFound)? fNumOfItems : fPicked.index;
			keyToPrevItem(index - 1);
		}
		break;

	case 0x1C:	//kLeftArrowKey
		isUpdated = true;
		if (fPicked.index == kIndexNotFound) {
			for (index = 0; index < fNumOfItems; ++index) {
				if (isItemNoPickable(index)) {
					break;
				}
			}
			if (index != fNumOfItems && (grid = fItemGrid[index]) != NULL) {
				fPicked.index = index;
				fPicked.isGrid = true;
				fPicked.gridY = 0;
				fPicked.gridX = grid->numOfColumns - 1;
			}
		} else if (grid != NULL && fPicked.gridY != 0) {
			--fPicked.gridY;
		}
		break;

	case 0x1D:	//kRightArrowKey
		isUpdated = true;
		if (fPicked.index == kIndexNotFound) {
			for (index = 0; index < fNumOfItems; ++index) {
				if (isItemNoPickable(index)) {
					break;
				}
			}
			if (index != fNumOfItems && (grid = fItemGrid[index]) != NULL) {
				fPicked.index = index;
				fPicked.isGrid = true;
				fPicked.gridY = 0;
				fPicked.gridX = 0;
			}
		} else if (grid != NULL && fPicked.gridY < grid->numOfColumns - 1) {
			++fPicked.gridY;
		}
		break;

	default:
		if (IsArray(fCommandKeys) && (index = FindKeyCommandInArray(fCommandKeys, keyCode, modifiers, NULL, NULL)) != kIndexNotFound) {
			FClicker(RA(NILREF));
			PickStuff prevPicked = fPicked;
			fPicked.index = index;
			if (fItemGrid[index] != NULL) {
				fPicked.isGrid = true;
				fPicked.gridY = 0;
				fPicked.gridX = 0;
			} else {
				fPicked.isGrid = false;
			}
			if (index != prevPicked.index || (fPicked.isGrid && (fPicked.gridX != prevPicked.gridX || fPicked.gridY != prevPicked.gridY))) {
				dirty();
				gRootView->update();
			}
			
			RefVar cmd(MakeCommand(aePickItem, this, (OpaqueRef)fPicked.index));
			RefVar cmdParm(ToObject(SYMA(string), (const char *)&fPicked, sizeof(fPicked)));
			CommandSetFrameParameter(cmd, cmdParm);
			SetFrameSlot(fContext, SYMA(allowKeysThrough), RA(TRUEREF));
			gApplication->dispatchCommand(cmd);
			isDismissed = true;
			isUpdated = true;
		}
		if (!isDismissed) {
			// check cmd-. cmd-w esc
			if ((FLAGTEST(modifiers, kCmdModifier) && (keyCode == '.' || keyCode == 'w')) || keyCode == 0x1B) {
				FClicker(RA(NILREF));
				SetFrameSlot(fContext, SYMA(allowKeysThrough), RA(TRUEREF));
				fAutoClose = true;
				RefVar cmd(MakeCommand(aeDropChild, fParent, (OpaqueRef)this));
				gApplication->dispatchCommand(cmd);
				isDismissed = true;
				isUpdated = true;
			}
		}
		break;
	}

	if (inChar != 0 && !isUpdated) {
		// non-cursor key was typed
		CTime timeNow = GetGlobalTime();
		if (ISNIL(fTypeSelectStr) || (timeNow - fLastKeyDownTime) > CTime(fTypeSelectTimeout*20, kMilliseconds)) {	// assume fTypeSelectTimeout is in ticks
			fTypeSelectStr = Clone(RA(emptyString));
		}
		fLastKeyDownTime = timeNow;
		ArrayIndex typeSelectStrLen = RSTRLEN(fTypeSelectStr);
		if (typeSelectStrLen < 20) {
			SetLength(fTypeSelectStr, (typeSelectStrLen + 1)*sizeof(UniChar));
			UniChar * lastCh = GetUString(fTypeSelectStr) + typeSelectStrLen;
			lastCh[0] = inChar;
			lastCh[1] = 0;
			++typeSelectStrLen;
		}
		CRichString typeSelectStr(fTypeSelectStr);
		ArrayIndex count = Length(fItems);
		ArrayIndex index = fPicked.index;
		int cmp = kItemLessThanCriteria;
		if (index != kIndexNotFound) {
			RefVar itemNoText(getItemNoText(index));
			if (NOTNIL(itemNoText)) {
				// try to match current selection
				CRichString itemStr(itemNoText);
				cmp = itemStr.compareSubStringCommon(typeSelectStr, 0, MIN(itemStr.length(), typeSelectStrLen));
			}
		}
		if (cmp != kItemEqualCriteria) {
			// try all items
			for (index = 0; index < count; ++index) {
				RefVar itemNoText(getItemNoText(index));
				if (NOTNIL(itemNoText)) {
					CRichString itemStr(itemNoText);
					cmp = itemStr.compareSubStringCommon(typeSelectStr, 0, MIN(itemStr.length(), typeSelectStrLen));
					if (cmp == kItemEqualCriteria) {
						break;
					}
				}
			}
		}
		if (cmp == kItemEqualCriteria) {
			fPicked.index = index;
			fPicked.isGrid = false;
		}
	}

	if (!isDismissed && fPicked.index != kIndexNotFound) {
		// scroll the picked item into view
		Rect itemRect;
		getGridItemRect(&fPicked, &itemRect);
		if (viewBounds.bottom < itemRect.top) {
			for ( ; viewBounds.bottom < itemRect.top;  getGridItemRect(&fPicked, &itemRect)) {
				scroll(SYMA(down), false);
			}
		} else {
			for ( ; itemRect.bottom < viewBounds.top; getGridItemRect(&fPicked, &itemRect)) {
				scroll(SYMA(up), false);
			}
		}
		NSSend(fContext, SYMA(SetScrollers));
		dirty();
		gRootView->update();
	}
	// whatever it was, we handled it
	return true;
}


/* -----------------------------------------------------------------------------
	Create the list of cmd-key info foreach pick item.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CPickView::getKeyCommandInfo(void)
{
	// init the list and space reqd to display cmd-key char
	fCommandKeys = NILREF;
	fCommandKeySpace = 0;
	//sp-1C
	RefVar item;
	RefVar cmdKey;
	RefVar scriptContext(getProto(SYMA(callbackContext)));
	if (ISNIL(scriptContext)) {
		scriptContext = fContext;
	}
	CView * vwContext = GetView(scriptContext);
	for (ArrayIndex i = 0, count = Length(fItems); i < count; ++i) {
		item = GetArraySlot(fItems, i);
		if (IsFrame(item)) {
			cmdKey = GetProtoVariable(item, SYMA(keyCommand));
			if (ISNIL(cmdKey)) {
				cmdKey = GetProtoVariable(item, SYMA(keyMessage));
				if (IsSymbol(cmdKey) && vwContext != NULL) {
					cmdKey = MatchKeyMessage(vwContext, cmdKey, 1);
				}
			}
			if (NOTNIL(cmdKey)) {
				if (ISNIL(fCommandKeys)) {
					fCommandKeys = MakeArray(fNumOfItems);
				}
				SetArraySlot(fCommandKeys, i, cmdKey);
				int wd = GetCommandCharWidth(cmdKey, &fTextStyle);
				if (wd > fCommandKeySpace) {
					fCommandKeySpace = wd;
				}
			}
		}
	}
}


/* -----------------------------------------------------------------------------
	Return the cmd-key for this item.
	Args:		index				index into list of items
	Return:	Ref				nil => no cmd-key specified
----------------------------------------------------------------------------- */

Ref
CPickView::getKeyCommand(ArrayIndex index)
{
	if (NOTNIL(fCommandKeys)) {
		return GetArraySlot(fCommandKeys, index);
	}
	return NILREF;
}


/* -----------------------------------------------------------------------------
	Return the width of the cmd-key for this item.
	Args:		index				index into list of items
	Return:	width, 0 => no cmd-key for this item or keyboard not connected
----------------------------------------------------------------------------- */

int
CPickView::getKeyCommandModifierWidth(ArrayIndex index)
{
	if (gRootView->commandKeyboardConnected() && NOTNIL(fCommandKeys)) {
		RefVar cmdKey(GetArraySlot(fCommandKeys, index));
		if (NOTNIL(cmdKey)) {
			return GetModifiersWidth(cmdKey);
		}
	}
	return 0;
}


/* -----------------------------------------------------------------------------
	Return an array showing by how much the list overflows the view bounds.
	Args:		--
	Return:	array object	0:	negative => what’s scrolled off the top
									1:	positive => what’s scrolled off the bottom
----------------------------------------------------------------------------- */

Ref
CPickView::getOverflows(void)
{
	Point origin = childOrigin();
	RefVar overflows(MakeArray(2));
	SetArraySlot(overflows, 0, MAKEINT(origin.v));
	SetArraySlot(overflows, 1, MAKEINT(fItemBottoms[fNumOfItems-1] - RectGetHeight(viewBounds)));
	return overflows;
}


/* -----------------------------------------------------------------------------
	Scroll the picker by the view height in the direction specified.
	Args:		inDirection			'up or 'down
				inupdate
	Return:	--
----------------------------------------------------------------------------- */

void
CPickView::scroll(RefArg inDirection, bool inUpdate)
{
	Point origin = childOrigin();
	int listHeight = fItemBottoms[fNumOfItems - 1];
	int viewHeight = RectGetHeight(viewBounds);
	int itemHeight;
	if (EQ(inDirection, SYMA(up))) {
		origin.v += viewHeight;
		if (origin.v > 0) {
			// can’t scroll any more
			origin.v = 0;
		} else {
			// we are scrolled down -- origin is negative
			// find index of new top item
			int index;
			for (index = fNumOfItems - 1; index >= 0; --index) {
				if (origin.v + fItemBottoms[index] <= 0) {
					break;
				}
			}
			itemHeight = fItemBottoms[index + 1] - fItemBottoms[index];
			if (itemHeight < viewHeight) {
				// align to top of item
				origin.v = -fItemBottoms[index];
			}
		}
	} else if (EQ(inDirection, SYMA(down))) {
		origin.v -= viewHeight;
		int scrollLimit = viewHeight - listHeight;	// scrollLimit is negative
		if (scrollLimit >= origin.v) {
			// can’t scroll any more
			origin.v = scrollLimit;
		} else {
			// we are scrolled up -- origin is positive
			// find index of new top item
			int index;
			for (index = 0; index < fNumOfItems; ++index) {
				if (origin.v + fItemBottoms[index] >= 0) {
					break;
				}
			}
			if (index != 0) {
				--index;
			}
			itemHeight = fItemBottoms[index + 1] - fItemBottoms[index];
			if (itemHeight < viewHeight) {
				// align to top of item
				origin.v = -fItemBottoms[index];
			}
		}
	}

	if (inUpdate) {
		fPicked.index = kIndexNotFound;
	}
	setOrigin(origin);
}


/* -----------------------------------------------------------------------------
	viewSetupFormScript
	Set up everything we need.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CPickView::setupForm(void)
{
	// release resources from previous incarnation
	if (fItemBottoms != NULL) {
		free(fItemBottoms), fItemBottoms = NULL;
	}
	if (fItemInfo != NULL) {
		free(fItemInfo), fItemInfo = NULL;
	}
	if (fItemGrid != NULL) {
		for (ArrayIndex i = 0; i < fNumOfItems; ++i) {
			if (fItemGrid[i] != NULL) {
				delete fItemGrid[i];
			}
		}
		free(fItemGrid), fItemGrid = NULL;
	}

	// call inherited
	CView::setupForm();

//	RefVar sp04(fContext);
//	sp1C = &fItems;	// items
//	sp18 = SYMA(viewFont)
//	sp14 = &fTextStyle

	fItems = GetProtoVariable(fContext, SYMA(pickItems));
	fNumOfItems = Length(fItems);
	fTypeSelectTimeout = RINT(GetPreference(SYMA(typeSelectTimeout)));
	fPickTextItemHeight = GetProtoVariable(fContext, SYMA(pickTextItemHeight));
	CreateTextStyleRecord(GetVariable(fContext, SYMA(viewFont)), &fTextStyle);
	GetStyleFontInfo(&fTextStyle, &f44);
	fAutoClose = NOTNIL(GetProtoVariable(fContext, SYMA(pickAutoClose)));

	//sp-0C -10 -1C -38 -40
	bool pickItemsMarkable = NOTNIL(GetProtoVariable(fContext, SYMA(pickItemsMarkable)));	//sp08
	int leftMargin = RINT(GetProtoVariable(fContext, SYMA(pickLeftMargin)));	//sp04
	int markWd = RINT(GetProtoVariable(fContext, SYMA(pickMarkWidth)));	//sp00
	fRightMargin = RINT(GetProtoVariable(fContext, SYMA(pickRightMargin)));
	fTopMargin = RINT(GetProtoVariable(fContext, SYMA(pickTopMargin)));
	fBottomMargin = RINT(GetProtoVariable(fContext, SYMA(pickBottomMargin)));

	//sp-04 -10 -2C -34
	GridInfo * grid;	//sp30
	fItemBottoms = (int *)malloc(fNumOfItems * sizeof(int));
	fItemInfo = (ItemInfo *)malloc(fNumOfItems * sizeof(ItemInfo));
	fItemGrid = (GridInfo **)malloc(fNumOfItems * sizeof(GridInfo*));	// should calloc?
	if (fItemBottoms == NULL || fItemInfo == NULL || fItemGrid == NULL) {
		if (fItemBottoms == NULL) {
			free(fItemBottoms);
		}
		if (fItemInfo == NULL) {
			free(fItemInfo);
		}
		if (fItemGrid == NULL) {
			free(fItemGrid);
		}
		OutOfMemory();
	}

	// build cmd-key info list foreach pick item
	getKeyCommandInfo();

	//sp-0C -28 -30
	int spx08 = RINT(getProto(SYMA(pickMaxWidth))) - fCommandKeySpace;
	//sp2C = gVarFrame
	RefVar spx04(GetGlobalVar(SYMA(_hiliteMenuItem)));
	//sp28 = gRootView
	bool spx00 = gRootView->commandKeyboardConnected() && NOTNIL(spx04) && fNumOfItems != 0;
	fPicked.index = kIndexNotFound;

	//sp-1C -24
	RefVar item;			//sp18
	RefVar slot;			//sp14
	PickStuff picked;		//sp04
	int fixedHt = -1;		//sp00
	int indent = -1;		//r7
	int maxItemWd = 0;	//r8
	int listHt = 0;		//r6
	//sp40 = &g00377388
	for (ArrayIndex i = 0; i < fNumOfItems; ++i) {
		//sp-08
		int r10 = 0;	//
		bool isPickable;	//spm04
		UniChar markCh;	//spm00
		picked.index = i;
		grid = NULL;
		item = GetArraySlot(fItems, i);

		// set up fixed indent and height if specified
		int itemIndent = getDisplayIndent(item);
		if (itemIndent > 0) {
			indent = itemIndent;
		}
		int itemHeight = getDisplayFixedHeight(item);
		if (itemHeight >= 0) {
			fixedHt = itemHeight;
		}

		// set up mark character
		item = getDisplayItem(i, &isPickable, &markCh);
		if (markCh != 0) {
			fHasMark = true;
		}
		setItemFlags(&picked, isPickable, markCh);

		int itemWd = getKeyCommandModifierWidth(i);	// r9	or indent?
		int itemHt;	// r10? some confusion here

		if (IsString(item)) {
			// it’s plain text, possibly with an icon
			//sp-50
			//sp48 = sp98/g00377388 zeroRect?
			TextBoundsInfo txBounds;	//sp2C
			CRichString str(item);
			ArrayIndex strLen = str.length();
			MeasureRichString(str, 0, strLen, &fTextStyle, gZeroFPoint, NULL, &txBounds);
			itemWd += txBounds.width;
			r10 = leftMargin - getKeyCommandModifierWidth(i);
			//sp-04
			if (itemWd > r10) {
				//sp-18
				RefVar styledStr(FStyledStrTruncate(RA(NILREF), Clone(item), MAKEINT(r10), getVar(SYMA(viewFont))));
				strLen = -RINT(FStrLen(RA(NILREF), styledStr));	// indicate string overflows space available by making length negative
				itemWd = leftMargin;
			}
			setItemLength(&picked, strLen);
			itemHt = fPickTextItemHeight;
			RefVar icon(getDisplayIcon(GetArraySlot(fItems, i)));
			if (NOTNIL(icon)) {
				// yup, item has an icon too
				//sp-08
				Rect iconBounds;
				if (!FromObject(GetFrameSlot(icon, SYMA(bounds)), &iconBounds)) {
					ThrowMsg("bad pictBounds frame for icon");
				}
				if (indent >= 0) {
					itemWd += indent;
				} else {
					itemWd += RectGetWidth(iconBounds) + 2;
				}
				// ensure item height is tall enough for this icon
				int iconHt = fTopMargin + RectGetHeight(iconBounds) + fBottomMargin;
				if (itemHt < iconHt) {
					itemHt = iconHt;
				}
			} else {
				if (indent >= 0) {
					itemWd += indent;
				}
			}

		} else if (IsSymbol(item)) {
			// it’s a separator
			if (EQ(item, SYMA(pickSeparator)) || EQ(item, SYMA(pickSolidSeparator))) {
				itemWd = 0;
				itemHt = kPickSeparatorHeight;
			} else {
				ThrowMsg("unsupported symbol");
			}

		} else if (IsFrame(item)) {
			// it’s a graphic of some kind
			if (FrameHasSlot(item, SYMA(bits)) || FrameHasSlot(item, SYMA(colorData)) || FrameHasSlot(item, SYMA(picture))) {
				Rect pictBounds;
				if (!FromObject(GetFrameSlot(item, SYMA(bounds)), &pictBounds)) {
					ThrowMsg("bad pictBounds frame");
				}
				itemWd = RectGetWidth(pictBounds);
				itemHt = fTopMargin + RectGetHeight(pictBounds) + fBottomMargin;
				grid = getGridInfo(item, &pictBounds);
			} else if (FrameHasSlot(item, SYMA(strokeList))) {
				Rect strokeBounds;
				if (!FromObject(GetFrameSlot(item, SYMA(strokeList)), &strokeBounds)) {
					ThrowMsg("bad strokeBounds frame");
				}
				itemWd = RectGetWidth(strokeBounds);
				int strokeHt = RectGetHeight(strokeBounds);
				if (strokeHt > kMaxPickItemHeight) {
					// limit height to fit menu item -- scale stroke
					itemWd = itemWd * kMaxPickItemHeight / strokeHt;
					strokeHt = kMaxPickItemHeight;
				}
				itemHt = fTopMargin + strokeHt + fBottomMargin;
			}
		}

//00185CCC
		if (fixedHt > 0 && !IsSymbol(item)) {
			itemHt = fixedHt;
		}
		listHt += itemHt;
		fItemBottoms[i] = listHt;
		fItemGrid[i] = grid;
		if (itemWd > maxItemWd) {
			maxItemWd = itemWd;
		}

		if (spx00 && isPickable) {
			fPicked.index = i;
			if (grid != NULL) {
				fPicked.isGrid = true;
				fPicked.gridX = 0;
				fPicked.gridY = 0;
			} else {
				fPicked.isGrid = false;
			}
			spx00 = false;
		}
	}

	if (gRootView->commandKeyboardConnected() && NOTNIL(fCommandKeys)) {
		maxItemWd += fCommandKeySpace + 2;
	}
	//sp-04
	if (fHasMark && !pickItemsMarkable) {
		SetFrameSlot(fContext, SYMA(pickItemsMarkable), RA(TRUEREF));
	}
	if (fHasMark) {
		fMarkIndent = leftMargin;
		fLeftMargin = leftMargin + markWd;
	} else {
		fMarkIndent = leftMargin;
		fLeftMargin = leftMargin;
	}

	int inset = 0;
	slot = GetProtoVariable(fContext, SYMA(viewFormat));
	if (NOTNIL(slot)) {
		int vwFormat = RINT(slot);
		if ((vwFormat & vfFrameMask) != 0) {
			inset = (vwFormat & vfPenMask) >> vfPenShift;
		} else {
			inset = 0;
		}
		inset += (vwFormat & vfInsetMask) >> vfInsetShift;
	}

	//sp-08
	Rect bBox;
	if (!FromObject(GetProtoVariable(fContext, SYMA(bounds)), &bBox)) {
		ThrowMsg("bad bounds frame");
	}
	//sp-10
	Rect screenArea;
	Rect appArea;
	SetRect(&screenArea, 0, 0, gScreenWidth, gScreenHeight);
	GetAppAreaBounds(&appArea);
	//sp-10
	Rect spm08 = PtInRect(*(Point *)&bBox, &appArea) ? appArea : screenArea;
	Rect availableBounds = spm08;	//spm00
	InsetRect(&availableBounds, inset, inset);

	slot = GetFrameSlot(fContext, SYMA(info));	// popup code sets the 'info slot with the popper-upper view
	bool isPopup = NOTNIL(slot);	//r9
	int availableHt = RectGetHeight(availableBounds);
	int pickerHt = (listHt < availableHt)? listHt : availableHt;
	if (pickerHt < listHt) {
		// we need scrollers
		RefVar scrollers(GetProtoVariable(fContext, SYMA(scrollers)));
		if (NOTNIL(scrollers)) {
			// we got ’em
			SetFrameSlot(scrollers, SYMA(viewFlags), MAKEINT(vVisible|vReadOnly));
			// increase margin to allow for arrows
			fRightMargin += 19;
		}
	}
	int listWd = fRightMargin + maxItemWd + fLeftMargin;
	int availableWd = RectGetWidth(availableBounds);
	int pickerWd = (listWd < availableWd)? listWd : availableWd;

	bool r9 = false;
	if (isPopup) {
//00186098
		//sp-10
		Rect sp08 = bBox;
		Rect vwBounds;
		vwBounds.top = vwBounds.bottom = -32767;
		CView * aView = (CView *)RefToAddress(slot);
		if (aView != NULL) {
			// find the child of the root == app view
			CView * parentView = aView->fParent;
			while (parentView != gRootView) {
				aView = aView->fParent;
				parentView = aView->fParent;
			}
			if ((aView->viewFormat & vfFillMask) != 0) {
				vwBounds = aView->viewBounds;
				SectRect(&spm08, &vwBounds, &vwBounds);
				// adjust within the app view
				r9 = AdjustPopupInRect(bBox, pickerWd, pickerHt, vwBounds, inset);
			}
		}
		if (aView == NULL || !RectContains(&vwBounds, &bBox)) {
			// no app view so adjust within screen
			bBox = sp08;
			r9 = AdjustPopupInRect(bBox, pickerWd, pickerHt, spm08, inset);
		}
	} else {
		if (bBox.left + pickerWd > availableBounds.right) {
			bBox.left = bBox.right - pickerWd;
		} else {
			bBox.right = bBox.left + pickerWd;
		}
		if (bBox.top + pickerHt > availableBounds.bottom && RectGetHeight(availableBounds)/2 < bBox.top) {
			bBox.top = bBox.bottom - pickerHt;
			r9 = true;
		} else {
			bBox.bottom = bBox.top + pickerHt;
		}
	}

	// adjust bounds to fit screen space available
	int dh = 0;
	int dv = 0;
	if (RectGetWidth(bBox) > RectGetWidth(availableBounds)) {
		// shrink bounds to fit screen space available
		bBox.left = availableBounds.left;
		bBox.right = availableBounds.right;
	} else {
		// ensure bounds are within screen space
		if (bBox.left < availableBounds.left) {
			dh = availableBounds.left - bBox.left;
		} else if (bBox.right > availableBounds.right) {
			dh = availableBounds.right - bBox.right;
		}
	}
	if (RectGetHeight(bBox) > RectGetHeight(availableBounds)) {
		// shrink bounds to fit screen space available
		bBox.top = availableBounds.top;
		bBox.bottom = availableBounds.bottom;
	} else {
		// ensure bounds are within screen space
		if (bBox.top < availableBounds.top) {
			dv = availableBounds.top - bBox.top;
		} else if (bBox.bottom > availableBounds.bottom) {
			dv = availableBounds.bottom - bBox.bottom;
		}
	}
	OffsetRect(&bBox, dh, dv);

	// adjust bounds to app area -- may have button bar top/left of screen
	//sp-04
	RefVar displayParms(GetGlobalVar(SYMA(displayParams)));
	dh = -RINT(GetProtoVariable(displayParms, SYMA(appAreaGlobalLeft)));
	dv = -RINT(GetProtoVariable(displayParms, SYMA(appAreaGlobalTop)));
	OffsetRect(&bBox, dh, dv);

	//sp-04
	if (r9) {
		SetFrameSlot(fContext, SYMA(viewEffect), MAKEINT(fxPopUpEffect));
	}
	SetFrameSlot(fContext, SYMA(viewBounds), ToObject(&bBox));
	if (fPicked.index != kIndexNotFound) {
		// an item is picked -- scroll it into view
		//sp-08
		Rect itemRect;
		getGridItemRect(&fPicked, &itemRect);
		while (viewBounds.bottom < itemRect.top) {
			scroll(SYMA(down), false);
			getGridItemRect(&fPicked, &itemRect);
		}
	}
}


#pragma mark -
/* -----------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
----------------------------------------------------------------------------- */

extern "C" {
Ref	FPickViewKeyDown(RefArg inRcvr, RefArg inChar, RefArg inKeyFlags);
Ref	FPickViewGetScrollerValues(RefArg inRcvr);	// originally FPickViewGetScollerValues !
Ref	FPickViewScroll(RefArg inRcvr);
}


Ref
FPickViewKeyDown(RefArg inRcvr, RefArg inChar, RefArg inKeyFlags)
{
	CPickView * view = (CPickView *)GetView(inRcvr);
	if (view == NULL) {
		return TRUEREF;
	}
	return MAKEBOOLEAN(view->handleKeyDown(RCHAR(inChar), RINT(inKeyFlags)));
}


Ref
FPickViewGetScrollerValues(RefArg inRcvr)
{
	CPickView * view = (CPickView *)GetView(inRcvr);
	if (view != NULL) {
		return view->getOverflows();
	}
	return NILREF;
}


Ref
FPickViewScroll(RefArg inRcvr, RefArg inView)
{
	CPickView * view = (CPickView *)GetView(inView);
	if (view != NULL) {
		view->scroll(inView, true);
	}
	return NILREF;
}
