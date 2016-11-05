#ifndef _CPP_FIBONACCI_TEST_
#define _CPP_FIBONACCI_TEST_

#include <memory>
#include <vector>
#include <tuple>
#include <functional>
#include "fibonacci.hpp"

/** \brief contains tool functions for whitebox test*/
template <typename K, typename T, typename Compare=std::less<K>>
class fibonacci_whitebox {

	// this class is only a container of static methods, creating an object of
	// this class is not allowed
	fibonacci_whitebox();

public:

	// useful types
	using fh_t = fibonacci_heap<K,T,Compare>;
	using sn_t = typename fh_t::internal_structure;
	using dn_t = typename fh_t::internal_data;
	using ss_t = std::shared_ptr<sn_t>;
	using ws_t = std::weak_ptr<sn_t>;
	using sd_t = std::shared_ptr<dn_t>;
	using wd_t = std::weak_ptr<dn_t>;

	/** \brief types of inconsistent errors */
	enum consistency_errors {
		unexpected_nullptr, ///< there shouldn't be a nullptr, but there is
		min_tree_property_violation, ///< the min tree property is violated
		wrong_parent_pointer, ///< parent pointer is not what it is supposed to be
		null_left_sibling_pointer, ///< left_sibling pointer is nullptr
		null_right_sibling_pointer, ///< right_sibling pointer is nullptr
		doubly_linked_list_property_violation, ///< the doubly linked list property is violated
		null_data_pointer, ///< data pointer is nullptr
		bad_data_structure_pointer, ///< data node and structure node don't point to each other
		bad_degree, ///< the degree value stored don't match the number of children
		bad_min_pointer, ///< the min pointer of the Fibonacci heap does not point to the minimum value
		bad_size, ///< the size information stored don't match the total number of nodes
		degree_too_large ///< degree goes beyond theoretical upper bound
	};

private:

	/** \brief recursively run consistency test on each internal_structure node
	 *
	 * @param node the node to be tested
	 *
	 * @param parent the value of parent pointer that this node is supposed to have
	 *
	 * @param head used to denote what phase this function is doing. If head==nullptr,
	 * then this function has just started at a doubly linked list; if head!=nullptr,
	 * then this function is testing inside the doubly linked list and head is the first
	 * element of the doubly linked list.
	 *
	 * @return number of elements inside the doubly linked list tested starting at from
	 * this point including the parameter "node" itself
	 */
	static size_t _data_structure_consistency_test(ss_t node, ss_t parent, ss_t head) {
		// if node is null, head must also be null
		if(!node)
			if(head) throw unexpected_nullptr;
		// if sibling test done
		if (node==head)
			return 0;
		// if this is the beginning of sibling test
		if (head==nullptr) head = node;
		// test min-tree property
		if(parent)
			if(Compare()(node->data->key,parent->data->key)) throw min_tree_property_violation;
		// test parent and sibling pointers
		if(node->parent.lock()!=parent) throw wrong_parent_pointer;
		if(node->left_sibling.expired())
			throw null_left_sibling_pointer;
		else if(node->left_sibling.lock()->right_sibling!=node)
			throw doubly_linked_list_property_violation;
		if(!node->right_sibling)
			throw null_right_sibling_pointer;
		else if(node->right_sibling->left_sibling.lock()!=node)
			throw doubly_linked_list_property_violation;
		// test structure and data pointers
		if(!node->data)
			throw null_data_pointer;
		else if(node->data->structure.lock()!=node)
			throw bad_data_structure_pointer;
		// recursively run test on child and test degree
		size_t calculated_degree = _data_structure_consistency_test(node->child, node, nullptr);
		if(node->degree!=calculated_degree) throw bad_degree;
		// recursively run test on siblings
		return 1 + _data_structure_consistency_test(node->right_sibling, parent, head);
	}

	/** \brief run binomial property test on a tree rooted at root
	 *
	 * @param root the root of the tree to be tested
	 */
	static bool _is_binomial(ss_t root) {
		if(!root) return true;
		size_t degree = root->degree;
		if(degree==0){
			if(root->child) throw bad_degree;
			else return true;
		}
		bool children_degrees[degree];
		for(bool &i:children_degrees)
			i = false;
		ss_t p=root->child;
		do {
			if(!p) throw unexpected_nullptr;
			bool subtreetest = _is_binomial(p);
			if(!subtreetest) return false;
			if(p->degree<0||p->degree>=degree) return false;
			if(children_degrees[p->degree]) return false;
			children_degrees[p->degree] = true;
			p=p->right_sibling;
		} while(p!=root->child);
		for(bool i:children_degrees)
			if(!i) return false;
		return true;
	}

	/** \brief count nodes in Fibonacci heap */
	static size_t count_nodes(ss_t root) {
		if(root==nullptr) return 0;
		size_t sum = 0;
		ss_t p = root;
		do {
			sum += 1 + count_nodes(p->child);
			p = p->right_sibling;
		} while(p!=root);
		return sum;
	}

	/** \brief test if element is in Fibonacci heap */
	static bool element_in(ss_t e, const fh_t &fh) {
		while(!e->parent.expired())
			e = e->parent.lock();
		bool found = false;
		ss_t p = fh.min;
		do {
			if(p==e) {
				found = true;
				break;
			}
			p = p->right_sibling;
		} while(p!=fh.min);
		return found;
	}

	/** \brief test if all the forests given have the same structure */
	static bool expect_same_tree_structure(std::vector<ss_t> nodes) {
		// if pointers are null, return
		bool allnull = true;
		bool anynull = false;
		for(ss_t &i:nodes) {
			allnull = allnull && (i==nullptr);
			anynull = anynull || (i==nullptr);
		}
		if(allnull!=anynull) return false;
		if(anynull) return true;
		// traverse sibling list
		std::vector<ss_t> ps = nodes;
		bool done = false;
		do {
			// check if keys and values of different p are the same
			for(ss_t &p1:ps){
				for(ss_t &p2:ps){
					if(p1->data->key!=p2->data->key) return false;
					if(p1->data->data!=p2->data->data) return false;
				}
				std::vector<ss_t> children = nodes;
				for(ss_t &i:children) {
					i = i->child;
				}
				bool subtreetest = expect_same_tree_structure(children);
				if(!subtreetest) return false;
			}
			// update ps
			for(ss_t &p:ps)
				p = p->right_sibling;
			// see if we are done with this sibling list
			bool alldone = true;
			bool anydone = false;
			for(size_t i=0;i<nodes.size();i++) {
				alldone = alldone && (ps[i]==nodes[i]);
				anydone = anydone || (ps[i]==nodes[i]);
			}
			if(anydone!=alldone) return false;
			done = anydone;
		} while(!done);
		return true;
	}

public:

	/** \brief test the consistency of the forest of min trees maintained inside fibonacci_heap
	*
	* The following things are tested:
	* 1. parent pointer
	* 2. sibling pointers
	* 3. degrees
	* 4. data and structure pointers
	* 5. min-tree property
	* 6. min pointer of Fibonacci heap
	* 7. size
	* 8. max_degree
	*/
	static void data_structure_consistency_test(const fh_t &fh) {
		// test for 1-5
		_data_structure_consistency_test(fh.min,nullptr,nullptr);
		// test for 6
		if(fh.min) {
			for(ss_t p=fh.min->right_sibling; p!=fh.min; p=p->right_sibling) {
				if(!p) throw unexpected_nullptr;
				if(Compare()(p->data->key,fh.min->data->key)) throw bad_min_pointer;
			}
		}
		// test for 7
		if(fh._size!=count_nodes(fh.min)) throw bad_size;
		// test for 8
		if(fh.min) {
			size_t max_deg = fh.max_degree();
			ss_t p = fh.min;
			do {
				if(!p) throw unexpected_nullptr;
				if(p->degree>max_deg) throw degree_too_large;
				p = p->right_sibling;
			} while(p!=fh.min);
		}
	}

	/** \brief test whether the fibonacci_heap object is copied/moved correctly
	 *
	 * The following things are tested:
	 * 1. Are the new data structure consistent?
	 * 2. Is the property that there is no overlap between old and new Fibonacci heap satisfied?
	 * 3. Are the tree structures kept the same?
	 */
	static bool copy_move_test(const fh_t &fh) {
		fh_t fh2(fh); // copy constructor
		fh_t fh3 = fh; // copy constructor
		fh_t fh4;
		fh4 = fh; // assignment
		fh_t fh5 = fh_t(fh); // copy constructor, then move constructor
		fh_t fh6;
		fh6 = fh_t(fh); // copy constructor, then assignment
		// test for 1
		data_structure_consistency_test(fh);
		data_structure_consistency_test(fh2);
		data_structure_consistency_test(fh3);
		data_structure_consistency_test(fh4);
		data_structure_consistency_test(fh5);
		data_structure_consistency_test(fh6);
		// test for 2
		if(fh.min==fh2.min) return false;
		if(fh.min==fh3.min) return false;
		if(fh.min==fh4.min) return false;
		if(fh.min==fh5.min) return false;
		if(fh.min==fh6.min) return false;
		// test for 3
		return expect_same_tree_structure({fh.min,fh2.min,fh3.min,fh4.min,fh5.min,fh6.min});
	}

	/** \brief expect that this fibonacci_heap must be a binomial heap
	 *
	 * In the case that only insert, meld and remove min is performed,
	 * a Fibonacci heap should be exactly the same as a binomial heap.
	 * This method is designed to be called in this case to expect that
	 * the Fibonacci heap is actually a binomial heap.
	 */
	static bool is_binomial(const fh_t &fh) {
		if(fh.size()==0) return true;
		ss_t p=fh.min;
		do {
			bool subtreetest = _is_binomial(p);
			if(!subtreetest) return false;
			if(!p) throw unexpected_nullptr;
			p=p->right_sibling;
		} while(p!=fh.min);
		return true;
	}

	/** \brief test whether the cleanup procedure of a Fibonacci heap works well during descruction
	 *
	 * The following things are tested:
	 * 1. Are all the structure nodes destroyed?
	 * 2. Are all the data nodes without external reference destroyed?
	 * 3. Are the reference counts of all the data nodes with external reference decrease by one?
	 *
	 * @param fhptr the pointer pointing to the Fibonacci heap to be destroyed
	 */
	static bool destroy_and_test(std::shared_ptr<fh_t> &fhptr) {
		std::vector<ws_t> sn_clean_list;
		std::vector<wd_t> dn_clean_list;
		using kpl_t = std::tuple<wd_t,int>;
		std::vector<kpl_t> dn_keep_list;
		// dump out weak pointers to all the structure and data nodes
		std::function<void (ss_t,ss_t)> traverse = [&](ss_t node,ss_t head) {
			if(node==head) return;
			if(head==nullptr) head=node;
			sn_clean_list.push_back(node);
			if(node->data.unique())
				dn_clean_list.push_back(node->data);
			else
				dn_keep_list.push_back(make_tuple(node->data,node->data.use_count()));
			traverse(node->right_sibling,head);
			traverse(node->child,nullptr);
		};
		traverse(fhptr->min,nullptr);
		// destroy
		if(fhptr.unique()) throw "the shared_ptr must be unique in order to do destroy and test";
		fhptr.reset();
		// test for 1
		for(ws_t &i : sn_clean_list) {
			if(!i.expired()) return false;
		}
		// test for 2
		for(wd_t &i : dn_clean_list) {
			if(!i.expired()) return false;
		}
		// test for 3
		for(kpl_t &i : dn_keep_list) {
			if(std::get<0>(i).expired()) return false;
			if(std::get<0>(i).use_count()!=std::get<1>(i)-1) return false;
			if(!std::get<0>(i).lock()->structure.expired()) return false;
		}
		return true;
	}
};

#endif
