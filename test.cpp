#include "variant.hpp"
#include <cassert>
#include <iostream>
using namespace std;
using namespace xx;

struct A { std::string v; A(std::string s): v{s} {} };
struct B { int x; };
struct C { int y = 55; std::string v; C() {} C(int y): y(y) {} };

using AB = variant<A, B>;
using BC = variant<B, C>;
using CA = variant<C, A>;
using ABC = variant<A, B, C>;

template <typename T>
void eat(T v) {}

void test_variant()
{
	auto a = A{"aaa"};
	auto b = AB{a};
	auto c = ABC{b};
	auto d = AB{c};
	auto e = CA{d};
	auto f = A(e);
	//auto g = ABC{5};

	assert(b.is<A>());
	assert(b.get<A>().v == "aaa");
	assert(c.is<A>());
	assert(c.get<A>().v == "aaa");
	assert(d.is<A>());
	assert(d.get<A>().v == "aaa");
	assert(e.is<A>());
	assert(e.get<A>().v == "aaa");
	assert(f.v == "aaa");

	c = b;

	assert(b.is<A>());
	assert(b.get<A>().v == "aaa");
	assert(c.is<A>());
	assert(c.get<A>().v == "aaa");
	assert(d.is<A>());
	assert(d.get<A>().v == "aaa");
	assert(e.is<A>());
	assert(e.get<A>().v == "aaa");
	assert(f.v == "aaa");

	//b = c;

	if (false) {
		auto z = BC{c}; // would be undefined behavior
	}

	//eat<A>(e);
	eat<CA>(e);
	eat<CA>(a);
	eat<ABC>(b);
	eat<AB>(b);
	//eat<BC>(b); // wouldn't compile - need explicit conversion
	if (false) {
		eat<BC>(BC{b}); // would be undefined behavior
	}
	eat<CA>(CA{b});
	auto x = A(c);
}

void test_in_place()
{
	auto x = ABC{in_place<C>};
	assert(x.is<C>());
	assert(x.get<C>().y == 55);

	auto z = ABC{in_place<C>, 77};
	assert(z.is<C>());
	assert(z.get<C>().y == 77);
}

void test_example1()
{
	using IntOrFloat = xx::variant<int, float>;

	auto v = IntOrFloat{5};
	if (rand()%2)
		v = 0.5f;

	if (v.is<int>())
		assert(v.get<int>() == 5);
	else
		assert(v.get<float>() == 0.5);
}

struct Select { Select(){} }; struct Insert { Insert(){} }; struct Delete { Delete(){} };
using ROQuery = Select;
using RWQuery = xx::variant<Insert, Delete>;
using AnyQuery = xx::variant<Select, Insert, Delete>;

auto parse_query(std::string) -> AnyQuery { return Select{}; }
void apply_ro_query(ROQuery q) {}
void apply_rw_query(RWQuery q) {assert(q.is<Insert>() || q.is<Delete>());}

void test_example2()
{
	auto q = parse_query("SELECT *;");
	assert(q.is<Select>());
	auto s = Select(q);
}

void test_example3()
{
	{
		auto q = parse_query("SELECT *;");
		// apply_ro_query(q); // ERROR - Select(AnyQuery) is explicit
		// apply_rw_query(q); // ERROR - RWQuery(AnyQuery) is explicit
		if (q.is<RWQuery>())
			assert(false);
		else
			apply_ro_query(ROQuery(q));
	}

	{
		auto q = AnyQuery{Insert{}};
		if (q.is<RWQuery>())
			apply_rw_query(RWQuery(q));
		else
			assert(false);
	}
}

int main()
{
	test_variant();
	test_in_place();
	test_example1();
	test_example2();
	test_example3();
	return 0;
}
