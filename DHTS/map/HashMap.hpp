#include <iostream>
#include <unistd.h>
#include <memory>
#include <string.h>
#include <cmath>
#include <mutex>
#include <shared_mutex>
#include <cstdlib>
#include <thread>

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
    K key;
    V value;
    SegmentObject<K, V> *next = nullptr;
};

template<class K, class V>
class Segment {
public:
    unsigned long long int hash;
    int size = 0;
    SegmentObject<K, V> *head = nullptr;
};


template<class K, class V>
class ArrayOfSegments {
public:
    Segment<K, V> *segments;
    int arraySize;
    int elementsCount;

    ArrayOfSegments() {
        this->segments = new Segment<K, V>[16];
        this->arraySize = 16;
        elementsCount = 0;
    }

    ArrayOfSegments(int arraySize) {
        this->segments = new Segment<K, V>[arraySize];
        this->arraySize = arraySize;
        elementsCount = 0;
    }

    ~ArrayOfSegments() {
        delete[] this->segments;
    }
};

template<class K, class V>
class HashMap {
private:
    int internalMapsCount;
    ArrayOfSegments<K, V>* arrayOfSegments;
    std::shared_mutex* arrayOfMutex;

    int32_t hash(K keyString, int32_t range = 10000000) {
        int64_t key = typeToInteger(keyString);
        int64_t b = 1;
        int64_t j = 0;
        for (int i = 0; i < 5; i++) {
            b = j;
            key = key * 2862933555777941757ULL + j;
            j = (b + 1) * (double(1LL << 31) / double((key >> 33) + 1));
        }
        return fabs(b % range);
    }

    HashMap<K, V> *getPtr() {
        return this;
    }

    int insertIntoInternalArray(K key, V value, ArrayOfSegments<K, V>& aos) {
	    int hash = this->hash(key);
        hash = hash >> int(std::log2(internalMapsCount));

        int index2 = hash % aos.arraySize;

        aos.segments[index2].hash = hash;

        SegmentObject<K, V> *ptr = aos.segments[index2].head;

        while (true) {
            if (ptr == nullptr) { // empty list
                ptr = new SegmentObject<K, V>();
                ptr->key = key;
                ptr->value = value;
                aos.segments[index2].head = ptr;
                return 1;
            }
            if (ptr->key == key) {
                ptr->value = value;
                return 0;
            }
            if (ptr->next == nullptr) {
                ptr->next = new SegmentObject<K, V>();
                ptr->next->key = key;
                ptr->next->value = value;
                return 1;
            }
            if (ptr->next != nullptr) {
                ptr = ptr->next;
            }
        }
    }


public:

    friend class Iterator<K, V>;

    HashMap() {
        this->internalMapsCount = 8;
        arrayOfSegments = new ArrayOfSegments<K, V>[this->internalMapsCount];
        arrayOfMutex = new std::shared_mutex[this->internalMapsCount];
        std::cout << "HashMap() initialized" << std::endl;
    }

    HashMap(int internalMapsCount) {
 	if (internalMapsCount <= 0 || (internalMapsCount & (internalMapsCount -1)) != 0) {
            internalMapsCount = pow(2, std::floor(std::log2(internalMapsCount)));
        }
        this->internalMapsCount = 2*internalMapsCount;
        arrayOfSegments = new ArrayOfSegments<K, V>[this->internalMapsCount];
        arrayOfMutex = new std::shared_mutex[this->internalMapsCount];
        std::cout << "HashMap() initialized" << std::endl;
    }


    V insertNew(K key, V value) {
        int hash = this->hash(key);
        int index = hash & (this->internalMapsCount - 1);

        std::unique_lock <std::shared_mutex> lock(arrayOfMutex[index]);

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
        std::shared_lock <std::shared_mutex> lock(arrayOfMutex[index]);
        SegmentObject<K, V> *ptr = arrayOfSegments[index].segments[index2].head;


        while (true) {
            if (ptr == nullptr) {
                break;
            } else {
                if (ptr->key == key) {
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
        std::unique_lock <std::shared_mutex> lock(arrayOfMutex[index]);

        SegmentObject<K, V> *ptr = arrayOfSegments[index].segments[index2].head;

        while (true) {
            if (ptr == nullptr) {
                break;
            }
            else if (ptr->key == key) {
                V value = ptr->value;
                arrayOfSegments[index].segments[index2].head = ptr->next;
                ptr = nullptr;
                arrayOfSegments[index].segments[index2].size =
                        arrayOfSegments[index].segments[index2].size - 1;
                arrayOfSegments[index].elementsCount = arrayOfSegments[index].elementsCount - 1;
                return value;
            }
            if (ptr->next != nullptr) {
                if (ptr->next->key == key) {
                    V value = ptr->next->value;
                    auto temp = ptr->next->next;
                    ptr->next = nullptr;
                    ptr->next = temp;
                    arrayOfSegments[index].segments[index2].size =
                            arrayOfSegments[index].segments[index2].size - 1;
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

        ArrayOfSegments<K, V>* newArrayOfSegments = new ArrayOfSegments<K, V> (4 * arraySize);

        int hash;
        int index2;
        SegmentObject<K, V> *ptr;

        for (int i = 0; i < arraySize; i++) {
            ptr = this->arrayOfSegments[arrayIndex].segments[i].head;
            while (true) {
                if (ptr == nullptr) {
                    break;
                }
                insertIntoInternalArray(ptr->key, ptr->value, *newArrayOfSegments);
                ptr = ptr->next;
            }
        }
        this->arrayOfSegments[arrayIndex] = *newArrayOfSegments;
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
    HashMap<K, V> *mapPointer;
    std::shared_ptr <SegmentObject<K, V>> currentSegmentObject;
    Segment<K, V> currentSegment;
    ArrayOfSegments<K, V> *currentArray;
    int currentSegmentIndex;
    int currentArrayIndex;

public:
    Iterator() = delete;

    Iterator(std::shared_ptr <HashMap<K, V>> map) {
        mapPointer = map->getPtr();
        currentArrayIndex = 0;
        currentSegmentIndex = 0;
        std::shared_lock <std::shared_mutex> lock(mapPointer->arrayOfMutex[currentArrayIndex]);
        currentArray = &mapPointer->arrayOfSegments[currentArrayIndex];
        currentSegment = currentArray->segments[currentArrayIndex];
        currentSegmentObject = currentSegment.head;
        if(currentSegmentObject == nullptr) {
            next();
        }
    }

    V get() {
	return currentSegmentObject->value;
    }

    bool next() {
        while (true) {
            if (currentSegmentObject != nullptr && currentSegmentObject->next != nullptr) {
                std::shared_lock <std::shared_mutex> lock(mapPointer->arrayOfMutex[currentArrayIndex]);
                currentSegmentObject = currentSegmentObject->next;
                return true;
            } else if (currentSegmentIndex - 1 < currentArray->arraySize) {
                std::shared_lock <std::shared_mutex> lock(mapPointer->arrayOfMutex[currentArrayIndex]);
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
                std::shared_lock <std::shared_mutex> lock(mapPointer->arrayOfMutex[++currentArrayIndex]);
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


