#include <cstdio>
#include <sstream>
using std::sprintf;
using std::stringstream;

#include "../main/common.hpp"
#include "../back/ir.hpp"
#include "ast.hpp"

const bool EEYORE_GEN_COMMENTS = true;
bool OUTPUT_REGALLOC_PREFIX = true;
bool OUTPUT_DEF_USE = true;
const int EEYORE_INST_BUFSIZE = 512;

static char instbuf[EEYORE_INST_BUFSIZE];

#define cdef(def) ((def)->pos==DefArg ? 'p' : 'T')

string RVal::eeyore_ref_global() {
    char buf[32];
    switch(type) {
        case ConstExp: sprintf(buf, "%d", val.constexp); break;
        case Reference: sprintf(buf, "%c%d", cdef(val.reference), val.reference->index); break;
        case TempVar: sprintf(buf, "t%d", val.tempvar); break;
    }
    return string(buf);
}
string RVal::eeyore_ref_local(IrFuncDef *func) {
    string pfx;
    char buf[32];

    if(OUTPUT_REGALLOC_PREFIX && type != ConstExp) {
        if(type==Reference && val.reference->pos==DefGlobal)
            pfx = "{global}";
        else {
            auto it = func->vreg_map.find(reguid());
            /* // flag:return-label
            if(type==TempVar && val.tempvar==func->_eeyore_retval_var.val.tempvar)
                pfx = "{retval}";
            else */
            if(it==func->vreg_map.end())
                pfx = "{???}";
            else
                pfx = it->second.analyzed_eeyore_ref();
        }
    }

    switch(type) {
        case Reference: sprintf(buf, "%c%d", cdef(val.reference), val.reference->index); break;
        case ConstExp: sprintf(buf, "%d", val.constexp); break;
        case TempVar: sprintf(buf, "t%d", val.tempvar); break;
    }
    return pfx + string(buf);
}


string LVal::eeyore_ref_global() {
    char buf[32];
    switch(type) {
        case Reference: sprintf(buf, "%c%d", cdef(val.reference), val.reference->index); break;
        case TempVar: sprintf(buf, "t%d", val.tempvar); break;
    }
    return string(buf);
}
string LVal::eeyore_ref_local(IrFuncDef *func) {
    string pfx;
    char buf[32];

    if(OUTPUT_REGALLOC_PREFIX) {
        if(type==Reference && val.reference->pos==DefGlobal)
            pfx = "{global}";
        else {
            auto it = func->vreg_map.find(reguid());
            /* // flag:return-label
            if(type==TempVar && val.tempvar==func->_eeyore_retval_var.val.tempvar)
                pfx = "{retval}";
            else */
            if(it==func->vreg_map.end())
                pfx = "{???}";
            else
                pfx = it->second.analyzed_eeyore_ref();
        }
    }

    switch(type) {
        case Reference: sprintf(buf, "%c%d", cdef(val.reference), val.reference->index); break;
        case TempVar: sprintf(buf, "t%d", val.tempvar); break;
    }
    return pfx + string(buf);
}

#undef cdef

#define outcomment(fmt, ...) do { \
    snprintf(instbuf, sizeof(instbuf), "%s // " fmt, buf.back().c_str(), __VA_ARGS__); \
    buf.pop_back(); \
    buf.push_back(string(instbuf)); \
} while(0)

#define eey(v) ((v).eeyore_ref_local(func).c_str())

void IrRoot::output_eeyore(list<string> &buf) {
    outasm("// BEGIN EEYORE");

    outasm("//--- GLOBAL DECL");
    for(const auto& decl: decls) {
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

void IrDecl::output_eeyore(list<string> &buf) {
    if(def_or_null!=nullptr && def_or_null->idxinfo->dims() > 0) // array var
        outasm("var %d %s", def_or_null->initval.totelems*4, dest.eeyore_ref_global().c_str());
    else
        outasm("var %s", dest.eeyore_ref_global().c_str());
}

void IrInit::output_eeyore(list<string> &buf) {
    if(def->idxinfo->dims()>0) { // array init
        assert(offset_bytes>=0);
        outasm("%s [%d] = %d", dest.eeyore_ref_global().c_str(), offset_bytes, val);
    }
    else { // scalar init
        assert(offset_bytes==-1);
        outasm("%s = %d", dest.eeyore_ref_global().c_str(), val);
    }
}

void IrFuncDef::output_eeyore(list<string> &buf) {
    outasm("f_%s [%d]", name.c_str(), (int)params->val.size());

    for(const auto& decl: decls) {
        decl.first->output_eeyore(buf);
        if(EEYORE_GEN_COMMENTS && !decl.second.empty())
            outcomment("local: %s", decl.second.c_str());
    }

    for(const auto& stmt: stmts) {
        stmt.first->output_eeyore(buf);
        if(EEYORE_GEN_COMMENTS && !stmt.second.empty())
            outcomment("stmt: %s", stmt.second.c_str());
        if(OUTPUT_DEF_USE) {
            stringstream ss;
            ss << "DEF: ";
            for(int def: stmt.first->defs())
                ss << demystify_reguid(def) << ' ';
            ss << "| USE: ";
            for(int use: stmt.first->uses())
                ss << demystify_reguid(use) << ' ';
            ss << "| ALIVE: ";
            for(int alive: stmt.first->alive_pooled_vars)
                ss << demystify_reguid(alive) << ' ';
            ss << "| MEET: ";
            for(int meet: stmt.first->meet_pooled_vars)
                ss << demystify_reguid(meet) << ' ';
            outcomment("\n//    ____  %s", ss.str().c_str());
        }
    }

    outasm("end f_%s", name.c_str());
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
    outstmt("%s [%d] = %s", eey(dest), doffset, eey(src));
}

void IrArrayGet::output_eeyore(list<string> &buf) {
    outstmt("%s = %s [%d]", eey(dest), eey(src), soffset);
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

void IrParam::output_eeyore_handled_by_call(list<string> &buf) {
    outstmt("param %s", eey(param));
    outcomment("#%d", pidx);
}

void IrCallVoid::output_eeyore(list<string> &buf) {
    for(auto param: params)
        param->output_eeyore_handled_by_call(buf);
    outstmt("call f_%s", name.c_str());
}

void IrCall::output_eeyore(list<string> &buf) {
    for(auto param: params)
        param->output_eeyore_handled_by_call(buf);
    outstmt("%s = call f_%s", eey(ret), name.c_str());
}

void IrReturnVoid::output_eeyore(list<string> &buf) {
    /* // flag:return-label
    outstmt("goto l%d", func->return_label);
    */
    outstmt("return");
}

void IrReturn::output_eeyore(list<string> &buf) {
    /* // flag:return-label
    outstmt("%s = %s", eey(func->_eeyore_retval_var), eey(retval));
    outstmt("goto l%d", func->return_label);
    */
    outstmt("return %s", eey(retval));
}

/* // flag:return-label
void IrLabelReturn::output_eeyore(list<string> &buf) {
    outstmt("l%d:", func->return_label);
    if(func->type==FuncVoid)
        outstmt("return");
    else
        outstmt("return %s", eey(func->_eeyore_retval_var));
}
*/

void IrLocalArrayFillZero::output_eeyore(list<string> &buf) {
    outstmt("//[ local_array_fill_zero %s ]", eey(dest));
}

#undef eey