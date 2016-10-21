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
#include <cmath>

#ifdef FIBONACCI_HEAP_TEST_FRIEND
class FIBONACCI_HEAP_TEST_FRIEND;
#endif

/** \brief A C++ implementation of Fibonacci heap
 *
 * @param K the type for keys
 * @param T the type for data
 * @param Compare the class that define the order of keys, with default value the "<".
 */
template <typename K, typename T, typename Compare=std::less<K>>
class fibonacci_heap {

public:
	class node;

private:

	/** To allow user defined test class to access private members of this class,
	  * simply define the test class name as macro FIBONACCI_HEAP_TEST_FRIEND
	  */
	#ifdef FIBONACCI_HEAP_TEST_FRIEND
	friend class FIBONACCI_HEAP_TEST_FRIEND;
	#endif

	class internal_data;

	/** \brief the internal class responsible for the structure in Fibonacci heap
	 * Structural information and data are stored to make it easier for std::shared_ptr
	 * to automatically clean up memory without destroying user's pointer to data.
	 */
	class internal_structure {
		bool childcut = false;
		size_t degree;
		std::shared_ptr<internal_data> data;
		std::shared_ptr<internal_structure> right_sibling;
		std::weak_ptr<internal_structure> left_sibling;
		std::shared_ptr<internal_structure> child;
		std::weak_ptr<internal_structure> parent;
		~internal_structure() {
			data->structure = nullptr;
			// cut loops inside child's sibling list so that std::shared_ptr can
			// automatically free unneeded memory
			if(child) child->right_sibling = nullptr;
		}
	};

	/** \brief the interal class used to store data in Fibonacci heap */
	class internal_data {
	public:
		internal_data(K key,const T &data):key(key),data(data) {}
		internal_data(K key,T &&data):key(key),data(data) {}
		std::weak_ptr<internal_structure> structure;
		K key;
		T data;
		~internal_data() {
			if(!structure.expired()) throw "data node in use, destructing will cause unexpected behavior";
		}
	};

	/** \brief recursively duplicate nodes and create a new forest, including structure node and data node
	 *
	 * @param root the root node of the tree to be duplicated
	 *
	 * @param head used to denote what phase this function is doing. If head==nullptr,
	 * then this function has just started at a doubly linked list; if head!=nullptr,
	 * then this function is checking inside the doubly linked list and head is the first
	 * element of the doubly linked list.
	 *
	 * @param newhead used only in the case when head!=nullptr, then newhead is
	 * the pointer towards the duplicated node of head
	 *
	 * @return pointer to the node of the duplicated tree
	 */
	static std::shared_ptr<internal_structure> duplicate_nodes(std::shared_ptr<const internal_structure> root,std::shared_ptr<const internal_structure> head,std::shared_ptr<internal_structure> newhead) {
		if(root==head) return nullptr;
		std::shared_ptr<internal_structure> newroot = std::make_shared<internal_structure>(*root);
		if(head==nullptr) {
			head = root;
			newhead = newroot;
		}
		// setup new data
		std::shared_ptr<internal_data> newroot_data = std::make_shared<internal_data>(root->data);
		newroot_data->structure = newroot;
		newroot->data = newroot_data;
		// setup new right_sibling
		newroot->right_sibling = duplicate_nodes(root->right_sibling, head, newhead);
		if(newroot->right_sibling==nullptr)
			newroot->right_sibling = newhead;
		// setup new left_sibling
		newroot->right_sibling->left_sibling = newroot;
		// setup new child
		newroot->child = duplicate_nodes(root->child, nullptr, nullptr);
		// setup new parent
		newroot->parent = nullptr;
		if(newroot->child) newroot->child->parent = newroot;
	}

	/** \brief Meld another forest to this Fibonacci heap.
	 *
	 * @param node pointer to the any root of the forest
	 */
	void meld(std::shared_ptr<internal_structure> node) {
		// update parent
		std::shared_ptr<internal_structure> oldhead = node;
		std::shared_ptr<internal_structure> p = oldhead;
		do {
			p->parent = min->parent;
			if(Compare()(p->data->key,node->data->key))
				node = p;
			p=p->right_sibling;
		} while(p!=oldhead);
		// merge sibling list
		std::swap(min->right_sibling,node->right_sibling);
		std::swap(min->right_sibling->left_sibling,node->right_sibling->left_sibling);
		if(Compare()(node->data->key,min->data->key))
			min = node;
	}

	/** \brief insert a data node */
	node insert(std::shared_ptr<internal_data> datanode) {
		_size++;
		datanode->structure = std::make_shared<internal_structure>();
		datanode->structure->left_sibling = datanode->structure;
		datanode->structure->right_sibling = datanode->structure;
		meld(datanode->structure);
		return datanode->structure;
	}

	/** \brief remove the subtree rooted at p */
	void remove_tree(std::shared_ptr<internal_structure> p) {
		if(p->parent) {
			p->parent->degree--;
			if(p->parent->degree==0)
				p->parent->child = nullptr;
			else if(p->parent->child==p)
				p->parent->child = p->right_sibling;
		}
		std::swap(p->left_sibling->right_sibling, p->right_sibling);
		std::swap(p->right_sibling->left_sibling, p->left_sibling);
	}

	/** \brief cascading cut */
	void cascading_cut(std::shared_ptr<internal_structure> p) {
		if(p==nullptr) return;
		std::shared_ptr<internal_structure> oldparent = p->parent;
		if(oldparent){
			if(p->childcut){
				remove_tree(p);
				meld(p);
				cascading_cut(oldparent);
			} else
				p->childcut = true;
		}
	}

	/** \brief Remove the element specified by the parameter.*/
	void remove(std::shared_ptr<internal_structure> p) {
		if(p==min)
			return remove();
		_size--;
		// remove n from tree
		remove_tree(p);
		p->data->structure = nullptr;
		// insert n's child back
		meld(p->child);
		// cascading cut
		cascading_cut(p->parent);
	}

	/** \brief calculate the max degree of nodes */
	size_t max_degree() {
		return std::floor(std::log(_size)/std::log((std::sqrt(5.0)+1.0)/2.0));
	}

	std::shared_ptr<internal_structure> min;
	size_t _size = 0;

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

	/** \brief the copy constructor.
	 *
	 * Shallow copy will mess up the data structure and therefore is not allowed.
	 * Whenever the user tries to make a copy of a fibonacci_heap object, a deep
	 * copy will be made.
	 *
	 * Also note that the node objects at old Fibonacci heap can not be used at
	 * copied Fibonacci heap.
	 *
	 * @param old the Fibonacci heap to be copied
	 */
	fibonacci_heap(const fibonacci_heap &old):_size(old._size),min(duplicate_nodes(old.min,nullptr,nullptr)) {}

	/** \brief the move constructor.
	 *
	 * Move all the data from old Fibonacci heap to new one. The node objects at
	 * old Fibonacci heap can be used at new Fibonacci heap.
	 *
	 * @param old the Fibonacci heap to move data from
	 */
	fibonacci_heap(fibonacci_heap &&old):_size(old._size),min(old.min) {
		old.min = nullptr;
	}

	~fibonacci_heap() {
		// cut loops inside the forest list so that std::shared_ptr can
		// automatically free unneeded memory
		if(min) min->right_sibling = nullptr;
	}

	/** \brief the assignment operator, using copy-and-swap idiom
	 *
	 * @param old the Fibonacci heap to be copied
	 *
	 * @return reference to this object
	 */
	fibonacci_heap& operator = (fibonacci_heap old) {
		std::swap(this->_size,old->_size);
		std::swap(this->min,old->min);
		return *this;
	}

	/** \brief Reference to nodes in Fibonacci heap.
	 *
	 * Objects of node should be returned from methods of fibonacci_heap,
	 * and will keep valid throughout the whole lifetime of the Fibonacci heap.
	 * If the original Fibonacci heap is copied to a new heap, node objects of
	 * the original Fibonacci heap will not work on the new heap.
	 */
	class node {

		/** \brief pointer to interanl node */
		std::shared_ptr<internal_data> internal;

		/** \brief create a node object from internal nodes
		 *
		 * This is a private constructor, so the users are not allowed to create a node object.
		 * @param internal pointer to internal node
		 */
		node(std::shared_ptr<internal_structure> internal):internal(internal->data){}

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
	size_t size() { return _size; }

	/** \brief Insert an element.
	 *
	 * @param key the key of the element to be inserted
	 * @param data the data of the element to be inserted
	 * @return node object holding the inserted element
	 */
	node insert(K key,const T &data) { return insert(internal_data(key, data)); }

	/** \brief Insert an element.
	 *
	 * @param key the key of the element to be inserted
	 * @param value the data of the element to be inserted
	 * @return node object holding the inserted element
	 */
	node insert(K key,T &&data)  { return insert(internal_data(key, data)); }

	/** \brief Insert an element.
	 *
	 * @param node the node object holding the key and data of the element to be inserted
	 * @return node object holding the inserted element
	 */
	node insert(node n) { return insert(n.key(),n.data()); }

	/** \brief Return the top element.
	 * @return the node object on the top
	 */
	node top() const {
		if(_size==0) throw "this Fibonacci heap is empty";
		return node(min);
	}

	/** \brief Meld another Fibonacci heap to this Fibonacci heap.
	 *
	 * After meld, all the data will be moved to this Fibonacci heap, and the
	 * parameter "fh" will become empty. After meld, both the node objects of
	 * this and the node objects of parameter "fh" will work on this.
	 *
	 * @param fh the Fibonacci heap to be melded
	 */
	void meld(fibonacci_heap<K,T,Compare> &fh) {
		meld(fh.min);
		fh.min = nullptr;
		_size += fh._size;
		fh._size = 0;
	}

	/** \brief Descrease (or increase if you use greater as Compare) the key of the given node.
	 *
	 * It is the user's responsibility to make sure that the given node is
	 * actually in this Fibonacci heap. Trying to decrease a key of a node
	 * not in this Fibonacci heap will have undefined behavior.
	 *
	 * @param n the node object holding the key and data of the element to be inserted
	 * @param new_key the new key of the node
	 */
	void decrease_key(node n,K new_key) {
		if(Compare()(n->key(),new_key)) throw "increase_key is not supported";
		if(n.internal->structure==nullptr) throw "the given node is not in this Fibonacci heap";
		std::shared_ptr<internal_structure> p = n.internal->structure->parent;
		if(p) {
			if(Compare()(new_key,p->data->key)) {
				remove_tree(n.internal->structure);
				meld(n.internal->structure);
				cascading_cut(p);
			} else
				n.internal->key = new_key;
		} else {
			n.internal->key = new_key;
			if(Compare()(new_key,min->data->key))
				min = n.internal->structure;
		}
	}

	/** \brief Remove the top element.
	 * @return the removed node object
	 */
	node remove() {
		if(_size==0) throw "no element to remove";
		std::shared_ptr<internal_structure> oldmin = min;
		min == nullptr;

		// merge trees of same degrees
		std::shared_ptr<internal_structure> trees[max_degree()+1];
		std::shared_ptr<internal_structure> p = min->child;
		while(p!=min) {
			std::shared_ptr<internal_structure> q = p;

			// move p to next root
			p = p->right_sibling;
			if(p==min->child)
				p = min->right_sibling;

			while(trees[q->degree]!=nullptr) {
				bool q_is_smaller = Compare()(q,trees[q->degree]);
				std::shared_ptr<internal_structure> smaller = q_is_smaller?q:trees[q->degree];
				std::shared_ptr<internal_structure> larger = q_is_smaller?trees[q->degree]:q;
				trees[q->degree] = nullptr;
				larger->childcut = false;
				larger->parent = smaller;
				smaller->degree++;
				if(smaller->child==nullptr) {
					smaller->child = larger;
					larger->right_sibling = larger->left_sibling = larger;
				} else {
					larger->right_sibling = smaller->child->right_sibling;
					smaller->child->right_sibling->left_sibling = larger;
					larger->left_sibling = smaller->child;
					smaller->child->right_sibling = larger;
				}
				q = smaller;
			}
			trees[q->degree] = q;
		}

		// construct sibling list for roots
		for(std::shared_ptr<internal_structure> p:trees) {
			if(p==nullptr) continue;
			p->parent = nullptr;
			if(min==nullptr) {
				min = p;
				p->right_sibling = p->left_sibling = p;
			} else {
				p->right_sibling = min->right_sibling;
				p->left_sibling = min;
				min->right_sibling.left_sibling = p;
				min->right_sibling = p;
				if(Compare()(p->data->key,min->data->key))
					min = p;
			}
		}

		_size--;
		oldmin->data->structure = nullptr;
		return node(oldmin->data);
	}

	/** \brief Remove the element specified by the node object.
	 *
	 * It is the user's responsibility to make sure that the given node is
	 * actually in this Fibonacci heap. Trying to remove a node not in this
	 * Fibonacci heap will have undefined behavior.
	 *
	 * @param n the node to be removed
	 * @return the removed node object
	 */
	node remove(node n) {
		if(n.internal->structure==nullptr) throw "the given node is not in this Fibonacci heap";
		remove(n.internal->structure);
		return n;
	}
};

#endif
