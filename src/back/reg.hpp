#pragma once

#include <cassert>

/*

POOLED: tempvar
POOLED(=REG): ref param
POOLED: ref local scalar
=STK: ref local array
=GLB: ref global

*/

struct Preg {
    char cat;
    int index;

    Preg(char cat, int index): cat(cat), index(index) {
        assert(index>=0);
        if(cat=='s')
            assert(index<=11);
        else if(cat=='t')
            assert(index<=6);
        else if(cat=='a')
            assert(index<=7);
        else if(cat=='x')
            assert(index<=0);
        else
            assert(false);
    }

    bool operator==(const Preg &rhs) const {
        return cat == rhs.cat && index == rhs.index;
    }
    bool operator!=(const Preg &rhs) const {
        return !(rhs == *this);
    }
    struct Hash {
        size_t operator()(const Preg &p) const {
            return p.cat*13 + p.index;
        }
    };

    string tigger_ref() {
        char buf[16];
        sprintf(buf, "{reg: %c%d}", cat, index);
        return string(buf);
    }
};

struct Vreg {
    enum VregPos {
        VregInStack, VregInReg
    } pos;

    int stackspan;
    int stackoffset;
    Preg reg;

private:
    Vreg(bool instack):
        pos(instack ? VregInStack : VregInReg), stackspan(0), stackoffset(-1), reg('x', 0) {}

public:
    Vreg(Preg reg):
        pos(VregInReg), stackspan(0), stackoffset(-1), reg(reg) {}
    static Vreg asReg(char cat, int index) {
        return {Preg(cat, index)};
    }
    static Vreg asStack(int elems, int offset) {
        Vreg ret = Vreg(true);
        assert(elems>0);
        assert(offset>=0);
        ret.stackspan = elems;
        ret.stackoffset = offset;
        return ret;
    }

    string tigger_ref() {
        if(pos==VregInReg)
            return reg.tigger_ref();
        else {
            char buf[16];
            if(stackspan==1)
                sprintf(buf, "{stk #%d}", stackoffset);
            else
                sprintf(buf, "{stk #%d +%d}", stackoffset, stackspan);
            return string(buf);
        }
    }
};