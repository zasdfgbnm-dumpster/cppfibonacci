test:test.cpp fibonacci.hpp fibonacci_test.hpp
	g++ -g -Wall test.cpp -o test -lgtest

runtest:test
	./test
