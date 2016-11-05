#include <iostream>
using namespace std;

#include "test.hpp"
#include "fibonacci_whitebox.hpp"
#include <map>

/** \brief randomly insert,remove min, meld elements and check if binomial heap
 * properties are maintained after each operation */
TEST(whitebox,binomial) {
	using eng_t = random_fibonacci_heap_engine<int>;
	int steps = 1000000;
	eng_t r;
	r.pdecreasekey = 0;
	r.premoveany = 0;
	for(int i=0;i<steps;i++) {
		r.random_step();
		int a01[] = {0,1};
		for(int i:a01) {
			if(r.fh[i]){
				ASSERT_TRUE(eng_t::whitebox::is_binomial(*r.fh[i]));
			}
		}
	}
}

/** \brief run random operations and check consistency after each operation */
TEST(whitebox,consistency) {
	using eng_t = random_fibonacci_heap_engine<int>;
	int steps = 1000000;
	eng_t r;
	for(int i=0;i<steps;i++) {
		r.random_step();
		int a01[] = {0,1};
		for(int i:a01) {
			if(r.fh[i]){
				eng_t::whitebox::data_structure_consistency_test(*r.fh[i]);
			}
		}
	}
}

/** \brief generate a random Fibonacci heap and test copy and move constructor and assignment operator */
TEST(whitebox,copy_move) {
	using eng_t = random_fibonacci_heap_engine<int>;
	int ntests = 1000;
	int steps = 100;
	for(int test_idx=0;test_idx<ntests;test_idx++) {
		eng_t r;
		for(int i=0;i<steps;i++)
			r.random_step();
		int a01[] = {0,1};
		for(int i:a01) {
			if(r.fh[i]){
				ASSERT_TRUE(eng_t::whitebox::copy_move_test(*r.fh[i]));
			}
		}
	}
}

/** \brief generate a random Fibonacci heap and test if this heap destroy correctly */
TEST(whitebox,destroy) {
	using eng_t = random_fibonacci_heap_engine<int>;
	int ntests = 10000;
	int steps = 100;
	double pthrow = 0.8;
	for(int test_idx=0;test_idx<ntests;test_idx++) {
		eng_t r;
		for(int i=0;i<steps;i++)
			r.random_step();
		int a01[] = {0,1};
		for(int i:a01) {
			if(r.fh[i]){
				// random throw items in r.nodes
				auto it = std::remove_if(r.nodes[i].begin(), r.nodes[i].end(), [&](random_fibonacci_heap_engine<int>::fh_t::node&)->bool{ return r.u01(r.rng)<pthrow; });
				r.nodes[i].erase(it, r.nodes[i].end());
				ASSERT_TRUE(eng_t::whitebox::destroy_and_test(r.fh[i]));
			}
		}
	}
}

/** \brief helper class to count number of instances of a value */
class instance_count {
public:
	static std::map<int,size_t> n;
	int value;
	instance_count(int val):value(val) {
		if(n.count(val)>0)
			n[val]++;
		else
			n[val] = 1;
	}
	instance_count(const instance_count &r):value(r.value) {
		n[r.value]++;
	}
	instance_count &operator=(const instance_count &r) {
		n[value]--;
		value = r.value;
		n[value]++;
		return *this;
	}
	~instance_count() {
		n[value]--;
	}
};
std::map<int,size_t> instance_count::n;

/** \brief test for memory leakage */
TEST(blackbox,leakage) {
	using eng_t = random_fibonacci_heap_engine<instance_count>;
	int ntests = 10000;
	int steps = 100;
	for(int test_idx=0;test_idx<ntests;test_idx++) {
		eng_t r;
		for(int i=0;i<steps;i++)
			r.random_step();
		// clear stored data in engine
		int a01[] = {0,1};
		for(int i:a01) {
			if(r.fh[i]) {
				r.nodes[i].clear();
				r.fh[i].reset();
			}
		}
		// test if all instance counts of all value is zero
		for(auto &p : instance_count::n) {
			if(p.second!=0)
				cout << "value = " << p.first << endl;
			ASSERT_EQ(p.second,0);
		}
		instance_count::n.clear();
	}
}

/** \brief randomly insert, remove, change or merge some elements and see if
 * Fibonacci heap can generate a sorted list of remaining elements*/
TEST(blackbox,sort) {}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
