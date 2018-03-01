// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "memblock.h"
#include "stream.h"

namespace cwiclo {

template <typename T>
class vector {
public:
    using value_type		= T;
    using pointer		= value_type*;
    using const_pointer		= const value_type*;
    using reference		= value_type&;
    using const_reference	= const value_type&;
    using iterator		= pointer;
    using const_iterator	= const_pointer;
    using size_type		= memblock::size_type;
    using difference_type	= memblock::difference_type;
    using initlist_t		= std::initializer_list<value_type>;
public:
    inline			vector (void)			: _data() { }
    inline explicit		vector (size_type n)		: _data() { uninitialized_default_construct_n (insert_hole(begin(),n), n); }
    inline			vector (size_type n, const T& v): _data() { uninitialized_fill_n (insert_hole(begin(),n), n, v); }
    inline			vector (const vector& v)	: _data() { uninitialized_copy_n (v.begin(), v.size(), iterator(_data.insert(_data.begin(),v.bsize()))); }
    inline			vector (const_iterator i1, const_iterator i2) noexcept;
    template <size_type N>
    inline constexpr		vector (const T (&a)[N])	: _data (a, N*sizeof(T)) { static_assert (is_trivial<T>::value, "array ctor only works for trivial types"); }
    inline			vector (initlist_t v) noexcept;
    inline			vector (vector&& v)		: _data (move(v._data)) {}
    inline			~vector (void) noexcept		{ destroy (begin(), end()); }
    inline auto&		operator= (const vector& v)	{ assign (v); return *this; }
    inline auto&		operator= (vector&& v)		{ assign (move(v)); return *this; }
    inline auto&		operator= (initlist_t v)	{ assign (v); return *this; }
    bool			operator== (const vector& v) const noexcept;
    inline			operator const memlink& (void) const	{ return _data; }
    inline void			reserve (size_type n)		{ _data.reserve (n * sizeof(T)); }
    void			resize (size_type n) noexcept;
    void			resize (size_type n, const_reference v) noexcept;
    inline size_type		capacity (void) const		{ return _data.capacity() / sizeof(T);	}
    inline size_type		size (void) const		{ return _data.size() / sizeof(T);	}
    inline size_type		bsize (void) const		{ return _data.size();			}
    inline size_type		max_size (void) const		{ return _data.max_size() / sizeof(T);	}
    inline bool			empty (void) const		{ return _data.empty();			}
    inline auto			data (void)			{ return pointer (_data.data());	}
    inline auto			data (void) const		{ return const_pointer (_data.data());	}
    inline auto			begin (void)			{ return iterator (_data.begin());	}
    inline auto			begin (void) const		{ return const_iterator (_data.begin());}
    inline auto			cbegin (void) const		{ return begin();			}
    inline auto			end (void)			{ return iterator (_data.end());	}
    inline auto			end (void) const		{ return const_iterator (_data.end());	}
    inline auto			cend (void) const		{ return end();				}
    inline auto			iat (size_type i)		{ assert (i <= size()); return begin() + i; }
    inline auto			iat (size_type i) const		{ assert (i <= size()); return begin() + i; }
    inline auto			ciat (size_type i) const	{ assert (i <= size()); return cbegin() + i; }
    inline auto&		at (size_type i)		{ assert (i < size()); return begin()[i]; }
    inline auto&		at (size_type i) const		{ assert (i < size()); return begin()[i]; }
    inline auto&		cat (size_type i) const		{ assert (i < size()); return cbegin()[i]; }
    inline auto&		operator[] (size_type i)	{ return at (i); }
    inline auto&		operator[] (size_type i) const	{ return at (i); }
    inline auto&		front (void)			{ assert (!empty()); return at(0); }
    inline auto&		front (void) const		{ assert (!empty()); return at(0); }
    inline auto&		back (void)			{ assert (!empty()); return end()[-1]; }
    inline auto&		back (void) const		{ assert (!empty()); return end()[-1]; }
    inline void			clear (void)			{ destroy (begin(), end()); _data.clear(); }
    inline void			swap (vector&& v)		{ _data.swap (move(v._data)); }
    inline void			deallocate (void) noexcept	{ destroy (begin(), end()); _data.deallocate(); }
    inline void			shrink_to_fit (void) noexcept	{ _data.shrink_to_fit(); }
    inline void			assign (const vector& v)	{ assign (v.begin(), v.end()); }
    inline void			assign (vector&& v)		{ swap (v); }
    inline void			assign (const_iterator i1, const_iterator i2) noexcept;
    inline void			assign (size_type n,const T& v)		{ resize (n); fill (begin(), end(), v); }
    inline void			assign (initlist_t v)			{ assign (v.begin(), v.end()); }
    inline auto			insert (const_iterator ip)		{ return construct_at (insert_hole (ip, 1)); }
    inline auto			insert (const_iterator ip, const T& v)	{ return emplace (ip, v); }
    inline auto			insert (const_iterator ip, T&& v)	{ return emplace (ip, move(v)); }
    auto			insert (const_iterator ip, size_type n, const T& v) noexcept;
    auto			insert (const_iterator ip, const_iterator i1, const_iterator i2) noexcept;
    inline auto			insert (const_iterator ip, initlist_t v)	{ return insert (ip, v.begin(), v.end()); }
    template <typename... Args>
    inline auto			emplace (const_iterator ip, Args&&... args)	{ return construct_at (insert_hole(ip,1), forward<Args>(args)...); }
    template <typename... Args>
    inline auto&		emplace_back (Args&&... args)			{ return *emplace(end(), forward<Args>(args)...); }
    auto			erase (const_iterator ep, size_type n = 1) noexcept;
    inline auto			erase (const_iterator ep1, const_iterator ep2)	{ assert (ep1 <= ep2); return erase (ep1, ep2 - ep1); }
    auto			replace (const_iterator rp1, const_iterator rp2, const_iterator i1, const_iterator i2)
									{ return insert (erase (rp1, rp2), i1, i2); }
    inline auto&		push_back (const T& v)			{ return emplace_back (v); }
    inline auto&		push_back (T&& v)			{ return emplace_back (move(v)); }
    inline auto&		push_back (void)			{ return emplace_back(); }
    inline auto			append (const T& v)			{ return push_back(v); }
    inline auto			append (T&& v)				{ return push_back(move(v)); }
    inline auto			append (const_iterator i1, const_iterator i2)	{ return insert (end(), i1, i2); }
    inline auto			append (size_type n, const T& v)	{ return insert (end(), n, v); }
    inline auto			append (initlist_t v)			{ return append (v.begin(), v.end()); }
    inline void			pop_back (void)				{ destroy_at (end()-1); _data.memlink::resize (_data.size() - sizeof(T)); }
    inline void			manage (pointer p, size_type n)		{ _data.manage (p, n * sizeof(T)); }
    inline bool			is_linked (void) const			{ return !_data.capacity(); }
    inline void			unlink (void)				{ _data.unlink(); }
    inline void			copy_link (void) noexcept;
    inline void			link (const_pointer p, size_type n)	{ _data.link (memblock::const_pointer(p), n * sizeof(T)); }
    inline void			link (const vector& v)			{ _data.link (v); }
    inline void			link (const_pointer f, const_pointer l)	{ link (f, l-f); }
    void			read (istream& is) noexcept;
    void			write (ostream& os) const noexcept;
    inline void			write (sstream& os) const noexcept;
protected:
    inline auto			insert_hole (const_iterator ip, size_type n)
				    { return iterator (_data.insert (memblock::const_iterator(ip), n*sizeof(T))); }
private:
    memblock			_data;	///< Raw element data, consecutively stored.
};

//----------------------------------------------------------------------

} // namespace cwiclo
STREAM_ALIGN (cmemlink, stream_align<cmemlink::size_type>::value)
namespace cwiclo {

template <typename T> struct stream_align<vector<T>> {
    static constexpr const streamsize value = stream_align<memblock::size_type>::value;
};

//----------------------------------------------------------------------

/// Copies range [\p i1, \p i2]
template <typename T>
vector<T>::vector (const_iterator i1, const_iterator i2) noexcept
:_data()
{
    const auto n = i2-i1;
    uninitialized_copy_n (i1, n, insert_hole(begin(),n));
}

template <typename T>
vector<T>::vector (initlist_t v) noexcept
:_data()
{
    uninitialized_copy_n (v.begin(), v.size(), insert_hole(begin(),v.size()));
}

/// Resizes the vector to contain \p n elements.
template <typename T>
void vector<T>::resize (size_type n) noexcept
{
    reserve (n);
    auto ihfirst = end(), ihlast = begin()+n;
    destroy (ihlast, ihfirst);
    uninitialized_default_construct (ihfirst, ihlast);
    _data.memlink::resize (n*sizeof(T));
}

/// Resizes the vector to contain \p n elements made from \p v.
template <typename T>
void vector<T>::resize (size_type n, const_reference v) noexcept
{
    reserve (n);
    auto ihfirst = end(), ihlast = begin()+n;
    destroy (ihlast, ihfirst);
    uninitialized_fill (ihfirst, ihlast, v);
    _data.memlink::resize (n*sizeof(T));
}

/// Compares two vectors
template <typename T>
bool vector<T>::operator== (const vector& v) const noexcept
{
    if (is_trivial<T>::value)
	return _data == v._data;
    if (size() != v.size())
	return false;
    for (size_type i = 0; i < size(); ++i)
	if (!(at(i) == v.at(i)))
	    return false;
    return true;
}

/// Copies the range [\p i1, \p i2]
template <typename T>
void vector<T>::assign (const_iterator i1, const_iterator i2) noexcept
{
    assert (i1 <= i2);
    resize (i2 - i1);
    copy (i1, i2, begin());
}

/// Inserts \p n elements with value \p v at offsets \p ip.
template <typename T>
auto vector<T>::insert (const_iterator ip, size_type n, const T& v) noexcept
{
    auto ih = insert_hole (ip, n);
    uninitialized_fill_n (ih, n, v);
    return ih;
}

/// Inserts range [\p i1, \p i2] at offset \p ip.
template <typename T>
auto vector<T>::insert (const_iterator ip, const_iterator i1, const_iterator i2) noexcept
{
    assert (i1 <= i2);
    auto n = i2 - i1;
    auto ih = insert_hole (ip, n);
    uninitialized_copy_n (i1, n, ih);
    return ih;
}

/// Removes \p count elements at offset \p ep.
template <typename T>
auto vector<T>::erase (const_iterator cep, size_type n) noexcept
{
    auto ep = const_cast<iterator>(cep);
    destroy_n (ep, n);
    return iterator (_data.erase (memblock::iterator(ep), n * sizeof(T)));
}

template <typename T>
void vector<T>::copy_link (void) noexcept
{
    if (is_trivial<T>::value)
	_data.copy_link();
    else
	assign (begin(), end());
}

template <typename T>
void vector<T>::read (istream& is) noexcept
{
    if (stream_align<T>::value <= stream_align<size_type>::value && is_trivially_copyable<T>::value)
	return _data.read (is, sizeof(T));
    size_type n; is >> n;
    if (stream_align<T>::value > stream_align<size_type>::value)
	is >> ios::talign<T>();
    if (is_trivially_copyable<T>::value) {
	assert (is.remaining() >= n*sizeof(T));
	const T* d = is.ptr<T>();
	assign (d, d+n);
	is.skip (n*sizeof(T));
    } else {
	resize (n);
	for (auto& i : *this)
	    is >> i;
    }
    if (stream_align<T>::value < stream_align<size_type>::value)
	is >> ios::talign<size_type>();
}

template <typename T>
void vector<T>::write (ostream& os) const noexcept
{
    if (stream_align<T>::value <= stream_align<size_type>::value && is_trivially_copyable<T>::value)
	return _data.write (os, sizeof(T));
    os << size();
    if (stream_align<T>::value > stream_align<size_type>::value)
	os << ios::talign<T>();
    if (is_trivially_copyable<T>::value) {
	copy_n (begin(), size(), os.ptr<T>());
	os.skip (bsize());
    } else for (const auto& i : *this)
	os << i;
    if (stream_align<T>::value < stream_align<size_type>::value)
	os << ios::talign<size_type>();
}

template <typename T>
inline void vector<T>::write (sstream& ss) const noexcept
{
    if (stream_align<T>::value <= stream_align<size_type>::value && is_trivially_copyable<T>::value)
	return _data.write (ss, sizeof(T));
    ss << size();
    if (stream_align<T>::value > stream_align<size_type>::value)
	ss << ios::talign<T>();
    if (is_trivially_copyable<T>::value)
	ss.skip (bsize());
    else for (const auto& i : *this)
	ss << i;
    if (stream_align<T>::value < stream_align<size_type>::value)
	ss << ios::talign<size_type>();
}

} // namespace cwiclo
