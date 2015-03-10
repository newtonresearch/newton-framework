/*
	File:		Docker.h

	Contains:	Docker declarations.
					The Docker uses the Dock Protocol to communicate with a desktop machine.
					See also NTKProtocol.h

	Written by:	Newton Research Group, 2012.
*/

#if !defined(__DOCKER_H)
#define __DOCKER_H

#include "Events.h"
#include "EndpointPipe.h"
#include "OptionArray.h"
#include "DockProtocol.h"
#include "CommAddresses.h"
#include "DES.h"


/*------------------------------------------------------------------------------
	The dock dynamic array.
	Well, every other sub-system has its own implementation, why shouldn’t we?
	This one is used for recording protocol extension ids.
------------------------------------------------------------------------------*/

class CDockerDynArray
{
public:
					CDockerDynArray();
					~CDockerDynArray();

	NewtonErr	addAndReplaceZero(ULong inId, ArrayIndex& index);
	NewtonErr	add(ULong inId);
	void			replace(ArrayIndex index, ULong inId);
	ArrayIndex	find(ULong inId);

private:
	ULong *		fArray;
	ArrayIndex	fSize;
	ArrayIndex	fAllocatedSize;
};


/*------------------------------------------------------------------------------
	C C u r s o r A r r a y
	Used only by CDocker -- not implemented.
------------------------------------------------------------------------------*/

class CCursorArray
{
public:
			CCursorArray(void);
			~CCursorArray(void);

	Ref	add(RefArg inCursor);
	Ref	remove(ArrayIndex index);
	Ref	get(ArrayIndex index);
};


/*------------------------------------------------------------------------------
	The dock endpoint.
------------------------------------------------------------------------------*/
enum ConnectionType
{
	kSerial,
	kAppleTalk,
	kType2,
	kMNPSerial,
	kSharpIR,
	kMNPModem,
	kIrDA
};


class CEzEndpointPipe : public CEndpointPipe
{
public:
					CEzEndpointPipe();
					CEzEndpointPipe(bool);
					~CEzEndpointPipe();

	void			init(RefArg inOptions, Timeout inTimeout);
	void			init(COptionArray * inOpenOptions, COptionArray * inBindOptions, COptionArray * inConnectOptions, bool inUseEOP, Timeout inTimeout);
	void			init(ConnectionType inType, Handle inData, Timeout inTimeout);
	NewtonErr	commonInit(Timeout inTimeout);

	void			getSerialEndpoint(void);
	void			getMNPSerialEndpoint(void);
	void			getMNPModemEndpoint(void);
	void			getSharpIREndpoint(void);
	void			getIrDAEndpoint(void);
	void			getADSPEndpoint(void);

	ArrayIndex	bytesAvailable(void);
	void			abort(void);
	NewtonErr	tearDown(void);

private:
	NewtonErr		fError;		//+2C
	Handle			f30;
	Timeout			fEzTimeout;	//+34
	COptionArray	f38;
	COption *		fBytesAvailableOption;	//+50
	bool				f54;
//size+58
};

NewtonErr	EzSerialOptions(COptionArray * outOptions, Handle inData, size_t inWrBufLen, size_t inRdBufLen);
NewtonErr	EzNBPLookup(CCMAAppleTalkAddr*, char**);
NewtonErr	EzADSPConnectOptions(COptionArray*, char**, long, long);
NewtonErr	EzMNPSerialOptions(COptionArray*, char**);
NewtonErr	EzMNPConnectOptions(COptionArray*, char**);
NewtonErr	EzSharpIROptions(COptionArray*, char**);
NewtonErr	EzMNPModemOptions(COptionArray*, char**);
NewtonErr	EzIrDAOptions(COptionArray*, char**);
bool			EzConvertOptions(RefArg inOptions, COptionArray ** outOpenOptions, COptionArray ** outBindOptions, COptionArray ** outConnectOptions);


/*------------------------------------------------------------------------------
	The dock event reader/writer.
	Base class of CDocker, but also of CCommServer
------------------------------------------------------------------------------*/

class CEzPipeProtocol
{
public:
	void		protocolInit(EventClass inClass, EventId inId);

	void		findDockerHeader(EventType& outCommand, size_t& outLength);
	void		readDockerHeader(EventType& outCommand, size_t& outLength);
	void		writeDockerHeader(EventType inCommand, bool inDone);

protected:
	CEzEndpointPipe * fPipe;

private:
	EventClass	fEvtClass;
	EventId		fEvtId;
};


/*------------------------------------------------------------------------------
	The dock controller.
------------------------------------------------------------------------------*/

enum eDockingState {};

class CDocker : public CEzPipeProtocol
{
public:
				CDocker();
				~CDocker();

// connection protocol
	void		connect(RefArg, RefArg, RefArg);
	NewtonErr	doConnection(RefArg, RefArg, RefArg, bool&);
	void		callConnectionApp(RefArg, RefArg);
	void		setTimeout(void);
	void		setWhichIcons(void);
	void		checkCancel(ULong&);
	void		abortConnection(long);
	void		delay(ULong);
	void		doGetPassword(void);
	void		verifyPassword(void);
	void		retryPassword(RefArg);
	void		cleanUpIfError(bool);
	bool		cleanUpIfStopping(bool);
	void		waitForDisconnect(void);

	NewtonErr	processBuiltinCommand(bool&);
	void		processCommand(bool&, bool&);
	void		processException(Exception*);

	void		stop(void);
	void		waitForStopToComplete(void);

	void		getPlatform(void);
	void		outOfMemory(void);

	void		doDisplaySlip(void);

	void		testMessage(void);
	void		testRefMessage(void);
	void		tossDataStructures(void);

	bool		waitAndLockDocker(void);
	void		unlockDocker(void);
	bool		getDockerLock(void);

	void		setState(eDockingState);
	void		getState(void);
	void		compatibilityHacks(void);

	void		refsEqual(RefArg, RefArg);
	void		framesEqual(RefArg, RefArg, ULong);

	void		finishSequence(short&, short);

// function/method execution
	void		callFunction(bool);

// protocol extensions
	void		readProtocolExtension(void);
	void		readRemoveProtocolExtension(void);
	NewtonErr	installProtocolExtension(RefArg inName, RefArg inFn, ULong inId);
	NewtonErr	removeProtocolExtension(RefArg inName, ULong inId);
	bool		checkProtocolExtension(EventType inCommand, bool& outArg2);
	bool		checkProtocolPatch(EventType inCommand, bool& outArg2);

// sync/backup
	void		writeSyncOptions(void);
	void		getSyncChanges(void);
	void		getBackupCursor(void);
	void		doRestorePatch(void);

// package loading
	void		readPackage(void);
	void		doRemovePackage(void);
	void		doRestorePackage(void);
	void		getPackageInfo(void);

// keyboard passthrough
	void		doKeyboardPassthrough(void);
	void		keyboardProcessCommand(void);

// import/export
	void		doImportParametersSlip(void);

// stores
	void		writeDefaultStore(void);
	void		writeStoreNames(void);
	void		setCurrentStore(bool);
	void		getCurrentStore(void);
	void		freeCurrentStore(void);
	void		setStoreSignature(void);
	void		setStoreToDefault(void);
	Ref		makeStoreFrame(RefArg inStore);
	void		reserveCurrentStore(RefArg);

// soups
	void		createSoup(void);
	void		createSoupFromSoupDef(void);
	void		setupSoup(void);
	void		setSoupInfoFrame(void);
	void		clearSoupDirty(void);
	void		getSoupIDCount(RefArg);
	void		verifySoup(void);
	void		setCurrentSoup(bool);
	void		readCurrentSoup(void);
	void		setSoupSignature(void);
	void		addChangedSoup(RefArg, ULong);
	void		soupChangedSinceLastBackup(void);
	void		sendSoup(void);
	void		backupSoup(void);

	void		writeSoupNames(void);
	void		writeSoupInfo(bool);
	void		writeIndexDescription(bool);
	void		writeInheritanceFrame(void);
	void		writeSoupIDs(void);
	void		writeChangedIDs(void);
	void		writeEntry(ULong, RefArg);
	void		writePatches(void);

// cursors
	void		remoteQuery(void);
	Ref		remoteGetCursor(void);
	void		remoteCursorGotoKey(void);
	void		remoteCursorMap(void);
	void		remoteCursorEntry(void);
	void		remoteCursorMove(void);
	void		remoteCursorNext(void);
	void		remoteCursorPrev(void);
	void		remoteCursorReset(void);
	void		remoteCursorResetToEnd(void);
	void		remoteCursorWhichEnd(void);
	void		remoteCursorCountEntries(void);
	void		remoteCursorFree(void);

	void		validateQuery(void);

// soup entries
	void		broadcastChanges(void);
	void		returnEntry(ULong);
	void		getEntryFromID(ULong);
	void		addEntry(bool);
	void		changeEntry(void);
	void		replaceEntryContents(RefArg);
	void		convertEntry(RefArg);
	void		shouldBackupEntry(RefArg);
	void		entriesEqual(RefArg);
	void		isDuplicateEntry(RefArg);
	void		deleteEntries(void);
	void		emptyOrDelete(ULong inEvtTag);

// reading
	size_t		bytesAvailable(bool);
	bool			readChunk(void * inBuf, size_t inLength, bool inNoMore);
	NewtonErr	readBytes(size_t& ioLength, RefVar inTarget);
	NewtonErr	readCommand(RefVar&, bool, bool);
	NewtonErr	readCommandData(RefVar& outData);
	NewtonErr	readResult(void);
	char *		readString(size_t inLength);
	NewtonErr	readResultString(void);
	void			readData(RefVar& outData);
	Ref			readRef(RefArg inStore);
	void			flushCommand(void);
	NewtonErr	flushCommandData(void);
	void			flushPadding(size_t inLength);
	// protocol exchange
	void		readInitiateDocking(void);
	void		readDesktopInfo(void);
	void		readPassword(void);
	void		readSourceVersion(void);

// writing
	NewtonErr	writeBytes(RefArg inTarget);
	void		pad(size_t inLength);
	void		writeLong(ULong, ULong);
	void		writeResult(NewtonErr inResult);
	void		writeString(const UniChar * inString);
	void		writeRef(ULong, RefArg);
	void		writeCommand(RefArg, RefArg, long, bool, ULong&);
	// protocol exchange
	void		writeNewtonName(void);
	void		writePassword(RefArg);

private:
	RefStruct			f0C;
	RefStruct			fTargetStore;		//+10
	RefStruct			fTargetSoup;		//+14
	RefStruct			f18;
	RefStruct			f1C;
	RefStruct			f20;
	RefStruct			f24;
	RefStruct			f28;
	bool					f2C;
	bool					f2D;
	UChar					f2E;
	bool					f2F;
	bool					f30;	// isReadingData
	bool					f31;	// isWritingData
	bool					fIsLocked;			//+32
	int					fVBOCompression;	//+34
	size_t				fLengthRead;		//+38
	size_t				fLengthWritten;	//+3C
	RefStruct			f40;
	EventType			fEvtTag;				//+44
	size_t				fEvtLength;			//+48
	ULong					fProtocolVersion;	//+4C
	NewtonErr			fError;				//+50
	CCursorArray *		fCursors;			//+54
	int					f58;
	int					f5C;
	int					f60;
	int					f64;
	int					f68;
	RefStruct			f6C;
	RefStruct			f70;
	ULong					f74;					//+74	desktop time
	ULong					f78;					//+78	our time
	CDockerDynArray *	f7C;						//+7C
	CDockerDynArray *	fRegdExtensionIds;	//+80
	RefStruct			fRegdExtensionFns;	//+84;
	RefStruct			fDesktopApps;		//+88
	SNewtNonce			fDesktopKey;		//+8C
	SNewtNonce			fNewtonKey;			//+94
	ULong					fSessionType;		//+A4
	ULong					fDesktopType;		//+A8
	bool					fAC;
	bool					fAD;
	bool					fAE;
	bool					fAF;
	bool					fB0;
	bool					fB1;
	bool					fIsSelectiveSync;	//+B2
	bool					fB3;
	bool					fB4;
};

#endif	/* __DOCKER_H */
