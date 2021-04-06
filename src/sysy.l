%{
#include <cstring>

#include "enum_defs.hpp"
#include "sysy.tab.hpp"

extern void yyerror(const char*);
%}

%option yylineno

%%

 /* keywords */

const {return KW_CONST;}
int {return KW_INT;}
void {return KW_VOID;}
if {return KW_IF;}
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
    memcpy(yylval.ident_str, yytext, yyleng);
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

[ \t\n] {}

%%

int yywrap() {
    return 1;
}