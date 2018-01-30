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
    inline auto			release (void)			{ auto rv (_p); _p = nullptr; return rv; }
    inline void			reset (pointer p = nullptr)	{ assert (p != _p || !p); auto ov (_p); _p = p; delete ov; }
    inline void			swap (unique_ptr& v)		{ swap (_p, v._p); }
    inline constexpr explicit	operator bool (void) const	{ return _p != nullptr; }
    inline auto&		operator= (pointer p)		{ reset (p); return *this; }
    inline auto&		operator= (unique_ptr&& p)	{ reset (p.release()); return *this; }
    void			operator=(const unique_ptr&) = delete;
    inline constexpr auto&	operator* (void) const		{ return *get(); }
    inline constexpr auto	operator-> (void) const		{ return get(); }
    inline constexpr bool	operator== (const pointer p) const	{ return _p == p; }
    inline constexpr bool	operator== (const unique_ptr& p) const	{ return _p == p._p; }
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

// Helper templates to bulk construct trivial types
namespace {

template <typename P, bool Trivial>
struct type_ctors {
    inline void call (P f, P l) { for (; f < l; ++f) construct_at (f); }
    inline void call (P f, ssize_t n) { for (;--n >= 0; ++f) construct_at (f); }
};
template <typename P>
struct type_ctors<P,true> {
    inline void call (P f, size_t n) { memset (f, 0, n*sizeof(*f)); }
    inline void call (P f, P l) { if (f < l) call (f, l-f); }
};
template <typename P, bool Trivial>
struct type_dtors {
    inline void call (P f, P l) { for (; f < l; ++f) destroy_at (f); }
    inline void call (P f, ssize_t n) { for (;--n >= 0; ++f) destroy_at (f); }
};
template <typename P>
struct type_dtors<P,true> {
#ifndef NDEBUG
    inline void call (P f, size_t n) { memset (f, 0xcd, n*sizeof(*f)); }
    inline void call (P f, P l) { if (f < l) call (f, l-f); }
#else
    inline void call (P, P) {}
    inline void call (P, size_t) {}
#endif
};
} // namespace

/// Calls the placement new on \p p.
template <typename I>
inline auto uninitialized_default_construct (I f, I l)
{
    using value_type = typename iterator_traits<I>::value_type;
    using type_ctors_t = type_ctors<I,is_trivially_constructible<value_type>::value>;
    type_ctors_t().call (f, l);
    return f;
}

/// Calls the placement new on \p p.
template <typename I>
inline auto uninitialized_default_construct_n (I f, size_t n)
{
    using value_type = typename iterator_traits<I>::value_type;
    using type_ctors_t = type_ctors<I,is_trivially_constructible<value_type>::value>;
    type_ctors_t().call (f, n);
    return f;
}

/// Calls the destructor on elements in range [f, l) without calling delete.
template <typename I>
inline void destroy (I f, I l) noexcept
{
    using value_type = typename iterator_traits<I>::value_type;
    using type_dtors_t = type_dtors<I,is_trivially_destructible<value_type>::value>;
    type_dtors_t().call (f, l);
}

/// Calls the destructor on elements in range [f, f+n) without calling delete.
template <typename I>
inline void destroy_n (I f, size_t n) noexcept
{
    using value_type = typename iterator_traits<I>::value_type;
    using type_dtors_t = type_dtors<I,is_trivially_destructible<value_type>::value>;
    type_dtors_t().call (f, n);
}

//}}}-------------------------------------------------------------------
//{{{ copy and fill

namespace {
template <typename II, typename OI, bool Trivial>
struct type_copy {
    inline void call (II first, II last, OI result)
	{ for (; first < last; ++result, ++first) *result = *first; }
    inline void call_n (II first, ssize_t n, OI result)
	{ for (; --n >= 0; ++result, ++first) *result = *first; }
};
template <typename I>
struct type_copy<I,I,true> {
    inline void call (I first, I last, I result)
	{ memcpy (result, first, (last-first)*sizeof(*first)); }
    inline void call_n (I first, size_t n, I result)
	{ memcpy (result, first, n*sizeof(*first)); }
};
}

template <typename II, typename OI>
auto copy (II first, II last, OI result)
{
    using value_type = typename iterator_traits<II>::value_type;
    using type_copy_t = type_copy<II,OI,is_trivially_copyable<value_type>::value>;
    type_copy_t().call (first, last, result);
    return result;
}

template <typename II, typename OI>
auto copy_n (II first, size_t n, OI result)
{
    using value_type = typename iterator_traits<II>::value_type;
    using type_copy_t = type_copy<II,OI,is_trivially_copyable<value_type>::value>;
    type_copy_t().call_n (first, n, result);
    return result;
}

template <typename I, typename T>
auto fill (I first, I last, const T& v)
{
    for (; first < last; ++first)
	*first = v;
    return first;
}

template <typename I, typename T>
auto fill_n (I first, ssize_t n, const T& v)
{
    for (; --n >= 0; ++first)
	*first = v;
    return first;
}

extern "C" void brotate (void* vf, void* vm, void* vl) noexcept;

template <typename T>
T* rotate (T* f, T* m, T* l)
    { brotate (f, m, l); return f; }

//}}}-------------------------------------------------------------------
//{{{ uninitialized fill and copy

/// Copies [first, last) into result by calling copy constructors in result.
template <typename II, typename OI>
inline auto uninitialized_copy (II first, II last, OI result)
{
    if (is_same<II,OI>::value && is_trivially_copyable<typename iterator_traits<II>::value_type>::value)
	return copy (first, last, result);
    for (; first < last; ++result, ++first)
	construct_at (&*result, *first);
    return result;
}

/// Copies [first, first + n) into result by calling copy constructors in result.
template <typename II, typename OI>
inline auto uninitialized_copy_n (II first, ssize_t n, OI result)
{
    if (is_same<II,OI>::value && is_trivially_copyable<typename iterator_traits<II>::value_type>::value)
	return copy_n (first, n, result);
    for (; --n >= 0; ++result, ++first)
	construct_at (&*result, *first);
    return result;
}

/// Copies [first, last) into result by calling move constructors in result.
template <typename II, typename OI>
inline auto uninitialized_move (II first, II last, OI result)
{
    if (is_same<II,OI>::value && is_trivially_copyable<typename iterator_traits<II>::value_type>::value)
	return copy (first, last, result);
    for (; first < last; ++result, ++first)
	construct_at (&*result, move(*first));
    return result;
}

/// Copies [first, first + n) into result by calling move constructors in result.
template <typename II, typename OI>
inline auto uninitialized_move_n (II first, ssize_t n, OI result)
{
    if (is_same<II,OI>::value && is_trivially_copyable<typename iterator_traits<II>::value_type>::value)
	return copy_n (first, n, result);
    for (; --n >= 0; ++result, ++first)
	construct_at (&*result, move(*first));
    return result;
}

/// Calls construct on all elements in [first, last) with value \p v.
template <typename I, typename T>
inline void uninitialized_fill (I first, I last, const T& v)
{
    for (; first < last; ++first)
	construct_at (&*first, v);
}

/// Calls construct on all elements in [first, first + n) with value \p v.
template <typename I, typename T>
inline auto uninitialized_fill_n (I first, ssize_t n, const T& v)
{
    for (; --n >= 0; ++first)
	construct_at (&*first, v);
    return first;
}

//}}}-------------------------------------------------------------------
//{{{ Searching algorithms

template <typename I, typename T>
auto lower_bound (I f, I l, const T& v)
{
    while (f < l) {
	auto m = f + (l - f)/2;
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
    return (b == l || v < *b) ? nullptr : b;
}

template <typename I, typename T>
I linear_search (I f, I l, const T& v)
{
    for (; f < l; ++f)
	if (*f == v)
	    return f;
    return nullptr;
}

template <typename I, typename T>
auto upper_bound (I f, I l, const T& v)
{
    while (f < l) {
	auto m = f + (l - f)/2;
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
auto lower_bound (const Container& c, typename Container::const_reference v)
    { return lower_bound (c.begin(), c.end(), v); }
template <typename Container>
auto upper_bound (const Container& c, typename Container::const_reference v)
    { return upper_bound (c.begin(), c.end(), v); }
template <typename Container>
auto binary_search (const Container& c, typename Container::const_reference v)
    { return binary_search (c.begin(), c.end(), v); }
template <typename Container>
auto linear_search (const Container& c, typename Container::const_reference v)
    { return linear_search (c.begin(), c.end(), v); }
template <typename Container>
void sort (Container& c)
    { sort (c.begin(), c.end()); }

} // namespace cwiclo
//}}}-------------------------------------------------------------------
