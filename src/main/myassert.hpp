#pragma once

#include <cstdio>
#include <cstdlib>

inline void myassert_fail(int line, const char *fn) {
    printf("error: assertion failed\n%s (line %d)\n", fn, line);
    exit(5+line%100);
}

#define assert(x) do {if(!(x)) myassert_fail(__LINE__, __FILE__);} while(0)