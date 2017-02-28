/*
	File:		Indexes.h

	Contains:	Soup index declarations.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__INDEXES_H)
#define __INDEXES_H 1

#include "StoreWrapper.h"
#include "NodeCache.h"

/*----------------------------------------------------------------------
	S K e y
	An SKey instance is a data item.
----------------------------------------------------------------------*/

#define kSKeyBufSize 78

struct SKey
{
			SKey();

	// setters
	void	set(ArrayIndex inKeySize, void * inKey);
	void	setFlags(unsigned char inFlags);
	void	setMissingKey(int inBit);
	void	setSize(unsigned short inSize);
	// getters
	ArrayIndex	size(void) const;
	ULong			flags(void) const;
	const void *	data(void) const;

	bool	equals(const SKey & o) const;

	SKey&	operator=(const SKey & inKey);
	SKey&	operator=(int inKey);
	SKey&	operator=(UniChar inKey);
	SKey&	operator=(const double inKey);

	operator int() const;
	operator UniChar() const;
	operator double() const;

	enum { kOffsetToData = sizeof(unsigned short) };

protected:
	unsigned short	flagBits:8;		// 0x80 => is multislot?
	unsigned short	length:8;
	char	buf[kSKeyBufSize];		// actually variable length, but this is what you get if you new SKey
};

inline					SKey::SKey() : flagBits(0), length(0)  { }
inline ArrayIndex		SKey::size(void) const  { return length; }
inline ULong			SKey::flags(void) const  { return flagBits; }
inline const void *	SKey::data(void) const  { return buf; }


struct STagsBits : public SKey
{
	void	setTag(unsigned short index);
	bool	validTest(const STagsBits & inArg1, int inArg2) const;
};


enum
{
	kTagsEqual,
	kTagsAll,
	kTagsAny,
	kTagsNone
};

// Function interface
Ref	GetEntryKey(RefArg inEntry, RefArg inPath);
bool	GetEntrySKey(RefArg inEntry, RefArg inQuerySpec, SKey * outKey, bool * outIsComplexKey);
void	MultiKeyToSKey(RefArg inKey, RefArg inType, SKey * outKey);
void	RichStringToSKey(RefArg inStr, SKey * outKey);
void	KeyToSKey(RefArg inKey, RefArg inType, SKey * outKey, short * outSize, bool * outIsComplexKey);
Ref	SKeyToKey(const SKey& inKey, RefArg inType, short * outSize);


/*----------------------------------------------------------------------
	K e y F i e l d
	A KeyField holds a list of SKeys.
	The first SKey is yer actual key, followed by 0..n data values.
	Since keys and data are preceded by their (short) length, they must always be short aligned.
----------------------------------------------------------------------*/
#define kKeyFieldBufSize 98

struct KeyField
{
	enum { kData, kDupData };
	enum { kOffsetToData = sizeof(unsigned short) };

	SKey * key(void) const;

// should really supply accessors and make these private
// or just make CSoupIndex a friend?
	unsigned short type:2;
	unsigned short	length:14;
	char	buf[kKeyFieldBufSize];
}__attribute__((__packed__));

inline SKey * KeyField::key(void) const  { return (SKey *)buf; }

/*
	Dup nodes are suffixed w/ PACKED extra data
*/
struct DupInfo
{
	short	count;
	ULong	id;
}__attribute__((__packed__));


/*----------------------------------------------------------------------
	I n d e x I n f o
----------------------------------------------------------------------*/

struct IndexInfo
{
	PSSId		rootNodeId;		// +00
	size_t	nodeSize;		// +04	block size (512)
	ULong		keyType;			// +08	string=0, int=1, char=2, real=3, symbol=4, multislot=6
	ULong		dataType;		// +0C	uninitialized=0, normal=1, tags=5
	ULong		x10;	// origin system=0, user=2
	ULong		x14;	// multitypes
	char		x18;	// sort
	char		x19;	// sorting
	short		x1A;
};

// key types
enum { kKeyTypeString, kKeyTypeInt, kKeyTypeChar, kKeyTypeReal, kKeyTypeSymbol, kKeyTypeMultislot };
// data types
enum { kDataTypeUninitialized, kDataTypeNormal, kDataTypeTags = 5 };

struct IndexState
{
	NodeHeader * node;
	int slot;
	bool isDup;
	DupNodeHeader * dupNode;
};

struct PSSIdMapping
{
	PSSId id1;
	PSSId id2;
};


typedef int (*StopProcPtr)(SKey *, SKey *, void *);
typedef int (CSoupIndex::*KeyCompareProcPtr)(const SKey&, const SKey&);


/*----------------------------------------------------------------------
	C A b s t r a c t S o u p I n d e x
----------------------------------------------------------------------*/

class CAbstractSoupIndex
{
public:
					CAbstractSoupIndex();
	virtual		~CAbstractSoupIndex();

	virtual int	find(SKey*, SKey*, SKey*, bool) = 0;
	virtual int	first(SKey*, SKey*) = 0;
	virtual int	last(SKey*, SKey*) = 0;
	virtual int	next(SKey*, SKey*, int, SKey*, SKey*) = 0;
	virtual int	prior(SKey*, SKey*, int, SKey*, SKey*) = 0;

	int	findPrior(SKey*, SKey*, SKey*, bool, bool);
};


/*----------------------------------------------------------------------
	C S o u p I n d e x
----------------------------------------------------------------------*/
#include "Sorting.h"

class CSortingTable;

class CSoupIndex : public CAbstractSoupIndex
{
public:
			CSoupIndex();
			~CSoupIndex();

	static PSSId create(CStoreWrapper * inStoreWrapper, IndexInfo * info);

	void			init(CStoreWrapper * inStoreWrapper, PSSId inId, const CSortingTable * inSortingTable);
	NewtonErr	readInfo(void);

	void			createFirstRoot(void);
	void			setRootNode(PSSId inId);
	NodeHeader *	newNode(void);
	DupNodeHeader *	newDupNode(void);
	void			initNode(NodeHeader * inNode, PSSId inId);
	void			setNodeNo(NodeHeader * inNode, int inSlot, ULong inNodeNum);
	NodeHeader *	readRootNode(bool inCreate);
	NodeHeader *	readANode(PSSId inId, PSSId inNextId);
	DupNodeHeader *	readADupNode(PSSId inId);
	void			deleteNode(PSSId inId);

	void		updateNode(NodeHeader * inNode);
	void		updateDupNode(NodeHeader * inNode);
	void		changeNode(NodeHeader * inNode);
	void		freeNodes(NodeHeader * inNodes);

	void		freeDupNodes(KeyField * inField);

	void		storeAborted(void);
	void		destroy(void);

	void		kfAssembleKeyField(KeyField * outField, void * inKey, void * inData);
	void		kfDisassembleKeyField(KeyField * inField, SKey * outKey, SKey * outData);
	size_t	kfSizeOfKey(void * inKey);
	size_t	kfSizeOfData(void * inData);
	void *	kfFirstDataAddr(KeyField * inField);
	bool		kfNextDataAddr(KeyField * inField, void * inData, void ** outNextData);
	void *	kfLastDataAddr(KeyField * inField);
	size_t	kfDupCount(KeyField * inField);
	void		kfSetDupCount(KeyField * inField, short inCount);
	ULong		kfNextDupId(KeyField * inField);
	void		kfSetNextDupId(KeyField * inField, PSSId inId);
	void		kfInsertData(KeyField * ioField, void * inLocation, void * inData);
	void		kfDeleteData(KeyField * ioField, void * inData);
	void		kfReplaceFirstData(KeyField * ioField, void * inData);
	void *	kfFindDataAddr(KeyField * inField, void * inData, void ** outData);
	void		kfConvertKeyField(int, KeyField * inField);

	void *	firstDupDataAddr(DupNodeHeader * inDupNode);
	bool		nextDupDataAddr(DupNodeHeader * inDupNode, void * inData, void**);
	void *	lastDupDataAddr(KeyField*, DupNodeHeader ** ioDupNode);
	bool		prependDupData(DupNodeHeader * inDupNode, void * inData);
	bool		appendDupData(DupNodeHeader * inDupNode, void * inData);
	bool		deleteDupData(DupNodeHeader * inDupNode, void * inData);
	void *	findDupDataAddr(DupNodeHeader * inDupNode, void * inData, void ** outData);
	void *	findNextDupDataAddr(DupNodeHeader ** ioDupNode, void * inData, bool*);
	void *	findPriorDupDataAddr(DupNodeHeader ** ioDupNode, void * inData, bool*);

	ULong			leftNodeNo(NodeHeader * inNode, int inSlot);
	ULong			rightNodeNo(NodeHeader * inNode, int inSlot);
	ULong			firstNodeNo(NodeHeader * inNode);
	ULong			lastNodeNo(NodeHeader * inNode);
	KeyField *	keyFieldAddr(NodeHeader * inNode, int inSlot);
	KeyField *	firstKeyField(NodeHeader * inNode);
	KeyField *	lastKeyField(NodeHeader * inNode);
	char *		keyFieldBase(NodeHeader * inNode);

	void		createNewRoot(KeyField * inField, ULong);
	bool		insertKey(KeyField*, NodeHeader * inNode, ULong*, bool*);
	void		insertAfterDelete(KeyField*, ULong, NodeHeader * inNode);
	bool		deleteKey(KeyField*, NodeHeader * inNode, bool*);
	bool		moveKey(KeyField * inFromField, KeyField * outToField);
	void		copyKeyFmNode(KeyField * outField, ULong * outNo, NodeHeader * inNode, int inSlot);
	KeyField *	keyBeforeNodeNo(NodeHeader * inNode, ULong inNo, int * outSlot);
	KeyField *	keyAfterNodeNo(NodeHeader * inNode, ULong inNo, int * outSlot);
	int		lastSlotInNode(NodeHeader * inNode);
	size_t	bytesInNode(NodeHeader * inNode);
	bool		roomInNode(NodeHeader * inNode, KeyField * inField);
	bool		nodeUnderflow(NodeHeader * inNode);
	bool		keyInNode(KeyField*, NodeHeader * inNode, PSSId*, int*);
	void		putKeyIntoNode(KeyField*, ULong, NodeHeader * inNode, int);
	void		deleteKeyFromNode(NodeHeader * inNode, int);

	bool		findFirstKey(NodeHeader * inNode, KeyField*);
	bool		findLastKey(NodeHeader * inNode, KeyField*);
	bool		balanceTwoNodes(NodeHeader * inNode1, NodeHeader * inNode2, int);
	bool		mergeTwoNodes(KeyField * inField, NodeHeader * inNode1, NodeHeader * inNode2, NodeHeader * inNode3);
	void		splitANode(KeyField*, ULong*, NodeHeader * inNode, int);
	void		deleteTheKey(NodeHeader * inNode, int, KeyField*);
	bool		getLeafKey(KeyField*, NodeHeader * inNode);

	void		checkForDupData(KeyField*, void*);
	void		storeDupData(KeyField*, void*);
	void		insertDupData(KeyField*, NodeHeader*, int, ULong*, bool*);

	int		compareKeys(const SKey& inKey1, const SKey& inKey2);
	int		stringKeyCompare(const SKey& inKey1, const SKey& inKey2);
	int		longKeyCompare(const SKey& inKey1, const SKey& inKey2);
	int		characterKeyCompare(const SKey& inKey1, const SKey& inKey2);
	int		doubleKeyCompare(const SKey& inKey1, const SKey& inKey2);
	int		asciiKeyCompare(const SKey& inKey1, const SKey& inKey2);
	int		rawKeyCompare(const SKey& inKey1, const SKey& inKey2);
	int		multiKeyCompare(const SKey& inKey1, const SKey& inKey2);

	int		_BTEnterKey(KeyField * inField);
	int		_BTGetNextDupKey(KeyField * inField);
	int		_BTGetNextKey(KeyField * inField);
	int		_BTGetPriorDupKey(KeyField * inField);
	int		_BTGetPriorKey(KeyField * inField);
	int		_BTRemoveKey(KeyField * inField);

	bool		search(KeyField * inKey, NodeHeader ** ioNode, int * ioSlot);
	int		searchNext(KeyField * inKey, NodeHeader ** ioNode, int * ioSlot);
	int		searchPrior(KeyField * inKey, NodeHeader ** ioNode, int * ioSlot);
	int		searchNextDup(KeyField * inKey, NodeHeader ** ioNode, int * ioSlot, DupNodeHeader**);
	int		searchPriorDup(KeyField * inKey, NodeHeader ** ioNode, int * ioSlot, DupNodeHeader**);

	int		findNextKey(KeyField * inKey, NodeHeader ** ioNode, int * ioSlot);
	int		findPriorKey(KeyField * inKey, NodeHeader ** ioNode, int * ioSlot);
	int		findAndGetState(KeyField*, IndexState * outState);
	int		findLastAndGetState(KeyField * inKey, IndexState * outState);
	int		findPriorAndGetState(KeyField * inKey, bool, IndexState * outState);
	int		moveAndGetState(bool, int, KeyField * inKey, IndexState * outState);
	int		moveUsingState(bool, int, KeyField * inKey, IndexState * ioState);

	NewtonErr	add(SKey * inKey, SKey * inData);
	NewtonErr	addInTransaction(SKey * inKey, SKey * inData);
	NewtonErr	Delete(SKey * inKey, SKey * inData);

	int			search(int, SKey * inKey, SKey * inData, StopProcPtr inStopProc, void * ioParms, SKey * outKey, SKey * outData);

	virtual int	find(SKey*, SKey*, SKey*, bool);
	virtual int	first(SKey * outKey, SKey * outData);
	virtual int	last(SKey * outKey, SKey * outData);
	virtual int	next(SKey * inKey, SKey * inData, int inArg3, SKey * outKey, SKey * outData);
	virtual int	prior(SKey * inKey, SKey * inData, int inArg3, SKey * outKey, SKey * outData);

	void		nodeSize(NodeHeader * inNode, size_t & ioSize);
	void		dupNodeSize(KeyField * inField, size_t & ioSize);
	size_t	totalSize(void);

	CStore *	store(void);
	CNodeCache * nodeCache(void);
	PSSId		indexId(void) const;

private:
	static KeyCompareProcPtr	fKeyCompareFns[7];
	static short		fKeySizes[7];
	static KeyField *	fKeyField;
	static KeyField *	fSavedKey;
	static KeyField *	fLeafKey;

	friend class CUnionSoupIndex;

	CStoreWrapper *	fStoreWrapper;		// +04
	CNodeCache *		fCache;				// +08
	PSSId					fInfoId;				// +0C
	IndexInfo			fInfo;				// +10
	int					f_BTStatus;			// +2C	insert/remove status
	short					fFixedKeySize;		// +30
	short					fFixedDataSize;	// +34
	KeyCompareProcPtr	fCompareKeyFn;
	KeyCompareProcPtr	fCompareDataFn;
	const CSortingTable *	fSortingTable;
};

inline 	CStore *		 CSoupIndex::store(void)  { return fStoreWrapper->store(); }
inline	CNodeCache * CSoupIndex::nodeCache(void)  { return fCache; }
inline	PSSId			 CSoupIndex::indexId(void) const  { return fInfoId; }


/*----------------------------------------------------------------------
	C U n i o n S o u p I n d e x
----------------------------------------------------------------------*/

struct UnionIndexData
{
				UnionIndexData();
				~UnionIndexData();

	CSoupIndex *	index;			// +00
	int				validity;		// +04
	IndexState		state;			// +08
	KeyField			keyField;		// +18
	KeyField *		kf;				// +7C
	int				cacheState;		// +80
};

// validity
enum { kIsInvalid, kIsValid, kIsBad };


class CUnionSoupIndex : public CAbstractSoupIndex
{
public:
					CUnionSoupIndex(ArrayIndex inNumOfIndexes, UnionIndexData * indexes);
	virtual		~CUnionSoupIndex();

	virtual int	find(SKey * inKey, SKey * outKey, SKey * outData, bool inArg4);
	virtual int	first(SKey * outKey, SKey * outData);
	virtual int	last(SKey * outKey, SKey * outData);
	virtual int	next(SKey * inKey, SKey * inData, int, SKey * outKey, SKey * outData);
	virtual int	prior(SKey * inKey, SKey * inData, int, SKey * outKey, SKey * outData);

	int			search(bool inDoForward, SKey * inKey, SKey * inData, StopProcPtr inStopProc, void * ioParms, SKey * outKey, SKey * outData, int);
	int			currentSoupGone(SKey*, SKey*, SKey*);
	int			moveToNextSoup(bool, int, SKey*, bool);
	void			setCurrentSoup(int);
	void			invalidateState(void);
	bool			isValidState(SKey*, SKey*);
	void			commit(void);

	ArrayIndex		i(void) const;
	CSoupIndex *	index(void) const;
	CSoupIndex *	index(ArrayIndex inSeq) const;

private:
	size_t			fNumOfSoupsInUnion;	// +04
	UnionIndexData *	fIndexData;			// +08
	ArrayIndex		fSeqInUnion;			// +0C
	bool				fIsForwardSearch;		// +10
};

inline ArrayIndex		CUnionSoupIndex::i(void) const  { return fSeqInUnion; }
inline CSoupIndex *	CUnionSoupIndex::index(void) const	{ return index(fSeqInUnion); }
inline CSoupIndex *	CUnionSoupIndex::index(ArrayIndex inSeq) const	{ return fIndexData[inSeq].index; }


/*----------------------------------------------------------------------
	F u n c t i o n   D e c l a r a t i o n s
----------------------------------------------------------------------*/

// Tags
int	AddTag(RefArg, RefArg);
ArrayIndex	CountTags(RefArg);
Ref	EncodeQueryTags(RefArg indexDesc, RefArg inQuerySpec);
bool	TagsValidTest(CSoupIndex & index, RefArg inTags, PSSId inTagsId);


// Indexes
Ref	GetTagsIndexDesc(RefArg inSpec);
void	AlterTagsIndex(bool inAdd, CSoupIndex & ioSoupIndex, PSSId inId, RefArg inKey, RefArg inSoup, RefArg inTags);
bool	UpdateTagsIndex(RefArg inSoup, RefArg indexSpec, RefArg, RefArg, PSSId inId);
void	AlterIndexes(bool inAdd, RefArg inSoup, RefArg inArg3, PSSId inId);
bool	IndexPathsEqual(RefArg inPath1, RefArg inPath2);
Ref	IndexPathToIndexDesc(RefArg inSoup, RefArg inPath, int * outIndex);
bool	UpdateIndexes(RefArg inSoup, RefArg inArg2, RefArg inArg3, PSSId inId, bool * outArg5);
bool	CompareSoupIndexes(RefArg inArg1, RefArg inArg2);
int	CopyIndexStopFn(SKey *, SKey *, void *);
void	CopySoupIndexes(RefArg, RefArg, PSSIdMapping *, ArrayIndex, RefArg, ULong);
void	AbortSoupIndexes(RefArg inSoup);
Ref	NewIndexDesc(RefArg, RefArg, RefArg);
Ref	AddNewSoupIndexes(RefArg inSoup, RefArg inStore, RefArg indexes);
void	IndexDescToIndexInfo(RefArg indexSpec, IndexInfo * outInfo);
void	IndexEntries(RefArg inSoup, RefArg indexSpec);
void	CreateSoupIndexObjects(RefArg inSoup);
void	GCDeleteIndexObjects(void *);

CSoupIndex *	GetSoupIndexObject(RefArg, PSSId);


#endif	/* __INDEXES_H */
