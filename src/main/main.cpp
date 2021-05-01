#include <cstdio>
#include <cstring>
using namespace std;

#include "../front/ast.hpp"

extern int yyparse();
extern void yyrestart(FILE*);

#define mainerror(...) do { \
    printf("main error: "); \
    printf(__VA_ARGS__ ); \
    exit(1); \
} while(0)

AstCompUnit *ast_root;

FILE *openyyfile(string fn) {
    printf("open file for test: %s\n", fn.c_str());

    FILE *f = fopen(fn.c_str(), "r");
    yyrestart(f);

    printf("opened\n");
    return f;
}

FILE *oj_in, *oj_out;

void parse_oj_args(int argc, char **argv) {
    if(argc==5) { // to asm
        char **new_argv = new char*[6];
        static char flag[] = "-m";

        new_argv[1] = argv[1]; // -S
        new_argv[2] = flag;
        new_argv[3] = argv[2]; // input
        new_argv[4] = argv[3]; // -o
        new_argv[5] = argv[4]; // output

        argc = 6;
        argv = new_argv;
    }

    if(argc!=6)
        mainerror("argc count is %d", argc);
    if(strcmp(argv[1], "-S")!=0 || strcmp(argv[2], "-e")!=0 || strcmp(argv[4], "-o")!=0)
        mainerror("argv error");
    
    oj_in = fopen(argv[3], "r");
    oj_out = fopen(argv[5], "w");
}

int main(int argc, char **argv) {
    /// PREPARE
    parse_oj_args(argc, argv);
    yyrestart(oj_in);

    /// PARSE
    yyparse();

    /// COMPLETE TREE
    ast_root->complete_tree();

    /// GEN IR
    auto *ir_root = new IrRoot();
    ast_root->gen_ir(ir_root);

    /// GEN CFG, REG ALLOC, CALC DESTROY SET
    ir_root->install_builtin_destroy_sets();
    for(const auto& funcpair: ir_root->funcs) {
        auto func = funcpair.first;
        func->connect_all_cfg();
        func->regalloc();
        func->report_destroyed_set();
    }

    /// GEN EEYORE
    list<string> eey_buf;
    ir_root->output_eeyore(eey_buf);

    /// OUTPUT
    for(const auto &s: eey_buf)
        fprintf(oj_out, "%s\n", s.c_str());

    /// CLEANUP
    fclose(oj_in);
    fclose(oj_out);
    Ast::delete_all();
    Ir::delete_all();

    return 0;
}