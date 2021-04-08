#pragma once

#include <vector>
#include <string>
using std::vector;
using std::string;
using std::pair;
using std::make_pair;

const int EEYORE_INST_BUFSIZE = 1024;

#define outasm(...) do { \
    sprintf(instbuf, __VA_ARGS__); \
    push_instruction(instbuf); \
} while(0)

struct EEyoreWorld {
    int temp_var_top;
    int label_top;
    vector<string> *instructions;
    bool should_emit_temp_var;

    EEyoreWorld(): temp_var_top(-1), label_top(-1), should_emit_temp_var(true) {
        instructions = new vector<string>();
    }
    ~EEyoreWorld() {
        delete instructions;
    }
    void clear() {
        temp_var_top = -1;
        label_top = -1;
        should_emit_temp_var = true;
        instructions->clear();
    }
    void push_instruction(char *buf) {
        instructions->push_back(buf);
    }
    int gen_label() {
        return ++label_top;
    }
    int gen_temp_var() {
        static char instbuf[EEYORE_INST_BUFSIZE];

        int tvar = ++temp_var_top;

        if(should_emit_temp_var)
            outasm("var t%d // tempvar", tvar);

        return tvar;
    }
    void print(FILE *f) {
        fprintf(f, "///// BEGIN EEYORE\n");
        for(const auto& inst: *instructions)
            fprintf(f, "%s\n", inst.c_str());
        fprintf(f, "///// END EEYORE\n");
    }
};

extern EEyoreWorld eeyore_world;

#undef outasm