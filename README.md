# variantxx
Clean and simple implementation of a discriminated union in C++14.

```c++
class xx::variant<T...>;
```

## What's that?
A discriminated union is an object that contains exactly one object of one of specified classes (ie. a union) and knows its type.

## Examples
```c++
using IntOrFloat = xx::variant<int, float>;

auto v = IntOrFloat{5};
if (...)
    v = 0.5f;

if (v.is<int>())
    std::cout << v.get<int>() << std::endl;
else
    std::cout << v.get<float>() << std::endl;
```

```c++
class Select; class Insert; class Delete;
using AnyQuery = xx::variant<Select, Insert, Delete>;

auto parse_query(std::string) -> AnyQuery;

auto q = parse_query("SELECT *;");
assert(q.is<Select>());
auto s = Select(q);
```

```c++
class Select; class Insert; class Delete;
using ROQuery = Select;
using RWQuery = xx::variant<Insert, Delete>;
using AnyQuery = xx::variant<Select, Insert, Delete>;

auto parse_query(std::string) -> AnyQuery;
void apply_ro_query(ROQuery);
void apply_rw_query(RWQuery);

auto q = parse_query("SELECT *;");
// apply_ro_query(q); // ERROR - Select(AnyQuery) is explicit
// apply_rw_query(q); // ERROR - RWQuery(AnyQuery) is explicit
if (q.is<RWQuery>())
    apply_rw_query(RWQuery(q));
else
    apply_ro_query(ROQuery(q));
```
## Member functions
### (constructor)
```c++
variant(variant&&);
variant(const variant&);
```
Standard move/copy constructors. **Safe.**

```c++
variant(X&&);
variant(const X&);
```
Construct the variant with a move/copy of the given object. **Safe.**

Enabled iff X is included in T...

```c++
variant(in_place_t<X>, Arg&&... arg);
```
Construct the variant with a X object in place - forwarding arg... as parameters to X's constructor. **Safe.**

Enabled iff X is included in T...

```c++
variant(variant<X...>&&);
variant(const variant<X...>&);
```
Cast a different, but compatible variant to this variant. **Safe.**

Enabled iff X... is a subset of T...

```c++
explicit variant(variant<X...>&&);
explicit variant(const variant<X...>&);
```
Cast a different, NOT compatible variant to this variant. **Unsafe**, you're expected to ensure that the other variant stores an object supported by this variant. In case it stores an object of a unsupported type - behavior is **undefined**.

Enabled iff X... is NOT a subset of T...

### operator=
```c++
operator=(variant&&) -> variant&;
operator=(const variant&) -> variant&;
```
Standard move/copy assignment operators. They destroy the currently stored object and then copy/move-construct a new one. **Safe.**

```c++
operator=(variant<X...>&&) -> variant&;
operator=(const variant<X...>&) -> variant&;
```
Destroys the currently stored object, and move/copy-constructs a new one from a given compatible variant. **Safe.**

Enabled iff X is included in T...

### is&lt;X>()
```c++
is<X>() const -> bool;
```
Checks if the stored object is of the type X. **Safe.**

```c++
is<xx::variant<X...>>() const -> bool;
```
Checks if the stored object is of one of the X... types. **Safe.**

### get&lt;X>()
```c++
get<X>() -> X&;
get<X>() const -> const X&;
```
Returns the stored object. **Unsafe**, you're expected to ensure that X is the type of the stored object (for example by using is&lt;X>() to check the type before calling get&lt;X>()).

Note: get&lt;variant&lt;...>>() is unsupported (as it's impossible to return a reference to a result of casting). Use type casting to cast to different variant types.

Enabled iff X is included in T...

### operator X()
```c++
explicit operator X() const;
```
Returns the stored object. **Unsafe** (as in get<X>()).

Enabled iff X is included in T...
