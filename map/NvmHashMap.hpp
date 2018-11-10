#include "map.hpp"
#include <iostream>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/utils.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <memory>
#include <string.h>

namespace put
{
   template <typename V> class NvmHashMap : Map<V>
   {
     private:
	
	nvobj::persistent_ptr<Segment> root;
	
	

	size_t hash(int key)
	{
		return std::hash(key);
	}	

     public:

        NvmHashMap()
	{
		std::cout << "NvmHashMap() start" << std::endl;
		
		this->root = nvobj::make_persistent<Segment>();		

		std::cout << "NvmHashMap() stop" << std::endl;
	}

	int size()
	{
		throw "Not implemented yet!";
	}	
	
	void insert(int key, V value)
	{
		this->root->value = value;
	}

	void insertNew(int key, V value)
	{
		throw "Not implemented yet!";	
	}

	V get(int key)
	{
		return this->root->value;
	}

	V remove(int key)
	{
		throw "Not implemented yet!";
	}
	
	V remove(int key, V value)
	{
		throw "Not implemented yet!";
	}
   }


  template <typename V> class Segment
  {
    public:
	int key;
	int hash;
	V value;
	Segment* next;
  }

}
