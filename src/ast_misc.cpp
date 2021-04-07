#include "ast_defs.hpp"
#include "complete_ast_phase.cpp"
#include <cassert>

#define istype(ptr,cls) (dynamic_cast<cls*>(ptr)!=nullptr)

///// push_val

void AstCompUnit::push_val(Ast *next) {
    assert(istype(next, AstDecl) || istype(next, AstFuncDef));
    val.push_back(next);
}

void AstBlock::push_val(Ast *next) {
    assert(istype(next, AstDecl) || istype(next, AstStmt));
    body.push_back(next);
}

///// propagate

void AstDecl::propagate_property() {
    static int idx = 0; // global and local vars share same indexing in eeyore
    for(AstDef *def: defs->val) {
        def->type = _type;
        def->is_const = _is_const;
        def->index = idx++;
    }
}

void AstDecl::propagate_defpos(DefPosition pos) {
    for(AstDef *def: defs->val) {
        def->pos = pos;
    }
}

void AstFuncDefParams::propagate_property_and_defpos() {
    int idx = 0;
    for(AstDef *def: val) {
        def->type = VarInt;
        def->is_const = false;
        def->index = idx++;
        def->pos = DefArg;
    }
}

///// complete tree phase

void AstCompUnit::complete_tree() {
    TreeCompleter(this).complete_tree_main();
}

void AstDef::calc_initval() {
    if(ast_initval_or_null)
        initval.calc_if_needed(ast_initval_or_null);
    else { // no initval, default to 0
        auto ptr = new AstInitVal(false);
        ptr->val.single = new AstExpLiteral(0);
        initval.calc_if_needed(ptr);
    }
}
