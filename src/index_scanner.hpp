#pragma once

struct AstInitVal;
struct AstMaybeIdx;
struct AstExp;

class InitVal {
    int *value;
    vector<int> shape;
    int totelems;

    int calcstep;
    int calcdim;
    int calcpos;

    bool calculated;

    void calc(AstInitVal *v);

public:

    InitVal(AstMaybeIdx *shapeinfo);

    void calc_if_needed(AstInitVal *v) {
        if(!calculated) {
            calc(v);
            calculated = true;
        }
    }
    AstExp *getoffset(AstMaybeIdx *idxinfo, bool allowpartial);
    int getvalue(AstMaybeIdx *idxinfo);
};