#include "ast_defs.hpp"

ConstExpResult AstExpLVal::calc_const() {
    return ConstExpResult::asError("not implemented");
}

ConstExpResult AstExpLiteral::calc_const() {
    return ConstExpResult::asError("not implemented");
}

ConstExpResult AstExpFunctionCall::calc_const() {
    return ConstExpResult::asError("not implemented");
}

ConstExpResult AstExpOpUnary::calc_const() {
    return ConstExpResult::asError("not implemented");
}

ConstExpResult AstExpOpBinary::calc_const() {
    return ConstExpResult::asError("not implemented");
}
