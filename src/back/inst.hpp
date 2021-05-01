#pragma once

#include <string>
#include <list>
#include <unordered_map>
#include <cassert>
using std::string;
using std::list;
using std::unordered_map;

#include "../main/gc.hpp"
#include "reg.hpp"
#include "../front/enum_defs.hpp"

///// FORWARD DECL

struct InstStmt;

///// BASE

struct Inst: GarbageCollectable {
    Inst(): GarbageCollectable() {}
    virtual ~Inst() = default;
};

///// LANGUAGE CONSTRUCTS

struct InstDeclScalar: Inst {
    string name;
    int initval;

    InstDeclScalar(string name, int initval):
        name(name), initval(initval) {}
};

struct InstDeclArray: Inst {
    int globalidx;
    int bytes;
    unordered_map<int, int> initval;

    InstDeclArray(int globalidx, int bytes):
        globalidx(globalidx), bytes(bytes), initval({}) {
        assert(bytes%4==0);
    }
};

struct InstFuncDef: Inst {
    int globalidx;
    int params_count;
    int stacksize;
    list<InstStmt*> stmts;

    InstFuncDef(int globalidx, int params_count, int stacksize):
        globalidx(globalidx), params_count(params_count), stacksize(stacksize) {}

    void push_stmt(InstStmt *stmt) {
        stmts.push_back(stmt);
    }
};

struct InstRoot: Inst {
    list<InstDeclScalar*> decl_scalar;
    list<InstDeclArray*> decl_array;
    list<InstFuncDef*> funcs;

    void push_decl(InstDeclScalar *decl) {
        decl_scalar.push_back(decl);
    }
    void push_decl(InstDeclArray *decl) {
        decl_array.push_back(decl);
    }
    void push_func(InstFuncDef *func) {
        funcs.push_back(func);
    }
};

///// STATEMENT

struct InstStmt: Inst {};

struct InstOpBinary: InstStmt {
    Preg dest;
    Preg operand1;
    BinaryOpKinds op;
    Preg operand2;

    InstOpBinary(Preg dest, Preg operand1, BinaryOpKinds op, Preg operand2):
        dest(dest), operand1(operand1), op(op), operand2(operand2) {}
};

struct InstOpUnary: InstStmt {
    Preg dest;
    BinaryOpKinds op;
    Preg operand;

    InstOpUnary(Preg dest, BinaryOpKinds op, Preg operand):
        dest(dest), op(op), operand(operand) {}
};

struct InstMov: InstStmt {
    Preg dest;
    Preg src;

    InstMov(Preg dest, Preg src):
        dest(dest), src(src) {}
};

struct InstLoadImm: InstStmt {
    Preg dest;
    int imm;

    InstLoadImm(Preg dest, int imm):
        dest(dest), imm(imm) {}
};

struct InstArraySet: InstStmt {
    Preg dest;
    int doffset;
    Preg src;

    InstArraySet(Preg dest, int doffset, Preg src):
        dest(dest), doffset(doffset), src(src) {}
};

struct InstArrayGet: InstStmt {
    Preg dest;
    Preg src;
    int soffset;

    InstArrayGet(Preg dest, Preg src, int soffset):
        dest(dest), src(src), soffset(soffset) {}
};

struct InstCondGoto: InstStmt {
    Preg operand1;
    BinaryOpKinds op;
    Preg operand2;
    int label;

    InstCondGoto(Preg operand1, BinaryOpKinds op, Preg operand2, int label):
        operand1(operand1), op(op), operand2(operand2), label(label) {}
};

struct InstGoto: InstStmt {
    int label;

    InstGoto(int label):
        label(label) {}
};

struct InstLabel: InstStmt {
    int label;

    InstLabel(int label):
        label(label) {}
};

struct InstCall: InstStmt {
    string name;

    InstCall(string name):
        name(name) {}
};

struct InstRet: InstStmt {
    InstRet() {}
};

struct InstStoreStack: InstStmt {
    Preg src;
    int stackidx;

    InstStoreStack(Preg src, int stackidx):
        src(src), stackidx(stackidx) {}
};

struct InstLoadStack: InstStmt {
    int stackidx;
    Preg dest;

    InstLoadStack(int stackidx, Preg dest):
        stackidx(stackidx), dest(dest) {}
};

struct InstLoadGlobal: InstStmt {
    int globalidx;
    Preg dest;

    InstLoadGlobal(int globalidx, Preg dest):
        globalidx(globalidx), dest(dest) {}
};

struct InstLoadAddrStack: InstStmt {
    int stackidx;
    Preg dest;

    InstLoadAddrStack(int stackidx, Preg dest):
        stackidx(stackidx), dest(dest) {}
};

struct InstLoadAddrGlobal: InstStmt {
    int globalidx;
    Preg dest;

    InstLoadAddrGlobal(int globalidx, Preg dest):
        globalidx(globalidx), dest(dest) {}
};
