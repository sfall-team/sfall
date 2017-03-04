#pragma once

#include "Module.h"

class BugFixes : public Module {
	const char* name() { return "BugFixes"; }
	void init();
};
