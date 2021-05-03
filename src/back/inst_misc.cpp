#include "inst.hpp"

InstStmt *InstFuncDef::get_last_stmt() {
    if(stmts.empty())
        return new InstComment("");
    else
        return stmts.back();
}