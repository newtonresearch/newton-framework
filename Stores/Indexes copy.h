/*
	File:		Indexes.h

	Contains:	Soup index declarations.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__INDEXES_H)
#define __INDEXES_H 1

/*----------------------------------------------------------------------
	S K e y
	An SKey instance is a data item.
----------------------------------------------------------------------*/

struct SKey
{
			SKey();

	// setters
	void	set(size_t inKeySize, void * inKey);
	void	setFlags(unsigned char inFlags);
	void	setMissingKey(int inBit);
	void	setSize(unsigned short inSize);
	// getters
	size_t	size(void) const;
	ULong		flags(void) const;
	const void *	data(void) const;

	BOOL	equals(const SKey & o) const;

	SKey&	operator=(const SKey & inKey);
	SKey&	operator=(long inKey);
	SKey&	operator=(UniChar inKey);
	SKey&	operator=(const double inKey);

	operator long() const;
	operator UniChar() const;
	operator double() const;

	enum { kOffsetToData = sizeof(unsigned short) };

protected:
	unsigned short	flagBits:8;		// 0x80 => is multislot?
	unsigned short	length:8;
	char	buf[78];						// actually variable length, but this is what you get if you new SKey
};

inline SKey::SKey()
	: flagBits(0), length(0)
{ }

inline size_t	SKey::size(void) const
{ return length; }

inline ULong	SKey::flags(void) const
{ return flagBits; }

inline const void *	SKey::data(void) const
{ return buf; }


struct STagsBits : public SKey
{
	void	setTag(unsigned short index);
};

// Function interface
Ref	GetEntryKey(RefArg inEntry, RefArg inPath);
BOOL	GetEntrySKey(RefArg inEntry, RefArg inQuerySpec, SKey * outKey, BOOL * outIsComplexKey);
void	MultiKeyToSKey(RefArg inKey, RefArg inType, SKey * outKey);
void	RichStringToSKey(RefArg inStr, SKey * outKey);
void	KeyToSKey(RefArg inKey, RefArg inType, SKey * outKey, short * outSize, BOOL * outIsComplexKey);
Ref	SKeyToKey(const SKey& inKey, RefArg inType, short * outSize);


/*----------------------------------------------------------------------
	K e y F i e l d
	A KeyField holds a list of SKeys.
	The first SKey is yer actual key, followed by 0..n data values.
	Since keys and data are preceded by their (short) length, they must always be short aligned.
----------------------------------------------------------------------*/

struct KeyField
{
	enum { kOffsetToData = sizeof(unsigned short) };

// should really supply accessors and make these private
// or just make CSoupIndex a friend?
	unsigned short flags:2;
	unsigned short	length:14;
	char	buf[300];	// don’t really know
};

/*
	Dup nodes are suffixed w/ PACKED extra data
*/
struct DupInfo
{
	short	count;
	ULong	id;
};


/*----------------------------------------------------------------------
	I n d e x I n f o
----------------------------------------------------------------------*/

struct IndexInfo
{
	PSSId		x00;	// id of index block
	size_t	x04;	// size (starts at 512)
	ULong		x08;	// type string=0, int=1, char=2, real=3, symbol=4, multislot=6
	ULong		x0C;	// class uninitialized=0, normal=1, tags=5
	ULong		x10;	// origin system=0, user=2
	ULong		x14;	// multitypes
	char		x18;	// sort
	char		x19;	// sorting
	short		x1A;
};

struct IndexState;

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

	virtual int	find(SKey*, SKey*, SKey*, BOOL) = 0;
	virtual int	first(SKey*, SKey*) = 0;
	virtual int	last(SKey*, SKey*) = 0;
	virtual int	next(SKey*, SKey*, int, SKey*, SKey*) = 0;
	virtual int	prior(SKey*, SKey*, int, SKey*, SKey*) = 0;

	int	findPrior(SKey*, SKey*, SKey*, BOOL, BOOL);
};


/*----------------------------------------------------------------------
	C S o u p I n d e x
----------------------------------------------------------------------*/
#include "NodeCache.h"
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
	void			setNodeNo(NodeHeader * inNode, long index, ULong inNodeNum);
	NodeHeader *	readRootNode(BOOL inCreate);
	NodeHeader *	readANode(PSSId inId, ULong);
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
	BOOL		kfNextDataAddr(KeyField * inField, void * inData, void ** outNextData);
	void *	kfLastDataAddr(KeyField * inField);
	size_t	kfDupCount(KeyField * inField);
	void		kfSetDupCount(KeyField * inField, short inCount);
	ULong		kfNextDupId(KeyField * inField);
	void		kfSetNextDupId(KeyField * inField, PSSId inId);
	void		kfInsertData(KeyField * ioField, void * inLocation, void * inData);
	void		kfDeleteData(KeyField * ioField, void * inData);
	void		kfReplaceFirstData(KeyField * ioField, void * inData);
	void *	kfFindDataAddr(KeyField * inField, void * inData, void ** outData);
	void		kfConvertKeyField(long, KeyField * inField);

	void *	firstDupDataAddr(DupNodeHeader * inDupNode);
	BOOL		nextDupDataAddr(DupNodeHeader * inDupNode, void * inData, void**);
	void *	lastDupDataAddr(KeyField*, DupNodeHeader ** ioDupNode);
	BOOL		prependDupData(DupNodeHeader * inDupNode, void * inData);
	BOOL		appendDupData(DupNodeHeader * inDupNode, void * inData);
	BOOL		deleteDupData(DupNodeHeader * inDupNode, void * inData);
	void *	findDupDataAddr(DupNodeHeader * inDupNode, void * inData, void ** outData);
	void *	findNextDupDataAddr(DupNodeHeader ** ioDupNode, void * inData, BOOL*);
	void *	findPriorDupDataAddr(DupNodeHeader ** ioDupNode, void * inData, BOOL*);

	KeyField *	keyFieldAddr(NodeHeader * inNode, long index);
	ULong			leftNodeNo(NodeHeader * inNode, long index);
	ULong			rightNodeNo(NodeHeader * inNode, long index);
	ULong			firstNodeNo(NodeHeader * inNode);
	ULong			lastNodeNo(NodeHeader * inNode);
	KeyField *	firstKeyField(NodeHeader * inNode);
	KeyField *	lastKeyField(NodeHeader * inNode);
	char *		keyFieldBase(NodeHeader * inNode);

	void		createNewRoot(KeyField * inField, unsigned long);
	BOOL		insertKey(KeyField*, NodeHeader * inNode, unsigned long*, BOOL*);
	void		insertAfterDelete(KeyField*, unsigned long, NodeHeader * inNode);
	BOOL		deleteKey(KeyField*, NodeHeader * inNode, BOOL*);
	BOOL		moveKey(KeyField * inFromField, KeyField * outToField);
	void		copyKeyFmNode(KeyField * outField, ULong * outNo, NodeHeader * inNode, long inSlot);
	KeyField *	keyBeforeNodeNo(NodeHeader * inNode, ULong inNo, long * outSlot);
	KeyField *	keyAfterNodeNo(NodeHeader * inNode, ULong inNo, long * outSlot);
	int		lastSlotInNode(NodeHeader * inNode);
	size_t	bytesInNode(NodeHeader * inNode);
	BOOL		roomInNode(NodeHeader * inNode, KeyField * inField);
	BOOL		nodeUnderflow(NodeHeader * inNode);
	BOOL		keyInNode(KeyField*, NodeHeader * inNode, unsigned long*, long*);
	void		putKeyIntoNode(KeyField*, unsigned long, NodeHeader * inNode, long);
	void		deleteKeyFromNode(NodeHeader * inNode, long);

	BOOL		findFirstKey(NodeHeader * inNode, KeyField*);
	BOOL		findLastKey(NodeHeader * inNode, KeyField*);
	BOOL		balanceTwoNodes(NodeHeader * inNode1, NodeHeader * inNode2, long);
	void		mergeTwoNodes(KeyField * inField, NodeHeader*, NodeHeader*, NodeHeader*);
	void		splitANode(KeyField*, unsigned long*, NodeHeader * inNode, long);
	void		deleteTheKey(NodeHeader * inNode, long, KeyField*);
	BOOL		getLeafKey(KeyField*, NodeHeader * inNode);

	void		checkForDupData(KeyField*, void*);
	void		storeDupData(KeyField*, void*);
	void		insertDupData(KeyField*, NodeHeader*, long, ULong*, BOOL*);

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

	BOOL		search(KeyField * inKey, NodeHeader ** ioNode, long * ioSlot);
	int		searchNext(KeyField * inKey, NodeHeader ** ioNode, long * ioSlot);
	int		searchPrior(KeyField * inKey, NodeHeader ** ioNode, long * ioSlot);
	int		searchNextDup(KeyField * inKey, NodeHeader ** ioNode, long * ioSlot, DupNodeHeader**);
	int		searchPriorDup(KeyField * inKey, NodeHeader ** ioNode, long * ioSlot, DupNodeHeader**);

	int		findNextKey(KeyField * inKey, NodeHeader ** ioNode, long * ioSlot);
	int		findPriorKey(KeyField * inKey, NodeHeader ** ioNode, long * ioSlot);
	void		findAndGetState(KeyField*, IndexState*);
	void		findLastAndGetState(KeyField*, IndexState*);
	void		findPriorAndGetState(KeyField*, unsigned char, IndexState*);
	void		moveAndGetState(unsigned char, int, KeyField*, IndexState*);
	void		moveUsingState(unsigned char, int, KeyField*, IndexState*);

	NewtonErr	add(SKey * inKey, SKey * inData);
	NewtonErr	addInTransaction(SKey * inKey, SKey * inData);
	int			search(int, SKey*, SKey*, StopProcPtr, void*, SKey*, SKey*);
	NewtonErr	Delete(SKey * inKey, SKey * inData);

	virtual int	find(SKey*, SKey*, SKey*, BOOL);
	virtual int	first(SKey*, SKey*);
	virtual int	last(SKey*, SKey*);
	virtual int	next(SKey*, SKey*, int, SKey*, SKey*);
	virtual int	prior(SKey*, SKey*, int, SKey*, SKey*);

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

	CStoreWrapper *	f04;
	CNodeCache *		f08;
	PSSId					f0C;	// rootNodeId
	IndexInfo			f10;
	int					f2C;
	short					f30;
	short					f34;
	KeyCompareProcPtr	f38;
	KeyCompareProcPtr	f3C;
	const CSortingTable *	f40;
};

inline 	CStore *	CSoupIndex::store(void)
{ return f04->store(); }

inline	CNodeCache * CSoupIndex::nodeCache(void)
{ return f08; }

inline	PSSId		CSoupIndex::indexId(void) const
{ return f0C; }


/*----------------------------------------------------------------------
	C U n i o n S o u p I n d e x
----------------------------------------------------------------------*/

struct UnionIndexData
{
				UnionIndexData();
				~UnionIndexData();

	CSoupIndex *	fIndex;			// +00
	long				f04;
	KeyField			f18;
	KeyField *		f7C;
	long *			f80;
};


class CUnionSoupIndex : public CAbstractSoupIndex
{
public:
					CUnionSoupIndex(unsigned inNumOfIndexes, UnionIndexData * indexes);
	virtual		~CUnionSoupIndex();

	virtual int	find(SKey * inKey, SKey * outKey, SKey * outData, BOOL inArg4);
	virtual int	first(SKey * outKey, SKey * outData);
	virtual int	last(SKey * outKey, SKey * outData);
	virtual int	next(SKey*, SKey*, int, SKey*, SKey*);
	virtual int	prior(SKey*, SKey*, int, SKey*, SKey*);

	int			search(BOOL, SKey*, SKey*, StopProcPtr, void*, SKey*, SKey*, int);
	int			currentSoupGone(SKey*, SKey*, SKey*);
	int			moveToNextSoup(BOOL, int, SKey*, BOOL);
	void			setCurrentSoup(long);
	void			invalidateState(void);
	BOOL			isValidState(SKey*, SKey*);
	void			commit(void);

	int			i(void) const;
	CSoupIndex *	index(void) const;
	CSoupIndex *	index(int inSeq) const;

private:
	long				f00;
	size_t			fNumOfSoupsInUnion;	// +04
	UnionIndexData *	fIndexData;			// +08
	int				fSeqInUnion;			// +0C
	long				f10;
};

inline int  CUnionSoupIndex::i(void) const  { return fSeqInUnion; }
inline CSoupIndex *	CUnionSoupIndex::index(void) const	{ return index(fSeqInUnion); }
inline CSoupIndex *	CUnionSoupIndex::index(int inSeq) const	{ return fIndexData[inSeq].fIndex; }


/*----------------------------------------------------------------------
	F u n c t i o n   D e c l a r a t i o n s
----------------------------------------------------------------------*/

// Tags
int	AddTag(RefArg, RefArg);
int	CountTags(RefArg);
Ref	EncodeQueryTags(RefArg indexDesc, RefArg inQuerySpec);
BOOL	TagsValidTest(CSoupIndex & index, RefArg inArg2, unsigned long inArg3);


// Indexes
Ref	GetTagsIndexDesc(RefArg inSpec);
void	AlterTagsIndex(unsigned char inSelector, CSoupIndex & ioSoupIndex, PSSId inId, RefArg inKey, RefArg inSoup, RefArg inTags);
BOOL	UpdateTagsIndex(RefArg inSoup, RefArg indexSpec, RefArg, RefArg, PSSId inId);
void	AlterIndexes(unsigned char inSelector, RefArg inSoup, RefArg inArg3, PSSId inId);
BOOL	IndexPathsEqual(RefArg inPath1, RefArg inPath2);
Ref	IndexPathToIndexDesc(RefArg inSoup, RefArg inPath, int * outIndex);
BOOL	UpdateIndexes(RefArg inSoup, RefArg inArg2, RefArg inArg3, PSSId inId, BOOL * outArg5);
BOOL	CompareSoupIndexes(RefArg inArg1, RefArg inArg2);
int	CopyIndexStopFn(SKey *, SKey *, void *);
void	CopySoupIndexes(RefArg, RefArg, PSSIdMapping *, long, RefArg, long);
void	AbortSoupIndexes(RefArg inSoup);
Ref	NewIndexDesc(RefArg, RefArg, RefArg);
Ref	AddNewSoupIndexes(RefArg inSoup, RefArg inStore, RefArg indexes);
void	IndexDescToIndexInfo(RefArg indexSpec, IndexInfo * outInfo);
void	IndexEntries(RefArg inSoup, RefArg indexSpec);
void	CreateSoupIndexObjects(RefArg inSoup);
void	GCDeleteIndexObjects(void *);

CSoupIndex *	GetSoupIndexObject(RefArg, unsigned long);


#endif	/* __INDEXES_H */
