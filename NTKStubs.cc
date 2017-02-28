#include "Objects.h"

extern "C" {
Ref FAddDelayedCall(RefArg rcvr);
Ref FAddDelayedSend(RefArg rcvr);
Ref FAddKeyCommand(RefArg rcvr);
Ref FAddKeyCommands(RefArg rcvr);
Ref FAddStepView(RefArg rcvr);
Ref FAddUndoAction(RefArg rcvr);
Ref FAddUndoCall(RefArg rcvr);
Ref FAddUndoSend(RefArg rcvr);
Ref FAddView(RefArg rcvr);
Ref FArrayToPoints(RefArg rcvr);
Ref FBlockKeyCommand(RefArg rcvr);
Ref FBookAvailable(RefArg rcvr);
Ref FBookRemoved(RefArg rcvr);
Ref FBuildContext(RefArg rcvr);
Ref FBusyBoxControl(RefArg rcvr);
Ref FCalibrateTablet(RefArg rcvr);
Ref FCaretRelativeToVisibleRect(RefArg rcvr);
Ref FCategorizeKeyCommands(RefArg rcvr);
Ref FChangeStylesOfRange(RefArg rcvr);
Ref FChildViewFramesX(RefArg rcvr);
Ref FClearHardKeymap(RefArg rcvr);
Ref FClearPopup(RefArg rcvr);
Ref FClearUndoStacks(RefArg rcvr);
Ref FClipboardCommand(RefArg rcvr);
Ref FCloseX(RefArg rcvr);
Ref FCommandKeyboardConnected(RefArg rcvr);
Ref FCountGesturePoints(RefArg rcvr);
Ref FCurrentExports(RefArg rcvr);
Ref FCurrentImports(RefArg rcvr);
Ref FDV(RefArg rcvr);
Ref FDebug(RefArg rcvr);
Ref FDebugMemoryStats(RefArg rcvr);
Ref FDebugRunUntilIdle(RefArg rcvr);
Ref FDeleteX(RefArg rcvr);
Ref FDirtyBoxX(RefArg rcvr);
Ref FDirtyX(RefArg rcvr);
Ref FDismissPopup(RefArg rcvr);
Ref FDisplaySplashGraphic(RefArg rcvr);
Ref FDoDrawing(RefArg rcvr);
Ref FDoPopup(RefArg rcvr);
Ref FDoScrubEffect(RefArg rcvr);
Ref FDragAndDrop(RefArg rcvr);
Ref FDragAndDropLtd(RefArg rcvr);
Ref FDragX(RefArg rcvr);
Ref FDrawPolygons(RefArg rcvr);
Ref FDrawShape(RefArg rcvr);
Ref FDropHilites(RefArg rcvr);
Ref FEffectX(RefArg rcvr);
Ref FEventPause(RefArg rcvr);
Ref FExitModalDialog(RefArg rcvr);
Ref FExtractData(RefArg rcvr);
Ref FExtractRangeAsRichString(RefArg rcvr);
Ref FExtractTextRange(RefArg rcvr);
Ref FFilterDialog(RefArg rcvr);
Ref FFindKeyCommand(RefArg rcvr);
Ref FFlushImports(RefArg rcvr);
Ref FForkScript(RefArg rcvr);
Ref FFulfillImportTable(RefArg rcvr);
Ref FGatherKeyCommands(RefArg rcvr);
Ref FGestalt(RefArg rcvr);
Ref FGesturePoint(RefArg rcvr);
Ref FGestureType(RefArg rcvr);
Ref FGetAlternates(RefArg rcvr);
Ref FGetCalibration(RefArg rcvr);
Ref FGetCaretBox(RefArg rcvr);
Ref FGetCaretInfo(RefArg rcvr);
Ref FGetClipboard(RefArg rcvr);
Ref FGetClipboardIcon(RefArg rcvr);
Ref FGetDrawBoxX(RefArg rcvr);
Ref FGetFlags(RefArg rcvr);
Ref FGetFrameStuff(RefArg rcvr);
Ref FGetHiliteOffsets(RefArg rcvr);
Ref FGetId(RefArg rcvr);
Ref FGetInkWordInfo(RefArg rcvr);
Ref FGetKeyView(RefArg rcvr);
Ref FGetPackages(RefArg rcvr);
Ref FGetPoint(RefArg rcvr);
Ref FGetPointsArray(RefArg rcvr);
Ref FGetPointsArrayXY(RefArg rcvr);
Ref FGetPolygons(RefArg rcvr);
Ref FGetPopup(RefArg rcvr);
Ref FGetRemoteWriting(RefArg rcvr);
Ref FGetRoot(RefArg rcvr);
Ref FGetScoreArray(RefArg rcvr);
Ref FGetStyleAtOffset(RefArg rcvr);
Ref FGetStylesOfRange(RefArg rcvr);
Ref FGetTextFlags(RefArg rcvr);
Ref FGetTrueModifiers(RefArg rcvr);
Ref FGetUndoState(RefArg rcvr);
Ref FGetUnitDownTime(RefArg rcvr);
Ref FGetUnitEndTime(RefArg rcvr);
Ref FGetUnitStartTime(RefArg rcvr);
Ref FGetUnitUpTime(RefArg rcvr);
Ref FGetView(RefArg rcvr);
Ref FGetWordArray(RefArg rcvr);
Ref FGlobalBoxX(RefArg rcvr);
Ref FGlobalOuterBoxX(RefArg rcvr);
Ref FHandleInkWord(RefArg rcvr);
Ref FHandleInsertItems(RefArg rcvr);
Ref FHandleKeyEvents(RefArg rcvr);
Ref FHandleRawInk(RefArg rcvr);
Ref FHandleUnit(RefArg rcvr);
Ref FHideCaret(RefArg rcvr);
Ref FHideX(RefArg rcvr);
Ref FHiliteOwner(RefArg rcvr);
Ref FHiliteUniqueX(RefArg rcvr);
Ref FHiliteViewChildren(RefArg rcvr);
Ref FHiliteX(RefArg rcvr);
Ref FHiliter(RefArg rcvr);
Ref FHobbleTablet(RefArg rcvr);
Ref FInRepeatedKeyCommand(RefArg rcvr);
Ref FInkOff(RefArg rcvr);
Ref FInkOffUnHobbled(RefArg rcvr);
Ref FInkOn(RefArg rcvr);
Ref FInsertItemsAtCaret(RefArg rcvr);
Ref FInsertTabletSample(RefArg rcvr);
Ref FInsetRect(RefArg rcvr);
Ref FInvertRect(RefArg rcvr);
Ref FIsCommandKeystroke(RefArg rcvr);
Ref FIsKeyDown(RefArg rcvr);
Ref FIsPrimShape(RefArg rcvr);
Ref FIsPtInRect(RefArg rcvr);
Ref FIsTabletCalibrationNeeded(RefArg rcvr);
Ref FJournalReplayALine(RefArg rcvr);
Ref FJournalReplayAStroke(RefArg rcvr);
Ref FJournalReplayBusy(RefArg rcvr);
Ref FJournalReplayStrokes(RefArg rcvr);
Ref FJournalStartRecord(RefArg rcvr);
Ref FJournalStopRecord(RefArg rcvr);
Ref FKeyIn(RefArg rcvr);
Ref FKeyboardConnected(RefArg rcvr);
Ref FLayoutTableX(RefArg rcvr);
Ref FLayoutVerticallyX(RefArg rcvr);
Ref FLoadFontCache(RefArg rcvr);
Ref FLocalBoxX(RefArg rcvr);
Ref FMakeBitmap(RefArg rcvr);
Ref FMakeLine(RefArg rcvr);
Ref FMakeRect(RefArg rcvr);
Ref FMakeShape(RefArg rcvr);
Ref FMakeText(RefArg rcvr);
Ref FMakeTextBox(RefArg rcvr);
Ref FMakeTextLines(RefArg rcvr);
Ref FMatchKeyMessage(RefArg rcvr);
Ref FModalDialog(RefArg rcvr);
Ref FModalRecognitionOff(RefArg rcvr);
Ref FModalRecognitionOn(RefArg rcvr);
Ref FModalState(RefArg rcvr);
Ref FMoveBehindX(RefArg rcvr);
Ref FNTKAlive(RefArg rcvr);
Ref FNTKDownload(RefArg rcvr);
Ref FNTKListener(RefArg rcvr);
Ref FNTKSend(RefArg rcvr);
Ref FNextKeyView(RefArg rcvr);
Ref FOffsetRect(RefArg rcvr);
Ref FOffsetShape(RefArg rcvr);
Ref FOffsetView(RefArg rcvr);
Ref FParentX(RefArg rcvr);
Ref FPenPos(RefArg rcvr);
Ref FPendingImports(RefArg rcvr);
Ref FPickViewKeyDown(RefArg rcvr);
Ref FPointToCharOffset(RefArg rcvr);
Ref FPointToWord(RefArg rcvr);
Ref FPointsToArray(RefArg rcvr);
Ref FPositionCaret(RefArg rcvr);
Ref FPostAndDo(RefArg rcvr);
Ref FPostCommand(RefArg rcvr);
Ref FPostCommandParam(RefArg rcvr);
Ref FPostKeyString(RefArg rcvr);
Ref FPurgeAreaCache(RefArg rcvr);
Ref FRectsOverlap(RefArg rcvr);
Ref FRedoChildrenX(RefArg rcvr);
Ref FRegisterGestalt(RefArg rcvr);
Ref FRegisterOpenKeyboard(RefArg rcvr);
Ref FRelBounds(RefArg rcvr);
Ref FRemoveStepView(RefArg rcvr);
Ref FRemoveView(RefArg rcvr);
Ref FReplaceGestalt(RefArg rcvr);
Ref FRestoreKeyView(RefArg rcvr);
Ref FRevealEffectX(RefArg rcvr);
Ref FSectRect(RefArg rcvr);
Ref FSendKeyMessage(RefArg rcvr);
Ref FSetBounds(RefArg rcvr);
Ref FSetCalibration(RefArg rcvr);
Ref FSetCaretInfo(RefArg rcvr);
Ref FSetClipboard(RefArg rcvr);
Ref FSetHiliteNoUpdateX(RefArg rcvr);
Ref FSetHiliteX(RefArg rcvr);
Ref FSetKeyView(RefArg rcvr);
Ref FSetOriginX(RefArg rcvr);
Ref FSetPopup(RefArg rcvr);
Ref FSetRemoteWriting(RefArg rcvr);
Ref FSetSysAlarm(RefArg rcvr);
Ref FSetupIdleX(RefArg rcvr);
Ref FShapeBounds(RefArg rcvr);
Ref FShowCaret(RefArg rcvr);
Ref FShowX(RefArg rcvr);
Ref FSlideEffectX(RefArg rcvr);
Ref FStartBypassTablet(RefArg rcvr);
Ref FStopBypassTablet(RefArg rcvr);
Ref FStrokeBounds(RefArg rcvr);
Ref FStrokeDone(RefArg rcvr);
Ref FStrokeInPicture(RefArg rcvr);
Ref FSyncChildrenX(RefArg rcvr);
Ref FSyncScrollX(RefArg rcvr);
Ref FSyncViewX(RefArg rcvr);
Ref FTabletBufferEmpty(RefArg rcvr);
Ref FToggleX(RefArg rcvr);
Ref FTopicByName(RefArg rcvr);
Ref FTrackButtonX(RefArg rcvr);
Ref FTrackHiliteX(RefArg rcvr);
Ref FTranslateKey(RefArg rcvr);
Ref FUnionRect(RefArg rcvr);
Ref FUnregisterOpenKeyboard(RefArg rcvr);
Ref FViewAutopsy(RefArg rcvr);
Ref FViewContainsCaretView(RefArg rcvr);
Ref FVisibleBox(RefArg rcvr);
Ref FVoteOnWordUnit(RefArg rcvr);
Ref FYieldToFork(RefArg rcvr);
Ref FpkgDownload(RefArg rcvr);
}

Ref FAddDelayedCall(RefArg rcvr) { return NILREF; }
Ref FAddDelayedSend(RefArg rcvr) { return NILREF; }
Ref FAddKeyCommand(RefArg rcvr) { return NILREF; }
Ref FAddKeyCommands(RefArg rcvr) { return NILREF; }
Ref FAddStepView(RefArg rcvr) { return NILREF; }
Ref FAddUndoAction(RefArg rcvr) { return NILREF; }
Ref FAddUndoCall(RefArg rcvr) { return NILREF; }
Ref FAddUndoSend(RefArg rcvr) { return NILREF; }
Ref FAddView(RefArg rcvr) { return NILREF; }
Ref FArrayToPoints(RefArg rcvr) { return NILREF; }
Ref FBlockKeyCommand(RefArg rcvr) { return NILREF; }
Ref FBookAvailable(RefArg rcvr) { return NILREF; }
Ref FBookRemoved(RefArg rcvr) { return NILREF; }
Ref FBuildContext(RefArg rcvr) { return NILREF; }
Ref FBusyBoxControl(RefArg rcvr) { return NILREF; }
Ref FCalibrateTablet(RefArg rcvr) { return NILREF; }
Ref FCaretRelativeToVisibleRect(RefArg rcvr) { return NILREF; }
Ref FCategorizeKeyCommands(RefArg rcvr) { return NILREF; }
Ref FChangeStylesOfRange(RefArg rcvr) { return NILREF; }
Ref FChildViewFramesX(RefArg rcvr) { return NILREF; }
Ref FClearHardKeymap(RefArg rcvr) { return NILREF; }
Ref FClearPopup(RefArg rcvr) { return NILREF; }
Ref FClearUndoStacks(RefArg rcvr) { return NILREF; }
Ref FClipboardCommand(RefArg rcvr) { return NILREF; }
Ref FCloseX(RefArg rcvr) { return NILREF; }
Ref FCommandKeyboardConnected(RefArg rcvr) { return NILREF; }
Ref FCountGesturePoints(RefArg rcvr) { return NILREF; }
Ref FCurrentExports(RefArg rcvr) { return NILREF; }
Ref FCurrentImports(RefArg rcvr) { return NILREF; }
Ref FDV(RefArg rcvr) { return NILREF; }
Ref FDebug(RefArg rcvr) { return NILREF; }
Ref FDebugMemoryStats(RefArg rcvr) { return NILREF; }
Ref FDebugRunUntilIdle(RefArg rcvr) { return NILREF; }
Ref FDeleteX(RefArg rcvr) { return NILREF; }
Ref FDirtyBoxX(RefArg rcvr) { return NILREF; }
Ref FDirtyX(RefArg rcvr) { return NILREF; }
Ref FDismissPopup(RefArg rcvr) { return NILREF; }
Ref FDisplaySplashGraphic(RefArg rcvr) { return NILREF; }
Ref FDoDrawing(RefArg rcvr) { return NILREF; }
Ref FDoPopup(RefArg rcvr) { return NILREF; }
Ref FDoScrubEffect(RefArg rcvr) { return NILREF; }
Ref FDragAndDrop(RefArg rcvr) { return NILREF; }
Ref FDragAndDropLtd(RefArg rcvr) { return NILREF; }
Ref FDragX(RefArg rcvr) { return NILREF; }
Ref FDrawPolygons(RefArg rcvr) { return NILREF; }
Ref FDrawShape(RefArg rcvr) { return NILREF; }
Ref FDropHilites(RefArg rcvr) { return NILREF; }
Ref FEffectX(RefArg rcvr) { return NILREF; }
Ref FEventPause(RefArg rcvr) { return NILREF; }
Ref FExitModalDialog(RefArg rcvr) { return NILREF; }
Ref FExtractData(RefArg rcvr) { return NILREF; }
Ref FExtractRangeAsRichString(RefArg rcvr) { return NILREF; }
Ref FExtractTextRange(RefArg rcvr) { return NILREF; }
Ref FFilterDialog(RefArg rcvr) { return NILREF; }
Ref FFindKeyCommand(RefArg rcvr) { return NILREF; }
Ref FFlushImports(RefArg rcvr) { return NILREF; }
Ref FForkScript(RefArg rcvr) { return NILREF; }
Ref FFulfillImportTable(RefArg rcvr) { return NILREF; }
Ref FGatherKeyCommands(RefArg rcvr) { return NILREF; }
Ref FGestalt(RefArg rcvr) { return NILREF; }
Ref FGesturePoint(RefArg rcvr) { return NILREF; }
Ref FGestureType(RefArg rcvr) { return NILREF; }
Ref FGetAlternates(RefArg rcvr) { return NILREF; }
Ref FGetCalibration(RefArg rcvr) { return NILREF; }
Ref FGetCaretBox(RefArg rcvr) { return NILREF; }
Ref FGetCaretInfo(RefArg rcvr) { return NILREF; }
Ref FGetClipboard(RefArg rcvr) { return NILREF; }
Ref FGetClipboardIcon(RefArg rcvr) { return NILREF; }
Ref FGetDrawBoxX(RefArg rcvr) { return NILREF; }
Ref FGetFlags(RefArg rcvr) { return NILREF; }
Ref FGetFrameStuff(RefArg rcvr) { return NILREF; }
Ref FGetHiliteOffsets(RefArg rcvr) { return NILREF; }
Ref FGetId(RefArg rcvr) { return NILREF; }
Ref FGetInkWordInfo(RefArg rcvr) { return NILREF; }
Ref FGetKeyView(RefArg rcvr) { return NILREF; }
Ref FGetPackages(RefArg rcvr) { return NILREF; }
Ref FGetPoint(RefArg rcvr) { return NILREF; }
Ref FGetPointsArray(RefArg rcvr) { return NILREF; }
Ref FGetPointsArrayXY(RefArg rcvr) { return NILREF; }
Ref FGetPolygons(RefArg rcvr) { return NILREF; }
Ref FGetPopup(RefArg rcvr) { return NILREF; }
Ref FGetRemoteWriting(RefArg rcvr) { return NILREF; }
Ref FGetRoot(RefArg rcvr) { return NILREF; }
Ref FGetScoreArray(RefArg rcvr) { return NILREF; }
Ref FGetStyleAtOffset(RefArg rcvr) { return NILREF; }
Ref FGetStylesOfRange(RefArg rcvr) { return NILREF; }
Ref FGetTextFlags(RefArg rcvr) { return NILREF; }
Ref FGetTrueModifiers(RefArg rcvr) { return NILREF; }
Ref FGetUndoState(RefArg rcvr) { return NILREF; }
Ref FGetUnitDownTime(RefArg rcvr) { return NILREF; }
Ref FGetUnitEndTime(RefArg rcvr) { return NILREF; }
Ref FGetUnitStartTime(RefArg rcvr) { return NILREF; }
Ref FGetUnitUpTime(RefArg rcvr) { return NILREF; }
Ref FGetView(RefArg rcvr) { return NILREF; }
Ref FGetWordArray(RefArg rcvr) { return NILREF; }
Ref FGlobalBoxX(RefArg rcvr) { return NILREF; }
Ref FGlobalOuterBoxX(RefArg rcvr) { return NILREF; }
Ref FHandleInkWord(RefArg rcvr) { return NILREF; }
Ref FHandleInsertItems(RefArg rcvr) { return NILREF; }
Ref FHandleKeyEvents(RefArg rcvr) { return NILREF; }
Ref FHandleRawInk(RefArg rcvr) { return NILREF; }
Ref FHandleUnit(RefArg rcvr) { return NILREF; }
Ref FHideCaret(RefArg rcvr) { return NILREF; }
Ref FHideX(RefArg rcvr) { return NILREF; }
Ref FHiliteOwner(RefArg rcvr) { return NILREF; }
Ref FHiliteUniqueX(RefArg rcvr) { return NILREF; }
Ref FHiliteViewChildren(RefArg rcvr) { return NILREF; }
Ref FHiliteX(RefArg rcvr) { return NILREF; }
Ref FHiliter(RefArg rcvr) { return NILREF; }
Ref FHobbleTablet(RefArg rcvr) { return NILREF; }
Ref FInRepeatedKeyCommand(RefArg rcvr) { return NILREF; }
Ref FInkOff(RefArg rcvr) { return NILREF; }
Ref FInkOffUnHobbled(RefArg rcvr) { return NILREF; }
Ref FInkOn(RefArg rcvr) { return NILREF; }
Ref FInsertItemsAtCaret(RefArg rcvr) { return NILREF; }
Ref FInsertTabletSample(RefArg rcvr) { return NILREF; }
Ref FInsetRect(RefArg rcvr) { return NILREF; }
Ref FInvertRect(RefArg rcvr) { return NILREF; }
Ref FIsCommandKeystroke(RefArg rcvr) { return NILREF; }
Ref FIsKeyDown(RefArg rcvr) { return NILREF; }
Ref FIsPrimShape(RefArg rcvr) { return NILREF; }
Ref FIsPtInRect(RefArg rcvr) { return NILREF; }
Ref FIsTabletCalibrationNeeded(RefArg rcvr) { return NILREF; }
Ref FJournalReplayALine(RefArg rcvr) { return NILREF; }
Ref FJournalReplayAStroke(RefArg rcvr) { return NILREF; }
Ref FJournalReplayBusy(RefArg rcvr) { return NILREF; }
Ref FJournalReplayStrokes(RefArg rcvr) { return NILREF; }
Ref FJournalStartRecord(RefArg rcvr) { return NILREF; }
Ref FJournalStopRecord(RefArg rcvr) { return NILREF; }
Ref FKeyIn(RefArg rcvr) { return NILREF; }
Ref FKeyboardConnected(RefArg rcvr) { return NILREF; }
Ref FLayoutTableX(RefArg rcvr) { return NILREF; }
Ref FLayoutVerticallyX(RefArg rcvr) { return NILREF; }
Ref FLoadFontCache(RefArg rcvr) { return NILREF; }
Ref FLocalBoxX(RefArg rcvr) { return NILREF; }
Ref FMakeBitmap(RefArg rcvr) { return NILREF; }
Ref FMakeLine(RefArg rcvr) { return NILREF; }
Ref FMakeRect(RefArg rcvr) { return NILREF; }
Ref FMakeShape(RefArg rcvr) { return NILREF; }
Ref FMakeText(RefArg rcvr) { return NILREF; }
Ref FMakeTextBox(RefArg rcvr) { return NILREF; }
Ref FMakeTextLines(RefArg rcvr) { return NILREF; }
Ref FMatchKeyMessage(RefArg rcvr) { return NILREF; }
Ref FModalDialog(RefArg rcvr) { return NILREF; }
Ref FModalRecognitionOff(RefArg rcvr) { return NILREF; }
Ref FModalRecognitionOn(RefArg rcvr) { return NILREF; }
Ref FModalState(RefArg rcvr) { return NILREF; }
Ref FMoveBehindX(RefArg rcvr) { return NILREF; }
Ref FNTKAlive(RefArg rcvr) { return NILREF; }
Ref FNTKDownload(RefArg rcvr) { return NILREF; }
Ref FNTKListener(RefArg rcvr) { return NILREF; }
Ref FNTKSend(RefArg rcvr) { return NILREF; }
Ref FNextKeyView(RefArg rcvr) { return NILREF; }
Ref FOffsetRect(RefArg rcvr) { return NILREF; }
Ref FOffsetShape(RefArg rcvr) { return NILREF; }
Ref FOffsetView(RefArg rcvr) { return NILREF; }
Ref FParentX(RefArg rcvr) { return NILREF; }
Ref FPenPos(RefArg rcvr) { return NILREF; }
Ref FPendingImports(RefArg rcvr) { return NILREF; }
Ref FPickViewKeyDown(RefArg rcvr) { return NILREF; }
Ref FPointToCharOffset(RefArg rcvr) { return NILREF; }
Ref FPointToWord(RefArg rcvr) { return NILREF; }
Ref FPointsToArray(RefArg rcvr) { return NILREF; }
Ref FPositionCaret(RefArg rcvr) { return NILREF; }
Ref FPostAndDo(RefArg rcvr) { return NILREF; }
Ref FPostCommand(RefArg rcvr) { return NILREF; }
Ref FPostCommandParam(RefArg rcvr) { return NILREF; }
Ref FPostKeyString(RefArg rcvr) { return NILREF; }
Ref FPurgeAreaCache(RefArg rcvr) { return NILREF; }
Ref FRectsOverlap(RefArg rcvr) { return NILREF; }
Ref FRedoChildrenX(RefArg rcvr) { return NILREF; }
Ref FRegisterGestalt(RefArg rcvr) { return NILREF; }
Ref FRegisterOpenKeyboard(RefArg rcvr) { return NILREF; }
Ref FRelBounds(RefArg rcvr) { return NILREF; }
Ref FRemoveStepView(RefArg rcvr) { return NILREF; }
Ref FRemoveView(RefArg rcvr) { return NILREF; }
Ref FReplaceGestalt(RefArg rcvr) { return NILREF; }
Ref FRestoreKeyView(RefArg rcvr) { return NILREF; }
Ref FRevealEffectX(RefArg rcvr) { return NILREF; }
Ref FSectRect(RefArg rcvr) { return NILREF; }
Ref FSendKeyMessage(RefArg rcvr) { return NILREF; }
Ref FSetBounds(RefArg rcvr) { return NILREF; }
Ref FSetCalibration(RefArg rcvr) { return NILREF; }
Ref FSetCaretInfo(RefArg rcvr) { return NILREF; }
Ref FSetClipboard(RefArg rcvr) { return NILREF; }
Ref FSetHiliteNoUpdateX(RefArg rcvr) { return NILREF; }
Ref FSetHiliteX(RefArg rcvr) { return NILREF; }
Ref FSetKeyView(RefArg rcvr) { return NILREF; }
Ref FSetOriginX(RefArg rcvr) { return NILREF; }
Ref FSetPopup(RefArg rcvr) { return NILREF; }
Ref FSetRemoteWriting(RefArg rcvr) { return NILREF; }
Ref FSetSysAlarm(RefArg rcvr) { return NILREF; }
Ref FSetupIdleX(RefArg rcvr) { return NILREF; }
Ref FShapeBounds(RefArg rcvr) { return NILREF; }
Ref FShowCaret(RefArg rcvr) { return NILREF; }
Ref FShowX(RefArg rcvr) { return NILREF; }
Ref FSlideEffectX(RefArg rcvr) { return NILREF; }
Ref FStartBypassTablet(RefArg rcvr) { return NILREF; }
Ref FStopBypassTablet(RefArg rcvr) { return NILREF; }
Ref FStrokeBounds(RefArg rcvr) { return NILREF; }
Ref FStrokeDone(RefArg rcvr) { return NILREF; }
Ref FStrokeInPicture(RefArg rcvr) { return NILREF; }
Ref FSyncChildrenX(RefArg rcvr) { return NILREF; }
Ref FSyncScrollX(RefArg rcvr) { return NILREF; }
Ref FSyncViewX(RefArg rcvr) { return NILREF; }
Ref FTabletBufferEmpty(RefArg rcvr) { return NILREF; }
Ref FToggleX(RefArg rcvr) { return NILREF; }
Ref FTopicByName(RefArg rcvr) { return NILREF; }
Ref FTrackButtonX(RefArg rcvr) { return NILREF; }
Ref FTrackHiliteX(RefArg rcvr) { return NILREF; }
Ref FTranslateKey(RefArg rcvr) { return NILREF; }
Ref FUnionRect(RefArg rcvr) { return NILREF; }
Ref FUnregisterOpenKeyboard(RefArg rcvr) { return NILREF; }
Ref FViewAutopsy(RefArg rcvr) { return NILREF; }
Ref FViewContainsCaretView(RefArg rcvr) { return NILREF; }
Ref FVisibleBox(RefArg rcvr) { return NILREF; }
Ref FVoteOnWordUnit(RefArg rcvr) { return NILREF; }
Ref FYieldToFork(RefArg rcvr) { return NILREF; }
Ref FpkgDownload(RefArg rcvr) { return NILREF; }
