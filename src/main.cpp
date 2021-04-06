#include <cstdio>
using namespace std;

#include "test_lexer.cpp"
#include "test_parser.cpp"

extern int yyparse();

Ast *ast_root;

int main(int argc, char **argv) {
    //test_lexer(argv[1]);
    test_parser(argv[1]);
    //yyparse();
}