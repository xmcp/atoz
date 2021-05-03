#include "ir.hpp"
#include "../front/ast.hpp"

#include <queue>
#include <stack>
#include <unordered_set>
using std::queue;
using std::stack;
using std::unordered_set;

const bool OUTPUT_REC_LEARNT = false;
const bool OUTPUT_REC_TOOK = true;

void clear_inqueue(IrFuncDef *func) {
    for(const auto& stmtpair: func->stmts)
        stmtpair.first->_regalloc_inqueue = false;
}

void propagate_alive_vars(IrFuncDef *func) {
    clear_inqueue(func);

    queue<IrStmt*> q;

    // push all stmts
    for(const auto& stmtpair: func->stmts) {
        stmtpair.first->_regalloc_inqueue = true;
        q.push(stmtpair.first);
    }

    while(!q.empty()) {
        IrStmt *stmt = q.front();
        stmt->_regalloc_inqueue = false;
        q.pop();

        unordered_set<int> meet;
        for(auto next: stmt->next)
            for(auto x: next->alive_pooled_vars)
                meet.insert(x);

        stmt->meet_pooled_vars = meet;

        for(auto def: stmt->defs())
            meet.erase(def);
        for(auto use: stmt->uses())
            meet.insert(use);

        if(stmt->alive_pooled_vars != meet) { // update alive vars
            stmt->alive_pooled_vars = meet;
            for(auto prev: stmt->prev)
                if(!prev->_regalloc_inqueue) {
                    prev->_regalloc_inqueue = true;
                    q.push(prev);
                }
        }
    }
}

struct CorrGraph {
    unordered_set<int> nodes;
    unordered_map<int, int> degrees;
    unordered_map<int, unordered_set<int>> edges;
    unordered_map<int, unordered_set<int>> edges_removed;

    void addnode(int n) {
        nodes.insert(n);
    }

    void addedge(int a, int b) {
        assert(a!=b);
        assert(nodes.find(a)!=nodes.end());
        assert(nodes.find(b)!=nodes.end());

        if(edges[a].find(b)!=edges[a].end()) { // already linked
            assert(edges[b].find(a)!=edges[b].end());
            return;
        }

        //printf("addedge %d %d\n", a, b);

        edges[a].insert(b);
        edges[b].insert(a);

        degrees[a]++;
        degrees[b]++;
    }

    void rmedge(int a, int b) {
        //printf("rmedge %d %d\n", a, b);

        assert(edges[a].find(b)!=edges[a].end());
        assert(edges[b].find(a)!=edges[b].end());

        edges[a].erase(b);
        edges[b].erase(a);

        degrees[a]--;
        degrees[b]--;

        edges_removed[a].insert(b);
        edges_removed[b].insert(a);
    }

    void rmnode(int x) {
        //printf("rmnode %d\n", x);
        assert(nodes.find(x)!=nodes.end());

        auto all_edges = edges[x];
        for(auto y: all_edges)
            rmedge(x, y); // note that `erase` will invalidate iterator

        assert(degrees[x]==0);
        degrees.erase(x);

        nodes.erase(x);
    }

    int find_colorable_node(int max_degree_exclusive) {
        for(int x: nodes)
            if(degrees[x]<max_degree_exclusive)
                return x;
        return *nodes.begin();
    }
};

CorrGraph collect_correlation(IrFuncDef *func) {
    clear_inqueue(func);

    CorrGraph graph;

    if(func->stmts.empty())
        return graph;

    queue<IrStmt*> q;
    func->stmts.front().first->_regalloc_inqueue = true;
    q.push(func->stmts.front().first);

    while(!q.empty()) {
        IrStmt *stmt = q.front();
        q.pop();

        for(auto var: stmt->alive_pooled_vars)
            graph.addnode(var);

        for(auto it = stmt->alive_pooled_vars.cbegin(); it != stmt->alive_pooled_vars.end(); it++) {
            auto it2 = it;
            for(it2++; it2!=stmt->alive_pooled_vars.end(); it2++)
                graph.addedge(*it, *it2);
        }

        for(auto next: stmt->next)
            if(!next->_regalloc_inqueue) {
                next->_regalloc_inqueue = true;
                q.push(next);
            }
    }

    // remove unreachable stmts
    for(auto it=func->stmts.begin(); it!=func->stmts.end();) {
        if(!it->first->_regalloc_inqueue) {
            list<string> buf;
            it->first->output_eeyore(buf);

            // output info
            if(!istype(it->first, IrReturn)) {
                printf("warning: removed unreachable stmt in %s:\n", func->name.c_str());
                for(const auto &line: buf)
                    printf("  > %s\n", line.c_str());
            }

            it = func->stmts.erase(it);
        }
        else
            it++;
    }

    return graph;
}

int get_sacrificed_node(CorrGraph graph) {
    for(auto x: graph.nodes) {
        if(x>=REGUID_ARG_OFFSET) // never spilloffset args
            continue;

        // todo: add heuristic here
        return x;
    }
    assert(false);
}

Preg choose_reg(CorrGraph graph, int x, vector<Preg> avail_regs, const unordered_map<int, Vreg> &vreg_map, Preg recommendation) {
    unordered_set<Preg, Preg::Hash> useful_regs;
    for(Preg reg: avail_regs) { // test each reg
        for(int y: graph.edges_removed[x]) {
            auto it = vreg_map.find(y);
            if(it!=vreg_map.end() && it->second.pos==Vreg::VregInReg && it->second.reg==reg)
                goto fail; // a neighbor var uses this reg
        }

        useful_regs.insert(reg);

        fail: ; // try next reg
    }

    assert(!useful_regs.empty());

    if(useful_regs.find(recommendation)!=useful_regs.end()) {
        if(OUTPUT_REC_TOOK)
            printf(
                "info: regalloc took recommendation %s -> %s\n",
                demystify_reguid(x).c_str(),
                recommendation.tigger_ref().c_str()
            );
        return recommendation;
    } else {
        if(OUTPUT_REC_TOOK && recommendation!=Preg('x', 0))
            printf(
                "info: regalloc SKIP recommendation %s -> %s (used %s)\n",
                demystify_reguid(x).c_str(),
                recommendation.tigger_ref().c_str(),
                (*useful_regs.begin()).tigger_ref().c_str()
            );
        return *useful_regs.begin();
    }
}

void swap_preg(Preg a, Preg b, unordered_map<int, Vreg> &vreg_map) {
    for(auto &it: vreg_map)
        if(it.second.pos==Vreg::VregInReg) {
            if(it.second.reg==a)
                it.second.reg = b;
            else if(it.second.reg==b)
                it.second.reg = a;
        }
}

void check_alloc_does_not_conflict(IrFuncDef *func) {
    for(const auto& stmt: func->stmts) { // for each stmt
        unordered_set<Vreg, Vreg::Hash> workingset;

        for(auto uid: stmt.first->alive_pooled_vars) { // assert alive vars do not map to same vreg
            Vreg reg = func->get_vreg(uid);
            assert(workingset.find(reg)==workingset.end());

            workingset.insert(reg);
        }
    }
}

struct Recommender {
    unordered_map<int, int> rec_parent;
    unordered_map<int, Preg> rec_label;

    Recommender() {}

    int _find(int x) {
        if(rec_parent.find(x)==rec_parent.end() || rec_parent[x]==x)
            return x;
        else
            return (rec_parent[x] = _find(rec_parent[x]));
    }
    void _union(int x, int y) {
        x = _find(x);
        y = _find(y);
        if(x!=y) {
            rec_parent[x] = y;
            if(rec_label.find(y)==rec_label.end() && rec_label.find(x)!=rec_label.end())
                rec_label.insert(make_pair(y, rec_label.find(x)->second));
        }
    }

    void mark_mov(IrMov *stmt) {
        if(stmt->dest.regpooled() && stmt->src.regpooled()) {
            if(OUTPUT_REC_LEARNT)
                printf(
                    "info: detected mov %s <-> %s\n",
                    stmt->dest.eeyore_ref_global().c_str(),
                    stmt->src.eeyore_ref_global().c_str()
                );

            _union(stmt->dest.reguid(), stmt->src.reguid());
        }
    }
    void mark_ret(IrReturn *stmt) {
        if(stmt->retval.regpooled()) {
            if(OUTPUT_REC_LEARNT)
                printf(
                    "info: detected ret %s\n",
                    stmt->retval.eeyore_ref_global().c_str()
                );

            rec_label.insert(make_pair(_find(stmt->retval.reguid()), Preg('a', 0)));
        }
    }
    void mark_param(IrParam *stmt) {
        if(stmt->param.regpooled()) {
            if(OUTPUT_REC_LEARNT)
                printf(
                    "info: detected param %s #%d\n",
                    stmt->param.eeyore_ref_global().c_str(),
                    stmt->pidx
                );

            rec_label.insert(make_pair(_find(stmt->param.reguid()), Preg('a', stmt->pidx)));
        }
    }
    void mark_setreg(int vregid, Preg preg) {
        if(OUTPUT_REC_LEARNT)
                printf(
                    "info: learned reg %s -> %s\n",
                    demystify_reguid(vregid).c_str(),
                    preg.tigger_ref().c_str()
                );

        rec_label.insert(make_pair(_find(vregid), preg));
    }

    Preg get_recommendation(int vregid) {
        int group = _find(vregid);
        if(rec_label.find(group)==rec_label.end())
            return Preg('x', 0);
        else
            return rec_label.find(group)->second;
    }
};

Recommender scan_recommendations(IrFuncDef *func) {
    Recommender rec;
    for(const auto& stmtpair: func->stmts) {
        auto stmt = stmtpair.first;
        if(istype(stmt, IrMov))
            rec.mark_mov((IrMov*)stmt);
        else if(istype(stmt, IrReturn))
            rec.mark_ret((IrReturn*)stmt);
        else if(istype(stmt, IrParam))
            rec.mark_param((IrParam*)stmt);
    }
    return rec;
}

void IrFuncDef::regalloc() {
    propagate_alive_vars(this);
    auto graph = collect_correlation(this); // will also remove unreachable stmts

    // only `regpooled` (tempvar, arg, local scalar) vars in this graph

    for(const auto& declpair: decls) {
        if(declpair.first->def_or_null==nullptr) // skip tempvar
            continue;
        // for all local var defs

        int reguid = declpair.first->dest.reguid();

        decl_map.insert(make_pair(reguid, declpair.first->def_or_null));

        if(declpair.first->def_or_null->idxinfo->dims() > 0) {
            // map local array -> stack
            auto totelems = declpair.first->dest.val.reference->initval.totelems;
            vreg_map.insert(make_pair(
                reguid,
                Vreg::asStack(totelems, spillsize)
            ));
            spillsize += totelems;
        }
    }

    vector<Preg> avail_regs;

    // t0 and t1 reserved for spilled register and assembler temporary
    for(int t=2; t<=6; t++)
        avail_regs.push_back(Preg('t', t));
    for(int a=0; a<=7; a++)
        avail_regs.push_back(Preg('a', a));
    for(int s=0; s<=11; s++)
        avail_regs.push_back(Preg('s', s));

    Recommender rec = scan_recommendations(this);

    // now colorize the graph

    stack<int> stk_colorable;

    while(!graph.nodes.empty()) {
        int x = graph.find_colorable_node(avail_regs.size());

        if(graph.degrees[x] < (int)avail_regs.size()) { // colorable
            stk_colorable.push(x);
            graph.rmnode(x);

        } else { // all nodes not colorable
            // remove one node
            x = get_sacrificed_node(graph);
            graph.rmnode(x);

            // map it onto stack
            auto it = decl_map.find(x); // local -> found, tempvar -> notfound
            int arrelems = it==decl_map.end() ? 1 : it->second->initval.totelems;
            vreg_map.insert(make_pair(x, Vreg::asStack(arrelems, spillsize)));
            spillsize += arrelems;
        }
    }

    while(!stk_colorable.empty()) { // for each colorable node
        int x = stk_colorable.top();
        stk_colorable.pop();

        // map it to a preg
        Preg reg = choose_reg(graph, x, avail_regs, vreg_map, rec.get_recommendation(x));
        vreg_map.insert(make_pair(x, reg));
        rec.mark_setreg(x, reg);
    }

    check_alloc_does_not_conflict(this);

    for(int i=0; i<(int)params->val.size(); i++) { // check args
        int uid = REGUID_ARG_OFFSET + i;
        Preg shouldbe = Preg('a', i);
        auto it = vreg_map.find(uid);
        if(it==vreg_map.end()) {
            printf("warning: <%s> arg %d not used\n", name.c_str(), i);
        } else {
            assert(it->second.pos==Vreg::VregInReg);
            if(it->second.reg != shouldbe) {
                // arg not in correct reg, make it correct
                swap_preg(it->second.reg, shouldbe, vreg_map);
            }
        }
    }
}
