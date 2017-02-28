/*
	File:		Power.h

	Contains:	Power management.

	Written by:	Newton Research Group, 2009.
*/

#if !defined(__POWER_H)
#define __POWER_H 1

extern void IOPowerOn(ULong inSelector);
extern void IOPowerOff(ULong inSelector);
extern void IOPowerOffAll(void);

#endif	/* __POWER_H */
