// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "memory.h"
#include <alloca.h>

extern "C" void* _realloc (void* p, size_t n)
{
    p = realloc (p, n);
    if (!p)
	abort();
    return p;
}

extern "C" void* _alloc (size_t n)
{
    auto p = _realloc (nullptr, n);
    #ifndef NDEBUG
	memset (p, 0xcd, n);
    #endif
    return p;
}

extern "C" void _free (void* p)
    { free(p); }

//----------------------------------------------------------------------

void* operator new (size_t n)	WEAKALIAS("_alloc");
void* operator new[] (size_t n)	WEAKALIAS("_alloc");

void  operator delete (void* p) noexcept	WEAKALIAS("_free");
void  operator delete[] (void* p) noexcept	WEAKALIAS("_free");
void  operator delete (void* p, size_t n) noexcept	WEAKALIAS("_free");
void  operator delete[] (void* p, size_t n) noexcept	WEAKALIAS("_free");

//----------------------------------------------------------------------

namespace cwiclo {

extern "C" void brotate (void* vf, void* vm, void* vl) noexcept
{
    auto f = (char*) vf, m = (char*) vm, l = (char*) vl;
    const auto lsz (m-f), hsz (l-m), hm (min (lsz, hsz));
    if (!hm)
	return;
    auto t = alloca (hm);
    if (hsz < lsz) {
	memcpy (t, m, hsz);
	memmove (f+hsz, f, lsz);
	memcpy (f, t, hsz);
    } else {
	memcpy (t, f, lsz);
	memmove (f, m, hsz);
	memcpy (l-lsz, t, lsz);
    }
}

} // namespace cwiclo
