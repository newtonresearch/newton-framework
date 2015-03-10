/*
	File:		Compiler-AST.h

	Contains:	The NewtonScript compiler.
					Parsing (AST generation) based on ideas from LLVM
					and not using yacc/flex tools as the original did.

	Written by:	The Newton Tools group.
*/

#if !defined(__AST_H)
#define __AST_H 1

#include <string>
#include <vector>


struct Token
{
	int id;		// defined in Tokens.h
	struct
	{
		int type;
		RefStruct ref;
/*		// can’t put values in a union b/c std::string has non-trivial copy ctor
		long integer;
		double real;
		std::string symbol;
		std::u16string string;
*/	} value;
	struct
	{
		int lineNumber;
		int colmNumber;
	} location;

	int precedence(void);
};


enum
{
	TYPEnone, TYPEboolean, TYPEcharacter, TYPEinteger, TYPEreal, TYPEref, TYPEsymbol, TYPEstring
};


struct Value
{
};

struct Function
{
};


/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() {}
  virtual Value *Codegen() = 0;
  virtual void dump() = 0;
};


typedef std::vector<ExprAST*> ExprList;
typedef std::vector<std::pair<std::string, ExprAST*> > TaggedExprList;
//typedef std::pair<std::string, ExprAST*> TaggedExpr;
//typedef std::vector<TaggedExpr> TaggedExprList;

class TaggedExpr : public ExprAST {
	std::string tag;
	ExprAST * expr;
public:
  TaggedExpr(const std::string &inTag, ExprAST *inExpr) : tag(inTag), expr(inExpr) {}
  virtual Value *Codegen();
  virtual void dump();
};



class ExprSeqAST : public ExprAST {
  ExprList seq;
public:
  ExprSeqAST(const ExprList &inSeq) : seq(inSeq) {}
  virtual Value *Codegen();
  virtual void dump();
};

class SymbolAST : public ExprAST {
  std::string name;
public:
  SymbolAST(const std::string &inName) : name(inName) {}
  virtual Value *Codegen();
  virtual void dump();
};

class GlobalFuncAST : public ExprAST {
  std::string name;
 std::vector<std::pair<int,std::string> > args;
  ExprAST * body;
public:
  GlobalFuncAST(std::string inName, const std::vector<std::pair<int,std::string>  > &inArgs, ExprAST * inBody)
    : name(inName), args(inArgs), body(inBody) {}
  virtual Value *Codegen();
  virtual void dump();
};


class GlobalDeclAST : public ExprAST
{
  std::string name;
  ExprAST * value;
public:
	GlobalDeclAST(std::string inSymbol, ExprAST * inDecl) : name(inSymbol), value(inDecl) {}
  virtual Value *Codegen();
  virtual void dump();
};

class ConstantDeclAST : public ExprAST
{
	TaggedExprList decls;
public:
	ConstantDeclAST(TaggedExprList &inDecls) : decls(inDecls) {}
  virtual Value *Codegen();
  virtual void dump();
};

class LocalDeclAST : public ExprAST
{
	int type;
	TaggedExprList decls;
public:
	LocalDeclAST(int inType, TaggedExprList &inDecls) : type(inType), decls(inDecls) {}
  virtual Value *Codegen();
  virtual void dump();
};

class ConstExprAST : public ExprAST {
  Token value;
public:
  ConstExprAST(Token &inToken) : value(inToken) {}
  virtual Value *Codegen();
  virtual void dump();
};

class PathExprAST : public ExprAST {
  std::vector<std::string> elements;
public:
  PathExprAST(const std::vector<std::string> &inElements) : elements(inElements) {}
  virtual Value *Codegen();
  virtual void dump();
};

class ArrayAST : public ExprAST {
  std::string className;
  ExprList elements;
public:
  ArrayAST(std::string inClass, ExprList &inElements) : className(inClass), elements(inElements) {}
  virtual Value *Codegen();
  virtual void dump();
};

class FrameAST : public ExprAST {
  TaggedExprList elements;
public:
  FrameAST(TaggedExprList &inElements) : elements(inElements) {}
  virtual Value *Codegen();
  virtual void dump();
};

class FuncExprAST : public ExprAST {
  bool isNative;
  std::vector<std::pair<int,std::string> > args;
  ExprAST * body;
public:
  FuncExprAST(bool inNative, const std::vector<std::pair<int,std::string> > &inArgs, ExprAST * inBody)
    : isNative(inNative), args(inArgs), body(inBody) {}
  virtual Value *Codegen();
  virtual void dump();
};

class FrameAccessExprAST : public ExprAST {
  ExprAST * frame;
  ExprAST * accessor;
public:
  FrameAccessExprAST(ExprAST * inFrame, ExprAST * inAccessor) : frame(inFrame), accessor(inAccessor) {}
  virtual Value *Codegen();
  virtual void dump();
};

class ArrayAccessExprAST : public ExprAST {
  ExprAST * array;
  ExprAST * accessor;
public:
  ArrayAccessExprAST(ExprAST * inArray, ExprAST * inAccessor) : array(inArray), accessor(inAccessor) {}
  virtual Value *Codegen();
  virtual void dump();
};

class FuncAccessExprAST : public ExprAST {
  ExprAST * receiver;
  std::string message;
public:
  FuncAccessExprAST(ExprAST * inReceiver, const std::string &inMessage) : receiver(inReceiver), message(inMessage) {}
  virtual Value *Codegen();
  virtual void dump();
};

class MessageSendExprAST : public ExprAST {
  ExprAST * receiver;
  std::string message;
  ExprList args;
public:
  MessageSendExprAST(ExprAST * inReceiver, const std::string &inMessage, ExprList &inArgs) : receiver(inReceiver), message(inMessage), args(inArgs) {}
  virtual Value *Codegen();
  virtual void dump();
};

class ReceiverExprAST : public ExprAST {
  int type;
public:
  ReceiverExprAST(int inType) : type(inType) {}
  virtual Value *Codegen();
  virtual void dump();
};

enum
{
	kReceiverSelf,
	kReceiverInherited
};

class ExistsExprAST : public ExprAST {
  ExprAST * operand;
public:
  ExistsExprAST(ExprAST * inOperand) : operand(inOperand) {}
  virtual Value *Codegen();
  virtual void dump();
};


class AssignExprAST : public ExprAST {
  ExprAST *lhs, *rhs;
public:
  AssignExprAST(ExprAST * inLHS, ExprAST * inRHS) : lhs(inLHS), rhs(inRHS) {}
  virtual Value *Codegen();
  virtual void dump();
};




/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
  double Val;
public:
  NumberExprAST(double val) : Val(val) {}
  virtual Value *Codegen();
  virtual void dump();
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  std::string Name;
public:
  VariableExprAST(const std::string &name) : Name(name) {}
  virtual Value *Codegen();
  virtual void dump();
};

class LiteralExprAST : public ExprAST {
  ExprAST *literal;
public:
  LiteralExprAST(ExprAST * inValue) : literal(inValue) {}
  virtual Value *Codegen();
  virtual void dump();
};

/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST {
  int op;
  ExprAST * operand;
public:
  UnaryExprAST(int inOp, ExprAST * inOperand) : op(inOp), operand(inOperand) {}
  virtual Value *Codegen();
  virtual void dump();
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  int op;
  ExprAST * lhs, * rhs;
public:
  BinaryExprAST(int inOp, ExprAST * inLHS, ExprAST * inRHS) : op(inOp), lhs(inLHS), rhs(inRHS) {}
  virtual Value *Codegen();
  virtual void dump();
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
  ExprAST* callee;
  ExprList args;
public:
  CallExprAST(ExprAST * inFunc, ExprList & inArgs) : callee(inFunc), args(inArgs) {}
  virtual Value *Codegen();
  virtual void dump();
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
  ExprAST *cond, *then, *els;
public:
  IfExprAST(ExprAST * inCond, ExprAST * inThen, ExprAST *inElse) : cond(inCond), then(inThen), els(inElse) {}
  virtual Value *Codegen();
  virtual void dump();
};

class LoopExprAST : public ExprAST {
  ExprAST *body;
public:
  LoopExprAST(ExprAST * inBody) : body(inBody) {}
  virtual Value *Codegen();
  virtual void dump();
};

/// ForExprAST - Expression class for for/in.
class ForExprAST : public ExprAST {
  std::string iterName;
  ExprAST * start, * stop, * step, *body;
public:
  ForExprAST(const std::string &inItername, ExprAST * inStart, ExprAST * inStop, ExprAST * inStep, ExprAST * inBody)
    : iterName(inItername), start(inStart), stop(inStop), step(inStep), body(inBody) {}
  virtual Value *Codegen();
  virtual void dump();
};

class ForeachExprAST : public ExprAST {
  std::string varName;
  std::string indexName;
  bool isDeeply, doCollect;
  ExprAST * iter, *body;
public:
  ForeachExprAST(const std::string &inVarName, const std::string &inIndexName, bool inDeeply, ExprAST * inIter, bool inCollect, ExprAST * inBody)
    : varName(inVarName), indexName(inIndexName), isDeeply(inDeeply), iter(inIter), doCollect(inCollect), body(inBody) {}
  virtual Value *Codegen();
  virtual void dump();
};

class WhileExprAST : public ExprAST {
  ExprAST *cond, *body;
public:
  WhileExprAST(ExprAST * inCond, ExprAST * inBody) : cond(inCond), body(inBody) {}
  virtual Value *Codegen();
  virtual void dump();
};

class RepeatExprAST : public ExprAST {
  ExprAST *cond, *body;
public:
  RepeatExprAST(ExprAST * inCond, ExprAST * inBody) : cond(inCond), body(inBody) {}
  virtual Value *Codegen();
  virtual void dump();
};

class BreakExprAST : public ExprAST {
  ExprAST *value;
public:
  BreakExprAST(ExprAST * inValue) : value(inValue) {}
  virtual Value *Codegen();
  virtual void dump();
};

class ReturnExprAST : public ExprAST {
  ExprAST *value;
public:
  ReturnExprAST(ExprAST * inValue) : value(inValue) {}
  virtual Value *Codegen();
  virtual void dump();
};

class TryExprAST : public ExprAST {
  ExprAST *body;
  TaggedExprList handlers;
public:
  TryExprAST(ExprAST * inBody, const TaggedExprList & inHandlers) : body(inBody), handlers(inHandlers) {}
  virtual Value *Codegen();
  virtual void dump();
};




/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;
public:
  PrototypeAST(const std::string &name, const std::vector<std::string> &args)
    : Name(name), Args(args) {}
  
  Function *Codegen();
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
  PrototypeAST *Proto;
  ExprAST *Body;
public:
  FunctionAST(PrototypeAST *proto, ExprAST *body)
    : Proto(proto), Body(body) {}
  
  Function *Codegen();
};


#endif	/* __AST_H */
