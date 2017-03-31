/*
	File:		Preference.cc

	Contains:	Preference implementation.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "Globals.h"
#include "Lookup.h"
#include "Preference.h"
#include "ROMResources.h"


Ref
GetPreference(RefArg inTag)
{
	RefVar	config(GetGlobalVar(SYMA(userConfiguration)));
	return GetProtoVariable(config, inTag);
}


void
SetPreference(RefArg inTag, RefArg inValue)
{
	RefVar	config(GetGlobalVar(SYMA(userConfiguration)));
	SetFrameSlot(config, inTag, inValue);
}



