#pragma once

#include <type_traits>
#include <typeinfo>
#include <algorithm>
#include <cassert>

namespace xx
{
	template <typename... T>
	struct _pack_contains: public std::false_type {};

	template <typename F, typename T, typename... Rest>
	struct _pack_contains<F, T, Rest...>: public _pack_contains<F, Rest...> {};

	template <typename F, typename... Rest>
	struct _pack_contains<F, F, Rest...>: public std::true_type {};

	template <typename... T>
	const bool pack_contains = _pack_contains<T...>();

	template <typename T>
	struct in_place_t {};

	template <typename T>
	extern constexpr const in_place_t<T> in_place = in_place_t<T>{};

	template <typename X>
	struct _variant_is
	{
		constexpr static bool is(const std::type_info* t) { return t == &typeid(X); }
	};

	template <typename... T>
	class variant;

	struct null_t {null_t(){}};

	template <typename... X>
	struct _variant_is<variant<X...>>
	{
		constexpr static bool is(const std::type_info* t) { return std::max({_variant_is<X>::is(t)...}); }
	};

	template <typename... T>
	class [[gnu::visibility("default")]] variant
	{
		template <typename... X> friend class variant;
		template <typename X> friend struct _variant_is;

		const std::type_info* _type = nullptr;
		std::aligned_union_t<0, T...> data;

		template <typename X> static const bool includes = pack_contains<X, T...>;
		template <typename... X> struct _includes_all;
		template <typename... X> static const bool includes_all = _includes_all<X...>();

		template <typename X> constexpr void _destroy();
		template <typename X, typename U> constexpr void _init(const U&);
		template <typename X, typename U> constexpr void _init(U&&);
		template <typename X, typename U> constexpr auto _init_tolerant(const U&) -> std::enable_if_t<includes<X>>;
		template <typename X, typename U> constexpr auto _init_tolerant(U&&) -> std::enable_if_t<includes<X>>;
		template <typename X, typename U> constexpr auto _init_tolerant(const U&) -> std::enable_if_t<!includes<X>>;
		template <typename X, typename U> constexpr auto _init_tolerant(U&&) -> std::enable_if_t<!includes<X>>;

		constexpr void destroy();
		template <typename... X> constexpr void init(const variant<X...>& o);
		template <typename... X> constexpr void init(variant<X...>&& o);
		template <typename... X> constexpr void init_tolerant(const variant<X...>& o);
		template <typename... X> constexpr void init_tolerant(variant<X...>&& o);

		template <typename... X>
		inline static void noop(X&&...) {}
	public:
		~variant() {destroy();}

		// We don't support empty state.
		// Want empty state? Add another type to the variant.
		variant() = delete;

		// Safe single type constructors.

		template <typename X, typename Z = std::enable_if_t<includes<X>>>
		variant(X&& v)
			: _type(&typeid(X))
		{
			new (&data) X{std::move(v)};
		}

		template <typename X, typename Z = std::enable_if_t<includes<X>>>
		variant(const X& v)
			: _type(&typeid(X))
		{
			new (&data) X{v};
		}

		template <typename X, typename Z = std::enable_if_t<includes<X>>, typename... Arg>
		variant(in_place_t<X>, Arg&&... arg)
			: _type(&typeid(X))
		{
			new (&data) X{std::forward<Arg>(arg)...};
		}

		// Safe conversion of variants of types that are all included in this variant.

		template <typename... X>
		variant(const variant<X...>& o, std::enable_if_t<includes_all<X...>>* = 0)
		{
			init(o);
			assert(_type);
		}

		template <typename... X>
		variant(variant<X...>&& o, std::enable_if_t<includes_all<X...>>* = 0)
		{
			init(std::move(o));
			assert(_type);
		}

		// Unsafe conversion of not-compatible variants - explicit.
		// Note: Undefined Behavior if used with incorrect type.

		template <typename... X>
		explicit variant(const variant<X...>& o, std::enable_if_t<!includes_all<X...>>* = 0)
		{
			init_tolerant(o);
			assert(_type);
		}

		template <typename... X>
		explicit variant(variant<X...>&& o, std::enable_if_t<!includes_all<X...>>* = 0)
		{
			init_tolerant(std::move(o));
			assert(_type);
		}

		// Copy/move.

		constexpr variant(const variant& o)
		{
			init(o);
		}

		constexpr variant(variant&& o)
		{
			init(std::move(o));
		}

		// Safe conversion of variants of types that are all included in this variant.

		template <typename... X>
		constexpr auto operator=(const variant<X...>& o) -> std::enable_if_t<includes_all<X...>, variant&>
		{
			destroy();
			init(o);
			assert(_type);
			return *this;
		}

		template <typename... X>
		constexpr auto operator=(variant<X...>&& o) -> std::enable_if_t<includes_all<X...>, variant&>
		{
			destroy();
			init(std::move(o));
			assert(_type);
			return *this;
		}

		// Copy/move.

		constexpr auto operator=(const variant& o) -> variant&
		{
			destroy();
			init(o);
			assert(_type);
			return *this;
		}

		constexpr auto operator=(variant&& o) -> variant&
		{
			destroy();
			init(std::move(o));
			assert(_type);
			return *this;
		}

		// Get typeid of the stored type.
		const std::type_info* type() const
		{
			return _type;
		}

		// Type check method. Always defined.

		template <typename X>
		constexpr bool is() const
		{
			return _variant_is<X>::is(_type);
		}

		// Getter. Defined <=> X is supported by this variant.
		// Note: Undefined Behavior if used with incorrect type.

		template <typename X>
		constexpr auto get() -> std::enable_if_t<includes<X>, X&>
		{
			return *reinterpret_cast<X*>(&data);
		}

		template <typename X>
		constexpr auto get() const -> std::enable_if_t<includes<X>, const X&>
		{
			return *reinterpret_cast<const X*>(&data);
		}

		template <typename A, typename... X> struct FirstTypeT;
		template <typename F, typename... X> struct FirstTypeT { using type = F; };
		using FirstType = typename FirstTypeT<T...>::type;

		template <typename X, typename Ret, typename F>
		constexpr void _call(F&& f, variant<null_t, Ret>& ret) const
		{
			if (is<X>())
			{
				ret = Ret{f(get<X>())};
				assert(ret._type == &typeid(Ret));
			}
		}

		template <typename F>
		constexpr auto call(F&& f) const -> decltype(f(std::declval<FirstType>()))
		{
			using Ret = decltype(f(std::declval<FirstType>()));
			auto ret = variant<null_t, Ret>{null_t{}};
			noop((_call<T, Ret>(std::forward<F>(f), ret), 1)...);
			assert(ret._type == &typeid(Ret));
			return ret.template get<Ret>();
		}

		template <typename X, typename Ret, typename F>
		constexpr void _call(F&& f, variant<null_t, Ret>& ret)
		{
			if (is<X>())
			{
				ret = Ret{f(get<X>())};
				assert(ret._type == &typeid(Ret));
			}
		}

		template <typename F>
		constexpr auto call(F&& f) -> decltype(f(std::declval<FirstType&>()))
		{
			using Ret = decltype(f(std::declval<FirstType&>()));
			auto ret = variant<null_t, Ret>{null_t{}};
			noop((_call<T, Ret>(std::forward<F>(f), ret), 1)...);
			assert(ret._type == &typeid(Ret));
			return ret.template get<Ret>();
		}

		// Unsafe conversion to a single type - explicit.
		// Note: Undefined Behavior if used with incorrect type.
		template <typename X>
		constexpr explicit operator X() const
		{
			return get<X>();
		}
	};

	template <typename... T>
	inline auto operator==(const variant<T...>& a, const variant<T...>& b) -> bool
	{
		if (a.type() != b.type())
			return false;

		return a.call([&b](const auto& aa){return aa == b.template get<std::remove_cv_t<std::remove_reference_t<decltype(aa)>>>();});
	}

	template <typename... T>
	inline auto operator!=(const variant<T...>& a, const variant<T...>& b) -> bool
	{
		return !(a == b);
	}

	template <typename... T> template <typename F, typename... X>
	struct variant<T...>::_includes_all<F, X...>: public std::conditional_t<
		includes<F>, _includes_all<X...>, std::false_type
	>{};

	template <typename... T> template <typename F>
	struct variant<T...>::_includes_all<F>: public std::conditional_t<
		includes<F>, std::true_type, std::false_type
	>{};

	template <typename... T> template <typename X>
	inline constexpr void variant<T...>::_destroy()
	{
		if (is<X>())
			get<X>().~X();
	}

	template <typename... T> template <typename X, typename U>
	inline constexpr void variant<T...>::_init(const U& o)
	{
		if (o.template is<X>())
		{
			_type = o._type;
			new (&get<X>()) X(o.template get<X>());
		}
	}

	template <typename... T> template <typename X, typename U>
	inline constexpr void variant<T...>::_init(U&& o)
	{
		if (o._type == &typeid(X))
		{
			_type = o._type;
			new (&get<X>()) X(std::move(o.template get<X>()));
		}
	}

	template <typename... T> template <typename X, typename U>
	inline constexpr auto variant<T...>::_init_tolerant(const U& o) -> std::enable_if_t<includes<X>>
	{
		_init<X>(o);
	}

	template <typename... T> template <typename X, typename U>
	inline constexpr auto variant<T...>::_init_tolerant(U&& o) -> std::enable_if_t<includes<X>>
	{
		_init<X>(std::move(o));
	}

	template <typename... T> template <typename X, typename U>
	inline constexpr auto variant<T...>::_init_tolerant(const U&) -> std::enable_if_t<!includes<X>>
	{}

	template <typename... T> template <typename X, typename U>
	inline constexpr auto variant<T...>::_init_tolerant(U&&) -> std::enable_if_t<!includes<X>>
	{}

	template <typename... T>
	constexpr void variant<T...>::destroy()
	{
		noop((_destroy<T>(), 1)...);
		_type = 0;
	}

	template <typename... T> template <typename... X>
	constexpr void variant<T...>::init(const variant<X...>& o)
	{
		noop((_init<X>(o), 1)...);
	}

	template <typename... T> template <typename... X>
	constexpr void variant<T...>::init(variant<X...>&& o)
	{
		noop((_init<X>(std::move(o)), 1)...);
	}

	template <typename... T> template <typename... X>
	constexpr void variant<T...>::init_tolerant(const variant<X...>& o)
	{
		noop((_init_tolerant<X>(o), 1)...);
	}

	template <typename... T> template <typename... X>
	constexpr void variant<T...>::init_tolerant(variant<X...>&& o)
	{
		noop((_init_tolerant<X>(std::move(o)), 1)...);
	}
}
