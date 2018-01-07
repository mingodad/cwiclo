// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "memblock.h"

//{{{ Stream-related types ---------------------------------------------
namespace cwiclo {

using streamsize = cmemlink::size_type;
using streampos = streamsize;

//}}}-------------------------------------------------------------------
//{{{ istream

class istream {
public:
    using const_pointer		= cmemlink::const_pointer;
    enum { is_reading = true, is_writing = false, is_sizing = false };
public:
    inline constexpr		istream (const_pointer p, const_pointer e)	: _p(p),_e(e) {}
    inline constexpr		istream (const_pointer p, streamsize sz)	: istream(p,p+sz) {}
    inline constexpr		istream (const cmemlink& m)			: istream(m.data(),m.size()) {}
    inline constexpr auto	end (void) const	{ return _e; }
    inline constexpr streamsize	remaining (void) const	{ return end()-_p; }
    template <typename T>
    inline auto			ptr (void) const	{ return reinterpret_cast<const T*>(_p); }
    inline void			skip (streamsize sz)	{ seek (_p + sz); }
    inline void			align (streamsize g)	{ seek (const_pointer (Align (uintptr_t(_p), g))); }
    inline void			read (void* __restrict__ p, streamsize sz) __restrict__ {
				    assert (remaining() >= sz);
				    memcpy (p, _p, sz); skip(sz);
				}
    const char*			read_strz (void) {
				    const char* __restrict__ v = ptr<char>();
				    auto se = (const_pointer) memchr (v, 0, remaining());
				    if (!se)
					return nullptr;
				    seek (se+1);
				    return v;
				}
    template <typename T>
    inline void			readv (T& v) __restrict__ {
				    const T* __restrict__ p = ptr<T>();
				    v = *p;
				    skip(sizeof(T));
				}
    template <typename T>
    inline istream&		operator>> (T& v);
protected:
    inline void			seek (const_pointer p)	{ assert(p <= end()); _p = p; }
private:
    const_pointer		_p;
    const const_pointer		_e;
};

//}}}-------------------------------------------------------------------
//{{{ ostream

class ostream {
public:
    using pointer		= cmemlink::pointer;
    using const_pointer		= cmemlink::const_pointer;
    enum { is_reading = false, is_writing = true, is_sizing = false };
public:
    inline constexpr		ostream (pointer p, const_pointer e)	: _p(p),_e(e) {}
    inline constexpr		ostream (pointer p, streamsize sz)	: ostream(p,p+sz) {}
    inline constexpr		ostream (memlink& m)			: ostream(m.data(),m.size()) {}
    inline constexpr auto	end (void) const	{ return _e; }
    inline constexpr streamsize	remaining (void) const	{ return end()-_p; }
    template <typename T>
    inline T*			ptr (void)		{ return reinterpret_cast<T*>(_p); }
    inline void			skip (streamsize sz) {
				    pointer __restrict__ p = _p;
				    for (auto i = 0u; i < sz; ++i) {
					assert (p+1 < end());
					*p++ = 0;
				    }
				    _p = p;
				}
    inline void			align (streamsize g) {
				    pointer __restrict__ p = _p;
				    while (uintptr_t(p) % g) {
					assert (p+1 < end());
					*p++ = 0;
				    }
				    _p = p;
				}
    inline void			write (const void* __restrict__ p, streamsize sz) __restrict__ {
				    assert (remaining() >= sz);
				    _p = (pointer) mempcpy (_p, p, sz);
				}
    inline void			write_strz (const char* s)	{ write (s, strlen(s)+1); }
    template <typename T>
    inline void			writev (const T& v) __restrict__ {
				    assert(remaining() >= sizeof(T));
				    T* __restrict__ p = ptr<T>(); *p = v;
				    _p += sizeof(T);
				}
    template <typename T>
    inline ostream&		operator<< (const T& v);
protected:
    inline void			seek (pointer p)	{ assert(p < end()); _p = p; }
private:
    pointer	_p;
    const const_pointer		_e;
};

//}}}-------------------------------------------------------------------
//{{{ sstream

class sstream {
public:
    using const_pointer		= cmemlink::const_pointer;
    enum { is_reading = false, is_writing = false, is_sizing = true };
public:
    inline constexpr		sstream (void)		: _sz() {}
    inline constexpr auto	size (void) const	{ return _sz; }
    inline constexpr streamsize	remaining (void) const	{ return UINT32_MAX; }
    inline void			skip (streamsize sz)	{ _sz += sz; }
    inline void			align (streamsize g)	{ _sz = Align (_sz, g); }
    inline void			write (const void*, streamsize sz) { skip (sz); }
    inline void			write_strz (const char* s) { write (s, strlen(s)+1); }
    template <typename T>
    inline void			writev (const T& v)	{ write (&v, sizeof(v)); }
    template <typename T>
    inline sstream&		operator<< (const T& v);
private:
    streampos			_sz;
};

//}}}-------------------------------------------------------------------
//{{{ Stream operators << and >>

template <typename T, bool Trivial>
struct type_streaming {
    static inline void read (istream& is, T& v) { v.read (is); }
    static inline void write (ostream& os, const T& v) { v.write (os); }
    static inline void size (sstream& ss, const T& v) { v.write (ss); }
};
template <typename T>
struct type_streaming<T,true> {
    static inline void read (istream& is, T& v) { is.readv(v); }
    static inline void write (ostream& os, const T& v) { os.writev(v); }
    static inline void size (sstream& ss, const T& v) { ss.writev(v); }
};
template <typename T>
using type_streaming_t = type_streaming<remove_reference_t<T>,is_trivial<remove_reference_t<T>>::value>;

template <typename T>
istream& istream::operator>> (T& v)
    { type_streaming_t<T>::read (*this, v); return *this; }
template <typename T>
ostream& ostream::operator<< (const T& v)
    { type_streaming_t<T>::write (*this, v); return *this; }
template <typename T>
sstream& sstream::operator<< (const T& v)
    { type_streaming_t<T>::size (*this, v); return *this; }

//}}}-------------------------------------------------------------------
//{{{ stream_size_of and stream_align

/// Returns the size of the given object. Do not overload - use sstream.
template <typename T>
inline auto stream_size_of (const T& v) { sstream ss; ss << v; return ss.size(); }

/// Returns the recommended stream alignment for type \p T. Override with STREAM_ALIGN.
template <typename T>
struct stream_align { static constexpr const streamsize value = alignof(T); };

template <typename T>
inline constexpr auto stream_align_of (const T&) { return stream_align<T>::value; }

#define STREAM_ALIGN(type,grain)	\
    template <> struct stream_align<type> { static constexpr const streamsize value = grain; }

//}}}-------------------------------------------------------------------
//{{{ stream operators

namespace ios {

/// Stream functor to allow inline align() calls.
class align {
public:
    constexpr explicit	align (streamsize grain = c_DefaultAlignment) : _grain(grain) {}
    inline void		read (istream& is) const	{ is.align (_grain); }
    inline void		write (ostream& os) const	{ os.align (_grain); }
    inline void		write (sstream& ss) const	{ ss.align (_grain); }
private:
    const streamsize	_grain;
};

/// Stream functor to allow type-based alignment.
template <typename T>
class talign : public align {
public:
    constexpr explicit	talign (void) : align (stream_align<T>::value) {}
};

/// Stream functor to allow inline skip() calls.
class skip {
public:
    constexpr explicit 	skip (streamsize n)		: _n(n) {}
    inline void		read (istream& is) const	{ is.skip (_n); }
    inline void		write (ostream& os) const	{ os.skip (_n); }
    inline void		write (sstream& ss) const	{ ss.skip (_n); }
private:
    const streamsize	_n;
};

auto& operator>> (istream& is, const align& v) { v.read (is); return is; }
auto& operator>> (istream& is, const skip& v) { v.read (is); return is; }

} // namespace ios
} // namespace cwiclo
//}}}-------------------------------------------------------------------
