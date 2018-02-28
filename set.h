// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "vector.h"

namespace cwiclo {

template <typename T>
class set : public vector<T> {
public:
    using vecbase = vector<T>;
    using typename vecbase::value_type;
    using typename vecbase::reference;
    using typename vecbase::const_reference;
    using typename vecbase::pointer;
    using typename vecbase::const_pointer;
    using typename vecbase::iterator;
    using typename vecbase::const_iterator;
    using typename vecbase::size_type;
    using typename vecbase::difference_type;
    using typename vecbase::initlist_t;
public:
			using vecbase::vector;
			using vecbase::insert;
    inline		set (const set& v)	: vecbase(v) {}
    inline		set (const vecbase& v)	: vecbase(v) { sort(*this); }
    template <size_type N>
    inline constexpr	set (const T (&a)[N])	: vecbase(a) { sort(*this); }
    inline		set (initlist_t v)	: vecbase(v) { sort(*this); }
    inline		set (set&& v)		: vecbase(move(v)) {}
    inline		set (const_iterator i1, const_iterator i2)	: vecbase(i1,i2) { sort(*this); }
    inline void		assign (const_iterator i1, const_iterator i2)	{ vecbase::assign(i1,i2); sort(*this); }
    inline void		assign (const set& v)		{ vecbase::assign(v); }
    inline void		assign (const vecbase& v)	{ vecbase::assign(v); sort(*this); }
    inline void		assign (set&& v)		{ vecbase::assign(move(v)); }
    inline void		assign (vecbase&& v)		{ vecbase::assign(move(v)); sort(*this); }
    inline void		assign (size_type n,const T& v)	{ vecbase::assign(n,v); }
    inline void		assign (initlist_t v)		{ vecbase::assign(v); sort(*this); }
    inline auto&	operator= (const set& v)	{ assign (v); return *this; }
    inline auto&	operator= (const vecbase& v)	{ assign (v); return *this; }
    inline auto&	operator= (set&& v)		{ assign (move(v)); return *this; }
    inline auto&	operator= (vecbase&& v)		{ assign (move(v)); return *this; }
    inline auto&	operator= (initlist_t v)	{ assign (v); return *this; }
    template <typename U>
    inline auto		find (const U& v) const		{ return binary_search (this->begin(), this->end(), v); }
    template <typename U>
    inline auto		lower_bound (const U& v) const	{ return ::cwiclo::lower_bound (this->begin(), this->end(), v); }
    template <typename U>
    inline auto		upper_bound (const U& v) const	{ return ::cwiclo::upper_bound (this->begin(), this->end(), v); }
    template <typename U>
    inline auto		find (const U& v)		{ return const_cast<iterator>(const_cast<const set&>(*this).find (v)); }
    template <typename U>
    inline auto		lower_bound (const U& v)	{ return const_cast<iterator>(const_cast<const set&>(*this).lower_bound (v)); }
    template <typename U>
    inline auto		upper_bound (const U& v)	{ return const_cast<iterator>(const_cast<const set&>(*this).upper_bound (v)); }
    auto		insert (const_reference v) noexcept;
    auto		insert (T&& v) noexcept;
    inline void		insert (const_iterator i1, const_iterator i2)	{ for (; i1 < i2; ++i1) insert (*i1); }
    inline void		insert (initlist_t v)			{ insert (v.begin(), v.end()); }
    template <typename... Args>
    auto		emplace (Args&&... args) noexcept;
    inline void		erase (const_reference v) noexcept;
    inline auto		erase (const_iterator ep)		{ return vecbase::erase(ep); }
    inline auto		erase (const_iterator ep1, const_iterator ep2) { return vecbase::erase (ep1, ep2); }
};

/// Inserts \p v into the container, maintaining the sort order.
template <typename T>
auto set<T>::insert (const_reference v) noexcept
{
    auto ip = lower_bound (v);
    if (ip == this->end() || v < *ip)
	ip = vecbase::insert (ip, v);
    else
	*ip = v;
    return ip;
}

/// Move-inserts \p v into the container, maintaining the sort order.
template <typename T>
auto set<T>::insert (T&& v) noexcept
{
    auto ip = lower_bound (v);
    if (ip == this->end() || v < *ip)
	ip = vecbase::insert (ip, move(v));
    else
	*ip = move(v);
    return ip;
}

/// In-place creates an element
template <typename T>
template <typename... Args>
auto set<T>::emplace (Args&&... args) noexcept
{
    auto e = &vecbase::emplace_back (forward<Args>(args)...),
	iend = this->end(), ilast = iend-1,
	ip = ::cwiclo::lower_bound (this->begin(), ilast, *e);
    if (ip == ilast || *e < *ip)	// New value
	return rotate (ip, ilast, iend);
    *ip = *e;	// Replace existing value
    vecbase::pop_back();
    return ip;
}

/// Erases the element with value \p v.
template <typename T>
void set<T>::erase (const_reference v) noexcept
{
    auto ip = find (v);
    if (ip)
	erase (ip);
}

} // namespace cwiclo
