#pragma once

#include "Module.h"

namespace sfall
{

class Input : public Module {
public:
	const char* name() { return "InputPatch"; }
	void init();
};

}
