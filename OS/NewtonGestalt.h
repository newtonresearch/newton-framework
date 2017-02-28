/*
	File:		NewtonGestalt.h

	Contains:	Newton gestalt definitions.

	Written by:	Newton Research Group.
*/

#if !defined(__NEWTONGESTALT_H)
#define __NEWTONGESTALT_H 1

#if !defined(__NEWTON_H)
#include "Newton.h"
#endif

#if !(defined(FRAM) || defined(forDocker) || defined(forNTK))
#ifndef __USERPORTS_H
#include "UserPorts.h"
#endif
#endif	/* FRAM */

typedef uint32_t GestaltSelector;


/*--------------------------------------------------------------------------------
	Global gestalt selectors.
--------------------------------------------------------------------------------*/

#define kGestalt_Base							0x01000001
#define kGestalt_Version						(kGestalt_Base + 1)
#define kGestalt_SystemInfo					(kGestalt_Base + 2)
#define kGestalt_RebootInfo					(kGestalt_Base + 3)
#define kGestalt_NewtonScriptVersion		(kGestalt_Base + 4)
#define kGestalt_PatchInfo						(kGestalt_Base + 5)
#define kGestalt_SoundInfo						(kGestalt_Base + 6)
#define kGestalt_PCMCIAInfo					(kGestalt_Base + 7)
#define kGestalt_RexInfo						(kGestalt_Base + 8)

#define kGestalt_Extended_Base				0x02000001
#define kGestalt_SoftContrast					(kGestalt_Extended_Base + 2)
#define kGestalt_Ext_KMEAsicRegs	 			(kGestalt_Extended_Base + 3)
#define kGestalt_Ext_KMEAsicAddr	 			(kGestalt_Extended_Base + 4)
#define kGestalt_Ext_KMEPlatform	 			(kGestalt_Extended_Base + 5)
#define kGestalt_Ext_Backlight		 		(kGestalt_Extended_Base + 6)
#define kGestalt_Ext_ParaGraphRecognizer	(kGestalt_Extended_Base + 7)
#define kGestalt_Ext_VolumeInfo				(kGestalt_Extended_Base + 8)
#define kGestalt_Ext_SoundHWInfo				(kGestalt_Extended_Base + 9)


/*--------------------------------------------------------------------------------
	Global gestalt parameters for GestaltSystemInfo.
--------------------------------------------------------------------------------*/

#define kGestalt_Manufacturer_Apple			0x01000000
#define kGestalt_Manufacturer_Sharp 		0x10000100

#define kGestalt_MachineType_MessagePad	0x10001000
#define kGestalt_MachineType_Lindy			0x00726377
#define kGestalt_MachineType_Bic				0x10002000
#define kGestalt_MachineType_Q				0x10003000
#define kGestalt_MachineType_K				0x10004000
#define kGestalt_MachineType_Mac				0x1000F000


#if !(defined(FRAM) || defined(forDocker) || defined(forNTK))

/*--------------------------------------------------------------------------------
	Gestalt parameter blocks.
--------------------------------------------------------------------------------*/
#include "QDTypes.h"

struct GestaltVersion
{
	ULong		fVersion;
};


struct GestaltSystemInfo
{
	ULong		fManufacturer;
	ULong		fMachineType;
	ULong		fROMVersion;
	ULong		fROMStage;
	ULong		fRAMSize;
	ULong		fScreenWidth;
	ULong		fScreenHeight;
	ULong		fPatchVersion;
	Point		fScreenResolution;
	ULong		fScreenDepth;
	FPoint	fTabletResolution;
	ULong		fCPUType;
	float		fCPUSpeed;
	ULong		fManufactureDate;
};


struct GestaltRebootInfo
{
	NewtonErr	fRebootReason;
	ULong			fRebootCount;
};


struct GestaltNewtonScriptVersion
{
	ULong		fVersion;
};


struct GestaltPCMCIAInfo
{
	ULong		fServicesAvailibility;			// PCMCIA services availability
	ULong		fReserverd0;						// Reserved (0)
	ULong		fReserverd1;						// Reserved (0)
	ULong		fReserverd2;						// Reserved (0)
	ULong		fReserverd3;						// Reserved (0)
	ULong		fReserverd4;						// Reserved (0)
	ULong		fReserverd5;						// Reserved (0)
	ULong		fReserverd6;						// Reserved (0)
};

enum kPCMCIAServicesAvailibility						// GestaltPCMCIAInfo.fServicesAvailibility
{																// (Undefind bits are 0)
	kPCMCIACISParserAvailable		=	0x00000001,	// CIS parser available
	kPCMCIACISIteratorAvailable	=	0x00000002,	// CIS iterator available
	kPCMCIAExtendedCCardPCMCIA		=	0x00000004,	// Extended CCardPCMCIA class
	kPCMCIAExtendedCCardSocket		=	0x00000008	// Extended CCardSocket class
};


struct GestaltSoftContrast
{
	bool		fHasSoftContrast;			// true = has software contrast control
												// false = hardware contrast control
	SLong		fMinContrast;
	SLong		fMaxContrast;
};


struct GestaltBacklight
{
	bool		fHasBacklight;				// true = has backlight
												// false = no backlight
};


struct GestaltParaGraphRecognizer
{
	bool		fHasParaGraphRecognizer;	// true = has recognizer
													// false = no recognizer
};


struct _PatchInfo
{
	ULong	fPatchCheckSum;
	ULong	fPatchVersion;
	ULong	fPatchPageCount;
	ULong	fPatchFirstPageIndex;
};

struct GestaltPatchInfo
{
	ULong		fTotalPatchPageCount;
	_PatchInfo fPatch[5];
};


struct _RexInfo
{
	ULong	signatureA;
	ULong	signatureB;
	ULong	checksum;
	ULong	headerVersion;
	ULong	manufacturer;
	ULong	version;
	ULong	length;
	ULong	id;
	ULong	start;
	ULong	count;
};

struct GestaltRexInfo
{
	_RexInfo fRex[4];
};


/*--------------------------------------------------------------------------------
	User object to return Gestalt parameter blocks.
--------------------------------------------------------------------------------*/

class CUGestalt
{
public:
					CUGestalt();

	NewtonErr	gestalt(GestaltSelector inSelector, void * ioParmBlock, size_t inParmSize);
	NewtonErr	gestalt(GestaltSelector inSelector, void * ioParmBlock, size_t * ioParmSize);
	NewtonErr	registerGestalt(GestaltSelector inSelector, void * ioParmBlock, size_t inParmSize);
	NewtonErr	replaceGestalt(GestaltSelector inSelector, void * ioParmBlock, size_t inParmSize);

private:
	CUPort		fGestaltPort;
};


#endif	/* FRAM */
#endif	/* __NEWTONGESTALT_H */
