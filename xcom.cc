// This file is part of the casycom project
//
// Copyright (c) 2015 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "xcom.h"
#include <fcntl.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <paths.h>
#if __has_include(<arpa/inet.h>) && !defined(NDEBUG)
    #include <arpa/inet.h>
#endif

//{{{ COM --------------------------------------------------------------
namespace cwiclo {

DEFINE_INTERFACE (COM)

string PCOM::StringFromInterfaceList (const iid_t* elist) noexcept // static
{
    string elstr;
    if (elist && *elist) {
	for (auto e = elist; *e; ++e)
	    elstr.appendf ("%s,", *e);
	elstr.pop_back();
    }
    return elstr;
}

Msg PCOM::ExportMsg (mrid_t extid, const string& elstr) noexcept // static
{
    Msg msg (Msg::Link{}, PCOM::M_Export(), stream_size_of(elstr), extid);
    auto os = msg.Write();
    os << elstr;
    return msg;
}

Msg PCOM::ErrorMsg (mrid_t extid, const string& errmsg) noexcept // static
{
    Msg msg (Msg::Link{}, PCOM::M_Error(), stream_size_of(errmsg), extid);
    auto os = msg.Write();
    os << errmsg;
    return msg;
}

//}}}-------------------------------------------------------------------
//{{{ PExtern

DEFINE_INTERFACE (Extern)

int PExtern::Connect (const sockaddr* addr, socklen_t addrlen) noexcept
{
    int fd = socket (addr->sa_family, SOCK_STREAM| SOCK_NONBLOCK| SOCK_CLOEXEC, IPPROTO_IP);
    if (fd < 0)
	return fd;
    if (0 > connect (fd, addr, addrlen) && errno != EINPROGRESS && errno != EINTR) {
	DEBUG_PRINTF ("[E] Failed to connect to socket: %s\n", strerror(errno));
	close (fd);
	return fd = -1;
    }
    Open (fd);
    return fd;
}

/// Create local socket with given path
int PExtern::ConnectLocal (const char* path) noexcept
{
    sockaddr_un addr;
    addr.sun_family = PF_LOCAL;
    if ((int) ArraySize(addr.sun_path) <= snprintf (ArrayBlock(addr.sun_path), "%s", path)) {
	errno = ENAMETOOLONG;
	return -1;
    }
    DEBUG_PRINTF ("[X] Connecting to socket %s\n", addr.sun_path);
    return Connect ((const sockaddr*) &addr, sizeof(addr));
}

/// Create local socket of the given name in the system standard location for such
int PExtern::ConnectSystemLocal (const char* sockname) noexcept
{
    sockaddr_un addr;
    addr.sun_family = PF_LOCAL;
    if ((int) ArraySize(addr.sun_path) <= snprintf (ArrayBlock(addr.sun_path), _PATH_VARRUN "%s", sockname)) {
	errno = ENAMETOOLONG;
	return -1;
    }
    DEBUG_PRINTF ("[X] Connecting to socket %s\n", addr.sun_path);
    return Connect ((const sockaddr*) &addr, sizeof(addr));
}

/// Create local socket of the given name in the user standard location for such
int PExtern::ConnectUserLocal (const char* sockname) noexcept
{
    sockaddr_un addr;
    addr.sun_family = PF_LOCAL;
    const char* runtimeDir = getenv ("XDG_RUNTIME_DIR");
    if (!runtimeDir)
	runtimeDir = _PATH_TMP;
    if ((int) ArraySize(addr.sun_path) <= snprintf (ArrayBlock(addr.sun_path), "%s/%s", runtimeDir, sockname)) {
	errno = ENAMETOOLONG;
	return -1;
    }
    DEBUG_PRINTF ("[X] Connecting to socket %s\n", addr.sun_path);
    return Connect ((const sockaddr*) &addr, sizeof(addr));
}

int PExtern::ConnectIP4 (in_addr_t ip, in_port_t port) noexcept
{
    sockaddr_in addr = {};
    addr.sin_family = PF_INET;
    addr.sin_port = port;
    #ifdef UC_VERSION
	addr.sin_addr = ip;
    #else
	addr.sin_addr = { ip };
    #endif
#ifndef NDEBUG
    char addrbuf [64];
    #ifdef UC_VERSION
	DEBUG_PRINTF ("[X] Connecting to socket %s:%hu\n", inet_intop(addr.sin_addr, addrbuf, sizeof(addrbuf)), port);
    #else
	DEBUG_PRINTF ("[X] Connecting to socket %s:%hu\n", inet_ntop(PF_INET, &addr.sin_addr, addrbuf, sizeof(addrbuf)), port);
    #endif
#endif
    return Connect ((const sockaddr*) &addr, sizeof(addr));
}

/// Create local IPv4 socket at given port on the loopback interface
int PExtern::ConnectLocalIP4 (in_port_t port) noexcept
    { return PExtern::ConnectIP4 (INADDR_LOOPBACK, port); }

/// Create local IPv6 socket at given ip and port
int PExtern::ConnectIP6 (in6_addr ip, in_port_t port) noexcept
{
    sockaddr_in6 addr = {};
    addr.sin6_family = PF_INET6;
    addr.sin6_addr = ip;
    addr.sin6_port = port;
#if !defined(NDEBUG) && !defined(UC_VERSION)
    char addrbuf [128];
    DEBUG_PRINTF ("[X] Connecting to socket %s:%hu\n", inet_ntop(PF_INET6, &addr.sin6_addr, addrbuf, sizeof(addrbuf)), port);
#endif
    return Connect ((const sockaddr*) &addr, sizeof(addr));
}

/// Create local IPv6 socket at given ip and port
int PExtern::ConnectLocalIP6 (in_port_t port) noexcept
{
    sockaddr_in6 addr = {};
    addr.sin6_family = PF_INET6;
    addr.sin6_addr = IN6ADDR_LOOPBACK_INIT;
    addr.sin6_port = port;
    DEBUG_PRINTF ("[X] Connecting to socket localhost6:%hu\n", port);
    return Connect ((const sockaddr*) &addr, sizeof(addr));
}

int PExtern::LaunchPipe (const char* exe, const char* arg) noexcept
{
    // Check if executable exists before the fork to allow proper error handling
    char exepath [PATH_MAX];
    const char* exefp = executable_in_path (exe, ArrayBlock(exepath));
    if (!exefp) {
	errno = ENOENT;
	return -1;
    }

    // Create socket pipe, will be connected to stdin in server
    enum { socket_ClientSide, socket_ServerSide, socket_N };
    int socks [socket_N];
    if (0 > socketpair (PF_LOCAL, SOCK_STREAM| SOCK_NONBLOCK, 0, socks))
	return -1;

    int fr = fork();
    if (fr < 0) {
	close (socks[socket_ClientSide]);
	close (socks[socket_ServerSide]);
	return -1;
    }
    if (fr == 0) {	// Server side
	close (socks[socket_ClientSide]);
	dup2 (socks[socket_ServerSide], STDIN_FILENO);
	execl (exefp, exe, arg, NULL);

	// If exec failed, log the error and exit
	Msger::Error ("failed to launch pipe to '%s %s': %s\n", exe, arg, strerror(errno));
	exit (EXIT_FAILURE);
    }
    // Client side
    close (socks[socket_ServerSide]);
    Open (socks[socket_ClientSide]);
    return socks[socket_ClientSide];
}

//}}}-------------------------------------------------------------------
//{{{ PExternR

DEFINE_INTERFACE (ExternR)

//}}}-------------------------------------------------------------------
//{{{ Extern

Extern::Extern (const Msg::Link& l) noexcept
: Msger (l)
,_sockfd (-1)
,_timer (MsgerId())
,_reply (l)
,_bwritten (0)
,_outq()
,_relays()
,_einfo{}
,_bread (0)
,_inmsg()
,_infd (-1)
{
    _relays.emplace_back (MsgerId(), MsgerId(), extid_COM);
    ExternList().push_back (this);
}

Extern::~Extern (void) noexcept
{
    Extern_Close();
    remove_if (ExternList(), [&](const Extern* e)
	{ return e == this; });
}

bool Extern::Dispatch (Msg& msg) noexcept
{
    return PTimerR::Dispatch (this,msg)
	|| PExtern::Dispatch (this,msg)
	|| PCOM::Dispatch (this,msg)
	|| Msger::Dispatch (msg);
}

void Extern::QueueOutgoing (Msg&& msg) noexcept
{
    _outq.emplace_back (move (msg));
    TimerR_Timer (_sockfd);
}

Extern::RelayProxy* Extern::RelayProxyById (mrid_t id) noexcept
{
    return linear_search_if (_relays, [&](const RelayProxy& r)
	    { return r.relay.Dest() == id; });
}

Extern::RelayProxy* Extern::RelayProxyByExtid (mrid_t extid) noexcept
{
    return linear_search_if (_relays, [&](const RelayProxy& r)
	    { return r.extid == extid; });
}

mrid_t Extern::RegisterRelay (const COMRelay* relay) noexcept
{
    auto rp = RelayProxyById (relay->MsgerId());
    if (!rp)
	rp = &_relays.emplace_back (MsgerId(), relay->MsgerId(), CreateExtidFromRelayId (relay->MsgerId()));
    rp->pRelay = relay;
    return rp->extid;
}

void Extern::UnregisterRelay (const COMRelay* relay) noexcept
{
    auto rp = RelayProxyById (relay->MsgerId());
    if (rp)
	_relays.erase (rp);
}

Extern* Extern::LookupById (mrid_t id) noexcept // static
{
    auto ep = linear_search_if (ExternList(), [&](auto e)
		{ return e->MsgerId() == id; });
    return ep ? *ep : nullptr;
}

Extern* Extern::LookupByImported (iid_t iid) noexcept // static
{
    auto ep = linear_search_if (ExternList(), [&](auto e)
		{ return e->Info().IsImporting(iid); });
    return ep ? *ep : nullptr;
}

Extern* Extern::LookupByRelayId (mrid_t rid) noexcept // static
{
    auto ep = linear_search_if (ExternList(), [&](auto e)
		{ return nullptr != e->RelayProxyById (rid); });
    return ep ? *ep : nullptr;
}

//}}}-------------------------------------------------------------------
//{{{ Extern::ExtMsg

Extern::ExtMsg::ExtMsg (Msg&& msg) noexcept
:_body (msg.MoveBody())
,_h { Align (_body.size(), Msg::BODY_ALIGNMENT)
    , msg.Extid()
    , msg.FdOffset()
    , WriteHeaderStrings (msg.Method()) }
{
    assert (_body.capacity() >= _h.sz && "message body must be created aligned to Msg::BODY_ALIGNMENT");
    _body.memlink::resize (_h.sz);
}

uint8_t Extern::ExtMsg::WriteHeaderStrings (methodid_t method) noexcept
{
    // _hbuf contains iface\0method\0signature\0, padded to Msg::HEADER_ALIGNMENT
    auto iface = InterfaceOfMethod (method);
    assert (ptrdiff_t(sizeof(_hbuf)) >= InterfaceNameSize(iface)+MethodNextOffset(method)-2 && "the interface and method names for this message are too long to export");
    ostream os (_hbuf, sizeof(_hbuf));
    os.write (iface, InterfaceNameSize(iface));
    os.write (method, MethodNextOffset(method)-2);
    os.align (Msg::HEADER_ALIGNMENT);
    return sizeof(_h) + distance (_hbuf, os.ptr());
}

void Extern::ExtMsg::WriteIOVecs (iovec* iov, streamsize bw) noexcept
{
    // Setup the two iovecs, 0 for header, 1 for body
    // bw is the bytes already written in previous sendmsg call
    auto hp = HeaderPtr();	// char* to full header
    auto hsz = _h.hsz + sizeof(_h)*!_h.hsz;
    if (bw < hsz) {	// still need to write header
	hsz -= bw;
	hp += bw;
	bw = 0;
    } else {		// header already written
	bw -= hsz;
	hp = nullptr;
	hsz = 0;
    }
    iov[0].iov_base = hp;
    iov[0].iov_len = hsz;
    iov[1].iov_base = _body.iat(bw);
    iov[1].iov_len = _h.sz - bw;
}

int Extern::ExtMsg::PassedFd (void) const noexcept
{
    if (!HasFd())
	return -1;
    istream fdis (_body.iat(_h.fdoffset), sizeof(int));
    return fdis.readv<int>();
}

methodid_t Extern::ExtMsg::ParseMethod (void) const noexcept
{
    streamsize ssz = _h.hsz-sizeof(_h);
    auto ifacename = _hbuf;
    auto methodname = strnext_r (ifacename, ssz);
    if (!ssz)
	return nullptr;
    auto signame = strnext_r (methodname, ssz);
    if (!ssz)
	return nullptr;
    auto methodend = strnext_r (signame, ssz);
    auto methodnamesz = methodend - methodname;
    auto iface = App::InterfaceByName (ifacename, distance(ifacename,methodname));
    if (!iface)
	return nullptr;
    return LookupInterfaceMethod (iface, methodname, methodnamesz);
}

void Extern::ExtMsg::DebugDump (void) const noexcept
{
    if (DEBUG_MSG_TRACE) {
	DEBUG_PRINTF ("[X] Message for extid %u of size %u completed:\n", _h.extid, _h.sz);
	hexdump (HeaderPtr(), _h.hsz);
	hexdump (_body.data(), _body.size());
    }
}

//}}}-------------------------------------------------------------------
//{{{ Extern::Extern

void Extern::Extern_Open (int fd, const iid_t* eifaces, bool isServer) noexcept
{
    if (!ValidateSocket (fd))
	return Error ("invalid socket type");
    _sockfd = fd;
    _einfo.exported = eifaces;
    _einfo.isServer = isServer;
    EnableCredentialsPassing (true);
    // Initial handshake is an exchange of COM::Export messages
    QueueOutgoing (PCOM::ExportMsg (extid_COM, eifaces));
}

void Extern::Extern_Close (void) noexcept
{
    auto fd = _sockfd;
    _sockfd = -1;
    close (fd);
    SetFlag (f_Unused);
}

bool Extern::ValidateSocket (int fd) noexcept
{
    // The incoming socket must be a stream socket
    int v;
    socklen_t l = sizeof(v);
    if (getsockopt (fd, SOL_SOCKET, SO_TYPE, &v, &l) < 0 || v != SOCK_STREAM)
	return false;

    // And it must match the family (PF_LOCAL or PF_INET)
    sockaddr_storage ss;
    l = sizeof(ss);
    if (getsockname (fd, (sockaddr*) &ss, &l) < 0)
	return false;
    _einfo.isUnixSocket = false;
    if (ss.ss_family == PF_LOCAL)
	_einfo.isUnixSocket = true;
    else if (ss.ss_family != PF_INET)
	return false;

    // If matches, need to set the fd nonblocking for the poll loop to work
    int f = fcntl (fd, F_GETFL);
    if (f < 0)
	return false;
    if (0 > fcntl (fd, F_SETFL, f| O_NONBLOCK| O_CLOEXEC))
	return false;
    return true;
}

void Extern::EnableCredentialsPassing (int enable) noexcept
{
    if (_sockfd < 0 || !_einfo.isUnixSocket)
	return;
    if (0 > setsockopt (_sockfd, SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable)))
	return ErrorLibc ("setsockopt(SO_PASSCRED)");
}

//}}}-------------------------------------------------------------------
//{{{ Extern::COM

void Extern::COM_Error (const lstring& errmsg) noexcept
{
    // Errors occuring on in the Extern Msger on the other side of the socket
    Error ("%s", errmsg.c_str());	// report it on this side
}

void Extern::COM_Export (string elist) noexcept
{
    // Other side of the socket listing exported interfaces as a comma-separated list
    _einfo.imported.clear();
    foreach (ei, elist) {
	auto eic = elist.find (',', ei);
	if (!eic)
	    eic = elist.end();
	*eic++ = 0;
	auto iid = App::InterfaceByName (ei, eic-ei);
	if (iid)	// _einfo.imported only contains interfaces supported by this App
	    _einfo.imported.push_back (iid);
	ei = eic;
    }
    _reply.Connected (&_einfo);
}

void Extern::COM_Delete (void) noexcept
{
    // This happens when the Extern Msger on the other side of the socket dies
    SetFlag (f_Unused);
}

//}}}-------------------------------------------------------------------
//{{{ Extern::Timer

void Extern::TimerR_Timer (PTimer::fd_t) noexcept
{
    if (_sockfd >= 0)
	ReadIncoming();
    auto tcmd = PTimer::WatchCmd::Read;
    if (_sockfd >= 0 && WriteOutgoing())
	tcmd = PTimer::WatchCmd::ReadWrite;
    if (_sockfd >= 0)
	_timer.Watch (tcmd, _sockfd);
}

//{{{2 WriteOutgoing ---------------------------------------------------

// Writes queued messages. Returns true if need to wait for write.
bool Extern::WriteOutgoing (void) noexcept
{
    // Write all queued messages
    while (!_outq.empty()) {
	// Build sendmsg header
	msghdr mh = {};

	// Add fd if being passed
	char fdbuf [CMSG_SPACE(sizeof(int))] = {};
	int passedfd = _outq.front().PassedFd();
	unsigned nm = (passedfd >= 0);
	if (nm && !_bwritten) {	// only the first write passes the fd
	    mh.msg_control = fdbuf;
	    mh.msg_controllen = sizeof(fdbuf);
	    cmsghdr* cmsg = CMSG_FIRSTHDR(&mh);
	    cmsg->cmsg_len = sizeof(fdbuf);
	    cmsg->cmsg_level = SOL_SOCKET;
	    cmsg->cmsg_type = SCM_RIGHTS;
	    ostream fdos ((ostream::pointer) CMSG_DATA (cmsg), sizeof(int));
	    fdos << passedfd;
	}

	// See how many messages can be written at once, limited by fd passing.
	// Can only pass one fd per sendmsg call, but can aggregate the rest.
	while (nm < _outq.size() && !_outq[nm].HasFd())
	    ++nm;

	// Create iovecs for output
	iovec iov [2*nm];	// two iovecs per message, header and body
	mh.msg_iov = iov;
	mh.msg_iovlen = 2*nm;
	for (auto m = 0u, bw = _bwritten; m < nm; ++m, bw = 0)
	    _outq[m].WriteIOVecs (&iov[2*m], bw);

	// And try writing it all
	int smr = sendmsg (_sockfd, &mh, MSG_NOSIGNAL);
	if (smr <= 0) {
	    if (!smr || errno == ECONNRESET)	// smr == 0 when remote end closes. No error then, just need to close this end too.
		DEBUG_PRINTF ("[X] %hu.Extern: wsocket %d closed by the other end\n", MsgerId(), _sockfd);
	    else {
		if (errno == EINTR)
		    continue;
		if (errno == EAGAIN)
		    return true;
		ErrorLibc ("sendmsg");
	    }
	    Extern_Close();
	    return false;
	}
	// At this point sendmsg has succeeded and wrote some bytes
	DEBUG_PRINTF ("[X] Wrote %d bytes to socket %d\n", smr, _sockfd);
	_bwritten += smr;

	// Close the fd once successfully passed
	if (passedfd >= 0)
	    close (passedfd);

	// Erase messages that have been fully written
	auto ndone = 0u;
	for (; ndone < nm && _bwritten >= _outq[ndone].Size(); ++ndone)
	    _bwritten -= _outq[ndone].Size();
	_outq.erase (_outq.begin(), ndone);

	assert (((_outq.empty() && !_bwritten) || (_bwritten < _outq.front().Size()))
		&& "_bwritten must now be equal to bytes written from first message in queue");
    }
    return false;
}

//}}}2------------------------------------------------------------------
//{{{2 ReadIncoming

void Extern::ReadIncoming (void) noexcept
{
    for (;;) {	// Read until EAGAIN
	// Create iovecs for input
	// There are three of them, representing the header and the body
	// of each message, plus the fixed header of the next. The common
	// case is to read the variable parts and the fixed header of the
	// next message in each recvmsg call.
	ExtMsg::Header fh = {};
	iovec iov[3] = {{},{},{&fh,sizeof(fh)}};
	_inmsg.WriteIOVecs (iov, _bread);

	// Ancillary space for fd and credentials
	char cmsgbuf [CMSG_SPACE(sizeof(int)) + CMSG_SPACE(sizeof(ucred))] = {};

	// Build struct for recvmsg
	msghdr mh = {};
	mh.msg_iov = iov;
	mh.msg_iovlen = 2 + (_bread >= sizeof(fh));	// read another header only when already have one
	mh.msg_control = cmsgbuf;
	mh.msg_controllen = sizeof(cmsgbuf);

	// Receive some data
	int rmr = recvmsg (_sockfd, &mh, 0);
	if (rmr <= 0) {
	    if (!rmr || errno == ECONNRESET)	// br == 0 when remote end closes. No error then, just need to close this end too.
		DEBUG_PRINTF ("[X] %hu.Extern: rsocket %d closed by the other end\n", MsgerId(), _sockfd);
	    else {
		if (errno == EINTR)
		    continue;
		if (errno == EAGAIN)
		    return;	// this is the exit point
		ErrorLibc ("recvmsg");
	    }
	    return Extern_Close();
	}
	DEBUG_PRINTF ("[X] %hu.Extern: read %d bytes from socket %d\n", MsgerId(), rmr, _sockfd);
	_bread += rmr;

	// Check if ancillary data was passed
	for (cmsghdr* cmsg = CMSG_FIRSTHDR(&mh); cmsg; cmsg = CMSG_NXTHDR(&mh, cmsg)) {
	    if (cmsg->cmsg_type == SCM_CREDENTIALS) {
		istream is ((istream::pointer) CMSG_DATA(cmsg), sizeof(ucred));
		is >> _einfo.creds;
		EnableCredentialsPassing (false);	// Credentials only need to be received once
		DEBUG_PRINTF ("[X] Received credentials: pid=%u,uid=%u,gid=%u\n", _einfo.creds.pid, _einfo.creds.uid, _einfo.creds.gid);
	    } else if (cmsg->cmsg_type == SCM_RIGHTS) {
		if (_infd >= 0) {	// if message is sent in multiple parts, the fd must only be sent with the first piece
		    Error ("multiple file descriptors received in one message");
		    return Extern_Close();
		}
		istream is ((istream::pointer) CMSG_DATA(cmsg), sizeof(int));
		is >> _infd;
		DEBUG_PRINTF ("[X] Received fd %d\n", _infd);
	    }
	}

	// If the read message is complete, validate it and queue for delivery
	if (_bread >= _inmsg.Size()) {
	    _bread -= _inmsg.Size();
	    _inmsg.DebugDump();

	    // Write the passed fd into the body
	    if (_inmsg.HasFd()) {
		assert (_infd >= 0 && "_infd magically disappeared since header check");
		_inmsg.SetPassedFd (_infd);
		_infd = -1;
	    }

	    if (!AcceptIncomingMessage()) {
		Error ("invalid message");
		return Extern_Close();
	    }

	    // Copy the fixed header of the next message
	    _inmsg.SetHeader (fh);
	    assert (_bread <= sizeof(fh) && "recvmsg read unrequested data");
	}
	// Here, a message has been accepted, or there was no message,
	// and there is a partially or fully read fixed header of another.

	// Now can check if fixed header is valid
	if (_bread == sizeof(fh)) {
	    auto& h = _inmsg.GetHeader();
	    if (h.hsz < ExtMsg::MIN_MSG_HEADER_SIZE
		    || !IsAligned (h.hsz, Msg::HEADER_ALIGNMENT)
		    || h.sz > ExtMsg::MAX_MSG_BODY_SIZE
		    || !IsAligned (h.sz, Msg::BODY_ALIGNMENT)
		    || (h.fdoffset != Msg::NO_FD_INCLUDED
			&& (_infd < 0	// the fd must be passed at this point
			    || h.fdoffset+sizeof(int) > h.sz
			    || !IsAligned (h.fdoffset, Msg::FD_ALIGNMENT)))
		    || h.extid > extid_ServerLast) {
		Error ("invalid message");
		return Extern_Close();
	    }
	    _inmsg.ResizeBody (h.sz);
	}
    }
}

bool Extern::AcceptIncomingMessage (void) noexcept
{
    // Validate the message using method signature
    auto method = _inmsg.ParseMethod();
    if (!method) {
	DEBUG_PRINTF ("[XE] Incoming message has invalid header strings\n");
	return false;
    }
    auto msgis = _inmsg.Read();
    auto vsz = Msg::ValidateSignature (msgis, SignatureOfMethod(method));
    if (Align (vsz, Msg::BODY_ALIGNMENT) != _inmsg.BodySize()) {
	DEBUG_PRINTF ("[XE] Incoming message body failed validation\n");
	return false;
    }
    _inmsg.TrimBody (vsz);	// Local messages store unpadded size

    // Lookup or create local relay proxy
    auto rp = RelayProxyByExtid (_inmsg.Extid());
    if (!rp) {
	// Verify that the requested interface is on the exported list
	if (!_einfo.IsExporting (InterfaceOfMethod (method))) {
	    DEBUG_PRINTF ("[XE] Incoming message requests unexported interface\n");
	    return false;
	}
	// Verify that the other side sets extid correctly
	if (_einfo.isServer ^ (_inmsg.Extid() < extid_ServerBase)) {
	    DEBUG_PRINTF ("[XE] Extern connection peer allocates incorrect extids\n");
	    return false;
	}
	DEBUG_PRINTF ("[X] Creating new extid link %hu\n", _inmsg.Extid());
	rp = &_relays.emplace_back (MsgerId(), mrid_New, _inmsg.Extid());
	//
	// Create a COMRelay as the destination. It will then create the
	// actual server Msger using the interface in the message.
	rp->relay.CreateDestAs (PCOM::Interface());
    }

    // Create local message from ExtMsg and forward it to the COMRelay
    rp->relay.Forward (Msg (rp->relay.Link(), method, _inmsg.MoveBody(), _inmsg.Extid(), _inmsg.FdOffset()));
    return true;
}
//}}}2
//}}}-------------------------------------------------------------------
//{{{ COMRelay

COMRelay::COMRelay (const Msg::Link& l) noexcept
: Msger (l)
//
// COMRelays can be created either by local callers sending messages to
// imported interfaces, or by an Extern delivering messages to local
// instances of exported interfaces.
//
,_pExtern (Extern::LookupById (l.src))
//
// Messages coming from an extern will require creating a local Msger,
// while messages going to the extern from l.src local caller.
//
,_localp (l.dest, _pExtern ? mrid_t(mrid_New) : l.src)
//
// Extid will be determined when the connection interface is known
,_extid()
{
}

COMRelay::~COMRelay (void) noexcept
{
    // The relay is destroyed when:
    // 1. The local Msger is destroyed. COM delete message is sent to the
    //    remote side as notification.
    // 2. The remote object is destroyed. The relay is marked unused in
    //    COMRelay_COM_Delete and the extern pointer is reset to prevent
    //    further messages to remote object. Here, no message is sent.
    // 3. The Extern object is destroyed. pExtern is reset in
    //    COMRelay_ObjectDestroyed, and no message is sent here.
    if (_pExtern) {
	if (_extid)
	    _pExtern->QueueOutgoing (PCOM::DeleteMsg (_extid));
	_pExtern->UnregisterRelay (this);
    }
    _pExtern = nullptr;
    _extid = 0;
}

bool COMRelay::Dispatch (Msg& msg) noexcept
{
    // COM messages are processed here
    if (PCOM::Dispatch (this, msg))
	return true;

    // Messages to imported interfaces need to be routed to the Extern
    // that imports it. The interface was unavailable in ctor, so here.
    if (!_pExtern) {	// If null here, then this relay was created by a local Msger
	auto iface = msg.Interface();
	if (!(_pExtern = Extern::LookupByImported (iface))) {
	    Error ("interface %s has not been imported", iface);
	    return false;	// the caller should have waited for Extern Connected reply before creating this
	}
    }
    // Now that the interface is known and extern pointer is available,
    // the relay can register and obtain a connection extid.
    if (!_extid)
	_extid = _pExtern->RegisterRelay (this);

    // Forward the message in the direction opposite which it was received
    if (msg.Src() == _localp.Dest()) {
	msg.SetExtid (_extid);
	_pExtern->QueueOutgoing (move(msg));
    } else {
	assert (msg.Extid() == _extid && "Extern routed a message to the wrong relay");
	_localp.Forward (move(msg));
    }
    return true;
}

bool COMRelay::OnError (mrid_t eid, const string& errmsg) noexcept
{
    // An unhandled error in the local object is forwarded to the remote
    // object. At this point it will be considered handled. The remote
    // will decide whether to delete itself, which will propagate here.
    if (_pExtern && eid == _localp.Dest()) {
	DEBUG_PRINTF ("[X] COMRelay forwarding error to extern creator\n");
	_pExtern->QueueOutgoing (PCOM::ErrorMsg (_extid, errmsg));
	return true;	// handled on the remote end.
    }
    // Errors occuring in the Extern object or elsewhere can not be handled
    // by forwarding, so fall back to default handling.
    return Msger::OnError (eid, errmsg);
}

void COMRelay::OnMsgerDestroyed (mrid_t id) noexcept
{
    // When the Extern object is destroyed, this notification arrives from
    // the App when the Extern created this relay. Relays created by local
    // Msgers will be manually notified by the Extern being deleted. In the
    // first case, the Extern object is available to send the COM Destroy
    // notification, in the second, it is not.
    if (id != _localp.Dest())
	_pExtern = nullptr;	// when not, do not try to send the message

    // In both cases, the relay can no longer function, and so is deleted
    SetFlag (f_Unused);
}

void COMRelay::COM_Error (const lstring& errmsg) noexcept
{
    // COM_Error is received for errors in the remote object. The remote
    // object is destroyed and COM_Delete will shortly follow. Here, create
    // a local error and send it to the local object.
    Error ("%s", errmsg.c_str());
    // Because the local object may not be the creator of this relay, the
    // error must be forwarded there manually.
    App::Instance().ForwardError (_localp.Dest(), _localp.Src());
}

void COMRelay::COM_Export (const lstring&) noexcept
{
    // Relays never receive this message
}

void COMRelay::COM_Delete (void) noexcept
{
    // COM_Delete indicates that the remote object has been destroyed.
    _pExtern = nullptr;	// No further messages are to be sent.
    _extid = 0;
    SetFlag (f_Unused);	// The relay and local object are to be destroyed.
}

//}}}-------------------------------------------------------------------
//{{{ PExternServer

PExternServer::~PExternServer (void) noexcept
{
    if (_sockname) {
	unlink (_sockname);
	free (_sockname);
	_sockname = nullptr;
    }
}

/// Create server socket bound to the given address
int PExternServer::Bind (const sockaddr* addr, socklen_t addrlen, const iid_t* eifaces) noexcept
{
    int fd = socket (addr->sa_family, SOCK_STREAM| SOCK_NONBLOCK| SOCK_CLOEXEC, IPPROTO_IP);
    if (fd < 0)
	return fd;
    if (0 > bind (fd, addr, addrlen) && errno != EINPROGRESS) {
	DEBUG_PRINTF ("[E] Failed to bind to socket: %s\n", strerror(errno));
	close (fd);
	return -1;
    }
    if (0 > listen (fd, SOMAXCONN)) {
	DEBUG_PRINTF ("[E] Failed to listen to socket: %s\n", strerror(errno));
	close (fd);
	return -1;
    }
    if (addr->sa_family == PF_LOCAL) {
	if (_sockname)
	    free (_sockname);
	_sockname = strdup (((const sockaddr_un*)addr)->sun_path);
    }
    Open (fd, eifaces, WhenEmpty::Remain);
    return fd;
}

/// Create local socket with given path
int PExternServer::BindLocal (const char* path, const iid_t* eifaces) noexcept
{
    sockaddr_un addr;
    addr.sun_family = PF_LOCAL;
    if ((int) ArraySize(addr.sun_path) <= snprintf (ArrayBlock(addr.sun_path), "%s", path)) {
	errno = ENAMETOOLONG;
	return -1;
    }
    DEBUG_PRINTF ("[X] Creating server socket %s\n", addr.sun_path);
    int fd = Bind ((const sockaddr*) &addr, sizeof(addr), eifaces);
    if (0 > chmod (addr.sun_path, DEFFILEMODE))
	DEBUG_PRINTF ("[E] Failed to change socket permissions: %s\n", strerror(errno));
    return fd;
}

/// Create local socket of the given name in the system standard location for such
int PExternServer::BindSystemLocal (const char* sockname, const iid_t* eifaces) noexcept
{
    sockaddr_un addr;
    addr.sun_family = PF_LOCAL;
    if ((int) ArraySize(addr.sun_path) <= snprintf (ArrayBlock(addr.sun_path), _PATH_VARRUN "%s", sockname)) {
	errno = ENAMETOOLONG;
	return -1;
    }
    DEBUG_PRINTF ("[X] Creating server socket %s\n", addr.sun_path);
    int fd = Bind ((const sockaddr*) &addr, sizeof(addr), eifaces);
    if (0 > chmod (addr.sun_path, DEFFILEMODE))
	DEBUG_PRINTF ("[E] Failed to change socket permissions: %s\n", strerror(errno));
    return fd;
}

/// Create local socket of the given name in the user standard location for such
int PExternServer::BindUserLocal (const char* sockname, const iid_t* eifaces) noexcept
{
    sockaddr_un addr;
    addr.sun_family = PF_LOCAL;
    const char* runtimeDir = getenv ("XDG_RUNTIME_DIR");
    if (!runtimeDir)
	runtimeDir = _PATH_TMP;
    if ((int) ArraySize(addr.sun_path) <= snprintf (ArrayBlock(addr.sun_path), "%s/%s", runtimeDir, sockname)) {
	errno = ENAMETOOLONG;
	return -1;
    }
    DEBUG_PRINTF ("[X] Creating server socket %s\n", addr.sun_path);
    int fd = Bind ((const sockaddr*) &addr, sizeof(addr), eifaces);
    if (0 > chmod (addr.sun_path, S_IRUSR| S_IWUSR))
	DEBUG_PRINTF ("[E] Failed to change socket permissions: %s\n", strerror(errno));
    return fd;
}

/// Create local IPv4 socket at given ip and port
int PExternServer::BindIP4 (in_addr_t ip, in_port_t port, const iid_t* eifaces) noexcept
{
    sockaddr_in addr = {};
    addr.sin_family = PF_INET,
    #ifdef UC_VERSION
	addr.sin_addr = ip;
    #else
	addr.sin_addr = { ip };
    #endif
    addr.sin_port = port;
    return Bind ((const sockaddr*) &addr, sizeof(addr), eifaces);
}

/// Create local IPv4 socket at given port on the loopback interface
int PExternServer::BindLocalIP4 (in_port_t port, const iid_t* eifaces) noexcept
    { return BindIP4 (INADDR_LOOPBACK, port, eifaces); }

/// Create local IPv6 socket at given ip and port
int PExternServer::BindIP6 (in6_addr ip, in_port_t port, const iid_t* eifaces) noexcept
{
    sockaddr_in6 addr = {};
    addr.sin6_family = PF_INET6;
    addr.sin6_addr = ip;
    addr.sin6_port = port;
    return Bind ((const sockaddr*) &addr, sizeof(addr), eifaces);
}

/// Create local IPv6 socket at given ip and port
int PExternServer::BindLocalIP6 (in_port_t port, const iid_t* eifaces) noexcept
{
    sockaddr_in6 addr = {};
    addr.sin6_family = PF_INET6;
    addr.sin6_addr = IN6ADDR_LOOPBACK_INIT;
    addr.sin6_port = port;
    return Bind ((const sockaddr*) &addr, sizeof(addr), eifaces);
}

//}}}-------------------------------------------------------------------
//{{{ ExternServer

ExternServer::ExternServer (const Msg::Link& l) noexcept
: Msger(l)
,_timer (l.dest)
,_reply (l)
,_sockfd (-1)
,_eifaces()
,_conns()
{
}

bool ExternServer::OnError (mrid_t eid, const string& errmsg) noexcept
{
    if (_timer.Dest() == eid || MsgerId() == eid)
	return false;	// errors in listened socket are fatal
    // Error in accepted socket. Handle by logging the error and removing record in ObjectDestroyed.
    syslog (LOG_ERR, "%s", errmsg.c_str());
    return true;
}

void ExternServer::OnMsgerDestroyed (mrid_t mid) noexcept
{
    DEBUG_PRINTF ("[X] Client connection %hu dropped\n", mid);
    remove_if (_conns, [&](const PExtern& e)
	{ return e.Dest() == mid; });
    if (_conns.empty() && Flag (f_CloseWhenEmpty))
	SetFlag (f_Unused);
}

bool ExternServer::Dispatch (Msg& msg) noexcept
{
    return PTimerR::Dispatch (this, msg)
	|| PExternServer::Dispatch (this, msg)
	|| PExternR::Dispatch (this, msg)
	|| Msger::Dispatch (msg);
}

void ExternServer::TimerR_Timer (PTimer::fd_t) noexcept
{
    for (int cfd; 0 <= (cfd = accept (_sockfd, nullptr, nullptr));) {
	DEBUG_PRINTF ("[X] Client connection accepted on fd %d\n", cfd);
	_conns.emplace_back (MsgerId()).Open (cfd, _eifaces);
    }
    if (errno == EAGAIN) {
	DEBUG_PRINTF ("[X] Resuming wait on fd %d\n", _sockfd);
	_timer.WaitRead (_sockfd);
    } else {
	DEBUG_PRINTF ("[X] Accept failed with error %s\n", strerror(errno));
	ErrorLibc ("accept");
    }
}

void ExternServer::ExternServer_Open (int fd, const iid_t* eifaces, PExternServer::WhenEmpty closeWhenEmpty) noexcept
{
    assert (_sockfd == -1 && "each ExternServer instance can only listen to one socket");
    if (0 > fcntl (fd, F_SETFL, O_NONBLOCK| fcntl (fd, F_GETFL)))
	return ErrorLibc ("fcntl(SETFL(O_NONBLOCK))");
    _sockfd = fd;
    _eifaces = eifaces;
    SetFlag (f_CloseWhenEmpty, closeWhenEmpty != PExternServer::WhenEmpty::Remain);
    TimerR_Timer (_sockfd);
}

void ExternServer::ExternServer_Close (void) noexcept
{
    SetFlag (f_Unused);
}

void ExternServer::ExternR_Connected (const ExternInfo* einfo) noexcept
{
    _reply.Connected (einfo);
}

} // namespace cwiclo
//}}}-------------------------------------------------------------------
