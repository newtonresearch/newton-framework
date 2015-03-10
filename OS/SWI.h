/*
	File:		SWI.h

	Contains:	SWI numbers.

	Written by:	Newton Research Group
*/

#if !defined(__SWI_H)
#define __SWI_H 1

// SWI numbers

#define kGetPortSWI						 0
#define kPortSendSWI						 1
#define kPortReceiveSWI					 2
#define kEnterAtomicSWI					 3
#define kExitAtomicSWI					 4
#define kGenericSWI						 5
#define k06SWI								 6
#define k07SWI								 7
#define kFlushMMUSWI						 8
#define k09SWI								 9
#define kVersionSWI						10
#define kSemOpSWI							11
#define k12SWI								12
#define kMemSetBufferSWI				13
#define kMemGetSizeSWI					14
#define kMemCopyToSharedSWI			15
#define kMemCopyFromSharedSWI			16
#define kMemMsgSetTimerParmsSWI		17
#define kMemMsgSetMsgAvailPortSWI	18
#define kMemMsgGetSenderTaskIdSWI	19
#define kMemMsgSetUserRefConSWI		20
#define kMemMsgGetUserRefConSWI		21
#define kMemMsgCheckForDoneSWI		22
#define kMemMsgMsgDoneSWI				23
#define k24SWI								24
#define k25SWI								25
#define kMemCopyDoneSWI					26
#define kMonitorDispatchSWI			27
#define kMonitorExitSWI					28
#define kMonitorThrowSWI				29
#define k30SWI								30
#define k31SWI								31
#define kMonitorFlushSWI				32
#define kPortResetFilterSWI			33
#define kSchedulerSWI					34


// Generic SWI numbers
// - where SWI == kGenericSWI (5)

#define kTaskSetGlobals					 0
#define kTaskGiveObject					 1
#define kTaskAcceptObject				 2
#define kGetGlobalTime					 3
#define k04GenericSWI					 4
#define kTaskResetAccountTime			 5
#define kTaskGetNextId					 6
#define kForgetPhysMapping				 7
#define kForgetPermMapping				 8
#define kForgetMapping					 9

#define k15GenericSWI					15
#define k16GenericSWI					16
#define kPMRelease						17
#define k18GenericSWI					18
#define k19GenericSWI					19
#define k20GenericSWI					20
#define k21GenericSWI					21
#define k22GenericSWI					22
#define k23GenericSWI					23
#define k24GenericSWI					24
#define k25GenericSWI					25
#define k26GenericSWI					26
#define kTaskYield						27
#define kReboot							28
#define kRestart							29
#define k30GenericSWI					30
#define k31GenericSWI					31
#define kRememberMappingUsingPAddr	32
#define kSetDomainRange					33
#define kClearDomainRange				34
#define kSetEnvironment					35	// with return
#define kGetEnvironment					36	// with return
#define kAddDomainToEnvironment		37
#define kRemoveDomainFromEnvironment 38
#define kEnvironmentHasDomain			39	// with return
#define kSemGroupSetRefCon				40
#define kSemGroupGetRefCon				41
#define kVtoP								42
#define kRealTimeClockDispatch		43
#define kMemObjManagerDispatch		44
#define kNetworkPersistentInfo		45
#define kTaskBequeath					46
#define k47GenericSWI					47
#define kResetRebootReason				48
#define kRemovePMappings				49
#define kClearFIQ							50
#define kSetTabletCalibration			51
#define kGetTabletCalibration			52
#define k53GenericSWI					53
#define k54GenericSWI					54
#define k55GenericSWI					55
#define k56GenericSWI					56
#define k57GenericSWI					57
#define k58GenericSWI					58
#define kGetRExConfigEntry				59
#define k60GenericSWI					60
#define k61GenericSWI					61
#define k62GenericSWI					62
#define k63GenericSWI					63
#define kGetRExPtr						64
#define kGetLastRExConfigEntry		65
#define k66GenericSWI					66
#define kResetPortFlags					67
#define kPowerOffSystem					68
#define kPauseSystem						69
#define k70GenericSWI					70
#define k71GenericSWI					71
#define k72GenericSWI					72
#define k73GenericSWI					73
#define k74GenericSWI					74
#define kGetTaskStackInfo				75


#endif	/* __SWI_H */
