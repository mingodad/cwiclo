// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "string.h"
#include "set.h"
#include "msg.h"
#include <sys/poll.h>
#include <time.h>

//{{{ Timer interface --------------------------------------------------
namespace cwiclo {

enum ETimerWatchCmd : uint32_t {
    WATCH_STOP              = 0,
    WATCH_READ              = POLLIN,
    WATCH_WRITE             = POLLOUT,
    WATCH_RDWR              = WATCH_READ| WATCH_WRITE,
    WATCH_TIMER             = POLLMSG,
    WATCH_READ_TIMER        = WATCH_READ| WATCH_TIMER,
    WATCH_WRITE_TIMER       = WATCH_WRITE| WATCH_TIMER,
    WATCH_RDWR_TIMER        = WATCH_RDWR| WATCH_TIMER
};
enum {
    TIMER_MAX = INT64_MAX,
    TIMER_NONE = UINT64_MAX
};

//----------------------------------------------------------------------

struct ITimer : public Interface {
    enum : imethod_t { imethod_Watch };
    using mstime_t = uint64_t;
    template <typename O>
    bool Dispatch (O* o, const Msg& msg) const {
	if (msg.InterfaceName() != this->name || msg.IMethod() != imethod_Watch)
	    return false;
	auto is = msg.Read();
	auto cmd = is.readv<ETimerWatchCmd>();
	auto fd = is.readv<int>();
	auto timer = is.readv<mstime_t>();
	o->Timer_Watch (cmd, fd, timer);
	return true;
    }
    static mstime_t Now (void) noexcept;
};
extern const ITimer i_Timer;

//----------------------------------------------------------------------

class PTimer : public Proxy {
public:
    using mstime_t = ITimer::mstime_t;
public:
    explicit	PTimer (mrid_t caller) : Proxy (caller) {}
    void	Watch (ETimerWatchCmd cmd, int fd, mstime_t timeoutms)
		    { Send (&i_Timer, ITimer::imethod_Watch, cmd, fd, timeoutms); }
    void	Stop (void)					{ Watch (WATCH_STOP, -1, TIMER_NONE); }
    void	Timer (mstime_t timeoutms)			{ Watch (WATCH_TIMER, -1, timeoutms); }
    void	WaitRead (int fd, mstime_t t = TIMER_NONE)	{ Watch (WATCH_READ, fd, t); }
    void	WaitWrite (int fd, mstime_t t = TIMER_NONE)	{ Watch (WATCH_WRITE, fd, t); }
    void	WaitRdWr (int fd, mstime_t t = TIMER_NONE)	{ Watch (WATCH_RDWR, fd, t); }
};

//----------------------------------------------------------------------

struct ITimerR : public Interface {
    enum : imethod_t { imethod_Timer };
    using mstime_t = uint64_t;
    template <typename O>
    bool Dispatch (O* o, const Msg& msg) const {
	if (msg.InterfaceName() != this->name || msg.IMethod() != imethod_Timer)
	    return false;
	o->TimerR_Timer (msg.Read().readv<int>());
	return true;
    }
};
extern const ITimerR i_TimerR;

//----------------------------------------------------------------------

class PTimerR : public ProxyR {
public:
    explicit	PTimerR (const Msg::Link& l) : ProxyR (l) {}
    void	Timer (int fd) { Send (&i_TimerR, ITimerR::imethod_Timer, fd); }
};

//}}}-------------------------------------------------------------------
//{{{ App

class App : public Msger {
public:
    using argc_t	= int;
    using argv_t	= char* const*;
    using mstime_t	= ITimer::mstime_t;
    using msgq_t	= vector<Msg>;
    enum { f_Quitting = Msger::f_Last, f_DebugMsgTrace, f_Last };
public:
    static auto&	Instance (void)			{ return *s_App; }
    inline void		ProcessArgs (argc_t, argv_t)	{ }
    int			Run (void) noexcept;
    bool		ValidMsgerId (mrid_t id) const	{ return id >= mrid_First && id <= _msgers.size(); }
    Msg::Link&		CreateLink (Msg::Link& l, iid_t iid) noexcept;
    Msg&		CreateMsg (Msg::Link& l, iid_t iid, imethod_t imethod, streamsize size, mrid_t extid = 0, Msg::fdoffset_t fdo = Msg::NO_FD_IN_MESSAGE)
			    { return _outq.emplace_back (CreateLink(l,iid),iid,imethod,size,extid,fdo); }
    msgq_t::size_type	HasMessagesFor (mrid_t mid) const noexcept;
    inline void		Quit (int ec = EXIT_SUCCESS)	{ s_ExitCode = ec; SetFlag (f_Quitting); }
    void		DeleteUnusedMsgers (void) noexcept;
    void		DeleteMsger (mrid_t mid) noexcept;
    void		RunTimers (void) noexcept;
    unsigned		GetPollTimerList (pollfd* pfd, unsigned pfdsz, int& timeout) const noexcept;
    void		CheckPollTimers (const pollfd* fds) noexcept;
protected:
			App (void);
    virtual		~App (void) noexcept;
    static void		FatalSignalHandler (int sig);
    static void		MsgSignalHandler (int sig);
private:
    //{{{2 Msgerp ------------------------------------------------------
    struct Msgerp {
	using element_type = Msger;
	using pointer = element_type*;
	using const_pointer = const element_type*;
	using reference = element_type&;
	enum { SLOT_IN_USE = 1 };
    public:
	constexpr		Msgerp (pointer p)	: _p(p) {}
				Msgerp (Msgerp&& v)	: _p(v.release()) {}
				Msgerp (const Msgerp&) = delete;
				~Msgerp (void) noexcept	{ reset(); }
	constexpr pointer	get (void) const	{ return _p; }
	pointer			release (void)		{ auto r(_p); _p = nullptr; return r; }
	bool			created (void) const	{ return SLOT_IN_USE < uintptr_t(_p); }
	void			reset (pointer p = nullptr) {
				    auto q(_p);
				    _p = p;
				    if (SLOT_IN_USE < uintptr_t(q) && !q->Flag(f_Static))
					delete q;
				}
	void			destroy (void)		{ reset (pointer(uintptr_t(SLOT_IN_USE))); }
	void			swap (Msgerp&& v)	{ ::cwiclo::swap (_p, v._p); }
	auto&			operator= (pointer p)	{ reset (p); return *this; }
	auto&			operator= (Msgerp&& p)	{ reset (p.release()); return *this; }
	constexpr auto&		operator* (void) const	{ return *get(); }
	constexpr pointer	operator-> (void) const	{ return get(); }
	void			operator= (const Msgerp&) = delete;
	constexpr bool		operator== (const_pointer p) const	{ return _p == p; }
	bool			operator== (mrid_t id) const		{ return _p->MsgerId() == id; }
	bool			operator< (mrid_t id) const		{ return _p->MsgerId() < id; }
	bool			operator== (const Msgerp& p) const	{ return *this == p->MsgerId(); }
	bool			operator< (const Msgerp& p) const	{ return *this < p->MsgerId(); }
    private:
	pointer		_p;
    };
    //}}}2--------------------------------------------------------------
    //{{{2 Msger factory templates
    template <typename M, bool NamedCreate>
    struct __MsgerFactory { static Msger* create (const Msg::Link& l) { return new M(l); } };
    template <typename M>	// this variant is used for singleton Msgers
    struct __MsgerFactory<M,true> { static Msger* create (const Msg::Link& l) { return M::Create(l); } };

    using pfn_msger_factory = Msger* (*)(const Msg::Link& l);
    template <typename M>
    static Msger* MsgerFactory (const Msg::Link& l)
	{ return __MsgerFactory<M,has_msger_named_create<M>::value>::create(l); }

    // Maps a factory to an interface
    struct MsgerImplements {
	iid_t			iface;
	pfn_msger_factory	factory;
    };
    //}}}2--------------------------------------------------------------
public:
    //{{{2 Timer
    friend class Timer;
    class Timer : public Msger {
    public:
			Timer (const Msg::Link& l) : Msger(l),_nextfire(),_reply(l),_cmd(),_fd(-1) { App::Instance().AddTimer (this); }
			~Timer (void) noexcept	{ App::Instance().RemoveTimer (this); }
	virtual bool	Dispatch (const Msg& msg) noexcept { return i_Timer.Dispatch(this,msg) || Msger::Dispatch(msg); }
	void		Timer_Watch (ETimerWatchCmd cmd, int fd, mstime_t timeoutms)
			    { _cmd = cmd; _fd = fd; _nextfire = timeoutms + (timeoutms <= TIMER_MAX ? ITimer::Now() : 0); }
	void		Stop (void)		{ SetFlag (f_Unused); _cmd = WATCH_STOP; _fd = -1; _nextfire = TIMER_NONE; }
	void		Fire (void)		{ _reply.Timer (_fd); Stop(); }
	auto		Fd (void) const		{ return _fd; }
	auto		Cmd (void) const	{ return _cmd; }
	auto		NextFire (void) const	{ return _nextfire; }
	auto		PollMask (void) const	{ return _cmd; }
    public:
	ITimer::mstime_t	_nextfire;
	PTimerR		_reply;
	ETimerWatchCmd	_cmd;
	int		_fd;
    };
    //}}}2--------------------------------------------------------------
private:
    inline void		InstallSignalHandlers (void);
    mrid_t		AllocateMrid (void) noexcept;
    auto		MridFromIndex (mrid_t idx) const	{ return idx + mrid_First; }
    auto		IndexFromMrid (mrid_t id) const		{ return id - mrid_First; }
    auto&		MsgerpById (mrid_t id)	{ return _msgers[IndexFromMrid(id)]; }
    pfn_msger_factory	MsgerFactoryFor (iid_t id) {
			    for (auto mii = s_MsgerImpls; mii->iface; ++mii)
				if (mii->iface == id)
				    return mii->factory;
			    return nullptr;
			}
    Msger*		CreateMsger (const Msg::Link& l, iid_t iid) noexcept;
    void		AddTimer (Timer* t)	{ _timers.push_back(t); }
    void		RemoveTimer (Timer* t)	{ foreach (i, _timers) if (*i == t) --(i=_timers.erase(i)); }
private:
    msgq_t		_outq;
    msgq_t		_inq;
    vector<Msgerp>	_msgers;
    vector<Timer*>	_timers;
    static App*		s_App;
    static const MsgerImplements s_MsgerImpls[];
    static int		s_ExitCode;
    static int		s_LastSignal;
    static int		s_LastChildStatus;
    static pid_t	s_LastChild;
};

//}}}-------------------------------------------------------------------
//{{{ main template

template <typename A>
inline int Tmain (typename A::argc_t argc, typename A::argv_t argv)
{
    auto& a = A::Instance();
    a.ProcessArgs (argc, argv);
    return a.Run();
}
#define CwicloMain(A)	\
int main (A::argc_t argc, A::argv_t argv) \
    { return Tmain<A> (argc, argv); }

#define BEGIN_MSGERS	const App::MsgerImplements App::s_MsgerImpls[] = {
#define REGISTER_MSGER(iface,mgtype)	{ &iface, &MsgerFactory<mgtype> },
#define END_MSGERS	{nullptr,nullptr}};

#define BEGIN_CWICLO_APP(A)	\
    CwicloMain(A)		\
    BEGIN_MSGERS
#define END_CWICLO_APP		END_MSGERS

//}}}-------------------------------------------------------------------
//{{{ Debugging macros

#ifndef NDEBUG
    #define DEBUG_MSG_TRACE	App::Instance().Flag(App::f_DebugMsgTrace)
    #define DEBUG_PRINTF(...)	do { if (DEBUG_MSG_TRACE) printf (__VA_ARGS__); } while (false)
#else
    #define DEBUG_MSG_TRACE	false
    #define DEBUG_PRINTF(...)	do {} while (false)
#endif

} // namespace cwiclo
//}}}-------------------------------------------------------------------
