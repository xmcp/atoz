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

#define generror(...) do { \
    printf("codegen eeyore error: "); \
    printf(__VA_ARGS__); \
    exit(1); \
} while(0)

static char instbuf[EEYORE_INST_BUFSIZE];

void AstCompUnit::gen_eeyore() {
    for(auto *sub: val) // Decl, FuncDef
        if(istype(sub, AstDecl))
            ((AstDecl*)sub)->gen_eeyore(true);
        else // FuncDef
            ((AstFuncDef*)sub)->gen_eeyore();
}

void AstDecl::gen_eeyore(bool incl_decl) {
    if(incl_decl) {
        for(auto *def: defs->val)
            def->gen_eeyore_decl();
    }
    for(auto *def: defs->val)
        def->gen_eeyore_init();
}

void AstDef::gen_eeyore_decl() {
    if(pos==DefArg)
        return;

    if(idxinfo->val.empty())
        outasm("var T%d // decl - primitive %s", index, name.c_str());
    else
        outasm("var %d T%d // decl - array %s", initval.totelems*4, index, name.c_str());
}

void AstDef::gen_eeyore_init() {
    for(int i=0; i<initval.totelems; i++)
        if(initval.value[i] != nullptr) {
            int trval = initval.value[i]->gen_eeyore();
            outasm("T%d [%d] = t%d // init %s #%d/%d", index, i*4, trval, name.c_str(), i, initval.totelems);
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
}

void AstFuncUseParams::gen_eeyore() {
    for(auto *param: val) {
        auto *lval = dynamic_cast<AstExpLVal*>(param);
        if(lval && lval->dim_left>0) { // pass array as pointer
            AstExp *idx = lval->def->initval.getoffset(lval->idxinfo, true);
            int tidx = idx->gen_eeyore();
            int trval = eeyore_world.gen_temp_var();
            outasm("t%d = T%d [t%d] // useparam - array access", trval, lval->def->index, tidx);
            outasm("param t%d // useparam - array pass", trval);
        } else { // pass primitive
            int trval = param->gen_eeyore();
            outasm("param t%d // useparam - primitive", trval);
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
    int trval = rval->gen_eeyore();

    if(lval->dim_left!=0)
        generror("assignment lval got dim %d for var %s", lval->dim_left, lval->name.c_str());

    if(!lval->idxinfo->val.empty()) { // has array index
        AstExp *idx = lval->def->initval.getoffset(lval->idxinfo, true);
        int tlidx = idx->gen_eeyore();
        outasm("T%d [t%d] = t%d // assign", lval->def->index, tlidx, trval);
    } else { // plain value
        outasm("T%d = t%d // assign", lval->def->index, trval);
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
    int tcond = cond->gen_eeyore();

    int lskip = eeyore_world.gen_label();
    outasm("if t%d == 0 goto l%d // ifonly", tcond, lskip);

    body->gen_eeyore();
    outasm("l%d: // ifonly - lskip", lskip);
}

void AstStmtIfElse::gen_eeyore() {
    int tcond = cond->gen_eeyore();

    int lfalse = eeyore_world.gen_label();
    outasm("if t%d == 0 goto l%d // ifelse", tcond, lfalse);

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

    int tcond = cond->gen_eeyore();
    outasm("if t%d == 0 goto l%d // while - done", tcond, ldone);

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
    int tret = retval->gen_eeyore();
    outasm("return t%d // return", tret);
}

int AstExpLVal::gen_eeyore() {
    int tval = eeyore_world.gen_temp_var();

    if(dim_left!=0)
        generror("exp got dim %d for var %s", dim_left, name.c_str());

    if(!def->idxinfo->val.empty()) {
        AstExp *idx = def->initval.getoffset(idxinfo, false);
        int tidx = idx->gen_eeyore();
        outasm("t%d = T%d [t%d] // lval - array", tval, def->index, tidx);
    } else {
        outasm("t%d = T%d // lval - primitive", tval, def->index);
    }
    return tval;
}

int AstExpLiteral::gen_eeyore() {
    int tval = eeyore_world.gen_temp_var();
    outasm("t%d = %d", tval, val);
    return tval;
}

int AstExpFunctionCall::gen_eeyore() {
    params->gen_eeyore();

    if(def->type==FuncVoid) {
        outasm("call f_%s", name.c_str());
        return -1;
    } else {
        int tret = eeyore_world.gen_temp_var();
        outasm("t%d = call f_%s", tret, name.c_str());
        return tret;
    }
}

int AstExpOpUnary::gen_eeyore() {
    int top = operand->gen_eeyore();
    if(top<0)
        generror("unary operand not primitive");

    int tret = eeyore_world.gen_temp_var();
    outasm("t%d = %s t%d", tret, cvt_from_unary(op).c_str(), top);
    return tret;
}

int AstExpOpBinary::gen_eeyore() {
    int top1 = operand1->gen_eeyore();
    int top2 = operand2->gen_eeyore();
    if(top1<0 || top2<0)
        generror("binary operand not primitive");

    int tret = eeyore_world.gen_temp_var();
    outasm("t%d = t%d %s t%d", tret, top1, cvt_from_binary(op).c_str(), top2);
    return tret;
}