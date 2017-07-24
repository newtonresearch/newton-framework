// ---------------------------------------------------------------------------
// Somewhat experimental fakeframes (maybe this should be done with a
// preprocessor other than the C++ preprocessor).
//
// To declare:
//
// 	DefineFrame3(CFunction, funcPtr, numArgs, docString);
//
// To initialize:
//
// 	SetUpBuiltinMap3(CFunction, SYMA(funcptr), SYMA(numArgs), SYMA(docString));
//
// And somewhere you have to define this:
//
// 	Ref CFunction::fgPrototype;
//
// ---------------------------------------------------------------------------

extern Ref gClassMap;
Ref		AllocateMapWithTags(RefArg superMap, RefArg tags);


#define SUBMHeader(numTags)											\
	do {																		\
		RefVar tags_ = AllocateArray(NILREF, numTags + 1);		\
		SetArraySlot(tags_, 0, SYMA(class));

#define SUBMTag(index, tag)											\
		SetArraySlot(tags_, index + 1, tag);

#define SUBMFooter(type)												\
		RefVar map = AllocateMapWithTags(NILREF, tags_);		\
		type##::fgPrototype = AllocateFrameWithMap(map);		\
		AddGCRoot(type##::fgPrototype);								\
		SetArraySlot(type##::fgPrototype, 0, Intern(#type));	\
	} while (0);

#define SetUpBuiltinMap1(type, s1)								\
	SUBMHeader(1)														\
	SUBMTag(0, s1)														\
	SUBMFooter(type)

#define SetUpBuiltinMap2(type, s1, s2)							\
	SUBMHeader(2)														\
	SUBMTag(0, s1)														\
	SUBMTag(1, s2)														\
	SUBMFooter(type)

#define SetUpBuiltinMap3(type, s1, s2, s3)					\
	SUBMHeader(3)														\
	SUBMTag(0, s1)														\
	SUBMTag(1, s2)														\
	SUBMTag(2, s3)														\
	SUBMFooter(type)

#define SetUpBuiltinMap4(type, s1, s2, s3, s4)				\
	SUBMHeader(4)														\
	SUBMTag(0, s1)														\
	SUBMTag(1, s2)														\
	SUBMTag(2, s3)														\
	SUBMTag(3, s4)														\
	SUBMFooter(type)

#define SetUpBuiltinMap5(type, s1, s2, s3, s4, s5)			\
	SUBMHeader(5)														\
	SUBMTag(0, s1)														\
	SUBMTag(1, s2)														\
	SUBMTag(2, s3)														\
	SUBMTag(3, s4)														\
	SUBMTag(4, s5)														\
	SUBMFooter(type)

#define SetUpBuiltinMap6(type, s1, s2, s3, s4, s5, s6)	\
	SUBMHeader(6)														\
	SUBMTag(0, s1)														\
	SUBMTag(1, s2)														\
	SUBMTag(2, s3)														\
	SUBMTag(3, s4)														\
	SUBMTag(4, s5)														\
	SUBMTag(5, s6)														\
	SUBMFooter(type)

#define DefinePtrClass(type)														\
	class type##Ptr : public TFramesObjectPtr {								\
	  public:																			\
		type##Ptr()	{ }																\
		type##Ptr(Ref r) : TFramesObjectPtr(r)	{ }							\
		type##Ptr(const RefStruct& r) : TFramesObjectPtr(r)	{ }		\
		type##Ptr(const RefVar& r) : TFramesObjectPtr(r)	{ }			\
		~type##Ptr() { }																\
		type * operator ->() const { return (type *)(char*) *this; }	\
	}

#define FrameStructHeader(type)										\
	struct type {															\
		static Ref fgPrototype;											\
		static Ref Allocate() { return Clone(fgPrototype); }	\
		ObjHeader	header;												\
		Ref			map;													\
		Ref			classSlot;											\
		Ref

#define DefineFrame1(type, s1)										\
	extern Ref g##type##Map;											\
	struct type;															\
	DefinePtrClass(type);												\
	FrameStructHeader(type) s1;										\
	enum { s_class, s_##s1 };											\
	}

#define DefineFrame2(type, s1, s2)									\
	extern Ref g##type##Map;											\
	struct type;															\
	DefinePtrClass(type);												\
	FrameStructHeader(type) s1, s2;									\
	enum { s_class, s_##s1, s_##s2 };								\
	}

#define DefineFrame3(type, s1, s2, s3)								\
	extern Ref g##type##Map;											\
	struct type;															\
	DefinePtrClass(type);												\
	FrameStructHeader(type) s1, s2, s3;								\
	enum { s_class, s_##s1, s_##s2, s_##s3 };						\
	}

#define DefineFrame4(type, s1, s2, s3, s4)						\
	extern Ref g##type##Map;											\
	struct type;															\
	DefinePtrClass(type);												\
	FrameStructHeader(type) s1, s2, s3, s4;						\
	enum { s_class, s_##s1, s_##s2, s_##s3, s_##s4 };			\
	}

#define DefineFrame5(type, s1, s2, s3, s4, s5)					\
	extern Ref g##type##Map;											\
	struct type;															\
	DefinePtrClass(type);												\
	FrameStructHeader(type) s1, s2, s3, s4, s5;					\
	enum { s_class, s_##s1, s_##s2, s_##s3, s_##s4, s_##s5 };\
	}

#define DefineFrame6(type, s1, s2, s3, s4, s5, s6)				\
	extern Ref g##type##Map;											\
	struct type;															\
	DefinePtrClass(type);												\
	FrameStructHeader(type) s1, s2, s3, s4, s5, s6;				\
	enum { s_class, s_##s1, s_##s2, s_##s3, s_##s4, s_##s5, s_##s6 };\
	}
