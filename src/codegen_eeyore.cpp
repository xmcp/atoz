#include <cstdio>
#include <string>
#include <vector>
using std::sprintf;
using std::string;
using std::vector;

#include "ast.hpp"
#include "eeyore_world.hpp"

#define istype(ptr, cls) (dynamic_cast<cls*>(ptr)!=nullptr)

#define outasm(...) do { \
    sprintf(instbuf, __VA_ARGS__); \
    eeyore_world.push_instruction(instbuf); \
} while(0)

#define cdef(def) ((def)->pos==DefArg ? 'p' : 'T')

#define generror(...) do { \
    printf("codegen eeyore error: "); \
    printf(__VA_ARGS__); \
    exit(1); \
} while(0)

static char instbuf[EEYORE_INST_BUFSIZE];

const char *ConstOrVar::eeyore_ref() {
    static int _idx = 0;
    static char bufs[16][16]; // multiple bufs because printf will eval all args before using the buf
    _idx = (_idx+1)%16;
    char *buf = &bufs[_idx][0];
    switch(type) {
        case Reference: sprintf(buf, "%c%d", cdef(val.reference), val.reference->index); break;
        case ConstExp: sprintf(buf, "%d", val.constexp); break;
        case TempVar: sprintf(buf, "t%d", val.tempvar); break;
    }
    return buf;
}

void AstCompUnit::gen_eeyore() {
    for(auto *sub: val) // Decl, FuncDef
        if(istype(sub, AstDecl))
            ((AstDecl*)sub)->gen_eeyore(true);
        else // FuncDef
            ((AstFuncDef*)sub)->gen_eeyore();
}

void AstDecl::gen_eeyore(bool is_global) {
    if(is_global) { // decl for local vars are handled in FuncDef
        for(auto *def: defs->val)
            def->gen_eeyore_decl();
    }
    for(auto *def: defs->val)
        def->gen_eeyore_init(is_global);
}

void AstDef::gen_eeyore_decl() {
    if(pos==DefArg)
        return;

    if(idxinfo->val.empty())
        outasm("var T%d // decl - primitive %s", index, name.c_str());
    else
        outasm("var %d T%d // decl - array %s", initval.totelems*4, index, name.c_str());
}

void AstDef::gen_eeyore_init(bool is_global) {
    if(idxinfo->val.empty()) { // primitive
        if(initval.value[0] != nullptr) {
            if(is_global) { // global: use constexpr init
                auto constres = initval.value[0]->get_const();
                if(constres.iserror)
                    generror("initializer for global var %s not constant", name.c_str());

                outasm("T%d = %d // init %s global", index, constres.val, name.c_str());
            } else { // local: use dynamic init
                ConstOrVar trval = initval.value[0]->gen_eeyore();
                outasm("T%d = %s // init %s local", index, trval.eeyore_ref(), name.c_str());
            }
        }
    } else { // array
        for(int i=0; i<initval.totelems; i++)
            if(initval.value[i] != nullptr) {
                if(is_global) { // global: use constexpr init
                    auto constres = initval.value[i]->get_const();
                    if(constres.iserror)
                        generror("initializer for global var %s not constant", name.c_str());

                    outasm("T%d [%d] = %d // init %s global #%d/%d", index, i*4, constres.val, name.c_str(), i, initval.totelems);
                } else { // local: use dynamic init
                    ConstOrVar trval = initval.value[i]->gen_eeyore();
                    outasm("T%d [%d] = %s // init %s local #%d/%d", index, i*4, trval.eeyore_ref(), name.c_str(), i, initval.totelems);
                }
            }
    }
}


void AstFuncDef::gen_eeyore() {
    outasm("f_%s [%d] // funcdef - begin", name.c_str(), (int)params->val.size());

    // mark begin pos
    vector<string> *old_instqueue = eeyore_world.instructions;
    int old_tempvartop = eeyore_world.temp_var_top;

    // virtualize instqueue for inner temp vars
    eeyore_world.instructions = new vector<string>();
    if(!eeyore_world.should_emit_temp_var)
        generror("nested funcdef: %s", name.c_str());
    eeyore_world.should_emit_temp_var = false;

    // gen decl for body
    for(auto *def: defs_inside)
        def->gen_eeyore_decl();

    // gen body
    outasm("// funcdef - body");
    body->gen_eeyore();

    if(type==FuncInt)
        outasm("return 0 // funcdef - end");
    else
        outasm("return // funcdef - end");
    outasm("end f_%s // funcdef - end", name.c_str());

    // undo virtualization
    vector<string> *new_instqueue = eeyore_world.instructions;
    eeyore_world.instructions = old_instqueue;
    eeyore_world.should_emit_temp_var = true;

    // gen tempvar decl
    for(int i=old_tempvartop+1; i<=eeyore_world.temp_var_top; i++)
        outasm("var t%d // tempvar in func body", i);
    
    // put func body into instqueue
    for(const auto &inst: *new_instqueue)
        eeyore_world.instructions->push_back(inst);

    delete new_instqueue;
}

void AstFuncUseParams::gen_eeyore() {
    for(auto *param: val) {
        auto *lval = dynamic_cast<AstExpLVal*>(param);
        if(lval && lval->dim_left>0) { // pass array as pointer
            AstExp *idx = lval->def->initval.getoffset_bytes(lval->idxinfo, true);
            ConstOrVar tidx = idx->gen_eeyore();
            int trval = eeyore_world.gen_temp_var();
            outasm("t%d = %c%d + %s // useparam - array access", trval, cdef(lval->def), lval->def->index, tidx.eeyore_ref());
            outasm("param t%d // useparam - array pass", trval);
        } else { // pass primitive
            ConstOrVar trval = param->gen_eeyore();
            outasm("param %s // useparam - primitive", trval.eeyore_ref());
        }
    }
}

void AstBlock::gen_eeyore() {
    for(auto *sub: body) {// Decl, Stmt
        if(istype(sub, AstStmt))
            ((AstStmt*)sub)->gen_eeyore();
        else { // Decl handled in funcdef
            ((AstDecl*)sub)->gen_eeyore(false);
        }
    }
}

void AstStmtAssignment::gen_eeyore() {
    ConstOrVar trval = rval->gen_eeyore();

    if(lval->dim_left!=0)
        generror("assignment lval got dim %d for var %s", lval->dim_left, lval->name.c_str());

    if(!lval->idxinfo->val.empty()) { // has array index
        AstExp *idx = lval->def->initval.getoffset_bytes(lval->idxinfo, true);
        ConstOrVar tlidx = idx->gen_eeyore();
        outasm("%c%d [%s] = %s // assign", cdef(lval->def), lval->def->index, tlidx.eeyore_ref(), trval.eeyore_ref());
    } else { // plain value
        outasm("%c%d = %s // assign", cdef(lval->def), lval->def->index, trval.eeyore_ref());
    }
}

void AstStmtExp::gen_eeyore() {
    exp->gen_eeyore();
}

void AstStmtVoid::gen_eeyore() {
    outasm("// stmtvoid");
}

void AstStmtBlock::gen_eeyore() {
    block->gen_eeyore();
}

void AstStmtIfOnly::gen_eeyore() {
    ConstOrVar tcond = cond->gen_eeyore();

    int lskip = eeyore_world.gen_label();
    outasm("if %s == 0 goto l%d // ifonly", tcond.eeyore_ref(), lskip);

    body->gen_eeyore();
    outasm("l%d: // ifonly - lskip", lskip);
}

void AstStmtIfElse::gen_eeyore() {
    ConstOrVar tcond = cond->gen_eeyore();

    int lfalse = eeyore_world.gen_label();
    outasm("if %s == 0 goto l%d // ifelse", tcond.eeyore_ref(), lfalse);

    body_true->gen_eeyore();
    int ldone = eeyore_world.gen_label();
    outasm("goto l%d // ifelse - truedone", ldone);

    outasm("l%d: // ifelse - lfalse", lfalse);
    body_false->gen_eeyore();

    outasm("l%d: // ifelse - ldone", ldone);
}

void AstStmtWhile::gen_eeyore() {
    ltest = eeyore_world.gen_label();
    ldone = eeyore_world.gen_label();

    outasm("l%d: // while - ltest", ltest);

    ConstOrVar tcond = cond->gen_eeyore();
    outasm("if %s == 0 goto l%d // while - done", tcond.eeyore_ref(), ldone);

    body->gen_eeyore();
    outasm("goto l%d // while - totest", ltest);
    outasm("l%d: // while - ldone", ldone);
}

void AstStmtBreak::gen_eeyore() {
    outasm("goto l%d // break", loop->ldone);
}

void AstStmtContinue::gen_eeyore() {
    outasm("goto l%d // continue", loop->ltest);
}

void AstStmtReturnVoid::gen_eeyore() {
    outasm("return");
}

void AstStmtReturn::gen_eeyore() {
    ConstOrVar tret = retval->gen_eeyore();
    outasm("return %s // return", tret.eeyore_ref());
}

ConstOrVar AstExpLVal::gen_eeyore() {
    if(dim_left!=0)
        generror("exp got dim %d for var %s", dim_left, name.c_str());

    if(!get_const().iserror) {
        return ConstOrVar::asConstExp(get_const().val);
    }

    int tval = eeyore_world.gen_temp_var();

    if(!def->idxinfo->val.empty()) {
        AstExp *idx = def->initval.getoffset_bytes(idxinfo, false);
        ConstOrVar tidx = idx->gen_eeyore();
        outasm("t%d = %c%d [%s] // lval - array", tval, cdef(def), def->index, tidx.eeyore_ref());
    } else {

        outasm("t%d = %c%d // lval - primitive", tval, cdef(def), def->index);
    }
    return ConstOrVar::asTempVar(tval);
}

ConstOrVar AstExpLiteral::gen_eeyore() {
    return ConstOrVar::asConstExp(val);
}

ConstOrVar AstExpFunctionCall::gen_eeyore() {
    params->gen_eeyore();

    // special functions
    if(name=="starttime") {
        outasm("param %d // func call special", loc.lineno);
        outasm("call f__sysy_starttime // func call special");
        return ConstOrVar::asTempVar(-1);
    }
    if(name=="stoptime") {
        outasm("param %d // func call special", loc.lineno);
        outasm("call f__sysy_stoptime // func call special");
        return ConstOrVar::asTempVar(-1);
    }

    if(def->type==FuncVoid) {
        outasm("call f_%s", name.c_str());
        return ConstOrVar::asTempVar(-1);
    } else {
        int tret = eeyore_world.gen_temp_var();
        outasm("t%d = call f_%s", tret, name.c_str());
        return ConstOrVar::asTempVar(tret);
    }
}

ConstOrVar AstExpOpUnary::gen_eeyore() {
    if(!get_const().iserror) {
        return ConstOrVar::asConstExp(get_const().val);
    }

    int tret = eeyore_world.gen_temp_var();

    ConstOrVar top = operand->gen_eeyore();
    if(top.type==ConstOrVar::TempVar && top.val.tempvar<0)
        generror("unary operand not primitive");

    outasm("t%d = %s %s", tret, cvt_from_unary(op).c_str(), top.eeyore_ref());
    return ConstOrVar::asTempVar(tret);
}

ConstOrVar AstExpOpBinary::gen_eeyore() {
    if(!get_const().iserror) {
        return ConstOrVar::asConstExp(get_const().val);
    }

    int tret = eeyore_world.gen_temp_var();

    ConstOrVar top1 = ConstOrVar::asConstExp(0), top2 = ConstOrVar::asConstExp(0);
    int lskip;
    
    // these ops will shortcircuit
    if(op==OpAnd) {
        outasm("t%d = 0 // op and - default value", tret);
        lskip = eeyore_world.gen_label();

        top1 = operand1->gen_eeyore();
        outasm("if %s == 0 goto l%d // op and - test 1", top1.eeyore_ref(), lskip);

        top2 = operand2->gen_eeyore();
        outasm("if %s == 0 goto l%d // op and - test 2", top2.eeyore_ref(), lskip);

        outasm("t%d = 1 // op and - pass test", tret);
        outasm("l%d: // op and - lskip", lskip);
        return ConstOrVar::asTempVar(tret);

    } else if(op==OpOr) {
        outasm("t%d = 1 // op or - default value", tret);
        lskip = eeyore_world.gen_label();

        top1 = operand1->gen_eeyore();
        outasm("if %s != 0 goto l%d // op or - test 1", top1.eeyore_ref(), lskip);

        top2 = operand2->gen_eeyore();
        outasm("if %s != 0 goto l%d // op or - test 2", top2.eeyore_ref(), lskip);

        outasm("t%d = 0 // op or - pass test", tret);
        outasm("l%d: // op or - lskip", lskip);
        return ConstOrVar::asTempVar(tret);
    }

    // general binary ops
    top1 = operand1->gen_eeyore();
    top2 = operand2->gen_eeyore();
    if(top1.type==ConstOrVar::TempVar && top1.val.tempvar<0)
        generror("binary operand1 not primitive");
    if(top2.type==ConstOrVar::TempVar && top2.val.tempvar<0)
        generror("binary operand2 not primitive");

    outasm("t%d = %s %s %s", tret, top1.eeyore_ref(), cvt_from_binary(op).c_str(), top2.eeyore_ref());
    return ConstOrVar::asTempVar(tret);
}