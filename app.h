// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "string.h"
#include "set.h"

namespace cwiclo {

class App {
public:
    using argc_t	= int;
    using argv_t	= char* const*;
public:
    static auto&	Instance (void)	{ return *s_App; }
    void		ProcessArgs (argc_t argc, argv_t argv);
    int			Run (void)	{ return s_ExitCode; }
protected:
			App (void);
    static void		FatalSignalHandler (int sig);
    static void		MsgSignalHandler (int sig);
private:
    inline void		InstallSignalHandlers (void);
private:
    static App*		s_App;
    static int		s_ExitCode;
    static int		s_LastSignal;
    static int		s_LastChildStatus;
    static pid_t	s_LastChild;
};

//----------------------------------------------------------------------

template <typename App>
inline int Tmain (typename App::argc_t argc, typename App::argv_t argv)
{
    auto& app = App::Instance();
    app.ProcessArgs (argc, argv);
    return app.Run();
}
#define CwicloMain(App)	\
int main (App::argc_t argc, App::argv_t argv) \
    { return Tmain<App> (argc, argv); }

} // namespace cwiclo
