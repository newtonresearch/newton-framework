/*
	File:		Interrupt.h

	Contains:	Interrupt handler declarations.

	Written by:	Newton Research Group
*/

#if !defined(__INTERRUPT_H)
#define	__INTERRUPT_H 1

#include "NewtonTypes.h"


// Hardware registers

extern ULong	g0F181000;	// RW? real-time clock (seconds since 01/01/1904)
extern ULong	g0F181400;  // W  real-time clock alarm (seconds? minutes?)
extern ULong	g0F181800;  // R  system clock

extern ULong	g0F182400;	// R  tablet interrupt
extern ULong	g0F182800;	// W  system time of next timer interrupt
extern ULong	g0F182C00;	// W  system time of next scheduler interrupt
extern ULong	g0F183000;  // |
extern ULong	g0F183400;  // |
extern ULong	g0F183800;  // |  (Voyager match registers)
extern ULong	g0F183C00;
extern ULong	g0F184000;	// W	timer interrupt enable
extern ULong	g0F184400;
extern ULong	g0F184800;
extern ULong	g0F184C00;


// Interrupt control structure

typedef NewtonErr (*NewtonInterruptHandler)(void *);


struct InterruptPriority
{
	long	x00;
	long	x04;
};


enum x08bits
{
//	kIntIsAdditional	= 0x10,
//	kIntIsRegistered	= 0x40,
	kIntIsEnabled		= 0x80
//	kIntIsMasked		= 0x100,
};


struct InterruptObject
{
	ULong					x00;
	int32_t				x04;
	ULong					x08;			// +08	0x80 -> enabled
	InterruptObject *	next;			// +0C	next interrupt in queue
	NewtonInterruptHandler	handler;		// +10
	InterruptObject *	queue;		// +14	queue this interrupt belongs to
	int32_t				x18;
	short					x1C;			// priority?
	short					x1E;			// |
};


extern InterruptObject *	gTimerIntObj;		// 0C100E68
extern InterruptObject *	gSchedulerIntObj;	// 0C100E6C
extern InterruptObject *	gPCMCIA0IntObj;	// 0C100E70
extern InterruptObject *	gPCMCIA1IntObj;	// 0C100E74


InterruptObject *	RegisterInterrupt(ULong inArg1, void * inArg2, NewtonInterruptHandler inHandler, ULong inArg4);
void					QuickEnableInterrupt(InterruptObject * interrupt);
NewtonErr			EnableInterrupt(InterruptObject * interrupt, ULong inBits);
NewtonErr			DisableInterrupt(InterruptObject * interrupt);
NewtonErr			ClearInterrupt(InterruptObject * interrupt);


#endif	/* __INTERRUPT_H */
