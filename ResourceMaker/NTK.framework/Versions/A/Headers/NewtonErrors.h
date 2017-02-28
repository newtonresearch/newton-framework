/*
	File:		NewtonErrors.h
	
	Contains:	Error codes for Newton C++ Tools.

	Written by:	Newton Research Group
*/

#if !defined(__NEWTONERRORS_H)
#define __NEWTONERRORS_H 1

// ---------------  Error bases…  ---------------

#define ERRBASE_COMMON				   (-7000)	// Common errors		(see below)
#define ERRBASE_NEWT					   (-8000)	// Newt errors
#define ERRBASE_OS					  (-10000)	// OS errors
#define ERRBASE_INTERRUPT			  (-11000)	// Interrupt errors
#define ERRBASE_APPLETALK			  (-12000)	// AppleTalk errors
#define ERRBASE_UTILITYCLASSES	  (-14000)	// UtilityClass errors
#define ERRBASE_COMMTOOL			  (-16000)	// CommTool errors
#define ERRBASE_SERIAL				  (-18000)	// SerialTool errors
#define ERRBASE_MNP					  (-20000)	// MNP errors
#define ERRBASE_FAX					  (-22000)	// FAX errors
#define ERRBASE_TAPI					  (-23000)	// TAPI errors
#define ERRBASE_MODEM				  (-24000)	// Modem errors
#define ERRBASE_COMMMGR				  (-26000)	// CommMgr errors
#define ERRBASE_DOCKER				  (-28000)	// Docker errors
#define ERRBASE_SOUND				  (-30000)	// Sound errors
#define ERRBASE_COMPRESSION		  (-32000)	// Compression errors
#define ERRBASE_MEMORY				  (-34000)	// Memory errors
#define ERRBASE_TRANSPORT			  (-36000)	// Transport errors
#define ERRBASE_SHARPIR				  (-38000)	// Sharp IR errors
#define ERRBASE_IRDA					  (-38500)	// IrDA errors
#define ERRBASE_ONLINESERVICES	  (-40000)	// Online service errs
#define ERRBASE_EMAIL				  (-42000)	// Online service errs
#define ERRBASE_PRINTING			  (-44000)	// Printing errors
#define ERRBASE_BLACKBIRD			  (-46000)	// TSI BlackBird errors (Obsolete)
#define ERRBASE_MESSAGINGENABLER	  (-46000)	// Messaging Enabler errors
#define ERRBASE_FRAMES				  (-48000)	// Frames errors
#define ERRBASE_WIZARD				  (-50000)	// Wizard errors
#define ERRBASE_GRAPHICS			  (-52000)	// Graphics errors
#define ERRBASE_COMMSCRIPT			  (-54000)	// CommScripting errors
#define ERRBASE_TABLET				  (-56000)	// Tablet errors
#define ERRBASE_PAGEDTSTORE		  (-58000)	// Alien store device errors
#define ERRBASE_INET					  (-60000)	// InetTool/NIE errors

#define ERRBASE_PRODUCTSPECIFIC	(-1000000)	// Product specific errors (non-Apple errors)
															// -1000000 -> -1999999

// ---------------  Common errors…  -------------

#define noErr									  0
#define memFullErr						 (-108)								// Not enough room in heap zone [traditional]
#define userCanceledErr					 (-128)								// also [traditional]
#define kNoMemory				(ERRBASE_COMMON)

// ---------------  General utility class errors…  -------------

#define kUCErrNotImplemented			(ERRBASE_UTILITYCLASSES -  1)
#define kUCErrNoMemory 					(ERRBASE_UTILITYCLASSES -  2)
#define kUCErrBadPositionMode 		(ERRBASE_UTILITYCLASSES -  3)
#define kUCErrAlreadyInited 			(ERRBASE_UTILITYCLASSES -  4)
#define kUCErrBadSize 					(ERRBASE_UTILITYCLASSES -  5)
#define kUCErrOverflow 					(ERRBASE_UTILITYCLASSES -  6)
#define kUCErrUnderflow 				(ERRBASE_UTILITYCLASSES -  7)
#define kUCErrRangeCheck 				(ERRBASE_UTILITYCLASSES -  8)
#define kUCErrElementSizeMismatch 	(ERRBASE_UTILITYCLASSES -  9)
#define kUCErrNotInitialized 			(ERRBASE_UTILITYCLASSES - 10)
#define kUCErrNilPtr						(ERRBASE_UTILITYCLASSES - 11)

// ---------------  AppWorld errors…  -------------

#define kAEErrNoHandler					(ERRBASE_UTILITYCLASSES - 100)

// ---------------  Memory errors…  ---------------
//	Things that can go wrong with heaps
//	(certainly NOT exhaustive, but all of the ones returned by CheckHeap())

#define kMemErrBogusBlockType 				(ERRBASE_MEMORY	  )	// e.g. not free, direct or indirect
#define kMemErrUnalignedPointer				(ERRBASE_MEMORY -  1)	// pointer not aligned to 4-byte boundary
#define kMemErrPointerOutOfHeap				(ERRBASE_MEMORY -  2)	// pointer to outside of heap
#define kMemErrBogusInternalBlockType		(ERRBASE_MEMORY -  3)	// Unknown infrastructure type
#define kMemErrMisplacedFreeBlock			(ERRBASE_MEMORY -  4)	// free block where there shouldn't be one
#define kMemErrBadFreelistPointer			(ERRBASE_MEMORY -  5)	// free-list pointer points outside of heap
#define kMemErrFreelistPointerPointsAtJunk (ERRBASE_MEMORY -  6)	// free-list pointer doesn't point at a free block
#define kMemErrBadForwardMarch				(ERRBASE_MEMORY -  7)	// Invalid block size
#define kMemErrBogusBlockSize					(ERRBASE_MEMORY -  8)	// forbidden bits set in block-size
#define kMemErrBlockSizeLessThanMinimum	(ERRBASE_MEMORY -  9)	// heap blocks have a certain minimum size...
#define kMemErrPreposterousBlockSize		(ERRBASE_MEMORY - 10)	// heap block too large (>2GB) probably don't 
#define kMemErrBogusFreeCount					(ERRBASE_MEMORY - 11)	// total free is bigger than entire heap
#define kMemErrBadNilPointer					(ERRBASE_MEMORY - 12)	// Nil pointer where not allowed 
#define kMemErrFreeSpaceDisagreement1		(ERRBASE_MEMORY - 13)	// tracked -vs- actual free-space is different
#define kMemErrFreeSpaceDisagreement2		(ERRBASE_MEMORY - 14)	// tracked -vs- linked free-space is different
#define kMemErrBadMasterPointer				(ERRBASE_MEMORY - 15)	// master pointer doesn't point back to handle block
#define kMemErrBadBlockDeltaSize				(ERRBASE_MEMORY - 16)	// bad block-size adjustment
#define kMemErrBadInternalBlockType			(ERRBASE_MEMORY - 17)	// possibly mangled internal block
#define kMemErrHeapCorruptErr					(ERRBASE_MEMORY - 18)	// The heap is invalid [apparently whacked?]
#define kMemErrExceptionGrokkingHeap		(ERRBASE_MEMORY - 19)	// caught an exception checking the heap [this is bad]
#define kMemErrBadHeapHeader					(ERRBASE_MEMORY - 20)	// Invalid heap header

// ---------------  Store and soup errors…  ---------------

#define kNSErrNotAFrameStore					(ERRBASE_FRAMES -  1)	// The PCMCIA card is not a data storage card
#define kNSErrOldStoreFormat					(ERRBASE_FRAMES -  2)	// Store format is too old to understand
#define kNSErrNewStoreFormat					(ERRBASE_FRAMES -  3)	// Store format is too new to understand
#define kNSErrStoreCorrupted					(ERRBASE_FRAMES -  4)	// Store is corrupted, can't recover
#define kNSErrObjectCorrupted					(ERRBASE_FRAMES -  5)	// Single object is corrupted, can't recover
#define kNSErrUnknownStreamFormat			(ERRBASE_FRAMES -  6)	// Object stream has unknown format version
#define kNSErrInvalidFaultBlock				(ERRBASE_FRAMES -  7)	// Fault block is invalid (probably from a removed store)
#define kNSErrNotAFaultBlock					(ERRBASE_FRAMES -  8)	// Not a fault block (internal error)
#define kNSErrNotASoupEntry					(ERRBASE_FRAMES -  9)	// Not a soup entry
#define kNSErrStoreNotRegistered				(ERRBASE_FRAMES - 10)	// Tried to remove a store that wasn't registered
#define kNSErrUnknownIndexType				(ERRBASE_FRAMES - 11)	// Soup index has an unknown type
#define kNSErrUnknownKeyStructure			(ERRBASE_FRAMES - 12)	// Soup index has an unknown key structure
#define kNSErrNoSuchIndex						(ERRBASE_FRAMES - 13)	// Soup index does not exist
#define kNSErrDuplicateSoupName				(ERRBASE_FRAMES - 14)	// A soup with this name already exists
#define kNSErrCantCopyToUnionSoup			(ERRBASE_FRAMES - 15)	// Tried to CopyEntries to a union soup
#define kNSErrInvalidSoup						(ERRBASE_FRAMES - 16)	// Soup is invalid (probably from a removed store)
#define kNSErrInvalidStore						(ERRBASE_FRAMES - 17)	// entry is invalid (probably from a removed store)
#define kNSErrInvalidEntry						(ERRBASE_FRAMES - 18)	// Entry is invalid (probably from a removed store)
#define kNSErrKeyHasWrongType					(ERRBASE_FRAMES - 19)	// Key does not have the type specified in the index
#define kNSErrStoreIsROM						(ERRBASE_FRAMES - 20)	// Store is in ROM
#define kNSErrDuplicateIndex					(ERRBASE_FRAMES - 21)	// Soup already has an index with this path
#define kNSErrInternalError					(ERRBASE_FRAMES - 22)	// Internal error--something unexpected happened
#define kNSErrCantRemoveUIdIndex				(ERRBASE_FRAMES - 23)	// Tried to RemoveIndex the _uniqueId index
#define kNSErrInvalidQueryType				(ERRBASE_FRAMES - 24)	// Query type missing or unknown
#define kNSErrIndexCorrupted					(ERRBASE_FRAMES - 25)	// Discovered index inconsistency
#define kNSErrInvalidTagsCount				(ERRBASE_FRAMES - 26)	// max tags count has been reached
#define kNSErrNoTags								(ERRBASE_FRAMES - 27)	// soup does not have tags (PlainSoupModifyTag, PlainSoupRemoveTags)
#define kNSErrInvalidTagSpec					(ERRBASE_FRAMES - 28)	// tagSpec frame is invalid
#define kNSErrWrongStoreVersion				(ERRBASE_FRAMES - 29)	// Store cannot handle the feature (e.g. large objects)
#define kNSErrInvalidSorting					(ERRBASE_FRAMES - 30)	// indexDesc requests an unknown sorting table
#define kNSErrInvalidUnion						(ERRBASE_FRAMES - 31)	// can not UnionSoup (different sorting tables)
#define kNSErrBadIndexDesc						(ERRBASE_FRAMES - 32)	// Bad index description
#define kNSErrVBOKey								(ERRBASE_FRAMES - 33)	// Soup entries keys can not be virtual binaries
#define kNSErrInvalidSoupName					(ERRBASE_FRAMES - 34)	// Soup name is too long

// ---------------  Object system errors…  ---------------

#define kNSErrObjectPointerOfNonPtr			(ERRBASE_FRAMES - 200)	// ObjectPtr of non-pointer
#define kNSErrBadMagicPointer					(ERRBASE_FRAMES - 201)	// Bad magic pointer
#define kNSErrEmptyPath							(ERRBASE_FRAMES - 202)	// Empty path
#define kNSErrBadSegmentInPath				(ERRBASE_FRAMES - 203)	// Invalid segment in path expression
#define kNSErrPathFailed						(ERRBASE_FRAMES - 204)	// Path failed
#define kNSErrOutOfBounds						(ERRBASE_FRAMES - 205)	// Index out of bounds (string or array)
#define kNSErrObjectsNotDistinct				(ERRBASE_FRAMES - 206)	// Source and dest must be different objects
#define kNSErrLongOutOfRange					(ERRBASE_FRAMES - 207)	// Long out of range
#define kNSErrSettingHeapSizeTwice			(ERRBASE_FRAMES - 208)	// Call SetObjectHeapSize only once, before InitObjects
#define kNSErrGcDuringGc						(ERRBASE_FRAMES - 209)	// GC during GC...this is bad!
#define kNSErrBadArgs							(ERRBASE_FRAMES - 210)	// Bad args
#define kNSErrStringTooBig						(ERRBASE_FRAMES - 211)	// String too big
#define kNSErrTFramesObjectPtrOfNil			(ERRBASE_FRAMES - 212)	// TFramesObjectPtr of NIL
#define kNSErrUnassignedTFramesObjectPtr	(ERRBASE_FRAMES - 213)	// unassigned TFramesObjectPtr
#define kNSErrObjectReadOnly					(ERRBASE_FRAMES - 214)	// Object is read-only
#define kNSErrReturn								(ERRBASE_FRAMES - 215)	// 
#define kNSErrOutOfObjectMemory				(ERRBASE_FRAMES - 216)	// Ran out of heap memory
#define kNSErrDerefMagicPointer				(ERRBASE_FRAMES - 217)	// Magic pointers cannot be dereferenced in Fram
#define kNSErrNegativeLength					(ERRBASE_FRAMES - 218)	// Negative length
#define kNSErrOutOfRange						(ERRBASE_FRAMES - 219)	// Value out of range
#define kNSErrCouldntResizeLockedObject	(ERRBASE_FRAMES - 220)	// Couldn't resize locked object
#define kNSErrBadPackageRef					(ERRBASE_FRAMES - 221)	// Reference to deactivated package
#define kNSErrBadExceptionName				(ERRBASE_FRAMES - 222)	// Exception not a subexception of |evt.ex|
#define kNSErrBadStream							(ERRBASE_FRAMES - 223)	// Invalid item encountered in stream
#define kNSErrFuncInStream						(ERRBASE_FRAMES - 224)	// Function object encountered in stream

// ---------------  Bad type errors…  ---------------

#define kNSErrNotAFrame							(ERRBASE_FRAMES - 400)	// Expected a frame
#define kNSErrNotAnArray						(ERRBASE_FRAMES - 401)	// Expected an array
#define kNSErrNotAString						(ERRBASE_FRAMES - 402)	// Expected a string
#define kNSErrNotAPointer						(ERRBASE_FRAMES - 403)	// Expected a pointer, array, or binary object
#define kNSErrNotANumber						(ERRBASE_FRAMES - 404)	// Expected a number
#define kNSErrNotAReal							(ERRBASE_FRAMES - 405)	// Expected a real
#define kNSErrNotAnInteger						(ERRBASE_FRAMES - 406)	// Expected an integer
#define kNSErrNotACharacter					(ERRBASE_FRAMES - 407)	// Expected a character
#define kNSErrNotABinaryObject				(ERRBASE_FRAMES - 408)	// Expected a binary object
#define kNSErrNotAPathExpr						(ERRBASE_FRAMES - 409)	// Expected a path expression (or a symbol or integer)
#define kNSErrNotASymbol						(ERRBASE_FRAMES - 410)	// Expected a symbol
#define kNSErrNotAFunction						(ERRBASE_FRAMES - 411)	// Expected a function
#define kNSErrNotAFrameOrArray				(ERRBASE_FRAMES - 412)	// Expected a frame or an array
#define kNSErrNotAnArrayOrNil					(ERRBASE_FRAMES - 413)	// Expected an array or NIL
#define kNSErrNotAStringOrNil					(ERRBASE_FRAMES - 414)	// Expected a string or NIL
#define kNSErrNotABinaryObjectOrNil			(ERRBASE_FRAMES - 415)	// Expected a binary object or NIL
#define kNSErrUnexpectedFrame					(ERRBASE_FRAMES - 416)	// Unexpected frame
#define kNSErrUnexpectedBinaryObject		(ERRBASE_FRAMES - 417)	// Unexpected binary object
#define kNSErrUnexpectedImmediate			(ERRBASE_FRAMES - 418)	// Unexpected immediate
#define kNSErrNotAnArrayOrString				(ERRBASE_FRAMES - 419)	// Expected an array or string
#define kNSErrNotAVBO							(ERRBASE_FRAMES - 420)	// Expected a vbo
#define kNSErrNotAPackage						(ERRBASE_FRAMES - 421)	// Expected a package
#define kNSErrNotNil								(ERRBASE_FRAMES - 422)	// Expected a NIL
#define kNSErrNotASymbolOrNil					(ERRBASE_FRAMES - 423)  // Expected NIL or a symbol
#define kNSErrNotTrueOrNil						(ERRBASE_FRAMES - 424)  // Expected NIL or True
#define kNSErrNotAnIntegerOrArray			(ERRBASE_FRAMES - 425)  // Expected an integer or an array
#define kNSErrNotAPlainString					(ERRBASE_FRAMES - 426)  // Expected a non-rich string

// ---------------  Compiler errors…  ---------------

#define kNSErrNoREP								(ERRBASE_FRAMES - 600)  // could not open a listener window
#define kNSErrSyntaxError						(ERRBASE_FRAMES - 601)  // syntax error
//#define kNSErr										(ERRBASE_FRAMES - 602)  // 
#define kNSErrBadAssign							(ERRBASE_FRAMES - 603)  // cannot assign to a constant
#define kNSErrBadSubscriptTest				(ERRBASE_FRAMES - 604)  // cannot test for subscript existence; use length
//#define kNSErr										(ERRBASE_FRAMES - 605)  // global variables not allowed in applications
#define kNSErrGlobalAndConstCollision		(ERRBASE_FRAMES - 606)  // cannot have a global variable and a global constant with the same name
#define kNSErrConstRedefined					(ERRBASE_FRAMES - 607)  // cannot redefine a constant
#define kNSErrVarAndConstCollision			(ERRBASE_FRAMES - 608)  // variable and constant have same name
#define kNSErrNonLiteralConst					(ERRBASE_FRAMES - 609)  // non-literal expression for constant initializer
#define kNSErrEOFInAString						(ERRBASE_FRAMES - 610)  // end if input inside a string
#define kNSErrBadEscape							(ERRBASE_FRAMES - 611)  // no escapes but \\u are allowed after \\u
#define kNSErrIllegalEscape					(ERRBASE_FRAMES - 612)  // no escapes but \\u are allowed after \\u
#define kNSErrBadHex								(ERRBASE_FRAMES - 613)  // only hex digits allowed within \\u
#define kNSErrLineRequired						(ERRBASE_FRAMES - 614)  // line required after #
#define kNSErrNumberRequired					(ERRBASE_FRAMES - 615)  // number required after #line
#define kNSErrSpaceRequired					(ERRBASE_FRAMES - 616)  // space required after number
#define kNSErr2HexDigitsRequired				(ERRBASE_FRAMES - 617)  // 2 hex digits required
#define kNSErr4HexDigitsRequired				(ERRBASE_FRAMES - 618)  // 4 hex digits required
#define kNSErrUnrecognized						(ERRBASE_FRAMES - 619)  // unrecognized token in input stream
#define kNSErrInvalidHex						(ERRBASE_FRAMES - 620)  // invalid hexadecimal imteger
#define kNSErrInvalidDecimal					(ERRBASE_FRAMES - 622)  // invalid decimal imteger
#define kNSErrBadPath							(ERRBASE_FRAMES - 623)  // expected '.' in path expression
#define kNSErrNumberTooLong					(ERRBASE_FRAMES - 625)  // buffer length exceeded
#define kNSErrHashForbidden					(ERRBASE_FRAMES - 626)  // #xxxx not allowed from NTK
#define kNSErrDigitRequired					(ERRBASE_FRAMES - 628)  // Decimal digit required after @

// ---------------  Interpreter errors…  ---------------

#define kNSErrNotInBreakLoop					(ERRBASE_FRAMES - 800)	// Not in a break loop
#define kNSErrTooManyArgs						(ERRBASE_FRAMES - 802)	// Too many args for a CFunction
#define kNSErrWrongNumberOfArgs				(ERRBASE_FRAMES - 803)	// Wrong number of args
#define kNSErrZeroForLoopIncr					(ERRBASE_FRAMES - 804)	// For loop by expression has value zero
#define kNSErrUndefinedBytecode				(ERRBASE_FRAMES - 805)	// Bytecode is unknown to interpreter
#define kNSErrNoCurrentException				(ERRBASE_FRAMES - 806)	// No current exception
#define kNSErrUndefinedVariable				(ERRBASE_FRAMES - 807)	// Undefined variable
#define kNSErrUndefinedGlobalFunction		(ERRBASE_FRAMES - 808)	// Undefined global function
#define kNSErrUndefinedMethod					(ERRBASE_FRAMES - 809)	// Undefined method
#define kNSErrMissingProtoForResend			(ERRBASE_FRAMES - 810)	// No _proto for inherited send
#define kNSErrNilContext						(ERRBASE_FRAMES - 811)	// Tried to access slot in NIL context
#define kNSErrBadCharForString				(ERRBASE_FRAMES - 815)	// The operation would make the (rich) string invalid


#endif	/* __NEWTONERRORS_H */
