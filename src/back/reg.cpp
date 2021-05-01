#include "reg.hpp"
#include "inst.hpp"

Preg Vreg::load_into_preg(InstFuncDef *func, int tempregidx) {
    assert(tempregidx == 0 || tempregidx == 1);
    if(pos==VregInStack) {
        Preg tmpreg = Preg('t', tempregidx);
        func->push_stmt(new InstLoadStack(tmpreg, spilloffset));
        return tmpreg;
    } else {
        return reg;
    }
}

static const Preg PREG_STORE = Preg('t', 0);

Preg Vreg::get_stored_preg() {
    if(pos==VregInReg)
        return reg;
    else
        return PREG_STORE;
}

void Vreg::store_onto_stack_if_needed(InstFuncDef *func) {
    if(pos==VregInStack) {
        func->push_stmt(new InstStoreStack(spilloffset, PREG_STORE));
    }
}

void Preg::caller_save_before(InstFuncDef *func, int stackoffset) {
    func->push_stmt(new InstStoreStack(stackoffset, *this));
}

void Preg::caller_load_after(InstFuncDef *func, int stackoffset) {
    func->push_stmt(new InstLoadStack(*this, stackoffset));
}
