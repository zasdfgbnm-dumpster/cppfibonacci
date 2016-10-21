#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <functional>
#include <tuple>
#define FIBONACCI_HEAP_TEST_FRIEND fibonacci_test
#include "fibonacci.hpp"

using namespace std;

/** \brief container than contains implementation of tests */
class fibonacci_test {

	// this class is only a container, creating an object of this class is not allowed
	fibonacci_test();

public:

	/** \brief the class containing functions to test fibonacci_heap
	 *
	 * @param K the type for keys
	 * @param T the type for data
	 * @param Compare the class that define the order of keys, with default value the "<".
	 */
	template <typename K, typename T, typename Compare=std::less<K>>
	class fibonacci_test_basic {
	public:

		// useful types
		using fh_t = fibonacci_heap<T,K,Compare>;
		using sn_t = typename fibonacci_heap<T,K,Compare>::internal_structure;
		using dn_t = typename fibonacci_heap<T,K,Compare>::internal_data;

	private:

		/* This class contains only static methods, therefore creating an object of
		 * this class is not allowed */
		fibonacci_test_basic();

		/** \brief recursively run consistency check on each internal_structure node
		 *
		 * @param node the node to be checked
		 *
		 * @param parent the value of parent pointer that this node is supposed to have
		 *
		 * @param head used to denote what phase this function is doing. If head==nullptr,
		 * then this function has just started at a doubly linked list; if head!=nullptr,
		 * then this function is checking inside the doubly linked list and head is the first
		 * element of the doubly linked list.
		 *
		 * @return number of elements inside the doubly linked list checked starting at from
		 * this point including the parameter "node" itself
		 */
		static int _data_structure_consistency_check(const fh_t &fh,shared_ptr<sn_t> node, shared_ptr<sn_t> parent, shared_ptr<sn_t> head) {
			// if sibling check done
			if (node==head) return 0;
			// if this is the beginning of sibling check
			if (head==nullptr) head = node;
			// check min-tree property
			if(parent) EXPECT_TRUE(Compare()(parent->data->key,node->data->key));
			// check fh pointer
			EXPECT_EQ(node->fh,&fh);
			// check parent and sibling pointers
			EXPECT_EQ(node->parent,parent);
			EXPECT_EQ(node->left_sibling->right_sibling,node);
			EXPECT_EQ(node->right_sibling->left_sibling,node);
			// check structure and data pointers
			EXPECT_EQ(node->data->structure,node);
			// recursively run test on child and check degree
			size_t calculated_degree = _data_structure_consistency_check(fh,node->child, node, nullptr);
			EXPECT_EQ(node->degree,calculated_degree);
			// recursively run test on siblings
			return _data_structure_consistency_check(fh,node->right_sibling, parent, head)+1;
		}

		/** \brief run binomial property test on a tree rooted at root
		 *
		 * @param root the root of the tree to be tested
		 */
		static void _expect_binomial(shared_ptr<sn_t> root) {
			size_t degree = root->degree;
			if(degree == 0) {
				EXPECT_EQ(root->child,nullptr);
				return;
			}
			bool children_degrees[degree];
			for(bool &i:children_degrees)
				i = false;
			shared_ptr<sn_t> p=root->child;
			do {
				_expect_binomial(p);
				EXPECT_FALSE(children_degrees[p->degree]);
				children_degrees[p->degree] = true;
				p=p->right_sibling;
			} while(p!=root->child);
			for(bool i:children_degrees)
				EXPECT_TRUE(i);
		}

	public:

		/** \brief check the consistency of the forest of min trees maintained inside fibonacci_heap
		*
		* The following things are checked:
		* * parent pointer
		* * sibling pointers
		* * degrees
		* * data and structure pointers
		* * min-tree property
		*/
		static void data_structure_consistency_check(const fh_t &fh) {
			test_structure_node(fh.min,nullptr,nullptr);
			// check if the min pointer really point to the main
			for(shared_ptr<sn_t> p=fh.min->right_sibling; p!=fh.min; p=p->right_sibling) {
				EXPECT_LE(fh.min->data->key,p->data->key);
			}
		}

		/** \brief test whether the cleanup procedure of a Fibonacci heap works well during descruction
		 *
		 * @param fhptr the pointer pointing to the Fibonacci heap to be destroyed
		 */
		void destroy_and_test(shared_ptr<fh_t> &fhptr) {
			vector<weak_ptr<sn_t>> sn_clean_list;
			vector<weak_ptr<dn_t>> dn_clean_list;
			vector<tuple<weak_ptr<dn_t>,size_t>> dn_keep_list;
			// dump out weak pointers to all the structure and data nodes
			function<void (weak_ptr<sn_t>,weak_ptr<sn_t>)> traverse = [&](weak_ptr<sn_t> node,weak_ptr<sn_t> head) {
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
			// test if things are cleaned up well
			for(weak_ptr<sn_t> &i : sn_clean_list) {
				EXPECT_TRUE(i.expired());
			}
			for(weak_ptr<dn_t> &i : dn_clean_list) {
				EXPECT_TRUE(i.expired());
			}
			for(tuple<weak_ptr<dn_t>,size_t> &i : dn_keep_list) {
				EXPECT_EQ(get<0>(i).use_count(),get<1>(i)-1);
				EXPECT_EQ(get<0>(i)->structure,nullptr);
			}
		}

		/** \brief test whether the fibonacci_heap object is copied correctly */
		static void copy_test(const fh_t &fh) {
			fh_t fh2(fh);
			fh_t fh3 = fh;
			fh_t fh4;
			fh4 = fh;
			// check data structure consistency
			data_structure_consistency_check(fh);
			data_structure_consistency_check(fh2);
			data_structure_consistency_check(fh3);
			data_structure_consistency_check(fh4);
		}

		/** \brief expect that this fibonacci_heap must be a binomial heap
		 *
		 * In the case that only insert, meld and remove min is performed,
		 * a Fibonacci heap should be exactly the same as a binomial heap.
		 * This method is designed to be called in this case to expect that
		 * the Fibonacci heap is actually a binomial heap.
		 */
		static void expect_binomial(const fh_t &fh) {
			shared_ptr<sn_t> p=fh.min->right_sibling;
			do {
				_expect_binomial(p);
				p=p->right_sibling;
			} while(p!=fh.min);
		}
	};
};
