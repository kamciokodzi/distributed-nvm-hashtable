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

#define INTERNAL_MAPS_COUNT 16

template<class V>
        class Value {
public:
    V value;
};

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

    pmem::obj::persistent_ptr<Segment<V> > segments[INTERNAL_MAPS_COUNT];
    pmem::obj::persistent_ptr<Value<long> > mc[INTERNAL_MAPS_COUNT];

    size_t hash(int key) {
        return key;
        //return std::hash<int>(key);
    }

public:

    NvmHashMap() {
        std::cout << "NvmHashMap() start" << std::endl;

        for (int i = 0; i < INTERNAL_MAPS_COUNT; i++) {
            this->segments[i] = pmem::obj::make_persistent< Segment<V> >();
            this->mc[i] = pmem::obj::make_persistent<Value <long> >();
        }

        std::cout << "NvmHashMap() stop" << std::endl;
    }

    int calcSize(int triesToBlock = 5) {
        int size = 0;
        long mc_sum = 0;

        for (int i = 0; i < INTERNAL_MAPS_COUNT; i++) {
            size++;
            // TODO: increment by size of segments[i]
            mc_sum += mc[i]->value;
        }

        long mc_sum2 = 0;
        for (int i = 0; i < INTERNAL_MAPS_COUNT; i++) {
            mc_sum2 += mc[i]->value;
        }

        if (mc_sum == mc_sum2) {
            return size;
        } else {
            if (triesToBlock > 0) {
                return calcSize(triesToBlock - 1);
            } else {
                throw "Not implemented yet";
                // put lock on the array and calculate size
            }
        }

        throw "Not implemented yet!";
    }

    void insert(int key, V value) {
        this->segments[key & (INTERNAL_MAPS_COUNT-1)]->value = value;
    }

    void insertNew(int key, V value) {
        throw "Not implemented yet!";
    }

    V get(int key) {
        return this->segments[key & (INTERNAL_MAPS_COUNT-1)]->value;
    }

    V remove(int key) {
        throw "Not implemented yet!";
    }

    V remove(int key, V value) {
        throw "Not implemented yet!";
    }
};


