#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-convert-member-functions-to-static"

#include <unordered_map>
#include <string>
#include <cassert>
using std::unordered_map;
using std::string;
using std::make_pair;

#include "ast_defs.hpp"

#define lookuperror(...) do { \
    printf("lookup error: "); \
    printf(__VA_ARGS__ ); \
    exit(1); \
} while(0)

template<typename T>
class StackedTable {
    unordered_map<string, T*> table;

public:
    StackedTable *parent;

    StackedTable(StackedTable *parent):
        parent(parent) {
        //printf("new %x from %x\n", (int)(long long)this, (int)(long long)parent);
    }

    void put(string name, T *val) {
        assert(val!=nullptr);
        //printf("put %x %s\n",(int)(long long)this, name.c_str());

        if(table.find(name)!=table.end())
            lookuperror("duplicate symbol: %s", name.c_str());

        table.insert(make_pair(name, val));
    }

    T *get(string name) {
        //printf("get %x %s\n", (int)(long long)this, name.c_str());

        auto it = table.find(name);
        if(it==table.end()) {
            if(!parent) {
                lookuperror("unknown symbol: %s", name.c_str());
                //return nullptr; // impossible
            } else
                return parent->get(name);
        }
        else
            return it->second;
    }
};

class SymTable {
public:
    StackedTable<AstFuncDef> func;
    StackedTable<AstDef> var;

    SymTable(SymTable *parent):
        func(parent ? &parent->func : nullptr),
        var(parent ? &parent->var : nullptr) {}
};

class NameLooker {
    AstCompUnit *root;

public:
    NameLooker(AstCompUnit *root):
        root(root) {}

    #define visitall(list) do { \
        for(auto *def: list) \
            visit(def, tbl); \
    } while(0)

    void visit(AstCompUnit *node, SymTable *tbl) {
        visitall(node->val);
    }

    void visit(AstDecl *node, SymTable *tbl) {
        visit(node->defs, tbl);
    }

    void visit(AstDefs *node, SymTable *tbl) {
        visitall(node->val);
    }

    void visit(AstDef *node, SymTable *tbl) {
        visit(node->idxinfo, tbl);
        if(node->initval_or_null!=nullptr) {
            visit(node->initval_or_null, tbl);
        }
        tbl->var.put(node->name, node);
    }

    void visit(AstMaybeIdx *node, SymTable *tbl) {
        visitall(node->val);
    }

    void visit(AstInitVal *node, SymTable *tbl) {
        if(node->is_many)
            visitall(*node->val.many);
        else
            visit(node->val.single, tbl);
    }

    void visit(AstFuncDef *node, SymTable *tbl) {
        tbl->func.put(node->name, node);
        tbl = new SymTable(tbl);
        visit(node->params, tbl); // params are in local scope
        visit(node->body, tbl);
        delete tbl;
    }

    void visit(AstFuncDefParams *node, SymTable *tbl) {
        visitall(node->val);
    }

    void visit(AstFuncUseParams *node, SymTable *tbl) {
        visitall(node->val);
    }

    void visit(AstBlock *node, SymTable *tbl) {
        tbl = new SymTable(tbl);
        visitall(node->body);
        delete tbl;
    }

    void visit(AstStmtAssignment *node, SymTable *tbl) {
        visit(node->lval, tbl);
        visit(node->rval, tbl);
    }

    void visit(AstStmtExp *node, SymTable *tbl) {
        visit(node->exp, tbl);
    }

    void visit(AstStmtVoid *node, SymTable *tbl) {}

    void visit(AstStmtBlock *node, SymTable *tbl) {
        visit(node->block, tbl);
    }

    void visit(AstStmtIfOnly *node, SymTable *tbl) {
        visit(node->cond, tbl);
        visit(node->body, tbl);
    }

    void visit(AstStmtIfElse *node, SymTable *tbl) {
        visit(node->cond, tbl);
        visit(node->body_true, tbl);
        visit(node->body_false, tbl);
    }

    void visit(AstStmtWhile *node, SymTable *tbl) {
        visit(node->cond, tbl);
        visit(node->body, tbl);
    }

    void visit(AstStmtBreak *node, SymTable *tbl) {}

    void visit(AstStmtContinue *node, SymTable *tbl) {}

    void visit(AstStmtReturnVoid *node, SymTable *tbl) {}

    void visit(AstStmtReturn *node, SymTable *tbl) {
        visit(node->retval, tbl);
    }

    void visit(AstExpLVal *node, SymTable *tbl) {
        node->def = tbl->var.get(node->name);
        if(node->def->idxinfo->val.size() != node->idxinfo->val.size())
            lookuperror(
                "lval shape mismatch: %s, expect %d, actual %d\n",
                node->name.c_str(),
                (int)node->idxinfo->val.size(),
                (int)node->def->idxinfo->val.size()
            );
        visit(node->idxinfo, tbl);
    }

    void visit(AstExpLiteral *node, SymTable *tbl) {}

    void visit(AstExpFunctionCall *node, SymTable *tbl) {
        node->def = tbl->func.get(node->name);
        if(node->def->params->val.size() != node->params->val.size())
            lookuperror(
                "param shape mismatch: %s, expect %d, actual %d\n",
                node->name.c_str(),
                (int)node->params->val.size(),
                (int)node->def->params->val.size()
            );
    }

    void visit(AstExpOpUnary *node, SymTable *tbl) {
        visit(node->operand, tbl);
    }

    void visit(AstExpOpBinary *node, SymTable *tbl) {
        visit(node->operand1, tbl);
        visit(node->operand2, tbl);
    }

    void visit(Ast *node, SymTable *tbl) {
        #define istype(ptr, cls) (dynamic_cast<cls*>(ptr)!=nullptr)
        #define proctype(type) do { \
            if(istype(node, type)) {visit((type*)node, tbl); return;} \
        } while(0)

        proctype(AstCompUnit);
        proctype(AstDecl);
        proctype(AstDefs);
        proctype(AstDef);
        proctype(AstMaybeIdx);
        proctype(AstInitVal);
        proctype(AstFuncDef);
        proctype(AstFuncDefParams);
        proctype(AstFuncUseParams);
        proctype(AstBlock);
        proctype(AstStmtAssignment);
        proctype(AstStmtExp);
        proctype(AstStmtVoid);
        proctype(AstStmtBlock);
        proctype(AstStmtIfOnly);
        proctype(AstStmtIfElse);
        proctype(AstStmtWhile);
        proctype(AstStmtBreak);
        proctype(AstStmtContinue);
        proctype(AstStmtReturnVoid);
        proctype(AstStmtReturn);
        proctype(AstExpLVal);
        proctype(AstExpLiteral);
        proctype(AstExpFunctionCall);
        proctype(AstExpOpUnary);
        proctype(AstExpOpBinary);

        printf("!! ignored node\n");
    }

    void install_builtin_names(SymTable *tbl) {
        tbl->func.put("getint", new AstFuncDef(
            FuncInt, "getint", new AstFuncDefParams(), new AstBlock()
        ));

        tbl->func.put("getch", new AstFuncDef(
            FuncInt, "getch", new AstFuncDefParams(), new AstBlock()
        ));

        auto *param_idx_single = new AstMaybeIdx();
        param_idx_single->push_val(new AstExpLiteral(-1));
        auto *param_arr = new AstFuncDefParams();
        param_arr->push_val(new AstDef(
            "_arg", param_idx_single, nullptr // int[]
        ));
        tbl->func.put("getarray", new AstFuncDef(
            FuncInt, "getarray", param_arr, new AstBlock()
        ));

        auto *param_int = new AstFuncDefParams();
        param_int->push_val(new AstDef(
            "_arg", new AstMaybeIdx(), nullptr // int
        ));
        tbl->func.put("putint", new AstFuncDef(
            FuncVoid, "putint", param_int, new AstBlock()
        ));

        tbl->func.put("putch", new AstFuncDef(
            FuncVoid, "putch", param_int, new AstBlock()
        ));

        auto *param_arr2 = new AstFuncDefParams();
        param_arr2->push_val(new AstDef(
            "_arg", new AstMaybeIdx(), nullptr // int
        ));
        param_arr2->push_val(new AstDef(
            "_arg2", param_idx_single, nullptr // int[]
        ));
        tbl->func.put("putarray", new AstFuncDef(
            FuncVoid, "putarray", param_arr2, new AstBlock()
        ));

        tbl->func.put("starttime", new AstFuncDef(
            FuncVoid, "starttime", new AstFuncDefParams(), new AstBlock()
        ));

        tbl->func.put("stoptime", new AstFuncDef(
            FuncVoid, "stoptime", new AstFuncDefParams(), new AstBlock()
        ));
    }

    void lookup_main() {
        auto *tbl = new SymTable(nullptr);
        install_builtin_names(tbl);
        visit(root, tbl);
        delete tbl;
    }
};
#pragma clang diagnostic pop