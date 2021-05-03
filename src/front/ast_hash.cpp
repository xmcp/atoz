#include <functional>
using std::hash;

#include "ast.hpp"

static hash<string> hstr;
static hash<int> hint;

template<typename T>
static asthash_t hvec(vector<T> v) {
    asthash_t ret = hint(v.size());
    for(int x=(int)v.size()-1; x>=0; x--)
        ret += hint(x) + v[x]->asthash();
    return ret;
}

asthash_t AstDecl::asthash() {
    return hstr("decl") + defs->asthash();
}

asthash_t AstDefs::asthash() {
    return hstr("defs") + hvec(val);
}

asthash_t AstDef::asthash() {
    return hstr("def") +  (
        hint(type) + hint(is_const) + hstr(name) +
        idxinfo->asthash() + (ast_initval_or_null ? ast_initval_or_null->asthash() : hstr("null"))
    );
}

asthash_t AstMaybeIdx::asthash() {
    return hstr("idx") + hvec(val);
}

asthash_t AstInitVal::asthash() {
    return hstr("init") + (
        is_many ? (hstr("many") + hvec(*val.many)) : (hstr("single") + val.single->asthash())
    );
}

asthash_t AstFuncDef::asthash() {
    return hstr("func") + hint(type) + params->asthash() + body->asthash();
}


asthash_t AstFuncDefParams::asthash() {
    return hstr("defp") + hvec(val);
}


asthash_t AstFuncUseParams::asthash() {
    return hstr("defp") + hvec(val);
}

asthash_t AstBlock::asthash() {
    return hstr("defp") + hvec(body);
}

asthash_t AstStmtAssignment::asthash() {
    return hstr("assignment") + lval->asthash() + rval->asthash();
}

asthash_t AstStmtExp::asthash() {
    return hstr("exp") + exp->asthash();
}

asthash_t AstStmtVoid::asthash() {
    return hstr("void");
}

asthash_t AstStmtBlock::asthash() {
    return hstr("block") + block->asthash();
}

asthash_t AstStmtIfOnly::asthash() {
    return hstr("ifonly") + cond->asthash() + body->asthash();
}

asthash_t AstStmtIfElse::asthash() {
    return hstr("ifelse") + cond->asthash() + body_true->asthash() + body_false->asthash();
}

asthash_t AstStmtWhile::asthash() {
    return hstr("while") + cond->asthash() + body->asthash();
}

asthash_t AstStmtBreak::asthash() {
    return hstr("break");
}

asthash_t AstStmtContinue::asthash() {
    return hstr("continue");
}

asthash_t AstStmtReturnVoid::asthash() {
    return hstr("returnvoid");
}

asthash_t AstStmtReturn::asthash() {
    return hstr("return") + retval->asthash();
}

asthash_t AstExpLVal::asthash() {
    return hstr("lval") + hstr(name) + idxinfo->asthash();
}

asthash_t AstExpLiteral::asthash() {
    return hstr("literal") + hint(val);
}

asthash_t AstExpFunctionCall::asthash() {
    return hstr("call") + hstr(name) + params->asthash();
}

asthash_t AstExpOpUnary::asthash() {
    return hstr("una") + hint(op) + operand->asthash();
}

asthash_t AstExpOpBinary::asthash() {
    return hstr("bin") + hint(op) + operand1->asthash() + operand2->asthash();
}