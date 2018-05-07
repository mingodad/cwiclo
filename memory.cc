// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "memory.h"
#include <alloca.h>
#include <sys/stat.h>
#if __has_include(<execinfo.h>)
    #include <execinfo.h>
#endif

extern "C" void* _realloc (void* p, size_t n) noexcept
{
    p = realloc (p, n);
    if (!p)
	abort();
    return p;
}

extern "C" void* _alloc (size_t n) noexcept
{
    auto p = _realloc (nullptr, n);
    #ifndef NDEBUG
	memset (p, 0xcd, n);
    #endif
    return p;
}

extern "C" void _free (void* p) noexcept
    { free(p); }
extern "C" void _free_n (void* p, size_t) noexcept
    { free(p); }

//----------------------------------------------------------------------

void* operator new (size_t n)	WEAKALIAS("_alloc");
void* operator new[] (size_t n)	WEAKALIAS("_alloc");

void  operator delete (void* p) noexcept	WEAKALIAS("_free");
void  operator delete[] (void* p) noexcept	WEAKALIAS("_free");
void  operator delete (void* p, size_t n) noexcept	WEAKALIAS("_free_n");
void  operator delete[] (void* p, size_t n) noexcept	WEAKALIAS("_free_n");

//----------------------------------------------------------------------

namespace std {

void terminate (void) noexcept { abort(); }

} // namespace std
namespace __cxxabiv1 {
extern "C" {

#ifndef NDEBUG
#define TERMINATE_ALIAS(name)	void name (void) noexcept { assert (!#name); std::terminate(); }
#else
#define TERMINATE_ALIAS(name)	void name (void) noexcept WEAKALIAS("_ZSt9terminatev");
#endif
TERMINATE_ALIAS (__cxa_call_unexpected)
TERMINATE_ALIAS (__cxa_pure_virtual)
TERMINATE_ALIAS (__cxa_deleted_virtual)
TERMINATE_ALIAS (__cxa_bad_cast)
TERMINATE_ALIAS (__cxa_bad_typeid)
TERMINATE_ALIAS (__cxa_throw_bad_array_new_length)
TERMINATE_ALIAS (__cxa_throw_bad_array_length)
#undef TERMINATE_ALIAS

int __cxa_guard_acquire (__guard* g) noexcept { return !g->test_and_set(); }
void __cxa_guard_release (__guard*) noexcept {}
void __cxa_guard_abort (__guard* g) noexcept { g->clear(); }

} // extern "C"
} // namespace __cxxabiv1

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
#if __has_include(<execinfo.h>)
    void* frames[32];
    auto nf = backtrace (ArrayBlock(frames));
    if (nf <= 1)
	return;
    auto syms = backtrace_symbols (frames, nf);
    if (!syms)
	return;
    for (auto f = 1; f < nf; ++f) {
	// Symbol names are formatted thus: "file(function+0x42) [0xAddress]"
	// Parse out the function name
	auto fnstart = strchr (syms[f], '(');
	if (!fnstart)
	    fnstart = syms[f];
	else
	    ++fnstart;
	auto fnend = strchr (fnstart, '+');
	if (!fnend) {
	    fnend = strchr (fnstart, ')');
	    if (!fnend)
		fnend = fnstart + strlen(fnstart);
	}
	auto addrstart = strchr (fnend, '[');
	if (addrstart) {
	    auto addr = strtoul (++addrstart, nullptr, 0);
	    if constexpr (sizeof(void*) <= 8)
		printf ("%8lx\t", addr);
	    else
		printf ("%16lx\t", addr);
	}
	fwrite (fnstart, 1, fnend-fnstart, stdout);
	fputc ('\n', stdout);
    }
    free (syms);
    fflush (stdout);
#endif
}

#ifndef UC_VERSION
extern "C" void hexdump (const void* vp, size_t n) noexcept
{
    auto p = (const uint8_t*) vp;
    for (size_t i = 0; i < n; i += 16) {
	for (auto j = 0u; j < 16; ++j) {
	    if (i+j < n)
		printf ("%02x ", p[i+j]);
	    else
		printf ("   ");
	}
	for (auto j = 0u; j < 16; ++j) {
	    if (i+j < n && p[i+j] >= ' ' && p[i+j] <= '~')
		printf ("%c", p[i+j]);
	    else
		printf (" ");
	}
	printf ("\n");
    }
}

//----------------------------------------------------------------------

const char* executable_in_path (const char* efn, char* exe, size_t exesz) noexcept
{
    if (efn[0] == '/' || (efn[0] == '.' && (efn[1] == '/' || efn[1] == '.'))) {
	if (0 != access (efn, X_OK))
	    return nullptr;
	return efn;
    }

    const char* penv = getenv("PATH");
    if (!penv)
	penv = "/bin:/usr/bin:.";
    char path [PATH_MAX];
    snprintf (ArrayBlock(path), "%s/%s"+3, penv);

    for (char *pf = path, *pl = pf; *pf; pf = pl) {
	while (*pl && *pl != ':')
	    ++pl;
	*pl++ = 0;
	snprintf (exe, exesz, "%s/%s", pf, efn);
	if (0 == access (exe, X_OK))
	    return exe;
    }
    return nullptr;
}

unsigned sd_listen_fds (void) noexcept
{
    const char* e = getenv("LISTEN_PID");
    if (!e || getpid() != (pid_t) strtoul (e, nullptr, 10))
	return 0;
    e = getenv("LISTEN_FDS");
    return e ? strtoul (e, nullptr, 10) : 0;
}

int mkpath (const char* path, mode_t mode) noexcept
{
    char pbuf [PATH_MAX], *pbe = pbuf;
    do {
	if (*path == '/' || !*path) {
	    *pbe = '\0';
	    if (pbuf[0] && 0 > mkdir (pbuf, mode) && errno != EEXIST)
		return -1;
	}
	*pbe++ = *path;
    } while (*path++);
    return 0;
}

int rmpath (const char* path) noexcept
{
    char pbuf [PATH_MAX];
    for (auto pend = stpcpy (pbuf, path)-1;; *pend = 0) {
	if (0 > rmdir(pbuf))
	    return (errno == ENOTEMPTY || errno == EACCES) ? 0 : -1;
	do {
	    if (pend <= pbuf)
		return 0;
	} while (*--pend != '/');
    }
    return 0;
}
#endif // UC_VERSION

} // namespace cwiclo
