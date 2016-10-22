#include <iostream>
using namespace std;
#include "fibonacci.hpp"
#include "fibonacci_test.hpp"
#include <gtest/gtest.h>
#include <random>
#include <algorithm>
#include <limits>

using namespace std;

using whitebox = fibonacci_whitebox_test<int,int>;

/** \brief an engine to do random operations and generate random Fibonacci heaps */
class random_engine {
public:
	using fh_t = fibonacci_heap<int,int>;
	random_device r;
	default_random_engine rng;
	uniform_real_distribution<double> u01 = uniform_real_distribution<double>(0,1);
	uniform_int_distribution<int> ui01 = uniform_int_distribution<int>(0,1);
	uniform_int_distribution<int> uint = uniform_int_distribution<int>(numeric_limits<int>::min(),numeric_limits<int>::max());
	shared_ptr<fh_t> fh[2];
	vector<fh_t::node> nodes[2];

	random_engine():rng(r()) {}

	double pnew = 0.1;
	double pcopy = 0.5;
	double pdestroy = 0.005;
	double pmeld = 0.1;
	double premoveany = 0.5;
	double pdecreasekey = 0.5;

	/** \brief the probability that a Fibonacci heap has a given size */
	virtual double probability(size_t size) {
		double mu = 500;
		double s = 200;
		return exp(-(size-mu)*(size-mu)/(2*s*s));
	};

	/** \brief make a random step */
	virtual void random_step() {
		cout << "size = ";
		if(fh[0]==nullptr)
			cout << "null";
		else
			cout << fh[0]->size();
		cout << " , ";
		if(fh[1]==nullptr)
			cout << "null";
		else
			cout << fh[1]->size();
		cout << endl;
		// initialize or nothing
		if(fh[0]==nullptr&&fh[1]==nullptr) {
			cout << "initialize" << endl;
			int i = ui01(rng);
			fh[i] = make_shared<fh_t>();
			return;
		}
		// meld or nothing
		if(fh[0]!=nullptr && fh[1]!=nullptr && u01(rng)<pmeld) {
			cout << "meld" << endl;
			int i = ui01(rng);
			fh[i]->meld(*fh[1-i]);
			nodes[i].insert(nodes[i].end(),nodes[1-i].begin(),nodes[1-i].end());
			nodes[1-i].clear();
			return;
		}
		// destroy one or nothing
		if(fh[0]!=nullptr && fh[1]!=nullptr && u01(rng)<pdestroy) {
			cout << "destroy" << endl;
			int i = ui01(rng);
			fh[i].reset();
			nodes[i].clear();
			return;
		}
		// create new or nothing
		int i = ui01(rng);
		if(fh[i]==nullptr && u01(rng)<pnew) {
			cout << "create new" << endl;
			if(u01(rng)<pcopy)
				fh[i] = make_shared<fh_t>(*fh[1-i]);
			else
				fh[i] = make_shared<fh_t>();
			return;
		}
		if(fh[i]==nullptr)
			i = 1-i;
		// insert, remove, remove(node), or decrease_key
		double movetype = u01(rng);
		if(fh[i]->size()==0)
			movetype = 0;
		double acceptrate = 0;
		if(movetype<0.5) // insert
			acceptrate = probability(fh[i]->size()+1)/probability(fh[i]->size());
		else
			acceptrate = probability(fh[i]->size()-1)/probability(fh[i]->size());
		cout << "accept rate = " << acceptrate << endl;
		if(u01(rng)<acceptrate) {
			if(movetype<0.5) {
				cout << "insert" << endl;
				// insert
				nodes[i].push_back(fh[i]->insert(uint(rng),uint(rng)));
				return;
			} else {
				if((!nodes[i].empty())&&u01(rng)<premoveany) {
					cout << "remove(node)" << endl;
					// remove(node)
					size_t s = nodes[i].size();
					uniform_int_distribution<int> dist(0,s-1);
					size_t rmpos = dist(rng);
					cout << "rmpos = " << rmpos << endl;
					fh[i]->remove(nodes[i][rmpos]);
					cout << "done remove" << endl;
					nodes[i].erase(nodes[i].begin()+rmpos);
					return;
				} else { // remove()
					cout << "remove" << endl;
					fh_t::node r = fh[i]->remove();
					nodes[i].erase(remove(nodes[i].begin(),nodes[i].end(),r),nodes[i].end());
					return;
				}
			}
		} else {
			if((!nodes[i].empty())&&u01(rng)<pdecreasekey) {
				cout << "decrease key" << endl;
				// decrease_key(node)
				size_t s = nodes[i].size();
				uniform_int_distribution<int> dist(0,s-1);
				fh_t::node n = nodes[i][dist(rng)];
				uniform_int_distribution<int> dist2(numeric_limits<int>::min(),n.key());
				fh[i]->decrease_key(n,dist2(rng));
				return;
			} else {
				cout << "insert & remove min" << endl;
				// insert then remove min
				nodes[i].push_back(fh[i]->insert(uint(rng),uint(rng)));
				fh_t::node r = fh[i]->remove();
				nodes[i].erase(remove(nodes[i].begin(),nodes[i].end(),r),nodes[i].end());
				return;
			}
		}
	}
};

/** \brief a simple test that create easy Fibonacci heaps and do operations on it */
TEST(whitebox,a_simple_example) {
	fibonacci_heap<int,int> fh1({
		{1,2},
		{3,4},
		{5,6}
	});
	whitebox::data_structure_consistency_test(fh1);
	fibonacci_heap<int,int> fh2({
		{1,2},
		{3,4},
		{5,6}
	});
	whitebox::data_structure_consistency_test(fh2);
	fh1.meld(fh2);
	whitebox::data_structure_consistency_test(fh1);
	whitebox::data_structure_consistency_test(fh2);
	fibonacci_heap<int,int> fh3;
	whitebox::data_structure_consistency_test(fh3);
	fh1.meld(fh3);
	whitebox::data_structure_consistency_test(fh1);
	whitebox::data_structure_consistency_test(fh3);
	cout << "fh1 size " << fh1.size() << endl;
	fh1.remove();
	whitebox::data_structure_consistency_test(fh1);
	cout << "fh1 size " << fh1.size() << endl;
	fh1.remove();
	whitebox::data_structure_consistency_test(fh1);
	cout << "fh1 size " << fh1.size() << endl;
	fh1.remove();
	whitebox::data_structure_consistency_test(fh1);
	cout << "fh1 size " << fh1.size() << endl;
	fh1.remove();
	whitebox::data_structure_consistency_test(fh1);
	cout << "fh1 size " << fh1.size() << endl;
}

/** \brief run random operations and check consistency after each operation */
TEST(whitebox,consistency) {
	// int steps = 10000;
	// random_engine r;
	// for(int i=0;i<steps;i++) {
	// 	cout << "step = " << i << endl;
	// 	r.random_step();
	// 	int a01[] = {0,1};
	// 	for(int i:a01) {
	// 		if(r.fh[i]) fibonacci_test::data_structure_consistency_test(*r.fh[i]);
	// 	}
	// }
}

/** \brief randomly insert,remove min, meld elements and check if binomial heap
 * properties are maintained after each operation */
TEST(whitebox,binomial) {

}

/** \brief generate a random Fibonacci heap and test copy and move constructor and assignment operator */
TEST(whitebox,copy_move) {

}

/** \brief generate a random Fibonacci heap and test if this heap destroy correctly */
TEST(whitebox,destroy) {}

/** \brief randomly insert, remove, change or merge some elements and see if
 * Fibonacci heap can generate a sorted list of remaining elements*/
TEST(blackbox,sort) {}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
