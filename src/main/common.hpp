#pragma once

#include <cstdio>
#include <cstdlib>

inline void myassert_fail(int line, const char *fn) {
    printf("error: assertion failed\n%s (line %d)\n", fn, line);
    exit(5+line%100);
}

#define assert(x) do {if(!(x)) myassert_fail(__LINE__, __FILE__);} while(0)

inline bool imm_overflows(int imm) {
    return imm<=-2047 || imm>=2047;
}

#define outasm(...) do { \
    snprintf(instbuf, sizeof(instbuf), __VA_ARGS__); \
    buf.push_back(string(instbuf)); \
} while(0)

#define outstmt(...) do { \
    outasm("    " __VA_ARGS__); \
} while(0)
