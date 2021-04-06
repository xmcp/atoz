#include "ast_defs.hpp"
#include <cassert>

#define istype(ptr,cls) (dynamic_cast<cls*>(ptr)!=nullptr)

void AstCompUnit::push_val(Ast *next) {
    assert(istype(next, AstDecl) || istype(next, AstFuncDef));
    val.push_back(next);
}