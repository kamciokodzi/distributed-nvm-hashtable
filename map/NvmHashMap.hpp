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

#define INTERNAL_MAPS_COUNT 10

template<class V>
        class Value {
public:
    V value;
};

template<class V>
class SegmentObject {
public:
    pmem::obj::p<V> value;
    pmem::obj::p<int> hash;
    pmem::obj::persistent_ptr<SegmentObject<V> > next = nullptr;
};

template<class V>
class Segment {
public:
    pmem::obj::p<int> key;
    pmem::obj::p<int> listSize=0;
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
    pmem::obj::p<int> arraySize;
    ArrayOfSegments() {
        this->arraySize = 10;
        for(int i=0; i<this->arraySize; i++) {
            this->segments[i] = pmem::obj::make_persistent< Segment<V> >();
        }
    }
    ArrayOfSegments(int arraySize){
        std::cout<< " HAHA " << arraySize << std::endl;
        this->arraySize = arraySize;
        for(int i=0; i<arraySize; i++) {
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
        int index = key & INTERNAL_MAPS_COUNT - 1;
        std::unique_lock <pmem::obj::mutex> lock(arrayOfSegmentsMutex[index]);
        int hash = 2 * key;
        int index2 = key % arrayOfSegments[index]->arraySize;

        if(this->needResize(index2)) {
            std::cout<< "PRZYDALBY SIE EXPAND arrayIndex " << index2 << std::endl;
            expand(index2);
        }

        arrayOfSegments[index]->segments[index2]->key = key;
        arrayOfSegments[index]->segments[index2]->listSize = arrayOfSegments[index]->segments[index2]->listSize + 1;
        pmem::obj::persistent_ptr <SegmentObject<V> > ptr = arrayOfSegments[index]->segments[index2]->head;
        while (true) {
            if (ptr->next == nullptr) { // it's the last item of the list
                if (ptr->hash == hash) {
                    break;
                }
                auto pop = pmem::obj::pool_by_vptr(this);
                pmem::obj::transaction::run(pop, [&] {
                    ptr->next = pmem::obj::make_persistent<SegmentObject<V> >();
                    ptr->next->hash = hash;
                    ptr->next->value = value;
                });
                break;
            } else {
                ptr = ptr->next;
                if (ptr->hash == hash) {
                    break;
                }
            }
        }
    }

    V get(int key) {
        int index = key & INTERNAL_MAPS_COUNT - 1;
        int index2 = key % arrayOfSegments[index]->arraySize;
        pmem::obj::persistent_ptr <SegmentObject<V>> ptr = arrayOfSegments[index]->segments[index2]->head;
        int hash = 2 * key;


        while (true) {
            if (ptr == nullptr) {
                break;
            } else {
                if (ptr->hash == hash) {
                    return ptr->value;
                } else {
                    if (ptr->next != nullptr) {
                        ptr = ptr->next;
                    } else { //end of the list
                        break;
                    }
                }
            }
        }

        return -1;
    }

    void expand(int arrayIndex) {
        int size = this->arrayOfSegments[arrayIndex]->arraySize ;

        pmem::obj::persistent_ptr<ArrayOfSegments<V> > arrayOfSegments;
        auto pop = pmem::obj::pool_by_vptr(this);
        pmem::obj::transaction::run(pop, [&] {
            arrayOfSegments = pmem::obj::make_persistent<ArrayOfSegments<V> >(2*size);
        });
        std::cout<< "NEW SIZE " << arrayOfSegments->arraySize << std::endl;


        for(int i=0; i<size; i++) {
            arrayOfSegments->segments[i] = this->arrayOfSegments[arrayIndex]->segments[i];
	        std::cout<<"original key: " << this->arrayOfSegments[arrayIndex]->segments[i]->key<<std::endl;
	        std::cout<<"copied key: " << arrayOfSegments->segments[i]->key <<std::endl;

        }
        std::cout << "HEHEHE" << std::endl;
        this->arrayOfSegments[arrayIndex] = arrayOfSegments;
        std::cout<< "NO I GUNWO " << std::endl;
    }

    int getMapSize() {
        int size = 0;

        for(int i =0; i<INTERNAL_MAPS_COUNT; i++) {
            for (int j = 0; j < this->arrayOfSegments[i]->arraySize; j++) {
                size += this->arrayOfSegments[i]->segments[j]->listSize;
            }
        }

        return size;
    }

    int getNumberOfInsertedElements(int arrayIndex) {
        int size =0;

        for(int i=0; i < this->arrayOfSegments[arrayIndex]->arraySize; i++){
            size += this->arrayOfSegments[arrayIndex]->segments[i]->listSize;
        }

        return size;
    }

    bool needResize(int arrayIndex) {
        int numberOfElements = 0;
        int numberOfCollidedElements = 0;


        for(int i=0; i< this->arrayOfSegments[arrayIndex]->arraySize; i++) {

            if(this->arrayOfSegments[arrayIndex]->segments[i]->listSize > 1) {
                numberOfCollidedElements += this->arrayOfSegments[arrayIndex]->segments[i]->listSize-1;
            }
            numberOfElements += this->arrayOfSegments[arrayIndex]->segments[i]->listSize;

        }

        if(numberOfElements == 0) {
            return false;
        }

        std::cout<< "No of collided el. " << numberOfCollidedElements << std::endl;
        std::cout<< "No of el. " << numberOfElements << std::endl;
        std::cout<<"Ten no stosunek " << (float)numberOfCollidedElements/(float)numberOfElements << std::endl;

        if((float)numberOfCollidedElements/(float)numberOfElements >= 0.75) {
            return true;
        }

        return false;
    }

};


