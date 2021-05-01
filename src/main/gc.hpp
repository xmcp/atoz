#pragma once

#include <vector>
using std::vector;

struct GarbageCollectable;

static vector<GarbageCollectable*> allocated_ptrs;

struct GarbageCollectable {
    GarbageCollectable() {
        allocated_ptrs.push_back(this);
    }

    static void delete_all() {
        for(auto ptr: allocated_ptrs)
            delete ptr;
    }
};