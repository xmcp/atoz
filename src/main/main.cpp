#include <cstdio>
#include <cstring>
using namespace std;

#include "gc.hpp"
#include "../front/ast.hpp"

extern int yyparse();
extern void yyrestart(FILE*);

extern bool OUTPUT_REGALLOC_PREFIX;
extern bool OUTPUT_DEF_USE;

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

enum OutputFormat {
    Eeyore, AnalyzedEeyore, Tigger, Asm
} output_format;

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
    if(strcmp(argv[1], "-S")!=0 || strlen(argv[2])!=2 || argv[2][0]!='-' || strcmp(argv[4], "-o")!=0)
        mainerror("argv error");
    
    oj_in = fopen(argv[3], "r");
    oj_out = fopen(argv[5], "w");

    switch(argv[2][1]) {
        case 'e': output_format = Eeyore; break;
        case 'a': output_format = AnalyzedEeyore; break;
        case 't': output_format = Tigger; break;
        case 'm': output_format = Asm; break;
    }
}

int main(int argc, char **argv) {
    bool skip_analyze = false;

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

    if(output_format==Eeyore) {
        OUTPUT_REGALLOC_PREFIX = false;
        OUTPUT_DEF_USE = false;
        skip_analyze = true;
    }

    /// GEN CFG, REG ALLOC, CALC DESTROY SET
    if(!skip_analyze) {
        ir_root->install_builtin_destroy_sets();
        for(const auto& funcpair: ir_root->funcs) {
            auto func = funcpair.first;
            func->connect_all_cfg();
            func->regalloc();
            func->report_destroyed_set();
        }
    }

    /// GEN EEYORE AND OUTPUT
    if(output_format==Eeyore || output_format==AnalyzedEeyore) {
        list<string> eey_buf;
        ir_root->output_eeyore(eey_buf);

        for(const auto &s: eey_buf)
            fprintf(oj_out, "%s\n", s.c_str());

        goto cleanup;
    }

    mainerror("tigger and asm not implemented");

    cleanup:

    /// CLEANUP
    fclose(oj_in);
    fclose(oj_out);
    GarbageCollectable::delete_all();

    return 0;
}