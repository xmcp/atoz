#pragma once

#include <vector>
#include <string>
#include <cassert>
using std::vector;
using std::string;

#include "enum_defs.hpp"

extern int yylineno;
extern int atoz_yycol;

///// FORWARD DECL

struct AstDefs;
struct AstDef;
struct AstMaybeIdx;
struct AstInitVal;
struct AstExp;
struct AstFuncDefParams;
struct AstBlock;
struct AstExpLVal;

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

///// LANGUAGE CONSTRUCTS

struct AstCompUnit: Ast {
    vector<Ast*> val; // Decl, FuncDef

    AstCompUnit() {}
    void push_val(Ast *next);

    void lookup_name();
};

struct AstDecl: Ast {
    VarType _type;
    bool _is_const;
    AstDefs *defs;

    AstDecl(VarType type, bool is_const, AstDefs *defs):
        _type(type), _is_const(is_const), defs(defs) {
            propagate_property();
        }
    
    void propagate_property();
    void propagate_defpos(DefPosition pos);
};

struct AstDefs: Ast {
    vector<AstDef*> val;

    AstDefs() {}
    void push_val(AstDef *next) {
        val.push_back(next);
    }
};

struct AstDef: Ast {
    string name;
    AstMaybeIdx *idxinfo;
    AstInitVal *initval_or_null;

    // will be propagated
    VarType type;
    bool is_const;
    DefPosition pos;
    int index;

    AstDef(string var_name, AstMaybeIdx *idxinfo, AstInitVal *initval):
        name(var_name), idxinfo(idxinfo), initval_or_null(initval),
        type(VarInt), is_const(false), pos(DefUnknown), index(-1) {}
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

    AstInitVal(bool is_many): is_many(is_many), val({}) {
        if(is_many)
            val.many = new vector<AstInitVal*>();
    }
    void push_val(AstInitVal *next) {
        assert(is_many);
        val.many->push_back(next);
    }
};

struct AstFuncDef: Ast {
    FuncType type;
    string name;
    AstFuncDefParams *params;
    AstBlock *body;

    AstFuncDef(FuncType type, string func_name, AstFuncDefParams *params, AstBlock *body):
        type(type), name(func_name), params(params), body(body) {}
};

struct AstFuncDefParams: Ast {
    vector<AstDef*> val;

    AstFuncDefParams() {}
    void push_val(AstDef *next) {
        val.push_back(next);
    }
    void propagate_property_and_defpos();
};

struct AstFuncUseParams: Ast {
    vector<AstExp*> val;

    AstFuncUseParams() {}
    void push_val(AstExp *next) {
        val.push_back(next);
    }
};

struct AstBlock: Ast {
    vector<Ast*> body; // Decl, Stmt

    AstBlock() {}
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
    void gen_eeyore() override;
};

struct AstStmtExp: AstStmt {
    AstExp *exp;

    AstStmtExp(AstExp *exp): AstStmt(StmtExp),
        exp(exp) {}
    void gen_eeyore() override;
};

struct AstStmtVoid: AstStmt {
    AstStmtVoid(): AstStmt(StmtVoid) {}
    void gen_eeyore() override;
};

struct AstStmtBlock: AstStmt {
    AstBlock *block;

    AstStmtBlock(AstBlock *block): AstStmt(StmtBlock),
        block(block) {}
    void gen_eeyore() override;
};

struct AstStmtIfOnly: AstStmt {
    AstExp *cond;
    AstStmt *body;

    AstStmtIfOnly(AstExp *cond, AstStmt *body): AstStmt(StmtIfOnly),
        cond(cond), body(body) {}
    void gen_eeyore() override;
};

struct AstStmtIfElse: AstStmt {
    AstExp *cond;
    AstStmt *body_true;
    AstStmt *body_false;

    AstStmtIfElse(AstExp *cond, AstStmt *body_true, AstStmt *body_false): AstStmt(StmtIfElse),
        cond(cond), body_true(body_true), body_false(body_false) {}
    void gen_eeyore() override;
};

struct AstStmtWhile: AstStmt {
    AstExp *cond;
    AstStmt *body;

    AstStmtWhile(AstExp *cond, AstStmt *body): AstStmt(StmtWhile),
        cond(cond), body(body) {}
    void gen_eeyore() override;
};

struct AstStmtBreak: AstStmt {
    AstStmtBreak(): AstStmt(StmtBreak) {}
    void gen_eeyore() override;
};

struct AstStmtContinue: AstStmt {
    AstStmtContinue(): AstStmt(StmtContinue) {}
    void gen_eeyore() override;
};

struct AstStmtReturnVoid: AstStmt {
    AstStmtReturnVoid(): AstStmt(StmtReturnVoid) {}
    void gen_eeyore() override;
};

struct AstStmtReturn: AstStmt {
    AstExp *retval;

    AstStmtReturn(AstExp *retval): AstStmt(StmtReturn),
        retval(retval) {}
    void gen_eeyore() override;
};

///// EXPRESSION

struct AstExp: Ast {
    virtual ConstExpResult calc_const() = 0;
};

struct AstExpLVal: AstExp {
    string name;
    AstDef *def;
    AstMaybeIdx *idxinfo;

    AstExpLVal(string name, AstMaybeIdx *idxinfo):
        name(name), idxinfo(idxinfo),
        def(nullptr) {}
    ConstExpResult calc_const() override;
};

struct AstExpLiteral: AstExp {
    int val;

    AstExpLiteral(int val): val(val) {}
    ConstExpResult calc_const() override;
};

struct AstExpFunctionCall: AstExp {
    string name;
    AstFuncDef *def;
    AstFuncUseParams *params;

    AstExpFunctionCall(string name, AstFuncUseParams *params):
        name(name), params(params),
        def(nullptr) {}
    ConstExpResult calc_const() override;
};

struct AstExpOpUnary: AstExp {
    UnaryOpKinds op;
    AstExp *operand;

    AstExpOpUnary(UnaryOpKinds op, AstExp *operand):
        op(op), operand(operand) {}
    ConstExpResult calc_const() override;
};

struct AstExpOpBinary: AstExp {
    BinaryOpKinds op;
    AstExp *operand1;
    AstExp *operand2;

    AstExpOpBinary(BinaryOpKinds op, AstExp *operand1, AstExp *operand2):
        op(op), operand1(operand1), operand2(operand2) {}
    ConstExpResult calc_const() override;
};