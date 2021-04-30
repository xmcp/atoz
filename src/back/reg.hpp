#pragma once

#include <cassert>

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
    Preg reg;

private:
    Vreg(bool instack):
        pos(instack ? VregInStack : VregInReg), stackspan(0), reg('x', 0) {}

public:
    Vreg(Preg reg):
        pos(VregInReg), stackspan(0), reg(reg) {}
    static Vreg asReg(char cat, int index) {
        return {Preg(cat, index)};
    }
    static Vreg asStack(int elems) {
        Vreg ret = Vreg(true);
        assert(elems>0);
        ret.stackspan = elems;
        return ret;
    }

    string tigger_ref() {
        if(pos==VregInReg)
            return reg.tigger_ref();
        else {
            char buf[16];
            sprintf(buf, "{stk *%d}", stackspan);
            return string(buf);
        }
    }
};