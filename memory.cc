// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "memory.h"
#include <alloca.h>

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
