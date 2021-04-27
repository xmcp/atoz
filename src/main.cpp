#include <cstdio>
#include <cstring>
using namespace std;

#include "ast.hpp"

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
    if(argc!=6)
        mainerror("argc count is %d", argc);
    if(strcmp(argv[1], "-S")!=0 || strcmp(argv[2], "-e")!=0 || strcmp(argv[4], "-o")!=0)
        mainerror("argv error");
    
    oj_in = fopen(argv[3], "r");
    oj_out = fopen(argv[5], "w");
}

int main(int argc, char **argv) {
    /**
    // LOCAL TEST
    
    FILE *f = openyyfile(argv[1]);

    //test_lexer();

    ast_root = nullptr;

    yyparse();
    printf("parsed, will complete tree\n");
    ast_root->complete_tree();
    printf("will gen eeyore\n");
    eeyore_world.clear();
    ast_root->gen_ir();
    eeyore_world.print(stdout);

    fclose(f);
    printf("TEST PASSED!\n");
    Ast::delete_all();

    /*/
    // OJ

    parse_oj_args(argc, argv);
    yyrestart(oj_in);

    yyparse();

    ast_root->complete_tree();

    auto *ir_root = new IrRoot();
    ast_root->gen_ir(ir_root);

    vector<string> eey_buf;
    ir_root->output_eeyore(eey_buf);
    for(const auto &s: eey_buf)
        printf("%s\n", s.c_str());

    fclose(oj_in);
    fclose(oj_out);
    Ast::delete_all();
    Ir::delete_all();

    //*/

    return 0;
}