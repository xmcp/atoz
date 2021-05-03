#include "ast.hpp"
#include "../back/ir.hpp"

bool DO_DETECT_BUILTIN = true;
const bool OUTPUT_ASTHASH = false;

const asthash_t HASH_MEMCPY = 0x40fb81eca966c941ULL;

void IrFuncDefMemcpy::gen_inst(InstRoot *root) {
    /*
     * {t0} x = {a1} dst_pos << 2
     * {a0} dst = {a0} dst + {t0} x
     * {a1} upper = {a1} len << 2
     * {a1} upper = {a0} src + {a1} upper
     * loop:
     *     x = {a2} src [0]
     *     {a0} dst [0] = x
     *     {a2} src += 4
     *     {a0} dst += 4
     * if {a0} src < {a1} upper goto loop
     * {a0} ret = {a3} len
     * return
     */
    auto func = new InstFuncDef(name, 4, 0);
    root->push_func(func);

    auto dst = Preg('a', 0), ret = Preg('a', 0);
    auto dst_pos = Preg('a', 1), upper = Preg('a', 1);
    auto src = Preg('a', 2);
    auto len = Preg('a', 3);
    auto x = Preg('t', 0);

    int loop = gen_label();

    func->push_stmt(new InstComment("builtin memcpy"));
    func->push_stmt(new InstLeftShiftI(x, dst_pos, 2));
    func->push_stmt(new InstOpBinary(dst, dst, OpPlus, x));
    func->push_stmt(new InstLeftShiftI(upper, len, 2));
    func->push_stmt(new InstOpBinary(upper, src, OpPlus, upper));
    func->push_stmt(new InstLabel(loop));
    func->push_stmt(new InstArrayGet(x, src, 0));
    func->push_stmt(new InstArraySet(dst, 0, x));
    func->push_stmt(new InstAddI(src, src, 4));
    func->push_stmt(new InstAddI(dst, dst, 4));
    func->push_stmt(new InstCondGoto(src, RelLess, upper, loop));
    func->push_stmt(new InstMov(ret, len));
    func->push_stmt(new InstRet(func));
}
void IrFuncDefMemcpy::report_destroyed_set() {
    root->destroy_sets.insert(make_pair(name, unordered_set<Preg, Preg::Hash>{
            Preg('a', 0), Preg('a', 1), Preg('a', 2), Preg('a', 3)
    }));
}

void IrFuncDefMul::gen_inst(InstRoot *root) {
    /*
     * if(a1 == 0) return a0;
     * t0 = a0;
     * a0 = 0;
     * a2 = mod;
     * loop:
     *     t1 = a1 << 31;
     *     if(t1 == 0) goto skip;
     *         a0 += t0;
     *         a0 %= a2;
     *     skip:
     *     a1 >>= 1;
     *     if(a1 == 0) return a0;
     *     t0 <<= 1;
     *     t0 %= a2;
     * goto loop;
     */
    auto func = new InstFuncDef(name, 4, 0);
    root->push_func(func);

    auto a0 = Preg('a', 0), a1 = Preg('a', 1), a2 = Preg('a', 2);
    auto t0 = Preg('t', 0), t1 = Preg('t', 1), x0 = Preg('x', 0);

    int loop = gen_label();
    int skip = gen_label();
    int ret = gen_label();

    func->push_stmt(new InstComment("builtin mul"));
    func->push_stmt(new InstCondGoto(a1, RelEq, x0, ret));
    func->push_stmt(new InstMov(t0, a0));
    func->push_stmt(new InstLoadImm(a0, 0));
    func->push_stmt(new InstLoadImm(a2, 998244353));
    func->push_stmt(new InstLabel(loop));
    func->push_stmt(new InstLeftShiftI(t1, a1, 31));
    func->push_stmt(new InstCondGoto(t1, RelEq, x0, skip));
    func->push_stmt(new InstOpBinary(a0, a0, OpPlus, t0));
    func->push_stmt(new InstOpBinary(a0, a0, OpMod, a2));
    func->push_stmt(new InstLabel(skip));
    func->push_stmt(new InstLeftShiftI(a1, a1, -1));
    func->push_stmt(new InstCondGoto(a1, RelEq, x0, ret));
    func->push_stmt(new InstLeftShiftI(t0, t0, 1));
    func->push_stmt(new InstOpBinary(t0, t0, OpMod, a2));
    func->push_stmt(new InstGoto(loop));
    func->push_stmt(new InstLabel(ret));
    func->push_stmt(new InstRet(func));
}

void IrFuncDefMul::report_destroyed_set() {
    root->destroy_sets.insert(make_pair(name, unordered_set<Preg, Preg::Hash>{
            Preg('a', 0), Preg('a', 1), Preg('a', 2)
    }));
}

const asthash_t HASH_MUL = 0x11a46e3784c27ad0ULL;

inline IrFuncDefBuiltin *create_builtin_wrapper(AstFuncDef *ast, IrFuncDef *ir) {
    static_assert(sizeof(asthash_t)==8, "use 64-bit!");
    if(!DO_DETECT_BUILTIN)
        return nullptr;

    asthash_t hash = ast->asthash();
    if(OUTPUT_ASTHASH)
        printf("hash of `%s` = %llx\n", ast->name.c_str(), hash);

    if(ast->name=="memmove" && hash==HASH_MEMCPY) {
        printf("info: got builtin memcpy: %s\n", ast->name.c_str());
        return new IrFuncDefMemcpy(ir->root, ir->type, ir->name, ir->params);
    } else if(ast->name=="multiply" && hash==HASH_MUL) {
        printf("info: got builtin mul: %s\n", ast->name.c_str());
        return new IrFuncDefMul(ir->root, ir->type, ir->name, ir->params);
    }
    return nullptr;
}