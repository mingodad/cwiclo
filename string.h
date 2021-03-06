// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "memblock.h"

namespace cwiclo {

class lstring;

class string : public memblock {
public:
    inline		string (void)					: memblock() { set_zero_terminated(); }
    inline		string (const_pointer s, size_type len)		: memblock (s,len,true) { copy_link(); }
    inline		string (const_pointer s1, const_pointer s2)	: string (s1,s2-s1) {}
    inline		string (const_pointer s)			: memblock (s, strlen(s), true) {}
    inline constexpr	string (string&& s)				: memblock (move(s)) {}
    inline		string (const string& s)			: string (s.data(), s.size()) {}
    constexpr auto	c_str (void) const				{ assert ((!end() || !*end()) && "This string is linked to data that is not 0-terminated. This may cause serious security problems. Please assign the data instead of linking."); return data(); }
    constexpr auto&	back (void) const				{ return at(size()-1); }
    inline auto&	back (void)					{ return at(size()-1); }
    inline void		push_back (const_reference c)			{ resize(size()+1); back() = c; }
    auto		insert (const_iterator ip, const_reference c, size_type n = 1)	{ return fill_n (memblock::insert (ip, n), n, c); }
    inline auto		insert (const_iterator ip, const_pointer s, size_type n)	{ return memblock::insert (ip, s, n); }
    inline auto		insert (const_iterator ip, const string& s)			{ return insert (ip, s.c_str(), s.size()); }
    inline auto		insert (const_iterator ip, const_pointer s)			{ return insert (ip, s, strlen(s)); }
    inline auto		insert (const_iterator ip, const_pointer f, const_iterator l)	{ return insert (ip, f, l-f); }
    int			insertv (const_iterator ip, const char* fmt, va_list args) noexcept;
    int			insertf (const_iterator ip, const char* fmt, ...) noexcept PRINTFARGS(3,4);
			using memblock::append;
    inline void	   	append (const_pointer s)			{ append (s, strlen(s)); }
    inline void		append (const string& s)			{ append (s.begin(), s.size()); }
    inline void		append (const_iterator i1, const_iterator i2)	{ assert (i1<=i2); append (i1, i2-i1); }
    int			appendv (const char* fmt, va_list args) noexcept;
    int			appendf (const char* fmt, ...) noexcept PRINTFARGS(2,3);
			using memblock::assign;
    inline void	    	assign (const_pointer s)			{ assign (s, strlen(s)); }
    inline void		assign (const string& s)			{ assign (s.begin(), s.size()); }
    inline void		assign (const_iterator i1, const_iterator i2)	{ assert (i1<=i2); assign (i1, i2-i1); }
    int			assignv (const char* fmt, va_list args) noexcept;
    int			assignf (const char* fmt, ...) noexcept PRINTFARGS(2,3);
    inline static auto	create_from_file (const char* filename)noexcept	{ string r; r.read_file (filename); return r; }
    template <typename... Args>
    inline static auto	createf (const char* fmt, Args&&... args)	{ string r; r.assignf (fmt, forward<Args>(args)...); return r; }
    static int		compare (const_iterator f1, const_iterator l1, const_iterator f2, const_iterator l2) noexcept;
    inline auto		compare (const string& s) const			{ return compare (begin(), end(), s.begin(), s.end()); }
    inline auto		compare (const_pointer s) const			{ return compare (begin(), end(), s, s + strlen(s)); }
    inline void		swap (string&& s)				{ memblock::swap (move(s)); }
    inline auto&	operator= (const string& s)			{ assign (s); return *this; }
    inline auto&	operator= (const_pointer s)			{ assign (s); return *this; }
    inline auto&	operator= (const_reference c)			{ resize (1); back() = c; return *this; }
    inline auto&	operator+= (const string& s)			{ append (s); return *this; }
    inline auto&	operator+= (const_pointer s)			{ append (s); return *this; }
    inline auto&	operator+= (const_reference c)			{ push_back (c); return *this; }
    inline auto		operator+ (const string& s) const		{ auto result (*this); result += s; return result; }
    inline bool		operator== (const string& s) const		{ return memblock::operator== (s); }
    bool		operator== (const_pointer s) const noexcept;
    inline bool		operator== (const_reference c) const		{ return size() == 1 && c == at(0); }
    inline bool		operator!= (const string& s) const		{ return !operator== (s); }
    inline bool		operator!= (const_pointer s) const		{ return !operator== (s); }
    inline bool		operator!= (const_reference c) const		{ return !operator== (c); }
    inline bool		operator< (const string& s) const		{ return 0 > compare (s); }
    inline bool		operator< (const_pointer s) const		{ return 0 > compare (s); }
    inline bool		operator< (const_reference c) const		{ return 0 > compare (begin(), end(), &c, &c + 1); }
    inline bool		operator> (const string& s) const		{ return 0 < compare (s); }
    inline bool		operator> (const_pointer s) const		{ return 0 < compare (s); }
    inline bool		operator> (const_reference c) const		{ return 0 < compare (begin(), end(), &c, &c + 1); }
    inline bool		operator<= (const_pointer s) const		{ return 0 >= compare (s); }
    inline bool		operator>= (const_pointer s) const		{ return 0 <= compare (s); }
    inline		operator const lstring& (void) const		{ return reinterpret_cast<const lstring&>(*this); }
    inline auto		erase (const_iterator ep, size_type n = 1)	{ return memblock::erase (ep, n); }
    inline auto		erase (const_iterator f, const_iterator l)	{ assert (f<=l); return erase (f, l-f); }
    inline void		pop_back (void)					{ assert (capacity() && "modifying a const linked string"); assert (size() && "pop_back called on empty string"); memlink::resize (size()-1); *end() = 0; }
    inline void		clear (void)					{ memlink::resize (0); auto e = end(); if (e) { assert (capacity() && "modifying a const linked string"); *e = 0; }}

    inline auto		replace (const_iterator f, const_iterator l, const_pointer s, size_type slen)	{ return memblock::replace (f, l-f, s, slen); }
    inline auto		replace (const_iterator f, const_iterator l, const_pointer s)			{ return replace (f, l, s, strlen(s)); }
    inline auto		replace (const_iterator f, const_iterator l, const_pointer i1,const_pointer i2)	{ return replace (f, l, i1, i2-i1); }
    inline auto		replace (const_iterator f, const_iterator l, const string& s)			{ return replace (f, l, s.begin(), s.end()); }
    iterator		replace (const_iterator f, const_iterator l, size_type n, value_type c) noexcept;

    inline auto		find (const_pointer s, const_iterator fi) const		{ return const_iterator (strstr (fi, s)); }
    inline auto		find (const string& s, const_iterator fi) const		{ return find (s.c_str(), fi); }
    inline auto		find (const_pointer s) const				{ return find (s, begin()); }
    inline auto		find (const string& s) const				{ return find (s, begin()); }
    inline auto		find (const_reference c, const_iterator fi) const	{ return const_iterator (strchr (fi, c)); }
    inline auto		find (const_reference c) const				{ return find (c, begin()); }

    inline auto		find (const_pointer s, const_iterator fi)		{ return UNCONST_MEMBER_FN (find,s,fi); }
    inline auto		find (const string& s, const_iterator fi)		{ return UNCONST_MEMBER_FN (find,s,fi); }
    inline auto		find (const_pointer s)					{ return UNCONST_MEMBER_FN (find,s); }
    inline auto		find (const string& s)					{ return UNCONST_MEMBER_FN (find,s); }
    inline auto		find (const_reference c, const_iterator fi)		{ return UNCONST_MEMBER_FN (find,c,fi); }
    inline auto		find (const_reference c)				{ return UNCONST_MEMBER_FN (find,c); }

    const_iterator	rfind (const_pointer s, const_iterator fi) const noexcept;
    inline auto		rfind (const string& s, const_iterator fi) const	{ return rfind (s.c_str(), fi); }
    inline auto		rfind (const_pointer s) const				{ return rfind (s, end()); }
    inline auto		rfind (const string& s) const				{ return rfind (s, end()); }
    inline auto		rfind (const_reference c, const_iterator fi) const	{ return const_iterator (memrchr (begin(), c, fi-begin())); }
    inline auto		rfind (const_reference c) const				{ return rfind (c, end()); }

    inline auto		rfind (const_pointer s, const_iterator fi)		{ return UNCONST_MEMBER_FN (rfind,s,fi); }
    inline auto		rfind (const string& s, const_iterator fi)		{ return UNCONST_MEMBER_FN (rfind,s,fi); }
    inline auto		rfind (const_pointer s)					{ return UNCONST_MEMBER_FN (rfind,s); }
    inline auto		rfind (const string& s)					{ return UNCONST_MEMBER_FN (rfind,s); }
    inline auto		rfind (const_reference c, const_iterator fi)		{ return UNCONST_MEMBER_FN (rfind,c,fi); }
    inline auto		rfind (const_reference c)				{ return UNCONST_MEMBER_FN (rfind,c); }

    auto		find_first_of (const_pointer s) const noexcept		{ auto rsz = strcspn (c_str(), s); return rsz >= size() ? nullptr : iat(rsz); }
    inline auto		find_first_of (const string& s) const			{ return find_first_of (s.c_str()); }
    auto		find_first_not_of (const_pointer s) const noexcept	{ auto rsz = strspn (c_str(), s); return rsz >= size() ? nullptr : iat(rsz); }
    inline auto		find_first_not_of (const string& s) const		{ return find_first_not_of (s.c_str()); }

    inline auto		find_first_of (const_pointer s)				{ return UNCONST_MEMBER_FN (find_first_of,s); }
    inline auto		find_first_of (const string& s)				{ return UNCONST_MEMBER_FN (find_first_of,s); }
    inline auto		find_first_not_of (const_pointer s)			{ return UNCONST_MEMBER_FN (find_first_not_of,s); }
    inline auto		find_first_not_of (const string& s)			{ return UNCONST_MEMBER_FN (find_first_not_of,s); }
};

//----------------------------------------------------------------------

class lstring : public cmemlink {
public:
			lstring (void)					: cmemlink () { set_zero_terminated(); }
    constexpr		lstring (const_pointer s, size_type len)	: cmemlink (s, len, true) {}
    constexpr		lstring (const_pointer s1, const_pointer s2)	: lstring (s1, s2-s1) {}
    inline		lstring (const_pointer s)			: lstring (s, strlen(s)) {}
    constexpr		lstring (lstring&& s)				: cmemlink (move(s)) {}
    constexpr		lstring (const lstring& s)			: cmemlink (s) {}
    constexpr		lstring (const string& s)			: cmemlink (s) {}
    inline void		swap (lstring&& s)				{ cmemlink::swap (move(s)); }
    inline auto&	operator= (const string& s)			{ cmemlink::operator= (s); return *this; }
    inline auto&	operator= (lstring&& s)				{ cmemlink::operator= (move(s)); return *this; }

    inline auto&	str (void) const				{ return reinterpret_cast<const string&>(*this); }
    inline		operator const string& (void) const		{ return str(); }

    constexpr auto	c_str (void) const				{ assert ((!end() || !*end()) && "This string is linked to data that is not 0-terminated. This may cause serious security problems. Please assign the data instead of linking."); return data(); }
    constexpr auto&	back (void) const				{ return at(size()-1); }

    inline static int	compare (const_iterator f1, const_iterator l1, const_iterator f2, const_iterator l2)
									{ return string::compare (f1,l1,f2,l2); }
    inline auto		compare (const lstring& s) const		{ return compare (begin(), end(), s.begin(), s.end()); }
    inline auto		compare (const string& s) const			{ return compare (begin(), end(), s.begin(), s.end()); }
    inline auto		compare (const_pointer s) const			{ return compare (begin(), end(), s, s + strlen(s)); }

    inline bool		operator== (const string& s) const		{ return str() == s; }
    inline bool		operator== (const_pointer s) const		{ return str() == s; }
    inline bool		operator== (const_reference c) const		{ return str() == c; }
    inline bool		operator!= (const string& s) const		{ return str() != s; }
    inline bool		operator!= (const_pointer s) const		{ return str() != s; }
    inline bool		operator!= (const_reference c) const		{ return str() != c; }
    inline bool		operator< (const string& s) const		{ return str() < s; }
    inline bool		operator< (const_pointer s) const		{ return str() < s; }
    inline bool		operator< (const_reference c) const		{ return str() < c; }
    inline bool		operator> (const string& s) const		{ return str() > s; }
    inline bool		operator> (const_pointer s) const		{ return str() > s; }
    inline bool		operator> (const_reference c) const		{ return str() > c; }
    inline bool		operator<= (const_pointer s) const		{ return str() <= s; }
    inline bool		operator>= (const_pointer s) const		{ return str() >= s; }

    inline auto		find (const_pointer s, const_iterator fi) const		{ return str().find (s, fi); }
    inline auto		find (const string& s, const_iterator fi) const		{ return str().find (s, fi); }
    inline auto		find (const_pointer s) const				{ return str().find (s); }
    inline auto		find (const string& s) const				{ return str().find (s); }
    inline auto		find (const_reference c, const_iterator fi) const	{ return str().find (c, fi); }
    inline auto		find (const_reference c) const				{ return str().find (c); }

    inline auto		rfind (const_pointer s, const_iterator fi) const	{ return str().rfind (s, fi); }
    inline auto		rfind (const string& s, const_iterator fi) const	{ return str().rfind (s, fi); }
    inline auto		rfind (const_pointer s) const				{ return str().rfind (s); }
    inline auto		rfind (const string& s) const				{ return str().rfind (s); }
    inline auto		rfind (const_reference c, const_iterator fi) const	{ return str().rfind (c, fi); }
    inline auto		rfind (const_reference c) const				{ return str().rfind (c); }

    inline auto		find_first_of (const_pointer s) const			{ return str().find_first_of (s); }
    inline auto		find_first_of (const string& s) const			{ return str().find_first_of (s); }
    inline auto		find_first_not_of (const_pointer s) const		{ return str().find_first_not_of (s); }
    inline auto		find_first_not_of (const string& s) const		{ return str().find_first_not_of (s); }

    void		resize (size_type sz) = delete;
    void		clear (void) = delete;
};

} // namespace cwiclo
