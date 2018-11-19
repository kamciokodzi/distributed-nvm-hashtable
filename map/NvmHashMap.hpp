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
        std::unique_lock <pmem::obj::mutex> lock(arrayOfSegmentsMutex[index]);

        std::cout << "Locked. Key = " << key << ". Hash: " << hash << ". Value = " << value << std::endl;

        int index2 = hash % arrayOfSegments[index]->size;
        std::cout << "Index1 = " << index << ". Index2 = " << index2 << std::endl;
        arrayOfSegments[index]->segments[index2]->hash = hash;
        pmem::obj::persistent_ptr <SegmentObject<V> > ptr = arrayOfSegments[index]->segments[index2]->head;
        while (true) {
            if (ptr == nullptr) {
                std::cout<<"First element! Empty list" << std::endl;
            }
            if (ptr->next == nullptr) { // it's the last item of the list
                if (ptr->key == key) {
                    std::cout << "Found element with the same key. Inserting new element aborted" << std::endl;
                    break;
                }
                std::cout << "Inserting new SegmentObject" << std::endl;
                auto pop = pmem::obj::pool_by_vptr(this);
                pmem::obj::transaction::run(pop, [&] {
                    ptr->next = pmem::obj::make_persistent<SegmentObject<V> >();
                    ptr->next->key = key;
                    ptr->next->value = value;
                });
                break;
            } else {
                std::cout << "Jumping to the next segmentObject" << std::endl;
                ptr = ptr->next;
                std::cout << "Ptr->key = " << ptr->key << std::endl;
                if (ptr->key == key) {
                    std::cout << "Found element with the same key. Inserting new element aborted" << std::endl;
                    break;
                }
            }
        }
    }

    V get(int key) {
        int hash = 2 * key;
        int index = hash & INTERNAL_MAPS_COUNT - 1;
        int index2 = hash % arrayOfSegments[index]->size;
        std::cout << "Index1 = " << index << ". Index2 = " << index2 << std::endl;
        pmem::obj::persistent_ptr <SegmentObject<V>> ptr = arrayOfSegments[index]->segments[index2]->head;

        std::cout << "Key = " << key << ". Hash: " << hash << std::endl;

        while (true) {
            if (ptr == nullptr) {
                std::cout << "Empty list. Element not found." << std::endl;
                break;
            } else {
                if (ptr->key == key) {
                    std::cout << "Found element with key = " << key << ". Value = " << ptr->value << std::endl;
                    return ptr->value;
                } else {
                    if (ptr->next != nullptr) {
                        std::cout << "Key = " << ptr->key << ". Value = " << ptr->value << std::endl;
                        std::cout << "Jumping to the next segmentObject" << std::endl;
                        ptr = ptr->next;
                    } else { //end of the list
                        std::cout << "Element not found." << std::endl;
                        break;
                    }
                }
            }
        }

        return -1;
    }

//    int remove(int key) {
//        int index = key & INTERNAL_MAPS_COUNT - 1;
//        int index2 = key % arrayOfSegments[index]->size;
//        std::cout << "Index1 = " << index << ". Index2 = " << index2 << std::endl;
//        pmem::obj::persistent_ptr <SegmentObject<V>> ptr = arrayOfSegments[index]->segments[index2]->head;
//        int hash = 2 * key;
//
//        std::cout << "Key = " << key << ". Hash: " << hash << std::endl;
//
//        while (true) {
//            if (ptr == nullptr) {
//                std::cout << "Empty list. Element not found." << std::endl;
//                break;
//            } else {
//                if (ptr->next != nullptr) { // at least 2 elements in list
//                    if (ptr->hash == hash) {
//                        auto pop = pmem::obj::pool_by_vptr(this);
//                        pmem::obj::transaction::run(pop, [&] {
//                            pmem::obj::delete_persistent<SegmentObject<V> >(ptr);
//                        });
//                    }
//                    if (ptr->next->hash == hash) {
//
//                    }
//                } else { //1 element list
//                    if (ptr->hash == hash) {
//                        auto pop = pmem::obj::pool_by_vptr(this);
//                        pmem::obj::transaction::run(pop, [&] {
//                            pmem::obj::delete_persistent<SegmentObject<V> >(ptr);
//                        });
//                    }
//                }
//
//                    if (ptr->next->hash == hash) {
//                    std::cout << "Found element with hash = " << hash << ". Value = " << ptr->value << std::endl;
//                    if (ptr->next != nullptr) {
//                        ptr=
//                    } else {
//
//                    }
//                    pmem::obj::transaction::run(pop, [&] {
//                        pmem::obj::delete_persistent<SegmentObject<V> >(ptr);
//                    });
//                    return ptr->value;
//                } else {
//                    if (ptr->next != nullptr) {
//                        std::cout << "Hash = " << ptr->hash << ". Value = " << ptr->value << std::endl;
//                        std::cout << "Jumping to the next segmentObject" << std::endl;
//                        ptr = ptr->next;
//                    } else { //end of the list
//                        std::cout << "Element not found." << std::endl;
//                        break;
//                    }
//                }
//            }
//        }
//    }


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


