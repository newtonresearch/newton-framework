/*
	File:		DockProtocol.h

	Contains:	Public interface to the Newton Dock protocol.

	Written by:	Newton Research Group, 2005-2011.
*/

#if !defined(__DOCKPROTOCOL_H)
#define __DOCKPROTOCOL_H 1

/*------------------------------------------------------------------------------
	The dock protocol packet structure.
------------------------------------------------------------------------------*/

struct DockEventHeader
{
	EventClass	evtClass;
	EventId		evtId;
	EventType	tag;
	uint32_t		length;
};

/*------------------------------------------------------------------------------
	Some objects have no fixed length...
------------------------------------------------------------------------------*/
#define kIndeterminateLength	0xFFFFFFFF


/*------------------------------------------------------------------------------
	Alignment of objects in the stream.
------------------------------------------------------------------------------*/
#define kStreamAlignment		4


/*------------------------------------------------------------------------------
	The dock event id.
------------------------------------------------------------------------------*/
#define kDockEventId		'dock'


/*------------------------------------------------------------------------------
	Protocol version for Dante Dock application.
------------------------------------------------------------------------------*/
#define kBaseProtocolVersion   9
#define kDanteProtocolVersion	10


/*------------------------------------------------------------------------------
	Default timeout in seconds if no comms acivity.
------------------------------------------------------------------------------*/
#define kDefaultTimeout			30


/*------------------------------------------------------------------------------
	Dock protocol commands.
------------------------------------------------------------------------------*/

// Starting a Session
// Newton -> Desktop
#define kDRequestToAutoDock			'auto'
#define kDRequestToDock					'rtdk'
#define kDNewtonName						'name'
#define kDNewtonInfo						'ninf'
#define kDPassword						'pass'
// Desktop -> Newton
#define kDInitiateDocking				'dock'
#define kDSetTimeout						'stim'
#define kDWhichIcons						'wicn'
#define kDDesktopInfo					'dinf'
#define kDPWWrong							'pwbd'

// Desktop <- -> Newton
#define kDResult							'dres'
#define kDDisconnect						'disc'
#define kDHello							'helo'
#define kDTest								'test'
#define kDRefTest							'rtst'
#define kDUnknownCommand				'unkn'

// Desktop -> Newton
#define kDLastSyncTime					'stme'
#define kDGetStoreNames					'gsto'
#define kDGetSoupNames					'gets'
#define kDSetCurrentStore				'ssto'
#define kDSetCurrentSoup				'ssou'
#define kDGetSoupIDs						'gids'
#define kDGetChangedIDs					'gcid'
#define kDDeleteEntries					'dele'
#define kDAddEntry						'adde'
#define kDChangedEntry					'cent'
#define kDReturnEntry					'rete'
#define kDReturnChangedEntry			'rcen'
#define kDEmptySoup						'esou'
#define kDDeleteSoup						'dsou'
#define kDGetIndexDescription			'gind'
#define kDCreateSoup						'csop'
#define kDGetSoupInfo					'gsin'
#define kDSetSoupInfo					'sinf'
// Newton 1 protocol
#define kDGetPackageIDs					'gpid'
#define kDBackupPackages				'bpkg'
#define kDDeleteAllPackages			'dpkg'
#define kDDeletePkgDir					'dpkd'
#define kDGetInheritance				'ginh'
#define kDGetPatches						'gpat'
#define kDRestorePatch					'rpat'

// Newton -> Desktop
#define kDCurrentTime					'time'
#define kDStoreNames						'stor'
#define kDSoupNames						'soup'
#define kDSoupIDs							'sids'
#define kDSoupInfo						'sinf'
#define kDIndexDescription				'indx'
#define kDChangedIDs						'cids'
#define kDAddedID							'adid'
#define kDEntry							'entr'
// Newton 1 protocol
#define kDPackageIdList					'pids'
#define kDPackage							'apkg'
#define kDInheritance					'dinh'
#define kDPatches							'patc'

// Load Package
// Newton -> Desktop
#define kDLoadPackageFile				'lpfl'
// Desktop -> Newton
#define kDLoadPackage					'lpkg'

// Remote Query
// Desktop -> Newton
#define kDQuery							'qury'
#define kDCursorGotoKey					'goto'
#define kDCursorMap						'cmap'
#define kDCursorEntry					'crsr'
#define kDCursorMove						'move'
#define kDCursorNext						'next'
#define kDCursorPrev						'prev'
#define kDCursorReset					'rset'
#define kDCursorResetToEnd				'rend'
#define kDCursorWhichEnd				'whch'
#define kDCursorCountEntries			'cnt '
#define kDCursorFree						'cfre'
// Newton -> Desktop
#define kDLongData						'ldta'
#define kDRefResult						'ref '

// Keyboard Passthrough
// Desktop <- -> Newton
#define kDStartKeyboardPassthrough	'kybd'
// Desktop -> Newton
#define kDKeyboardChar					'kbdc'
#define kDKeyboardString				'kbds'

// Misc additions
// Newton -> Desktop
#define kDDefaultStore					'dfst'
#define kDAppNames						'appn'
#define kDImportParameterSlipResult 'islr'
#define kDPackageInfo					'pinf'
#define kDSetBaseID						'base'
#define kDBackupIDs						'bids'
#define kDBackupSoupDone				'bsdn'
#define kDSoupNotDirty					'ndir'
#define kDSynchronize					'sync'
#define kDCallResult						'cres'

// Desktop -> Newton
#define kDRemovePackage					'rmvp'
#define kDResultString					'ress'
#define kDSourceVersion					'sver'
#define kDAddEntryWithUniqueID		'auni'
#define kDGetPackageInfo				'gpin'
#define kDGetDefaultStore				'gdfs'
#define kDCreateDefaultSoup			'cdsp'
#define kDGetAppNames					'gapp'
#define kDRegProtocolExtension		'pext'
#define kDRemoveProtocolExtension	'rpex'
#define kDSetStoreSignature			'ssig'
#define kDSetSoupSignature				'ssos'
#define kDImportParametersSlip		'islp'
#define kDGetPassword					'gpwd'
#define kDSendSoup						'snds'
#define kDBackupSoup						'bksp'
#define kDSetStoreName					'ssna'
#define kDCallGlobalFunction			'cgfn'
#define kDCallRootMethod				'crmf'	// spec says 'crmd'!
#define kDSetVBOCompression			'cvbo'
#define kDRestorePatch					'rpat'

// Desktop <- -> Newton
#define kDOperationDone					'opdn'
#define kDOperationCanceled			'opca'	// spec says 'opcn'!
#define kDOpCanceledAck					'ocaa'

// Sync and Selective Sync
// Newton -> Desktop
#define kDRequestToSync					'ssyn'
#define kDSyncOptions					'sopt'

// Desktop -> Newton
#define kDGetSyncOptions				'gsyn'
#define kDSyncResults					'sres'
#define kDSetStoreGetNames				'ssgn'
#define kDSetSoupGetInfo				'ssgi'
#define kDGetChangedIndex				'cidx'
#define kDGetChangedInfo				'cinf'

// File browsing
// Newton -> Desktop
#define kDRequestToBrowse				'rtbr'
#define kDGetDevices						'gdev'	// Windows only
#define kDGetDefaultPath				'dpth'
#define kDGetFilesAndFolders			'gfil'
#define kDSetPath							'spth'
#define kDGetFileInfo					'gfin'
#define kDInternalStore					'isto'
#define kDResolveAlias					'rali'
#define kDGetFilters						'gflt'	// Windows only
#define kDSetFilter						'sflt'	// Windows only
#define kDSetDrive						'sdrv'	// Windows only

// Desktop -> Newton
#define kDDevices							'devs'	// Windows only
#define kDFilters							'filt'	// Windows only
#define kDPath								'path'
#define kDFilesAndFolders				'file'
#define kDFileInfo						'finf'
#define kDGetInternalStore				'gist'
#define kDAliasResolved					'alir'

// File importing
// Newton -> Desktop
#define kDImportFile						'impt'
#define kDSetTranslator					'tran'

// Desktop -> Newton
#define kDTranslatorList				'trnl'
#define kDImporting						'dimp'
#define kDSoupsChanged					'schg'
#define kDSetStoreToDefault			'sdef'

// Restore originated on Newton
// Newton -> Desktop
#define kDRestoreFile					'rsfl'
#define kDGetRestoreOptions			'grop'
#define kDRestoreAll						'rall'

// Desktop <- -> Newton
#define kDRestoreOptions				'ropt'
#define kDRestorePackage				'rpkg'

// Desktop Initiated Functions while connected
// Desktop  -> Newton
#define kDDesktopInControl				'dsnc'
//#define kDRequestToSync				'ssyn'
#define kDRequestToRestore				'rrst'
#define kDRequestToInstall				'rins'

// Newton 2.1
#define kDSetStatusText					'stxt'


/*------------------------------------------------------------------------------
	N e w t o n I n f o

	Newton Information block, as returned by kDNewtonName.
------------------------------------------------------------------------------*/

typedef struct
{
	uint32_t	fNewtonID;				/* A unique id to identify a particular newton */
	uint32_t	fManufacturer;			/* A decimal integer indicating the manufacturer of the device */
	uint32_t	fMachineType;			/* A decimal integer indicating the hardware type of the device */
	uint32_t	fROMVersion;			/* A decimal number indicating the major and minor ROM version numbers */
											/* The major number is in front of the decimal, the minor number after */
	uint32_t	fROMStage;				/* A decimal integer indicating the language (English, German, French) */
											/* and the stage of the ROM (alpha, beta, final) */
	uint32_t	fRAMSize;
	uint32_t	fScreenHeight;			/* An integer representing the height of the screen in pixels */
	uint32_t	fScreenWidth;			/* An integer representing the width of the screen in pixels */
	uint32_t	fPatchVersion;			/* 0 on an unpatched Newton and nonzero on a patched Newton */
	uint32_t	fNOSVersion;
	uint32_t	fInternalStoreSig;	/* signature of the internal store */
	uint32_t	fScreenResolutionV;	/* An integer representing the number of vertical pixels per inch */
	uint32_t	fScreenResolutionH;	/* An integer representing the number of horizontal pixels per inch */
	uint32_t	fScreenDepth;			/* The bit depth of the LCD screen */
	uint32_t	fSystemFlags;			/* various bit flags */
											/* 1 = has serial number */
											/* 2 = has target protocol */
	uint32_t	fSerialNumber[2];
	uint32_t	fTargetProtocol;
} NewtonInfo;


/*------------------------------------------------------------------------------
	P a c k a g e I n f o

	Newton1 Package Information block, as returned by kDPackageIDList.
------------------------------------------------------------------------------*/

typedef struct
{
	uint32_t	fPackageSize;
	uint32_t	fPackageId;
	uint32_t	fPackageVersion;
	uint32_t	fFormat;
	uint32_t	fDeviceKind;
	uint32_t	fDeviceNumber;
	uint32_t	fDeviceId;
	uint32_t	fModifyDate;
	uint32_t	fIsCopyProtected;
	uint32_t	fLength;
	uint16_t	fName[];					/* fLength bytes of unicode string */
} PackageInfo;


/*------------------------------------------------------------------------------
	D o c k A p p I n f o

	The connection application definition record.
------------------------------------------------------------------------------*/

typedef struct
{
	const char *	fAppName;
	int		fAppID;
	int		fAppVersion;
	int		fSessionType;
	int		fWhichIcons;
	int		fPWRetryAttempts;
	int		fConnectDelay;
	bool		(*fStartInControl)(int);
	bool		fStartStoreSet;
	bool		fAllowIncrementalProc;
	int		fUserData;
} DockAppInfo;


/*------------------------------------------------------------------------------
	Desktop type.
------------------------------------------------------------------------------*/

enum
{
	kMacDesktop,
	kWindowsDesktop
};


/*------------------------------------------------------------------------------
	Session type.
------------------------------------------------------------------------------*/

enum
{
	kNoSession,
	kSettingUpSession,
	kSynchronizeSession,
	kRestoreSession,
	kLoadPackageSession,
	kTestCommSession,
	kLoadPatchSession,
	kUpdatingStoresSession
};


/*------------------------------------------------------------------------------
	Icon mask.
------------------------------------------------------------------------------*/

enum
{
	kBackupIcon		= 1,
	kRestoreIcon	= 1 << 1,
	kInstallIcon	= 1 << 2,
	kImportIcon		= 1 << 3,
	kSyncIcon		= 1 << 4,
	kKeyboardIcon	= 1 << 5
};


/*------------------------------------------------------------------------------
	Desktop file types.
------------------------------------------------------------------------------*/

enum
{
	kDesktop,
	kDesktopFile,
	kDesktopFolder,
	kDesktopDisk
};


/*------------------------------------------------------------------------------
	Desktop disk drive types.
------------------------------------------------------------------------------*/

enum
{
	kFloppyDrive,
	kHardDrive,
	kCDROMDrive,
	kNetDrive
};


/*------------------------------------------------------------------------------
	Info to return with kDAppNames.
------------------------------------------------------------------------------*/

enum
{
	kNamesAndSoupsForAllStores,
	kNamesAndSoupsForThisStore,
	kNamesForAllStores,
	kNamesForThisStore
};


/*------------------------------------------------------------------------------
	Backup file origin.
------------------------------------------------------------------------------*/

enum
{
	kRestoreToNewton,
	kRestoreToCard
};


/*------------------------------------------------------------------------------
	VBO compression.
------------------------------------------------------------------------------*/

enum
{
	kUncompressedVBOs,
	kCompressedPackagesOnly,
	kCompressedVBOs
};


/*------------------------------------------------------------------------------
	Source OS version.
------------------------------------------------------------------------------*/

enum SourceVersion
{
	kOnePointXData = 1,
	kTwoPointXData = 2
};


#endif	/* __DOCKPROTOCOL_H */
