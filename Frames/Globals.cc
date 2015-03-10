/*
	File:		Globals.c

	Contains:	Global object definitions.

	Written by:	Newton Research Group.
*/

#include "Objects.h"

/*----------------------------------------------------------------------
	G l o b a l s
------------------------------------------------------------------------
	This is a GC root so it will be updated automatically.
----------------------------------------------------------------------*/

Ref	gVarFrame;
Ref *	RSgVarFrame = &gVarFrame;

