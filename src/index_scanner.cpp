#include <cstring>
using std::memset;

#include "ast_defs.hpp"
#include "index_scanner.hpp"

#define scanerror(...) do { \
    printf("index scan error: "); \
    printf(__VA_ARGS__ ); \
    exit(1); \
} while(0)

InitVal::InitVal(AstMaybeIdx *shapeinfo):
    value(nullptr), totelems(0), calcstep(1), calcdim(0), calcpos(0), calculated(false) {
    for(auto exp: shapeinfo->val) {
        auto res = exp->calc_const();
        if(res.iserror)
            scanerror("shape not const: %s", res.error.c_str());
        if(res.val<0)
            scanerror("shape not positive: %d", res.val);

        shape.push_back(res.val);
        calcstep *= res.val;
    }
    totelems = calcstep;
    value = new int[totelems];
    memset(value, 0, sizeof(value));
}

void InitVal::calc(AstInitVal *v) {
    if(!v->is_many) {
        if(calcpos>=totelems)
            scanerror("too many initializers: item length is %d", totelems);

        auto res = v->val.single->calc_const();
        if(res.iserror)
            scanerror("initializer not const: %s", res.error.c_str());

        value[calcpos] = res.val;
        calcpos++;
    } else {
        if(calcdim==(int)shape.size())
            scanerror("initializer dimension too deep");

        int dimsize = shape[calcdim];
        int posbegin = calcpos;
        calcdim++;
        calcstep /= dimsize;

        if(posbegin%calcstep != 0)
            scanerror("initializer not aligned across dimension");

        for(auto vv: *v->val.many)
            calc(vv);

        if(calcpos > posbegin+calcstep)
            scanerror("too many initializers for this dimension");

        calcpos = posbegin+calcstep;
        calcstep *= dimsize;
        calcdim--;
    }
}

template<typename T>
T *strip_location(T *node) {
    node->loc.lineno = -1;
    node->loc.colno = -1;
    return node;
}

AstExp *InitVal::getoffset(AstMaybeIdx *idxinfo, bool allowpartial) {
    if(allowpartial) {
        if(idxinfo->val.size()>shape.size())
            scanerror("offset shape too deep: item depth %d, offset %d", (int)shape.size(), (int)idxinfo->val.size());
    } else {
        if(idxinfo->val.size()!=shape.size())
            scanerror("offset shape incorrect: item depth %d, offset %d", (int)shape.size(), (int)idxinfo->val.size());
    }

    AstExp *idx = strip_location(new AstExpLiteral(0));
    int dim = 0;
    int step = totelems;
    for(auto exp: idxinfo->val) {
        step /= shape[dim];
        // idx = idx + step * exp
        idx = strip_location(new AstExpOpBinary(
                OpPlus,
                idx,
                strip_location(new AstExpOpBinary(
                        OpMul,
                        strip_location(new AstExpLiteral(step)),
                        exp
                ))
        ));
        dim++;
    }
    return idx;
}

int InitVal::getvalue(AstMaybeIdx *idxinfo) {
    AstExp *offset = getoffset(idxinfo, false);

    ConstExpResult res = offset->calc_const();
    if(res.iserror)
        scanerror("index not const: %s", res.error.c_str());

    return res.val;
}

