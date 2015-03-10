/*
	File:		CommManagerInterface.h

	Contains:	CommManager interface

	Copyright:	© 1992-1995 by Apple Computer, Inc., all rights reserved.
*/

#if !defined(__COMMMANAGERINTERFACE_H)
#define __COMMMANAGERINTERFACE_H 1

#include "NewtonErrors.h"

#ifndef FRAM

#ifndef __EVENTS_H
#include "Events.h"
#endif

#ifndef	__SYSTEMEVENTS_H
#include "SystemEvents.h"
#endif

#ifndef	__NEWTON_H
#include "Newton.h"
#endif

#ifndef	__KERNELTYPES_H
#include "KernelTypes.h"
#endif

#ifndef __OPTIONARRAY_H
#include "OptionArray.h"
#endif

#ifndef	__COMMOPTIONS_H
#include "CommOptions.h"
#endif

#ifndef __COMMSERVICES_H
#include "CommServices.h"
#endif

class CEndpoint;

#endif // not FRAM


#define kCMErrAlreadyInitialized			(ERRBASE_COMMMGR)
#define kCMErrNoSuchCommand				(ERRBASE_COMMMGR -  1)
#define kCMErrUnknownService				(ERRBASE_COMMMGR -  2)
#define kCMErrServiceExists				(ERRBASE_COMMMGR -  3)
#define kCMErrNoServiceSpecified			(ERRBASE_COMMMGR -  4)		// no service specified in the COptionArray
#define kCMErrServiceNotFound				(ERRBASE_COMMMGR -  5)		// no service registered of the type specified in the COptionArray
#define kCMErrNoEndPointExists			(ERRBASE_COMMMGR -  6)		// usually returned when CMStartService should have been called
#define kCMErrNoPublicPortExists			(ERRBASE_COMMMGR -  7)		// usually returned when CMGetEndPoint should have been called
#define kCMErrNoKnownLastDevice			(ERRBASE_COMMMGR -  8)		// no known last connected device
#define kCMErrTupleNotDeviceId			(ERRBASE_COMMMGR -  9)		// tuple received, but not device id tuple
#define kCMErrExpectedServiceInfoRsp	(ERRBASE_COMMMGR - 10)		// service info response tuple expected
#define kCMErrUnsupportedService			(ERRBASE_COMMMGR - 11)		// unsupported service, can only load packages
#define kCMErrSCPLoadInProgress			(ERRBASE_COMMMGR - 12)		// SCP load is in progress, cannot issue another
#define kCMErrSCPLoadNotAvailable		(ERRBASE_COMMMGR - 13)		// SCP load call is not supported on this machine
#define kCMErrBadSpeedResponseTuple		(ERRBASE_COMMMGR - 14)		// can process this speed
#define kCMErrNoKnownLastPackageLoaded	(ERRBASE_COMMMGR - 15)		// no package was previously loaded through SCPLoader


#ifndef FRAM

/*--------------------------------------------------------------------------------
	CConnectedDevice

	Object describing last known connected device.
	Use CMGetLastDevice to return this structure.
--------------------------------------------------------------------------------*/

#define kConnectedToSCC			(1)
#define kConnectedToPCMCIA		(2)

class CConnectedDevice
{
public:
				CConnectedDevice();

	CTime		fLastConnectTime;	// when device was last known to be connected
	ULong		fConnectedTo;		// physical object device is connected to
	ULong		fDeviceType;		// device type
	ULong		fManufacturer;		// manufacturer
	ULong		fVersion;			// version
};

inline CConnectedDevice::CConnectedDevice() : fLastConnectTime(0), fDeviceType(0), fManufacturer(0), fVersion(0) { }


/*--------------------------------------------------------------------------------
	CDeviceIdNotification
--------------------------------------------------------------------------------*/

class CDeviceIdNotification : public CSystemEvent
{
public:
	CConnectedDevice	fConnectedDevice;
};


/*--------------------------------------------------------------------------------
	CServiceInfo

	Information about how to talk to a particular service, returned from
	CMStartService (see below).
--------------------------------------------------------------------------------*/

#define kServiceByPort				(0x01)

class CServiceInfo
{
public:
	ObjectId	getPortId(void);
	ULong		getServiceId(void);

	void		setPortId(ObjectId portId);
	void		setServiceId(ULong serviceId);

private:
	UByte		fFlags;			// what service is returning
	ObjectId	fPortId;			// port of service
	ULong		fServiceId;		// service Id
};

inline ObjectId	CServiceInfo::getPortId(void)		{ return fPortId; }
inline ULong		CServiceInfo::getServiceId(void)	{ return fServiceId; }


/*--------------------------------------------------------------------------------
	initialize the CommManager; called once by the OS at boot time.
--------------------------------------------------------------------------------*/
NewtonErr	InitializeCommManager(void);

// client calls…

/*--------------------------------------------------------------------------------
	start the service described in the COptionArray,
	return the TObjectId of the port associated with
	the service.
--------------------------------------------------------------------------------*/
NewtonErr CMStartService(COptionArray* options, CServiceInfo* serviceInfo);

/*--------------------------------------------------------------------------------
	same as CMStartService, however used internally
	by tools to start other tools
--------------------------------------------------------------------------------*/
NewtonErr CMStartServiceInternal(COptionArray* options, CServiceInfo* serviceInfo);

/*--------------------------------------------------------------------------------
	start the service described in the COptionArray,
	return the CEndpoint representing the interface
	to the service.  If handleAborts is TRUE, a user
	abort handler is automatically installed
--------------------------------------------------------------------------------*/
NewtonErr CMGetEndpoint(COptionArray* options, CEndpoint** endPoint, bool handleAborts = NO);

/*--------------------------------------------------------------------------------
	returns a CConnectedDevice object, describing the last
	known connected device and the time it was connected
	returns kCMErrNoKnownLastDevice when no connected device
	is known
--------------------------------------------------------------------------------*/
NewtonErr CMGetLastDevice(CConnectedDevice* lastDevice);

/*--------------------------------------------------------------------------------
	sets last connected device to the one specified
	in lastDevice.  not necessary to set fLastConnectTime
	in lastDevice, call sets it
--------------------------------------------------------------------------------*/
NewtonErr CMSetLastDevice(CConnectedDevice* lastDevice);

/*--------------------------------------------------------------------------------
	given a service identifier, return the version
	if no such service exists, the error kCMErrUnknownService
	is returned
	version must be seeded:
	-set to kFindNewestVersion to return newest version
	-set to specific version to find that specific version
	-set to 0 to find any version (normally used just to see
	if some service exists)
--------------------------------------------------------------------------------*/
#define kFindNewestVersion		(0xFFFFFFFF)

NewtonErr CMGetServiceVersion(ULong serviceId, ULong* version);

/*--------------------------------------------------------------------------------
	tweak the serial line looking for a SCP aware device.
	the waitPeriod parameter is the number of milliseconds
	to wait on the device.
	tries is the number of times to wait on device, applying
	waitPeriod millisecond wait for each try.
	if filter is set to kSCPJustIdentify,
	just try to identify the device (a subsequent
	call to CMGetLastDevice will tell you what device
	has been identified).  if filter is set to
	kSCPLoadAnyDevice, attempt to load a package from
	any device connected and at least provide identification
	information if device supports it.  if filter is set
	to any other id, attempt to load a package only if
	that device matches the id
--------------------------------------------------------------------------------*/
#define kSCPJustIdentifiy		(0)				// just try to identify device
#define kSCPLoadAnyDevice		'****'			// wildcard, attempt to load any device

NewtonErr CMSCPLoad(ULong waitPeriod, ULong tries = 1, ULong filter = kSCPLoadAnyDevice);

#endif // FRAM

#endif	/* __COMMMANAGERINTERFACE_H */
