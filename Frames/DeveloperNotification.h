/*
	File:		DeveloperNotification.h

	Contains:	Exception handling declarations.
					Used by the interpreter.

	Written by:	Newton Research Group, 2013.
*/

#if !defined(__DEVELOPERNOTIFICATION_H)
#define __DEVELOPERNOTIFICATION_H 1


extern bool	DeveloperNotified(Exception * inException);
extern void	RememberDeveloperNotified(Exception * inException);
extern void	ForgetDeveloperNotified(ExceptionName inName);


#endif	/* __DEVELOPERNOTIFICATION_H */
