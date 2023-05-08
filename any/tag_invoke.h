#pragma once

/*

Read:

https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1895r0.pdf

Example:

// Define a new CPO named mylib::foo()
//
// “Customization point object.” This is a notion introduced by Eric Niebler’s Ranges library.
// [http://eel.is/c++draft/customization.point.object#1]

namespace mylib {

inline constexpr struct foo_cpo {
	template <typename T>
	auto operator()(const T& x) const -> mireo::tag_invoke_result_t<foo_cpo, const T&> {
		// CPO dispatches to tag_invoke() call; passes the CPO itself as first argument.
		return mireo::tag_invoke(*this, x);
	}
} foo;

}

// Use the mylib::foo() CPO
template <class T> requires std::invocable<mireo::tag_t<mylib::foo>, const T&>
void print_foo(const T& x) {
	// Just call the CPO like an ordinary function
	std::cout << mylib::foo(x) << std::endl;
}

// Customise the mylib::foo() CPO for othertype
namespace otherlib {

struct othertype {
	int x;

	friend int tag_invoke(mireo::tag_t<mylib::foo>, const othertype& x) {
		return x.x;
	}
};

}

// Can now call print_foo() function.
void example() {
	otherlib::othertype x;
	print_foo(x);
}

*/

#include <type_traits>

namespace mireo {

namespace _tag_invoke {

struct _fn {
	template <typename CPO, typename... Args>
	constexpr auto operator()(CPO cpo, Args&&... args) const
		noexcept(noexcept(tag_invoke((CPO &&) cpo, (Args &&) args...)))
		-> decltype(tag_invoke((CPO &&) cpo, (Args &&) args...))
	{
		return tag_invoke((CPO &&) cpo, (Args &&) args...);
	}
};

template <typename CPO, typename... Args>
using tag_invoke_result_t = decltype(tag_invoke(std::declval<CPO&&>(), std::declval<Args&&>()...));

using yes_type = char;
using no_type = char(&)[2];

template <typename CPO, typename... Args>
auto try_tag_invoke(int) noexcept(noexcept(tag_invoke(std::declval<CPO&&>(), std::declval<Args&&>()...)))
	-> decltype(static_cast<void>(tag_invoke(std::declval<CPO&&>(), std::declval<Args&&>()...)), yes_type { });

template <typename CPO, typename... Args>
no_type try_tag_invoke(...) noexcept(false);

template <template <typename...> class T, typename... Args>
struct defer {
	using type = T<Args...>;
};

struct empty { };

}  // namespace _tag_invoke

namespace _tag_invoke_cpo {
inline constexpr _tag_invoke::_fn tag_invoke { };
}

using namespace _tag_invoke_cpo;

template <auto& CPO>
using tag_t = std::remove_cvref_t<decltype(CPO)>;

// Manually implement the traits here rather than defining them in terms of
// the corresponding std::invoke_result/is_invocable/is_nothrow_invocable
// traits to improve compile-times. We don't need all of the generality of the
// std:: traits and the tag_invoke traits are used heavily through libunifex
// so optimising them for compile time makes a big difference.

using _tag_invoke::tag_invoke_result_t;

template <typename CPO, typename... Args>
inline constexpr bool is_tag_invocable_v =
	(sizeof(_tag_invoke::try_tag_invoke<CPO, Args...>(0)) == sizeof(_tag_invoke::yes_type));

template <typename CPO, typename... Args>
struct tag_invoke_result : std::conditional_t<
		is_tag_invocable_v<CPO, Args...>,
		_tag_invoke::defer<tag_invoke_result_t, CPO, Args...>,
		_tag_invoke::empty>
{ };

template <typename CPO, typename... Args>
using is_tag_invocable = std::bool_constant<is_tag_invocable_v<CPO, Args...>>;

template <typename CPO, typename... Args>
inline constexpr bool is_nothrow_tag_invocable_v = noexcept(_tag_invoke::try_tag_invoke<CPO, Args...>(0));

template <typename CPO, typename... Args>
using is_nothrow_tag_invocable = std::bool_constant<is_nothrow_tag_invocable_v<CPO, Args...>>;

template <typename CPO, typename... Args>
concept tag_invocable = is_tag_invocable_v<CPO, Args...>;

} // namespace mireo
