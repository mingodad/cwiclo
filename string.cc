// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "string.h"
#include <stdarg.h>

namespace cwiclo {

int string::appendv (const char* fmt, va_list args) noexcept
{
    va_list args2;
    int rv, wcap;
    iterator ip = end();
    do {
	va_copy (args2, args);
	wcap = end()-ip+1;
	rv = vsnprintf (ip, wcap, fmt, args2);
	if (rv <= 0)
	    break;
	resize (size()+rv);
    } while (rv >= wcap);
    return rv;
}

int string::appendf (const char* fmt, ...) noexcept
{
    va_list args;
    va_start (args, fmt);
    auto rv = appendv (fmt, args);
    va_end (args);
    return rv;
}

int string::assignv (const char* fmt, va_list args) noexcept
{
    clear();
    return appendv (fmt, args);
}

int string::assignf (const char* fmt, ...) noexcept
{
    clear();
    va_list args;
    va_start (args, fmt);
    auto rv = appendv (fmt, args);
    va_end (args);
    return rv;
}

int string::insertv (const_iterator cip, const char* fmt, va_list args) noexcept
{
    auto ip = const_cast<iterator>(cip);
    const char c = (ip < end() ? *ip : 0);
    for (int wcap = 0;;) {
	va_list args2;
	va_copy (args2, args);
	auto rv = vsnprintf (ip, wcap, fmt, args2);
	if (rv <= wcap) {
	    ip[rv] = c;
	    return rv;
	}
	ip = memblock::insert (ip, rv);
	wcap = rv+1;
    }
}

int string::insertf (const_iterator ip, const char* fmt, ...) noexcept
{
    va_list args;
    va_start (args, fmt);
    auto rv = insertv (ip, fmt, args);
    va_end (args);
    return rv;
}

bool string::operator== (const_pointer s) const noexcept
{
    return strlen(s) == size() && 0 == strcmp (c_str(), s);
}

void string::replace (const_iterator f, const_iterator l, const_pointer s, size_type slen) noexcept
{
    auto dsz = difference_type(slen) - (l-f);
    if (dsz > 0)
	f = memblock::insert (f, dsz);
    else
	f = memblock::erase (f, dsz);
    memcpy (const_cast<pointer>(f), s, slen);
}

void string::replace (const_iterator f, const_iterator l, size_type n, value_type c) noexcept
{
    auto dsz = difference_type(n) - (l-f);
    if (dsz > 0)
	f = memblock::insert (f, dsz);
    else
	f = memblock::erase (f, dsz);
    memset (const_cast<pointer>(f), c, n);
}

auto string::rfind (const_pointer s, const_iterator fi) const noexcept -> const_iterator
{
    const_iterator h = begin(), r = nullptr;
    fi -= strlen(s);
    while (h && h < fi)
	r = h = strstr (h, s);
    return r < fi ? r : nullptr;
}

} // namespace cwiclo
