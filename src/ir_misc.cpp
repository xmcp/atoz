#include "ir.hpp"

int IrFuncDef::gen_label() {
    return root->_gen_label();
}

LVal IrFuncDef::gen_scalar_tempvar() {
    int tidx = root->_gen_tempvar();
    push_decl(new IrDecl(LVal::asTempVar(tidx)));
    return LVal::asTempVar(tidx);
}
