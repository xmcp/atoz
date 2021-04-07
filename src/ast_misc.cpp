#include "ast_defs.hpp"
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