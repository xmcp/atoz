#include <cstdio>
using std::sprintf;

#include "back/ir.hpp"
#include "front/ast.hpp"

const bool EEYORE_GEN_COMMENTS = true;

const int EEYORE_INST_BUFSIZE = 512;
static char instbuf[EEYORE_INST_BUFSIZE];

#define cdef(def) ((def)->pos==DefArg ? 'p' : 'T')

string RVal::eeyore_ref() {
    char buf[16];
    switch(type) {
        case Reference: sprintf(buf, "%c%d", cdef(val.reference), val.reference->index); break;
        case ConstExp: sprintf(buf, "%d", val.constexp); break;
        case TempVar: sprintf(buf, "t%d", val.tempvar); break;
    }
    return string(buf);
}

string LVal::eeyore_ref() {
    char buf[16];
    switch(type) {
        case Reference: sprintf(buf, "%c%d", cdef(val.reference), val.reference->index); break;
        case TempVar: sprintf(buf, "t%d", val.tempvar); break;
    }
    return string(buf);
}

#undef cdef

#define outasm(...) do { \
    sprintf(instbuf, __VA_ARGS__); \
    buf.push_back(string(instbuf)); \
} while(0)

#define outstmt(...) do { \
    outasm("    " __VA_ARGS__); \
} while(0)

#define outcomment(fmt, ...) do { \
    sprintf(instbuf, "%s // " fmt, buf.back().c_str(), __VA_ARGS__); \
    buf.pop_back(); \
    buf.push_back(string(instbuf)); \
} while(0)

#define eey(v) ((v).eeyore_ref().c_str())

void IrDecl::output_eeyore(list<string> &buf) {
    if(is_array)
        outasm("var %d %s", size_bytes, eey(dest));
    else
        outasm("var %s", eey(dest));
}

void IrInit::output_eeyore(list<string> &buf) {
    if(is_array)
        outasm("%s [%d] = %d", eey(dest), offset_bytes, val);
    else
        outasm("%s = %d", eey(dest), val);
}

void IrFuncDef::output_eeyore(list<string> &buf) {
    outasm("f_%s [%d]", name.c_str(), args);

    for(auto decl: decls) {
        decl.first->output_eeyore(buf);
        if(EEYORE_GEN_COMMENTS && !decl.second.empty())
            outcomment("local: %s", decl.second.c_str());
    }

    for(auto stmt: stmts) {
        stmt.first->output_eeyore(buf);
        if(EEYORE_GEN_COMMENTS && !stmt.second.empty())
            outcomment("stmt: %s", stmt.second.c_str());
    }

    outasm("end f_%s", name.c_str());
}

void IrRoot::output_eeyore(list<string> &buf) {
    outasm("// BEGIN EEYORE");

    outasm("//--- GLOBAL DECL");
    for(auto decl: decls) {
        decl.first->output_eeyore(buf);
        if(EEYORE_GEN_COMMENTS && !decl.second.empty())
            outcomment("global: %s", decl.second.c_str());
    }

    outasm("");
    outasm("//--- GLOBAL INIT");
    for(auto init: inits) {
        init.first->output_eeyore(buf);
        if(EEYORE_GEN_COMMENTS && !init.second.empty())
            outcomment("init: %s", init.second.c_str());
    }

    outasm("");
    outasm("//--- FUNCTIONS");
    for(auto func: funcs) {
        func.first->output_eeyore(buf);
        if(EEYORE_GEN_COMMENTS && !func.second.empty())
            outcomment("func: %s", func.second.c_str());
        outasm("");
    }

    outasm("// END EEYORE");
}

void IrOpBinary::output_eeyore(list<string> &buf) {
    outstmt("%s = %s %s %s", eey(dest), eey(operand1), cvt_from_binary(op).c_str(), eey(operand2));
}

void IrOpUnary::output_eeyore(list<string> &buf) {
    outstmt("%s = %s %s", eey(dest), cvt_from_unary(op).c_str(), eey(operand));
}

void IrMov::output_eeyore(list<string> &buf) {
    outstmt("%s = %s", eey(dest), eey(src));
}

void IrArraySet::output_eeyore(list<string> &buf) {
    outstmt("%s [%s] = %s", eey(dest), eey(doffset), eey(src));
}

void IrArrayGet::output_eeyore(list<string> &buf) {
    outstmt("%s = %s [%s]", eey(dest), eey(src), eey(soffset));
}

void IrCondGoto::output_eeyore(list<string> &buf) {
    outstmt("if %s %s %s goto l%d", eey(operand1), cvt_from_binary(op).c_str(), eey(operand2), label);
}

void IrGoto::output_eeyore(list<string> &buf) {
    outstmt("goto l%d", label);
}

void IrLabel::output_eeyore(list<string> &buf) {
    outstmt("l%d:", label);
}

void IrParam::output_eeyore(list<string> &buf) {
    outstmt("param %s", eey(param));
}

void IrCallVoid::output_eeyore(list<string> &buf) {
    outstmt("call f_%s", name.c_str());
}

void IrCall::output_eeyore(list<string> &buf) {
    outstmt("%s = call f_%s", eey(ret), name.c_str());
}

void IrReturnVoid::output_eeyore(list<string> &buf) {
    outstmt("return");
}

void IrReturn::output_eeyore(list<string> &buf) {
    outstmt("return %s", eey(retval));
}

#undef eey