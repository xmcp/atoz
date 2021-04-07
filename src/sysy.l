%{
#include <cstring>

#include "enum_defs.hpp"
#include "ast_defs.hpp"
#include "sysy.tab.hpp"

extern void yyerror(const char*);
int atoz_yycol = 0;

#define YY_USER_ACTION do { \
    atoz_yycol += yyleng; \
} while(0);

%}

%option yylineno

%%

 /* keywords */

const {return KW_CONST;}
int {return KW_INT;}
void {return KW_VOID;}
if {return KW_IF;}
else {return KW_ELSE;}
while {return KW_WHILE;}
break {return KW_BREAK;}
continue {return KW_CONTINUE;}
return {return KW_RETURN;}

 /* symbols */

\, {return COMMA;}
\; {return SEMICOLON;}
\= {return ASSIGN;}

\( {return L_PAREN;}
\) {return R_PAREN;}
\[ {return L_BRACKET;}
\] {return R_BRACKET;}
\{ {return L_BRACE;}
\} {return R_BRACE;}

 /* operators */

\+|\- {
    yylval.opt_add = yytext[0]=='+' ? LexPlus : LexMinus;
    return OPTYPE_ADD;
}
\*|\/|\% {
    yylval.opt_mul = yytext[0]=='*' ? LexMul : yytext[0]=='/' ? LexDiv : LexMod;
    return OPTYPE_MUL;
}
(\<|\>)=? {
    if(yytext[1]=='=')
        yylval.opt_rel = yytext[0]=='<' ? LexLeq : LexGeq;
    else
        yylval.opt_rel = yytext[0]=='<' ? LexLess : LexGreater;
    return OPTYPE_REL;
}
[!=]= {
    yylval.opt_eq = yytext[0]=='!' ? LexNeq : LexEq;
    return OPTYPE_EQ;
}

\&\& {return OP_AND;}
\|\| {return OP_OR;}
\! {return OP_NOT;}

 /* identifiers */

[a-zA-Z_][a-zA-Z_0-9]* {
    char *ptr = new char[yyleng+1];
    ptr[yyleng] = '\0';
    memcpy(ptr, yytext, yyleng);
    yylval.ident_str = ptr;
    return IDENT;
}

 /* literals */

[1-9][0-9]* |
0[0-9]* |
0[xX][0-9a-fA-F]+ {
    yylval.lit_int = strtol(yytext, nullptr, 0);
    return LITERAL;
}

 /* comments */

\/\/.*$ {}
\/\*([^*]|\*[^/])*\*+\/ {}

 /* whitespaces */

[ \t] {}
\n {
    atoz_yycol = 0;
}

%%

int yywrap() {
    atoz_yycol = 0;
    return 1;
}