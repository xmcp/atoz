/**
 * In this phase, we construct a complete AST from parse result.
 * Variable (function) names are mapped to its def_or_null.
 * We also parse initval and array index here.
 * Some type checking are done.
 */

#include <unordered_map>
#include <string>
using std::unordered_map;
using std::string;
using std::make_pair;

#include "../main/common.hpp"
#include "ast.hpp"

#define lookuperror(...) do { \
    printf("name lookup error: "); \
    printf(__VA_ARGS__ ); \
    exit(1); \
} while(0)

#define typeerror(...) do { \
    printf("type error: "); \
    printf(__VA_ARGS__ ); \
    exit(1); \
} while(0)

template<typename T>
class StackedTable {
    unordered_map<string, T*> table;
    bool notfound_ok;

public:
    StackedTable *parent;

    StackedTable(StackedTable *parent, bool notfound_ok):
        parent(parent), notfound_ok(notfound_ok) {
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
                if(notfound_ok)
                    return nullptr;
                else
                    lookuperror("unknown symbol: %s", name.c_str());
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
    StackedTable<Ast> special;

    SymTable(SymTable *parent):
        func(parent ? &parent->func : nullptr, false),
        var(parent ? &parent->var : nullptr, false),
        special(parent ? &parent->special : nullptr, true) {}
};

class TreeCompleter {
    AstCompUnit *root;

public:
    TreeCompleter(AstCompUnit *root):
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
        if(node->ast_initval_or_null != nullptr) {
            visit(node->ast_initval_or_null, tbl);
        }
        node->calc_initval();
        tbl->var.put(node->name, node);

        if(node->pos==DefLocal) {
            AstFuncDef *func = (AstFuncDef*)tbl->special.get("FuncDef");
            if(!func)
                lookuperror("local var not in funcdef scope: %s", node->name.c_str());
            //func->defs_inside.push_back(node);
        }

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
        tbl->special.put("FuncDef", node);
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
        tbl = new SymTable(tbl);
        tbl->special.put("StmtWhile", node);
        visit(node->body, tbl);
        delete tbl;
    }

    void visit(AstStmtBreak *node, SymTable *tbl) {
        AstStmtWhile *loop = (AstStmtWhile*)tbl->special.get("StmtWhile");
        if(!loop)
            typeerror("break stmt not in loop");
        node->loop = loop;
    }

    void visit(AstStmtContinue *node, SymTable *tbl) {
        AstStmtWhile *loop = (AstStmtWhile*)tbl->special.get("StmtWhile");
        if(!loop)
            typeerror("continue stmt not in loop");
        node->loop = loop;
    }

    void visit(AstStmtReturnVoid *node, SymTable *tbl) {
        AstFuncDef *func = (AstFuncDef*)tbl->special.get("FuncDef");
        if(!func)
            typeerror("return void not in funcdef scope");
        if(func->type!=FuncVoid)
            typeerror("return void in non-void function %s", func->name.c_str());
    }

    void visit(AstStmtReturn *node, SymTable *tbl) {
        visit(node->retval, tbl);

        AstFuncDef *func = (AstFuncDef*)tbl->special.get("FuncDef");
        if(!func)
            typeerror("return not in funcdef scope");
        if(func->type==FuncVoid)
            typeerror("return in void function %s", func->name.c_str());
    }

    void visit(AstExpLVal *node, SymTable *tbl) {
        node->def = tbl->var.get(node->name);
        node->dim_left = (int)(node->def->idxinfo->dims() - node->idxinfo->dims());
        if(node->dim_left<0)
            typeerror(
                "lval shape mismatch: %s, defined %d, index actual %d\n",
                node->name.c_str(),
                (int)node->def->idxinfo->dims(),
                (int)node->idxinfo->dims()
            );
        visit(node->idxinfo, tbl);
    }

    void visit(AstExpLiteral *node, SymTable *tbl) {}

    void visit(AstExpFunctionCall *node, SymTable *tbl) {
        node->def = tbl->func.get(node->name);
        visitall(node->params->val);

        // check number of params
        if(node->def->params->val.size() != node->params->val.size())
            typeerror(
                "param number mismatch: %s, expect %d, actual %d\n",
                node->name.c_str(),
                (int)node->params->val.size(),
                (int)node->def->params->val.size()
            );

        // check each param depth
        for(int i=0; i<(int)node->def->params->val.size(); i++) {
            int expect_depth = node->def->params->val[i]->idxinfo->dims();
            auto *lval = dynamic_cast<AstExpLVal*>(node->params->val[i]);

            if(expect_depth>0) { // expected array
                if(!lval)
                    typeerror("function `%s` param %d expects array", node->name.c_str(), i);
                int var_depth = lval->def->idxinfo->dims();
                int idx_depth = lval->idxinfo->dims();
                if(expect_depth!=var_depth-idx_depth)
                    typeerror(
                        "function `%s` param %d expects depth %d but got %d - %d",
                        node->name.c_str(), i,
                        expect_depth, var_depth, idx_depth
                    );
            } else { // expected primitive
                if(!lval) { // not a lval
                    // good, because exp is always primitive
                } else { // if is a lval, check depth should be 0
                    int var_depth = lval->def->idxinfo->dims();
                    int idx_depth = lval->idxinfo->dims();
                    if(var_depth!=idx_depth)
                        typeerror(
                            "function `%s` param %d expects primitive but got %d - %d",
                            node->name.c_str(), i,
                            var_depth, idx_depth
                        );
                }
            }
        }
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

        typeerror("unknown node type\n");
    }

    void install_builtin_names(SymTable *tbl) {
        tbl->func.put("getint", new AstFuncDef(
            FuncInt, "getint", new AstFuncDefParams(), new AstBlock()
        ));

        tbl->func.put("getch", new AstFuncDef(
            FuncInt, "getch", new AstFuncDefParams(), new AstBlock()
        ));

        auto *param_idx_single = new AstMaybeIdx();
        param_idx_single->push_val(new AstExpLiteral(1));
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

    void complete_tree_main() {
        auto *tbl = new SymTable(nullptr);
        install_builtin_names(tbl);
        visit(root, tbl);
        delete tbl;
    }
};