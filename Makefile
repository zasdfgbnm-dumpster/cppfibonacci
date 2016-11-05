test:test.cpp fibonacci.hpp fibonacci_whitebox.hpp test.hpp
	g++ -g -Wall test.cpp -o test -lgtest

example:example.cpp fibonacci.hpp
	g++ -O2 -Wall example.cpp -o example -lgtest

.PHONY:docs
docs:
	rm -rf docs
	doxygen Doxyfile

runtest:test
	./test
