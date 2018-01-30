// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "msg.h"
#include "app.h"

namespace cwiclo {

Msg::Msg (const Link& l, iid_t iid, imethod_t imethod, streamsize size, mrid_t extid, fdoffset_t fdo)
:_iid(iid)
,_link(l)
,_imethod(imethod)
,_fdoffset(fdo)
,_extid(extid)
,_body(size)
{
}

streamsize Msg::Verify (void) const noexcept
{
    return _body.size();
}

//----------------------------------------------------------------------

Msg& Proxy::CreateMsg (iid_t iid, imethod_t imethod, streamsize sz) noexcept
{
    return App::Instance().CreateMsg (_link, iid, imethod, sz);
}

#ifndef NDEBUG
void Proxy::CommitMsg (Msg& msg, ostream& os) noexcept
{
    assert (!os.remaining() && "Message body written size does not match requested size");
    assert (msg.Size() == msg.Verify() && "Message body does not match method signature");
}
#endif

} // namespace cwiclo
