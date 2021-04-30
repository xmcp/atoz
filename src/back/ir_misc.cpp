#include "back/ir.hpp"
#include "front/ast.hpp"

#define istype(ptr, cls) (dynamic_cast<cls*>(ptr)!=nullptr)

string demystify_reguid(int uid) {
    char buf[16];
    if(uid<0)
        sprintf(buf, "t%d", -(uid+1));
    else if(uid>=REGUID_ARG_OFFSET)
        sprintf(buf, "p%d", uid-REGUID_ARG_OFFSET);
    else
        sprintf(buf, "T%d", uid);
    return string(buf);
}

bool RVal::regpooled() {
    if(type==ConstExp)
        return false;
    if(type==Reference && val.reference->pos==DefGlobal) // global
        return false;
    if(type==Reference && val.reference->pos==DefLocal && val.reference->idxinfo->dims()>0) // local array
        return false;

    // tempvar, local scalar, arg
    return true;
}
int RVal::reguid() {
    assert(type!=ConstExp);
    assert(!(type==Reference && val.reference->pos==DefGlobal));

    if(type==TempVar)
        return -val.tempvar-1;
    else if(val.reference->pos==DefArg)
        return val.reference->index + REGUID_ARG_OFFSET;
    else // local
        return val.reference->index;
}

bool LVal::regpooled() {
    if(type==Reference && val.reference->pos==DefGlobal)
        return false;
    if(type==Reference && val.reference->pos==DefLocal && val.reference->idxinfo->dims()>0) // local array
        return false;

    // tempvar, local scalar, arg
    return true;
}
int LVal::reguid() {
    assert(!(type==Reference && val.reference->pos==DefGlobal));

    if(type==TempVar)
        return -val.tempvar-1;
    else if(val.reference->pos==DefArg)
        return val.reference->index + REGUID_ARG_OFFSET;
    else // local
        return val.reference->index;
}

int IrFuncDef::gen_label() {
    return root->_gen_label();
}

LVal IrFuncDef::gen_scalar_tempvar() {
    int tidx = root->_gen_tempvar();
    push_decl(new IrDecl(tidx));
    return LVal::asTempVar(tidx);
}

void IrFuncDef::push_stmt(IrStmt *stmt, string comment) {
    stmts.push_back(make_pair(stmt, comment));
    if(istype(stmt, IrLabel)) {
        auto label_stmt = (IrLabel*)stmt;
        labels.insert(make_pair(label_stmt->label, label_stmt));
    }
    if(istype(stmt, IrLabelReturn)) {
        labels.insert(make_pair(return_label, (IrLabel*)stmt));
    }
}

void IrFuncDef::connect_all_cfg() {
    for(auto it = stmts.cbegin(); it!=stmts.cend();) {
        auto stmt = it->first;
        it++;
        auto nextline = it->first;

        stmt->cfg_calc_next(nextline, this);

        for(auto nstmt: stmt->next) {
            if(nstmt==nullptr)
                cfgerror("stmt links to nullptr");

            nstmt->prev.push_back(stmt);
        }
    }
}