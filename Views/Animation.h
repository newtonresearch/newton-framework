/*
	File:		Animation.h

	Contains:	Animation declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__ANIMATION_H)
#define __ANIMATION_H 1


enum EffectKind
{
	kPlainEffect,
	kTrashEffect,
	kSlideEffect,
	kPoofEffect
};


class CAnimate
{
public:
				CAnimate();
				~CAnimate();

	void		doEffect(RefArg inSoundEffect);
	void		multiEffect(RefArg inSoundEffect);
	void		poofEffect(void);
	void		crumpleEffect(void);
	void		crumpleSprite(Rect *, Rect *);

	void		setupPlainEffect(CView * inView, bool, long);
	void		setupTrashEffect(CView * inView);
	void		setupSlideEffect(CView * inView, const Rect * inBounds, long inHorizontal, long inVertical);
	void		setupPoofEffect(CView * inView, const Rect * inBounds);
	void		setupDragEffect(CView * inView);

	void		preSetup(CView * inView, EffectKind inEffect);
	void		postSetup(const Rect * inBnd1, const Rect * inBnd2, const Rect * inBnd3);

private:
//	CBits					fBits;			// +00
//	CSaveScreenBits	fSaveScreen;	// +34
//	CRegionStruct		f60;
	Rect					fxBox;			// +64
	Rect					f6C;
	Rect					f74;
	Rect					f7C;				// for slide effect
	Point					f84;
	CView *				fView;			// +88
	EffectKind			fEffect;			// +8C
	ULong					f90;
	ULong					f94;
	ULong					f98;
	long					fxDef;			// +9C
	RefStruct *			fContext;		// +A0
	bool					fA4;
	bool					fA5;
	ULong					fMask;			// +A8
	ExceptionCleanup	fException;		// +AC
//	size +BC
};


#endif	/* __ANIMATION_H */
