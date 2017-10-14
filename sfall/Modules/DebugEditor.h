#pragma once

namespace sfall
{

class DebugEditor : public Module {
public:
	const char* name() { return "Debug Editor"; }
	void init();
};

}
