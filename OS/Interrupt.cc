/*
	File:		Interrupt.cc

	Contains:	Interrupt handlers.

	Written by:	Newton Research Group
*/

#include "Newton.h"
#include "Interrupt.h"
#include "SharedTypes.h"
#include "UserGlobals.h"
#include "OSErrors.h"

/*------------------------------------------------------------------------------
	H a r d w a r e   R e g i s t e r s
------------------------------------------------------------------------------*/

ULong	g0F181000;	// RW? real-time clock (seconds since 01/01/1904)
ULong	g0F181400;  // W  real-time clock alarm (seconds? minutes?)
ULong	g0F181800;  // R  system clock

ULong	g0F182400;	// R	ACMOD
ULong	g0F182800;	// W  system time of next timer interrupt
ULong	g0F182C00;	// W	system time of next scheduler interrupt

ULong	g0F183000;
ULong	g0F183400;  // R = 0F5FEBA4 interrupt mask
ULong	g0F183800;
ULong	g0F183C00;
ULong	g0F184000;	// W	timer interrupt enable
ULong	g0F184400;
ULong	g0F184800;
ULong	g0F184C00;

/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

InterruptObject *	gInterruptFIQHead;		// +00	0C100E3C
InterruptObject *	gInterruptIRQHead;		// +04	0C100E40
int					gFIQSavedStackHead;		// +08	0C100E44
int					gIRQSavedStackHead;		// +0C	0C100E48
ULong					gFIQInterruptOverhead;	// +10	0C100E4C
ULong					gIRQInterruptOverhead;	// +14	0C100E50
ULong					gIntMaskShadowReg;		// +18	0C100E54
int					gAtomicFIQNestCountFast = 0;		//	0C100E58		these are set in InitCGlobals()
int					gAtomicIRQNestCountFast = 0;		// 0C100E5C

int					gAtomicNestCount = 0;				// 0C100FE8
ULong					gOldStatus;								// 0C100FEC

int					gAtomicFIQNestCount = 0;			// 0C100FF0
ULong					gOldFIQStatus;							// 0C100FF4

InterruptObject *	gTimerIntObj;
InterruptObject *	gSchedulerIntObj;
InterruptObject *	gPCMCIA0IntObj;
InterruptObject *	gPCMCIA1IntObj;


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern void			TimerInterruptHandler(void * inQueue);
extern void			PreemptiveTimerInterruptHandler(void * inQueue);
extern void			DispatchTricInterrupt(void * inQueue);
		 NewtonErr	HLevel2Handler(void * inQueue);

InterruptObject *	RegisterInterrupt(ULong inSourceMask, void * inQueue, NewtonInterruptHandler inHandler, ULong inFlags);
NewtonErr			RegisterInterrupt(InterruptObject ** ioQueue, ULong inSourceMask, void * inQueue, NewtonInterruptHandler inHandler, ULong inFlags);
NewtonErr			RegisterAdditionalInterrupt(InterruptObject ** ioQueue, ULong inSourceMask, void * inQueue, NewtonInterruptHandler inHandler, ULong inFlags);
NewtonErr			PackInterruptObject(InterruptObject * interrupt, ULong inSourceMask, void * inQueue, NewtonInterruptHandler inHandler);
NewtonErr			RegisterInterruptObject(InterruptObject * interrupt);
NewtonErr			RegisterInterruptObject(InterruptObject * interrupt, InterruptObject ** inArg2);

NewtonErr			AddInterrupt(InterruptObject ** ioQueue, InterruptObject * interrupt);
NewtonErr			RemoveInterrupt(InterruptObject * interrupt);
NewtonErr			SubtractInterrupt(InterruptObject ** ioQueue, InterruptObject * interrupt);
InterruptObject *	FindInterrupt(InterruptObject * interrupt, ULong inSourceMask);
InterruptObject *	WhichHandlerPresent(ULong inArg1);
InterruptPriority	GetInterruptPriority(InterruptObject * interrupt);
NewtonErr			ChangeInterruptPriority(InterruptObject * interrupt, InterruptPriority newPriority);

void					QuickEnableInterrupt(InterruptObject * interrupt);
NewtonErr			EnableInterrupt(InterruptObject * interrupt, ULong bits);
NewtonErr			DisableInterrupt(InterruptObject * interrupt);
void					DisableAllInterrupts(void);
NewtonErr			CheckEnables(InterruptObject * interrupt, ULong bits);
NewtonErr			UnmaskInterrupt(InterruptObject * interrupt);
NewtonErr			ClearInterrupt(InterruptObject * interrupt);


/*------------------------------------------------------------------------------
	Initialize interrupt hardware.
	Read the interrupt mask register into a shadow variable.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
HInitInterrupts(void)
{
	Heap savedHeap = GetHeap();
	SetHeap(NULL);
	gInterruptFIQHead = gInterruptIRQHead = NULL;
	gIntMaskShadowReg = g0F183400;
	SetHeap(savedHeap);
}


/*------------------------------------------------------------------------------
	Initialize interrupt tables.
	Register handlers for the timer, the scheduler and PC cards.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
InitInterruptTables(void)
{
// Set up timer interrupt
	RegisterInterrupt(&gTimerIntObj, 0x0020, NULL, (NewtonInterruptHandler) TimerInterruptHandler, 0x0001);

// Set up scheduler interrupt
	RegisterInterrupt(&gSchedulerIntObj, 0x0040, NULL, (NewtonInterruptHandler) PreemptiveTimerInterruptHandler, 0x0001);
	EnableInterrupt(gSchedulerIntObj, 1);

#if defined(correct)
// Set up card interrupts
	InterruptPriority thePriority;
	RegisterInterrupt(&gPCMCIA0IntObj, 0x00010000, 0x00010000, (NewtonInterruptHandler) DispatchTricInterrupt, 0x0401);
	thePriority = GetInterruptPriority(gPCMCIA0IntObj);
	thePriority.x00 = 1;
	ChangeInterruptPriority(gPCMCIA0IntObj, thePriority);

	RegisterInterrupt(&gPCMCIA1IntObj, 0x02000000, 0x02000000, (NewtonInterruptHandler) DispatchTricInterrupt, 0x0401);
	thePriority = GetInterruptPriority(gPCMCIA1IntObj);
	thePriority.x00 = 0;
	ChangeInterruptPriority(gPCMCIA1IntObj, thePriority);
#endif
}


/*------------------------------------------------------------------------------
	Register an interrupt.
	Args:		inSourceMask
				inQueue
				inHandler
				inFlags
	Return:	error code
------------------------------------------------------------------------------*/

InterruptObject *
RegisterInterrupt(ULong inSourceMask, void * inQueue, NewtonInterruptHandler inHandler, ULong inFlags)
{
	NewtonErr			err;
	InterruptObject *	obj;
	if ((err = RegisterInterrupt(&obj, inSourceMask, inQueue, inHandler, inFlags)) == kIntErrInterruptAlreadyExists)
		err = RegisterAdditionalInterrupt(&obj, inSourceMask, inQueue, inHandler, inFlags);
	if (err)
		obj = NULL;
	return obj;
}


/*------------------------------------------------------------------------------
	Register an interrupt - create the handler queue with it.
	Args:		outIntObj
				inSourceMask
				inQueue
				inHandler
				inFlags
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
RegisterInterrupt(InterruptObject ** outIntObj, ULong inSourceMask, void * inQueue, NewtonInterruptHandler inHandler, ULong inFlags)
{
	NewtonErr err = noErr;
	Heap savedHeap = GetHeap();
	SetHeap(NULL);
	XTRY
	{
		*outIntObj = NULL;

		// create new interrupt object
		InterruptObject * interrupt = new InterruptObject;
		XFAILIF(interrupt == NULL, err = kIntErrCouldNotCreateInterrupt;)

		// pack new interrupt object with parms given
		XFAILIF(err = PackInterruptObject(interrupt, inSourceMask, inQueue, inHandler), delete interrupt;)
		// sanity check interrupt enables
		XFAILIF(err = CheckEnables(interrupt, inFlags), delete interrupt;)

		EnterFIQAtomic();
		// add to interrupt handler queue
		XFAILIF(err = RegisterInterruptObject(interrupt), ExitFIQAtomic(); delete interrupt;)

		// start out with interrupt disabled
		DisableInterrupt(interrupt);
		ClearInterrupt(interrupt);
		UnmaskInterrupt(interrupt);

		interrupt->x08 = (interrupt->x08 & ~0x0000040F)		// scrub relevant bits
							| (inFlags & 0x0000040F);				// insert required bits

		ExitFIQAtomic();
		*outIntObj = interrupt;
	}
	XENDTRY;
	SetHeap(savedHeap);
	return err;
}


/*------------------------------------------------------------------------------
	Register an additional interrupt - create the handler queue with it.
	Args:		ioQueue
				inSourceMask
				inQueue
				inHandler
				inFlags
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
RegisterAdditionalInterrupt(InterruptObject ** ioQueue, ULong inSourceMask, void * inQueue, NewtonInterruptHandler inHandler, ULong inFlags)
{
	NewtonErr err;
	Heap savedHeap = GetHeap();
	SetHeap(NULL);
	XTRY
	{
		*ioQueue = NULL;

		InterruptObject * interrupt = new InterruptObject;
		XFAILIF(interrupt == NULL, err = kIntErrCouldNotCreateInterrupt;)

		XFAILIF(err = PackInterruptObject(interrupt, inSourceMask, inQueue, inHandler), delete interrupt;)
		XFAILIF(err = CheckEnables(interrupt, inFlags), delete interrupt;)

		InterruptObject * additionalInterrupt = new InterruptObject;
		XFAILIF(additionalInterrupt == NULL, delete interrupt; err = kIntErrCouldNotCreateInterrupt;)

		XFAILIF(err = PackInterruptObject(additionalInterrupt, interrupt->x00, 0, HLevel2Handler), delete interrupt; delete additionalInterrupt;)

		additionalInterrupt->x08 |= 0x10;

		EnterFIQAtomic();
		XFAILIF(err = RegisterInterruptObject(interrupt, &additionalInterrupt), ExitFIQAtomic(); delete interrupt; delete additionalInterrupt;)

		DisableInterrupt(interrupt);
		ClearInterrupt(interrupt);
		UnmaskInterrupt(interrupt);

		interrupt->x08 = (interrupt->x08 & ~0x0000040F)		// scrub relevant bits
							| (inFlags & 0x0000040F);				// insert required bits

		ExitFIQAtomic();
		delete additionalInterrupt;

		*ioQueue = interrupt;
	}
	XENDTRY;
	SetHeap(savedHeap);
	return err;
}


/*------------------------------------------------------------------------------
	Handle the additional (level 2) interrupt queue.
	Args:		interrupt		interrupt object containing level 2 items
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
HLevel2Handler(void * inQueue)
{
	int r5 = 0;
	InterruptObject * interrupt;
	// iterate over handler queue
	for (interrupt = (InterruptObject *)inQueue; interrupt != NULL; interrupt = interrupt->next)
	{
		if ((interrupt->x08 & kIntIsEnabled) != 0)
			// interrupt is enabled -- handle it
			r5 |= interrupt->handler(interrupt->queue);
	}
	return r5;
}


/*------------------------------------------------------------------------------
	Complete an interrupt object record.
	Args:		interrupt
				inSourceMask
				inQueue
				inHandler
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
PackInterruptObject(InterruptObject * interrupt, ULong inSourceMask, void * inQueue, NewtonInterruptHandler inHandler)
{
	interrupt->x00 = inSourceMask;
	interrupt->x08 = 0;
	interrupt->queue = (InterruptObject *)inQueue;
	interrupt->handler = inHandler;
	interrupt->x1C = 0;
	interrupt->x1E = 1;
	interrupt->next = NULL;
	interrupt->x04 = 0;
	interrupt->x18 = 0;
	return noErr;
}


/*------------------------------------------------------------------------------
	Register an interrupt object with the appropriate (FIQ|IRQ) queue.
	Args:		interrupt
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
RegisterInterruptObject(InterruptObject * interrupt)
{
	interrupt->next = NULL;
	if (WhichHandlerPresent(interrupt->x00) != NULL)
		return kIntErrInterruptAlreadyExists;
	return AddInterrupt((interrupt->x1C == 1) ? &gInterruptFIQHead : &gInterruptIRQHead, interrupt);
}


NewtonErr
RegisterInterruptObject(InterruptObject * interrupt, InterruptObject ** ioQueue)
{
	NewtonErr			err;
	InterruptPriority thePriority;
	InterruptObject *	existingInterrupt;

	interrupt->next = NULL;
	if ((existingInterrupt = WhichHandlerPresent(interrupt->x00)) != NULL)
	{
		if ((existingInterrupt->x08 & 0x10) != 0)
		{
			if (existingInterrupt->x1C < interrupt->x1C
			&&  existingInterrupt->x1E < interrupt->x1E)
			{
				// existing interrupt must be boosted to same priority
				thePriority = GetInterruptPriority(interrupt);
				if ((err = ChangeInterruptPriority(existingInterrupt, thePriority)) != noErr)
					return err;
			}
			interrupt->x08 |= 0x40;
			return AddInterrupt(&existingInterrupt->queue, interrupt);
		}
		else
		{
			if (ioQueue == NULL)
				return kIntErrNoInterruptQueue;
			// get higher priority
			thePriority = GetInterruptPriority(existingInterrupt->x1C < interrupt->x1C ? interrupt : existingInterrupt);
			if ((err = RemoveInterrupt(existingInterrupt)) != noErr)
				return err;
			InterruptObject *	qItem = *ioQueue;
			*ioQueue = NULL;
			RegisterInterruptObject(qItem);
			if ((err = ChangeInterruptPriority(qItem, thePriority)) != noErr)
				return err;
			interrupt->x08 |= 0x40;
			existingInterrupt->x08 |= 0x40;
			qItem = qItem->queue;
			if ((err = AddInterrupt(&qItem, existingInterrupt)) != noErr)
				return err;
			return AddInterrupt(&qItem, interrupt);
		}
	}
	return AddInterrupt((interrupt->x1C == 1) ? &gInterruptFIQHead : &gInterruptIRQHead, interrupt);
}

#pragma mark -

/*------------------------------------------------------------------------------
	Handle PC card interrupt.
------------------------------------------------------------------------------*/

#if 0
int					gNumberOfHWSockets = 2;	// 0C100AB4
TCardSocket *		gCardSockets[2];			// 0C105FD4

void
DispatchTricInterrupt(void * intObj)
{
	for (ArrayIndex i = 0; i < gNumberOfHWSockets; ++i)
		gCardSockets[i]->interruptDispatcher((ULong) intObj);
}
#endif

#pragma mark -

/*------------------------------------------------------------------------------
	Add an interrupt object to the queue.
	Args:		ioQueue
				interrupt
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
AddInterrupt(InterruptObject ** ioQueue, InterruptObject * interrupt)
{
	// assume this will be added to the queue
	interrupt->next = NULL;

	// examine the queue
	InterruptObject * qItem = *ioQueue;
	if (qItem == NULL)
	{
		// no interrupt objects in the queue yet, just add this
		*ioQueue = interrupt;
	}
	else
	{
		int priority = interrupt->x1E;	// originally short
		if (priority >= qItem->x1E)
		{
			// new interrupt has higher priority, add it to the head of the queue
			interrupt->next = qItem;
			*ioQueue = interrupt;
		}
		else
		{
			// find position in the queue for this priority item
			while (priority < qItem->x1E)
			{
				if ((qItem = qItem->next) == NULL)
				{
					// reached the end of the queue, tack it onto the end
					qItem->next = interrupt;
					interrupt->next = NULL;
					return noErr;
				}
			}
			// thread item into the queue
			interrupt->next = qItem->next;
			qItem->next = interrupt;
		}
	}
	return noErr;
}


/*------------------------------------------------------------------------------
	Remove an interrupt object from its queue.
	Args:		interrupt
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
RemoveInterrupt(InterruptObject * interrupt)
{
	NewtonErr err = noErr;

	if (DisableInterrupt(interrupt) == noErr)
	{
		EnterFIQAtomic();
		if ((err = SubtractInterrupt(&gInterruptFIQHead, interrupt)) != noErr)
			err = SubtractInterrupt(&gInterruptIRQHead, interrupt);
		ExitFIQAtomic();
	}
	return err;
}


/*------------------------------------------------------------------------------
	Remove an interrupt object from a queue.
	Args:		ioQueue
				interrupt
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
SubtractInterrupt(InterruptObject ** ioQueue, InterruptObject * interrupt)
{
	NewtonErr	err;
	InterruptObject * qItem;

	if ((qItem = *ioQueue) != NULL)
	{
		if (qItem == interrupt)
		{
			// interrupt is at head of queue; point queue at next
			*ioQueue = qItem->next;
			return noErr;
		}

		for ( ; qItem->next != NULL; qItem = qItem->next)
		{
			if (qItem->next == interrupt)
			{
				// found it; point past it
				qItem->next = qItem->next->next;
				return noErr;
			}

			if ((qItem->x08 & 0x10) != 0)
			{
				if ((err = SubtractInterrupt(&(qItem->queue), interrupt)) != kIntErrCouldNotRemoveInterrupt)
					return err;	// could be noErr
				// at this point we have err = kIntErrCouldNotRemoveInterrupt
			}
		}
	}
	return  kIntErrCouldNotRemoveInterrupt;
}


/*------------------------------------------------------------------------------
	Find an interrupt in a queue.
	Args:		interrupt
				inSourceMask
	Return:	the interrupt object
------------------------------------------------------------------------------*/

InterruptObject *
FindInterrupt(InterruptObject * interrupt, ULong inSourceMask)
{
	for ( ; interrupt != NULL; interrupt = interrupt->next)
	{
		if ((interrupt->x00 & inSourceMask) != 0
		 && (interrupt->x08 & 0x20) == 0)
			return interrupt;
	}
	return NULL;
}


InterruptObject *
WhichHandlerPresent(ULong inArg1)
{
	InterruptObject * interrupt = FindInterrupt(gInterruptFIQHead, inArg1);
	return (interrupt != NULL)	? interrupt
										: FindInterrupt(gInterruptIRQHead, inArg1);
}


InterruptPriority
GetInterruptPriority(InterruptObject * interrupt)
{
	InterruptPriority thePriority;
	EnterFIQAtomic();
	thePriority.x00 = interrupt->x1C;
	thePriority.x04 = interrupt->x1E & 0x0F;
	ExitFIQAtomic();
	return thePriority;
}


NewtonErr
ChangeInterruptPriority(InterruptObject * interrupt, InterruptPriority newPriority)
{
	NewtonErr err;

	EnterFIQAtomic();
	if ((err = RemoveInterrupt(interrupt)) != noErr)
	{
		ExitFIQAtomic();
		return err;
	}

	if (interrupt->x1C != newPriority.x00)
		g0F183C00 = (g0F183C00 & ~interrupt->x00) | interrupt->x00;
	interrupt->x1C = newPriority.x00;
	interrupt->x1E = newPriority.x04;
	if ((err = RegisterInterruptObject(interrupt, NULL)) == noErr)
	{
		InterruptObject * handler, *r1;
		if ((handler = WhichHandlerPresent(interrupt->x00)) != interrupt
		 && (handler->x08 & 0x10) != 0
		 && (r1 = handler->queue) != NULL
		 && handler->x1E > r1->x1E
		 && handler->x1C > r1->x1C)
		{
			InterruptPriority thePriority;
			thePriority = GetInterruptPriority(handler->queue);
			err = ChangeInterruptPriority(handler, thePriority);
		}
	}
	
	ExitFIQAtomic();
	return err;
}


#pragma mark -


/*------------------------------------------------------------------------------
	Enable interrupt.
------------------------------------------------------------------------------*/

void
QuickEnableInterrupt(InterruptObject * interrupt)
{
	EnterFIQAtomic();
	interrupt->x08 |= kIntIsEnabled;
	ULong bits = interrupt->x00;
	if (bits == 0x00400000 || bits == 0x08000000 || bits == 0x04000000)
	{
		if ((interrupt->x08 & 0x00000001) != 0)
			SetAndClearBitsAtomic(&g0F184000, interrupt->x00, 0);
		if ((interrupt->x08 & 0x00000002) != 0)
			SetAndClearBitsAtomic(&g0F184400, interrupt->x00, 0);
		if ((interrupt->x08 & 0x00000400) != 0)
			SetAndClearBitsAtomic(&g0F184800, interrupt->x00, 0);
		gIntMaskShadowReg |= interrupt->x00;
	}
	ExitFIQAtomic();
}


NewtonErr
EnableInterrupt(InterruptObject * interrupt, ULong bits)
{
	NewtonErr err;
	if ((err = CheckEnables(interrupt, bits)) == noErr)
	{
		interrupt->x08 = (interrupt->x08 & ~0x0000040F)		// scrub relevant bits
							| (bits & 0x0000040F)					// insert required bits
							| kIntIsEnabled;
		QuickEnableInterrupt(interrupt);
	}
	return err;
}


NewtonErr
DisableInterrupt(InterruptObject * interrupt)
{
	if ((interrupt->x08 & 0x40) != 0)
		interrupt->x08 &= ~kIntIsEnabled;

	else
	{
		EnterFIQAtomic();
		ULong bits = interrupt->x00;
		if (bits == 0x00400000 || bits == 0x08000000 || bits == 0x04000000)
		{
			SetAndClearBitsAtomic(&g0F184000, 0, interrupt->x00);
			SetAndClearBitsAtomic(&g0F184400, 0, interrupt->x00);
			gIntMaskShadowReg &= ~interrupt->x00;
			ClearInterrupt(interrupt);
			interrupt->x08 &= ~kIntIsEnabled;
		}
		ExitFIQAtomic();
	}
	return noErr;
}


void
DisableAllInterrupts(void)
{
	g0F184000 = 0x0C400000;
	g0F184400 = 0x0C000000;
	g0F184800 = 0x00400000;
	g0F183C00 = 0x0C400000;
	g0F183400 = 0x0C400000;
	g0F183800 = 0xFFFFFFFF;
	gIntMaskShadowReg = 0;
}


NewtonErr
CheckEnables(InterruptObject * interrupt, ULong bits)
{
	ULong	r2 = ((bits & 0x00000001) != 0) && ((interrupt->x00 & 0x07FFFFFF) == 0) ? 1 : 0;
	ULong	r3 = ((bits & 0x00000002) != 0) && ((interrupt->x00 & 0x1CE00000) == 0) ? 1 : 0;
	ULong	r0 = ((bits & 0x00000400) != 0) && ((interrupt->x00 & 0x03FF8004) == 0) ? 1 : 0;
	if (((bits & 0x0000000C) | r2 | r3 | r0) != 0)
		return kIntErrBadInterruptEnable;
	return noErr;
}


NewtonErr
UnmaskInterrupt(InterruptObject * interrupt)
{
	interrupt->x08 &= ~0x00000100;
	return noErr;
}


NewtonErr
ClearInterrupt(InterruptObject * interrupt)
{
	if ((interrupt->x08 & 0x00000040) != 0)
		interrupt->x08 &= ~0x00000200;
	else
		g0F183800 = interrupt->x00;
	return noErr;
}

#pragma mark -

/*------------------------------------------------------------------------------
	A t o m i c
------------------------------------------------------------------------------*/

extern bool			gWantDeferred;
extern bool			gDoSchedule;

extern "C" NewtonErr	DoSchedulerSWI(void);


/*------------------------------------------------------------------------------
	Enter an atomic action.
	Disable FIQ and prevent task switching.
	All these function names map to the same address:
		EnterFIQAtomic
		PublicEnterFIQAtomic
		_EnterFIQAtomic
		_EnterFIQAtomicFast
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void	EnterFIQAtomic(void)
{
	g0F183400 = 0x0C400000;
	gAtomicFIQNestCountFast++;
}


/*------------------------------------------------------------------------------
	Exit an atomic action.
	Re-enable FIQ and allow task switching.
	All these function names map to the same address:
		ExitFIQAtomic
		PublicExitFIQAtomic
		_ExitFIQAtomic
		_ExitFIQAtomicFast
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void	ExitFIQAtomic(void)
{
	gAtomicFIQNestCountFast--;
	if (gAtomicFIQNestCountFast < 0)
		DebugStr("ExitAtomic underflow");

	if (gAtomicFIQNestCountFast == 0)
	{
		ULong		bits = gIntMaskShadowReg;
		if (gAtomicIRQNestCountFast != 0)
			bits &= g0F183C00;
		bits |= 0x0C400000;
		if (IsSuperMode())
			g0F183400 = bits;
		else
		{
			g0F183400 = bits;
			if (gWantDeferred || gDoSchedule)
				DoSchedulerSWI();
		}
	}
}


/*------------------------------------------------------------------------------
	Enter an atomic action.
	Disable IRQ interrupts and prevent task switching.
	All these function names map to the same address:
		EnterAtomic
		PublicEnterAtomic
		_EnterAtomic
		_EnterAtomicFast

	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void	EnterAtomic(void)
{
	if (gAtomicIRQNestCountFast != 0)
		gAtomicIRQNestCountFast++;

	else
	{
		g0F183400 = 0x0C400000;
		ULong		bits = gIntMaskShadowReg;
		bits &= g0F183C00;
		bits |= 0x0C400000;
		gAtomicIRQNestCountFast++;
		g0F183400 = bits;
	}
}


/*------------------------------------------------------------------------------
	Exit an atomic action.
	Re-enable IRQ interrupts and allow task switching.
		ExitAtomic
		PublicExitAtomic
		_ExitAtomic
		_ExitAtomicFast

	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void	ExitAtomic(void)
{
	gAtomicIRQNestCountFast--;
	if (gAtomicIRQNestCountFast < 0)
		DebugStr("ExitAtomic underflow");

	if (gAtomicIRQNestCountFast == 0
	&&  gAtomicFIQNestCountFast == 0)
	{
		ULong		bits = gIntMaskShadowReg;
		bits |= 0x0C400000;
		if (IsSuperMode())
			g0F183400 = bits;
		else
		{
			g0F183400 = bits;
			if (gWantDeferred || gDoSchedule)
				DoSchedulerSWI();
		}
	}
}

#pragma mark -

/*------------------------------------------------------------------------------
	Handle the FIQ interrupt request.
------------------------------------------------------------------------------*/
extern "C" void	ClearFIQMask(void);

NewtonErr
ClearFIQAtomic(void)
{
	if (IsSuperMode())
		ClearFIQMask();
	else
		GenericSWI(kClearFIQ);
	return noErr;
}


#if 0
void
FIQCleanUp(void)
{
// assembler
}


void
FIQHandler(void)
{
//	LK -= 4
//	MRS	R4, SPSR
//	AND	R4, R4, #&001F
	if (R4 == 0x10 || R4 == 0x00)
		g0C008400.xB0 = 0x55555555;
//	MCR	Internal, R0, Environment	; R0 = 0x55555555 from above

	if (g0F183000 & 0x0C400000 == 0)
	{
	//	LK = 0x01B139A8
		DispatchFIQInterrupt();
	}
	else if (g0F183000 & 0x0C000000 != 0)
	{
		g0F183800 = 0x01400000;
		BootOS();
	}
	else if FLAGTEST(gDebuggerBits, kDebuggerIsPresent)
	{
	//	LK += 4
		DebuggerIntHandler();
	}
	else
	{
		do {
			g0F183800 = 0x00400000;
			SafeShortTimerDelay(20*kMilliseconds);
		} while (g0F183000 & 0x00400000);
		if (g0F184C00 & 0x00400000)
			ResetFromResetSwitch();
		FIQCleanUp();
	}
	
}


int
DispatchFIQInterrupt(void)
{
	InterruptObject * interrupt = gInterruptFIQHead;
	ULong mask = g0F183000 & g0F183C00 & g0F183400;
	do {
		ULong bits = interrupt->x00;
		if (MASKTEST(bits, mask) != 0)
		{
			g0F183800 = bits;
			interrupt->handler(interrupt->queue);
			mask = g0F183000 & g0F183C00 & g0F183400;
		}
		else if ((interrupt = interrupt->next) == NULL)
		{
			HDefaultFIQHandler(NULL);
			g0F183800 = mask;
			break;
		}
	} while (mask);
	return 0;
}

int
HDefaultFIQHandler(void *)
{
	return 0;
}


void
DebuggerIntHandler(void)
{
// assembler
//	ReportException();
}

#pragma mark -

/*------------------------------------------------------------------------------
	Handle the IRQ interrupt request.
------------------------------------------------------------------------------*/

void
IRQHandler(void)
{
//	LK -= 4
//	STMDB	SP!, {R0-R12, LK}
	g0C008400.xB0 = 0x55555555;
//	MCR	Internal, R0, Environment	; R0 = 0x55555555 from above
//	MRS	R4, SPSR
// AND	R4, R4, #&001F
//	LK = IRQCleanUp
	DispatchIRQInterrupt();
}

int
DispatchIRQInterrupt(void)
{
	InterruptObject * interrupt = gInterruptIRQHead;
	ULong mask = g0F183000 & ~g0F183C00 & g0F183400;	// this identifies the source of the interrupt
	do {
		ULong bits = interrupt->x00;
		if (bits & mask)
		{
			g0F183800 = bits;
			interrupt->handler(interrupt->queue);
			mask = g0F183000 & ~g0F183C00 & g0F183400;
		}
		else if ((interrupt = interrupt->next) == NULL)
		{
			HDefaultIRQHandler(NULL);
			g0F183800 = mask;
			break;
		}
	} while (mask);
	return 0;
}

int
HDefaultIRQHandler(void *)
{
	return 0;
}
#endif


#pragma mark -
/* -----------------------------------------------------------------------------
	Handle simulated IRQ interrupt request.
	Args:		inSourceMask
	Return:	--

	This is called from the main-event-loop so we need to persuade the OS we are
	actually in an interrupt handler
	otherwise ExitAtomic() will try to switch task

	In the simulated environment we only see two interrupt sources:
		0x0020	timer
		0x0040	scheduler
----------------------------------------------------------------------------- */

void
DispatchFakeInterrupt(ULong inSourceMask)
{
	EnterAtomic();
	InterruptObject * interrupt = gInterruptIRQHead;
	do {
		ULong bits = interrupt->x00;
		if (MASKTEST(bits, inSourceMask) != 0)
		{
			interrupt->handler(interrupt->queue);
			inSourceMask &= ~bits;
		}
		else if ((interrupt = interrupt->next) == NULL)
		{
			break;
		}
	} while (inSourceMask);
	ExitAtomic();
}

