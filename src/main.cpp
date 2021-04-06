#include <cstdio>
using namespace std;

extern void yyparse();

int main() {
    printf("main");
    yyparse();
}