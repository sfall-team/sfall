#pragma once

namespace sfall
{

class MainLoopHook {
public:
	static const char* name() { return "MainLoopHook"; }
	static void init();

	static bool displayWinUpdateState;
};

}
