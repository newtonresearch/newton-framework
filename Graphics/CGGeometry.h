/* CoreGraphics - CGGeometry.h
 * Copyright (c) 1998-2003 Apple Computer, Inc.
 * All rights reserved.
 *	Adapted for Newton.
 */

#ifndef CGGEOMETRY_H_
#define CGGEOMETRY_H_

#define CG_EXTERN extern
#define CG_INLINE inline

/* Points. */

struct CGPoint {
    float x;
    float y;
};
typedef struct CGPoint CGPoint;

/* Sizes. */

struct CGSize {
    float width;
    float height;
};
typedef struct CGSize CGSize;

/* Rectangles. */

struct CGRect {
    CGPoint origin;
    CGSize size;
};
typedef struct CGRect CGRect;


/* Make a point from `(x, y)'. */

CG_INLINE CGPoint CGPointMake(float x, float y);

/* Make a size from `(width, height)'. */

CG_INLINE CGSize CGSizeMake(float width, float height);

/* Make a rect from `(x, y; width, height)'. */

CG_INLINE CGRect CGRectMake(float x, float y, float width, float height);


/* Return the width of `rect'. */

CG_EXTERN float CGRectGetWidth(CGRect rect);

/* Return the height of `rect'. */

CG_EXTERN float CGRectGetHeight(CGRect rect);


/*** Definitions of inline functions. ***/

CG_INLINE CGPoint CGPointMake(float x, float y)
{
    CGPoint p; p.x = x; p.y = y; return p;
}

CG_INLINE CGSize CGSizeMake(float width, float height)
{
    CGSize size; size.width = width; size.height = height; return size;
}

CG_INLINE CGRect CGRectMake(float x, float y, float width, float height)
{
    CGRect rect;
    rect.origin.x = x; rect.origin.y = y;
    rect.size.width = width; rect.size.height = height;
    return rect;
}

#endif	/* CGGEOMETRY_H_ */
