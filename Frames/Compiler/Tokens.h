/*
	File:		Tokens.h

	Contains:	NewtonScript lexical tokens.

	Written by:	Newton Research Group.
*/

#if !defined(__TOKENS_H)
#define __TOKENS_H 1

// literals
#define TOKENconst 258
#define TOKENsymbol 259
#define TOKENinteger 260
#define TOKENreal 261
#define TOKENrefConst 262
#define TOKENref 910		// FOR DEBUG
#define TOKENbad	911

//assignment
#define TOKENassign 297
// string operator
#define TOKENAmperAmper 306
// arithmetic operators
#define TOKENdiv 307
#define TOKENmod 308
#define TOKENLShift 309
#define TOKENRShift 310
// relational operators
#define TOKENEQL 303
#define TOKENNEQ 304
#define TOKENLEQ 301
#define TOKENGEQ 302
// boolean operators
#define TOKENand 298
#define TOKENor 299
// unary operators
#define TOKENuMinus 311
#define TOKENnot 300
// message send
#define TOKENsendIfDefined 312

// reserved words
#define TOKENbegin 263
#define TOKENend 264
#define TOKENfunc 265
#define TOKENnative 266
#define TOKENglobal 267
#define TOKENlocal 277
#define TOKENconstant 269
#define TOKENif 270
#define TOKENthen 271
#define TOKENelse 272
#define TOKENtry 273
#define TOKENonexception 274
#define TOKENloop 278
#define TOKENfor 279
#define TOKENto 280
#define TOKENby 281
#define TOKENwhile 282
#define TOKENdo 285
#define TOKENrepeat 283
#define TOKENuntil 284
#define TOKENcall 286
#define TOKENwith 287
#define TOKENinvoke 288
#define TOKENforeach 289
#define TOKENin 290
#define TOKENdeeply 291
#define TOKENself 292
#define TOKENinherited 293
#define TOKENreturn 294
#define TOKENbreak 295
#define TOKENexists 305

// these don’t look very lexical to me
#define TOKENOptionalExpr 257
#define TOKENgFunction 268
#define TOKENBuildArray 275
#define TOKENBuildFrame 276
#define TOKENKeepGoing 296

#endif	/* __TOKENS_H */
