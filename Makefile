
CXX=g++
CXX=clang++
CXXFLAGS=-Wall -Werror -Wextra -pedantic -Wno-char-subscripts -Wno-sign-compare -Wno-unknown-pragmas -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -g -std=c++17 -fdiagnostics-color -Wl,-E

.PHONY: test test_variant

test: test_variant

test_variant: test_variant.bin
	./test_variant.bin

test_variant.bin: test.cpp variant.hpp
	$(CXX) $(CXXFLAGS) test.cpp -o test_variant.bin

clean:
	rm test_variant.bin
