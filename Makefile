test:test.cpp fibonacci.hpp fibonacci_whitebox.hpp
	g++ -g -Wall test.cpp -o test -lgtest

runtest:test
	./test
