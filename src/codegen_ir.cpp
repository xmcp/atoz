#include <cstdio>

#include "ast.hpp"

#define istype(ptr, cls) (dynamic_cast<cls*>(ptr)!=nullptr)

#define generror(...) do { \
    printf("codegen eeyore error: "); \
    printf(__VA_ARGS__); \
    exit(1); \
} while(0)

void AstCompUnit::gen_ir(IrRoot *root) {
    for(auto *sub: val) // Decl, FuncDef
        if(istype(sub, AstDecl))
            ((AstDecl *) sub)->gen_ir_global(root);
        else // FuncDef
            ((AstFuncDef*)sub)->gen_ir(root);
}

void AstDecl::gen_ir_global(IrRoot *root) {
    for(auto *def: defs->val) {
        def->gen_ir_decl(root);
        def->gen_ir_init_global(root);
    }
}
void AstDecl::gen_ir_local(IrFuncDef *func) {
    for(auto *def: defs->val) {
        def->gen_ir_decl(func);
        def->gen_ir_init_local(func);
    }
}

void AstDef::gen_ir_decl(IrDeclContainer *cont) {
    assert(pos!=DefArg); // funcdef->params is not walked in this phase

    if(idxinfo->val.empty())
        cont->push_decl(new IrDecl(this), name);
    else
        cont->push_decl(new IrDecl(this, initval.totelems), name);
}

void AstDef::gen_ir_init_global(IrRoot *root) {
    if(idxinfo->val.empty()) { // primitive
        if(initval.value[0] != nullptr) {
            auto constres = initval.value[0]->get_const();
            if(constres.iserror)
                generror("initializer for global var %s not constant", name.c_str());

            root->push_init(new IrInit(this, constres.val), name);
        }
    } else { // array
        for(int i=0; i<initval.totelems; i++)
            if(initval.value[i] != nullptr) {
                auto constres = initval.value[i]->get_const();
                if(constres.iserror)
                    generror("initializer for global var %s not constant", name.c_str());

                root->push_init(new IrInit(this, i, constres.val), name);
            }
    }
}

void AstDef::gen_ir_init_local(IrFuncDef *func) {
    if(idxinfo->val.empty()) { // primitive
        if(initval.value[0] != nullptr) {
            RVal trval = initval.value[0]->gen_rval(func);
            //outasm("T%d = %s // init %s local", index, trval.eeyore_ref(), name.c_str());
            func->push_stmt(new IrMov(this, trval), name);
        }
    } else { // array
        for(int i=0; i<initval.totelems; i++)
            if(initval.value[i] != nullptr) {
                RVal trval = initval.value[i]->gen_rval(func);
                //outasm("T%d [%d] = %s // init %s local #%d/%d", index, i*4, trval.eeyore_ref(), name.c_str(), i, initval.totelems);
                func->push_stmt(new IrArraySet(this, RVal::asConstExp(i*4), trval), name);
            }
    }
}


void AstFuncDef::gen_ir(IrRoot *root) {
    auto *func = new IrFuncDef(root, name, (int)params->val.size());
    root->push_func(func);

    body->gen_ir(func);

    if(type==FuncInt)
        //outasm("return 0 // funcdef - end");
        func->push_stmt(new IrReturn(RVal::asConstExp(0)), "func epilog");
    else
        //outasm("return // funcdef - end");
        func->push_stmt(new IrReturnVoid(), "func epilog");
}

void AstFuncUseParams::gen_ir(IrFuncDef *func) {
    for(auto *param: val) {
        auto *lval = dynamic_cast<AstExpLVal*>(param);
        if(lval && lval->dim_left>0) { // pass array as pointer
            AstExp *idx = lval->def->initval.get_offset_bytes(lval->idxinfo, true);
            RVal tidx = idx->gen_rval(func);
            LVal trval = func->gen_scalar_tempvar();
            //outasm("t%d = %c%d + %s // useparam - array access", trval, cdef(lval->def), lval->def->index, tidx.eeyore_ref());
            func->push_stmt(new IrOpBinary(trval, lval->def, OpPlus, tidx), "param array access");
            //outasm("param t%d // useparam - array pass", trval);
            func->push_stmt(new IrParam(trval));
        } else { // pass primitive
            RVal trval = param->gen_rval(func);
            //outasm("param %s // useparam - primitive", trval.eeyore_ref());
            func->push_stmt(new IrParam(trval));
        }
    }
}

void AstBlock::gen_ir(IrFuncDef *func) {
    for(auto *sub: body) {// Decl, Stmt
        if(istype(sub, AstStmt))
            ((AstStmt *) sub)->gen_ir(func);
        else { // Decl handled in funcdef
            ((AstDecl *) sub)->gen_ir_local(func);
        }
    }
}

void AstStmtAssignment::gen_ir(IrFuncDef *func) {
    RVal val = rval->gen_rval(func);

    if(lval->dim_left!=0)
        generror("assignment lval got dim %d for var %s", lval->dim_left, lval->name.c_str());

    if(!lval->idxinfo->val.empty()) { // has array index
        AstExp *idx = lval->def->initval.get_offset_bytes(lval->idxinfo, true);
        RVal lidx = idx->gen_rval(func);
        //outasm("%c%d [%s] = %s // assign", cdef(lval->def), lval->def->index, tlidx.eeyore_ref(), trval.eeyore_ref());
        func->push_stmt(new IrArraySet(lval->def, lidx, val));
    } else { // plain value
        //outasm("%c%d = %s // assign", cdef(lval->def), lval->def->index, trval.eeyore_ref());
        func->push_stmt(new IrMov(lval->def, val));
    }
}

void AstStmtExp::gen_ir(IrFuncDef *func) {
    exp->gen_rval(func); // it will store its result in a tempvar, but ignored here
}

void AstStmtVoid::gen_ir(IrFuncDef *func) {
    // do nothing
}

void AstStmtBlock::gen_ir(IrFuncDef *func) {
    block->gen_ir(func);
}

void AstStmtIfOnly::gen_ir(IrFuncDef *func) {
    RVal tcond = cond->gen_rval(func);

    int lskip = func->gen_label();
    //outasm("if %s == 0 goto l%d // ifonly", tcond.eeyore_ref(), lskip);
    func->push_stmt(new IrCondGoto(tcond, OpEq, RVal::asConstExp(0), lskip), "ifonly");

    body->gen_ir(func);
    //outasm("l%d: // ifonly - lskip", lskip);
    func->push_stmt(new IrLabel(lskip), "ifonly - lskip");
}

void AstStmtIfElse::gen_ir(IrFuncDef *func) {
    RVal tcond = cond->gen_rval(func);

    int lfalse = func->gen_label();
    //outasm("if %s == 0 goto l%d // ifelse", tcond.eeyore_ref(), lfalse);
    func->push_stmt(new IrCondGoto(tcond, OpEq, RVal::asConstExp(0), lfalse), "ifelse");

    body_true->gen_ir(func);
    int ldone = func->gen_label();
    //outasm("goto l%d // ifelse - truedone", ldone);
    func->push_stmt(new IrGoto(ldone), "ifelse - true done");

    //outasm("l%d: // ifelse - lfalse", lfalse);
    func->push_stmt(new IrLabel(lfalse), "ifelse - lfalse");
    body_false->gen_ir(func);

    //outasm("l%d: // ifelse - ldone", ldone);
    func->push_stmt(new IrLabel(ldone), "ifelse - ldone");
}

void AstStmtWhile::gen_ir(IrFuncDef *func) {
    ltest = func->gen_label();
    ldone = func->gen_label();

    //outasm("l%d: // while - ltest", ltest);
    func->push_stmt(new IrLabel(ltest), "while - ltest");

    RVal tcond = cond->gen_rval(func);
    //outasm("if %s == 0 goto l%d // while - done", tcond.eeyore_ref(), ldone);
    func->push_stmt(new IrCondGoto(tcond, OpEq, RVal::asConstExp(0), ldone), "while - done");

    body->gen_ir(func);
    //outasm("goto l%d // while - totest", ltest);
    func->push_stmt(new IrGoto(ltest), "while - to test");
    //outasm("l%d: // while - ldone", ldone);
    func->push_stmt(new IrLabel(ldone), "while - ldone");
}

void AstStmtBreak::gen_ir(IrFuncDef *func) {
    //outasm("goto l%d // break", loop->ldone);
    func->push_stmt(new IrGoto(loop->ldone), "break");
}

void AstStmtContinue::gen_ir(IrFuncDef *func) {
    //outasm("goto l%d // continue", loop->ltest);
    func->push_stmt(new IrGoto(loop->ltest), "continue");
}

void AstStmtReturnVoid::gen_ir(IrFuncDef *func) {
    //outasm("return");
    func->push_stmt(new IrReturnVoid());
}

void AstStmtReturn::gen_ir(IrFuncDef *func) {
    RVal tret = retval->gen_rval(func);
    //outasm("return %s // return", tret.eeyore_ref());
    func->push_stmt(new IrReturn(tret));
}

RVal AstExpLVal::gen_rval(IrFuncDef *func) {
    if(dim_left!=0)
        generror("exp got dim %d for var %s", dim_left, name.c_str());

    if(!get_const().iserror) {
        return RVal::asConstExp(get_const().val);
    }

    if(!def->idxinfo->val.empty()) { // array
        AstExp *idx = def->initval.get_offset_bytes(idxinfo, false);
        RVal tidx = idx->gen_rval(func);

        LVal tval = func->gen_scalar_tempvar();
        //outasm("t%d = %c%d [%s] // lval - array", tval, cdef(def), def->index, tidx.eeyore_ref());
        func->push_stmt(new IrArrayGet(tval, def, tidx));
        return tval;
    } else { //primitive
        return def;
    }
}

RVal AstExpLiteral::gen_rval(IrFuncDef *func) {
    return RVal::asConstExp(val);
}

RVal AstExpFunctionCall::gen_rval(IrFuncDef *func) {
    params->gen_ir(func);

    // special functions
    if(name=="starttime") {
        //outasm("param %d // func call special", loc.lineno);
        func->push_stmt(new IrParam(RVal::asConstExp(loc.lineno)), "func call special");
        //outasm("call f__sysy_starttime // func call special");
        func->push_stmt(new IrCallVoid("_sysy_starttime"));
        return RVal::asTempVar(-1);
    }
    if(name=="stoptime") {
        //outasm("param %d // func call special", loc.lineno);
        func->push_stmt(new IrParam(RVal::asConstExp(loc.lineno)), "func call special");
        //outasm("call f__sysy_stoptime // func call special");
        func->push_stmt(new IrCallVoid("_sysy_stoptime"));
        return RVal::asTempVar(-1);
    }

    if(def->type==FuncVoid) {
        //outasm("call f_%s", name.c_str());
        func->push_stmt(new IrCallVoid(name));
        return RVal::asTempVar(-1);
    } else {
        LVal tret = func->gen_scalar_tempvar();
        //outasm("t%d = call f_%s", tret, name.c_str());
        func->push_stmt(new IrCall(tret, name));
        return tret;
    }
}

RVal AstExpOpUnary::gen_rval(IrFuncDef *func) {
    if(!get_const().iserror) {
        return RVal::asConstExp(get_const().val);
    }

    LVal tret = func->gen_scalar_tempvar();

    RVal top = operand->gen_rval(func);
    if(top.type == RVal::TempVar && top.val.tempvar < 0)
        generror("unary operand not primitive");

    //outasm("t%d = %s %s", tret, cvt_from_unary(op).c_str(), top.eeyore_ref());
    func->push_stmt(new IrOpUnary(tret, op, top));
    return tret;
}

RVal AstExpOpBinary::gen_rval(IrFuncDef *func) {
    if(!get_const().iserror) {
        return RVal::asConstExp(get_const().val);
    }

    LVal tret = func->gen_scalar_tempvar();

    RVal top1 = RVal::asConstExp(0), top2 = RVal::asConstExp(0);
    int lskip;
    
    // these ops will shortcircuit
    if(op==OpAnd) {
        //outasm("t%d = 0 // op and - default value", tret);
        func->push_stmt(new IrMov(tret, RVal::asConstExp(0)), "op and - default value");
        lskip = func->gen_label();

        top1 = operand1->gen_rval(func);
        //outasm("if %s == 0 goto l%d // op and - test 1", top1.eeyore_ref(), lskip);
        func->push_stmt(new IrCondGoto(top1, OpEq, RVal::asConstExp(0), lskip), "op and - test 1");

        top2 = operand2->gen_rval(func);
        //outasm("if %s == 0 goto l%d // op and - test 2", top2.eeyore_ref(), lskip);
        func->push_stmt(new IrCondGoto(top2, OpEq, RVal::asConstExp(0), lskip), "op and - test 2");

        //outasm("t%d = 1 // op and - pass test", tret);
        func->push_stmt(new IrMov(tret, RVal::asConstExp(1)), "op and - passed test");
        //outasm("l%d: // op and - lskip", lskip);
        func->push_stmt(new IrLabel(lskip), "op and - lskip");
        return tret;

    } else if(op==OpOr) {
        //outasm("t%d = 1 // op or - default value", tret);
        func->push_stmt(new IrMov(tret, RVal::asConstExp(1)), "op or - default value");
        lskip = func->gen_label();

        top1 = operand1->gen_rval(func);
        //outasm("if %s != 0 goto l%d // op or - test 1", top1.eeyore_ref(), lskip);
        func->push_stmt(new IrCondGoto(top1, OpNeq, RVal::asConstExp(0), lskip), "op or - test 1");

        top2 = operand2->gen_rval(func);
        //outasm("if %s != 0 goto l%d // op or - test 2", top2.eeyore_ref(), lskip);
        func->push_stmt(new IrCondGoto(top2, OpNeq, RVal::asConstExp(0), lskip), "op or - test 2");

        //outasm("t%d = 0 // op or - pass test", tret);
        func->push_stmt(new IrMov(tret, RVal::asConstExp(0)), "op or - passed test");
        //outasm("l%d: // op or - lskip", lskip);
        func->push_stmt(new IrLabel(lskip), "op or - lskip");
        return tret;
    }

    // general binary ops
    top1 = operand1->gen_rval(func);
    top2 = operand2->gen_rval(func);
    if(top1.type == RVal::TempVar && top1.val.tempvar < 0)
        generror("binary operand1 not primitive");
    if(top2.type == RVal::TempVar && top2.val.tempvar < 0)
        generror("binary operand2 not primitive");

    //outasm("t%d = %s %s %s", tret, top1.eeyore_ref(), cvt_from_binary(op).c_str(), top2.eeyore_ref());
    func->push_stmt(new IrOpBinary(tret, top1, op, top2));
    return tret;
}
