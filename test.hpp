#include <iostream>
#include "fibonacci.hpp"
#include "fibonacci_whitebox.hpp"
#include <gtest/gtest.h>
#include <random>
#include <algorithm>
#include <limits>

using namespace std;

/** \brief an engine to do random operations and generate random Fibonacci heaps */
template <typename val_t>
class random_fibonacci_heap_engine {
	int count = 0;
public:
	using whitebox = fibonacci_whitebox<int,val_t>;
	bool verbose = false;
	bool showdot = false;
	using fh_t = fibonacci_heap<int,val_t>;
	random_device r;
	default_random_engine rng;
	uniform_real_distribution<double> u01 = uniform_real_distribution<double>(0,1);
	uniform_int_distribution<int> ui01 = uniform_int_distribution<int>(0,1);
	uniform_int_distribution<int> uint = uniform_int_distribution<int>(numeric_limits<int>::min(),numeric_limits<int>::max());
	shared_ptr<fh_t> fh[2];
	vector<typename fh_t::node> nodes[2];

	random_fibonacci_heap_engine():rng(r()) {}

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

	/** \brief print out the DOT language of the two Fibonacci heap */
	virtual void show() {
		if(showdot) {
			if(fh[0])
				cout << fh[0]->dot() << endl;
			if(fh[1])
				cout << fh[1]->dot() << endl;
		}
	}

	/** \brief print the header for each step */
	virtual void header() {
		if(verbose) {
			cout << "=================================================" << endl;
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
	}

	/** \brief initialize a Fibonacci heap */
	virtual void initialize(int i) {
		if(verbose)
			cout << "fh["  << i << "]" << ".initialize()" << endl;
		fh[i] = make_shared<fh_t>();
		while(fh[i]->size()<init_size) {
			insert(i);
		}
	}

	/** \brief meld two Fibonacci heap */
	virtual void meld(int i) {
		if(verbose)
			cout << "fh["  << i << "]" << ".meld(fh[" << 1-i << "])" << endl;
		fh[i]->meld(*fh[1-i]);
		nodes[i].insert(nodes[i].end(),nodes[1-i].begin(),nodes[1-i].end());
		nodes[1-i].clear();
	}

	/** \brief destroy a Fibonacci heap */
	virtual void destroy(int i) {
		if(verbose)
			cout << "fh["  << i << "]" << ".destroy()" << endl;
		fh[i].reset();
		nodes[i].clear();
	}

	/** \brief create a new Fibonacci heap */
	virtual void create_new(int i) {
		if(verbose)
			cout << "fh["  << i << "]" << " = new" << endl;
		fh[i] = make_shared<fh_t>();
	}

	/** \brief copy a Fibonacci heap*/
	virtual void copy(int i) {
		if(verbose)
			cout << "fh["  << i << "]" << " = fh[" << 1-i << "]" << endl;
		fh[i] = make_shared<fh_t>(*fh[1-i]);
	}

	/** \brief insert an element to a Fibonacci heap */
	virtual void insert(int i) {
		int key = uint(rng);
		int value = uint(rng);
		if(verbose)
			cout << "fh["  << i << "]" << ".insert(" << key << "," << value << ")" << endl;
		// insert
		nodes[i].push_back(fh[i]->insert(key,value));
	}

	/** \brief remove a given element */
	virtual void remove_any(int i) {
		size_t s = nodes[i].size();
		uniform_int_distribution<int> dist(0,s-1);
		size_t rmpos = dist(rng);
		if(verbose)
			cout << "fh["  << i << "]" << ".remove(" << nodes[i][rmpos].key() << ")" << endl;
		fh[i]->remove(nodes[i][rmpos]);
		nodes[i].erase(nodes[i].begin()+rmpos);
	}

	/** \brief remove min */
	virtual void remove_min(int i) {
		if(verbose)
			cout << "fh["  << i << "]" << ".remove()";
		typename fh_t::node r = fh[i]->remove();
		if(verbose)
			cout << " , min.key = " << r.key() << endl;
		nodes[i].erase(remove(nodes[i].begin(),nodes[i].end(),r),nodes[i].end());
	}

	/** \brief decrease key */
	virtual void decrease_key(int i) {
		size_t s = nodes[i].size();
		uniform_int_distribution<int> dist(0,s-1);
		typename fh_t::node n = nodes[i][dist(rng)];
		uniform_int_distribution<int> dist2(numeric_limits<int>::min(),n.key());
		int target = dist2(rng);
		if(verbose)
			cout << "fh["  << i << "]" << ".decrease_key(" << n.key() << "->" << target << ")" << endl;
		fh[i]->decrease_key(n,target);
	}

	#define RUN_SHOW_RETURN(f) {f(i);show();return;}
	/** \brief make a random step */
	virtual void random_step() {
		header();
		int i = ui01(rng);
		// initialize or nothing
		if(fh[0]==nullptr&&fh[1]==nullptr)
			RUN_SHOW_RETURN(initialize)
		// meld or nothing
		if(fh[0]!=nullptr && fh[1]!=nullptr && u01(rng)<pmeld)
			RUN_SHOW_RETURN(meld)
		// destroy one or nothing
		if(fh[0]!=nullptr && fh[1]!=nullptr && u01(rng)<pdestroy)
			RUN_SHOW_RETURN(destroy)
		// create new or nothing
		if(fh[i]==nullptr && u01(rng)<pnew) {
			if(u01(rng)<pcopy)
				RUN_SHOW_RETURN(copy)
			else
				RUN_SHOW_RETURN(create_new)
		}
		if(fh[i]==nullptr)
			i = 1-i;
		// insert, remove, remove(node), or decrease_key
		double movetype = fh[i]->size()?u01(rng):0;
		double acceptrate = 0;
		int new_size = fh[i]->size() + movetype<0.5?1:-1;
		acceptrate = probability(new_size)/probability(fh[i]->size());
		if(u01(rng)<acceptrate) {
			if(movetype<0.5)
				RUN_SHOW_RETURN(insert)
			else {
				if((!nodes[i].empty())&&u01(rng)<premoveany)
					RUN_SHOW_RETURN(remove_any)
				else
					RUN_SHOW_RETURN(remove_min)
			}
		} else {
			if((!nodes[i].empty())&&u01(rng)<pdecreasekey)
				RUN_SHOW_RETURN(decrease_key)
			else {
				insert(i);
				remove_min(i);
				show();
			}
		}
	}
	#undef RUN_SHOW_RETURN
};
