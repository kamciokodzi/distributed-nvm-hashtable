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
#include <libpmemobj++/shared_mutex.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <unistd.h>
#include <memory>
#include <string.h>
#include <shared_mutex>

#define INTERNAL_MAPS_COUNT 16

template<class, class>
class Iterator;

template<class V>
class Value {
public:
    V value;
};

template<class K>
class Key {
public:
    K key;
};

template<class K, class V>
class SegmentObject {
public:
    pmem::obj::p <K> key;
    pmem::obj::p <V> value;
    pmem::obj::persistent_ptr <SegmentObject<K, V>> next = nullptr;
};

template<class K, class V>
class Segment {
public:
    pmem::obj::p<int> hash;
    pmem::obj::p<int> size = 0;
    pmem::obj::persistent_ptr <SegmentObject<K, V>> head = nullptr;
};


template<class K, class V>
class ArrayOfSegments {
public:
    pmem::obj::persistent_ptr<Segment<K, V>[]> segments;
    pmem::obj::p<int> arraySize;

    ArrayOfSegments() {
        this->segments = pmem::obj::make_persistent<Segment<K, V>[]>(10);
        this->arraySize = 10;
    }

    ArrayOfSegments(int arraySize) {
        this->segments = pmem::obj::make_persistent<Segment<K, V>[]>(arraySize);
        this->arraySize = arraySize;
    }
};

template<class K, class V>
class NvmHashMap {
private:
    pmem::obj::persistent_ptr <ArrayOfSegments<K, V>> arrayOfSegments[INTERNAL_MAPS_COUNT];
    pmem::obj::shared_mutex arrayOfSegmentsMutex[INTERNAL_MAPS_COUNT];

    unsigned long long int hash(K key) {
        unsigned long long int keyPositive = (unsigned long long int) std::hash<K>()(key);
        return abs(keyPositive);
    }

    pmem::obj::persistent_ptr <ArrayOfSegments<K, V>> getArrayOfSegmentsWithId(int id) {
        if (id < INTERNAL_MAPS_COUNT)
            return arrayOfSegmentsMutex[id];
        else return nullptr;
    }

    NvmHashMap<K, V> *getPtr() {
        return this;
    }

public:

    friend class Iterator<K, V>;

    NvmHashMap() {
        std::cout << "NvmHashMap() start" << std::endl;
        for (int i = 0; i < INTERNAL_MAPS_COUNT; i++) {
            this->arrayOfSegments[i] = pmem::obj::make_persistent<ArrayOfSegments<K, V> >();
        }
        std::cout << "NvmHashMap() stop" << std::endl;
    }


    V insertNew(K key, V value) {
        int hash = this->hash(key);
        int index = hash & (INTERNAL_MAPS_COUNT - 1);


        std::unique_lock <pmem::obj::shared_mutex> lock(arrayOfSegmentsMutex[index]);

        if (this->needResize(index)) {
            expand(index);
        }

        int index2 = hash % this->arrayOfSegments[index]->arraySize; //Important AFTER expand

        arrayOfSegments[index]->segments[index2].hash = hash;
        pmem::obj::persistent_ptr <SegmentObject<K, V>> ptr = arrayOfSegments[index]->segments[index2].head;

        while (true) {
            if (ptr == nullptr) { // empty list
                auto pop = pmem::obj::pool_by_vptr(this);
                pmem::obj::transaction::run(pop, [&] {
                    ptr = pmem::obj::make_persistent<SegmentObject<K, V> >();
                    ptr->key = key;
                    ptr->value = value;
                    arrayOfSegments[index]->segments[index2].size = arrayOfSegments[index]->segments[index2].size + 1;
                    arrayOfSegments[index]->segments[index2].head = ptr;
                });
                break;
            }
            if (ptr->next == nullptr) { // it's the last item of the list
                auto pop = pmem::obj::pool_by_vptr(this);
                pmem::obj::transaction::run(pop, [&] {
                    ptr->next = pmem::obj::make_persistent<SegmentObject<K, V> >();
                    ptr->next->key = key;
                    ptr->next->value = value;
                    arrayOfSegments[index]->segments[index2].size = arrayOfSegments[index]->segments[index2].size + 1;
                });
                break;
            }
            if (ptr->key.get_rw() == key) {
                ptr->value = value;
                break;
            }

            if (ptr->next != nullptr) {
                ptr = ptr->next;
            } else {
                break;
            }
        }
        return value;
    }

    V get(K key) {
        int hash = this->hash(key);
        int index = hash & (INTERNAL_MAPS_COUNT - 1);
        int index2 = hash % arrayOfSegments[index]->arraySize;
        pmem::obj::persistent_ptr <SegmentObject<K, V>> ptr = arrayOfSegments[index]->segments[index2].head;

        while (true) {
            if (ptr == nullptr) {
                break;
            } else {
                if (ptr->key.get_rw() == key) {
                    return ptr->value;
                }
            }
            if (ptr->next != nullptr) {
                ptr = ptr->next;
            } else {
                break;
            }
        }
        return (V) 0;
    }

    V remove(K key) {
        int hash = this->hash(key);
        int index = hash & (INTERNAL_MAPS_COUNT - 1);
        int index2 = hash % arrayOfSegments[index]->arraySize;
        pmem::obj::persistent_ptr <SegmentObject<K, V>> ptr = arrayOfSegments[index]->segments[index2].head;

        while (true) {
            if (ptr == nullptr) {
                break;
            } else {
                if (ptr->key.get_rw() == key) {
                    V value = ptr->value;
                    auto pop = pmem::obj::pool_by_vptr(this);
                    pmem::obj::transaction::run(pop, [&] {
                        arrayOfSegments[index]->segments[index2].head = ptr->next;
                        pmem::obj::delete_persistent<SegmentObject<K, V> >(ptr);
                        arrayOfSegments[index]->segments[index2].size =
                                arrayOfSegments[index]->segments[index2].size - 1;
                    });
                    return value;
                }
            }
            if (ptr->next != nullptr) {
                if (ptr->next->key.get_rw() == key) {
                    V value = ptr->next->value;
                    auto temp = ptr->next->next;
                    auto pop = pmem::obj::pool_by_vptr(this);
                    pmem::obj::transaction::run(pop, [&] {
                        pmem::obj::delete_persistent<SegmentObject<K, V> >(ptr->next);
                        ptr->next = temp;
                        arrayOfSegments[index]->segments[index2].size =
                                arrayOfSegments[index]->segments[index2].size - 1;
                    });
                    return value;
                }
                ptr = ptr->next;
            } else {
                break;
            }
        }
        return (V) 0;
    }

    void expand(int arrayIndex) {
        int arraySize = this->arrayOfSegments[arrayIndex]->arraySize;

        pmem::obj::persistent_ptr <ArrayOfSegments<K, V>> arrayOfSegments;
        auto pop = pmem::obj::pool_by_vptr(this);
        pmem::obj::transaction::run(pop, [&] {
            arrayOfSegments = pmem::obj::make_persistent<ArrayOfSegments<K, V> >(2 * arraySize);

//            std::cout<< "NEW SIZE " << arrayOfSegments->arraySize << std::endl;

            int hash;
            int index2;
            pmem::obj::persistent_ptr <SegmentObject<K, V>> ptr;
            pmem::obj::persistent_ptr <SegmentObject<K, V>> tmp;

            for (int i = 0; i < arraySize; i++) {
                while (true) {
                    ptr = this->arrayOfSegments[arrayIndex]->segments[i].head;
                    if (ptr == nullptr) {
                        break;
                    }
                    this->arrayOfSegments[arrayIndex]->segments[i].head = ptr->next;
                    ptr->next = nullptr;

                    hash = this->hash(ptr->key);
                    index2 = hash % arrayOfSegments->arraySize;
//                    std::cout<<"Old index: "<<hash % arraySize<<" New index: "<<index2<<std::endl;
                    tmp = arrayOfSegments->segments[index2].head;
                    if (tmp == nullptr) {
                        arrayOfSegments->segments[index2].head = ptr;
                    } else {
                        while (true) {
                            if (tmp->next == nullptr) {
                                tmp->next = ptr;
                                break;
                            } else {
                                tmp = tmp->next;
                            }
                        }
                    }
                }
            }
            pmem::obj::delete_persistent<ArrayOfSegments<K, V> >(this->arrayOfSegments[arrayIndex]);
            this->arrayOfSegments[arrayIndex] = arrayOfSegments;
        });
    }

    int getMapSize() {
        int size = 0;

        for (int i = 0; i < INTERNAL_MAPS_COUNT; i++) {
            for (int j = 0; j < this->arrayOfSegments[i]->arraySize; j++) {
                size += this->arrayOfSegments[i]->segments[j].size;
            }
        }

        return size;
    }

    int getNumberOfInsertedElements(int arrayIndex) {
        int size = 0;

        for (int i = 0; i < this->arrayOfSegments[arrayIndex]->arraySize; i++) {
            size += this->arrayOfSegments[arrayIndex]->segments[i].size;
        }

        return size;
    }

    bool needResize(int arrayIndex) {
        int numberOfElements = 0;
        int numberOfCollidedElements = 0;

        for (int i = 0; i < this->arrayOfSegments[arrayIndex]->arraySize; i++) {

            if (this->arrayOfSegments[arrayIndex]->segments[i].size > 1) {
                numberOfCollidedElements += this->arrayOfSegments[arrayIndex]->segments[i].size - 1;
            }
            numberOfElements += this->arrayOfSegments[arrayIndex]->segments[i].size;
        }

        if (numberOfElements == 0) {
            return false;
        }

//        std::cout<< "No of collided el. " << numberOfCollidedElements << std::endl;
//        std::cout<< "No of el. " << numberOfElements << std::endl;
//        std::cout<<"Ratio " << (float)numberOfCollidedElements/(float)numberOfElements << std::endl;

        if ((float) numberOfCollidedElements / (float) numberOfElements >= 0.75) {
            return true;
        }

        return false;
    }
};

template<class K, class V>
class Iterator {
private:
    NvmHashMap<K, V> *mapPointer;
    pmem::obj::persistent_ptr <SegmentObject<K, V>> currentSegmentObject;
    Segment<K, V> currentSegment;
    pmem::obj::persistent_ptr <ArrayOfSegments<K, V>> currentArray;
    int currentSegmentIndex;
    int currentArrayIndex;

public:
    Iterator() = delete;

    Iterator(pmem::obj::persistent_ptr <NvmHashMap<K, V>> map) {
        mapPointer = map->getPtr();
        currentArrayIndex = 0;
        currentSegmentIndex = 0;
        std::shared_lock <pmem::obj::shared_mutex> lock(mapPointer->arrayOfSegmentsMutex[currentArrayIndex]);
        currentArray = mapPointer->arrayOfSegments[currentArrayIndex];
        currentSegment = currentArray->segments[currentArrayIndex];
        currentSegmentObject = currentSegment.head;
    }

    V get() {
        return currentSegmentObject->value.get_ro();
    }

    bool next() {
        while (true) {
            if (currentSegmentObject->next != nullptr) {
                std::shared_lock <pmem::obj::shared_mutex> lock(mapPointer->arrayOfSegmentsMutex[currentArrayIndex]);
                currentSegmentObject = currentSegmentObject->next;
                return true;
            } else if (currentSegmentIndex - 1 < currentArray->arraySize) {
                std::shared_lock <pmem::obj::shared_mutex> lock(mapPointer->arrayOfSegmentsMutex[currentArrayIndex]);
                do {
                    currentSegment = currentArray->segments[++currentSegmentIndex];
                } while (currentSegment.head == nullptr && currentSegmentIndex - 1 < currentArray->arraySize);
                if (currentSegment.head != nullptr) {
                    currentSegmentObject = currentSegment.head;
                    return true;
                } else {
                    continue;
                }
            } else if (currentArrayIndex < INTERNAL_MAPS_COUNT - 1) {
                std::shared_lock <pmem::obj::shared_mutex> lock(mapPointer->arrayOfSegmentsMutex[++currentArrayIndex]);
                currentArray = mapPointer->arrayOfSegments[currentArrayIndex];
                currentSegmentIndex = 0;
                currentSegment = currentArray->segments[0];
                if (currentSegment.head != nullptr) {
                    currentSegmentObject = currentSegment.head;
                    return true;
                } else {
                    continue;
                }
            } else return false;
        }
    }

};


