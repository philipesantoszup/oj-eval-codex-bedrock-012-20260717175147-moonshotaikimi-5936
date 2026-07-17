/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {
    /**
     * In linked_hashmap, iteration ordering is differ from map,
     * which is the order in which keys were inserted into the map.
     * You should maintain a doubly-linked list running through all
     * of its entries to keep the correct iteration order.
     *
     * Note that insertion order is not affected if a key is re-inserted
     * into the map.
     */
    
template<
	class Key,
	class T,
	class Hash = std::hash<Key>, 
	class Equal = std::equal_to<Key>
> class linked_hashmap {
public:
	/**
	 * the internal type of data.
	 * it should have a default constructor, a copy constructor.
	 * You can use sjtu::linked_hashmap as value_type by typedef.
	 */
	typedef pair<const Key, T> value_type;

private:
	// Node for doubly linked list (insertion order)
	struct Node {
		value_type *data;
		Node *prev;
		Node *next;
		
		Node() : data(nullptr), prev(nullptr), next(nullptr) {}
		Node(const value_type &val) : data(new value_type(val)), prev(nullptr), next(nullptr) {}
		~Node() {
			if (data != nullptr) {
				delete data;
			}
		}
	};

	// Hash table node
	struct HashNode {
		Node *listNode;
		HashNode *next;
		
		HashNode() : listNode(nullptr), next(nullptr) {}
		HashNode(Node *ln, HashNode *n) : listNode(ln), next(n) {}
	};

	// Sentinels for doubly linked list
	Node *head;  // dummy head
	Node *tail;  // dummy tail
	
	// Hash table
	HashNode **buckets;
	size_t bucketCount;
	size_t numElements;
	
	// Hash and equality functions
	Hash hashFunc;
	Equal equalFunc;
	
	static constexpr double LOAD_FACTOR = 0.75;
	static constexpr size_t INITIAL_CAPACITY = 16;

	// Get hash index
	size_t getIndex(const Key &key) const {
		return hashFunc(key) % bucketCount;
	}
	
	// Find in hash table
	HashNode* findInBucket(size_t idx, const Key &key) const {
		HashNode *cur = buckets[idx];
		while (cur != nullptr) {
			if (equalFunc(cur->listNode->data->first, key)) {
				return cur;
			}
			cur = cur->next;
		}
		return nullptr;
	}
	
	// Rehash when load factor exceeded
	void rehash() {
		size_t newBucketCount = bucketCount * 2;
		HashNode **newBuckets = new HashNode*[newBucketCount]();
		
		for (size_t i = 0; i < bucketCount; ++i) {
			HashNode *cur = buckets[i];
			while (cur != nullptr) {
				HashNode *next = cur->next;
				size_t newIdx = hashFunc(cur->listNode->data->first) % newBucketCount;
				cur->next = newBuckets[newIdx];
				newBuckets[newIdx] = cur;
				cur = next;
			}
		}
		
		delete[] buckets;
		buckets = newBuckets;
		bucketCount = newBucketCount;
	}
	
	// Insert into hash table (without rehash check)
	void insertToHashTable(Node *node) {
		size_t idx = getIndex(node->data->first);
		HashNode *hn = new HashNode(node, buckets[idx]);
		buckets[idx] = hn;
	}
	
	// Remove from hash table
	void removeFromHashTable(const Key &key) {
		size_t idx = getIndex(key);
		HashNode *cur = buckets[idx];
		HashNode *prev = nullptr;
		while (cur != nullptr) {
			if (equalFunc(cur->listNode->data->first, key)) {
				if (prev == nullptr) {
					buckets[idx] = cur->next;
				} else {
					prev->next = cur->next;
				}
				delete cur;
				return;
			}
			prev = cur;
			cur = cur->next;
		}
	}
	
	// Clear hash table
	void clearHashTable() {
		for (size_t i = 0; i < bucketCount; ++i) {
			HashNode *cur = buckets[i];
			while (cur != nullptr) {
				HashNode *next = cur->next;
				delete cur;
				cur = next;
			}
			buckets[i] = nullptr;
		}
	}
	
	// Clear linked list
	void clearLinkedList() {
		Node *cur = head->next;
		while (cur != tail) {
			Node *next = cur->next;
			delete cur;
			cur = next;
		}
		head->next = tail;
		tail->prev = head;
	}
	
	void init() {
		head = new Node();
		tail = new Node();
		head->next = tail;
		tail->prev = head;
		
		bucketCount = INITIAL_CAPACITY;
		buckets = new HashNode*[bucketCount]();
		numElements = 0;
	}

public:
	/**
	 * see BidirectionalIterator at CppReference for help.
	 *
	 * if there is anything wrong throw invalid_iterator.
	 *     like it = linked_hashmap.begin(); --it;
	 *       or it = linked_hashmap.end(); ++end();
	 */
	class const_iterator;
	class iterator {
	private:
		Node *node;
		const linked_hashmap *map;
		
		friend class linked_hashmap;
		friend class const_iterator;
		
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = typename linked_hashmap::value_type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::output_iterator_tag;


		iterator() : node(nullptr), map(nullptr) {}
		iterator(Node *n, const linked_hashmap *m) : node(n), map(m) {}
		iterator(const iterator &other) : node(other.node), map(other.map) {}
		/**
		 * TODO iter++
		 */
		iterator operator++(int) {
			if (node == nullptr || node == map->tail) {
				throw invalid_iterator();
			}
			iterator tmp(*this);
			node = node->next;
			return tmp;
		}
		/**
		 * TODO ++iter
		 */
		iterator & operator++() {
			if (node == nullptr || node == map->tail) {
				throw invalid_iterator();
			}
			node = node->next;
			return *this;
		}
		/**
		 * TODO iter--
		 */
		iterator operator--(int) {
			if (node == nullptr || node == map->head || node->prev == map->head) {
				throw invalid_iterator();
			}
			iterator tmp(*this);
			node = node->prev;
			return tmp;
		}
		/**
		 * TODO --iter
		 */
		iterator & operator--() {
			if (node == nullptr || node == map->head || node->prev == map->head) {
				throw invalid_iterator();
			}
			node = node->prev;
			return *this;
		}
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory).
		 */
		value_type & operator*() const {
			if (node == nullptr || node == map->tail) {
				throw invalid_iterator();
			}
			return *(node->data);
		}
		bool operator==(const iterator &rhs) const {
			return node == rhs.node;
		}
		bool operator==(const const_iterator &rhs) const {
			return node == rhs.node;
		}
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const {
			return node != rhs.node;
		}
		bool operator!=(const const_iterator &rhs) const {
			return node != rhs.node;
		}

		/**
		 * for the support of it->first. 
		 * See <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/> for help.
		 */
		value_type* operator->() const noexcept {
			return node->data;
		}
	};

	class const_iterator {
	private:
		const Node *node;
		const linked_hashmap *map;
		
		friend class linked_hashmap;
		
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = const typename linked_hashmap::value_type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::output_iterator_tag;

		const_iterator() : node(nullptr), map(nullptr) {}
		const_iterator(const Node *n, const linked_hashmap *m) : node(n), map(m) {}
		const_iterator(const const_iterator &other) : node(other.node), map(other.map) {}
		const_iterator(const iterator &other) : node(other.node), map(other.map) {}
		
		const_iterator operator++(int) {
			if (node == nullptr || node == map->tail) {
				throw invalid_iterator();
			}
			const_iterator tmp(*this);
			node = node->next;
			return tmp;
		}
		
		const_iterator & operator++() {
			if (node == nullptr || node == map->tail) {
				throw invalid_iterator();
			}
			node = node->next;
			return *this;
		}
		
		const_iterator operator--(int) {
			if (node == nullptr || node == map->head || node->prev == map->head) {
				throw invalid_iterator();
			}
			const_iterator tmp(*this);
			node = node->prev;
			return tmp;
		}
		
		const_iterator & operator--() {
			if (node == nullptr || node == map->head || node->prev == map->head) {
				throw invalid_iterator();
			}
			node = node->prev;
			return *this;
		}
		
		const value_type & operator*() const {
			if (node == nullptr || node == map->tail) {
				throw invalid_iterator();
			}
			return *(node->data);
		}
		
		bool operator==(const const_iterator &rhs) const {
			return node == rhs.node;
		}
		
		bool operator==(const iterator &rhs) const {
			return node == rhs.node;
		}
		
		bool operator!=(const const_iterator &rhs) const {
			return node != rhs.node;
		}
		
		bool operator!=(const iterator &rhs) const {
			return node != rhs.node;
		}
		
		const value_type* operator->() const noexcept {
			return node->data;
		}
	};

	/**
	 * TODO two constructors
	 */
	linked_hashmap() {
		init();
	}
	
	linked_hashmap(const linked_hashmap &other) {
		init();
		for (const_iterator it = other.cbegin(); it != other.cend(); ++it) {
			insert(*it);
		}
	}

	/**
	 * TODO assignment operator
	 */
	linked_hashmap & operator=(const linked_hashmap &other) {
		if (this != &other) {
			clear();
			for (const_iterator it = other.cbegin(); it != other.cend(); ++it) {
				insert(*it);
			}
		}
		return *this;
	}

	/**
	 * TODO Destructors
	 */
	~linked_hashmap() {
		clear();
		delete head;
		delete tail;
		delete[] buckets;
	}

	/**
	 * TODO
	 * access specified element with bounds checking
	 * Returns a reference to the mapped value of the element with key equivalent to key.
	 * If no such element exists, an exception of type `index_out_of_bound'
	 */
	T & at(const Key &key) {
		size_t idx = getIndex(key);
		HashNode *hn = findInBucket(idx, key);
		if (hn == nullptr) {
			throw index_out_of_bound();
		}
		return hn->listNode->data->second;
	}
	
	const T & at(const Key &key) const {
		size_t idx = getIndex(key);
		HashNode *hn = findInBucket(idx, key);
		if (hn == nullptr) {
			throw index_out_of_bound();
		}
		return hn->listNode->data->second;
	}

	/**
	 * TODO
	 * access specified element 
	 * Returns a reference to the value that is mapped to a key equivalent to key,
	 *   performing an insertion if such key does not already exist.
	 */
	T & operator[](const Key &key) {
		size_t idx = getIndex(key);
		HashNode *hn = findInBucket(idx, key);
		if (hn != nullptr) {
			return hn->listNode->data->second;
		}
		// Insert new element
		if (numElements + 1 > bucketCount * LOAD_FACTOR) {
			rehash();
			idx = getIndex(key);
		}
		// Create new node
		Node *newNode = new Node(value_type(key, T()));
		// Insert at end of linked list
		newNode->prev = tail->prev;
		newNode->next = tail;
		tail->prev->next = newNode;
		tail->prev = newNode;
		// Insert into hash table
		HashNode *newHashNode = new HashNode(newNode, buckets[idx]);
		buckets[idx] = newHashNode;
		++numElements;
		return newNode->data->second;
	}

	/**
	 * behave like at() throw index_out_of_bound if such key does not exist.
	 */
	const T & operator[](const Key &key) const {
		return at(key);
	}

	/**
	 * return a iterator to the beginning
	 */
	iterator begin() {
		return iterator(head->next, this);
	}
	
	const_iterator cbegin() const {
		return const_iterator(head->next, this);
	}

	/**
	 * return a iterator to the end
	 * in fact, it returns past-the-end.
	 */
	iterator end() {
		return iterator(tail, this);
	}
	
	const_iterator cend() const {
		return const_iterator(tail, this);
	}

	/**
	 * checks whether the container is empty
	 * return true if empty, otherwise false.
	 */
	bool empty() const {
		return numElements == 0;
	}

	/**
	 * returns the number of elements.
	 */
	size_t size() const {
		return numElements;
	}

	/**
	 * clears the contents
	 */
	void clear() {
		clearHashTable();
		clearLinkedList();
		numElements = 0;
	}

	/**
	 * insert an element.
	 * return a pair, the first of the pair is
	 *   the iterator to the new element (or the element that prevented the insertion), 
	 *   the second one is true if insert successfully, or false.
	 */
	pair<iterator, bool> insert(const value_type &value) {
		const Key &key = value.first;
		size_t idx = getIndex(key);
		HashNode *hn = findInBucket(idx, key);
		if (hn != nullptr) {
			// Key already exists
			return pair<iterator, bool>(iterator(hn->listNode, this), false);
		}
		
		// Check if rehash is needed
		if (numElements + 1 > bucketCount * LOAD_FACTOR) {
			rehash();
			idx = getIndex(key);
		}
		
		// Create new node
		Node *newNode = new Node(value_type(key, value.second));
		// Insert at end of linked list
		newNode->prev = tail->prev;
		newNode->next = tail;
		tail->prev->next = newNode;
		tail->prev = newNode;
		// Insert into hash table
		HashNode *newHashNode = new HashNode(newNode, buckets[idx]);
		buckets[idx] = newHashNode;
		++numElements;
		return pair<iterator, bool>(iterator(newNode, this), true);
	}

	/**
	 * erase the element at pos.
	 *
	 * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
	 */
	void erase(iterator pos) {
		if (pos == end() || pos.node == nullptr || pos.map != this) {
			throw invalid_iterator();
		}
		
		Node *node = pos.node;
		const Key &key = node->data->first;
		
		// Remove from linked list
		node->prev->next = node->next;
		node->next->prev = node->prev;
		
		// Remove from hash table
		removeFromHashTable(key);
		
		// Delete node
		delete node;
		--numElements;
	}

	/**
	 * Returns the number of elements with key 
	 *   that compares equivalent to the specified argument,
	 *   which is either 1 or 0 
	 *     since this container does not allow duplicates.
	 */
	size_t count(const Key &key) const {
		size_t idx = hashFunc(key) % bucketCount;
		HashNode *hn = findInBucket(idx, key);
		return (hn != nullptr) ? 1 : 0;
	}

	/**
	 * Finds an element with key equivalent to key.
	 * key value of the element to search for.
	 * Iterator to an element with key equivalent to key.
	 *   If no such element is found, past-the-end (see end()) iterator is returned.
	 */
	iterator find(const Key &key) {
		size_t idx = getIndex(key);
		HashNode *hn = findInBucket(idx, key);
		if (hn != nullptr) {
			return iterator(hn->listNode, this);
		}
		return end();
	}
	
	const_iterator find(const Key &key) const {
		size_t idx = hashFunc(key) % bucketCount;
		HashNode *hn = findInBucket(idx, key);
		if (hn != nullptr) {
			return const_iterator(hn->listNode, this);
		}
		return cend();
	}
};

}

#endif
