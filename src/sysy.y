%{
#include <cstdio>
#include <cstdlib>

#include "enum_defs.hpp"
#include "ast_defs.hpp"

extern int yylex();
int yyerror(const char*);
%}

%union {
    enum LexTypeAdd opt_add;
    enum LexTypeMul opt_mul;
    enum LexTypeRel opt_rel;
    enum LexTypeEq opt_eq;
    char *ident_str;
    int lit_int;
};

%token KW_CONST KW_INT KW_VOID KW_IF KW_WHILE KW_BREAK KW_CONTINUE KW_RETURN // keywords
%token COMMA SEMICOLON ASSIGN // , ; =
%token L_PAREN R_PAREN L_BRACKET R_BRACKET L_BRACE R_BRACE // () [] {}
%token <opt_add> OPTYPE_ADD // + -
%token <opt_mul> OPTYPE_MUL // * / %
%token <opt_rel> OPTYPE_REL // < > <= >=
%token <opt_eq> OPTYPE_EQ // == !=
%token OP_AND // &&
%token OP_OR // ||
%token OP_NOT // !
%token <ident_str> IDENT
%token <lit_int> LITERAL

 // https://stackoverflow.com/questions/6911214/how-to-make-else-associate-with-farthest-if-in-yacc
%nonassoc PREC_IF_ONLY
%nonassoc PREC_IF_ELSE

%start CompUnit

%%

CompUnit: CompUnit LITERAL {}

CompUnit: LITERAL {
    printf("got const %d\n", $1);
}

%%

int yyerror(const char *msg) {
    fprintf(stderr, "yyerror: %s\n", msg);
    exit(1);
}
