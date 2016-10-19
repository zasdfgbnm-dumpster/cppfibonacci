#ifndef _CPP_FIBONACCI_
#define _CPP_FIBONACCI_

/** \mainpage cppfibonacci: a C++ implementation of Fibonacci heap
 *
 * For API, see fibonacci_heap.
 *
 * For sample code, see @ref example.cpp.
 */

#include <functional>
#include <tuple>
#include <initializer_list>
#include <memory>

/** \brief A C++ implementation of Fibonacci heap
 *
 * @param K the type for keys
 * @param T the type for data
 * @param Compare the class that define the order of keys, with default value the "<".
 */
template <typename K, typename T, typename Compare=std::less<K>>
class fibonacci_heap {

	class internal_data;

	/** \brief the internal class responsible for the structure in Fibonacci heap
	 * Structural information and data are stored to make it easier for std::shared_ptr
	 * to automatically clean up memory without destroying user's pointer to data.
	 */
	class internal_structure {
		bool childcut;
		size_t degree;
		std::shared_ptr<internal_data> data;
		std::shared_ptr<internal_structure> right_sibling;
		std::weak_ptr<internal_structure> left_sibling;
		std::shared_ptr<internal_structure> children;
		std::weak_ptr<internal_structure> parent;
	};

	/** \brief the interal class used to store data in Fibonacci heap */
	class internal_data {
	public:
		std::weak_ptr<internal_structure> structure;
		K key;
		T data;
	};

public:

	/** \brief Create an empty Fibonacci heap. */
	fibonacci_heap() = default;

	/** \brief Initialize a Fibonacci heap from list of key data pairs.
	 * @param list the list of key data pairs
	 */
	fibonacci_heap(std::initializer_list<std::tuple<K,T>> list) {
		for(auto &i:list)
			insert(std::get<0>(i),std::get<1>(i));
	}

	/** \brief Reference to nodes in Fibonacci heap.
	 *
	 * Objects of node should be returned from methods of fibonacci_heap,
	 * and will keep valid throughout the whole lifetime of the Fibonacci heap
	 * no matter what operations the users did.
	 */
	class node {

		/** \brief pointer to interanl node */
		std::shared_ptr<internal_data> internal;

		/** \brief create a node object from internal nodes
		 *
		 * This is a private constructor, so the users are not allowed to create a node object.
		 * @param internal pointer to internal node
		 */
		node(internal_data *internal):internal(internal){}

	public:

		/** \brief get the key of this node.
		 * @return the key of this node
		 */
		K key() const { return internal->key; }

		/** \brief get the data stored in this node.
		 * @return the lvalue holding the data stored in this node
		 */
		T &data() { return internal->data; }

		/** \brief get the data stored in this node.
		 * @return the rvalue holding the data stored in this node
		 */
		const T &data() const { return internal->data; }

	};

	/** \brief Return the number of elements stored.
	 *
	 * @return number of elements stored in this Fibonacci heap
	 */
	size_t size();

	/** \brief Insert an element.
	 *
	 * @param key the key of the element to be inserted
	 * @param data the data of the element to be inserted
	 * @return node object holding the inserted element
	 */
	node insert(K key,const T &data);

	/** \brief Insert an element.
	 *
	 * @param key the key of the element to be inserted
	 * @param value the data of the element to be inserted
	 * @return node object holding the inserted element
	 */
	node insert(K key,const T &&data);

	/** \brief Insert an element.
	 *
	 * @param node the node object holding the key and data of the element to be inserted
	 * @return node object holding the inserted element
	 */
	node insert(node n) { return insert(n.key(),n.data()); }

	/** \brief Return the top element.
	 * @return the node object on the top
	 */
	node top() const;

	/** \brief Meld another Fibonacci heap to this Fibonacci heap.
	 * @param heap the Fibonacci heap to be melded
	 */
	void meld(const fibonacci_heap<K,T,Compare> &heap);

	/** \brief Descrease (or increase if you use greater as Compare) the key of the given node.
	 *
	 * @param node the node object holding the key and data of the element to be inserted
	 * @param new_key the new key of the node
	 */
	void decrease_key(node,K new_key);

	/** \brief Remove the top element.
	 * @return the removed node object
	 */
	node remove();

	/** \brief Remove the element specified by the node object.
	 * @param n the node to be removed
	 * @return the removed node object
	 */
	node remove(node n);
};

#endif
