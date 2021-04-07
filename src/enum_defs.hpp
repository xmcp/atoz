#pragma once

#include <cassert>

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
    OpPos, OpNeg, OpNot
};

enum BinaryOpKinds {
    OpPlus, OpMinus, OpMul, OpDiv, OpMod,
    OpLess, OpGreater, OpLeq, OpGeq, OpEq, OpNeq,
    OpAnd, OpOr
};

// helper func to conver lexer type -> parser type

inline UnaryOpKinds cvt_to_unary(LexTypeAdd op) {
    if(op==LexPlus) return OpPos;
    if(op==LexMinus) return OpNeg;
    assert(false);
}

inline BinaryOpKinds cvt_to_binary(LexTypeAdd op) {
    if(op==LexPlus) return OpPlus;
    if(op==LexMinus) return OpMinus;
    assert(false);
}

inline BinaryOpKinds cvt_to_binary(LexTypeMul op) {
    if(op==LexMul) return OpMul;
    if(op==LexDiv) return OpDiv;
    if(op==LexMod) return OpMod;
    assert(false);
}

inline BinaryOpKinds cvt_to_binary(LexTypeRel op) {
    if(op==LexLess) return OpLess;
    if(op==LexGreater) return OpGreater;
    if(op==LexLeq) return OpLeq;
    if(op==LexGeq) return OpGeq;
    assert(false);
}

inline BinaryOpKinds cvt_to_binary(LexTypeEq op) {
    if(op==LexEq) return OpEq;
    if(op==LexNeq) return OpNeq;
    assert(false);
}

// def position

enum DefPosition { DefUnknown, DefGlobal, DefLocal, DefArg };