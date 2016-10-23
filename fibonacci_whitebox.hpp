#ifndef _CPP_FIBONACCI_TEST_
#define _CPP_FIBONACCI_TEST_

#include <memory>
#include <vector>
#include <tuple>
#include <functional>

// for debug only
#include <iostream>
using namespace std;

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
	static bool _data_structure_consistency_test(ss_t node, ss_t parent, ss_t head, size_t &degree) {
		// if node is null, head must also be null
		if(!node)
			if(head) return false;
		// if sibling test done
		if (node==head) {
			degree = 0;
			return true;
		}
		// if this is the beginning of sibling test
		if (head==nullptr) head = node;
		// test min-tree property
		if(parent)
			if(Compare()(node->data->key,parent->data->key)) return false;
		// test parent and sibling pointers
		if(node->parent.expired())
			return false;
		else if(node->parent.lock()!=parent)
			return false;
		if(node->left_sibling.expired())
			return false;
		else if(node->left_sibling.lock()->right_sibling!=node)
			return false;
		if(!node->right_sibling)
			return false;
		else if(node->right_sibling->left_sibling.lock()!=node)
			return false;
		// test structure and data pointers
		if(!node->data)
			return false;
		else if(node->data->structure.lock()!=node)
			return false;
		// recursively run test on child and test degree
		size_t calculated_degree;
		bool subtreetest = _data_structure_consistency_test(node->child, node, nullptr,calculated_degree);
		if(!subtreetest)
			return false;
		if(node->degree!=calculated_degree)
			return false;
		// recursively run test on siblings
		bool siblingtest = _data_structure_consistency_test(node->right_sibling, parent, head,degree);
		degree++;
		return siblingtest;
	}

	/** \brief run binomial property test on a tree rooted at root
	 *
	 * @param root the root of the tree to be tested
	 */
	static bool _expect_binomial(ss_t root) {
		size_t degree = root->degree;
		if(degree == 0) {
			if(!root->child) return false;
			return true;
		}
		bool children_degrees[degree];
		for(bool &i:children_degrees)
			i = false;
		ss_t p=root->child;
		do {
			bool subtreetest = _expect_binomial(p);
			if(!subtreetest) return false;
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
	static bool data_structure_consistency_test(const fh_t &fh) {
		// test for 1-5
		cout << "start test" << endl;
		size_t unused;
		if(!_data_structure_consistency_test(fh.min,nullptr,nullptr,unused))
			return false;
		cout << "pass 1-5" << endl;
		// test for 6
		if(fh.min) {
			for(ss_t p=fh.min->right_sibling; p!=fh.min; p=p->right_sibling) {
				if(!p) return false;
				if(Compare()(p->data->key,fh.min->data->key)) return false;
			}
		}
		cout << "pass 6" << endl;
		// test for 7
		if(fh._size!=count_nodes(fh.min)) return false;
		cout << "pass 7" << endl;
		// test for 8
		if(fh.min) {
			size_t max_deg = fh.max_degree();
			ss_t p = fh.min;
			do {
				if(!p) return false;
				if(p->degree>max_deg) return false;
				p = p->right_sibling;
			} while(p!=fh.min);
		}
		cout << "pass 8" << endl;
		return true;
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
		bool test1 =
			data_structure_consistency_test(fh)  &&
			data_structure_consistency_test(fh2) &&
			data_structure_consistency_test(fh3) &&
			data_structure_consistency_test(fh4) &&
			data_structure_consistency_test(fh5) &&
			data_structure_consistency_test(fh6);
		if(!test1) return false;
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
	static bool expect_binomial(const fh_t &fh) {
		ss_t p=fh.min;
		do {
			bool subtreetest = _expect_binomial(p);
			if(!subtreetest) return false;
			p=p->right_sibling;
		} while(p!=fh.min);
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
		std::vector<std::tuple<wd_t,size_t>> dn_keep_list;
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
		for(std::tuple<wd_t,size_t> &i : dn_keep_list) {
			if(std::get<0>(i).expired()) return false;
			if(std::get<0>(i).use_count()!=std::get<1>(i)-1) return false;
			if(!std::get<0>(i).lock()->structure.expired()) return false;
		}
	}
};

#endif
