test:test.cpp fibonacci.hpp fibonacci_test.hpp
	g++ -Wall test.cpp -o test -lgtest

runtest:test
	./test
