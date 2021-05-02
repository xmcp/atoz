#pragma once

#include "../main/myassert.hpp"
#include <string>
using std::string;

// lexer type

enum LexTypeAdd { LexPlus, LexMinus };

enum LexTypeMul { LexMul, LexDiv, LexMod };

enum LexTypeRel { LexLess, LexGreater, LexLeq, LexGeq };

enum LexTypeEq { LexEq, LexNeq };

// parser type

enum VarType { VarInt };

enum FuncType { FuncVoid, FuncInt };

enum StmtKinds {
    StmtAssignment, StmtExp, StmtVoid, StmtBlock,
    StmtIfOnly, StmtIfElse, StmtWhile,
    StmtBreak, StmtContinue,
    StmtReturnVoid, StmtReturn
};

enum UnaryOpKinds {
    OpPos, OpNeg, OpNot // + - !
};

enum BinaryOpKinds {
    OpPlus, OpMinus, OpMul, OpDiv, OpMod, // + - * / %
    OpLess, OpGreater, OpLeq, OpGeq, OpEq, OpNeq, // < > <= >= == !=
    OpAnd, OpOr // && ||
};

enum RelKinds { // can be represented by asm
    RelLess, RelGreater, RelLeq, RelGeq, RelEq, RelNeq
};

// helper func to conver lexer type -> parser type

inline UnaryOpKinds cvt_to_unary(LexTypeAdd op) {
    switch(op) {
        case LexPlus: return OpPos;
        case LexMinus: return OpNeg;
        default: assert(false);
    }
}

inline BinaryOpKinds cvt_to_binary(LexTypeAdd op) {
    switch(op) {
        case LexPlus: return OpPlus;
        case LexMinus: return OpMinus;
        default: assert(false);
    }
}

inline BinaryOpKinds cvt_to_binary(LexTypeMul op) {
    switch(op) {
        case LexMul: return OpMul;
        case LexDiv: return OpDiv;
        case LexMod: return OpMod;
        default: assert(false);
    }
}

inline BinaryOpKinds cvt_to_binary(LexTypeRel op) {
    switch(op) {
        case LexLess: return OpLess;
        case LexGreater: return OpGreater;
        case LexLeq: return OpLeq;
        case LexGeq: return OpGeq;
        default: assert(false);
    }
}

inline BinaryOpKinds cvt_to_binary(LexTypeEq op) {
    switch(op) {
        case LexEq: return OpEq;
        case LexNeq: return OpNeq;
        default: assert(false);
    }
}

inline string cvt_from_unary(UnaryOpKinds op) {
    switch(op) {
        case OpPos: return ""; // `t0 = t1` instead of `t0 = + t1`, because eeyore have no OpPos
        case OpNeg: return "-";
        case OpNot: return "!";
        default: assert(false); return "";
    }
}

inline string cvt_from_binary(BinaryOpKinds op) {
    switch(op) {
        case OpPlus: return "+";
        case OpMinus: return "-";
        case OpMul: return "*";
        case OpDiv: return "/";
        case OpMod: return "%";
        case OpLess: return "<";
        case OpGreater: return ">";
        case OpLeq: return "<=";
        case OpGeq: return ">=";
        case OpEq: return "==";
        case OpNeq: return "!=";
        case OpAnd: return "&&";
        case OpOr: return "||";
        default: assert(false); return "";
    }
}

inline string cvt_from_binary(RelKinds op) {
    switch(op) {
        case RelLess: return "<";
        case RelGreater: return ">";
        case RelLeq: return "<=";
        case RelGeq: return ">=";
        case RelEq: return "==";
        case RelNeq: return "!=";
        default: assert(false); return "";
    }
}

// def position

enum DefPosition { DefUnknown, DefGlobal, DefLocal, DefArg };
