
DEFFRAME3(canonicalFakeContext,
	_nextArgFrame, NILREF,
	_parent, NILREF,
	_implementor, NILREF)

DEFFRAME5(codeBlockPrototype,
	class, MAKEPTR(SYMCodeBlock),
	instructions, NILREF,
	literals, NILREF,
	argFrame, NILREF,
	numArgs, NILREF)

DEFFRAME6(debugCodeBlockPrototype,
	class, MAKEPTR(SYMCodeBlock),
	instructions, NILREF,
	literals, NILREF,
	argFrame, NILREF,
	numArgs, NILREF,
	debuggerInfo, NILREF)

DEFFRAME3(cFunctionPrototype,
	class, MAKEPTR(SYMCFunction),
	funcPtr, NILREF,
	numArgs, NILREF)

DEFFRAME4(debugCFunctionPrototype,
	class, MAKEPTR(SYMCFunction),
	funcPtr, NILREF,
	numArgs, NILREF,
	docString, NILREF)

DEFFRAME2(canonicalPoint,
	x, MAKEINT(0),
	y, MAKEINT(0))

DEFFRAME4(canonicalRect,
	top, MAKEINT(0),
	left, MAKEINT(0),
	bottom, MAKEINT(0),
	right, MAKEINT(0))

DEFFRAME5(canonicalBitmapShape,
	class, MAKEPTR(SYMbitmap),
	bounds, NILREF,
	data, NILREF,
	colorData, NILREF,
	mask, NILREF)

DEFFRAME3(canonicalTextShape,
	class, MAKEPTR(SYMtext),
	bounds, NILREF,
	data, NILREF)

DEFFRAME2(canonicalCaretInfo,
	view, NILREF,
	info, NILREF)

DEFFRAME3(canonicalParaCaretInfo,
	class, MAKEPTR(SYMparaCaret),
	length, NILREF,
	offset, NILREF)

DEFFRAME4(canonicalGrayFontSpec,
	size, MAKEINT(0),
	face, MAKEINT(0),
	family, MAKEINT(0),
	color, NILREF)

DEFFRAME4(strokeBundle,
	strokes, NILREF,
	bounds, NILREF,
	startTime, MAKEINT(0),
	endTime, MAKEINT(0))

DEFFRAME4(canonicalCardInfo,
	cardInfoVersion, MAKEINT(0),
	totalSockets, MAKEINT(0),
	totalCards, MAKEINT(0),
	socketInfos, MAKEPTR(emptyArray))

DEFFRAME5(canonicalPackageCallbackInfo,
	packageName, NILREF,
	packageSize, MAKEINT(0),
	numberOfParts, MAKEINT(0),
	currentPartNumber, MAKEINT(0),
	amountRead, MAKEINT(0))

DEFFRAME8(canonicalPMIteratorPackageFrame,
	id, NILREF,
	size, NILREF,
	store, NILREF,
	pssId, NILREF,
	title, NILREF,
	version, NILREF,
	timestamp, NILREF,
	copyProtection, NILREF)

DEFFRAME10(canonicalFramePartInstallInfo,
	partType, NILREF,
	partFrame, NILREF,
	packageId, NILREF,
	packageName, NILREF,
	partIndex, NILREF,
	size, NILREF,
	packageType, NILREF,
	packageStyle, NILREF,
	deviceKind, NILREF,
	deviceNumber, NILREF)

DEFFRAME5(canonicalFramePartRemoveInfo,
	partType, NILREF,
	partFrame, NILREF,
	packageId, NILREF,
	partIndex, NILREF,
	packageStyle, NILREF)

DEFFRAME3(canonicalFramePartSavedObject,
	partFrame, NILREF,
	packageStyle, NILREF,
	removeCookie, NILREF)

DEFFRAME5(canonicalCurrentExport,
	name, NILREF,
	major, NILREF,
	minor, NILREF,
	refCount, NILREF,
	exportTable, NILREF)

DEFFRAME2(canonicalCurrentImport,
	importTable, NILREF,
	client, NILREF)

DEFFRAME4(canonicalPendingImport,
	name, NILREF,
	major, NILREF,
	minor, NILREF,
	client, NILREF)

DEFFRAME4(canonicalExportTableClient,
	name, NILREF,
	major, NILREF,
	minor, NILREF,
	client, NILREF)

DEFPROTOFRAME5(storePrototype,
	_parent, NILREF,
	_proto, NILREF,
	soups, NILREF,
	store, NILREF,
	version, NILREF)

DEFFRAME4(storePersistent,
	ephemerals, NILREF,
	name, NILREF,
	nameIndex, NILREF,
	signature, NILREF)

DEFFRAME7(plainSoupPersistent,
	class, MAKEPTR(SYMdiskSoup),
	lastUId, NILREF,
	signature, NILREF,
	indexes, NILREF,
	flags, NILREF,
	indexesModTime, NILREF,
	infoModTime, NILREF)

DEFPROTOFRAME10(plainSoupPrototype,
	_parent, NILREF,
	_proto, NILREF,
	class, MAKEPTR(SYMplainSoup),
	TStore, NILREF,
	storeObj, NILREF,
	theName, NILREF,
	cache, NILREF,
	indexObjects, NILREF,
	indexNextUId, NILREF,
	cursors, NILREF)

DEFFRAME(unionSoupPrototype,
	MAKEMAGICPTR(232))

DEFFRAME2(cursorPrototype,
	_parent, NILREF,
	TCursor, NILREF)

DEFFRAME4(indexDescPrototype,
	structure, MAKEPTR(SYMslot),
	path, MAKEPTR(SYM_uniqueId),
	type, MAKEPTR(SYMint),
	index, NILREF)

DEFFRAME1(canonicalGestaltVersion,
	version, NILREF)

DEFFRAME17(canonicalGestaltSystemInfo,
	manufacturer, NILREF,
	manufactureDate, NILREF,
	machineType, NILREF,
	ROMstage, NILREF,
	ROMversion, NILREF,
	ROMversionString, NILREF,
	RAMsize, NILREF,
	screenWidth, NILREF,
	screenHeight, NILREF,
	screenResolutionX, NILREF,
	screenResolutionY, NILREF,
	screenDepth, NILREF,
	patchVersion, NILREF,
	tabletResolutionX, NILREF,
	tabletResolutionY, NILREF,
	CPUtype, NILREF,
	CPUspeed, NILREF)

DEFFRAME2(canonicalGestaltRebootInfo,
	rebootReason, NILREF,
	rebootCount, NILREF)

DEFFRAME2(canonicalGestaltPatchInfo,
	fTotalPatchPageCount, MAKEINT(0),
	fPatch, NILREF)

DEFFRAME4(canonicalGestaltPatchInfoArrayElement,
	fPatchChecksum, NILREF,
	fPatchVersion, NILREF,
	fPatchPageCount, NILREF,
	fPatchFirstPageIndex, NILREF)

DEFFRAME10(canonicalGestaltRexInfoArrayElement,
	signatureA, NILREF,
	signatureB, NILREF,
	checksum, NILREF,
	headerVersion, NILREF,
	manufacturer, NILREF,
	version, NILREF,
	length, NILREF,
	id, NILREF,
	start, NILREF,
	count, NILREF)

DEFFRAME(command, MAKEMAGICPTR(433))
DEFFRAME(rcNoRecog, MAKEMAGICPTR(442))
DEFFRAME(rcPrefsConfig, MAKEMAGICPTR(443))
DEFFRAME(protoRecConfig, MAKEMAGICPTR(450))

DEFFRAME(canonicalPopup, MAKEMAGICPTR(35))

DEFFRAME(canonicalDate, MAKEMAGICPTR(32))
DEFFRAME(dateTimeStrSpecs, MAKEMAGICPTR(66))

DEFFRAME(assistFrames, MAKEMAGICPTR(8))
DEFFRAME(bootSound, MAKEMAGICPTR(16))
DEFFRAME(dictionaryList, MAKEMAGICPTR(70))
DEFFRAME(DSExceptions, MAKEMAGICPTR(113))
DEFFRAME(protoDictionary, MAKEMAGICPTR(171))
DEFFRAME(salutationSuffix, MAKEMAGICPTR(249))
DEFFRAME(stopWordList, MAKEMAGICPTR(270))
DEFFRAME(rootProto, MAKEMAGICPTR(287))
DEFFRAME(StartIAProgress, MAKEMAGICPTR(296))
DEFFRAME(addSound, MAKEMAGICPTR(303))
DEFFRAME(TickleIAProgress, MAKEMAGICPTR(306))
DEFFRAME(bootLogoBitmap, MAKEMAGICPTR(320))
DEFFRAME(canonicalDictRAMFrame, MAKEMAGICPTR(480))
DEFFRAME(IACancelAlert, MAKEMAGICPTR(811))
DEFFRAME(protoSoundFrame, MAKEMAGICPTR(849))

