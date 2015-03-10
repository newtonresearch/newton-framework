/*
	File:		PartHandler.h

	Copyright:	© 1992-1996 by Apple Computer, Inc., all rights reserved.

	Derived from v1 internal, 1/8/96.
*/

#if !defined(__PARTHANDLER_H)
#define __PARTHANDLER_H 1

#ifndef __NEWTONTYPES_H
#include "NewtonTypes.h"
#endif

#ifndef __OSERRORS_H
#include "OSErrors.h"
#endif

#ifndef __PACKAGETYPES_H
#include "PackageTypes.h"
#endif


//////////////////////////////////////////////////////////////////////////

class CEventHandler;
class CPartEventHandler;
class CPkPartInstallEvent;
class CPkPartInstallEventReply;
class CPkPartRemoveEvent;
class CPkRegisterEvent;
class CEndpointPipe;
class CPipe;

class CUAsyncMessage;

ObjectId		PackageManagerPortId();
NewtonErr	LoadPackage(CEndpointPipe * inPipe, ObjectId * outPackageId, bool inWillRemove = YES);
NewtonErr	LoadPackage(CPipe * inPipe, ObjectId * outPackageId, bool inWillRemove = YES);

NewtonErr	LoadPackage(Ptr buffer, SourceType inType, ObjectId * outPackageId);
NewtonErr	LoadPackage(CEndpointPipe * inPipe, SourceType inType, ObjectId * outPackageId);
NewtonErr	LoadPackage(CPipe * inPipe, SourceType inType , ObjectId * outPackageId);

void			RemovePackage(ObjectId inPackageId);


//////////////////////////////////////////////////////////////////////////


class CPartHandler : public SingleObject
{
public:
				CPartHandler();
	virtual	~CPartHandler();

	NewtonErr   init(ULong type);

						// install and removal functions
	virtual	NewtonErr   install(const PartId& partId, SourceType sourceType,  PartInfo * partInfo) = 0;	// must override
	virtual	NewtonErr 	remove (const PartId& partId, PartType   partType,    RemoveObjPtr	removePtr) = 0;	// must override

						// backup methods
						// lastBackupDate is in minutes from the beginning of time
	virtual	NewtonErr 	getBackupInfo(const PartId& partId, PartType partType, RemoveObjPtr removePtr , PartInfo * partInfo, ULong lastBackupDate, bool * needsBackup);
	virtual	NewtonErr	backup(const PartId& partId, RemoveObjPtr removePtr , CPipe * pipe);

protected:
						// Expand: user may override. Default action is to do a straight copy
	virtual	NewtonErr	expand(void * data, CPipe * pipe, PartInfo * info);

					// return information functions
	void				setRemoveObjPtr(RemoveObjPtr obj);
	void				rejectPart(void);
	void				replyImmed(NewtonErr err);	// reply immediately to the package manager

					// query functions
	Ptr				getSourcePtr(void);

					// load part functions

					// Copy
					// a function for copying data from the source.
					// It will do a straight copy for pointers, will call expand() for pipes
	NewtonErr		copy(void * data);

private:
	friend class CPartEventHandler;

	void				install(CPkPartInstallEvent * installEvent);
	void				remove (CPkPartRemoveEvent  * removeEvent );

	NewtonErr		registerPart(void);
	NewtonErr		unregisterPart(void);


	ULong				fType;				// +04
	ULong				fMaxInfoSize;

	SourceType		fSourceType;
	PartSource		fSource;				// +14

	PartInfo *		fPartInfo;			// +1C

	RemoveObjPtr	fRemoveObjPtr;		// +20
	bool				fAccept;				// +24

	bool				fInitFailed;

	CEventHandler *fEventHandler;		// +28

	// async message stuff for registering and replying

	CUAsyncMessage *		fAsyncMessage;
	CPkRegisterEvent *	fRegisterEvent;	// +30
	bool						fReplied;

	// device info
};



/*------------------------------------------------------------------------------
	C P a c k a g e S t o r e P a r t H a n d l e r
------------------------------------------------------------------------------*/

class CPackageStorePartHandler : public CPartHandler
{
public:
					CPackageStorePartHandler();

	NewtonErr	install(const PartId & inPartId, SourceType inType, PartInfo * info);
	NewtonErr	remove(const PartId & inPartId, PartType inType, RemoveObjPtr inContext);
// size +40
};


/*------------------------------------------------------------------------------
	C F r a m e P a r t H a n d l e r
------------------------------------------------------------------------------*/

class CFramePartHandler : public CPartHandler
{
public:
	virtual NewtonErr	install(const PartId & inPartId, SourceType inSource, PartInfo * info);
	virtual NewtonErr	remove(const PartId & inPartId, PartType inType, RemoveObjPtr inContext);

	virtual NewtonErr	installFrame(RefArg inPartFrame, const PartId & inPartId, SourceType inSource, PartInfo * info) = 0;
	virtual NewtonErr	removeFrame(RefArg inContext, const PartId & inPartId, PartType inSource) = 0;

protected:
	virtual NewtonErr	expand(void * outData, CPipe * inPipe, PartInfo * info);
	NewtonErr	setFrameRemoveObject(RefArg inContext);

	RemoveObj *	fContext;	// +38
// size +3C
};


/*------------------------------------------------------------------------------
	C F o r m P a r t H a n d l e r
------------------------------------------------------------------------------*/

class CFormPartHandler : public CFramePartHandler
{
public:
	virtual NewtonErr	getBackupInfo(const PartId & inPartId, ULong, long, PartInfo *, ULong, bool *);
	virtual NewtonErr	backup(const PartId & inPartId, long, CPipe * inPipe);

	virtual NewtonErr	installFrame(RefArg inPartFrame, const PartId & inPartId, SourceType inSource, PartInfo * info);
	virtual NewtonErr	removeFrame(RefArg inContext, const PartId & inPartId, PartType inSource);
};


/*------------------------------------------------------------------------------
	C B o o k P a r t H a n d l e r
	Implementation probably belongs with the Librarian.
------------------------------------------------------------------------------*/

class CBookPartHandler : public CPartHandler
{
public:
	virtual NewtonErr	install(const PartId & inPartId, SourceType inSource, PartInfo * info);
	virtual NewtonErr	remove(const PartId & inPartId, PartType inSource, RemoveObjPtr inContext);

protected:
	virtual NewtonErr	expand(void * outData, CPipe * inPipe, PartInfo * info);
};


/*------------------------------------------------------------------------------
	C D i c t P a r t H a n d l e r
------------------------------------------------------------------------------*/

class CDictPartHandler : public CPartHandler
{
public:
	virtual NewtonErr	install(const PartId & inPartId, SourceType inSource, PartInfo * info);
	virtual NewtonErr	remove(const PartId & inPartId, PartType inSource, RemoveObjPtr inContext);

protected:
	virtual NewtonErr	expand(void * outData, CPipe * inPipe, PartInfo * info);

private:
	NewtonErr	addDictionaries(RefArg, RefArg);
};


/*------------------------------------------------------------------------------
	C A u t o S c r i p t P a r t H a n d l e r
------------------------------------------------------------------------------*/

class CAutoScriptPartHandler : public CFramePartHandler
{
public:
	virtual NewtonErr	installFrame(RefArg inPartFrame, const PartId & inPartId, SourceType inSource, PartInfo * info);
	virtual NewtonErr	removeFrame(RefArg inContext, const PartId & inPartId, PartType inSource);
};


/*------------------------------------------------------------------------------
	C C o m m P a r t H a n d l e r
------------------------------------------------------------------------------*/

class CCommPartHandler : public CAutoScriptPartHandler
{
public:
	virtual NewtonErr	installFrame(RefArg inPartFrame, const PartId & inPartId, SourceType inSource, PartInfo * info);
	virtual NewtonErr	removeFrame(RefArg inContext, const PartId & inPartId, PartType inSource);
};


/*------------------------------------------------------------------------------
	C F o n t P a r t H a n d l e r
	Originally called just CFontPart.
------------------------------------------------------------------------------*/

class CFontPartHandler : public CFramePartHandler
{
public:
	virtual NewtonErr	installFrame(RefArg inPartFrame, const PartId & inPartId, SourceType inSource, PartInfo * info);
	virtual NewtonErr	removeFrame(RefArg inContext, const PartId & inPartId, PartType inSource);
};


#endif	/* __PARTHANDLER_H */
