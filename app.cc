// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "app.h"
#include <signal.h>
#include <sys/wait.h>

//{{{ Timer and Signal interfaces --------------------------------------
namespace cwiclo {

DEFINE_INTERFACE (Timer)
DEFINE_INTERFACE (TimerR)
DEFINE_INTERFACE (Signal)

auto PTimer::Now (void) noexcept -> mstime_t
{
    struct timespec t;
    if (0 > clock_gettime (CLOCK_REALTIME, &t))
	return 0;
    return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

//}}}-------------------------------------------------------------------
//{{{ App

App*	App::s_App		= nullptr;	// static
int	App::s_ExitCode		= EXIT_SUCCESS;	// static
uint32_t App::s_ReceivedSignals	= 0;		// static

App::App (void)
: Msger (mrid_App)
,_outq()
,_inq()
,_msgers()
{
    assert (!s_App && "there must be only one App object");
    s_App = this;
    InstallSignalHandlers();
    _msgers.emplace_back (this);
}

App::~App (void) noexcept
{
}

//}}}-------------------------------------------------------------------
//{{{ Signal handling

#define S(s) (1<<(s))
enum {
    sigset_Die	= S(SIGILL)|S(SIGABRT)|S(SIGBUS)|S(SIGFPE)|S(SIGSYS)|S(SIGSEGV)|S(SIGALRM)|S(SIGXCPU),
    sigset_Quit	= S(SIGINT)|S(SIGQUIT)|S(SIGTERM)|S(SIGPWR),
    sigset_Msg	= sigset_Quit|S(SIGHUP)|S(SIGCHLD)|S(SIGWINCH)|S(SIGURG)|S(SIGXFSZ)|S(SIGUSR1)|S(SIGUSR2)|S(SIGPIPE)
};
enum { qc_ShellSignalQuitOffset = 128 };

void App::InstallSignalHandlers (void)
{
    for (auto sig = 0u; sig < NSIG; ++sig) {
	if (sigset_Msg & S(sig))
	    signal (sig, MsgSignalHandler);
	else if (sigset_Die & S(sig))
	    signal (sig, FatalSignalHandler);
    }
}

void App::FatalSignalHandler (int sig) // static
{
    static atomic_flag doubleSignal = ATOMIC_FLAG_INIT;
    if (!doubleSignal.test_and_set()) {
        #if HAVE_STRSIGNAL
	    printf ("[S] Error: %s\n", strsignal(sig));
	#else
	    printf ("[S] Error: %d\n", sig);
	#endif
	#ifndef NDEBUG
	    print_backtrace();
	#endif
	exit (qc_ShellSignalQuitOffset+sig);
    }
    _Exit (qc_ShellSignalQuitOffset+sig);
}

void App::MsgSignalHandler (int sig) // static
{
    s_ReceivedSignals |= S(sig);
}

//}}}-------------------------------------------------------------------
//{{{ Msger lifecycle

mrid_t App::AllocateMrid (void) noexcept
{
    mrid_t id = 0;
    for (; id < _msgers.size(); ++id)
	if (_msgers[id] == nullptr)
	    return id;
    _msgers.emplace_back (nullptr);
    return id;
}

Msger* App::CreateMsger (const Msg::Link& l, iid_t iid) noexcept
{
    auto fac = MsgerFactoryFor (iid);
    Msger* r = nullptr;
    if (fac)
	r = (*fac)(l);
    #ifndef NDEBUG	// Log failure to create in debug mode
	if (!r && (!iid || !iid[0] || iid[iid[-1]-1] != 'R')) { // reply messages do not recreate dead Msgers
	    if (!fac) {
		DEBUG_PRINTF ("Error: no factory registered for interface %s\n", iid ? iid : "(iid_null)");
		assert (!"Unable to find factory for the given interface. You must register a Msger for every interface you use using REGISTER_MSGER in the BEGIN_MSGER/END_MSGER block.");
	    } else {
		DEBUG_PRINTF ("Error: failed to create Msger for interface %s\n", iid ? iid : "(iid_null)");
		assert (!"Failed to create Msger for the given destination. Msger constructors are not allowed to fail or throw.");
	    }
	}
    #endif
    return r;
}

Msg::Link& App::CreateLink (Msg::Link& l, iid_t iid) noexcept
{
    assert (l.src != mrid_New && "You may only create links originating from an existing Msger");
    assert ((l.dest == mrid_New || l.dest == mrid_Broadcast || ValidMsgerId(l.dest)) && "Invalid link destination requested");
    if (l.dest == mrid_Broadcast)
	return l;
    if (l.dest == mrid_New)
	l.dest = AllocateMrid();
    auto& mrp = MsgerpById (l.dest);
    if (!mrp.created())
	mrp = CreateMsger (l, iid);
    return l;
}

void App::DeleteMsger (mrid_t mid) noexcept
{
    auto& m = _msgers[mid];
    if (!m.created())
	return;
    // Notify connected Msgers of this one's destruction
    for (auto& u : _msgers)
	if (u.created() && u->MsgerId() != m->MsgerId() && (u->CreatorId() == m->MsgerId() || u->MsgerId() == m->CreatorId()))
	    u->OnMsgerDestroyed (m->MsgerId());	// this may mark u unused
    m.destroy();
    // Destroying does release the mrid slot; that must be done explicitly.
}

void App::DeleteUnusedMsgers (void) noexcept
{
    // A Msger is unused if it has f_Unused flag set and has no pending messages in _outq
    for (auto& m : _msgers)
	if (m.created() && m->Flag(f_Unused) && !HasMessagesFor (m->MsgerId()))
	    DeleteMsger (m->MsgerId());
}

//}}}-------------------------------------------------------------------
//{{{ Message loop

void App::ProcessMessageQueue (void) noexcept
{
    // Setup the queues
    _inq.clear();		// input queue was processed on the last iteration
    _inq.swap (move(_outq));	// output queue now becomes the input queue
    if (_inq.empty()) {
	DEBUG_PRINTF ("Warning: ran out of packets. Quitting.\n");
	SetFlag (f_Quitting);	// running out of packets is usually not what you want, but not exactly an error
    }

    // Dispatch all messages in the input queue
    for (auto& msg : _inq) {
	// Dump the message if tracing
	if (DEBUG_MSG_TRACE) {
	    DEBUG_PRINTF ("Msg: %hu -> %hu.%s.%s [%u] = {""{{\n", msg.Src(), msg.Dest(), msg.Interface(), msg.Method(), msg.Size());
	    auto msgbody = msg.Read();
	    hexdump (msgbody.ptr<char>(), msgbody.remaining());
	    DEBUG_PRINTF ("}""}}\n");
	}

	// Create the dispatch range. Broadcast messages go to all, the rest go to one.
	auto mg = 0u, mgend = _msgers.size();
	if (msg.Dest() != mrid_Broadcast) {
	    if (!ValidMsgerId (msg.Dest())) {
		DEBUG_PRINTF ("Error: invalid message destination %hu. Ignoring message.\n", msg.Dest());
		continue;
	    }
	    mg = msg.Dest();
	    mgend = mg+1;
	}
	for (; mg < mgend; ++mg) {
	    auto& msger = _msgers[mg];
	    if (!msger.created())
		continue; // errors for msger creation failures were reported in CreateMsger; here just try to continue

	    auto accepted = msger->Dispatch(msg);

	    if (!accepted && msg.Dest() != mrid_Broadcast)
		DEBUG_PRINTF ("Error: message delivered, but not accepted by the destination Msger. Did you forget to add the interface to the Dispatch override?\n");
	}
    }
    // End-of-iteration housekeeping
    DeleteUnusedMsgers();
    ForwardReceivedSignals();
}

void App::ForwardReceivedSignals (void) noexcept
{
    auto oldrs = s_ReceivedSignals;
    if (!oldrs)
	return;
    PSignal psig (mrid_App);
    for (auto i = 0u; i < sizeof(s_ReceivedSignals)*8; ++i)
	if (oldrs & S(i))
	    psig.Signal (i);
    // clear only the signal bits processed, in case new signals arrived during the loop
    s_ReceivedSignals ^= oldrs;
}

App::msgq_t::size_type App::HasMessagesFor (mrid_t mid) const noexcept
{
    App::msgq_t::size_type n = 0;
    for (auto& msg : _outq)
	if (msg.Dest() == mid)
	    ++n;
    return n;
}

//}}}-------------------------------------------------------------------
//{{{ Timers

void App::RunTimers (void) noexcept
{
    if (_timers.empty())
	return;

    // Populate the fd list and find the nearest timer
    pollfd fds [_timers.size()];
    int timeout;
    auto nfds = GetPollTimerList (fds, _timers.size(), timeout);
    if (!nfds && !timeout)
	return;
    if (!_outq.empty())	// do not wait if there are messages to process
	timeout = 0;

    // And wait
    if (DEBUG_MSG_TRACE) {
	if (timeout > 0)
	    DEBUG_PRINTF ("[I] Waiting for %d ms ", timeout);
	else if (timeout < 0)
	    DEBUG_PRINTF ("[I] Waiting indefinitely ");
	else if (!timeout)
	    DEBUG_PRINTF ("[I] Checking ");
	DEBUG_PRINTF ("%u file descriptors from %u timers", nfds, _timers.size());
    }

    // And poll
    poll (fds, nfds, timeout);

    // Then, check timers for expiration
    CheckPollTimers (fds);
}

unsigned App::GetPollTimerList (pollfd* pfd, unsigned pfdsz, int& timeout) const noexcept
{
    // Put all valid fds into the pfd list and calculate the nearest timeout
    // Note that there may be a timeout without any fds
    //
    auto npfd = 0u;
    PTimer::mstime_t nearest = PTimer::TIMER_MAX;
    for (auto t : _timers) {
	nearest = min (nearest, t->NextFire());
	if (t->Fd() >= 0 && t->Cmd() != PTimer::WATCH_STOP) {
	    if (npfd >= pfdsz)
		break;
	    pfd[npfd].fd = t->Fd();
	    pfd[npfd].events = t->Cmd();
	    pfd[npfd++].revents = 0;
	}
    }
    if (nearest == PTimer::TIMER_MAX)	// Wait indefinitely
	timeout = -!!npfd;	// If no fds, then don't wait at all
    else // get current time and compute timeout to nearest
	timeout = max (nearest - PTimer::Now(), 0);
    return npfd;
}

void App::CheckPollTimers (const pollfd* fds) noexcept
{
    // Poll errors are checked for each fd with POLLERR. Other errors are ignored.
    // poll will exit when there are fds available or when the timer expires
    auto now = PTimer::Now();
    const auto* cfd = fds;
    for (auto t : _timers) {
	bool timerExpired = t->NextFire() <= now,
	    hasFd = (t->Fd() >= 0 && t->Cmd() != PTimer::WATCH_STOP),
	    fdFired = hasFd && (cfd->revents & (POLLERR| t->Cmd()));

	// Log the firing if tracing
	if (DEBUG_MSG_TRACE) {
	    if (timerExpired)
		DEBUG_PRINTF("[T]\tTimer %lu fired at %lu\n", t->NextFire(), now);
	    if (fdFired) {
		DEBUG_PRINTF("[T]\tFile descriptor %d ", cfd->fd);
		if (cfd->revents & POLLIN)	DEBUG_PRINTF("can be read\n");
		if (cfd->revents & POLLOUT)	DEBUG_PRINTF("can be written\n");
		if (cfd->revents & POLLMSG)	DEBUG_PRINTF("has extra data\n");
		if (cfd->revents & POLLERR)	DEBUG_PRINTF("has errors\n");
	    }
	}

	// Firing the timer will remove it (on next idle)
	if (timerExpired || fdFired)
	    t->Fire();
	cfd += hasFd;
    }
}

} // namespace cwiclo
//}}}-------------------------------------------------------------------
