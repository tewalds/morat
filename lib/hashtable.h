
#pragma once

#include "bits.h"
//#include "log.h"
//#include "string.h"

class HashTable {
	// A simple hash table. It support resizing, but you need to call resize once in a while
	// to make it happen. This is so it doesn't need to check regularly whether it needs to resize.
	// Alternatively, just initialize it big enough so you won't need to resize.

	//TODO: make lock free: http://preshing.com/20130605/the-worlds-simplest-lock-free-hash-table/

	static const uint32_t null_key = ~0;

	struct Entry {
		uint32_t key;
		double value;
		Entry() : key(null_key), value(0.0) { }
		Entry(uint32_t k, double v) : key(k), value(v) { }
	};

	class Iterator {
		Entry * cur, * end;
	public:
		Iterator(Entry * c, Entry * e) : cur(c-1), end(e) { ++(*this); }
		const Entry & operator * ()  const { return *cur; }
		const Entry * operator -> () const { return cur; }
		bool operator == (const Iterator & rhs) const { return (cur == rhs.cur); }
		bool operator != (const Iterator & rhs) const { return (cur != rhs.cur); }
		Iterator & operator ++ (){ //prefix form
			do {
				++cur;
			} while(cur != end && cur->key == null_key);
			return *this;
		}
	};

	uint32_t size;
	uint32_t mask;
	uint32_t resize_threshold;
	uint32_t entries;
	Entry * table;
	double default_value;
//	uint64_t collisions;
//	uint64_t lookups;

public:

	HashTable() : size(0), mask(0), resize_threshold(0), entries(0), table(NULL), default_value(0) { }
	HashTable(uint32_t s, double def=0.0) : default_value(def) { init(s); }
	~HashTable(){
//		logerr("Collisions: " + to_str(collisions) + ", lookups:" + to_str(lookups) + ", " + to_str(100.0*collisions/lookups, 2) + "%\n");
		if(table)
			delete[] table;
		table = NULL;
	}

	void set_default(double value){
		default_value = value;
	}

	void init(uint32_t s){
		size = roundup(s)*2;
		mask = size-1;
		resize_threshold = size*3/4;
		entries = 0;
		table = new Entry[size];
		clear();
//		collisions = 0;
//		lookups = 0;
	}

	void resize() {
		if(entries < resize_threshold)
			return;
		HashTable n(entries*2, default_value);
//		logerr("resize: " + to_str(entries) + " entries so " + to_str(size) + " -> " + to_str(n.size) + "\n");

		for(Entry * i = table; i != table+size; ++i)
			if(i->key != null_key)
				n[i->key] = i->value;

		delete[] table;
		table = n.table;
		n.table = NULL;
		size = n.size;
		mask = n.mask;
		resize_threshold = n.resize_threshold;
	}

	Iterator begin() const { return Iterator(table, table+size); }
	Iterator end()   const { return Iterator(table+size, table+size); }

	void clear(){
		for(uint32_t i = 0; i < size; i++)
			table[i] = Entry(null_key, 0.0); // value doesn't matter, default set at insertion time
		entries = 0;
	}

	double & operator[](uint32_t pattern) {
//		lookups++;
		uint32_t i = mix_bits(pattern) & mask;
//		int j = 1;
		while(true){
			if(table[i].key == pattern)
				return table[i].value;
			if(table[i].key == null_key){
				table[i] = Entry(pattern, default_value);
				entries++;
				return table[i].value;
			}
			i = (i + 1) & mask; // simple linear probing
//			i = (i + j) & mask;
//			i = (i + j*j) & mask; // quadratic probing
//			j++;
//			collisions++;
		}
	}
};
