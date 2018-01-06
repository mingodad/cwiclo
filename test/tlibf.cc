// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "../memblock.h"
#include <ctype.h>
using namespace cwiclo;

//{{{ TestML -----------------------------------------------------------

static void WriteML (const memlink& l)
{
    printf ("memlink{%u}: ", l.size());
    for (auto c : l)
	putchar (isprint(c) ? c : '.');
    putchar ('\n');
}

static void TestML (void)
{
    char str[] = "abcdefghijklmnopqrstuvwzyz";
    memlink::const_pointer cstr = str;

    memlink a (ArrayBlock(str));
    if (a.begin() != str)
	printf ("begin() failed on memlink\n");
    a.static_link (str);
    if (a.begin() + 5 != &str[5])
	printf ("begin() + 5 failed on memlink\n");
    if (0 != memcmp (a.begin(), str, ArraySize(str)))
	printf ("memcmp failed on memlink\n");
    WriteML (a);
    memlink cb;
    cb.link (cstr, ArraySize(str));
    if (cb.data() != cstr)
	printf ("begin() of const failed on memlink\n");
    if (cb.begin() != cstr)
	printf ("begin() failed on memlink\n");
    WriteML (cb);
    if (!(a == cb))
	printf ("operator== failed on memlink\n");
    memlink b (str, ArraySize(str));
    b.resize (ArraySize(str) - 2);
    a = b;
    if (a.data() != b.data())
	printf ("begin() after assignment failed on memlink\n");
    a.link (str, ArraySize(str) - 1);
    WriteML (a);
    a.insert (a.begin() + 5, 9);
    fill_n (a.begin() + 5, 9, '-');
    WriteML (a);
    a.erase (a.begin() + 9, 7);
    fill_n (a.end() - 7, 7, '=');
    WriteML (a);
}
//}}}
//{{{ TestMB -----------------------------------------------------------

static void WriteMB (const memblock& l)
{
    printf ("memblock{%u}: ", l.size());
    for (auto c : l)
	putchar (isprint(c) ? c : '.');
    putchar ('\n');
}

static void TestMB (void)
{
    char strTest[] = "abcdefghijklmnopqrstuvwxyz";
    const auto strTestLen = strlen(strTest);
    const auto cstrTest = strTest;

    memblock a, b;
    a.link (strTest, strTestLen);
    if (a.begin() != strTest)
	printf ("begin() failed on memblock\n");
    if (a.begin() + 5 != &strTest[5])
	printf ("begin() + 5 failed on memblock\n");
    if (0 != memcmp (a.begin(), strTest, strTestLen))
	printf ("memcmp failed on memblock\n");
    WriteMB (a);
    b.link (cstrTest, strTestLen);
    if (b.data() != cstrTest)
	printf ("begin() of const failed on memblock\n");
    if (b.begin() != cstrTest)
	printf ("begin() failed on memblock\n");
    WriteMB (b);
    if (!(a == b))
	printf ("operator== failed on memblock\n");
    b.copy_link();
    if (b.data() == nullptr || b.data() == cstrTest)
	printf ("copy_link failed on memblock\n");
    if (!(a == b))
	printf ("copy_link didn't copy\n");
    b.resize (strTestLen - 2);
    a = b;
    if (a.begin() == b.begin())
	printf ("Assignment does not copy a link\n");
    a.deallocate();
    a.assign (strTest, strTestLen);
    WriteMB (a);
    a.insert (a.begin() + 5, 9);
    fill_n (a.begin() + 5, 9, '-');
    WriteMB (a);
    a.erase (a.begin() + 2, 7);
    fill_n (a.end() - 7, 7, '=');
    WriteMB (a);
    a.resize (0);
    WriteMB (a);
    a.resize (strTestLen + strTestLen / 2);
    fill_n (a.iat(strTestLen), strTestLen/2, '+');
    WriteMB (a);
}
//}}}

int main (void)
{
    using stdtestfunc_t	= void (*)(void);
    static const stdtestfunc_t c_Tests[] = {
	TestML,
	TestMB
    };
    for (auto i = 0u; i < ArraySize(c_Tests); ++i) {
	printf ("######################################################################\n");
	c_Tests[i]();
    }
    return EXIT_SUCCESS;
}
