#pragma once

#include <vector>
#include <string>
#include <cassert>
using std::vector;
using std::string;

#include "enum_defs.hpp"
#include "index_scanner.hpp"

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
    bool iserror;
    string error;
    int val;

private:
    ConstExpResult(bool iserror, string error, int val):
        iserror(iserror), error(error), val(val) {}

public:
    ConstExpResult(int val):
        iserror(false), error(""), val(val) {}

    static ConstExpResult asValue(int val) {
        return ConstExpResult(val);
    }
    static ConstExpResult asError(string error) {
        return ConstExpResult(true, error, 0);
    }
    bool isalways(int v) {
        return !iserror && val==v;
    }
};

struct ConstOrVar {
    enum ConstOrVarType {
        Reference, ConstExp, TempVar
    } type;
    union {
        AstDef *reference;
        int constexp;
        int tempvar;
    } val;

private:
    ConstOrVar(ConstOrVarType type): type(type), val({}) {}

public:
    const char *eeyore_ref();
    static ConstOrVar asReference(AstDef* ref) {
        ConstOrVar ret(Reference);
        ret.val.reference = ref;
        return ret;
    }
    static ConstOrVar asConstExp(int val) {
        ConstOrVar ret(ConstExp);
        ret.val.constexp = val;
        return ret;
    }
    static ConstOrVar asTempVar(int tidx) {
        ConstOrVar ret(TempVar);
        ret.val.tempvar = tidx;
        return ret;
    }
};

///// LANGUAGE CONSTRUCTS

struct AstCompUnit: Ast {
    vector<Ast*> val; // Decl, FuncDef

    AstCompUnit() {}
    void push_val(Ast *next);

    void complete_tree();
    void gen_eeyore();
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
    void gen_eeyore(bool is_global);
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
    AstInitVal *ast_initval_or_null;

    // in yyparse phase
    VarType type;
    bool is_const;
    DefPosition pos;
    int index;

    // in tree completing phase
    InitVal initval;

    AstDef(string var_name, AstMaybeIdx *idxinfo, AstInitVal *ast_initval):
            name(var_name), idxinfo(idxinfo), ast_initval_or_null(ast_initval), initval(),
            type(VarInt), is_const(false), pos(DefUnknown), index(-1) {}
    void calc_initval();
    void gen_eeyore_decl();
    void gen_eeyore_init(bool is_global);
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

    // in tree completing phase
    vector<AstDef*> defs_inside;

    AstFuncDef(FuncType type, string func_name, AstFuncDefParams *params, AstBlock *body):
        type(type), name(func_name), params(params), body(body) {}
    void gen_eeyore();
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
    void gen_eeyore();
};

struct AstBlock: Ast {
    vector<Ast*> body; // Decl, Stmt

    AstBlock() {}
    void push_val(Ast *next);
    void gen_eeyore();
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

    // in gen eeyore phase
    int ltest;
    int ldone;

    AstStmtWhile(AstExp *cond, AstStmt *body): AstStmt(StmtWhile),
        cond(cond), body(body), ltest(-1), ldone(-1) {}
    void gen_eeyore() override;
};

struct AstStmtBreak: AstStmt {
    // in tree completing phase
    AstStmtWhile *loop;

    AstStmtBreak(): AstStmt(StmtBreak), loop(nullptr) {}
    void gen_eeyore() override;
};

struct AstStmtContinue: AstStmt {
    // in tree completing phase
    AstStmtWhile *loop;

    AstStmtContinue(): AstStmt(StmtContinue), loop(nullptr) {}
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
private:
    ConstExpResult const_cache;
    bool const_calculated;

public:
    AstExp(): const_calculated(false), const_cache(0) {}

    virtual ConstExpResult calc_const() = 0;
    virtual ConstOrVar gen_eeyore() = 0; // return tmp label
    ConstExpResult get_const() {
        if(!const_calculated) {
            const_cache = calc_const();
            const_calculated = true;
        }
        return const_cache;
    }
};

struct AstExpLVal: AstExp {
    string name;
    AstMaybeIdx *idxinfo;

    // in tree completing phase
    AstDef *def;
    int dim_left;

    AstExpLVal(string name, AstMaybeIdx *idxinfo):
        name(name), idxinfo(idxinfo),
        def(nullptr), dim_left(-1) {}
    ConstExpResult calc_const() override;
    ConstOrVar gen_eeyore() override;
};

struct AstExpLiteral: AstExp {
    int val;

    AstExpLiteral(int val): val(val) {}
    ConstExpResult calc_const() override;
    ConstOrVar gen_eeyore() override;
};

struct AstExpFunctionCall: AstExp {
    string name;
    AstFuncUseParams *params;

    // in tree completing phase
    AstFuncDef *def;

    AstExpFunctionCall(string name, AstFuncUseParams *params):
        name(name), params(params),
        def(nullptr) {}
    ConstExpResult calc_const() override;
    ConstOrVar gen_eeyore() override;
};

struct AstExpOpUnary: AstExp {
    UnaryOpKinds op;
    AstExp *operand;

    AstExpOpUnary(UnaryOpKinds op, AstExp *operand):
        op(op), operand(operand) {}
    ConstExpResult calc_const() override;
    ConstOrVar gen_eeyore() override;
};

struct AstExpOpBinary: AstExp {
    BinaryOpKinds op;
    AstExp *operand1;
    AstExp *operand2;

    AstExpOpBinary(BinaryOpKinds op, AstExp *operand1, AstExp *operand2):
        op(op), operand1(operand1), operand2(operand2) {}
    ConstExpResult calc_const() override;
    ConstOrVar gen_eeyore() override;
};