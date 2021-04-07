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

    #define gen_ptr_type(name) Ast##name* ptr_##name

    gen_ptr_type(CompUnit);
    gen_ptr_type(Decl);
    gen_ptr_type(Defs);
    gen_ptr_type(Def);
    gen_ptr_type(MaybeIdx);
    gen_ptr_type(InitVal);
    gen_ptr_type(FuncDef);
    gen_ptr_type(FuncDefParams);
    gen_ptr_type(FuncUseParams);
    gen_ptr_type(Block);
    gen_ptr_type(BlockItems);
    gen_ptr_type(Stmt);
    gen_ptr_type(Exp);
    gen_ptr_type(ExpLVal);

    #undef gen_ptr_type
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

%type <ptr_CompUnit> CompUnit
%type <ptr_Decl> Decl
%type <ptr_Defs> Defs
%type <ptr_Def> Def FuncDefParam
%type <ptr_MaybeIdx> MaybeIdx
%type <ptr_InitVal> InitVal  ManyInitVal  ZeroOrManyInitVal
%type <ptr_FuncDef> FuncDef
%type <ptr_FuncDefParams> ManyFuncDefParam ZeroOrManyFuncDefParam
%type <ptr_Block> Block BlockItems
%type <ptr_Stmt> Stmt
%type <ptr_Exp> Exp Cond PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp
%type <ptr_ExpLVal> LVal  
%type <ptr_FuncUseParams> ManyFuncUseParam ZeroOrManyFuncUseParam

%%

 // todo: delete[] ident_str

 ///// LANGUAGE CONSTRUCTS

Root: CompUnit {
    ast_root = (Ast*)($1);
};

CompUnit: Decl {
    $$ = new AstCompUnit();
    $1->propagate_defpos(DefGlobal);
    $$->push_val($1);
};
CompUnit: FuncDef {
    $$ = new AstCompUnit();
    $$->push_val($1);
};
CompUnit: CompUnit Decl {
    $1->push_val($2);
    $2->propagate_defpos(DefGlobal);
    $$ = $1;
};
CompUnit: CompUnit FuncDef {
    $1->push_val($2);
    $$ = $1;
};

Decl: KW_CONST KW_INT Defs SEMICOLON {
    $$ = new AstDecl(VarInt, true, $3);
};
Decl: KW_INT Defs SEMICOLON {
    $$ = new AstDecl(VarInt, false, $2);
};

Defs: Def {
    $$ = new AstDefs();
    $$->push_val($1);
};
Defs: Defs COMMA Def {
    $1->push_val($3);
    $$ = $1;
};

Def: IDENT MaybeIdx ASSIGN InitVal {
    $$ = new AstDef($1, $2, $4);
};
Def: IDENT MaybeIdx {
    $$ = new AstDef($1, $2, nullptr);
};

MaybeIdx: /*empty*/ {
    $$ = new AstMaybeIdx();
};
MaybeIdx: MaybeIdx L_BRACKET Exp R_BRACKET {
    $1->push_val($3);
    $$ = $1;
};

InitVal: Exp {
    $$ = new AstInitVal(false);
    $$->val.single = $1;
};
InitVal: L_BRACE ZeroOrManyInitVal R_BRACE {
    $$ = $2;
};

ZeroOrManyInitVal: /*empty*/ {
    $$ = new AstInitVal(true);
};
ZeroOrManyInitVal: ManyInitVal {
    $$ = $1;
};
ManyInitVal: InitVal {
    $$ = new AstInitVal(true);
    $$->push_val($1);
};
ManyInitVal: ManyInitVal COMMA InitVal {
    $1->push_val($3);
    $$ = $1;
};

FuncDef: KW_VOID IDENT L_PAREN ZeroOrManyFuncDefParam R_PAREN Block {
    $$ = new AstFuncDef(FuncVoid, $2, $4, $6);
};
FuncDef: KW_INT IDENT L_PAREN ZeroOrManyFuncDefParam R_PAREN Block {
    $$ = new AstFuncDef(FuncInt, $2, $4, $6);
};

ZeroOrManyFuncDefParam: /*empty*/ {
    $$ = new AstFuncDefParams();
};
ZeroOrManyFuncDefParam: ManyFuncDefParam {
    $1->propagate_property_and_defpos();
    $$ = $1;
};
ManyFuncDefParam: FuncDefParam {
    $$ = new AstFuncDefParams();
    $$->push_val($1);
};
ManyFuncDefParam: ManyFuncDefParam COMMA FuncDefParam {
    $1->push_val($3);
    $$ = $1;
};

FuncDefParam: KW_INT IDENT {
    $$ = new AstDef($2, new AstMaybeIdx(), nullptr);
};
FuncDefParam: KW_INT IDENT L_BRACKET R_BRACKET MaybeIdx {
    $5->val.insert($5->val.begin(), nullptr);
    $$ = new AstDef($2, $5, nullptr);
};

Block: L_BRACE BlockItems R_BRACE {
    $$ = $2;
};
BlockItems: /*empty*/ {
    $$ = new AstBlock();
};
BlockItems: BlockItems Decl {
    $1->push_val($2);
    $2->propagate_defpos(DefGlobal);
    $$ = $1;
};
BlockItems: BlockItems Stmt {
    $1->push_val($2);
    $$ = $1;
};

 ///// STATEMENT

Stmt: LVal ASSIGN Exp SEMICOLON {
    $$ = new AstStmtAssignment($1, $3);
};
Stmt: Exp SEMICOLON {
    $$ = new AstStmtExp($1);
};
Stmt: SEMICOLON {
    $$ = new AstStmtVoid();
};
Stmt: Block {
    $$ = new AstStmtBlock($1);
};
Stmt: KW_IF L_PAREN Cond R_PAREN Stmt %prec PREC_LOWER_THAN_ELSE {
    $$ = new AstStmtIfOnly($3, $5);
};
Stmt: KW_IF L_PAREN Cond R_PAREN Stmt KW_ELSE Stmt {
    $$ = new AstStmtIfElse($3, $5, $7);
};
Stmt: KW_WHILE L_PAREN Cond R_PAREN Stmt {
    $$ = new AstStmtWhile($3, $5);
};
Stmt: KW_BREAK SEMICOLON {
    $$ = new AstStmtBreak();
};
Stmt: KW_CONTINUE SEMICOLON {
    $$ = new AstStmtContinue();
};
Stmt: KW_RETURN SEMICOLON {
    $$ = new AstStmtReturnVoid();
};
Stmt: KW_RETURN Exp SEMICOLON {
    $$ = new AstStmtReturn($2);
};

 ///// EXPRESSION

Exp: AddExp {
    $$ = $1;
};

Cond: LOrExp {
    $$ = $1;
};

LVal: IDENT MaybeIdx {
    $$ = new AstExpLVal($1, $2);
};

PrimaryExp: L_PAREN Exp R_PAREN {
    $$ = $2;
};
PrimaryExp: LVal {
    $$ = $1;
};
PrimaryExp: LITERAL {
    $$ = new AstExpLiteral($1);
};

UnaryExp: PrimaryExp {
    $$ = $1;
};
UnaryExp: IDENT L_PAREN ZeroOrManyFuncUseParam R_PAREN {
    $$ = new AstExpFunctionCall($1, $3);
};
UnaryExp: OPTYPE_ADD UnaryExp {
    $$ = new AstExpOpUnary(cvt_to_unary($1), $2);
};
UnaryExp: OP_NOT UnaryExp {
    $$ = new AstExpOpUnary(OpNot, $2);
};

ZeroOrManyFuncUseParam: /*empty*/ {
    $$ = new AstFuncUseParams();
};
ZeroOrManyFuncUseParam: ManyFuncUseParam {
    $$ = $1;
};
ManyFuncUseParam: Exp {
    $$ = new AstFuncUseParams();
    $$->push_val($1);
};
ManyFuncUseParam: ManyFuncUseParam COMMA Exp {
    $1->push_val($3);
    $$ = $1;
};

MulExp: UnaryExp {
    $$ = $1;
};
MulExp: MulExp OPTYPE_MUL UnaryExp {
    $$ = new AstExpOpBinary(cvt_to_binary($2), $1, $3);
};

AddExp: MulExp {
    $$ = $1;
};
AddExp: AddExp OPTYPE_ADD MulExp {
    $$ = new AstExpOpBinary(cvt_to_binary($2), $1, $3);
};

RelExp: AddExp {
    $$ = $1;
};
RelExp: RelExp OPTYPE_REL AddExp {
    $$ = new AstExpOpBinary(cvt_to_binary($2), $1, $3);
};

EqExp: RelExp {
    $$ = $1;
};
EqExp: EqExp OPTYPE_EQ RelExp {
    $$ = new AstExpOpBinary(cvt_to_binary($2), $1, $3);
};

LAndExp: EqExp {
    $$ = $1;
};
LAndExp: LAndExp OP_AND EqExp {
    $$ = new AstExpOpBinary(OpAnd, $1, $3);
};

LOrExp: LAndExp {
    $$ = $1;
};
LOrExp: LOrExp OP_OR LAndExp {
    $$ = new AstExpOpBinary(OpOr, $1, $3);
};

%%

int yyerror(const char *msg) {
    fprintf(stderr, "yyerror: %s at line %d col %d\n", msg, yylineno, atoz_yycol);
    exit(1);
}
