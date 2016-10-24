#include <iostream>
using namespace std;
#include "fibonacci.hpp"
#include "fibonacci_whitebox.hpp"
#include <gtest/gtest.h>
#include <random>
#include <algorithm>
#include <limits>

using namespace std;

using whitebox = fibonacci_whitebox<int,int>;
bool verbose = false;

/** \brief an engine to do random operations and generate random Fibonacci heaps */
class random_fibonacci_heap_engine {
	int count = 0;
public:
	using fh_t = fibonacci_heap<int,int>;
	random_device r;
	default_random_engine rng;
	uniform_real_distribution<double> u01 = uniform_real_distribution<double>(0,1);
	uniform_int_distribution<int> ui01 = uniform_int_distribution<int>(0,1);
	uniform_int_distribution<int> uint = uniform_int_distribution<int>(numeric_limits<int>::min(),numeric_limits<int>::max());
	shared_ptr<fh_t> fh[2];
	vector<fh_t::node> nodes[2];

	//random_fibonacci_heap_engine():rng(r()) {}

	double pnew = 0.1;
	double pcopy = 0.5;
	double pdestroy = 0.005;
	double pmeld = 0.1;
	double premoveany = 0.5;
	double pdecreasekey = 0.5;
	size_t init_size = 400;

	/** \brief the probability that a Fibonacci heap has a given size */
	virtual double probability(size_t size) {
		double mu = 500;
		double s = 200;
		return exp(-(size-mu)*(size-mu)/(2*s*s));
	};

	void show() {
		if(fh[0])
			cout << fh[0]->dot() << endl;
		if(fh[1])
			cout << fh[1]->dot() << endl;
	}

	/** \brief make a random step */
	virtual void random_step() {
		if(verbose)
			cout << "=================================================" << endl;
		if(verbose) {
			cout << "step = " << count++;
			cout << " , size = ";
			if(fh[0]==nullptr)
				cout << "null";
			else
				cout << fh[0]->size();
			cout << ",";
			if(fh[1]==nullptr)
				cout << "null";
			else
				cout << fh[1]->size();
			cout << endl;
		}
		int i = ui01(rng);
		if(verbose)
			cout << "fh["  << i << "]";
		// initialize or nothing
		if(fh[0]==nullptr&&fh[1]==nullptr) {
			if(verbose)
				cout << ".initialize()" << endl;
			fh[i] = make_shared<fh_t>();
			while(fh[i]->size()<init_size) {
				int key = uint(rng);
				int value = uint(rng);
				if(verbose)
					cout << ".insert(" << key << "," << value << ")" << endl;
				// insert
				nodes[i].push_back(fh[i]->insert(key,value));
			}
			if(verbose)
				show();
			return;
		}
		// meld or nothing
		if(fh[0]!=nullptr && fh[1]!=nullptr && u01(rng)<pmeld) {
			if(verbose)
				cout << ".meld(fh[" << 1-i << "])" << endl;
			fh[i]->meld(*fh[1-i]);
			nodes[i].insert(nodes[i].end(),nodes[1-i].begin(),nodes[1-i].end());
			nodes[1-i].clear();
			if(verbose)
				show();
			return;
		}
		// destroy one or nothing
		if(fh[0]!=nullptr && fh[1]!=nullptr && u01(rng)<pdestroy) {
			if(verbose)
				cout << ".destroy()" << endl;
			fh[i].reset();
			nodes[i].clear();
			if(verbose)
				show();
			return;
		}
		// create new or nothing
		if(fh[i]==nullptr && u01(rng)<pnew) {
			if(u01(rng)<pcopy) {
				if(verbose)
					cout << " = fh[" << 1-i << "]" << endl;
				fh[i] = make_shared<fh_t>(*fh[1-i]);
			} else {
				if(verbose)
					cout << " = new" << endl;
				fh[i] = make_shared<fh_t>();
			}
			if(verbose)
				show();
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
		if(u01(rng)<acceptrate) {
			if(movetype<0.5) {
				int key = uint(rng);
				int value = uint(rng);
				if(verbose)
					cout << ".insert(" << key << "," << value << ")" << endl;
				// insert
				nodes[i].push_back(fh[i]->insert(key,value));
				if(verbose)
					show();
				return;
			} else {
				if((!nodes[i].empty())&&u01(rng)<premoveany) {
					// remove(node)
					size_t s = nodes[i].size();
					uniform_int_distribution<int> dist(0,s-1);
					size_t rmpos = dist(rng);
					if(verbose)
						cout << ".remove(" << nodes[i][rmpos].key() << ")" << endl;
					fh[i]->remove(nodes[i][rmpos]);
					nodes[i].erase(nodes[i].begin()+rmpos);
					if(verbose)
						show();
					return;
				} else { // remove()
					if(verbose)
						cout << ".remove()" << endl;
					fh_t::node r = fh[i]->remove();
					nodes[i].erase(remove(nodes[i].begin(),nodes[i].end(),r),nodes[i].end());
					if(verbose)
						show();
					return;
				}
			}
		} else {
			if((!nodes[i].empty())&&u01(rng)<pdecreasekey) {
				// decrease_key(node)
				size_t s = nodes[i].size();
				uniform_int_distribution<int> dist(0,s-1);
				fh_t::node n = nodes[i][dist(rng)];
				uniform_int_distribution<int> dist2(numeric_limits<int>::min(),n.key());
				int target = dist2(rng);
				if(verbose)
					cout << ".decrease_key(" << n.key() << "->" << target << endl;
				fh[i]->decrease_key(n,target);
				if(verbose)
					show();
				return;
			} else {
				int key = uint(rng);
				int value = uint(rng);
				if(verbose)
					cout << ".insert(" << key << "," << value << ")" << endl;
				// insert
				nodes[i].push_back(fh[i]->insert(key,value));
				if(verbose)
					cout << "fh[" << i << "].remove()" << endl;
				fh_t::node r = fh[i]->remove();
				nodes[i].erase(remove(nodes[i].begin(),nodes[i].end(),r),nodes[i].end());
				if(verbose)
					show();
				return;
			}
		}
	}
};

/** \brief randomly insert,remove min, meld elements and check if binomial heap
 * properties are maintained after each operation */
TEST(whitebox,binomial) {
	int steps = 100000;
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
	int steps = 100000;
	random_fibonacci_heap_engine r;
	for(int i=0;i<steps;i++) {
		r.random_step();
		int a01[] = {0,1};
		for(int i:a01) {
			if(r.fh[i]){
				whitebox::data_structure_consistency_test(*r.fh[i]);;
			}
		}
	}
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
