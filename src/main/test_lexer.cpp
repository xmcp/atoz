#include <string>
using std::string;

#include "../front/enum_defs.hpp"
#include "../front/ast.hpp"
#include "../front/sysy.tab.hpp"

extern int yylex();
extern void yyrestart(FILE*);

inline void describe_token(int token) {
    //printf("got token ");
    switch(token) {
        case KW_CONST:
            printf("<const> "); break;
        case KW_INT:
            printf("<int> "); break;
        case KW_VOID:
            printf("<void> "); break;
        case KW_IF:
            printf("<if> "); break;
        case KW_ELSE:
            printf("<else> "); break;
        case KW_WHILE:
            printf("<while> "); break;
        case KW_BREAK:
            printf("<break> "); break;
        case KW_CONTINUE:
            printf("<continue> "); break;
        case KW_RETURN:
            printf("<return> "); break;
        
        case COMMA:
            printf(", "); break;
        case SEMICOLON:
            printf("; "); break;
        case ASSIGN:
            printf("= "); break;

        case L_PAREN:
            printf("`(` "); break;
        case R_PAREN:
            printf("`)` "); break;
        case L_BRACKET:
            printf("`[` "); break;
        case R_BRACKET:
            printf("`]` "); break;
        case L_BRACE:
            printf("`{` "); break;
        case R_BRACE:
            printf("`}` "); break;

        case OPTYPE_ADD:
            printf("{add: %s} ", yylval.opt_add==LexPlus ? "+" : "-"); break;
        case OPTYPE_MUL:
            printf("{mul: %s} ", yylval.opt_mul==LexMul ? "*" : yylval.opt_mul==LexDiv ? "/" : "%"); break;
        case OPTYPE_REL:
            printf("{rel: %s} ", yylval.opt_rel==LexLess ? "<" : yylval.opt_rel==LexGreater ? ">" : yylval.opt_rel==LexLeq ? "<=" : ">="); break;
        case OPTYPE_EQ:
            printf("{eq: %s} ", yylval.opt_eq==LexEq ? "==" : "!="); break;

        case OP_AND:
            printf("{&&} "); break;
        case OP_OR:
            printf("{||} "); break;
        case OP_NOT:
            printf("{!} "); break;

        case IDENT:
            printf("{ident: %s} ", yylval.ident_str); delete[] yylval.ident_str; break;
        case LITERAL:
            printf("{lit: %d} ", yylval.lit_int); break;

        default:
            printf("{wtf: %d} ", token);
            exit(1);
    }
}

inline void test_lexer() {
    int token;
    while((token = yylex())) {
        describe_token(token);
    }

    printf("<eof>\nTEST PASSED!\n");
}