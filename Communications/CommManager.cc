/*
	File:		CommManager.cc

	Contains:	Communications code.

	Written by:	Newton Research Group.
*/

#include "UserGlobals.h"
#include "KernelTasks.h"
#include "NameServer.h"
#include "VirtualMemory.h"
#include "CMWorld.h"
#include "CommManagerInterface.h"
#include "Debugger.h"

bool			GetCommManagerPort(CUPort * outPort);
NewtonErr	InitializeCommHardware(void);
NewtonErr	RegisterROMProtocols(void);
NewtonErr	RegisterNetworkROMProtocols(void);
NewtonErr	RegisterCommunicationsROMProtocols(void);


Heap	gHALHeap;		// 0C100F7C


/*--------------------------------------------------------------------------------
	Initialize the Communications Manager.
	Args:		--
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
InitializeCommManager(void)
{
	NewtonErr err;
	XTRY
	{
		InitializeCommHardware();

		CUPort cmPort;
		XFAILNOT(GetCommManagerPort(&cmPort), err = kCMErrAlreadyInitialized;)

		CCMWorld cmWorld;
		XFAIL(err = cmWorld.init('cmgr', YES, 5*KByte))

		err = RegisterROMProtocols();
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Ask the name server for the Communications Manager port.
	Args:		outPort			the port
	Return:	YES => we got it okay
--------------------------------------------------------------------------------*/

bool
GetCommManagerPort(CUPort * outPort)
{
	return GetOSPortFromName('cmgr', outPort);
}


/*--------------------------------------------------------------------------------
	Initialize thecCommunications hardware.
	Args:		--
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
InitializeCommHardware(void)
{
	NewtonErr	err;
#if 0
	Heap			saveHeap = GetHeap();
	CHMOSerialVoyagerHardware	hw;	// +24

	if ((err = NewVMHeap(0, 50000, &gHALHeap, 0)) == noErr)
	{
		SetHeap(gHALHeap);
		XTRY
		{
			CFIQTimer *	timer(&g0F181800);	// ms counter register
			XFAILNOT(timer, err = kOSErrNoMemory;)
			timer->init();

			PTheSerChipRegistry *	serialRegistry = PTheSerChipRegistry::classInfo()->make();
			if (serialRegistry)
				serialRegistry->init();

			CAsyncDebugLink::classInfo()->registerProtocol();
			CGeoPortDebugLink::classInfo()->registerProtocol();
			CSerialChip16450::classInfo()->registerProtocol();

			CSerialChipVoyager *	voyager;

			XFAILNOT(voyager = CSerialChipVoyager::classInfo()->make(), err = MemError();)
			hw.f0C = 0x0F1C0000;
			hw.f10 = 0;
			hw.f18 = 0x00020000;
			hw.f1C = 0;
			hw.f14 = 'extr';
			if (voyager.initByOption(&hw.f00) != noErr)
				voyager.delete();

			XFAILNOT(voyager = CSerialChipVoyager::classInfo()->make(), err = MemError();)
			hw.f0C = 0x0F1D0000;
			hw.f10 = 1;
			hw.f14 = 'infr';
			hw.f18 = 0x00040000;
			hw.f1C = 0;
			if (voyager.initByOption(&hw.f00) != noErr)
				voyager.delete();

			XFAILNOT(voyager = CSerialChipVoyager::classInfo()->make(), err = MemError();)
			hw.f0C = 0x0F1E0000;
			hw.f10 = 2;
			hw.f14 = 'tblt';
			hw.f18 = 0x00080000;
			hw.f1C = 0;
			if (voyager.initByOption(&hw.f00) != noErr)
				voyager.delete();

			XFAILNOT(voyager = CSerialChipVoyager::classInfo()->make(), err = MemError();)
			hw.f0C = 0x0F1F0000;
			hw.f10 = 3;
			hw.f14 = 'mdem';
			hw.f18 = 0x00100000;
			hw.f1C = 0;
			if (voyager.initByOption(&hw.f00) != noErr)
				voyager.delete();
		}
		XENDTRY;
		SetHeap(saveHeap);
	}
	if (FLAGTEST(gDebuggerBits, kDebuggerUseExtSerial))
		InitSerialDebugging(1, 1, 'extr', 57600);
	else if (FLAGTEST(gDebuggerBits, kDebuggerUseGeoPort)
		InitSerialDebugging(1, 1, 'gpdl', 57600);
#else
	err = noErr;
#endif
	return err;
}


NewtonErr
RegisterROMProtocols(void)
{
	NewtonErr err;
	err = RegisterNetworkROMProtocols();
	err = RegisterCommunicationsROMProtocols();
	return err;
}


NewtonErr
RegisterNetworkROMProtocols(void)
{
#if 0
	CAppleTalkService::classInfo()->registerProtocol();
	CADSPEndpoint::classInfo()->registerProtocol();
	CLocalTalkLink::classInfo()->registerProtocol();
	CAppleTalkStack::classInfo()->registerProtocol();
#endif
	return noErr;
}


NewtonErr
RegisterCommunicationsROMProtocols(void)
{
#if 0
	CFaxService::classInfo()->registerProtocol();
	CModemService::classInfo()->registerProtocol();
	CMNPService::classInfo()->registerProtocol();
	CAsync::classInfo()->registerProtocol();
	CFramedAsync::classInfo()->registerProtocol();
	CP3::classInfo()->registerProtocol();
	CLocalTalk::classInfo()->registerProtocol();
	CIrDA::classInfo()->registerProtocol();
	CIR::classInfo()->registerProtocol();
	CKeyboard::classInfo()->registerProtocol();
	CTVRemote::classInfo()->registerProtocol();
	CIRSniff::classInfo()->registerProtocol();
	CIRProbe::classInfo()->registerProtocol();
	CSerialEndpoint::classInfo()->registerProtocol();
	PMux::classInfo()->registerProtocol();
#endif
	return noErr;
}


NewtonErr
CMGetEndpoint(COptionArray * inOptions, CEndpoint ** outEndpoint, bool inHandleAborts)
{
	NewtonErr err = noErr;
	*outEndpoint = NULL;

#if 0
	XTRY
	{
		CCMOEndpointName	epName;
		COptionIterator iter(inOptions);
		COption * epOption = iter.findOption(kCMOEndpointName);
		*outEndpoint = MakeByName("CEndpoint", epOption ? epOption->label() : epName);	// hmm... need the name, not the label?
		XFAILNOT(*outEndpoint, err = kCMErrNoEndPointExists;)

		CEndpointEventHandler * eventHandler = new CEndpointEventHandler(*outEndpoint, inHandleAborts);
		XFAILNOT(eventHandler, err = kCMErrNoEndPointExists;)

		CServiceInfo info;
		XFAIL(err = CMStartService(inOptions, &info))	// shouldnÕt we delete the eventHandler on error?

		eventHandler->init();
		err = (*outEndpoint)->initBaseEndpoint(eventHandler);
	}
	XENDTRY;
	XDOFAIL(err && *outEndpoint)
	{
		(*outEndpoint)->destroy()
		*outEndpoint = NULL;
	}
	XENDFAIL;
#endif

	return err;
}

