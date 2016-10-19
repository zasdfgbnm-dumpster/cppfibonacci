/** @example example.cpp
 *
 * This is an example on the usage of fibonacci_heap.
 *
 * The code can be found at file sample.cpp
 */
#include "fibonacci.hpp"
#include <string>
#include <iostream>

using namespace std;

int main() {

	// initialize an empty Fibonacci heap using int as key and string as data
	fibonacci_heap<int,string> fh1;

	// initialize a Fibonacci heap of the same type from a array of key data pairs
	fibonacci_heap<int,string> fh2 = { {3,"three"}, {4,"four"} };

	// insert two elements to each Fibonacci heap
	fh1.insert(10,"the first element in fh1");
	fh1.insert(1,"the second element in fh1");
	auto node3 = fh2.insert(2,"the first element in fh2");
	auto node4 = fh2.insert(20,"the second element in fh2");

	// print the top element of each Fibonacci heap
	cout << "top element of fh1 has a key: " << fh1.top().key() << endl;
	cout << "top element of fh1 has a data: " << fh1.top().data() << endl;
	cout << "top element of fh2 has a key: " << fh2.top().key() << endl;
	cout << "top element of fh2 has a data: " << fh1.top().data() << endl;

	// change the data of the top element of fh1
	fh1.top().data() = "I'm the top element of fh1!";

	// print the size of fh1 and fh2
	cout << "size of fh1: " << fh1.size() << endl;
	cout << "size of fh2: " << fh2.size() << endl;

	// meld fh2 into fh1
	fh1.meld(fh2);

	// print the size of fh1 and fh2 after meld
	cout << "size of fh1 after meld is: " << fh1.size() << endl;
	cout << "size of fh2 after meld is: " << fh2.size() << endl;

	// decrease the key of node4
	fh1.decrease_key(node4,0);

	// change the data of node4
	node4.data() = "Am I the new top of fh1 now?";

	// remove node3 from fh1
	fh1.remove(node3);

	// remove the top element from fh1
	fh1.remove();

	// insert node3 back to fh1
	fh1.insert(node3);

}
