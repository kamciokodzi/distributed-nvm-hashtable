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

#define INTERNAL_MAPS_COUNT 2

template<class V>
        class Value {
public:
    V value;
};

template<class V>
class Segment {
public:
    pmem::obj::p<int> key;
    pmem::obj::p<int> hash;
    pmem::obj::p<V> value;
    pmem::obj::persistent_ptr<Segment<V> > next = nullptr;
};

template<class V>
class ArrayOfSegments {
public:
    pmem::obj::p<Segment<V>[] > segments;

    ArrayOfSegments() {
        for(int i=0; i<10; i++){
            this->segments[i] = pmem::obj::make_persistent< Segment<V> >();
        }
    }
}

template<typename V>
class NvmHashMap {
private:

    pmem::obj::persistent_ptr<ArrayOfSegments<V> > arrayOfSegments[INTERNAL_MAPS_COUNT];
    // pmem::obj::persistent_ptr<Segment<V> > segments[INTERNAL_MAPS_COUNT];
    // pmem::obj::persistent_ptr<Value<long> > mc[INTERNAL_MAPS_COUNT];
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

//     void insert(int key, V value) {
//         int segment = key & (INTERNAL_MAPS_COUNT-1);

//         std::unique_lock<pmem::obj::mutex> lock(segmentMutex[segment]);

//         pmem::obj::persistent_ptr<Segment<V> > ptr = segments[segment];
//         bool update = false;
//         while(true)
//         {
//             if(ptr->key == key)
//             {
//                 ptr->value = value;
//                 update = true;
// //                std::cout << "Updated value in segment " << segment << " with key " << key << std::endl;
//                 break;
//             }
//             if(ptr->next != nullptr)
//                 ptr = ptr->next;
//             else
//                 break;
//         }

//         if(!update) {
//             auto pop = pmem::obj::pool_by_vptr(this);
//             pmem::obj::transaction::run(pop, [&] {
//                 ptr->next = pmem::obj::make_persistent<Segment<V> >();
// 	    });
//         ptr->next->key = key;
//         ptr->next->value = value;
//   //          std::cout << "Inserted value to segment " << segment << std::endl;
//         }
//     }

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
//         }
//     }

//     V get(int key) {
//         int segment = key & (INTERNAL_MAPS_COUNT-1);
//         pmem::obj::persistent_ptr<Segment<V> > ptr = segments[segment];
//         while(true)
//         {
//             if(ptr->key == key)
//             {
//                 std::cout << "Found element with key " << key << " in segment " << segment << ". Its value = " << ptr->value << std::endl;
//                 return ptr->value;
//             }
//             if(ptr->next != nullptr)
//                 ptr = ptr->next;
//             else
//                 break;
//         }
//         std::cout << "Element not found" << std::endl;
//         return -1;
//     }

//     int remove(int key) {
//         int segment = key & (INTERNAL_MAPS_COUNT -1);
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


