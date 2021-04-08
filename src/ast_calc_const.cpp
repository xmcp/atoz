#include "enum_defs.hpp"
#include "ast.hpp"

ConstExpResult do_unary_op(UnaryOpKinds op, int val) {
    switch(op) {
        case OpPos:
            return val;
        case OpNeg:
            return -val;
        case OpNot:
            return !val;
        default:
            assert(false); return ConstExpResult::asError("impossible");
    }
}

ConstExpResult do_binary_op(BinaryOpKinds op, int val1, int val2) {
    switch(op) {
        case OpPlus:
            return val1+val2;
        case OpMinus:
            return val1-val2;
        case OpMul:
            return val1*val2;
        case OpDiv:
            if(val2==0)
                return ConstExpResult::asError("divide by zero");
            return val1/val2;
        case OpMod:
            if(val2==0)
                return ConstExpResult::asError("mod by zero");
            return val1%val2;
        case OpLess:
            return val1<val2;
        case OpGreater:
            return val1>val2;
        case OpLeq:
            return val1<=val2;
        case OpGeq:
            return val1>=val2;
        case OpEq:
            return val1==val2;
        case OpNeq:
            return val1!=val2;
        case OpAnd:
            return val1&&val2;
        case OpOr:
            return val1||val2;
        default:
            assert(false); return ConstExpResult::asError("impossible");
    }
}

ConstExpResult AstExpLVal::calc_const() {
    if(!def->is_const)
        return ConstExpResult::asError("lval not const");

    def->initval.calc_if_needed(def->ast_initval_or_null);
    return def->initval.getvalue(idxinfo);
}

ConstExpResult AstExpLiteral::calc_const() {
    return val;
}

ConstExpResult AstExpFunctionCall::calc_const() {
    return ConstExpResult::asError("function call is not constexpr");
}

ConstExpResult AstExpOpUnary::calc_const() {
    ConstExpResult val = operand->get_const();
    if(val.iserror)
        return val;

    return do_unary_op(op, val.val);
}

ConstExpResult AstExpOpBinary::calc_const() {
    ConstExpResult val1 = operand1->get_const();
    if(val1.iserror)
        return val1;
    ConstExpResult val2 = operand2->get_const();
    if(val2.iserror)
        return val2;

    return do_binary_op(op, val1.val, val2.val);
}
