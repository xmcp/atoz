#pragma once

#include <vector>
#include <string>
using std::vector;
using std::string;

#include "enum_defs.hpp"

extern int yylineno;
extern int atoz_yycol;

///// BASE

struct NodeLocation {
    int lineno;
    int colno;
    NodeLocation(): lineno(yylineno), colno(atoz_yycol)  {}
};

struct Ast {
    NodeLocation loc;
    Ast(): loc() {
    }
    // https://stackoverflow.com/questions/15114093/getting-source-type-is-not-polymorphic-when-trying-to-use-dynamic-cast
    virtual ~Ast() = default;
};

struct ConstExpResult {
    string error;
    int val;

private:
    ConstExpResult(string error, int val):
        error(error), val(val) {}

public:
    static ConstExpResult asValue(int val) {
        return ConstExpResult("", val);
    }
    static ConstExpResult asError(string error) {
        return ConstExpResult(error, 0);
    }
};

///// FORWARD DECL

struct AstVarType;
struct AstDefs;
struct AstDef;
struct AstMaybeIdx;
struct AstInitVal;
struct AstExp;
struct AstFuncType;
struct AstFuncDefParams;
struct AstFuncDefParam;
struct AstBlock;
struct AstBlockItems;
struct AstExpLVal;

///// LANGUAGE CONSTRUCTS

struct AstCompUnit: Ast {
    vector<Ast*> val; // Decl, FuncDef

    AstCompUnit() {}
    void push_val(Ast *next);
};

struct AstDecl: Ast {
    AstVarType *type;
    bool is_const;
    AstDefs *defs;

    AstDecl(AstVarType *type, bool is_const, AstDefs *defs):
        type(type), is_const(is_const), defs(defs) {}
};

struct AstDefs: Ast {
    vector<AstDef*> val;

    AstDefs() {}
    void push_val(AstDef *next) {
        val.push_back(next);
    }
};

struct AstVarType: Ast { // manual
    VarType val;
    
    AstVarType(VarType val): val(val) {}
};

struct AstDef: Ast {
    string name;
    AstMaybeIdx *idxinfo;
    AstInitVal *initval_or_null;

    AstDef(string var_name, AstMaybeIdx *idxinfo, AstInitVal *initval):
        name(var_name), idxinfo(idxinfo), initval_or_null(initval) {}
};

struct AstMaybeIdx: Ast {
    vector<AstExp*> val;

    AstMaybeIdx() {}
    void push_val(AstExp *next) {
        val.push_back(next);
    }
};

struct AstInitVal: Ast {
    bool is_many;
    union {
        AstExp *single;
        vector<AstInitVal*> *many;
    } val;
};

struct AstFuncDef: Ast {
    AstFuncType *type;
    string name;
    AstFuncDefParams *params;
    AstBlock *body;

    AstFuncDef(AstFuncType *type, string func_name, AstFuncDefParams *params, AstBlock *body):
        type(type), name(func_name), params(params), body(body) {}
};

struct AstFuncType: Ast { // manual
    FuncType val;

    AstFuncType(FuncType val): val(val) {}
};

struct AstFuncDefParams: Ast {
    vector<AstFuncDefParam*> val;

    AstFuncDefParams() {}
    void push_val(AstFuncDefParam *next) {
        val.push_back(next);
    }
};

struct AstFuncDefParam: Ast {
    AstVarType *type;
    string name;
    AstMaybeIdx *idxinfo_or_null;

    AstFuncDefParam(AstVarType *type, string param_name, AstMaybeIdx *idxinfo_or_null):
        type(type), name(param_name), idxinfo_or_null(idxinfo_or_null) {}
};

struct AstFuncUseParams: Ast {
    vector<AstExp*> val;

    AstFuncUseParams() {}
    void push_val(AstExp *next) {
        val.push_back(next);
    }
};

struct AstBlock: Ast {
    AstBlockItems *body;

    AstBlock(AstBlockItems *body): body(body) {}
};

struct AstBlockItems: Ast {
    vector<Ast*> val; // Decl, Stmt

    AstBlockItems() {}
    void push_val(Ast *next);
};

///// STATEMENT

struct AstStmt: Ast {
    StmtKinds kind;
    
    AstStmt(StmtKinds kind): kind(kind) {}
    virtual void gen_eeyore() = 0;
};

struct AstStmtAssignment: AstStmt {
    AstExpLVal *lval;
    AstExp *rval;

    AstStmtAssignment(AstExpLVal *lval, AstExp *rval): AstStmt(StmtAssignment),
        lval(lval), rval(rval) {}
    void gen_eeyore();
};

struct AstStmtVoid: AstStmt {
    AstStmtVoid(): AstStmt(StmtVoid) {}
    void gen_eeyore();
};

struct AstStmtBlock: AstStmt {
    AstBlock *block;

    AstStmtBlock(AstBlock *block): AstStmt(StmtBlock),
        block(block) {}
    void gen_eeyore();
};

struct AstStmtIfOnly: AstStmt {
    AstExp *cond;
    AstStmt *body;

    AstStmtIfOnly(AstExp *cond, AstStmt *body): AstStmt(StmtIfOnly),
        cond(cond), body(body) {}
    void gen_eeyore();
};

struct AstStmtIfElse: AstStmt {
    AstExp *cond;
    AstStmt *body_true;
    AstStmt *body_false;

    AstStmtIfElse(AstExp *cond, AstStmt *body_true, AstStmt *body_false): AstStmt(StmtIfElse),
        cond(cond), body_true(body_true), body_false(body_false) {}
    void gen_eeyore();
};

struct AstStmtWhile: AstStmt {
    AstExp *cond;
    AstStmt *body;

    AstStmtWhile(AstExp *cond, AstStmt *body): AstStmt(StmtWhile),
        cond(cond), body(body) {}
    void gen_eeyore();
};

struct AstStmtBreak: AstStmt {
    AstStmtBreak(): AstStmt(StmtBreak) {}
    void gen_eeyore();
};

struct AstStmtContinue: AstStmt {
    AstStmtContinue(): AstStmt(StmtContinue) {}
    void gen_eeyore();
};

struct AstStmtReturnVoid: AstStmt {
    AstStmtReturnVoid(): AstStmt(StmtReturnVoid) {}
    void gen_eeyore();
};

struct AstStmtReturn: AstStmt {
    AstExp *retval;

    AstStmtReturn(AstExp *retval): AstStmt(StmtReturn),
        retval(retval) {}
    void gen_eeyore();
};

///// EXPRESSION

struct AstExp: Ast {
    virtual ConstExpResult calc_const() = 0;
};

struct AstExpLVal: AstExp {
    string name;
    AstMaybeIdx *idxinfo;

    AstExpLVal(string name, AstMaybeIdx *idxinfo):
        name(name), idxinfo(idxinfo) {}
    ConstExpResult calc_const();
};

struct AstExpLiteral: AstExp {
    int val;

    AstExpLiteral(int val): val(val) {}
    ConstExpResult calc_const();
};

struct AstExpFunctionCall: AstExp {
    string name;
    AstFuncUseParams params;

    AstExpFunctionCall(string name, AstFuncUseParams params):
        name(name), params(params) {}
    ConstExpResult calc_const();
};

struct AstExpOpUnary: AstExp {
    UnaryOpKinds op;
    AstExp *operand;

    AstExpOpUnary(UnaryOpKinds op, AstExp *operand):
        op(op), operand(operand) {}
    ConstExpResult calc_const();
};

struct AstExpOpBinary: AstExp {
    UnaryOpKinds op;
    AstExp *operand1;
    AstExp *operand2;

    AstExpOpBinary(UnaryOpKinds op, AstExp *operand1, AstExp *operand2):
        op(op), operand1(operand1), operand2(operand2) {}
    ConstExpResult calc_const();
};