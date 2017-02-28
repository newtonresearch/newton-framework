/*
	File:		NewtonKit.h

	Contains:	Public interface to the Newton framework.

	Written by:	Newton Research Group, 2005.
*/

#if !defined(__NEWTONKIT_H)
#define __NEWTONKIT_H 1

#if __LITTLE_ENDIAN__
#define hasByteSwapping 1
#endif

#define BYTE_SWAP_SIZE(n) (((n << 16) & 0x00FF0000) | (n & 0x0000FF00) | ((n >> 16) & 0x000000FF))
#if defined(hasByteSwapping)
#define CANONICAL_SIZE BYTE_SWAP_SIZE
#else
#define CANONICAL_SIZE(n) (n)
#endif


#include "NewtonTypes.h"
#include "NewtonWidgets.h"
#include "NewtonDebug.h"
#include "OS/ROMExtension.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#endif	/* __NEWTONKIT_H */
