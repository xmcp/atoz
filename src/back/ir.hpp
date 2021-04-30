#pragma once

#include <vector>
#include <list>
#include <utility> // pair
using std::vector;
using std::list;
using std::pair;
using std::make_pair;

#include "front/enum_defs.hpp"

#define Commented(x) pair<x, string>

///// FORWARD DECL

struct Ir;
struct IrRoot;
struct IrStmt;
struct AstDef;

///// BASE

static vector<Ir*> allocated_ir_ptrs;

struct Ir {
    Ir() {
        allocated_ir_ptrs.push_back(this);
    }
    virtual ~Ir() = default;

    static void delete_all() {
        for(auto ptr: allocated_ir_ptrs)
            delete ptr;
    }
};

struct LVal {
    enum LValType {
        Reference, TempVar
    } type;
    union {
        AstDef *reference;
        int tempvar;
    } val;

private:
    LVal(LValType type): type(type), val({}) {}

public:
    LVal(AstDef *def): type(Reference), val({.reference=def}) {}

    string eeyore_ref();
    static LVal asTempVar(int tidx) {
        LVal ret(TempVar);
        ret.val.tempvar = tidx;
        return ret;
    }
};

struct RVal {
    enum RValType {
        Reference, ConstExp, TempVar
    } type;
    union {
        AstDef *reference;
        int constexp;
        int tempvar;
    } val;

private:
    RVal(RValType type): type(type), val({}) {}

public:
    RVal(LVal lval):
        type(lval.type==LVal::Reference ? Reference : TempVar), val({}) {
        if(lval.type==LVal::Reference)
            val.reference = lval.val.reference;
        else
            val.tempvar = lval.val.tempvar;
    }
    RVal(AstDef *def): type(Reference), val({.reference=def}) {}

    string eeyore_ref();
    static RVal asConstExp(int val) {
        RVal ret(ConstExp);
        ret.val.constexp = val;
        return ret;
    }
    static RVal asTempVar(int tidx) {
        RVal ret(TempVar);
        ret.val.tempvar = tidx;
        return ret;
    }
};

///// LANGUAGE CONSTRUCTS

struct IrDecl: Ir {
    LVal dest;
    bool is_array;
    int size_bytes;

    IrDecl(LVal dest):
        dest(dest), is_array(false), size_bytes(-1) {}
    IrDecl(LVal dest, int elems):
        dest(dest), is_array(true), size_bytes(elems*4) {}

    void output_eeyore(list<string> &buf);
};

struct IrInit: Ir {
    LVal dest;
    bool is_array;
    int offset_bytes;
    int val;

    IrInit(LVal dest, int val):
        dest(dest), is_array(false), offset_bytes(-1), val(val) {}
    IrInit(LVal dest, int idx, int val):
        dest(dest), is_array(true), offset_bytes(idx*4), val(val) {}

    void output_eeyore(list<string> &buf);
};

struct IrDeclContainer: Ir {
    list<Commented(IrDecl*)> decls;

    IrDeclContainer():
        decls({}) {}
    void push_decl(IrDecl *decl, string comment = "") {
        decls.push_back(make_pair(decl, comment));
    }
};

struct IrFuncDef: IrDeclContainer {
    string name;
    int args;
    list<Commented(IrStmt*)> stmts;

    // used in gen_label and gen_scalar_tempvar
    IrRoot *root;

    IrFuncDef(IrRoot *root, string name, int args):
        root(root), IrDeclContainer(), name(name), args(args), stmts({}) {}
    void push_stmt(IrStmt *stmt, string comment = "") {
        stmts.push_back(make_pair(stmt, comment));
    }

    int gen_label();
    LVal gen_scalar_tempvar();

    void output_eeyore(list<string> &buf);
};

struct IrRoot: IrDeclContainer {
    list<Commented(IrInit*)> inits;
    list<Commented(IrFuncDef*)> funcs;
    int tempvar_top;
    int label_top;

    IrRoot():
        IrDeclContainer(), inits({}), funcs({}), tempvar_top(0), label_top(0) {}

    void push_init(IrInit *init, string comment = "") {
        inits.push_back(make_pair(init, comment));
    }
    void push_func(IrFuncDef *func, string comment = "") {
        funcs.push_back(make_pair(func, comment));
    }

    // proxied by IrFuncDef
    int _gen_label() {
        return ++label_top;
    }
    int _gen_tempvar() {
        int tvar = ++tempvar_top;
        return tvar;
    }

    void output_eeyore(list<string> &buf);
};

///// STATEMENT

struct IrStmt: Ir {
    virtual void output_eeyore(list<string> &buf) = 0;
};

struct IrOpBinary: IrStmt {
    LVal dest;
    RVal operand1;
    BinaryOpKinds op;
    RVal operand2;

    IrOpBinary(LVal dest, RVal operand1, BinaryOpKinds op, RVal operand2):
        dest(dest), operand1(operand1), op(op), operand2(operand2) {}

    void output_eeyore(list<string> &buf) override;
};

struct IrOpUnary: IrStmt {
    LVal dest;
    UnaryOpKinds op;
    RVal operand;

    IrOpUnary(LVal dest, UnaryOpKinds op, RVal operand):
        dest(dest), op(op), operand(operand) {}

    void output_eeyore(list<string> &buf) override;
};

struct IrMov: IrStmt {
    LVal dest;
    RVal src;

    IrMov(LVal dest, RVal src):
        dest(dest), src(src) {}

    void output_eeyore(list<string> &buf) override;
};

struct IrArraySet: IrStmt {
    LVal dest;
    RVal doffset;
    RVal src;

    IrArraySet(LVal dest, RVal doffset, RVal src):
        dest(dest), doffset(doffset), src(src) {}

    void output_eeyore(list<string> &buf) override;
};

struct IrArrayGet: IrStmt {
    LVal dest;
    RVal src;
    RVal soffset;

    IrArrayGet(LVal dest, RVal src, RVal soffset):
        dest(dest), src(src), soffset(soffset) {}

    void output_eeyore(list<string> &buf) override;
};


struct IrCondGoto: IrStmt {
    RVal operand1;
    BinaryOpKinds op; // should be logical op
    RVal operand2;
    int label;

    IrCondGoto(RVal operand1, BinaryOpKinds op, RVal operand2, int label):
        operand1(operand1), op(op), operand2(operand2), label(label) {}

    void output_eeyore(list<string> &buf) override;
};

struct IrGoto: IrStmt {
    int label;

    IrGoto(int label):
        label(label) {}

    void output_eeyore(list<string> &buf) override;
};

struct IrLabel: IrStmt {
    int label;

    IrLabel(int label):
        label(label) {}

    void output_eeyore(list<string> &buf) override;
};

struct IrParam: IrStmt {
    RVal param;

    IrParam(RVal param):
        param(param) {}

    void output_eeyore(list<string> &buf) override;
};

struct IrCallVoid: IrStmt {
    string name;

    IrCallVoid(string fn):
        name(fn) {}

    void output_eeyore(list<string> &buf) override;
};

struct IrCall: IrStmt {
    string name;
    LVal ret;

    IrCall(LVal ret, string fn):
        ret(ret), name(fn) {}

    void output_eeyore(list<string> &buf) override;
};

struct IrReturnVoid: IrStmt {
    IrReturnVoid() {}

    void output_eeyore(list<string> &buf) override;
};

struct IrReturn: IrStmt {
    RVal retval;
    IrReturn(RVal retval):
        retval(retval) {}

    void output_eeyore(list<string> &buf) override;
};