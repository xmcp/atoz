#include "back/ir.hpp"
#include "front/ast.hpp"

#include <queue>
#include <stack>
using std::queue;
using std::stack;

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
            for(auto x: next->_regalloc_alive_vars)
                meet.insert(x);

        for(auto def: stmt->defs())
            meet.erase(def);
        for(auto use: stmt->uses())
            meet.insert(use);

        if(stmt->_regalloc_alive_vars != meet) { // update alive vars
            stmt->_regalloc_alive_vars = meet;
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

        for(auto var: stmt->_regalloc_alive_vars)
            graph.addnode(var);

        for(auto it = stmt->_regalloc_alive_vars.cbegin(); it!=stmt->_regalloc_alive_vars.end(); it++) {
            auto it2 = it;
            for(it2++; it2!=stmt->_regalloc_alive_vars.end(); it2++)
                graph.addedge(*it, *it2);
        }

        for(auto next: stmt->next)
            if(!next->_regalloc_inqueue) {
                next->_regalloc_inqueue = true;
                q.push(next);
            }
    }

    return graph;
}

int get_sacrificed_node(CorrGraph graph) {
    for(auto x: graph.nodes) {
        if(x>=REGUID_ARG_OFFSET) // never spill args
            continue;

        // todo: add heuristic here
        return x;
    }
}

Preg choose_reg(CorrGraph graph, int x, vector<Preg> avail_regs, const unordered_map<int, Vreg> &vreg_map) {
    for(Preg reg: avail_regs) { // test each reg
        for(int y: graph.edges_removed[x]) {
            auto it = vreg_map.find(y);
            if(it!=vreg_map.end() && it->second.pos==Vreg::VregInReg && it->second.reg==reg)
                goto fail; // a neighbor var uses this reg
        }

        return reg;

        fail: ; // try next reg
    }
    assert(false); // no reg available, it shouldn't happen
}

void swap_preg(Preg a, Preg b, unordered_map<int, Vreg> &vreg_map) {
    for(auto it: vreg_map)
        if(it.second.pos==Vreg::VregInReg) {
            if(it.second.reg==a)
                it.second.reg = b;
            else if(it.second.reg==b)
                it.second.reg = a;
        }
}

void IrFuncDef::regalloc() {
    propagate_alive_vars(this);
    auto graph = collect_correlation(this); // only `regpooled` (tempvar, arg, local scalar) vars in this graph

    for(const auto& declpair: decls) {
        if(declpair.first->def_or_null==nullptr) // skip tempvar
            continue;
        // for all local var defs

        int reguid = declpair.first->dest.reguid();

        decl_map.insert(make_pair(reguid, declpair.first->def_or_null));

        if(declpair.first->def_or_null->idxinfo->dims() > 0) {
            // map local array -> stack
            vreg_map.insert(make_pair(
                reguid,
                Vreg::asStack(declpair.first->dest.val.reference->initval.totelems)
            ));
        }
    }

    vector<Preg> avail_regs;

    // todo: spare some regs for spill
    for(int t=0; t<=6; t++)
        avail_regs.push_back(Preg('t', t));
    for(int a=0; a<=7; a++)
        avail_regs.push_back(Preg('a', a));
    for(int s=0; s<=11; s++)
        avail_regs.push_back(Preg('s', s));

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
            int arrsize = it==decl_map.end() ? 1 : it->second->initval.totelems;
            vreg_map.insert(make_pair(x, Vreg::asStack(arrsize)));
        }
    }

    while(!stk_colorable.empty()) { // for each colorable node
        int x = stk_colorable.top();
        stk_colorable.pop();

        // map it to a preg
        Preg reg = choose_reg(graph, x, avail_regs, vreg_map);
        vreg_map.insert(make_pair(x, reg));
    }

    for(int i=0; i<args; i++) { // check args
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
