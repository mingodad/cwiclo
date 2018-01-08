// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "app.h"
#include <signal.h>
#include <sys/wait.h>

//{{{ App --------------------------------------------------------------
namespace cwiclo {

App*	App::s_App		= nullptr;	// static
int	App::s_LastSignal	= 0;		// static
int	App::s_LastChild	= 0;		// static
pid_t	App::s_LastChildStatus	= EXIT_SUCCESS;	// static
int	App::s_ExitCode		= EXIT_SUCCESS;	// static

App::App (void)
{
    assert (!s_App && "there must be only one App object");
    s_App = this;
    InstallSignalHandlers();
}

void App::ProcessArgs (argc_t, argv_t)
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
	printf ("[S] Error: %s\n", strsignal(sig));
	exit (qc_ShellSignalQuitOffset+sig);
    }
    _Exit (qc_ShellSignalQuitOffset+sig);
}

void App::MsgSignalHandler (int sig) // static
{
    s_LastSignal = sig;
    if (sig == SIGCHLD)
	s_LastChild = waitpid (-1, &s_LastChildStatus, WNOHANG);
}
#undef S

//}}}-------------------------------------------------------------------

} // namespace cwiclo
