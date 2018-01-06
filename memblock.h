// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "memory.h"

namespace cwiclo {

class alignas(16) memblock {
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
    inline constexpr		memblock (void)				: _data(nullptr), _size(), _capz() {}
    inline constexpr		memblock (pointer p, size_type n)	: _data(p), _size(n), _capz() {}
    inline constexpr		memblock (const_pointer p, size_type n)	: _data(const_cast<pointer>(p)), _size(n), _capz() {}
    inline			memblock (void* p, size_type n)		: memblock (reinterpret_cast<pointer>(p), n) {}
    inline			memblock (const void* p, size_type n)	: memblock (reinterpret_cast<const_pointer>(p), n) {}
    inline constexpr		memblock (const memblock& v)		: _data(v._data), _size(v._size), _capacity(), _zerot(v._zerot) {}
    inline			memblock (memblock&& v)			: _data(v._data), _size(v._size), _capz(v._capz) { v._capacity = 0; }
    inline auto&		operator= (const memblock& v)		{ link (v); return *this; }
    inline auto&		operator= (memblock&& v)		{ link (v); v.unlink(); return *this; }
    inline constexpr auto	max_size (void) const			{ return UINT32_MAX/2; }
    inline constexpr auto	size (void) const			{ return _size; }
    inline constexpr auto	empty (void) const			{ return !size(); }
    inline constexpr auto	capacity (void) const			{ return _capacity; }
    inline auto			data (void)				{ return _data; }
    inline constexpr auto	data (void) const			{ return _data; }
    inline constexpr auto	cdata (void) const			{ return _data; }
    inline auto			begin (void)				{ return data(); }
    inline constexpr auto	begin (void) const			{ return data(); }
    inline constexpr auto	cbegin (void) const			{ return begin(); }
    inline auto			end (void)				{ return begin()+size(); }
    inline constexpr auto	end (void) const			{ return begin()+size(); }
    inline constexpr auto	cend (void) const			{ return end(); }
    inline auto			iat (size_type i) const			{ assert (i <= size()); return begin() + i; }
    inline auto			ciat (size_type i) const		{ assert (i <= size()); return cbegin() + i; }
    inline auto&		at (size_type i) const			{ assert (i < size()); return begin()[i]; }
    inline auto&		operator[] (size_type i) const		{ return at (i); }
    inline void			link (pointer p, size_type n)		{ _data = p; _size = n; }
    inline void			link (const_pointer p, size_type n)	{ link (const_cast<pointer>(p), n); }
    inline void			link (const memblock& v)		{ link (v.begin(), v.size()); }
    inline void			unlink (void)				{ _data = nullptr; _size = 0; _capacity = 0; }
protected:
    inline bool			zero_terminated (void) const		{ return _zerot; }
    inline void			set_zero_terminated (void)		{ _zerot = 1; }
    inline void			set_capacity (size_type c)		{ _capacity = c; }
private:
    pointer			_data;			///< Pointer to the data block
    size_type			_size alignas(8);	///< Size of the data block. Aligning _size makes cmemlink 16 bytes on 32 and 64 bit platforms.
    union {
	size_type		_capz;
	struct {
	    size_type		_capacity:31;
	    size_type		_zerot:1;
	};
    };
};

} // namespace cwiclo
