#include <string>
using std::string;

#include "enum_defs.hpp"
#include "ast_defs.hpp"
#include "sysy.tab.hpp"

extern int yylex();
extern void yyrestart(FILE*);
extern Ast *ast_root;

inline void test_parser(string fn) {
    printf("open file for test: %s\n", fn.c_str());

    FILE *f = fopen(fn.c_str(), "r");
    yyrestart(f);

    printf("opened\n");

    ast_root = nullptr;
    int ret = yyparse();
    if(ret || ast_root==nullptr) {
        printf("PARSE FAILED %d\n", ret);
        return;
    }

    printf("TEST PASSED!\n");
}