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
    inline constexpr		memblock (std::initializer_list<value_type> v)	: memblock (v.begin(), v.size()*sizeof(value_type)) {}
    inline auto			begin (void) const			{ return _data; }
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
