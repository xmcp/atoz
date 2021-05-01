#include "back/ir.hpp"
#include "front/ast.hpp"

#include <algorithm>
using std::max;

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

    // tempvar, local scalar, param
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

    // tempvar, local scalar, param
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
    /* // flag:return-label
    if(istype(stmt, IrLabelReturn)) {
        labels.insert(make_pair(return_label, (IrLabel*)stmt));
    }
    */
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

void IrFuncDef::report_destroyed_set() {
    unordered_set<Preg, Preg::Hash> destory_set;

    // destoryed by allocated regs
    for(auto vregpair: vreg_map)
        if(vregpair.second.pos==Vreg::VregInReg)
            destory_set.insert(vregpair.second.reg);

    // insert it first, therefore we can get it if the function recurses
    root->destroy_sets.insert(make_pair(name, destory_set));

    // destroyed by sub functions
    for(const auto& stmtpair: stmts) {
        string funcname = "";
        if(istype(stmtpair.first, IrCall))
            funcname = ((IrCall*)stmtpair.first)->name;
        else if(istype(stmtpair.first, IrCallVoid))
            funcname = ((IrCallVoid*)stmtpair.first)->name;

        if(!funcname.empty()) {
            // update destroy set

            auto it = root->destroy_sets.find(funcname);
            assert(it!=root->destroy_sets.end());

            for(auto var: it->second)
                destory_set.insert(var);

            // update caller save size
            int workingset = 0;
            for(auto uid: stmtpair.first->alive_vars) {
                auto vit = vreg_map.find(uid);
                if(vit!=vreg_map.end() && vit->second.pos==Vreg::VregInReg)
                    workingset++;
            }
            callersavesize = max(callersavesize, workingset);
        }
    }

    // update destory sets
    root->destroy_sets.insert(make_pair(name, destory_set));
}

void IrRoot::install_builtin_destroy_sets() {
    unordered_set<Preg, Preg::Hash> destory_set = {
        Preg('t', 0), Preg('t', 1), Preg('t', 2), Preg('t', 3), Preg('t', 4), Preg('t', 5), Preg('t', 6),
        Preg('a', 0), Preg('a', 1), Preg('a', 2), Preg('a', 3), Preg('a', 4), Preg('a', 5), Preg('a', 6), Preg('a', 7),
    };
    destroy_sets.insert(make_pair("getint", destory_set));
    destroy_sets.insert(make_pair("getch", destory_set));
    destroy_sets.insert(make_pair("getarray", destory_set));
    destroy_sets.insert(make_pair("putint", destory_set));
    destroy_sets.insert(make_pair("putch", destory_set));
    destroy_sets.insert(make_pair("putarray", destory_set));
    destroy_sets.insert(make_pair("_sysy_starttime", destory_set));
    destroy_sets.insert(make_pair("_sysy_stoptime", destory_set));
}