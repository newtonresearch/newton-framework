/*
	File:		OptionArray.h

	Contains:	Interface to CommAPI config/options

	Copyright:	© 1992-1996 by Apple Computer, Inc., all rights reserved.
*/

#if !defined(__OPTIONARRAY_H)
#define __OPTIONARRAY_H 1

#ifndef FRAM
#ifndef __NEWTMEMORY_H
#include "NewtonMemory.h"
#endif
#ifndef __USERSHAREDMEM_H
#include "UserSharedMem.h"
#endif
#endif // notFRAM


// COption status bit masks
#define kProcessedFlagMask		(0x80000000)
#define kExtendedFormatMask	(0x40000000)	// not the same as COptionExtended!

// COption element types
#define kServiceSpecificType	(0x02000000)	// used for COptionExtended
#define kServiceType				(0x04000000)
#define kAddressType				(0x08000000)
#define kOptionType				(0x0C000000)
#define kConfigType				(0x10000000)
#define kTypeMask					(0x3F000000)

// COption OpCode mask
#define kOpCodeMask				(0x0000FF00)
#define kOpCodeResultMask		(0x000000FF)


// COption (individual) opCodes
#define opInvalid					(0x0000)			//    0: invalid opCode (the default)
#define opSetNegotiate			(0x0100)			//  256: set, may not be what we expected
#define opSetRequired			(0x0200)			//  512: set, fail if unable
#define opGetDefault				(0x0300)			//  768: get default option value
#define opGetCurrent				(0x0400)			// 1024: get current option value

// COptionArray (aggregate) opCodes [used in CEndpoint::optMgmt]
#define opProcess					(0x0500)			// 1280: process each option (use option opCode)


#ifdef FRAM

// COption (individual) result codes
#define opInProgress				 1			// operation is in progress, and not yet complete
#define opSuccess					 0			// operation completed successfully
#define opFailure					-1			// operation failed
#define opPartSuccess			-2			// (set) but actual value different from requested value (opSetNegotiate only)
#define opReadOnly				-3			// set attempted on read-only option
#define opNotSupported			-4			// option not supported
#define opBadOpCode				-5			// opCode is invalid

// COptionArray (aggregate) result codes
#define opNotFound				-6			// option not found
#define opTruncated				-7			// one or more requested options are missing
#define opPadded					-8			// option was padded with garbage

#endif // FRAM

#ifndef FRAM

// COption (individual) result codes
#define opInProgress				((ULong)1)			// operation is in progress, and not yet complete
#define opSuccess					((ULong)0)			// operation completed successfully
#define opFailure					((ULong)-1)			// operation failed
#define opPartSuccess			((ULong)-2)			// (set) but actual value different from requested value (opSetNegotiate only)
#define opReadOnly				((ULong)-3)			// set attempted on read-only option
#define opNotSupported			((ULong)-4)			// option not supported
#define opBadOpCode				((ULong)-5)			// opCode is invalid

// COptionArray (aggregate) result codes
#define opNotFound				((ULong)-6)			// option not found
#define opTruncated				((ULong)-7)			// one or more requested options are missing
#define opPadded					((ULong)-8)			// option was padded with garbage

class COptionArray;
class COptionIterator;
class CSubArrayOption;

typedef int OptionOpCodes;
typedef int OptionOpCodesResult;


/*--------------------------------------------------------------------------------
	COption
--------------------------------------------------------------------------------*/

class COption
{
public:
				COption(ULong type = kOptionType);

	bool		isProcessed() const;	// has this option been processed?
	bool		isExtended() const;	// is this option in extended format?
	bool		isService() const;	// is this COption a service type?
	bool		isAddress() const;	// is this COption an address type?
	bool		isOption() const;		// is this COption a option type?
	bool		isConfig() const;		// is this COption a config type?
	bool		isServiceSpecific() const;		// is this COption matched for a particular service?

	void		reset();

	void		setLabel(ULong label);
	void		setLength(size_t length);
	ULong		label() const;			// return label
	size_t	length() const;		// return length

	void		setOpCode(ULong opCode);			// set the option opCode
	void		setOpCodeResult(ULong opCode);	// set the option opCode result
	ULong		getOpCode() const;					// return the option opCode
	ULong		getOpCodeResults() const;			// return the option opCode results

	void		setProcessed();	// set as processed
	void		setAsService(ULong serviceId);	// Set fLabel to the serviceId and call SetAsService()
	void		setAsService();						// Set the service flag
	void		setAsOption(ULong optionId);
	void		setAsConfig(ULong configId);
	void		setAsAddress(ULong addrId);

	NewtonErr	copyDataFrom(COption* source);
	COption *	clone(void);

private:
	friend class COptionIterator;
	friend class COptionArray;
	friend class COptionExtended;

	ULong		fLabel;			// option label
	ULong		fFlags;			// flag bits
	size_t	fLength;			// length of data following, excluding header (i.e., fLabel, fFlags, fLength)

	UChar		fData[0];		// variable amount of data follows, derived classes specify
};

inline bool			COption::isProcessed() const			{ return ( (fFlags & kProcessedFlagMask) == kProcessedFlagMask ); }
inline bool			COption::isExtended() const			{ return ( (fFlags & kExtendedFormatMask) == kExtendedFormatMask ); }
inline bool			COption::isService() const				{ return ( (fFlags & kTypeMask) == kServiceType ); }
inline bool			COption::isAddress() const				{ return ( (fFlags & kTypeMask) == kAddressType ); }
inline bool			COption::isOption() const				{ return ( (fFlags & kTypeMask) == kOptionType ); }
inline bool			COption::isConfig() const				{ return ( (fFlags & kTypeMask) == kConfigType ); }
inline bool			COption::isServiceSpecific() const	{ return ( (fFlags & kTypeMask) == kServiceSpecificType ); }
inline void			COption::setLabel(ULong inLabel)		{ fLabel = inLabel; }
inline void			COption::setLength(size_t inLength)	{ fLength = inLength; }
inline ULong		COption::label() const					{ return fLabel; }
inline size_t		COption::length() const					{ return fLength; }
inline void			COption::setOpCode(ULong opCode)		{ fFlags = (fFlags & ~kOpCodeMask) | (opCode & kOpCodeMask); }
inline void			COption::setOpCodeResult(ULong opCode)	{ fFlags = (fFlags & ~kOpCodeResultMask) | (opCode & kOpCodeResultMask); }
inline ULong		COption::getOpCode() const				{ return (fFlags & kOpCodeMask); }
inline ULong		COption::getOpCodeResults() const	{ return ((long) ((SByte)(fFlags & kOpCodeResultMask))); }	// we want the low eight bits to be sign extended before returning as ULong
inline void			COption::setProcessed()					{ fFlags |= kProcessedFlagMask; }


/*--------------------------------------------------------------------------------
	COptionExtended
--------------------------------------------------------------------------------*/

class COptionExtended : public COption
{
public:
					COptionExtended(ULong type = kOptionType);

	void			setAsServiceSpecific(ULong service);
	ULong			serviceLabel() const;
	void			setExtendedResult(NewtonErr result);
	NewtonErr	getExtendedResult() const;

private:
	ULong			fServiceLabel;
	NewtonErr	fExtendedResult;
};

inline ULong		COptionExtended::serviceLabel() const	{ return fServiceLabel; }
inline void			COptionExtended::setExtendedResult(NewtonErr result)	{ fExtendedResult = result; }
inline NewtonErr	COptionExtended::getExtendedResult() const	{ return fExtendedResult; }


/*--------------------------------------------------------------------------------
	CSubArrayOption
--------------------------------------------------------------------------------*/

class CSubArrayOption : public COption
{
public:
					CSubArrayOption(size_t inSize, ULong inCount);

	size_t		count(void) const;
	Ptr			array(void) const;

private:
	size_t		fCount;
	UChar			fArray[];
};

inline size_t		CSubArrayOption::count(void) const		{ return fCount; }
inline Ptr			CSubArrayOption::array(void) const		{ return (Ptr) fArray; }


/*--------------------------------------------------------------------------------
	COptionArray
--------------------------------------------------------------------------------*/

class COptionArray : SingleObject
{
public:
						COptionArray();
						~COptionArray();

		NewtonErr	init(void);
		NewtonErr	init(size_t initialSize);
		NewtonErr	init(ObjectId sharedId, ULong optionCount);
		NewtonErr	init(CSubArrayOption* array);

		void			reset();

	// array manipulation primitives

		COption *	optionAt(ArrayIndex index);
		NewtonErr	copyOptionAt(ArrayIndex index, COption* copy);

		NewtonErr	appendOption(COption* opt);
		NewtonErr	appendVarOption(COption* opt, void* varData, size_t varLen);
		NewtonErr	appendSubArray(COptionArray* subArray);

		NewtonErr	insertOptionAt(ArrayIndex index, COption* opt);
		NewtonErr	insertVarOptionAt(ArrayIndex index, COption* opt, void* varData, size_t varLen);
		NewtonErr	insertSubArrayAt(ArrayIndex index, COptionArray* subArray);

		NewtonErr	removeOptionAt(ArrayIndex index);
		NewtonErr	removeAllOptions(void);

	// miscellaneous functions

		size_t		getSize();				// how many bytes array contains
		ArrayIndex	count();					// how many COption elements on array

		NewtonErr	merge(COptionArray* optionArray);
		bool			isEmpty(void);

	// shared memory access

		bool			isShared();
		ObjectId		getSharedId();
		NewtonErr	makeShared(ULong permissions = kSMemReadOnly);
		NewtonErr	unshare();

		NewtonErr	copyFromShared(ObjectId sharedId, ULong count);
		NewtonErr	copyToShared(ObjectId sharedId);
		NewtonErr	shadowCopyBack(void);

private:
		friend class COptionIterator;

		ArrayIndex		fCount;				// elements in array
		char *			fArrayBlock;		// element storage
		COptionIterator *	fIterator;		// linked list of iterators active on this array
		CUSharedMem		fSharedMemoryObject;
		bool				fIsShared;			// YES if we have been shared
		bool				fShadow;				// YES if were created from a shared memory object

#ifdef DebugOptionArrayAtomicity
		bool				fNeedSemaphore;
#endif
};

inline size_t		COptionArray::getSize()			{ return GetPtrSize(fArrayBlock); }
inline ArrayIndex	COptionArray::count()			{ return fCount; }
inline NewtonErr	COptionArray::appendOption(COption* opt)	{ return insertOptionAt(fCount, opt); }
inline NewtonErr	COptionArray::appendVarOption(COption* opt, void* varData, size_t varLen)	{ return insertVarOptionAt(fCount, opt, varData, varLen); }
inline NewtonErr	COptionArray::appendSubArray(COptionArray* subArray)	{ return insertSubArrayAt(fCount, subArray); }
inline bool			COptionArray::isEmpty()			{ return (fCount == 0); }
inline bool			COptionArray::isShared()		{ return fIsShared; }
inline ObjectId	COptionArray::getSharedId()	{ return (ObjectId)fSharedMemoryObject; }


/*--------------------------------------------------------------------------------
	COptionIterator
--------------------------------------------------------------------------------*/

class COptionIterator : SingleObject
{
public:
						COptionIterator();
						COptionIterator(COptionArray * itsOptionArray);
						COptionIterator(COptionArray * itsOptionArray, ArrayIndex itsLowBound, ArrayIndex itsHighBound);
						~COptionIterator();

		void			init(COptionArray * itsOptionArray, ArrayIndex itsLowBound, ArrayIndex itsHighBound);
		void			initBounds(ArrayIndex itsLowBound, ArrayIndex itsHighBound);

		void			reset(void);
		void			resetBounds(void);

		ArrayIndex	firstIndex(void);
		ArrayIndex	nextIndex(void);
		ArrayIndex	currentIndex(void);

		COption *	firstOption(void);
		COption *	nextOption(void);
		COption *	currentOption(void);

		COption *	findOption(ULong label);

		bool			more(void);

protected:
		void			advance(void);

		COptionArray *	fOptionArray;			// the associated option array
		ArrayIndex		fCurrentIndex;			// current index of this iteration
		ArrayIndex		fLowBound;				// lower bound of iteration in progress
		ArrayIndex		fHighBound;				// upper bound of iteration in progress
		COption *		fCurrentOption;

private:
		friend class COptionArray;

		void			removeOptionAt(ArrayIndex theIndex);
		void			insertOptionAt(ArrayIndex theIndex);
		void			deleteArray(void);

		COptionIterator *	appendToList(COptionIterator* toList);
		COptionIterator *	removeFromList(void);

		COptionIterator *	fPreviousLink;			// link to previous iterator
		COptionIterator *	fNextLink;				// link to next iterator

// private and not implemented to prevent copying and assignment

		COptionIterator(const COptionIterator&);
		COptionIterator& operator=(const COptionIterator&);
};


#endif // notFRAM

#endif	/* __OPTIONARRAY_H */
