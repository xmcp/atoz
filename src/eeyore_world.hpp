#pragma once

#include <vector>
#include <string>
using std::vector;
using std::string;

const int EEYORE_INST_BUFSIZE = 1024;

struct EEyoreWorld {
    int temp_var_top;
    int label_top;
    vector<string> instructions;

    EEyoreWorld(): temp_var_top(-1), label_top(-1) {}
    void push_instruction(char *buf) {
        instructions.push_back(string(buf));
    }
    int gen_label() {
        return ++label_top;
    }
    int gen_temp_var() {
        return ++temp_var_top;
    }
};

extern EEyoreWorld eeyore_world;