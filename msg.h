// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "vector.h"
#include "string.h"
#include "metamac.h"

//{{{ Types ------------------------------------------------------------
namespace cwiclo {

using mrid_t = uint16_t;
enum : mrid_t {
    mrid_New,
    mrid_First,
    mrid_App = mrid_First,
    mrid_Last = numeric_limits<mrid_t>::max()-1,
    mrid_Broadcast
};

using imethod_t = int8_t;

//}}}-------------------------------------------------------------------
//{{{ Interface

struct Interface {
    const char*	name;
    const char* const* method;
public:
    inline constexpr auto Method (imethod_t i) const { return method[i]; }
};
using iid_t = const Interface*;

// Use this to define the i_Interface variables
// Example: DEFINE_INTERFACE (MyInterface, "Call1\0uix", "Call2\0x");
#define DEFINE_INTERFACE(iface,...)\
    const I##iface i_##iface = { { #iface, (const char*[]) { __VA_ARGS__, nullptr }} }

//}}}-------------------------------------------------------------------
//{{{ Msg

class Msg {
public:
    struct Link {
	mrid_t	src;
	mrid_t	dest;
    };
    using fdoffset_t = uint8_t;
    enum {
	NO_FD_IN_MESSAGE = numeric_limits<fdoffset_t>::max(),
	MESSAGE_HEADER_ALIGNMENT = 8,
	MESSAGE_BODY_ALIGNMENT = MESSAGE_HEADER_ALIGNMENT,
	MESSAGE_FD_ALIGNMENT = alignof(int)
    };
public:
			Msg (const Link& l, iid_t iid, imethod_t imethod, streamsize size, mrid_t extid = 0, fdoffset_t fdo = NO_FD_IN_MESSAGE);
    inline auto&	GetLink (void) const	{ return _link; }
    inline auto		Src (void) const	{ return GetLink().src; }
    inline auto		Dest (void) const	{ return GetLink().dest; }
    inline auto		Size (void) const	{ return _body.size(); }
    inline auto		GetInterface (void) const	{ return _iid; }
    inline auto		InterfaceName (void) const	{ return GetInterface()->name; }
    inline auto		IMethod (void) const		{ return _imethod; }
    inline auto		Method (void) const	{ return GetInterface()->Method(IMethod()); }
    inline auto		Extid (void) const	{ return _extid; }
    inline auto		FdOffset (void) const	{ return _fdoffset; }
    inline istream	Read (void) const	{ return istream (_body); }
    inline ostream	Write (void)		{ return ostream (_body); }
    streamsize		Verify (void) const noexcept;
private:
    iid_t		_iid;
    Link		_link;
    imethod_t		_imethod;
    fdoffset_t		_fdoffset;
    mrid_t		_extid;
    memblock		_body;
};

//}}}-------------------------------------------------------------------
//{{{ Proxy

class Proxy {
public:
    constexpr explicit	Proxy (mrid_t from, mrid_t to=mrid_New)	: _link {from,to} {}
			Proxy (const Proxy&) = delete;
    void		operator= (const Proxy&) = delete;
    constexpr auto&	Link (void) const			{ return _link; }
    constexpr auto	Src (void) const			{ return Link().src; }
    constexpr auto	Dest (void) const			{ return Link().dest; }
    Msg&		CreateMsg (iid_t iid, imethod_t imethod, streamsize sz) noexcept;
#ifdef NDEBUG	// CommitMsg only does debug checking
    void		CommitMsg (Msg&, ostream&) noexcept	{ }
#else
    void		CommitMsg (Msg& msg, ostream& os) noexcept;
#endif
    template <typename... Args>
    inline void Send (iid_t iid, imethod_t imethod, const Args&... args) {
	auto& msg = CreateMsg (iid, imethod, variadic_stream_size(args...));
	auto os = msg.Write();
	(os << ... << args);
	CommitMsg (msg, os);
    }
private:
    Msg::Link		_link;
};

class ProxyR : public Proxy {
public:
    explicit		ProxyR (const Msg::Link& l) : Proxy(l.dest, l.src) {}
};

//}}}-------------------------------------------------------------------
//{{{ Msger

class Msger {
public:
    enum { f_Unused, f_Static, f_Last };
public:
    virtual		~Msger (void) noexcept		{ }
    auto		CreatorId (void) const		{ return _link.src; }
    auto		MsgerId (void) const		{ return _link.dest; }
    auto		Flag (unsigned f) const		{ return GetBit(_flags,f); }
    virtual bool	Dispatch (const Msg&) noexcept	{ return false; }
    virtual bool	OnError (mrid_t, const string&) noexcept
			    { SetFlag (f_Unused); return false; }
    virtual void	OnMsgerDestroyed (mrid_t mid) noexcept
			    { if (mid == CreatorId()) SetFlag (f_Unused); }
protected:
    explicit		Msger (const Msg::Link& l)	:_link(l),_flags() {}
    explicit		Msger (mrid_t id)		:_link{id,id},_flags(1u<<f_Static) {}
			Msger (const Msger&) = delete;
    void		operator= (const Msger&) = delete;
    void		SetFlag (unsigned f, bool v = true)	{ SetBit(_flags,f,v); }
private:
    Msg::Link		_link;
    uint32_t		_flags;
};

//{{{2 has_msger_named_create ------------------------------------------

template <typename T> class __has_msger_named_create {
    template <typename O, T& (*)(const Msg::Link&)> struct test_for_Create {};
    template <typename O> static true_type found (test_for_Create<O,&O::Create>*);
    template <typename O> static false_type found (...);
public:
    using type = decltype(found<T>(nullptr));
};
// Differentiates between normal and singleton Msger classes
template <typename T>
struct has_msger_named_create : public __has_msger_named_create<T>::type {};

//}}}2------------------------------------------------------------------

} // namespace cwiclo
//}}}-------------------------------------------------------------------
