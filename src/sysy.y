%{
#include <cstdio>
#include <cstdlib>

#include "enum_defs.hpp"
#include "ast_defs.hpp"

extern int yylex();
int yyerror(const char*);
extern Ast *ast_root;

%}

%union {
    enum LexTypeAdd opt_add;
    enum LexTypeMul opt_mul;
    enum LexTypeRel opt_rel;
    enum LexTypeEq opt_eq;
    char *ident_str;
    int lit_int;
    Ast* ast;
};

%token KW_CONST KW_INT KW_VOID KW_IF KW_ELSE KW_WHILE KW_BREAK KW_CONTINUE KW_RETURN // keywords
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

 // https://stackoverflow.com/questions/1737460/how-to-find-shift-reduce-conflict-in-this-yacc-file
%nonassoc PREC_LOWER_THAN_ELSE
%nonassoc KW_ELSE

%start Root

%type <ast> CompUnit Decl Defs Def MaybeIdx InitVal ManyInitVal ZeroOrManyInitVal
%type <ast> FuncDef ManyFuncDefParam ZeroOrManyFuncDefParam FuncDefParam Block BlockItems Stmt
%type <ast> Exp Cond LVal PrimaryExp UnaryExp ManyFuncUseParam ZeroOrManyFuncUseParam
%type <ast> MulExp AddExp RelExp EqExp LAndExp LOrExp

%%

 ///// LANGUAGE CONSTRUCTS

Root: CompUnit {
    ast_root = (Ast*)(1); // todo: change this
};

CompUnit: Decl {

};

CompUnit: FuncDef {
    
};

CompUnit: CompUnit Decl {

};

CompUnit: CompUnit FuncDef {

};

Decl: KW_CONST KW_INT Defs SEMICOLON {

};

Decl: KW_INT Defs SEMICOLON {

};

Defs: Def {

};

Defs: Defs COMMA Def {

};

Def: IDENT MaybeIdx ASSIGN InitVal {

};

Def: IDENT MaybeIdx {

};

MaybeIdx: /*empty*/ {

};

MaybeIdx: MaybeIdx L_BRACKET Exp R_BRACKET {

};

InitVal: Exp {

};

InitVal: L_BRACE ZeroOrManyInitVal R_BRACE {

};

ZeroOrManyInitVal: /*empty*/ {

};

ZeroOrManyInitVal: ManyInitVal {

};

ManyInitVal: InitVal {

};

ManyInitVal: ManyInitVal COMMA InitVal {

};

FuncDef: KW_VOID IDENT L_PAREN ZeroOrManyFuncDefParam R_PAREN Block {

};

FuncDef: KW_INT IDENT L_PAREN ZeroOrManyFuncDefParam R_PAREN Block {

};

ZeroOrManyFuncDefParam: /*empty*/ {

};

ZeroOrManyFuncDefParam: ManyFuncDefParam {

};

ManyFuncDefParam: FuncDefParam {

};

ManyFuncDefParam: ManyFuncDefParam COMMA FuncDefParam {

};

FuncDefParam: KW_INT IDENT {

};

FuncDefParam: KW_INT IDENT L_BRACKET R_BRACKET MaybeIdx {

};

Block: L_BRACE BlockItems R_BRACE {

};

BlockItems: /*empty*/ {

};

BlockItems: BlockItems Decl {

};
BlockItems: BlockItems Stmt {

};

 ///// STATEMENT

Stmt: LVal ASSIGN Exp SEMICOLON {

};

Stmt: Exp SEMICOLON {

};

Stmt: SEMICOLON {

};

Stmt: Block {

};

Stmt: KW_IF L_PAREN Cond R_PAREN Stmt %prec PREC_LOWER_THAN_ELSE {

};

Stmt: KW_IF L_PAREN Cond R_PAREN Stmt KW_ELSE Stmt {

};

Stmt: KW_WHILE L_PAREN Cond R_PAREN Stmt {

};

Stmt: KW_BREAK SEMICOLON {

};

Stmt: KW_CONTINUE SEMICOLON {

};

Stmt: KW_RETURN SEMICOLON {

};

Stmt: KW_RETURN Exp SEMICOLON {

};

 ///// EXPRESSION

Exp: AddExp {

};

Cond: LOrExp {

};

LVal: IDENT MaybeIdx {

};

PrimaryExp: L_PAREN Exp R_PAREN {

};

PrimaryExp: LVal {

};

PrimaryExp: LITERAL {

};

UnaryExp: PrimaryExp {

};

UnaryExp: IDENT L_PAREN ZeroOrManyFuncUseParam R_PAREN {

};

UnaryExp: OPTYPE_ADD UnaryExp {

};
UnaryExp: OP_NOT UnaryExp {

};

ZeroOrManyFuncUseParam: /*empty*/ {

};

ZeroOrManyFuncUseParam: ManyFuncUseParam {

};

ManyFuncUseParam: Exp {

};

ManyFuncUseParam: ManyFuncUseParam COMMA Exp {

};

MulExp: UnaryExp {

};

MulExp: MulExp OPTYPE_MUL UnaryExp {

};

AddExp: MulExp {

};

AddExp: AddExp OPTYPE_ADD MulExp {

};

RelExp: AddExp {

};

RelExp: RelExp OPTYPE_REL AddExp {

};

EqExp: RelExp {

};

EqExp: EqExp OPTYPE_EQ RelExp {

};

LAndExp: EqExp {

};

LAndExp: LAndExp OP_AND EqExp {

};

LOrExp: LAndExp {

};

LOrExp: LOrExp OP_OR LAndExp {

};

%%

int yyerror(const char *msg) {
    fprintf(stderr, "yyerror: %s at line %d col %d\n", msg, yylineno, atoz_yycol);
    exit(1);
}
