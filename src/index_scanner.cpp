#include <cstring>
using std::memset;

#include "ast.hpp"
#include "index_scanner.hpp"

#define scanerror(...) do { \
    printf("index scan error: "); \
    printf(__VA_ARGS__ ); \
    exit(1); \
} while(0)

InitVal::InitVal():
    value(nullptr), totelems(0), calcstep(1), calcdim(0), calcpos(0), calculated(false) {}

void InitVal::init(AstMaybeIdx *shapeinfo) {
    for(auto exp: shapeinfo->val) {
        auto res = exp->get_const();
        if(res.iserror)
            scanerror("shape not const: %s", res.error.c_str());
        if(res.val<0)
            scanerror("shape not positive: %d", res.val);

        shape.push_back(res.val);
        calcstep *= res.val;
    }
    totelems = calcstep;
    value = new AstExp*[totelems];
    memset(value, 0, totelems*sizeof(void*));
}

void InitVal::calc(AstInitVal *v) {
    if(!v->is_many) {
        if(calcpos>=totelems)
            scanerror("too many initializers: item length is %d", totelems);

        value[calcpos] = v->val.single;
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

        if(calcpos > posbegin+calcstep*dimsize)
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

AstExp *InitVal::getoffset_bytes(AstMaybeIdx *idxinfo, bool allowpartial) {
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
        AstExp *next = step==1 ? exp : strip_location(new AstExpOpBinary(
                OpMul,
                strip_location(new AstExpLiteral(step)),
                exp
        ));
        idx = idx->get_const().isalways(0) ? next : strip_location(new AstExpOpBinary(OpPlus, idx, next));
        dim++;
    }
    return strip_location(new AstExpOpBinary(
        OpMul,
        strip_location(new AstExpLiteral(4)),
        idx
    ));
}

int InitVal::getvalue(AstMaybeIdx *idxinfo) {
    AstExp *offset = getoffset_bytes(idxinfo, false);

    ConstExpResult res = offset->get_const();
    if(res.iserror)
        scanerror("index not const: %s", res.error.c_str());

    int idx = res.val;
    if(idx%4)
        scanerror("index not aligned: got %d", idx);

    AstExp *exp = value[idx/4];
    if(!exp) // not initialized
        return 0;

    res = exp->get_const();
    if(res.iserror)
        scanerror("array value at %d not const: %s", idx, res.error.c_str());

    return res.val;
}


