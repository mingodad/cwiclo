// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "app.h"
#include <sys/socket.h>
#include <netinet/in.h>

//{{{ COM --------------------------------------------------------------
namespace cwiclo {

class PCOM : public Proxy {
    DECLARE_INTERFACE (COM, (Error,"s")(Export,"s")(Delete,""))
public:
		PCOM (mrid_t src, mrid_t dest)	: Proxy (src, dest) {}
		~PCOM (void) noexcept		{ FreeId(); }
    void	Error (const string& errmsg)	{ Send (M_Error(), errmsg); }
    void	Export (const string& elist)	{ Send (M_Export(), elist); }
    void	Delete (void)			{ Send (M_Delete()); }
    void	Forward (Msg&& msg)		{ Proxy::Forward (move(msg)); }
  static string	StringFromInterfaceList (const iid_t* elist) noexcept;
    static Msg	ErrorMsg (mrid_t extid, const string& errmsg) noexcept;
    static Msg	ExportMsg (mrid_t extid, const string& elstr) noexcept;
    static Msg	ExportMsg (mrid_t extid, const iid_t* elist) noexcept
							{ return ExportMsg (extid, StringFromInterfaceList (elist)); }
    static Msg	DeleteMsg (mrid_t extid) noexcept	{ return Msg (Msg::Link{}, PCOM::M_Delete(), 0, extid); }
    template <typename O>
    static bool	Dispatch (O* o, const Msg& msg) noexcept {
	if (msg.Method() == M_Error())
	    o->COM_Error (msg.Read().readv<lstring>());
	else if (msg.Method() == M_Export())
	    o->COM_Export (msg.Read().readv<string>());
	else if (msg.Method() == M_Delete())
	    o->COM_Delete ();
	else
	    return false;
	return true;
    }
};

//}}}-------------------------------------------------------------------
//{{{ PExtern

class PExtern : public Proxy {
    DECLARE_INTERFACE (Extern, (Open,"xib")(Close,""))
public:
    explicit	PExtern (mrid_t caller)	: Proxy(caller) {}
		~PExtern (void)		{ FreeId(); }
    void	Close (void)		{ Send (M_Close()); }
    void	Open (int fd, const iid_t* eifaces, bool isServer = true)
		    { Send (M_Open(), eifaces, fd, isServer); }
    void	Open (int fd)		{ Open (fd, nullptr, false); }
    int		Connect (const sockaddr* addr, socklen_t addrlen) noexcept;
    int		ConnectIP4 (in_addr_t ip, in_port_t port) noexcept;
    int		ConnectIP6 (in6_addr ip, in_port_t port) noexcept;
    int		ConnectLocal (const char* path) noexcept;
    int		ConnectLocalIP4 (in_port_t port) noexcept;
    int		ConnectLocalIP6 (in_port_t port) noexcept;
    int		ConnectSystemLocal (const char* sockname) noexcept;
    int		ConnectUserLocal (const char* sockname) noexcept;
    int		LaunchPipe (const char* exe, const char* arg) noexcept;
    template <typename O>
    inline static bool Dispatch (O* o, const Msg& msg) noexcept {
	if (msg.Method() == M_Open()) {
	    auto is = msg.Read();
	    auto eifaces = is.readv<const iid_t*>();
	    auto fd = is.readv<int>();
	    auto isServer = is.readv<bool>();
	    o->Extern_Open (fd, eifaces, isServer);
	} else if (msg.Method() == M_Close())
	    o->Extern_Close();
	else
	    return false;
	return true;
    }
};

//}}}-------------------------------------------------------------------
//{{{ ExternInfo

struct ExternInfo {
    vector<iid_t>	imported;
    const iid_t*	exported;
    struct ucred	creds;
    mrid_t		oid;
    bool		isServer;
    bool		isUnixSocket;
public:
    auto IsImporting (iid_t iid) const
	{ return linear_search (imported, iid); }
    bool IsExporting (iid_t iid) const {
	for (auto e = exported; e && *e; ++e)
	    if (*e == iid)
		return true;
	return false;
    }
};

//}}}-------------------------------------------------------------------
//{{{ PExternR

class PExternR : public ProxyR {
    DECLARE_INTERFACE (ExternR, (Connected,"x"))
public:
    explicit	PExternR (const Msg::Link& l)		: ProxyR(l) {}
    void	Connected (const ExternInfo* einfo)	{ Send (M_Connected(), einfo); }
    template <typename O>
    inline static bool Dispatch (O* o, const Msg& msg) noexcept {
	if (msg.Method() != M_Connected())
	    return false;
	o->ExternR_Connected (msg.Read().readv<const ExternInfo*>());
	return true;
    }

};

//}}}-------------------------------------------------------------------
//{{{ COMRelay

class Extern;

class COMRelay : public Msger {
public:
    explicit		COMRelay (const Msg::Link& l) noexcept;
    virtual		~COMRelay (void) noexcept override;
    virtual bool	Dispatch (Msg& msg) noexcept override;
    virtual bool	OnError (mrid_t eid, const string& errmsg) noexcept override;
    virtual void	OnMsgerDestroyed (mrid_t id) noexcept override;
    inline void		COM_Error (const string& errmsg) noexcept;
    inline void		COM_Export (const string& elist) noexcept;
    inline void		COM_Delete (void) noexcept;
private:
    Extern*	_pExtern;	// Outgoing connection object
    PCOM	_localp;	// Proxy to the local object
    mrid_t	_extid;		// Extern link id
};

//}}}-------------------------------------------------------------------
//{{{ Extern

class Extern : public Msger {
protected:
    enum {
	// Each extern connection has two sides and each side must be able
	// to assign a unique extid to each Msger-Msger link across the socket.
	// Msger ids for COMRelays are unique for each process, and so can be
	// used as an extid on one side. The other side, however, needs another
	// address space. Here it is provided as extid_ServerBase. Extern
	// connection is bidirectional and has no functional difference between
	// sides, so selecting a server side is an arbitrary choice. By default,
	// the side that binds to the connection socket is the server and the
	// side that connects is the client.
	//
	extid_ClientBase,
	extid_COM = extid_ClientBase,
	// and mrid + halfrange on the server
	// (32000 is 0x7d00, mostly round number in both bases; helpful in the debugger)
	extid_ServerBase = extid_ClientBase+32000,
	extid_ClientLast = extid_ServerBase-1,
	extid_ServerLast = extid_ServerBase+(extid_ClientLast-extid_ClientBase)
    };
public:
    explicit		Extern (const Msg::Link& l) noexcept;
    virtual		~Extern (void) noexcept;
    auto&		Info (void) const	{ return _einfo; }
    virtual bool	Dispatch (Msg& msg) noexcept override;
    void		QueueOutgoing (Msg&& msg) noexcept;
    static Extern*	LookupById (mrid_t id) noexcept;
    static Extern*	LookupByImported (iid_t id) noexcept;
    mrid_t		RegisterRelay (const COMRelay* relay) noexcept;
    void		UnregisterRelay (const COMRelay* relay) noexcept;
    inline void		Extern_Open (int fd, const iid_t* eifaces, bool isServer) noexcept;
    void		Extern_Close (void) noexcept;
    inline void		COM_Error (const string& errmsg) noexcept;
    inline void		COM_Export (string&& elist) noexcept;
    inline void		COM_Delete (void) noexcept;
    void		TimerR_Timer (int fd) noexcept;
private:
    //{{{2 ExtMsg ------------------------------------------------------
    // Message formatted for reading/writing to socket
    class ExtMsg {
    public:
	struct alignas(8) Header {
	    uint32_t	sz;		// Message body size, aligned to c_MsgAlignment
	    uint16_t	extid;		// Destination node mrid
	    uint8_t	fdoffset;	// Offset to file descriptor in message body, if passing
	    uint8_t	hsz;		// Full size of header
	};
	enum {
	    MIN_MSG_HEADER_SIZE = Align (sizeof(Header)+sizeof("i\0m\0"), Msg::HEADER_ALIGNMENT),
	    MAX_MSG_HEADER_SIZE = UINT8_MAX-sizeof(Header),
	    MAX_MSG_BODY_SIZE = (1<<24)-1
	};
    public:
			ExtMsg (void)		: _body(),_h{},_hbuf{} {}
	inline		ExtMsg (Msg&& msg) noexcept;
	streamsize	HeaderSize (void) const	{ return _h.hsz; }
	auto&		GetHeader (void) const	{ return _h; }
	streamsize	BodySize (void) const	{ return _h.sz; }
	auto		Extid (void) const	{ return _h.extid; }
	auto		FdOffset (void) const	{ return _h.fdoffset; }
	streamsize	Size (void) const	{ return BodySize() + HeaderSize(); }
	bool		HasFd (void) const	{ return FdOffset() != Msg::NO_FD_INCLUDED; }
	void		SetHeader (const Header& h)	{ _h = h; }
	void		ResizeBody (streamsize sz)	{ _body.resize (sz); }
	void		TrimBody (streamsize sz)	{ _body.memlink::resize (sz); }
	auto&&		MoveBody (void)		{ return move(_body); }
	void		SetPassedFd (int fd)	{ assert (HasFd()); ostream os (_body.iat(_h.fdoffset), sizeof(fd)); os << fd; }
	int		PassedFd (void) const noexcept;
	void		WriteIOVecs (iovec* iov, streamsize bw) noexcept;
	istream		Read (void) const	{ return istream (_body.data(), _body.size()); }
	methodid_t	ParseMethod (void) const noexcept;
	inline void	DebugDump (void) const noexcept;
    private:
	const char*	HeaderPtr (void) const	{ auto hp = _hbuf; return hp-sizeof(_h); }
	char*		HeaderPtr (void)	{ return const_cast<char*>(const_cast<const ExtMsg*>(this)->HeaderPtr()); }
	uint8_t		WriteHeaderStrings (methodid_t method) noexcept;
    private:
	memblock	_body;
	Header		_h;
	char		_hbuf [MAX_MSG_HEADER_SIZE];
    };
    //}}}2--------------------------------------------------------------
    //{{{2 RelayProxy
    struct RelayProxy {
	const COMRelay*	pRelay;
	PCOM		relay;
	mrid_t		extid;
    public:
	RelayProxy (mrid_t src, mrid_t dest, mrid_t eid)
	    : pRelay(), relay(src,dest), extid(eid) {}
    };
    //}}}2--------------------------------------------------------------
private:
    mrid_t		CreateExtidFromRelayId (mrid_t id) const noexcept
			    { return id + (_einfo.isServer ? extid_ServerBase : extid_ClientBase); }
    static auto&	ExternList (void) noexcept
			    { static vector<Extern*> s_ExternList; return s_ExternList; }
    RelayProxy*		RelayProxyByExtid (mrid_t extid) noexcept;
    RelayProxy*		RelayProxyById (mrid_t id) noexcept;
    bool		WriteOutgoing (void) noexcept;
    void		ReadIncoming (void) noexcept;
    inline bool		AcceptIncomingMessage (void) noexcept;
    inline bool		ValidateSocket (int fd) noexcept;
    void		EnableCredentialsPassing (int enable) noexcept;
private:
    int			_sockfd;
    PTimer		_timer;
    PExternR		_reply;
    streamsize		_bwritten;
    vector<ExtMsg>	_outq;		// messages queued for export
    vector<RelayProxy>	_relays;
    ExternInfo		_einfo;
    streamsize		_bread;
    ExtMsg		_inmsg;		// currently incoming message
    int			_infd;
};

#define REGISTER_EXTERNS\
    REGISTER_MSGER (Extern, Extern)\
    REGISTER_MSGER (COM, COMRelay)\
    REGISTER_MSGER (Timer, App::Timer)

#define REGISTER_EXTERN_MSGER(iface)\
    REGISTER_MSGER (iface, COMRelay)

//}}}-------------------------------------------------------------------
//{{{ PExternServer

class PExternServer : public Proxy {
    DECLARE_INTERFACE (ExternServer, (Open,"xib")(Close,""))
public:
    explicit	PExternServer (mrid_t caller)	: Proxy(caller),_sockname(nullptr) {}
		~PExternServer (void) noexcept;
    void	Close (void)			{ Send (M_Close()); }
    void	Open (int fd, const iid_t* eifaces, bool closeWhenEmpty = true)
		    { Send (M_Open(), eifaces, fd, closeWhenEmpty); }
    int		Bind (const sockaddr* addr, socklen_t addrlen, const iid_t* eifaces) noexcept NONNULL();
    int		BindLocal (const char* path, const iid_t* eifaces) noexcept NONNULL();
    int		BindUserLocal (const char* sockname, const iid_t* eifaces) noexcept NONNULL();
    int		BindSystemLocal (const char* sockname, const iid_t* eifaces) noexcept NONNULL();
    int		BindIP4 (in_addr_t ip, in_port_t port, const iid_t* eifaces) noexcept NONNULL();
    int		BindLocalIP4 (in_port_t port, const iid_t* eifaces) noexcept NONNULL();
    int		BindIP6 (in6_addr ip, in_port_t port, const iid_t* eifaces) noexcept NONNULL();
    int		BindLocalIP6 (in_port_t port, const iid_t* eifaces) noexcept NONNULL();
    template <typename O>
    inline static bool	Dispatch (O* o, const Msg& msg) noexcept {
	if (msg.Method() == M_Open()) {
	    auto is = msg.Read();
	    auto eifaces = is.readv<const iid_t*>();
	    auto fd = is.readv<int>();
	    auto closeWhenEmpty = is.readv<bool>();
	    o->ExternServer_Open (fd, eifaces, closeWhenEmpty);
	} else if (msg.Method() == M_Close())
	    o->ExternServer_Close();
	else
	    return false;
	return true;
    }
private:
    char*	_sockname;
};

//}}}-------------------------------------------------------------------
//{{{ ExternServer

class ExternServer : public Msger {
    enum { f_CloseWhenEmpty = Msger::f_Last, f_Last };
public:
			ExternServer (const Msg::Link& l) noexcept;
    virtual bool	OnError (mrid_t eid, const string& errmsg) noexcept override;
    virtual void	OnMsgerDestroyed (mrid_t mid) noexcept override;
    virtual bool	Dispatch (Msg& msg) noexcept override;
    inline void		TimerR_Timer (int) noexcept;
    inline void		ExternServer_Open (int fd, const iid_t* eifaces, bool closeWhenEmpty) noexcept;
    inline void		ExternServer_Close (void) noexcept;
    inline void		ExternR_Connected (const ExternInfo* einfo) noexcept;
private:
    PTimer		_timer;
    PExternR		_reply;
    int			_sockfd;
    const iid_t*	_eifaces;
    vector<PExtern>	_conns;
};

} // namespace cwiclo
//}}}-------------------------------------------------------------------
