// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "memory.h"

namespace cwiclo {

class istream;
class ostream;
class sstream;

class alignas(16) cmemlink {
public:
    using value_type		= char;
    using pointer		= value_type*;
    using const_pointer		= const value_type*;
    using reference		= value_type&;
    using const_reference	= const value_type&;
    using size_type		= uint32_t;
    using difference_type	= ptrdiff_t;
    using const_iterator	= const_pointer;
    using iterator		= pointer;
public:
    inline constexpr		cmemlink (void)				: _data(), _size(), _capz() {}
    inline constexpr		cmemlink (pointer p, size_type n)	: _data(p), _size(n), _capz() {}
    inline constexpr		cmemlink (const_pointer p, size_type n)	: cmemlink (const_cast<pointer>(p), n) {}
    inline constexpr		cmemlink (pointer p, size_type n, bool z)	: _data(p), _size(n), _zerot(z), _capacity(0) {}
    inline constexpr		cmemlink (const_pointer p, size_type n, bool z)	: cmemlink (const_cast<pointer>(p),n,z) {}
    inline			cmemlink (void* p, size_type n)		: cmemlink (reinterpret_cast<pointer>(p), n) {}
    inline			cmemlink (const void* p, size_type n)	: cmemlink (reinterpret_cast<const_pointer>(p), n) {}
    inline constexpr		cmemlink (const cmemlink& v)		: _data(v._data), _size(v._size), _zerot(v._zerot), _capacity() {}
    inline			cmemlink (cmemlink&& v)			: _data(v._data), _size(v._size), _capz(v._capz) { v._capacity = 0; }
    inline auto&		operator= (const cmemlink& v)		{ link (v); return *this; }
    inline auto&		operator= (cmemlink&& v)		{ swap (move(v)); return *this; }
    inline constexpr auto	max_size (void) const			{ return numeric_limits<size_type>::max()/2-1; }
    inline constexpr auto	size (void) const			{ return _size; }
    inline constexpr auto	empty (void) const			{ return !size(); }
    inline constexpr auto	capacity (void) const			{ return _capacity; }
    inline constexpr auto	data (void) const			{ return _data; }
    inline constexpr auto	cdata (void) const			{ return _data; }
    inline constexpr auto	begin (void) const			{ return data(); }
    inline constexpr auto	cbegin (void) const			{ return begin(); }
    inline constexpr auto	end (void) const			{ return begin()+size(); }
    inline constexpr auto	cend (void) const			{ return end(); }
    inline constexpr auto	iat (size_type i) const			{ assert (i <= size()); return begin() + i; }
    inline constexpr auto	ciat (size_type i) const		{ assert (i <= size()); return cbegin() + i; }
    inline auto&		at (size_type i) const			{ assert (i < size()); return begin()[i]; }
    inline auto&		operator[] (size_type i) const		{ return at (i); }
    inline bool			operator== (const cmemlink& v) const	{ return size() == v.size() && 0 == memcmp (data(), v.data(), size()); }
    inline void			link (pointer p, size_type n)		{ _data = p; _size = n; }
    inline void			link (const_pointer p, size_type n)	{ link (const_cast<pointer>(p), n); }
    inline void			link (pointer p, size_type n, bool z)		{ link(p,n); _zerot = z; }
    inline void			link (const_pointer p, size_type n, bool z)	{ link (const_cast<pointer>(p), n, z); }
    inline void			link (const cmemlink& v)			{ link (v.begin(), v.size(), v.zero_terminated()); }
    inline void			unlink (void)				{ _data = nullptr; _size = 0; _capacity = 0; }
    void			swap (cmemlink&& v)			{ ::cwiclo::swap(_data, v._data); ::cwiclo::swap(_size, v._size); ::cwiclo::swap(_capz,v._capz); }
    inline void			resize (size_type sz)			{ _size = sz; }
    inline void			clear (void)				{ resize(0); }
    void			link_read (istream& is, size_type elsize = sizeof(value_type)) noexcept;
    inline void			read (istream& is, size_type elsize = sizeof(value_type))	{ link_read (is, elsize); }
    void			write (ostream& os, size_type elsize = sizeof(value_type)) const noexcept;
    void			write (sstream& os, size_type elsize = sizeof(value_type)) const noexcept;
protected:
    inline constexpr auto	dataw (void)				{ return _data; }
    inline constexpr bool	zero_terminated (void) const		{ return _zerot; }
    inline void			set_capacity (size_type c)		{ _capacity = c; }
private:
    pointer			_data;			///< Pointer to the data block
    size_type			_size alignas(8);	///< Size of the data block. Aligning _size makes ccmemlink 16 bytes on 32 and 64 bit platforms.
    union {
	size_type		_capz;
	struct {
	    bool		_zerot:1;
	    size_type		_capacity:31;
	};
    };
};

//----------------------------------------------------------------------

class memlink : public cmemlink {
public:
				using cmemlink::cmemlink;
				using cmemlink::data;
				using cmemlink::begin;
				using cmemlink::end;
				using cmemlink::iat;
				using cmemlink::at;
    inline constexpr		memlink (const memlink& v)		: cmemlink(v) {}
    inline			memlink (memlink&& v)			: cmemlink(move(v)) {}
    inline constexpr auto	data (void)				{ return dataw(); }
    inline constexpr auto	begin (void)				{ return data(); }
    inline constexpr auto	end (void)				{ return begin()+size(); }
    inline constexpr auto	iat (size_type i)			{ assert (i <= size()); return begin() + i; }
    inline auto&		at (size_type i)			{ assert (i < size()); return begin()[i]; }
    inline auto&		operator= (const memlink& v)		{ cmemlink::operator=(v); return *this; }
    inline auto&		operator= (memlink&& v)			{ cmemlink::operator=(move(v)); return *this; }
    inline auto&		operator[] (size_type i)		{ return at (i); }
    inline auto&		operator[] (size_type i) const		{ return at (i); }
    void			swap (memlink&& v)			{ cmemlink::swap(move(v)); }
    iterator			insert (const_iterator start, size_type n) noexcept;
    iterator			erase (const_iterator start, size_type n) noexcept;
};

//----------------------------------------------------------------------

class memblock : public memlink {
public:
				using memlink::memlink;
				memblock (size_type sz) noexcept	: memlink() { resize (sz); }
				memblock (const memblock& v)		: memlink(v) {}
				memblock (memblock&& v)			: memlink(move(v)) {}
				~memblock (void) noexcept		{ deallocate(); }
    inline void			manage (pointer p, size_type n)		{ assert(!capacity() && "unlink or deallocate first"); link (p, n); set_capacity(n); }
    void			assign (const_pointer p, size_type n) noexcept;
    inline void			assign (const memblock& v) noexcept	{ assign (v.data(), v.size()); }
    inline void			assign (memblock&& v) noexcept		{ swap (move(v)); }
    inline memblock&		operator= (const memblock& v) noexcept	{ assign (v); return *this; }
    inline memblock&		operator= (memblock&& v) noexcept	{ assign (move(v)); return *this; }
    void			swap (memblock&& v) noexcept		{ memlink::swap (move(v)); }
    void			reserve (size_type sz) noexcept;
    void			resize (size_type sz) noexcept;
    inline void			clear (void)				{ resize(0); }
    inline void			copy_link (void) noexcept		{ resize (size()); }
    iterator			insert (const_iterator start, size_type n) noexcept;
    iterator			erase (const_iterator start, size_type n) noexcept;
    void			shrink_to_fit (void) noexcept;
    void			deallocate (void) noexcept;
    void			read (istream& is, size_type elsize = sizeof(value_type)) noexcept;
};

//----------------------------------------------------------------------

/// Use with memlink-derived classes to link to a static array
#define static_link(v)	link (ArrayBlock(v))
/// Use with memlink-derived classes to allocate and link to stack space.
#define alloca_link(n)	link (alloca(n), (n))

} // namespace cwiclo
