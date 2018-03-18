// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "config.h"

//{{{ Type modifications -----------------------------------------------
namespace cwiclo {

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

template <typename T> struct remove_inner_const { using type = T; };
template <typename T> struct remove_inner_const<const T> { using type = T; };
template <typename T> struct remove_inner_const<const T*> { using type = T*; };
template <typename T> struct remove_inner_const<const T&> { using type = T&; };
template <typename T> using remove_inner_const_t = typename remove_inner_const<T>::type;

template <typename T> struct add_inner_const { using type = const T; };
template <typename T> struct add_inner_const<const T> { using type = const T; };
template <typename T> struct add_inner_const<const T*> { using type = const T*; };
template <typename T> struct add_inner_const<const T&> { using type = const T&; };
template <typename T> struct add_inner_const<T*> { using type = const T*; };
template <typename T> struct add_inner_const<T&> { using type = const T&; };
template <typename T> using add_inner_const_t = typename add_inner_const<T>::type;

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

// The weakest possible cast to an already convertible type
template <typename T> constexpr decltype(auto) implicit_cast (remove_reference_t<T>& v) { return v; }

// Create a passthrough non-const member function from a call to a const member function
#define UNCONST_MEMBER_FN(f,...)	\
    const_cast<remove_inner_const_t<decltype((const_cast<add_inner_const_t<decltype(this)>>(this)->f(__VA_ARGS__)))>>(const_cast<add_inner_const_t<decltype(this)>>(this)->f(__VA_ARGS__));

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
static inline NONNULL() auto strnext_r (const char* s, unsigned& n)
{
#if __x86__
    if (!compile_constant(strlen(s)))
	asm ("repnz\tscasb":"+D"(s),"+c"(n):"a"(0):"memory");
    else
#endif
    { auto l = strnlen(s, n); l += !!l; s += l; n -= l; }
    return s;
}
static inline NONNULL() auto strnext_r (char* s, unsigned& n)
    { return const_cast<char*>(strnext_r (const_cast<const char*>(s), n)); }
static inline NONNULL() auto strnext (const char* s)
    { unsigned n = UINT_MAX; return strnext_r(s,n); }
static inline NONNULL() auto strnext (char* s)
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

template <typename T> inline T le_to_native (const T& v) { return v; }
template <typename T> inline T be_to_native (const T& v) { return bswap (v); }
template <typename T> inline T native_to_le (const T& v) { return v; }
template <typename T> inline T native_to_be (const T& v) { return bswap (v); }

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

template <typename T>
inline constexpr bool is_negative (const T& v) {
    if constexpr (is_signed<T>::value)
	return v < 0;
    else
	return false;
}
template <typename T>
inline constexpr auto sign (T v)
    { return (0 < v) - is_negative(v); }
template <typename T>
inline constexpr make_unsigned_t<T> absv (T v)
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
template <typename T>
inline constexpr make_unsigned_t<T> Square (T n)
    { return n*n; }

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
//{{{ SIMD intrinsic types

// Intrinsic functions are typed for a specific vector type, such as
// float[4]. In cwiclo, pretty much all of the uses for SSE do bulk
// memory moves, for which strict typing is inconvenient and usually
// wrong anyway. This union is used to present appropriate type to
// intrinsic functions.

union alignas(16) simd16_t {
    using ui_t	= uint32_t	__attribute__((vector_size(16)));
    using si_t	= int32_t	__attribute__((vector_size(16)));
    using uq_t	= uint64_t	__attribute__((vector_size(16)));
    using sq_t	= int64_t	__attribute__((vector_size(16)));
    using sf_t	= float		__attribute__((vector_size(16)));
    using sd_t	= double	__attribute__((vector_size(16)));

    sf_t	sf;
    sd_t	sd;
    ui_t	ui;
    si_t	si;
    uq_t	uq;
    sq_t	sq;
    uint8_t	aub [16];
    int8_t	asb [16];
    uint16_t	auw [8];
    int16_t	asw [8];
    uint32_t	aui [4];
    int32_t	asi [4];
    uint64_t	auq [2];
    int64_t	asq [2];
    float	asf [4];
    double	asd [2];

    static inline CONST auto zero_sf (void) noexcept;
};

#if __clang__
namespace {

#if __AVX__
static inline auto __builtin_ia32_loadups (const float* f)
    { simd16_t::sf_t v; asm ("vmovups\t%1, %0":"=x"(v):"m"(*(const simd16_t::sf_t*) f)); return (v); }
static inline void __builtin_ia32_storeups (float* f, simd16_t::sf_t v)
    { asm ("vmovups\t%1, %0":"=m"(*(simd16_t::sf_t*) f):"x"(v)); }
static inline auto __builtin_ia32_xorps (simd16_t::sf_t v1, simd16_t::sf_t v2)
    { asm ("vxorps\t%1, %0, %0":"+x"(v1):"x"(v2)); return (v1); }
static inline auto __builtin_ia32_paddq128 (simd16_t::sf_t v1, simd16_t::sf_t v2)
    { asm ("vpaddq\t%1, %0, %0":"+x"(v1):"x"(v2)); return (v1); }
#elif __SSE2__
static inline auto __builtin_ia32_loadups (const float* f)
    { simd16_t::sf_t v; asm ("movups\t%1, %0":"=x"(v):"m"(*(const simd16_t::sf_t*) f)); return (v); }
static inline void __builtin_ia32_storeups (float* f, simd16_t::sf_t v)
    { asm ("movups\t%1, %0":"=m"(*(simd16_t::sf_t*) f):"x"(v)); }
static inline auto __builtin_ia32_xorps (simd16_t::sf_t v1, simd16_t::sf_t v2)
    { asm ("xorps\t%1, %0":"+x"(v1):"x"(v2)); return (v1); }
static inline auto __builtin_ia32_paddq128 (simd16_t::sf_t v1, simd16_t::sf_t v2)
    { asm ("paddq\t%1, %0":"+x"(v1):"x"(v2)); return (v1); }
#endif
static inline auto __builtin_ia32_movss (simd16_t::sf_t v1, simd16_t::sf_t v2)
    { v1[0] = v2[0]; return (v1); }

} // namespace
#endif // clang

/// Returns a zero-filled xmm register for use with the "x" constraint.
auto simd16_t::zero_sf (void) noexcept // static
{
    simd16_t z; asm("":"=x"(z.sf));	// asm forces use of undefined value in z.sf register
    return __builtin_ia32_xorps(z.sf,z.sf);
}

//}}}-------------------------------------------------------------------
//{{{ File utility functions

// Read ntr bytes from fd, accounting for partial reads and EINTR
inline int complete_read (int fd, char* p, size_t ntr) noexcept
{
    int nr = 0;
    while (ntr) {
	auto r = read (fd, p, ntr);
	if (r <= 0) {
	    if (errno == EINTR)
		continue;
	    return -1;
	}
	ntr -= r;
	nr += r;
	p += r;
    }
    return nr;
}

// Write ntw bytes to fd, accounting for partial writes and EINTR
inline int complete_write (int fd, const char* p, size_t ntw) noexcept
{
    int nw = 0;
    while (ntw) {
	auto r = write (fd, p, ntw);
	if (r <= 0) {
	    if (errno == EINTR)
		continue;
	    return -1;
	}
	ntw -= r;
	nw += r;
	p += r;
    }
    return nw;
}

#ifndef UC_VERSION
extern "C" const char* executable_in_path (const char* efn, char* exe, size_t exesz) noexcept NONNULL();
extern "C" int mkpath (const char* path, mode_t mode) noexcept NONNULL();
extern "C" int rmpath (const char* path) noexcept NONNULL();
#endif

enum { SD_LISTEN_FDS_START = STDERR_FILENO+1 };
extern "C" unsigned sd_listen_fds (void) noexcept;

} // namespace cwiclo
//}}}-------------------------------------------------------------------
