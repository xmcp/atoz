#include <cstdio>
using namespace std;

#include "test_lexer.cpp"

extern int yyparse();

AstCompUnit *ast_root;

FILE *openyyfile(string fn) {
    printf("open file for test: %s\n", fn.c_str());

    FILE *f = fopen(fn.c_str(), "r");
    yyrestart(f);

    printf("opened\n");
    return f;
}

int main(int argc, char **argv) {
    FILE *f = openyyfile(argv[1]);

    //test_lexer();

    ast_root = nullptr;

    yyparse();
    printf("parsed, will lookup\n");
    ast_root->lookup_name();

    fclose(f);
    printf("TEST PASSED!\n");
    return 0;
}