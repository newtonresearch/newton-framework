/*
	File:		RefStar.s

	Contains:	Newton object system ROM Ref* data.
					This file is built automatically by BuildRefStarDefs()
					from the ROM image and RS symbols.
	Written by:	Newton Research Group, 2016.
*/
		.data
		.align	2

#define kFlagsBinary (0x40<<24)
#define kFlagsArray  (0x41<<24)
#define kFlagsFrame  (0x43<<24)

#if __LP64__
/* Refs are 64-bit, object header is long size|flags, quad gcStuff, quad class */
#define Ref .quad
#define kHeaderSize 20
#define ArrayObj(_len) \
		.long		kHeaderSize + (_len<<3) + kFlagsArray ;\
		Ref		0, MAKEPTR(SYMarray)
#define FrameMapObj(_len) \
		.long		kHeaderSize + ((_len+1)<<3) + kFlagsArray ;\
		Ref		0, MAKEINT(2), NILREF
#define FrameMapProtoObj(_len) \
		.long		kHeaderSize + ((_len+1)<<3) + kFlagsArray ;\
		Ref		0, MAKEINT(2+4), NILREF
#define FrameObj(_len, _map) \
		.long		kHeaderSize + (_len<<3) + kFlagsFrame ;\
		Ref		0, MAKEPTR(_map)

#else
/* Refs are 32-bit, object header is long size|flags, long gcStuff, long class */
#define Ref .long
#define kHeaderSize 12
#define ArrayObj(_len) \
		.long		kHeaderSize + (_len<<2) + kFlagsArray ;\
		Ref		0, MAKEPTR(SYMarray)
#define FrameMapObj(_len) \
		.long		kHeaderSize + ((_len+1)<<2) + kFlagsArray ;\
		Ref		0, MAKEINT(2), NILREF
#define FrameMapProtoObj(_len) \
		.long		kHeaderSize + ((_len+1)<<2) + kFlagsArray ;\
		Ref		0, MAKEINT(2+4), NILREF
#define FrameObj(_len, _map) \
		.long		kHeaderSize + (_len<<2) + kFlagsFrame ;\
		Ref		0, MAKEPTR(_map)
#endif

#define SymbolObj(_len) \
		.long		kHeaderSize + 4 + _len + kFlagsBinary ;\
		Ref		0, 0x55552
#define StringObj(_len) \
		.long		kHeaderSize + ((_len+1)<<1) + kFlagsBinary ;\
		Ref		0, MAKEPTR(SYMstring)

#define NILREF 2
#define MAKEINT(n) (n<<2)
#define MAKEMAGICPTR(n) (n<<2)+3
#define MAKEPTR(p) p+1

		.globl	_RSupBitmap
_RupBitmap:		Ref	MAKEMAGICPTR(323)
_RSupBitmap:	Ref	_RupBitmap
		.globl	_RSdateTimeStrSpecs
_RdateTimeStrSpecs:		Ref	MAKEMAGICPTR(66)
_RSdateTimeStrSpecs:	Ref	_RdateTimeStrSpecs
		.globl	_RSstorageCardTypes
_RstorageCardTypes:		Ref	MAKEMAGICPTR(271)
_RSstorageCardTypes:	Ref	_RstorageCardTypes
		.globl	_RSdirectoryconversionframe
_Rdirectoryconversionframe:		Ref	MAKEMAGICPTR(570)
_RSdirectoryconversionframe:	Ref	_Rdirectoryconversionframe
		.globl	_RSnewtROLabelTimeInputLine
_RnewtROLabelTimeInputLine:		Ref	MAKEMAGICPTR(621)
_RSnewtROLabelTimeInputLine:	Ref	_RnewtROLabelTimeInputLine
		.globl	_RSoverviewBitmap
_RoverviewBitmap:		Ref	MAKEMAGICPTR(329)
_RSoverviewBitmap:	Ref	_RoverviewBitmap
		.globl	_RSprotoLogPicker
_RprotoLogPicker:		Ref	MAKEMAGICPTR(609)
_RSprotoLogPicker:	Ref	_RprotoLogPicker
		.globl	_RSprotoMeetingPlacePopup
_RprotoMeetingPlacePopup:		Ref	MAKEMAGICPTR(392)
_RSprotoMeetingPlacePopup:	Ref	_RprotoMeetingPlacePopup
		.globl	_RSprotoPhoneExpando
_RprotoPhoneExpando:		Ref	MAKEMAGICPTR(194)
_RSprotoPhoneExpando:	Ref	_RprotoPhoneExpando
		.globl	_RSrouteForward
_RrouteForward:		Ref	MAKEMAGICPTR(771)
_RSrouteForward:	Ref	_RrouteForward
		.globl	_RSplusMinusBitmap
_RplusMinusBitmap:		Ref	MAKEMAGICPTR(688)
_RSplusMinusBitmap:	Ref	_RplusMinusBitmap
		.globl	_RSStartIAProgress
_RStartIAProgress:		Ref	MAKEMAGICPTR(296)
_RSStartIAProgress:	Ref	_RStartIAProgress
		.globl	_RSnewtFalseEntryView
_RnewtFalseEntryView:		Ref	MAKEMAGICPTR(652)
_RSnewtFalseEntryView:	Ref	_RnewtFalseEntryView
		.globl	_RSprotoConvertInkSlip
_RprotoConvertInkSlip:		Ref	MAKEMAGICPTR(391)
_RSprotoConvertInkSlip:	Ref	_RprotoConvertInkSlip
		.globl	_RSstylusUpBitmap
_RstylusUpBitmap:		Ref	MAKEMAGICPTR(776)
_RSstylusUpBitmap:	Ref	_RstylusUpBitmap
		.globl	_RSDTMF9
_RDTMF9:		Ref	MAKEMAGICPTR(702)
_RSDTMF9:	Ref	_RDTMF9
		.globl	_RSfinePenTipBitmap
_RfinePenTipBitmap:		Ref	MAKEMAGICPTR(707)
_RSfinePenTipBitmap:	Ref	_RfinePenTipBitmap
		.globl	_RSprintPageMessage
_RprintPageMessage:		Ref	MAKEMAGICPTR(155)
_RSprintPageMessage:	Ref	_RprintPageMessage
		.globl	_RSprotoConfigPicker
_RprotoConfigPicker:		Ref	MAKEMAGICPTR(610)
_RSprotoConfigPicker:	Ref	_RprotoConfigPicker
		.globl	_RSmetaSoupName
_RmetaSoupName:		Ref	MAKEMAGICPTR(129)
_RSmetaSoupName:	Ref	_RmetaSoupName
		.globl	_RSnumberFilter
_RnumberFilter:		Ref	MAKEMAGICPTR(782)
_RSnumberFilter:	Ref	_RnumberFilter
		.globl	_RSsoupFinder
_RsoupFinder:		Ref	MAKEMAGICPTR(237)
_RSsoupFinder:	Ref	_RsoupFinder
		.globl	_RScorrectorTrailBits
_RcorrectorTrailBits:		Ref	MAKEMAGICPTR(799)
_RScorrectorTrailBits:	Ref	_RcorrectorTrailBits
		.globl	_RSinboxSoupName
_RinboxSoupName:		Ref	MAKEMAGICPTR(111)
_RSinboxSoupName:	Ref	_RinboxSoupName
		.globl	_RSusPhoneFilter
_RusPhoneFilter:		Ref	MAKEMAGICPTR(793)
_RSusPhoneFilter:	Ref	_RusPhoneFilter
		.globl	_RSextrasROMIcons
_RextrasROMIcons:		Ref	MAKEMAGICPTR(233)
_RSextrasROMIcons:	Ref	_RextrasROMIcons
		.globl	_RSkbdDeleteBitmap
_RkbdDeleteBitmap:		Ref	MAKEMAGICPTR(752)
_RSkbdDeleteBitmap:	Ref	_RkbdDeleteBitmap
		.globl	_RSprotoLeftRightScroller
_RprotoLeftRightScroller:		Ref	MAKEMAGICPTR(657)
_RSprotoLeftRightScroller:	Ref	_RprotoLeftRightScroller
		.globl	_RSprinterChooserButton
_RprinterChooserButton:		Ref	MAKEMAGICPTR(153)
_RSprinterChooserButton:	Ref	_RprinterChooserButton
		.globl	_RSnewtAreaCodeLine
_RnewtAreaCodeLine:		Ref	MAKEMAGICPTR(26)
_RSnewtAreaCodeLine:	Ref	_RnewtAreaCodeLine
		.globl	_RSprotoSendButton
_RprotoSendButton:		Ref	MAKEMAGICPTR(488)
_RSprotoSendButton:	Ref	_RprotoSendButton
		.globl	_RSDTMF9Bitmap
_RDTMF9Bitmap:		Ref	MAKEMAGICPTR(702)
_RSDTMF9Bitmap:	Ref	_RDTMF9Bitmap
		.globl	_RScloud1
_Rcloud1:		Ref	MAKEMAGICPTR(53)
_RScloud1:	Ref	_Rcloud1
		.globl	_RScanonicalTitle
_RcanonicalTitle:		Ref	MAKEMAGICPTR(41)
_RScanonicalTitle:	Ref	_RcanonicalTitle
		.globl	_RSnewtRONumView
_RnewtRONumView:		Ref	MAKEMAGICPTR(416)
_RSnewtRONumView:	Ref	_RnewtRONumView
		.globl	_RScorrectorTwoButtons
_RcorrectorTwoButtons:		Ref	MAKEMAGICPTR(800)
_RScorrectorTwoButtons:	Ref	_RcorrectorTwoButtons
		.globl	_RSnewtLabelSimpleDateInputLine
_RnewtLabelSimpleDateInputLine:		Ref	MAKEMAGICPTR(682)
_RSnewtLabelSimpleDateInputLine:	Ref	_RnewtLabelSimpleDateInputLine
		.globl	_RScalculator
_Rcalculator:		Ref	MAKEMAGICPTR(17)
_RScalculator:	Ref	_Rcalculator
		.globl	_RSmailEditor
_RmailEditor:		Ref	MAKEMAGICPTR(120)
_RSmailEditor:	Ref	_RmailEditor
		.globl	_RSnewtEmbeddedEntryView
_RnewtEmbeddedEntryView:		Ref	MAKEMAGICPTR(666)
_RSnewtEmbeddedEntryView:	Ref	_RnewtEmbeddedEntryView
		.globl	_RSrecognizerUserChoices
_RrecognizerUserChoices:		Ref	MAKEMAGICPTR(49)
_RSrecognizerUserChoices:	Ref	_RrecognizerUserChoices
		.globl	_RSprotoPrefFrame
_RprotoPrefFrame:		Ref	MAKEMAGICPTR(809)
_RSprotoPrefFrame:	Ref	_RprotoPrefFrame
		.globl	_RSrouteReaddress
_RrouteReaddress:		Ref	MAKEMAGICPTR(757)
_RSrouteReaddress:	Ref	_RrouteReaddress
		.globl	_RSnewtProtoLineBase
_RnewtProtoLineBase:		Ref	MAKEMAGICPTR(564)
_RSnewtProtoLineBase:	Ref	_RnewtProtoLineBase
		.globl	_RSaddingMachine
_RaddingMachine:		Ref	MAKEMAGICPTR(372)
_RSaddingMachine:	Ref	_RaddingMachine
		.globl	_RSextrasDrawer
_RextrasDrawer:		Ref	MAKEMAGICPTR(832)
_RSextrasDrawer:	Ref	_RextrasDrawer
		.globl	_RSletters
_Rletters:		Ref	MAKEMAGICPTR(115)
_RSletters:	Ref	_Rletters
		.globl	_RSconversionRates
_RconversionRates:		Ref	MAKEMAGICPTR(767)
_RSconversionRates:	Ref	_RconversionRates
		.globl	_RSnewtStateFilter
_RnewtStateFilter:		Ref	MAKEMAGICPTR(648)
_RSnewtStateFilter:	Ref	_RnewtStateFilter
		.globl	_RSnewtROLabelNumInputLine
_RnewtROLabelNumInputLine:		Ref	MAKEMAGICPTR(619)
_RSnewtROLabelNumInputLine:	Ref	_RnewtROLabelNumInputLine
		.globl	_RSprotoEditView
_RprotoEditView:		Ref	MAKEMAGICPTR(836)
_RSprotoEditView:	Ref	_RprotoEditView
		.globl	_RSprotoEndpoint
_RprotoEndpoint:		Ref	MAKEMAGICPTR(174)
_RSprotoEndpoint:	Ref	_RprotoEndpoint
		.globl	_RSkbdCapsLockBitmap
_RkbdCapsLockBitmap:		Ref	MAKEMAGICPTR(755)
_RSkbdCapsLockBitmap:	Ref	_RkbdCapsLockBitmap
		.globl	_RSkbdTabBitmap
_RkbdTabBitmap:		Ref	MAKEMAGICPTR(748)
_RSkbdTabBitmap:	Ref	_RkbdTabBitmap
		.globl	_RScanonicalDataContext
_RcanonicalDataContext:		Ref	MAKEMAGICPTR(31)
_RScanonicalDataContext:	Ref	_RcanonicalDataContext
		.globl	_RSDTMF1
_RDTMF1:		Ref	MAKEMAGICPTR(694)
_RSDTMF1:	Ref	_RDTMF1
		.globl	_RSnewtTextView
_RnewtTextView:		Ref	MAKEMAGICPTR(415)
_RSnewtTextView:	Ref	_RnewtTextView
		.globl	_RSstopWordList
_RstopWordList:		Ref	MAKEMAGICPTR(270)
_RSstopWordList:	Ref	_RstopWordList
		.globl	_RScalculatorBitmap
_RcalculatorBitmap:		Ref	MAKEMAGICPTR(333)
_RScalculatorBitmap:	Ref	_RcalculatorBitmap
		.globl	_RSfilledStarBitmap
_RfilledStarBitmap:		Ref	MAKEMAGICPTR(14)
_RSfilledStarBitmap:	Ref	_RfilledStarBitmap
		.globl	_RShandScrollBitmap
_RhandScrollBitmap:		Ref	MAKEMAGICPTR(713)
_RShandScrollBitmap:	Ref	_RhandScrollBitmap
		.globl	_RSdatesBitmap
_RdatesBitmap:		Ref	MAKEMAGICPTR(602)
_RSdatesBitmap:	Ref	_RdatesBitmap
		.globl	_RSlocationPicker
_RlocationPicker:		Ref	MAKEMAGICPTR(512)
_RSlocationPicker:	Ref	_RlocationPicker
		.globl	_RSnotification
_Rnotification:		Ref	MAKEMAGICPTR(135)
_RSnotification:	Ref	_Rnotification
		.globl	_RSprotoDatePicker
_RprotoDatePicker:		Ref	MAKEMAGICPTR(387)
_RSprotoDatePicker:	Ref	_RprotoDatePicker
		.globl	_RSprotoRichLabelInputLine
_RprotoRichLabelInputLine:		Ref	MAKEMAGICPTR(675)
_RSprotoRichLabelInputLine:	Ref	_RprotoRichLabelInputLine
		.globl	_RSdrawerOpen
_RdrawerOpen:		Ref	MAKEMAGICPTR(77)
_RSdrawerOpen:	Ref	_RdrawerOpen
		.globl	_RSremindSlip
_RremindSlip:		Ref	MAKEMAGICPTR(235)
_RSremindSlip:	Ref	_RremindSlip
		.globl	_RSukPhoneFilter
_RukPhoneFilter:		Ref	MAKEMAGICPTR(796)
_RSukPhoneFilter:	Ref	_RukPhoneFilter
		.globl	_RSnotifyCloud
_RnotifyCloud:		Ref	MAKEMAGICPTR(13)
_RSnotifyCloud:	Ref	_RnotifyCloud
		.globl	_RSnewtEntryRollHeader
_RnewtEntryRollHeader:		Ref	MAKEMAGICPTR(410)
_RSnewtEntryRollHeader:	Ref	_RnewtEntryRollHeader
		.globl	_RScardfileIndices
_RcardfileIndices:		Ref	MAKEMAGICPTR(44)
_RScardfileIndices:	Ref	_RcardfileIndices
		.globl	_RSnewtPageLayout
_RnewtPageLayout:		Ref	MAKEMAGICPTR(404)
_RSnewtPageLayout:	Ref	_RnewtPageLayout
		.globl	_RSrouteMissing
_RrouteMissing:		Ref	MAKEMAGICPTR(770)
_RSrouteMissing:	Ref	_RrouteMissing
		.globl	_RSioprotoReceiveButton
_RioprotoReceiveButton:		Ref	MAKEMAGICPTR(822)
_RSioprotoReceiveButton:	Ref	_RioprotoReceiveButton
		.globl	_RSDTMF1Bitmap
_RDTMF1Bitmap:		Ref	MAKEMAGICPTR(694)
_RSDTMF1Bitmap:	Ref	_RDTMF1Bitmap
		.globl	_RSprotoNameRefDataDef
_RprotoNameRefDataDef:		Ref	MAKEMAGICPTR(240)
_RSprotoNameRefDataDef:	Ref	_RprotoNameRefDataDef
		.globl	_RSprotoBottomTitle
_RprotoBottomTitle:		Ref	MAKEMAGICPTR(161)
_RSprotoBottomTitle:	Ref	_RprotoBottomTitle
		.globl	_RSprotoPictureView
_RprotoPictureView:		Ref	MAKEMAGICPTR(839)
_RSprotoPictureView:	Ref	_RprotoPictureView
		.globl	_RSrepeatMeetingName
_RrepeatMeetingName:		Ref	MAKEMAGICPTR(239)
_RSrepeatMeetingName:	Ref	_RrepeatMeetingName
		.globl	_RSprotoStatusTemplate
_RprotoStatusTemplate:		Ref	MAKEMAGICPTR(467)
_RSprotoStatusTemplate:	Ref	_RprotoStatusTemplate
		.globl	_RSkeyDivideBitmap
_RkeyDivideBitmap:		Ref	MAKEMAGICPTR(737)
_RSkeyDivideBitmap:	Ref	_RkeyDivideBitmap
		.globl	_RSprotoRouteSlip
_RprotoRouteSlip:		Ref	MAKEMAGICPTR(486)
_RSprotoRouteSlip:	Ref	_RprotoRouteSlip
		.globl	_RSprotoTitleText
_RprotoTitleText:		Ref	MAKEMAGICPTR(563)
_RSprotoTitleText:	Ref	_RprotoTitleText
		.globl	_RSnewtRollEntryView
_RnewtRollEntryView:		Ref	MAKEMAGICPTR(409)
_RSnewtRollEntryView:	Ref	_RnewtRollEntryView
		.globl	_RSprotoTableDef
_RprotoTableDef:		Ref	MAKEMAGICPTR(224)
_RSprotoTableDef:	Ref	_RprotoTableDef
		.globl	_RSrcPrefsConfig
_RrcPrefsConfig:		Ref	MAKEMAGICPTR(443)
_RSrcPrefsConfig:	Ref	_RrcPrefsConfig
		.globl	_RSdefaultSilentPrefInfo
_RdefaultSilentPrefInfo:		Ref	MAKEMAGICPTR(390)
_RSdefaultSilentPrefInfo:	Ref	_RdefaultSilentPrefInfo
		.globl	_RSaddSound
_RaddSound:		Ref	MAKEMAGICPTR(303)
_RSaddSound:	Ref	_RaddSound
		.globl	_RSnewtNewStationeryButton
_RnewtNewStationeryButton:		Ref	MAKEMAGICPTR(813)
_RSnewtNewStationeryButton:	Ref	_RnewtNewStationeryButton
		.globl	_RSdrawerClose
_RdrawerClose:		Ref	MAKEMAGICPTR(76)
_RSdrawerClose:	Ref	_RdrawerClose
		.globl	_RSlargePenTip
_RlargePenTip:		Ref	MAKEMAGICPTR(710)
_RSlargePenTip:	Ref	_RlargePenTip
		.globl	_RSrightBitmap
_RrightBitmap:		Ref	MAKEMAGICPTR(326)
_RSrightBitmap:	Ref	_RrightBitmap
		.globl	_RScopier
_Rcopier:		Ref	MAKEMAGICPTR(380)
_RScopier:	Ref	_Rcopier
		.globl	_RSmissingViewDef
_RmissingViewDef:		Ref	MAKEMAGICPTR(590)
_RSmissingViewDef:	Ref	_RmissingViewDef
		.globl	_RSprotoRecConfig
_RprotoRecConfig:		Ref	MAKEMAGICPTR(450)
_RSprotoRecConfig:	Ref	_RprotoRecConfig
		.globl	_RSprovincePicker
_RprovincePicker:		Ref	MAKEMAGICPTR(457)
_RSprovincePicker:	Ref	_RprovincePicker
		.globl	_RShandwritingStyleImages
_RhandwritingStyleImages:		Ref	MAKEMAGICPTR(673)
_RShandwritingStyleImages:	Ref	_RhandwritingStyleImages
		.globl	_RSfaxDriver
_RfaxDriver:		Ref	MAKEMAGICPTR(81)
_RSfaxDriver:	Ref	_RfaxDriver
		.globl	_RSassistBitmap
_RassistBitmap:		Ref	MAKEMAGICPTR(592)
_RSassistBitmap:	Ref	_RassistBitmap
		.globl	_RSextrasBitmap
_RextrasBitmap:		Ref	MAKEMAGICPTR(705)
_RSextrasBitmap:	Ref	_RextrasBitmap
		.globl	_RSratchetSound
_RratchetSound:		Ref	MAKEMAGICPTR(302)
_RSratchetSound:	Ref	_RratchetSound
		.globl	_RStopicMarkers
_RtopicMarkers:		Ref	MAKEMAGICPTR(766)
_RStopicMarkers:	Ref	_RtopicMarkers
		.globl	_RSaustPhoneFilter
_RaustPhoneFilter:		Ref	MAKEMAGICPTR(795)
_RSaustPhoneFilter:	Ref	_RaustPhoneFilter
		.globl	_RSnewtFilter
_RnewtFilter:		Ref	MAKEMAGICPTR(638)
_RSnewtFilter:	Ref	_RnewtFilter
		.globl	_RSextrasSoupDef
_RextrasSoupDef:		Ref	MAKEMAGICPTR(307)
_RSextrasSoupDef:	Ref	_RextrasSoupDef
		.globl	_RSlocationPopup
_RlocationPopup:		Ref	MAKEMAGICPTR(436)
_RSlocationPopup:	Ref	_RlocationPopup
		.globl	_RSprotoPrefInfo
_RprotoPrefInfo:		Ref	MAKEMAGICPTR(527)
_RSprotoPrefInfo:	Ref	_RprotoPrefInfo
		.globl	_RSupArrowBitmap
_RupArrowBitmap:		Ref	MAKEMAGICPTR(327)
_RSupArrowBitmap:	Ref	_RupArrowBitmap
		.globl	_RSnewtonNewtBitmap
_RnewtonNewtBitmap:		Ref	MAKEMAGICPTR(778)
_RSnewtonNewtBitmap:	Ref	_RnewtonNewtBitmap
		.globl	_RSprotoNewSetClock
_RprotoNewSetClock:		Ref	MAKEMAGICPTR(378)
_RSprotoNewSetClock:	Ref	_RprotoNewSetClock
		.globl	_RSrcInkOrText
_RrcInkOrText:		Ref	MAKEMAGICPTR(449)
_RSrcInkOrText:	Ref	_RrcInkOrText
		.globl	_RSnewtInfoButton
_RnewtInfoButton:		Ref	MAKEMAGICPTR(124)
_RSnewtInfoButton:	Ref	_RnewtInfoButton
		.globl	_RSprotoLongLatTextPicker
_RprotoLongLatTextPicker:		Ref	MAKEMAGICPTR(523)
_RSprotoLongLatTextPicker:	Ref	_RprotoLongLatTextPicker
		.globl	_RSprotoRoll
_RprotoRoll:		Ref	MAKEMAGICPTR(206)
_RSprotoRoll:	Ref	_RprotoRoll
		.globl	_RSprotoHiliteButton
_RprotoHiliteButton:		Ref	MAKEMAGICPTR(184)
_RSprotoHiliteButton:	Ref	_RprotoHiliteButton
		.globl	_RScanonicalRect
_RcanonicalRect:		Ref	MAKEMAGICPTR(36)
_RScanonicalRect:	Ref	_RcanonicalRect
		.globl	_RScanonicalGroupee
_RcanonicalGroupee:		Ref	MAKEMAGICPTR(34)
_RScanonicalGroupee:	Ref	_RcanonicalGroupee
		.globl	_RSprotoTimeButton
_RprotoTimeButton:		Ref	MAKEMAGICPTR(128)
_RSprotoTimeButton:	Ref	_RprotoTimeButton
		.globl	_RSprotoLabeledBatteryGauge
_RprotoLabeledBatteryGauge:		Ref	MAKEMAGICPTR(316)
_RSprotoLabeledBatteryGauge:	Ref	_RprotoLabeledBatteryGauge
		.globl	_RSglobeBitmap
_RglobeBitmap:		Ref	MAKEMAGICPTR(330)
_RSglobeBitmap:	Ref	_RglobeBitmap
		.globl	_RSdefaultSendPrefInfo
_RdefaultSendPrefInfo:		Ref	MAKEMAGICPTR(483)
_RSdefaultSendPrefInfo:	Ref	_RdefaultSendPrefInfo
		.globl	_RSrcRerecognizeConfig
_RrcRerecognizeConfig:		Ref	MAKEMAGICPTR(447)
_RSrcRerecognizeConfig:	Ref	_RrcRerecognizeConfig
		.globl	_RSconnectMessage
_RconnectMessage:		Ref	MAKEMAGICPTR(56)
_RSconnectMessage:	Ref	_RconnectMessage
		.globl	_RSlargePenTipBitmap
_RlargePenTipBitmap:		Ref	MAKEMAGICPTR(710)
_RSlargePenTipBitmap:	Ref	_RlargePenTipBitmap
		.globl	_RSfontSystem12
_RfontSystem12:		Ref	MAKEMAGICPTR(90)
_RSfontSystem12:	Ref	_RfontSystem12
		.globl	_RSprotoTransportPrefs
_RprotoTransportPrefs:		Ref	MAKEMAGICPTR(678)
_RSprotoTransportPrefs:	Ref	_RprotoTransportPrefs
		.globl	_RSrouteCallBitmap
_RrouteCallBitmap:		Ref	MAKEMAGICPTR(760)
_RSrouteCallBitmap:	Ref	_RrouteCallBitmap
		.globl	_RSampmSlides
_RampmSlides:		Ref	MAKEMAGICPTR(604)
_RSampmSlides:	Ref	_RampmSlides
		.globl	_RSnewtCheckAllButton
_RnewtCheckAllButton:		Ref	MAKEMAGICPTR(872)
_RSnewtCheckAllButton:	Ref	_RnewtCheckAllButton
		.globl	_RSprotoCharEdit
_RprotoCharEdit:		Ref	MAKEMAGICPTR(393)
_RSprotoCharEdit:	Ref	_RprotoCharEdit
		.globl	_RSdigitalTimePopup
_RdigitalTimePopup:		Ref	MAKEMAGICPTR(147)
_RSdigitalTimePopup:	Ref	_RdigitalTimePopup
		.globl	_RSprotoDateExpando
_RprotoDateExpando:		Ref	MAKEMAGICPTR(170)
_RSprotoDateExpando:	Ref	_RprotoDateExpando
		.globl	_RSaliasCursor
_RaliasCursor:		Ref	MAKEMAGICPTR(536)
_RSaliasCursor:	Ref	_RaliasCursor
		.globl	_RShiliteSound
_RhiliteSound:		Ref	MAKEMAGICPTR(110)
_RShiliteSound:	Ref	_RhiliteSound
		.globl	_RSownerBitmap
_RownerBitmap:		Ref	MAKEMAGICPTR(764)
_RSownerBitmap:	Ref	_RownerBitmap
		.globl	_RSsaveLetterWeights
_RsaveLetterWeights:		Ref	MAKEMAGICPTR(252)
_RSsaveLetterWeights:	Ref	_RsaveLetterWeights
		.globl	_RSsoloPokerTemplate
_RsoloPokerTemplate:		Ref	MAKEMAGICPTR(831)
_RSsoloPokerTemplate:	Ref	_RsoloPokerTemplate
		.globl	_RScitySoupName
_RcitySoupName:		Ref	MAKEMAGICPTR(251)
_RScitySoupName:	Ref	_RcitySoupName
		.globl	_RSprotoFloater
_RprotoFloater:		Ref	MAKEMAGICPTR(179)
_RSprotoFloater:	Ref	_RprotoFloater
		.globl	_RSnewtROComboView
_RnewtROComboView:		Ref	MAKEMAGICPTR(441)
_RSnewtROComboView:	Ref	_RnewtROComboView
		.globl	_RSprotoDictionary
_RprotoDictionary:		Ref	MAKEMAGICPTR(171)
_RSprotoDictionary:	Ref	_RprotoDictionary
		.globl	_RSprotoIOCategory
_RprotoIOCategory:		Ref	MAKEMAGICPTR(186)
_RSprotoIOCategory:	Ref	_RprotoIOCategory
		.globl	_RSnewtProtoLine
_RnewtProtoLine:		Ref	MAKEMAGICPTR(565)
_RSnewtProtoLine:	Ref	_RnewtProtoLine
		.globl	_RScanonicalCorrectorAlternates
_RcanonicalCorrectorAlternates:		Ref	MAKEMAGICPTR(348)
_RScanonicalCorrectorAlternates:	Ref	_RcanonicalCorrectorAlternates
		.globl	_RSalarmIconBitmap
_RalarmIconBitmap:		Ref	MAKEMAGICPTR(373)
_RSalarmIconBitmap:	Ref	_RalarmIconBitmap
		.globl	_RSedgeDrawer
_RedgeDrawer:		Ref	MAKEMAGICPTR(78)
_RSedgeDrawer:	Ref	_RedgeDrawer
		.globl	_RSsystemSoupIndexes
_RsystemSoupIndexes:		Ref	MAKEMAGICPTR(275)
_RSsystemSoupIndexes:	Ref	_RsystemSoupIndexes
		.globl	_RSDTMF6
_RDTMF6:		Ref	MAKEMAGICPTR(699)
_RSDTMF6:	Ref	_RDTMF6
		.globl	_RSfontSystem12Underline
_RfontSystem12Underline:		Ref	MAKEMAGICPTR(92)
_RSfontSystem12Underline:	Ref	_RfontSystem12Underline
		.globl	_RSnewtCountrySymbolFilter
_RnewtCountrySymbolFilter:		Ref	MAKEMAGICPTR(146)
_RSnewtCountrySymbolFilter:	Ref	_RnewtCountrySymbolFilter
		.globl	_RScanonicalPopup
_RcanonicalPopup:		Ref	MAKEMAGICPTR(35)
_RScanonicalPopup:	Ref	_RcanonicalPopup
		.globl	_RScloseBoxBitmap
_RcloseBoxBitmap:		Ref	MAKEMAGICPTR(334)
_RScloseBoxBitmap:	Ref	_RcloseBoxBitmap
		.globl	_RSdialTones
_RdialTones:		Ref	MAKEMAGICPTR(68)
_RSdialTones:	Ref	_RdialTones
		.globl	_RSusDateFilter
_RusDateFilter:		Ref	MAKEMAGICPTR(785)
_RSusDateFilter:	Ref	_RusDateFilter
		.globl	_RSanalogTimePopup
_RanalogTimePopup:		Ref	MAKEMAGICPTR(280)
_RSanalogTimePopup:	Ref	_RanalogTimePopup
		.globl	_RSprotoTable
_RprotoTable:		Ref	MAKEMAGICPTR(223)
_RSprotoTable:	Ref	_RprotoTable
		.globl	_RSnewtStationeryPopupButton
_RnewtStationeryPopupButton:		Ref	MAKEMAGICPTR(812)
_RSnewtStationeryPopupButton:	Ref	_RnewtStationeryPopupButton
		.globl	_RSkeyButtBitmap
_RkeyButtBitmap:		Ref	MAKEMAGICPTR(332)
_RSkeyButtBitmap:	Ref	_RkeyButtBitmap
		.globl	_RSprotoKeyboard
_RprotoKeyboard:		Ref	MAKEMAGICPTR(187)
_RSprotoKeyboard:	Ref	_RprotoKeyboard
		.globl	_RSroutingBitmap
_RroutingBitmap:		Ref	MAKEMAGICPTR(312)
_RSroutingBitmap:	Ref	_RroutingBitmap
		.globl	_RSrouteTrashBitmap
_RrouteTrashBitmap:		Ref	MAKEMAGICPTR(291)
_RSrouteTrashBitmap:	Ref	_RrouteTrashBitmap
		.globl	_RSprotoTimeDeltaTextPicker
_RprotoTimeDeltaTextPicker:		Ref	MAKEMAGICPTR(522)
_RSprotoTimeDeltaTextPicker:	Ref	_RprotoTimeDeltaTextPicker
		.globl	_RSfontSystem9
_RfontSystem9:		Ref	MAKEMAGICPTR(99)
_RSfontSystem9:	Ref	_RfontSystem9
		.globl	_RSprotoTracer
_RprotoTracer:		Ref	MAKEMAGICPTR(661)
_RSprotoTracer:	Ref	_RprotoTracer
		.globl	_RSprotoPrintPage
_RprotoPrintPage:		Ref	MAKEMAGICPTR(201)
_RSprotoPrintPage:	Ref	_RprotoPrintPage
		.globl	_RSprintSlip
_RprintSlip:		Ref	MAKEMAGICPTR(156)
_RSprintSlip:	Ref	_RprintSlip
		.globl	_RSyearPopup
_RyearPopup:		Ref	MAKEMAGICPTR(358)
_RSyearPopup:	Ref	_RyearPopup
		.globl	_RSprotoTextPicker
_RprotoTextPicker:		Ref	MAKEMAGICPTR(626)
_RSprotoTextPicker:	Ref	_RprotoTextPicker
		.globl	_RSanalogClock
_RanalogClock:		Ref	MAKEMAGICPTR(5)
_RSanalogClock:	Ref	_RanalogClock
		.globl	_RSDTMF6Bitmap
_RDTMF6Bitmap:		Ref	MAKEMAGICPTR(699)
_RSDTMF6Bitmap:	Ref	_RDTMF6Bitmap
		.globl	_RSjapanPrefectureName
_RjapanPrefectureName:		Ref	MAKEMAGICPTR(856)
_RSjapanPrefectureName:	Ref	_RjapanPrefectureName
		.globl	_RSradioOffBitmap
_RradioOffBitmap:		Ref	MAKEMAGICPTR(297)
_RSradioOffBitmap:	Ref	_RradioOffBitmap
		.globl	_RSnewtROSymbolView
_RnewtROSymbolView:		Ref	MAKEMAGICPTR(494)
_RSnewtROSymbolView:	Ref	_RnewtROSymbolView
		.globl	_RScalendarNotesSoupDef
_RcalendarNotesSoupDef:		Ref	MAKEMAGICPTR(368)
_RScalendarNotesSoupDef:	Ref	_RcalendarNotesSoupDef
		.globl	_RSasciiShift
_RasciiShift:		Ref	MAKEMAGICPTR(7)
_RSasciiShift:	Ref	_RasciiShift
		.globl	_RSflapSlides
_RflapSlides:		Ref	MAKEMAGICPTR(651)
_RSflapSlides:	Ref	_RflapSlides
		.globl	_RStodoName
_RtodoName:		Ref	MAKEMAGICPTR(281)
_RStodoName:	Ref	_RtodoName
		.globl	_RSprotoBeamer
_RprotoBeamer:		Ref	MAKEMAGICPTR(502)
_RSprotoBeamer:	Ref	_RprotoBeamer
		.globl	_RSprinterSerialPicker
_RprinterSerialPicker:		Ref	MAKEMAGICPTR(154)
_RSprinterSerialPicker:	Ref	_RprinterSerialPicker
		.globl	_RSprotoIOStateMachine
_RprotoIOStateMachine:		Ref	MAKEMAGICPTR(501)
_RSprotoIOStateMachine:	Ref	_RprotoIOStateMachine
		.globl	_RSonlineServices
_RonlineServices:		Ref	MAKEMAGICPTR(138)
_RSonlineServices:	Ref	_RonlineServices
		.globl	_RSnewtIntegerFilter
_RnewtIntegerFilter:		Ref	MAKEMAGICPTR(640)
_RSnewtIntegerFilter:	Ref	_RnewtIntegerFilter
		.globl	_RSDSExceptions
_RDSExceptions:		Ref	MAKEMAGICPTR(113)
_RSDSExceptions:	Ref	_RDSExceptions
		.globl	_RSprMemoFormat
_RprMemoFormat:		Ref	MAKEMAGICPTR(871)
_RSprMemoFormat:	Ref	_RprMemoFormat
		.globl	_RSfontSystem9Underline
_RfontSystem9Underline:		Ref	MAKEMAGICPTR(101)
_RSfontSystem9Underline:	Ref	_RfontSystem9Underline
		.globl	_RSkeySmallButtonBitmap
_RkeySmallButtonBitmap:		Ref	MAKEMAGICPTR(715)
_RSkeySmallButtonBitmap:	Ref	_RkeySmallButtonBitmap
		.globl	_RSkeyButtonBitmap
_RkeyButtonBitmap:		Ref	MAKEMAGICPTR(714)
_RSkeyButtonBitmap:	Ref	_RkeyButtonBitmap
		.globl	_RSrepeatQuerySpec
_RrepeatQuerySpec:		Ref	MAKEMAGICPTR(243)
_RSrepeatQuerySpec:	Ref	_RrepeatQuerySpec
		.globl	_RScountryPicker
_RcountryPicker:		Ref	MAKEMAGICPTR(458)
_RScountryPicker:	Ref	_RcountryPicker
		.globl	_RSnameRefValidationFrame
_RnameRefValidationFrame:		Ref	MAKEMAGICPTR(241)
_RSnameRefValidationFrame:	Ref	_RnameRefValidationFrame
		.globl	_RSnorthSouthBitmap
_RnorthSouthBitmap:		Ref	MAKEMAGICPTR(689)
_RSnorthSouthBitmap:	Ref	_RnorthSouthBitmap
		.globl	_RSnewtEntryViewFiling
_RnewtEntryViewFiling:		Ref	MAKEMAGICPTR(408)
_RSnewtEntryViewFiling:	Ref	_RnewtEntryViewFiling
		.globl	_RSrcBuildChains
_RrcBuildChains:		Ref	MAKEMAGICPTR(448)
_RSrcBuildChains:	Ref	_RrcBuildChains
		.globl	_RSprotoPinWithoutLogic
_RprotoPinWithoutLogic:		Ref	MAKEMAGICPTR(557)
_RSprotoPinWithoutLogic:	Ref	_RprotoPinWithoutLogic
		.globl	_RSprotoMultiCursor
_RprotoMultiCursor:		Ref	MAKEMAGICPTR(103)
_RSprotoMultiCursor:	Ref	_RprotoMultiCursor
		.globl	_RSsearchPrefix
_RsearchPrefix:		Ref	MAKEMAGICPTR(255)
_RSsearchPrefix:	Ref	_RsearchPrefix
		.globl	_RSukDateFilter
_RukDateFilter:		Ref	MAKEMAGICPTR(787)
_RSukDateFilter:	Ref	_RukDateFilter
		.globl	_RSROMAvailablePrinters
_RROMAvailablePrinters:		Ref	MAKEMAGICPTR(245)
_RSROMAvailablePrinters:	Ref	_RROMAvailablePrinters
		.globl	_RSworldClock
_RworldClock:		Ref	MAKEMAGICPTR(298)
_RSworldClock:	Ref	_RworldClock
		.globl	_RSnewtLabelInputLine
_RnewtLabelInputLine:		Ref	MAKEMAGICPTR(422)
_RSnewtLabelInputLine:	Ref	_RnewtLabelInputLine
		.globl	_RSdefaultOutboxPrefInfo
_RdefaultOutboxPrefInfo:		Ref	MAKEMAGICPTR(529)
_RSdefaultOutboxPrefInfo:	Ref	_RdefaultOutboxPrefInfo
		.globl	_RSsoundOff
_RsoundOff:		Ref	MAKEMAGICPTR(263)
_RSsoundOff:	Ref	_RsoundOff
		.globl	_RSvconfirm
_Rvconfirm:		Ref	MAKEMAGICPTR(584)
_RSvconfirm:	Ref	_Rvconfirm
		.globl	_RSROMInternational
_RROMInternational:		Ref	MAKEMAGICPTR(246)
_RSROMInternational:	Ref	_RROMInternational
		.globl	_RScuckooSound
_RcuckooSound:		Ref	MAKEMAGICPTR(301)
_RScuckooSound:	Ref	_RcuckooSound
		.globl	_RStimesRomanFont
_RtimesRomanFont:		Ref	MAKEMAGICPTR(278)
_RStimesRomanFont:	Ref	_RtimesRomanFont
		.globl	_RSbootSound
_RbootSound:		Ref	MAKEMAGICPTR(16)
_RSbootSound:	Ref	_RbootSound
		.globl	_RScanonicalDate
_RcanonicalDate:		Ref	MAKEMAGICPTR(32)
_RScanonicalDate:	Ref	_RcanonicalDate
		.globl	_RSprotoTitle
_RprotoTitle:		Ref	MAKEMAGICPTR(229)
_RSprotoTitle:	Ref	_RprotoTitle
		.globl	_RSprotoTextExpando
_RprotoTextExpando:		Ref	MAKEMAGICPTR(227)
_RSprotoTextExpando:	Ref	_RprotoTextExpando
		.globl	_RStabRightHiliteBitmap
_RtabRightHiliteBitmap:		Ref	MAKEMAGICPTR(729)
_RStabRightHiliteBitmap:	Ref	_RtabRightHiliteBitmap
		.globl	_RSnewtOverLayout
_RnewtOverLayout:		Ref	MAKEMAGICPTR(405)
_RSnewtOverLayout:	Ref	_RnewtOverLayout
		.globl	_RSioTransportButton
_RioTransportButton:		Ref	MAKEMAGICPTR(821)
_RSioTransportButton:	Ref	_RioTransportButton
		.globl	_RSnoConvertSoupList
_RnoConvertSoupList:		Ref	MAKEMAGICPTR(578)
_RSnoConvertSoupList:	Ref	_RnoConvertSoupList
		.globl	_RSbindi
_Rbindi:		Ref	MAKEMAGICPTR(691)
_RSbindi:	Ref	_Rbindi
		.globl	_RSclick
_Rclick:		Ref	MAKEMAGICPTR(51)
_RSclick:	Ref	_Rclick
		.globl	_RSaddresseeSlip
_RaddresseeSlip:		Ref	MAKEMAGICPTR(1)
_RSaddresseeSlip:	Ref	_RaddresseeSlip
		.globl	_RSprotoPreferencesField
_RprotoPreferencesField:		Ref	MAKEMAGICPTR(613)
_RSprotoPreferencesField:	Ref	_RprotoPreferencesField
		.globl	_RSphonePad
_RphonePad:		Ref	MAKEMAGICPTR(676)
_RSphonePad:	Ref	_RphonePad
		.globl	_RSdefaultInfoPrefs
_RdefaultInfoPrefs:		Ref	MAKEMAGICPTR(551)
_RSdefaultInfoPrefs:	Ref	_RdefaultInfoPrefs
		.globl	_RSprotoUSStatesTextPicker
_RprotoUSStatesTextPicker:		Ref	MAKEMAGICPTR(633)
_RSprotoUSStatesTextPicker:	Ref	_RprotoUSStatesTextPicker
		.globl	_RSkeyRightParenBitmap
_RkeyRightParenBitmap:		Ref	MAKEMAGICPTR(744)
_RSkeyRightParenBitmap:	Ref	_RkeyRightParenBitmap
		.globl	_RSvgauge
_Rvgauge:		Ref	MAKEMAGICPTR(583)
_RSvgauge:	Ref	_Rvgauge
		.globl	_RSkeyColonBitmap
_RkeyColonBitmap:		Ref	MAKEMAGICPTR(735)
_RSkeyColonBitmap:	Ref	_RkeyColonBitmap
		.globl	_RSkeySlashBitmap
_RkeySlashBitmap:		Ref	MAKEMAGICPTR(745)
_RSkeySlashBitmap:	Ref	_RkeySlashBitmap
		.globl	_RSmidPunctBitmap
_RmidPunctBitmap:		Ref	MAKEMAGICPTR(597)
_RSmidPunctBitmap:	Ref	_RmidPunctBitmap
		.globl	_RSeraseSlip
_ReraseSlip:		Ref	MAKEMAGICPTR(574)
_RSeraseSlip:	Ref	_ReraseSlip
		.globl	_RSprotoStateMachine
_RprotoStateMachine:		Ref	MAKEMAGICPTR(500)
_RSprotoStateMachine:	Ref	_RprotoStateMachine
		.globl	_RSmediumPenTip
_RmediumPenTip:		Ref	MAKEMAGICPTR(709)
_RSmediumPenTip:	Ref	_RmediumPenTip
		.globl	_RScrumple
_Rcrumple:		Ref	MAKEMAGICPTR(62)
_RScrumple:	Ref	_Rcrumple
		.globl	_RSprotoStateMachineInputSpec
_RprotoStateMachineInputSpec:		Ref	MAKEMAGICPTR(525)
_RSprotoStateMachineInputSpec:	Ref	_RprotoStateMachineInputSpec
		.globl	_RSkeyPlusBitmap
_RkeyPlusBitmap:		Ref	MAKEMAGICPTR(742)
_RSkeyPlusBitmap:	Ref	_RkeyPlusBitmap
		.globl	_RSprotoBookmark
_RprotoBookmark:		Ref	MAKEMAGICPTR(159)
_RSprotoBookmark:	Ref	_RprotoBookmark
		.globl	_RSpostmark
_Rpostmark:		Ref	MAKEMAGICPTR(779)
_RSpostmark:	Ref	_Rpostmark
		.globl	_RSrouteLog
_RrouteLog:		Ref	MAKEMAGICPTR(759)
_RSrouteLog:	Ref	_RrouteLog
		.globl	_RSnewtFilingButton
_RnewtFilingButton:		Ref	MAKEMAGICPTR(440)
_RSnewtFilingButton:	Ref	_RnewtFilingButton
		.globl	_RSprotoPictIndexer
_RprotoPictIndexer:		Ref	MAKEMAGICPTR(196)
_RSprotoPictIndexer:	Ref	_RprotoPictIndexer
		.globl	_RSusStdPhoneFilter
_RusStdPhoneFilter:		Ref	MAKEMAGICPTR(792)
_RSusStdPhoneFilter:	Ref	_RusStdPhoneFilter
		.globl	_RSprotoNote
_RprotoNote:		Ref	MAKEMAGICPTR(382)
_RSprotoNote:	Ref	_RprotoNote
		.globl	_RSnewtStationerMenu
_RnewtStationerMenu:		Ref	MAKEMAGICPTR(399)
_RSnewtStationerMenu:	Ref	_RnewtStationerMenu
		.globl	_RSfreshCardName
_RfreshCardName:		Ref	MAKEMAGICPTR(556)
_RSfreshCardName:	Ref	_RfreshCardName
		.globl	_RScalendar
_Rcalendar:		Ref	MAKEMAGICPTR(18)
_RScalendar:	Ref	_Rcalendar
		.globl	_RScardfile
_Rcardfile:		Ref	MAKEMAGICPTR(43)
_RScardfile:	Ref	_Rcardfile
		.globl	_RSvstatus
_Rvstatus:		Ref	MAKEMAGICPTR(580)
_RSvstatus:	Ref	_Rvstatus
		.globl	_RScountrySoupName
_RcountrySoupName:		Ref	MAKEMAGICPTR(295)
_RScountrySoupName:	Ref	_RcountrySoupName
		.globl	_RSprotoInfoButton
_RprotoInfoButton:		Ref	MAKEMAGICPTR(478)
_RSprotoInfoButton:	Ref	_RprotoInfoButton
		.globl	_RSprotoRoutingFormat
_RprotoRoutingFormat:		Ref	MAKEMAGICPTR(260)
_RSprotoRoutingFormat:	Ref	_RprotoRoutingFormat
		.globl	_RScloud3
_Rcloud3:		Ref	MAKEMAGICPTR(55)
_RScloud3:	Ref	_Rcloud3
		.globl	_RSkbdRightBitmap
_RkbdRightBitmap:		Ref	MAKEMAGICPTR(750)
_RSkbdRightBitmap:	Ref	_RkbdRightBitmap
		.globl	_RSkbdShiftBitmap
_RkbdShiftBitmap:		Ref	MAKEMAGICPTR(753)
_RSkbdShiftBitmap:	Ref	_RkbdShiftBitmap
		.globl	_RSrecorderEngine
_RrecorderEngine:		Ref	MAKEMAGICPTR(850)
_RSrecorderEngine:	Ref	_RrecorderEngine
		.globl	_RStransportScripts
_RtransportScripts:		Ref	MAKEMAGICPTR(774)
_RStransportScripts:	Ref	_RtransportScripts
		.globl	_RSdefaultInboxPrefInfo
_RdefaultInboxPrefInfo:		Ref	MAKEMAGICPTR(528)
_RSdefaultInboxPrefInfo:	Ref	_RdefaultInboxPrefInfo
		.globl	_RScoverPageFormat
_RcoverPageFormat:		Ref	MAKEMAGICPTR(60)
_RScoverPageFormat:	Ref	_RcoverPageFormat
		.globl	_RSpageFooter
_RpageFooter:		Ref	MAKEMAGICPTR(140)
_RSpageFooter:	Ref	_RpageFooter
		.globl	_RSmediumPenTipBitmap
_RmediumPenTipBitmap:		Ref	MAKEMAGICPTR(709)
_RSmediumPenTipBitmap:	Ref	_RmediumPenTipBitmap
		.globl	_RSgtScenes
_RgtScenes:		Ref	MAKEMAGICPTR(106)
_RSgtScenes:	Ref	_RgtScenes
		.globl	_RSalarmQuerySpec
_RalarmQuerySpec:		Ref	MAKEMAGICPTR(3)
_RSalarmQuerySpec:	Ref	_RalarmQuerySpec
		.globl	_RSdateIntervalPopup
_RdateIntervalPopup:		Ref	MAKEMAGICPTR(356)
_RSdateIntervalPopup:	Ref	_RdateIntervalPopup
		.globl	_RSprotoPeoplePicker
_RprotoPeoplePicker:		Ref	MAKEMAGICPTR(664)
_RSprotoPeoplePicker:	Ref	_RprotoPeoplePicker
		.globl	_RSuserConfiguration
_RuserConfiguration:		Ref	MAKEMAGICPTR(285)
_RSuserConfiguration:	Ref	_RuserConfiguration
		.globl	_RSoutboxBitmap
_RoutboxBitmap:		Ref	MAKEMAGICPTR(337)
_RSoutboxBitmap:	Ref	_RoutboxBitmap
		.globl	_RStodoSoupName
_RtodoSoupName:		Ref	MAKEMAGICPTR(282)
_RStodoSoupName:	Ref	_RtodoSoupName
		.globl	_RSprotoStateMachineCallbackSpec
_RprotoStateMachineCallbackSpec:		Ref	MAKEMAGICPTR(525)
_RSprotoStateMachineCallbackSpec:	Ref	_RprotoStateMachineCallbackSpec
		.globl	_RSnewtStatusBar
_RnewtStatusBar:		Ref	MAKEMAGICPTR(401)
_RSnewtStatusBar:	Ref	_RnewtStatusBar
		.globl	_RSoutboxSoupDef
_RoutboxSoupDef:		Ref	MAKEMAGICPTR(539)
_RSoutboxSoupDef:	Ref	_RoutboxSoupDef
		.globl	_RScanonicalCharCorrector
_RcanonicalCharCorrector:		Ref	MAKEMAGICPTR(394)
_RScanonicalCharCorrector:	Ref	_RcanonicalCharCorrector
		.globl	_RSprotoOrientation
_RprotoOrientation:		Ref	MAKEMAGICPTR(474)
_RSprotoOrientation:	Ref	_RprotoOrientation
		.globl	_RScalendarSoupDef
_RcalendarSoupDef:		Ref	MAKEMAGICPTR(365)
_RScalendarSoupDef:	Ref	_RcalendarSoupDef
		.globl	_RScardfileSoupDef
_RcardfileSoupDef:		Ref	MAKEMAGICPTR(361)
_RScardfileSoupDef:	Ref	_RcardfileSoupDef
		.globl	_RSnewtStationery
_RnewtStationery:		Ref	MAKEMAGICPTR(451)
_RSnewtStationery:	Ref	_RnewtStationery
		.globl	_RSrcDefaultConfig
_RrcDefaultConfig:		Ref	MAKEMAGICPTR(444)
_RSrcDefaultConfig:	Ref	_RrcDefaultConfig
		.globl	_RScardBitmap
_RcardBitmap:		Ref	MAKEMAGICPTR(347)
_RScardBitmap:	Ref	_RcardBitmap
		.globl	_RSDTMF3
_RDTMF3:		Ref	MAKEMAGICPTR(696)
_RSDTMF3:	Ref	_RDTMF3
		.globl	_RSrepeatIndices
_RrepeatIndices:		Ref	MAKEMAGICPTR(238)
_RSrepeatIndices:	Ref	_RrepeatIndices
		.globl	_RSrouteAddSender
_RrouteAddSender:		Ref	MAKEMAGICPTR(772)
_RSrouteAddSender:	Ref	_RrouteAddSender
		.globl	_RSdefaultStateIcons
_RdefaultStateIcons:		Ref	MAKEMAGICPTR(290)
_RSdefaultStateIcons:	Ref	_RdefaultStateIcons
		.globl	_RSprotoTimeDeltaPicker
_RprotoTimeDeltaPicker:		Ref	MAKEMAGICPTR(516)
_RSprotoTimeDeltaPicker:	Ref	_RprotoTimeDeltaPicker
		.globl	_RSprotoLettersPreferencesField
_RprotoLettersPreferencesField:		Ref	MAKEMAGICPTR(612)
_RSprotoLettersPreferencesField:	Ref	_RprotoLettersPreferencesField
		.globl	_RSnewtApplication
_RnewtApplication:		Ref	MAKEMAGICPTR(398)
_RSnewtApplication:	Ref	_RnewtApplication
		.globl	_RSnewtMailNetChooser
_RnewtMailNetChooser:		Ref	MAKEMAGICPTR(531)
_RSnewtMailNetChooser:	Ref	_RnewtMailNetChooser
		.globl	_RSnewtRollShowStationeryButton
_RnewtRollShowStationeryButton:		Ref	MAKEMAGICPTR(815)
_RSnewtRollShowStationeryButton:	Ref	_RnewtRollShowStationeryButton
		.globl	_RSprotoBorder
_RprotoBorder:		Ref	MAKEMAGICPTR(160)
_RSprotoBorder:	Ref	_RprotoBorder
		.globl	_RSprotoKeypad
_RprotoKeypad:		Ref	MAKEMAGICPTR(188)
_RSprotoKeypad:	Ref	_RprotoKeypad
		.globl	_RSprotoPicker
_RprotoPicker:		Ref	MAKEMAGICPTR(195)
_RSprotoPicker:	Ref	_RprotoPicker
		.globl	_RSprLetterFormat
_RprLetterFormat:		Ref	MAKEMAGICPTR(870)
_RSprLetterFormat:	Ref	_RprLetterFormat
		.globl	_RSnewtEntryShowStationeryButton
_RnewtEntryShowStationeryButton:		Ref	MAKEMAGICPTR(816)
_RSnewtEntryShowStationeryButton:	Ref	_RnewtEntryShowStationeryButton
		.globl	_RSprotoFinderWindow
_RprotoFinderWindow:		Ref	MAKEMAGICPTR(511)
_RSprotoFinderWindow:	Ref	_RprotoFinderWindow
		.globl	_RSioItemStatus
_RioItemStatus:		Ref	MAKEMAGICPTR(817)
_RSioItemStatus:	Ref	_RioItemStatus
		.globl	_RSpasswordSlip
_RpasswordSlip:		Ref	MAKEMAGICPTR(10)
_RSpasswordSlip:	Ref	_RpasswordSlip
		.globl	_RSprotoExtrasIcon
_RprotoExtrasIcon:		Ref	MAKEMAGICPTR(806)
_RSprotoExtrasIcon:	Ref	_RprotoExtrasIcon
		.globl	_RSprotoNewShowBar
_RprotoNewShowBar:		Ref	MAKEMAGICPTR(669)
_RSprotoNewShowBar:	Ref	_RprotoNewShowBar
		.globl	_RScheckBitmap
_RcheckBitmap:		Ref	MAKEMAGICPTR(763)
_RScheckBitmap:	Ref	_RcheckBitmap
		.globl	_RSDTMF3Bitmap
_RDTMF3Bitmap:		Ref	MAKEMAGICPTR(696)
_RSDTMF3Bitmap:	Ref	_RDTMF3Bitmap
		.globl	_RSrouteReply
_RrouteReply:		Ref	MAKEMAGICPTR(756)
_RSrouteReply:	Ref	_RrouteReply
		.globl	_RSnewtROTextDateView
_RnewtROTextDateView:		Ref	MAKEMAGICPTR(418)
_RSnewtROTextDateView:	Ref	_RnewtROTextDateView
		.globl	_RSpriorityItems
_RpriorityItems:		Ref	MAKEMAGICPTR(12)
_RSpriorityItems:	Ref	_RpriorityItems
		.globl	_RScardEventHandlers
_RcardEventHandlers:		Ref	MAKEMAGICPTR(541)
_RScardEventHandlers:	Ref	_RcardEventHandlers
		.globl	_RSnewtTextTimeView
_RnewtTextTimeView:		Ref	MAKEMAGICPTR(492)
_RSnewtTextTimeView:	Ref	_RnewtTextTimeView
		.globl	_RSopenPadlockBitmap
_RopenPadlockBitmap:		Ref	MAKEMAGICPTR(600)
_RSopenPadlockBitmap:	Ref	_RopenPadlockBitmap
		.globl	_RSprotoPeoplePopup
_RprotoPeoplePopup:		Ref	MAKEMAGICPTR(371)
_RSprotoPeoplePopup:	Ref	_RprotoPeoplePopup
		.globl	_RSprotoExtrasListItem
_RprotoExtrasListItem:		Ref	MAKEMAGICPTR(807)
_RSprotoExtrasListItem:	Ref	_RprotoExtrasListItem
		.globl	_RSkeyCapsBitmap
_RkeyCapsBitmap:		Ref	MAKEMAGICPTR(331)
_RSkeyCapsBitmap:	Ref	_RkeyCapsBitmap
		.globl	_RSrouteBeamIcon
_RrouteBeamIcon:		Ref	MAKEMAGICPTR(731)
_RSrouteBeamIcon:	Ref	_RrouteBeamIcon
		.globl	_RScallSlip
_RcallSlip:		Ref	MAKEMAGICPTR(27)
_RScallSlip:	Ref	_RcallSlip
		.globl	_RSnewtNrLabelDatenTimeInputLine
_RnewtNrLabelDatenTimeInputLine:		Ref	MAKEMAGICPTR(637)
_RSnewtNrLabelDatenTimeInputLine:	Ref	_RnewtNrLabelDatenTimeInputLine
		.globl	_RSautodockTemplate
_RautodockTemplate:		Ref	MAKEMAGICPTR(833)
_RSautodockTemplate:	Ref	_RautodockTemplate
		.globl	_RScaretPunctBitmap
_RcaretPunctBitmap:		Ref	MAKEMAGICPTR(595)
_RScaretPunctBitmap:	Ref	_RcaretPunctBitmap
		.globl	_RSinboxBitmap
_RinboxBitmap:		Ref	MAKEMAGICPTR(336)
_RSinboxBitmap:	Ref	_RinboxBitmap
		.globl	_RSprefsBitmap
_RprefsBitmap:		Ref	MAKEMAGICPTR(719)
_RSprefsBitmap:	Ref	_RprefsBitmap
		.globl	_RSnewtDatenTimeFilter
_RnewtDatenTimeFilter:		Ref	MAKEMAGICPTR(644)
_RSnewtDatenTimeFilter:	Ref	_RnewtDatenTimeFilter
		.globl	_RSprotoSchedulePicker
_RprotoSchedulePicker:		Ref	MAKEMAGICPTR(293)
_RSprotoSchedulePicker:	Ref	_RprotoSchedulePicker
		.globl	_RSsettingsShapeBitmap
_RsettingsShapeBitmap:		Ref	MAKEMAGICPTR(724)
_RSsettingsShapeBitmap:	Ref	_RsettingsShapeBitmap
		.globl	_RSrouteUpdateBitmap
_RrouteUpdateBitmap:		Ref	MAKEMAGICPTR(761)
_RSrouteUpdateBitmap:	Ref	_RrouteUpdateBitmap
		.globl	_RSpoof
_Rpoof:		Ref	MAKEMAGICPTR(314)
_RSpoof:	Ref	_Rpoof
		.globl	_RSinboxSoupDef
_RinboxSoupDef:		Ref	MAKEMAGICPTR(538)
_RSinboxSoupDef:	Ref	_RinboxSoupDef
		.globl	_RSprotoDragnGo
_RprotoDragnGo:		Ref	MAKEMAGICPTR(804)
_RSprotoDragnGo:	Ref	_RprotoDragnGo
		.globl	_RSprotoPopInPlace
_RprotoPopInPlace:		Ref	MAKEMAGICPTR(377)
_RSprotoPopInPlace:	Ref	_RprotoPopInPlace
		.globl	_RSazVertTabs
_RazVertTabs:		Ref	MAKEMAGICPTR(339)
_RSazVertTabs:	Ref	_RazVertTabs
		.globl	_RSaustStdPhoneFilter
_RaustStdPhoneFilter:		Ref	MAKEMAGICPTR(794)
_RSaustStdPhoneFilter:	Ref	_RaustStdPhoneFilter
		.globl	_RSprotoSmallKeyboardButton
_RprotoSmallKeyboardButton:		Ref	MAKEMAGICPTR(624)
_RSprotoSmallKeyboardButton:	Ref	_RprotoSmallKeyboardButton
		.globl	_RSioboxCursor
_RioboxCursor:		Ref	MAKEMAGICPTR(540)
_RSioboxCursor:	Ref	_RioboxCursor
		.globl	_RSprotoKeyboardButton
_RprotoKeyboardButton:		Ref	MAKEMAGICPTR(434)
_RSprotoKeyboardButton:	Ref	_RprotoKeyboardButton
		.globl	_RSloadCalibration
_RloadCalibration:		Ref	MAKEMAGICPTR(117)
_RSloadCalibration:	Ref	_RloadCalibration
		.globl	_RSprotoSoftButtonBarIcon
_RprotoSoftButtonBarIcon:		Ref	MAKEMAGICPTR(858)
_RSprotoSoftButtonBarIcon:	Ref	_RprotoSoftButtonBarIcon
		.globl	_RSprotoView
_RprotoView:		Ref	MAKEMAGICPTR(835)
_RSprotoView:	Ref	_RprotoView
		.globl	_RSioTransportOver
_RioTransportOver:		Ref	MAKEMAGICPTR(823)
_RSioTransportOver:	Ref	_RioTransportOver
		.globl	_RSpaperLinedBitmap
_RpaperLinedBitmap:		Ref	MAKEMAGICPTR(717)
_RSpaperLinedBitmap:	Ref	_RpaperLinedBitmap
		.globl	_RSprotoNotifyForm
_RprotoNotifyForm:		Ref	MAKEMAGICPTR(542)
_RSprotoNotifyForm:	Ref	_RprotoNotifyForm
		.globl	_RSprotoStaticText
_RprotoStaticText:		Ref	MAKEMAGICPTR(218)
_RSprotoStaticText:	Ref	_RprotoStaticText
		.globl	_RSprotoStatusIcon
_RprotoStatusIcon:		Ref	MAKEMAGICPTR(469)
_RSprotoStatusIcon:	Ref	_RprotoStatusIcon
		.globl	_RShelpBookBitmap
_RhelpBookBitmap:		Ref	MAKEMAGICPTR(593)
_RShelpBookBitmap:	Ref	_RhelpBookBitmap
		.globl	_RSfontSystem14
_RfontSystem14:		Ref	MAKEMAGICPTR(93)
_RSfontSystem14:	Ref	_RfontSystem14
		.globl	_RSfilingSlip
_RfilingSlip:		Ref	MAKEMAGICPTR(84)
_RSfilingSlip:	Ref	_RfilingSlip
		.globl	_RSrouteDuplicateIcon
_RrouteDuplicateIcon:		Ref	MAKEMAGICPTR(292)
_RSrouteDuplicateIcon:	Ref	_RrouteDuplicateIcon
		.globl	_RSazTabsSlimmaskcz
_RazTabsSlimmaskcz:		Ref	MAKEMAGICPTR(561)
_RSazTabsSlimmaskcz:	Ref	_RazTabsSlimmaskcz
		.globl	_RSalarmSoupConversionFrame
_RalarmSoupConversionFrame:		Ref	MAKEMAGICPTR(503)
_RSalarmSoupConversionFrame:	Ref	_RalarmSoupConversionFrame
		.globl	_RSnewtNrLabelTimeInputLine
_RnewtNrLabelTimeInputLine:		Ref	MAKEMAGICPTR(635)
_RSnewtNrLabelTimeInputLine:	Ref	_RnewtNrLabelTimeInputLine
		.globl	_RSnewtSmartNameFilter
_RnewtSmartNameFilter:		Ref	MAKEMAGICPTR(650)
_RSnewtSmartNameFilter:	Ref	_RnewtSmartNameFilter
		.globl	_RSprotoLabelInputLine
_RprotoLabelInputLine:		Ref	MAKEMAGICPTR(189)
_RSprotoLabelInputLine:	Ref	_RprotoLabelInputLine
		.globl	_RSgtPens
_RgtPens:		Ref	MAKEMAGICPTR(105)
_RSgtPens:	Ref	_RgtPens
		.globl	_RSnewtTimeFilter
_RnewtTimeFilter:		Ref	MAKEMAGICPTR(643)
_RSnewtTimeFilter:	Ref	_RnewtTimeFilter
		.globl	_RSassistant
_Rassistant:		Ref	MAKEMAGICPTR(353)
_RSassistant:	Ref	_Rassistant
		.globl	_RStockSound
_RtockSound:		Ref	MAKEMAGICPTR(300)
_RStockSound:	Ref	_RtockSound
		.globl	_RSprotoPalette
_RprotoPalette:		Ref	MAKEMAGICPTR(482)
_RSprotoPalette:	Ref	_RprotoPalette
		.globl	_RSukCountyName
_RukCountyName:		Ref	MAKEMAGICPTR(524)
_RSukCountyName:	Ref	_RukCountyName
		.globl	_RSnewtSmartPhoneFilter
_RnewtSmartPhoneFilter:		Ref	MAKEMAGICPTR(181)
_RSnewtSmartPhoneFilter:	Ref	_RnewtSmartPhoneFilter
		.globl	_RSprotoMultilinePicker
_RprotoMultilinePicker:		Ref	MAKEMAGICPTR(559)
_RSprotoMultilinePicker:	Ref	_RprotoMultilinePicker
		.globl	_RSclosedPadlockBitmap
_RclosedPadlockBitmap:		Ref	MAKEMAGICPTR(601)
_RSclosedPadlockBitmap:	Ref	_RclosedPadlockBitmap
		.globl	_RSprotoMapTextPicker
_RprotoMapTextPicker:		Ref	MAKEMAGICPTR(631)
_RSprotoMapTextPicker:	Ref	_RprotoMapTextPicker
		.globl	_RSprotoRollItem
_RprotoRollItem:		Ref	MAKEMAGICPTR(208)
_RSprotoRollItem:	Ref	_RprotoRollItem
		.globl	_RSprotoWordInfo
_RprotoWordInfo:		Ref	MAKEMAGICPTR(615)
_RSprotoWordInfo:	Ref	_RprotoWordInfo
		.globl	_RSnewtRollLayout
_RnewtRollLayout:		Ref	MAKEMAGICPTR(403)
_RSnewtRollLayout:	Ref	_RnewtRollLayout
		.globl	_RSprotoMonthView
_RprotoMonthView:		Ref	MAKEMAGICPTR(842)
_RSprotoMonthView:	Ref	_RprotoMonthView
		.globl	_RSprotoPrintForm
_RprotoPrintForm:		Ref	MAKEMAGICPTR(680)
_RSprotoPrintForm:	Ref	_RprotoPrintForm
		.globl	_RSDTMF8
_RDTMF8:		Ref	MAKEMAGICPTR(701)
_RSDTMF8:	Ref	_RDTMF8
		.globl	_RSfontSystem14Underline
_RfontSystem14Underline:		Ref	MAKEMAGICPTR(95)
_RSfontSystem14Underline:	Ref	_RfontSystem14Underline
		.globl	_RSdigitSlides
_RdigitSlides:		Ref	MAKEMAGICPTR(622)
_RSdigitSlides:	Ref	_RdigitSlides
		.globl	_RSalarmIconTinyBitmap
_RalarmIconTinyBitmap:		Ref	MAKEMAGICPTR(591)
_RSalarmIconTinyBitmap:	Ref	_RalarmIconTinyBitmap
		.globl	_RSazTabsMaskcz
_RazTabsMaskcz:		Ref	MAKEMAGICPTR(397)
_RSazTabsMaskcz:	Ref	_RazTabsMaskcz
		.globl	_RStouchtonePad
_RtouchtonePad:		Ref	MAKEMAGICPTR(677)
_RStouchtonePad:	Ref	_RtouchtonePad
		.globl	_RSvphoneKeypad
_RvphoneKeypad:		Ref	MAKEMAGICPTR(586)
_RSvphoneKeypad:	Ref	_RvphoneKeypad
		.globl	_RSusStdDateFilter
_RusStdDateFilter:		Ref	MAKEMAGICPTR(784)
_RSusStdDateFilter:	Ref	_RusStdDateFilter
		.globl	_RSrepeatNotesSoupDef
_RrepeatNotesSoupDef:		Ref	MAKEMAGICPTR(367)
_RSrepeatNotesSoupDef:	Ref	_RrepeatNotesSoupDef
		.globl	_RScountryLocPicker
_RcountryLocPicker:		Ref	MAKEMAGICPTR(553)
_RScountryLocPicker:	Ref	_RcountryLocPicker
		.globl	_RSprotoDatenTimeTextPicker
_RprotoDatenTimeTextPicker:		Ref	MAKEMAGICPTR(630)
_RSprotoDatenTimeTextPicker:	Ref	_RprotoDatenTimeTextPicker
		.globl	_RSprotoSlider
_RprotoSlider:		Ref	MAKEMAGICPTR(212)
_RSprotoSlider:	Ref	_RprotoSlider
		.globl	_RSremoveSound
_RremoveSound:		Ref	MAKEMAGICPTR(304)
_RSremoveSound:	Ref	_RremoveSound
		.globl	_RSprotoDateDurationTextPicker
_RprotoDateDurationTextPicker:		Ref	MAKEMAGICPTR(607)
_RSprotoDateDurationTextPicker:	Ref	_RprotoDateDurationTextPicker
		.globl	_RSnewtROEditView
_RnewtROEditView:		Ref	MAKEMAGICPTR(412)
_RSnewtROEditView:	Ref	_RnewtROEditView
		.globl	_RSnewtSmartCompanyFilter
_RnewtSmartCompanyFilter:		Ref	MAKEMAGICPTR(659)
_RSnewtSmartCompanyFilter:	Ref	_RnewtSmartCompanyFilter
		.globl	_RSpreviewRemoteView
_RpreviewRemoteView:		Ref	MAKEMAGICPTR(679)
_RSpreviewRemoteView:	Ref	_RpreviewRemoteView
		.globl	_RSprotoFilingButton
_RprotoFilingButton:		Ref	MAKEMAGICPTR(176)
_RSprotoFilingButton:	Ref	_RprotoFilingButton
		.globl	_RSparagraphData
_RparagraphData:		Ref	MAKEMAGICPTR(684)
_RSparagraphData:	Ref	_RparagraphData
		.globl	_RSDTMF8Bitmap
_RDTMF8Bitmap:		Ref	MAKEMAGICPTR(701)
_RSDTMF8Bitmap:	Ref	_RDTMF8Bitmap
		.globl	_RSsystemSymbolFont
_RsystemSymbolFont:		Ref	MAKEMAGICPTR(272)
_RSsystemSymbolFont:	Ref	_RsystemSymbolFont
		.globl	_RSletterHeader
_RletterHeader:		Ref	MAKEMAGICPTR(537)
_RSletterHeader:	Ref	_RletterHeader
		.globl	_RSrecInkBitmap
_RrecInkBitmap:		Ref	MAKEMAGICPTR(722)
_RSrecInkBitmap:	Ref	_RrecInkBitmap
		.globl	_RSinfoBitmap
_RinfoBitmap:		Ref	MAKEMAGICPTR(381)
_RSinfoBitmap:	Ref	_RinfoBitmap
		.globl	_RSwakeupBeep
_RwakeupBeep:		Ref	MAKEMAGICPTR(16)
_RSwakeupBeep:	Ref	_RwakeupBeep
		.globl	_RSbasicCallTransport
_RbasicCallTransport:		Ref	MAKEMAGICPTR(535)
_RSbasicCallTransport:	Ref	_RbasicCallTransport
		.globl	_RSprotoDoubleClock
_RprotoDoubleClock:		Ref	MAKEMAGICPTR(464)
_RSprotoDoubleClock:	Ref	_RprotoDoubleClock
		.globl	_RSstarterParagraph
_RstarterParagraph:		Ref	MAKEMAGICPTR(267)
_RSstarterParagraph:	Ref	_RstarterParagraph
		.globl	_RSTickleIAProgress
_RTickleIAProgress:		Ref	MAKEMAGICPTR(306)
_RSTickleIAProgress:	Ref	_RTickleIAProgress
		.globl	_RSblacklistNowBuiltin
_RblacklistNowBuiltin:		Ref	MAKEMAGICPTR(214)
_RSblacklistNowBuiltin:	Ref	_RblacklistNowBuiltin
		.globl	_RScaretPunctBits
_RcaretPunctBits:		Ref	MAKEMAGICPTR(802)
_RScaretPunctBits:	Ref	_RcaretPunctBits
		.globl	_RSrouteFaxBitmap
_RrouteFaxBitmap:		Ref	MAKEMAGICPTR(515)
_RSrouteFaxBitmap:	Ref	_RrouteFaxBitmap
		.globl	_RSdatePopup
_RdatePopup:		Ref	MAKEMAGICPTR(317)
_RSdatePopup:	Ref	_RdatePopup
		.globl	_RSprotoDivider
_RprotoDivider:		Ref	MAKEMAGICPTR(172)
_RSprotoDivider:	Ref	_RprotoDivider
		.globl	_RSvolumeSlider
_RvolumeSlider:		Ref	MAKEMAGICPTR(860)
_RSvolumeSlider:	Ref	_RvolumeSlider
		.globl	_RSprotoLongitudePicker
_RprotoLongitudePicker:		Ref	MAKEMAGICPTR(519)
_RSprotoLongitudePicker:	Ref	_RprotoLongitudePicker
		.globl	_RSrctryLettersConfig
_RrctryLettersConfig:		Ref	MAKEMAGICPTR(446)
_RSrctryLettersConfig:	Ref	_RrctryLettersConfig
		.globl	_RSnewtAboutView
_RnewtAboutView:		Ref	MAKEMAGICPTR(152)
_RSnewtAboutView:	Ref	_RnewtAboutView
		.globl	_RSsystemConversionFrame
_RsystemConversionFrame:		Ref	MAKEMAGICPTR(569)
_RSsystemConversionFrame:	Ref	_RsystemConversionFrame
		.globl	_RSprotoPin
_RprotoPin:		Ref	MAKEMAGICPTR(192)
_RSprotoPin:	Ref	_RprotoPin
		.globl	_RSprotoTimeTextPicker
_RprotoTimeTextPicker:		Ref	MAKEMAGICPTR(627)
_RSprotoTimeTextPicker:	Ref	_RprotoTimeTextPicker
		.globl	_RSprotoStatusBar
_RprotoStatusBar:		Ref	MAKEMAGICPTR(220)
_RSprotoStatusBar:	Ref	_RprotoStatusBar
		.globl	_RSstarterPolygon
_RstarterPolygon:		Ref	MAKEMAGICPTR(268)
_RSstarterPolygon:	Ref	_RstarterPolygon
		.globl	_RSsystemSoupName
_RsystemSoupName:		Ref	MAKEMAGICPTR(276)
_RSsystemSoupName:	Ref	_RsystemSoupName
		.globl	_RSarrayErsatzCursor
_RarrayErsatzCursor:		Ref	MAKEMAGICPTR(505)
_RSarrayErsatzCursor:	Ref	_RarrayErsatzCursor
		.globl	_RSDTMF0
_RDTMF0:		Ref	MAKEMAGICPTR(693)
_RSDTMF0:	Ref	_RDTMF0
		.globl	_RSdigitWideFlap
_RdigitWideFlap:		Ref	MAKEMAGICPTR(687)
_RSdigitWideFlap:	Ref	_RdigitWideFlap
		.globl	_RSfontSystem18Bold
_RfontSystem18Bold:		Ref	MAKEMAGICPTR(97)
_RSfontSystem18Bold:	Ref	_RfontSystem18Bold
		.globl	_RSadditionalBootShiftKeyFn
_RadditionalBootShiftKeyFn:		Ref	MAKEMAGICPTR(808)
_RSadditionalBootShiftKeyFn:	Ref	_RadditionalBootShiftKeyFn
		.globl	_RScopperfield
_Rcopperfield:		Ref	MAKEMAGICPTR(58)
_RScopperfield:	Ref	_Rcopperfield
		.globl	_RSrouteMailBitmap
_RrouteMailBitmap:		Ref	MAKEMAGICPTR(732)
_RSrouteMailBitmap:	Ref	_RrouteMailBitmap
		.globl	_RSukStdDateFilter
_RukStdDateFilter:		Ref	MAKEMAGICPTR(786)
_RSukStdDateFilter:	Ref	_RukStdDateFilter
		.globl	_RSreviewDict
_RreviewDict:		Ref	MAKEMAGICPTR(244)
_RSreviewDict:	Ref	_RreviewDict
		.globl	_RSbootScriptWannabes
_RbootScriptWannabes:		Ref	MAKEMAGICPTR(549)
_RSbootScriptWannabes:	Ref	_RbootScriptWannabes
		.globl	_RSprotoAddressPicker
_RprotoAddressPicker:		Ref	MAKEMAGICPTR(259)
_RSprotoAddressPicker:	Ref	_RprotoAddressPicker
		.globl	_RSabstractArrow
_RabstractArrow:		Ref	MAKEMAGICPTR(568)
_RSabstractArrow:	Ref	_RabstractArrow
		.globl	_RSdateQuerySpec
_RdateQuerySpec:		Ref	MAKEMAGICPTR(65)
_RSdateQuerySpec:	Ref	_RdateQuerySpec
		.globl	_RSprPlainFormat
_RprPlainFormat:		Ref	MAKEMAGICPTR(869)
_RSprPlainFormat:	Ref	_RprPlainFormat
		.globl	_RSsoundRecorder
_RsoundRecorder:		Ref	MAKEMAGICPTR(852)
_RSsoundRecorder:	Ref	_RsoundRecorder
		.globl	_RSprotoFrameFormat
_RprotoFrameFormat:		Ref	MAKEMAGICPTR(52)
_RSprotoFrameFormat:	Ref	_RprotoFrameFormat
		.globl	_RSnumericKeys
_RnumericKeys:		Ref	MAKEMAGICPTR(376)
_RSnumericKeys:	Ref	_RnumericKeys
		.globl	_RSzoneChooser
_RzoneChooser:		Ref	MAKEMAGICPTR(310)
_RSzoneChooser:	Ref	_RzoneChooser
		.globl	_RSnewtNetChooser
_RnewtNetChooser:		Ref	MAKEMAGICPTR(509)
_RSnewtNetChooser:	Ref	_RnewtNetChooser
		.globl	_RSprotoGaugeView
_RprotoGaugeView:		Ref	MAKEMAGICPTR(841)
_RSprotoGaugeView:	Ref	_RprotoGaugeView
		.globl	_RSprotoThumbnail
_RprotoThumbnail:		Ref	MAKEMAGICPTR(484)
_RSprotoThumbnail:	Ref	_RprotoThumbnail
		.globl	_RSrcSingleCharacterConfig
_RrcSingleCharacterConfig:		Ref	MAKEMAGICPTR(445)
_RSrcSingleCharacterConfig:	Ref	_RrcSingleCharacterConfig
		.globl	_RSphoneText
_RphoneText:		Ref	MAKEMAGICPTR(148)
_RSphoneText:	Ref	_RphoneText
		.globl	_RSprotoBook
_RprotoBook:		Ref	MAKEMAGICPTR(158)
_RSprotoBook:	Ref	_RprotoBook
		.globl	_RSnewtSmartNameView
_RnewtSmartNameView:		Ref	MAKEMAGICPTR(427)
_RSnewtSmartNameView:	Ref	_RnewtSmartNameView
		.globl	_RSprotoConfigServer
_RprotoConfigServer:		Ref	MAKEMAGICPTR(499)
_RSprotoConfigServer:	Ref	_RprotoConfigServer
		.globl	_RSIACancelAlert
_RIACancelAlert:		Ref	MAKEMAGICPTR(811)
_RSIACancelAlert:	Ref	_RIACancelAlert
		.globl	_RSnewtstatusbarnoclose
_Rnewtstatusbarnoclose:		Ref	MAKEMAGICPTR(73)
_RSnewtstatusbarnoclose:	Ref	_Rnewtstatusbarnoclose
		.globl	_RSprototransportheader
_Rprototransportheader:		Ref	MAKEMAGICPTR(477)
_RSprototransportheader:	Ref	_Rprototransportheader
		.globl	_RSusStateSoupName
_RusStateSoupName:		Ref	MAKEMAGICPTR(452)
_RSusStateSoupName:	Ref	_RusStateSoupName
		.globl	_RSDTMF0Bitmap
_RDTMF0Bitmap:		Ref	MAKEMAGICPTR(693)
_RSDTMF0Bitmap:	Ref	_RDTMF0Bitmap
		.globl	_RSnewtSmartPhoneView
_RnewtSmartPhoneView:		Ref	MAKEMAGICPTR(428)
_RSnewtSmartPhoneView:	Ref	_RnewtSmartPhoneView
		.globl	_RSprotoCallAssistButton
_RprotoCallAssistButton:		Ref	MAKEMAGICPTR(668)
_RSprotoCallAssistButton:	Ref	_RprotoCallAssistButton
		.globl	_RSfaxHeader
_RfaxHeader:		Ref	MAKEMAGICPTR(82)
_RSfaxHeader:	Ref	_RfaxHeader
		.globl	_RSprotooutCategory
_RprotooutCategory:		Ref	MAKEMAGICPTR(319)
_RSprotooutCategory:	Ref	_RprotooutCategory
		.globl	_RSprotoPrintFormat
_RprotoPrintFormat:		Ref	MAKEMAGICPTR(200)
_RSprotoPrintFormat:	Ref	_RprotoPrintFormat
		.globl	_RSdefRotateFunc
_RdefRotateFunc:		Ref	MAKEMAGICPTR(588)
_RSdefRotateFunc:	Ref	_RdefRotateFunc
		.globl	_RSnewtROLabelDateInputLine
_RnewtROLabelDateInputLine:		Ref	MAKEMAGICPTR(620)
_RSnewtROLabelDateInputLine:	Ref	_RnewtROLabelDateInputLine
		.globl	_RSalarmWakeup
_RalarmWakeup:		Ref	MAKEMAGICPTR(4)
_RSalarmWakeup:	Ref	_RalarmWakeup
		.globl	_RScities
_Rcities:		Ref	MAKEMAGICPTR(50)
_RScities:	Ref	_Rcities
		.globl	_RSblindEntryLine
_RblindEntryLine:		Ref	MAKEMAGICPTR(552)
_RSblindEntryLine:	Ref	_RblindEntryLine
		.globl	_RSworldmapBitmap
_RworldmapBitmap:		Ref	MAKEMAGICPTR(321)
_RSworldmapBitmap:	Ref	_RworldmapBitmap
		.globl	_RSnewtLabelTimeInputLine
_RnewtLabelTimeInputLine:		Ref	MAKEMAGICPTR(493)
_RSnewtLabelTimeInputLine:	Ref	_RnewtLabelTimeInputLine
		.globl	_RSprotoCommand
_RprotoCommand:		Ref	MAKEMAGICPTR(433)
_RSprotoCommand:	Ref	_RprotoCommand
		.globl	_RSprotoTransportPopup
_RprotoTransportPopup:		Ref	MAKEMAGICPTR(534)
_RSprotoTransportPopup:	Ref	_RprotoTransportPopup
		.globl	_RSnotifyCloudMask
_RnotifyCloudMask:		Ref	MAKEMAGICPTR(74)
_RSnotifyCloudMask:	Ref	_RnotifyCloudMask
		.globl	_RSnewtazTabs
_RnewtazTabs:		Ref	MAKEMAGICPTR(430)
_RSnewtazTabs:	Ref	_RnewtazTabs
		.globl	_RSrebootSlip
_RrebootSlip:		Ref	MAKEMAGICPTR(573)
_RSrebootSlip:	Ref	_RrebootSlip
		.globl	_RSprotoCharCorrector
_RprotoCharCorrector:		Ref	MAKEMAGICPTR(394)
_RSprotoCharCorrector:	Ref	_RprotoCharCorrector
		.globl	_RSprotoParagraphView
_RprotoParagraphView:		Ref	MAKEMAGICPTR(837)
_RSprotoParagraphView:	Ref	_RprotoParagraphView
		.globl	_RSsnoozeNotification
_RsnoozeNotification:		Ref	MAKEMAGICPTR(86)
_RSsnoozeNotification:	Ref	_RsnoozeNotification
		.globl	_RSplunk
_Rplunk:		Ref	MAKEMAGICPTR(313)
_RSplunk:	Ref	_Rplunk
		.globl	_RSprotoSetClock
_RprotoSetClock:		Ref	MAKEMAGICPTR(210)
_RSprotoSetClock:	Ref	_RprotoSetClock
		.globl	_RSroutePrintIcon
_RroutePrintIcon:		Ref	MAKEMAGICPTR(730)
_RSroutePrintIcon:	Ref	_RroutePrintIcon
		.globl	_RSletterWeightQuery
_RletterWeightQuery:		Ref	MAKEMAGICPTR(116)
_RSletterWeightQuery:	Ref	_RletterWeightQuery
		.globl	_RScanonicalCompass
_RcanonicalCompass:		Ref	MAKEMAGICPTR(28)
_RScanonicalCompass:	Ref	_RcanonicalCompass
		.globl	_RSfontSystem10Bold
_RfontSystem10Bold:		Ref	MAKEMAGICPTR(88)
_RSfontSystem10Bold:	Ref	_RfontSystem10Bold
		.globl	_RSprotoPolygonView
_RprotoPolygonView:		Ref	MAKEMAGICPTR(838)
_RSprotoPolygonView:	Ref	_RprotoPolygonView
		.globl	_RSprotodefaultstatusbutton
_Rprotodefaultstatusbutton:		Ref	MAKEMAGICPTR(848)
_RSprotodefaultstatusbutton:	Ref	_Rprotodefaultstatusbutton
		.globl	_RSdictionaries
_Rdictionaries:		Ref	MAKEMAGICPTR(69)
_RSdictionaries:	Ref	_Rdictionaries
		.globl	_RStodo2SoupDef
_Rtodo2SoupDef:		Ref	MAKEMAGICPTR(370)
_RStodo2SoupDef:	Ref	_Rtodo2SoupDef
		.globl	_RSrepeatMeetingSoupDef
_RrepeatMeetingSoupDef:		Ref	MAKEMAGICPTR(366)
_RSrepeatMeetingSoupDef:	Ref	_RrepeatMeetingSoupDef
		.globl	_RSfunBeep
_RfunBeep:		Ref	MAKEMAGICPTR(102)
_RSfunBeep:	Ref	_RfunBeep
		.globl	_RSazTabsSlimMaska
_RazTabsSlimMaska:		Ref	MAKEMAGICPTR(562)
_RSazTabsSlimMaska:	Ref	_RazTabsSlimMaska
		.globl	_RSfilingExtBitmap
_RfilingExtBitmap:		Ref	MAKEMAGICPTR(712)
_RSfilingExtBitmap:	Ref	_RfilingExtBitmap
		.globl	_RSkbdOptionBitmap
_RkbdOptionBitmap:		Ref	MAKEMAGICPTR(754)
_RSkbdOptionBitmap:	Ref	_RkbdOptionBitmap
		.globl	_RSnewtfloatingbar
_Rnewtfloatingbar:		Ref	MAKEMAGICPTR(459)
_RSnewtfloatingbar:	Ref	_Rnewtfloatingbar
		.globl	_RSbackupSlip
_RbackupSlip:		Ref	MAKEMAGICPTR(579)
_RSbackupSlip:	Ref	_RbackupSlip
		.globl	_RSnewtClockShowBar
_RnewtClockShowBar:		Ref	MAKEMAGICPTR(162)
_RSnewtClockShowBar:	Ref	_RnewtClockShowBar
		.globl	_RSfaxRoutingIcon
_RfaxRoutingIcon:		Ref	MAKEMAGICPTR(515)
_RSfaxRoutingIcon:	Ref	_RfaxRoutingIcon
		.globl	_RSprotoDayPicker
_RprotoDayPicker:		Ref	MAKEMAGICPTR(605)
_RSprotoDayPicker:	Ref	_RprotoDayPicker
		.globl	_RSpaperrollSoupName
_RpaperrollSoupName:		Ref	MAKEMAGICPTR(144)
_RSpaperrollSoupName:	Ref	_RpaperrollSoupName
		.globl	_RSeworldCountries
_ReworldCountries:		Ref	MAKEMAGICPTR(825)
_RSeworldCountries:	Ref	_ReworldCountries
		.globl	_RSprotoInCategory
_RprotoInCategory:		Ref	MAKEMAGICPTR(318)
_RSprotoInCategory:	Ref	_RprotoInCategory
		.globl	_RSnewtPhonePopupEdit
_RnewtPhonePopupEdit:		Ref	MAKEMAGICPTR(587)
_RSnewtPhonePopupEdit:	Ref	_RnewtPhonePopupEdit
		.globl	_RSprotoContainerView
_RprotoContainerView:		Ref	MAKEMAGICPTR(847)
_RSprotoContainerView:	Ref	_RprotoContainerView
		.globl	_RSprotoRichInputLine
_RprotoRichInputLine:		Ref	MAKEMAGICPTR(674)
_RSprotoRichInputLine:	Ref	_RprotoRichInputLine
		.globl	_RSnewtPrefsView
_RnewtPrefsView:		Ref	MAKEMAGICPTR(107)
_RSnewtPrefsView:	Ref	_RnewtPrefsView
		.globl	_RSuserDictQuery
_RuserDictQuery:		Ref	MAKEMAGICPTR(286)
_RSuserDictQuery:	Ref	_RuserDictQuery
		.globl	_RSnewtNumView
_RnewtNumView:		Ref	MAKEMAGICPTR(417)
_RSnewtNumView:	Ref	_RnewtNumView
		.globl	_RSfrenchDepartmentName
_RfrenchDepartmentName:		Ref	MAKEMAGICPTR(854)
_RSfrenchDepartmentName:	Ref	_RfrenchDepartmentName
		.globl	_RSprotoCorrectContext
_RprotoCorrectContext:		Ref	MAKEMAGICPTR(167)
_RSprotoCorrectContext:	Ref	_RprotoCorrectContext
		.globl	_RScanonicalCorrector
_RcanonicalCorrector:		Ref	MAKEMAGICPTR(30)
_RScanonicalCorrector:	Ref	_RcanonicalCorrector
		.globl	_RSleadingPunctBitmap
_RleadingPunctBitmap:		Ref	MAKEMAGICPTR(596)
_RSleadingPunctBitmap:	Ref	_RleadingPunctBitmap
		.globl	_RSDTMF5
_RDTMF5:		Ref	MAKEMAGICPTR(698)
_RSDTMF5:	Ref	_RDTMF5
		.globl	_RScalendarStuff
_RcalendarStuff:		Ref	MAKEMAGICPTR(25)
_RScalendarStuff:	Ref	_RcalendarStuff
		.globl	_RSazTabsMaska
_RazTabsMaska:		Ref	MAKEMAGICPTR(340)
_RSazTabsMaska:	Ref	_RazTabsMaska
		.globl	_RSalphaKeys
_RalphaKeys:		Ref	MAKEMAGICPTR(375)
_RSalphaKeys:	Ref	_RalphaKeys
		.globl	_RSrcNoRecog
_RrcNoRecog:		Ref	MAKEMAGICPTR(442)
_RSrcNoRecog:	Ref	_RrcNoRecog
		.globl	_RSprotoCheckboxIcon
_RprotoCheckboxIcon:		Ref	MAKEMAGICPTR(165)
_RSprotoCheckboxIcon:	Ref	_RprotoCheckboxIcon
		.globl	_RSprotoPopupButton
_RprotoPopupButton:		Ref	MAKEMAGICPTR(386)
_RSprotoPopupButton:	Ref	_RprotoPopupButton
		.globl	_RSsearchSuffix
_RsearchSuffix:		Ref	MAKEMAGICPTR(256)
_RSsearchSuffix:	Ref	_RsearchSuffix
		.globl	_RSprotoStatusProgress
_RprotoStatusProgress:		Ref	MAKEMAGICPTR(472)
_RSprotoStatusProgress:	Ref	_RprotoStatusProgress
		.globl	_RSusDefaultPhoneFilter
_RusDefaultPhoneFilter:		Ref	MAKEMAGICPTR(791)
_RSusDefaultPhoneFilter:	Ref	_RusDefaultPhoneFilter
		.globl	_RSprotoFindItem
_RprotoFindItem:		Ref	MAKEMAGICPTR(178)
_RSprotoFindItem:	Ref	_RprotoFindItem
		.globl	_RSnewtLabelNumInputLine
_RnewtLabelNumInputLine:		Ref	MAKEMAGICPTR(423)
_RSnewtLabelNumInputLine:	Ref	_RnewtLabelNumInputLine
		.globl	_RSnewtNumberFilter
_RnewtNumberFilter:		Ref	MAKEMAGICPTR(641)
_RSnewtNumberFilter:	Ref	_RnewtNumberFilter
		.globl	_RSnewtShowBar
_RnewtShowBar:		Ref	MAKEMAGICPTR(143)
_RSnewtShowBar:	Ref	_RnewtShowBar
		.globl	_RScanadianProvinceName
_RcanadianProvinceName:		Ref	MAKEMAGICPTR(453)
_RScanadianProvinceName:	Ref	_RcanadianProvinceName
		.globl	_RSmultiDatePopup
_RmultiDatePopup:		Ref	MAKEMAGICPTR(357)
_RSmultiDatePopup:	Ref	_RmultiDatePopup
		.globl	_RSnewtCityFilter
_RnewtCityFilter:		Ref	MAKEMAGICPTR(647)
_RSnewtCityFilter:	Ref	_RnewtCityFilter
		.globl	_RSinitialInheritanceFrame
_RinitialInheritanceFrame:		Ref	MAKEMAGICPTR(112)
_RSinitialInheritanceFrame:	Ref	_RinitialInheritanceFrame
		.globl	_RSprotoStreamingEndpoint
_RprotoStreamingEndpoint:		Ref	MAKEMAGICPTR(466)
_RSprotoStreamingEndpoint:	Ref	_RprotoStreamingEndpoint
		.globl	_RSprotoGeneralPopup
_RprotoGeneralPopup:		Ref	MAKEMAGICPTR(671)
_RSprotoGeneralPopup:	Ref	_RprotoGeneralPopup
		.globl	_RSprotoStatusBarber
_RprotoStatusBarber:		Ref	MAKEMAGICPTR(21)
_RSprotoStatusBarber:	Ref	_RprotoStatusBarber
		.globl	_RSprotoDesktopEndpoint
_RprotoDesktopEndpoint:		Ref	MAKEMAGICPTR(623)
_RSprotoDesktopEndpoint:	Ref	_RprotoDesktopEndpoint
		.globl	_RSprotoPictRadioButton
_RprotoPictRadioButton:		Ref	MAKEMAGICPTR(197)
_RSprotoPictRadioButton:	Ref	_RprotoPictRadioButton
		.globl	_RSkeyRadicalBitmap
_RkeyRadicalBitmap:		Ref	MAKEMAGICPTR(743)
_RSkeyRadicalBitmap:	Ref	_RkeyRadicalBitmap
		.globl	_RSnewtqbeTextView
_RnewtqbeTextView:		Ref	MAKEMAGICPTR(420)
_RSnewtqbeTextView:	Ref	_RnewtqbeTextView
		.globl	_RSdateIndices
_RdateIndices:		Ref	MAKEMAGICPTR(64)
_RSdateIndices:	Ref	_RdateIndices
		.globl	_RSDTMF5bitmap
_RDTMF5bitmap:		Ref	MAKEMAGICPTR(698)
_RSDTMF5bitmap:	Ref	_RDTMF5bitmap
		.globl	_RSprotoDictionaryCursor
_RprotoDictionaryCursor:		Ref	MAKEMAGICPTR(345)
_RSprotoDictionaryCursor:	Ref	_RprotoDictionaryCursor
		.globl	_RSdisconnectMessage
_RdisconnectMessage:		Ref	MAKEMAGICPTR(71)
_RSdisconnectMessage:	Ref	_RdisconnectMessage
		.globl	_RSprotoOutlineView
_RprotoOutlineView:		Ref	MAKEMAGICPTR(844)
_RSprotoOutlineView:	Ref	_RprotoOutlineView
		.globl	_RSfilingBitmap
_RfilingBitmap:		Ref	MAKEMAGICPTR(711)
_RSfilingBitmap:	Ref	_RfilingBitmap
		.globl	_RSfrcanTimeFilter
_RfrcanTimeFilter:		Ref	MAKEMAGICPTR(790)
_RSfrcanTimeFilter:	Ref	_RfrcanTimeFilter
		.globl	_RSnewtROTextView
_RnewtROTextView:		Ref	MAKEMAGICPTR(414)
_RSnewtROTextView:	Ref	_RnewtROTextView
		.globl	_RSsimpleBeep
_RsimpleBeep:		Ref	MAKEMAGICPTR(4)
_RSsimpleBeep:	Ref	_RsimpleBeep
		.globl	_RSlongLatPicker
_RlongLatPicker:		Ref	MAKEMAGICPTR(521)
_RSlongLatPicker:	Ref	_RlongLatPicker
		.globl	_RScanonicalGesturePoint
_RcanonicalGesturePoint:		Ref	MAKEMAGICPTR(352)
_RScanonicalGesturePoint:	Ref	_RcanonicalGesturePoint
		.globl	_RScribNote
_RcribNote:		Ref	MAKEMAGICPTR(61)
_RScribNote:	Ref	_RcribNote
		.globl	_RScolonSlides
_RcolonSlides:		Ref	MAKEMAGICPTR(603)
_RScolonSlides:	Ref	_RcolonSlides
		.globl	_RSstatePicker
_RstatePicker:		Ref	MAKEMAGICPTR(456)
_RSstatePicker:	Ref	_RstatePicker
		.globl	_RStrailingPunctBitmap
_RtrailingPunctBitmap:		Ref	MAKEMAGICPTR(598)
_RStrailingPunctBitmap:	Ref	_RtrailingPunctBitmap
		.globl	_RSkeyTimesBitmap
_RkeyTimesBitmap:		Ref	MAKEMAGICPTR(746)
_RSkeyTimesBitmap:	Ref	_RkeyTimesBitmap
		.globl	_RSespyFont
_RespyFont:		Ref	MAKEMAGICPTR(80)
_RSespyFont:	Ref	_RespyFont
		.globl	_RSnewtSymbolFilter
_RnewtSymbolFilter:		Ref	MAKEMAGICPTR(645)
_RSnewtSymbolFilter:	Ref	_RnewtSymbolFilter
		.globl	_RSprotoCategorizedOverview
_RprotoCategorizedOverview:		Ref	MAKEMAGICPTR(663)
_RSprotoCategorizedOverview:	Ref	_RprotoCategorizedOverview
		.globl	_RSromFontList
_RromFontList:		Ref	MAKEMAGICPTR(533)
_RSromFontList:	Ref	_RromFontList
		.globl	_RSfontStyleItems
_RfontStyleItems:		Ref	MAKEMAGICPTR(855)
_RSfontStyleItems:	Ref	_RfontStyleItems
		.globl	_RSprotoTxViewFinder
_RprotoTxViewFinder:		Ref	MAKEMAGICPTR(827)
_RSprotoTxViewFinder:	Ref	_RprotoTxViewFinder
		.globl	_RSroutePutAway
_RroutePutAway:		Ref	MAKEMAGICPTR(758)
_RSroutePutAway:	Ref	_RroutePutAway
		.globl	_RSmeetingName
_RmeetingName:		Ref	MAKEMAGICPTR(126)
_RSmeetingName:	Ref	_RmeetingName
		.globl	_RSmessageNotification
_RmessageNotification:		Ref	MAKEMAGICPTR(127)
_RSmessageNotification:	Ref	_RmessageNotification
		.globl	_RSprotoPictureButton
_RprotoPictureButton:		Ref	MAKEMAGICPTR(198)
_RSprotoPictureButton:	Ref	_RprotoPictureButton
		.globl	_RSbookmarkBitmap
_RbookmarkBitmap:		Ref	MAKEMAGICPTR(344)
_RSbookmarkBitmap:	Ref	_RbookmarkBitmap
		.globl	_RSkeyCommaBitmap
_RkeyCommaBitmap:		Ref	MAKEMAGICPTR(736)
_RSkeyCommaBitmap:	Ref	_RkeyCommaBitmap
		.globl	_RSplinkBeep
_RplinkBeep:		Ref	MAKEMAGICPTR(150)
_RSplinkBeep:	Ref	_RplinkBeep
		.globl	_RSautodockIcon
_RautodockIcon:		Ref	MAKEMAGICPTR(834)
_RSautodockIcon:	Ref	_RautodockIcon
		.globl	_RSprotoPictTextButton
_RprotoPictTextButton:		Ref	MAKEMAGICPTR(667)
_RSprotoPictTextButton:	Ref	_RprotoPictTextButton
		.globl	_RSundoBitmap
_RundoBitmap:		Ref	MAKEMAGICPTR(733)
_RSundoBitmap:	Ref	_RundoBitmap
		.globl	_RSrecTextBitmap
_RrecTextBitmap:		Ref	MAKEMAGICPTR(720)
_RSrecTextBitmap:	Ref	_RrecTextBitmap
		.globl	_RSnoFilter
_RnoFilter:		Ref	MAKEMAGICPTR(780)
_RSnoFilter:	Ref	_RnoFilter
		.globl	_RStodoSoupDef
_RtodoSoupDef:		Ref	MAKEMAGICPTR(369)
_RStodoSoupDef:	Ref	_RtodoSoupDef
		.globl	_RSprotoLatitudePicker
_RprotoLatitudePicker:		Ref	MAKEMAGICPTR(518)
_RSprotoLatitudePicker:	Ref	_RprotoLatitudePicker
		.globl	_RSstandardStyles
_RstandardStyles:		Ref	MAKEMAGICPTR(264)
_RSstandardStyles:	Ref	_RstandardStyles
		.globl	_RScountries
_Rcountries:		Ref	MAKEMAGICPTR(59)
_RScountries:	Ref	_Rcountries
		.globl	_RSioItemLayout
_RioItemLayout:		Ref	MAKEMAGICPTR(819)
_RSioItemLayout:	Ref	_RioItemLayout
		.globl	_RSglobalHeapVarWannabes
_RglobalHeapVarWannabes:		Ref	MAKEMAGICPTR(547)
_RSglobalHeapVarWannabes:	Ref	_RglobalHeapVarWannabes
		.globl	_RSprotoListPicker
_RprotoListPicker:		Ref	MAKEMAGICPTR(461)
_RSprotoListPicker:	Ref	_RprotoListPicker
		.globl	_RSprotoTableEntry
_RprotoTableEntry:		Ref	MAKEMAGICPTR(225)
_RSprotoTableEntry:	Ref	_RprotoTableEntry
		.globl	_RSnewtSoup
_RnewtSoup:		Ref	MAKEMAGICPTR(429)
_RSnewtSoup:	Ref	_RnewtSoup
		.globl	_RSprotoPenSizeMenu
_RprotoPenSizeMenu:		Ref	MAKEMAGICPTR(868)
_RSprotoPenSizeMenu:	Ref	_RprotoPenSizeMenu
		.globl	_RScheckBitmaps
_RcheckBitmaps:		Ref	MAKEMAGICPTR(11)
_RScheckBitmaps:	Ref	_RcheckBitmaps
		.globl	_RSsaveCalibration
_RsaveCalibration:		Ref	MAKEMAGICPTR(250)
_RSsaveCalibration:	Ref	_RsaveCalibration
		.globl	_RSfindBitmap
_RfindBitmap:		Ref	MAKEMAGICPTR(706)
_RSfindBitmap:	Ref	_RfindBitmap
		.globl	_RSprotoPersonaPopup
_RprotoPersonaPopup:		Ref	MAKEMAGICPTR(497)
_RSprotoPersonaPopup:	Ref	_RprotoPersonaPopup
		.globl	_RScanTimeFilter
_RcanTimeFilter:		Ref	MAKEMAGICPTR(789)
_RScanTimeFilter:	Ref	_RcanTimeFilter
		.globl	_RSpaperrollIndices
_RpaperrollIndices:		Ref	MAKEMAGICPTR(142)
_RSpaperrollIndices:	Ref	_RpaperrollIndices
		.globl	_RSphoneBitmap
_RphoneBitmap:		Ref	MAKEMAGICPTR(322)
_RSphoneBitmap:	Ref	_RphoneBitmap
		.globl	_RSprotoDigitBase
_RprotoDigitBase:		Ref	MAKEMAGICPTR(520)
_RSprotoDigitBase:	Ref	_RprotoDigitBase
		.globl	_RSprotoDragger
_RprotoDragger:		Ref	MAKEMAGICPTR(132)
_RSprotoDragger:	Ref	_RprotoDragger
		.globl	_RSfaxSlip
_RfaxSlip:		Ref	MAKEMAGICPTR(83)
_RSfaxSlip:	Ref	_RfaxSlip
		.globl	_RSputAwayPicker
_RputAwayPicker:		Ref	MAKEMAGICPTR(775)
_RSputAwayPicker:	Ref	_RputAwayPicker
		.globl	_RSprotoCorrectInfo
_RprotoCorrectInfo:		Ref	MAKEMAGICPTR(614)
_RSprotoCorrectInfo:	Ref	_RprotoCorrectInfo
		.globl	_RSaction_list
_Raction_list:		Ref	MAKEMAGICPTR(0)
_RSaction_list:	Ref	_Raction_list
		.globl	_RSprotoDrawer
_RprotoDrawer:		Ref	MAKEMAGICPTR(173)
_RSprotoDrawer:	Ref	_RprotoDrawer
		.globl	_RSkbdDictBitmap
_RkbdDictBitmap:		Ref	MAKEMAGICPTR(751)
_RSkbdDictBitmap:	Ref	_RkbdDictBitmap
		.globl	_RSnewtAreaCodePhoneLine
_RnewtAreaCodePhoneLine:		Ref	MAKEMAGICPTR(294)
_RSnewtAreaCodePhoneLine:	Ref	_RnewtAreaCodePhoneLine
		.globl	_RSprotoRemoteView
_RprotoRemoteView:		Ref	MAKEMAGICPTR(843)
_RSprotoRemoteView:	Ref	_RprotoRemoteView
		.globl	_RSprotoDurationTextPicker
_RprotoDurationTextPicker:		Ref	MAKEMAGICPTR(628)
_RSprotoDurationTextPicker:	Ref	_RprotoDurationTextPicker
		.globl	_RScloud2
_Rcloud2:		Ref	MAKEMAGICPTR(54)
_RScloud2:	Ref	_Rcloud2
		.globl	_RSnewtEntryView
_RnewtEntryView:		Ref	MAKEMAGICPTR(406)
_RSnewtEntryView:	Ref	_RnewtEntryView
		.globl	_RSprotoListView
_RprotoListView:		Ref	MAKEMAGICPTR(465)
_RSprotoListView:	Ref	_RprotoListView
		.globl	_RSprotoOverview
_RprotoOverview:		Ref	MAKEMAGICPTR(191)
_RSprotoOverview:	Ref	_RprotoOverview
		.globl	_RSshapeName
_RshapeName:		Ref	MAKEMAGICPTR(261)
_RSshapeName:	Ref	_RshapeName
		.globl	_RSprotoRoutingSlip
_RprotoRoutingSlip:		Ref	MAKEMAGICPTR(209)
_RSprotoRoutingSlip:	Ref	_RprotoRoutingSlip
		.globl	_RSkeypadBitmap
_RkeypadBitmap:		Ref	MAKEMAGICPTR(716)
_RSkeypadBitmap:	Ref	_RkeypadBitmap
		.globl	_RSprotoTxView
_RprotoTxView:		Ref	MAKEMAGICPTR(826)
_RSprotoTxView:	Ref	_RprotoTxView
		.globl	_RSstdAddressee
_RstdAddressee:		Ref	MAKEMAGICPTR(247)
_RSstdAddressee:	Ref	_RstdAddressee
		.globl	_RSdefaultConfiguration
_RdefaultConfiguration:		Ref	MAKEMAGICPTR(530)
_RSdefaultConfiguration:	Ref	_RdefaultConfiguration
		.globl	_RScalendarStrings
_RcalendarStrings:		Ref	MAKEMAGICPTR(24)
_RScalendarStrings:	Ref	_RcalendarStrings
		.globl	_RSrecorderChassis
_RrecorderChassis:		Ref	MAKEMAGICPTR(851)
_RSrecorderChassis:	Ref	_RrecorderChassis
		.globl	_RSmailSlip
_RmailSlip:		Ref	MAKEMAGICPTR(122)
_RSmailSlip:	Ref	_RmailSlip
		.globl	_RSprotoStatusText
_RprotoStatusText:		Ref	MAKEMAGICPTR(473)
_RSprotoStatusText:	Ref	_RprotoStatusText
		.globl	_RSunionSoupPrototype
_RunionSoupPrototype:		Ref	MAKEMAGICPTR(232)
_RSunionSoupPrototype:	Ref	_RunionSoupPrototype
		.globl	_RSeastWestBitmap
_ReastWestBitmap:		Ref	MAKEMAGICPTR(690)
_RSeastWestBitmap:	Ref	_ReastWestBitmap
		.globl	_RSprotoRCheckbox
_RprotoRCheckbox:		Ref	MAKEMAGICPTR(204)
_RSprotoRCheckbox:	Ref	_RprotoRCheckbox
		.globl	_RStimeDeltaPopup
_RtimeDeltaPopup:		Ref	MAKEMAGICPTR(517)
_RStimeDeltaPopup:	Ref	_RtimeDeltaPopup
		.globl	_RStickSound
_RtickSound:		Ref	MAKEMAGICPTR(299)
_RStickSound:	Ref	_RtickSound
		.globl	_RSprotoCancelButton
_RprotoCancelButton:		Ref	MAKEMAGICPTR(163)
_RSprotoCancelButton:	Ref	_RprotoCancelButton
		.globl	_RSprotoClockShowBar
_RprotoClockShowBar:		Ref	MAKEMAGICPTR(670)
_RSprotoClockShowBar:	Ref	_RprotoClockShowBar
		.globl	_RSprotoDisplayClock
_RprotoDisplayClock:		Ref	MAKEMAGICPTR(504)
_RSprotoDisplayClock:	Ref	_RprotoDisplayClock
		.globl	_RSprotoSoundChannel
_RprotoSoundChannel:		Ref	MAKEMAGICPTR(431)
_RSprotoSoundChannel:	Ref	_RprotoSoundChannel
		.globl	_RSprotoValidateSlip
_RprotoValidateSlip:		Ref	MAKEMAGICPTR(508)
_RSprotoValidateSlip:	Ref	_RprotoValidateSlip
		.globl	_RSusTimeFilter
_RusTimeFilter:		Ref	MAKEMAGICPTR(788)
_RSusTimeFilter:	Ref	_RusTimeFilter
		.globl	_RSsymbolFont
_RsymbolFont:		Ref	MAKEMAGICPTR(272)
_RSsymbolFont:	Ref	_RsymbolFont
		.globl	_RSprotoPickView
_RprotoPickView:		Ref	MAKEMAGICPTR(845)
_RSprotoPickView:	Ref	_RprotoPickView
		.globl	_RSprotoCitiesTextPicker
_RprotoCitiesTextPicker:		Ref	MAKEMAGICPTR(634)
_RSprotoCitiesTextPicker:	Ref	_RprotoCitiesTextPicker
		.globl	_RSnewtCustomFilter
_RnewtCustomFilter:		Ref	MAKEMAGICPTR(437)
_RSnewtCustomFilter:	Ref	_RnewtCustomFilter
		.globl	_RSprotoRadioButton
_RprotoRadioButton:		Ref	MAKEMAGICPTR(202)
_RSprotoRadioButton:	Ref	_RprotoRadioButton
		.globl	_RStryLettersBitmap
_RtryLettersBitmap:		Ref	MAKEMAGICPTR(801)
_RStryLettersBitmap:	Ref	_RtryLettersBitmap
		.globl	_RSnewYorkFont
_RnewYorkFont:		Ref	MAKEMAGICPTR(131)
_RSnewYorkFont:	Ref	_RnewYorkFont
		.globl	_RSnewtSymbolView
_RnewtSymbolView:		Ref	MAKEMAGICPTR(495)
_RSnewtSymbolView:	Ref	_RnewtSymbolView
		.globl	_RSprotoStoryCard
_RprotoStoryCard:		Ref	MAKEMAGICPTR(221)
_RSprotoStoryCard:	Ref	_RprotoStoryCard
		.globl	_RSblacklistGenericApology
_RblacklistGenericApology:		Ref	MAKEMAGICPTR(213)
_RSblacklistGenericApology:	Ref	_RblacklistGenericApology
		.globl	_RScanonicalTextBlock
_RcanonicalTextBlock:		Ref	MAKEMAGICPTR(40)
_RScanonicalTextBlock:	Ref	_RcanonicalTextBlock
		.globl	_RSnewtCountryFilter
_RnewtCountryFilter:		Ref	MAKEMAGICPTR(649)
_RSnewtCountryFilter:	Ref	_RnewtCountryFilter
		.globl	_RSprotoNotesContent
_RprotoNotesContent:		Ref	MAKEMAGICPTR(134)
_RSprotoNotesContent:	Ref	_RprotoNotesContent
		.globl	_RSDTMF2
_RDTMF2:		Ref	MAKEMAGICPTR(695)
_RSDTMF2:	Ref	_RDTMF2
		.globl	_RStabLeftBitmap
_RtabLeftBitmap:		Ref	MAKEMAGICPTR(726)
_RStabLeftBitmap:	Ref	_RtabLeftBitmap
		.globl	_RSnewtRollOverLayout
_RnewtRollOverLayout:		Ref	MAKEMAGICPTR(374)
_RSnewtRollOverLayout:	Ref	_RnewtRollOverLayout
		.globl	_RSusDefaultDateFilter
_RusDefaultDateFilter:		Ref	MAKEMAGICPTR(783)
_RSusDefaultDateFilter:	Ref	_RusDefaultDateFilter
		.globl	_RSbanner
_Rbanner:		Ref	MAKEMAGICPTR(489)
_RSbanner:	Ref	_Rbanner
		.globl	_RSmailRegister
_RmailRegister:		Ref	MAKEMAGICPTR(121)
_RSmailRegister:	Ref	_RmailRegister
		.globl	_RSscheduleView
_RscheduleView:		Ref	MAKEMAGICPTR(254)
_RSscheduleView:	Ref	_RscheduleView
		.globl	_RSkeyEqualsBitmap
_RkeyEqualsBitmap:		Ref	MAKEMAGICPTR(738)
_RSkeyEqualsBitmap:	Ref	_RkeyEqualsBitmap
		.globl	_RSpageCounterForm
_RpageCounterForm:		Ref	MAKEMAGICPTR(355)
_RSpageCounterForm:	Ref	_RpageCounterForm
		.globl	_RSbootSoupWannabes
_RbootSoupWannabes:		Ref	MAKEMAGICPTR(548)
_RSbootSoupWannabes:	Ref	_RbootSoupWannabes
		.globl	_RSnewtLabelSymbolInputLine
_RnewtLabelSymbolInputLine:		Ref	MAKEMAGICPTR(496)
_RSnewtLabelSymbolInputLine:	Ref	_RnewtLabelSymbolInputLine
		.globl	_RSprotoMeetingSoupFinder
_RprotoMeetingSoupFinder:		Ref	MAKEMAGICPTR(149)
_RSprotoMeetingSoupFinder:	Ref	_RprotoMeetingSoupFinder
		.globl	_RSprintForm
_RprintForm:		Ref	MAKEMAGICPTR(341)
_RSprintForm:	Ref	_RprintForm
		.globl	_RSdefaultStatusMsgs
_RdefaultStatusMsgs:		Ref	MAKEMAGICPTR(653)
_RSdefaultStatusMsgs:	Ref	_RdefaultStatusMsgs
		.globl	_RSprotoTAPIEndpoint
_RprotoTAPIEndpoint:		Ref	MAKEMAGICPTR(662)
_RSprotoTAPIEndpoint:	Ref	_RprotoTAPIEndpoint
		.globl	_RSprotoShowBar
_RprotoShowBar:		Ref	MAKEMAGICPTR(211)
_RSprotoShowBar:	Ref	_RprotoShowBar
		.globl	_RSassistUtilities
_RassistUtilities:		Ref	MAKEMAGICPTR(805)
_RSassistUtilities:	Ref	_RassistUtilities
		.globl	_RSDTMF2Bitmap
_RDTMF2Bitmap:		Ref	MAKEMAGICPTR(695)
_RSDTMF2Bitmap:	Ref	_RDTMF2Bitmap
		.globl	_RSprotoPrefsRollItem
_RprotoPrefsRollItem:		Ref	MAKEMAGICPTR(385)
_RSprotoPrefsRollItem:	Ref	_RprotoPrefsRollItem
		.globl	_RSbackdropBitmap
_RbackdropBitmap:		Ref	MAKEMAGICPTR(762)
_RSbackdropBitmap:	Ref	_RbackdropBitmap
		.globl	_RSa2zBitmap
_Ra2zBitmap:		Ref	MAKEMAGICPTR(349)
_RSa2zBitmap:	Ref	_Ra2zBitmap
		.globl	_RSrouteTransport
_RrouteTransport:		Ref	MAKEMAGICPTR(769)
_RSrouteTransport:	Ref	_RrouteTransport
		.globl	_RShelveticaFont
_RhelveticaFont:		Ref	MAKEMAGICPTR(109)
_RShelveticaFont:	Ref	_RhelveticaFont
		.globl	_RSpaletteBitmap
_RpaletteBitmap:		Ref	MAKEMAGICPTR(335)
_RSpaletteBitmap:	Ref	_RpaletteBitmap
		.globl	_RSnewtqbeLabelInputLine
_RnewtqbeLabelInputLine:		Ref	MAKEMAGICPTR(426)
_RSnewtqbeLabelInputLine:	Ref	_RnewtqbeLabelInputLine
		.globl	_RSprotoTextButton
_RprotoTextButton:		Ref	MAKEMAGICPTR(226)
_RSprotoTextButton:	Ref	_RprotoTextButton
		.globl	_RSstarterClipboard
_RstarterClipboard:		Ref	MAKEMAGICPTR(265)
_RSstarterClipboard:	Ref	_RstarterClipboard
		.globl	_RSprotoPeriodicAlarmEditor
_RprotoPeriodicAlarmEditor:		Ref	MAKEMAGICPTR(2)
_RSprotoPeriodicAlarmEditor:	Ref	_RprotoPeriodicAlarmEditor
		.globl	_RSioTransportHeader
_RioTransportHeader:		Ref	MAKEMAGICPTR(818)
_RSioTransportHeader:	Ref	_RioTransportHeader
		.globl	_RSparagraphCodebook2
_RparagraphCodebook2:		Ref	MAKEMAGICPTR(145)
_RSparagraphCodebook2:	Ref	_RparagraphCodebook2
		.globl	_RSassistFrames
_RassistFrames:		Ref	MAKEMAGICPTR(8)
_RSassistFrames:	Ref	_RassistFrames
		.globl	_RScanonicalDictRAMFrame
_RcanonicalDictRAMFrame:		Ref	MAKEMAGICPTR(480)
_RScanonicalDictRAMFrame:	Ref	_RcanonicalDictRAMFrame
		.globl	_RSnewtROLabelInputLine
_RnewtROLabelInputLine:		Ref	MAKEMAGICPTR(421)
_RSnewtROLabelInputLine:	Ref	_RnewtROLabelInputLine
		.globl	_RSnewtPhoneFilter
_RnewtPhoneFilter:		Ref	MAKEMAGICPTR(646)
_RSnewtPhoneFilter:	Ref	_RnewtPhoneFilter
		.globl	_RSprotoPrefFramer
_RprotoPrefFramer:		Ref	MAKEMAGICPTR(810)
_RSprotoPrefFramer:	Ref	_RprotoPrefFramer
		.globl	_RScontinents
_Rcontinents:		Ref	MAKEMAGICPTR(454)
_RScontinents:	Ref	_Rcontinents
		.globl	_RSprotoDigit
_RprotoDigit:		Ref	MAKEMAGICPTR(462)
_RSprotoDigit:	Ref	_RprotoDigit
		.globl	_RSvalidSlots
_RvalidSlots:		Ref	MAKEMAGICPTR(308)
_RSvalidSlots:	Ref	_RvalidSlots
		.globl	_RSprotoFloatnGo
_RprotoFloatnGo:		Ref	MAKEMAGICPTR(180)
_RSprotoFloatnGo:	Ref	_RprotoFloatnGo
		.globl	_RSprotoApp
_RprotoApp:		Ref	MAKEMAGICPTR(157)
_RSprotoApp:	Ref	_RprotoApp
		.globl	_RSioPrintPreview
_RioPrintPreview:		Ref	MAKEMAGICPTR(532)
_RSioPrintPreview:	Ref	_RioPrintPreview
		.globl	_RSprotoCorrector
_RprotoCorrector:		Ref	MAKEMAGICPTR(168)
_RSprotoCorrector:	Ref	_RprotoCorrector
		.globl	_RSasciiBreak
_RasciiBreak:		Ref	MAKEMAGICPTR(6)
_RSasciiBreak:	Ref	_RasciiBreak
		.globl	_RSfontSystem12Bold
_RfontSystem12Bold:		Ref	MAKEMAGICPTR(91)
_RSfontSystem12Bold:	Ref	_RfontSystem12Bold
		.globl	_RSioIndices
_RioIndices:		Ref	MAKEMAGICPTR(114)
_RSioIndices:	Ref	_RioIndices
		.globl	_RSunicode
_Runicode:		Ref	MAKEMAGICPTR(283)
_RSunicode:	Ref	_Runicode
		.globl	_RSgetSerialNumber
_RgetSerialNumber:		Ref	MAKEMAGICPTR(846)
_RSgetSerialNumber:	Ref	_RgetSerialNumber
		.globl	_RSkeyPeriodBitmap
_RkeyPeriodBitmap:		Ref	MAKEMAGICPTR(741)
_RSkeyPeriodBitmap:	Ref	_RkeyPeriodBitmap
		.globl	_RSprotoCountryTextPicker
_RprotoCountryTextPicker:		Ref	MAKEMAGICPTR(632)
_RSprotoCountryTextPicker:	Ref	_RprotoCountryTextPicker
		.globl	_RSprotoSoupOverview
_RprotoSoupOverview:		Ref	MAKEMAGICPTR(460)
_RSprotoSoupOverview:	Ref	_RprotoSoupOverview
		.globl	_RSsmallPenTip
_RsmallPenTip:		Ref	MAKEMAGICPTR(708)
_RSsmallPenTip:	Ref	_RsmallPenTip
		.globl	_RSnewtEntryLockedIcon
_RnewtEntryLockedIcon:		Ref	MAKEMAGICPTR(507)
_RSnewtEntryLockedIcon:	Ref	_RnewtEntryLockedIcon
		.globl	_RSdictionaryList
_RdictionaryList:		Ref	MAKEMAGICPTR(70)
_RSdictionaryList:	Ref	_RdictionaryList
		.globl	_RSkeyMinusBitmap
_RkeyMinusBitmap:		Ref	MAKEMAGICPTR(740)
_RSkeyMinusBitmap:	Ref	_RkeyMinusBitmap
		.globl	_RSprotoImageView
_RprotoImageView:		Ref	MAKEMAGICPTR(485)
_RSprotoImageView:	Ref	_RprotoImageView
		.globl	_RSprotoExpandoShell
_RprotoExpandoShell:		Ref	MAKEMAGICPTR(175)
_RSprotoExpandoShell:	Ref	_RprotoExpandoShell
		.globl	_RSprotoFormatPicker
_RprotoFormatPicker:		Ref	MAKEMAGICPTR(487)
_RSprotoFormatPicker:	Ref	_RprotoFormatPicker
		.globl	_RSprotoNumberPicker
_RprotoNumberPicker:		Ref	MAKEMAGICPTR(72)
_RSprotoNumberPicker:	Ref	_RprotoNumberPicker
		.globl	_RSprotoConfirm
_RprotoConfirm:		Ref	MAKEMAGICPTR(544)
_RSprotoConfirm:	Ref	_RprotoConfirm
		.globl	_RSprotoZonesTable
_RprotoZonesTable:		Ref	MAKEMAGICPTR(230)
_RSprotoZonesTable:	Ref	_RprotoZonesTable
		.globl	_RSprotoTwoLinePicker
_RprotoTwoLinePicker:		Ref	MAKEMAGICPTR(550)
_RSprotoTwoLinePicker:	Ref	_RprotoTwoLinePicker
		.globl	_RScalendarNotesName
_RcalendarNotesName:		Ref	MAKEMAGICPTR(22)
_RScalendarNotesName:	Ref	_RcalendarNotesName
		.globl	_RSextraProtos
_RextraProtos:		Ref	MAKEMAGICPTR(589)
_RSextraProtos:	Ref	_RextraProtos
		.globl	_RSprotoSmartCluster
_RprotoSmartCluster:		Ref	MAKEMAGICPTR(75)
_RSprotoSmartCluster:	Ref	_RprotoSmartCluster
		.globl	_RSDTMF7
_RDTMF7:		Ref	MAKEMAGICPTR(700)
_RSDTMF7:	Ref	_RDTMF7
		.globl	_RScontainerName
_RcontainerName:		Ref	MAKEMAGICPTR(57)
_RScontainerName:	Ref	_RcontainerName
		.globl	_RScanonicalContext
_RcanonicalContext:		Ref	MAKEMAGICPTR(29)
_RScanonicalContext:	Ref	_RcanonicalContext
		.globl	_RSpreparingMessage
_RpreparingMessage:		Ref	MAKEMAGICPTR(151)
_RSpreparingMessage:	Ref	_RpreparingMessage
		.globl	_RSprotoDeckOfCards
_RprotoDeckOfCards:		Ref	MAKEMAGICPTR(828)
_RSprotoDeckOfCards:	Ref	_RprotoDeckOfCards
		.globl	_RSstampFrameBitmap
_RstampFrameBitmap:		Ref	MAKEMAGICPTR(725)
_RSstampFrameBitmap:	Ref	_RstampFrameBitmap
		.globl	_RSaustralianStateName
_RaustralianStateName:		Ref	MAKEMAGICPTR(513)
_RSaustralianStateName:	Ref	_RaustralianStateName
		.globl	_RSnewtEntryPageHeader
_RnewtEntryPageHeader:		Ref	MAKEMAGICPTR(309)
_RSnewtEntryPageHeader:	Ref	_RnewtEntryPageHeader
		.globl	_RSprotoEmporiumPopup
_RprotoEmporiumPopup:		Ref	MAKEMAGICPTR(498)
_RSprotoEmporiumPopup:	Ref	_RprotoEmporiumPopup
		.globl	_RSonlineMessages
_RonlineMessages:		Ref	MAKEMAGICPTR(137)
_RSonlineMessages:	Ref	_RonlineMessages
		.globl	_RSstateLocPicker
_RstateLocPicker:		Ref	MAKEMAGICPTR(554)
_RSstateLocPicker:	Ref	_RstateLocPicker
		.globl	_RSprotoHorizontalUpDownScroller
_RprotoHorizontalUpDownScroller:		Ref	MAKEMAGICPTR(475)
_RSprotoHorizontalUpDownScroller:	Ref	_RprotoHorizontalUpDownScroller
		.globl	_RSsmallPenTipBitmap
_RsmallPenTipBitmap:		Ref	MAKEMAGICPTR(708)
_RSsmallPenTipBitmap:	Ref	_RsmallPenTipBitmap
		.globl	_RSflip
_Rflip:		Ref	MAKEMAGICPTR(85)
_RSflip:	Ref	_Rflip
		.globl	_RSprotoExtrasControlButton
_RprotoExtrasControlButton:		Ref	MAKEMAGICPTR(857)
_RSprotoExtrasControlButton:	Ref	_RprotoExtrasControlButton
		.globl	_RSmarkupBitmap
_RmarkupBitmap:		Ref	MAKEMAGICPTR(342)
_RSmarkupBitmap:	Ref	_RmarkupBitmap
		.globl	_RSdefaultItemStateMsgs
_RdefaultItemStateMsgs:		Ref	MAKEMAGICPTR(654)
_RSdefaultItemStateMsgs:	Ref	_RdefaultItemStateMsgs
		.globl	_RSprotoDatenYearPicker
_RprotoDatenYearPicker:		Ref	MAKEMAGICPTR(555)
_RSprotoDatenYearPicker:	Ref	_RprotoDatenYearPicker
		.globl	_RSfontSystem9Bold
_RfontSystem9Bold:		Ref	MAKEMAGICPTR(100)
_RSfontSystem9Bold:	Ref	_RfontSystem9Bold
		.globl	_RShandwritingFont
_RhandwritingFont:		Ref	MAKEMAGICPTR(571)
_RShandwritingFont:	Ref	_RhandwritingFont
		.globl	_RSprotoGauge
_RprotoGauge:		Ref	MAKEMAGICPTR(182)
_RSprotoGauge:	Ref	_RprotoGauge
		.globl	_RSprotoPreferencesTitle
_RprotoPreferencesTitle:		Ref	MAKEMAGICPTR(384)
_RSprotoPreferencesTitle:	Ref	_RprotoPreferencesTitle
		.globl	_RScanonicalBaseInfo
_RcanonicalBaseInfo:		Ref	MAKEMAGICPTR(681)
_RScanonicalBaseInfo:	Ref	_RcanonicalBaseInfo
		.globl	_RSnewtLabelCustomInputLine
_RnewtLabelCustomInputLine:		Ref	MAKEMAGICPTR(438)
_RSnewtLabelCustomInputLine:	Ref	_RnewtLabelCustomInputLine
		.globl	_RSaboutNewton
_RaboutNewton:		Ref	MAKEMAGICPTR(859)
_RSaboutNewton:	Ref	_RaboutNewton
		.globl	_RSmodemSetups
_RmodemSetups:		Ref	MAKEMAGICPTR(217)
_RSmodemSetups:	Ref	_RmodemSetups
		.globl	_RSprotoNavigator
_RprotoNavigator:		Ref	MAKEMAGICPTR(354)
_RSprotoNavigator:	Ref	_RprotoNavigator
		.globl	_RStimeIntervalPopup
_RtimeIntervalPopup:		Ref	MAKEMAGICPTR(284)
_RStimeIntervalPopup:	Ref	_RtimeIntervalPopup
		.globl	_RScompatibleFinder
_RcompatibleFinder:		Ref	MAKEMAGICPTR(359)
_RScompatibleFinder:	Ref	_RcompatibleFinder
		.globl	_RSDTMF7Bitmap
_RDTMF7Bitmap:		Ref	MAKEMAGICPTR(700)
_RSDTMF7Bitmap:	Ref	_RDTMF7Bitmap
		.globl	_RStypewriter
_Rtypewriter:		Ref	MAKEMAGICPTR(432)
_RStypewriter:	Ref	_Rtypewriter
		.globl	_RSrecShapeBitmap
_RrecShapeBitmap:		Ref	MAKEMAGICPTR(721)
_RSrecShapeBitmap:	Ref	_RrecShapeBitmap
		.globl	_RSprotoRollBrowser
_RprotoRollBrowser:		Ref	MAKEMAGICPTR(207)
_RSprotoRollBrowser:	Ref	_RprotoRollBrowser
		.globl	_RSprotoStatus
_RprotoStatus:		Ref	MAKEMAGICPTR(219)
_RSprotoStatus:	Ref	_RprotoStatus
		.globl	_RSprotoTransport
_RprotoTransport:		Ref	MAKEMAGICPTR(389)
_RSprotoTransport:	Ref	_RprotoTransport
		.globl	_RSrouteBeamBitmap
_RrouteBeamBitmap:		Ref	MAKEMAGICPTR(731)
_RSrouteBeamBitmap:	Ref	_RrouteBeamBitmap
		.globl	_RSbookBitmap
_RbookBitmap:		Ref	MAKEMAGICPTR(351)
_RSbookBitmap:	Ref	_RbookBitmap
		.globl	_RSleftBitmap
_RleftBitmap:		Ref	MAKEMAGICPTR(325)
_RSleftBitmap:	Ref	_RleftBitmap
		.globl	_RSnewtEntryViewRouting
_RnewtEntryViewRouting:		Ref	MAKEMAGICPTR(407)
_RSnewtEntryViewRouting:	Ref	_RnewtEntryViewRouting
		.globl	_RSabstractScroller
_RabstractScroller:		Ref	MAKEMAGICPTR(567)
_RSabstractScroller:	Ref	_RabstractScroller
		.globl	_RSprotoRecognitionCheckbox
_RprotoRecognitionCheckbox:		Ref	MAKEMAGICPTR(205)
_RSprotoRecognitionCheckbox:	Ref	_RprotoRecognitionCheckbox
		.globl	_RSmpTableNoop
_RmpTableNoop:		Ref	MAKEMAGICPTR(861)
_RSmpTableNoop:	Ref	_RmpTableNoop
		.globl	_RSnoMarkupBitmap
_RnoMarkupBitmap:		Ref	MAKEMAGICPTR(343)
_RSnoMarkupBitmap:	Ref	_RnoMarkupBitmap
		.globl	_RSprotoParagraph
_RprotoParagraph:		Ref	MAKEMAGICPTR(193)
_RSprotoParagraph:	Ref	_RprotoParagraph
		.globl	_RSbuttonBar
_RbuttonBar:		Ref	MAKEMAGICPTR(692)
_RSbuttonBar:	Ref	_RbuttonBar
		.globl	_RSDTMFPound
_RDTMFPound:		Ref	MAKEMAGICPTR(704)
_RSDTMFPound:	Ref	_RDTMFPound
		.globl	_RSprotoFindCategory
_RprotoFindCategory:		Ref	MAKEMAGICPTR(177)
_RSprotoFindCategory:	Ref	_RprotoFindCategory
		.globl	_RSprotoRepeatPicker
_RprotoRepeatPicker:		Ref	MAKEMAGICPTR(123)
_RSprotoRepeatPicker:	Ref	_RprotoRepeatPicker
		.globl	_RSprotoCursiveCheckbox
_RprotoCursiveCheckbox:		Ref	MAKEMAGICPTR(169)
_RSprotoCursiveCheckbox:	Ref	_RprotoCursiveCheckbox
		.globl	_RSprotoYearPicker
_RprotoYearPicker:		Ref	MAKEMAGICPTR(108)
_RSprotoYearPicker:	Ref	_RprotoYearPicker
		.globl	_RSprotoDiamondButton
_RprotoDiamondButton:		Ref	MAKEMAGICPTR(625)
_RSprotoDiamondButton:	Ref	_RprotoDiamondButton
		.globl	_RSprotoCloseBox
_RprotoCloseBox:		Ref	MAKEMAGICPTR(166)
_RSprotoCloseBox:	Ref	_RprotoCloseBox
		.globl	_RSnewtTextDateView
_RnewtTextDateView:		Ref	MAKEMAGICPTR(419)
_RSnewtTextDateView:	Ref	_RnewtTextDateView
		.globl	_RSrestorePrefsSlip
_RrestorePrefsSlip:		Ref	MAKEMAGICPTR(577)
_RSrestorePrefsSlip:	Ref	_RrestorePrefsSlip
		.globl	_RScancelBitmap
_RcancelBitmap:		Ref	MAKEMAGICPTR(311)
_RScancelBitmap:	Ref	_RcancelBitmap
		.globl	_RSprefsSlipProto
_RprefsSlipProto:		Ref	MAKEMAGICPTR(575)
_RSprefsSlipProto:	Ref	_RprefsSlipProto
		.globl	_RSprotoInputLine
_RprotoInputLine:		Ref	MAKEMAGICPTR(185)
_RSprotoInputLine:	Ref	_RprotoInputLine
		.globl	_RSprotoPolygon
_RprotoPolygon:		Ref	MAKEMAGICPTR(199)
_RSprotoPolygon:	Ref	_RprotoPolygon
		.globl	_RSvStatusTitle
_RvStatusTitle:		Ref	MAKEMAGICPTR(581)
_RSvStatusTitle:	Ref	_RvStatusTitle
		.globl	_RSnewtROTextTimeView
_RnewtROTextTimeView:		Ref	MAKEMAGICPTR(491)
_RSnewtROTextTimeView:	Ref	_RnewtROTextTimeView
		.globl	_RSprotoFullRouteSlip
_RprotoFullRouteSlip:		Ref	MAKEMAGICPTR(655)
_RSprotoFullRouteSlip:	Ref	_RprotoFullRouteSlip
		.globl	_RScanonicalGroup
_RcanonicalGroup:		Ref	MAKEMAGICPTR(33)
_RScanonicalGroup:	Ref	_RcanonicalGroup
		.globl	_RSprotoTextList
_RprotoTextList:		Ref	MAKEMAGICPTR(228)
_RSprotoTextList:	Ref	_RprotoTextList
		.globl	_RSrecToggle
_RrecToggle:		Ref	MAKEMAGICPTR(234)
_RSrecToggle:	Ref	_RrecToggle
		.globl	_RSprotoDigitalClock
_RprotoDigitalClock:		Ref	MAKEMAGICPTR(463)
_RSprotoDigitalClock:	Ref	_RprotoDigitalClock
		.globl	_RSfontSystem18
_RfontSystem18:		Ref	MAKEMAGICPTR(96)
_RSfontSystem18:	Ref	_RfontSystem18
		.globl	_RSgoAwayBitmap
_RgoAwayBitmap:		Ref	MAKEMAGICPTR(334)
_RSgoAwayBitmap:	Ref	_RgoAwayBitmap
		.globl	_RSnewtCheckbox
_RnewtCheckbox:		Ref	MAKEMAGICPTR(618)
_RSnewtCheckbox:	Ref	_RnewtCheckbox
		.globl	_RSscheduleSlip
_RscheduleSlip:		Ref	MAKEMAGICPTR(253)
_RSscheduleSlip:	Ref	_RscheduleSlip
		.globl	_RSrouteDuplicateBitmap
_RrouteDuplicateBitmap:		Ref	MAKEMAGICPTR(292)
_RSrouteDuplicateBitmap:	Ref	_RrouteDuplicateBitmap
		.globl	_RSbackupPrefsSlip
_RbackupPrefsSlip:		Ref	MAKEMAGICPTR(576)
_RSbackupPrefsSlip:	Ref	_RbackupPrefsSlip
		.globl	_RSDTMFPoundBitmap
_RDTMFPoundBitmap:		Ref	MAKEMAGICPTR(704)
_RSDTMFPoundBitmap:	Ref	_RDTMFPoundBitmap
		.globl	_RSkbdReturnBitmap
_RkbdReturnBitmap:		Ref	MAKEMAGICPTR(747)
_RSkbdReturnBitmap:	Ref	_RkbdReturnBitmap
		.globl	_RSkeyBulletBitmap
_RkeyBulletBitmap:		Ref	MAKEMAGICPTR(734)
_RSkeyBulletBitmap:	Ref	_RkeyBulletBitmap
		.globl	_RSnewtonModemName
_RnewtonModemName:		Ref	MAKEMAGICPTR(216)
_RSnewtonModemName:	Ref	_RnewtonModemName
		.globl	_RSnewtLabelPhoneInputLine
_RnewtLabelPhoneInputLine:		Ref	MAKEMAGICPTR(425)
_RSnewtLabelPhoneInputLine:	Ref	_RnewtLabelPhoneInputLine
		.globl	_RSdownBitmap
_RdownBitmap:		Ref	MAKEMAGICPTR(324)
_RSdownBitmap:	Ref	_RdownBitmap
		.globl	_RSprotoBasicEndpoint
_RprotoBasicEndpoint:		Ref	MAKEMAGICPTR(383)
_RSprotoBasicEndpoint:	Ref	_RprotoBasicEndpoint
		.globl	_RSDTMFstar
_RDTMFstar:		Ref	MAKEMAGICPTR(703)
_RSDTMFstar:	Ref	_RDTMFstar
		.globl	_RSpaperrollSoupDef
_RpaperrollSoupDef:		Ref	MAKEMAGICPTR(364)
_RSpaperrollSoupDef:	Ref	_RpaperrollSoupDef
		.globl	_RSlocationCursor
_RlocationCursor:		Ref	MAKEMAGICPTR(258)
_RSlocationCursor:	Ref	_RlocationCursor
		.globl	_RSprotoKeyboardView
_RprotoKeyboardView:		Ref	MAKEMAGICPTR(840)
_RSprotoKeyboardView:	Ref	_RprotoKeyboardView
		.globl	_RSprotoRepeatView
_RprotoRepeatView:		Ref	MAKEMAGICPTR(279)
_RSprotoRepeatView:	Ref	_RprotoRepeatView
		.globl	_RSconfirmButtonLists
_RconfirmButtonLists:		Ref	MAKEMAGICPTR(543)
_RSconfirmButtonLists:	Ref	_RconfirmButtonLists
		.globl	_RSchargingBitmap
_RchargingBitmap:		Ref	MAKEMAGICPTR(599)
_RSchargingBitmap:	Ref	_RchargingBitmap
		.globl	_RSdiamanteBitmap
_RdiamanteBitmap:		Ref	MAKEMAGICPTR(594)
_RSdiamanteBitmap:	Ref	_RdiamanteBitmap
		.globl	_RSnotesOverview
_RnotesOverview:		Ref	MAKEMAGICPTR(362)
_RSnotesOverview:	Ref	_RnotesOverview
		.globl	_RSglobalVarWannabes
_RglobalVarWannabes:		Ref	MAKEMAGICPTR(546)
_RSglobalVarWannabes:	Ref	_RglobalVarWannabes
		.globl	_RSprotoSenderPopup
_RprotoSenderPopup:		Ref	MAKEMAGICPTR(20)
_RSprotoSenderPopup:	Ref	_RprotoSenderPopup
		.globl	_RSstylusDownBitmap
_RstylusDownBitmap:		Ref	MAKEMAGICPTR(777)
_RSstylusDownBitmap:	Ref	_RstylusDownBitmap
		.globl	_RScanonicalInkWordInfo
_RcanonicalInkWordInfo:		Ref	MAKEMAGICPTR(476)
_RScanonicalInkWordInfo:	Ref	_RcanonicalInkWordInfo
		.globl	_RSrecSketchBitmap
_RrecSketchBitmap:		Ref	MAKEMAGICPTR(723)
_RSrecSketchBitmap:	Ref	_RrecSketchBitmap
		.globl	_RSzero2nineBitmap
_Rzero2nineBitmap:		Ref	MAKEMAGICPTR(350)
_RSzero2nineBitmap:	Ref	_Rzero2nineBitmap
		.globl	_RSnewtRoutingButton
_RnewtRoutingButton:		Ref	MAKEMAGICPTR(439)
_RSnewtRoutingButton:	Ref	_RnewtRoutingButton
		.globl	_RSprotoPeopleDataDef
_RprotoPeopleDataDef:		Ref	MAKEMAGICPTR(231)
_RSprotoPeopleDataDef:	Ref	_RprotoPeopleDataDef
		.globl	_RSradioOnBitmap
_RradioOnBitmap:		Ref	MAKEMAGICPTR(257)
_RSradioOnBitmap:	Ref	_RradioOnBitmap
		.globl	_RSfontSystem18Underline
_RfontSystem18Underline:		Ref	MAKEMAGICPTR(98)
_RSfontSystem18Underline:	Ref	_RfontSystem18Underline
		.globl	_RSprotoLabelPicker
_RprotoLabelPicker:		Ref	MAKEMAGICPTR(190)
_RSprotoLabelPicker:	Ref	_RprotoLabelPicker
		.globl	_RSnewtNrLabelDateInputLine
_RnewtNrLabelDateInputLine:		Ref	MAKEMAGICPTR(636)
_RSnewtNrLabelDateInputLine:	Ref	_RnewtNrLabelDateInputLine
		.globl	_RSbootLogoBitmap
_RbootLogoBitmap:		Ref	MAKEMAGICPTR(320)
_RSbootLogoBitmap:	Ref	_RbootLogoBitmap
		.globl	_RSDTMFstarBitmap
_RDTMFstarBitmap:		Ref	MAKEMAGICPTR(703)
_RSDTMFstarBitmap:	Ref	_RDTMFstarBitmap
		.globl	_RSnewtDateFilter
_RnewtDateFilter:		Ref	MAKEMAGICPTR(642)
_RSnewtDateFilter:	Ref	_RnewtDateFilter
		.globl	_RSdrawPokerTemplate
_RdrawPokerTemplate:		Ref	MAKEMAGICPTR(830)
_RSdrawPokerTemplate:	Ref	_RdrawPokerTemplate
		.globl	_RSloadLetterWeights
_RloadLetterWeights:		Ref	MAKEMAGICPTR(119)
_RSloadLetterWeights:	Ref	_RloadLetterWeights
		.globl	_RSromPhrasalLexicon
_RromPhrasalLexicon:		Ref	MAKEMAGICPTR(248)
_RSromPhrasalLexicon:	Ref	_RromPhrasalLexicon
		.globl	_RSnewtShowStationeryButton
_RnewtShowStationeryButton:		Ref	MAKEMAGICPTR(814)
_RSnewtShowStationeryButton:	Ref	_RnewtShowStationeryButton
		.globl	_RSdayView
_RdayView:		Ref	MAKEMAGICPTR(67)
_RSdayView:	Ref	_RdayView
		.globl	_RScopyrightNotice
_RcopyrightNotice:		Ref	MAKEMAGICPTR(184)
_RScopyrightNotice:	Ref	_RcopyrightNotice
		.globl	_RSnotifyIcon
_RnotifyIcon:		Ref	MAKEMAGICPTR(136)
_RSnotifyIcon:	Ref	_RnotifyIcon
		.globl	_RSwordFilter
_RwordFilter:		Ref	MAKEMAGICPTR(781)
_RSwordFilter:	Ref	_RwordFilter
		.globl	_RStoSubjectSlip
_RtoSubjectSlip:		Ref	MAKEMAGICPTR(45)
_RStoSubjectSlip:	Ref	_RtoSubjectSlip
		.globl	_RSmapPopup
_RmapPopup:		Ref	MAKEMAGICPTR(360)
_RSmapPopup:	Ref	_RmapPopup
		.globl	_RSstdForms
_RstdForms:		Ref	MAKEMAGICPTR(269)
_RSstdForms:	Ref	_RstdForms
		.globl	_RSrubricPopup
_RrubricPopup:		Ref	MAKEMAGICPTR(435)
_RSrubricPopup:	Ref	_RrubricPopup
		.globl	_RSvProgress
_RvProgress:		Ref	MAKEMAGICPTR(582)
_RSvProgress:	Ref	_RvProgress
		.globl	_RSprotoLetterFormat
_RprotoLetterFormat:		Ref	MAKEMAGICPTR(305)
_RSprotoLetterFormat:	Ref	_RprotoLetterFormat
		.globl	_RSnewtShowMenu
_RnewtShowMenu:		Ref	MAKEMAGICPTR(400)
_RSnewtShowMenu:	Ref	_RnewtShowMenu
		.globl	_RSdataName
_RdataName:		Ref	MAKEMAGICPTR(63)
_RSdataName:	Ref	_RdataName
		.globl	_RSloadGlobals
_RloadGlobals:		Ref	MAKEMAGICPTR(118)
_RSloadGlobals:	Ref	_RloadGlobals
		.globl	_RScaretspacebits
_Rcaretspacebits:		Ref	MAKEMAGICPTR(803)
_RScaretspacebits:	Ref	_Rcaretspacebits
		.globl	_RSdigitFlap
_RdigitFlap:		Ref	MAKEMAGICPTR(686)
_RSdigitFlap:	Ref	_RdigitFlap
		.globl	_RSfontSystem10
_RfontSystem10:		Ref	MAKEMAGICPTR(87)
_RSfontSystem10:	Ref	_RfontSystem10
		.globl	_RSvBarber
_RvBarber:		Ref	MAKEMAGICPTR(585)
_RSvBarber:	Ref	_RvBarber
		.globl	_RSgenevaFont
_RgenevaFont:		Ref	MAKEMAGICPTR(104)
_RSgenevaFont:	Ref	_RgenevaFont
		.globl	_RScorrectorMidBits
_RcorrectorMidBits:		Ref	MAKEMAGICPTR(798)
_RScorrectorMidBits:	Ref	_RcorrectorMidBits
		.globl	_RSnotesBitmap
_RnotesBitmap:		Ref	MAKEMAGICPTR(765)
_RSnotesBitmap:	Ref	_RnotesBitmap
		.globl	_RSworldPicker
_RworldPicker:		Ref	MAKEMAGICPTR(455)
_RSworldPicker:	Ref	_RworldPicker
		.globl	_RSdatenTimePopup
_RdatenTimePopup:		Ref	MAKEMAGICPTR(288)
_RSdatenTimePopup:	Ref	_RdatenTimePopup
		.globl	_RSnewtSmartAddressFilter
_RnewtSmartAddressFilter:		Ref	MAKEMAGICPTR(660)
_RSnewtSmartAddressFilter:	Ref	_RnewtSmartAddressFilter
		.globl	_RSprotoBatteryGauge
_RprotoBatteryGauge:		Ref	MAKEMAGICPTR(315)
_RSprotoBatteryGauge:	Ref	_RprotoBatteryGauge
		.globl	_RSprotoHorizontal2DScroller
_RprotoHorizontal2DScroller:		Ref	MAKEMAGICPTR(608)
_RSprotoHorizontal2DScroller:	Ref	_RprotoHorizontal2DScroller
		.globl	_RSnewtEditView
_RnewtEditView:		Ref	MAKEMAGICPTR(413)
_RSnewtEditView:	Ref	_RnewtEditView
		.globl	_RSprotoRepeatDateDurationTextPicker
_RprotoRepeatDateDurationTextPicker:		Ref	MAKEMAGICPTR(19)
_RSprotoRepeatDateDurationTextPicker:	Ref	_RprotoRepeatDateDurationTextPicker
		.globl	_RSprotoSoundFrame
_RprotoSoundFrame:		Ref	MAKEMAGICPTR(849)
_RSprotoSoundFrame:	Ref	_RprotoSoundFrame
		.globl	_RSnewtPopupEdit
_RnewtPopupEdit:		Ref	MAKEMAGICPTR(510)
_RSnewtPopupEdit:	Ref	_RnewtPopupEdit
		.globl	_RSprotoPreferencesPopup
_RprotoPreferencesPopup:		Ref	MAKEMAGICPTR(611)
_RSprotoPreferencesPopup:	Ref	_RprotoPreferencesPopup
		.globl	_RSviewRoot
_RviewRoot:		Ref	MAKEMAGICPTR(287)
_RSviewRoot:	Ref	_RviewRoot
		.globl	_RScanonicalScrollee
_RcanonicalScrollee:		Ref	MAKEMAGICPTR(37)
_RScanonicalScrollee:	Ref	_RcanonicalScrollee
		.globl	_RSprotoampmCluster
_RprotoampmCluster:		Ref	MAKEMAGICPTR(379)
_RSprotoampmCluster:	Ref	_RprotoampmCluster
		.globl	_RSprotoStatusGauge
_RprotoStatusGauge:		Ref	MAKEMAGICPTR(471)
_RSprotoStatusGauge:	Ref	_RprotoStatusGauge
		.globl	_RSpaperCallBitmap
_RpaperCallBitmap:		Ref	MAKEMAGICPTR(718)
_RSpaperCallBitmap:	Ref	_RpaperCallBitmap
		.globl	_RSroutePasteText
_RroutePasteText:		Ref	MAKEMAGICPTR(773)
_RSroutePasteText:	Ref	_RroutePasteText
		.globl	_RScardAction
_RcardAction:		Ref	MAKEMAGICPTR(363)
_RScardAction:	Ref	_RcardAction
		.globl	_RSDTMF4
_RDTMF4:		Ref	MAKEMAGICPTR(697)
_RSDTMF4:	Ref	_RDTMF4
		.globl	_RSfontSystem10Underline
_RfontSystem10Underline:		Ref	MAKEMAGICPTR(89)
_RSfontSystem10Underline:	Ref	_RfontSystem10Underline
		.globl	_RSnamesBitmap
_RnamesBitmap:		Ref	MAKEMAGICPTR(479)
_RSnamesBitmap:	Ref	_RnamesBitmap
		.globl	_RSprotoPinWindowWithoutLogic
_RprotoPinWindowWithoutLogic:		Ref	MAKEMAGICPTR(558)
_RSprotoPinWindowWithoutLogic:	Ref	_RprotoPinWindowWithoutLogic
		.globl	_RSnewtSimpleDateFilter
_RnewtSimpleDateFilter:		Ref	MAKEMAGICPTR(683)
_RSnewtSimpleDateFilter:	Ref	_RnewtSimpleDateFilter
		.globl	_RSpagePreviewForm
_RpagePreviewForm:		Ref	MAKEMAGICPTR(141)
_RSpagePreviewForm:	Ref	_RpagePreviewForm
		.globl	_RSnetChooser
_RnetChooser:		Ref	MAKEMAGICPTR(130)
_RSnetChooser:	Ref	_RnetChooser
		.globl	_RSstdClosing
_RstdClosing:		Ref	MAKEMAGICPTR(236)
_RSstdClosing:	Ref	_RstdClosing
		.globl	_RSprotoThumbnailFloater
_RprotoThumbnailFloater:		Ref	MAKEMAGICPTR(481)
_RSprotoThumbnailFloater:	Ref	_RprotoThumbnailFloater
		.globl	_RScanonicalCharGrid
_RcanonicalCharGrid:		Ref	MAKEMAGICPTR(606)
_RScanonicalCharGrid:	Ref	_RcanonicalCharGrid
		.globl	_RShollowStarBitmap
_RhollowStarBitmap:		Ref	MAKEMAGICPTR(15)
_RShollowStarBitmap:	Ref	_RhollowStarBitmap
		.globl	_RSdstSoupName
_RdstSoupName:		Ref	MAKEMAGICPTR(658)
_RSdstSoupName:	Ref	_RdstSoupName
		.globl	_RSnewtInfoBox
_RnewtInfoBox:		Ref	MAKEMAGICPTR(672)
_RSnewtInfoBox:	Ref	_RnewtInfoBox
		.globl	_RSprotoDateTextPicker
_RprotoDateTextPicker:		Ref	MAKEMAGICPTR(629)
_RSprotoDateTextPicker:	Ref	_RprotoDateTextPicker
		.globl	_RStimePopup
_RtimePopup:		Ref	MAKEMAGICPTR(147)
_RStimePopup:	Ref	_RtimePopup
		.globl	_RSprotoRadioCluster
_RprotoRadioCluster:		Ref	MAKEMAGICPTR(203)
_RSprotoRadioCluster:	Ref	_RprotoRadioCluster
		.globl	_RSprotoRecorderView
_RprotoRecorderView:		Ref	MAKEMAGICPTR(853)
_RSprotoRecorderView:	Ref	_RprotoRecorderView
		.globl	_RSprotoRecognitionCluster
_RprotoRecognitionCluster:		Ref	MAKEMAGICPTR(506)
_RSprotoRecognitionCluster:	Ref	_RprotoRecognitionCluster
		.globl	_RSDTMF4Bitmap
_RDTMF4Bitmap:		Ref	MAKEMAGICPTR(697)
_RSDTMF4Bitmap:	Ref	_RDTMF4Bitmap
		.globl	_RSnewtLayout
_RnewtLayout:		Ref	MAKEMAGICPTR(402)
_RSnewtLayout:	Ref	_RnewtLayout
		.globl	_RStabLeftHiliteBitmap
_RtabLeftHiliteBitmap:		Ref	MAKEMAGICPTR(728)
_RStabLeftHiliteBitmap:	Ref	_RtabLeftHiliteBitmap
		.globl	_RScanonicalScroller
_RcanonicalScroller:		Ref	MAKEMAGICPTR(38)
_RScanonicalScroller:	Ref	_RcanonicalScroller
		.globl	_RScardAlertTemplate
_RcardAlertTemplate:		Ref	MAKEMAGICPTR(9)
_RScardAlertTemplate:	Ref	_RcardAlertTemplate
		.globl	_RSprotoStrokesItem
_RprotoStrokesItem:		Ref	MAKEMAGICPTR(222)
_RSprotoStrokesItem:	Ref	_RprotoStrokesItem
		.globl	_RSdockerBitmap
_RdockerBitmap:		Ref	MAKEMAGICPTR(338)
_RSdockerBitmap:	Ref	_RdockerBitmap
		.globl	_RSprotoStatusCloseBox
_RprotoStatusCloseBox:		Ref	MAKEMAGICPTR(468)
_RSprotoStatusCloseBox:	Ref	_RprotoStatusCloseBox
		.globl	_RSprotoUpDownScroller
_RprotoUpDownScroller:		Ref	MAKEMAGICPTR(656)
_RSprotoUpDownScroller:	Ref	_RprotoUpDownScroller
		.globl	_RSprotoMeetingPlacePicker
_RprotoMeetingPlacePicker:		Ref	MAKEMAGICPTR(665)
_RSprotoMeetingPlacePicker:	Ref	_RprotoMeetingPlacePicker
		.globl	_RSkeyLeftParenBitmap
_RkeyLeftParenBitmap:		Ref	MAKEMAGICPTR(739)
_RSkeyLeftParenBitmap:	Ref	_RkeyLeftParenBitmap
		.globl	_RSazTabsSlimBitmap
_RazTabsSlimBitmap:		Ref	MAKEMAGICPTR(560)
_RSazTabsSlimBitmap:	Ref	_RazTabsSlimBitmap
		.globl	_RSprotoGlance
_RprotoGlance:		Ref	MAKEMAGICPTR(183)
_RSprotoGlance:	Ref	_RprotoGlance
		.globl	_RSazTabs
_RazTabs:		Ref	MAKEMAGICPTR(617)
_RSazTabs:	Ref	_RazTabs
		.globl	_RSnotepaper
_Rnotepaper:		Ref	MAKEMAGICPTR(133)
_RSnotepaper:	Ref	_Rnotepaper
		.globl	_RSprotoCard
_RprotoCard:		Ref	MAKEMAGICPTR(829)
_RSprotoCard:	Ref	_RprotoCard
		.globl	_RScharsVersion
_RcharsVersion:		Ref	MAKEMAGICPTR(48)
_RScharsVersion:	Ref	_RcharsVersion
		.globl	_RSrouteFaxIcon
_RrouteFaxIcon:		Ref	MAKEMAGICPTR(515)
_RSrouteFaxIcon:	Ref	_RrouteFaxIcon
		.globl	_RSdownArrowBitmap
_RdownArrowBitmap:		Ref	MAKEMAGICPTR(328)
_RSdownArrowBitmap:	Ref	_RdownArrowBitmap
		.globl	_RSprotoTAPIPicker
_RprotoTAPIPicker:		Ref	MAKEMAGICPTR(572)
_RSprotoTAPIPicker:	Ref	_RprotoTAPIPicker
		.globl	_RSstarterInk
_RstarterInk:		Ref	MAKEMAGICPTR(266)
_RSstarterInk:	Ref	_RstarterInk
		.globl	_RScanonicalTable
_RcanonicalTable:		Ref	MAKEMAGICPTR(39)
_RScanonicalTable:	Ref	_RcanonicalTable
		.globl	_RSroutePrintBitmap
_RroutePrintBitmap:		Ref	MAKEMAGICPTR(730)
_RSroutePrintBitmap:	Ref	_RroutePrintBitmap
		.globl	_RSsalutationSuffix
_RsalutationSuffix:		Ref	MAKEMAGICPTR(249)
_RSsalutationSuffix:	Ref	_RsalutationSuffix
		.globl	_RSioioboxSoup
_RioioboxSoup:		Ref	MAKEMAGICPTR(824)
_RSioioboxSoup:	Ref	_RioioboxSoup
		.globl	_RSprotoEworldEndpoint
_RprotoEworldEndpoint:		Ref	MAKEMAGICPTR(388)
_RSprotoEworldEndpoint:	Ref	_RprotoEworldEndpoint
		.globl	_RSnewtTextFilter
_RnewtTextFilter:		Ref	MAKEMAGICPTR(639)
_RSnewtTextFilter:	Ref	_RnewtTextFilter
		.globl	_RSoutboxSoupName
_RoutboxSoupName:		Ref	MAKEMAGICPTR(139)
_RSoutboxSoupName:	Ref	_RoutboxSoupName
		.globl	_RSkbdLeftBitmap
_RkbdLeftBitmap:		Ref	MAKEMAGICPTR(749)
_RSkbdLeftBitmap:	Ref	_RkbdLeftBitmap
		.globl	_RScalendarSoupName
_RcalendarSoupName:		Ref	MAKEMAGICPTR(23)
_RScalendarSoupName:	Ref	_RcalendarSoupName
		.globl	_RScardfileSoupName
_RcardfileSoupName:		Ref	MAKEMAGICPTR(47)
_RScardfileSoupName:	Ref	_RcardfileSoupName
		.globl	_RSfontSystem14Bold
_RfontSystem14Bold:		Ref	MAKEMAGICPTR(94)
_RSfontSystem14Bold:	Ref	_RfontSystem14Bold
		.globl	_RSprotoWordInterp
_RprotoWordInterp:		Ref	MAKEMAGICPTR(616)
_RSprotoWordInterp:	Ref	_RprotoWordInterp
		.globl	_RSnewtStationeryView
_RnewtStationeryView:		Ref	MAKEMAGICPTR(411)
_RSnewtStationeryView:	Ref	_RnewtStationeryView
		.globl	_RScanonicalPoint
_RcanonicalPoint:		Ref	MAKEMAGICPTR(566)
_RScanonicalPoint:	Ref	_RcanonicalPoint
		.globl	_RStabRightBitmap
_RtabRightBitmap:		Ref	MAKEMAGICPTR(727)
_RStabRightBitmap:	Ref	_RtabRightBitmap
		.globl	_RSnewtLabelDateInputLine
_RnewtLabelDateInputLine:		Ref	MAKEMAGICPTR(424)
_RSnewtLabelDateInputLine:	Ref	_RnewtLabelDateInputLine
		.globl	_RSblacklist
_Rblacklist:		Ref	MAKEMAGICPTR(215)
_RSblacklist:	Ref	_Rblacklist
		.globl	_RScardfileQuerySpec
_RcardfileQuerySpec:		Ref	MAKEMAGICPTR(46)
_RScardfileQuerySpec:	Ref	_RcardfileQuerySpec
		.globl	_RScorrectorLeadBits
_RcorrectorLeadBits:		Ref	MAKEMAGICPTR(797)
_RScorrectorLeadBits:	Ref	_RcorrectorLeadBits
		.globl	_RStargetFrameFormat
_RtargetFrameFormat:		Ref	MAKEMAGICPTR(490)
_RStargetFrameFormat:	Ref	_RtargetFrameFormat
		.globl	_RSazTabsBitmap
_RazTabsBitmap:		Ref	MAKEMAGICPTR(396)
_RSazTabsBitmap:	Ref	_RazTabsBitmap
		.globl	_RSdsPlaceHints
_RdsPlaceHints:		Ref	MAKEMAGICPTR(545)
_RSdsPlaceHints:	Ref	_RdsPlaceHints
		.globl	_RSonlineBitmap
_RonlineBitmap:		Ref	MAKEMAGICPTR(346)
_RSonlineBitmap:	Ref	_RonlineBitmap
		.globl	_RSioProtoShowByButton
_RioProtoShowByButton:		Ref	MAKEMAGICPTR(820)
_RSioProtoShowByButton:	Ref	_RioProtoShowByButton
		.globl	_RSmeeting
_Rmeeting:		Ref	MAKEMAGICPTR(125)
_RSmeeting:	Ref	_Rmeeting
		.globl	_RSdigitFlapLoWord
_RdigitFlapLoWord:		Ref	MAKEMAGICPTR(685)
_RSdigitFlapLoWord:	Ref	_RdigitFlapLoWord
		.globl	_RSrepeatNotesName
_RrepeatNotesName:		Ref	MAKEMAGICPTR(242)
_RSrepeatNotesName:	Ref	_RrepeatNotesName
		.globl	_RSrouteDeleteIcon
_RrouteDeleteIcon:		Ref	MAKEMAGICPTR(291)
_RSrouteDeleteIcon:	Ref	_RrouteDeleteIcon
		.globl	_RSerrorTable
_RerrorTable:		Ref	MAKEMAGICPTR(79)
_RSerrorTable:	Ref	_RerrorTable
		.globl	_RSfinePenTip
_RfinePenTip:		Ref	MAKEMAGICPTR(707)
_RSfinePenTip:	Ref	_RfinePenTip
		.globl	_RSprotoStatusButton
_RprotoStatusButton:		Ref	MAKEMAGICPTR(470)
_RSprotoStatusButton:	Ref	_RprotoStatusButton
		.globl	_RSprotoCheckbox
_RprotoCheckbox:		Ref	MAKEMAGICPTR(164)
_RSprotoCheckbox:	Ref	_RprotoCheckbox
		.globl	_RSrouteMailIcon
_RrouteMailIcon:		Ref	MAKEMAGICPTR(732)
_RSrouteMailIcon:	Ref	_RrouteMailIcon
