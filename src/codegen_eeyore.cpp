#include <cstdio>
using std::sprintf;

#include "ast.hpp"
#include "eeyore_world.hpp"

#define istype(ptr,cls) (dynamic_cast<cls*>(ptr)!=nullptr)

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
            ((AstDecl*)sub)->gen_eeyore();
        else // FuncDef
            ((AstFuncDef*)sub)->gen_eeyore();
}

void AstDecl::gen_eeyore() {
    for(auto *def: defs->val)
        def->gen_eeyore_decl();
    for(auto *def: defs->val)
        def->gen_eeyore_init();
}

void AstDef::gen_eeyore_decl() {
    if(pos==DefArg)
        return;

    if(idxinfo->val.empty())
        outasm("var T%d", index);
    else
        outasm("var %d T%d", initval.totelems*4, index);
}

void AstDef::gen_eeyore_init() {
    for(int i=0; i<initval.totelems; i++)
        if(initval.value[i] != nullptr) {
            initval.value[i]->gen_eeyore();
            int trval = eeyore_world.temp_var_top;
            outasm("T%d [%d] = t%d", index, i, trval);
        }
}


void AstFuncDef::gen_eeyore() {
    for(auto *def: defs_inside)
        def->gen_eeyore_decl();
    for(auto *def: defs_inside)
        def->gen_eeyore_init();

    body->gen_eeyore();
}

void AstFuncUseParams::gen_eeyore() {
    for(auto *param: val) {
        param->gen_eeyore();
        int trval = eeyore_world.temp_var_top;
        outasm("param t%d", trval);
    }
}

void AstBlock::gen_eeyore() {
    for(auto *sub: body) // Decl, Stmt
        if(istype(sub, AstStmt)) // Decl is handled in CompUnit or FuncDef
            ((AstStmt*)sub)->gen_eeyore();
}

void AstStmtAssignment::gen_eeyore() {
    rval->gen_eeyore();
    int trval = eeyore_world.temp_var_top;

    if(!lval->idxinfo->val.empty()) { // has array index
        AstExp *idx = lval->def->initval.getoffset(lval->idxinfo, true);
        idx->gen_eeyore();
        int tlidx = eeyore_world.temp_var_top;
        outasm("T%d [t%d] = t%d", lval->def->index, tlidx, trval);
    } else { // plain value
        outasm("T%d = t%d", lval->def->index, trval);
    }
}

void AstStmtExp::gen_eeyore() {
    exp->gen_eeyore();
}

void AstStmtVoid::gen_eeyore() {
    // empty
}

void AstStmtBlock::gen_eeyore() {
    block->gen_eeyore();
}

void AstStmtIfOnly::gen_eeyore() {
    cond->gen_eeyore();
    int tcond = eeyore_world.temp_var_top;

    int lskip = eeyore_world.gen_label();
    outasm("if t%d == 0 goto l%d", tcond, lskip);

    body->gen_eeyore();
    outasm("l%d:", lskip);
}

void AstStmtIfElse::gen_eeyore() {
    
}

void AstStmtWhile::gen_eeyore() {
    
}

void AstStmtBreak::gen_eeyore() {
    
}

void AstStmtContinue::gen_eeyore() {
    
}

void AstStmtReturnVoid::gen_eeyore() {
    
}

void AstStmtReturn::gen_eeyore() {
    
}

void AstExpLVal::gen_eeyore() {

}

void AstExpLiteral::gen_eeyore() {

}

void AstExpFunctionCall::gen_eeyore() {

}

void AstExpOpUnary::gen_eeyore() {

}

void AstExpOpBinary::gen_eeyore() {

}