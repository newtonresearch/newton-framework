/*
	File:		Preference.h

	Contains:	Preference declaration.

	Written by:	Newton Research Group.
*/

#if !defined(__PREFERENCE_H)
#define __PREFERENCE_H 1

#if defined(__cplusplus)
extern "C" {
#endif

Ref	GetPreference(RefArg inTag);
void	SetPreference(RefArg inTag, RefArg inValue);

#if defined(__cplusplus)
}
#endif

#endif	/* __PREFERENCE_H */
