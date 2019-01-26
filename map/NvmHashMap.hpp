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
#include <cmath>
#include <shared_mutex>
#include <cstdlib> 

template<class T>
int64_t typeToInteger(T arg);

template <>
int64_t typeToInteger(std::string arg)
{
    int64_t result = 0;
    for(int i = 0; i < arg.length(); i++) {
        result += pow(arg[i], i);
    }
    return result;
}


template <>
int64_t typeToInteger(const char* tab_arg)
{
    std::string arg(tab_arg);
    int64_t result = 0;
    for(int i = 0; i < arg.length(); i++) {
        result += pow(arg[i], i);
    }
    return result;
}

template <>
int64_t typeToInteger(int arg) {return arg;}

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
    pmem::obj::p<int32_t> hash;
    pmem::obj::p<int> size = 0;
    pmem::obj::persistent_ptr <SegmentObject<K, V>> head = nullptr;
};


template<class K, class V>
class ArrayOfSegments {
public:
    pmem::obj::persistent_ptr<Segment<K, V>[]> segments;
    pmem::obj::p<int> arraySize;
    pmem::obj::p<int> elementsCount;

    ArrayOfSegments() {
        this->segments = pmem::obj::make_persistent<Segment<K, V>[]>(16);
        this->arraySize = 16;
        elementsCount = 0;
    }

    ArrayOfSegments(int arraySize) {
        this->segments = pmem::obj::make_persistent<Segment<K, V>[]>(arraySize);
        this->arraySize = arraySize;
        elementsCount = 0;
    }

    ~ArrayOfSegments() {
        pmem::obj::delete_persistent<Segment<K, V>[]>(this->segments);

    }
};

template<class K, class V>
class NvmHashMap {
private:
    int internalMapsCount;
    pmem::obj::persistent_ptr <ArrayOfSegments<K, V>[] > arrayOfSegments;
    pmem::obj::persistent_ptr <pmem::obj::shared_mutex[]> arrayOfMutex;

    int32_t hash(K keyString) {
        int64_t key = typeToInteger(keyString);
        int32_t num_buckets = 10000000;
        int64_t b = 1;
        int64_t j = 0;
        while (j < num_buckets) {
            b = j;
            key = key * 2862933555777941757ULL + 1;
            j = (b + 1) * (double(1LL << 31) / double((key >> 33) + 1));
        }
        return fabs(b);
    }

    NvmHashMap<K, V> *getPtr() {
        return this;
    }

    int  insertIntoInternalArray(K key, V value, ArrayOfSegments<K, V>& aos) {
	int hash = this->hash(key);
        hash = hash >> int(std::log2(internalMapsCount));

        int index2 = hash % aos.arraySize;

        aos.segments[index2].hash = hash;
        pmem::obj::persistent_ptr <SegmentObject<K, V>> ptr = aos.segments[index2].head;

        while (true) {
            auto pop = pmem::obj::pool_by_vptr(this);
            if (ptr == nullptr) { // empty list
                pmem::obj::transaction::run(pop, [&] {
                    ptr = pmem::obj::make_persistent<SegmentObject<K, V> >();
                    ptr->key = key;
                    ptr->value = value;
                    aos.segments[index2].head = ptr;
                });
                return 1;
            }
            if (ptr->key.get_rw() == key) {
                pmem::obj::transaction::run(pop, [&] {
                    ptr->value = value;
                });
                return 0;
            }
            if (ptr->next == nullptr) { // it's the last item of the list
                pmem::obj::transaction::run(pop, [&] {
                    ptr->next = pmem::obj::make_persistent<SegmentObject<K, V> >();
                    ptr->next->key = key;
                    ptr->next->value = value;
                });
                return 1;
            }
            if (ptr->next != nullptr) {
                ptr = ptr->next;
            }
        }
    }


public:

    friend class Iterator<K, V>;

    NvmHashMap() {
        this->internalMapsCount = 8;
        arrayOfSegments = pmem::obj::make_persistent<ArrayOfSegments<K, V>[] >(this->internalMapsCount);
        arrayOfMutex = pmem::obj::make_persistent<pmem::obj::shared_mutex[] >(this->internalMapsCount);
        std::cout << "NvmHashMap() initialized" << std::endl;
    }

    NvmHashMap(int internalMapsCount) {
        if (internalMapsCount <= 0 || (internalMapsCount & (internalMapsCount -1)) != 0) {
            internalMapsCount = pow(2, std::floor(std::log2(internalMapsCount)));
        }
        this->internalMapsCount = internalMapsCount;
        arrayOfSegments = pmem::obj::make_persistent<ArrayOfSegments<K, V>[] >(this->internalMapsCount);
        arrayOfMutex = pmem::obj::make_persistent<pmem::obj::shared_mutex[] >(this->internalMapsCount);
        std::cout << "NvmHashMap() initialized" << std::endl;
    }


    V insertNew(K key, V value) {
        int hash = this->hash(key);
        int index = hash & (this->internalMapsCount - 1);

        std::unique_lock <pmem::obj::shared_mutex> lock(arrayOfMutex[index]);

        if (arrayOfSegments[index].elementsCount > 0.7*arrayOfSegments[index].arraySize) {
            expand(index);
        }

        int res = insertIntoInternalArray(key, value, arrayOfSegments[index]);
        arrayOfSegments[index].elementsCount = arrayOfSegments[index].elementsCount + res;
        return value;
    }

    V get(K key) {
        int hash = this->hash(key);
        int index = hash & (this->internalMapsCount - 1);
        hash = hash >> int(std::log2(internalMapsCount));
        int index2 = hash % arrayOfSegments[index].arraySize;
        std::shared_lock <pmem::obj::shared_mutex> lock(arrayOfMutex[index]);
        pmem::obj::persistent_ptr <SegmentObject<K, V>> ptr = arrayOfSegments[index].segments[index2].head;


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
        throw "Did not found element";
    }

    V remove(K key) {
	int hash = this->hash(key);
        int index = hash & (this->internalMapsCount - 1);
        hash = hash >> int(std::log2(internalMapsCount));
        int index2 = hash % arrayOfSegments[index].arraySize;
        std::unique_lock <pmem::obj::shared_mutex> lock(arrayOfMutex[index]);

        pmem::obj::persistent_ptr <SegmentObject<K, V>> ptr = arrayOfSegments[index].segments[index2].head;

        while (true) {
            if (ptr == nullptr) {
                break;
            }
            else if (ptr->key.get_rw() == key) {
                V value = ptr->value;
                auto pop = pmem::obj::pool_by_vptr(this);
                pmem::obj::transaction::run(pop, [&] {
                    arrayOfSegments[index].segments[index2].head = ptr->next;
                    pmem::obj::delete_persistent<SegmentObject<K, V> >(ptr);
                    arrayOfSegments[index].segments[index2].size =
                            arrayOfSegments[index].segments[index2].size - 1;
                });
                arrayOfSegments[index].elementsCount = arrayOfSegments[index].elementsCount - 1;
                return value;
            }
            if (ptr->next != nullptr) {
                if (ptr->next->key.get_rw() == key) {
                    V value = ptr->next->value;
                    auto temp = ptr->next->next;
                    auto pop = pmem::obj::pool_by_vptr(this);
                    pmem::obj::transaction::run(pop, [&] {
                        pmem::obj::delete_persistent<SegmentObject<K, V> >(ptr->next);
                        ptr->next = temp;
                        arrayOfSegments[index].segments[index2].size =
                                arrayOfSegments[index].segments[index2].size - 1;
                    });
                    arrayOfSegments[index].elementsCount = arrayOfSegments[index].elementsCount - 1;
                    return value;
                }
                ptr = ptr->next;
            } else {
                break;
            }
        }
        throw "Did not found element";
    }

    void expand(int arrayIndex) {
        int arraySize = this->arrayOfSegments[arrayIndex].arraySize;

        pmem::obj::persistent_ptr <ArrayOfSegments<K, V>> newArrayOfSegments;
        auto pop = pmem::obj::pool_by_vptr(this);

        pmem::obj::transaction::run(pop, [&] {
            newArrayOfSegments = pmem::obj::make_persistent<ArrayOfSegments<K, V> >(4 * arraySize);

            int hash;
            int index2;
            pmem::obj::persistent_ptr <SegmentObject<K, V>> ptr;

            for (int i = 0; i < arraySize; i++) {
                ptr = this->arrayOfSegments[arrayIndex].segments[i].head;
                while (true) {
                    if (ptr == nullptr) {
                        break;
                    }

                    insertIntoInternalArray(ptr->key.get_ro(), ptr->value.get_ro(), *newArrayOfSegments);
                    ptr = ptr->next;
                }
            }
            this->arrayOfSegments[arrayIndex] = *newArrayOfSegments;
        });
    }

    int getMapSize() {
        int size = 0;

        for (int i = 0; i < this->internalMapsCount; i++) {
            for (int j = 0; j < this->arrayOfSegments[i]->arraySize; j++) {
                size += this->arrayOfSegments[i]->segments[j].size;
            }
        }

        return size;
    }

    int getNumberOfInsertedElements(int arrayIndex) {
        int size = 0;

        for (int i = 0; i < this->arrayOfSegments[arrayIndex].arraySize; i++) {
            size += this->arrayOfSegments[arrayIndex].segments[i].size;
        }

        return size;
    }
};

template<class K, class V>
class Iterator {
private:
    NvmHashMap<K, V> *mapPointer;
    pmem::obj::persistent_ptr <SegmentObject<K, V>> currentSegmentObject;
    Segment<K, V> currentSegment;
    ArrayOfSegments<K, V> *currentArray;
    int currentSegmentIndex;
    int currentArrayIndex;

public:
    Iterator() = delete;

    Iterator(pmem::obj::persistent_ptr <NvmHashMap<K, V>> map) {

        mapPointer = map->getPtr();
        currentArrayIndex = 0;
        currentSegmentIndex = 0;
        std::shared_lock <pmem::obj::shared_mutex> lock(mapPointer->arrayOfMutex[currentArrayIndex]);
        currentArray = &mapPointer->arrayOfSegments[currentArrayIndex];
        currentSegment = currentArray->segments[currentArrayIndex];
        currentSegmentObject = currentSegment.head;
	if(currentSegmentObject == nullptr) {
	    next();
	}
    }

    V getValue() {
	return currentSegmentObject->value.get_ro();
    }

    V getKey() {
        return currentSegmentObject->key.get_ro();
    }

    bool next() {
	while (true) {
            if (currentSegmentObject != nullptr && currentSegmentObject->next != nullptr) {
                std::shared_lock <pmem::obj::shared_mutex> lock(mapPointer->arrayOfMutex[currentArrayIndex]);
                currentSegmentObject = currentSegmentObject->next;
                return true;
            } else if (currentSegmentIndex - 1 < currentArray->arraySize) {
                std::shared_lock <pmem::obj::shared_mutex> lock(mapPointer->arrayOfMutex[currentArrayIndex]);
                do {
                    currentSegment = currentArray->segments[++currentSegmentIndex];
                } while (currentSegment.head == nullptr && currentSegmentIndex - 1 < currentArray->arraySize);
                if (currentSegment.head != nullptr) {
                    currentSegmentObject = currentSegment.head;
                    return true;
                } else {
                    continue;
                }
            } else if (currentArrayIndex < mapPointer->internalMapsCount - 1) {
		std::shared_lock <pmem::obj::shared_mutex> lock(mapPointer->arrayOfMutex[++currentArrayIndex]);
                currentArray = &mapPointer->arrayOfSegments[currentArrayIndex];
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


