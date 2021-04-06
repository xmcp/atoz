#pragma once

// used in yylval union

enum LexTypeAdd { LexPlus, LexMinus };

enum LexTypeMul { LexMul, LexDiv, LexMod };

enum LexTypeRel { LexLess, LexGreater, LexLeq, LexGeq };

enum LexTypeEq { LexEq, LexNeq };

// used in parser

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