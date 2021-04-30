#pragma once

#include <vector>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <utility> // pair
using std::vector;
using std::list;
using std::unordered_set;
using std::unordered_map;
using std::pair;
using std::make_pair;

#include "front/enum_defs.hpp"
#include "reg.hpp"

#define Commented(x) pair<x, string>

#define cfgerror(...) do { \
    printf("connect cfg error: "); \
    printf(__VA_ARGS__ ); \
    exit(1); \
} while(0)

///// FORWARD DECL

struct Ir;
struct IrRoot;
struct IrFuncDef;
struct IrStmt;
struct IrLabel;
struct AstDef;
struct AstFuncDefParams;

///// BASE

const int REGUID_ARG_OFFSET = 10000;
string demystify_reguid(int uid);

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

    static LVal asTempVar(int tidx) {
        LVal ret(TempVar);
        ret.val.tempvar = tidx;
        return ret;
    }

    string eeyore_ref_global();
    string eeyore_ref_local(IrFuncDef *func);
    bool regpooled();
    int reguid();
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

    string eeyore_ref_local(IrFuncDef *func);
    bool regpooled();
    int reguid();
};

///// LANGUAGE CONSTRUCTS

struct IrDecl: Ir {
    AstDef *def_or_null; // null for tempvar
    LVal dest;

    IrDecl(AstDef *def):
        def_or_null(def), dest(def) {}
    explicit IrDecl(int tempvar):
        def_or_null(nullptr), dest(LVal::asTempVar(tempvar)) {}

    void output_eeyore(list<string> &buf);
};

struct IrInit: Ir {
    LVal dest;
    AstDef *def;
    int offset_bytes;
    int val;

    IrInit(AstDef *def, int val):
        dest(def), def(def), offset_bytes(-1), val(val) {}
    IrInit(AstDef *def, int idx, int val):
        dest(def), def(def), offset_bytes(idx*4), val(val) {}

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
    IrRoot *root;

    FuncType type;
    string name;
    AstFuncDefParams *params;
    list<Commented(IrStmt*)> stmts;

    int return_label;
    LVal _eeyore_retval_var; // used in gen_eeyore, this tempvar is not in cfg because tigger doesn't need it

    int spillsize; // in words
    int callersavesize; // in words, initialized in `report_destroyed_set`

    IrFuncDef(IrRoot *root, FuncType type, string name, AstFuncDefParams *params): IrDeclContainer(),
       root(root), type(type), name(name), params(params), stmts({}), spillsize(0), callersavesize(0),
       return_label(gen_label()), _eeyore_retval_var(gen_scalar_tempvar()) {}
    void push_stmt(IrStmt *stmt, string comment = "");

    int gen_label();
    LVal gen_scalar_tempvar();

    void output_eeyore(list<string> &buf);

    // cfg

    unordered_map<int, IrLabel*> labels; // label id -> ir node
    unordered_map<int, Vreg> vreg_map; // def uid -> vreg
    unordered_map<int, AstDef*> decl_map; // def uid -> def node

    void connect_all_cfg();
    void regalloc();
    void report_destroyed_set();
};

struct IrRoot: IrDeclContainer {
    list<Commented(IrInit*)> inits;
    list<Commented(IrFuncDef*)> funcs;
    int tempvar_top;
    int label_top;

    unordered_map<string, unordered_set<Preg, Preg::Hash>> destroy_sets;

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
        return label_top++;
    }
    int _gen_tempvar() {
        return tempvar_top++;
    }

    void output_eeyore(list<string> &buf);

    void install_builtin_destroy_sets();
};

///// STATEMENT

struct IrStmt: Ir {
    IrFuncDef *func;

    IrStmt(IrFuncDef *func): func(func) {}

    virtual void output_eeyore(list<string> &buf) = 0;

    // cfg
    vector<IrStmt*> next, prev;
    virtual void cfg_calc_next(IrStmt *nextline, IrFuncDef *func) { next.push_back(nextline); }
    virtual vector<int> defs() { return vector<int>(); }
    virtual vector<int> uses() { return vector<int>(); }

    // regalloc
    bool _regalloc_inqueue = false;
    unordered_set<int> alive_vars;
};

#define push_if_pooled(x) do { \
    if(x.regpooled()) \
        v.push_back(x.reguid()); \
} while(0)

struct IrOpBinary: IrStmt {
    LVal dest;
    RVal operand1;
    BinaryOpKinds op;
    RVal operand2;

    IrOpBinary(IrFuncDef *func, LVal dest, RVal operand1, BinaryOpKinds op, RVal operand2): IrStmt(func),
        dest(dest), operand1(operand1), op(op), operand2(operand2) {}

    void output_eeyore(list<string> &buf) override;

    // cfg
    vector<int> defs() override {
        auto v = vector<int>();
        push_if_pooled(dest);
        return v;
    }
    vector<int> uses() override {
        auto v = vector<int>();
        push_if_pooled(operand1);
        push_if_pooled(operand2);
        return v;
    }
};

struct IrOpUnary: IrStmt {
    LVal dest;
    UnaryOpKinds op;
    RVal operand;

    IrOpUnary(IrFuncDef *func, LVal dest, UnaryOpKinds op, RVal operand): IrStmt(func),
        dest(dest), op(op), operand(operand) {}

    void output_eeyore(list<string> &buf) override;

    // cfg
    vector<int> defs() override {
        auto v = vector<int>();
        push_if_pooled(dest);
        return v;
    }
    vector<int> uses() override {
        auto v = vector<int>();
        push_if_pooled(operand);
        return v;
    }
};

struct IrMov: IrStmt {
    LVal dest;
    RVal src;

    IrMov(IrFuncDef *func, LVal dest, RVal src): IrStmt(func),
        dest(dest), src(src) {}

    void output_eeyore(list<string> &buf) override;

    // cfg
    vector<int> defs() override {
        auto v = vector<int>();
        push_if_pooled(dest);
        return v;
    }
    vector<int> uses() override {
        auto v = vector<int>();
        push_if_pooled(src);
        return v;
    }
};

struct IrArraySet: IrStmt {
    LVal dest;
    int doffset;
    RVal src;

    IrArraySet(IrFuncDef *func, LVal dest, int doffset, RVal src): IrStmt(func),
        dest(dest), doffset(doffset), src(src) {
        assert(doffset%4==0);
    }

    void output_eeyore(list<string> &buf) override;

    // cfg
    vector<int> defs() override {
        auto v = vector<int>();
        push_if_pooled(dest);
        return v;
    }
    vector<int> uses() override {
        auto v = vector<int>();
        push_if_pooled(dest); // the ptr to array is read
        push_if_pooled(src);
        return v;
    }
};

struct IrArrayGet: IrStmt {
    LVal dest;
    RVal src;
    int soffset;

    IrArrayGet(IrFuncDef *func, LVal dest, RVal src, int soffset): IrStmt(func),
        dest(dest), src(src), soffset(soffset) {
        assert(soffset%4==0);
    }

    void output_eeyore(list<string> &buf) override;

    // cfg
    vector<int> defs() override {
        auto v = vector<int>();
        push_if_pooled(dest);
        return v;
    }
    vector<int> uses() override {
        auto v = vector<int>();
        push_if_pooled(src);
        return v;
    }
};


struct IrCondGoto: IrStmt {
    RVal operand1;
    BinaryOpKinds op; // should be logical op
    RVal operand2;
    int label;

    IrCondGoto(IrFuncDef *func, RVal operand1, BinaryOpKinds op, RVal operand2, int label): IrStmt(func),
        operand1(operand1), op(op), operand2(operand2), label(label) {}

    void output_eeyore(list<string> &buf) override;

    // cfg
    void cfg_calc_next(IrStmt *nextline, IrFuncDef *func) override {
        auto label_stmt = func->labels.find(label)->second;
        next.push_back((IrStmt*)label_stmt);
        next.push_back(nextline);
    }
    vector<int> uses() override {
        auto v = vector<int>();
        push_if_pooled(operand1);
        push_if_pooled(operand2);
        return v;
    }
};

struct IrGoto: IrStmt {
    int label;

    IrGoto(IrFuncDef *func, int label): IrStmt(func),
        label(label) {}

    void output_eeyore(list<string> &buf) override;

    // cfg
    void cfg_calc_next(IrStmt *_nextline, IrFuncDef *func) override {
        auto label_stmt = func->labels.find(label)->second;
        next.push_back((IrStmt*)label_stmt);
    }
};

struct IrLabel: IrStmt {
    int label;

    IrLabel(IrFuncDef *func, int label): IrStmt(func),
        label(label) {}

    void output_eeyore(list<string> &buf) override;
};

struct IrParam: IrStmt {
    RVal param;

    IrParam(IrFuncDef *func, RVal param): IrStmt(func),
        param(param) {}

    void output_eeyore(list<string> &buf) override;

    // cfg
    vector<int> uses() override {
        auto v = vector<int>();
        push_if_pooled(param);
        return v;
    }
};

struct IrCallVoid: IrStmt {
    string name;

    IrCallVoid(IrFuncDef *func, string fn): IrStmt(func),
        name(fn) {}

    void output_eeyore(list<string> &buf) override;
};

struct IrCall: IrStmt {
    string name;
    LVal ret;

    IrCall(IrFuncDef *func, LVal ret, string fn): IrStmt(func),
        ret(ret), name(fn) {}

    void output_eeyore(list<string> &buf) override;

    // cfg
    vector<int> defs() override {
        auto v = vector<int>();
        push_if_pooled(ret);
        return v;
    }
};

struct IrReturnVoid: IrStmt {
    IrReturnVoid(IrFuncDef *func): IrStmt(func) {}

    void output_eeyore(list<string> &buf) override;

    // cfg
    void cfg_calc_next(IrStmt *_nextline, IrFuncDef *func) override {
        auto label_stmt = func->labels.find(func->return_label)->second;
        next.push_back((IrStmt*)label_stmt);
    }
};

struct IrReturn: IrStmt {
    RVal retval;
    IrReturn(IrFuncDef *func, RVal retval): IrStmt(func),
        retval(retval) {}

    void output_eeyore(list<string> &buf) override;

    // cfg
    void cfg_calc_next(IrStmt *_nextline, IrFuncDef *func) override {
        auto label_stmt = func->labels.find(func->return_label)->second;
        next.push_back((IrStmt*)label_stmt);
    }
    vector<int> uses() override {
        auto v = vector<int>();
        push_if_pooled(retval);
        return v;
    }
};

struct IrLabelReturn: IrLabel {
    IrLabelReturn(IrFuncDef *func, int label): IrLabel(func, label) {}

    void output_eeyore(list<string> &buf) override;

    // cfg
    void cfg_calc_next(IrStmt *_nextline, IrFuncDef *_func) override {
        // connects to nothing
    }
};