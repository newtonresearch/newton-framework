/*
flex -d -i -Cfe  definition of NewtonScript
*/

%{
#include "NewtonTypes.h"
#include "y.tab.h"
%}

%x	linecomment
%x	fullcomment

%%
	/* eat up one-line comments */
"//"			BEGIN(linecomment);
<linecomment>\n		BEGIN(INITIAL);

	/* eat up multi-line comments */
"/*"			BEGIN(fullcomment);
<fullcomment>[^*]*
<fullcomment>"*"+[^*/]*
<fullcomment>"*"+"/"	BEGIN(INITIAL);

	/* eat up whitespace */
[ \t\012\015]

	/* terminate appropriately */
<<EOF>>		return -1;

	/* |symbol| */
"|"		{
				long size;
				UniChar * str = GetCharsUntil('|', NO, size);
				if (str == NULL)
					return -1;
				char cstr255[256];
				yylval = Intern(ConvertFromUnicode(str, &cstr255, 2, 256));
				free(str);
				return kTokenSymbol;
			}


		/* number */
[0-9]		return GetNumber();

"#line "

"@"		{
				long mptr = 0;
				UniChar ch = input->GetChar();
				if (!IsDigit(ch))
					Error(kNSErrDigitRequired);
				do
				{
					mptr = mptr * 10 + ch - '0';
					ch = input->GetChar();
				} while (ch != EOF && IsDigit(ch));
				input->UngetChar(ch);
				yylval = MAKEMAGICPTR(mptr);
				return kTokenRefConst;
			}


"\""		{
				Ref	val;
				long	size;
				UniChar * str = GetCharsUntil('\"', YES, &size);
				if (str == NULL)
					return -1;
				val = AllocateBinary(SYMA(string), size);
				memmove(BinaryData(val), str, size);
				free(str);
				yylval = val;
				return kTokenConst;
			}

"$"		return kTokenConst;

"<="		return kTokenLEQ;

"<>"		return kTokenNEQ;

"<<"		return kTokenLShift;

">="		return kTokenGEQ;

">>"		return kTokenRShift;

"="		return kTokenEQL;

":="		return kTokenAssign;

":?"		return kTokenSendIfDefined;

"&&"		return kTokenAmperAmper;

[-+*/()'.\[\],;{}]	return *yytext;

.			CompilerError(kNSErrUnrecognized, MAKECHAR(ch));

%%
