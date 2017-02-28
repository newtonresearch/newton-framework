/*	----------------------------------------------------------------
**
**	Protocols.h  --  Protocols, monitors, protocol meta-information, protocol registry
**
**		class CProtocol;				// Base class for protocols
**		class CClassInfo;				// Meta-information for protocols
**		MONITOR CClassInfoRegistry;		// Clearing house for implementations of protocols
**
**	Copyright © 1992-1994, Apple Computer, Inc.   All Rights Reserved.
**
**	----------------------------------------------------------------
**
**	Derived from v31 internal.
**
*/

#ifndef __PROTOCOLS_H
#define __PROTOCOLS_H

#ifndef __NEWTON_H
#include "Newton.h"
#endif


/*
**	Redefine some ProtocolGen keywords when we're really
**	going through CFront
*/

#ifndef PROTOCOLGEN
#define PROTOCOL				class
#define MONITOR				class
#define PSEUDOSTATIC			/*nothing*/
#define NONVIRTUAL			/*nothing*/
#define INVISIBLE				/*nothing*/
#define PROTOCOLVERSION(x)	/*nothing*/
#define MONITORVERSION(x)	/*nothing*/
#define CAPABILITIES(x)		/*nothing*/
#endif

/*
**	If no protocols, define VIRTUAL/ENDVIRTUAL
**	so we can compile protocols as classes on Mac etc.
**	(that is, functions become pure virtual).
*/

#ifdef hasNoProtocols
#define VIRTUAL				virtual
#define ENDVIRTUAL			= 0
#else
#define VIRTUAL				/*nothing*/
#define ENDVIRTUAL			/*nothing*/
#endif


class CClassInfo;
MONITOR CClassInfoRegistry;


typedef void * (*AllocProcPtr)(void);
typedef void (*FreeProcPtr)(void *);
typedef int (*EntryProcPtr)(void);

//	CodeProcPtr	--	Pass control to the first byte of a hunk-O-code
//						with a selector as the first argument.

typedef void * (*CodeProcPtr)(int selector, ...);


/*--------------------------------------------------------------------------------
	C P r o t o c o l
	Base class for protocols and protocol monitors.
--------------------------------------------------------------------------------*/

class CProtocol
{
public:
	void			become(const CProtocol * instance);	// forward to an instance
	void			become(ObjectId inMonitorId);			// forward to a monitor (via kernel id)
	ObjectId		getMonitorId() const;					// ==> monitor id, or zero
	operator		ObjectId();									// ==> monitor id, or zero
	const CClassInfo *	classInfo() const;			// ==> info about the protocol (doesn't work for monitors)
	void			setType(const CClassInfo * info);	// set this instance's type

	NewtonErr	startMonitor(								// start object up as a monitor
							size_t		inStackSize,
							ObjectId		inEnvironmentId = 0,
							ULong			inName = 'mntr',
							bool			inRebootProtected = false);

	NewtonErr	destroyMonitor();							// destroy the monitor

private:
	/*
	**	Never change these
	**	(generated glue depends on this layout)
	*/
	void *				fRuntime;			// runtime usage (e.g. Throw() cleanup of autos for exceptions)
	const CProtocol *	fRealThis;			// -> true "this" for auto instances
	int32_t *			fBTable;				// -> dispatch table
//	const void **		fBTable;				// originally was jump table
	ObjectId				fMonitorId;			// for monitors, the monitor id

	friend class CClassInfo;				// allow (e.g.) CClassInfo::makeAt to diddle our innards
	friend void PrivateClassInfoMakeAt(const CClassInfo *, const void * proto);	// for pre jump table use
};

inline CProtocol::operator ObjectId()
{ return fMonitorId; }


/*
**	Put one of these inside the {...} of your protocol implementations.
**	It declares some magic stuff you’re probably better off not puzzling over.
**
**	That is:
**
**		PROTOCOL TFooImpl : public TFoo
**		{
**		public:
**			PROTOCOL_IMPL_HEADER_MACRO(TFooImpl)
**			<your methods>
**		};
**
**	ProtocolGen makes one of these for you if you generate source code
**	via '-ServerHeader'.
**
*/
#if defined(forARM)  ||  defined(PROTOCOLGEN)
#define	PROTOCOL_IMPL_HEADER_MACRO(name) \
	static size_t sizeOf(void); \
	static const CClassInfo * classInfo(void);
#else
#define	PROTOCOL_IMPL_HEADER_MACRO(name) \
	static size_t sizeOf(void); \
	static const CClassInfo * classInfo(void);
#endif /*forARM*/

/*
**	For each of your protocol implementations, put one of these in your
**	".c" or ".cp" files.  You can follow it with a semicolon or not, depending
**	on how lax CFront is about extra semicolons today.
**
**		#include	"FooImpl.h"
**
**		PROTOCOL_IMPL_SOURCE_MACRO(TFooImpl)
**
**	ProtocolGen makes one of these for you if you generate source
**	code via '-ServerCFile'.
**
*/
#if forARM
#define	PROTOCOL_IMPL_SOURCE_MACRO(name) \
	size_t name :: sizeOf(void) { return sizeof(name); }
#else
#define	PROTOCOL_IMPL_SOURCE_MACRO(name) \
	size_t name :: sizeOf(void) { return sizeof(name); }
#endif /*forARM*/


/*--------------------------------------------------------------------------------
	C C l a s s I n f o
	Meta-information for protocols.
--------------------------------------------------------------------------------*/

class CClassInfo
{
public:
	const char *	implementationName(void)	const;	// implementation name
	const char *	interfaceName(void)			const;	// name of public interface
	const char *	signature(void)				const;	// signature (actually, capability list)
	size_t			size(void)						const;	// instance size
	EntryProcPtr	entryProc(void)				const;	// return address of monitor entry proc
	AllocProcPtr	allocProc(void)				const;	// return address of OperatorNew() proc, or nil
	FreeProcPtr		freeProc(void)					const;	// return address of OperatorDelete() proc, or nil
	ULong				version(void)					const;	// implementation version
	ULong				flags(void)						const;	// various flags (see below)
	NewtonErr		registerProtocol(void)		const;	// register with protocol-server
	NewtonErr		deregisterProtocol(void)	const;	// de-register with protocol-server
	CProtocol *		make(void)						const;	// was New(); make an instance
	void				makeAt(const void *)			const;	// construct an instance at the address
	void				destroy(CProtocol *)			const;	// destroy an instance at the address
	CodeProcPtr		selector(void)					const;	// return address of selector proc
	const char *	getCapability(const char * inKey)	const;	// test if protocol has a specific capability, return it
	const char *	getCapability(ULong inIdentifier)	const;	// test if protocol has a specific capability, return it
	bool				hasInstances(ArrayIndex * outCount)	const; 	// return true if instances of this protocol exist, count = number of them

private:
	friend class CProtocol;
	friend size_t PrivateClassInfoSize(const CClassInfo *);	// for pre jump table use
	friend void PrivateClassInfoMakeAt(const CClassInfo *, const void * proto);
	friend const char * PrivateClassInfoInterfaceName(const CClassInfo *);
	friend const char * PrivateClassInfoImplementationName(const CClassInfo *);

	/*
	**	Never change these; generated glue depends on this layout,
	** and if you change anything here you will definitely wish you hadn’t.
	**
	**	This structure is relocatable.  Please keep it that way.
	*/
#if __LP64__
	int32_t			fReserved1;					// (reserved for future use, zero for now)
	int32_t			fNameDelta;					// SRO (Self-Relative-Offset) to asciz implementation name
	int32_t			fInterfaceDelta;			// SRO to asciz protocol name
	int32_t			fSignatureDelta;			// SRO to asciz signature
	int32_t			fBTableDelta;				// SRO to dispatch table
	int32_t			fEntryProcDelta;			// SRO to monitor entry (valid only for monitors)
	uint32_t			fSizeofBranch;				// offset to sizeof-code
	uint32_t			fAllocBranch;				// offset to alloc-code, or zero
	uint32_t			fFreeBranch;				// offset to OperatorDelete code, or zero
	uint32_t			fDefaultNewBranch;		// offset to New(void), or MOV PC,LK
	uint32_t			fDefaultDeleteBranch;	// offset to Delete(void), or MOV PC,LK
	uint32_t			fVersion;					// this implementation's version
	uint32_t			fFlags;						// various flags (see below)
	int32_t			fReserved2;					// (reserved for future use, zero for now)
	uint32_t			fSelectorBranch;			// offset to bail-out function (that returns nil now)
#else
	long				fReserved1;					// (reserved for future use, zero for now)
	long				fNameDelta;					// SRO (Self-Relative-Offset) to asciz implementation name
	long				fInterfaceDelta;			// SRO to asciz protocol name
	long				fSignatureDelta;			// SRO to asciz signature
	long				fBTableDelta;				// SRO to dispatch table
	long				fEntryProcDelta;			// SRO to monitor entry (valid only for monitors)
	unsigned long	fSizeofBranch;				// ARM branch to sizeof-code
	unsigned long	fAllocBranch;				// ARM branch to alloc-code, or zero
	unsigned long	fFreeBranch;				// ARM branch to OperatorDelete code, or zero
	unsigned long	fDefaultNewBranch;		// ARM branch to New(void), or MOV PC,LK
	unsigned long	fDefaultDeleteBranch;	// ARM branch to Delete(void), or MOV PC,LK
	unsigned long	fVersion;					// this implementation's version
	unsigned long	fFlags;						// various flags (see below)
	long				fReserved2;					// (reserved for future use, zero for now)
	unsigned long	fSelectorBranch;			// ARM branch to bail-out function (that returns nil now)
#endif
};


inline ULong CClassInfo::version() const
{ return fVersion; }

inline ULong CClassInfo::flags(void) const
{ return fFlags; }

enum
{
	 kci_IsMonitor		= 0x00000001		// bit 0 ==> is monitor
};


/* -------------------------------------------------------------------------------
	intel assembler glue.
------------------------------------------------------------------------------- */

#if __LP64__
#define CLASSINFO_BEGIN " leaq 0f(%rip), %rax\n leave\n ret\n .align 2\n0:"
#define CLASSINFO_END "5: \n6: movl 0, %eax\n ret\n"
#else
#define CLASSINFO_BEGIN " leal 0f, %eax\n ret\n .align 2\n0:"
#define CLASSINFO_END "5: \n6: movl 0, %eax \n ret\n"
#endif

/* -------------------------------------------------------------------------------
	C C l a s s I n f o R e g i s t r y
	P-class interface.
	A registry of protocols.
------------------------------------------------------------------------------- */

MONITOR CClassInfoRegistry : public CProtocol
{
public:
	static CClassInfoRegistry * make(const char * inName);	// was New()
	void			destroy(void);											// was Delete()

	NewtonErr	registerProtocol(const CClassInfo *, ULong refCon = 0);
	NewtonErr	deregisterProtocol(const CClassInfo *, bool specific = false);
	bool			isProtocolRegistered(const CClassInfo *, bool specific = false) const;
#if defined(correct)
	int						seed() const;
	const CClassInfo *	first(int seed, ULong * pRefCon=0) const;
	const CClassInfo *	next(int seed, const CClassInfo * from, ULong * pRefCon=0) const;
	const CClassInfo *	find(const char * intf, const char * impl, int skipCount, ULong * pRefCon=0) const;
#endif
	const CClassInfo *	satisfy(const char * intf, const char * impl, ULong version) const;
	//	2.0 calls
	const CClassInfo *	satisfy(const char * intf, const char * impl, const char * capability) const;
	const CClassInfo *	satisfy(const char * intf, const char * impl, const char * capability, const char * capabilityValue) const;
	const CClassInfo *	satisfy(const char * intf, const char * impl, const int capability, const int capabilityValue = 0) const;

	void			updateInstanceCount(const CClassInfo * classinfo, int adjustment);
	ArrayIndex	getInstanceCount(const CClassInfo * classinfo);
};


extern void							StartupProtocolRegistry(void);
extern CClassInfoRegistry *	GetProtocolRegistry(void);

extern CProtocol *				MakeByName(const char * abstract, const char * implementation);
extern CProtocol *				MakeByName(const char * abstract, const char * implementation, ULong version);	// ask for version
extern CProtocol *				MakeByName(const char * abstract, const char * implementation, const char * capability);
extern const CClassInfo *		ClassInfoByName(const char * abstract, const char * implementation, ULong version = 0);
extern CProtocol *				AllocInstanceByName(const char * abstract, const char * implementation);
extern void							FreeInstance(CProtocol *);


/*	----------------------------------------------------------------
**
**	Standard selectors for hunks-of-code
**
*/
enum
{
	kCodeInit			= 0,			// int (*code)(kCodeInit);	// initialization, returns zero on success
	kCodeVersion		= 1,			// int (*code)(kCode);		// version, zero for now
	kCodeInfo			= 2,			// int (*code)(kCode);		// any number you like
	kCodeClassCount	= 3,			// int (*code)(kCode);		// ==> # classes
	kCodeClassAt		= 4,			// const CClassInfo* (*code)(kCode, int Nth);	// class-info-proc for Nth class
	kCodeReserved5		= 5,			// reserved
	kCodeReserved6		= 6,			// reserved
	kCodeReserved7		= 7			// reserved
	// First generally available selector is 8
};


/*	----------------------------------------------------------------
**
**	Functions for working with hunks-of-code
**
*/
extern const CClassInfo *	ClassInfoFromHunkByName(void * hunk, const char * abstract, const char * implementation);
extern CProtocol *			NewFromHunkByName(void * hunk, const char * abstract, const char * implementation);


/*	----------------------------------------------------------------
**
**	SAFELY_CAST  --  safely cast an instance of a protocol to some
**					 expected implementation
**
**		o	result is zero if the implementation isn't the exact
**			expected one
**
*/
#define	SAFELY_CAST(instance, to) \
	instance->classInfo() == to::classInfo() ? (to*)instance : 0

#endif /* __PROTOCOLS_H */
