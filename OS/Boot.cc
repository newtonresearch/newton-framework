/*
	File:		Boot.cc

	Contains:	Boot code.

	Written by:	Newton Research Group.
*/

#include "UserGlobals.h"
#include "KernelGlobals.h"
#include "SWI.h"
#include "MemArch.h"
#include "RDM.h"
#include "MemObjManager.h"
#include "MemMgr.h"
#include "MMU.h"
#include "PhysicalMemory.h"
#include "NameServer.h"
#include "ROMExtension.h"
#include "Compression.h"
#include "NewtWorld.h"
#include "Environment.h"
#include "Tablet.h"


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

ULong		gCPSR = kSuperMode + kFIQDisable + kIRQDisable;	// start out with interrupts disabled

ULong		gHardwareType = kGestalt_MachineType_Mac;
ULong		gROMManufacturer = kGestalt_Manufacturer_Apple;
ULong		gROMVersion = 0x00030000;
ULong		gROMStage   = 0x00008000;
ULong		gManufDate = 0;

bool		gHasVoyagerRevBD;		// 0C100E80
bool		gCountTaskTime;		// 0C101008
bool		gOSIsRunning;			// 0C10102C	domains & environments are inited

SGlobalsThatLiveAcrossReboot gGlobalsThatLiveAcrossReboot;	// 0C1061C4


/*------------------------------------------------------------------------------
	H a r d w a r e   r e g i s t e r s
------------------------------------------------------------------------------*/

UByte		g0F001800 = 0x40;		// boot info	RAM bank 1 exists? size?
UByte		g0F001C00 = 0x00;		// boot info	RAM bank 2 exists? size?
ULong		g0F110000;				// keep-alive
extern ULong		g0F181000;	// real-time clock (seconds)

/*
	'dio '
  0       g0F048800
  1       g0F04A800
  2       g0F04AC00

	DMA
  0       g0F08FC00
  1       g0F098400

	'gpio'
  0       g0F18C000 R  GPIO Interrupt status     #4
  1       g0F18C400    GPIO Interrupt				 #8
  2       g0F18C800    Int ack?						 #12
  3       g0F18CC00 RW GPIO Interrupt            #16
  4       g0F18D000 RW GPIO Interrupt            #20
  5       g0F18D400 RW GPIO Input Data           #24
  6       g0F18D800 RW GPIO Interrupt            #28
  7       g0F18DC00 RW GPIO Pullup
  8       g0F18E000 RW GPIO Polarity
  A       g0F18E800 RW GPIO Direction
  B       g0F18EC00 RW GPIO Output Data
*/

void *	g0F1C0000;	// serial
void *	g0F1D0000;	// serial
void *	g0F1E0000;	// serial
void *	g0F1F0000;	// serial

struct Unknown
{
	unsigned		hi1 : 8;
	unsigned		lo3 : 24;
} g0F242400 = { 0x01, 0xF9453C };


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern "C" void	ResetNewton(void);	// for access from Cocoa layer

extern long			InitGlobalWorld(void);
extern SGlobalsThatLiveAcrossReboot *	GetBankInfo(size_t inBank1Size, size_t inBank2Size);
extern void			RExScanner(SGlobalsThatLiveAcrossReboot *	info);
extern void			InitCGlobals(const MemInit * inMap);
extern void			VMemInit(void);

void		SetRebootReason(long inReason);
void		SetUnsuccessfulBootCount(int inCount);
void		PowerOffAndReboot(NewtonErr inError);
void		Restart(void);
void		ResetFromResetSwitch(void);
void		BootOS(void);
void		OSBoot(void);

void		UserInit(void);
void		UserBoot(void);
void		InitialKSRVTask(void);

void		InitStdIO(void);

extern void			StartupProtocolRegistry(void);
extern NewtonErr	InitializePackageManager(ObjectId inId);

extern void			HInitInterrupts();
extern void			InitInterruptTables();
extern NewtonErr	InitMemArchObjs(void);
extern NewtonErr	InitDomainsAndEnvironments(void);
extern void			InitTime(void);
extern void			StartTime(void);
extern void			InitRealTimeClock(void);


/*------------------------------------------------------------------------------
	Reset vectors

	Address	Exception	Mode
	00000000	Reset			super		BootOS
	00000004	Instruction	undef		FP_UndefHandlers_Start_JT
	00000008	SWI			super		SWIBoot
	0000000C	Prefetch		abort		PrefetchAbortHandler
	00000010	Data			abort		DataAbortHandler
	00000014	Reserved					DataAbortHandler
	00000018 IRQ			IRQ		IRQHandler
	0000001C FIQ			FIQ		FIQHandler
------------------------------------------------------------------------------*/

typedef	void (*ExceptionProcPtr)(void);

struct ExceptionVectors
{
	ExceptionProcPtr	reset;
	ExceptionProcPtr	instruction;
	ExceptionProcPtr	SWI;
	ExceptionProcPtr	prefetch;
	ExceptionProcPtr	data;
	ExceptionProcPtr	reserved;
	ExceptionProcPtr	IRQ;
	ExceptionProcPtr	FIQ;
};

const ExceptionVectors	gVector =						// at address 0
{
	BootOS
};


/*------------------------------------------------------------------------------
	Following the reset vectors is a set of C variable initialization mappings.
------------------------------------------------------------------------------*/

MemInit  gDataAreaTable[2] =						// at address 0x0040
{
	{  'data',
		0x0071A95C,		// ROM address of init data
		0x0C100800,		// virtual address of start of data
		0x0C105AF0,		// virtual address of end of data, start of zeroed data area
		0x52F0,			// size of data
		0x2324 },		// size of zero data
	{  0  }
};


/*------------------------------------------------------------------------------
	And then a level one descriptor definition table.
------------------------------------------------------------------------------*/

const SectionDef	g8MegContinuousTableStart[] =		// at address 0x0100
{
	{	0x00000000,		// 00000000 - 1 page of RAM
		0x00000400,		// pageTable 1
		0x00100000,
		0x00000011 },	// kUpdateable | kPageDescriptorType

	{	0x00100000,		// 00100000 - 16MB of ROM
		0x00100000,		// sectionBase 1
		0x00F00000,
		0x0000081E },	// kUpdateable | kCacheable | kBufferable | kSectionDescriptorType

	{	0x03500000,
		0x00000000,
		0x00800000,
		0x00000802 },	// kSectionDescriptorType

	{	0x04000000,
		0x00000000,
		0x00100000,
		0x0000081E },	// kUpdateable | kCacheable | kBufferable | kSectionDescriptorType

	{	0x05000000,
		0x02000000,
		0x00200000,
		0x00000C02 },	// kSectionDescriptorType

	{	0x0C000000,
		0x00001400,		// pagetable 5
		0x00100000,
		0x00000011 },	// kUpdateable | kPageDescriptorType

	{	0x0F000000,
		0x0F000000,
		0x21000000,
		0x00000C02 },	// kSectionDescriptorType

	{	0xFFFFFFFF }
};

const SectionDef	g8MegContinuousTableStartFor4MbK[] =	// at address 0x0174
{
	{	0x00000000,
		0x00000400,
		0x00100000,
		0x00000011 },

	{	0x00100000,
		0x00100000,
		0x00F00000,
		0x0000081E },

	{	0x03500000,
		0x00000000,
		0x00800000,
		0x00000802 },

	{	0x04000000,
		0x00000000,
		0x00100000,
		0x0000081E },

	{	0x05000000,
		0x02000000,
		0x00200000,
		0x00000C02 },

	{	0x0C000000,
		0x00001800,
		0x00100000,
		0x00000011 },

	{	0x0F000000,
		0x0F000000,
		0x21000000,
		0x00000C02 },

	{	0xFFFFFFFF }
};

/*------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
------------------------------------------------------------------------------*/

NewtonErr
ResetRebootReason(void)
{
	if (IsSuperMode())
	{
		if (gGlobalsThatLiveAcrossReboot.fRebootReason != kOSErrRebootCalibrationMissing)
			gGlobalsThatLiveAcrossReboot.fRebootReason = noErr;
		gGlobalsThatLiveAcrossReboot.fUnsuccessfulBootCount = 0;
		return noErr;
	}

	else
		return GenericSWI(kResetRebootReason);
}


void
SetRebootReason(long inReason)
{
	if (g0F242400.lo3 == 0
	|| (g0F001800 == 0 && g0F001C00 == 0))
		return;

	SGlobalsThatLiveAcrossReboot *	pGlobals;
	pGlobals = MakeGlobalsPtr(g0F001800);
	pGlobals->fRebootReason = inReason;
	pGlobals->fMagicNumber = kRebootMagicNumber;
}


NewtonErr
GetRebootReason(void)
{
	if (g0F242400.lo3 == 0
	|| (g0F001800 == 0 && g0F001C00 == 0))
		return -10083;

	SGlobalsThatLiveAcrossReboot *	pGlobals;
	pGlobals = MakeGlobalsPtr(g0F001800);
	if (pGlobals->fMagicNumber != kRebootMagicNumber)
		return -10085;

	return pGlobals->fRebootReason;
}


void
SetUnsuccessfulBootCount(int inCount)
{
	if (g0F242400.lo3 == 0
	|| (g0F001800 == 0 && g0F001C00 == 0))
		return;

	SGlobalsThatLiveAcrossReboot *	pGlobals;
	pGlobals = MakeGlobalsPtr(g0F001800);
	pGlobals->fUnsuccessfulBootCount = inCount;
	pGlobals->fMagicNumber = kRebootMagicNumber;
}


int
GetUnsuccessfulBootCount(void)
{
	if (g0F242400.lo3 == 0
	|| (g0F001800 == 0 && g0F001C00 == 0))
		return 0;

	SGlobalsThatLiveAcrossReboot *	pGlobals;
	pGlobals = MakeGlobalsPtr(g0F001800);
	if (pGlobals->fMagicNumber != kRebootMagicNumber)
		return kOSErrRebootBatteryChanged;

	return pGlobals->fUnsuccessfulBootCount;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Initialize globals that DON’T live across reboot - ones that are reset
	each boot.
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

void
InitGlobalsThatLiveAcrossReboot(void)
{
	gGlobalsThatLiveAcrossReboot.fWarmBootCount = 0;
	gGlobalsThatLiveAcrossReboot.fLastNodeId = 0;
	gGlobalsThatLiveAcrossReboot.fTimeAtColdBoot = g0F181000;
	gGlobalsThatLiveAcrossReboot.fProcessorOnTime = 0;
	gGlobalsThatLiveAcrossReboot.fScreenOnTime = 0;
	gGlobalsThatLiveAcrossReboot.fSerialOnTime = 0;
	gGlobalsThatLiveAcrossReboot.fSoundOnTime = 0;
#if defined (correct)
	gGlobalsThatLiveAcrossReboot.fCompactState.init();
#endif
	memset(gGlobalsThatLiveAcrossReboot.fReserved, 0, 32);

	gGlobalsThatLiveAcrossReboot.fUnsuccessfulBootCount++;
}


/*------------------------------------------------------------------------------
	Set up info about ROM & RAM memory.
	Args:		inBank1Size		size of RAM bank 1
				inBank2Size		size of RAM bank 2
	Return:  pointer to persistent globals
------------------------------------------------------------------------------*/

SGlobalsThatLiveAcrossReboot *
MemoryTest(size_t inBank1Size, size_t inBank2Size)
{
	SGlobalsThatLiveAcrossReboot *	pGlobals = GetBankInfo(inBank1Size, inBank2Size);
	RExScanner(pGlobals);

	if (pGlobals->fMagicNumber != kRebootMagicNumber)
	{
		if (pGlobals->fRebootReason != kOSErrRebootCalibrationMissing)
			pGlobals->fRebootReason = noErr;
		pGlobals->fUnsuccessfulBootCount = 0;
	}
	pGlobals->fTotalPatchPageCount = 0;
	pGlobals->fWarmBootCount = 0;
	pGlobals->fRebuildPageTracker = 0;
	pGlobals->fPersistentDataInUse = 0xBDB3ABB8;
	pGlobals->fMagicNumber = kRebootMagicNumber;

	return pGlobals;
}


/*------------------------------------------------------------------------------
	Recover persistent state after reset.
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

void
PersistentRecovery(void)
{
	BuildMemObjDatabase();
	InitGlobalsThatLiveAcrossReboot();
	VMemInit();
	FlushTheMMU();
}


/*------------------------------------------------------------------------------
	Finish up initialization after C globals are valid.
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

void
PostCGlobalsHWInit(void)
{
	gHasVoyagerRevBD = true;

#if defined(correct)
	if (g0F183000 & 0x04000000)
		if (gGlobalsThatLiveAcrossReboot->fRebootReason != kOSErrRebootCalibrationMissing)
			gGlobalsThatLiveAcrossReboot->fRebootReason = kOSErrRebootPowerFault;

	if (g0F183000 & 0x08000000)
		if (gGlobalsThatLiveAcrossReboot->fRebootReason != kOSErrRebootCalibrationMissing)
			gGlobalsThatLiveAcrossReboot->fRebootReason = kOSErrRebootBatteryFault;
#endif

// LowLevelProcRevLevel()  but doesn’t do anything with the result
	gGlobalsThatLiveAcrossReboot.fMagicNumber = kRebootMagicNumber;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Power off cleanly and reboot.
	Args:		inError			reason for reboot
	Return:  --
------------------------------------------------------------------------------*/

void
PowerOffAndReboot(NewtonErr inError)
{
	EnterFIQAtomic();
#if defined(correct)
	IOPowerOffAll();
	DisableAllInterrupts();
	TGPIOInterface * gpIO = GetGPIOInterfaceObject();
	gpIO->init();
	RegisterPowerSwitchInterrupt();
	EnableSysPowerInterrupt();
	PowerOffSystem();
	ClearInterruptBits(0xFFFFFFFF);
#endif
	Reboot(inError, 0, true);
	ExitFIQAtomic();
}


/*------------------------------------------------------------------------------
	Reboot.
	Args:		inError			reason for reboot
				inRebootType	kRebootMagicNumber -> cold, else warm boot
				inSafe			yes -> wait for any protected monitors to finish
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
Reboot(NewtonErr inError, ULong inRebootType, bool inSafe)
{
	if (IsSuperMode())
	{
		EnterFIQAtomic();

		if (gGlobalsThatLiveAcrossReboot.fRebootReason != kOSErrRebootCalibrationMissing)
			gGlobalsThatLiveAcrossReboot.fMagicNumber = kRebootMagicNumber;
		if (inRebootType == kRebootMagicNumber	// cold boot
		&& inError != kOSErrSorrySystemFailure
		&& gGlobalsThatLiveAcrossReboot.fRebootReason != kOSErrRebootCalibrationMissing)
			gGlobalsThatLiveAcrossReboot.fRebootReason = noErr;

		if (inSafe && gRebootProtectCount == 0)
		{
			gWantReboot = true;
			ExitFIQAtomic();
		}
		else if (gGlobalsThatLiveAcrossReboot.fUnsuccessfulBootCount <= kMaxUnsuccessfulBootCount)
			gVector.reset();
#if defined(correct)
		else
		{
			DisableAllInterrupts();
			IOPowerOffAll();
			for ( ; ; )
				g0F110000 = 0x08;
		}
#endif
		return noErr;
	}

	else
		return GenericSWI(kReboot, inError, inRebootType, inSafe);
}


/*------------------------------------------------------------------------------
	Continue reboot after protected monitors have finished (see inSafe above).
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

void
Restart(void)
{
	if (IsSuperMode())
	{
		EnterFIQAtomic();
		gVector.reset();
	}
	else
		GenericSWI(kRestart);
}


/*------------------------------------------------------------------------------
	Handle the reset button.
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/
void	ResetNewton(void) { ResetFromResetSwitch(); }

void
ResetFromResetSwitch(void)
{
/*
	MOV	R0, #&00B0
	ORR	R0, R0, #&1000
	MCR	Internal, R0, Configuration	; set 32-bit big-endian mode
*/
	SetRebootReason(kOSErrRebootFromResetSwitch);
	SetUnsuccessfulBootCount(0);
	BootOS();
}


/*------------------------------------------------------------------------------
	Boot the OS.
	This is where our story REALLY starts.
	Call tree:

	MemoryTest
		GetBankInfo				init CRAMTable
		RExScanner				read RAMAllocation config into persistent globals
	InitCGlobals				map C globals and mem obj database into virtual space
	PersistentRecovery
		BuildMemObjDatabase	build mem obj database
		InitGlobalsThatLiveAcrossReboot	zero non-persistent globals
		VMemInit					init virtual memory pages (safe heap)
		FlushTheMMU
	PostCGlobalsHWInit		nothing of any significance
	OSBoot						create task to boot OS

	And it should start in assembler…
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

void
BootOS(void)
{
#if !defined(correct)
	// allocate physical memory
extern KernelArea	gKernelArea;
extern PAddr gPhysRAM;
	// we want 4MByte aligned on a MByte boundary
	gPhysRAM = (PAddr)ALIGN(malloc(4*MByte + kSectionSize), kSectionSize);
	// change fixed VAddr to mem we allocated
	gDataAreaTable->dataStart = gPhysRAM + 0x0800;
#endif

/*
		MOV	R0, #&00B0
		ORR	R0, R0, #&1000
		MCR	Internal, R0, Configuration	; set 32-bit big-endian mode

		MRC	Internal, R0, CR0
		BIC	R0, R0, #&0F
		EOR	R0, R0, #&44000000
		EOR	R0, R0, #&00010000
		EORS	R0, R0, #&0000A100
		MRCEQ	Internal, R0, CR15				; if CPU = D-1-A10 read CR15 !?

		LDR	R0, [g0F183800]
		MOV	R1, #&01400000
		STR	R1, [R0]

		LDR	R0, [g0F185000]
		MOV	R1, #&00000001
		STR	R1, [R0]

		BNE	L20
		LDR	R0, [g0F242400]
		LDR	R0, [R0]
		BICS  R0, R0, #&FF000000
		BNE	RestoreCPUStateAndStartSystem ; &000191D0, private
	
L20	LDR	R0, [g0F183C00]
		MOV	R1, #&0C400000
		STR	R1, [R0]
		LDR	R0, [g0F184000]
		MOV	R1, #&0D400000
		STR	R1, [R0]
		LDR	R0, [g0F184400]
		STR	R1, [R0]
		LDR	R0, [g0F184800]
		STR	R1, [R0]

		LDR	R0, [g0F183400]
		STR	R1, [R0]

		LDR	R0, [g0F18C800]
		MOV	R1, #&00000001
		STR	R1, [R0]

		MSR	CPSR, #&D1			; enter fiq mode, interrupts disabled
		NOP
		NOP
		BL		GetRebootReason
		MOV	R11, R0				; R11_fiq
		BL		GetUnsuccessfulBootCount
		MOV	R12, R0				; R12_fiq
		MSR	CPSR, #&D3			; resume svc mode
		NOP
		NOP
*/
	gCPSR = kFIQMode + kFIQDisable + kIRQDisable;
	NewtonErr  whyBoot = GetRebootReason();
	int  bootCount = GetUnsuccessfulBootCount();

/*
		LDR	R0, [g0F111400]
		LDR	R1, [x000007E6]		; 2022
		STR	R1, [R0]
		LDR	R0, [x0000016F]		; 367 = 100µs ?
		BL		SafeShortTimerDelay
		LDR	R0, [g0F111400]
		MOV	R1, #0
		STR	R1, [R0]

		BL		BasicBusControlRegInit
		LDR	R0, [gDiagBootStub]  ; no idea why, first thing DiagBootStub does is to LDR R0
		BL		DiagBootStub

		MSR	CPSR, #&D1  ; enter fiq mode
		NOP
		NOP
		MOV	R8, R0		; r8_fiq, ramBank1Size, set by DiagBootStub?
		MOV	R9, R1		; r9_fiq, ramBank2Size
		MSR	CPSR, #&D3  ; resume svc mode
		NOP
		NOP

		CMP	R3, #0		; set by DiagBootStub?
		BNE	DiagHook

		MOV	R0, #&000000B0
		ORR	R0, R0, #&00001000
		MCR	Internal, R0, Configuration	; set 32-bit big-endian mode

		MSR	CPSR, #&D1  ; enter fiq mode
		NOP
		NOP
		MOV	R0, R11
		BL		SetRebootReason
		MOV	R0, R12
		BL		SetUnsuccessfulBootCount

		MOV	R5, R8
		MOV	R6, R9
		MSR	CPSR, #&D3  ; resume svc mode
		NOP
		NOP
*/

	SetRebootReason(whyBoot);
	SetUnsuccessfulBootCount(bootCount);

/*
		MOV	R11, #0

		MOV	R0, R5
		BL		GetSuperStacksPhysBase
		ADD	SP, R0, #1*KByte		; reset stack

		MOV	R0, #&FFFFFFFF
		BL		SetAbortStack			; set SP in abort mode

		MOV	R0, R5
		MOV	R4, R5
		MOV	R1, R6
		BL		MemoryTest				; 000187F8 BL &00313920 | and discover available DRAM
		MOV	R7, R0

		BL		GetfBankAddr
		MOV	R4, R0
		MOV	R0, R5
		BL		GetSuperStacksPhysBase
		STR	R4, [R0]					; set up RAM table info
		MOV	R1, #23
		STR	R1, [R0, #4]
		BL		CopyRAMTableToKernelArea

		TEQ	R5, #0
		LDRNE R2, [g00000100]
		LDREQ R2, [g00000174]
		MOV	R5, R2
		MOV	R0, #1
		MOV	R1, #0
		MOV	R3, R7
		BL		InitTheMMUTables
*/
	size_t			ramBank1Size = 4*MByte;
	size_t			ramBank2Size = 0;
	SGlobalsThatLiveAcrossReboot *	pGlobals = MemoryTest(ramBank1Size, ramBank2Size);
	SBankInfo *		bankInfo = pGlobals->fBank;
	KernelArea *	bootInfo = (KernelArea *)GetSuperStacksPhysBase(ramBank1Size);
	bootInfo->ptr = bankInfo;
	bootInfo->num = 23;
	CopyRAMTableToKernelArea(bootInfo);
	InitTheMMUTables(true, false, (ramBank1Size != 0) ? (PAddr) g8MegContinuousTableStart : (PAddr) g8MegContinuousTableStartFor4MbK, pGlobals);
/*
		MOV	R0, #&00000055
		ORR	R0, R0, #&00005500
		ORR	R0, R0, #&00550000
		ORR	R0, R0, #&55000000
		MCR	Internal, R0, Environment  ; all domains have client access

		BL		GetKernelStackVirtualTop
		MOV	R4, R0					; VAddr virtualStackTop

		MOV	R0, R7					; pGlobals
		BL		GetPrimaryTablePhysBaseMMUOff
		MCR	Internal, R0, Base			; set the Level One translation table address

		MRC	Internal, R0, CR0
		AND	R1, R0, #&0F
		BIC	R0, R0, #&0F
		EOR	R0, R0, #&41000000
		EOR	R0, R0, #&00040000
		EORS	R0, R0, #&00007100
		CMPNE	R1, #2
		MOV	R0, #&B0							; set 32-bit big-endian mode
		ORR	R0, R0, #&00000005			; enable cache and MMU
		ORR	R0, R0, #&00001100
		ORRGE	R0, R0, #&00000008			; if CPU = A-4-710 or rev >= 2 enable write buffer
		MCR	Internal, R0, Configuration

		BL		FlushTheCache					; the MMU is now on!

		MOV	SP, R4							; reset the system stack

		BL		HandleDebugCard				; does nothing
		BL		InitSpecialStacks
		BL		SaveDebuggerCPUInfo
		LDR	R0, [gDataAreaTable]
		BL		InitCGlobals
		MOV	R0, R5
		BL		PersistentRecovery
		LDR	R0, [g0038D6AC]
		BL		InitParamBlockFromImagePhysical  ; does nothing
		BL		PostCGlobalsHWInit
		BL		InitInterruptStacks			; 00018B78, private, follows SetFIQStack
		InitFinished
*/
#if !defined(correct)
	gKernelArea.bank[0] = *bankInfo;
#endif

	InitCGlobals(gDataAreaTable);
	PersistentRecovery();
	PostCGlobalsHWInit();

/*
		LDR	R0, [g00000034]	; points to InitCirrusHW
		MOV	LK, PC + 8
		LDR	PC, [R0]

		BL		DisableAllInterrupts
		BL		AdjustRealTimeClock

		MSR	CPSR, #&10			; enter usr_32 mode
		NOP
		NOP

		BL		OSBoot
		ExitToShell
*/

	gCPSR = kUserMode;
	OSBoot();
}


/*------------------------------------------------------------------------------
	Continue booting in C++…
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

void
OSBoot(void)
{
	CEnvironment	bootEnvironment, * environment;
	CTask			 	bootTask, * task;
	ObjectId			environmentId;
	ObjectId			mysteryId;
	TaskGlobals		globals;

//	Set up a globals pointer. No task data yet; just empty task globals.
//	We just need a valid pointer.
	gCurrentGlobals = (TaskGlobals *)&globals + 1;

// Set up a null task while we’re booting.
	bootEnvironment.fDomainAccess = (kClientDomain << (kDomainBits * 2))
											 |(kClientDomain << (kDomainBits * 1))
											 | kClientDomain;	// 0x15 3 client domains
	bootTask.fEnvironment = &bootEnvironment;
	gCurrentTask = &bootTask;

// Initialize interrupts for timers and scheduler.
	HInitInterrupts();
	InitInterruptTables();

// Create the kernel object table.
	gObjectTable = new CObjectTable;
	gObjectTable->init();

// Initialize the memory architecture, domains and environments.
	InitMemArchCore();
	InitKernelDomainAndEnvironment();

	MemObjManager::findEnvironmentId('krnl', &environmentId);
	environment = (CEnvironment *)IdToObj(kEnvironmentType, environmentId);

// No task swaps while we’re adding new tasks
	EnterFIQAtomic();

// Initialize the kernel’s global variables and timers.
	InitGlobalWorld();
	InitTime();
	InitRealTimeClock();

// Initialize user (as opposed to supervisor) space.
	UserInit();

// Register a task to be performed when the system’s doing nothing.
//	When it’s REALLY doing nothing. No other tasks are alive.
//	This, of course, “should never happen”, but if it does, reboot.
	task = new CTask();
	gIdleTask = task;
	RegisterObject(task, kTaskType, kSystemId, NULL);
	task->init((TaskProcPtr) OSBoot, 0, kNoId, kNoId, kIdleTaskPriority, 'idle', environment);

	gCurrentTask = gIdleTask;
	SwapInGlobals(gIdleTask);

// Register the user task. This will boot the user space.
	task = new CTask();
	RegisterObject(task, kTaskType, kSystemId, &mysteryId);
	task->init((TaskProcPtr) UserBoot, 2*KByte, kNoId, kNoId, kKernelTaskPriority, 'user', environment);

// Start the timer and tablet, and kick the scheduler into action.
	StartTime();
	TabBoot();
	gCountTaskTime = false;	// correct => true; collect processor usage stats
	StopScheduler();
	StartScheduler();

// Now tasks can get going.
	ExitFIQAtomic();

// Schedule the user task.
	EnterAtomic();
	ScheduleTask(task);	// correct => adds task even when current
	ExitAtomic();

// Send the idle task to sleep.
//	SleepTask();
}

#pragma mark -

/*------------------------------------------------------------------------------
	Initialize user space.
	We need a user monitor to speak to the object manager, and a null user port.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
UserInit(void)
{
	gUObjectMgrMonitor = new CUMonitor(GetPortSWI(kGetObjectPort));
	gUNullPort = new CUPort(GetPortSWI(kGetNullPort));
}


/*------------------------------------------------------------------------------
	Boot user space.
	This is executed as a kernel task named 'user'.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
UserBoot(void)
{
	CULockingSemaphore::staticInit();
	CURdWrSemaphore::staticInit();
	AddSemaphoreToHeap(GetHeap());

	InitMemArchObjs();
	InitDomainsAndEnvironments();

	MemObjManager::findHeapRef('user', &gSkiaHeapBase);	// never referenced
	InitROMDomainManager();

	gOSIsRunning = true;
	
	CTime now = CURealTimeAlarm::time();
	srand(now.convertTo(kSeconds));

	XTRY
	{
		CUTask	userTask;
		ObjectId	environmentId;
		XFAIL(MemObjManager::findEnvironmentId('ksrv', &environmentId))
		XFAIL(userTask.init((TaskProcPtr) InitialKSRVTask, 26*KByte, 0, NULL, kUserTaskPriority, 'ksrv', environmentId))
		userTask.start();
	}
	XENDTRY;

	SetBequeathId(*gIdleTask);
}


/*------------------------------------------------------------------------------
	Set up the user runtime.
	o	Start up the protocol registry so dynamic classes can be loaded.
	o	Initialize stdio for debugging.
	o	Start up the name server so we can access shared resources and gestalt.
	o	Register the ROM domain manager
		and initialize the package manager so we can load packages.
	o	Start the user task that loads the NewtonScript world.
	This is executed as a user task named 'ksrv'.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
InitialKSRVTask(void)
{
	StartupProtocolRegistry();
	InitStdIO();
	InitializeNameServer();
	RegisterROMDomainManager();

	XTRY
	{
		ObjectId envId;
		XFAIL(MemObjManager::findEnvironmentId('prot', &envId))	// protected environment
		InitializePackageManager(envId);

		XFAIL(MemObjManager::findEnvironmentId('user', &envId))	// user environment
		CLoader worldLoader;
		worldLoader.init('drvl', true, kSpawnedTaskStackSize, kUserTaskPriority, envId);
	}
	XENDTRY;

	SetBequeathId(*gIdleTask);
}


// ••••• Stubs

void
InitStdIO(void)
{
//	No need for this in a POSIX environment
}


#if 0
CBIOInterface		gBIOInterface;
CGPIOInterface		gGPIOInterface;
CDMAManager			gDMAManager;

void
InitCirrusHW(void)
{
	gBIOInterface.init();
	gGPIOInterface.init();
	gSerialNumberROM.init();
	gDMAManager.init();
}
#endif

#pragma mark -

NewtonErr
GetNetworkPersistentInfo(ULong * outInfo)
{
	NewtonErr	err;
	ULong			info;
	if (IsSuperMode())
	{
		info = gGlobalsThatLiveAcrossReboot.fLastNodeId;
		err = noErr;
	}
	else
	{
		TaskGlobals *	taskGlobals = TaskSwitchedGlobals();
		taskGlobals->fParams[0] = 0;
		err = GenericSWI(kNetworkPersistentInfo);
		info = taskGlobals->fParams[1];
	}
	*outInfo = info;
	return err;
}


NewtonErr
SetNetworkPersistentInfo(ULong info)
{
	NewtonErr	err;
	if (IsSuperMode())
	{
		gGlobalsThatLiveAcrossReboot.fLastNodeId = info;
		err = noErr;
	}
	else
	{
		TaskGlobals *	taskGlobals = TaskSwitchedGlobals();
		taskGlobals->fParams[0] = 1;
		taskGlobals->fParams[1] = info;
		err = GenericSWI(kNetworkPersistentInfo);
	}
	return err;
}


NewtonErr
PrimGetNetworkPersistentInfo(void)
{
	NewtonErr		err;
	TaskGlobals *	taskGlobals = TaskSwitchedGlobals();
	if (taskGlobals->fParams[0] == 0)
		err = GetNetworkPersistentInfo(&taskGlobals->fParams[1]);
	else if (taskGlobals->fParams[0] == 1)
		err = SetNetworkPersistentInfo(taskGlobals->fParams[1]);
	else
		err = kOSErrBadParameters;
	return err;
}

