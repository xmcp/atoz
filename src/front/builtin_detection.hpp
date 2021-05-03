#include "ast.hpp"
#include "../back/ir.hpp"

inline bool is_builtin_memcpy(AstFuncDef *ast) {
    // int dst[], int dst_pos, int src[], int len

    if(ast->params->val.size()!=4)
        return false;
    if(!(
        ast->params->val[0]->name=="dst" &&
        ast->params->val[1]->name=="dst_pos" &&
        ast->params->val[2]->name=="src" &&
        ast->params->val[3]->name=="len"
    ))
        return false;
    if(!(
        ast->params->val[0]->idxinfo->dims()==1 &&
        ast->params->val[1]->idxinfo->dims()==0 &&
        ast->params->val[2]->idxinfo->dims()==1 &&
        ast->params->val[3]->idxinfo->dims()==0
    ))
        return false;

    if(ast->body->body.size()!=3)
        return false;

    if(!(
        istype(ast->body->body[0], AstDecl) &&
        istype(ast->body->body[1], AstStmtWhile) &&
        istype(ast->body->body[2], AstStmtReturn)
    ))
        return false;

    auto st0 = (AstDecl*)ast->body->body[0];
    auto st1 = (AstStmtWhile*)ast->body->body[1];
    auto st2 = (AstStmtReturn*)ast->body->body[2];

    // int i = 0;
    if(!(
        st0->defs->val.size()==1 &&
        st0->defs->val[0]->name=="i" &&
        st0->defs->val[0]->idxinfo->dims()==0 &&
        st0->defs->val[0]->initval.value[0]->get_const().isalways(0)
    ))
        return false;

    // while (i < len)
    if(!(
        istype(st1->cond, AstExpOpBinary) &&
        ((AstExpOpBinary*)st1->cond)->op==OpLess &&
        istype(((AstExpOpBinary*)st1->cond)->operand1, AstExpLVal) &&
        ((AstExpLVal*)((AstExpOpBinary*)st1->cond)->operand1)->name=="i" &&
        istype(((AstExpOpBinary*)st1->cond)->operand2, AstExpLVal) &&
        ((AstExpLVal*)((AstExpOpBinary*)st1->cond)->operand2)->name=="len"
    ))
        return false;

    if(!(
        istype(st1->body, AstStmtBlock) &&
        ((AstStmtBlock*)st1->body)->block->body.size()==2 &&
        istype(((AstStmtBlock*)st1->body)->block->body[0], AstStmtAssignment) &&
        istype(((AstStmtBlock*)st1->body)->block->body[1], AstStmtAssignment)
    ))
        return false;

    auto *sub1 = (AstStmtAssignment*)((AstStmtBlock*)st1->body)->block->body[0];
    auto *sub2 = (AstStmtAssignment*)((AstStmtBlock*)st1->body)->block->body[1];

    // dst[dst_pos + i]
    if(!(
        sub1->lval->name=="dst" &&
        sub1->lval->idxinfo->val.size()==1 &&
        istype(sub1->lval->idxinfo->val[0], AstExpOpBinary) &&
        ((AstExpOpBinary*)sub1->lval->idxinfo->val[0])->op==OpPlus &&
        istype(((AstExpOpBinary*)sub1->lval->idxinfo->val[0])->operand1, AstExpLVal) &&
        istype(((AstExpOpBinary*)sub1->lval->idxinfo->val[0])->operand2, AstExpLVal) &&
        ((AstExpLVal*)((AstExpOpBinary*)sub1->lval->idxinfo->val[0])->operand1)->name=="dst_pos" &&
        ((AstExpLVal*)((AstExpOpBinary*)sub1->lval->idxinfo->val[0])->operand2)->name=="i"
    ))
        return false;

    //  = src[i];
    if(!(
        istype(sub1->rval, AstExpLVal) &&
        ((AstExpLVal*)sub1->rval)->name=="src" &&
        ((AstExpLVal*)sub1->rval)->idxinfo->val.size()==1 &&
        istype(((AstExpLVal*)sub1->rval)->idxinfo->val[0], AstExpLVal) &&
        ((AstExpLVal*)((AstExpLVal*)sub1->rval)->idxinfo->val[0])->name=="i"
    ))
        return false;

    // i = i + 1;
    if(!(
        sub2->lval->name=="i" &&
        istype(sub2->rval, AstExpOpBinary) &&
        ((AstExpOpBinary*)sub2->rval)->op==OpPlus &&
        istype(((AstExpOpBinary*)sub2->rval)->operand1, AstExpLVal) &&
        istype(((AstExpOpBinary*)sub2->rval)->operand2, AstExpLiteral) &&
        ((AstExpLVal*)((AstExpOpBinary*)sub2->rval)->operand1)->name=="i" &&
        ((AstExpLiteral*)((AstExpOpBinary*)sub2->rval)->operand2)->val==1
    ))
        return false;

    // return i;
    if(!(
        istype(st2->retval, AstExpLVal) &&
        ((AstExpLVal*)st2->retval)->name=="i"
    ))
        return false;

    return true;
}

void IrFuncDefMemcpy::output_eeyore(list<string> &buf) {
    buf.push_back("// builtin: memcpy");
}

void IrFuncDefMemcpy::gen_inst(InstRoot *root) {
    /*
     * # a0 dst, ret
     * # a1 dst_pos, upper
     * # a2 src
     * # a3 len
     * # t0 x
     *
     * {t0} x = {a1} dst_pos << 2
     * {a0} dst = {a0} dst + {t0} x
     * {a1} upper = {a1} len << 2
     * {a1} upper = {a0} src + {a1} upper
     * loop:
     * x = {a2} src [0]
     * {a0} dst [0] = x
     * {a2} src += 4
     * {a0} dst += 4
     * if {a0} src < {a1} upper goto loop
     * {a0} ret = {a3} len
     * return
     */
    auto func = new InstFuncDef(name, 4, 0);
    root->push_func(func);

    auto dst = Preg('a', 0), ret = Preg('a', 0);
    auto dst_pos = Preg('a', 1), upper = Preg('a', 1);
    auto src = Preg('a', 2);
    auto len = Preg('a', 3);
    auto x = Preg('t', 0);

    func->push_stmt(new InstComment("builtin memcpy"));
    func->push_stmt(new InstLeftShiftI(x, dst_pos, 2));
    func->push_stmt(new InstOpBinary(dst, dst, OpPlus, x));
    func->push_stmt(new InstLeftShiftI(upper, len, 2));
    func->push_stmt(new InstOpBinary(upper, src, OpPlus, upper));
    func->push_stmt(new InstLabel(looplabel));
    func->push_stmt(new InstArrayGet(x, src, 0));
    func->push_stmt(new InstArraySet(dst, 0, x));
    func->push_stmt(new InstAddI(src, src, 4));
    func->push_stmt(new InstAddI(dst, dst, 4));
    func->push_stmt(new InstCondGoto(src, RelLess, upper, looplabel));
    func->push_stmt(new InstMov(ret, len));
    func->push_stmt(new InstRet(func));
}

void IrFuncDefMemcpy::report_destroyed_set() {
    root->destroy_sets.insert(make_pair(name, unordered_set<Preg, Preg::Hash>{
            Preg('a', 0), Preg('a', 1), Preg('a', 2), Preg('a', 3)
    }));
}

IrFuncDefBuiltin *wrap_builtin_memcpy(IrFuncDef *ir) {
    auto ret = new IrFuncDefMemcpy(ir->root, ir->type, ir->name, ir->params);
    return ret;
}

inline IrFuncDefBuiltin *create_builtin_wrapper(AstFuncDef *ast, IrFuncDef *ir) {
    if(is_builtin_memcpy(ast)) {
        printf("info: got memcpy: %s\n", ast->name.c_str());
        return wrap_builtin_memcpy(ir);
    }
    return nullptr;
}