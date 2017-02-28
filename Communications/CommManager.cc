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
#include "Endpoint.h"
#include "EndpointEvents.h"
#include "Debugger.h"

NewtonErr	GetCommManagerPort(CUPort * outPort);
NewtonErr	InitializeCommHardware(void);
NewtonErr	RegisterROMProtocols(void);
NewtonErr	RegisterNetworkROMProtocols(void);
NewtonErr	RegisterCommunicationsROMProtocols(void);


Heap	gHALHeap;


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
		XFAILIF(GetCommManagerPort(&cmPort) == noErr, err = kCMErrAlreadyInitialized;)

		CCMWorld cmWorld;
		XFAIL(err = cmWorld.init('cmgr', true, 5*KByte))

		err = RegisterROMProtocols();
	}
	XENDTRY;
	return err;
}


/*--------------------------------------------------------------------------------
	Ask the name server for the Communications Manager port.
	Args:		outPort			the port
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
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
	NewtonErr err;
#if 0
	Heap saveHeap = GetHeap();
	CHMOSerialVoyagerHardware	hw;	// +24

	if ((err = NewVMHeap(0, 50000, &gHALHeap, 0)) == noErr)
	{
		SetHeap(gHALHeap);
		XTRY
		{
			CFIQTimer *	timer(&g0F181800);	// ms counter register
			XFAILNOT(timer, err = kOSErrNoMemory;)
			timer->init();

			PTheSerChipRegistry * serialRegistry = PTheSerChipRegistry::classInfo()->make();
			if (serialRegistry)
				serialRegistry->init();

			CAsyncDebugLink::classInfo()->registerProtocol();
			CGeoPortDebugLink::classInfo()->registerProtocol();
			CSerialChip16450::classInfo()->registerProtocol();

			CSerialChipVoyager *	voyager;

			voyager = CSerialChipVoyager::classInfo()->make();
			XFAILIF(voyager == NULL, err = MemError();)
			hw.f0C = 0x0F1C0000;
			hw.f10 = 0;
			hw.f18 = 0x00020000;
			hw.f1C = 0;
			hw.f14 = 'extr';
			if (voyager.initByOption(&hw.f00) != noErr)
				voyager.delete();

			voyager = CSerialChipVoyager::classInfo()->make();
			XFAILIF(voyager == NULL, err = MemError();)
			hw.f0C = 0x0F1D0000;
			hw.f10 = 1;
			hw.f14 = 'infr';
			hw.f18 = 0x00040000;
			hw.f1C = 0;
			if (voyager.initByOption(&hw.f00) != noErr)
				voyager.delete();

			voyager = CSerialChipVoyager::classInfo()->make();
			XFAILIF(voyager == NULL, err = MemError();)
			hw.f0C = 0x0F1E0000;
			hw.f10 = 2;
			hw.f14 = 'tblt';
			hw.f18 = 0x00080000;
			hw.f1C = 0;
			if (voyager.initByOption(&hw.f00) != noErr)
				voyager.delete();

			voyager = CSerialChipVoyager::classInfo()->make();
			XFAILIF(voyager == NULL, err = MemError();)
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
	else if (FLAGTEST(gDebuggerBits, kDebuggerUseGeoPort))
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
	// service
	CAppleTalkService::classInfo()->registerProtocol();
	// endpoint
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
	// services -- serial
	CFaxService::classInfo()->registerProtocol();
	CModemService::classInfo()->registerProtocol();
	CMNPService::classInfo()->registerProtocol();
	CAsyncService::classInfo()->registerProtocol();
	CFramedAsyncService::classInfo()->registerProtocol();
	CP3ServiceService::classInfo()->registerProtocol();
	CKeyboardService::classInfo()->registerProtocol();
	// network
	CLocalTalkService::classInfo()->registerProtocol();
	// infra-red
	CIrDAService::classInfo()->registerProtocol();
	CIRService::classInfo()->registerProtocol();
	CTVRemoteService::classInfo()->registerProtocol();
	CIRSniffService::classInfo()->registerProtocol();
	CIRProbeService::classInfo()->registerProtocol();
	// endpoints
	CSerialEndpoint::classInfo()->registerProtocol();
	PMuxServiceStarter::classInfo()->registerProtocol();
#endif
	return noErr;
}


NewtonErr
CMSendMessage(CCMEvent * inRequest, size_t inRequestSize, CCMEvent * outReply, size_t inReplySize)
{
	NewtonErr err;
	XTRY
	{
		size_t size;
		CUPort port;
		XFAIL(err = GetCommManagerPort(&port))
		XFAIL(err = port.sendRPC(&size, inRequest, inRequestSize, outReply, inReplySize))
		err = outReply->f0C;
	}
	XENDTRY;
	return err;
}


NewtonErr
CMStartService(COptionArray * inOptions, CServiceInfo * inServiceInfo)
{
	return CMStartServiceInternal(inOptions, inServiceInfo);
}


NewtonErr
CMStartServiceInternal(COptionArray * inOptions, CServiceInfo * outServiceInfo)
{
	CCMEvent reply;	//sp00 size+1C
	CCMEvent request;	//sp1C size+14
//	request(kCommManagerId);
//	request.f08 = 2;
//	request.f10 = inOptions;
//
	NewtonErr err = CMSendMessage(&request, sizeof(request), &reply, sizeof(reply));
//	if (err == noErr)
//		outServiceInfo = reply.f10;
	return err;
}


NewtonErr
CMGetEndpoint(COptionArray * inOptions, CEndpoint ** outEndpoint, bool inHandleAborts)
{
	NewtonErr err = noErr;
	*outEndpoint = NULL;

	XTRY
	{
/*		CCMOEndpointName	defaultEpName;
		COptionIterator iter(inOptions);
		CCMOEndpointName * epOption = (CCMOEndpointName *)iter.findOption(kCMOEndpointName);
		if (epOption == NULL)
			epOption = &defaultEpName;
		*outEndpoint = (CEndpoint *)MakeByName("CEndpoint", epOption->fClassName);
		XFAILIF(*outEndpoint == NULL, err = kCMErrNoEndPointExists;)

		CEndpointEventHandler * eventHandler = new CEndpointEventHandler(*outEndpoint, inHandleAborts);
		XFAILIF(eventHandler == NULL, err = kCMErrNoEndPointExists;)

		CServiceInfo info;
		XFAILIF(err = CMStartService(inOptions, &info), delete eventHandler;)	// original doesnÕt delete the eventHandler

		eventHandler->init(info.getPortId(), info.getServiceId());
		err = (*outEndpoint)->initBaseEndpoint(eventHandler);
*/	}
	XENDTRY;
	XDOFAIL(err)
	{
		if (*outEndpoint != NULL)
			(*outEndpoint)->destroy(), *outEndpoint = NULL;
	}
	XENDFAIL;

	return err;
}

