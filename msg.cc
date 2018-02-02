// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "msg.h"
#include "app.h"

namespace cwiclo {

methodid_t LookupInterfaceMethod (iid_t iid, const char* __restrict__ mname, size_t mnamesz) noexcept
{
    for (methodid_t __restrict__ mid = iid+iid[-1]; mid[0]; mid += mid[0])
	if (uint8_t(mid[0]) == mnamesz && 0 == memcmp (mname, mid+2, mnamesz))
	    return mid+2;
    return nullptr;
}

//----------------------------------------------------------------------

Msg::Msg (const Link& l, methodid_t mid, streamsize size, mrid_t extid, fdoffset_t fdo)
:_method (mid)
,_link (l)
,_extid (extid)
,_fdoffset (fdo)
,_body (size)
{
}

streamsize Msg::Verify (void) const noexcept
{
    return _body.size();
}

//----------------------------------------------------------------------

Msg& ProxyB::CreateMsg (methodid_t mid, streamsize sz) noexcept
{
    return App::Instance().CreateMsg (_link, mid, sz);
}

#ifndef NDEBUG
void ProxyB::CommitMsg (Msg& msg, ostream& os) noexcept
{
    assert (!os.remaining() && "Message body written size does not match requested size");
    assert (msg.Size() == msg.Verify() && "Message body does not match method signature");
}
#endif

} // namespace cwiclo
