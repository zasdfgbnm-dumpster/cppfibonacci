#ifndef _CPP_FIBONACCI_TEST_
#define _CPP_FIBONACCI_TEST_

#include <memory>
#include <vector>
#include <tuple>
#include <functional>
#include <gtest/gtest.h>

#include "fibonacci.hpp"

/** \brief container than contains implementation of tests */
template <typename K, typename T, typename Compare>
class fibonacci_whitebox_test {

	// this class is only a container, creating an object of this class is not allowed
	fibonacci_whitebox_test();

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
	static void _data_structure_consistency_test(ss_t node, ss_t parent, ss_t head, size_t &degree) {
		// if node is null, head must also be null
		if(!node)
			ASSERT_FALSE(head);
		// if sibling test done
		if (node==head) {
			degree = 0;
			return;
		}
		// if this is the beginning of sibling test
		if (head==nullptr) head = node;
		// test min-tree property
		if(parent) ASSERT_LE(parent->data->key,node->data->key);
		// test parent and sibling pointers
		ASSERT_EQ(node->parent.lock(),parent);
		ASSERT_FALSE(node->left_sibling.expired());
		ASSERT_EQ(node->left_sibling.lock()->right_sibling,node);
		ASSERT_EQ(node->right_sibling->left_sibling.lock(),node);
		// test structure and data pointers
		ASSERT_EQ(node->data->structure.lock(),node);
		// recursively run test on child and test degree
		size_t calculated_degree;
		_data_structure_consistency_test(node->child, node, nullptr,calculated_degree);
		ASSERT_EQ(node->degree,calculated_degree);
		// recursively run test on siblings
		_data_structure_consistency_test(node->right_sibling, parent, head,degree);
		degree++;
	}

	/** \brief run binomial property test on a tree rooted at root
	 *
	 * @param root the root of the tree to be tested
	 */
	static void _expect_binomial(ss_t root) {
		size_t degree = root->degree;
		if(degree == 0) {
			ASSERT_EQ(root->child,nullptr);
			return;
		}
		bool children_degrees[degree];
		for(bool &i:children_degrees)
			i = false;
		ss_t p=root->child;
		do {
			_expect_binomial(p);
			ASSERT_FALSE(children_degrees[p->degree]);
			children_degrees[p->degree] = true;
			p=p->right_sibling;
		} while(p!=root->child);
		for(bool i:children_degrees)
			ASSERT_TRUE(i);
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
	static void element_in(ss_t e, const fh_t &fh) {
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
		ASSERT_TRUE(found);
	}

	/** \brief test if all the forests given have the same structure */
	static void expect_same_tree_structure(std::vector<ss_t> nodes) {
		// if pointers are null, return
		bool allnull = true;
		bool anynull = false;
		for(ss_t &i:nodes) {
			allnull = allnull && (i==nullptr);
			anynull = anynull || (i==nullptr);
		}
		ASSERT_EQ(allnull,anynull);
		if(anynull) return;
		// traverse sibling list
		std::vector<ss_t> ps = nodes;
		bool done = false;
		do {
			// check if keys and values of different p are the same
			for(ss_t &p1:ps){
				for(ss_t &p2:ps){
					ASSERT_EQ(p1->data->key,p2->data->key);
					ASSERT_EQ(p1->data->data,p2->data->data);
				}
				std::vector<ss_t> children = nodes;
				for(ss_t &i:children) {
					i = i->child;
				}
				expect_same_tree_structure(children);
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
			ASSERT_EQ(anydone,alldone);
			done = anydone;
		} while(!done);
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
		size_t unused;
		_data_structure_consistency_test(fh.min,nullptr,nullptr,unused);
		// test for 6
		if(fh.min) {
			for(ss_t p=fh.min->right_sibling; p!=fh.min; p=p->right_sibling) {
				ASSERT_TRUE(p);
				ASSERT_LE(fh.min->data->key,p->data->key);
			}
		}
		// test for 7
		ASSERT_EQ(fh._size,count_nodes(fh.min));
		// test for 8
		if(fh.min) {
			size_t max_deg = fh.max_degree();
			ss_t p = fh.min;
			do {
				ASSERT_TRUE(p);
				ASSERT_LE(p->degree,max_deg);
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
	static void copy_move_test(const fh_t &fh) {
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
		ASSERT_NE(fh.min, fh2.min);
		ASSERT_NE(fh.min, fh3.min);
		ASSERT_NE(fh.min, fh4.min);
		ASSERT_NE(fh.min, fh5.min);
		ASSERT_NE(fh.min, fh6.min);
		// test for 3
		expect_same_tree_structure({fh.min,fh2.min,fh3.min,fh4.min,fh5.min,fh6.min});
	}

	/** \brief expect that this fibonacci_heap must be a binomial heap
	 *
	 * In the case that only insert, meld and remove min is performed,
	 * a Fibonacci heap should be exactly the same as a binomial heap.
	 * This method is designed to be called in this case to expect that
	 * the Fibonacci heap is actually a binomial heap.
	 */
	static void expect_binomial(const fh_t &fh) {
		ss_t p=fh.min;
		do {
			_expect_binomial(p);
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
	void destroy_and_test(std::shared_ptr<fh_t> &fhptr) {
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
		ASSERT_TRUE(fhptr.unique());
		fhptr.reset();
		// test for 1
		for(ws_t &i : sn_clean_list) {
			ASSERT_TRUE(i.expired());
		}
		// test for 2
		for(wd_t &i : dn_clean_list) {
			ASSERT_TRUE(i.expired());
		}
		// test for 3
		for(std::tuple<wd_t,size_t> &i : dn_keep_list) {
			ASSERT_EQ(std::get<0>(i).use_count(),std::get<1>(i)-1);
			ASSERT_TRUE(std::get<0>(i).lock()->structure.expired());
		}
	}
};

#endif
