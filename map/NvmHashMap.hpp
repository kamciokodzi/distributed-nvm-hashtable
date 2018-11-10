#include "map.hpp"
#include <iostream>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/utils.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <unistd.h>
#include <memory>
#include <string.h>

template<class V>
class Segment {
public:
    int key;
    int hash;
    V value;
    Segment *next;
};


template<typename V>
class NvmHashMap {
private:

    pmem::obj::persistent_ptr < Segment<V> > root;

    size_t hash(int key) {
        return key;
        //return std::hash<int>(key);
    }

public:

    NvmHashMap() {
        std::cout << "NvmHashMap() start" << std::endl;

        this->root = pmem::obj::make_persistent < Segment < V > > ();

        std::cout << "NvmHashMap() stop" << std::endl;
    }

    int size() {
        throw "Not implemented yet!";
    }

    void insert(int key, V value) {
        this->root->value = value;
    }

    void insertNew(int key, V value) {
        throw "Not implemented yet!";
    }

    V get(int key) {
        return this->root->value;
    }

    V remove(int key) {
        throw "Not implemented yet!";
    }

    V remove(int key, V value) {
        throw "Not implemented yet!";
    }
};


