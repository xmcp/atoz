#pragma once

struct AstInitVal;
struct AstMaybeIdx;
struct AstExp;

class InitVal {
    vector<int> shape;

    int calcstep;
    int calcdim;
    int calcpos;

    bool calculated;

    void calc(AstInitVal *v);

public:
    AstExp **value;
    int totelems;

    InitVal(); // yyparse phase
    void init(AstMaybeIdx *shapeinfo); // tree completing phase, after name is looked up

    void calc_if_needed(AstInitVal *v) {
        if(!calculated) {
            calc(v);
            calculated = true;
        }
    }
    AstExp *getoffset(AstMaybeIdx *idxinfo, bool allowpartial);
    int getvalue(AstMaybeIdx *idxinfo);
};