test:test.cpp fibonacci.hpp
	g++ -Wall test.cpp -o test -lgtest

runtest:test
	./test

rundebug:debug.cpp fibonacci.hpp
	g++ -Wall debug.cpp -o debug -lgtest
	./debug
