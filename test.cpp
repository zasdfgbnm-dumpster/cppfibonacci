#include <iostream>
using namespace std;

#include "test.hpp"
#include "fibonacci_whitebox.hpp"

/** \brief randomly insert,remove min, meld elements and check if binomial heap
 * properties are maintained after each operation */
TEST(whitebox,binomial) {
	int steps = 1000000;
	random_fibonacci_heap_engine r;
	r.pdecreasekey = 0;
	r.premoveany = 0;
	for(int i=0;i<steps;i++) {
		r.random_step();
		int a01[] = {0,1};
		for(int i:a01) {
			if(r.fh[i]){
				ASSERT_TRUE(whitebox::is_binomial(*r.fh[i]));
			}
		}
	}
}

/** \brief run random operations and check consistency after each operation */
TEST(whitebox,consistency) {
	int steps = 1000000;
	random_fibonacci_heap_engine r;
	for(int i=0;i<steps;i++) {
		r.random_step();
		int a01[] = {0,1};
		for(int i:a01) {
			if(r.fh[i]){
				whitebox::data_structure_consistency_test(*r.fh[i]);
			}
		}
	}
}

/** \brief generate a random Fibonacci heap and test copy and move constructor and assignment operator */
TEST(whitebox,copy_move) {
	int ntests = 1000;
	int steps = 100;
	for(int test_idx=0;test_idx<ntests;test_idx++) {
		random_fibonacci_heap_engine r;
		for(int i=0;i<steps;i++)
			r.random_step();
		int a01[] = {0,1};
		for(int i:a01) {
			if(r.fh[i]){
				ASSERT_TRUE(whitebox::copy_move_test(*r.fh[i]));
			}
		}
	}
}

/** \brief generate a random Fibonacci heap and test if this heap destroy correctly */
TEST(whitebox,destroy) {
	int ntests = 1000;
	int steps = 100;
	double pthrow = 0.8;
	for(int test_idx=0;test_idx<ntests;test_idx++) {
		random_fibonacci_heap_engine r;
		for(int i=0;i<steps;i++)
			r.random_step();
		int a01[] = {0,1};
		for(int i:a01) {
			if(r.fh[i]){
				// random throw items in r.nodes
				auto it = std::remove_if(r.nodes[i].begin(), r.nodes[i].end(), [&](random_fibonacci_heap_engine::fh_t::node&)->bool{ return r.u01(r.rng)<pthrow; });
				r.nodes[i].erase(it, r.nodes[i].end());
				ASSERT_TRUE(whitebox::destroy_and_test(r.fh[i]));
			}
		}
	}
}

/** \brief randomly insert, remove, change or merge some elements and see if
 * Fibonacci heap can generate a sorted list of remaining elements*/
TEST(blackbox,sort) {}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
