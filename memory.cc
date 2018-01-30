// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "memory.h"
#include <alloca.h>
#if HAVE_EXECINFO_H
    #include <execinfo.h>
#endif

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

extern "C" void print_backtrace (void) noexcept
{
#if HAVE_EXECINFO_H
    void* frames[32];
    auto nf = backtrace (ArrayBlock(frames));
    if (nf > 1) {
	fflush (stdout);
	backtrace_symbols_fd (&frames[1], nf-1, STDOUT_FILENO);
    }
#endif
}

#ifndef UC_VERSION
static inline char _num_to_digit (uint8_t b)
{
    char d = (b & 0xF) + '0';
    return d <= '9' ? d : d+('A'-'0'-10);
}
static inline bool _printable (char c)
{
    return c >= 32 && c < 127;
}
extern "C" void hexdump (const void* vp, size_t n)
{
    auto p = (const uint8_t*) vp;
    char line[65]; line[64] = 0;
    for (size_t i = 0; i < n; i += 16) {
	memset (line, ' ', sizeof(line)-1);
	for (size_t h = 0; h < 16; ++h) {
	    if (i+h < n) {
		uint8_t b = p[i+h];
		line[h*3] = _num_to_digit(b>>4);
		line[h*3+1] = _num_to_digit(b);
		line[h+3*16] = _printable(b) ? b : '.';
	    }
	}
	puts (line);
    }
}
#endif

} // namespace cwiclo
