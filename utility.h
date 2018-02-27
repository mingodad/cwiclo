// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "config.h"
namespace cwiclo {

//{{{ Type modifications -----------------------------------------------

/// true or false templatized constant for metaprogramming
template <typename T, T v>
struct integral_constant {
    using value_type = T;
    using type = integral_constant<value_type,v>;
    static constexpr const value_type value = v;
    constexpr operator value_type() const { return value; }
    constexpr auto operator()() const { return value; }
};

using true_type = integral_constant<bool, true>;
using false_type = integral_constant<bool, false>;

template <typename T, typename U> struct is_same : public false_type {};
template <typename T> struct is_same<T,T> : public true_type {};

template <typename T> struct remove_reference		{ using type = T; };
template <typename T> struct remove_reference<T&>	{ using type = T; };
template <typename T> struct remove_reference<T&&>	{ using type = T; };
template <typename T> using remove_reference_t = typename remove_reference<T>::type;

template <typename T> T&& declval (void) noexcept;

template <typename T> struct is_trivial : public integral_constant<bool, __is_trivial(T)> {};
template <typename T> struct is_trivially_constructible : public integral_constant<bool, __has_trivial_constructor(T)> {};
template <typename T> struct is_trivially_destructible : public integral_constant<bool, __has_trivial_destructor(T)> {};
template <typename T> struct is_trivially_copyable : public integral_constant<bool, __has_trivial_copy(T)> {};

template <typename T> struct make_unsigned	{ using type = T; };
template <> struct make_unsigned<char>		{ using type = unsigned char; };
template <> struct make_unsigned<short>		{ using type = unsigned short; };
template <> struct make_unsigned<int>		{ using type = unsigned int; };
template <> struct make_unsigned<long>		{ using type = unsigned long; };
template <typename T> using make_unsigned_t = typename make_unsigned<T>::type;

template <typename T> struct is_signed : public integral_constant<bool, !is_same<T,make_unsigned_t<T>>::value> {};

template <typename T> struct bits_in_type	{ static constexpr const size_t value = sizeof(T)*8; };

//}}}-------------------------------------------------------------------
//{{{ numeric limits

template <typename T>
struct numeric_limits {
private:
    using base_type = remove_reference_t<T>;
public:
    static constexpr const bool is_signed = ::cwiclo::is_signed<T>::value;	///< True if the type is signed.
    static constexpr const bool is_integral = is_trivial<T>::value;	///< True if fixed size and cast-copyable.
    static inline constexpr auto min (void)	{ return is_signed ? base_type(1)<<(bits_in_type<base_type>::value-1) : base_type(0); }
    static inline constexpr auto max (void)	{ return base_type(min()-1); }
};

//}}}-------------------------------------------------------------------
//{{{ Array macros

/// Returns the number of elements in a static vector
template <typename T, size_t N> constexpr inline auto ArraySize (T(&a)[N])
{
    static_assert (sizeof(a), "C++ forbids zero-size arrays");
    return N;
}
/// Returns the end() for a static vector
template <typename T, size_t N> constexpr inline auto ArrayEnd (T(&a)[N])
{ return &a[ArraySize(a)]; }
/// Expands into a ptr,ArraySize expression for the given static vector; useful as link arguments.
#define ArrayBlock(v)	&(v)[0], ArraySize(v)
/// Expands into a begin,end expression for the given static vector; useful for algorithm arguments.
#define ArrayRange(v)	&(v)[0], ArrayEnd(v)

/// Expands into a ptr,sizeof expression for the given static data block
#define DataBlock(v)	&(v)[0], sizeof(v)

/// Shorthand for container iteration.
#define foreach(i,ctr)	for (auto i = (ctr).begin(); i < (ctr).end(); ++i)
/// Shorthand for container reverse iteration.
#define eachfor(i,ctr)	for (auto i = (ctr).end(); i-- > (ctr).begin();)

/// Returns s+strlen(s)+1
static inline NONNULL() const char* strnext_r (const char* s, unsigned& n)
{
#if __x86__
    if (!compile_constant(strlen(s)))
	asm ("repnz\tscasb":"+D"(s),"+c"(n):"a"(0):"memory");
    else
#endif
    { auto l = strnlen(s, n); l += !!l; s += l; n -= l; }
    return s;
}
static inline NONNULL() const char* strnext (const char* s)
    { unsigned n = UINT_MAX; return strnext_r(s,n); }

template <typename T> inline constexpr auto advance (T* p, ptrdiff_t n) { return p + n; }
template <> inline auto advance (void* p, ptrdiff_t n) { return advance ((char*)p, n); }
template <> inline auto advance (const void* p, ptrdiff_t n) { return advance ((const char*)p, n); }

template <typename T> inline constexpr auto distance (T* f, T* l) { return l-f; }
template <> inline auto distance (void* f, void* l) { return distance ((char*)f, (char*)l); }
template <> inline auto distance (const void* f, const void* l) { return distance ((const char*)f, (const char*)l); }

//}}}----------------------------------------------------------------------
//{{{ bswap

template <typename T>
inline T bswap (T v) { assert (!"Only integer types are swappable"); return v; }
template <> inline uint8_t bswap (uint8_t v)	{ return v; }
template <> inline uint16_t bswap (uint16_t v)	{ return __builtin_bswap16 (v); }
template <> inline uint32_t bswap (uint32_t v)	{ return __builtin_bswap32 (v); }
template <> inline uint64_t bswap (uint64_t v)	{ return __builtin_bswap64 (v); }
template <> inline int8_t bswap (int8_t v)	{ return v; }
template <> inline int16_t bswap (int16_t v)	{ return __builtin_bswap16 (v); }
template <> inline int32_t bswap (int32_t v)	{ return __builtin_bswap32 (v); }
template <> inline int64_t bswap (int64_t v)	{ return __builtin_bswap64 (v); }

//}}}----------------------------------------------------------------------
//{{{ min and max

template <typename T>
inline constexpr auto min (const T& a, const remove_reference_t<T>& b)
    { return a < b ? a : b; }
template <typename T>
inline constexpr auto max (const T& a, const remove_reference_t<T>& b)
    { return b < a ? a : b; }

//}}}----------------------------------------------------------------------
//{{{ sign and absv

namespace {
    template <typename T, bool Signed>
    struct __is_negative { inline constexpr bool operator()(const T& v) { return v < 0; } };
    template <typename T>
    struct __is_negative<T,false> { inline constexpr bool operator()(const T&) { return false; } };
}
template <typename T>
inline constexpr bool is_negative (const T& v)
    { return __is_negative<T,is_signed<T>::value>()(v); }
template <typename T>
inline constexpr auto sign (T v)
    { return (0 < v) - is_negative(v); }
template <typename T>
inline constexpr make_unsigned<T> absv (T v)
    { return is_negative(v) ? -v : v; }
template <typename T>
inline constexpr T MultBySign (T a, remove_reference_t<T> b)
    { return is_negative<T>(b) ? -a : a; }

//}}}----------------------------------------------------------------------
//{{{ Align

enum { c_DefaultAlignment = alignof(void*) };

template <typename T>
inline constexpr T Floor (T n, remove_reference_t<T> grain = c_DefaultAlignment)
    { return n - n % grain; }
template <typename T>
inline constexpr auto Align (T n, remove_reference_t<T> grain = c_DefaultAlignment)
    { return Floor<T> (n + MultBySign<T> (grain-1, n), grain); }
template <typename T>
inline constexpr bool IsAligned (T n, remove_reference_t<T> grain = c_DefaultAlignment)
    { return !(n % grain); }
template <typename T>
inline constexpr auto Round (T n, remove_reference_t<T> grain)
    { return Floor<T> (n + MultBySign<T> (grain/2, n), grain); }
template <typename T>
inline constexpr auto DivRU (T n1, remove_reference_t<T> n2)
    { return (n1 + MultBySign<T> (n2-1, n1)) / n2; }
template <typename T>
inline constexpr auto DivRound (T n1, remove_reference_t<T> n2)
    { return (n1 + MultBySign<T> (n2/2, n1)) / n2; }

//}}}----------------------------------------------------------------------
//{{{ Bit manipulation

template <typename T>
constexpr inline bool GetBit (T v, unsigned i)
    { return v&T(T(1)<<i); }
template <typename T>
inline void SetBit (T& v, unsigned i, bool b=true)
    { T m(T(T(1)<<i)); v=b?T(v|m):T(v&~m); }
template <typename T>
inline constexpr auto Rol (T v, remove_reference_t<T> n)
    { return (v << n) | (v >> (bits_in_type<T>::value-n)); }
template <typename T>
inline constexpr auto Ror (T v, remove_reference_t<T> n)
    { return (v >> n) | (v << (bits_in_type<T>::value-n)); }

template <typename T>
inline auto FirstBit (T v, remove_reference_t<T> nbv)
{
    auto n = nbv;
#if __x86__
    if (!compile_constant(v)) asm ("bsr\t%1, %0":"+r"(n):"rm"(v)); else
#endif
    if (v) n = (bits_in_type<T>::value-1) - __builtin_clz(v);
    return n;
}

template <typename T>
inline T NextPow2 (T v)
{
    T r = v-1;
#if __x86__
    if (!compile_constant(r)) asm("bsr\t%0, %0":"+r"(r)); else
#endif
    { r = FirstBit(r,r); if (r >= bits_in_type<T>::value-1) r = T(-1); }
    return 1<<(1+r);
}

template <typename T>
inline constexpr bool IsPow2 (T v)
    { return !(v&(v-1)); }

//}}}----------------------------------------------------------------------
//{{{ atomic_flag

enum memory_order {
    memory_order_relaxed = __ATOMIC_RELAXED,
    memory_order_consume = __ATOMIC_CONSUME,
    memory_order_acquire = __ATOMIC_ACQUIRE,
    memory_order_release = __ATOMIC_RELEASE,
    memory_order_acq_rel = __ATOMIC_ACQ_REL,
    memory_order_seq_cst = __ATOMIC_SEQ_CST
};

namespace {

// Use in lock wait loops to relax the CPU load
static inline void tight_loop_pause (void)
{
    #if __x86__
	#if __clang__
	    __asm__ volatile ("rep nop");
	#else
	    __builtin_ia32_pause();
	#endif
    #else
	usleep (1);
    #endif
}

template <typename T>
static inline T kill_dependency (T v) noexcept
    { return T(v); }
static inline void atomic_thread_fence (memory_order order) noexcept
    { __atomic_thread_fence (order); }
static inline void atomic_signal_fence (memory_order order) noexcept
    { __atomic_signal_fence (order); }

} // namespace

class atomic_flag {
    bool		_v;
public:
			atomic_flag (void) = default;
    inline constexpr	atomic_flag (bool v)	: _v(v) {}
			atomic_flag (const atomic_flag&) = delete;
    atomic_flag&	operator= (const atomic_flag&) = delete;
    void		clear (memory_order order = memory_order_seq_cst)
			    { __atomic_clear (&_v, order); }
    bool		test_and_set (memory_order order = memory_order_seq_cst)
			    { return __atomic_test_and_set (&_v, order); }
};
#define ATOMIC_FLAG_INIT	{false}

class atomic_scope_lock {
    atomic_flag& _f;
public:
    explicit atomic_scope_lock (atomic_flag& f) noexcept : _f(f) { while (_f.test_and_set()) tight_loop_pause(); }
    ~atomic_scope_lock (void) noexcept { _f.clear(); }
};

//}}}-------------------------------------------------------------------

} // namespace cwiclo
