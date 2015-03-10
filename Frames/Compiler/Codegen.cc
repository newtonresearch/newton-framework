/*
	File:		Codegen.cc

	Contains:	The NewtonScript compiler.
					Code generation based on ideas from LLVM
					and not using yacc/flex tools as the original did.

	Written by:	The Newton Tools group.
*/

#include "Objects.h"
#include "Compiler.h"
#include "Tokens.h"
#include "Compiler-AST.h"


Value *
ExprSeqAST::Codegen(void)
{ return NULL; }

void
ExprSeqAST::dump(void)
{
	for (std::vector<ExprAST*>::iterator iter = seq.begin(); iter != seq.end(); ++iter)
		{ (*iter)->dump(); }
}


Value *
SymbolAST::Codegen(void)
{ return NULL; }

void
SymbolAST::dump(void)
{
	printf("<symbol %s> ", name.c_str());
}


Value *
GlobalFuncAST::Codegen(void)
{ return NULL; }

void
GlobalFuncAST::dump(void)
{
	printf("<global func %s (", name.c_str());
	for (std::vector<std::pair<int,std::string> >::iterator iter = args.begin(); iter != args.end(); ++iter)
		{ printf("(%d)%s, ", (*iter).first, (*iter).second.c_str()); }
	printf(")\n");
	body->dump();
	printf(">\n");
}


Value *
TaggedExpr::Codegen(void)
{ return NULL; }

void
TaggedExpr::dump(void)
{
	printf("%s ::= ", tag.c_str());
	expr->dump();
}


Value *
GlobalDeclAST::Codegen(void)
{ return NULL; }

void
GlobalDeclAST::dump(void)
{
	printf("<global %s := ", name.c_str());
	value->dump();
	printf(">\n");
}



Value *
ConstantDeclAST::Codegen(void)
{ return NULL; }

void
ConstantDeclAST::dump(void)
{
	printf("<constant ");
	for (std::vector<std::pair<std::string, ExprAST*> >::iterator iter = decls.begin(); iter != decls.end(); ++iter)
		{ printf("%s := ", (*iter).first.c_str()); (*iter).second->dump(); printf(", "); }
	printf(">\n");
}



Value *
LocalDeclAST::Codegen(void)
{ return NULL; }

void
LocalDeclAST::dump(void)
{
	printf("<local ");
	if (type == 1)
		printf("(int) ");
	else if (type == 2)
		printf("(array) ");
	for (std::vector<std::pair<std::string, ExprAST*> >::iterator iter = decls.begin(); iter != decls.end(); ++iter)
		{ printf("%s ", (*iter).first.c_str()); if ((*iter).second) printf("::= "), (*iter).second->dump(); printf(", "); }
	printf(">\n");
}



Value *
ConstExprAST::Codegen(void)
{ return NULL; }

void
ConstExprAST::dump(void)
{
	printf("<constant ");
	PrintObject(value.value.ref, 0);
	printf(">");
}



Value *
PathExprAST::Codegen(void)
{ return NULL; }

void
PathExprAST::dump(void)
{
	printf("<pathExpr ");
	for (std::vector<std::string>::iterator iter = elements.begin(); iter != elements.end(); ++iter)
		{ printf("%s.", (*iter).c_str()); }
	printf("> ");
}



Value *
ArrayAST::Codegen(void)
{ return NULL; }

void
ArrayAST::dump(void)
{
	printf("<array ");
	if (!className.empty())
		printf("%s: ", className.c_str());
	for (std::vector<ExprAST*>::iterator iter = elements.begin(); iter != elements.end(); ++iter)
		{ (*iter)->dump(); printf(", "); }
	printf("> ");
}



Value *
FrameAST::Codegen(void)
{ return NULL; }

void
FrameAST::dump(void)
{
	printf("<frame ");
	for (std::vector<std::pair<std::string, ExprAST*> >::iterator iter = elements.begin(); iter != elements.end(); ++iter)
		{ printf("%s ::= ", (*iter).first.c_str()); (*iter).second->dump(); printf(", "); }
	printf("> ");
}



Value *
FuncExprAST::Codegen(void)
{ return NULL; }

void
FuncExprAST::dump(void)
{
	printf("<func ");
	if (isNative)
		printf("native ");
	printf("(");
	for (std::vector<std::pair<int,std::string> >::iterator iter = args.begin(); iter != args.end(); ++iter)
		{ printf("(%d)%s, ", (*iter).first, (*iter).second.c_str()); }
	printf(")\n");
	body->dump();
	printf(">\n");
}



Value *
FrameAccessExprAST::Codegen(void)
{ return NULL; }

void
FrameAccessExprAST::dump(void)
{
	printf("<frame accessor "); frame->dump(); printf("."); accessor->dump(); printf("> ");
}



Value *
ArrayAccessExprAST::Codegen(void)
{ return NULL; }

void
ArrayAccessExprAST::dump(void)
{
	printf("<array accessor "); array->dump(); printf("["); accessor->dump(); printf("]> ");
}



Value *
FuncAccessExprAST::Codegen(void)
{ return NULL; }

void
FuncAccessExprAST::dump(void)
{
	printf("<func accessor ");
	receiver->dump();
	printf(" :%s>/n", message.c_str());
}



Value *
MessageSendExprAST::Codegen(void)
{ return NULL; }

void
MessageSendExprAST::dump(void)
{
	printf("<invoke ");
	receiver->dump();
	printf(" :%s(", message.c_str());
	for (std::vector<ExprAST*>::iterator iter = args.begin(); iter != args.end(); ++iter)
		{ (*iter)->dump(); printf(", "); }
	printf(")>\n");
}


Value *
CallExprAST::Codegen(void)
{ return NULL; }

void
CallExprAST::dump(void)
{
	printf("<call ");
	callee->dump();
	printf(" with ");
	for (std::vector<ExprAST*>::iterator iter = args.begin(); iter != args.end(); ++iter)
		{ (*iter)->dump(); printf(", "); }
	printf(">\n");
}



Value *
ReceiverExprAST::Codegen(void)
{ return NULL; }

void
ReceiverExprAST::dump(void)
{
	if (type == kReceiverSelf)
		printf("<self>");
	else if (type == kReceiverInherited)
		printf("<inherited>");
	else
		printf("<BAD RECEIVER>");
}



Value *
AssignExprAST::Codegen(void)
{ return NULL; }

void
AssignExprAST::dump(void)
{
	printf("< ");
	lhs->dump();
	printf(" := ");
	rhs->dump();
	printf(">\n");
}


Value *
UnaryExprAST::Codegen(void)
{ return NULL; }

void
UnaryExprAST::dump(void)
{
	printf("< ");
	switch (op)
	{
	case TOKENuMinus: case '-':	printf("-"); break;
	case TOKENnot:						printf("not "); break;
	}
	operand->dump();
	printf("> ");
}


Value *
BinaryExprAST::Codegen(void)
{ return NULL; }

void
BinaryExprAST::dump(void)
{
	printf("< ");
	lhs->dump();
	switch (op)
	{
	case TOKENsendIfDefined:	printf(":? "); break;
	case TOKENLShift:				printf("<< "); break;
	case TOKENRShift:				printf(">> "); break;
	case TOKENdiv:					printf("div "); break;
	case TOKENmod:					printf("mod "); break;
	case TOKENAmperAmper:		printf("&& "); break;
	case TOKENexists:				printf("exists "); break;
	case TOKENEQL:					printf("= "); break;
	case TOKENNEQ:					printf("<> "); break;
	case TOKENnot:					printf("not "); break;
	case TOKENand:					printf("and "); break;
	case TOKENor:					printf("or "); break;
	case TOKENassign:				printf(":= "); break;
	default:							printf("%c ", op); break;
	}
	rhs->dump();
	printf("> ");
}


Value *
LiteralExprAST::Codegen(void)
{ return NULL; }

void
LiteralExprAST::dump(void)
{
	printf("<'");
	literal->dump();
	printf("> ");
}


Value *
ExistsExprAST::Codegen(void)
{ return NULL; }

void
ExistsExprAST::dump(void)
{
	printf("<");
	operand->dump();
	printf(" exists> ");
}


Value *
IfExprAST::Codegen(void)
{ return NULL; }

void
IfExprAST::dump(void)
{
	printf("<if ");
	cond->dump();
	printf(" then ");
	then->dump();
	if (els)
	{
		printf(" else ");
		els->dump();
	}
	printf(">\n");
}


Value *
LoopExprAST::Codegen(void)
{ return NULL; }

void
LoopExprAST::dump(void)
{
	printf("<loop ");
	body->dump();
	printf(">\n");
}


Value *
ForExprAST::Codegen(void)
{ return NULL; }

void
ForExprAST::dump(void)
{
	printf("<for %s := ", iterName.c_str());
	start->dump();
	printf(" to ");
	stop->dump();
	if (step)
	{
		printf(" by ");
		step->dump();
	}
	printf(" do ");
	body->dump();
	printf(">\n");
}


Value *
ForeachExprAST::Codegen(void)
{ return NULL; }

void
ForeachExprAST::dump(void)
{
	printf("<foreach %s ", varName.c_str());
	if (!indexName.empty())
		printf(", %s ", indexName.c_str());
	if (isDeeply)
		printf("deeply ");
	printf("in ");
	iter->dump();
	printf("do ");
	body->dump();
	printf(">\n");
}


Value *
WhileExprAST::Codegen(void)
{ return NULL; }

void
WhileExprAST::dump(void)
{
	printf("<while ");
	cond->dump();
	printf("do ");
	body->dump();
	printf(">\n");
}


Value *
RepeatExprAST::Codegen(void)
{ return NULL; }

void
RepeatExprAST::dump(void)
{
	printf("<repeat ");
	body->dump();
	printf("until ");
	cond->dump();
	printf(">\n");
}


Value *
BreakExprAST::Codegen(void)
{ return NULL; }

void
BreakExprAST::dump(void)
{
	printf("<break "); if (value) value->dump(); printf(">\n");
}


Value *
TryExprAST::Codegen(void)
{ return NULL; }

void
TryExprAST::dump(void)
{
	printf("<try ");
	body->dump();
	for (std::vector<std::pair<std::string, ExprAST*> >::iterator iter = handlers.begin(); iter != handlers.end(); ++iter)
		{ printf("onexception %s do ", (*iter).first.c_str()); (*iter).second->dump(); printf(", "); }
	printf(">\n");
}


Value *
ReturnExprAST::Codegen(void)
{ return NULL; }

void
ReturnExprAST::dump(void)
{
	printf("<return "); if (value) value->dump(); printf(">\n");
}

