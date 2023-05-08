#pragma once

/**

Read:

https://github.com/facebookexperimental/libunifex/blob/main/doc/type_erasure.md

Synopsis:

template <typename... CPOs>
class any;

	Type-erasing wrapper is a move-only wrapper that implements a small-object optimisation
	that allows storing the wrapped object inline if it is smaller than the inline buffer,
	and heap-allocates storage for the object if it does not fit in the inline buffer.

template <typename... CPOs>
class any_copyable;

	Copyable variant of `mireo::any`. Requires wrapped objects to be copy-constructible and
	copy-assignable.

template <class Signature, typename... CPOs>
class any_function;

	Similar to std::function that can be further customized with CPOs; like mireo::any.

	mireo::any_function<int(int, char**)> the_main = main;
	the_main(argc, argv);

template <class Signature, typename... CPOs>
class any_copyable_function;

	Copyable variant of `mireo::any_function`.

	mireo::any_copyable_function<int(int, char**)> main1 = &main;
	auto main2 = main1;

*/

#include <memory>
#include "tag_invoke.h"

namespace mireo {

//
// this_
//

struct this_ { };

template <typename T>
inline constexpr bool is_this_v = false;
template <>
inline constexpr bool is_this_v<this_> = true;
template <>
inline constexpr bool is_this_v<this_&> = true;
template <>
inline constexpr bool is_this_v<this_&&> = true;
template <>
inline constexpr bool is_this_v<const this_> = true;
template <>
inline constexpr bool is_this_v<const this_&> = true;
template <>
inline constexpr bool is_this_v<const this_&&> = true;

namespace detail {

struct _ignore {
	template <typename T> _ignore(T&&) noexcept { }
};

template <typename>
struct _replace_this;

template <>
struct _replace_this<void> {
	template <typename Arg, typename>
	using apply = Arg;

	template <typename Arg>
	static Arg&& get(Arg&& arg, _ignore) noexcept {
		return (Arg &&) arg;
	}
};

template <>
struct _replace_this<this_> {
	template <typename, typename T>
	using apply = T;

	template <typename T>
	static T&& get(_ignore, T& obj) noexcept {
		return (T &&) obj;
	}
};

template <>
struct _replace_this<this_&> {
	template <typename, typename T>
	using apply = T&;

	template <typename T>
	static T& get(_ignore, T& obj) noexcept {
		return obj;
	}
};

template <>
struct _replace_this<this_&&> {
	template <typename, typename T>
	using apply = T&&;

	template <typename T>
	static T&& get(_ignore, T& obj) noexcept {
		return (T &&) obj;
	}
};

template <>
struct _replace_this<const this_&> {
	template <typename, typename T>
	using apply = const T&;

	template <typename T>
	static const T& get(_ignore, T& obj) noexcept {
		return obj;
	}
};

template <>
struct _replace_this<const this_&&> {
	template <typename, typename T>
	using type = const T&&;

	template <typename T>
	static const T&& get(_ignore, T& obj) noexcept {
		return (const T&&) obj;
	}
};

template <typename Arg>
using _normalize_t = std::conditional_t<is_this_v<Arg>, Arg, void>;

template <typename T>
using replace_this = _replace_this<_normalize_t<T>>;

template <typename Arg, typename T>
using replace_this_t = typename replace_this<Arg>::template apply<Arg, T>;

template <typename Arg>
using replace_this_with_void_ptr_t = std::conditional_t<is_this_v<Arg>, void*, Arg>;

template <bool...>
struct _extract_this {
	template <typename TFirst, typename... TRest>
	TFirst&& operator()(TFirst&& first, TRest&&...) const noexcept {
		return (TFirst&&) first;
	}
};

template <bool... IsThis>
struct _extract_this<false, IsThis...> {
	template <typename... TRest>
	decltype(auto) operator()(_ignore, TRest&&... rest) const noexcept {
		static_assert(sizeof...(IsThis) > 0, "Arguments to extract_this");
		return _extract_this<IsThis...>{ }((TRest &&) rest...);
	}
};

template <typename... Ts>
using extract_this = _extract_this<is_this_v<Ts>...>;

//
// overload
//

template <typename CPO, typename Sig>
struct _cpo_t {
	struct type;
};

// This type will have the associated namespaces
// of CPO (by inheritance) but not those of Sig.
template <typename CPO, typename Sig>
struct _cpo_t<CPO, Sig>::type : CPO {
	constexpr type() = default;
	constexpr type(CPO) noexcept {}

	using base_cpo_t = CPO;
	using type_erased_signature_t = Sig;
};

template <typename CPO, typename Enable = void>
struct base_cpo {
	using type = CPO;
};

template <typename CPO>
struct base_cpo<CPO, std::void_t<typename CPO::base_cpo_t>> {
	using type = typename CPO::base_cpo_t;
};

template <typename CPO, typename Sig>
inline constexpr typename _cpo_t<CPO, Sig>::type _cpo { };

template <typename Sig>
struct _sig { };

template <typename Sig>
inline constexpr _sig<Sig> const sig { };

template <typename CPO>
using base_cpo_t = typename base_cpo<CPO>::type;

//
// vtable
//

template <typename CPO, typename T, typename Ret, typename... Args>
Ret _vtable_invoke(CPO cpo, replace_this_with_void_ptr_t<Args>... args) noexcept {
	void* this_ptr = extract_this<Args...> { }(args...);
	return ((CPO &&)cpo)(replace_this<Args>::get((decltype(args)&&)args, *static_cast<T*>(this_ptr))...);
}

template <typename CPO, typename Sig = typename CPO::type_erased_signature_t>
class vtable_entry;

template <typename CPO, typename Ret, typename... Args>
class vtable_entry<CPO, Ret(Args...) noexcept> {
public:
	using fn_t = Ret(base_cpo_t<CPO>, replace_this_with_void_ptr_t<Args>...) noexcept;

	constexpr fn_t* get() const noexcept {
		return _fn;
	}

	template <typename T>
	static constexpr vtable_entry create() noexcept {
		return vtable_entry { &_vtable_invoke<base_cpo_t<CPO>, T, Ret, Args...> };
	}

private:
	explicit constexpr vtable_entry(fn_t* fn) noexcept : _fn(fn) {
	}

	fn_t* _fn;
};

template <typename CPO, typename Ret, typename... Args>
class vtable_entry<CPO, Ret(Args...)> {
public:
	using fn_t = Ret(base_cpo_t<CPO>, replace_this_with_void_ptr_t<Args>...);

	constexpr fn_t* get() const noexcept {
		return _fn;
	}

	template <typename T>
	static constexpr vtable_entry create() noexcept {
		return vtable_entry { &_vtable_invoke<base_cpo_t<CPO>, T, Ret, Args...> };
	}

private:
	explicit constexpr vtable_entry(fn_t* fn) noexcept : _fn(fn) {
	}

	fn_t* _fn;
};

template <typename... CPOs>
class vtable : private vtable_entry<CPOs>... {

	explicit constexpr vtable(vtable_entry<CPOs>... entries) noexcept : vtable_entry<CPOs> { entries }... {
	}

public:
	template <typename T>
	static const vtable* inst() {
		static constexpr vtable v { vtable_entry<CPOs>::template create<T>()... };
		return &v;
	};

	template <typename CPO>
	constexpr auto get() const noexcept -> typename vtable_entry<CPO>::fn_t* {
		const vtable_entry<CPO>& entry = *this;
		return entry.get();
	}
};

//
// get_wrapped_object / with_forwarding_tag_invoke
//

struct _get_wrapped_object_cpo {
	template <typename T> requires tag_invocable<_get_wrapped_object_cpo, T>
	auto operator()(T&& wrapper) const noexcept(is_nothrow_tag_invocable_v<_get_wrapped_object_cpo, T>) ->
		tag_invoke_result_t<_get_wrapped_object_cpo, T>
	{
		return mireo::tag_invoke(*this, static_cast<T&&>(wrapper));
	}
};

inline constexpr _get_wrapped_object_cpo get_wrapped_object { };

template <typename Derived, typename CPO, typename Sig>
struct _with_forwarding_tag_invoke;

template <typename Derived, typename CPO>
using with_forwarding_tag_invoke = typename _with_forwarding_tag_invoke<
	Derived,
	base_cpo_t<CPO>,
	typename CPO::type_erased_signature_t
>::type;

// noexcept(false) specialisation
template <typename Derived, typename CPO, typename Ret, typename... Args>
struct _with_forwarding_tag_invoke<Derived, CPO, Ret(Args...)> {
	struct type {
		friend Ret tag_invoke(CPO cpo, replace_this_t<Args, Derived>... args) {
			auto& wrapper = extract_this<Args...>{}(args...);
			auto& wrapped = get_wrapped_object(wrapper);
			return static_cast<CPO&&>(cpo)(replace_this<Args>::get(static_cast<decltype(args)&&>(args), wrapped)...);
		}
	};
};

// noexcept(true) specialisation
template <typename Derived, typename CPO, typename Ret, typename... Args>
struct _with_forwarding_tag_invoke<Derived, CPO, Ret(Args...) noexcept> {
	struct type {
		friend Ret tag_invoke(CPO cpo, replace_this_t<Args, Derived>... args) noexcept {
			auto& wrapper = extract_this<Args...>{}(args...);
			auto& wrapped = get_wrapped_object(wrapper);
			return static_cast<CPO&&>(cpo)(replace_this<Args>::get(static_cast<decltype(args)&&>(args), wrapped)...);
		}
	};
};

//
// with_type_erased_tag_invoke
//

template <typename Derived, typename CPO, typename Sig>
struct _with_type_erased_tag_invoke;

template <typename Derived, typename CPO, typename Ret, typename... Args>
struct _with_type_erased_tag_invoke<Derived, CPO, Ret(Args...)> {
	struct type {
		friend Ret tag_invoke(base_cpo_t<CPO> cpo, replace_this_t<Args, Derived>... args) {
			using cpo_t = base_cpo_t<CPO>;
			auto&& t = extract_this<Args...>{ }((decltype(args)&&)args...);
			void* ptr = get_object_address(t);
			auto* fn = get_vtable(t)->template get<CPO>();
			return fn((cpo_t&&) cpo, replace_this<Args>::get((decltype(args)&&)args, ptr)...);
		}
	};
};

//
// any_heap_allocated_storage
//

template <typename T, typename Allocator>
class _any_heap_allocated_storage {
	struct state;

	using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<state>;
	using allocator_traits = std::allocator_traits<allocator_type>;

	// This is the state that is actually heap-allocated.
	struct state {
		template <typename... Args> requires std::constructible_from<T, Args...>
		explicit state(std::allocator_arg_t, allocator_type allocator, std::in_place_type_t<T>, Args&&... args) :
			object(static_cast<Args&&>(args)...),
			allocator(std::move(allocator))
		{
		}

		[[no_unique_address]] T object;
		[[no_unique_address]] allocator_type allocator;
	};

	// This is the base-class object that holds the pointer to the
	// heap-allocated state.
	struct base {
		template <typename... Args> requires std::constructible_from<T, Args...>
		base(std::allocator_arg_t, allocator_type allocator, std::in_place_type_t<T>, Args&&... args) {
			_state = allocator_traits::allocate(allocator, 1);
			allocator_traits::construct(
				allocator,
				_state,
				std::allocator_arg,
				allocator,
				std::in_place_type<T>,
				static_cast<Args&&>(args)...
			);
		}

		template <typename... Args> requires (
			!std::same_as<Allocator, allocator_type> &&
			std::constructible_from<T, Args...>
		)
		explicit base(std::allocator_arg_t, Allocator allocator, std::in_place_type_t<T>, Args&&... args) :
			base(
				std::allocator_arg,
				allocator_type(std::move(allocator)),
				std::in_place_type<T>,
				static_cast<Args&&>(args)...
			)
		{
		}

		base(const base& other) requires std::copy_constructible<T> :
			base(
				std::allocator_arg,
				const_cast<const allocator_type&>(other._state->allocator),
				std::in_place_type<T>,
				const_cast<const T&>(other._state->object)
			)
		{
		}

		base(base&& other) noexcept : _state(std::exchange(other._state, nullptr)) {
		}

		~base() {
			if (_state != nullptr) {
				allocator_type alloc = std::move(_state->allocator);
				_state->~state();
				std::allocator_traits<allocator_type>::deallocate(alloc, _state, 1);
			}
		}

	private:
		friend T& tag_invoke(tag_t<get_wrapped_object>, base& self) noexcept {
			return self._state->object;
		}

		friend const T& tag_invoke(tag_t<get_wrapped_object>, const base& self) noexcept {
			return self._state->object;
		}

		state* _state;
	};

	template <typename... CPOs>
	struct concrete final {
		class type : public base, private detail::with_forwarding_tag_invoke<type, CPOs>... {
			using base::base;
		};
	};

public:
	template <typename... CPOs>
	using type = typename concrete<CPOs...>::type;
};

template <typename T, typename Allocator, typename... CPOs>
using any_heap_allocated_storage = typename _any_heap_allocated_storage<T, Allocator>::template type<CPOs...>;

//
// _destroy_cpo / _move_construct_cpo / _copy_construct_cpo / _invoke_cpo
//

struct _destroy_cpo {
	using type_erased_signature_t = void(this_&) noexcept;

	template <typename T>
	void operator()(T& object) const noexcept {
		object.~T();
	}
};

struct _move_construct_cpo {
	using type_erased_signature_t = void(void* p, this_&& src) noexcept;

	template <typename T>
	void operator()(void* p, T&& src) const noexcept {
		::new (p) T(static_cast<T&&>(src));
	}
};

struct _copy_construct_cpo {
	using type_erased_signature_t = void(void* p, const this_& src) noexcept;

	template <typename T>
	void operator()(void* p, const T& src) const noexcept {
		::new (p) T(src);
	}
};

template <typename Sig> struct _invoke_cpo;

template <typename R, typename... Args>
struct _invoke_cpo<R(Args...)> {
	using type_erased_signature_t = R(this_&, Args...);

	template <class F>
	static constexpr bool with_tag_invoke_v = mireo::is_tag_invocable_v<_invoke_cpo, F, Args...>;

	template <class F>
	static constexpr bool nothrow_invoke_v = with_tag_invoke_v<F> ?
		mireo::is_nothrow_tag_invocable_v<_invoke_cpo, F, Args...> :
		std::is_nothrow_invocable_v<F, Args...>;

	template<typename F>
	R operator()(F&& fn, Args... arg) const noexcept(nothrow_invoke_v<F>) {
		if constexpr (with_tag_invoke_v<F>) {
			return mireo::tag_invoke(*this, (F&&)fn, (Args&&)arg...);
		}
		else {
			return ((F&&)fn)((Args&&)arg...);
		}
	}
};

}  // namespace detail

//
// with_type_erased_tag_invoke
//
// When defining a type-erasing wrapper type, Derived, you can privately inherit
// from this class to have the type opt-in to customising the specified CPO.
//

template <typename Derived, typename CPO>
using with_type_erased_tag_invoke = typename detail::_with_type_erased_tag_invoke<
	Derived,
	CPO,
	typename
	CPO::type_erased_signature_t
>::type;

//
// _any_object
//

template <std::size_t InlineSize, typename DefaultAllocator, bool IsCopyable, typename... CPOs>
struct _any_object {
	// Pad size/alignment out to allow storage of at least a pointer.
	static constexpr std::size_t inline_alignment = alignof(void*);
	static constexpr std::size_t inline_size = InlineSize < sizeof(void*) ? sizeof(void*) : InlineSize;

	// move-constructor is (must be) noexcept
	template <typename T>
	static constexpr bool can_be_stored_inplace_v = (sizeof(T) <= inline_size && alignof(T) <= inline_alignment);

	static constexpr bool copyable_v = IsCopyable;

	class type;
};

template <std::size_t InlineSize, typename DefaultAllocator, bool IsCopyable, typename... CPOs>
class _any_object<InlineSize, DefaultAllocator, IsCopyable, CPOs...>::type :
	private with_type_erased_tag_invoke<type, CPOs>...
{
	using vtable_t = std::conditional_t<
		_any_object::copyable_v,
		detail::vtable<detail::_destroy_cpo, detail::_move_construct_cpo, detail::_copy_construct_cpo, CPOs...>,
		detail::vtable<detail::_destroy_cpo, detail::_move_construct_cpo, CPOs...>
	>;

	const vtable_t* _vtable = nullptr;
	alignas(inline_alignment) std::byte _storage[inline_size];

public:
	type() = default;

	template <typename T> requires (!std::is_same_v<type, std::remove_cvref_t<T>>)
	type(T&& object) : type(std::in_place_type<std::remove_cvref_t<T>>, static_cast<T&&>(object)) {
	}

	template <typename T, typename Allocator>
	explicit type(std::allocator_arg_t, Allocator allocator, T&& value) noexcept :
		type(
			std::allocator_arg,
			std::move(allocator),
			std::in_place_type<std::remove_cvref_t<T>>,
			static_cast<T&&>(value)
		)
	{
	}

	template <typename T, typename... Args> requires _any_object::can_be_stored_inplace_v<T>
	explicit type(std::in_place_type_t<T>, Args&&... args) : _vtable(vtable_t::template inst<T>()) {
		::new (static_cast<void*>(&_storage)) T(static_cast<Args&&>(args)...);
	}

	template <typename T, typename... Args> requires (!_any_object::can_be_stored_inplace_v<T>)
	explicit type(std::in_place_type_t<T>, Args&&... args) :
		type(std::allocator_arg, DefaultAllocator(), std::in_place_type<T>, static_cast<Args&&>(args)...)
	{
	}

	template <typename T, typename Allocator, typename... Args> requires _any_object::can_be_stored_inplace_v<T>
	explicit type(std::allocator_arg_t, Allocator, std::in_place_type_t<T>, Args&&... args) noexcept :
		type(std::in_place_type<T>, static_cast<Args&&>(args)...)
	{
	}

	template <typename T, typename Alloc, typename... Args> requires (!_any_object::can_be_stored_inplace_v<T>)
	explicit type(std::allocator_arg_t, Alloc alloc, std::in_place_type_t<T>, Args&&... args) :
		_vtable(vtable_t::template inst<detail::any_heap_allocated_storage<T, Alloc, CPOs...>>())
	{
		::new (static_cast<void*>(&_storage)) detail::any_heap_allocated_storage<T, Alloc, CPOs...>(
			std::allocator_arg,
			std::move(alloc),
			std::in_place_type<T>,
			static_cast<Args&&>(args)...
		);
	}

	type(const type& other) noexcept requires _any_object::copyable_v : _vtable(other._vtable) {
		if (_vtable) {
			auto* copy_cons = _vtable->template get<detail::_copy_construct_cpo>();
			copy_cons(detail::_copy_construct_cpo { }, &_storage, get_object_address(other));
		}
	}

	type(type&& other) noexcept : _vtable(other._vtable) {
		if (_vtable) {
			auto* move_cons = _vtable->template get<detail::_move_construct_cpo>();
			move_cons(detail::_move_construct_cpo{ }, &_storage, &other._storage);
		}
	}

	~type() noexcept {
		if (_vtable) {
			auto* destroy = _vtable->template get<detail::_destroy_cpo>();
			destroy(detail::_destroy_cpo{ }, &_storage);
		}
	}

	type& operator=(const type& other) noexcept requires _any_object::copyable_v {
		if (std::addressof(other) == this)
			return *this;

		if (_vtable) {
			auto* destroy = _vtable->template get<detail::_destroy_cpo>();
			destroy(detail::_destroy_cpo{}, &_storage);
		}
		_vtable = other._vtable;
		if (_vtable) {
			auto* copy_cons = _vtable->template get<detail::_copy_construct_cpo>();
			copy_cons(detail::_copy_construct_cpo { }, &_storage, get_object_address(other));
		}
		return *this;
	}

	type& operator=(type&& other) noexcept {
		if (std::addressof(other) == this)
			return *this;

		if (_vtable) {
			auto* destroy = _vtable->template get<detail::_destroy_cpo>();
			destroy(detail::_destroy_cpo{ }, &_storage);
		}
		_vtable = other._vtable;
		if (_vtable) {
			auto* move_cons = _vtable->template get<detail::_move_construct_cpo>();
			move_cons(detail::_move_construct_cpo{ }, &_storage, &other._storage);
		}
		return *this;
	}

	template <typename T> requires
		_any_object::can_be_stored_inplace_v<std::remove_cvref_t<T>> &&
		(!std::is_same_v<type, std::remove_cvref_t<T>>)
	type& operator=(T&& value) noexcept {
		if (_vtable) {
			auto* destroy = _vtable->template get<detail::_destroy_cpo>();
			destroy(detail::_destroy_cpo{ }, &_storage);
		}
		using value_type = std::remove_cvref_t<T>;
		::new (static_cast<void*>(&_storage)) value_type(static_cast<T&&>(value));
		_vtable = vtable_t::template inst<value_type>();
		return *this;
	}

	template <typename T> requires
		(!_any_object::can_be_stored_inplace_v<std::remove_cvref_t<T>>) &&
		(!std::is_same_v<type, std::remove_cvref_t<T>>)
	type& operator=(T&& value) noexcept {
		if (_vtable) {
			auto* destroy = _vtable->template get<detail::_destroy_cpo>();
			destroy(detail::_destroy_cpo{ }, &_storage);
		}
		using value_type = detail::any_heap_allocated_storage<std::remove_cvref_t<T>, DefaultAllocator, CPOs...>;
		::new (static_cast<void*>(&_storage)) value_type(
			std::allocator_arg,
			DefaultAllocator { },
			std::in_place_type<std::remove_cvref_t<T>>,
			static_cast<T&&>(value)
		);
		_vtable = vtable_t::template inst<value_type>();
		return *this;
	}

	explicit operator bool() const noexcept {
		return _vtable != nullptr;
	}

private:
	friend const vtable_t* get_vtable(const type& self) noexcept {
		return self._vtable;
	}

	friend void* get_object_address(const type& self) noexcept {
		return const_cast<void*>(static_cast<const void*>(&self._storage));
	}
};

//
// _any_function_t
//

template <typename Sig, size_t InlineSize, typename DefaultAllocator, bool IsCopyable, typename... CPOs>
class _any_function_t;

template <typename R, typename... Args, size_t InlineSize, typename DefaultAllocator, bool IsCopyable, typename... CPOs>
class _any_function_t<R(Args...), InlineSize, DefaultAllocator, IsCopyable, CPOs...> :
	public _any_object<InlineSize, DefaultAllocator, IsCopyable, detail::_invoke_cpo<R(Args...)>, CPOs...>::type
{
	using invoke_cpo = detail::_invoke_cpo<R(Args...)>;
	using base_t = typename _any_object<
		InlineSize,
		DefaultAllocator,
		IsCopyable,
		detail::_invoke_cpo<R(Args...)>,
		CPOs...
	>::type;

public:
	using result_type = R;

	using base_t::base_t; // use constructors from base

	R operator()(Args... arg) const noexcept {
		auto* invoke = get_vtable(*this)->template get<invoke_cpo>();
		return invoke(invoke_cpo { }, get_object_address(*this), (Args&&)arg...);
	}
};

//
// basic_any / basic_any_copyable
//

template <std::size_t InlineSize, typename DefaultAllocator, typename... CPOs>
using basic_any_copyable_t = typename _any_object<InlineSize, DefaultAllocator, true, CPOs...>::type;

template <std::size_t InlineSize, typename DefaultAllocator, typename... CPOs>
using basic_any_t = typename _any_object<InlineSize, DefaultAllocator, false, CPOs...>::type;

template <std::size_t InlineSize, typename DefaultAllocator, auto&... CPOs>
using basic_any_copyable = basic_any_copyable_t<InlineSize, DefaultAllocator, mireo::tag_t<CPOs>...>;

template <std::size_t InlineSize, typename DefaultAllocator, auto&... CPOs>
using basic_any = basic_any_t<InlineSize, DefaultAllocator, mireo::tag_t<CPOs>...>;

//
// any / any_copyable
//

template <typename... CPOs>
using any_copyable_t = basic_any_copyable_t<4 * sizeof(void*), std::allocator<std::byte>, CPOs...>;

template <auto&... CPOs>
using any_copyable = any_copyable_t<mireo::tag_t<CPOs>...>;

template <typename... CPOs>
using any_t = basic_any_t<4 * sizeof(void*), std::allocator<std::byte>, CPOs...>;

template <auto&... CPOs>
using any = any_t<mireo::tag_t<CPOs>...>;

//
// basic_any_function / basic_any_copyable_function
//

template <typename Sig, size_t InlineSize, typename DefaultAllocator, typename... CPOs>
using basic_any_function_t = _any_function_t<Sig, InlineSize, DefaultAllocator, false, CPOs...>;

template <typename Sig, size_t InlineSize, typename DefaultAllocator, auto&... CPOs>
using basic_any_function = basic_any_function_t<Sig, InlineSize, DefaultAllocator, mireo::tag_t<CPOs>...>;

template <typename Sig, size_t InlineSize, typename DefaultAllocator, typename... CPOs>
using basic_any_copyable_function_t = _any_function_t<Sig, InlineSize, DefaultAllocator, true, CPOs...>;

template <typename Sig, size_t InlineSize, typename DefaultAllocator, auto&... CPOs>
using basic_any_copyable_function = basic_any_copyable_function_t<
	Sig,
	InlineSize,
	DefaultAllocator,
	mireo::tag_t<CPOs>...
>;

//
// any_function / any_copyable_function
//

template <typename Sig, typename... CPOs>
using any_function_t = basic_any_function_t<Sig, 4 * sizeof(void*), std::allocator<std::byte>, CPOs...>;

template <typename Sig, auto&... CPOs>
using any_function = any_function_t<Sig, mireo::tag_t<CPOs>...>;

template <typename Sig, typename... CPOs>
using any_copyable_function_t = basic_any_copyable_function_t<
	Sig,
	4 * sizeof(void*),
	std::allocator<std::byte>,
	CPOs...
>;

template <typename Sig, auto&... CPOs>
using any_copyable_function = any_copyable_function_t<Sig, mireo::tag_t<CPOs>...>;

//
// any_invoke
//

template <typename Sig>
inline constexpr detail::_invoke_cpo<Sig> any_invoke { };

//
// overload
//

template <auto& CPO, typename Sig>
using overload_t = typename detail::_cpo_t<tag_t<CPO>, Sig>::type;

template <typename Sig, typename CPO>
constexpr typename detail::_cpo_t<CPO, Sig>::type const& overload(CPO const&, detail::_sig<Sig> = { }) noexcept {
	return detail::_cpo<CPO, Sig>;
}

} // namespace mireo
