
CXX=g++
CXX=clang++

.PHONY: test test_variant

test: test_variant

test_variant: test_variant.bin
	./test_variant.bin

test_variant.bin: test.cpp variant.hpp
	$(CXX) -std=c++14 test.cpp -o test_variant.bin

clean:
	rm test_variant.bin
