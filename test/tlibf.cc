// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "../app.h"
#include "../vector.h"
#include "../multiset.h"
#include "../string.h"
#include "../stream.h"
#include <ctype.h>
#include <stdarg.h>
using namespace cwiclo;

//{{{ LibTestApp -------------------------------------------------------

class LibTestApp : public App {
public:
    static auto&	Instance (void) { static LibTestApp s_App; return s_App; }
    inline int		Run (void);
private:
    inline		LibTestApp (void) : App() {}
    static void		TestML (void);
    static void		TestMB (void);
    static void		TestVector (void);
    static void		TestMultiset (void);
    static void		TestString (void);
    static void		TestStringVector (void);
    static void		TestStreams (void);
    static void		WriteML (const memlink& l);
    static void		WriteMB (const memblock& l);
    static void		PrintVector (const vector<int>& v);
    static vector<int>	MakeIotaVector (unsigned n);
    static vector<int>	SubtractVector (const vector<int>& v1, const vector<int>& v2);
    static void		PrintString (const string& str);
};

//}}}-------------------------------------------------------------------
//{{{ TestML

void LibTestApp::WriteML (const memlink& l) // static
{
    printf ("memlink{%u}: ", l.size());
    for (auto c : l)
	putchar (isprint(c) ? c : '.');
    putchar ('\n');
}

void LibTestApp::TestML (void) // static
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
//}}}-------------------------------------------------------------------
//{{{ TestMB

void LibTestApp::WriteMB (const memblock& l) // static
{
    printf ("memblock{%u}: ", l.size());
    for (auto c : l)
	putchar (isprint(c) ? c : '.');
    putchar ('\n');
}

void LibTestApp::TestMB (void) // static
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

//}}}-------------------------------------------------------------------
//{{{ TestVector

void LibTestApp::PrintVector (const vector<int>& v) // static
{
    putchar ('{');
    for (auto i = 0u; i < v.size(); ++i) {
	if (i)
	    putchar (',');
	printf ("%d", v[i]);
    }
    puts ("}");
}

vector<int> LibTestApp::MakeIotaVector (unsigned n) // static
{
    vector<int> r (n);
    for (auto i = 0u; i < r.size(); ++i)
	r[i] = i;
    return r;
}

vector<int> LibTestApp::SubtractVector (const vector<int>& v1, const vector<int>& v2) // static
{
    vector<int> r (v1.begin(), v1.iat(min(v1.size(),v2.size())));
    for (auto i = 0u; i < r.size(); ++i)
	r[i] = v1[i] - v2[i];
    return r;
}

struct A {
    A (void) { puts ("A::A"); }
    ~A (void) { puts ("A::~A"); }
};

void LibTestApp::TestVector (void) // static
{
    const vector<int> vstd { 8,3,1,2,5,6,1,3,4,9 };
    PrintVector (vstd);
    auto v = vstd;
    v.resize (17);
    fill_n (v.iat(vstd.size()), v.size()-vstd.size(), 7);
    v.resize (14);
    PrintVector (v);
    auto dv = SubtractVector (v, MakeIotaVector (v.size()));
    PrintVector (dv);
    v.shrink_to_fit();
    printf ("v: front %d, back %d, [4] %d, capacity %u\n", v.front(), v.back(), v[4], v.capacity());
    v.insert (v.iat(4), {23,24,25});
    v.emplace (v.iat(2), 77);
    v.emplace_back (62);
    v.emplace_back (62);
    v.push_back (62);
    v.erase (v.end()-2);
    v.pop_back();
    PrintVector (v);
    sort (v);
    PrintVector (v);
    printf ("lower_bound(7): %tu\n", lower_bound (v,7)-v.begin());
    printf ("upper_bound(7): %tu\n", upper_bound (v,7)-v.begin());
    printf ("binary_search(3): %tu\n", binary_search (v,3)-v.begin());
    auto s42 = binary_search (v, 42);
    if (s42)
	printf ("binary_search(42): %tu\n", s42-v.begin());

    puts ("Constructing vector<A>(3)");
    vector<A> av (3);
    puts ("resize vector<A> to 4");
    av.resize (4);
    puts ("erase 2");
    av.erase (av.iat(2), 2);
    puts ("deallocating");
}

//}}}-------------------------------------------------------------------
//{{{ TestMultiset

void LibTestApp::TestMultiset (void) // static
{
    multiset<int> v {1, 8, 9, 2, 3, 1, 1};
    v.insert ({4, 6, 1, 3, 4});
    printf ("multiset:\t");
    PrintVector (v);
    printf ("erase(3):\t");
    v.erase (3);
    PrintVector (v);
    auto f = v.find (7);
    if (f)
	printf ("7 found at %ld\n", distance(v.begin(),f));
    f = v.find (6);
    if (f)
	printf ("6 found at %ld\n", distance(v.begin(),f));
    printf ("lower_bound(4) at %ld\n", distance(v.begin(),v.lower_bound (4)));
    printf ("upper_bound(4) at %ld\n", distance(v.begin(),v.upper_bound (4)));
    printf ("lower_bound(5) at %ld\n", distance(v.begin(),v.lower_bound (5)));
    v.insert (v.lower_bound(5), 5);
    PrintVector (v);
}

//}}}-------------------------------------------------------------------
//{{{ TestString

static void MyFormat (const char* fmt, ...) PRINTFARGS(1,2);
static void MyFormat (const char* fmt, ...)
{
    string buf;
    va_list args;
    va_start (args, fmt);
    buf.assignv (fmt, args);
    printf ("Custom vararg MyFormat: %s\n", buf.c_str());
    va_end (args);
}

void LibTestApp::TestString (void) // static
{
    static const char c_TestString1[] = "123456789012345678901234567890";
    static const char c_TestString2[] = "abcdefghijklmnopqrstuvwxyz";
    static const char c_TestString3[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    string s1 (c_TestString1);
    string s2 (ArrayRange (c_TestString2));
    auto s3 (s1);

    puts (s1.c_str());
    puts (s2.c_str());
    puts (s3.c_str());
    s3.reserve (48);
    s3.resize (20);

    auto i = 0u;
    for (; i < s3.size(); ++ i)
	s3.at(i) = s3.at(i);
    for (i = 0; i < s3.size(); ++ i)
	s3[i] = s3[i];
    printf ("%s\ns3.size() = %u, max_size() = ", s3.c_str(), s3.size());
    if (s3.max_size() == numeric_limits<string::size_type>::max()/2-1)
	printf ("MAX/2-1");
    else
	printf ("%u", s3.max_size());
    printf (", capacity() = %u\n", s3.capacity());

    s1.unlink();
    s1 = c_TestString2;
    s1 += c_TestString3;
    s1 += '$';
    puts (s1.c_str());

    s1 = "Hello";
    s2.deallocate();
    s2 = "World";
    s3 = s1 + s2;
    puts (s3.c_str());
    s3 = "Concatenated ";
    s3 += s1.c_str();
    s3 += s2;
    s3 += " string.";
    puts (s3.c_str());

    if (s1 < s2)
	puts ("s1 < s2");
    if (s1 == s1)
	puts ("s1 == s1");
    if (s1[0] != s1[0])
	puts ("s1[0] != s1[0]");

    string s4;
    s4.link (s1);
    if (s1 == s4)
	puts ("s1 == s4");

    s1 = c_TestString1;
    string s5 (s1.begin() + 4, s1.begin() + 4 + 5);
    string s6 (s1.begin() + 4, s1.begin() + 4 + 5);
    if (s5 == s6)
	puts (string::createf("%s == %s", s5.c_str(), s6.c_str()).c_str());
    string tail (s1.iat(7), s1.end());
    printf ("&s1[7] =\t%s\n", tail.c_str());

    printf ("initial:\t%s\n", s1.c_str());
    printf ("erase([5]-9)\t");
    s1.erase (s1.iat(5), s1.find('9')-s1.iat(5));
    printf ("%s\n", s1.c_str());
    printf ("erase(5,5)\t");
    s1.erase (s1.iat(5), 2U);
    s1.erase (s1.iat(5), 3);
    assert (!*s1.end());
    puts (s1.c_str());
    printf ("push_back('x')\t");
    s1.push_back ('x');
    assert (!*s1.end());
    puts (s1.c_str());
    printf ("pop_back()\n");
    s1.pop_back();
    assert (!*s1.end());
    printf ("insert(10,#)\t");
    s1.insert (s1.iat(10), '#');
    assert (!*s1.end());
    puts (s1.c_str());
    printf ("replace(0,5,@)\t");
    s1.replace (s1.begin(), s1.iat(5), 2, '@');
    assert (!*s1.end());
    puts (s1.c_str());

    s1 = c_TestString1;
    printf ("8 found at\t%s\n", s1.find('8'));
    printf ("8 found again\t%s\n", s1.find ('8',s1.find('8')+1));
    printf ("9 found at\t%s\n", s1.find ("9"));
    printf ("7 rfound at\t%s\n", s1.rfind ('7'));
    printf ("7 rfound again\t%s\n", s1.rfind('7', s1.rfind('7') - 1));
    printf ("67 rfound at\t%s\n", s1.rfind ("67"));
    if (!s1.rfind("X"))
	puts ("X was not rfound");
    else
	printf ("X rfound at\t%s\n", s1.rfind ("X"));
    auto poundfound = s1.find ("#");
    if (poundfound)
	printf ("# found at\t%s\n", poundfound);
    printf ("[456] found at\t%s\n", s1.find_first_of ("456"));

    s2.clear();
    assert (!*s2.end());
    if (s2.empty())
	printf ("s2 is empty [%s], capacity %u bytes\n", s2.c_str(), s2.capacity());

    s2.assignf ("<const] %d, %s, 0x%08X", 42, "[rfile>", 0xDEADBEEF);
    s2.appendf (", 0%o, appended", 012345);
    s2.insertf (s2.iat(31), "; %u, inserted", 12345);
    printf ("<%u bytes of %u> Format '%s'\n", s2.size(), s2.capacity(), s2.c_str());
    MyFormat ("'<const] %d, %s, 0x%08X'", 42, "[rfile>", 0xDEADBEEF);
}

//}}}-------------------------------------------------------------------
//{{{ TestStringVector

void LibTestApp::PrintString (const string& str) // static
{
    puts (str.c_str());
}

void LibTestApp::TestStringVector (void) // static
{
    vector<string> v = { "Hello world!", "Hello again!", "element3", "element4", "element5_long_element5" };

    auto bogusi = linear_search (v, string("bogus"));
    if (bogusi)
	printf ("bogus found at position %td\n", bogusi - v.begin());

    foreach (i,v) PrintString(*i);

    if (!(v[2] == string("element3")))
	printf ("operator== failed\n");
    auto el3i = linear_search (v, string("element3"));
    if (el3i)
	printf ("%s found at position %td\n", el3i->c_str(), el3i - v.begin());
    bogusi = linear_search (v, string("bogus"));
    if (bogusi)
	printf ("%s found at position %td\n", bogusi->c_str(), bogusi - v.begin());

    vector<string> v2;
    v2 = v;
    v = v2;
    v.erase (v.end(), v.end());
    printf ("After erase (end,end):\n");
    foreach (i,v) PrintString(*i);
    v = v2;
    v.erase (v.begin() + 2, 2);
    printf ("After erase (2,2):\n");
    foreach (i,v) PrintString(*i);
    v = v2;
    v.pop_back();
    printf ("After pop_back():\n");
    foreach (i,v) PrintString(*i);
    v = v2;
    v.insert (v.begin() + 1, v2.begin() + 1, v2.begin() + 1 + 3);
    printf ("After insert(1,1,3):\n");
    foreach (i,v) PrintString(*i);
    v = v2;
    sort (v);
    printf ("After sort:\n");
    foreach (i,v) PrintString(*i);
    el3i = binary_search (v, string("element3"));
    if (el3i)
	printf ("%s found at position %td\n", el3i->c_str(), el3i - v.begin());
    bogusi = binary_search (v, string("bogus"));
    if (bogusi)
	printf ("%s found at position %td\n", bogusi->c_str(), bogusi - v.begin());
}
//}}}-------------------------------------------------------------------
//{{{ TestStreams

void LibTestApp::TestStreams (void) // static
{
    const uint8_t magic_Char = 0x12;
    const uint16_t magic_Short = 0x1234;
    const uint32_t magic_Int = 0x12345678;
    const float magic_Float = 0.12345678;
    const double magic_Double = 0.123456789123456789;
    const bool magic_Bool = true;

    char c = magic_Char;
    unsigned char uc = magic_Char;
    int i = magic_Int;
    short si = magic_Short;
    long li = magic_Int;
    unsigned int ui = magic_Int;
    unsigned short usi = magic_Short;
    unsigned long uli = magic_Int;
    float f = magic_Float;
    double d = magic_Double;
    bool bv = magic_Bool;

    sstream ss;
    ss << c;
    ss << uc;
    ss << ios::talign<bool>() << bv;
    ss << ios::talign<int>() << i;
    ss << ui;
    ss << ios::align() << li;
    ss << uli;
    ss << ios::talign<float>() << f;
    ss << ios::talign<double>() << d;
    ss << si;
    ss << usi;

    memblock b;
    b.resize (ss.size());
    fill (b.begin(), b.end(), 0xcd);
    ostream os (b);

    os << c;
    os << uc;
    os << ios::talign<bool>() << bv;
    os << ios::talign<int>() << i;
    os << ui;
    os << ios::align() << li;
    os << uli;
    os << ios::talign<float>() << f;
    os << ios::talign<double>() << d;
    os << si;
    os << usi;
    if (!os.remaining())
	printf ("Correct number of bytes written\n");
    else
	printf ("Incorrect (%u of %u) number of bytes written\n", b.size()-os.remaining(), b.size());

    c = 0;
    uc = 0;
    bv = false;
    i = ui = li = uli = 0;
    f = 0; d = 0;
    si = usi = 0;

    istream is (b);
    is >> c;
    is >> uc;
    is >> ios::talign<bool>() >> bv;
    is >> ios::talign<int>() >> i;
    is >> ui;
    is >> ios::align() >> li;
    is >> uli;
    is >> ios::talign<float>() >> f;
    is >> ios::talign<double>() >> d;
    is >> si;
    is >> usi;
    if (!is.remaining())
	printf ("Correct number of bytes read\n");
    else
	printf ("Incorrect (%u of %u) number of bytes read\n", b.size()-is.remaining(), b.size());

    printf ("Values:\n"
	"char:    0x%02X\n"
	"u_char:  0x%02X\n"
	"bool:    %d\n"
	"int:     0x%08X\n"
	"u_int:   0x%08X\n"
	"long:    0x%08lX\n"
	"u_long:  0x%08lX\n"
	"float:   %.8f\n"
	"double:  %.15f\n"
	"short:   0x%04X\n"
	"u_short: 0x%04X\n",
	static_cast<int>(c), static_cast<int>(uc), static_cast<int>(bv),
	i, ui, li, uli, f, d, static_cast<int>(si), static_cast<int>(usi));

    if (isatty (STDIN_FILENO)) {
	printf ("\nBinary dump:\n");
	for (auto bi = 0u; bi < b.size(); ++bi) {
	    if (bi && !(bi % 8))
		putchar ('\n');
	    printf ("%02hhx ", (unsigned char) b[bi]);
	}
	putchar ('\n');
    }
}
//}}}-------------------------------------------------------------------
//{{{ Run tests

int LibTestApp::Run (void)
{
    using stdtestfunc_t	= void (*)(void);
    static const stdtestfunc_t c_Tests[] = {
	TestML,
	TestMB,
	TestVector,
	TestMultiset,
	TestString,
	TestStringVector,
	TestStreams
    };
    for (auto i = 0u; i < ArraySize(c_Tests); ++i) {
	printf ("######################################################################\n");
	c_Tests[i]();
    }
    return EXIT_SUCCESS;
}

BEGIN_CWICLO_APP (LibTestApp)
END_CWICLO_APP

//}}}-------------------------------------------------------------------
