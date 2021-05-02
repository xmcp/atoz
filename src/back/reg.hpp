#pragma once

#include "../main/myassert.hpp"
#include <string>
using std::string;

/*

VREG+POOLED: tempvar
VREG+POOLED(=REG): ref param
VREG+POOLED: ref local scalar
VREG(=STK): ref local array
=GLB: ref global

*/

struct InstFuncDef;

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

    string analyzed_eeyore_ref() {
        char buf[32];
        sprintf(buf, "{reg: %c%d}", cat, index);
        return string(buf);
    }
    string tigger_ref() {
        char buf[32];
        sprintf(buf, "%c%d", cat, index);
        return string(buf);
    }

    void caller_save_before(InstFuncDef *func, int stackoffset);
    void caller_load_after(InstFuncDef *func, int stackoffset);
};

struct Vreg {
    enum VregPos {
        VregInStack, VregInReg
    } pos;

    int spillspan;
    int spilloffset;
    Preg reg;

private:
    Vreg(bool instack):
            pos(instack ? VregInStack : VregInReg), spillspan(0), spilloffset(-1), reg('x', 0) {}

public:
    Vreg(Preg reg):
            pos(VregInReg), spillspan(0), spilloffset(-1), reg(reg) {}
    static Vreg asReg(char cat, int index) {
        return {Preg(cat, index)};
    }
    static Vreg asStack(int elems, int offset) {
        Vreg ret = Vreg(true);
        assert(elems>0);
        assert(offset>=0);
        ret.spillspan = elems;
        ret.spilloffset = offset;
        return ret;
    }

    bool operator==(const Vreg &rhs) const {
        return (
            pos==rhs.pos &&
            (pos==VregInReg ?
                reg==rhs.reg :
                (spilloffset==rhs.spilloffset)
            )
        );
    }
    bool operator!=(const Vreg &rhs) const {
        return !(rhs == *this);
    }
    struct Hash {
        size_t operator()(const Vreg &v) const {
            if(v.pos==VregInReg)
                return Preg::Hash()(v.reg);
            else
                return -v.spilloffset;
        }
    };

    string analyzed_eeyore_ref() {
        if(pos==VregInReg)
            return reg.analyzed_eeyore_ref();
        else {
            char buf[32];
            if(spillspan == 1)
                sprintf(buf, "{stk #%d}", spilloffset);
            else
                sprintf(buf, "{stk #%d +%d}", spilloffset, spillspan);
            return string(buf);
        }
    }

    // spill
    Preg load_into_preg(InstFuncDef *func, int tempreg);
    Preg get_stored_preg();
    void store_onto_stack_if_needed(InstFuncDef *func);
};
