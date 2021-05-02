#include "inst.hpp"

const int ASM_INST_BUFSIZE = 512;

static char instbuf[ASM_INST_BUFSIZE];

#define outasm(...) do { \
    snprintf(instbuf, sizeof(instbuf), __VA_ARGS__); \
    buf.push_back(string(instbuf)); \
} while(0)

#define outstmt(...) do { \
    outasm("    " __VA_ARGS__); \
} while(0)

void InstRoot::output_asm(list<string> &buf) {
    outasm("# BEGIN ASM");

    outasm("#--- SCALAR DECL");
    for(auto scalarpair: decl_scalars)
        scalarpair.second->output_asm(buf);

    outasm("");
    outasm("#--- ARRAY DECL");
    for(auto arraypair: decl_arrays)
        arraypair.second->output_asm(buf);

    outasm("");
    for(auto fn: funcs) {
        fn->output_asm(buf);
        outasm("");
    }

    outasm("# END ASM");
}

void InstDeclScalar::output_asm(list<string> &buf) {
    outasm("  .global   global_var");
    outasm("  .section  .sdata");
    outasm("  .align    2");
    outasm("  .type     global_var, @object");
    outasm("  .size     global_var, 4");
    outasm("v%d:", globalidx);
    outasm("  .word     %d", initval);
}

void InstDeclArray::output_asm(list<string> &buf) {
    outasm("  .comm v%d, %d, 4", globalidx, totbytes);
}

#define STK(stacksize) (((stacksize)/4 + 1) * 16)

void InstFuncDef::output_asm(list<string> &buf) {
    outasm("#--- FUNCTION %s", name.c_str());
    outasm("  .text");
    outasm("  .align  2");
    outasm("  .global %s", name.c_str());
    outasm("  .type   %s, @function", name.c_str());
    outasm("");
    outasm("%s:", name.c_str());
    outasm("  addi    sp, sp, -%d", STK(stacksize)); // todo: deal with big numbers
    outasm("  sw      ra, %d(sp)", STK(stacksize)-4); // todo: deal with big numbers

    for(auto stmt: stmts)
        stmt->output_asm(buf);

    outasm("  .size   %s, .-%s", name.c_str(), name.c_str());
}

#define tig(p) (p.tigger_ref().c_str())

void InstOpBinary::output_asm(list<string> &buf) {
    switch(op) {
        case OpPlus:
            outstmt("add %s, %s, %s", tig(dest), tig(operand1), tig(operand2));
            break;
        case OpMinus:
            outstmt("sub %s, %s, %s", tig(dest), tig(operand1), tig(operand2));
            break;
        case OpMul:
            outstmt("mul %s, %s, %s", tig(dest), tig(operand1), tig(operand2));
            break;
        case OpDiv:
            outstmt("div %s, %s, %s", tig(dest), tig(operand1), tig(operand2));
            break;
        case OpMod:
            outstmt("rem %s, %s, %s", tig(dest), tig(operand1), tig(operand2));
            break;
        case OpLess:
            outstmt("slt %s, %s, %s", tig(dest), tig(operand1), tig(operand2));
            break;
        case OpGreater:
            outstmt("sgt %s, %s, %s", tig(dest), tig(operand1), tig(operand2));
            break;
        case OpLeq:
            outstmt("sgt %s, %s, %s", tig(dest), tig(operand1), tig(operand2));
            outstmt("seqz %s, %s", tig(dest), tig(dest));
            break;
        case OpGeq:
            outstmt("slt %s, %s, %s", tig(dest), tig(operand1), tig(operand2));
            outstmt("seqz %s, %s", tig(dest), tig(dest));
            break;
        case OpEq:
            outstmt("xor %s, %s, %s", tig(dest), tig(operand1), tig(operand2));
            outstmt("seqz %s, %s", tig(dest), tig(dest));
            break;
        case OpNeq:
            outstmt("xor %s, %s, %s", tig(dest), tig(operand1), tig(operand2));
            outstmt("snez %s, %s", tig(dest), tig(dest));
            break;
        case OpAnd:
        case OpOr:
        default:
            assert(false);
            assert(false);
            break;
    }
}

void InstOpUnary::output_asm(list<string> &buf) {
    switch(op) {
        case OpPos:
            outstmt("mv %s, %s", tig(dest), tig(operand));
            break;
        case OpNeg:
            outstmt("neg %s, %s", tig(dest), tig(operand));
            break;
        case OpNot:
            outstmt("seqz %s, %s", tig(dest), tig(operand));
            break;
        default:
            assert(false);
            break;
    }
}

void InstMov::output_asm(list<string> &buf) {
    outstmt("mv %s, %s", tig(dest), tig(src));
}

void InstLoadImm::output_asm(list<string> &buf) {
    outstmt("li %s, %d", tig(dest), imm); // todo: deal with big numbers
}

void InstArraySet::output_asm(list<string> &buf) {
    outstmt("sw %s, %d(%s)", tig(src), doffset, tig(dest)); // todo: deal with big numbers
}

void InstArrayGet::output_asm(list<string> &buf) {
    outstmt("lw %s, %d(%s)", tig(dest), soffset, tig(src)); // todo: deal with big numbers
}

void InstCondGoto::output_asm(list<string> &buf) {
    switch(op) {
        case RelLess:
            outstmt("blt %s, %s, .l%d", tig(operand1), tig(operand2), label);
            break;
        case RelGreater:
            outstmt("bgt %s, %s, .l%d", tig(operand1), tig(operand2), label);
            break;
        case RelLeq:
            outstmt("ble %s, %s, .l%d", tig(operand1), tig(operand2), label);
            break;
        case RelGeq:
            outstmt("bge %s, %s, .l%d", tig(operand1), tig(operand2), label);
            break;
        case RelEq:
            outstmt("beq %s, %s, .l%d", tig(operand1), tig(operand2), label);
            break;
        case RelNeq:
            outstmt("bne %s, %s, .l%d", tig(operand1), tig(operand2), label);
            break;
        default:
            assert(false);
            break;
    }
}

void InstGoto::output_asm(list<string> &buf) {
    outstmt("j .l%d", label);
}

void InstLabel::output_asm(list<string> &buf) {
    outstmt(".l%d:", label);
}

void InstCall::output_asm(list<string> &buf) {
    outstmt("call %s", name.c_str());
}

void InstRet::output_asm(list<string> &buf) {
    outstmt("lw ra, %d(sp)", STK(fn_stacksize)-4); // todo: deal with big numbers
    outstmt("addi sp, sp, %d", STK(fn_stacksize)); // todo: deal with big numbers
    outstmt("ret");
}

void InstStoreStack::output_asm(list<string> &buf) {
    outstmt("sw %s, %d(sp)", tig(src), stackidx*4); // todo: deal with big numbers
}

void InstLoadStack::output_asm(list<string> &buf) {
    outstmt("lw %s, %d(sp)", tig(dest), stackidx*4); // todo: deal with big numbers
}

void InstLoadGlobal::output_asm(list<string> &buf) {
    outstmt("lui %s, %%hi(v%d)", tig(dest), globalidx);
    outstmt("lw %s, %%lo(v%d)(%s)", tig(dest), globalidx, tig(dest));
}

void InstLoadAddrStack::output_asm(list<string> &buf) {
    outstmt("addi %s, sp, %d", tig(dest), stackidx*4); // todo: deal with big numbers
}

void InstLoadAddrGlobal::output_asm(list<string> &buf) {
    outstmt("la %s, v%d", tig(dest), globalidx);
}

// https://stackoverflow.com/questions/5878775/how-to-find-and-replace-string
string str_replace(string data, string toSearch, string replaceStr) {
    size_t pos = data.find(toSearch);
    while(pos != std::string::npos) {
        // Replace this occurrence of Sub String
        data.replace(pos, toSearch.size(), replaceStr);
        // Get the next occurrence from the current position
        pos = data.find(toSearch, pos + replaceStr.size());
    }
    return data;
}

void InstComment::output_asm(list<string> &buf) {
    if(comment.empty())
        outasm("");
    else
        outasm("# _ %s", str_replace(comment, "//", "# _").c_str());
}

#undef tig
#undef outasm
