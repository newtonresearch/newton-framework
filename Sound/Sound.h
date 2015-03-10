/*
	File:		Sound.h

	Contains:	Sound definition.

	Written by:	Newton Research Group.
*/

#if !defined(__SOUND_H)
#define __SOUND_H 1

#include "Objects.h"

/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

void	InitSound(void);
void	PlaySound(RefArg inContext, RefArg inSound);

extern "C" {
Ref	FClicker(RefArg inRcvr);
Ref	FPlaySound(RefArg inContext, RefArg inSound);
Ref	FPlaySoundIrregardless(RefArg inContext, RefArg inSound);
}

#endif	/* __SOUND_H */
