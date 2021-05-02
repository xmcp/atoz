#include "inst.hpp"

const int TIGGER_INST_BUFSIZE = 512;

static char instbuf[TIGGER_INST_BUFSIZE];

#define outasm(...) do { \
    sprintf_s(instbuf, sizeof(instbuf), __VA_ARGS__); \
    buf.push_back(string(instbuf)); \
} while(0)

#define outstmt(...) do { \
    outasm("    " __VA_ARGS__); \
} while(0)

void InstRoot::output_tigger(list<string> &buf) {
    outasm("// BEGIN TIGGER");

    outasm("//--- SCALAR DECL");
    for(auto scalarpair: decl_scalars)
        scalarpair.second->output_tigger(buf);

    outasm("");
    outasm("//--- ARRAY DECL");
    for(auto arraypair: decl_arrays)
        arraypair.second->output_tigger(buf);

    outasm("");
    outasm("//--- FUNCTIONS");
    for(auto fn: funcs) {
        fn->output_tigger(buf);
        outasm("");
    }

    outasm("// END TIGGER");
}

void InstDeclScalar::output_tigger(list<string> &buf) {
    outasm("v%d = %d", globalidx, initval);
}

void InstDeclArray::output_tigger(list<string> &buf) {
    outasm("v%d = malloc %d", globalidx, totbytes);
}

void InstFuncDef::output_tigger(list<string> &buf) {
    outasm("f_%s [%d] [%d]", name.c_str(), params_count, stacksize);

    for(auto stmt: stmts)
        stmt->output_tigger(buf);

    outasm("end f_%s", name.c_str());
}

#define tig(p) (p.tigger_ref().c_str())

void InstOpBinary::output_tigger(list<string> &buf) {
    outstmt("%s = %s %s %s", tig(dest), tig(operand1), cvt_from_binary(op).c_str(), tig(operand2));
}

void InstOpUnary::output_tigger(list<string> &buf) {
    outstmt("%s = %s %s", tig(dest), cvt_from_unary(op).c_str(), tig(operand));
}

void InstMov::output_tigger(list<string> &buf) {
    outstmt("%s = %s", tig(dest), tig(src));
}

void InstLoadImm::output_tigger(list<string> &buf) {
    outstmt("%s = %d", tig(dest), imm);
}

void InstArraySet::output_tigger(list<string> &buf) {
    outstmt("%s [%d] = %s", tig(dest), doffset, tig(src));
}

void InstArrayGet::output_tigger(list<string> &buf) {
    outstmt("%s = %s [%d]", tig(dest), tig(src), soffset);
}

void InstCondGoto::output_tigger(list<string> &buf) {
    outstmt("if %s %s %s goto l%d", tig(operand1), cvt_from_binary(op).c_str(), tig(operand2), label);
}

void InstGoto::output_tigger(list<string> &buf) {
    outstmt("goto l%d", label);
}

void InstLabel::output_tigger(list<string> &buf) {
    outstmt("l%d:", label);
}

void InstCall::output_tigger(list<string> &buf) {
    outstmt("call f_%s", name.c_str());
}

void InstRet::output_tigger(list<string> &buf) {
    outstmt("return");
}

void InstStoreStack::output_tigger(list<string> &buf) {
    outstmt("store %s %d", tig(src), stackidx);
}

void InstLoadStack::output_tigger(list<string> &buf) {
    outstmt("load %d %s", stackidx, tig(dest));
}

void InstLoadGlobal::output_tigger(list<string> &buf) {
    outstmt("load v%d %s", globalidx, tig(dest));
}

void InstLoadAddrStack::output_tigger(list<string> &buf) {
    outstmt("loadaddr %d %s", stackidx, tig(dest));
}

void InstLoadAddrGlobal::output_tigger(list<string> &buf) {
    outstmt("loadaddr v%d %s", globalidx, tig(dest));
}

void InstComment::output_tigger(list<string> &buf) {
    if(comment.empty())
        outasm("");
    else
        outasm("// %s", comment.c_str());
}

#undef tig
#undef outasm

