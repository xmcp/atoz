#include "../main/common.hpp"
#include "ir.hpp"

bool IrFuncDef::peekhole_optimize() {
    bool changed = false;
    auto lastit = stmts.end();

    for(auto it=stmts.begin(); it!=stmts.end();) {
        if(istype(it->first, IrMov)) {
            auto *movstmt = (IrMov*)it->first;
            if(movstmt->src.type==RVal::TempVar && lastit!=stmts.end()) {
                /*
                 * tempvar = op1 op op2   <- last
                 * T... = tempvar         <- movstmt
                 *
                 * -- can be optimized to --
                 *
                 * T... = op1 op op2      <- last
                 */

                auto last = lastit->first;

                if(istype(last, IrOpBinary)) {
                    ((IrOpBinary*)last)->dest = movstmt->dest;
                    it = stmts.erase(it);
                    changed = true; continue;
                } else if(istype(last, IrOpUnary)) {
                    ((IrOpUnary*)last)->dest = movstmt->dest;
                    it = stmts.erase(it);
                    changed = true; continue;
                } else if(istype(last, IrCall)) {
                    ((IrCall*)last)->ret = movstmt->dest;
                    it = stmts.erase(it);
                    changed = true; continue;
                } else if(istype(last, IrMov)) {
                    ((IrMov*)last)->dest = movstmt->dest;
                    it = stmts.erase(it);
                    changed = true; continue;
                }
            }
        } else if(istype(it->first, IrCondGoto)) {
            auto *gotostmt = (IrCondGoto*)it->first;
            if(
                gotostmt->operand1.type==RVal::TempVar && gotostmt->operand2.type==RVal::ConstExp
                && gotostmt->operand2.val.constexp==0
            ) {
                if(lastit!=stmts.end() && istype(lastit->first, IrOpBinary)) {
                    /*
                     * tempvar = op1 rel op2       <- last
                     * if tempvar == 0 goto label  <- gotostmt
                     *
                     * -- can be optimized to --
                     *
                     * if op1 !rel op2 goto label  <- last
                     */

                    auto lastbin = (IrOpBinary*)lastit->first;
                    RelKinds relop = cvt_to_rel(lastbin->op);

                    if(
                        lastbin->dest==gotostmt->operand1 && relop!=NotARel
                        && (gotostmt->op==RelEq || gotostmt->op==RelNeq)
                    ) {
                        gotostmt->operand1 = lastbin->operand1;
                        gotostmt->operand2 = lastbin->operand2;
                        gotostmt->op = gotostmt->op==RelEq ? rel_invert(relop) : relop;
                        stmts.erase(lastit);
                        lastit = it;
                        it++;
                        changed = true; continue;
                    }
                }
            }
        }

        // nothing to optimize here, continue
        lastit = it;
        it++;
    }

    return changed;
}