#pragma once
#include "Module.h"

class Input : public Module {
public:
	const char* name() { return "InputPatch"; }
	void init();
};
