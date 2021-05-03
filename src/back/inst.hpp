#pragma once

#include <string>
#include <list>
#include <unordered_map>
#include <utility> // make_pair
using std::string;
using std::list;
using std::unordered_map;
using std::make_pair;

#include "../main/common.hpp"
#include "../main/gc.hpp"
#include "reg.hpp"
#include "../front/enum_defs.hpp"

///// FORWARD DECL

struct InstStmt;

///// BASE

struct Inst: GarbageCollectable {
    Inst(): GarbageCollectable() {}
};

///// LANGUAGE CONSTRUCTS

struct InstDeclScalar: Inst {
    int globalidx;
    int initval;

    InstDeclScalar(int globalidx):
        globalidx(globalidx), initval(0) {}

    void output_tigger(list<string> &buf);
    void output_asm(list<string> &buf);
};

struct InstDeclArray: Inst {
    int globalidx;
    int totbytes;
    unordered_map<int, int> initval; // offset bytes -> val

    InstDeclArray(int globalidx, int totbytes):
            globalidx(globalidx), totbytes(totbytes), initval({}) {
        assert(totbytes % 4 == 0);
    }

    void output_tigger(list<string> &buf);
    void output_asm(list<string> &buf);
};

struct InstFuncDef: Inst {
    string name;
    int params_count;
    int stacksize; // in words
    list<InstStmt*> stmts;

    bool isleaf;

    InstFuncDef(string name, int params_count, int stacksize):
        name(name), params_count(params_count), stacksize(stacksize), isleaf(true) {}

    void push_stmt(InstStmt *stmt) {
        stmts.push_back(stmt);
    }

    InstStmt *get_last_stmt();

    void output_tigger(list<string> &buf);
    void output_asm(list<string> &buf);
};

struct InstRoot: Inst {
    unordered_map<int, InstDeclScalar*> decl_scalars;
    unordered_map<int, InstDeclArray*> decl_arrays;
    list<InstFuncDef*> funcs;
    InstFuncDef* mainfunc;

    void push_decl(int idx, InstDeclScalar *decl) {
        decl_scalars.insert(make_pair(idx, decl));
    }
    void push_decl(int idx, InstDeclArray *decl) {
        decl_arrays.insert(make_pair(idx, decl));
    }
    void push_func(InstFuncDef *func) {
        funcs.push_back(func);
        if(func->name=="main")
            mainfunc = func;
    }

    void output_tigger(list<string> &buf);
    void output_asm(list<string> &buf);
};

///// STATEMENT

struct InstStmt: Inst {
    virtual void output_tigger(list<string> &buf) = 0;
    virtual void output_asm(list<string> &buf) = 0;
};

struct InstOpBinary: InstStmt {
    Preg dest;
    Preg operand1;
    BinaryOpKinds op;
    Preg operand2;

    InstOpBinary(Preg dest, Preg operand1, BinaryOpKinds op, Preg operand2):
        dest(dest), operand1(operand1), op(op), operand2(operand2) {}

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstOpUnary: InstStmt {
    Preg dest;
    UnaryOpKinds op;
    Preg operand;

    InstOpUnary(Preg dest, UnaryOpKinds op, Preg operand):
        dest(dest), op(op), operand(operand) {}

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstMov: InstStmt {
    Preg dest;
    Preg src;

    InstMov(Preg dest, Preg src):
        dest(dest), src(src) {}

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstLoadImm: InstStmt {
    Preg dest;
    int imm;

    InstLoadImm(Preg dest, int imm):
        dest(dest), imm(imm) {}

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstArraySet: InstStmt {
    Preg dest;
    int doffset;
    Preg src;

    InstArraySet(Preg dest, int doffset, Preg src):
        dest(dest), doffset(doffset), src(src) {
        assert(!imm_overflows(doffset));
    }

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstArrayGet: InstStmt {
    Preg dest;
    Preg src;
    int soffset;

    InstArrayGet(Preg dest, Preg src, int soffset):
        dest(dest), src(src), soffset(soffset) {
        assert(src!=Preg('t', 0)); // t0 used as temp
    }

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstCondGoto: InstStmt {
    Preg operand1;
    RelKinds op;
    Preg operand2;
    int label;

    InstCondGoto(Preg operand1, RelKinds op, Preg operand2, int label):
        operand1(operand1), op(op), operand2(operand2), label(label) {}

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstGoto: InstStmt {
    int label;

    InstGoto(int label):
        label(label) {}

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstLabel: InstStmt {
    int label;

    InstLabel(int label):
        label(label) {}

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstCall: InstStmt {
    string name;

    InstCall(string name):
        name(name) {}

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstRet: InstStmt {
    InstFuncDef *func;
    InstRet(InstFuncDef *func):
        func(func) {}

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstStoreStack: InstStmt {
    int stackidx;
    Preg src;

    InstStoreStack(int stackidx, Preg src):
        stackidx(stackidx), src(src) {}

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstLoadStack: InstStmt {
    Preg dest;
    int stackidx;

    InstLoadStack(Preg dest, int stackidx):
        dest(dest), stackidx(stackidx) {}

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstLoadGlobal: InstStmt {
    Preg dest;
    int globalidx;

    InstLoadGlobal(Preg dest, int globalidx):
        dest(dest), globalidx(globalidx) {}

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstLoadAddrStack: InstStmt {
    Preg dest;
    int stackidx;

    InstLoadAddrStack(Preg dest, int stackidx):
        dest(dest), stackidx(stackidx) {
        assert(stackidx>=0);
    }

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstLoadAddrGlobal: InstStmt {
    Preg dest;
    int globalidx;

    InstLoadAddrGlobal(Preg dest, int globalidx):
        dest(dest), globalidx(globalidx) {}

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstComment: InstStmt {
    string comment;

    InstComment(string comment):
        comment(comment) {}

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstAddI: InstStmt {
    Preg dest;
    Preg operand1;
    int operand2;

    InstAddI(Preg dest, Preg operand1, int operand2):
        dest(dest), operand1(operand1), operand2(operand2) {
        assert(!imm_overflows(operand2));
    }

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};

struct InstLeftShiftI: InstStmt {
    Preg dest;
    Preg operand1;
    int operand2;

    InstLeftShiftI(Preg dest, Preg operand1, int operand2):
        dest(dest), operand1(operand1), operand2(operand2) {
        assert(operand2<=31 && operand2>=-31);
    }

    void output_tigger(list<string> &buf) override;
    void output_asm(list<string> &buf) override;
};
