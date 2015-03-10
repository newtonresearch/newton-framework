/*
	File:		SoundErrors.h
	
	Contains:	Error codes for Newton sound system.

	Written by:	Newton Research Group, 2007
*/

#if !defined(__SOUNDERRORS_H)
#define __SOUNDERRORS_H 1

#include "NewtonErrors.h"


// ---------------  Sound errors…  -------------

#define kSndErrGeneric					(ERRBASE_SOUND)
#define kSndErrNoMemory					(ERRBASE_SOUND -  1)
#define kSndErrInvalidMessage 		(ERRBASE_SOUND -  2)
#define kSndErrNotPlayed				(ERRBASE_SOUND -  3)
#define kSndErrNoDecompressor 		(ERRBASE_SOUND -  4)
#define kSndErrBufferTooSmall 		(ERRBASE_SOUND -  5)
#define kSndErrPlayerBusy 				(ERRBASE_SOUND -  6)
#define kSndErrRecorderBusy			(ERRBASE_SOUND -  7)
#define kSndErrNoSamples 				(ERRBASE_SOUND -  8)
#define kSndErrBadConfiguration		(ERRBASE_SOUND -  9)
#define kSndErrChannelIsClosed 		(ERRBASE_SOUND - 10)
#define kSndErrCancelled				(ERRBASE_SOUND - 11)
#define kSndErrNoVolume					(ERRBASE_SOUND - 12)


#endif	/* __SOUNDERRORS_H */
