// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

// You would normally include cwiclo.h this way
//#include <cwiclo.h>

// But for the purpose of building the tests, local headers are
// included directly to ensure they are used instead of installed.
#include "../app.h"
using namespace cwiclo;

//----------------------------------------------------------------------
// For communication between objects, both sides of the link must be
// defined as well as the interface. The interface object, by convention
// named i_Name, is of type Interface and contains its name, method
// names and signatures, and the dispatch method. The proxy
// "object" is used to send messages to the remote object, and contains
// methods mirroring those on the remote object, that are implemented
// by marshalling the arguments into a message. The remote object is
// created by the framework, using the a factor created by registration.

//----------------------------------------------------------------------
// Interface objects contain the interface name and the names and
// call signatures of its methods. Define this first.
//
struct IPing : public Interface {
    // Methods are usually referred to by index of type imethod_t
    enum : imethod_t { imethod_Ping };
    //
    // The Dispatch method is called from the destination object's
    // aggregate Dispatch method. A templated implementation like
    // this can be inlined for minimum overhead. The return value
    // indicates whether the message was accepted.
    //
    template <typename O>
    bool Dispatch (O* o, const Msg& msg) const {
	// It is the interface's responsibility to check that the message
	// is addressed to it. The object's Dispatch calls every interface.
	// Direct pointer comparison is acceptable.
	if (msg.InterfaceName() != this->name)
	    return false;
	if (msg.IMethod() == imethod_Ping) {
	    // Each method unmarshals the arguments and calls the handling object
	    auto is = msg.Read();
	    uint32_t v;
	    is >> v;
	    o->Ping_Ping (v);
	} else
	    return false;	// protocol errors are handled by caller
	return true;
    }
};
// The interface object will contain the values
extern const IPing i_Ping;

//----------------------------------------------------------------------

// On the calling end is the proxy object:
class PPing : public Proxy {
public:
    // Proxies are constructed with the calling object's oid.
    // Reply messages sent through reply interfaces, here PingR,
    // will be delivered to the given object.
    explicit		PPing (mrid_t caller) : Proxy (caller) {}
    // Methods are implemented by simple marshalling
    void		Ping (uint32_t v) {
			    auto& msg = CreateMsg (&i_Ping, IPing::imethod_Ping, stream_size_of(v));
			    auto os = msg.Write();
			    os << v;
			    CommitMsg (msg, os);
			}
};

//----------------------------------------------------------------------
// Interfaces are always one-way. If replies are desired, a separate
// reply interface must be implemented.

// The naming convention ends the class name with an R.
struct IPingR : public Interface {
    enum : imethod_t { imethod_Ping };
    template <typename O>
    bool Dispatch (O* o, const Msg& msg) const {
	if (msg.InterfaceName() != this->name)
	    return false;
	if (msg.IMethod() == imethod_Ping) {
	    auto is = msg.Read();
	    uint32_t v;
	    is >> v;
	    o->PingR_Ping (v);
	} else
	    return false;	// protocol errors are handled by caller
	return true;
    }
};
extern const IPingR i_PingR;

//----------------------------------------------------------------------

// Reply proxies have their own base class
class PPingR : public ProxyR {
public:
    // Reply proxies are constructed from the owning object's creating
    // link, copied from the message that created it. ProxyR will reverse
    // the link, sending replies to the originating object.
    //
    explicit		PPingR (const Msg::Link& l) : ProxyR (l) {}
    void		Ping (uint32_t v)
			// Using variadic Send is the easiest way to
			// create a message that only marshals arguments.
			    { Send (&i_PingR, IPingR::imethod_Ping, v); }
};

#define LOG(...)	{printf(__VA_ARGS__);fflush(stdout);}

//----------------------------------------------------------------------
// Finally, a server object must be defined that will implement the
// Ping interface by replying with the received argument.

class PingMsger : public Msger {
public:
			PingMsger (const Msg::Link& l)
			    : Msger(l),_reply(l),_nPings(0)
			    { LOG ("Created Ping%hu\n", MsgerId()); }
    virtual		~PingMsger (void) noexcept
			    { LOG ("Destroy Ping%hu\n", MsgerId()); }
    inline void		Ping_Ping (uint32_t v) {
			    LOG ("Ping%hu: %u, %u total\n", MsgerId(), v, ++_nPings);
			    _reply.Ping (v);
			}
    virtual bool	Dispatch (const Msg& msg) noexcept override {
			    return i_Ping.Dispatch (this, msg)
					|| Msger::Dispatch(msg);
			}
private:
    PPingR		_reply;
    uint32_t		_nPings;
};
