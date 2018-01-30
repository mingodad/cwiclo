// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "ping.h"

class TestApp : public App {
public:
    static auto&	Instance (void) { static TestApp s_App; return s_App; }
    virtual bool	Dispatch (const Msg& msg) noexcept override
			    { return i_PingR.Dispatch(this,msg) || App::Dispatch(msg); }
    void		PingR_Ping (uint32_t v) {
			    LOG ("Ping %u reply received in app\n", v);
			    if (++v < 5)
				_pinger.Ping (v);
			    else
				Quit();
			}
private:
			TestApp (void)
			    : App(), _pinger(MsgerId())
			    { _pinger.Ping(1); }
private:
    PPing		_pinger;
};

BEGIN_CWICLO_APP (TestApp)
    REGISTER_MSGER (i_Ping, PingMsger)
END_CWICLO_APP
