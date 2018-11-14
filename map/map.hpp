/**
 * Map.hpp
 *
 */


template<typename V>
class Map {
public:
    virtual void insert(int key, V value) = 0;

    virtual void insertNew(int key, V value) = 0;

    virtual V get(int key) = 0;

    virtual V remove(int key) = 0;

    virtual V remove(int key, V value) = 0;

    virtual int size() = 0;
};