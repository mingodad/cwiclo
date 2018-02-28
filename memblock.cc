// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "memblock.h"
#include "stream.h"
#include <fcntl.h>
#include <sys/stat.h>

namespace cwiclo {

/// Reads the object from stream \p s
void cmemlink::link_read (istream& is, size_type elsize) noexcept
{
    assert (!capacity() && "allocated memory in cmemlink; deallocate or unlink first");
    size_type n; is >> n; n *= elsize;
    auto nskip = Align (n, stream_align<size_type>::value);
    if (is.remaining() < nskip)
	return;	// errors should have been reported by the message validator
    auto p = is.ptr<value_type>();
    is.skip (nskip);
    if (zero_terminated()) {
	if (!n)
	    --p;	// point to the end of n, which is zero
	else
	    --n;	// clip the zero from size
    }
    link (p, n);
}

void cmemlink::write (ostream& os, size_type elsize) const noexcept
{
    auto sz = size();
    if (sz)
	sz += zero_terminated();
    os << size_type(sz/elsize);
    os.write (data(), sz);
    os.align (stream_align<size_type>::value);
}

void cmemlink::write (sstream& os, size_type elsize) const noexcept
{
    auto sz = size();
    if (sz)
	sz += zero_terminated();
    os << size_type(sz/elsize);
    os.write (data(), sz);
    os.align (stream_align<size_type>::value);
}

int cmemlink::write_file (const char* filename) const noexcept
{
    int fd = open (filename, O_WRONLY| O_TRUNC| O_CREAT| O_CLOEXEC);
    if (fd < 0)
	return -1;
    int r = complete_write (fd, data(), size());
    if (0 > close (fd))
	return -1;
    return r;
}

// Write to a temp file, close it, and move it over filename.
// This method ensures that filename will always be valid, with
// incomplete or otherwise failed writes will end up in a temp file.
//
int cmemlink::write_file_atomic (const char* filename) const noexcept
{
    char tmpfilename [PATH_MAX];
    snprintf (ArrayBlock(tmpfilename), "%s.XXXXXX", filename);

    // Create the temp file
    int ofd = mkstemp (tmpfilename);
    if (ofd < 0)
	return -1;

    // Write the data, minding EINTR
    auto bw = complete_write (ofd, data(), size());
    if (0 > close (ofd))
	return -1;

    // If everything went well, can overwrite the old file
    if (bw >= 0 && 0 > rename (tmpfilename, filename))
	return -1;
    return bw;
}

//----------------------------------------------------------------------

auto memlink::insert (const_iterator ii, size_type n) noexcept -> iterator
{
    assert (data() || !n);
    auto istart = const_cast<iterator>(ii), iend = istart + n;
    assert (istart >= begin() && iend <= end());
    memmove (iend, istart, end()-iend);
    return istart;
}

auto memlink::erase (const_iterator ie, size_type n) noexcept -> iterator
{
    assert (data() || !n);
    auto istart = const_cast<iterator>(ie), iend = istart + n;
    assert (istart >= begin() && iend <= end());
    memmove (istart, iend, end()-iend);
    return istart;
}

//----------------------------------------------------------------------

void memblock::reserve (size_type sz) noexcept
{
    if ((sz += zero_terminated()) <= capacity())
	return;
    sz = NextPow2 (sz);
    auto oldBlock (capacity() ? data() : nullptr);
    auto newBlock = reinterpret_cast<pointer> (_realloc (oldBlock, sz));
    if (!oldBlock && data())
	memcpy (newBlock, data(), min (size() + zero_terminated(), sz));
    link (newBlock, size());
    set_capacity (sz);
}

void memblock::deallocate (void) noexcept
{
    if (capacity()) {
	assert (data() && "Internal error: space allocated, but the pointer is nullptr");
	free (data());
    }
    unlink();
}

void memblock::shrink_to_fit (void) noexcept
{
    assert (capacity() && "call copy_link first");
    const auto sz = size();
    auto newBlock = reinterpret_cast<pointer> (realloc (data(), sz));
    if (newBlock || !sz) {
	link (newBlock, sz);
	set_capacity (sz);
    }
}

void memblock::resize (size_type sz) noexcept
{
    reserve (sz);
    memlink::resize (sz);
    if (zero_terminated())
	*end() = 0;
}

void memblock::assign (const_pointer p, size_type sz) noexcept
{
    resize (sz);
    memcpy (data(), p, sz);
}

auto memblock::insert (const_iterator start, size_type n) noexcept -> iterator
{
    const auto ip = start - begin();
    assert (ip <= size());
    resize (size() + n);
    memlink::insert (iat(ip), n);
    if (zero_terminated())
	*end() = 0;
    return iat (ip);
}

/// Shifts the data in the linked block from \p start + \p n to \p start.
auto memblock::erase (const_iterator start, size_type n) noexcept -> iterator
{
    const auto ep = start - begin();
    assert (ep + n <= size());
    memlink::erase (start, n);
    memlink::resize (size() - n);
    if (zero_terminated())
	*end() = 0;
    return iat (ep);
}

void memblock::replace (const_iterator ip, size_type ipn, const_pointer s, size_type sn) noexcept
{
    auto dsz = difference_type(sn) - ipn;
    auto ipw = (dsz > 0 ? insert (ip, dsz) : erase (ip, -dsz));
    memcpy (ipw, s, sn);
}

/// Reads the object from stream \p s
void memblock::read (istream& is, size_type elsize) noexcept
{
    size_type n; is >> n; n *= elsize;
    auto nskip = Align (n, stream_align<size_type>::value);
    if (is.remaining() < nskip)
	return;	// errors should have been reported by the message validator
    if (zero_terminated() && n)
	--n;
    reserve (nskip);
    memlink::resize (n);
    is.read (data(), nskip);
}

int memblock::read_file (const char* filename) noexcept
{
    int fd = open (filename, O_RDONLY);
    if (fd < 0)
	return -1;
    auto autoclose = make_scope_exit ([&]{ close(fd); });
    struct stat st;
    if (0 > fstat (fd, &st))
	return -1;
    resize (st.st_size);
    return complete_read (fd, data(), st.st_size);
}

} // namespace cwiclo
