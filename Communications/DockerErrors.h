/*
	File:		DockerErrors.h
	
	Contains:	Error codes for the Docker.

	Written by:	Newton Research Group, 2012.
*/

#if !defined(__DOCKERERRORS_H)
#define __DOCKERERRORS_H 1

#include "NewtonErrors.h"


#define kDockErrBadStoreSignature				(ERRBASE_DOCKER -  1)
#define kDockErrBadEntry							(ERRBASE_DOCKER -  2)
#define kDockErrAborted								(ERRBASE_DOCKER -  3)
#define kDockErrBadQuery							(ERRBASE_DOCKER -  4)
#define kDockErrReadEntryError					(ERRBASE_DOCKER -  5)
#define kDockErrBadCurrentSoup					(ERRBASE_DOCKER -  6)
#define kDockErrBadCommandLength					(ERRBASE_DOCKER -  7)
#define kDockErrEntryNotFound						(ERRBASE_DOCKER -  8)
#define kDockErrBadConnection						(ERRBASE_DOCKER -  9)
#define kDockErrFileNotFound						(ERRBASE_DOCKER - 10)
#define kDockErrIncompatibleProtocol			(ERRBASE_DOCKER - 11)
#define kDockErrProtocolError						(ERRBASE_DOCKER - 12)
#define kDockErrDockingCanceled					(ERRBASE_DOCKER - 13)
#define kDockErrStoreNotFound						(ERRBASE_DOCKER - 14)
#define kDockErrSoupNotFound						(ERRBASE_DOCKER - 15)
#define kDockErrBadHeader							(ERRBASE_DOCKER - 16)
#define kDockErrOutOfMemory						(ERRBASE_DOCKER - 17)
#define kDockErrNewtonVersionTooNew				(ERRBASE_DOCKER - 18)
#define kDockErrPackageCantLoad					(ERRBASE_DOCKER - 19)
#define kDockErrProtocolExtAlreadyRegistered	(ERRBASE_DOCKER - 20)
#define kDockErrRemoteImportError 				(ERRBASE_DOCKER - 21)
#define kDockErrBadPasswordError					(ERRBASE_DOCKER - 22)
#define kDockErrRetryPW								(ERRBASE_DOCKER - 23)
#define kDockErrIdleTooLong						(ERRBASE_DOCKER - 24)
#define kDockErrOutOfPower							(ERRBASE_DOCKER - 25)
#define kDockErrBadCursor							(ERRBASE_DOCKER - 26)
#define kDockErrAlreadyBusy						(ERRBASE_DOCKER - 27)
#define kDockErrDesktopError						(ERRBASE_DOCKER - 28)
#define kDockErrCantConnectToModem				(ERRBASE_DOCKER - 29)
#define kDockErrDisconnected						(ERRBASE_DOCKER - 30)
#define kDockErrAccessDenied						(ERRBASE_DOCKER - 31)

#endif	/* __DOCKERERRORS_H */
