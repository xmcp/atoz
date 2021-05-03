#include "../main/common.hpp"
#include "inst.hpp"

const int TIGGER_INST_BUFSIZE = 512;

static char instbuf[TIGGER_INST_BUFSIZE];

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

void InstAddI::output_tigger(list<string> &buf) {
    if(dest==operand1 && operand2==0)
        outstmt("//%s = %s + 0", tig(dest), tig(operand1));
    else
        outstmt("%s = %s + %d", tig(dest), tig(operand1), operand2);
}

void InstLeftShiftI::output_tigger(list<string> &buf) {
    if(operand2>0)
        outstmt("%s = %s * %d // shift left", tig(dest), tig(operand1), 1<<operand2);
    else if(operand2<0)
        outstmt("%s = %s / %d // shift right", tig(dest), tig(operand1), 1<<(-operand2));
    else
        outstmt("%s = %s // shift 0", tig(dest), tig(operand1));
}

void InstLeftShift::output_tigger(list<string> &buf) {
    // tigger does not support this
    outstmt("!! %s = %s << %s", tig(dest), tig(operand1), tig(operand2));
}

#undef tig
#undef outasm

