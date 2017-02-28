#include "Objects.h"

#if !defined(forNTK)
extern "C" {
Ref FDefineGlobalConstant(RefArg inRcvr, RefArg inTag, RefArg inObj);
Ref FUnDefineGlobalConstant(RefArg inRcvr, RefArg inTag);
Ref FDefPureFn(RefArg inRcvr, RefArg inTag, RefArg inFn);
Ref FStuffHex(RefArg inRcvr, RefArg inHexStr, RefArg inClass);
Ref ReadStreamFile(RefArg inRcvr, RefArg inFilename);
}
#include "protoProtoEditor.h"

Ref FDefineGlobalConstant(RefArg inRcvr, RefArg inTag, RefArg inObj) { return NILREF; }
Ref FUnDefineGlobalConstant(RefArg inRcvr, RefArg inTag) { return NILREF; }
Ref FDefPureFn(RefArg inRcvr, RefArg inTag, RefArg inFn) { return NILREF; }
Ref FStuffHex(RefArg rcvr, RefArg inHexStr, RefArg inClass) { return NILREF; }
Ref ReadStreamFile(RefArg inRcvr, RefArg inFilename) { return NILREF; }

Ref Selection(RefArg inRcvr) { return NILREF; }
Ref SelectionOffset(RefArg inRcvr) { return NILREF; }
Ref SelectionLength(RefArg inRcvr) { return NILREF; }
Ref SetSelection(RefArg inRcvr, RefArg inStart, RefArg inLength) { return NILREF; }
Ref ReplaceSelection(RefArg inRcvr, RefArg inString) { return NILREF; }
Ref TextString(RefArg inRcvr) { return NILREF; }
Ref TextLength(RefArg inRcvr) { return NILREF; }
Ref FindLine(RefArg inRcvr, RefArg inOffset) { return NILREF; }
Ref LineStart(RefArg inRcvr, RefArg inLine) { return NILREF; }
Ref NumberOfLines(RefArg inRcvr) { return NILREF; }
Ref LineTop(RefArg inRcvr, RefArg inLine) { return NILREF; }
Ref LineBottom(RefArg inRcvr, RefArg inLine) { return NILREF; }
Ref LineBaseline(RefArg inRcvr, RefArg inLine) { return NILREF; }
Ref LeftEdge(RefArg inRcvr, RefArg inOffset) { return NILREF; }
Ref RightEdge(RefArg inRcvr, RefArg inOffset) { return NILREF; }
Ref PointToOffset(RefArg inRcvr, RefArg inH, RefArg inV) { return NILREF; }
Ref MapChars(RefArg inRcvr, RefArg inClosure, RefArg inStart, RefArg inLength) { return NILREF; }
Ref SearchChars(RefArg inRcvr, RefArg inClosure, RefArg inStart, RefArg inLength) { return NILREF; }
Ref Peek(RefArg inRcvr, RefArg inOffset) { return NILREF; }
Ref SetKeyHandler(RefArg inRcvr, RefArg inKey, RefArg inSymbol) { return NILREF; }
Ref GetKeyHandler(RefArg inRcvr, RefArg inKey) { return NILREF; }
Ref SetMetaBit(RefArg inRcvr) { return NILREF; }
Ref QuoteCharacter(RefArg inRcvr) { return NILREF; }
Ref CharacterClass(RefArg inRcvr, RefArg inChar) { return NILREF; }
Ref SetCharacterClass(RefArg inRcvr, RefArg inChar, RefArg inClass) { return NILREF; }
Ref TokenStart(RefArg inRcvr, RefArg inOffset) { return NILREF; }
Ref TokenEnd(RefArg inRcvr, RefArg inOffset) { return NILREF; }
Ref TellUser(RefArg inRcvr, RefArg inString) { return NILREF; }
Ref VisibleTop(RefArg inRcvr) { return NILREF; }
Ref VisibleLeft(RefArg inRcvr) { return NILREF; }
Ref VisibleHeight(RefArg inRcvr) { return NILREF; }
Ref VisibleWidth(RefArg inRcvr) { return NILREF; }
Ref SetVisibleTop(RefArg inRcvr, RefArg inNewTop) { return NILREF; }
Ref SetVisibleLeft(RefArg inRcvr, RefArg inNewLeft) { return NILREF; }
#endif

extern "C" {
Ref AddBookmark(RefArg rcvr);
Ref AddHistory(RefArg rcvr);
Ref AddInkMarks(RefArg rcvr);
Ref AddToContentArea(RefArg rcvr);
Ref Append(RefArg rcvr);
Ref AuthorData(RefArg rcvr);
Ref BookClosed(RefArg rcvr);
Ref BookTitle(RefArg rcvr);
Ref Bookmarks(RefArg rcvr);
Ref CFDispose(RefArg rcvr);
Ref CFInstantiate(RefArg rcvr);
Ref CFRecord(RefArg rcvr);
Ref CIAbort(RefArg rcvr);
Ref CIAccept(RefArg rcvr);
Ref CIBytesAvailable(RefArg rcvr);
Ref CICaller(RefArg rcvr);
Ref CIConnect(RefArg rcvr);
Ref CIDisconnect(RefArg rcvr);
Ref CIDispose(RefArg rcvr);
Ref CIDisposeLeavingTEndpoint(RefArg rcvr);
Ref CIFlushInput(RefArg rcvr);
Ref CIFlushOutput(RefArg rcvr);
Ref CIFlushPartial(RefArg rcvr);
Ref CIGetOptions(RefArg rcvr);
Ref CIInput(RefArg rcvr);
Ref CIInputAvailable(RefArg rcvr);
Ref CIInstantiate(RefArg rcvr);
Ref CIInstantiateFromEndpoint(RefArg rcvr);
Ref CIJustBind(RefArg rcvr);
Ref CIJustClose(RefArg rcvr);
Ref CIJustConnect(RefArg rcvr);
Ref CIJustDisconnect(RefArg rcvr);
Ref CIJustListen(RefArg rcvr);
Ref CIJustOpen(RefArg rcvr);
Ref CIJustRelease(RefArg rcvr);
Ref CIJustUnBind(RefArg rcvr);
Ref CIListen(RefArg rcvr);
Ref CINewAbort(RefArg rcvr);
Ref CINewAccept(RefArg rcvr);
Ref CINewBind(RefArg rcvr);
Ref CINewConnect(RefArg rcvr);
Ref CINewDisconnect(RefArg rcvr);
Ref CINewDispose(RefArg rcvr);
Ref CINewDisposeLeavingTEndpoint(RefArg rcvr);
Ref CINewDisposeLeavingTool(RefArg rcvr);
Ref CINewFlushInput(RefArg rcvr);
Ref CINewFlushPartial(RefArg rcvr);
Ref CINewInput(RefArg rcvr);
Ref CINewInstantiate(RefArg rcvr);
Ref CINewInstantiateFromEndpoint(RefArg rcvr);
Ref CINewListen(RefArg rcvr);
Ref CINewOption(RefArg rcvr);
Ref CINewOutput(RefArg rcvr);
Ref CINewPartial(RefArg rcvr);
Ref CINewSetInputSpec(RefArg rcvr);
Ref CINewSetState(RefArg rcvr);
Ref CINewState(RefArg rcvr);
Ref CINewUnBind(RefArg rcvr);
Ref CIOutput(RefArg rcvr);
Ref CIOutputDone(RefArg rcvr);
Ref CIOutputFrame(RefArg rcvr);
Ref CIPartial(RefArg rcvr);
Ref CIReadyForOutput(RefArg rcvr);
Ref CIReject(RefArg rcvr);
Ref CIRelease(RefArg rcvr);
Ref CIRequestsPending(RefArg rcvr);
Ref CISNewInstantiate(RefArg rcvr);
Ref CISNewInstantiateFromEndpoint(RefArg rcvr);
Ref CISStreamIn(RefArg rcvr);
Ref CISStreamOut(RefArg rcvr);
Ref CISetInputSpec(RefArg rcvr);
Ref CISetOptions(RefArg rcvr);
Ref CISetSync(RefArg rcvr);
Ref CIStartCCL(RefArg rcvr);
Ref CIState(RefArg rcvr);
Ref CIStopCCL(RefArg rcvr);
Ref CSDispose(RefArg rcvr);
Ref CSGetDefaultConfig(RefArg rcvr);
Ref CSInstantiate(RefArg rcvr);
Ref CSSetDefaultConfig(RefArg rcvr);
Ref CallOnlineServices(RefArg rcvr);
Ref CheezyIntersect(RefArg rcvr);
Ref CheezySubsumption(RefArg rcvr);
Ref CircleDistance(RefArg rcvr);
Ref CleanString(RefArg rcvr);
Ref CommonAncestors(RefArg rcvr);
Ref CompositeClass(RefArg rcvr);
Ref CoordinateToLatitude(RefArg rcvr);
Ref CoordinateToLongitude(RefArg rcvr);
Ref CorrectSelect(RefArg rcvr);
Ref CorrectWord(RefArg rcvr);
Ref CountPages(RefArg rcvr);
Ref CuFind(RefArg rcvr);
Ref CurrentBook(RefArg rcvr);
Ref CurrentKiosk(RefArg rcvr);
Ref CurrentPage(RefArg rcvr);
Ref DisconnectOnlineServices(RefArg rcvr);
Ref DumpDict(RefArg rcvr);
Ref EWBufferStart(RefArg rcvr);
Ref EWConnectToHost(RefArg rcvr);
Ref EWDecodePacket(RefArg rcvr);
Ref EWEscape(RefArg rcvr);
Ref EWFileToFrame(RefArg rcvr);
Ref EWFrameToFile(RefArg rcvr);
Ref EWGetData(RefArg rcvr);
Ref EWGetError(RefArg rcvr);
Ref EWGetMacHeader(RefArg rcvr);
Ref EWInstantiate(RefArg rcvr);
Ref EWOutput(RefArg rcvr);
Ref EWOutputAtom(RefArg rcvr);
Ref EWSend(RefArg rcvr);
Ref EWSetError(RefArg rcvr);
Ref EWUnEscape(RefArg rcvr);
Ref EWUniAtomHandler(RefArg rcvr);
Ref EWUploadFile(RefArg rcvr);
Ref FAbs(RefArg rcvr);
Ref FActivate1XPackage(RefArg rcvr);
Ref FAddCapitalizedEntry(RefArg rcvr);
Ref FAddUnitInfo(RefArg rcvr);
Ref FAddWordInfo(RefArg rcvr);
Ref FAdjustParagraph(RefArg rcvr);
Ref FAutoAdd(RefArg rcvr);
Ref FAutoRemove(RefArg rcvr);
Ref FCeiling(RefArg rcvr);
Ref FChildTemplateFromTopic(RefArg rcvr);
Ref FClearCorrectionInfo(RefArg rcvr);
Ref FClearWordInfoFlags(RefArg rcvr);
Ref FClickLetterShapes(RefArg rcvr);
Ref FCloseRemote(RefArg rcvr);
Ref FCollapseTopic(RefArg rcvr);
Ref FConnAbort(RefArg rcvr);
Ref FConnBytesAvailable(RefArg rcvr);
Ref FConnConnect(RefArg rcvr);
Ref FConnDesktopType(RefArg rcvr);
Ref FConnDoConnection(RefArg rcvr);
Ref FConnDoKeyboardPassthrough(RefArg rcvr);
Ref FConnFlushCommandData(RefArg rcvr);
Ref FConnGetCurrentStore(RefArg rcvr);
Ref FConnGetSyncChanges(RefArg rcvr);
Ref FConnInstallProtocolExtension(RefArg rcvr);
Ref FConnInstantiate(RefArg rcvr);
Ref FConnReadBytes(RefArg rcvr);
Ref FConnReadCommand(RefArg rcvr);
Ref FConnReadCommandData(RefArg rcvr);
Ref FConnReadCommandHeader(RefArg rcvr);
Ref FConnRemoveProtocolExtension(RefArg rcvr);
Ref FConnRetryPassword(RefArg rcvr);
Ref FConnSetState(RefArg rcvr);
Ref FConnStop(RefArg rcvr);
Ref FConnWriteBytes(RefArg rcvr);
Ref FConnWriteCommand(RefArg rcvr);
Ref FConnWriteCommandHeader(RefArg rcvr);
Ref FConnectionState(RefArg rcvr);
Ref FCountLetterShapes(RefArg rcvr);
Ref FDeActivate1XPackage(RefArg rcvr);
Ref FDeleteStoreObject(RefArg rcvr);
Ref FDestroyProtocol(RefArg rcvr);
Ref FDispatchProtocol(RefArg rcvr);
Ref FDoEntryLearning(RefArg rcvr);
Ref FDrawExpando(RefArg rcvr);
Ref FDrawLetterShapes(RefArg rcvr);
Ref FExpandTopic(RefArg rcvr);
Ref FExtractRange(RefArg rcvr);
Ref FFindDictionaryFrame(RefArg rcvr);
Ref FFindNewInfo(RefArg rcvr);
Ref FFindWordInfo(RefArg rcvr);
Ref FFloor(RefArg rcvr);
Ref FForLoop(RefArg rcvr);
Ref FGetBinaryCompander(RefArg rcvr);
Ref FGetBinaryCompanderData(RefArg rcvr);
Ref FGetBinaryStore(RefArg rcvr);
Ref FGetBinaryStoredSize(RefArg rcvr);
Ref FGetCapability(RefArg rcvr);
Ref FGetChannelInputGain(RefArg rcvr);
Ref FGetChannelVolume(RefArg rcvr);
Ref FGetHiliteWeight(RefArg rcvr);
Ref FGetPkgInfoFromPSSid(RefArg rcvr);
Ref FGetSiblingSlot(RefArg rcvr);
Ref FGetStoreCardSlot(RefArg rcvr);
Ref FGetStoreCardType(RefArg rcvr);
Ref FGetStoreObjectSize(RefArg rcvr);
Ref FGetValue(RefArg rcvr);
Ref FGetWordInfo(RefArg rcvr);
Ref FGetWordList(RefArg rcvr);
Ref FHasCapability(RefArg rcvr);
Ref FHasSiblingSlot(RefArg rcvr);
Ref FInsertRange(RefArg rcvr);
Ref FIsCollapsed(RefArg rcvr);
Ref FIsLargeBinary(RefArg rcvr);
Ref FKeyHelpSlipDraw(RefArg rcvr);
Ref FKeyHelpSlipSetup(RefArg rcvr);
Ref FKeyboardInputX(RefArg rcvr);
Ref FLBClearCache(RefArg rcvr);
Ref FListBottom(RefArg rcvr);
Ref FLocalVar(RefArg rcvr);
Ref FMakeDragRef(RefArg rcvr);
Ref FMakeWordInfo(RefArg rcvr);
Ref FMarkerBounds(RefArg rcvr);
Ref FMergeWordInfo(RefArg rcvr);
Ref FMoveWordFirst(RefArg rcvr);
Ref FMungeStyles(RefArg rcvr);
Ref FNewProtocol(RefArg rcvr);
Ref FNewStoreObject(RefArg rcvr);
Ref FNextClassInfo(RefArg rcvr);
Ref FOffsetCorrectionInfo(RefArg rcvr);
Ref FOpenRemote(RefArg rcvr);
Ref FPickViewGetScollerValues(RefArg rcvr);
Ref FPickViewScroll(RefArg rcvr);
Ref FProcessBuiltinCommand(RefArg rcvr);
Ref FProtocolImplementationName(RefArg rcvr);
Ref FProtocolInterfaceName(RefArg rcvr);
Ref FProtocolSignature(RefArg rcvr);
Ref FProtocolVersion(RefArg rcvr);
Ref FReadStoreObject(RefArg rcvr);
Ref FReboot(RefArg rcvr);
Ref FRemove1XPackage(RefArg rcvr);
Ref FRemoveCorrectionInfo(RefArg rcvr);
Ref FRemoveToggledEntries(RefArg rcvr);
Ref FResetLearningDefaults(RefArg rcvr);
Ref FSendCode(RefArg rcvr);
Ref FSetChannelInputGain(RefArg rcvr);
Ref FSetChannelVolume(RefArg rcvr);
Ref FSetHiliteWeight(RefArg rcvr);
Ref FSetStoreObjectSize(RefArg rcvr);
Ref FSetWordInfoFlags(RefArg rcvr);
Ref FSetWordList(RefArg rcvr);
Ref FSetupTetheredListener(RefArg rcvr);
Ref FSetupVisibleChildren(RefArg rcvr);
Ref FSignum(RefArg rcvr);
Ref FSoundClose(RefArg rcvr);
Ref FSoundIsActive(RefArg rcvr);
Ref FSoundIsPaused(RefArg rcvr);
Ref FSoundOpen(RefArg rcvr);
Ref FSoundPause(RefArg rcvr);
Ref FSoundSchedule(RefArg rcvr);
Ref FSoundStart(RefArg rcvr);
Ref FSoundStop(RefArg rcvr);
Ref FStoreAbort(RefArg rcvr);
Ref FStringToFrameMapper(RefArg rcvr);
Ref FSuckPackageFromBinary(RefArg rcvr);
Ref FSuckPackageFromEnpoint(RefArg rcvr);
Ref FSuckPackageOffDeskTop(RefArg rcvr);
Ref FTXChangeRangeRulers(RefArg rcvr);
Ref FTXChangeRangeRuns(RefArg rcvr);
Ref FTXCharToPoint(RefArg rcvr);
Ref FTXClear(RefArg rcvr);
Ref FTXCopy(RefArg rcvr);
Ref FTXCut(RefArg rcvr);
Ref FTXExternalize(RefArg rcvr);
Ref FTXFinderFindString(RefArg rcvr);
Ref FTXFinderGetCountCharacters(RefArg rcvr);
Ref FTXFinderGetRangeText(RefArg rcvr);
Ref FTXGetContinuousRun(RefArg rcvr);
Ref FTXGetCountCharacters(RefArg rcvr);
Ref FTXGetCountPages(RefArg rcvr);
Ref FTXGetHiliteRange(RefArg rcvr);
Ref FTXGetLineRange(RefArg rcvr);
Ref FTXGetParagraphRange(RefArg rcvr);
Ref FTXGetRangeData(RefArg rcvr);
Ref FTXGetScrollValues(RefArg rcvr);
Ref FTXGetTextViewRect(RefArg rcvr);
Ref FTXGetTotalHeight(RefArg rcvr);
Ref FTXGetTotalWidth(RefArg rcvr);
Ref FTXGetWordRange(RefArg rcvr);
Ref FTXHideRuler(RefArg rcvr);
Ref FTXInsertPageBreak(RefArg rcvr);
Ref FTXInternalize(RefArg rcvr);
Ref FTXIsModified(RefArg rcvr);
Ref FTXIsRulerShown(RefArg rcvr);
Ref FTXPaste(RefArg rcvr);
Ref FTXPointToChar(RefArg rcvr);
Ref FTXReplace(RefArg rcvr);
Ref FTXReplaceAll(RefArg rcvr);
Ref FTXScroll(RefArg rcvr);
Ref FTXSetDrawOrigin(RefArg rcvr);
Ref FTXSetGeometry(RefArg rcvr);
Ref FTXSetHiliteRange(RefArg rcvr);
Ref FTXSetReadOnly(RefArg rcvr);
Ref FTXSetStore(RefArg rcvr);
Ref FTXShowRuler(RefArg rcvr);
Ref FTXUpdateRulerInfo(RefArg rcvr);
Ref FTXViewFindString(RefArg rcvr);
Ref FTestWordInfoFlags(RefArg rcvr);
Ref FTopicBottom(RefArg rcvr);
Ref FTopicIndexToView(RefArg rcvr);
Ref FVisibleTopicIndex(RefArg rcvr);
Ref FWriteEntireStoreObject(RefArg rcvr);
Ref FWriteStoreObject(RefArg rcvr);
Ref FastStringLookup(RefArg rcvr);
Ref FavorAction(RefArg rcvr);
Ref FavorObject(RefArg rcvr);
Ref Fdim(RefArg rcvr);
Ref FindContentBySlot(RefArg rcvr);
Ref FindContentByValue(RefArg rcvr);
Ref FindPageByContent(RefArg rcvr);
Ref FindPageBySubject(RefArg rcvr);
Ref FindPageByValue(RefArg rcvr);
Ref FormatVertical(RefArg rcvr);
Ref GenPhoneTypeList(RefArg rcvr);
Ref GeneratePhrases(RefArg rcvr);
Ref GenerateSubstrings(RefArg rcvr);
Ref GetDictItem(RefArg rcvr);
Ref GetRelevantTemplates(RefArg rcvr);
Ref GlueStrings(RefArg rcvr);
Ref GuessAddressee(RefArg rcvr);
Ref HiliteBlock(RefArg rcvr);
Ref History(RefArg rcvr);
Ref IASmartCFLookup(RefArg rcvr);
Ref ISATest(RefArg rcvr);
Ref InkMarks(RefArg rcvr);
Ref LatitudeToCoordinate(RefArg rcvr);
Ref LayoutMeeting(RefArg rcvr);
Ref LexDateLookup(RefArg rcvr);
Ref LexPhoneLookup(RefArg rcvr);
Ref LexTimeLookup(RefArg rcvr);
Ref LongitudeToCoordinate(RefArg rcvr);
Ref MakeLowerCase(RefArg rcvr);
Ref MakePhrasalLexEntry(RefArg rcvr);
Ref MapSymToFrame(RefArg rcvr);
Ref PageContents(RefArg rcvr);
Ref PageScroll(RefArg rcvr);
Ref PageThumbnail(RefArg rcvr);
Ref PathToRoot(RefArg rcvr);
Ref PhoneIndexToValue(RefArg rcvr);
Ref PhoneStringToValue(RefArg rcvr);
Ref PhoneSymToIndex(RefArg rcvr);
Ref PhoneSymToString(RefArg rcvr);
Ref PhraseFilter(RefArg rcvr);
Ref PrefixP(RefArg rcvr);
Ref PrepBook(RefArg rcvr);
Ref PrepRecConfig(RefArg rcvr);
Ref PreviousPage(RefArg rcvr);
Ref ReFlow(RefArg rcvr);
Ref ReadDomainOptions(RefArg rcvr);
Ref ReflowPreflight(RefArg rcvr);
Ref RefreshTopics(RefArg rcvr);
Ref RegisterBookRef(RefArg rcvr);
Ref ScrollToCurrent(RefArg rcvr);
Ref SendAbort(RefArg rcvr);
Ref StartIRSniffing(RefArg rcvr);
Ref StopIRSniffing(RefArg rcvr);
Ref Store1XPackageToVBO(RefArg rcvr);
Ref StoreConvertSoupSortTables(RefArg rcvr);
Ref StorePackageRestore(RefArg rcvr);
Ref StoreSegmentedPackageRestore(RefArg rcvr);
Ref SuffixP(RefArg rcvr);
Ref TableLookup(RefArg rcvr);
Ref TurnToContent(RefArg rcvr);
Ref TurnToPage(RefArg rcvr);
Ref UnionSoupFlush(RefArg rcvr);
Ref UniqueAppendList(RefArg rcvr);
Ref UniqueAppendListGen(RefArg rcvr);
Ref UnmatchedWords(RefArg rcvr);
Ref UnregisterBookRef(RefArg rcvr);
Ref UpdateBookmarks(RefArg rcvr);
Ref UseTrainingDataForRecognition(RefArg rcvr);
Ref WhereIsBook(RefArg rcvr);
Ref ZapCancel(RefArg rcvr);
Ref ZapReceive(RefArg rcvr);
Ref ZapSend(RefArg rcvr);
Ref ZoomView(RefArg rcvr);

Ref FRecognizeInkWord(RefArg rcvr);
Ref FntpTetheredListener(RefArg rcvr);
Ref FSubstituteChars(RefArg rcvr);
Ref FMatchedChar(RefArg rcvr);
Ref FPathToRoot(RefArg rcvr);
Ref FCircleDistance(RefArg rcvr);
Ref FInTryString(RefArg rcvr);
Ref FGetTrainingData(RefArg rcvr);
Ref FSetLearningData(RefArg rcvr);
Ref FNewByName(RefArg rcvr);
Ref FRecognize(RefArg rcvr);
Ref FGetCardTypes(RefArg rcvr);
Ref FUniqueAppend(RefArg rcvr);
Ref FDisplay(RefArg rcvr);
Ref FGetTone(RefArg rcvr);
Ref FLoadOnlinePackage(RefArg rcvr);
Ref FGetZoneList(RefArg rcvr);
Ref FIsMockEntry(RefArg rcvr);
Ref FOtherViewInUse(RefArg rcvr);
Ref FExtractRichStringFromParaSlots(RefArg rcvr);
Ref FPidToPackageLite(RefArg rcvr);
Ref FSetRamParaData(RefArg rcvr);
Ref FComputeParagraphHeight(RefArg rcvr);
Ref FAddHist(RefArg rcvr);
Ref FWRecIsBeingUsed(RefArg rcvr);
Ref FDumpDict(RefArg rcvr);
Ref FGetPkgInfoFromPssid(RefArg rcvr);
Ref FHaveZones(RefArg rcvr);
Ref DSTagString(RefArg rcvr);
Ref FPolyContainsInk(RefArg rcvr);
Ref DSFilterStringsAux(RefArg rcvr);
Ref FDSDate(RefArg rcvr);
Ref FGetHiliteIndex(RefArg rcvr);
Ref FGuessAddressee(RefArg rcvr);
Ref FActivateTestAgent(RefArg rcvr);
Ref FFinishRecognizing(RefArg rcvr);
Ref FReadDomainOptions(RefArg rcvr);
Ref FFrameDirty(RefArg rcvr);
Ref FAppleTalkOpenCount(RefArg rcvr);
Ref FDrawStringShapes(RefArg rcvr);
Ref FGetFontFamilyNum(RefArg rcvr);
Ref FUnmountCard(RefArg rcvr);
Ref FTestMDropConnection(RefArg rcvr);
Ref FUnblockStrokes(RefArg rcvr);
Ref FBackLight(RefArg rcvr);
Ref FMakeRegion(RefArg rcvr);
Ref FMergeInk(RefArg rcvr);
Ref FStartIRSniffing(RefArg rcvr);
Ref FStripInk(RefArg rcvr);
Ref FEntryCopy(RefArg rcvr);
Ref FSmartStop(RefArg rcvr);
Ref FGetFontFace(RefArg rcvr);
Ref FCollect(RefArg rcvr);
Ref FSendOnline(RefArg rcvr);
Ref FQuickLookDone(RefArg rcvr);
Ref FTestReadTextFile(RefArg rcvr);
Ref Fmax(RefArg rcvr);
Ref FNextMeeting(RefArg rcvr);
Ref FGetHilitedTextItems(RefArg rcvr);
Ref FReFlow(RefArg rcvr);
Ref FStringAnnotate(RefArg rcvr);
Ref FGetStroke(RefArg rcvr);
Ref FMashLists(RefArg rcvr);
Ref FStringShorten(RefArg rcvr);
Ref DSScanForWackyName(RefArg rcvr);
Ref FGetDictItem(RefArg rcvr);
Ref FGetDefaultFont(RefArg rcvr);
Ref FScanPrevWordEnd(RefArg rcvr);
Ref FStrExactCompare(RefArg rcvr);
Ref FMapSymToFrame(RefArg rcvr);
Ref FPSSidToPkgRef(RefArg rcvr);
Ref FSetFontFamily(RefArg rcvr);
Ref FStringToFrame(RefArg rcvr);
Ref FTestWillCallExit(RefArg rcvr);
Ref FLookupCompletions(RefArg rcvr);
Ref FCompressStrokes(RefArg rcvr);
Ref FInitSerialDebugging(RefArg rcvr);
Ref FRestorePatchPackage(RefArg rcvr);
Ref FGetDictionaryData(RefArg rcvr);
Ref FGetMatchedEntries(RefArg rcvr);
Ref FGetChar(RefArg rcvr);
Ref FGetVBOCompander(RefArg rcvr);
Ref FOpenAppleTalk(RefArg rcvr);
Ref FRemoveAutoAdd(RefArg rcvr);
Ref FSendRemoteControlCode(RefArg rcvr);
Ref FGetInkAt(RefArg rcvr);
Ref FCompositeClass(RefArg rcvr);
Ref FBackupPatchPackage(RefArg rcvr);
Ref FPhoneIndexToValue(RefArg rcvr);
Ref FMakeLexEntry(RefArg rcvr);
Ref FNextInkIndex(RefArg rcvr);
Ref FStdioOn(RefArg rcvr);
Ref FConvertFromMP(RefArg rcvr);
Ref FStrWidth(RefArg rcvr);
Ref FSpellDocBegin(RefArg rcvr);
Ref FFontLeading(RefArg rcvr);
Ref FDisposeTrainingData(RefArg rcvr);
Ref FCloseAppleTalk(RefArg rcvr);
Ref FBackLightStatus(RefArg rcvr);
Ref FBubbleArraySlot(RefArg rcvr);
Ref FAppendList(RefArg rcvr);
Ref ForigPhrase(RefArg rcvr);
Ref FDeActivatePackage(RefArg rcvr);
Ref FTestGetParameterString(RefArg rcvr);
Ref FOpenRemoteControl(RefArg rcvr);
Ref FNewMockEntry(RefArg rcvr);
Ref FSetFontParms(RefArg rcvr);
Ref FSmartCFQuery(RefArg rcvr);
Ref FTestMSetParameterString(RefArg rcvr);
Ref FLookupWord(RefArg rcvr);
Ref FDrawDateLabels(RefArg rcvr);
Ref FScanWordEnd(RefArg rcvr);
Ref FGetViewID(RefArg rcvr);
Ref FCloseRemoteControl(RefArg rcvr);
Ref FRecognizePoly(RefArg rcvr);
Ref FArrayPos(RefArg rcvr);
Ref FDrawLetterScript(RefArg rcvr);
Ref FGetVBOStoredSize(RefArg rcvr);
Ref FNBPLookupCount(RefArg rcvr);
Ref FStringToNumber(RefArg rcvr);
Ref FGetInsertionStyle(RefArg rcvr);
Ref FParaContainsInk(RefArg rcvr);
Ref FntkDownload(RefArg rcvr);
Ref FGetSortID(RefArg rcvr);
Ref FSuffixP(RefArg rcvr);
Ref DSResolveString(RefArg rcvr);
Ref FIsProtocolPartInUse(RefArg rcvr);
Ref FCalcInkBounds(RefArg rcvr);
Ref FMoveCorrectionInfo(RefArg rcvr);
Ref FGetLetterWeights(RefArg rcvr);
Ref FPowerOffRingiSho(RefArg rcvr);
Ref FGetFontSize(RefArg rcvr);
Ref FGetMyZone(RefArg rcvr);
Ref FDeActivate1_2EXPackage(RefArg rcvr);
Ref FRecognizeTextInStyles(RefArg rcvr);
Ref FPhoneSymToString(RefArg rcvr);
Ref FDrawMeetingGrid(RefArg rcvr);
Ref FGetGlobals(RefArg rcvr);
Ref FGetNames(RefArg rcvr);
Ref FlocalVar(RefArg rcvr);
Ref FFontDescent(RefArg rcvr);
Ref FhasSiblingSlot(RefArg rcvr);
Ref FBlockStrokes(RefArg rcvr);
Ref FUnorderedLessOrEqual(RefArg rcvr);
Ref FAddDelayedAction(RefArg rcvr);
Ref FScanWordStart(RefArg rcvr);
Ref FViewAllowsInk(RefArg rcvr);
Ref FStyleArrayContainsInk(RefArg rcvr);
Ref FStrTruncate(RefArg rcvr);
Ref FNBPStartLookup(RefArg rcvr);
Ref FDecodeRichString(RefArg rcvr);
Ref FGetBlue(RefArg rcvr);
Ref FClassInfoRegistrySeed(RefArg rcvr);
Ref FGetExportTableClients(RefArg rcvr);
Ref FIsEqualTone(RefArg rcvr);
Ref FGetCardSlotStores(RefArg rcvr);
Ref FUnionPoint(RefArg rcvr);
Ref FPidToPackage(RefArg rcvr);
Ref Fand(RefArg rcvr);
Ref FFavorObject(RefArg rcvr);
Ref FEntryReplace(RefArg rcvr);
Ref FGetRelevantTemplates(RefArg rcvr);
Ref FEntryReplaceWithModTime(RefArg rcvr);
Ref FLayoutMeeting(RefArg rcvr);
Ref FNBPStart(RefArg rcvr);
Ref FStrFontWidth(RefArg rcvr);
Ref DSPrevSubStr(RefArg rcvr);
Ref FCallOnlineService(RefArg rcvr);
Ref DSFindPossibleLocation(RefArg rcvr);
Ref FMakeInk(RefArg rcvr);
Ref FGetDynamicValue(RefArg rcvr);
Ref FCorrectSelect(RefArg rcvr);
Ref FGetVar(RefArg rcvr);
Ref FMergeStrokes(RefArg rcvr);
Ref DSConstructSubjectLine(RefArg rcvr);
Ref FStrokesAfterUnit(RefArg rcvr);
Ref FTestMStartTestFrame(RefArg rcvr);
Ref FntpDownloadPackage(RefArg rcvr);
Ref FTestReportError(RefArg rcvr);
Ref FSpellDocEnd(RefArg rcvr);
Ref FVBOUndoChanges(RefArg rcvr);
Ref FSetGlobal(RefArg rcvr);
Ref Freal(RefArg rcvr);
Ref FValidateWord(RefArg rcvr);
Ref FGetLetterHilite(RefArg rcvr);
Ref FAddDeferredAction(RefArg rcvr);
Ref FStripRecognitionWordDiacritsOK(RefArg rcvr);
Ref FTestReportMessage(RefArg rcvr);
Ref FGetVBOCompanderData(RefArg rcvr);
Ref FFastStrLookup(RefArg rcvr);
Ref FRecognizePara(RefArg rcvr);
Ref FGetGreen(RefArg rcvr);
Ref FSetHiliteIndex(RefArg rcvr);
Ref For(RefArg rcvr);
Ref FInkConvert(RefArg rcvr);
Ref FPSSidToPid(RefArg rcvr);
Ref FSplitInkAt(RefArg rcvr);
Ref FNumInkWordsInRange(RefArg rcvr);
Ref FRepeatInfoToText(RefArg rcvr);
Ref FSubstr(RefArg rcvr);
Ref FWordUnitToWordInfo(RefArg rcvr);
Ref FmodalState(RefArg rcvr);
Ref FStdioOff(RefArg rcvr);
Ref FConnEntriesEqual(RefArg rcvr);
Ref FSpellSkip(RefArg rcvr);
Ref FScanNextWord(RefArg rcvr);
Ref FStringFormat(RefArg rcvr);
Ref FTextBounds(RefArg rcvr);
Ref FScaleShape(RefArg rcvr);
Ref FSetFontFace(RefArg rcvr);
Ref FMakeRichString(RefArg rcvr);
Ref FExpandInk(RefArg rcvr);
Ref FSendAbort(RefArg rcvr);
Ref FReflowPreflight(RefArg rcvr);
Ref FGetVBOStore(RefArg rcvr);
Ref FMapPtY(RefArg rcvr);
Ref FActivatePackage(RefArg rcvr);
Ref FClearTryString(RefArg rcvr);
Ref FFlushStrokes(RefArg rcvr);
Ref FStripRecognitionWord(RefArg rcvr);
Ref FTestReadDataFile(RefArg rcvr);
Ref DSAddLexiconFrame(RefArg rcvr);
Ref FViewWorksWithCombCorrector(RefArg rcvr);
Ref FInsertStyledText(RefArg rcvr);
Ref FGetIndexChar(RefArg rcvr);
Ref FConvertDictionaryData(RefArg rcvr);
Ref Fisa(RefArg rcvr);
Ref FPidToPkgRef(RefArg rcvr);
Ref FSmartConcat(RefArg rcvr);
Ref FAnimateSimpleStroke(RefArg rcvr);
Ref FDSTime(RefArg rcvr);
Ref FGetLetterIndex(RefArg rcvr);
Ref FUnmatchedWords(RefArg rcvr);
Ref Fstats(RefArg rcvr);
Ref FStrokeBundleToInkWord(RefArg rcvr);
Ref FGetLearningData(RefArg rcvr);
Ref FCompressStrokesToInk(RefArg rcvr);
Ref DSFindPossibleName(RefArg rcvr);
Ref DSFindPossiblePhone(RefArg rcvr);
Ref FGetSelectionStack(RefArg rcvr);
Ref FSetDictionaryData(RefArg rcvr);
Ref FDrawOriginal(RefArg rcvr);
Ref FSetChar(RefArg rcvr);
Ref FStopIRSniffing(RefArg rcvr);
Ref FMapCursor(RefArg rcvr);
Ref FConvertForMP(RefArg rcvr);
Ref FPhoneSymToIndex(RefArg rcvr);
Ref FSmartStart(RefArg rcvr);
Ref FTestMStartTestCase(RefArg rcvr);
Ref FGetRamParaData(RefArg rcvr);
Ref FSpellCheck(RefArg rcvr);
Ref FMakePict(RefArg rcvr);
Ref FUseTrainingDataForRecognition(RefArg rcvr);
Ref FMakeStrokeBundle(RefArg rcvr);
Ref FExpandUnit(RefArg rcvr);
Ref FStrCompare(RefArg rcvr);
Ref FGenPhoneTypes(RefArg rcvr);
Ref FTieViews(RefArg rcvr);
Ref FGetFontFamilySym(RefArg rcvr);
Ref FGetStrokePoint(RefArg rcvr);
Ref FEntryMove(RefArg rcvr);
Ref FGetStrokePointsArray(RefArg rcvr);
Ref FhasVariable(RefArg rcvr);
Ref FFindShape(RefArg rcvr);
Ref FActivate1_2EXPackage(RefArg rcvr);
Ref FCountUnitStrokes(RefArg rcvr);
Ref FMakeRoundRect(RefArg rcvr);
Ref FCoordinateToLongitude(RefArg rcvr);
Ref FLongitudeToCoordinate(RefArg rcvr);
Ref FPictToShape(RefArg rcvr);
Ref FReBoot(RefArg rcvr);
Ref FStringFilter(RefArg rcvr);
Ref FSubsume(RefArg rcvr);
Ref FRosettaExtension(RefArg rcvr);
Ref FCountPoints(RefArg rcvr);
Ref FAddDictionary(RefArg rcvr);
Ref FGetViewFlags(RefArg rcvr);
Ref FGetZoneFromName(RefArg rcvr);
Ref FRemove1_2EXPackage(RefArg rcvr);
Ref FGetRandomDictionaryWord(RefArg rcvr);
Ref Farray(RefArg rcvr);
Ref FCancelOnlineService(RefArg rcvr);
Ref FMakeWedge(RefArg rcvr);
Ref FPackRGB(RefArg rcvr);
Ref FMakeCompactFont(RefArg rcvr);
Ref FFavorAction(RefArg rcvr);
Ref FNBPGetLookupNames(RefArg rcvr);
Ref FGenSubStrings(RefArg rcvr);
Ref FSetLetterWeights(RefArg rcvr);
Ref FSetFontSize(RefArg rcvr);
Ref FPositionToTime(RefArg rcvr);
Ref FTimeToPosition(RefArg rcvr);
Ref FTestMGetReportMsg(RefArg rcvr);
Ref FDeactivateTestAgent(RefArg rcvr);
Ref FSetTimeHardware(RefArg rcvr);
Ref FFontHeight(RefArg rcvr);
Ref FGenPhrases(RefArg rcvr);
Ref FNBPGetCount(RefArg rcvr);
Ref FResetLetterDefaults(RefArg rcvr);
Ref FClearVBOCache(RefArg rcvr);
Ref FNBPGetNames(RefArg rcvr);
Ref FConnBuildStoreFrame(RefArg rcvr);
Ref FGetRed(RefArg rcvr);
Ref FGetOnlineEndpoint(RefArg rcvr);
Ref FCalcBundleBounds(RefArg rcvr);
Ref FTryStringLength(RefArg rcvr);
Ref FgetSiblingSlot(RefArg rcvr);
Ref FIntersect(RefArg rcvr);
Ref FHist(RefArg rcvr);
Ref FforLoop(RefArg rcvr);
Ref FGetStrokeBounds(RefArg rcvr);
Ref FSetInkerPenSize(RefArg rcvr);
Ref FTestExit(RefArg rcvr);
Ref FRecSettingsChanged(RefArg rcvr);
Ref FGetAllMeetings(RefArg rcvr);
Ref FIsInkChar(RefArg rcvr);
Ref FStripDiacriticals(RefArg rcvr);
Ref FGetEditArray(RefArg rcvr);
Ref FCoordinateToLatitude(RefArg rcvr);
Ref FLatitudeToCoordinate(RefArg rcvr);
Ref FMapRect(RefArg rcvr);
Ref FFontAscent(RefArg rcvr);
Ref FWeekNumber(RefArg rcvr);
Ref FGetMeetingTypeInfo(RefArg rcvr);
Ref FGetNextMeetingTime(RefArg rcvr);
Ref FGetCorrectionWordInfo(RefArg rcvr);
Ref FTestGetParameterArray(RefArg rcvr);
Ref FCorrectWord(RefArg rcvr);
Ref FSetSlot(RefArg rcvr);
Ref FTextBox(RefArg rcvr);
Ref FBuildRecConfig(RefArg rcvr);
Ref FNBPStopLookup(RefArg rcvr);
Ref Fdebug(RefArg rcvr);
Ref FGetBitmapInfo(RefArg rcvr);
Ref FMakeOval(RefArg rcvr);
Ref FMungeBitmap(RefArg rcvr);
Ref FSetOnlineDisconnect(RefArg rcvr);
Ref FClickLetterScript(RefArg rcvr);
Ref FGetRangeText(RefArg rcvr);
Ref FSpellCorrect(RefArg rcvr);
Ref FSetPath(RefArg rcvr);
Ref FCommonAncestors(RefArg rcvr);
Ref FSetLetterHilite(RefArg rcvr);
Ref FLower(RefArg rcvr);
Ref FStrAssoc(RefArg rcvr);
Ref FAddAutoAdd(RefArg rcvr);
Ref FCountStrokes(RefArg rcvr);
Ref FAddInk(RefArg rcvr);
Ref FObjectPid(RefArg rcvr);
Ref FGetShapeInfo(RefArg rcvr);
Ref FMungeShape(RefArg rcvr);
Ref FGetDictionary(RefArg rcvr);
Ref Fmin(RefArg rcvr);
Ref FMakePolygon(RefArg rcvr);
Ref FPrevMeeting(RefArg rcvr);
Ref FTriggerWordRecognition(RefArg rcvr);
Ref FMeasuredNumberStr(RefArg rcvr);
Ref FAddTryString(RefArg rcvr);
Ref FNBPStop(RefArg rcvr);
Ref FPhoneStringToValue(RefArg rcvr);
Ref FClassInfoRegistryNext(RefArg rcvr);
Ref FRunInitScripts(RefArg rcvr);
Ref FTestFlushReportQueue(RefArg rcvr);
Ref FAppend(RefArg rcvr);
Ref FDESCreatePasswordKey(RefArg rcvr);
Ref FGetAllMeetingsUnique(RefArg rcvr);
Ref FDSPhone(RefArg rcvr);
Ref FConnectPassthruKeyboard(RefArg rcvr);
Ref FntkListener(RefArg rcvr);
Ref FMapPtX(RefArg rcvr);
Ref FCountLetters(RefArg rcvr);
Ref FViewAllowsInkWords(RefArg rcvr);
Ref FPreInitSerialDebugging(RefArg rcvr);
Ref FPointsArrayToStroke(RefArg rcvr);
Ref FClassInfoByName(RefArg rcvr);
Ref FDisconnectOnlineService(RefArg rcvr);
Ref FTranslate(RefArg rcvr);
Ref FDoCursiveTraining(RefArg rcvr);
Ref FPrefixP(RefArg rcvr);
Ref FUseWRec(RefArg rcvr);
Ref FReadCursiveOptions(RefArg rcvr);
Ref DSFilterStrings(RefArg rcvr);
Ref FBootSucceeded(RefArg rcvr);
Ref FGetMeetingIcon(RefArg rcvr);
}


Ref AddBookmark(RefArg rcvr) { return NILREF; }
Ref AddHistory(RefArg rcvr) { return NILREF; }
Ref AddInkMarks(RefArg rcvr) { return NILREF; }
Ref AddToContentArea(RefArg rcvr) { return NILREF; }
Ref Append(RefArg rcvr) { return NILREF; }
Ref AuthorData(RefArg rcvr) { return NILREF; }
Ref BookClosed(RefArg rcvr) { return NILREF; }
Ref BookTitle(RefArg rcvr) { return NILREF; }
Ref Bookmarks(RefArg rcvr) { return NILREF; }
Ref CFDispose(RefArg rcvr) { return NILREF; }
Ref CFInstantiate(RefArg rcvr) { return NILREF; }
Ref CFRecord(RefArg rcvr) { return NILREF; }
Ref CIAbort(RefArg rcvr) { return NILREF; }
Ref CIAccept(RefArg rcvr) { return NILREF; }
Ref CIBytesAvailable(RefArg rcvr) { return NILREF; }
Ref CICaller(RefArg rcvr) { return NILREF; }
Ref CIConnect(RefArg rcvr) { return NILREF; }
Ref CIDisconnect(RefArg rcvr) { return NILREF; }
Ref CIDispose(RefArg rcvr) { return NILREF; }
Ref CIDisposeLeavingTEndpoint(RefArg rcvr) { return NILREF; }
Ref CIFlushInput(RefArg rcvr) { return NILREF; }
Ref CIFlushOutput(RefArg rcvr) { return NILREF; }
Ref CIFlushPartial(RefArg rcvr) { return NILREF; }
Ref CIGetOptions(RefArg rcvr) { return NILREF; }
Ref CIInput(RefArg rcvr) { return NILREF; }
Ref CIInputAvailable(RefArg rcvr) { return NILREF; }
Ref CIInstantiate(RefArg rcvr) { return NILREF; }
Ref CIInstantiateFromEndpoint(RefArg rcvr) { return NILREF; }
Ref CIJustBind(RefArg rcvr) { return NILREF; }
Ref CIJustClose(RefArg rcvr) { return NILREF; }
Ref CIJustConnect(RefArg rcvr) { return NILREF; }
Ref CIJustDisconnect(RefArg rcvr) { return NILREF; }
Ref CIJustListen(RefArg rcvr) { return NILREF; }
Ref CIJustOpen(RefArg rcvr) { return NILREF; }
Ref CIJustRelease(RefArg rcvr) { return NILREF; }
Ref CIJustUnBind(RefArg rcvr) { return NILREF; }
Ref CIListen(RefArg rcvr) { return NILREF; }
Ref CINewAbort(RefArg rcvr) { return NILREF; }
Ref CINewAccept(RefArg rcvr) { return NILREF; }
Ref CINewBind(RefArg rcvr) { return NILREF; }
Ref CINewConnect(RefArg rcvr) { return NILREF; }
Ref CINewDisconnect(RefArg rcvr) { return NILREF; }
Ref CINewDispose(RefArg rcvr) { return NILREF; }
Ref CINewDisposeLeavingTEndpoint(RefArg rcvr) { return NILREF; }
Ref CINewDisposeLeavingTool(RefArg rcvr) { return NILREF; }
Ref CINewFlushInput(RefArg rcvr) { return NILREF; }
Ref CINewFlushPartial(RefArg rcvr) { return NILREF; }
Ref CINewInput(RefArg rcvr) { return NILREF; }
Ref CINewInstantiate(RefArg rcvr) { return NILREF; }
Ref CINewInstantiateFromEndpoint(RefArg rcvr) { return NILREF; }
Ref CINewListen(RefArg rcvr) { return NILREF; }
Ref CINewOption(RefArg rcvr) { return NILREF; }
Ref CINewOutput(RefArg rcvr) { return NILREF; }
Ref CINewPartial(RefArg rcvr) { return NILREF; }
Ref CINewSetInputSpec(RefArg rcvr) { return NILREF; }
Ref CINewSetState(RefArg rcvr) { return NILREF; }
Ref CINewState(RefArg rcvr) { return NILREF; }
Ref CINewUnBind(RefArg rcvr) { return NILREF; }
Ref CIOutput(RefArg rcvr) { return NILREF; }
Ref CIOutputDone(RefArg rcvr) { return NILREF; }
Ref CIOutputFrame(RefArg rcvr) { return NILREF; }
Ref CIPartial(RefArg rcvr) { return NILREF; }
Ref CIReadyForOutput(RefArg rcvr) { return NILREF; }
Ref CIReject(RefArg rcvr) { return NILREF; }
Ref CIRelease(RefArg rcvr) { return NILREF; }
Ref CIRequestsPending(RefArg rcvr) { return NILREF; }
Ref CISNewInstantiate(RefArg rcvr) { return NILREF; }
Ref CISNewInstantiateFromEndpoint(RefArg rcvr) { return NILREF; }
Ref CISStreamIn(RefArg rcvr) { return NILREF; }
Ref CISStreamOut(RefArg rcvr) { return NILREF; }
Ref CISetInputSpec(RefArg rcvr) { return NILREF; }
Ref CISetOptions(RefArg rcvr) { return NILREF; }
Ref CISetSync(RefArg rcvr) { return NILREF; }
Ref CIStartCCL(RefArg rcvr) { return NILREF; }
Ref CIState(RefArg rcvr) { return NILREF; }
Ref CIStopCCL(RefArg rcvr) { return NILREF; }
Ref CSDispose(RefArg rcvr) { return NILREF; }
Ref CSGetDefaultConfig(RefArg rcvr) { return NILREF; }
Ref CSInstantiate(RefArg rcvr) { return NILREF; }
Ref CSSetDefaultConfig(RefArg rcvr) { return NILREF; }
Ref CallOnlineServices(RefArg rcvr) { return NILREF; }
Ref CheezyIntersect(RefArg rcvr) { return NILREF; }
Ref CheezySubsumption(RefArg rcvr) { return NILREF; }
Ref CircleDistance(RefArg rcvr) { return NILREF; }
Ref CleanString(RefArg rcvr) { return NILREF; }
Ref CommonAncestors(RefArg rcvr) { return NILREF; }
Ref CompositeClass(RefArg rcvr) { return NILREF; }
Ref CoordinateToLatitude(RefArg rcvr) { return NILREF; }
Ref CoordinateToLongitude(RefArg rcvr) { return NILREF; }
Ref CorrectSelect(RefArg rcvr) { return NILREF; }
Ref CorrectWord(RefArg rcvr) { return NILREF; }
Ref CountPages(RefArg rcvr) { return NILREF; }
Ref CuFind(RefArg rcvr) { return NILREF; }
Ref CurrentBook(RefArg rcvr) { return NILREF; }
Ref CurrentKiosk(RefArg rcvr) { return NILREF; }
Ref CurrentPage(RefArg rcvr) { return NILREF; }
Ref DisconnectOnlineServices(RefArg rcvr) { return NILREF; }
Ref DumpDict(RefArg rcvr) { return NILREF; }
Ref EWBufferStart(RefArg rcvr) { return NILREF; }
Ref EWConnectToHost(RefArg rcvr) { return NILREF; }
Ref EWDecodePacket(RefArg rcvr) { return NILREF; }
Ref EWEscape(RefArg rcvr) { return NILREF; }
Ref EWFileToFrame(RefArg rcvr) { return NILREF; }
Ref EWFrameToFile(RefArg rcvr) { return NILREF; }
Ref EWGetData(RefArg rcvr) { return NILREF; }
Ref EWGetError(RefArg rcvr) { return NILREF; }
Ref EWGetMacHeader(RefArg rcvr) { return NILREF; }
Ref EWInstantiate(RefArg rcvr) { return NILREF; }
Ref EWOutput(RefArg rcvr) { return NILREF; }
Ref EWOutputAtom(RefArg rcvr) { return NILREF; }
Ref EWSend(RefArg rcvr) { return NILREF; }
Ref EWSetError(RefArg rcvr) { return NILREF; }
Ref EWUnEscape(RefArg rcvr) { return NILREF; }
Ref EWUniAtomHandler(RefArg rcvr) { return NILREF; }
Ref EWUploadFile(RefArg rcvr) { return NILREF; }
Ref FAbs(RefArg rcvr) { return NILREF; }
Ref FActivate1XPackage(RefArg rcvr) { return NILREF; }
Ref FAddCapitalizedEntry(RefArg rcvr) { return NILREF; }
Ref FAddUnitInfo(RefArg rcvr) { return NILREF; }
Ref FAddWordInfo(RefArg rcvr) { return NILREF; }
Ref FAdjustParagraph(RefArg rcvr) { return NILREF; }
Ref FAutoAdd(RefArg rcvr) { return NILREF; }
Ref FAutoRemove(RefArg rcvr) { return NILREF; }
Ref FCeiling(RefArg rcvr) { return NILREF; }
Ref FChildTemplateFromTopic(RefArg rcvr) { return NILREF; }
Ref FClearCorrectionInfo(RefArg rcvr) { return NILREF; }
Ref FClearWordInfoFlags(RefArg rcvr) { return NILREF; }
Ref FClickLetterShapes(RefArg rcvr) { return NILREF; }
Ref FCloseRemote(RefArg rcvr) { return NILREF; }
Ref FCollapseTopic(RefArg rcvr) { return NILREF; }
Ref FConnAbort(RefArg rcvr) { return NILREF; }
Ref FConnBytesAvailable(RefArg rcvr) { return NILREF; }
Ref FConnConnect(RefArg rcvr) { return NILREF; }
Ref FConnDesktopType(RefArg rcvr) { return NILREF; }
Ref FConnDoConnection(RefArg rcvr) { return NILREF; }
Ref FConnDoKeyboardPassthrough(RefArg rcvr) { return NILREF; }
Ref FConnFlushCommandData(RefArg rcvr) { return NILREF; }
Ref FConnGetCurrentStore(RefArg rcvr) { return NILREF; }
Ref FConnGetSyncChanges(RefArg rcvr) { return NILREF; }
Ref FConnInstallProtocolExtension(RefArg rcvr) { return NILREF; }
Ref FConnInstantiate(RefArg rcvr) { return NILREF; }
Ref FConnReadBytes(RefArg rcvr) { return NILREF; }
Ref FConnReadCommand(RefArg rcvr) { return NILREF; }
Ref FConnReadCommandData(RefArg rcvr) { return NILREF; }
Ref FConnReadCommandHeader(RefArg rcvr) { return NILREF; }
Ref FConnRemoveProtocolExtension(RefArg rcvr) { return NILREF; }
Ref FConnRetryPassword(RefArg rcvr) { return NILREF; }
Ref FConnSetState(RefArg rcvr) { return NILREF; }
Ref FConnStop(RefArg rcvr) { return NILREF; }
Ref FConnWriteBytes(RefArg rcvr) { return NILREF; }
Ref FConnWriteCommand(RefArg rcvr) { return NILREF; }
Ref FConnWriteCommandHeader(RefArg rcvr) { return NILREF; }
Ref FConnectionState(RefArg rcvr) { return NILREF; }
Ref FCountLetterShapes(RefArg rcvr) { return NILREF; }
Ref FDeActivate1XPackage(RefArg rcvr) { return NILREF; }
Ref FDeleteStoreObject(RefArg rcvr) { return NILREF; }
Ref FDestroyProtocol(RefArg rcvr) { return NILREF; }
Ref FDispatchProtocol(RefArg rcvr) { return NILREF; }
Ref FDoEntryLearning(RefArg rcvr) { return NILREF; }
Ref FDrawExpando(RefArg rcvr) { return NILREF; }
Ref FDrawLetterShapes(RefArg rcvr) { return NILREF; }
Ref FExpandTopic(RefArg rcvr) { return NILREF; }
Ref FExtractRange(RefArg rcvr) { return NILREF; }
Ref FFindDictionaryFrame(RefArg rcvr) { return NILREF; }
Ref FFindNewInfo(RefArg rcvr) { return NILREF; }
Ref FFindWordInfo(RefArg rcvr) { return NILREF; }
Ref FFloor(RefArg rcvr) { return NILREF; }
Ref FForLoop(RefArg rcvr) { return NILREF; }
Ref FGetBinaryCompander(RefArg rcvr) { return NILREF; }
Ref FGetBinaryCompanderData(RefArg rcvr) { return NILREF; }
Ref FGetBinaryStore(RefArg rcvr) { return NILREF; }
Ref FGetBinaryStoredSize(RefArg rcvr) { return NILREF; }
Ref FGetCapability(RefArg rcvr) { return NILREF; }
Ref FGetChannelInputGain(RefArg rcvr) { return NILREF; }
Ref FGetChannelVolume(RefArg rcvr) { return NILREF; }
Ref FGetHiliteWeight(RefArg rcvr) { return NILREF; }
Ref FGetPkgInfoFromPSSid(RefArg rcvr) { return NILREF; }
Ref FGetSiblingSlot(RefArg rcvr) { return NILREF; }
Ref FGetStoreCardSlot(RefArg rcvr) { return NILREF; }
Ref FGetStoreCardType(RefArg rcvr) { return NILREF; }
Ref FGetStoreObjectSize(RefArg rcvr) { return NILREF; }
Ref FGetValue(RefArg rcvr) { return NILREF; }
Ref FGetWordInfo(RefArg rcvr) { return NILREF; }
Ref FGetWordList(RefArg rcvr) { return NILREF; }
Ref FHasCapability(RefArg rcvr) { return NILREF; }
Ref FHasSiblingSlot(RefArg rcvr) { return NILREF; }
Ref FInsertRange(RefArg rcvr) { return NILREF; }
Ref FIsCollapsed(RefArg rcvr) { return NILREF; }
Ref FIsLargeBinary(RefArg rcvr) { return NILREF; }
Ref FKeyHelpSlipDraw(RefArg rcvr) { return NILREF; }
Ref FKeyHelpSlipSetup(RefArg rcvr) { return NILREF; }
Ref FKeyboardInputX(RefArg rcvr) { return NILREF; }
Ref FLBClearCache(RefArg rcvr) { return NILREF; }
Ref FListBottom(RefArg rcvr) { return NILREF; }
Ref FLocalVar(RefArg rcvr) { return NILREF; }
Ref FMakeDragRef(RefArg rcvr) { return NILREF; }
Ref FMakeWordInfo(RefArg rcvr) { return NILREF; }
Ref FMarkerBounds(RefArg rcvr) { return NILREF; }
Ref FMergeWordInfo(RefArg rcvr) { return NILREF; }
Ref FMoveWordFirst(RefArg rcvr) { return NILREF; }
Ref FMungeStyles(RefArg rcvr) { return NILREF; }
Ref FNewProtocol(RefArg rcvr) { return NILREF; }
Ref FNewStoreObject(RefArg rcvr) { return NILREF; }
Ref FNextClassInfo(RefArg rcvr) { return NILREF; }
Ref FOffsetCorrectionInfo(RefArg rcvr) { return NILREF; }
Ref FOpenRemote(RefArg rcvr) { return NILREF; }
Ref FPickViewGetScollerValues(RefArg rcvr) { return NILREF; }
Ref FPickViewScroll(RefArg rcvr) { return NILREF; }
Ref FProcessBuiltinCommand(RefArg rcvr) { return NILREF; }
Ref FProtocolImplementationName(RefArg rcvr) { return NILREF; }
Ref FProtocolInterfaceName(RefArg rcvr) { return NILREF; }
Ref FProtocolSignature(RefArg rcvr) { return NILREF; }
Ref FProtocolVersion(RefArg rcvr) { return NILREF; }
Ref FReadStoreObject(RefArg rcvr) { return NILREF; }
Ref FReboot(RefArg rcvr) { return NILREF; }
Ref FRemove1XPackage(RefArg rcvr) { return NILREF; }
Ref FRemoveCorrectionInfo(RefArg rcvr) { return NILREF; }
Ref FRemoveToggledEntries(RefArg rcvr) { return NILREF; }
Ref FResetLearningDefaults(RefArg rcvr) { return NILREF; }
Ref FSendCode(RefArg rcvr) { return NILREF; }
Ref FSetChannelInputGain(RefArg rcvr) { return NILREF; }
Ref FSetChannelVolume(RefArg rcvr) { return NILREF; }
Ref FSetHiliteWeight(RefArg rcvr) { return NILREF; }
Ref FSetStoreObjectSize(RefArg rcvr) { return NILREF; }
Ref FSetWordInfoFlags(RefArg rcvr) { return NILREF; }
Ref FSetWordList(RefArg rcvr) { return NILREF; }
Ref FSetupTetheredListener(RefArg rcvr) { return NILREF; }
Ref FSetupVisibleChildren(RefArg rcvr) { return NILREF; }
Ref FSignum(RefArg rcvr) { return NILREF; }
Ref FSoundClose(RefArg rcvr) { return NILREF; }
Ref FSoundIsActive(RefArg rcvr) { return NILREF; }
Ref FSoundIsPaused(RefArg rcvr) { return NILREF; }
Ref FSoundOpen(RefArg rcvr) { return NILREF; }
Ref FSoundPause(RefArg rcvr) { return NILREF; }
Ref FSoundSchedule(RefArg rcvr) { return NILREF; }
Ref FSoundStart(RefArg rcvr) { return NILREF; }
Ref FSoundStop(RefArg rcvr) { return NILREF; }
Ref FStoreAbort(RefArg rcvr) { return NILREF; }
Ref FStringToFrameMapper(RefArg rcvr) { return NILREF; }
Ref FSuckPackageFromBinary(RefArg rcvr) { return NILREF; }
Ref FSuckPackageFromEnpoint(RefArg rcvr) { return NILREF; }
Ref FSuckPackageOffDeskTop(RefArg rcvr) { return NILREF; }
Ref FTXChangeRangeRulers(RefArg rcvr) { return NILREF; }
Ref FTXChangeRangeRuns(RefArg rcvr) { return NILREF; }
Ref FTXCharToPoint(RefArg rcvr) { return NILREF; }
Ref FTXClear(RefArg rcvr) { return NILREF; }
Ref FTXCopy(RefArg rcvr) { return NILREF; }
Ref FTXCut(RefArg rcvr) { return NILREF; }
Ref FTXExternalize(RefArg rcvr) { return NILREF; }
Ref FTXFinderFindString(RefArg rcvr) { return NILREF; }
Ref FTXFinderGetCountCharacters(RefArg rcvr) { return NILREF; }
Ref FTXFinderGetRangeText(RefArg rcvr) { return NILREF; }
Ref FTXGetContinuousRun(RefArg rcvr) { return NILREF; }
Ref FTXGetCountCharacters(RefArg rcvr) { return NILREF; }
Ref FTXGetCountPages(RefArg rcvr) { return NILREF; }
Ref FTXGetHiliteRange(RefArg rcvr) { return NILREF; }
Ref FTXGetLineRange(RefArg rcvr) { return NILREF; }
Ref FTXGetParagraphRange(RefArg rcvr) { return NILREF; }
Ref FTXGetRangeData(RefArg rcvr) { return NILREF; }
Ref FTXGetScrollValues(RefArg rcvr) { return NILREF; }
Ref FTXGetTextViewRect(RefArg rcvr) { return NILREF; }
Ref FTXGetTotalHeight(RefArg rcvr) { return NILREF; }
Ref FTXGetTotalWidth(RefArg rcvr) { return NILREF; }
Ref FTXGetWordRange(RefArg rcvr) { return NILREF; }
Ref FTXHideRuler(RefArg rcvr) { return NILREF; }
Ref FTXInsertPageBreak(RefArg rcvr) { return NILREF; }
Ref FTXInternalize(RefArg rcvr) { return NILREF; }
Ref FTXIsModified(RefArg rcvr) { return NILREF; }
Ref FTXIsRulerShown(RefArg rcvr) { return NILREF; }
Ref FTXPaste(RefArg rcvr) { return NILREF; }
Ref FTXPointToChar(RefArg rcvr) { return NILREF; }
Ref FTXReplace(RefArg rcvr) { return NILREF; }
Ref FTXReplaceAll(RefArg rcvr) { return NILREF; }
Ref FTXScroll(RefArg rcvr) { return NILREF; }
Ref FTXSetDrawOrigin(RefArg rcvr) { return NILREF; }
Ref FTXSetGeometry(RefArg rcvr) { return NILREF; }
Ref FTXSetHiliteRange(RefArg rcvr) { return NILREF; }
Ref FTXSetReadOnly(RefArg rcvr) { return NILREF; }
Ref FTXSetStore(RefArg rcvr) { return NILREF; }
Ref FTXShowRuler(RefArg rcvr) { return NILREF; }
Ref FTXUpdateRulerInfo(RefArg rcvr) { return NILREF; }
Ref FTXViewFindString(RefArg rcvr) { return NILREF; }
Ref FTestWordInfoFlags(RefArg rcvr) { return NILREF; }
Ref FTopicBottom(RefArg rcvr) { return NILREF; }
Ref FTopicIndexToView(RefArg rcvr) { return NILREF; }
Ref FVisibleTopicIndex(RefArg rcvr) { return NILREF; }
Ref FWriteEntireStoreObject(RefArg rcvr) { return NILREF; }
Ref FWriteStoreObject(RefArg rcvr) { return NILREF; }
Ref FastStringLookup(RefArg rcvr) { return NILREF; }
Ref FavorAction(RefArg rcvr) { return NILREF; }
Ref FavorObject(RefArg rcvr) { return NILREF; }
Ref Fdim(RefArg rcvr) { return NILREF; }
Ref FindContentBySlot(RefArg rcvr) { return NILREF; }
Ref FindContentByValue(RefArg rcvr) { return NILREF; }
Ref FindPageByContent(RefArg rcvr) { return NILREF; }
Ref FindPageBySubject(RefArg rcvr) { return NILREF; }
Ref FindPageByValue(RefArg rcvr) { return NILREF; }
Ref FormatVertical(RefArg rcvr) { return NILREF; }
Ref GenPhoneTypeList(RefArg rcvr) { return NILREF; }
Ref GeneratePhrases(RefArg rcvr) { return NILREF; }
Ref GenerateSubstrings(RefArg rcvr) { return NILREF; }
Ref GetDictItem(RefArg rcvr) { return NILREF; }
Ref GetRelevantTemplates(RefArg rcvr) { return NILREF; }
Ref GlueStrings(RefArg rcvr) { return NILREF; }
Ref GuessAddressee(RefArg rcvr) { return NILREF; }
Ref HiliteBlock(RefArg rcvr) { return NILREF; }
Ref History(RefArg rcvr) { return NILREF; }
Ref IASmartCFLookup(RefArg rcvr) { return NILREF; }
Ref ISATest(RefArg rcvr) { return NILREF; }
Ref InkMarks(RefArg rcvr) { return NILREF; }
Ref LatitudeToCoordinate(RefArg rcvr) { return NILREF; }
Ref LayoutMeeting(RefArg rcvr) { return NILREF; }
Ref LexDateLookup(RefArg rcvr) { return NILREF; }
Ref LexPhoneLookup(RefArg rcvr) { return NILREF; }
Ref LexTimeLookup(RefArg rcvr) { return NILREF; }
Ref LongitudeToCoordinate(RefArg rcvr) { return NILREF; }
Ref MakeLowerCase(RefArg rcvr) { return NILREF; }
Ref MakePhrasalLexEntry(RefArg rcvr) { return NILREF; }
Ref MapSymToFrame(RefArg rcvr) { return NILREF; }
Ref PageContents(RefArg rcvr) { return NILREF; }
Ref PageScroll(RefArg rcvr) { return NILREF; }
Ref PageThumbnail(RefArg rcvr) { return NILREF; }
Ref PathToRoot(RefArg rcvr) { return NILREF; }
Ref PhoneIndexToValue(RefArg rcvr) { return NILREF; }
Ref PhoneStringToValue(RefArg rcvr) { return NILREF; }
Ref PhoneSymToIndex(RefArg rcvr) { return NILREF; }
Ref PhoneSymToString(RefArg rcvr) { return NILREF; }
Ref PhraseFilter(RefArg rcvr) { return NILREF; }
Ref PrefixP(RefArg rcvr) { return NILREF; }
Ref PrepBook(RefArg rcvr) { return NILREF; }
Ref PrepRecConfig(RefArg rcvr) { return NILREF; }
Ref PreviousPage(RefArg rcvr) { return NILREF; }
Ref ReFlow(RefArg rcvr) { return NILREF; }
Ref ReadDomainOptions(RefArg rcvr) { return NILREF; }
Ref ReflowPreflight(RefArg rcvr) { return NILREF; }
Ref RefreshTopics(RefArg rcvr) { return NILREF; }
Ref RegisterBookRef(RefArg rcvr) { return NILREF; }
Ref ScrollToCurrent(RefArg rcvr) { return NILREF; }
Ref SendAbort(RefArg rcvr) { return NILREF; }
Ref StartIRSniffing(RefArg rcvr) { return NILREF; }
Ref StopIRSniffing(RefArg rcvr) { return NILREF; }
Ref Store1XPackageToVBO(RefArg rcvr) { return NILREF; }
Ref StoreConvertSoupSortTables(RefArg rcvr) { return NILREF; }
Ref StorePackageRestore(RefArg rcvr) { return NILREF; }
Ref StoreSegmentedPackageRestore(RefArg rcvr) { return NILREF; }
Ref SuffixP(RefArg rcvr) { return NILREF; }
Ref TableLookup(RefArg rcvr) { return NILREF; }
Ref TurnToContent(RefArg rcvr) { return NILREF; }
Ref TurnToPage(RefArg rcvr) { return NILREF; }
Ref UnionSoupFlush(RefArg rcvr) { return NILREF; }
Ref UniqueAppendList(RefArg rcvr) { return NILREF; }
Ref UniqueAppendListGen(RefArg rcvr) { return NILREF; }
Ref UnmatchedWords(RefArg rcvr) { return NILREF; }
Ref UnregisterBookRef(RefArg rcvr) { return NILREF; }
Ref UpdateBookmarks(RefArg rcvr) { return NILREF; }
Ref UseTrainingDataForRecognition(RefArg rcvr) { return NILREF; }
Ref WhereIsBook(RefArg rcvr) { return NILREF; }
Ref ZapCancel(RefArg rcvr) { return NILREF; }
Ref ZapReceive(RefArg rcvr) { return NILREF; }
Ref ZapSend(RefArg rcvr) { return NILREF; }
Ref ZoomView(RefArg rcvr) { return NILREF; }

Ref FRecognizeInkWord(RefArg rcvr) { return NILREF; }
Ref FntpTetheredListener(RefArg rcvr) { return NILREF; }
Ref FSubstituteChars(RefArg rcvr) { return NILREF; }
Ref FMatchedChar(RefArg rcvr) { return NILREF; }
Ref FPathToRoot(RefArg rcvr) { return NILREF; }
Ref FCircleDistance(RefArg rcvr) { return NILREF; }
Ref FInTryString(RefArg rcvr) { return NILREF; }
Ref FGetTrainingData(RefArg rcvr) { return NILREF; }
Ref FSetLearningData(RefArg rcvr) { return NILREF; }
Ref FNewByName(RefArg rcvr) { return NILREF; }
Ref FRecognize(RefArg rcvr) { return NILREF; }
Ref FGetCardTypes(RefArg rcvr) { return NILREF; }
Ref FUniqueAppend(RefArg rcvr) { return NILREF; }
Ref FDisplay(RefArg rcvr) { return NILREF; }
Ref FGetTone(RefArg rcvr) { return NILREF; }
Ref FLoadOnlinePackage(RefArg rcvr) { return NILREF; }
Ref FGetZoneList(RefArg rcvr) { return NILREF; }
Ref FIsMockEntry(RefArg rcvr) { return NILREF; }
Ref FOtherViewInUse(RefArg rcvr) { return NILREF; }
Ref FExtractRichStringFromParaSlots(RefArg rcvr) { return NILREF; }
Ref FPidToPackageLite(RefArg rcvr) { return NILREF; }
Ref FSetRamParaData(RefArg rcvr) { return NILREF; }
Ref FComputeParagraphHeight(RefArg rcvr) { return NILREF; }
Ref FAddHist(RefArg rcvr) { return NILREF; }
Ref FWRecIsBeingUsed(RefArg rcvr) { return NILREF; }
Ref FDumpDict(RefArg rcvr) { return NILREF; }
Ref FGetPkgInfoFromPssid(RefArg rcvr) { return NILREF; }
Ref FHaveZones(RefArg rcvr) { return NILREF; }
Ref DSTagString(RefArg rcvr) { return NILREF; }
Ref FPolyContainsInk(RefArg rcvr) { return NILREF; }
Ref DSFilterStringsAux(RefArg rcvr) { return NILREF; }
Ref FDSDate(RefArg rcvr) { return NILREF; }
Ref FGetHiliteIndex(RefArg rcvr) { return NILREF; }
Ref FGuessAddressee(RefArg rcvr) { return NILREF; }
Ref FActivateTestAgent(RefArg rcvr) { return NILREF; }
Ref FFinishRecognizing(RefArg rcvr) { return NILREF; }
Ref FReadDomainOptions(RefArg rcvr) { return NILREF; }
Ref FFrameDirty(RefArg rcvr) { return NILREF; }
Ref FAppleTalkOpenCount(RefArg rcvr) { return NILREF; }
Ref FDrawStringShapes(RefArg rcvr) { return NILREF; }
Ref FGetFontFamilyNum(RefArg rcvr) { return NILREF; }
Ref FUnmountCard(RefArg rcvr) { return NILREF; }
Ref FTestMDropConnection(RefArg rcvr) { return NILREF; }
Ref FUnblockStrokes(RefArg rcvr) { return NILREF; }
Ref FBackLight(RefArg rcvr) { return NILREF; }
Ref FMakeRegion(RefArg rcvr) { return NILREF; }
Ref FMergeInk(RefArg rcvr) { return NILREF; }
Ref FStartIRSniffing(RefArg rcvr) { return NILREF; }
Ref FStripInk(RefArg rcvr) { return NILREF; }
Ref FEntryCopy(RefArg rcvr) { return NILREF; }
Ref FSmartStop(RefArg rcvr) { return NILREF; }
Ref FGetFontFace(RefArg rcvr) { return NILREF; }
Ref FCollect(RefArg rcvr) { return NILREF; }
Ref FSendOnline(RefArg rcvr) { return NILREF; }
Ref FQuickLookDone(RefArg rcvr) { return NILREF; }
Ref FTestReadTextFile(RefArg rcvr) { return NILREF; }
Ref Fmax(RefArg rcvr) { return NILREF; }
Ref FNextMeeting(RefArg rcvr) { return NILREF; }
Ref FGetHilitedTextItems(RefArg rcvr) { return NILREF; }
Ref FReFlow(RefArg rcvr) { return NILREF; }
Ref FStringAnnotate(RefArg rcvr) { return NILREF; }
Ref FGetStroke(RefArg rcvr) { return NILREF; }
Ref FMashLists(RefArg rcvr) { return NILREF; }
Ref FStringShorten(RefArg rcvr) { return NILREF; }
Ref DSScanForWackyName(RefArg rcvr) { return NILREF; }
Ref FGetDictItem(RefArg rcvr) { return NILREF; }
Ref FGetDefaultFont(RefArg rcvr) { return NILREF; }
Ref FScanPrevWordEnd(RefArg rcvr) { return NILREF; }
Ref FStrExactCompare(RefArg rcvr) { return NILREF; }
Ref FMapSymToFrame(RefArg rcvr) { return NILREF; }
Ref FPSSidToPkgRef(RefArg rcvr) { return NILREF; }
Ref FSetFontFamily(RefArg rcvr) { return NILREF; }
Ref FStringToFrame(RefArg rcvr) { return NILREF; }
Ref FTestWillCallExit(RefArg rcvr) { return NILREF; }
Ref FLookupCompletions(RefArg rcvr) { return NILREF; }
Ref FCompressStrokes(RefArg rcvr) { return NILREF; }
Ref FInitSerialDebugging(RefArg rcvr) { return NILREF; }
Ref FRestorePatchPackage(RefArg rcvr) { return NILREF; }
Ref FGetDictionaryData(RefArg rcvr) { return NILREF; }
Ref FGetMatchedEntries(RefArg rcvr) { return NILREF; }
Ref FGetChar(RefArg rcvr) { return NILREF; }
Ref FGetVBOCompander(RefArg rcvr) { return NILREF; }
Ref FOpenAppleTalk(RefArg rcvr) { return NILREF; }
Ref FRemoveAutoAdd(RefArg rcvr) { return NILREF; }
Ref FSendRemoteControlCode(RefArg rcvr) { return NILREF; }
Ref FGetInkAt(RefArg rcvr) { return NILREF; }
Ref FCompositeClass(RefArg rcvr) { return NILREF; }
Ref FBackupPatchPackage(RefArg rcvr) { return NILREF; }
Ref FPhoneIndexToValue(RefArg rcvr) { return NILREF; }
Ref FMakeLexEntry(RefArg rcvr) { return NILREF; }
Ref FNextInkIndex(RefArg rcvr) { return NILREF; }
Ref FStdioOn(RefArg rcvr) { return NILREF; }
Ref FConvertFromMP(RefArg rcvr) { return NILREF; }
Ref FStrWidth(RefArg rcvr) { return NILREF; }
Ref FSpellDocBegin(RefArg rcvr) { return NILREF; }
Ref FFontLeading(RefArg rcvr) { return NILREF; }
Ref FDisposeTrainingData(RefArg rcvr) { return NILREF; }
Ref FCloseAppleTalk(RefArg rcvr) { return NILREF; }
Ref FBackLightStatus(RefArg rcvr) { return NILREF; }
Ref FBubbleArraySlot(RefArg rcvr) { return NILREF; }
Ref FAppendList(RefArg rcvr) { return NILREF; }
Ref ForigPhrase(RefArg rcvr) { return NILREF; }
Ref FDeActivatePackage(RefArg rcvr) { return NILREF; }
Ref FTestGetParameterString(RefArg rcvr) { return NILREF; }
Ref FOpenRemoteControl(RefArg rcvr) { return NILREF; }
Ref FNewMockEntry(RefArg rcvr) { return NILREF; }
Ref FSetFontParms(RefArg rcvr) { return NILREF; }
Ref FSmartCFQuery(RefArg rcvr) { return NILREF; }
Ref FTestMSetParameterString(RefArg rcvr) { return NILREF; }
Ref FLookupWord(RefArg rcvr) { return NILREF; }
Ref FDrawDateLabels(RefArg rcvr) { return NILREF; }
Ref FScanWordEnd(RefArg rcvr) { return NILREF; }
Ref FGetViewID(RefArg rcvr) { return NILREF; }
Ref FCloseRemoteControl(RefArg rcvr) { return NILREF; }
Ref FRecognizePoly(RefArg rcvr) { return NILREF; }
Ref FArrayPos(RefArg rcvr) { return NILREF; }
Ref FDrawLetterScript(RefArg rcvr) { return NILREF; }
Ref FGetVBOStoredSize(RefArg rcvr) { return NILREF; }
Ref FNBPLookupCount(RefArg rcvr) { return NILREF; }
Ref FStringToNumber(RefArg rcvr) { return NILREF; }
Ref FGetInsertionStyle(RefArg rcvr) { return NILREF; }
Ref FParaContainsInk(RefArg rcvr) { return NILREF; }
Ref FntkDownload(RefArg rcvr) { return NILREF; }
Ref FGetSortID(RefArg rcvr) { return NILREF; }
Ref FSuffixP(RefArg rcvr) { return NILREF; }
Ref DSResolveString(RefArg rcvr) { return NILREF; }
Ref FIsProtocolPartInUse(RefArg rcvr) { return NILREF; }
Ref FCalcInkBounds(RefArg rcvr) { return NILREF; }
Ref FMoveCorrectionInfo(RefArg rcvr) { return NILREF; }
Ref FGetLetterWeights(RefArg rcvr) { return NILREF; }
Ref FPowerOffRingiSho(RefArg rcvr) { return NILREF; }
Ref FGetFontSize(RefArg rcvr) { return NILREF; }
Ref FGetMyZone(RefArg rcvr) { return NILREF; }
Ref FDeActivate1_2EXPackage(RefArg rcvr) { return NILREF; }
Ref FRecognizeTextInStyles(RefArg rcvr) { return NILREF; }
Ref FPhoneSymToString(RefArg rcvr) { return NILREF; }
Ref FDrawMeetingGrid(RefArg rcvr) { return NILREF; }
Ref FGetGlobals(RefArg rcvr) { return NILREF; }
Ref FGetNames(RefArg rcvr) { return NILREF; }
Ref FlocalVar(RefArg rcvr) { return NILREF; }
Ref FFontDescent(RefArg rcvr) { return NILREF; }
Ref FhasSiblingSlot(RefArg rcvr) { return NILREF; }
Ref FBlockStrokes(RefArg rcvr) { return NILREF; }
Ref FUnorderedLessOrEqual(RefArg rcvr) { return NILREF; }
Ref FAddDelayedAction(RefArg rcvr) { return NILREF; }
Ref FScanWordStart(RefArg rcvr) { return NILREF; }
Ref FViewAllowsInk(RefArg rcvr) { return NILREF; }
Ref FStyleArrayContainsInk(RefArg rcvr) { return NILREF; }
Ref FStrTruncate(RefArg rcvr) { return NILREF; }
Ref FNBPStartLookup(RefArg rcvr) { return NILREF; }
Ref FDecodeRichString(RefArg rcvr) { return NILREF; }
Ref FGetBlue(RefArg rcvr) { return NILREF; }
Ref FClassInfoRegistrySeed(RefArg rcvr) { return NILREF; }
Ref FGetExportTableClients(RefArg rcvr) { return NILREF; }
Ref FIsEqualTone(RefArg rcvr) { return NILREF; }
Ref FGetCardSlotStores(RefArg rcvr) { return NILREF; }
Ref FUnionPoint(RefArg rcvr) { return NILREF; }
Ref FPidToPackage(RefArg rcvr) { return NILREF; }
Ref Fand(RefArg rcvr) { return NILREF; }
Ref FFavorObject(RefArg rcvr) { return NILREF; }
Ref FEntryReplace(RefArg rcvr) { return NILREF; }
Ref FGetRelevantTemplates(RefArg rcvr) { return NILREF; }
Ref FEntryReplaceWithModTime(RefArg rcvr) { return NILREF; }
Ref FLayoutMeeting(RefArg rcvr) { return NILREF; }
Ref FNBPStart(RefArg rcvr) { return NILREF; }
Ref FStrFontWidth(RefArg rcvr) { return NILREF; }
Ref DSPrevSubStr(RefArg rcvr) { return NILREF; }
Ref FCallOnlineService(RefArg rcvr) { return NILREF; }
Ref DSFindPossibleLocation(RefArg rcvr) { return NILREF; }
Ref FMakeInk(RefArg rcvr) { return NILREF; }
Ref FGetDynamicValue(RefArg rcvr) { return NILREF; }
Ref FCorrectSelect(RefArg rcvr) { return NILREF; }
Ref FGetVar(RefArg rcvr) { return NILREF; }
Ref FMergeStrokes(RefArg rcvr) { return NILREF; }
Ref DSConstructSubjectLine(RefArg rcvr) { return NILREF; }
Ref FStrokesAfterUnit(RefArg rcvr) { return NILREF; }
Ref FTestMStartTestFrame(RefArg rcvr) { return NILREF; }
Ref FntpDownloadPackage(RefArg rcvr) { return NILREF; }
Ref FTestReportError(RefArg rcvr) { return NILREF; }
Ref FSpellDocEnd(RefArg rcvr) { return NILREF; }
Ref FVBOUndoChanges(RefArg rcvr) { return NILREF; }
Ref FSetGlobal(RefArg rcvr) { return NILREF; }
Ref Freal(RefArg rcvr) { return NILREF; }
Ref FValidateWord(RefArg rcvr) { return NILREF; }
Ref FGetLetterHilite(RefArg rcvr) { return NILREF; }
Ref FAddDeferredAction(RefArg rcvr) { return NILREF; }
Ref FStripRecognitionWordDiacritsOK(RefArg rcvr) { return NILREF; }
Ref FTestReportMessage(RefArg rcvr) { return NILREF; }
Ref FGetVBOCompanderData(RefArg rcvr) { return NILREF; }
Ref FFastStrLookup(RefArg rcvr) { return NILREF; }
Ref FRecognizePara(RefArg rcvr) { return NILREF; }
Ref FGetGreen(RefArg rcvr) { return NILREF; }
Ref FSetHiliteIndex(RefArg rcvr) { return NILREF; }
Ref For(RefArg rcvr) { return NILREF; }
Ref FInkConvert(RefArg rcvr) { return NILREF; }
Ref FPSSidToPid(RefArg rcvr) { return NILREF; }
Ref FSplitInkAt(RefArg rcvr) { return NILREF; }
Ref FNumInkWordsInRange(RefArg rcvr) { return NILREF; }
Ref FRepeatInfoToText(RefArg rcvr) { return NILREF; }
Ref FSubstr(RefArg rcvr) { return NILREF; }
Ref FWordUnitToWordInfo(RefArg rcvr) { return NILREF; }
Ref FmodalState(RefArg rcvr) { return NILREF; }
Ref FStdioOff(RefArg rcvr) { return NILREF; }
Ref FConnEntriesEqual(RefArg rcvr) { return NILREF; }
Ref FSpellSkip(RefArg rcvr) { return NILREF; }
Ref FScanNextWord(RefArg rcvr) { return NILREF; }
Ref FStringFormat(RefArg rcvr) { return NILREF; }
Ref FTextBounds(RefArg rcvr) { return NILREF; }
Ref FScaleShape(RefArg rcvr) { return NILREF; }
Ref FSetFontFace(RefArg rcvr) { return NILREF; }
Ref FMakeRichString(RefArg rcvr) { return NILREF; }
Ref FExpandInk(RefArg rcvr) { return NILREF; }
Ref FSendAbort(RefArg rcvr) { return NILREF; }
Ref FReflowPreflight(RefArg rcvr) { return NILREF; }
Ref FGetVBOStore(RefArg rcvr) { return NILREF; }
Ref FMapPtY(RefArg rcvr) { return NILREF; }
Ref FActivatePackage(RefArg rcvr) { return NILREF; }
Ref FClearTryString(RefArg rcvr) { return NILREF; }
Ref FFlushStrokes(RefArg rcvr) { return NILREF; }
Ref FStripRecognitionWord(RefArg rcvr) { return NILREF; }
Ref FTestReadDataFile(RefArg rcvr) { return NILREF; }
Ref DSAddLexiconFrame(RefArg rcvr) { return NILREF; }
Ref FViewWorksWithCombCorrector(RefArg rcvr) { return NILREF; }
Ref FInsertStyledText(RefArg rcvr) { return NILREF; }
Ref FGetIndexChar(RefArg rcvr) { return NILREF; }
Ref FConvertDictionaryData(RefArg rcvr) { return NILREF; }
Ref Fisa(RefArg rcvr) { return NILREF; }
Ref FPidToPkgRef(RefArg rcvr) { return NILREF; }
Ref FSmartConcat(RefArg rcvr) { return NILREF; }
Ref FAnimateSimpleStroke(RefArg rcvr) { return NILREF; }
Ref FDSTime(RefArg rcvr) { return NILREF; }
Ref FGetLetterIndex(RefArg rcvr) { return NILREF; }
Ref FUnmatchedWords(RefArg rcvr) { return NILREF; }
Ref Fstats(RefArg rcvr) { return NILREF; }
Ref FStrokeBundleToInkWord(RefArg rcvr) { return NILREF; }
Ref FGetLearningData(RefArg rcvr) { return NILREF; }
Ref FCompressStrokesToInk(RefArg rcvr) { return NILREF; }
Ref DSFindPossibleName(RefArg rcvr) { return NILREF; }
Ref DSFindPossiblePhone(RefArg rcvr) { return NILREF; }
Ref FGetSelectionStack(RefArg rcvr) { return NILREF; }
Ref FSetDictionaryData(RefArg rcvr) { return NILREF; }
Ref FDrawOriginal(RefArg rcvr) { return NILREF; }
Ref FSetChar(RefArg rcvr) { return NILREF; }
Ref FStopIRSniffing(RefArg rcvr) { return NILREF; }
Ref FMapCursor(RefArg rcvr) { return NILREF; }
Ref FConvertForMP(RefArg rcvr) { return NILREF; }
Ref FPhoneSymToIndex(RefArg rcvr) { return NILREF; }
Ref FSmartStart(RefArg rcvr) { return NILREF; }
Ref FTestMStartTestCase(RefArg rcvr) { return NILREF; }
Ref FGetRamParaData(RefArg rcvr) { return NILREF; }
Ref FSpellCheck(RefArg rcvr) { return NILREF; }
Ref FMakePict(RefArg rcvr) { return NILREF; }
Ref FUseTrainingDataForRecognition(RefArg rcvr) { return NILREF; }
Ref FMakeStrokeBundle(RefArg rcvr) { return NILREF; }
Ref FExpandUnit(RefArg rcvr) { return NILREF; }
Ref FStrCompare(RefArg rcvr) { return NILREF; }
Ref FGenPhoneTypes(RefArg rcvr) { return NILREF; }
Ref FTieViews(RefArg rcvr) { return NILREF; }
Ref FGetFontFamilySym(RefArg rcvr) { return NILREF; }
Ref FGetStrokePoint(RefArg rcvr) { return NILREF; }
Ref FEntryMove(RefArg rcvr) { return NILREF; }
Ref FGetStrokePointsArray(RefArg rcvr) { return NILREF; }
Ref FhasVariable(RefArg rcvr) { return NILREF; }
Ref FFindShape(RefArg rcvr) { return NILREF; }
Ref FActivate1_2EXPackage(RefArg rcvr) { return NILREF; }
Ref FCountUnitStrokes(RefArg rcvr) { return NILREF; }
Ref FMakeRoundRect(RefArg rcvr) { return NILREF; }
Ref FCoordinateToLongitude(RefArg rcvr) { return NILREF; }
Ref FLongitudeToCoordinate(RefArg rcvr) { return NILREF; }
Ref FPictToShape(RefArg rcvr) { return NILREF; }
Ref FReBoot(RefArg rcvr) { return NILREF; }
Ref FStringFilter(RefArg rcvr) { return NILREF; }
Ref FSubsume(RefArg rcvr) { return NILREF; }
Ref FRosettaExtension(RefArg rcvr) { return NILREF; }
Ref FCountPoints(RefArg rcvr) { return NILREF; }
Ref FAddDictionary(RefArg rcvr) { return NILREF; }
Ref FGetViewFlags(RefArg rcvr) { return NILREF; }
Ref FGetZoneFromName(RefArg rcvr) { return NILREF; }
Ref FRemove1_2EXPackage(RefArg rcvr) { return NILREF; }
Ref FGetRandomDictionaryWord(RefArg rcvr) { return NILREF; }
Ref Farray(RefArg rcvr) { return NILREF; }
Ref FCancelOnlineService(RefArg rcvr) { return NILREF; }
Ref FMakeWedge(RefArg rcvr) { return NILREF; }
Ref FPackRGB(RefArg rcvr) { return NILREF; }
Ref FMakeCompactFont(RefArg rcvr) { return NILREF; }
Ref FFavorAction(RefArg rcvr) { return NILREF; }
Ref FNBPGetLookupNames(RefArg rcvr) { return NILREF; }
Ref FGenSubStrings(RefArg rcvr) { return NILREF; }
Ref FSetLetterWeights(RefArg rcvr) { return NILREF; }
Ref FSetFontSize(RefArg rcvr) { return NILREF; }
Ref FPositionToTime(RefArg rcvr) { return NILREF; }
Ref FTimeToPosition(RefArg rcvr) { return NILREF; }
Ref FTestMGetReportMsg(RefArg rcvr) { return NILREF; }
Ref FDeactivateTestAgent(RefArg rcvr) { return NILREF; }
Ref FSetTimeHardware(RefArg rcvr) { return NILREF; }
Ref FFontHeight(RefArg rcvr) { return NILREF; }
Ref FGenPhrases(RefArg rcvr) { return NILREF; }
Ref FNBPGetCount(RefArg rcvr) { return NILREF; }
Ref FResetLetterDefaults(RefArg rcvr) { return NILREF; }
Ref FClearVBOCache(RefArg rcvr) { return NILREF; }
Ref FNBPGetNames(RefArg rcvr) { return NILREF; }
Ref FConnBuildStoreFrame(RefArg rcvr) { return NILREF; }
Ref FGetRed(RefArg rcvr) { return NILREF; }
Ref FGetOnlineEndpoint(RefArg rcvr) { return NILREF; }
Ref FCalcBundleBounds(RefArg rcvr) { return NILREF; }
Ref FTryStringLength(RefArg rcvr) { return NILREF; }
Ref FgetSiblingSlot(RefArg rcvr) { return NILREF; }
Ref FIntersect(RefArg rcvr) { return NILREF; }
Ref FHist(RefArg rcvr) { return NILREF; }
Ref FforLoop(RefArg rcvr) { return NILREF; }
Ref FGetStrokeBounds(RefArg rcvr) { return NILREF; }
Ref FSetInkerPenSize(RefArg rcvr) { return NILREF; }
Ref FTestExit(RefArg rcvr) { return NILREF; }
Ref FRecSettingsChanged(RefArg rcvr) { return NILREF; }
Ref FGetAllMeetings(RefArg rcvr) { return NILREF; }
Ref FIsInkChar(RefArg rcvr) { return NILREF; }
Ref FStripDiacriticals(RefArg rcvr) { return NILREF; }
Ref FGetEditArray(RefArg rcvr) { return NILREF; }
Ref FCoordinateToLatitude(RefArg rcvr) { return NILREF; }
Ref FLatitudeToCoordinate(RefArg rcvr) { return NILREF; }
Ref FMapRect(RefArg rcvr) { return NILREF; }
Ref FFontAscent(RefArg rcvr) { return NILREF; }
Ref FWeekNumber(RefArg rcvr) { return NILREF; }
Ref FGetMeetingTypeInfo(RefArg rcvr) { return NILREF; }
Ref FGetNextMeetingTime(RefArg rcvr) { return NILREF; }
Ref FGetCorrectionWordInfo(RefArg rcvr) { return NILREF; }
Ref FTestGetParameterArray(RefArg rcvr) { return NILREF; }
Ref FCorrectWord(RefArg rcvr) { return NILREF; }
Ref FSetSlot(RefArg rcvr) { return NILREF; }
Ref FTextBox(RefArg rcvr) { return NILREF; }
Ref FBuildRecConfig(RefArg rcvr) { return NILREF; }
Ref FNBPStopLookup(RefArg rcvr) { return NILREF; }
Ref Fdebug(RefArg rcvr) { return NILREF; }
Ref FGetBitmapInfo(RefArg rcvr) { return NILREF; }
Ref FMakeOval(RefArg rcvr) { return NILREF; }
Ref FMungeBitmap(RefArg rcvr) { return NILREF; }
Ref FSetOnlineDisconnect(RefArg rcvr) { return NILREF; }
Ref FClickLetterScript(RefArg rcvr) { return NILREF; }
Ref FGetRangeText(RefArg rcvr) { return NILREF; }
Ref FSpellCorrect(RefArg rcvr) { return NILREF; }
Ref FSetPath(RefArg rcvr) { return NILREF; }
Ref FCommonAncestors(RefArg rcvr) { return NILREF; }
Ref FSetLetterHilite(RefArg rcvr) { return NILREF; }
Ref FLower(RefArg rcvr) { return NILREF; }
Ref FStrAssoc(RefArg rcvr) { return NILREF; }
Ref FAddAutoAdd(RefArg rcvr) { return NILREF; }
Ref FCountStrokes(RefArg rcvr) { return NILREF; }
Ref FAddInk(RefArg rcvr) { return NILREF; }
Ref FObjectPid(RefArg rcvr) { return NILREF; }
Ref FGetShapeInfo(RefArg rcvr) { return NILREF; }
Ref FMungeShape(RefArg rcvr) { return NILREF; }
Ref FGetDictionary(RefArg rcvr) { return NILREF; }
Ref Fmin(RefArg rcvr) { return NILREF; }
Ref FMakePolygon(RefArg rcvr) { return NILREF; }
Ref FPrevMeeting(RefArg rcvr) { return NILREF; }
Ref FTriggerWordRecognition(RefArg rcvr) { return NILREF; }
Ref FMeasuredNumberStr(RefArg rcvr) { return NILREF; }
Ref FAddTryString(RefArg rcvr) { return NILREF; }
Ref FNBPStop(RefArg rcvr) { return NILREF; }
Ref FPhoneStringToValue(RefArg rcvr) { return NILREF; }
Ref FClassInfoRegistryNext(RefArg rcvr) { return NILREF; }
Ref FRunInitScripts(RefArg rcvr) { return NILREF; }
Ref FTestFlushReportQueue(RefArg rcvr) { return NILREF; }
Ref FAppend(RefArg rcvr) { return NILREF; }
Ref FDESCreatePasswordKey(RefArg rcvr) { return NILREF; }
Ref FGetAllMeetingsUnique(RefArg rcvr) { return NILREF; }
Ref FDSPhone(RefArg rcvr) { return NILREF; }
Ref FConnectPassthruKeyboard(RefArg rcvr) { return NILREF; }
Ref FntkListener(RefArg rcvr) { return NILREF; }
Ref FMapPtX(RefArg rcvr) { return NILREF; }
Ref FCountLetters(RefArg rcvr) { return NILREF; }
Ref FViewAllowsInkWords(RefArg rcvr) { return NILREF; }
Ref FPreInitSerialDebugging(RefArg rcvr) { return NILREF; }
Ref FPointsArrayToStroke(RefArg rcvr) { return NILREF; }
Ref FClassInfoByName(RefArg rcvr) { return NILREF; }
Ref FDisconnectOnlineService(RefArg rcvr) { return NILREF; }
Ref FTranslate(RefArg rcvr) { return NILREF; }
Ref FDoCursiveTraining(RefArg rcvr) { return NILREF; }
Ref FPrefixP(RefArg rcvr) { return NILREF; }
Ref FUseWRec(RefArg rcvr) { return NILREF; }
Ref FReadCursiveOptions(RefArg rcvr) { return NILREF; }
Ref DSFilterStrings(RefArg rcvr) { return NILREF; }
Ref FBootSucceeded(RefArg rcvr) { return NILREF; }
Ref FGetMeetingIcon(RefArg rcvr) { return NILREF; }
