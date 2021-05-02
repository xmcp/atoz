#include "inst.hpp"
#include "ir.hpp"
#include "../front/ast.hpp"

const bool INST_GEN_COMMENTS = true;

void warn_dest_not_used(LVal v, string funcname) {
    printf("warning: unused dest value ");
    if(v.type==LVal::TempVar)
        printf("{temp %d}", v.val.tempvar);
    else // reference
        printf("{ref %s}", v.val.reference->name.c_str());
    printf(" in function %s\n", funcname.c_str());
}

const Preg tmpreg0 = Preg('t', 0);
const Preg tmpreg1 = Preg('t', 1);

void IrRoot::gen_inst(InstRoot *root) {
    for(auto declpair: decls)
        declpair.first->gen_inst_global(root);

    for(auto initpair: inits)
        initpair.first->gen_inst_global(root);

    for(auto funcpair: funcs)
        funcpair.first->gen_inst(root);

    assert(root->mainfunc!=nullptr);

    // generate global array init to start of main
    const Preg rega0 = Preg('a', 0); // can be used before program starts
    for(auto arrdecl_pair: root->decl_arrays) { // global idx: decl
        if(arrdecl_pair.second->initval.empty())
            continue;

        // t0 stores global ptr (t1 for temp if index overflows), a0 stores offset

//----- BELOW: STMTS IN REVERSED ORDER, because of `push_front`

        for(auto kvpair: arrdecl_pair.second->initval) { // offset bytes -> val
            if(imm_overflows(kvpair.first)) {
/* ↑ */         root->mainfunc->stmts.push_front(new InstArraySet(tmpreg1, 0, rega0));
/* ↑ */         root->mainfunc->stmts.push_front(new InstOpBinary(tmpreg1, tmpreg0, OpPlus, tmpreg1));
/* ↑ */         root->mainfunc->stmts.push_front(new InstLoadImm(tmpreg1, kvpair.first));
            } else {
/* ↑ */         root->mainfunc->stmts.push_front(new InstArraySet(tmpreg0, kvpair.first, rega0));
            }
/* ↑ */     root->mainfunc->stmts.push_front(new InstLoadImm(rega0, kvpair.second));
        }
/* ↑ */ root->mainfunc->stmts.push_front(new InstLoadAddrGlobal(tmpreg0, arrdecl_pair.first));

//----- ABOVE: STMTS IN REVERSED ORDER
    }
}

void IrDecl::gen_inst_global(InstRoot *root) {
    assert(def_or_null!=nullptr);

    if(def_or_null->idxinfo->dims()>0) { // array
        root->push_decl(def_or_null->index, new InstDeclArray(def_or_null->index, def_or_null->initval.totelems*4));
    } else { // scalar
        root->push_decl(def_or_null->index, new InstDeclScalar(def_or_null->index));
    }
}

void IrInit::gen_inst_global(InstRoot *root) {
    if(def->idxinfo->dims()>0) { // array
        auto it = root->decl_arrays.find(def->index);
        assert(it!=root->decl_arrays.end());
        it->second->initval[offset_bytes] = val;
    } else { // scalar
        auto it = root->decl_scalars.find(def->index);
        assert(it!=root->decl_scalars.end());
        it->second->initval = val;
    }
}

void IrFuncDef::gen_inst(InstRoot *root) {
    auto func = new InstFuncDef(name, params->val.size(), spillsize+callersavesize);
    root->push_func(func);

    if(INST_GEN_COMMENTS) {
        string s = "DESTROYS:";
        for(Preg reg: this->root->get_destroy_set(name))
            s += " " + reg.tigger_ref();
        func->push_stmt(new InstComment(s));
    }

    for(auto stmtpair: stmts) {
        if(INST_GEN_COMMENTS) {
            func->push_stmt(new InstComment(""));
            list<string> cmt_buf;
            stmtpair.first->output_eeyore(cmt_buf);
            for(const auto& line: cmt_buf)
                func->push_stmt(new InstComment(line));
        }
        stmtpair.first->gen_inst(func);
    }
}


Preg fn_rload(RVal v, int idx, IrFuncDef *irfunc, InstFuncDef *instfunc) {
    Preg reg = Preg('t', idx);
    if(v.type==RVal::ConstExp) { // const
        instfunc->push_stmt(new InstLoadImm(reg, v.val.constexp));
        return reg;
    } else if(v.type==RVal::Reference && v.val.reference->pos==DefGlobal) { // ref global
        instfunc->push_stmt(new InstLoadGlobal(reg, v.val.reference->index));
        return reg;
    } else { // vregged
        return irfunc->get_vreg(v).load_into_preg(instfunc, idx);
    }
}

Preg fn_rstore(LVal v, IrFuncDef *irfunc) {
    if(v.type==LVal::Reference && v.val.reference->pos==DefGlobal) { // ref global
        return tmpreg0;
    } else { // vregged
        return irfunc->get_vreg(v).get_stored_preg();
    }
}

void fn_dostore(LVal v, IrFuncDef *irfunc, InstFuncDef *instfunc) {
    if(v.type==LVal::Reference && v.val.reference->pos==DefGlobal) { // ref global
        instfunc->push_stmt(new InstLoadAddrGlobal(tmpreg1, v.val.reference->index));
        instfunc->push_stmt(new InstArraySet(tmpreg1, 0, tmpreg0));
    } else { // vregged
        irfunc->get_vreg(v).store_onto_stack_if_needed(instfunc);
    }
}

#define rload(v, idx) fn_rload(v, idx, this->func, func)
#define rstore(v) fn_rstore(v, this->func)
#define dostore(v) fn_dostore(v, this->func, func)
#define unused(dest) ((dest).regpooled() && meet_pooled_vars.find((dest).reguid())==alive_pooled_vars.end())
#define ret_if_unused(dest) do { \
    if(unused(dest)) { \
        warn_dest_not_used(dest, this->func->name); \
        return; \
    } \
} while(0)

void IrOpBinary::gen_inst(InstFuncDef *func) {
    ret_if_unused(dest);

    if(operand1.type==RVal::Reference && operand1.val.reference->idxinfo->dims()>0) {
        // pointer math, e.g. ptr = arr + idx
        switch(operand1.val.reference->pos) {
            case DefGlobal:
                func->push_stmt(new InstLoadAddrGlobal(tmpreg0, operand1.val.reference->index));
                func->push_stmt(new InstOpBinary(rstore(dest), tmpreg0, op, rload(operand2, 1)));
                break;

            case DefLocal:
                assert(this->func->get_vreg(operand1).pos==Vreg::VregInStack);
                func->push_stmt(new InstLoadAddrStack(tmpreg0, this->func->get_vreg(operand1).spilloffset));
                func->push_stmt(new InstOpBinary(rstore(dest), tmpreg0, op, rload(operand2, 1)));
                break;

            case DefArg:
                assert(this->func->get_vreg(operand1).pos==Vreg::VregInReg);
                func->push_stmt(new InstOpBinary(rstore(dest), this->func->get_vreg(operand1).reg, op, rload(operand2, 1)));
                break;

            default:
                assert(false);
        }
    } else {
        func->push_stmt(new InstOpBinary(rstore(dest), rload(operand1, 0), op, rload(operand2, 1)));
    }
    dostore(dest);
}

void IrOpUnary::gen_inst(InstFuncDef *func) {
    ret_if_unused(dest);

    func->push_stmt(new InstOpUnary(rstore(dest), op, rload(operand, 1)));
    dostore(dest);
}

void IrMov::gen_inst(InstFuncDef *func) {
    ret_if_unused(dest);

    if(src.type==RVal::ConstExp)
        func->push_stmt(new InstLoadImm(rstore(dest), src.val.constexp));
    else
        func->push_stmt(new InstMov(rstore(dest), rload(src, 1)));
    dostore(dest);
}

void IrArraySet::gen_inst(InstFuncDef *func) {
    if(dest.type==LVal::TempVar) {
        func->push_stmt(new InstArraySet(rload(dest, 0), doffset, rload(src, 1)));
    } else { // reference
        switch(dest.val.reference->pos) {
            case DefGlobal:
                func->push_stmt(new InstLoadAddrGlobal(tmpreg0, dest.val.reference->index));
                func->push_stmt(new InstArraySet(tmpreg0, doffset, rload(src, 1)));
                break;

            case DefLocal:
                assert(this->func->get_vreg(dest).pos==Vreg::VregInStack);
                func->push_stmt(new InstLoadAddrStack(tmpreg0, this->func->get_vreg(dest).spilloffset + doffset/4));
                func->push_stmt(new InstArraySet(tmpreg0, 0, rload(src, 1)));
                break;

            case DefArg:
                assert(this->func->get_vreg(dest).pos==Vreg::VregInReg);
                func->push_stmt(new InstArraySet(this->func->get_vreg(dest).reg, doffset, rload(src, 1)));
                break;

            default:
                assert(false);
        }
    }
}

void IrArrayGet::gen_inst(InstFuncDef *func) {
    ret_if_unused(dest);

    if(src.type==RVal::TempVar) {
        func->push_stmt(new InstArrayGet(rstore(dest), rload(src, 1), soffset));
    } else { // reference
        switch(src.val.reference->pos) {
            case DefGlobal:
                func->push_stmt(new InstLoadAddrGlobal(tmpreg1, src.val.reference->index));
                func->push_stmt(new InstArrayGet(rstore(dest), tmpreg1, soffset));
                break;

            case DefLocal:
                assert(this->func->get_vreg(src).pos==Vreg::VregInStack);
                func->push_stmt(new InstLoadAddrStack(tmpreg1, this->func->get_vreg(src).spilloffset));
                func->push_stmt(new InstArrayGet(rstore(dest), tmpreg1, soffset));
                break;

            case DefArg:
                assert(this->func->get_vreg(src).pos==Vreg::VregInReg);
                func->push_stmt(new InstArrayGet(rstore(dest), this->func->get_vreg(src).reg, soffset));
                break;

            default:
                assert(false);
        }
    }
    dostore(dest);
}

void IrCondGoto::gen_inst(InstFuncDef *func) {
    func->push_stmt(new InstCondGoto(rload(operand1, 0), op, rload(operand2, 1), label));
}

void IrGoto::gen_inst(InstFuncDef *func) {
    func->push_stmt(new InstGoto(label));
}

void IrLabel::gen_inst(InstFuncDef *func) {
    func->push_stmt(new InstLabel(label));
}

void IrParam::gen_inst_handled_by_call(InstFuncDef *func) {
    func->push_stmt(new InstMov(Preg('a', pidx), rload(param, 1)));
}

template<typename T>
int vector_index_of(vector<T> v, T x) {
    for(int i=0; i<(int)v.size(); i++)
        if(v[i]==x)
            return i;
    return -1;
}

vector<Preg> IrCallVoid::gen_inst_common(InstFuncDef *func) {
    // collect caller saved regs

    vector<Preg> saved_regs;
    auto destroy_set = this->func->root->get_destroy_set(name);

    for(auto uid: alive_pooled_vars) {
        Vreg reg = this->func->get_vreg(uid);
        if(reg.pos==Vreg::VregInReg && destroy_set.find(reg.reg)!=destroy_set.end())
            saved_regs.push_back(reg.reg);
    }

    // save them

    for(int i=0; i<(int)saved_regs.size(); i++)
        saved_regs[i].caller_save_before(func, this->func->spillsize + i);

    // pass param
    // note that param in a* need to be loaded from stack to avoid collision

    for(auto param: params) {
        if(param->param.regpooled()) {
            Vreg vreg = this->func->get_vreg(param->param);
            if(vreg.pos==Vreg::VregInReg) {
                Preg preg = vreg.reg;
                int saveidx = vector_index_of(saved_regs, preg);

                if(preg.cat=='a' && preg.index<(int)params.size()) { // got a possible collision
                    assert(saveidx!=-1); // param should be alive

                    func->push_stmt(new InstLoadStack(
                        Preg('a', param->pidx),
                        this->func->spillsize + saveidx
                    ));
                    continue;
                }
            }
        }
        // otherwise, no collision, use simple mov
        param->gen_inst_handled_by_call(func);
    }

    // call
    func->push_stmt(new InstCall(name));

    return saved_regs;
}

void IrCallVoid::gen_inst(InstFuncDef *func) {
    auto saved_regs = gen_inst_common(func);

    // restore saved regs
    for(int i=0; i<(int)saved_regs.size(); i++)
        saved_regs[i].caller_load_after(func, this->func->spillsize + i);
}

void IrCall::gen_inst(InstFuncDef *func) {
    auto saved_regs = gen_inst_common(func); // up to `call`

    Vreg retreg = Vreg(Preg('x', 0));
    if(!unused(ret)) { // save retval
        func->push_stmt(new InstMov(rstore(ret), Preg('a', 0)));
        dostore(ret);
        retreg = ret.regpooled() ? this->func->get_vreg(ret.reguid()) : Vreg(Preg('x', 0));
    }

    // restore saved regs, except the retval reg
    for(int i=0; i<(int)saved_regs.size(); i++) {
        if(retreg.pos==Vreg::VregInReg && retreg.reg==saved_regs[i])
            continue;
        saved_regs[i].caller_load_after(func, this->func->spillsize + i);
    }
}

void IrReturnVoid::gen_inst(InstFuncDef *func) {
    func->push_stmt(new InstRet(func->stacksize));
}

void IrReturn::gen_inst(InstFuncDef *func) {
    func->push_stmt(new InstMov(Preg('a', 0), rload(retval, 1)));
    func->push_stmt(new InstRet(func->stacksize));
}