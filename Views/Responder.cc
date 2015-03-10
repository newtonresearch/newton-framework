/*
	File:		Responder.cc

	Contains:	CResponder implementation.

	Written by:	Newton Research Group.
*/

#import "Responder.h"
#include "Objects.h"
#include "Arrays.h"
#include "Globals.h"


/*----------------------------------------------------------------------
	C R e s p o n d e r
----------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clResponder, CResponder, CViewObject)


bool
CResponder::doCommand(RefArg inCmd)
{
	return NO;
}

#pragma mark -

/*----------------------------------------------------------------------
	C o m m a n d s
	
	Commands are messages (frames) sent to CResponders.
----------------------------------------------------------------------*/

/*----------------------------------------------------------------------
	Set and extract the id in a command frame.
	Args:		inCmd		the command frame
				inId		the new id
	Return:	--
----------------------------------------------------------------------*/

void
CommandSetId(RefArg inCmd, ULong inId)
{
	SetFrameSlot(inCmd, SYMA(id), MAKEINT(inId));
}


ULong
CommandId(RefArg inCmd)
{
	return RINT(GetFrameSlot(inCmd, SYMA(id)));
}


/*----------------------------------------------------------------------
	Set and extract the parameter slot in a command frame.
	Args:		inCmd		the command frame
				inArg		the integer value
	Return:	--
----------------------------------------------------------------------*/

void
CommandSetParameter(RefArg inCmd, OpaqueRef inArg)
{
	SetFrameSlot(inCmd, SYMA(parameter), MAKEINT(inArg));
}


OpaqueRef
CommandParameter(RefArg inCmd)
{
	return RINT(GetFrameSlot(inCmd, SYMA(parameter)));
}


/*----------------------------------------------------------------------
	Set and extract the frameParameter slot in a command frame.
	Args:		inCmd		the command frame
				inArg		the value
	Return:	--
----------------------------------------------------------------------*/

void
CommandSetFrameParameter(RefArg inCmd, RefArg inArg)
{
	SetFrameSlot(inCmd, SYMA(frameParameter), inArg);
}


Ref
CommandFrameParameter(RefArg inCmd)
{
	return GetFrameSlot(inCmd, SYMA(frameParameter));
}


/*----------------------------------------------------------------------
	Set a frame in the params array of a command frame.
	Args:		inCmd		the command frame
				inIndex	the index into the command frame’s params slot
							(an array)
				inArg		the parameter to set at that index
	Return:	--
----------------------------------------------------------------------*/

void
CommandSetIndexFrame(RefArg inCmd, ArrayIndex index, RefArg inArg)
{
	RefVar	params(GetFrameSlot(inCmd, SYMA(params)));
	if (ISNIL(params))
	{
		params = MakeArray(0);
		SetFrameSlot(inCmd, SYMA(params), params);
	}
	ArrayIndex	requiredLength = index + 1;
	if (Length(params) < requiredLength)
		SetLength(params, requiredLength);
	SetArraySlot(params, index, inArg);
}


/*----------------------------------------------------------------------
	Set and extract an integer value in the params array of a command
	frame.
	Args:		inCmd		the command frame
				inIndex	the index into the command frame’s params slot
							(an array)
				inArg		the parameter to set at that index
	Return:	--
----------------------------------------------------------------------*/

void
CommandSetIndexParameter(RefArg inCmd, ArrayIndex index, int inArg)
{
	RefVar	params(GetFrameSlot(inCmd, SYMA(params)));
	if (ISNIL(params))
	{
		params = MakeArray(0);
		SetFrameSlot(inCmd, SYMA(params), params);
	}
	ArrayIndex	requiredLength = index + 1;
	if (Length(params) < requiredLength)
		SetLength(params, requiredLength);
	SetArraySlot(params, index, MAKEINT(inArg));
}


int
CommandIndexParameter(RefArg inCmd, ArrayIndex index)
{
	RefVar	params(GetFrameSlot(inCmd, SYMA(params)));
	if (ISNIL(params) || Length(params) < index + 1)
		return 0;
	return RINT(GetArraySlot(params, index));
}


/*----------------------------------------------------------------------
	Set and extract text in a command frame.
	Args:		inCmd		the command frame
				inText	the text
	Return:	--
----------------------------------------------------------------------*/

void
CommandSetText(RefArg inCmd, RefArg inText)
{
	SetFrameSlot(inCmd, SYMA(text), inText);
}


Ref
CommandText(RefArg inCmd)
{
	return GetFrameSlot(inCmd, SYMA(text));
}


/*----------------------------------------------------------------------
	Set and extract the points array in a command frame.
	Args:		inCmd		the command frame
				inPts		the points array
	Return:	--
----------------------------------------------------------------------*/

void
CommandSetPoints(RefArg inCmd, RefArg inPts)
{
	SetFrameSlot(inCmd, SYMA(points), inPts);
}


Ref
CommandPoints(RefArg inCmd)
{
	return GetFrameSlot(inCmd, SYMA(points));
}


/*----------------------------------------------------------------------
	Set and extract the result in a command frame.
	Args:		inCmd		the command frame
				inResult	the result code
	Return:	--
----------------------------------------------------------------------*/

void
CommandSetResult(RefArg inCmd, NewtonErr inResult)
{
	SetFrameSlot(inCmd, SYMA(result), MAKEINT(inResult));
}


NewtonErr
CommandResult(RefArg inCmd)
{
	return RINT(GetFrameSlot(inCmd, SYMA(result)));
}
