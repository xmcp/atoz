#include "../main/common.hpp"
#include "ast.hpp"
#include "tree_completing.hpp"

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
        def->ast_is_const = _is_const;
        def->effectively_const = true;
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
        def->ast_is_const = false;
        def->effectively_const = false;
        def->index = idx++;
        def->pos = DefArg;
    }
}

///// complete tree phase

void AstCompUnit::complete_tree() {
    TreeCompleter(this).complete_tree_main();
}

void AstDef::calc_initval() {
    initval.init(idxinfo);
    if(ast_initval_or_null)
        initval.calc_if_needed(ast_initval_or_null);
    // else: no initval, no need to init
}
