test:test.cpp fibonacci.hpp fibonacci_whitebox.hpp test.hpp
	g++ -g -Wall test.cpp -o test -lgtest

runtest:test
	./test
