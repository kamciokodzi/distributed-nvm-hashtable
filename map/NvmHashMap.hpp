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
    pmem::obj::persistent_ptr<SegmentObject<V> > next = nullptr;
};

template<class V>
class Segment {
public:
    pmem::obj::p<int> key;
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
        for(int i=0; i<10; i++) {
            this->segments[i] = pmem::obj::make_persistent< Segment<V> >();
        }
    }
    ArrayOfSegments(int size){
        this->size = size;
        for(int i=0; i<size; i++) {
            this->segments[i] = pmem::obj::make_persistent< Segment<V> >();
        }
    }
};

template<typename V>
class NvmHashMap {
private:
    pmem::obj::persistent_ptr<ArrayOfSegments<V> > arrayOfSegments[INTERNAL_MAPS_COUNT];
    pmem::obj::mutex segmentMutex[INTERNAL_MAPS_COUNT];

    size_t hash(int key) {
        return key;
        //return std::hash<int>(key);
    }

public:

    NvmHashMap() {
        std::cout << "NvmHashMap() start" << std::endl;
        for (int i = 0; i < INTERNAL_MAPS_COUNT; i++) {
            this->arrayOfSegments[i] = pmem::obj::make_persistent< ArrayOfSegments<V> >();
            // this->arrayOfSegments[i] = new ArrayOfSegments<V> ();
            // this->segments[i] = pmem::obj::make_persistent< Segment<V> >();
            // this->mc[i] = pmem::obj::make_persistent<Value <long> >();
        }

        std::cout << "NvmHashMap() stop" << std::endl;
    }

    // int calcSize(int triesToBlock = 5) {
    //     int size = 0;
    //     long mc_sum = 0;

    //     for (int i = 0; i < INTERNAL_MAPS_COUNT; i++) {
    //         size++;
    //         // TODO: increment by size of segments[i]
    //         mc_sum += mc[i]->value;
    //     }

    //     long mc_sum2 = 0;
    //     for (int i = 0; i < INTERNAL_MAPS_COUNT; i++) {
    //         mc_sum2 += mc[i]->value;
    //     }

    //     if (mc_sum == mc_sum2) {
    //         return size;
    //     } else {
    //         if (triesToBlock > 0) {
    //             return calcSize(triesToBlock - 1);
    //         } else {
    //             throw "Not implemented yet";
    //             // put lock on the array and calculate size
    //         }
    //     }

    //     throw "Not implemented yet!";
    // }

    void insert(int key, V value, int tid) {
        std::unique_lock <pmem::obj::mutex> lock(segmentMutex[tid]);

        std::cout << key << value << tid;

        std::cout << "Thread: " << tid << ". Locked " << tid << std::endl;

        int index = key % this->arrayOfSegments[tid]->size;
        expand(tid);
        std::cout<<"Index: " << index << std::endl;
        pmem::obj::persistent_ptr <SegmentObject<V> > ptr = arrayOfSegments[tid]->segments[index]->head;
        if (ptr != nullptr)
        {
            std::cout<<"Head.value = " << ptr->value << std::endl;
        }
        std::cout << "Thread: " << tid << ". Log2" << std::endl;
        while (true)
        {
            std::cout << "Thread: " << tid << ". Log3" << std::endl;
            if (ptr->next == nullptr) {
                std::cout << "Thread: " << tid << ". Inserting new SegmentObject" << std::endl;
                auto pop = pmem::obj::pool_by_vptr(this);
                pmem::obj::transaction::run(pop, [&] {
                    ptr->next = pmem::obj::make_persistent<SegmentObject<V> >();
                });
                ptr->next->value = value;
                break;
            } else if (ptr->next != nullptr) {
                std::cout << "Thread: " << tid << ". Jumping to the next segmentObject" << std::endl;
                ptr = ptr->next;
            }
        }
    }

    void expand(int arrayIndex) {
        int size = this->arrayOfSegments[arrayIndex]->size ;
        std::cout<< size << std::endl;

        pmem::obj::persistent_ptr<ArrayOfSegments<V> > arrayOfSegments;
        auto pop = pmem::obj::pool_by_vptr(this);
        pmem::obj::transaction::run(pop, [&] {
            arrayOfSegments = pmem::obj::make_persistent<ArrayOfSegments<V> >(2*size);
        });

        for(int i=0; i<size; i++) {
            arrayOfSegments->segments[i] = this->arrayOfSegments[arrayIndex]->segments[i];
        }
        this->arrayOfSegments[arrayIndex] = arrayOfSegments;
    }


//     void insertNew(int key, V value) {
//         int segment = key & (INTERNAL_MAPS_COUNT-1);

//         std::unique_lock<pmem::obj::mutex> lock(segmentMutex[segment]);

//         pmem::obj::persistent_ptr<Segment<V> > ptr = segments[segment];
//         bool update = false;
//         while(true)
//         {
//             if(ptr->key == key)
//             {
//                 update = true;
//                 std::cout << "Element with key " << key << " already exists in segment " << segment << std::endl;
//                 break;
//             }
//             if(ptr->next != nullptr)
//                 ptr = ptr->next;
//             else
//                 break;
//         }

//         if(!update) {
// 	    auto pop = pmem::obj::pool_by_vptr(this);
// 	    pmem::obj::transaction::run(pop, [&] {
//                 ptr->next = pmem::obj::make_persistent<Segment<V> >();
// 	    });
//             ptr->next->key = key;
//             ptr->next->value = value;
//             std::cout << "Inserted value to segment " << segment << std::endl;
////         }
////     }
//
//     V get(int key, int tid) {
//
//         //std::cout << "Thread: " << tid << ". Locked " << std::endl;
//
//         pmem::obj::persistent_ptr <ArrayOfSegments<V>> ptr = arrayOfSegments[tid];
//
//         if (ptr -> segments[tid] != nullptr) {
//
//             while (true) {
//                 if (ptr->segments[tid]->key == key) {
//                     int value = ptr->segments[tid]->value;
//                     std::cout << "Thread: " << tid << ". Found element with key: " << key << ". Value: " << value << std::endl;
//                     return value;
//                 }
//                 if (ptr->segments[tid]->next != nullptr) {
//                     ptr->segments[tid] = ptr->segments[tid]->next;
//                 } else {
//                     break;
//                 }
//             }
//             std::cout << "Thread: " << tid << ". Element with key: " << key << " was not found." << std::endl;
//         }
//         return -1;
//     }
//
//     V iterate(int tid) {
//         pmem::obj::persistent_ptr <ArrayOfSegments<V>> ptr = arrayOfSegments[tid];
//
//         if (ptr -> segments[tid] != nullptr) {
//
//             while (true) {
//                 if (ptr->segments[tid]->next != nullptr) {
//                     ptr->segments[tid] = ptr->segments[tid]->next;
//                     std::cout << ptr->segments[tid]->value;
//                 } else {
//                     std::cout << std::endl;
//                     break;
//                 }
//             }
//         }
//         return -1;
//     }
//
////     int remove(int key) {
////         int segment = key & (INTERNAL_MAPS_COUNT -1);
//         pmem::obj::persistent_ptr<Segment<V> > ptr = segments[segment];
//         while(true)
//         {
//             if(ptr->next != nullptr && ptr->next->key == key)
//             {
//                 std::cout << "Found element with key" <<  key << "in segment" << segment << ". Trying to delete value = " << ptr->next->value << std::endl;
//                 auto temp = ptr->next->next;
// 		auto pop = pmem::obj::pool_by_vptr(this);
//                 pmem::obj::transaction::run(pop, [&] {
// 			pmem::obj::delete_persistent<Segment<V> >(ptr->next);
//                 });
// 		ptr->next = temp;
//                 return 1;
//             }
//             if(ptr->next != nullptr)
//             {
//                 ptr = ptr->next;
//             }
//             else
//                 break;
//         }
//         return -1;
//     }

//     V remove(int key, V value) {
//         throw "Not implemented yet!";
//     }
};


