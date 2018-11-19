#include "map.hpp"
#include <iostream>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/utils.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/mutex.hpp>
#include <unistd.h>
#include <memory>
#include <string.h>
#include <mutex>

#define INTERNAL_MAPS_COUNT 8

template<class V>
        class Value {
public:
    V value;
};

template<class V>
class SegmentObject {
public:
    pmem::obj::p<V> value;
    pmem::obj::p<int> key;
    pmem::obj::persistent_ptr<SegmentObject<V> > next = nullptr;
};

template<class V>
class Segment {
public:
    pmem::obj::p<int> hash;
    pmem::obj::persistent_ptr<SegmentObject<V> > head = nullptr;

    Segment()
    {
        head = pmem::obj::make_persistent< SegmentObject<V> >();
        head -> value = -1;
    }
};


template<class V>
class ArrayOfSegments {
public:
    pmem::obj::persistent_ptr<Segment<V> > segments[10];
    pmem::obj::p<int> size;
    ArrayOfSegments() {
        this->size = 10;
        for(int i=0; i<this->size; i++) {
            this->segments[i] = pmem::obj::make_persistent< Segment<V> >();
        }
    }
};

template<typename V>
class NvmHashMap {
private:
    pmem::obj::persistent_ptr<ArrayOfSegments<V> > arrayOfSegments[INTERNAL_MAPS_COUNT];
    pmem::obj::mutex arrayOfSegmentsMutex[INTERNAL_MAPS_COUNT];

    size_t hash(int key) {
        return key;
        //return std::hash<int>(key);
    }

public:

    NvmHashMap() {
        std::cout << "NvmHashMap() start" << std::endl;
        for (int i = 0; i < INTERNAL_MAPS_COUNT; i++) {
            this->arrayOfSegments[i] = pmem::obj::make_persistent< ArrayOfSegments<V> >();
            // this->mc[i] = pmem::obj::make_persistent<Value <long> >();
        }

        std::cout << "NvmHashMap() stop" << std::endl;
    }


    void insertNew(int key, V value) {
        int hash = 2 * key;
        int index = hash & INTERNAL_MAPS_COUNT - 1;
        int index2 = hash % arrayOfSegments[index]->size;
        std::unique_lock <pmem::obj::mutex> lock(arrayOfSegmentsMutex[index]);

//        std::cout << "Locked. Key = " << key << ". Hash = " << hash << ". Value = " << value << std::endl;
//        std::cout << "Index1 = " << index << ". Index2 = " << index2 << std::endl;
        arrayOfSegments[index]->segments[index2]->hash = hash;
        pmem::obj::persistent_ptr <SegmentObject<V> > ptr = arrayOfSegments[index]->segments[index2]->head;
        while (true) {
            if (ptr->next == nullptr) { // it's the last item of the list
                std::cout << "Inserting new SegmentObject with key = " << key << " and value = " << value << std::endl;
                auto pop = pmem::obj::pool_by_vptr(this);
                pmem::obj::transaction::run(pop, [&] {
                    ptr->key = key;
                    ptr->value = value;
                    ptr->next = pmem::obj::make_persistent<SegmentObject<V> >();
                });
                break;
            }
            if (ptr->key == key) {
//                std::cout << "Found element with the same key. Updating element" << std::endl;
                ptr->value = value;
                if (ptr->next == nullptr) { // it's the last item of the list
//                    std::cout << "Inserting new empty SegmentObject" << std::endl;
                    auto pop = pmem::obj::pool_by_vptr(this);
                    pmem::obj::transaction::run(pop, [&] {
                        ptr->next = pmem::obj::make_persistent<SegmentObject<V> >();
                    });
                    break;
                }
                break;
            }

            if (ptr->next != nullptr) {
//              std::cout << "Jumping to the next segmentObject" << std::endl;
                ptr = ptr->next;
            } else {
                break;
            }
        }
    }

    V get(int key) {
        int hash = 2 * key;
        int index = hash & INTERNAL_MAPS_COUNT - 1;
        int index2 = hash % arrayOfSegments[index]->size;
        pmem::obj::persistent_ptr <SegmentObject<V>> ptr = arrayOfSegments[index]->segments[index2]->head;

//        std::cout << "Index1 = " << index << ". Index2 = " << index2 << std::endl;
//        std::cout << "Key = " << key << ". Hash: " << hash << std::endl;

        while (true) {
            if (ptr->next == nullptr) {
                std::cout << "Did not found element with key = " << key << std::endl;
                break;
            } else {
                if (ptr->key == key) {
                    std::cout << "Found element with key = " << key << ". Value = " << ptr->value << std::endl;
                    return ptr->value;
                } else if (ptr->next->next == nullptr) {
                    std::cout << "Did not found element with key = " << key << std::endl;
                    break;
                } else if (ptr->next->key == key) {
                    std::cout << "Found element with key = " << key << ". Value = " << ptr->next->value << std::endl;
                    return ptr->next->value;
                }
            }
            if (ptr->next != nullptr) {
                ptr = ptr->next;
            } else {
                std::cout << "Did not found element with key = " << key << std::endl;
                break;
            }
        }

        return -1;
    }

    void remove (int key) {
        int hash = 2 * key;
        int index = hash & INTERNAL_MAPS_COUNT - 1;
        int index2 = hash % arrayOfSegments[index]->size;
        pmem::obj::persistent_ptr <SegmentObject<V>> ptr = arrayOfSegments[index]->segments[index2]->head;

//        std::cout << "Index1 = " << index << ". Index2 = " << index2 << std::endl;
//        std::cout << "Key = " << key << ". Hash: " << hash << std::endl;

        while (true) {
            if (ptr->next == nullptr) {
                std::cout << "Did not found element with key = " << key << std::endl;
                break;
            } else {
                if (ptr->next->key == key) {
                    std::cout << "ELO " << key << std::endl;
                    auto temp = ptr->next->next;
                    auto pop = pmem::obj::pool_by_vptr(this);
                    pmem::obj::transaction::run(pop, [&] {
                        pmem::obj::delete_persistent<SegmentObject<V> >(ptr->next);
                        ptr->next = temp;
                    });
                    std::cout << "Removed element with key = " << key << std::endl;
                    break;
                } else if (ptr->key == key) {
                    auto pop = pmem::obj::pool_by_vptr(this);
                    pmem::obj::transaction::run(pop, [&] {
                        arrayOfSegments[index]->segments[index2]->head = ptr->next;
                        pmem::obj::delete_persistent<SegmentObject<V> >(ptr);
                    });
                    std::cout << "Removed element with key = " << key << std::endl;
                    break;
                }
            }
            if (ptr->next != nullptr) {
                ptr = ptr->next;
            } else {
                std::cout << "Did not found element with key = " << key << std::endl;
                break;
            }
        }
    }

    // int calcSize(int triesToBlock = 5) {
    //     int size = 0;
    //     for (int i = 0; i < INTERNAL_MAPS_COUNT; i++) {
    //         size++;
    //         // TODO: increment by size of segments[i]
    //         mc_sum += mc[i]->value;
    //     long mc_sum2 = 0;
    //     for (int i = 0; i < INTERNAL_MAPS_COUNT; i++) {
    //         mc_sum2 += mc[i]->value;
    //     if (mc_sum == mc_sum2) {
    //         return size;
    //     } else {
    //         if (triesToBlock > 0) {
    //             return calcSize(triesToBlock - 1);
    //         } else {
    //             throw "Not implemented yet";
    //             // put lock on the array and calculate size
    //         }
    //     throw "Not implemented yet!";

};


