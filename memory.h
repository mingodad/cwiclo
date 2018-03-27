// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "utility.h"

//{{{ initializer_list -------------------------------------------------
namespace std {	// replaced gcc internal stuff must be in std::

/// Internal class for compiler support of C++11 initializer lists
template <typename T>
class initializer_list {
public:
    using value_type		= T;
    using size_type		= size_t;
    using const_reference	= const value_type&;
    using reference		= const_reference;
    using const_iterator	= const value_type*;
    using iterator		= const_iterator;
private:
    // This object can only be constructed by the compiler when the
    // {1,2,3} syntax is used, so the constructor must be private
    inline constexpr		initializer_list (const_iterator p, size_type sz) noexcept : _data(p), _size(sz) {}
public:
    inline constexpr		initializer_list (void)noexcept	: _data(nullptr), _size(0) {}
    inline constexpr auto	size (void) const noexcept	{ return _size; }
    inline constexpr auto	begin (void) const noexcept	{ return _data; }
    inline constexpr auto	end (void) const noexcept	{ return begin()+size(); }
    inline constexpr auto&	operator[] (size_type i) const	{ return begin()[i]; }
private:
    iterator			_data;
    size_type			_size;
};

} // namespace std
//}}}-------------------------------------------------------------------
//{{{ new and delete

extern "C" void* _realloc (void* p, size_t n) noexcept MALLOCLIKE MALLOCLIKE_ARG(2);
extern "C" void* _alloc (size_t n) noexcept MALLOCLIKE MALLOCLIKE_ARG(1);
extern "C" void _free (void* p) noexcept;

void* operator new (size_t n);
void* operator new[] (size_t n);
void  operator delete (void* p) noexcept;
void  operator delete[] (void* p) noexcept;
void  operator delete (void* p, size_t) noexcept;
void  operator delete[] (void* p, size_t) noexcept;

// Default placement versions of operator new.
inline void* operator new (size_t, void* p)	{ return p; }
inline void* operator new[] (size_t, void* p)	{ return p; }

// Default placement versions of operator delete.
inline void  operator delete  (void*, void*)	{ }
inline void  operator delete[](void*, void*)	{ }

//}}}-------------------------------------------------------------------
//{{{ Additional C++ ABI support

namespace std {

[[noreturn]] void terminate (void) noexcept;

} // namespace std
namespace __cxxabiv1 {

using __guard = cwiclo::atomic_flag;

extern "C" {

[[noreturn]] void __cxa_bad_cast (void) noexcept;
[[noreturn]] void __cxa_bad_typeid (void) noexcept;
[[noreturn]] void __cxa_throw_bad_array_new_length (void) noexcept;
[[noreturn]] void __cxa_throw_bad_array_length (void) noexcept;

// Compiler-generated thread-safe statics initialization
int __cxa_guard_acquire (__guard* g) noexcept;
void __cxa_guard_release (__guard* g) noexcept;
void __cxa_guard_abort (__guard* g) noexcept;

} // extern "C"
} // namespace __cxxabiv1
namespace abi = __cxxabiv1;

//}}}-------------------------------------------------------------------
//{{{ Rvalue forwarding

namespace cwiclo {

template <typename T> constexpr decltype(auto)
forward (remove_reference_t<T>& v) noexcept { return static_cast<T&&>(v); }

template<typename T> constexpr decltype(auto)
forward (remove_reference_t<T>&& v) noexcept { return static_cast<T&&>(v); }

template<typename T> constexpr decltype(auto)
move(T&& v) noexcept { return static_cast<remove_reference_t<T>&&>(v); }

//}}}-------------------------------------------------------------------
//{{{ Swap algorithms

/// Assigns the contents of a to b and the contents of b to a.
/// This is used as a primitive operation by many other algorithms.
template <typename T>
inline void swap (T& a, T& b)
    { auto t = move(a); a = move(b); b = move(t); }

template <typename T, size_t N>
inline void swap (T (&a)[N], T (&b)[N])
{
    for (size_t i = 0; i < N; ++i)
	swap (a[i], b[i]);
}

template <typename T, typename U = T>
inline auto exchange (T& o, U&& nv)
{
    auto ov = move(o);
    o = forward<T>(nv);
    return ov;
}

template <typename I>
inline void iter_swap (I a, I b)
    { swap (*a, *b); }

//}}}-------------------------------------------------------------------
//{{{ iterator_traits
template <typename Iterator>
struct iterator_traits {
    using value_type		= typename Iterator::value_type;
    using difference_type	= typename Iterator::difference_type;
    using pointer		= typename Iterator::pointer;
    using reference		= typename Iterator::reference;
};

template <typename T>
struct iterator_traits<T*> {
    using value_type		= T;
    using difference_type	= ptrdiff_t;
    using const_pointer		= const value_type*;
    using pointer		= value_type*;
    using const_reference	= const value_type&;
    using reference		= value_type&;
};
template <typename T>
struct iterator_traits<const T*> {
    using value_type		= T;
    using difference_type	= ptrdiff_t;
    using const_pointer		= const value_type*;
    using pointer		= const_pointer;
    using const_reference	= const value_type&;
    using reference		= const_reference;
};
template <>
struct iterator_traits<void*> {
    using value_type		= uint8_t;
    using difference_type	= ptrdiff_t;
    using const_pointer		= const void*;
    using pointer		= void*;
    using const_reference	= const value_type&;
    using reference		= value_type&;
};
template <>
struct iterator_traits<const void*> {
    using value_type		= uint8_t;
    using difference_type	= ptrdiff_t;
    using const_pointer		= const void*;
    using pointer		= const_pointer;
    using const_reference	= const value_type&;
    using reference		= const_reference;
};

//}}}-------------------------------------------------------------------
//{{{ unique_ptr

/// \brief A smart pointer.
///
/// Calls delete in the destructor; assignment transfers ownership.
/// This class does not work with void pointers due to the absence
/// of the required dereference operator.
template <typename T>
class unique_ptr {
public:
    using element_type		= T;
    using pointer		= element_type*;
    using reference		= element_type&;
public:
    inline constexpr		unique_ptr (void)		: _p (nullptr) {}
    inline constexpr explicit	unique_ptr (pointer p)		: _p (p) {}
    inline			unique_ptr (unique_ptr&& p)	: _p (p.release()) {}
				unique_ptr (const unique_ptr&) = delete;
    inline			~unique_ptr (void)		{ delete _p; }
    inline constexpr auto	get (void) const		{ return _p; }
    inline auto			release (void)			{ return exchange (_p, nullptr); }
    inline void			reset (pointer p = nullptr)	{ assert (p != _p || !p); delete exchange (_p, p); }
    inline void			swap (unique_ptr&& v)		{ ::cwiclo::swap (_p, v._p); }
    inline constexpr explicit	operator bool (void) const	{ return _p != nullptr; }
    inline auto&		operator= (pointer p)		{ reset (p); return *this; }
    inline auto&		operator= (unique_ptr&& p)	{ reset (p.release()); return *this; }
    void			operator=(const unique_ptr&) = delete;
    inline constexpr auto&	operator* (void) const		{ return *get(); }
    inline constexpr auto	operator-> (void) const		{ return get(); }
    inline constexpr bool	operator== (const pointer p) const	{ return _p == p; }
    inline constexpr bool	operator== (const unique_ptr& p) const	{ return _p == p._p; }
    inline constexpr bool	operator< (const pointer p) const	{ return _p < p; }
    inline constexpr bool	operator< (const unique_ptr& p) const	{ return _p < p._p; }
private:
    pointer			_p;
};

template <typename T, typename... Args>
inline auto make_unique (Args&&... args) { return unique_ptr<T> (new T (forward<Args>(args)...)); }

//}}}-------------------------------------------------------------------
//{{{ scope_exit

template <typename F>
class scope_exit {
public:
    inline explicit	scope_exit (F&& f) noexcept		: _f(move(f)),_enabled(true) {}
    inline		scope_exit (scope_exit&& f) noexcept	: _f(move(f._f)),_enabled(f._enabled) { f.release(); }
    inline void		release (void) noexcept			{ _enabled = false; }
    inline		~scope_exit (void) noexcept (noexcept (declval<F>()))	{ if (_enabled) _f(); }
			scope_exit (const scope_exit&) = delete;
    scope_exit&		operator= (const scope_exit&) = delete;
    scope_exit&		operator= (scope_exit&&) = delete;
private:
    F		_f;
    bool	_enabled;
};

template <typename F>
auto make_scope_exit (F&& f) noexcept
    { return scope_exit<remove_reference_t<F>>(forward<F>(f)); }

extern "C" void print_backtrace (void) noexcept;
#ifndef UC_VERSION
extern "C" void hexdump (const void* vp, size_t n) noexcept;
#endif

//}}}-------------------------------------------------------------------
//{{{ construct and destroy

/// Calls the placement new on \p p.
template <typename T>
inline auto construct_at (T* p)
    { return new (p) T; }

/// Calls the placement new on \p p.
template <typename T>
inline auto construct_at (T* p, const T& value)
    { return new (p) T (value); }

/// Calls the placement new on \p p with \p args.
template <typename T, typename... Args>
inline auto construct_at (T* p, Args&&... args)
    { return new (p) T (forward<Args>(args)...); }

/// Calls the destructor of \p p without calling delete.
template <typename T>
inline void destroy_at (T* p) noexcept
    { p->~T(); }

/// Calls the placement new on \p p.
template <typename I>
auto uninitialized_default_construct (I f, I l)
{
    if constexpr (is_trivially_constructible<typename iterator_traits<I>::value_type>::value) {
	if (f < l)
	    memset (f, 0, (l-f)*sizeof(*f));
    } else for (; f < l; ++f)
	construct_at (f);
    return f;
}

/// Calls the placement new on \p p.
template <typename I>
auto uninitialized_default_construct_n (I f, size_t n)
{
    if constexpr (is_trivially_constructible<typename iterator_traits<I>::value_type>::value)
	memset (f, 0, n*sizeof(*f));
    else for (ssize_t i = n; --i >= 0; ++f)
	construct_at (f);
    return f;
}

/// Calls the destructor on elements in range [f, l) without calling delete.
template <typename I>
void destroy (I f [[maybe_unused]], I l [[maybe_unused]]) noexcept
{
    if constexpr (!is_trivially_destructible<typename iterator_traits<I>::value_type>::value) {
	for (; f < l; ++f)
	    destroy_at (f);
    }
#ifndef NDEBUG
    else if (f < l)
	memset (f, 0xcd, (l-f)*sizeof(*f));
#endif
}

/// Calls the destructor on elements in range [f, f+n) without calling delete.
template <typename I>
inline void destroy_n (I f [[maybe_unused]], size_t n [[maybe_unused]]) noexcept
{
    if constexpr (!is_trivially_destructible<typename iterator_traits<I>::value_type>::value) {
	for (ssize_t i = n; --i >= 0; ++f)
	    destroy_at (f);
    }
#ifndef NDEBUG
    else memset (f, 0xcd, n*sizeof(*f));
#endif
}

//}}}-------------------------------------------------------------------
//{{{ copy and fill

template <typename II, typename OI>
auto copy (II f, II l, OI r)
{
    if constexpr (is_trivially_copyable<typename iterator_traits<II>::value_type>::value)
	memcpy (r, f, (l-f)*sizeof(*f));
    else for (; f < l; ++r, ++f)
	*r = *f;
    return r;
}

template <typename II, typename OI>
auto copy_n (II f, size_t n, OI r)
{
    if constexpr (is_trivially_copyable<typename iterator_traits<II>::value_type>::value)
	memcpy (r, f, n*sizeof(*f));
    else for (ssize_t i = n; --i >= 0; ++r, ++f)
	*r = *f;
    return r;
}

template <typename I, typename T>
auto fill (I f, I l, const T& v)
{
    for (; f < l; ++f)
	*f = v;
    return f;
}

template <typename I, typename T>
auto fill_n (I f, size_t n, const T& v)
{
    for (ssize_t i = n; --i >= 0; ++f)
	*f = v;
    return f;
}

extern "C" void brotate (void* vf, void* vm, void* vl) noexcept;

template <typename T>
T* rotate (T* f, T* m, T* l)
    { brotate (f, m, l); return f; }

template <typename T>
inline void itzero16 (T* t)
{
    static_assert (sizeof(T) == 16, "itzero16 must only be used on 16-byte types");
    static_assert (alignof(T) >= 16, "itzero16 must only be used on 16-aligned types");
#if __SSE2__
    reinterpret_cast<simd16_t*>(t)->sf = simd16_t::zero_sf();
#else
    memset (t, 0, sizeof(T));
#endif
}

template <typename T>
inline void itcopy16 (const T* f, T* t)
{
    static_assert (sizeof(T) == 16, "itcopy16 must only be used on 16-byte types");
    static_assert (alignof(T) >= 16, "itcopy16 must only be used on 16-aligned types");
#if __SSE2__
    reinterpret_cast<simd16_t*>(t)->sf = reinterpret_cast<const simd16_t*>(f)->sf;
#else
    *t = *f;
#endif
}

template <typename T>
inline void itswap16 (T* f, T* t)
{
    static_assert (sizeof(T) == 16, "itswap16 must only be used on 16-byte types");
    static_assert (alignof(T) >= 16, "itswap16 must only be used on 16-aligned types");
#if __SSE2__
    swap (reinterpret_cast<simd16_t*>(f)->sf, reinterpret_cast<simd16_t*>(t)->sf);
#else
    iter_swap (f, t);
#endif
}

template <typename T>
inline void itmoveinit16 (T* f, T* t)
{
    static_assert (sizeof(T) == 16, "itmoveinit16 must only be used on 16-byte types");
    static_assert (alignof(T) >= 16, "itmoveinit16 must only be used on 16-aligned types");
#if __SSE2__
    reinterpret_cast<simd16_t*>(t)->sf = reinterpret_cast<const simd16_t*>(f)->sf;
    reinterpret_cast<simd16_t*>(f)->sf = simd16_t::zero_sf();
#else
    itzero16 (t);
    itswap16 (f, t);
#endif
}

//}}}-------------------------------------------------------------------
//{{{ uninitialized fill and copy

/// Copies [f, l) into r by calling copy constructors in r.
template <typename II, typename OI>
inline auto uninitialized_copy (II f, II l, OI r)
{
    if constexpr (is_same<II,OI>::value && is_trivially_copyable<typename iterator_traits<II>::value_type>::value)
	return copy (f, l, r);
    for (; f < l; ++r, ++f)
	construct_at (&*r, *f);
    return r;
}

/// Copies [f, f + n) into r by calling copy constructors in r.
template <typename II, typename OI>
inline auto uninitialized_copy_n (II f, ssize_t n, OI r)
{
    if constexpr (is_same<II,OI>::value && is_trivially_copyable<typename iterator_traits<II>::value_type>::value)
	return copy_n (f, n, r);
    for (; --n >= 0; ++r, ++f)
	construct_at (&*r, *f);
    return r;
}

/// Copies [f, l) into r by calling move constructors in r.
template <typename II, typename OI>
inline auto uninitialized_move (II f, II l, OI r)
{
    if constexpr (is_same<II,OI>::value && is_trivially_copyable<typename iterator_traits<II>::value_type>::value)
	return copy (f, l, r);
    for (; f < l; ++r, ++f)
	construct_at (&*r, move(*f));
    return r;
}

/// Copies [f, f + n) into r by calling move constructors in r.
template <typename II, typename OI>
inline auto uninitialized_move_n (II f, ssize_t n, OI r)
{
    if constexpr (is_same<II,OI>::value && is_trivially_copyable<typename iterator_traits<II>::value_type>::value)
	return copy_n (f, n, r);
    for (; --n >= 0; ++r, ++f)
	construct_at (&*r, move(*f));
    return r;
}

/// Calls construct on all elements in [f, l) with value \p v.
template <typename I, typename T>
inline void uninitialized_fill (I f, I l, const T& v)
{
    for (; f < l; ++f)
	construct_at (&*f, v);
}

/// Calls construct on all elements in [f, f + n) with value \p v.
template <typename I, typename T>
inline auto uninitialized_fill_n (I f, ssize_t n, const T& v)
{
    for (; --n >= 0; ++f)
	construct_at (&*f, v);
    return f;
}

//}}}-------------------------------------------------------------------
//{{{ Searching algorithms

template <typename I, typename T>
I linear_search (I f, I l, const T& v)
{
    for (; f < l; ++f)
	if (*f == v)
	    return f;
    return nullptr;
}

template <typename I, typename P>
I linear_search_if (I f, I l, P p)
{
    for (; f < l; ++f)
	if (p(*f))
	    return f;
    return nullptr;
}

template <typename I, typename T>
auto lower_bound (I f, I l, const T& v)
{
    while (f < l) {
	auto m = f + uintptr_t(l - f)/2;
	if (*m < v)
	    f = m + 1;
	else
	    l = m;
    }
    return f;
}

template <typename I, typename T>
inline auto binary_search (I f, I l, const T& v)
{
    auto b = lower_bound (f, l, v);
    return (b < l && *b == v) ? b : nullptr;
}

template <typename I, typename T>
auto upper_bound (I f, I l, const T& v)
{
    while (f < l) {
	auto m = f + uintptr_t(l - f)/2;
	if (v < *m)
	    l = m;
	else
	    f = m + 1;
    }
    return l;
}

template <typename T>
int c_compare (const void* p1, const void* p2)
{
    auto& v1 = *(const T*) p1;
    auto& v2 = *(const T*) p2;
    return v1 < v2 ? -1 : (v2 < v1 ? 1 : 0);
}

template <typename I>
void sort (I f, I l)
{
    using value_type = typename iterator_traits<I>::value_type;
    qsort (f, l-f, sizeof(value_type), c_compare<value_type>);
}

template <typename Container>
auto linear_search (Container& c, typename Container::const_reference v)
    { return linear_search (c.begin(), c.end(), v); }
template <typename Container, typename P>
auto linear_search_if (Container& c, P p)
    { return linear_search_if (c.begin(), c.end(), p); }
template <typename Container>
auto lower_bound (Container& c, typename Container::const_reference v)
    { return lower_bound (c.begin(), c.end(), v); }
template <typename Container>
auto upper_bound (Container& c, typename Container::const_reference v)
    { return upper_bound (c.begin(), c.end(), v); }
template <typename Container>
auto binary_search (Container& c, typename Container::const_reference v)
    { return binary_search (c.begin(), c.end(), v); }
template <typename Container>
void sort (Container& c)
    { sort (c.begin(), c.end()); }

//}}}-------------------------------------------------------------------
//{{{ Other algorithms

template <typename Container, typename Discriminator>
void remove_if (Container& ctr, Discriminator f)
{
    foreach (i, ctr)
	if (f(*i))
	    --(i = ctr.erase(i));
}

} // namespace cwiclo
//}}}-------------------------------------------------------------------
