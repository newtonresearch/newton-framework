/*
	File:		NTKProtocol.h

	Contains:	Newton Toolkit communications protocol.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__NTKPROTOCOL_H)
#define __NTKPROTOCOL_H 1

/*------------------------------------------------------------------------------
	The NTK protocol packet structure -- same as the dock.
------------------------------------------------------------------------------*/

typedef struct
{
	EventClass	newt;
	EventId		dock;
	EventType	tag;
	uint32_t		length;
} SNewtCommand;


/*------------------------------------------------------------------------------
	NTK protocol commands.
------------------------------------------------------------------------------*/

#define kToolkitEventId				'ntp '

// Newton -> Desktop
#define kTConnect						'cnnt'
#define kTText							'text'
#define kTResult						'rslt'
#define kTEOM							'teom'
#define kTEnterBreakLoop			'eext'
#define kTExitBreakLoop				'bext'

#define kTDownload					'dpkg'

#define kTExceptionError			'eerr'
#define kTExceptionMessage			'estr'
#define kTExceptionRef				'eref'

// Desktop -> Newton
#define kTOK							'okln'
#define kTExecute						'lscb'
#define kTSetTimeout					'stou'
#define kTDeletePackage				'pkgX'
#define kTLoadPackage				'pkg '

// Desktop -> Newton or Newton -> Desktop
#define kTObject						'fobj'
#define kTCode							'code'
#define kTTerminate					'term'

#endif	/* __NTKPROTOCOL_H */
