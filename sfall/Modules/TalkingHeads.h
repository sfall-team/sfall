#pragma once

namespace sfall
{

class TalkingHeads {
public:
	static const char* name() { return "TalkingHeads"; }
	static void init();
	static void exit();

	static void OnAfterGameInit();

	static bool Use32Bit;
};

}
