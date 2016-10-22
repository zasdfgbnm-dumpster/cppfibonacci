#ifndef _CPP_FIBONACCI_
#define _CPP_FIBONACCI_

/** \mainpage cppfibonacci: a C++ implementation of Fibonacci heap
 *
 * For API, see fibonacci_heap.
 *
 * For sample code, see @ref example.cpp.
 *
 * GitHub address: https://github.com/zasdfgbnm/cppfibonacci
 */

#include <functional>
#include <tuple>
#include <initializer_list>
#include <memory>
#include <cmath>
#include <vector>

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

	class internal_structure;
	class internal_data;
	// useful short for types
	using ssp = std::shared_ptr<internal_structure>;
	using swp = std::weak_ptr<internal_structure>;

	/** \brief the internal class responsible for the structure in Fibonacci heap
	 * Structural information and data are stored to make it easier for std::shared_ptr
	 * to automatically clean up memory without destroying user's pointer to data.
	 */
	class internal_structure {
	public:
		bool childcut = false;
		size_t degree;
		std::shared_ptr<internal_data> data;
		ssp right_sibling;
		swp left_sibling;
		ssp child;
		swp parent;
		~internal_structure() {
			data->structure.reset();
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
		internal_data(const internal_data &old) = default;
		swp structure;
		K key;
		T data;
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
	static ssp duplicate_nodes(std::shared_ptr<const internal_structure> root,std::shared_ptr<const internal_structure> head,ssp newhead) {
		if(root==head) return nullptr;
		ssp newroot = std::make_shared<internal_structure>(*root);
		if(head==nullptr) {
			head = root;
			newhead = newroot;
		}
		// setup new data
		std::shared_ptr<internal_data> newroot_data = std::make_shared<internal_data>(*(root->data));
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
		newroot->parent.reset();
		if(newroot->child) newroot->child->parent = newroot;
		return newroot;
	}

	/** \brief Meld another forest to this Fibonacci heap.
	 *
	 * @param node pointer to the any root of the forest
	 * @param full whether to do a full meld or a temporary meld. A full meld fix
	 * the parent pointers of all roots that come in and update the min pointer,
	 * while a temporary meld only merge the sibling list and do nothing else.
	 */
	void meld(ssp node, bool full=true) {
		cout << "private meld" << endl;
		if(!node) return;
		// update parent
		ssp oldhead = node;
		ssp p = oldhead;
		if(full) {
			do {
				p->parent.reset();
				if(Compare()(p->data->key,node->data->key))
					node = p;
				p=p->right_sibling;
			} while(p!=oldhead);
		}
		// merge sibling list
		if(min) {
			std::swap(min->right_sibling,node->right_sibling);
			std::swap(min->right_sibling->left_sibling,node->right_sibling->left_sibling);
			if(full&&Compare()(node->data->key,min->data->key))
				min = node;
		} else {
			min = node;
		}
	}

	/** \brief insert a data node */
	node insert(std::shared_ptr<internal_data> datanode) {
		_size++;
		ssp p = std::make_shared<internal_structure>();
		datanode->structure = p;
		p->left_sibling = p;
		p->right_sibling = p;
		p->data = datanode;
		meld(p);
		return node(datanode);
	}

	/** \brief remove the subtree rooted at p */
	void remove_tree(ssp p) {
		cout << "remove_tree" << endl;
		if(!p->parent.expired()) {
			ssp pp = p->parent.lock();
			pp->degree--;
			if(pp->degree==0)
				pp->child = nullptr;
			else if(pp->child==p)
				pp->child = p->right_sibling;
		}
		std::swap(p->left_sibling.lock()->right_sibling, p->right_sibling);
		std::swap(p->right_sibling->left_sibling, p->left_sibling);
	}

	/** \brief cascading cut */
	void cascading_cut(ssp p) {
		if(p==nullptr) return;
		ssp oldparent = p->parent.lock();
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
	void remove(ssp p) {
		cout << "private remove" << endl;
		_size--;
		// remove n from tree
		remove_tree(p);
		cout << "done remove tree" << endl;
		p->data->structure.reset();
		// insert n's child back
		if(p->child) meld(p->child);
		// cascading cut
		cascading_cut(p->parent.lock());
	}

	/** \brief calculate the max degree of nodes */
	size_t max_degree() const {
		return std::floor(std::log(_size)/std::log((std::sqrt(5.0)+1.0)/2.0));
	}

	ssp min;
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
	fibonacci_heap(const fibonacci_heap &old):min(duplicate_nodes(old.min,nullptr,nullptr)),_size(old._size) {}

	/** \brief the move constructor.
	 *
	 * Move all the data from old Fibonacci heap to new one. The node objects at
	 * old Fibonacci heap can be used at new Fibonacci heap.
	 *
	 * @param old the Fibonacci heap to move data from
	 */
	fibonacci_heap(fibonacci_heap &&old):min(old.min),_size(old._size) {
		old.min = nullptr;
		old._size = 0;
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
		std::swap(this->_size,old._size);
		std::swap(this->min,old.min);
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

		friend class fibonacci_heap;

		/** \brief pointer to interanl node */
		std::shared_ptr<internal_data> internal;

		/** \brief create a node object from internal nodes
		 *
		 * This is a private constructor, so the users are not allowed to create a node object.
		 * @param internal pointer to internal node
		 */
		node(ssp internal):internal(internal->data){}

		/** \brief create a node object from internal nodes
		 *
		 * This is a private constructor, so the users are not allowed to create a node object.
		 * @param internal pointer to internal node
		 */
		node(std::shared_ptr<internal_data> internal):internal(internal){}

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

		/** \brief operator to test if two node are the same */
		bool operator==(node rhs) {
			return internal==rhs.internal;
		}

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
	node insert(K key,const T &data) { return insert(std::make_shared<internal_data>(key, data)); }

	/** \brief Insert an element.
	 *
	 * @param key the key of the element to be inserted
	 * @param value the data of the element to be inserted
	 * @return node object holding the inserted element
	 */
	node insert(K key,T &&data)  { return insert(std::make_shared<internal_data>(key, data)); }

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
		cout << "meld fh" << endl;
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
		if(Compare()(n.key(),new_key)) throw "increase_key is not supported";
		if(n.internal->structure.expired()) throw "the given node is not in this Fibonacci heap";
		ssp p = n.internal->structure.lock()->parent.lock();
		if(p) {
			if(Compare()(new_key,p->data->key)) {
				remove_tree(n.internal->structure.lock());
				meld(n.internal->structure.lock());
				cascading_cut(p);
			} else
				n.internal->key = new_key;
		} else {
			n.internal->key = new_key;
			if(Compare()(new_key,min->data->key))
				min = n.internal->structure.lock();
		}
	}

	/** \brief Remove the top element.
	 * @return the removed node object
	 */
	node remove() {
		cout << "remove min" << endl;
		if(_size==0) throw "no element to remove";
		ssp oldmin = min;
		if(_size==1) {
			_size = 0;
			min = nullptr;
			return node(oldmin);
		}

		// merge trees of same degrees
		cout << "max degree = " << max_degree() << endl;
		std::vector<ssp> trees(max_degree()+1);
		if(oldmin->child)
			meld(oldmin->child,false);
		int count = 0;
		for(ssp p=oldmin->right_sibling;p!=oldmin;) {
			cout << "tree #" << count++ << endl;
			cout << "tree=" << p.get() << " , left_sibling=" << p->left_sibling.lock().get() << " , right_sibling=" << p->right_sibling.get() << endl;
			ssp q = p;
			// same degree merge will change right_sibling of p, so we must update
			// p before same degree merge.
			p=p->right_sibling;
			while(trees[q->degree]) {
				cout << "merge needed for degree " << q->degree << endl;
				bool q_is_smaller = Compare()(q->data->key,trees[q->degree]->data->key);
				ssp smaller = q_is_smaller?q:trees[q->degree];
				ssp larger = q_is_smaller?trees[q->degree]:q;
				trees[q->degree] = nullptr;
				larger->childcut = false;
				larger->parent = smaller;
				smaller->degree++;
				if(!smaller->child) {
					smaller->child = larger;
					larger->right_sibling = larger;
					larger->left_sibling = larger;
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
		cout << "done same degree merge" << endl;

		// construct sibling list for roots
		for(ssp p:trees) {
			if(!p) continue;
			p->parent.reset();
			if(!min) {
				min = p;
				p->right_sibling = p;
				p->left_sibling = p;
			} else {
				p->right_sibling = min->right_sibling;
				p->left_sibling = min;
				min->right_sibling->left_sibling = p;
				min->right_sibling = p;
				if(Compare()(p->data->key,min->data->key))
					min = p;
			}
		}

		_size--;
		oldmin->data->structure.reset();
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
		if(n.internal->structure.expired()) throw "the given node is not in this Fibonacci heap";
		if(n.internal->structure.lock()==min) return remove();
		remove(n.internal->structure.lock());
		return n;
	}
};

#endif
